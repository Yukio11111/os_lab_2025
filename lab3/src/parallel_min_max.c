#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <errno.h>
#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для обработки сигналов
volatile sig_atomic_t timeout_occurred = 0;
pid_t *child_pids = NULL;
int pnum_global = 0;

// Обработчик сигнала ALARM
void alarm_handler(int sig) {
    timeout_occurred = 1;
    printf("Timeout occurred! Sending SIGKILL to child processes...\n");
    
    // Посылаем SIGKILL всем дочерним процессам
    for (int i = 0; i < pnum_global; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

// Неблокирующее ожидание дочерних процессов
int wait_for_children_nonblocking(int *active_processes) {
    int status;
    pid_t pid;
    int count = 0;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child process %d exited normally with status %d\n", 
                   pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process %d killed by signal %d\n", 
                   pid, WTERMSIG(status));
        }
        (*active_processes)--;
        count++;
    }
    
    if (pid == -1 && errno != ECHILD) {
        perror("waitpid failed");
    }
    
    return count;
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0;  // 0 означает нет таймаута
    bool with_files = false;

    // Обработка аргументов командной строки
    while (true) {
        int current_optind = optind ? optind : 1;
        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"timeout", required_argument, 0, 0},  // НОВЫЙ ПАРАМЕТР
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);
        
        if (c == -1) break;
        
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be positive\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be positive\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be positive\n");
                            return 1;
                        }
                        break;
                    case 3:  // НОВЫЙ: обработка timeout
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("timeout must be positive\n");
                            return 1;
                        }
                        printf("Timeout set to %d seconds\n", timeout);
                        break;
                    case 4:
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
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n", argv[0]);
        return 1;
    }
    
    // Инициализация глобальных переменных для обработки сигналов
    pnum_global = pnum;
    child_pids = malloc(pnum * sizeof(pid_t));
    if (child_pids == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    for (int i = 0; i < pnum; i++) {
        child_pids[i] = 0;
    }
    
    // Настройка обработчика сигнала ALARM
    if (timeout > 0) {
        signal(SIGALRM, alarm_handler);
        alarm(timeout);  // Устанавливаем таймер
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
                free(child_pids);
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
            child_pids[i] = child_pid;  // Сохраняем PID дочернего процесса
            
            if (child_pid == 0) {
                // Дочерний процесс - НЕ реагирует на SIGALRM
                signal(SIGALRM, SIG_IGN);
                
                int segment_size = array_size / pnum;
                int start = i * segment_size;
                int end = (i == pnum - 1) ? array_size : (i + 1) * segment_size;
                
                struct MinMax local_min_max = GetMinMax(array, start, end);
                
                if (with_files) {
                    char filename[25];
                    snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    if (file == NULL) {
                        printf("Failed to open file %s\n", filename);
                        exit(1);
                    }
                    fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                    fclose(file);
                } else {
                    close(pipes[i][0]);
                    write(pipes[i][1], &local_min_max, sizeof(struct MinMax));
                    close(pipes[i][1]);
                }
                free(array);
                free(child_pids);
                exit(0);
            }
        } else {
            printf("Fork failed!\n");
            free(child_pids);
            return 1;
        }
    }
    
    // Ожидание завершения дочерних процессов с таймаутом
    if (timeout > 0) {
        // С таймаутом: используем неблокирующее ожидание
        while (active_child_processes > 0 && !timeout_occurred) {
            // Ждем немного перед проверкой
            usleep(100000);  // 100ms
            
            // Неблокирующая проверка завершенных процессов
            wait_for_children_nonblocking(&active_child_processes);
            
            // Проверяем не произошел ли таймаут
            if (timeout_occurred) {
                printf("Timeout! Some processes were terminated\n");
                break;
            }
        }
        
        // Отменяем таймер, если он еще не сработал
        if (!timeout_occurred) {
            alarm(0);
        }
    } else {
        // Без таймаута: обычное блокирующее ожидание
        while (active_child_processes > 0) {
            wait(NULL);
            active_child_processes -= 1;
        }
    }
    
    // Сбор результатов
    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;
    
    int successful_processes = 0;
    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;
        
        // Пропускаем процессы, которые были завершены по таймауту
        if (timeout_occurred && child_pids[i] > 0) {
            // Проверяем, завершился ли процесс
            int status;
            if (waitpid(child_pids[i], &status, WNOHANG) == 0) {
                // Процесс все еще работает (должен быть убит по SIGKILL)
                continue;
            }
        }
        
        if (with_files) {
            char filename[25];
            snprintf(filename, sizeof(filename), "min_max_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                fscanf(file, "%d %d", &min, &max);
                fclose(file);
                remove(filename);
                successful_processes++;
            }
        } else {
            // Проверяем, можно ли читать из pipe
            close(pipes[i][1]);
            struct MinMax local_result;
            ssize_t bytes_read = read(pipes[i][0], &local_result, sizeof(struct MinMax));
            if (bytes_read == sizeof(struct MinMax)) {
                min = local_result.min;
                max = local_result.max;
                successful_processes++;
            }
            close(pipes[i][0]);
        }
        
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }
    
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;
    
    free(array);
    free(child_pids);
    
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Successful processes: %d/%d\n", successful_processes, pnum);
    printf("Elapsed time: %fms\n", elapsed_time);
    
    if (timeout_occurred) {
        printf("WARNING: Execution was terminated by timeout!\n");
        return 2;  // Специальный код возврата для таймаута
    }
    
    return 0;
}