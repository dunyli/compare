/*
 * quick_sort.c
 * Реализация быстрой сортировки
 */

#include "quick_sort.h"
#include <stdlib.h>
#include <string.h>

 /*
  * Обмен двух элементов
  */
static void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * Сортировка вставками для малых подмассивов
 * Используется для оптимизации быстрой сортировки
 */
static void insertion_sort_small(int arr[], int low, int high) {
    for (int i = low + 1; i <= high; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= low && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/*
 * Разбиение массива (с медианой из трёх)
 * Возвращает индекс опорного элемента
 */
static int partition(int arr[], int low, int high) {
    /* Медиана из трёх для улучшения производительности на частично отсортированных данных */
    int mid = low + (high - low) / 2;

    /* Сортируем три элемента и выбираем медиану как опорный */
    if (arr[mid] < arr[low]) swap(&arr[low], &arr[mid]);
    if (arr[high] < arr[low]) swap(&arr[low], &arr[high]);
    if (arr[high] < arr[mid]) swap(&arr[mid], &arr[high]);

    /* Перемещаем опорный элемент в конец */
    swap(&arr[mid], &arr[high]);

    int pivot = arr[high];  /* Опорный элемент */
    int i = low - 1;        /* Индекс для элементов меньше опорного */

    /* Проходим по массиву и переставляем элементы */
    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }

    /* Ставим опорный элемент на правильное место */
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

/*
 * Рекурсивная быстрая сортировка
 */
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        /* Для малых подмассивов используем сортировку вставками (оптимизация) */
        if (high - low < 20) {
            insertion_sort_small(arr, low, high);
            return;
        }

        /* Разбиваем массив */
        int pi = partition(arr, low, high);

        /* Рекурсивно сортируем левую и правую части */
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

/*
 * Упрощённый интерфейс быстрой сортировки
 */
void quick_sort_simple(int arr[], int n) {
    if (arr && n > 1) {
        quick_sort(arr, 0, n - 1);
    }
}