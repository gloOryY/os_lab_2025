#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid < 0) {
        // Ошибка при создании процесса
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Дочерний процесс
        printf("Child process (PID: %d) is launching sequential_min_max...\n", getpid());
        
        // Запускаем sequential_min_max с переданными аргументами
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        
        // Если execl вернул управление, значит произошла ошибка
        perror("execl failed");
        exit(1);
    } else {
        // Родительский процесс
        printf("Parent process (PID: %d) created child process (PID: %d)\n", getpid(), pid);
        printf("Waiting for child process to complete...\n");
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process was terminated by signal: %d\n", WTERMSIG(status));
        }
        
        printf("Parent process completed.\n");
    }
    
    return 0;
}
