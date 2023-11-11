#pragma once

#include "iterator.h"
#include <functional>
#include <mutex>
#include <utility>
template <class List>
class ListIterator: public Iterator<List> {
    public:
        ListIterator(typename Iterator<List>::PointerType _ptr)
            :Iterator<List>(_ptr)
        {
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
        }_list_entry_t;
        
        //bool (*m_lookup_func) (void*, void*);
        std::function<bool(void*, void*)> m_lookup_func;
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
        Iterator iter_init();
        Iterator iter_advance(Iterator&);
        void iter_cleanup(Iterator);


    private:
        void *__get_val(_list_entry_t* __entry);
        _list_entry_t* __lookup_lockless(void *__val);
        bool __insert_head_lockless(void *__val);
        bool __insert_tail_lockless(void *__val);
        bool __remove_lockless(void *__val);
        bool __remove_internal(_list_entry_t* __val);

    public:
        List(std::function<bool(void*, void*)>, bool __is_multi_threaded);
        List(std::function<bool(void*, void*)>);
        ~List();

        bool insert_head(void *__val);
        bool insert_tail(void *__val);
        bool remove(void *__val);
        bool remove_entry(void *__val_entry);
        void* lookup(void *__val);
        void** lookup_mutable(void *__val);
};

