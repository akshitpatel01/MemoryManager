#include "node_manager.h"
#include "db_instance.h"
#include "list.h"
#include "mongocxx/options/pool.hpp"
#include <cstdint>
#include <string>
#include <sys/types.h>

static uint32_t id = 0;

bool
node_manager::m_db_hash_lookup(void* _a, void* _b)
{
    uint32_t* __db1_key = static_cast<uint32_t*>(_a);
    uint32_t* __db2_key = static_cast<uint32_t*>(_b);
    
    return (*__db1_key == *__db2_key);
}

node_manager::node_manager()
    : m_db_hash([](void* _a, void* _b) -> bool {
            uint32_t* __db1_key = static_cast<uint32_t*>(_a);
            uint32_t* __db2_key = static_cast<uint32_t*>(_b);
            
            return (*__db1_key == *__db2_key);
            }),
        m_db_list([](void* _a, void* _b) -> bool {
            uint32_t* __db1_key = static_cast<uint32_t*>(_a);
            uint32_t* __db2_key = static_cast<uint32_t*>(_b);
            
            return (*__db1_key == *__db2_key);
            }), m_instance{}, m_client_pool{mongocxx::uri{"mongodb://localhost:27017/?minPoolSize=100&maxPoolSize=100"}}
{
}

bool
node_manager::add_db(std::string& db_name)
{
    /* TODO: check for duplicates*/

    db_instance *__new_db = new db_instance(std::move(db_name), &m_client_pool);
    if (__new_db == nullptr) {
        return false;
    }

    /* TODO: Register with blob manager */
    __new_db->set_id(++id);

#ifdef LOGS
    std::cout << __new_db->get_id() << "\n";
#endif
    m_db_hash.insert((uint32_t*)(((char*)__new_db) + __new_db->get_id_offset()), __new_db);
    m_db_list.insert_tail(__new_db);

    uint key = __new_db->get_id();
    if (m_db_hash.lookup(&key)) {
#ifdef LOGS
        std::cout << "Added successfully\n";
#endif
    }
    return true;
}

std::string node_manager::test()
{
    return "Test success!!\n";
}

void
node_manager::del_all(uint32_t _db_id)
{
    db_instance* __db = nullptr;

    if ((__db = m_db_hash.lookup(&_db_id))) {
        __db->del_all();
    } else {
        std::cout << "Database not present\n";
    }

}
bool 
node_manager::insert(const std::unique_ptr<segment_t>& _segment, uint32_t _db_id)
{
    db_instance* __db = nullptr;

    std::cout << "ID2: " << _segment->get_id() << "\n";
    if ((__db = m_db_hash.lookup(&_db_id))) {
        return __db->insert_segment(_segment->get_file_name().data(), _segment->get_id(), _segment->get_data(),
                                    _segment->get_len());
    } else {
        std::cout << "Database not present\n";
        return false;
    }
}

extern bsoncxx::document::view global_view;
std::unique_ptr<node_manager::segment_t>
node_manager::lookup(const char* _file_name, uint32_t _segment_id, uint32_t _db_id)
{
    db_instance* __db = nullptr;

    if ((__db = m_db_hash.lookup(&_db_id))) {
        return __db->lookup_segment(_file_name, _segment_id);
    } else {
        std::cout << "Database not present\n";
        return nullptr;
    }
}
bool
node_manager::remove(const char* _file_name, uint32_t _segment_id, uint32_t _db_id)
{
    db_instance* __db = nullptr;

    if ((__db = m_db_hash.lookup(&_db_id))) {
        return __db->remove_segment(_file_name, _segment_id);
    } else {
        std::cout << "Database not present\n";
        return false;
    }
}
node_manager::~node_manager()
{

    db_instance* __db = nullptr;
    uint32_t id;
    
    for(List::Iterator it = m_db_list.begin(); it != m_db_list.end(); it++) {
        __db = static_cast<db_instance*>(it.get_val());
        id = __db->get_id();
        if ((__db = m_db_hash.lookup(&id))) {
            std::cout << "Freed\n";
            delete __db;
        }
    }
    
    std::cout << "node manager destroyed\n";
}
