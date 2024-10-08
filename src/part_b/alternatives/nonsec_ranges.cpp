/**
 * This program encrypts and decrypts a message loaded from a file using DES and an arbitrary key, using MPI for parallel processing with non sequential key ranges.
 * 
 * Programación Paralela y Distribuida
 * Andrés Montoya - 21552
 * Francisco Castillo - 21562
 * Fernanda Esquivel - 21542
 */

#include <cstdint>
#include <openssl/des.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#define BLOCK_SIZE 8  // DES operates on 8-byte blocks

// Function implementations for DES encryption/decryption
void decryptBlock(uint64_t key, unsigned char* ciph)
{
    DES_cblock keyBlock;
    DES_key_schedule schedule;
    key &= 0xFFFFFFFFFFFFFF;  // Make the key be 56 bits
    memcpy(&keyBlock, &key, sizeof(keyBlock));
    DES_set_odd_parity(&keyBlock);
    if (DES_set_key_checked(&keyBlock, &schedule) != 0)
    {
        return;
    }
    DES_ecb_encrypt((DES_cblock*)ciph, (DES_cblock*)ciph, &schedule, DES_DECRYPT);
}

void encryptBlock(uint64_t key, unsigned char* ciph)
{
    DES_cblock keyBlock;
    DES_key_schedule schedule;
    key &= 0xFFFFFFFFFFFFFF;
    memcpy(&keyBlock, &key, sizeof(keyBlock));
    DES_set_odd_parity(&keyBlock);
    if (DES_set_key_checked(&keyBlock, &schedule) != 0)
        return;
    DES_ecb_encrypt((DES_cblock*)ciph, (DES_cblock*)ciph, &schedule, DES_ENCRYPT);
}

int readFile(const char* filename, unsigned char** buffer)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("Error opening file.\n");
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    *buffer = (unsigned char*)malloc(file_size + 1);
    fread(*buffer, 1, file_size, file);
    (*buffer)[file_size] = '\0'; // Ensure null-terminated string
    fclose(file);
    return file_size;
}

void encryptMessage(uint64_t key, unsigned char* ciph, int len)
{
    for (int i = 0; i < len; i += BLOCK_SIZE)
        encryptBlock(key, ciph + i);
}

void decryptMessage(uint64_t key, unsigned char* ciph, int len)
{
    for (int i = 0; i < len; i += BLOCK_SIZE)
        decryptBlock(key, ciph + i);
}

unsigned char* addPadding(unsigned char* message, int len, int* new_len)
{
    int padding_needed = BLOCK_SIZE - (len % BLOCK_SIZE);
    *new_len = len + padding_needed;
    unsigned char* padded_message = (unsigned char*)malloc(*new_len);
    memcpy(padded_message, message, len);
    for (int i = len; i < *new_len; ++i)
    {
        padded_message[i] = padding_needed;
    }
    return padded_message;
}

void removePadding(unsigned char* message, int* len)
{
    int padding_value = message[*len - 1];
    *len -= padding_value;
}

int searchKeyword(const char* decrypted, const char* keyword)
{
    return strstr(decrypted, keyword) != NULL;
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv); // Initialize MPI environment
    int numprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs); // Get number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get process rank

    const char* keyword = "es una prueba de";

    if (argc != 3)
    {
        if (rank == 0)
            printf("Usage: %s <key> <file>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    uint64_t original_key = strtoull(argv[1], NULL, 10);

    unsigned char* plaintext = NULL;
    int len;
    if (rank == 0)
        len = readFile(argv[2], &plaintext);

    MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0)
        plaintext = (unsigned char*)malloc(len);

    MPI_Bcast(plaintext, len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    int padded_len;
    unsigned char* padded_message = addPadding(plaintext, len, &padded_len);

    unsigned char* cipher = (unsigned char*)malloc(padded_len + 1);
    memcpy(cipher, padded_message, padded_len);

    clock_t start_time, end_time;
    double encrypt_time, decrypt_time;

    start_time = clock();
    encryptMessage(original_key, cipher, padded_len);
    end_time = clock();
    encrypt_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    if (rank == 0)
    {
        printf("Encrypted message:\n\t");
        for (int i = 0; i < padded_len; ++i)
            printf("%c", cipher[i]);
        printf("\nEncryption completed in %.6f seconds\n", encrypt_time);
    }

    unsigned char* brute_force_attempt = (unsigned char*)malloc(padded_len);

    uint64_t key_space_size = (1ULL << 56);  // 56 bits key space
    uint64_t found_key = -1;
    int found_flag = 0;
    int global_found_flag = 0;

    start_time = clock();
    for (uint64_t current_key = rank; current_key < key_space_size; current_key += numprocs)
    {
        memcpy(brute_force_attempt, cipher, padded_len);
        decryptMessage(current_key, brute_force_attempt, padded_len);

        int decrypted_len = padded_len;
        removePadding(brute_force_attempt, &decrypted_len);

        if (searchKeyword((char*)brute_force_attempt, keyword))
        {
            found_key = current_key;
            found_flag = 1;
            printf("\tProcess %d found key: %ld\n", rank, found_key);
        }

        // Broadcast the found flag and check for global termination
        MPI_Allreduce(&found_flag, &global_found_flag, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

        if (global_found_flag)
        {
            break;  // Exit the loop if any process found the key
        }
    }

    // Broadcast the found key to all processes
    MPI_Bcast(&found_key, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    end_time = clock();
    decrypt_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    if (rank == 0)
    {
        printf("\nDecryption completed in %.6f seconds\n", decrypt_time);

        if (found_key != -1)
        {
            printf("\tCorrect decryption key found: %ld\n", found_key);

            // Desencriptar el mensaje con la clave encontrada
            memcpy(brute_force_attempt, cipher, padded_len);
            decryptMessage(found_key, brute_force_attempt, padded_len);
            removePadding(brute_force_attempt, &padded_len);

            // Imprimir el mensaje desencriptado
            printf("\nDecrypted message: %.*s\n", padded_len, brute_force_attempt);

            if (searchKeyword((char*)brute_force_attempt, keyword))
                printf("Keyword '%s' found in the decrypted message.\n", keyword);
            else
                printf("Keyword '%s' NOT found in the decrypted message.\n");
        }
        else
        {
            printf("Decryption key not found.\n");
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    free(plaintext);
    free(padded_message);
    free(cipher);
    free(brute_force_attempt);

    MPI_Finalize();

    return 0;
}
