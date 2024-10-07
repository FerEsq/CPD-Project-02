FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    openmpi-bin \
    libopenmpi-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY ./src/part_a .
COPY ./src/part_b .