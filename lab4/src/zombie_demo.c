#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

void create_zombie() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID: %d) запущен\n", getpid());
        printf("Дочерний процесс (PID: %d) завершается\n", getpid());
        exit(0); // Завершаем дочерний процесс
    } else if (pid > 0) {
        // Родительский процесс
        printf("Родительский процесс (PID: %d) создал дочерний (PID: %d)\n", 
               getpid(), pid);
        printf("Родительский процесс НЕ вызывает wait(), создавая зомби...\n");
        
        // Ждем некоторое время, чтобы можно было наблюдать зомби
        sleep(10);
        
        // Теперь убираем зомби
        printf("Родительский процесс теперь вызывает wait()...\n");
        wait(NULL);
        printf("Зомби-процесс убран!\n");
    } else {
        perror("Ошибка при создании процесса");
        exit(1);
    }
}

void prevent_zombie_with_wait() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID: %d) запущен\n", getpid());
        sleep(2);
        printf("Дочерний процесс (PID: %d) завершается\n", getpid());
        exit(0);
    } else if (pid > 0) {
        // Родительский процесс
        printf("Родительский процесс (PID: %d) создал дочерний (PID: %d)\n", 
               getpid(), pid);
        printf("Родительский процесс сразу вызывает wait()...\n");
        
        wait(NULL); // Немедленно убираем дочерний процесс
        printf("Дочерний процесс корректно убран, зомби не создан!\n");
    } else {
        perror("Ошибка при создании процесса");
        exit(1);
    }
}

void prevent_zombie_with_signal() {
    // В реальном коде здесь нужно было бы установить обработчик SIGCHLD
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID: %d) запущен\n", getpid());
        sleep(2);
        printf("Дочерний процесс (PID: %d) завершается\n", getpid());
        exit(0);
    } else if (pid > 0) {
        // Родительский процесс
        printf("Родительский процесс (PID: %d) создал дочерний (PID: %d)\n", 
               getpid(), pid);
        printf("Родительский процесс продолжает работу (с обработчиком SIGCHLD)...\n");
        
        // Имитация работы родителя
        for (int i = 0; i < 5; i++) {
            printf("Родитель работает... (%d/5)\n", i + 1);
            sleep(1);
        }
        
        // В конце все равно вызываем wait для надежности
        wait(NULL);
        printf("Все дочерние процессы убраны!\n");
    } else {
        perror("Ошибка при создании процесса");
        exit(1);
    }
}

int main() {
    printf("=== Демонстрация зомби-процессов ===\n\n");
    
    int choice;
    printf("Выберите демонстрацию:\n");
    printf("1 - Создать зомби-процесс\n");
    printf("2 - Предотвратить зомби с помощью wait()\n");
    printf("3 - Предотвратить зомби (имитация обработки сигнала)\n");
    printf("Ваш выбор: ");
    scanf("%d", &choice);
    
    switch(choice) {
        case 1:
            printf("\n--- Создание зомби-процесса ---\n");
            printf("Запустите 'ps aux | grep defunct' в другом терминале\n");
            printf("чтобы увидеть зомби-процесс в течение 10 секунд\n\n");
            create_zombie();
            break;
        case 2:
            printf("\n--- Предотвращение зомби с помощью wait() ---\n");
            prevent_zombie_with_wait();
            break;
        case 3:
            printf("\n--- Предотвращение зомби (с обработкой сигналов) ---\n");
            prevent_zombie_with_signal();
            break;
        default:
            printf("Неверный выбор!\n");
            return 1;
    }
    
    return 0;
}