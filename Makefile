CC = clang
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c src/proc.c src/cpu.c src/ui.c
OUT = procmon

PREFIX ?= /usr/local

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) -lncurses

install: build
	mkdir -p $(PREFIX)/bin
	cp $(OUT) $(PREFIX)/bin/procmon

clean:
	rm -f procmon
