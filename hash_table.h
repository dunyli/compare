#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Тип для хеш-функции - принимает указатель на данные и их размер */
typedef uint32_t(*HashFunc)(const void* data, size_t size, uint32_t table_size);

/* Тип для функции сравнения ключей */
typedef int (*CompareFunc)(const void* a, const void* b);

/* Тип для функции копирования ключа */
typedef void* (*CopyFunc)(const void* data, size_t size);

/* Тип для функции освобождения ключа */
typedef void (*FreeFunc)(void* data);

/* Тип для функции-колбэка при обходе таблицы */
typedef void (*HashTableCallback)(const void* key, size_t key_size,
    const void* value, size_t value_size,
    void* user_data);

/* Узел цепочки - хранит ключ и значение любого типа */
typedef struct HashNode {
    void* key;                  /* Указатель на ключ (любого типа) */
    size_t key_size;            /* Размер ключа в байтах */
    void* value;                /* Указатель на значение (любого типа) */
    size_t value_size;          /* Размер значения в байтах */
    struct HashNode* next;      /* Указатель на следующий узел */
} HashNode;

/* Структура хеш-таблицы */
typedef struct {
    HashNode** buckets;         /* Массив указателей на цепочки */
    uint32_t size;              /* Текущий размер таблицы */
    uint32_t count;             /* Количество элементов */
    uint32_t max_chain_length;  /* Максимальная длина цепочки для расширения */

    /* Функции для работы с данными */
    HashFunc hash_func;         /* Хеш-функция */
    CompareFunc compare_func;   /* Функция сравнения ключей */
    CopyFunc copy_key_func;     /* Функция копирования ключа */
    CopyFunc copy_value_func;   /* Функция копирования значения */
    FreeFunc free_key_func;     /* Функция освобождения ключа */
    FreeFunc free_value_func;   /* Функция освобождения значения */
} HashTable;

#ifdef __cplusplus
extern "C" {
#endif

    /* Создание хеш-таблицы */
    HashTable* ht_create(
        uint32_t initial_size,
        HashFunc hash_func,
        CompareFunc compare_func,
        CopyFunc copy_key_func,
        CopyFunc copy_value_func,
        FreeFunc free_key_func,
        FreeFunc free_value_func,
        uint32_t max_chain_length
    );

    /* Вставка элемента */
    bool ht_insert(HashTable* ht, const void* key, size_t key_size,
        const void* value, size_t value_size);

    /* Поиск элемента */
    bool ht_search(HashTable* ht, const void* key, size_t key_size,
        void* out_value, size_t* out_value_size);

    /* Удаление элемента */
    bool ht_delete(HashTable* ht, const void* key, size_t key_size);

    /* Проверка наличия ключа */
    bool ht_contains(HashTable* ht, const void* key, size_t key_size);

    /* Получение размера */
    uint32_t ht_size(HashTable* ht);

    /* Проверка, пуста ли таблица */
    bool ht_is_empty(HashTable* ht);

    /* Очистка таблицы */
    void ht_clear(HashTable* ht);

    /* Уничтожение таблицы */
    void ht_destroy(HashTable* ht);

    /* Расширение таблицы */
    bool ht_resize(HashTable* ht, uint32_t new_size);

    /* Уплотнение таблицы */
    void ht_compact(HashTable* ht);

    /* Статистика таблицы */
    void ht_stats(HashTable* ht, uint32_t* num_buckets, uint32_t* num_elements,
        uint32_t* max_chain, double* avg_chain, double* load_factor);

    /* Обход таблицы с колбэком */
    void ht_foreach(HashTable* ht, HashTableCallback callback, void* user_data);

    /* Стандартные хеш-функции для разных типов */
    uint32_t hash_int(const void* data, size_t size, uint32_t table_size);
    uint32_t hash_string(const void* data, size_t size, uint32_t table_size);
    uint32_t hash_bytes(const void* data, size_t size, uint32_t table_size);
    uint32_t hash_pointer(const void* data, size_t size, uint32_t table_size);

    /* Стандартные функции сравнения */
    int compare_int(const void* a, const void* b);
    int compare_string(const void* a, const void* b);
    int compare_bytes(const void* a, const void* b);
    int compare_pointer(const void* a, const void* b);

    /* Стандартные функции копирования */
    void* copy_int(const void* data, size_t size);
    void* copy_string(const void* data, size_t size);
    void* copy_bytes(const void* data, size_t size);
    void* copy_pointer(const void* data, size_t size);

    /* Стандартные функции освобождения */
    void free_int(void* data);
    void free_string(void* data);
    void free_bytes(void* data);
    void free_pointer(void* data);

#ifdef __cplusplus
}
#endif

#endif /* HASH_TABLE_H */