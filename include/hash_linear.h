#pragma once

#include "hash_base.h"
#include "hash_linear.h"
#include "hash_simple.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <atomic>
#include "list.h"
#include <cassert>
#include <future>
#include <shared_mutex>

class Hash_linear: public __hash_base {
    private:
        bool (*m_lookup_func)(void *, void *);
        std::shared_ptr<Hash_simple> m_cur_tab;
        std::shared_ptr<Hash_simple> m_old_tab;
        std::future<bool> future_g;
        std::mutex m_migration_lock;
        std::shared_mutex m_cur_tab_lock;
        std::shared_mutex m_old_tab_lock;
        uint32_t m_key_size;
        uint32_t m_max_size;

    private:
        const uint32_t __universal_hash(void *key, uint size) const;
        std::shared_ptr<Hash_simple> __get_cur_tab();
        std::shared_ptr<Hash_simple> __get_old_tab();
        void __rel_cur_tab(std::shared_ptr<Hash_simple>& __tab);
        void __rel_old_tab(std::shared_ptr<Hash_simple>& __tab);
        bool __init_table(std::shared_ptr<Hash_simple>& _tab, uint32_t __size);
        void __del_tab(std::shared_ptr<Hash_simple> &_tab);
        bool __start_migration();
        bool __migration_internal();
        
    public:
        Hash_linear(bool (*__m_lookup_func)(void *, void *), uint32_t _key_size);
        bool insert(uint32_t hash, void *__key, void *__data);
        bool remove(uint32_t hash, void *__key);
        void* lookup(uint32_t hash, void *__key);
        const uint32_t calculate_hash(void *key, uint size) const;
        uint32_t get_size();
        ~Hash_linear();
};
