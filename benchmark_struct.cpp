/*
 * benchmark_struct.c
 * Реализация функций для сравнения хеш-таблицы и красно-чёрного дерева
 * Содержит все функции бенчмаркинга для структур данных
 */

#include "benchmark_struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

 /* ============================================================
  *                   1. ВЫСОКОТОЧНЫЙ ТАЙМЕР
  * ============================================================ */

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <sys/time.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif

  /*
   * Получение текущего времени в наносекундах
   * Для Windows использует QueryPerformanceCounter
   * Для Linux использует clock_gettime
   */
uint64_t get_time_ns(void) {
#ifdef _WIN32
    /* Windows: используем высокоточный счётчик производительности */
    LARGE_INTEGER frequency;  /* Частота счётчика (тиков в секунду) */
    LARGE_INTEGER counter;    /* Текущее значение счётчика */

    /* Получаем частоту счётчика */
    if (!QueryPerformanceFrequency(&frequency)) {
        return 0;
    }
    /* Получаем текущее значение счётчика */
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    /* Переводим в наносекунды: (тики * 1e9) / частота */
    return (uint64_t)((double)counter.QuadPart * 1000000000.0 / frequency.QuadPart);

#elif __linux__
    /* Linux: используем монотонные часы */
    struct timespec ts;

    /* CLOCK_MONOTONIC - часы, которые не изменяются при настройке системы */
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    /* Переводим в наносекунды: секунды * 1e9 + наносекунды */
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#else
#error "Unsupported platform"
    return 0;
#endif
}

/* ============================================================
 *                   2. ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
 * ============================================================ */

 /*
  * Генерация массива случайных ключей
  * Создаёт массив целых чисел для использования в тестах
  */
int* generate_keys(int n, int max_value) {
    /* Выделяем память под массив */
    int* keys = (int*)malloc(n * sizeof(int));
    if (!keys) return NULL;

    /* Заполняем массив случайными числами в диапазоне [0, max_value-1] */
    for (int i = 0; i < n; i++) {
        keys[i] = rand() % max_value;
    }

    return keys;
}

/* ============================================================
 *                   3. ПРОВЕРКА КОРРЕКТНОСТИ
 * ============================================================ */

 /*
  * Проверка корректности работы структур данных
  * Выполняет вставку, поиск и удаление, проверяет результаты
  */
bool verify_structures_correctness(int* keys, int n) {
    /* Создаём хеш-таблицу с начальным размером 1024 */
    HashTable* ht = ht_create(
        1024,           /* начальный размер */
        hash_int,       /* хеш-функция для int */
        compare_int,    /* сравнение int */
        copy_int,       /* копирование ключа */
        copy_int,       /* копирование значения */
        free_int,       /* освобождение ключа */
        free_int,       /* освобождение значения */
        8               /* максимальная длина цепочки */
    );
    if (!ht) return false;

    /* Создаём красно-чёрное дерево */
    RBTree* rb = rb_create(
        compare_int,    /* сравнение int */
        copy_int,       /* копирование ключа */
        copy_int,       /* копирование значения */
        free_int,       /* освобождение ключа */
        free_int        /* освобождение значения */
    );
    if (!rb) {
        ht_destroy(ht);
        return false;
    }

    bool ok = true;
    int value;
    size_t size;

    /* Шаг 1: Вставка элементов */
    for (int i = 0; i < n && ok; i++) {
        /* Вставляем в хеш-таблицу: ключ = keys[i], значение = i */
        if (!ht_insert(ht, &keys[i], sizeof(int), &i, sizeof(int))) ok = false;
        /* Вставляем в КЧД: ключ = keys[i], значение = i */
        if (!rb_insert(rb, &keys[i], sizeof(int), &i, sizeof(int))) ok = false;
    }

    /* Шаг 2: Поиск элементов */
    for (int i = 0; i < n && ok; i++) {
        /* Ищем в хеш-таблице */
        if (!ht_search(ht, &keys[i], sizeof(int), &value, &size)) ok = false;
        if (value != i) ok = false;

        /* Ищем в КЧД */
        if (!rb_search(rb, &keys[i], sizeof(int), &value, &size)) ok = false;
        if (value != i) ok = false;
    }

    /* Шаг 3: Удаление элементов */
    for (int i = 0; i < n && ok; i++) {
        /* Удаляем из хеш-таблицы */
        if (!ht_delete(ht, &keys[i], sizeof(int))) ok = false;
        /* Удаляем из КЧД */
        if (!rb_delete(rb, &keys[i], sizeof(int))) ok = false;
    }

    /* Очистка */
    ht_destroy(ht);
    rb_destroy(rb);

    return ok;
}

/* ============================================================
 *                   4. БЕНЧМАРКИ СТРУКТУР ДАННЫХ
 * ============================================================ */

 /*
  * Тест вставки элементов
  * Измеряет время вставки N элементов в обе структуры
  */
void benchmark_insert(HashTable* ht, RBTree* rb, int* keys, int n) {
    uint64_t start, end;

    /* === ИЗМЕРЕНИЕ ВСТАВКИ В ХЕШ-ТАБЛИЦУ === */
    start = get_time_ns();  /* Запоминаем время начала */
    for (int i = 0; i < n; i++) {
        /* Вставляем элемент: ключ = keys[i], значение = i */
        ht_insert(ht, &keys[i], sizeof(int), &i, sizeof(int));
    }
    end = get_time_ns();  /* Запоминаем время окончания */
    /* Переводим в микросекунды: (наносекунды / 1000) */
    double ht_time = (end - start) / 1000.0;

    /* === ИЗМЕРЕНИЕ ВСТАВКИ В КЧД === */
    start = get_time_ns();
    for (int i = 0; i < n; i++) {
        rb_insert(rb, &keys[i], sizeof(int), &i, sizeof(int));
    }
    end = get_time_ns();
    double rb_time = (end - start) / 1000.0;

    /* === ВЫВОД РЕЗУЛЬТАТОВ === */
    printf("Вставка %d элементов:\n", n);
    printf("  Хеш-таблица: %10.2f мкс (среднее: %6.2f нс/опер)\n",
        ht_time, (ht_time * 1000) / n);
    printf("  КЧД:         %10.2f мкс (среднее: %6.2f нс/опер)\n",
        rb_time, (rb_time * 1000) / n);
    /* Ускорение = время_КЧД / время_хеш - во сколько раз хеш-таблица быстрее */
    printf("  Ускорение:   %8.2fx\n", rb_time / ht_time);
    printf("\n");
}

/*
 * Тест поиска элементов
 * Измеряет время поиска N элементов в обеих структурах
 */
void benchmark_search(HashTable* ht, RBTree* rb, int* keys, int n) {
    uint64_t start, end;
    int value;
    size_t size;

    /* === ИЗМЕРЕНИЕ ПОИСКА В ХЕШ-ТАБЛИЦЕ === */
    start = get_time_ns();
    for (int i = 0; i < n; i++) {
        ht_search(ht, &keys[i], sizeof(int), &value, &size);
    }
    end = get_time_ns();
    double ht_time = (end - start) / 1000.0;

    /* === ИЗМЕРЕНИЕ ПОИСКА В КЧД === */
    start = get_time_ns();
    for (int i = 0; i < n; i++) {
        rb_search(rb, &keys[i], sizeof(int), &value, &size);
    }
    end = get_time_ns();
    double rb_time = (end - start) / 1000.0;

    printf("Поиск %d элементов:\n", n);
    printf("  Хеш-таблица: %10.2f мкс (среднее: %6.2f нс/опер)\n",
        ht_time, (ht_time * 1000) / n);
    printf("  КЧД:         %10.2f мкс (среднее: %6.2f нс/опер)\n",
        rb_time, (rb_time * 1000) / n);
    printf("  Ускорение:   %8.2fx\n", rb_time / ht_time);
    printf("\n");
}

/*
 * Тест удаления элементов
 * Измеряет время удаления N элементов из обеих структур
 */
void benchmark_delete(HashTable* ht, RBTree* rb, int* keys, int n) {
    uint64_t start, end;

    /* === ИЗМЕРЕНИЕ УДАЛЕНИЯ ИЗ ХЕШ-ТАБЛИЦЫ === */
    start = get_time_ns();
    for (int i = 0; i < n; i++) {
        ht_delete(ht, &keys[i], sizeof(int));
    }
    end = get_time_ns();
    double ht_time = (end - start) / 1000.0;

    /* === ИЗМЕРЕНИЕ УДАЛЕНИЯ ИЗ КЧД === */
    start = get_time_ns();
    for (int i = 0; i < n; i++) {
        rb_delete(rb, &keys[i], sizeof(int));
    }
    end = get_time_ns();
    double rb_time = (end - start) / 1000.0;

    printf("Удаление %d элементов:\n", n);
    printf("  Хеш-таблица: %10.2f мкс (среднее: %6.2f нс/опер)\n",
        ht_time, (ht_time * 1000) / n);
    printf("  КЧД:         %10.2f мкс (среднее: %6.2f нс/опер)\n",
        rb_time, (rb_time * 1000) / n);
    printf("  Ускорение:   %8.2fx\n", rb_time / ht_time);
    printf("\n");
}

/*
 * Полный бенчмарк для заданного размера данных
 * Выполняет все тесты (вставка, поиск, удаление) для N элементов
 */
void benchmark_structures_size(int n) {
    printf("\n");
    printf("========================================\n");
    printf("РАЗМЕР: %d элементов\n", n);
    printf("========================================\n");
    printf("\n");

    /* Шаг 1: Генерация ключей */
    int* keys = generate_keys(n, n * 5);
    if (!keys) {
        printf("Ошибка: не удалось сгенерировать ключи\n");
        return;
    }

    /* Шаг 2: Вычисляем начальный размер таблицы
     * Начальный размер должен быть БОЛЬШЕ количества элементов
     * Чтобы избежать частых расширений
     */
    uint32_t initial_size = 1024;
    /* Увеличиваем до тех пор, пока size > n * 1.5 */
    while (initial_size < (uint32_t)(n * 1.5)) {
        initial_size *= 2;
    }

    /* Шаг 3: Создаём хеш-таблицу */
    HashTable* ht = ht_create(
        initial_size,   /* начальный размер */
        hash_int,       /* хеш-функция для int */
        compare_int,    /* сравнение int */
        copy_int,       /* копирование ключа */
        copy_int,       /* копирование значения */
        free_int,       /* освобождение ключа */
        free_int,       /* освобождение значения */
        8               /* максимальная длина цепочки */
    );
    if (!ht) {
        printf("Ошибка: не удалось создать хеш-таблицу\n");
        free(keys);
        return;
    }

    /* Шаг 4: Создаём красно-чёрное дерево */
    RBTree* rb = rb_create(
        compare_int,    /* сравнение int */
        copy_int,       /* копирование ключа */
        copy_int,       /* копирование значения */
        free_int,       /* освобождение ключа */
        free_int        /* освобождение значения */
    );
    if (!rb) {
        printf("Ошибка: не удалось создать КЧД\n");
        ht_destroy(ht);
        free(keys);
        return;
    }

    /* Шаг 5: Запускаем бенчмарки */
    benchmark_insert(ht, rb, keys, n);    /* Тест вставки */
    benchmark_search(ht, rb, keys, n);    /* Тест поиска */
    benchmark_delete(ht, rb, keys, n);    /* Тест удаления */

    /* Шаг 6: Очистка */
    ht_destroy(ht);
    rb_destroy(rb);
    free(keys);
}

/*
 * Анализ скрытых констант в O(n)
 * Измеряет зависимость времени выполнения от размера данных
 * и вычисляет константы, скрытые в O(1) и O(log n)
 */
void analyze_structures_constants(void) {
    printf("\n");
    printf("========================================\n");
    printf("АНАЛИЗ СКРЫТЫХ КОНСТАНТ\n");
    printf("========================================\n");
    printf("\n");

    int sizes[] = { 10000, 50000, 100000, 500000, 1000000 };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    printf("Размер\t\tСреднее время операции (нс)\t\t     Константа\n");
    printf("        \tХеш-таблица\t\t   КЧД\t           Хеш      КЧД\n");
    printf("----------------------------------------------------------------------------\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];

        /* Генерируем ключи */
        int* keys = generate_keys(n, n * 5);
        if (!keys) continue;

        /* Вычисляем начальный размер таблицы */
        uint32_t initial_size = 1024;
        while (initial_size < (uint32_t)n) initial_size *= 2;

        /* Создаём структуры */
        HashTable* ht = ht_create(initial_size, hash_int, compare_int,
            copy_int, copy_int, free_int, free_int, 8);
        RBTree* rb = rb_create(compare_int, copy_int, copy_int, free_int, free_int);

        if (!ht || !rb) {
            free(keys);
            if (ht) ht_destroy(ht);
            if (rb) rb_destroy(rb);
            continue;
        }

        /* Вставляем элементы */
        for (int i = 0; i < n; i++) {
            ht_insert(ht, &keys[i], sizeof(int), &i, sizeof(int));
            rb_insert(rb, &keys[i], sizeof(int), &i, sizeof(int));
        }

        /* Измеряем время поиска 1000 случайных элементов */
        uint64_t ht_total = 0;
        uint64_t rb_total = 0;
        int value;
        size_t size;

        for (int i = 0; i < 1000; i++) {
            int idx = rand() % n;

            /* Измеряем время поиска в хеш-таблице */
            uint64_t start = get_time_ns();
            ht_search(ht, &keys[idx], sizeof(int), &value, &size);
            ht_total += get_time_ns() - start;

            /* Измеряем время поиска в КЧД */
            start = get_time_ns();
            rb_search(rb, &keys[idx], sizeof(int), &value, &size);
            rb_total += get_time_ns() - start;
        }

        /* Вычисляем среднее время на операцию (в наносекундах) */
        double ht_avg = (double)ht_total / 1000.0;
        double rb_avg = (double)rb_total / 1000.0;

        /*
         * Для хеш-таблицы константа = время операции (O(1))
         * Для КЧД константа = время операции / log2(n) (O(log n))
         */
        double ht_const = ht_avg;
        double rb_const = rb_avg / log2(n);

        printf("%7d\t\t%8.2f\t\t%8.2f\t  %6.2f  %6.2f\n",
            n, ht_avg, rb_avg, ht_const, rb_const);

        ht_destroy(ht);
        rb_destroy(rb);
        free(keys);
    }

    printf("\n");
    printf("Примечания:\n");
    printf("  - Для хеш-таблицы константа = время операции (O(1) с малой константой)\n");
    printf("  - Для КЧД константа = время операции / log2(n) (O(log n) с константой)\n");
    printf("  - Разница обусловлена хешированием vs балансировкой дерева\n");
    printf("  - Чем больше n, тем больше преимущество хеш-таблицы\n");
}

/*
 * Тест работы с разными типами данных
 * Демонстрирует универсальность структур через void*
 */
void test_different_types(void) {
    printf("\n");
    printf("========================================\n");
    printf("ТЕСТ РАБОТЫ С РАЗНЫМИ ТИПАМИ ДАННЫХ\n");
    printf("========================================\n");
    printf("\n");

    /* ===== ТЕСТ 1: ЦЕЛЫЕ ЧИСЛА ===== */
    printf("1. Тест с целочисленными ключами:\n");
    HashTable* ht_int = ht_create(100, hash_int, compare_int,
        copy_int, copy_int, free_int, free_int, 8);

    int keys_int[] = { 10, 20, 30, 40, 50 };
    int values_int[] = { 100, 200, 300, 400, 500 };

    /* Вставляем элементы */
    for (int i = 0; i < 5; i++) {
        ht_insert(ht_int, &keys_int[i], sizeof(int), &values_int[i], sizeof(int));
    }

    /* Ищем элементы */
    int found_int;
    size_t size_int;
    for (int i = 0; i < 5; i++) {
        if (ht_search(ht_int, &keys_int[i], sizeof(int), &found_int, &size_int)) {
            printf("  Ключ %d -> Значение %d\n", keys_int[i], found_int);
        }
    }
    ht_destroy(ht_int);

    /* ===== ТЕСТ 2: СТРОКИ ===== */
    printf("\n2. Тест со строковыми ключами:\n");
    HashTable* ht_str = ht_create(100, hash_string, compare_string,
        copy_string, copy_int, free_string, free_int, 8);

    const char* keys_str[] = { "apple", "banana", "cherry", "date", "elderberry" };
    int values_str[] = { 1, 2, 3, 4, 5 };

    /* Вставляем элементы (строки как ключи, int как значения) */
    for (int i = 0; i < 5; i++) {
        ht_insert(ht_str, keys_str[i], strlen(keys_str[i]) + 1,
            &values_str[i], sizeof(int));
    }

    /* Ищем элементы */
    int found_str;
    size_t size_str;
    for (int i = 0; i < 5; i++) {
        if (ht_search(ht_str, keys_str[i], strlen(keys_str[i]) + 1,
            &found_str, &size_str)) {
            printf("  Ключ '%s' -> Значение %d\n", keys_str[i], found_str);
        }
    }
    ht_destroy(ht_str);

    /* ===== ТЕСТ 3: БИНАРНЫЕ ДАННЫЕ (СТРУКТУРЫ) ===== */
    printf("\n3. Тест с бинарными данными (структуры):\n");
    typedef struct {
        int id;
        char name[20];
    } Person;

    HashTable* ht_bytes = ht_create(100, hash_bytes, compare_bytes,
        copy_bytes, copy_bytes, free_bytes, free_bytes, 8);

    Person persons[] = {
        {1, "Alice"},
        {2, "Bob"},
        {3, "Charlie"}
    };

    /* Вставляем структуры как ключи и значения */
    for (int i = 0; i < 3; i++) {
        ht_insert(ht_bytes, &persons[i], sizeof(Person),
            &persons[i], sizeof(Person));
    }

    /* Ищем структуру по ключу */
    Person search_person = { 2, "Bob" };
    Person found_person;
    size_t size_person;
    if (ht_search(ht_bytes, &search_person, sizeof(Person),
        &found_person, &size_person)) {
        printf("  Найден: id=%d, name='%s'\n", found_person.id, found_person.name);
    }
    ht_destroy(ht_bytes);

    printf("\n");
    printf("Вывод: структуры данных поддерживают любые типы через void*\n");
}