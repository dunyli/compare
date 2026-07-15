#define _CRT_SECURE_NO_WARNINGS

#include "rbtree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===== */

/*
 * Создание нового узла
 * Выделяет память, копирует ключ и значение
 */
static RBNode* rb_create_node(const void* key, size_t key_size,
    const void* value, size_t value_size,
    void* (*copy_key)(const void*, size_t),
    void* (*copy_value)(const void*, size_t)) {
    /* Выделяем память под узел */
    RBNode* node = (RBNode*)malloc(sizeof(RBNode));
    if (!node) return NULL;

    /* Копируем ключ */
    node->key = copy_key ? copy_key(key, key_size) : malloc(key_size);
    if (!node->key) {
        free(node);
        return NULL;
    }
    if (!copy_key) memcpy(node->key, key, key_size);
    node->key_size = key_size;

    /* Копируем значение */
    node->value = copy_value ? copy_value(value, value_size) : malloc(value_size);
    if (!node->value) {
        if (copy_key) {
            /* Если есть функция освобождения, используем её */
            free(node->key);
        }
        else {
            free(node->key);
        }
        free(node);
        return NULL;
    }
    if (!copy_value) memcpy(node->value, value, value_size);
    node->value_size = value_size;

    /* Инициализируем поля узла */
    node->color = RED;          /* Новый узел всегда красный */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    return node;
}

/*
 * Рекурсивное удаление узла и всех его потомков
 */
static void rb_destroy_node(RBNode* node,
    void (*free_key)(void*),
    void (*free_value)(void*)) {
    if (!node) return;

    /* Рекурсивно удаляем потомков */
    rb_destroy_node(node->left, free_key, free_value);
    rb_destroy_node(node->right, free_key, free_value);

    /* Освобождаем ключ */
    if (free_key) {
        free_key(node->key);
    }
    else {
        free(node->key);
    }

    /* Освобождаем значение */
    if (free_value) {
        free_value(node->value);
    }
    else {
        free(node->value);
    }

    free(node);  /* Освобождаем сам узел */
}

/*
 * Поиск минимального узла в поддереве
 */
static RBNode* rb_minimum(RBNode* node) {
    while (node->left != NULL) {
        node = node->left;  /* Идём всё время влево */
    }
    return node;  /* Самый левый узел - минимальный */
}

/*
 * Замена одного поддерева другим (transplant)
 * Заменяет узел u на узел v в дереве
 */
static void rb_transplant(RBTree* tree, RBNode* u, RBNode* v) {
    if (u->parent == NULL) {
        tree->root = v;              /* u был корнем */
    }
    else if (u == u->parent->left) {
        u->parent->left = v;         /* u был левым потомком */
    }
    else {
        u->parent->right = v;        /* u был правым потомком */
    }

    if (v != NULL) {
        v->parent = u->parent;       /* Обновляем родителя для v */
    }
}

/*
 * Левый поворот вокруг узла x
 * Преобразует: x -> y (y становится родителем x)
 */
static void rb_rotate_left(RBTree* tree, RBNode* x) {
    RBNode* y = x->right;           /* y - правый потомок x */
    x->right = y->left;             /* Левое поддерево y становится правым поддеревом x */

    if (y->left != NULL) {
        y->left->parent = x;        /* Обновляем родителя для левого поддерева y */
    }

    y->parent = x->parent;          /* y поднимается на место x */

    if (x->parent == NULL) {
        tree->root = y;             /* Если x был корнем, y становится корнем */
    }
    else if (x == x->parent->left) {
        x->parent->left = y;        /* Если x был левым потомком, y становится левым */
    }
    else {
        x->parent->right = y;       /* Если x был правым потомком, y становится правым */
    }

    y->left = x;                    /* x становится левым потомком y */
    x->parent = y;                  /* Обновляем родителя x */
}

/*
 * Правый поворот вокруг узла y
 * Преобразует: y <- x (x становится родителем y)
 */
static void rb_rotate_right(RBTree* tree, RBNode* y) {
    RBNode* x = y->left;            /* x - левый потомок y */
    y->left = x->right;             /* Правое поддерево x становится левым поддеревом y */

    if (x->right != NULL) {
        x->right->parent = y;       /* Обновляем родителя для правого поддерева x */
    }

    x->parent = y->parent;          /* x поднимается на место y */

    if (y->parent == NULL) {
        tree->root = x;             /* Если y был корнем, x становится корнем */
    }
    else if (y == y->parent->left) {
        y->parent->left = x;        /* Если y был левым потомком, x становится левым */
    }
    else {
        y->parent->right = x;       /* Если y был правым потомком, x становится правым */
    }

    x->right = y;                   /* y становится правым потомком x */
    y->parent = x;                  /* Обновляем родителя y */
}

/*
 * Восстановление свойств красно-чёрного дерева после вставки
 */
static void rb_insert_fixup(RBTree* tree, RBNode* node) {
    /* Пока родитель узла красный (нарушение свойства 4) */
    while (node->parent != NULL && node->parent->color == RED) {
        RBNode* parent = node->parent;
        RBNode* grandparent = parent->parent;

        if (parent == grandparent->left) {
            /* Родитель - левый потомок дедушки */
            RBNode* uncle = grandparent->right;

            if (uncle != NULL && uncle->color == RED) {
                /* Случай 1: Дядя красный - перекрашиваем */
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;     /* Поднимаемся к дедушке */
            }
            else {
                /* Случай 2: Дядя чёрный */
                if (node == parent->right) {
                    /* Если узел - правый потомок, делаем левый поворот */
                    node = parent;
                    rb_rotate_left(tree, node);
                    parent = node->parent;
                }

                /* Случай 3: Узел - левый потомок */
                parent->color = BLACK;
                grandparent->color = RED;
                rb_rotate_right(tree, grandparent);
            }
        }
        else {
            /* Родитель - правый потомок дедушки (симметричный случай) */
            RBNode* uncle = grandparent->left;

            if (uncle != NULL && uncle->color == RED) {
                /* Случай 1: Дядя красный - перекрашиваем */
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            }
            else {
                /* Случай 2: Дядя чёрный */
                if (node == parent->left) {
                    /* Если узел - левый потомок, делаем правый поворот */
                    node = parent;
                    rb_rotate_right(tree, node);
                    parent = node->parent;
                }

                /* Случай 3: Узел - правый потомок */
                parent->color = BLACK;
                grandparent->color = RED;
                rb_rotate_left(tree, grandparent);
            }
        }
    }

    /* Корень всегда чёрный (свойство 2) */
    tree->root->color = BLACK;
}

/*
 * Восстановление свойств красно-чёрного дерева после удаления
 */
static void rb_delete_fixup(RBTree* tree, RBNode* node) {
    /* Если node == NULL, то это особый случай - удаление листа */
    if (node == NULL) {
        if (tree->root != NULL) {
            tree->root->color = BLACK;  /* Корень должен быть чёрным */
        }
        return;
    }

    while (node != tree->root && node->color == BLACK) {
        if (node->parent == NULL) {
            break;  /* Защита от неожиданного NULL */
        }

        if (node == node->parent->left) {
            /* Узел - левый потомок */
            RBNode* sibling = node->parent->right;

            if (sibling == NULL) {
                node = node->parent;  /* Если брата нет, поднимаемся вверх */
                continue;
            }

            if (sibling->color == RED) {
                /* Случай 1: Брат красный */
                sibling->color = BLACK;
                node->parent->color = RED;
                rb_rotate_left(tree, node->parent);
                sibling = node->parent->right;

                if (sibling == NULL) {
                    node = node->parent;
                    continue;
                }
            }

            if ((sibling->left == NULL || sibling->left->color == BLACK) &&
                (sibling->right == NULL || sibling->right->color == BLACK)) {
                /* Случай 2: Оба племянника чёрные */
                sibling->color = RED;
                node = node->parent;
            }
            else {
                if (sibling->right == NULL || sibling->right->color == BLACK) {
                    /* Случай 3: Правый племянник чёрный */
                    if (sibling->left != NULL) {
                        sibling->left->color = BLACK;
                    }
                    sibling->color = RED;
                    rb_rotate_right(tree, sibling);
                    sibling = node->parent->right;

                    if (sibling == NULL) {
                        node = node->parent;
                        continue;
                    }
                }

                /* Случай 4: Правый племянник красный */
                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                if (sibling->right != NULL) {
                    sibling->right->color = BLACK;
                }
                rb_rotate_left(tree, node->parent);
                node = tree->root;
            }
        }
        else {
            /* Узел - правый потомок (симметричный случай) */
            RBNode* sibling = node->parent->left;

            if (sibling == NULL) {
                node = node->parent;
                continue;
            }

            if (sibling->color == RED) {
                /* Случай 1: Брат красный */
                sibling->color = BLACK;
                node->parent->color = RED;
                rb_rotate_right(tree, node->parent);
                sibling = node->parent->left;

                if (sibling == NULL) {
                    node = node->parent;
                    continue;
                }
            }

            if ((sibling->left == NULL || sibling->left->color == BLACK) &&
                (sibling->right == NULL || sibling->right->color == BLACK)) {
                /* Случай 2: Оба племянника чёрные */
                sibling->color = RED;
                node = node->parent;
            }
            else {
                if (sibling->left == NULL || sibling->left->color == BLACK) {
                    /* Случай 3: Левый племянник чёрный */
                    if (sibling->right != NULL) {
                        sibling->right->color = BLACK;
                    }
                    sibling->color = RED;
                    rb_rotate_left(tree, sibling);
                    sibling = node->parent->left;

                    if (sibling == NULL) {
                        node = node->parent;
                        continue;
                    }
                }

                /* Случай 4: Левый племянник красный */
                sibling->color = node->parent->color;
                node->parent->color = BLACK;
                if (sibling->left != NULL) {
                    sibling->left->color = BLACK;
                }
                rb_rotate_right(tree, node->parent);
                node = tree->root;
            }
        }
    }

    if (node != NULL) {
        node->color = BLACK;
    }
}

/* ===== ОСНОВНЫЕ ФУНКЦИИ ===== */

/*
 * Создание красно-чёрного дерева
 */
RBTree* rb_create(int (*compare_func)(const void*, const void*),
    void* (*copy_key_func)(const void*, size_t),
    void* (*copy_value_func)(const void*, size_t),
    void (*free_key_func)(void*),
    void (*free_value_func)(void*)) {
    if (!compare_func) return NULL;  /* Функция сравнения обязательна */

    RBTree* tree = (RBTree*)malloc(sizeof(RBTree));
    if (!tree) return NULL;

    tree->root = NULL;
    tree->size = 0;
    tree->compare_func = compare_func;
    tree->copy_key_func = copy_key_func;
    tree->copy_value_func = copy_value_func;
    tree->free_key_func = free_key_func;
    tree->free_value_func = free_value_func;

    return tree;
}

/*
 * Уничтожение дерева
 */
void rb_destroy(RBTree* tree) {
    if (!tree) return;
    rb_clear(tree);  /* Удаляем все узлы */
    free(tree);      /* Освобождаем структуру дерева */
}

/*
 * Очистка дерева (удаление всех элементов)
 */
void rb_clear(RBTree* tree) {
    if (!tree) return;
    rb_destroy_node(tree->root, tree->free_key_func, tree->free_value_func);
    tree->root = NULL;
    tree->size = 0;
}

/*
 * Вставка элемента в дерево
 */
bool rb_insert(RBTree* tree, const void* key, size_t key_size,
    const void* value, size_t value_size) {
    if (!tree || !key || !value) return false;

    /* Создаём новый узел */
    RBNode* new_node = rb_create_node(key, key_size, value, value_size,
        tree->copy_key_func, tree->copy_value_func);
    if (!new_node) return false;

    /* Ищем место для вставки */
    RBNode* current = tree->root;
    RBNode* parent = NULL;

    while (current != NULL) {
        parent = current;
        int cmp = tree->compare_func(key, current->key);

        if (cmp == 0) {
            /* Ключ уже существует - обновляем значение */
            if (tree->free_value_func) {
                tree->free_value_func(current->value);
            }
            else {
                free(current->value);
            }

            current->value = tree->copy_value_func ?
                tree->copy_value_func(value, value_size) :
                malloc(value_size);
            if (!current->value) {
                rb_destroy_node(new_node, tree->free_key_func, tree->free_value_func);
                return false;
            }
            if (!tree->copy_value_func) memcpy(current->value, value, value_size);
            current->value_size = value_size;

            rb_destroy_node(new_node, tree->free_key_func, tree->free_value_func);
            return true;
        }
        else if (cmp < 0) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    /* Вставляем новый узел */
    new_node->parent = parent;

    if (parent == NULL) {
        tree->root = new_node;  /* Дерево было пустым */
    }
    else if (tree->compare_func(key, parent->key) < 0) {
        parent->left = new_node;
    }
    else {
        parent->right = new_node;
    }

    tree->size++;

    /* Восстанавливаем свойства красно-чёрного дерева */
    rb_insert_fixup(tree, new_node);

    return true;
}

/*
 * Поиск элемента по ключу
 */
bool rb_search(RBTree* tree, const void* key, size_t key_size,
    void* out_value, size_t* out_value_size) {
    if (!tree || !key) return false;

    RBNode* current = tree->root;

    while (current != NULL) {
        int cmp = tree->compare_func(key, current->key);

        if (cmp == 0) {
            /* Нашли узел */
            if (out_value) {
                memcpy(out_value, current->value, current->value_size);
            }
            if (out_value_size) {
                *out_value_size = current->value_size;
            }
            return true;
        }
        else if (cmp < 0) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    return false;  /* Ключ не найден */
}

/*
 * Проверка наличия ключа в дереве
 */
bool rb_contains(RBTree* tree, const void* key, size_t key_size) {
    return rb_search(tree, key, key_size, NULL, NULL);
}

/*
 * Удаление элемента по ключу
 */
bool rb_delete(RBTree* tree, const void* key, size_t key_size) {
    if (!tree || !key) return false;

    /* Ищем узел для удаления */
    RBNode* current = tree->root;

    while (current != NULL) {
        int cmp = tree->compare_func(key, current->key);
        if (cmp == 0) break;
        else if (cmp < 0) current = current->left;
        else current = current->right;
    }

    if (!current) return false;  /* Ключ не найден */

    /* Удаляем узел */
    RBNode* y = current;
    RBNode* x = NULL;
    Color y_original_color = y->color;

    if (current->left == NULL) {
        x = current->right;
        rb_transplant(tree, current, current->right);
    }
    else if (current->right == NULL) {
        x = current->left;
        rb_transplant(tree, current, current->left);
    }
    else {
        y = rb_minimum(current->right);
        y_original_color = y->color;
        x = y->right;

        if (y->parent == current) {
            if (x != NULL) {
                x->parent = y;
            }
        }
        else {
            rb_transplant(tree, y, y->right);
            y->right = current->right;
            y->right->parent = y;
        }

        rb_transplant(tree, current, y);
        y->left = current->left;
        y->left->parent = y;
        y->color = current->color;
    }

    /* Освобождаем память удалённого узла */
    if (tree->free_key_func) {
        tree->free_key_func(current->key);
    }
    else {
        free(current->key);
    }
    if (tree->free_value_func) {
        tree->free_value_func(current->value);
    }
    else {
        free(current->value);
    }
    free(current);
    tree->size--;

    /* Если удалённый узел был чёрным, восстанавливаем баланс */
    if (y_original_color == BLACK) {
        if (x == NULL) {
            if (tree->root != NULL) {
                tree->root->color = BLACK;
            }
        }
        else {
            rb_delete_fixup(tree, x);
        }
    }

    return true;
}

/*
 * Получение количества элементов в дереве
 */
size_t rb_size(RBTree* tree) {
    return tree ? tree->size : 0;
}

/*
 * Проверка, пусто ли дерево
 */
bool rb_is_empty(RBTree* tree) {
    return tree ? (tree->size == 0) : true;
}

/*
 * Рекурсивный обход дерева (inorder - левый-корень-правый)
 */
static void rb_inorder_recursive(RBNode* node, RBTreeCallback callback, void* user_data) {
    if (!node) return;

    rb_inorder_recursive(node->left, callback, user_data);  /* Левое поддерево */
    callback(node->key, node->key_size, node->value, node->value_size, user_data);
    rb_inorder_recursive(node->right, callback, user_data); /* Правое поддерево */
}

/*
 * Рекурсивный обход дерева (preorder - корень-левый-правый)
 */
static void rb_preorder_recursive(RBNode* node, RBTreeCallback callback, void* user_data) {
    if (!node) return;

    callback(node->key, node->key_size, node->value, node->value_size, user_data);
    rb_preorder_recursive(node->left, callback, user_data);
    rb_preorder_recursive(node->right, callback, user_data);
}

/*
 * Рекурсивный обход дерева (postorder - левый-правый-корень)
 */
static void rb_postorder_recursive(RBNode* node, RBTreeCallback callback, void* user_data) {
    if (!node) return;

    rb_postorder_recursive(node->left, callback, user_data);
    rb_postorder_recursive(node->right, callback, user_data);
    callback(node->key, node->key_size, node->value, node->value_size, user_data);
}

/*
 * Обход дерева в симметричном порядке (inorder)
 */
void rb_inorder(RBTree* tree, RBTreeCallback callback, void* user_data) {
    if (!tree || !callback) return;
    rb_inorder_recursive(tree->root, callback, user_data);
}

/*
 * Обход дерева в прямом порядке (preorder)
 */
void rb_preorder(RBTree* tree, RBTreeCallback callback, void* user_data) {
    if (!tree || !callback) return;
    rb_preorder_recursive(tree->root, callback, user_data);
}

/*
 * Обход дерева в обратном порядке (postorder)
 */
void rb_postorder(RBTree* tree, RBTreeCallback callback, void* user_data) {
    if (!tree || !callback) return;
    rb_postorder_recursive(tree->root, callback, user_data);
}

/*
 * Вычисление высоты дерева
 */
static size_t rb_height_recursive(RBNode* node) {
    if (!node) return 0;
    size_t left_height = rb_height_recursive(node->left);
    size_t right_height = rb_height_recursive(node->right);
    return 1 + (left_height > right_height ? left_height : right_height);
}

/*
 * Получение высоты дерева
 */
size_t rb_height(RBTree* tree) {
    if (!tree) return 0;
    return rb_height_recursive(tree->root);
}

/*
 * Вычисление чёрной высоты дерева
 */
static size_t rb_black_height_recursive(RBNode* node) {
    if (!node) return 0;
    size_t left = rb_black_height_recursive(node->left);
    size_t right = rb_black_height_recursive(node->right);
    return (node->color == BLACK ? 1 : 0) + (left > right ? left : right);
}

/*
 * Получение статистики дерева
 */
void rb_stats(RBTree* tree, size_t* height, size_t* size, size_t* black_height) {
    if (!tree) return;

    if (height) *height = rb_height_recursive(tree->root);
    if (size) *size = tree->size;
    if (black_height) *black_height = rb_black_height_recursive(tree->root);
}

/*
 * Вспомогательные функции для работы со строками (для удобства использования)
 */

 /*
  * Вставка строкового ключа и целочисленного значения
  */
bool rb_insert_string(RBTree* tree, const char* key, int value) {
    return rb_insert(tree, key, strlen(key) + 1, &value, sizeof(int));
}

/*
 * Поиск по строковому ключу
 */
bool rb_search_string(RBTree* tree, const char* key, int* out_value) {
    return rb_search(tree, key, strlen(key) + 1, out_value, NULL);
}

/*
 * Удаление по строковому ключу
 */
bool rb_delete_string(RBTree* tree, const char* key) {
    return rb_delete(tree, key, strlen(key) + 1);
}