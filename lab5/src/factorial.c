#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <stdint.h>

// Структура для передачи данных в поток
typedef struct {
    int start;          // Начало диапазона для потока
    int end;            // Конец диапазона для потока
    int mod;            // Модуль для вычислений
    uint64_t result;    // Результат вычислений потока
} ThreadData;

// Глобальные переменные
uint64_t global_result = 1;              // Общий результат всех потоков
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;  // Мьютекс для защиты

// Функция, выполняемая каждым потоком
void* calculate_partial_factorial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    uint64_t local_result = 1;
    
    // Вычисляем произведение чисел в диапазоне [start, end]
    // Применяем модуль на каждом шаге, чтобы избежать переполнения
    for (int i = data->start; i <= data->end; i++) {
        local_result = (local_result * i) % data->mod;
    }
    
    // Сохраняем результат в структуре
    data->result = local_result;
    
    // Критическая секция: обновляем глобальный результат
    pthread_mutex_lock(&result_mutex);
    global_result = (global_result * local_result) % data->mod;
    pthread_mutex_unlock(&result_mutex);
    
    printf("Thread: range [%d, %d] -> partial result = %lu\n", 
           data->start, data->end, local_result);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    int k = 0;          // Число, факториал которого вычисляем
    int pnum = 0;       // Количество потоков
    int mod = 0;        // Модуль
    
    //ПАРСИНГ АРГУМЕНТОВ КОМАНДНОЙ СТРОКИ 
    
    // Определяем короткие опции (например: -k 10)
    const char* short_opts = "k:p:m:";
    
    // Определяем длинные опции (например: --pnum=4)
    struct option long_opts[] = {
        {"pnum", required_argument, NULL, 'p'},  // --pnum требует аргумент
        {"mod",  required_argument, NULL, 'm'},  // --mod требует аргумент
        {NULL, 0, NULL, 0}                       // Завершающий элемент
    };
    
    int opt;
    // Цикл обработки всех опций
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'k':  // Опция -k
                k = atoi(optarg);  // Преобразуем строку в число
                break;
            case 'p':  // Опция -p или --pnum
                pnum = atoi(optarg);
                break;
            case 'm':  // Опция -m или --mod
                mod = atoi(optarg);
                break;
            case '?':  // Неизвестная опция
                fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulo>\n", argv[0]);
                fprintf(stderr, "Example: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
                return 1;
        }
    }
    
    // Проверка корректности входных данных
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Error: all parameters must be positive\n");
        fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulo>\n", argv[0]);
        return 1;
    }
    
    printf("Computing %d! mod %d using %d threads\n", k, mod, pnum);
    
    //СОЗДАНИЕ И ЗАПУСК ПОТОКОВ
    
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * pnum);
    ThreadData* thread_data = (ThreadData*)malloc(sizeof(ThreadData) * pnum);
    
    // Вычисляем размер диапазона для каждого потока
    int range_size = k / pnum;  // Базовый размер диапазона
    int remainder = k % pnum;   // Остаток для распределения
    
    int current_start = 1;  // Начало текущего диапазона
    
    // Создаём потоки
    for (int i = 0; i < pnum; i++) {
        thread_data[i].start = current_start;
        
        // Вычисляем конец диапазона
        // Первые remainder потоков получают +1 элемент
        thread_data[i].end = current_start + range_size - 1;
        if (i < remainder) {
            thread_data[i].end++;
        }
        
        thread_data[i].mod = mod;
        thread_data[i].result = 1;
        
        // Создаём поток
        if (pthread_create(&threads[i], NULL, calculate_partial_factorial, 
                          &thread_data[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
        
        // Обновляем начало для следующего потока
        current_start = thread_data[i].end + 1;
    }
    
    // ОЖИДАНИЕ ЗАВЕРШЕНИЯ ВСЕХ ПОТОКОВ
    
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    printf("\nFinal result: %d! mod %d = %lu\n", k, mod, global_result);
    
    // Освобождаем память
    free(threads);
    free(thread_data);
    
    // Уничтожаем мьютекс
    pthread_mutex_destroy(&result_mutex);
    
    return 0;
}
