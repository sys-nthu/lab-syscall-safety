CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11 -g

BIN := readmail readmail-safe

.PHONY: all build clean

all: build

build: $(BIN)

readmail: readmail.c 
	$(CC) $(CFLAGS) $^ -o $@

readmail-safe: readmail.c
	$(CC) $(CFLAGS) -DCHANGE_USER $^ -o $@

clean:
	rm -f $(BIN)
