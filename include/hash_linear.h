#pragma once

#include "hash_base.h"
#include "hash_linear.h"
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <atomic>
#include "list.h"
#include <cassert>
extern "C" {
    #include "util.h"
}
//#define MULTI_THREAD
#define HASH_MAX_BUCKETS 409600
class Hash_linear: public __hash_base {
    private:
        typedef struct _bucket_t_ {
            private:
                list_t *_list;
                List* m_list;
                mutable std::mutex m_bucket_lock;
            public:
                _bucket_t_()
                    :_list(nullptr), m_list(nullptr)
                {

                }
                void set_list(list_t *__list)
                {
                    _list = __list;
                }
                void set_list(List *__list)
                {
                    m_list = __list;
                }
                
#ifdef C_LIST
                list_t* get_list() const
                {
                    return _list;
                }
#else
                List* get_list() const
                {
                    return m_list;
                }
#endif
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
                    list_delete(_list, false);
                    delete m_list;
                }
        } bucket_t;
        bucket_t _m_buckets[HASH_MAX_BUCKETS];
        bool (*_m_lookup_func)(void *, void *);
        std::atomic<uint32_t> _max_size;
        std::atomic<uint32_t> _cur_size;

    private:
        const uint32_t __universal_hash(void *key, uint size) const;
        bool __insert_lockless(uint32_t hash, void *__key, void *__data);
        bool __remove_lockless(uint32_t hash, void *__key);
        void* __lookup_lockless(uint32_t hash, void *__key) const;
        bool __insert(uint32_t hash, void *__key, void *__data);
        bool __remove(uint32_t hash, void *__key);
        void* __lookup(uint32_t hash, void *__key) const;
        
    public:
        Hash_linear(bool (*__m_lookup_func)(void *, void *));
        bool insert(uint32_t hash, void *__key, void *__data);
        bool remove(uint32_t hash, void *__key);
        void* lookup(uint32_t hash, void *__key) const;
        const uint32_t calculate_hash(void *key, uint size) const;
        ~Hash_linear();
};
