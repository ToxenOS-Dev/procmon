CC=clang
CFLAGS=-Wall -Wextra -Iinclude
LDFLAGS=-lncurses

SRC=src/main.c src/proc.c src/cpu.c src/ui.c
OUT=procmon

PREFIX ?= $$(pwd)/dist

install:
	mkdir -p $(PREFIX)/bin
	cp procmon $(PREFIX)/bin/procmon

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f procmon
