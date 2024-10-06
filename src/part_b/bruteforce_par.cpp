/**
 * This program encrypts and decrypts a message loaded from a file using DES and an arbitrary key, using MPI for parallel processing.
 * 
 * Programación Paralela y Distribuida
 * Andrés Montoya - 21552
 * Francisco Castillo - 21562
 * Fernanda Esquivel - 21542
 */

#include <openssl/des.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

/**
 * Decrypts a message using the DES algorithm.
 * @param key The key to use for decryption.
 * @param ciph The message to decrypt.
 * @param len The length of the message.
 */
void decrypt(long key, unsigned char *ciph, int len) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    memcpy(&keyBlock, &key, sizeof(keyBlock));
    DES_set_odd_parity(&keyBlock);
    if (DES_set_key_checked(&keyBlock, &schedule) != 0) {
        return;
    }

    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

/**
 * Encrypts a message using the DES algorithm.
 * @param key The key to use for encryption.
 * @param ciph The message to encrypt.
 * @param len The length of the message.
 */
void encrypt(long key, unsigned char *ciph, int len) {
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    memcpy(&keyBlock, &key, sizeof(keyBlock));
    DES_set_odd_parity(&keyBlock);
    if (DES_set_key_checked(&keyBlock, &schedule) != 0) {
        return;
    }

    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

/**
 * Function to read the content of a text file.
 * @param filename The name of the file to read.
 * @param buffer A pointer to store the file content.
 * @return The length of the file content.
 */
int readFile(const char *filename, unsigned char **buffer) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error opening file.\n");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    *buffer = (unsigned char *)malloc(file_size + 1);
    fread(*buffer, 1, file_size, file);
    (*buffer)[file_size] = '\0';  // Ensure null-terminated string
    fclose(file);

    return file_size;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);  // Initialize MPI environment

    int numprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);  // Get number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);      // Get process rank

    if (argc != 3) {
        if (rank == 0) {
            printf("Usage: %s <key> <file>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    // Parse the key from the argument
    long key = atol(argv[1]);

    // Read the content of the file only in rank 0
    unsigned char *plaintext = NULL;
    int len;
    if (rank == 0) {
        len = readFile(argv[2], &plaintext);
    }

    // Broadcast the length of the message to all processes
    MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate space for the plaintext and ciphertext
    if (rank != 0) {
        plaintext = (unsigned char *)malloc(len);
    }
    unsigned char *cipher = (unsigned char *)malloc(len);

    // Broadcast the plaintext from rank 0 to all processes
    MPI_Bcast(plaintext, len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Copy the plaintext into the cipher buffer for encryption
    memcpy(cipher, plaintext, len);

    // Variables for time measurement
    clock_t start_time, end_time;
    double encrypt_time, decrypt_time;

    // Measure encryption time
    start_time = clock();
    encrypt(key, cipher, len);
    end_time = clock();
    encrypt_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    if (rank == 0) {
        printf("Encrypted message (from process %d): %s\n", rank, cipher);
        printf("Encryption completed in %.6f seconds\n", encrypt_time);
    }

    // Measure decryption time
    start_time = clock();
    decrypt(key, cipher, len);
    end_time = clock();
    decrypt_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    if (rank == 0) {
        printf("Decrypted message (from process %d): %s\n", rank, cipher);
        printf("Decryption completed in %.6f seconds\n", decrypt_time);
    }

    // Clean up
    free(plaintext);
    free(cipher);

    MPI_Finalize();  // Finalize MPI environment

    return 0;
}
