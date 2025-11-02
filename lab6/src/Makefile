CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: client server

libcommon.a: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o
	ar rcs libcommon.a common.o

client: client.c libcommon.a
	$(CC) $(CFLAGS) -o client client.c libcommon.a -lpthread

server: server.c libcommon.a
	$(CC) $(CFLAGS) -o server server.c libcommon.a -lpthread

clean:
	rm -f client server libcommon.a *.o