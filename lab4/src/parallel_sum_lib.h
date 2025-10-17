#ifndef PARALLEL_SUM_LIB_H
#define PARALLEL_SUM_LIB_H

#include <sys/time.h>

struct SumArgs {
    int *array;
    int begin;
    int end;
};

// Функция для вычисления суммы в одном потоке
long long Sum(const struct SumArgs *args);

// Функция для многопоточного вычисления суммы
long long ParallelSum(int *array, int array_size, int threads_num, double *elapsed_time);

#endif