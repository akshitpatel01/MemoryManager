#include "hash_simple.h"
#include <iostream>
#include <ostream>

Hash_simple::~Hash_simple()
{
    std::cout << "Table destroyed Cur Size: " << m_cur_size << " Max size: " << m_max_size << std::endl;
}
Hash_simple::Hash_simple(uint32_t _size, bool (*__m_lookup_func)(void *, void *),
                            uint32_t _key_size)
    : m_max_size(_size), m_cur_size(0),
        m_tab(std::make_unique<m_bucket_t[]>(m_max_size)),
        m_lookup_func(__m_lookup_func),
        m_key_size(_key_size)
{
    uint i;

    if (m_tab == nullptr) {
        return;
    }

    for (i = 0; i < m_max_size; i++) {
        m_tab[i].set_list(new List(m_lookup_func));
    }
}

const uint32_t 
Hash_simple::calculate_hash(void* __key, uint __size) const
{
    return __universal_hash(__key, __size);
}
/* private functions
 */
bool 
Hash_simple::update_lockless(uint32_t _hash, void *_key, void *_data)
{
    bool __ret = false;
    void **entry_val = nullptr;
    
    if (!_key){
        return false;
    }
    
    if ((entry_val = lookup_lockless_mutable(_hash, _key)) != nullptr) {
        *entry_val = _data;
        __ret =  true;
    }
    return __ret;
}
bool 
Hash_simple::insert_lockless(uint32_t hash, void *_key, void *_data)
{
    bool ret = false;
    if (!_key) {
        return false;
    }
    
    if (update_lockless(hash, _key, _data) == true) {
        return true;
    }

    if ((ret = m_tab[hash].get_list()->insert_tail(_key, _data)) == true) {
        m_cur_size++;
    }

    return ret;
}
bool 
Hash_simple::insert(uint32_t hash, void *_key, void *_data)
{
    bool __ret = false;

    if (!_key) {
        return false;
    }
    m_tab[hash].lock_bucket();
    __ret = insert_lockless(hash, _key, _data);
    m_tab[hash].unlock_bucket();
    return __ret;
}

bool 
Hash_simple::insert(uint32_t hash, void *__key, void *__data, uint32_t &_size)
{
    bool __ret = insert(hash, __key, __data);
    _size = m_cur_size;
    return __ret;
}
bool 
Hash_simple::remove_lockless(uint32_t hash, void *__key)
{
    bool __ret = false;
    if (!__key){
        return false;
    }
    if ((__ret = m_tab[hash].get_list()->remove(__key, NULL)) == true) {
        __ret = true;
        m_cur_size--;
    }
    return __ret;
}
bool 
Hash_simple::remove(uint32_t hash, void *_key)
{
    bool __ret = false;
    if (!_key) {
        return false;
    }

    m_tab[hash].lock_bucket();
    __ret = remove_lockless(hash, _key);
    m_tab[hash].unlock_bucket();

    return __ret;
}

void*
Hash_simple::lookup_lockless(uint32_t hash, void *__key)
{
    return m_tab[hash].get_list()->lookup(__key, NULL);
}

void**
Hash_simple::lookup_lockless_mutable(uint32_t hash, void *__key)
{
    return m_tab[hash].get_list()->lookup_mutable(__key, NULL);
}

void*
Hash_simple::lookup(uint32_t hash, void *__key)
{
    void* ret = NULL;
    if (m_tab == nullptr || __key == nullptr) {
        return ret;
    }

    m_tab[hash].lock_bucket();
    ret = lookup_lockless(hash, __key);
    m_tab[hash].unlock_bucket();

    return ret;
}

const uint32_t 
Hash_simple::__universal_hash(void* __key, uint __size) const
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
Hash_simple::get_size()
{
    return m_cur_size;
}
uint32_t
Hash_simple::get_max_size()
{
    return m_max_size;
}
Hash_simple::m_hash_iter_t*
Hash_simple::iter_init()
{
    m_hash_iter_t *__temp = nullptr;
    if ((__temp = new m_hash_iter_t()) == nullptr) {
        return nullptr;
    }

    __temp->__bucket_ind = 0;
    m_tab[__temp->__bucket_ind].lock_bucket();
    __temp->__list_iter = m_tab[__temp->__bucket_ind].get_list()->iter_init();

    if (__temp->__list_iter->cur == nullptr) {
        __temp = iter_inc(__temp);
    }
    
    return __temp;
}
void 
Hash_simple::iter_clear(m_hash_iter_t* _iter)
{
    if (_iter == nullptr) {
        return;
    }
    
    m_tab[_iter->__bucket_ind].get_list()->iter_clear(_iter->__list_iter);
    delete _iter;
}
bool
Hash_simple::__advance_bucket(m_hash_iter_t* _iter)
{
    auto __list = m_tab[_iter->__bucket_ind].get_list();
    /* Cleanup */
    m_tab[_iter->__bucket_ind].unlock_bucket();
    __list->iter_clear(_iter->__list_iter);

    _iter->__bucket_ind += 1;
    if (_iter->__bucket_ind >= m_max_size) {
        return false;
    }
    m_tab[_iter->__bucket_ind].lock_bucket();
    _iter->__list_iter = m_tab[_iter->__bucket_ind].get_list()->iter_init();

    return true;
}
/* Return the first non-null next value if present.
 * If not present, return null.
 */
Hash_simple::m_hash_iter_t* 
Hash_simple::iter_inc(m_hash_iter_t* _iter)
{
   do {
       if (_iter->__list_iter->cur == nullptr) {
           if (__advance_bucket(_iter) == false) {
                return nullptr;
           }
       } else {
           auto __list = m_tab[_iter->__bucket_ind].get_list();
           _iter->__list_iter = __list->iter_inc(_iter->__list_iter);
       }
   } while (_iter->__list_iter->cur == nullptr);

   return _iter;
}

void* 
Hash_simple::iter_get_val(m_hash_iter_t* _iter)
{
   auto __list = m_tab[_iter->__bucket_ind].get_list();

   if (_iter == nullptr || _iter->__list_iter == nullptr) {
       return nullptr;
   }

    return __list->iter_get_val(_iter->__list_iter);
}

void* 
Hash_simple::iter_get_key(m_hash_iter_t* _iter)
{
   if (_iter == nullptr || _iter->__list_iter == nullptr) {
       return nullptr;
   }
   
   auto __list = m_tab[_iter->__bucket_ind].get_list();
   return __list->iter_get_key(_iter->__list_iter);
}
void
Hash_simple::set_moved(m_hash_iter_t *_iter)
{
    if (_iter == nullptr) {
        return;
    }

    auto __list = m_tab[_iter->__bucket_ind].get_list();
    __list->set_moved(_iter->__list_iter);
}
