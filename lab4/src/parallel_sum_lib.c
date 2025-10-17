#include "parallel_sum_lib.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

long long Sum(const struct SumArgs *args) {
    long long sum = 0;
    for (int i = args->begin; i < args->end; i++) {
        sum += args->array[i];
    }
    return sum;
}

void* ThreadSum(void* args) {
    struct SumArgs *sum_args = (struct SumArgs*)args;
    long long result = Sum(sum_args);
    return (void*)(long long)result;
}

long long ParallelSum(int *array, int array_size, int threads_num, double *elapsed_time) {
    struct timeval start_time, end_time;
    pthread_t threads[threads_num];
    struct SumArgs args[threads_num];
    
    // Замеряем время начала вычислений
    gettimeofday(&start_time, NULL);
    
    // Создаем потоки
    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * array_size / threads_num;
        args[i].end = (i + 1) * array_size / threads_num;
        
        // Для последнего потока убедимся, что end = array_size
        if (i == threads_num - 1) {
            args[i].end = array_size;
        }
        
        if (pthread_create(&threads[i], NULL, ThreadSum, (void*)&args[i])) {
            printf("Error: pthread_create failed!\n");
            exit(1);
        }
    }
    
    // Собираем результаты
    long long total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        long long thread_sum = 0;
        pthread_join(threads[i], (void**)&thread_sum);
        total_sum += thread_sum;
    }
    
    // Замеряем время окончания вычислений
    gettimeofday(&end_time, NULL);
    
    // Вычисляем затраченное время в миллисекундах
    *elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
    *elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0;
    
    return total_sum;
}