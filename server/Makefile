CC=$(CROSS_COMPILE)gcc
CFLAGS= -g -Wall -Werror

default : all

all : aesdsocket

aesdsocket : aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o -pthread -lrt $(CFLAGS)

aesdsocket.o : aesdsocket.c
	$(CC) -c aesdsocket.c -pthread -lrt $(CFLAGS)

.PHONY: clean
clean :
	-rm aesdsocket.o aesdsocket
