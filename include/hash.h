#pragma once

#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include "hash_linear.h"

template<class __hash_class,
            typename __value_type, typename __key_type>
class Hash {
    private:
        __hash_class m_hashclass_obj;
        __hash_base* m_hash_p;

    public:
        explicit 
        Hash(bool (*lookup_func)(void *, void *))
        : m_hashclass_obj(lookup_func), m_hash_p(&m_hashclass_obj)
        {
        }

        explicit
        Hash(const __hash_class& __external_obj, 
                bool (*lookup_func)(void *, void *))
        : m_hashclass_obj(__external_obj), m_hash_p(&m_hashclass_obj)
        {
        }

        bool insert(__key_type* __key, __value_type* __entry)
        {
            uint32_t _hash = m_hash_p->calculate_hash((void *)__key, sizeof(__key_type));
            return m_hash_p->insert(_hash, __key, __entry);
        }

        bool remove(__key_type* __key)
        {
            uint32_t _hash = m_hash_p->calculate_hash((void *)__key, sizeof(__key_type));
            return m_hash_p->remove(_hash, __key);
        }

        __value_type* lookup(__key_type* __key) const
        {
            uint32_t _hash = m_hash_p->calculate_hash((void *)__key, sizeof(__key_type));
            return (__value_type *) m_hash_p->lookup(_hash, __key);
        }
};


