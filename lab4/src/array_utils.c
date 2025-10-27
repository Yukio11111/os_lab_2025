#include "array_utils.h"
#include <stdlib.h>

// Генерация массива случайных чисел
void GenerateArray(int *array, unsigned int array_size, unsigned int seed) {
    srand(seed);
    for (unsigned int i = 0; i < array_size; i++) {
        array[i] = rand() % 100; // Числа от 0 до 99 для удобства
    }
}

// Суммирование части массива
int Sum(const struct SumArgs *args) {
    int sum = 0;
    for (int i = args->begin; i < args->end; i++) {
        sum += args->array[i];
    }
    return sum;
}