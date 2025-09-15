CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11

# Binaries
CREATES := create-v1 create-v2
OPENS   := open-v1 open-ro open-wo
BIN     := $(CREATES) $(OPENS)

.PHONY: all build clean

all: build
build: $(BIN)

create-v1: create-v1.c
	$(CC) $(CFLAGS) $< -o $@

create-v2: create-v2.c
	$(CC) $(CFLAGS) $< -o $@

open-v1: open-v1.c
	$(CC) $(CFLAGS) $< -o $@

open-ro: open-ro.c
	$(CC) $(CFLAGS) $< -o $@

open-wo: open-wo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BIN)
