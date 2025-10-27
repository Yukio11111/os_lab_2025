#ifndef ARRAY_UTILS_H
#define ARRAY_UTILS_H

// Структура для аргументов суммирования
struct SumArgs {
    int *array;
    int begin;
    int end;
};

// Функция генерации массива
void GenerateArray(int *array, unsigned int array_size, unsigned int seed);

// Функция суммирования части массива
int Sum(const struct SumArgs *args);

#endif