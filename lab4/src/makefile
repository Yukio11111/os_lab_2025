zombie_demo: zombie_demo.c
	$(CC) -o zombie_demo zombie_demo.c $(CFLAGS)

all: zombie_demo parallel_sum process_memory 

clean:
	rm -f zombie_demo process_memory parallel_sum array_utils.o
CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -pthread

.PHONY: all clean

process_memory: process_memory.c
	$(CC) $(CFLAGS) -o process_memory process_memory.c $(LDFLAGS)

.PHONY: all clean help

parallel_sum: parallel_sum.c array_utils.o
	$(CC) $(CFLAGS) -o parallel_sum parallel_sum.c array_utils.o $(LDFLAGS)

array_utils.o: array_utils.c array_utils.h
	$(CC) $(CFLAGS) -c array_utils.c -o array_utils.o
