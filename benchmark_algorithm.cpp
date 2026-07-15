/*
 * benchmark_algorithm.c
 * Реализация бенчмарка алгоритмов сортировки
 */

#include "benchmark_algorithm.h"
#include "quick_sort.h"
#include "insertion_sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

 /*
  * Высокоточный таймер (наносекунды)
  * Используем ту же реализацию, что и для структур
  */
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <sys/time.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif

static uint64_t get_time_ns(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (!QueryPerformanceFrequency(&frequency)) {
        return 0;
    }
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    return (uint64_t)((double)counter.QuadPart * 1000000000.0 / frequency.QuadPart);

#elif __linux__
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#else
#error "Unsupported platform"
    return 0;
#endif
}

/*
 * Генерация случайного массива
 */
int* generate_random_array(int n, int max_value) {
    int* arr = (int*)malloc(n * sizeof(int));
    if (!arr) return NULL;

    for (int i = 0; i < n; i++) {
        arr[i] = rand() % max_value;
    }

    return arr;
}

/*
 * Копирование массива
 */
int* copy_array(int arr[], int n) {
    int* copy = (int*)malloc(n * sizeof(int));
    if (!copy) return NULL;

    memcpy(copy, arr, n * sizeof(int));
    return copy;
}

/*
 * Проверка, отсортирован ли массив
 */
bool is_sorted(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return false;
        }
    }
    return true;
}

/*
 * Проверка корректности алгоритмов сортировки
 */
bool verify_sorting_correctness(void) {
    /* Тестовый массив */
    int arr[] = { 64, 34, 25, 12, 22, 11, 90 };
    int n = sizeof(arr) / sizeof(arr[0]);
    int expected[] = { 11, 12, 22, 25, 34, 64, 90 };

    /* Создаём копии для каждого алгоритма */
    int* arr_quick = copy_array(arr, n);
    int* arr_insert = copy_array(arr, n);

    if (!arr_quick || !arr_insert) {
        free(arr_quick);
        free(arr_insert);
        return false;
    }

    /* Выполняем сортировки */
    quick_sort_simple(arr_quick, n);
    insertion_sort(arr_insert, n);

    /* Проверяем результаты */
    bool ok = true;
    for (int i = 0; i < n && ok; i++) {
        if (arr_quick[i] != expected[i]) ok = false;
        if (arr_insert[i] != expected[i]) ok = false;
    }

    free(arr_quick);
    free(arr_insert);
    return ok;
}

/*
 * Бенчмарк алгоритмов сортировки
 */
void benchmark_sorting_algorithms(void) {
    printf("\n");
    printf("========================================\n");
    printf("СРАВНЕНИЕ АЛГОРИТМОВ СОРТИРОВКИ\n");
    printf("========================================\n");
    printf("\n");

    /* Размеры массивов для тестирования */
    int sizes[] = { 100, 500, 1000, 5000, 10000, 50000, 100000, 500000 };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printf("Размер\t\t Быстрая (мс)\t Вставками (мс)\t Ускорение\n");
    printf("----------------------------------------------------------------\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];

        /* Генерируем случайный массив */
        int* arr = generate_random_array(n, n * 100);
        if (!arr) {
            printf("Ошибка: не удалось сгенерировать массив\n");
            continue;
        }

        /* Создаём копии для каждого алгоритма */
        int* arr_quick = copy_array(arr, n);
        int* arr_insert = copy_array(arr, n);

        if (!arr_quick || !arr_insert) {
            free(arr);
            free(arr_quick);
            free(arr_insert);
            continue;
        }

        uint64_t start, end;
        double quick_time, insert_time;

        /* === БЫСТРАЯ СОРТИРОВКА === */
        start = get_time_ns();
        quick_sort_simple(arr_quick, n);
        end = get_time_ns();
        quick_time = (end - start) / 1000000.0;  /* Переводим в миллисекунды */

        /* === СОРТИРОВКА ВСТАВКАМИ === */
        start = get_time_ns();
        insertion_sort(arr_insert, n);
        end = get_time_ns();
        insert_time = (end - start) / 1000000.0;

        /* Проверяем корректность сортировки */
        if (!is_sorted(arr_quick, n) || !is_sorted(arr_insert, n)) {
            printf("Ошибка: массив не отсортирован правильно!\n");
        }

        /* Вывод результатов */
        printf("%8d\t%10.3f\t%10.3f\t%8.2fx\n",
            n, quick_time, insert_time, insert_time / quick_time);

        free(arr);
        free(arr_quick);
        free(arr_insert);
    }

    printf("\n");
    printf("Примечания:\n");
    printf("  - Быстрая сортировка: O(n log n) - эффективна для больших массивов\n");
    printf("  - Сортировка вставками: O(n^2) - эффективна для малых массивов (n < 100)\n");
    printf("  - Ускорение = время_вставками / время_быстрая\n");
    printf("  - Чем больше массив, тем больше преимущество быстрой сортировки\n");
}