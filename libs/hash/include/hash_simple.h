#pragma once

#include "list.h"
#include <atomic>
#include <functional>
#include <memory>
#include <sys/types.h>


#define MULTI_THREAD
#define HASH_MAX_BUCKETS 4096

class Hash_simple {
    private:
        typedef struct m_hash_entry_ {
            void* m_key;
            void* m_val;
            bool m_is_moved;
            m_hash_entry_(void* _key, void* _val, bool _is_moved)
                :m_key(_key), m_val(_val), m_is_moved(_is_moved)
            {
            }
        } m_hash_entry_t;
    private:
        typedef struct _bucket_t_ {
            private:
                List* m_list;
                mutable std::recursive_mutex m_bucket_lock;
            public:
                _bucket_t_()
                    : m_list(nullptr)
                {

                }
                void set_list(List *__list)
                {
                    m_list = __list;
                }
                
                List* get_list() const
                {
                    return m_list;
                }
 
                void lock_init()
                {
                }
                void lock_bucket() const
                {
                    m_bucket_lock.lock();
                }
                void unlock_bucket() const
                {
                    m_bucket_lock.unlock();
                }
                ~_bucket_t_ ()
                {
                    delete m_list;
                }
        } m_bucket_t;
        std::atomic_llong m_max_size;
        std::atomic_llong m_cur_size;
        std::unique_ptr<m_bucket_t[]> m_tab;
        std::function<bool(void*, void*)> m_lookup_func;
        const uint32_t __universal_hash(void *key, uint size) const;
        //uint32_t m_key_size;
    private:
      bool list_lookup_func(void *a, void *b) {
          m_hash_entry_t *m1 = static_cast<m_hash_entry_t *>(a);
          m_hash_entry_t *m2 = static_cast<m_hash_entry_t *>(b);

          return m_lookup_func(m1->m_key, m2->m_key);
      }

    public:
        using hash_entry_t = m_hash_entry_t;
        typedef struct hash_iter_t_ {
            uint __bucket_ind;
            List::Iterator __list_iter;
            hash_iter_t_(List& _list)
                :__bucket_ind(0), __list_iter(_list.begin())
            {
            }
        } m_hash_iter_t;
        m_hash_iter_t* iter_init();
        void iter_clear(m_hash_iter_t* _iter);
        m_hash_iter_t* iter_inc(m_hash_iter_t* _iter);
        bool __advance_bucket(m_hash_iter_t* _iter);
        void* iter_get_val(m_hash_iter_t* _iter);

    public:
        Hash_simple(uint32_t _size, std::function<bool(void*,void*)>, uint32_t _key_size);
        ~Hash_simple();
        bool insert(uint32_t hash, void *__key, void *__data);
        bool insert(uint32_t hash, void *__key, void *__data, uint32_t &_size);
        bool insert_lockless(uint32_t hash, void *__key, void *__data);
        bool update_lockless(uint32_t hash, void *__key, void *__data);

        void* lookup(uint32_t hash, void *__key);
        void* lookup_lockless(uint32_t hash, void *__key);
        void** lookup_lockless_mutable(uint32_t hash, void *__key);

        bool remove(uint32_t hash, void *__key);
        bool remove_lockless(uint32_t hash, void *__key);
        const uint32_t calculate_hash(void *key, uint size) const;

        uint32_t get_size();
        uint32_t get_max_size();
};
