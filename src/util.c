#include "stdio.h"
#include "stdlib.h"
#include "util.h"
#include <string.h>

void list_delete_entry (list_t *list, list_entry_t *entry, bool delete_entry) {
    if (delete_entry) {
        free(entry->__val);
    }
    free(entry);
}

void __list_remove_internal__ (list_t *list, list_entry_t *entry, bool delete_entry)
{
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        list->head = entry->next;
    }
    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        list->tail = entry->prev;
    }

    list_delete_entry(list, entry, delete_entry);
}

int get_list_entry (list_entry_t *list_entry, void * __key, void *entry)
{
    if (entry == NULL) {
        return -1;
    }
    list_entry->next = NULL;    
    list_entry->prev = NULL;   
    list_entry->__val = entry;
    list_entry->__key = __key;

    return 0;
}

int list_remove_entry (list_t *list, void *__key, void *entry, bool delete_entry)
{
    list_entry_t *iter = NULL;
    
    if (list == NULL) {
        return -1;
    }    

    if ((iter = list_lookup(list, __key, entry))) {
        __list_remove_internal__ (list, iter, delete_entry);
        return 1;
    }
    return 0;
}

int list_insert_tail (list_t *list, void* __key, void *entry)
{
    list_entry_t *list_entry = NULL;

    if ((list_entry = list_lookup(list, __key, entry)) != NULL) {
        list_entry->__val = entry;
        return 0;
    }
    list_entry = calloc(1, sizeof(list_entry_t)); 
    if (get_list_entry(list_entry, __key, entry) != 0) {
        return -1;
    }

    list_entry->next = NULL;
    list_entry->prev = list->tail;
    if (list->tail) {
        list->tail->next = list_entry;
    }
    if (!list->tail) {
        list->head = list_entry;
    }
    list->tail = list_entry;

    return 1;
}

int list_insert_head (list_t *list, void *__key, void *entry)
{
    list_entry_t *list_entry = NULL;

    if ((list_entry = list_lookup(list, __key, entry)) != NULL) {
        list_entry->__val = entry;
        return 0;
    }
    list_entry = calloc(1, sizeof(list_entry_t)); 
    if (get_list_entry(list_entry, __key, entry) != 0) {
        return -1;
    }

    list_entry->next = list->head;
    list_entry->prev = NULL;
    if (list->head) {
        list->head->prev = list_entry;
    }
    if (!list->head) {
        list->tail = list_entry;
    }
    list->head = list_entry;

    return 0;
}

void *list_lookup_entry (list_t *list, void *__key, void *entry)
{
    list_entry_t *list_entry = list_lookup(list, __key, entry);
    if (list_entry) {
        return list_entry->__val;
    }

    return NULL;
}

list_entry_t *list_lookup (list_t *list, void *__key, void *entry)
{
    list_entry_t *iter = list->head;
    if (iter == NULL) {
        return NULL;
    }    

    if (!__key && !entry) {
        return NULL;
    }
    if (__key) {
        while (iter) {
            if (list->lookup_func(iter->__key, __key)) {
                return iter;
            }
            iter = iter->next;
        }
    } else {
        while (iter) {
            if (list->lookup_func(iter->__val, entry)) {
                return iter;
            }
            iter = iter->next;
        }
    }
    return NULL;
}

list_t *list_init (bool (*lookup_func)(void *, void *))
{
    list_t *new_list = NULL;

    new_list = calloc(1, sizeof(list_t));
    if (new_list == NULL) {
        return NULL;
    }

    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->lookup_func = lookup_func;
    
    return new_list;
}

void list_delete (list_t *list, bool delete_entry) 
{
    if (list == NULL) {
        return;
    }

    list_entry_t *iter = list->head;
    list_entry_t *iter_next = NULL;
    
    while(iter) {
        iter_next = iter->next;
        list_delete_entry(list, iter, delete_entry);
        iter = iter_next;
    }

    free(list);
}
