#define _CRT_SECURE_NO_WARNINGS  /* Отключаем предупреждения о безопасности в Windows */

#define _POSIX_C_SOURCE 200809L  /* Включаем POSIX-функции */

/* Для Windows определяем strdup как _strdup */
#ifdef _WIN32
#define strdup _strdup
#endif

#include "hash_table.h"          /* Подключаем заголовочный файл с объявлениями */
#include <stdlib.h>              /* Для malloc, free, calloc */
#include <string.h>              /* Для memcpy, memcmp, strlen */
#include <stdio.h>               /* Для printf (отладка) */
#include <math.h>                /* Для математических операций */

/* ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===== */

/*
 * Создание нового узла хеш-таблицы
 * Параметры:
 *   - key: указатель на данные ключа
 *   - key_size: размер ключа в байтах
 *   - value: указатель на данные значения
 *   - value_size: размер значения в байтах
 *   - copy_key: функция копирования ключа (может быть NULL)
 *   - copy_value: функция копирования значения (может быть NULL)
 * Возвращает: указатель на созданный узел или NULL при ошибке
 */
static HashNode* ht_create_node(const void* key, size_t key_size,
    const void* value, size_t value_size,
    CopyFunc copy_key, CopyFunc copy_value) {
    /* Выделяем память под структуру узла */
    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) return NULL;  /* Проверка: если память не выделилась - возвращаем NULL */

    /* === КОПИРОВАНИЕ КЛЮЧА === */
    /* Если передана функция копирования - используем её, иначе - выделяем память сами */
    node->key = copy_key ? copy_key(key, key_size) : malloc(key_size);
    if (!node->key) {        /* Проверка: если память для ключа не выделилась */
        free(node);          /* Освобождаем уже выделенную память для узла */
        return NULL;         /* Возвращаем NULL */
    }
    /* Если функция копирования не использовалась - копируем данные вручную */
    if (!copy_key) memcpy(node->key, key, key_size);
    node->key_size = key_size;  /* Сохраняем размер ключа */

    /* === КОПИРОВАНИЕ ЗНАЧЕНИЯ === */
    /* Аналогично копируем значение */
    node->value = copy_value ? copy_value(value, value_size) : malloc(value_size);
    if (!node->value) {      /* Проверка: если память для значения не выделилась */
        /* Если есть функция освобождения ключа - используем её */
        if (copy_key) {
            /* В этой реализации предполагаем, что copy_key сама управляет памятью */
            /* В противном случае просто free */
            free(node->key);
        }
        else {
            free(node->key);  /* Освобождаем память ключа */
        }
        free(node);           /* Освобождаем память узла */
        return NULL;          /* Возвращаем NULL */
    }
    /* Если функция копирования не использовалась - копируем данные вручную */
    if (!copy_value) memcpy(node->value, value, value_size);
    node->value_size = value_size;  /* Сохраняем размер значения */

    node->next = NULL;  /* Инициализируем указатель на следующий узел как NULL */

    return node;  /* Возвращаем созданный узел */
}

/*
 * Уничтожение одного узла и освобождение всей связанной с ним памяти
 * Параметры:
 *   - node: указатель на узел для уничтожения
 *   - free_key: функция освобождения ключа (может быть NULL)
 *   - free_value: функция освобождения значения (может быть NULL)
 */
static void ht_destroy_node(HashNode* node, FreeFunc free_key, FreeFunc free_value) {
    if (!node) return;  /* Если узел NULL - ничего не делаем */

    /* === ОСВОБОЖДЕНИЕ КЛЮЧА === */
    /* Если передана функция освобождения - используем её, иначе - просто free */
    if (free_key) {
        free_key(node->key);  /* Используем пользовательскую функцию освобождения */
    }
    else {
        free(node->key);      /* Стандартное освобождение */
    }

    /* === ОСВОБОЖДЕНИЕ ЗНАЧЕНИЯ === */
    /* Аналогично освобождаем значение */
    if (free_value) {
        free_value(node->value);  /* Используем пользовательскую функцию освобождения */
    }
    else {
        free(node->value);        /* Стандартное освобождение */
    }

    free(node);  /* Освобождаем сам узел */
}

/*
 * Подсчёт длины цепочки (количество узлов в связанном списке)
 * Параметры:
 *   - head: указатель на первый узел цепочки
 * Возвращает: количество узлов в цепочке
 */
static uint32_t ht_chain_length(HashNode* head) {
    uint32_t len = 0;  /* Счётчик узлов */
    while (head) {     /* Пока не достигнут конец списка */
        len++;         /* Увеличиваем счётчик */
        head = head->next;  /* Переходим к следующему узлу */
    }
    return len;  /* Возвращаем длину цепочки */
}

/*
 * Поиск узла в цепочке по ключу
 * Параметры:
 *   - head: указатель на начало цепочки
 *   - key: указатель на данные ключа для поиска
 *   - key_size: размер ключа в байтах
 *   - compare: функция сравнения ключей
 * Возвращает: указатель на найденный узел или NULL
 */
static HashNode* ht_find_node_in_chain(HashNode* head, const void* key,
    size_t key_size, CompareFunc compare) {
    HashNode* curr = head;  /* Начинаем с головы цепочки */
    while (curr) {          /* Пока не достигнут конец цепочки */
        /* Сравниваем размеры и содержимое ключей */
        if (curr->key_size == key_size && compare(curr->key, key) == 0) {
            return curr;    /* Нашли - возвращаем указатель на узел */
        }
        curr = curr->next;  /* Переходим к следующему узлу */
    }
    return NULL;  /* Не нашли - возвращаем NULL */
}

/*
 * Проверка необходимости расширения таблицы
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 * Возвращает: true если нужно расширить, false если нет
 */
static bool ht_should_resize(HashTable* ht) {
    if (!ht) return false;

    /* === ПРОВЕРКА ЗАПОЛНЕНИЯ === */
    /* Расширяем при заполнении > 75% для поддержания хорошей производительности */
    double load_factor = (double)ht->count / ht->size;

    /* Если заполнение > 75% - расширяем */
    if (load_factor > 0.75) return true;

    /* Если заполнение > 50%, проверяем цепочки (но не все, а только некоторые) */
    if (load_factor > 0.50) {
        /* Проверяем только 5 случайных цепочек для экономии времени */
        for (int i = 0; i < 5; i++) {
            uint32_t idx = rand() % ht->size;
            if (ht_chain_length(ht->buckets[idx]) > ht->max_chain_length) {
                return true;
            }
        }
    }

    return false;
}

/* ===== ОСНОВНЫЕ ФУНКЦИИ ===== */

/*
 * Создание новой хеш-таблицы
 * Параметры:
 *   - initial_size: начальный размер таблицы (количество ячеек)
 *   - hash_func: функция хеширования
 *   - compare_func: функция сравнения ключей
 *   - copy_key_func: функция копирования ключа (может быть NULL)
 *   - copy_value_func: функция копирования значения (может быть NULL)
 *   - free_key_func: функция освобождения ключа (может быть NULL)
 *   - free_value_func: функция освобождения значения (может быть NULL)
 *   - max_chain_length: максимальная длина цепочки для автоматического расширения
 * Возвращает: указатель на созданную таблицу или NULL при ошибке
 */
HashTable* ht_create(uint32_t initial_size, HashFunc hash_func,
    CompareFunc compare_func, CopyFunc copy_key_func,
    CopyFunc copy_value_func, FreeFunc free_key_func,
    FreeFunc free_value_func, uint32_t max_chain_length) {

    /* Проверяем обязательные параметры */
    if (initial_size == 0 || !hash_func || !compare_func) return NULL;

    /* Выделяем память под структуру таблицы */
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) return NULL;  /* Проверка выделения памяти */

    /* Инициализируем поля структуры */
    ht->size = initial_size;           /* Устанавливаем размер таблицы */
    ht->count = 0;                     /* Изначально элементов нет */
    /* Если максимальная длина не задана - используем значение по умолчанию (8) */
    ht->max_chain_length = (max_chain_length > 0) ? max_chain_length : 8;

    /* Сохраняем указатели на пользовательские функции */
    ht->hash_func = hash_func;         /* Функция хеширования */
    ht->compare_func = compare_func;   /* Функция сравнения */
    ht->copy_key_func = copy_key_func; /* Функция копирования ключа */
    ht->copy_value_func = copy_value_func; /* Функция копирования значения */
    ht->free_key_func = free_key_func; /* Функция освобождения ключа */
    ht->free_value_func = free_value_func; /* Функция освобождения значения */

    /* Выделяем память под массив указателей на цепочки */
    ht->buckets = (HashNode**)calloc(initial_size, sizeof(HashNode*));
    if (!ht->buckets) {  /* Проверка выделения памяти */
        free(ht);        /* Освобождаем уже выделенную память для структуры */
        return NULL;     /* Возвращаем NULL */
    }

    return ht;  /* Возвращаем созданную таблицу */
}

/*
 * Полное уничтожение хеш-таблицы и освобождение всей памяти
 * Параметры:
 *   - ht: указатель на хеш-таблицу для уничтожения
 */
void ht_destroy(HashTable* ht) {
    if (!ht) return;  /* Если таблица NULL - ничего не делаем */

    ht_clear(ht);     /* Очищаем все элементы таблицы */
    free(ht->buckets); /* Освобождаем массив указателей */
    free(ht);          /* Освобождаем структуру таблицы */
}

/*
 * Очистка таблицы (удаление всех элементов)
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 */
void ht_clear(HashTable* ht) {
    if (!ht) return;  /* Если таблица NULL - ничего не делаем */

    /* Проходим по всем ячейкам таблицы */
    for (uint32_t i = 0; i < ht->size; i++) {
        HashNode* curr = ht->buckets[i];  /* Начинаем с головы цепочки */
        while (curr) {                    /* Пока не достигнут конец цепочки */
            HashNode* next = curr->next;  /* Сохраняем указатель на следующий узел */
            /* Уничтожаем текущий узел с использованием функций освобождения */
            ht_destroy_node(curr, ht->free_key_func, ht->free_value_func);
            curr = next;  /* Переходим к следующему узлу */
        }
        ht->buckets[i] = NULL;  /* Обнуляем указатель на цепочку */
    }
    ht->count = 0;  /* Сбрасываем счётчик элементов */
}

/*
 * Вставка или обновление элемента в хеш-таблице
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - key: указатель на данные ключа
 *   - key_size: размер ключа в байтах
 *   - value: указатель на данные значения
 *   - value_size: размер значения в байтах
 * Возвращает: true при успехе, false при ошибке
 */
bool ht_insert(HashTable* ht, const void* key, size_t key_size,
    const void* value, size_t value_size) {

    if (!ht || !key || !value) return false;

    /*
     * АВТОМАТИЧЕСКОЕ РАСШИРЕНИЕ
     * Расширяем только когда count >= size (заполнение 100%)
     * Это даёт оптимальный баланс скорости и памяти
     */
    if (ht->count >= ht->size) {
        /* Удваиваем размер */
        uint32_t new_size = ht->size * 2;
        ht_resize(ht, new_size);
    }

    /* === ВЫЧИСЛЕНИЕ ИНДЕКСА === */
    uint32_t index = ht->hash_func(key, key_size, ht->size);

    /* === ПОИСК СУЩЕСТВУЮЩЕГО КЛЮЧА === */
    HashNode* curr = ht->buckets[index];
    while (curr) {
        if (curr->key_size == key_size && ht->compare_func(curr->key, key) == 0) {
            /* Обновляем значение */
            if (ht->free_value_func) {
                ht->free_value_func(curr->value);
            }
            else {
                free(curr->value);
            }

            curr->value = ht->copy_value_func ?
                ht->copy_value_func(value, value_size) :
                malloc(value_size);
            if (!curr->value) return false;
            if (!ht->copy_value_func) memcpy(curr->value, value, value_size);
            curr->value_size = value_size;
            return true;
        }
        curr = curr->next;
    }

    /* === СОЗДАНИЕ НОВОГО УЗЛА === */
    HashNode* node = ht_create_node(key, key_size, value, value_size,
        ht->copy_key_func, ht->copy_value_func);
    if (!node) return false;

    /* === ВСТАВКА В НАЧАЛО ЦЕПОЧКИ === */
    node->next = ht->buckets[index];
    ht->buckets[index] = node;
    ht->count++;

    return true;
}

/*
 * Поиск значения по ключу
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - key: указатель на данные ключа
 *   - key_size: размер ключа в байтах
 *   - out_value: указатель для сохранения найденного значения (может быть NULL)
 *   - out_value_size: указатель для сохранения размера значения (может быть NULL)
 * Возвращает: true если ключ найден, false если нет
 */
bool ht_search(HashTable* ht, const void* key, size_t key_size,
    void* out_value, size_t* out_value_size) {
    /* Проверяем корректность параметров */
    if (!ht || !key) return false;

    /* Вычисляем хеш ключа и получаем индекс */
    uint32_t index = ht->hash_func(key, key_size, ht->size);

    /* Ищем узел в соответствующей цепочке */
    HashNode* node = ht_find_node_in_chain(ht->buckets[index], key,
        key_size, ht->compare_func);

    if (!node) return false;  /* Ключ не найден */

    /* Если передан указатель для сохранения значения - копируем данные */
    if (out_value) {
        memcpy(out_value, node->value, node->value_size);  /* Копируем значение */
    }
    /* Если передан указатель для сохранения размера - сохраняем его */
    if (out_value_size) {
        *out_value_size = node->value_size;
    }

    return true;  /* Ключ найден */
}

/*
 * Удаление элемента по ключу
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - key: указатель на данные ключа
 *   - key_size: размер ключа в байтах
 * Возвращает: true если ключ найден и удалён, false если нет
 */
bool ht_delete(HashTable* ht, const void* key, size_t key_size) {
    /* Проверяем корректность параметров */
    if (!ht || !key) return false;

    /* Вычисляем хеш ключа и получаем индекс */
    uint32_t index = ht->hash_func(key, key_size, ht->size);

    /* Начинаем обход цепочки с головы */
    HashNode* curr = ht->buckets[index];
    HashNode* prev = NULL;  /* Указатель на предыдущий узел (для корректировки связей) */

    /* Проходим по цепочке в поисках ключа */
    while (curr) {
        /* Сравниваем ключи: размер и содержимое */
        if (curr->key_size == key_size && ht->compare_func(curr->key, key) == 0) {
            /* === НАШЛИ УЗЕЛ ДЛЯ УДАЛЕНИЯ === */
            if (prev) {
                /* Узел в середине или конце цепочки */
                prev->next = curr->next;  /* Обходим удаляемый узел */
            }
            else {
                /* Узел в начале цепочки (голова) */
                ht->buckets[index] = curr->next;  /* Новой головой становится следующий узел */
            }

            /* Освобождаем память удалённого узла */
            ht_destroy_node(curr, ht->free_key_func, ht->free_value_func);
            ht->count--;  /* Уменьшаем счётчик элементов */

            return true;  /* Успешное удаление */
        }
        /* Переходим к следующему узлу */
        prev = curr;
        curr = curr->next;
    }

    return false;  /* Ключ не найден */
}

/*
 * Проверка наличия ключа в таблице
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - key: указатель на данные ключа
 *   - key_size: размер ключа в байтах
 * Возвращает: true если ключ существует, false если нет
 */
bool ht_contains(HashTable* ht, const void* key, size_t key_size) {
    /* Используем функцию поиска, игнорируя результат (только факт наличия) */
    return ht_search(ht, key, key_size, NULL, NULL);
}

/*
 * Получение количества элементов в таблице
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 * Возвращает: количество элементов
 */
uint32_t ht_size(HashTable* ht) {
    return ht ? ht->count : 0;  /* Если таблица NULL - возвращаем 0 */
}

/*
 * Проверка, пуста ли таблица
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 * Возвращает: true если пуста, false если нет
 */
bool ht_is_empty(HashTable* ht) {
    return ht ? (ht->count == 0) : true;  /* Если таблица NULL - считаем пустой */
}

/*
 * Расширение таблицы (изменение размера)
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - new_size: новый размер таблицы
 * Возвращает: true при успехе, false при ошибке
 */
bool ht_resize(HashTable* ht, uint32_t new_size) {
    if (!ht || new_size < ht->count) return false;  /* Проверка параметров */

    /* Сохраняем старые данные */
    HashNode** old_buckets = ht->buckets;  /* Старый массив указателей */
    uint32_t old_size = ht->size;          /* Старый размер таблицы */

    /* Создаём новый массив указателей */
    HashNode** new_buckets = (HashNode**)calloc(new_size, sizeof(HashNode*));
    if (!new_buckets) return false;  /* Проверка выделения памяти */

    /* Обновляем указатели в структуре */
    ht->buckets = new_buckets;  /* Устанавливаем новый массив */
    ht->size = new_size;        /* Обновляем размер */
    ht->count = 0;              /* Временно обнуляем счётчик (будем пересчитывать) */

    /* === ПЕРЕХЕШИРОВАНИЕ ВСЕХ ЭЛЕМЕНТОВ === */
    /* Проходим по всем старым ячейкам */
    for (uint32_t i = 0; i < old_size; i++) {
        HashNode* curr = old_buckets[i];  /* Начинаем с головы цепочки */
        while (curr) {
            HashNode* next = curr->next;  /* Сохраняем указатель на следующий узел */

            /* Вычисляем новый индекс для этого узла */
            uint32_t new_index = ht->hash_func(curr->key, curr->key_size, new_size);

            /* Вставляем узел в начало новой цепочки (быстрее) */
            curr->next = ht->buckets[new_index];  /* Связываем с текущей головой */
            ht->buckets[new_index] = curr;        /* Устанавливаем новый узел как голову */
            ht->count++;                          /* Увеличиваем счётчик */

            curr = next;  /* Переходим к следующему узлу */
        }
    }

    free(old_buckets);  /* Освобождаем старый массив указателей */
    return true;  /* Успешное расширение */
}

/*
 * Уплотнение таблицы (уменьшение размера при малом заполнении)
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 */
void ht_compact(HashTable* ht) {
    if (!ht) return;  /* Проверка параметров */

    /* Вычисляем коэффициент заполнения */
    double load_factor = (double)ht->count / ht->size;

    /* Если заполнение меньше 25% и размер больше 64 - уменьшаем */
    if (load_factor < 0.25 && ht->size > 64) {
        uint32_t new_size = ht->size / 2;  /* Уменьшаем вдвое */
        /* Убеждаемся, что новый размер не меньше количества элементов */
        if (new_size < ht->count) new_size = ht->count * 2;
        /* Пытаемся изменить размер */
        ht_resize(ht, new_size);
    }
}

/*
 * Получение статистики таблицы
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - num_buckets: указатель для сохранения количества ячеек (может быть NULL)
 *   - num_elements: указатель для сохранения количества элементов (может быть NULL)
 *   - max_chain: указатель для сохранения максимальной длины цепочки (может быть NULL)
 *   - avg_chain: указатель для сохранения средней длины цепочки (может быть NULL)
 *   - load_factor: указатель для сохранения коэффициента заполнения (может быть NULL)
 */
void ht_stats(HashTable* ht, uint32_t* num_buckets, uint32_t* num_elements,
    uint32_t* max_chain, double* avg_chain, double* load_factor) {
    if (!ht) return;  /* Проверка параметров */

    /* Сохраняем основные параметры, если указатели не NULL */
    if (num_buckets) *num_buckets = ht->size;
    if (num_elements) *num_elements = ht->count;
    if (load_factor) *load_factor = (double)ht->count / ht->size;

    /* Вычисляем статистику по цепочкам */
    uint32_t max_len = 0;      /* Максимальная длина цепочки */
    uint32_t total_len = 0;    /* Общая длина всех цепочек */
    uint32_t non_empty = 0;    /* Количество непустых ячеек */

    /* Проходим по всем ячейкам */
    for (uint32_t i = 0; i < ht->size; i++) {
        uint32_t len = ht_chain_length(ht->buckets[i]);  /* Длина текущей цепочки */
        if (len > 0) {          /* Если цепочка не пуста */
            non_empty++;        /* Увеличиваем счётчик непустых ячеек */
            total_len += len;   /* Добавляем к общей длине */
            if (len > max_len) max_len = len;  /* Обновляем максимум */
        }
    }

    /* Сохраняем результаты, если указатели не NULL */
    if (max_chain) *max_chain = max_len;
    if (avg_chain) *avg_chain = non_empty ? (double)total_len / non_empty : 0.0;
}

/*
 * Обход всех элементов таблицы с передачей пользовательским данным
 * Параметры:
 *   - ht: указатель на хеш-таблицу
 *   - callback: функция-колбэк, вызываемая для каждого элемента
 *   - user_data: пользовательские данные, передаваемые в колбэк
 */
void ht_foreach(HashTable* ht, HashTableCallback callback, void* user_data) {
    if (!ht || !callback) return;  /* Проверка параметров */

    /* Проходим по всем ячейкам таблицы */
    for (uint32_t i = 0; i < ht->size; i++) {
        HashNode* curr = ht->buckets[i];  /* Начинаем с головы цепочки */
        while (curr) {                    /* Пока не достигнут конец цепочки */
            /* Вызываем колбэк для текущего узла */
            callback(curr->key, curr->key_size,
                curr->value, curr->value_size,
                user_data);
            curr = curr->next;  /* Переходим к следующему узлу */
        }
    }
}

/* ===== СТАНДАРТНЫЕ ХЕШ-ФУНКЦИИ ===== */

/*
 * Хеш-функция для целых чисел (int)
 * Использует смешанный алгоритм для хорошего распределения
 */
uint32_t hash_int(const void* data, size_t size, uint32_t table_size) {
    (void)size;  /* Игнорируем размер, т.к. int фиксированного размера */
    uint32_t key = *(uint32_t*)data;  /* Преобразуем данные в 32-битное число */

    /* === СМЕШАННЫЙ ХЕШ ДЛЯ INT === */
    /* Алгоритм: смешивание бит для улучшения распределения */
    key = ((key >> 16) ^ key) * 0x45d9f3b;  /* Сдвиг, XOR и умножение */
    key = ((key >> 16) ^ key) * 0x45d9f3b;  /* Повтор для лучшего перемешивания */
    key = (key >> 16) ^ key;                /* Финальное XOR-смешивание */

    return key % table_size;  /* Приводим к размеру таблицы */
}

/*
 * Хеш-функция для строк (алгоритм FNV-1a)
 * Быстрый и надёжный алгоритм для строковых ключей
 */
uint32_t hash_string(const void* data, size_t size, uint32_t table_size) {
    /* FNV-1a алгоритм: простое, быстрое, хорошее распределение */
    const unsigned char* str = (const unsigned char*)data;  /* Указатель на строку */
    uint32_t hash = 2166136261U;  /* Начальное значение (offset_basis) */

    /* Проходим по каждому байту строки */
    for (size_t i = 0; i < size && str[i] != '\0'; i++) {
        hash ^= (uint32_t)str[i];          /* XOR с текущим байтом */
        hash *= 16777619U;                /* Умножение на простое число (FNV_prime) */
    }

    return hash % table_size;  /* Приводим к размеру таблицы */
}

/*
 * Хеш-функция для произвольных байтовых данных
 * Использует FNV-1a для бинарных данных
 */
uint32_t hash_bytes(const void* data, size_t size, uint32_t table_size) {
    const unsigned char* bytes = (const unsigned char*)data;  /* Указатель на данные */
    uint32_t hash = 2166136261U;  /* Начальное значение */

    /* Проходим по всем байтам данных */
    for (size_t i = 0; i < size; i++) {
        hash ^= (uint32_t)bytes[i];  /* XOR с текущим байтом */
        hash *= 16777619U;          /* Умножение на простое число */
    }

    return hash % table_size;  /* Приводим к размеру таблицы */
}

/*
 * Хеш-функция для указателей
 * Использует значение указателя как хеш
 */
uint32_t hash_pointer(const void* data, size_t size, uint32_t table_size) {
    (void)size;  /* Игнорируем размер */
    /* Преобразуем указатель в 32-битное значение */
    uintptr_t ptr = (uintptr_t)data;
    uint32_t hash = (uint32_t)(ptr ^ (ptr >> 32));  /* XOR старшей и младшей частей */

    /* Перемешивание бит для лучшего распределения */
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = (hash >> 16) ^ hash;

    return hash % table_size;
}

/* ===== СТАНДАРТНЫЕ ФУНКЦИИ СРАВНЕНИЯ ===== */

/*
 * Сравнение двух целых чисел (int)
 * Возвращает: 0 если равны, положительное если a > b, отрицательное если a < b
 */
int compare_int(const void* a, const void* b) {
    int ia = *(int*)a;  /* Приводим к int */
    int ib = *(int*)b;
    return (ia > ib) - (ia < ib);  /* Возвращаем -1, 0 или 1 */
}

/*
 * Сравнение двух строк
 * Использует стандартную функцию strcmp
 */
int compare_string(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}

/*
 * Сравнение произвольных байтовых данных
 * Использует memcmp для побайтового сравнения
 */
int compare_bytes(const void* a, const void* b) {
    /* Сравниваем по размеру указателя (упрощённо) */
    return memcmp(a, b, sizeof(void*));
}

/*
 * Сравнение двух указателей
 * Сравнивает значения указателей
 */
int compare_pointer(const void* a, const void* b) {
    uintptr_t pa = (uintptr_t)a;  /* Приводим к целочисленному типу */
    uintptr_t pb = (uintptr_t)b;
    return (pa > pb) - (pa < pb);  /* Возвращаем -1, 0 или 1 */
}

/* ===== СТАНДАРТНЫЕ ФУНКЦИИ КОПИРОВАНИЯ ===== */

/*
 * Копирование целого числа (int)
 * Выделяет память и копирует значение
 */
void* copy_int(const void* data, size_t size) {
    (void)size;  /* Игнорируем размер */
    int* new_int = (int*)malloc(sizeof(int));  /* Выделяем память */
    if (new_int) *new_int = *(int*)data;       /* Копируем значение */
    return new_int;  /* Возвращаем указатель на копию */
}

/*
 * Копирование строки
 * Использует strdup для копирования строки
 */
void* copy_string(const void* data, size_t size) {
    (void)size;  /* Игнорируем размер */
    return strdup((const char*)data);  /* strdup выделяет память и копирует строку */
}

/*
 * Копирование произвольных байтовых данных
 * Выделяет память и копирует указанное количество байт
 */
void* copy_bytes(const void* data, size_t size) {
    void* new_data = malloc(size);  /* Выделяем память */
    if (new_data) memcpy(new_data, data, size);  /* Копируем данные */
    return new_data;  /* Возвращаем указатель на копию */
}

/*
 * Копирование указателя
 * Выделяет память и копирует значение указателя
 */
void* copy_pointer(const void* data, size_t size) {
    (void)size;  /* Игнорируем размер */
    void** new_ptr = (void**)malloc(sizeof(void*));  /* Выделяем память */
    if (new_ptr) *new_ptr = *(void**)data;  /* Копируем указатель */
    return new_ptr;  /* Возвращаем указатель на копию */
}

/* ===== СТАНДАРТНЫЕ ФУНКЦИИ ОСВОБОЖДЕНИЯ ===== */

/*
 * Освобождение памяти для целого числа
 */
void free_int(void* data) {
    free(data);  /* Просто освобождаем память */
}

/*
 * Освобождение памяти для строки
 */
void free_string(void* data) {
    free(data);  /* Просто освобождаем память */
}

/*
 * Освобождение памяти для произвольных байтов
 */
void free_bytes(void* data) {
    free(data);  /* Просто освобождаем память */
}

/*
 * Освобождение памяти для указателя
 */
void free_pointer(void* data) {
    free(data);  /* Просто освобождаем память */
}