#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_zombies>\n", argv[0]);
        return 1;
    }
    
    int num_zombies = atoi(argv[1]);
    if (num_zombies <= 0) {
        printf("Number of zombies must be positive\n");
        return 1;
    }
    
    printf("Parent PID: %d\n", getpid());
    printf("Creating %d zombie processes...\n", num_zombies);
    printf("Run 'ps aux | grep defunct' in another terminal to see zombies\n");
    printf("Press Enter to clean up zombies and exit...\n");
    
    pid_t *child_pids = malloc(num_zombies * sizeof(pid_t));
    
    // Создаем дочерние процессы, которые становятся зомби
    for (int i = 0; i < num_zombies; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork failed");
            return 1;
        }
        
        if (pid == 0) {
            // Дочерний процесс
            printf("Child %d created with PID: %d\n", i, getpid());
            exit(0);  // Немедленно завершаемся, становимся зомби
        } else {
            // Родительский процесс
            child_pids[i] = pid;
        }
    }
    
    // Родитель НЕ вызывает wait() - процессы становятся зомби
    printf("\n%d zombie processes created!\n", num_zombies);
    printf("They will remain zombies until parent calls wait() or exits.\n");
    
    // Ждем нажатия Enter
    getchar();
    
    // Теперь убираем зомби
    printf("Cleaning up zombies...\n");
    for (int i = 0; i < num_zombies; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        printf("Reaped zombie with PID: %d\n", child_pids[i]);
    }
    
    free(child_pids);
    printf("All zombies cleaned up. Exiting...\n");
    
    return 0;
}