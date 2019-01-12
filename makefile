CC=gcc
CFLAGS=-lreadline -g -Wall -std=c11

all: clean mpsh

mpsh: mpsh.c lib.c lib.h
	$(CC) $(CFLAGS) -o mpsh mpsh.c lib.c

clean:
	$(RM) -rf mpsh *.dSYM
