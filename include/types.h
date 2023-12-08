#pragma once

#include <cstdint>
#include <sys/types.h>
#include <utility>
#include <vector>
namespace pType {
    using segment_ID = uint32_t;
    using node_ID = uint32_t;
    using db_ID = uint32_t;
}

struct node_meta_t {
    pType::node_ID m_id;
    uint32_t m_ip_addr;

    node_meta_t() = default;
};

struct db_meta_t {
    pType::db_ID m_id;
};

#define DEFAULT_DBS_PER_NODE 10
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
        :m_node(std::forward(args...)), m_dbs()
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
    ~node_db_map()
    {}
};

struct db_snapshot_t {
    node_meta_t m_node;
    db_meta_t m_db;    
};

enum action_t {
    NODE_ADD = 1,
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
    public:
        ID_helper() = default;

        T get_id()
        {
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
