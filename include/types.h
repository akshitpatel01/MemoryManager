#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <random>
#include <sys/types.h>
#include <utility>
#include <vector>
//#define LOGS
namespace pType {
    using segment_ID = uint32_t;
    using node_ID = uint32_t;
    using db_ID = uint32_t;
}

struct node_meta_t {
    pType::node_ID m_id;
    uint32_t m_ip_addr;

    node_meta_t() = default;
    node_meta_t(uint32_t _id, uint32_t _ip)
        :m_id(_id), m_ip_addr(_ip)
    {}
};

struct db_meta_t {
    pType::db_ID m_id;
};

#define DEFAULT_DBS_PER_NODE 10
#define SEGMENT_SIZE 1000000
#define REPLICATION_FACTOR 20
struct node_db_map {
    node_meta_t m_node;
    std::vector<db_meta_t> m_dbs;

    node_db_map(node_meta_t& _node)
        :m_node(_node), m_dbs()
    {
        m_dbs.reserve(DEFAULT_DBS_PER_NODE);
    }
    template<typename... ArgsType>
    node_db_map(ArgsType&&... args)
        :m_node(std::forward<ArgsType>(args)...), m_dbs()
    {
    }

    bool add_db(pType::db_ID _dID)
    {
        m_dbs.push_back({_dID});
        return true;
    }
    bool rem_db(pType::db_ID _dID)
    {
        uint i;
        for (i = 0; i < m_dbs.size(); i++) {
            if (m_dbs[i].m_id == _dID) {
                m_dbs[i] = m_dbs.back();
                m_dbs.pop_back();
                return true;
            }
        }

        return false;
    }
    friend std::ostream& operator<< (std::ostream& stream, const node_db_map& _map)
    {
        char ip[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &_map.m_node.m_ip_addr, ip, INET_ADDRSTRLEN))
        {
        }
        stream << "NodeId: " << _map.m_node.m_id << "\n";
        stream << "NodeIP: " << ip << "\n";
        //stream << "NodeIP: " << _map.m_node.m_ip_addr << "\n";
        for (auto& it: _map.m_dbs) {
            stream << "DBID: " << it.m_id << "\n";
        }

        return stream;
    }
    ~node_db_map()
    {}
};


struct db_snapshot_t {
    node_meta_t m_node;
    db_meta_t m_db;    
};

enum action_t {
    NODE_ADD = 0,
    DB_ADD,
    NODE_REM,
    DB_REM
};

enum {
    INVALID_ID = 0,
};

template <typename T>
class ID_helper {
    private:
        std::vector<T> m_free_list;
        T m_max_id;
        T m_cur_id;
        std::mutex m_lock;
    public:
        ID_helper(T _max_id, T start = 1)
            :m_max_id(_max_id), m_cur_id{start}
        {
        }

        T get_id()
        {
            std::scoped_lock<std::mutex> lock_(m_lock);
            T __ret = m_max_id;

            if (m_free_list.size() > 0) {
                __ret = m_free_list.back();
                m_free_list.pop_back();
            } else if (m_cur_id <= m_max_id) {
                __ret = m_cur_id++;
            }

            return __ret;
        }

        void free_id(T& _id)
        {
            m_free_list.push_back(_id);
        }
};

inline size_t hash_combine( size_t lhs, size_t rhs) {
    if constexpr (sizeof(size_t) >= 8) {
        lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
    } else {
        lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    }
    return lhs;
}

template<typename K, typename V>
class std::hash<std::pair<K,V>> {
    public:
    size_t operator() (std::pair<K,V>& _obj)
    {
        return hash_combine(std::hash<K>{}(_obj.first), std::hash<V>{}(_obj.second));
    }
};
template <typename T, size_t N>
class Hashfn {
    public:
    std::array<size_t, N> operator() (T& key)
   {
       std::minstd_rand0 rng(std::hash<T>{}(key));
       std::array<std::size_t, N> hashes;
       std::generate(std::begin(hashes), std::end(hashes), rng);
       return hashes;
   }
};

template<size_t N>
class Hashfn<void*, N> {
    uint64_t m_offset = 14695981039346656037U;
    uint64_t m_prime = 1099511628211;
    public:
        std::array<size_t, N> operator() (void* _data, size_t _size)
        {
            size_t _hash = m_offset; 
            size_t i = 0;

            for (; i < _size; i++) {
                _hash ^= static_cast<char*>(_data)[i];
                _hash *= m_prime;
            }

            std::minstd_rand0 rng(_hash);
            std::array<std::size_t, N> hashes;
            std::generate(std::begin(hashes), std::end(hashes), rng);
            return hashes;
        }
};
