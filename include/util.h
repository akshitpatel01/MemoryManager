#ifndef _UTIL_H_
#define _UTIL_H_

#include "stdbool.h"
#include "pthread.h"

typedef struct list_entry_t_ {
    void *__val;
    void *__key;
    struct list_entry_t_ *next;
    struct list_entry_t_ *prev;
} list_entry_t;

typedef struct list_h_ {
    pthread_mutex_t lock;
    list_entry_t *head;
    list_entry_t *tail;
    bool (*lookup_func)(void*, void*);
} list_t;

typedef struct lookup_params_t_ {
    void *val1;
    void *val2;
} lookup_params_t;

int list_insert_head (list_t *list, void *__key, void *entry);
int list_insert_tail (list_t *list, void* __key, void *entry);
list_entry_t *list_lookup (list_t *list, void *__key, void *entry);
int get_list_entry (list_entry_t *list_entry, void * __key, void *entry);
list_t *list_init (bool (*)(void *, void *));
void list_delete (list_t *ls, bool delete_entry);
int list_remove_entry (list_t *list, void *__key, void *entry, bool delete_entry);
void *list_lookup_entry (list_t *list, void *__key, void *entry);




#endif /*end _UTIL_H_*/
