////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// - Alistair O'Brien - 6/22/2020 - University of Cambridge
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      File:        list.h
//      Environment: Tiny OS
//      Description: A collection of useful macros to define a doubly linked list
//                   with a insert, delete and size methods. 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TINY_OS_LIST_H
#define TINY_OS_LIST_H

#include <lib/stddef.h>

#define DEFINE_LIST(node_type)              \
struct node_type;                           \
typedef struct node_type##_list {           \
    struct node_type * head;                \
    size_t size;                            \
} node_type##_list_t;


#define DEFINE_LINK(node_type)              \
struct node_type * next_##node_type;        \
struct node_type * prev_##node_type;

#define INIT_LIST(list)                     \
    do {                                    \
        list.head = null;                   \
        list.size = 0;                      \
    } while (0)

#define IMPLEMENT_LIST(node_type)                                                       \
void delete_##node_type##_list(node_type##_list_t *list, struct node_type * x) {        \
    if (x->prev_##node_type == null) {                                                  \
        list->head = x->next_##node_type;                                               \
    } else {                                                                            \
        x->prev_##node_type = x->next_##node_type;                                      \
    }                                                                                   \
                                                                                        \
    if (x -> next_##node_type != null) {                                                \
        x->next_##node_type->prev_##node_type = x->prev_##node_type;                    \
    }                                                                                   \
    x->prev_##node_type = null;                                                         \
    x->next_##node_type = null;                                                         \
    list->size--;                                                                       \
}                                                                                       \
                                                                                        \
void insert_##node_type##_list(node_type##_list_t *list, struct node_type * x) {        \
    x->next_##node_type = list->head;                                                   \
    x->prev_##node_type = null;                                                         \
    if (list->head != null) {                                                           \
        list->head->prev_##node_type = x;                                               \
    }                                                                                   \
    list->head = x;                                                                     \
    list->size++;                                                                       \
}                                                                                       \
                                                                                        \
struct node_type * peek_##node_type##_list(node_type##_list_t *list) {                  \
    return list->head;                                                                  \
}                                                                                       \
                                                                                        \
size_t size_##node_type##_list(node_type##_list_t *list) {                              \
    return list->size;                                                                  \
}                                                                                       \
                                                                                        \
struct node_type * pop_##node_type##_list(node_type##_list_t *list) {                   \
    if (size_##node_type##_list(list) == 0) return null;                                \
    struct node_type * x = peek_##node_type##_list(list);                               \
    delete_##node_type##_list(list, x);                                                 \
    return x;                                                                           \
}



#endif //TINY_OS_LIST_H
