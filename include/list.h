#pragma once

#include "iterator.h"
#include <mutex>
#include <utility>
template <class List>
class ListIterator: public Iterator<List> {
    public:
        ListIterator(typename Iterator<List>::PointerType _ptr)
            :Iterator<List>(_ptr)
        {
        }
        void* get_key()
        {
            return Iterator<List>::m_ptr->key;
        }
        void *get_val()
        {
            return Iterator<List>::m_ptr->val;
        }
};
class List {
    private:
        typedef struct _list_entry_t_ {
            struct _list_entry_t_* next;
            struct _list_entry_t_* prev;
            void *val;
            void *key;
            bool m_is_moved;
        }_list_entry_t;
        
        bool (*m_lookup_func) (void*, void*);
        _list_entry_t_ *m_tail;
        _list_entry_t_ *m_head;
        std::recursive_mutex m_list_lock;
        bool m_is_multi_threaded;

    public:
        using ValueType = typeof(_list_entry_t_);
        using Iterator = ListIterator<List>;
        Iterator begin() {
            return Iterator(m_head);
        }
        Iterator end() {
            return Iterator(m_tail);
        }

    private:
        void *__get_key(_list_entry_t* __entry);
        void *__get_val(_list_entry_t* __entry);
        _list_entry_t* __lookup_lockless(void *__key, void *__val);
        bool __insert_head_lockless(void *__key, void *__val);
        bool __insert_tail_lockless(void *__key, void *__val);
        bool __remove_lockless(void *__key, void *__val);

    public:
        List(bool (*__lookup_func) (void*, void*));
        List(bool (*__lookup_func) (void*, void*), bool __is_multi_threaded);
        ~List();

        bool insert_head(void *__key, void *__val);
        bool insert_tail(void *__key, void *__val);
        bool remove(void *__key, void *__val);
        void* lookup(void *__key, void *__val);
        void** lookup_mutable(void *__key, void *__val);
        void set_moved(void *__key, void *__val);
        void set_moved(Iterator& __iter);
};
