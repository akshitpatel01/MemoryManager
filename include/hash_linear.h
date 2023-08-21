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
#define MULTI_THREAD
#define HASH_MAX_BUCKETS 409600
class Hash_linear: public __hash_base {
    private:
        typedef struct _bucket_t_ {
            private:
                List* m_list;
                mutable std::mutex m_bucket_lock;
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
        } bucket_t;
        typedef enum {
            LOOKUP_FAILED = 0,
            LOOKUP_SUCCESS_OLD_TAB,
            LOOKUP_SUCCESS_NEW_TAB,
        } m_lookup_ret_params;
    private:
        bucket_t* m_cur_table;
        bucket_t* m_old_table;
        uint32_t m_old_tab_ref_cnt;
        uint32_t m_cur_tab_ref_cnt;
        bool (*_m_lookup_func)(void *, void *);
        std::atomic<uint32_t> m_max_size;
        std::atomic<uint32_t> m_cur_size;

    private:
        const uint32_t __universal_hash(void *key, uint size) const;
        bool __insert_lockless(uint32_t hash, void *__key, void *__data);
        bool __remove_lockless(uint32_t hash, void *__key);
        void* __lookup_lockless(bucket_t* _tab, uint32_t hash, void *__key);
        bool __insert(uint32_t hash, void *__key, void *__data);
        bool __remove(uint32_t hash, void *__key);
        void* __lookup(bucket_t* _tab, uint32_t hash, void *__key);
        void** __lookup_lockless_mutable(bucket_t* _tab, uint32_t hash, void *__key);
        bucket_t* __get_cur_tab();
        bucket_t* __get_old_tab();
        void __rel_cur_tab();
        void __rel_old_tab();
        bool __init_table(bucket_t** __tab, uint32_t __size);
        
    public:
        Hash_linear(bool (*__m_lookup_func)(void *, void *));
        bool insert(uint32_t hash, void *__key, void *__data);
        bool remove(uint32_t hash, void *__key);
        void* lookup(uint32_t hash, void *__key);
        const uint32_t calculate_hash(void *key, uint size) const;
        ~Hash_linear();
};
