#include "hash_linear.h"
#include "hash_simple.h"
#include "list.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <ratio>
#include <stdlib.h>
#include <sys/types.h>
#include <thread>
#include <tuple>

/* public functions
 */

Hash_linear::~Hash_linear()
{
    __del_tab(m_cur_tab);
    __del_tab(m_old_tab);
}
void
Hash_linear::__del_tab(std::shared_ptr<Hash_simple>& _tab) 
{
    _tab.reset();
}
Hash_linear::Hash_linear(std::function<bool(void*,void*)> __m_lookup_func, uint32_t _key_size)
    :m_lookup_func(__m_lookup_func), m_key_size(_key_size), m_max_size(HASH_MAX_BUCKETS)
{

    __init_table(m_cur_tab, HASH_MAX_BUCKETS);
    m_old_tab = nullptr;
}

bool 
Hash_linear::__init_table(std::shared_ptr<Hash_simple>& _tab, uint32_t _size)
{

    Hash_simple *__temp_obj = nullptr;

    __temp_obj = new Hash_simple(_size, m_lookup_func, m_key_size);
    if(__temp_obj == nullptr) {
        return false;
    }

    _tab.reset(std::move(__temp_obj));
    return true;
}
std::shared_ptr<Hash_simple>
Hash_linear::__get_cur_tab()
{
    m_cur_tab_lock.lock_shared();
    if (m_cur_tab == nullptr) {
        m_cur_tab_lock.unlock_shared();
        return nullptr;
    } else {
    }
    return m_cur_tab;
}

std::shared_ptr<Hash_simple>
Hash_linear::__get_old_tab()
{
    m_old_tab_lock.lock_shared();
    if (m_old_tab == nullptr) {
        m_old_tab_lock.unlock_shared();
        return nullptr;
    } else {
    }
    return m_old_tab;
}

void
Hash_linear::__rel_old_tab(std::shared_ptr<Hash_simple>& __tab)
{
    __tab.reset();
    m_old_tab_lock.unlock_shared();
}
void
Hash_linear::__rel_cur_tab(std::shared_ptr<Hash_simple>& __tab)
{
    __tab.reset();
    m_cur_tab_lock.unlock_shared();
}
bool 
Hash_linear::insert(uint32_t hash, void *_key, void *_data)
{
    bool __ret = false;
    std::shared_ptr<Hash_simple> __tab = nullptr;
    uint32_t __size;
    bool __need_migration = false;

#ifdef MULTI_THREAD
    if ((__tab = __get_old_tab()) != nullptr) {
        __ret = __tab->update_lockless(hash, _key, _data);
        __rel_old_tab(__tab);
        if (__ret) {
            return __ret;
        }
    }

    __tab = __get_cur_tab();
    if (__tab == nullptr) {
        std::cout << "Crash pls\n";
        exit(0);
        return false;
    }

    __ret =  __tab->insert(hash, _key, _data, __size);
    if (__size > (0.75 * m_max_size)) {{
        __need_migration = true;
    }}
    __rel_cur_tab(__tab);

    if (__need_migration) {
        //std::cout << "Need migrtion " << "Size: " << __size << std::endl;
        __start_migration();
    }
    return __ret;
#else
    return __insert_lockless(hash, __key, __data);
#endif
}

bool 
Hash_linear::remove(uint32_t _hash, void *_key)
{
    bool __ret = false;
    std::shared_ptr<Hash_simple> __tab = nullptr;
#ifdef MULTI_THREAD
    if ((__tab = __get_old_tab()) != nullptr) {
        __ret = __tab->remove(_hash, _key);
        __rel_old_tab(__tab);
        if (__ret == true) {
            return __ret;
        } else {
            __tab.reset();
        }
    }
    
    __tab = __get_cur_tab();
    if (__tab == nullptr) {
        std::cout << "Crash pls\n";
        exit(0);
        return false;
    }

    __ret = __tab->remove(_hash, _key);
    __rel_cur_tab(__tab);

    return __ret;
#else
    return __remove_lockless(hash, __key);
#endif
}

void* 
Hash_linear::lookup(uint32_t _hash, void *_key)
{
    void* entry_found = nullptr;
    std::shared_ptr<Hash_simple> old_tab = nullptr;
    std::shared_ptr<Hash_simple> cur_tab = nullptr;

#ifdef MULTI_THREAD
    old_tab = __get_old_tab();
    if (old_tab) {
        entry_found = old_tab->lookup(_hash, _key);
        __rel_old_tab(old_tab);
        if (entry_found != nullptr) {
            return entry_found;
        }
    }

    cur_tab = __get_cur_tab();
    if (cur_tab == nullptr) {
        std::cout << "some thing is not right\n";
        exit(0);
    }
    entry_found = cur_tab->lookup(_hash, _key);
    __rel_cur_tab(cur_tab);
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

uint32_t
Hash_linear::get_size()
{
    uint32_t __ret = 0;
    std::shared_ptr<Hash_simple> __tab = nullptr;

    if ((__tab = __get_old_tab()) != nullptr) {
        __ret += __tab->get_size();
        __rel_old_tab(__tab);
    }

    __tab = __get_cur_tab();
    if (__tab == nullptr) {
        std::cout << "Crash pls\n";
        exit(0);
        return 0;
    }
    __ret += __tab->get_size();
    __rel_cur_tab(__tab);

    return __ret;
}

bool
Hash_linear::__migration_internal()
{
    Hash_simple::m_hash_iter_t* __hash_iter = nullptr;
    Hash_simple::hash_entry_t* __item = nullptr;
    void *__key = nullptr;
    void *__val = nullptr;
    uint32_t __new_hash;
    std::shared_ptr<Hash_simple> _old_tab = nullptr;
    std::shared_ptr<Hash_simple> _new_tab = nullptr;
    std::lock_guard<std::mutex> __guard(m_tab_lock);

    if (m_iter_count > 0) {
        auto fut = std::async(std::launch::async, &Hash_linear::__migration_internal, this);
        future_g = std::move(fut);
        return true;
    }
    _old_tab = __get_old_tab();
    if (_old_tab == nullptr) {
        return false;
    }
    _new_tab = __get_cur_tab();
    if (_new_tab == nullptr) {
        std::cout << "Crash psl\n";
        exit(0);
        return false;
    }

    if ((__hash_iter = _old_tab->iter_init()) == nullptr) {
        __rel_old_tab(_old_tab);
        __rel_cur_tab(_new_tab);
        return false;
    }

    //int count = 0;
    while(__hash_iter) {
        // del && add
        //count++;
        __item = static_cast<Hash_simple::hash_entry_t*>(_old_tab->iter_get_val(__hash_iter));
        __key = __item->m_key;
        __val = __item->m_val;
        __item->m_is_moved = true;
        //_old_tab->remove(__hash_iter->__bucket_ind, __key);
        __new_hash = _new_tab->calculate_hash(__key, m_key_size);
        _new_tab->insert(__new_hash, __key, __val);
        __hash_iter = _old_tab->iter_inc(__hash_iter);
    }
    _old_tab->iter_clear(__hash_iter);
    __rel_cur_tab(_new_tab);
    __rel_old_tab(_old_tab);
    

    m_old_tab_lock.lock();
    m_old_tab.reset();
    m_old_tab_lock.unlock();
    return true;
}

bool
Hash_linear::__start_migration()
{
    Hash_simple *__new_tab = nullptr;
    std::shared_ptr<Hash_simple> __cur_tab;

    std::lock_guard<std::mutex> __guard(m_migration_lock);
    __cur_tab = __get_old_tab();
    //auto __status = future_g.wait_for(std::chrono::milliseconds(0));
    if (__cur_tab != nullptr) {
        __rel_old_tab(__cur_tab);
        return true;
    }
    __cur_tab = __get_cur_tab();
    __new_tab = new Hash_simple(__cur_tab->get_max_size()*2, m_lookup_func, m_key_size);
    if (__new_tab == nullptr) {
        return false;
    }

    m_max_size = __cur_tab->get_max_size()*2;
    __rel_cur_tab(__cur_tab);
    m_old_tab_lock.lock();
    m_cur_tab_lock.lock();
    m_old_tab = m_cur_tab;
    m_cur_tab.reset(std::move(__new_tab));
    m_old_tab_lock.unlock();
    m_cur_tab_lock.unlock();

    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto fut = std::async(std::launch::async, &Hash_linear::__migration_internal, this);
    future_g = std::move(fut);
    return true;
}

void*
Hash_linear::iter_init(void)
{
    std::lock_guard<std::mutex> __guard(m_tab_lock);
    m_hash_iter_t* __hash_iter = nullptr;
    std::shared_ptr<Hash_simple> _old_tab = nullptr;
    std::shared_ptr<Hash_simple> _new_tab = nullptr;
    
    _old_tab = __get_old_tab();
    if (_old_tab == nullptr) {
        _new_tab = __get_cur_tab();
        if (_new_tab == nullptr) {
            std::cout << "Crash psl\n";
            exit(0);
        }
        __hash_iter = new m_hash_iter_t(_new_tab->iter_init(), _new_tab, false);
        if (__hash_iter == nullptr) {
            std::cout << "Iter init failed\n";
            return nullptr;
        }
    } else {
        __hash_iter = new m_hash_iter_t(_old_tab->iter_init(), _old_tab, true);
        if (__hash_iter == nullptr) {
            std::cout << "Iter init failed\n";
            return nullptr;
        }
    }

    m_iter_count++;
    return __hash_iter;
}

void*
Hash_linear::iter_advance(void* _iter)
{
    m_hash_iter_t* __iter = static_cast<m_hash_iter_t*>(_iter);
    void* ret = nullptr;
    std::shared_ptr<Hash_simple> __tab;
    std::shared_ptr<Hash_simple> __old_tab;

    if (!__iter) {
        return nullptr;
    }
    if (__iter && __iter->m_iter) {
        __tab = __iter->m_tab;
        ret = __tab->iter_get_val(__iter->m_iter);
        __tab->iter_inc(__iter->m_iter);
    }

    if (!__iter) {
        __old_tab = __get_old_tab();
        if (__old_tab == __tab) {
            __iter = (m_hash_iter_t*)iter_init();
        }
    }

    return ret;
}
void
Hash_linear::iter_cleanup(void* _iter)
{
}
