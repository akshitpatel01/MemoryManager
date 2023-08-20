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
    uint i;
    _max_size = HASH_MAX_BUCKETS;
    _cur_size = 0;

    _m_buckets = new bucket_t[HASH_MAX_BUCKETS]();
    _m_pending_buckets = nullptr;
    m_cur_table = _m_buckets;
    for (i = 0; i < HASH_MAX_BUCKETS; i++) {
        _m_buckets[i].set_list(new List(_m_lookup_func));
        _m_buckets[i].lock_init();
    }
}
bool 
Hash_linear::insert(uint32_t hash, void *__key, void *__data)
{
#ifdef MULTI_THREAD
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
Hash_linear::lookup(uint32_t hash, void *__key) const
{
#ifdef MULTI_THREAD
    return __lookup(hash, __key);
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
Hash_linear::__lookup_lockless(uint32_t hash, void *__key) const
{
    return _m_buckets[hash].get_list()->lookup(__key, NULL);
}

void**
Hash_linear::__lookup_lockless_mutable(uint32_t hash, void *__key) const
{
    return _m_buckets[hash].get_list()->lookup_mutable(__key, NULL);
}

void*
Hash_linear::__lookup(uint32_t hash, void *__key) const
{
    void* ret = NULL;
    if (!__key) {
        return ret;
    }

    _m_buckets[hash].lock_bucket();
    ret = __lookup_lockless(hash, __key);
    _m_buckets[hash].unlock_bucket();

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
