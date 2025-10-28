#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Глобальные переменные
int k = 0;
int pnum = 1;
int mod = 1;
long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int start;
    int end;
} thread_data_t;

// Функция для вычисления произведения в диапазоне
void* compute_range(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long partial_result = 1;
    
    printf("Thread %d: computing range %d to %d\n", data->thread_id, data->start, data->end);
    
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * i) % mod;
    }
    
    // Захватываем мьютекс для обновления общего результата
    pthread_mutex_lock(&mutex);
    result = (result * partial_result) % mod;
    printf("Thread %d: partial result = %lld, total result = %lld\n", 
           data->thread_id, partial_result, result);
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

// Функция для разбора аргументов командной строки
void parse_arguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }
    
    // Проверка корректности параметров
    if (k <= 0) {
        printf("Error: k must be positive\n");
        exit(1);
    }
    if (pnum <= 0) {
        printf("Error: pnum must be positive\n");
        exit(1);
    }
    if (mod <= 0) {
        printf("Error: mod must be positive\n");
        exit(1);
    }
    
    printf("Parameters: k = %d, pnum = %d, mod = %d\n", k, pnum, mod);
}

int main(int argc, char* argv[]) {
    // Разбор аргументов командной строки
    parse_arguments(argc, argv);
    
    // Если количество потоков больше k, уменьшаем pnum
    if (pnum > k) {
        pnum = k;
        printf("Adjusted pnum to %d (cannot exceed k)\n", pnum);
    }
    
    // Создание потоков
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    // Вычисление размера блока для каждого потока
    int block_size = k / pnum;
    int remainder = k % pnum;
    
    int current_start = 1;
    
    // Создание и запуск потоков
    for (int i = 0; i < pnum; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start = current_start;
        
        // Распределяем остаток по первым потокам
        int current_end = current_start + block_size - 1;
        if (i < remainder) {
            current_end++;
        }
        
        thread_data[i].end = current_end;
        current_start = thread_data[i].end + 1;
        
        if (pthread_create(&threads[i], NULL, compute_range, &thread_data[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }
    
    // Вывод результата
    printf("\nFinal result: %d! mod %d = %lld\n", k, mod, result);
    
    // Уничтожение мьютекса
    pthread_mutex_destroy(&mutex);
    
    return 0;
}