#include "utils.h"
#include <stdlib.h>

void GenerateArray(int *array, unsigned int size, unsigned int seed) {
    srand(seed);
    for (unsigned int i = 0; i < size; i++) {
        array[i] = rand() % 100;  // Генерируем числа от 0 до 99 для удобства
    }
}