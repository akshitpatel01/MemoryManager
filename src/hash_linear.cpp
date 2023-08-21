#include "hash_linear.h"
#include "list.h"
#include "util.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <stdlib.h>

/* public functions
 */

Hash_linear::~Hash_linear()
{
    std::cout << "Cur size: " << _cur_size << std::endl;
    delete[] _m_buckets;
}
Hash_linear::Hash_linear(bool (*__m_lookup_func)(void *, void *))
    :_m_lookup_func(__m_lookup_func)
{
    m_max_size = HASH_MAX_BUCKETS;
    m_cur_size = 0;
    m_cur_tab_ref_cnt = 0;
    m_old_tab_ref_cnt = 0;

    __init_table(&m_cur_table, m_max_size);
    m_old_table = nullptr;
}

bool Hash_linear::__init_table(bucket_t** __tab, uint32_t __size)
{
    uint i;

    *__tab = new bucket_t[HASH_MAX_BUCKETS]();
    if (*__tab == nullptr) {
        return false;
    }
    for (i = 0; i < __size; i++) {
        (*__tab[i]).set_list(new List(_m_lookup_func));
        (*__tab[i]).lock_init();
    }

    return true;
}
Hash_linear::bucket_t*
Hash_linear::__get_cur_tab()
{
    m_cur_tab_ref_cnt++;
    return m_cur_table;
}

Hash_linear::bucket_t*
Hash_linear::__get_old_tab()
{
    if (m_old_table != nullptr) {
        m_old_tab_ref_cnt++;
        return m_old_table;
    }

    return nullptr;
}

void 
Hash_linear::__rel_cur_tab()
{
    m_cur_tab_ref_cnt--;
}

void
Hash_linear::__rel_old_tab()
{
    m_old_tab_ref_cnt--;
}

bool 
Hash_linear::insert(uint32_t hash, void *__key, void *__data)
{
    void *__entry_found = nullptr;
#ifdef MULTI_THREAD
        __entry_found = lookup(hash, __key);
        if (__entry_found) {

        }
    return __insert(hash, __key, __data);
#else
    return __insert_lockless(hash, __key, __data);
#endif
}

bool 
Hash_linear::remove(uint32_t hash, void *__key)
{
#ifdef MULTI_THREAD
    return __remove(hash, __key);
#else
    return __remove_lockless(hash, __key);
#endif
}

void* 
Hash_linear::lookup(uint32_t hash, void *__key)
{
    void* entry_found = nullptr;
    bucket_t* old_tab = nullptr;
    bucket_t* cur_tab = nullptr;

#ifdef MULTI_THREAD
    old_tab = __get_old_tab();
    if (old_tab) {
        entry_found = __lookup(old_tab, hash, __key);
        if (entry_found != nullptr) {
            __rel_old_tab();
            return entry_found;
        }
        __rel_old_tab();
    }

    cur_tab = __get_cur_tab();
    if (cur_tab == nullptr) {
        std::cout << "some thing is not right\n";
    }
    entry_found = __lookup(cur_tab, hash, __key);
    return entry_found;
#else
    return __lookup_lockless(hash, __key);
#endif
}

const uint32_t 
Hash_linear::calculate_hash(void* __key, uint __size) const
{
    return __universal_hash(__key, __size);
}

/* private functions
 */
bool 
Hash_linear::__insert_lockless(uint32_t hash, void *__key, void *__data)
{
    int ret;
    void **entry_val = nullptr;
    if (!__key) {
        return false;
    }
    
    if ((entry_val = __lookup_lockless_mutable(hash, __key)) != nullptr) {
        *entry_val = __data;
        return true;
    }


    if ((ret = _m_buckets[hash].get_list()->insert_tail(__key, __data)) == true) {
        _cur_size++;
    }

    return ret;
}
bool 
Hash_linear::__insert(uint32_t hash, void *__key, void *__data)
{
    bool ret = false;

    if (!__key) {
        return false;
    }

    _m_buckets[hash].lock_bucket();
    ret = __insert_lockless(hash, __key, __data);
    _m_buckets[hash].unlock_bucket();

    return ret;
}

bool 
Hash_linear::__remove_lockless(uint32_t hash, void *__key)
{
    int ret;
    if (!__key) {
        return false;
    }
    if ((ret = _m_buckets[hash].get_list()->remove(__key, NULL)) == true) {
        _cur_size--;
    }
    return ret;
}
bool 
Hash_linear::__remove(uint32_t hash, void *__key)
{
    bool ret = false;
    if (!__key) {
        return false;
    }

    _m_buckets[hash].lock_bucket();
    ret = __remove_lockless(hash, __key);
    _m_buckets[hash].unlock_bucket();

    return ret;
}

void*
Hash_linear::__lookup_lockless(bucket_t* _tab, uint32_t hash, void *__key)
{
    return _tab[hash].get_list()->lookup(__key, NULL);
}

void**
Hash_linear::__lookup_lockless_mutable(bucket_t* _tab, uint32_t hash, void *__key)
{
    return _tab[hash].get_list()->lookup_mutable(__key, NULL);
}

void*
Hash_linear::__lookup(bucket_t* _tab, uint32_t hash, void *__key)
{
    void* ret = NULL;
    if (_tab == nullptr || __key == nullptr) {
        return ret;
    }

    _tab[hash].lock_bucket();
    ret = __lookup_lockless(_tab, hash, __key);
    _tab[hash].unlock_bucket();

    return ret;
}

const uint32_t 
Hash_linear::__universal_hash(void* __key, uint __size) const
{
    unsigned long hash = 5381;
    int c;
    char *c_tmp = static_cast<char*>(__key);
    __size -= sizeof(int);
    __size++;

    while (__size--) {
        c = (*(int *)c_tmp++);
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return (hash % HASH_MAX_BUCKETS);
}
