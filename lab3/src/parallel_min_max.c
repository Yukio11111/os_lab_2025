#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;

    // Обработка аргументов командной строки
    while (true) {
        int current_optind = optind ? optind : 1;
        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);
        
        if (c == -1) break;
        
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: // --seed
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be positive\n");
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
                    case 2: // --pnum
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be positive\n");
                            return 1;
                        }
                        break;
                    case 3: // --by_files
                        with_files = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }
    
    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n", argv[0]);
        return 1;
    }
    
    // Создание массива
    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    
    // Создание pipes (если не используем файлы)
    int pipes[pnum][2];
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipes[i]) == -1) {
                printf("Pipe creation failed!\n");
                return 1;
            }
        }
    }
    
    int active_child_processes = 0;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    
    // Создание дочерних процессов
    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            if (child_pid == 0) {
                // Дочерний процесс
                int segment_size = array_size / pnum;
                int start = i * segment_size;
                int end = (i == pnum - 1) ? array_size : (i + 1) * segment_size;
                
                struct MinMax local_min_max = GetMinMax(array, start, end);
                
                if (with_files) {
                    // ИСПРАВЛЕНИЕ: Увеличили буфер и добавили проверку ошибок
                    char filename[25];  // Было 20
                    snprintf(filename, sizeof(filename), "min_max_%d.txt", i);  // Было sprintf
                    FILE *file = fopen(filename, "w");
                    if (file == NULL) {
                        printf("Failed to open file %s\n", filename);
                        exit(1);
                    }
                    fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                    fclose(file);
                } else {
                    // Используем pipes
                    close(pipes[i][0]); // закрываем чтение
                    write(pipes[i][1], &local_min_max, sizeof(struct MinMax));
                    close(pipes[i][1]);
                }
                free(array);
                exit(0);
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }
    
    // Ожидание завершения дочерних процессов
    while (active_child_processes > 0) {
        wait(NULL);
        active_child_processes -= 1;
    }
    
    // Сбор результатов
    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;
    
    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;
        
        if (with_files) {
            // ИСПРАВЛЕНИЕ: Увеличили буфер и добавили проверку ошибок
            char filename[25];  // Было 20
            snprintf(filename, sizeof(filename), "min_max_%d.txt", i);  // Было sprintf
            FILE *file = fopen(filename, "r");
            if (file == NULL) {
                printf("Failed to open file %s\n", filename);
                return 1;
            }
            fscanf(file, "%d %d", &min, &max);
            fclose(file);
            remove(filename); // удаляем временный файл
        } else {
            // Чтение из pipes
            close(pipes[i][1]); // закрываем запись
            struct MinMax local_result;
            read(pipes[i][0], &local_result, sizeof(struct MinMax));
            close(pipes[i][0]);
            min = local_result.min;
            max = local_result.max;
        }
        
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }
    
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;
    
    free(array);
    
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    
    return 0;
}