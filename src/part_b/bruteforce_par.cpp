#include <cstdint>
#include <openssl/des.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#define BLOCK_SIZE 8  // DES operates on 8-byte blocks

/**
 * Decrypts a block using the DES algorithm.
 * @param key The key to use for decryption.
 * @param ciph The block to decrypt.
 */
void decryptBlock(uint64_t key, unsigned char* ciph) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    // Make the key be 56 bits
    key &= 0xFFFFFFFFFFFFFF;

    memcpy(&keyBlock, &key, sizeof(keyBlock));
    DES_set_odd_parity(&keyBlock);
    if (DES_set_key_checked(&keyBlock, &schedule) != 0)
    {
        return;
    }

    DES_ecb_encrypt((DES_cblock*)ciph, (DES_cblock*)ciph, &schedule, DES_DECRYPT);
}

/**
 * Encrypts a block using the DES algorithm.
 * @param key The key to use for encryption.
 * @param ciph The block to encrypt.
 */
void encryptBlock(uint64_t key, unsigned char* ciph) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    // Make the key be 56 bits
    key &= 0xFFFFFFFFFFFFFF;

    memcpy(&keyBlock, &key, sizeof(keyBlock));
    DES_set_odd_parity(&keyBlock);
    if (DES_set_key_checked(&keyBlock, &schedule) != 0)
    {
        return;
    }

    DES_ecb_encrypt((DES_cblock*)ciph, (DES_cblock*)ciph, &schedule, DES_ENCRYPT);
}

/**
 * Function to read the content of a text file.
 * @param filename The name of the file to read.
 * @param buffer A pointer to store the file content.
 * @return The length of the file content.
 */
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

/**
 * Encrypts the entire message block by block.
 * @param key The key to use for encryption.
 * @param ciph The message to encrypt.
 * @param len The length of the message.
 */
void encryptMessage(uint64_t key, unsigned char* ciph, int len)
{
    for (int i = 0; i < len; i += BLOCK_SIZE)
    {
        encryptBlock(key, ciph + i);
    }
}

/**
 * Decrypts the entire message block by block.
 * @param key The key to use for decryption.
 * @param ciph The message to decrypt.
 * @param len The length of the message.
 */
void decryptMessage(uint64_t key, unsigned char* ciph, int len)
{
    for (int i = 0; i < len; i += BLOCK_SIZE)
    {
        decryptBlock(key, ciph + i);
    }
}

/**
 * Adds padding to the message to ensure it is a multiple of BLOCK_SIZE.
 * @param message The message to pad.
 * @param len The length of the message.
 * @param new_len Pointer to store the new length after padding.
 * @return A new message with padding.
 */
unsigned char* addPadding(unsigned char* message, int len, int* new_len)
{
    int padding_needed = BLOCK_SIZE - (len % BLOCK_SIZE);
    *new_len = len + padding_needed;

    unsigned char* padded_message = (unsigned char*)malloc(*new_len);
    memcpy(padded_message, message, len);

    // Add padding (PKCS5/PKCS7 style)
    for (int i = len; i < *new_len; ++i)
    {
        padded_message[i] = padding_needed;
    }

    return padded_message;
}

/**
 * Removes padding from the decrypted message.
 * @param message The decrypted message with padding.
 * @param len Pointer to the length of the message.
 */
void removePadding(unsigned char* message, int* len)
{
    int padding_value = message[*len - 1];
    *len -= padding_value;
}

/**
 * Searches for a keyword in a decrypted message.
 * @param decrypted The decrypted message.
 * @param keyword The word to search for.
 * @return 1 if the keyword is found, 0 otherwise.
 */
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

    // Definimos la oración fija que se quiere buscar en el texto descifrado
    const char* keyword = "es una prueba de";

    if (argc != 3)
    {
        // El programa ahora espera solo 2 argumentos: key y file
        if (rank == 0)
        {
            printf("Usage: %s <key> <file>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    // Parse the key from the argument for encryption
    uint64_t original_key = strtoull(argv[1], NULL, 10);

    // Read the content of the file only in rank 0
    unsigned char* plaintext = NULL;
    int len;
    if (rank == 0)
    {
        len = readFile(argv[2], &plaintext);
    }

    // Broadcast the length of the message to all processes
    MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate space for the plaintext and ciphertext
    if (rank != 0)
    {
        plaintext = (unsigned char*)malloc(len);
    }

    // Broadcast the plaintext from rank 0 to all processes
    MPI_Bcast(plaintext, len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Add padding to the plaintext
    int padded_len;
    unsigned char* padded_message = addPadding(plaintext, len, &padded_len);

    // Allocate space for the ciphertext
    unsigned char* cipher = (unsigned char*)malloc(padded_len + 1); // Make space for padding

    // Copy the padded message into the cipher buffer for encryption
    memcpy(cipher, padded_message, padded_len);

    // Variables for time measurement
    clock_t start_time, end_time;
    double encrypt_time, decrypt_time;

    // Measure encryption time
    start_time = clock();
    encryptMessage(original_key, cipher, padded_len);
    end_time = clock();
    encrypt_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    if (rank == 0)
    {
        printf("Encrypted message:\n\t");
        for (int i = 0; i < padded_len; ++i)
        {
            printf("%c", cipher[i]); // Print as raw characters for encryption
        }
        printf("\nEncryption completed in %.6f seconds\n", encrypt_time);
    }

    // Ahora realizaremos el ataque de fuerza bruta para encontrar la clave correcta.
    unsigned char* brute_force_attempt = (unsigned char*)malloc(padded_len);

    // Vamos a probar claves desde 0 hasta un límite de 56 bits
    long found_key = -1;
    uint64_t current_key = 1;
    const uint64_t upper_limit = (1ULL << 56); // Full keyspace for 56-bit DES

    start_time = clock();
    while (found_key == -1 && current_key < upper_limit)
    {
        // Copiar el mensaje cifrado para cada intento
        memcpy(brute_force_attempt, cipher, padded_len);

        // Intentar descifrar con la clave actual
        decryptMessage(current_key, brute_force_attempt, padded_len);

        // Remover el padding para verificar si el mensaje fue descifrado correctamente
        int decrypted_len = padded_len;
        removePadding(brute_force_attempt, &decrypted_len);

        // Verificar si el texto descifrado contiene la palabra clave
        if (searchKeyword((char*)brute_force_attempt, keyword))
            found_key = current_key;

        current_key++;
    }
    end_time = clock();
    decrypt_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    if (rank == 0)
    {
        printf("\nDecryption completed in %.6f seconds\n", decrypt_time);

        if (found_key != -1)
        {
            printf("\tCorrect decryption key found: %ld\n", found_key);

            // Verificar nuevamente si la palabra clave está en el mensaje descifrado
            if (searchKeyword((char*)brute_force_attempt, keyword))
            {
                printf("Keyword '%s' found in the decrypted message.\n", keyword);
            }
            else
            {
                printf("Keyword '%s' NOT found in the decrypted message.\n", keyword);
            }
        }
        else
        {
            printf("Decryption key not found.\n");
        }
    }

    // Clean up
    free(plaintext);
    free(padded_message);
    free(cipher);
    free(brute_force_attempt);

    MPI_Finalize(); // Finalize MPI environment

    return 0;
}