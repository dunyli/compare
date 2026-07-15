#ifndef INSERTION_SORT_H
#define INSERTION_SORT_H

/*
 * Сортировка вставками
 * Сложность: O(n^2) в худшем и среднем, O(n) в лучшем (почти отсортирован)
 * Эффективна для малых массивов (n < 100)
 */

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * Сортировка массива целых чисел методом вставками
	 * arr - указатель на массив
	 * n - количество элементов
	 */
	void insertion_sort(int arr[], int n);

	/*
	 * Сортировка части массива методом вставками
	 * arr - указатель на массив
	 * low - начальный индекс
	 * high - конечный индекс
	 */
	void insertion_sort_range(int arr[], int low, int high);

#ifdef __cplusplus
}
#endif

#endif /* INSERTION_SORT_H */