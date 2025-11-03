#include <stdio.h>
#include <pthread.h>
#include <unistd.h>   // Для функции sleep()

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ 

// Два мьютекса, которые будут использовать оба потока
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

// Переменная для демонстрации работы с общими данными
int shared_counter = 0;


// ФУНКЦИЯ ПЕРВОГО ПОТОКА 
// Этот поток захватывает мьютексы в порядке: mutex1 -> mutex2
void* thread1_function(void* arg) {
    printf("[Thread 1] Started\n");
    
    // Небольшая задержка для синхронизации (чтобы оба потока начали примерно одновременно)
    sleep(1);
    
    // ШАГ 1: Захватываем первый мьютекс
    printf("[Thread 1] Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("[Thread 1] Locked mutex1\n");
    
    // Имитация работы с ресурсом
    printf("[Thread 1] Working with resource1...\n");
    sleep(2);  // Задержка создаёт окно для возникновения deadlock
    
    // ШАГ 2: Пытаемся захватить второй мьютекс
    // ВНИМАНИЕ: Здесь возникает DEADLOCK!
    // Thread2 уже захватил mutex2 и ждёт mutex1
    printf("[Thread 1] Trying to lock mutex2... (holding mutex1)\n");
    pthread_mutex_lock(&mutex2);  // <-- БЛОКИРУЕТСЯ ЗДЕСЬ!
    
    // Эти строки НИКОГДА не выполнятся из-за deadlock
    printf("[Thread 1] Locked mutex2\n");
    shared_counter++;
    printf("[Thread 1] Incremented counter to %d\n", shared_counter);
    
    // Освобождение мьютексов (не выполнится)
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    printf("[Thread 1] Finished\n");  // Эта строка не выполнится
    return NULL;
}


// ФУНКЦИЯ ВТОРОГО ПОТОКА 
// Этот поток захватывает мьютексы в ОБРАТНОМ порядке: mutex2 -> mutex1
void* thread2_function(void* arg) {
    printf("[Thread 2] Started\n");
    
    // Небольшая задержка для синхронизации
    sleep(1);
    
    // ШАГ 1: Захватываем второй мьютекс (в обратном порядке!)
    printf("[Thread 2] Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("[Thread 2] Locked mutex2\n");
    
    // Имитация работы с ресурсом
    printf("[Thread 2] Working with resource2...\n");
    sleep(2);  // Задержка создаёт окно для возникновения deadlock
    
    // ШАГ 2: Пытаемся захватить первый мьютекс
    // Здесь возникает DEADLOCK!
    // Thread1 уже захватил mutex1 и ждёт mutex2
    printf("[Thread 2] Trying to lock mutex1... (holding mutex2)\n");
    pthread_mutex_lock(&mutex1);  // БЛОКИРУЕТСЯ ЗДЕСЬ
    
    // Эти строки НИКОГДА не выполнятся из-за deadlock
    printf("[Thread 2] Locked mutex1\n");
    shared_counter--;
    printf("[Thread 2] Decremented counter to %d\n", shared_counter);
    
    // Освобождение мьютексов (не выполнится)
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    printf("[Thread 2] Finished\n");  // Эта строка не выполнится
    return NULL;
}


// ГЛАВНАЯ ФУНКЦИЯ 
int main() {
    pthread_t thread1, thread2;
    
    printf("ДЕМОНСТРАЦИЯ DEADLOCK\n\n");
    
    // Инициализация мьютексов
    pthread_mutex_init(&mutex1, NULL);
    pthread_mutex_init(&mutex2, NULL);
    
    printf("Creating two threads...\n\n");
    
    // Создание двух потоков
    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);
    
    // Ожидание завершения потоков
    // Эти вызовы НИКОГДА не завершатся из-за deadlock!
    // Программу придётся принудительно завершать
    printf("Main thread waiting for threads to finish...\n");
    printf("(This will wait forever due to deadlock - press Ctrl+C to stop)\n\n");
    
    pthread_join(thread1, NULL);  // ЗАВИСНЕТ ЗДЕСЬ
    pthread_join(thread2, NULL);
    
    // Эти строки НИКОГДА не выполнятся
    printf("\nAll threads finished\n");
    printf("Final counter value: %d\n", shared_counter);
    
    // Освобождение ресурсов (не выполнится)
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    
    return 0;
}
