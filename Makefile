CC=gcc
CFLAGS=-Wall -Wextra -Wno-format-truncation -std=gnu17
OPT=-O2

SOURCE=src/lsc.c

BIN_NAME=lsc
BIN=bin

build:
	mkdir -p $(BIN)/release
	$(CC) $(CFLAGS) $(OPT) $(SOURCE) -o $(BIN)/release/$(BIN_NAME)

debug:
	mkdir -p $(BIN)/debug
	$(CC) $(CFLAGS) -g $(SOURCE) -o $(BIN)/debug/$(BIN_NAME)

all: build debug

clean:
	rm -rf $(BIN)