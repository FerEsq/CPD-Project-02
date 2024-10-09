# OpenMPI Cryptography
A repository that uses _brute force_ to encrypt and decrypt a message with the DES algorithm using OpenMPI.

## How to run?
First, let's get the container up and running with an interactive shell.
```bash
docker-compose run --build --rm -it dev
```

### Part A
Remember, access the `part_a` directory.
#### Sequential
To get the sequential program running, you need to compile the `bruteforce_sec.cpp` file and run the output file.
```bash
g++ -o out.o bruteforce_sec.cpp -lcrypto
./out.o
```
#### Parallel
To get the parallel program running, you need to compile the `bruteforce.cpp` file and run the output file.
```bash
mpicc -o out.o bruteforce.c -lssl -lcrypto
mpirun --allow-run-as-root -np 4 ./out.o
```
### Parte B
Remember, access the `part_b` directory.
#### _Naive_ approach
To get the _naive_ approach running, you need to compile the `bruteforce_par.cpp` file and run the output file.
```bash
mpic++ -o out.o bruteforce_par.cpp -lssl -lcrypto
mpirun --allow-run-as-root -np <n_processes> ./out.o <key> <file_name>.txt
```

#### Alternative 1 ```alternatives/nonsec_ranges.cpp```
```bash
mpic++ -o out.o nonsec_ranges.cpp -lssl -lcrypto
mpirun --allow-run-as-root -np <n_processes> ./out.o <key> <file_name>.txt
```

#### Alternative 2 ```alternatives/alter_extremes.cpp```
```bash
mpic++ -o out.o alter_extremes.cpp -lssl -lcrypto
mpirun --allow-run-as-root -np <n_processes> ./out.o <key> <file_name>.txt
```


### Example:
After you have compiled any of the `part_b` alternatives, you can run the following commands to test the program.
```bash
mpirun --allow-run-as-root -np 4 ./out1.o 42 message.txt
mpirun --allow-run-as-root -np 4 ./out1.o 12356 message.txt
mpirun --allow-run-as-root -np 4 ./out1.o 18014398509481983 message.txt
mpirun --allow-run-as-root -np 4 ./out1.o 18014398509481984 message.txt
```