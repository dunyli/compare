/*
 * insertion_sort.c
 * Реализация сортировки вставками
 */

#include "insertion_sort.h"

 /*
  * Сортировка части массива методом вставками
  */
void insertion_sort_range(int arr[], int low, int high) {
    for (int i = low + 1; i <= high; i++) {
        int key = arr[i];      /* Текущий элемент для вставки */
        int j = i - 1;

        /* Сдвигаем элементы больше key вправо */
        while (j >= low && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = key;      /* Вставляем key на правильное место */
    }
}

/*
 * Сортировка всего массива методом вставками
 */
void insertion_sort(int arr[], int n) {
    if (arr && n > 1) {
        insertion_sort_range(arr, 0, n - 1);
    }
}