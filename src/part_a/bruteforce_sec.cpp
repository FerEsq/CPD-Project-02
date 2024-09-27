/**
 * This program performs a sequential brute-force attack on a DES encrypted message.
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

// The message to search for
char search[] = "test";

/**
 * Tries a key to decrypt a message and checks if the message contains the search string.
 * @param key The key to try.
 * @param ciph The message to decrypt.
 * @param len The length of the message.
 * @return 1 if the message contains the search string, 0 otherwise.
 */
int tryKey(long key, unsigned char *ciph, int len) {
    unsigned char temp[len + 1];
    memcpy(temp, ciph, len);
    decrypt(key, temp, len);
    temp[len] = '\0'; 
    return strstr((char *)temp, search) != NULL;
}

int main(int argc, char *argv[]) {
    // The upper bound for the DES keys
    long upper = (1L << 20);

    // The message to decrypt
    unsigned char plaintext[] = "test: this is a message";
    unsigned char cipher[sizeof(plaintext)];
    long key;

    // Generate a random key and encrypt the message
    srand(time(NULL));
    key = rand() % upper;

    // Copy the message to the cipher
    memcpy(cipher, plaintext, sizeof(plaintext));
    encrypt(key, cipher, sizeof(plaintext));

    // Try all keys from 0 to upper
    long found = -1;
    for (long i = 0; i <= upper; ++i) {
        if (tryKey(i, cipher, sizeof(plaintext))) {
            found = i;
            break;
        }
    }

    if (found != -1) {
        // Decrypt the message with the found key
        decrypt(found, cipher, sizeof(plaintext));
        // Print the found key and the decrypted message
        printf("%li: \"%s\"\n", found, cipher);
    } else {
        printf("Key not found\n");
    }

    return 0;
}