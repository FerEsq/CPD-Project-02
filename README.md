# CPD-Project-02
Repositorio para el segundo proyecto de Computación Paralela y Distribuida.

```
docker-compose up <--build>
```

```
docker-compose exec dev /bin/bash
```

## Parte A
### Secuencial
```
g++ -o out.o bruteforce_sec.cpp -lcrypto
```
```
./out.o
```
### Paralelo
```
mpicc -o out.o bruteforce.c -lssl -lcrypto
```

```
mpirun --allow-run-as-root -np 4 ./out.o
```