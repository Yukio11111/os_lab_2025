#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include "array_utils.h"

// Функция для потока
void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;

    // Обработка аргументов командной строки
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while (true) {
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: // --threads_num
                        threads_num = atoi(optarg);
                        if (threads_num <= 0) {
                            printf("threads_num must be positive\n");
                            return 1;
                        }
                        break;
                    case 1: // --array_size
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be positive\n");
                            return 1;
                        }
                        break;
                    case 2: // --seed
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be positive\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (threads_num == 0 || array_size == 0 || seed == 0) {
        printf("Usage: %s --threads_num \"num\" --array_size \"num\" --seed \"num\"\n", argv[0]);
        return 1;
    }

    // Выделение памяти для массива
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        printf("Memory allocation failed for array\n");
        return 1;
    }

    // Генерация массива (не входит в замер времени)
    GenerateArray(array, array_size, seed);

    // Подготовка аргументов для потоков
    struct SumArgs args[threads_num];
    int segment_size = array_size / threads_num;
    
    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * segment_size;
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * segment_size;
    }

    // Создание потоков и замер времени
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    pthread_t threads[threads_num];
    
    // Создание потоков
    for (uint32_t i = 0; i < threads_num; i++) {
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
            printf("Error: pthread_create failed!\n");
            free(array);
            return 1;
        }
    }

    // Сбор результатов от потоков
    int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        int *thread_sum = NULL;
        if (pthread_join(threads[i], (void **)&thread_sum) != 0) {
            printf("Error: pthread_join failed!\n");
            free(array);
            return 1;
        }
        total_sum += (int)(size_t)thread_sum;
    }

    gettimeofday(&end_time, NULL);

    // Вычисление времени выполнения
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Вывод результатов
    printf("Total sum: %d\n", total_sum);
    printf("Elapsed time: %f ms\n", elapsed_time);

    free(array);
    return 0;
}