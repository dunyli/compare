#ifndef QUICK_SORT_H
#define QUICK_SORT_H

/*
 * Быстрая сортировка
 * Сложность: O(n log n) в среднем, O(n²) в худшем
 * Использует рекурсивное разделение массива
 */

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * Сортировка массива целых чисел методом быстрой сортировки
	 * arr - указатель на массив
	 * low - начальный индекс (обычно 0)
	 * high - конечный индекс (обычно n-1)
	 */
	void quick_sort(int arr[], int low, int high);

	/*
	 * Сортировка массива целых чисел методом быстрой сортировки (упрощённый интерфейс)
	 * arr - указатель на массив
	 * n - количество элементов
	 */
	void quick_sort_simple(int arr[], int n);

#ifdef __cplusplus
}
#endif

#endif /* QUICK_SORT_H */