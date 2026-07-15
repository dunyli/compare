#ifndef BENCHMARK_ALGORITHM_H
#define BENCHMARK_ALGORITHM_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Сравнение алгоритмов сортировки
 * Быстрая сортировка (O(n log n)) vs Сортировка вставками (O(n^2))
 */

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * Проверка корректности алгоритмов сортировки
	 * Возвращает true если всё работает правильно
	 */
	bool verify_sorting_correctness(void);

	/*
	 * Бенчмарк алгоритмов сортировки
	 * Сравнивает быструю сортировку и сортировку вставками
	 * Выводит время выполнения для разных размеров массивов
	 */
	void benchmark_sorting_algorithms(void);

	/*
	 * Генерация случайного массива для тестирования
	 * n - размер массива
	 * max_value - максимальное значение элемента
	 * Возвращает указатель на массив
	 */
	int* generate_random_array(int n, int max_value);

	/*
	 * Копирование массива
	 * Возвращает указатель на копию
	 */
	int* copy_array(int arr[], int n);

	/*
	 * Проверка, отсортирован ли массив
	 * Возвращает true если массив отсортирован по возрастанию
	 */
	bool is_sorted(int arr[], int n);

#ifdef __cplusplus
}
#endif

#endif /* BENCHMARK_ALGORITHM_H */