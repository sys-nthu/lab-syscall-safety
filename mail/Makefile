CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11 -g

BIN := readmail readmail-v2

.PHONY: all build clean

all: build

build: $(BIN)

readmail: readmail.c mail-common.c
	$(CC) $(CFLAGS) $^ -o $@

readmail-v2: readmail-v2.c mail-common.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(BIN)
