# CPD-Project-02
Repositorio para el segundo proyecto de Computación Paralela y Distribuida.

```bash
docker-compose run --build --rm -it dev
```

## Parte A
### Secuencial
```bash
g++ -o out.o bruteforce_sec.cpp -lcrypto
```
```bash
./out.o
```
### Paralelo
```bash
mpicc -o out.o bruteforce.c -lssl -lcrypto
```

```bash
mpirun --allow-run-as-root -np 4 ./out.o
```

## Parte B
### Paralelo
```bash
mpic++ -o out.o bruteforce_par.cpp -lssl -lcrypto
```

```bash
mpirun --allow-run-as-root -np <num_procesos> ./out.o <clave> <nombre_archivo_txt>
```

#### Ejemplo:
```bash
mpirun --allow-run-as-root -np 4 ./out.o 18014398509481983 message.txt
mpirun --allow-run-as-root -np 4 ./out.o 42 message.txt
```