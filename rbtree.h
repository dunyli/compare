#ifndef RBTREE_H
#define RBTREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Цвета узлов красно-чёрного дерева */
typedef enum { RED, BLACK } Color;

/* Тип для функции-колбэка при обходе дерева */
typedef void (*RBTreeCallback)(const void* key, size_t key_size,
    const void* value, size_t value_size,
    void* user_data);

/* Узел красно-чёрного дерева (универсальный) */
typedef struct RBNode {
    void* key;                  /* Указатель на ключ (любого типа) */
    size_t key_size;            /* Размер ключа в байтах */
    void* value;                /* Указатель на значение (любого типа) */
    size_t value_size;          /* Размер значения в байтах */
    Color color;                /* Цвет узла: RED или BLACK */
    struct RBNode* left;        /* Левый потомок */
    struct RBNode* right;       /* Правый потомок */
    struct RBNode* parent;      /* Родительский узел */
} RBNode;

/* Красно-чёрное дерево */
typedef struct {
    RBNode* root;               /* Корень дерева */
    size_t size;                /* Количество элементов в дереве */

    /* Функции для работы с данными */
    int (*compare_func)(const void* a, const void* b);  /* Сравнение ключей */
    void* (*copy_key_func)(const void* data, size_t size);  /* Копирование ключа */
    void* (*copy_value_func)(const void* data, size_t size); /* Копирование значения */
    void (*free_key_func)(void* data);   /* Освобождение ключа */
    void (*free_value_func)(void* data); /* Освобождение значения */
} RBTree;

/* Создание и удаление дерева */
RBTree* rb_create(int (*compare_func)(const void*, const void*),
    void* (*copy_key_func)(const void*, size_t),
    void* (*copy_value_func)(const void*, size_t),
    void (*free_key_func)(void*),
    void (*free_value_func)(void*));

void rb_destroy(RBTree* tree);
void rb_clear(RBTree* tree);

/* Вставка и обновление */
bool rb_insert(RBTree* tree, const void* key, size_t key_size,
    const void* value, size_t value_size);

/* Поиск */
bool rb_search(RBTree* tree, const void* key, size_t key_size,
    void* out_value, size_t* out_value_size);

bool rb_contains(RBTree* tree, const void* key, size_t key_size);

/* Удаление */
bool rb_delete(RBTree* tree, const void* key, size_t key_size);

/* Получение размера */
size_t rb_size(RBTree* tree);
bool rb_is_empty(RBTree* tree);

/* Обход дерева */
void rb_inorder(RBTree* tree, RBTreeCallback callback, void* user_data);
void rb_preorder(RBTree* tree, RBTreeCallback callback, void* user_data);
void rb_postorder(RBTree* tree, RBTreeCallback callback, void* user_data);

/* Статистика */
size_t rb_height(RBTree* tree);
void rb_stats(RBTree* tree, size_t* height, size_t* size, size_t* black_height);

/* Стандартные функции для строк (для удобства) */
bool rb_insert_string(RBTree* tree, const char* key, int value);
bool rb_search_string(RBTree* tree, const char* key, int* out_value);
bool rb_delete_string(RBTree* tree, const char* key);

#endif /* RBTREE_H */