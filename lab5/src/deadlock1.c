#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_function(void* arg) {
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 1: Locked mutex2\n");
    
    // Критическая секция
    printf("Thread 1: Entering critical section\n");
    sleep(1);
    printf("Thread 1: Exiting critical section\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

void* thread2_function(void* arg) {
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 2: Locked mutex1\n");
    
    // Критическая секция
    printf("Thread 2: Entering critical section\n");
    sleep(1);
    printf("Thread 2: Exiting critical section\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("=== Deadlock Demonstration ===\n");
    printf("Эта программа продемонстрирует классический deadlock сценарий.\n");
    printf("Два потока будут пытаться получить мьютексы в разном порядке.\n\n");
    
    if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    // Даем потокам время для выполнения
    sleep(5);
    
    printf("\n=== Программа, похоже, застряла в deadlock ===\n");
    printf("Ни один из потоков не может продолжить работу, потому что:\n");
    printf("- Поток 1 содержит mutex1 и ожидает mutex2\n");
    printf("- Поток 2 содержит mutex2 и ожидает mutex1\n");
    
    // В реальной программе нужно было бы обработать эту ситуацию
    pthread_cancel(thread1);
    pthread_cancel(thread2);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    
    return 0;
}