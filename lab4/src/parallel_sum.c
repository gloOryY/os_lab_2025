#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <pthread.h>

#include "utils.h"
#include "parallel_sum_lib.h"

int main(int argc, char **argv) {
    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;
    
    // Обработка аргументов командной строки
    static struct option options[] = {
        {"threads_num", required_argument, 0, 't'},
        {"array_size", required_argument, 0, 'a'},
        {"seed", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "t:a:s:", options, &option_index)) != -1) {
        switch (c) {
            case 't':
                threads_num = atoi(optarg);
                if (threads_num <= 0) {
                    printf("threads_num must be a positive number\n");
                    return 1;
                }
                break;
            case 'a':
                array_size = atoi(optarg);
                if (array_size <= 0) {
                    printf("array_size must be a positive number\n");
                    return 1;
                }
                break;
            case 's':
                seed = atoi(optarg);
                if (seed <= 0) {
                    printf("seed must be a positive number\n");
                    return 1;
                }
                break;
            case '?':
                printf("Usage: %s --threads_num <num> --array_size <num> --seed <num>\n", argv[0]);
                return 1;
        }
    }
    
    // Проверка обязательных параметров
    if (threads_num == 0 || array_size == 0 || seed == 0) {
        printf("Usage: %s --threads_num <num> --array_size <num> --seed <num>\n", argv[0]);
        return 1;
    }
    
    printf("Threads: %u, Array size: %u, Seed: %u\n", threads_num, array_size, seed);
    
    // Выделяем память под массив
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        printf("Error: memory allocation failed!\n");
        return 1;
    }
    
    // Генерируем массив (не входит в замер времени)
    GenerateArray(array, array_size, seed);
    
    // Проверяем сумму в одном потоке для верификации
    struct SumArgs single_thread_args = {array, 0, array_size};
    long long single_thread_sum = Sum(&single_thread_args);
    
    // Вычисляем сумму многопоточно и замеряем время
    double parallel_time;
    long long parallel_sum = ParallelSum(array, array_size, threads_num, &parallel_time);
    
    // Выводим результаты
    printf("Single-thread sum: %lld\n", single_thread_sum);
    printf("Parallel sum: %lld\n", parallel_sum);
    printf("Results match: %s\n", (single_thread_sum == parallel_sum) ? "YES" : "NO");
    printf("Execution time: %.2f ms\n", parallel_time);
    
    free(array);
    return 0;
}