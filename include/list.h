#pragma once

#include <mutex>
class List {
    private:
        typedef struct _list_entry_t_ {
            struct _list_entry_t_* next;
            struct _list_entry_t_* prev;
            void *val;
            void *key;
        }_list_entry_t;
        
        bool (*m_lookup_func) (void*, void*);
        _list_entry_t_ *m_head;
        _list_entry_t_ *m_tail;
        std::recursive_mutex m_list_lock;
        bool m_is_multi_threaded;

    public:
        typedef struct _list_iter_t_{
            struct _list_entry_t_* cur;
            struct _list_entry_t_* next;
            struct _list_entry_t_* prev;
        } list_iter_t;

    private:
        void *__get_key(_list_entry_t* __entry);
        void *__get_val(_list_entry_t* __entry);
        _list_entry_t* __iter_deref (list_iter_t *__iter);
        _list_entry_t* __lookup_lockless(void *__key, void *__val);
        bool __insert_head_lockless(void *__key, void *__val);
        bool __insert_tail_lockless(void *__key, void *__val);
        bool __remove_lockless(void *__key, void *__val);

    public:
        List(bool (*__lookup_func) (void*, void*));
        List(bool (*__lookup_func) (void*, void*), bool __is_multi_threaded);

        bool insert_head(void *__key, void *__val);
        bool insert_tail(void *__key, void *__val);
        bool remove(void *__key, void *__val);
        void* lookup(void *__key, void *__val);
        void** lookup_mutable(void *__key, void *__val);

        list_iter_t* iter_init();
        void iter_clear(list_iter_t *__iter);
        list_iter_t* iter_inc(list_iter_t *__iter);
        list_iter_t* iter_dec(list_iter_t *__iter);
};
