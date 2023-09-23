CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread -lrt
default : all

all : aesdsocket

aesdsocket : aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o $(LDFLAGS) $(CFLAGS)

aesdsocket.o : aesdsocket.c
	$(CC) -c aesdsocket.c $(LDFLAGS) $(CFLAGS)

.PHONY: clean
clean :
	-rm aesdsocket.o aesdsocket
