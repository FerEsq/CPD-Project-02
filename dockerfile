FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    openmpi-bin \
    libopenmpi-dev \
    libtirpc-dev \
    libgcrypt20-dev \
    libbsd-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY ./src .