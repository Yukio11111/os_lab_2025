CC=gcc
CFLAGS=-I.

# Target all собирает все программы
all: sequential_min_max parallel_min_max run_sequential

sequential_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Target для программы с exec (задание 4)
run_sequential: run_sequential.c
	$(CC) -o run_sequential run_sequential.c $(CFLAGS)

utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max run_sequential min_max_*.txt

.PHONY: all clean