#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s seed arraysize\n", argv[0]);
        return 1;
    }

    pid_t child_pid = fork();
    
    if (child_pid == -1) {
        printf("Fork failed!\n");
        return 1;
    }
    
    if (child_pid == 0) {
        // Дочерний процесс
        printf("Child process: Starting sequential_min_max...\n");
        
        // Запускаем sequential_min_max с переданными аргументами
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        execvp(args[0], args);
        
        // Если execvp вернул управление, значит произошла ошибка
        printf("execvp failed!\n");
        exit(1);
    } else {
        // Родительский процесс
        printf("Parent process: Waiting for child to complete...\n");
        
        int status;
        waitpid(child_pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Parent process: Child exited with status %d\n", WEXITSTATUS(status));
        } else {
            printf("Parent process: Child terminated abnormally\n");
        }
    }
    
    return 0;
}