#pragma once

#include <cstdint>
#include <sys/types.h>
class __hash_base {
    public:
        virtual bool insert(uint32_t hash, void *__key, void *__data) = 0;
        virtual bool remove(uint32_t hash, void *__key) = 0;
        virtual void* lookup(uint32_t hash, void *__key) = 0;
        virtual const uint32_t calculate_hash(void *key, uint size) const = 0;
        virtual void* iter_init(void) = 0;
        virtual void* iter_advance(void*) = 0;
        virtual void iter_cleanup(void*) = 0;
};
