#pragma once

#include "hash_base.h"
#include "hash_linear.h"
#include <array>
#include <cstdint>
#include <memory>
#include <pthread.h>
#include <atomic>
extern "C" {
    #include "util.h"
}
#define MULTI_THREAD
#define HASH_MAX_BUCKETS 4096
class Hash_linear: public __hash_base {
    private:
        //std::array<std::unique_ptr<list_t>, HASH_MAX_BUCKETS> _m_buckets;
        //list_t *_m_buckets[HASH_MAX_BUCKETS];
        typedef struct _bucket_t_ {
            private:
                list_t *_list;
                mutable pthread_mutex_t _bucket_lock;
            public:
                void set_list(list_t *__list)
                {
                    _list = __list;
                }
                list_t* get_list() const
                {
                    return _list;
                }
                void lock_init()
                {
                    pthread_mutex_init(&_bucket_lock, NULL);
                }
                void lock_bucket() const
                {
                    pthread_mutex_lock(&_bucket_lock);
                }
                void unlock_bucket() const
                {
                    pthread_mutex_unlock(&_bucket_lock);
                }
                ~_bucket_t_ ()
                {
                    pthread_mutex_destroy(&_bucket_lock);
                    list_delete(_list, false);
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
