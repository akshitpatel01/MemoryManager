#pragma once

/* Dependency: Central Manager
 */
#include "central_manager.h"
#include "types.h"
#include <cstddef>
#include <functional>
#include <limits>
#include <mutex>
#include <set>


class Hash_helper {
    public:
        Hash_helper(Central_manager& m_manager)
        {
            m_manager.register_db_update_cb(std::bind(&Hash_helper::handle_db_update, this, std::placeholders::_1, std::placeholders::_2));
        }

    public:
        bool handle_db_update(db_meta_t& _db, bool _add)
        {
            std::scoped_lock<std::mutex> lock_(m_lock);

            if (_add) {
                auto hash_array = Hashfn<pType::db_ID, REPLICATION_FACTOR>{}(_db.m_id);
                for (auto& it: hash_array) {
                    m_consistent_hash.insert({it, _db.m_id});
                }
                return true;
            } else {
                for (auto& it: m_consistent_hash) {
                    if (it.second == _db.m_id) {
                        m_consistent_hash.erase(it);
                    }
                }

                return true;
            }
        }

        pType::db_ID get_db(size_t _hash)
        {
            return 1;
           if (m_consistent_hash.size() == 0) {
               return INVALID_ID;
           }

           auto it = m_consistent_hash.lower_bound({_hash,std::numeric_limits<pType::db_ID>::min()});
           if (it == m_consistent_hash.end()) {
               it = m_consistent_hash.begin();
           }

           return it->second;
        }
    
    private:
        std::set<std::pair<size_t, pType::db_ID>> m_consistent_hash;
        std::mutex m_lock;
};
