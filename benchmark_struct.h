#ifndef BENCHMARK_STRUCT_H
#define BENCHMARK_STRUCT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Подключаем структуры данных */
#include "hash_table.h"
#include "rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * Высокоточный таймер в наносекундах
	 * Возвращает текущее время в наносекундах
	 */
	uint64_t get_time_ns(void);

	/*
	 * Генерация массива случайных ключей
	 * n - количество ключей
	 * max_value - максимальное значение ключа
	 * Возвращает указатель на массив ключей
	 */
	int* generate_keys(int n, int max_value);

	/*
	 * Проверка корректности работы структур данных
	 * keys - массив ключей
	 * n - количество ключей
	 * Возвращает true если всё работает корректно
	 */
	bool verify_structures_correctness(int* keys, int n);

	/*
	 * Бенчмарк вставки в структуры данных
	 * ht - хеш-таблица
	 * rb - красно-чёрное дерево
	 * keys - массив ключей
	 * n - количество элементов
	 */
	void benchmark_insert(HashTable* ht, RBTree* rb, int* keys, int n);

	/*
	 * Бенчмарк поиска в структурах данных
	 */
	void benchmark_search(HashTable* ht, RBTree* rb, int* keys, int n);

	/*
	 * Бенчмарк удаления из структур данных
	 */
	void benchmark_delete(HashTable* ht, RBTree* rb, int* keys, int n);

	/*
	 * Полный бенчмарк для заданного размера
	 * n - количество элементов
	 */
	void benchmark_structures_size(int n);

	/*
	 * Анализ скрытых констант структур данных
	 */
	void analyze_structures_constants(void);

	/*
	 * Тест с разными типами данных
	 * Демонстрирует универсальность структур
	 */
	void test_different_types(void);

#ifdef __cplusplus
}
#endif

#endif /* BENCHMARK_STRUCT_H */