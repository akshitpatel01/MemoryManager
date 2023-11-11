#include "hash_simple.h"
#include <iostream>
#include <ostream>

Hash_simple::~Hash_simple()
{
    std::cout << "Table destroyed Cur Size: " << m_cur_size << " Max size: " << m_max_size << std::endl;
}
Hash_simple::Hash_simple(uint32_t _size, std::function<bool(void*,void*)> __m_lookup_func,
                            uint32_t _key_size)
    : m_max_size(_size), m_cur_size(0),
        m_tab(std::make_unique<m_bucket_t[]>(m_max_size)),
        m_lookup_func(__m_lookup_func), m_key_size(_key_size)
{
    uint i;

    if (m_tab == nullptr) {
        return;
    }

    for (i = 0; i < m_max_size; i++) {
        m_tab[i].set_list(new List([this](void* a, void* b) -> bool {

                m_hash_entry_t *m1 = static_cast<m_hash_entry_t *>(a);
                m_hash_entry_t *m2 = static_cast<m_hash_entry_t *>(b);

                return m_lookup_func(m1->m_key, m2->m_key);
                }));
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

    m_hash_entry_t* __entry = new m_hash_entry_t(_key, _data, false);
    if ((ret = m_tab[hash].get_list()->insert_tail(__entry)) == true) {
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
    m_hash_entry_t __entry(__key, nullptr, false);
    m_hash_entry_t* __lookup_entry;
    if (!__key){
        return false;
    }
    
    __lookup_entry = (m_hash_entry_t*)*m_tab[hash].get_list()->lookup_mutable(&__entry);
    if ((__ret = m_tab[hash].get_list()->remove_entry(__lookup_entry)) == true) {
        __ret = true;
        delete __lookup_entry;
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
    m_hash_entry_t __entry(__key, nullptr, false);
    m_hash_entry_t* __entry_lookup = nullptr;
    __entry_lookup = static_cast<m_hash_entry_t*>(m_tab[hash].get_list()->lookup(&__entry));

    if (__entry_lookup && __entry_lookup->m_is_moved) {
        return nullptr;
    }

    return (__entry_lookup) ? __entry_lookup->m_val : nullptr;
}

void**
Hash_simple::lookup_lockless_mutable(uint32_t hash, void *__key)
{
    m_hash_entry_t __entry(__key, nullptr, false);
    m_hash_entry_t* __entry_lookup = nullptr;
    void** __en = nullptr;
    __en = (m_tab[hash].get_list()->lookup_mutable(&__entry));

    if (__en) {
        __entry_lookup = static_cast<m_hash_entry_t*>(*__en);
        if (__entry_lookup && __entry_lookup->m_is_moved) {
            return nullptr;
        }
    }
    
    return (__entry_lookup) ? &(__entry_lookup->m_val) : nullptr;
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
    if ((__temp = new m_hash_iter_t(*(m_tab[0].get_list()))) == nullptr) {
        return nullptr;
    }

    if (__temp->__list_iter == m_tab[__temp->__bucket_ind].get_list()->end()) {
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
    
    //m_tab[_iter->__bucket_ind].get_list()->iter_clear(_iter->__list_iter);
    delete _iter;
}
bool
Hash_simple::__advance_bucket(m_hash_iter_t* _iter)
{
    /* Cleanup */
    m_tab[_iter->__bucket_ind].unlock_bucket();

    _iter->__bucket_ind += 1;
    if (_iter->__bucket_ind >= m_max_size) {
        return false;
    }
    m_tab[_iter->__bucket_ind].lock_bucket();
    _iter->__list_iter = m_tab[_iter->__bucket_ind].get_list()->begin();

    return true;
}
/* Return the first non-null next value if present.
 * If not present, return null.
 */
Hash_simple::m_hash_iter_t* 
Hash_simple::iter_inc(m_hash_iter_t* _iter)
{
    bool __next_item_found = false;

    while (!__next_item_found) {
        auto __list = m_tab[_iter->__bucket_ind].get_list();

        if (_iter->__list_iter == __list->end()) {
            if (__advance_bucket(_iter) == false) {
                delete _iter;
                return nullptr;
            }
        } else {
            _iter->__list_iter++;

            if (_iter->__list_iter != __list->end()) {
                __next_item_found = true;
            }
        }
    }
    return _iter;
}

void* 
Hash_simple::iter_get_val(m_hash_iter_t* _iter)
{
   auto __list = m_tab[_iter->__bucket_ind].get_list();

   if (_iter == nullptr || _iter->__list_iter == __list->end()) {
       return nullptr;
   }

   return _iter->__list_iter.get_val();
}
