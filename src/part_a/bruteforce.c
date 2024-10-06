/**
 * This program is a parallel brute force attack on a DES encrypted message. This program is a modified version of the original to make it work with openssl.
 * Programación Paralela y distribuida
 * Andrés Montoya - 21552
 * Francisco Castillo - 21562
 * Fernanda Esquivel - 21542
 */

#include <openssl/des.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <time.h>

/**
 * Decrypts a message using the DES algorithm
 * @param key The key to use for decryption
 * @param ciph The message to decrypt
 * @param len The length of the message
 * @return void
 */
void decrypt(long key, unsigned char *ciph) {
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
 * Encrypts a message using the DES algorithm
 * @param key The key to use for encryption
 * @param ciph The message to encrypt
 * @param len The length of the message
 * @return void
 */
void encrypt(long key, unsigned char *ciph) {
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
 * Tries a key to decrypt a message and checks if the message contains the search string
 * @param key The key to try
 * @param ciph The message to decrypt
 * @param len The length of the message
 * @return 1 if the message contains the search string, 0 otherwise
 */
int tryKey(long key, unsigned char *ciph, int len) {
  unsigned char temp[len + 1];
  memcpy(temp, ciph, len);
  decrypt(key, temp);
  temp[len] = '\0'; 
  return strstr((char *)temp, search) != NULL;
}

int main(int argc, char *argv[]) {
  // Number of processes and process id
  int N, id;
  // The upper bound for the DES keys
  long upper = (1L << 20);
  long mylower, myupper;

  // MPI status and request
  MPI_Status st;
  MPI_Request req;
  int flag;

  // The message to decrypt
  unsigned char plaintext[] = "test: this is a message";
  unsigned char cipher[sizeof(plaintext)];
  long key;

  // Initialize MPI
  MPI_Init(&argc, &argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  if (id == 0) {
      // Generate a random key and encrypt the message, only the master node does this
      srand(time(NULL));
      key = rand() % upper;

      printf("Key: %li\n", key);

      // Copy the message to the cipher
      memcpy(cipher, plaintext, sizeof(plaintext));
      encrypt(key, cipher);
  }

  // Broadcast the cypher to all nodes
  MPI_Bcast(cipher, sizeof(cipher), MPI_UNSIGNED_CHAR, 0, comm);

  // Calculate the range of keys to try
  int ciphlen = sizeof(cipher);
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id + 1) - 1;
  if (id == N - 1) {
      myupper = upper - 1;
  }

  long found = 0;

  // Receive the found key from any node
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, 0, comm, &req);

  // Try the keys in the range
  for (long i = mylower; i <= myupper && (found == 0); ++i) {
      if (tryKey(i, cipher, ciphlen)) {
          if (found != 0) {
              break;
          }
          printf("Process %d found the key: %li\n", id, i);
          found = i;
          for (int node = 0; node < N; node++) {
              MPI_Send(&found, 1, MPI_LONG, node, 0, comm);
          }
          break;
      }
  }

  if (id == 0) {
    MPI_Wait(&req, &st);
    // decrypt the message with the found key
    decrypt(found, cipher);
    // Print the found key and the decrypted message
    printf("%li: \"%s\"\n", found, cipher);
  }

  MPI_Finalize();
  return 0;
}