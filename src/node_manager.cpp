#include "node_manager.h"
#include "bsoncxx/builder/basic/document.hpp"
#include "bsoncxx/builder/basic/kvp.hpp"
#include "bsoncxx/json.hpp"
#include "bsoncxx/types.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/options/update.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <sys/types.h>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;
static uint32_t id = 0;


segment::segment(void* _data, uint32_t _data_len, const char* _file_name,
                    uint32_t _segment_id)
    : m_data(_data), m_len(_data_len), m_file_name(_file_name),
      m_segment_id(_segment_id)
{
}

















uint32_t
db_instance::get_id_offset()
{
    return m_id_offset;
    //return (size_t)(&((db_instance*)0)->m_id);
}
void
db_instance::set_id(uint32_t _id)
{
    m_id = _id;
}

const uint32_t
db_instance::get_id()
{
    return m_id;
}

db_instance::db_instance(std::string&& _db_name, 
                            mongocxx::client& _mongo_client)
    : m_name(_db_name), m_id(0), m_id_offset(offsetof(db_instance, m_id)),
     m_database(_mongo_client[m_name]), m_collection(m_database["segments"])
     
     
{
    try {
        const auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        m_database.run_command(ping_cmd.view());
    } catch(const std::exception& e) {
        std::cout << "Cannot connect to database\n" << std::endl;
    }

    auto __index_specification = make_document(
            kvp("m_file_name", 1),
            kvp("m_segment_id", 1)
            );
    m_collection.create_index(std::move(__index_specification));
    
}

bool
db_instance::insert_segment(const char* _file_name, uint32_t _segment_id, void* _segment_data,
                            size_t _segment_size)
{

    /* Add only if not already present.
     * If already present, ignore
     */
    mongocxx::options::update __opts;
    __opts.upsert(true);

    auto __segment_doc = make_document(
            kvp("m_file_name", bsoncxx::types::b_string{_file_name}),
            kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
            kvp("m_segment_len", bsoncxx::types::b_int64{static_cast<int64_t>(_segment_size)}),
            kvp("m_segment_data", bsoncxx::types::b_binary{bsoncxx::binary_sub_type::k_binary, static_cast<uint32_t>(_segment_size), (uint8_t*)_segment_data})
            );
    auto __lookup_segment_doc = make_document(
            kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
            kvp("m_file_name", bsoncxx::types::b_string{_file_name})
            );

    auto __insert_one_result = m_collection.update_one(
            __lookup_segment_doc.view(),
            make_document(
                kvp("$setOnInsert", __segment_doc)
            ).view(),
            __opts
            );

    assert(__insert_one_result);
    /*   
         auto doc_view = doc_value.view();
         auto file_name = doc_view["m_file_name"];
         std::cout << "File name !!!!!!!!! " << file_name.get_string().value << std::endl;
         auto seg = doc_view["m_segment_data"];
         const unsigned char* s = seg.get_binary().bytes;

         char *c = new char[(size_t)((seg.get_binary().size+1)/8)]();
         memcpy(c, s, (seg.get_binary().size+1));
         std::cout << "segment_data: " << c << std::endl;
         std::cout << "segment size: " << seg.get_binary().size << std::endl;
         std::cout << "string size: " << strlen(c) << std::endl;
         */
    return true;
}

std::shared_ptr<segment>
db_instance::lookup_segment(const char* _file_name, uint32_t _segment_id)
{
    std::shared_ptr<segment> __ret_segment;

    auto __lookup_segment = m_collection.find_one(make_document(
                kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
                kvp("m_file_name", bsoncxx::types::b_string{_file_name})
                ));

    if (__lookup_segment) {
        //std::cout << bsoncxx::to_json(__lookup_segment->view()) << std::endl;
        auto segment_view = __lookup_segment->view(); 
        auto __file_name = segment_view["m_file_name"];
        auto __segment = segment_view["m_segment_data"];
        auto __segment_len = segment_view["m_segment_len"];
        auto __segment_id = segment_view["m_segment_id"];
        //auto test1 = segment_view["test"];

        //std::cout << "testing: " << test1.get_int64().value << "\n";
        return segment::create_shared(
                (void*)(__segment.get_binary().bytes),
                (uint64_t)__segment_len.get_int64().value,
                __file_name.get_string().value.to_string().c_str(),
                (uint32_t)__segment_id.get_int64().value
                );
    }

    return nullptr;
}

bool
db_instance::remove_segment(const char* _file_name, uint32_t _segment_id)
{
    auto __rem_doc = make_document(
            kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
            kvp("m_file_name", bsoncxx::types::b_string{_file_name})
            );

    auto __del_one_res = m_collection.delete_one(__rem_doc.view());
    assert(__del_one_res->deleted_count() == 1);

    return true;
}
void
db_instance::del_all()
{
    m_collection.delete_many({});
}
db_instance::~db_instance()
{
}




bool
node_manager::m_db_hash_lookup(void* _a, void* _b)
{
    uint32_t* __db1_key = static_cast<uint32_t*>(_a);
    uint32_t* __db2_key = static_cast<uint32_t*>(_b);
    
    return (*__db1_key == *__db2_key);
}
node_manager::node_manager()
    : m_db_hash(m_db_hash_lookup), m_instance(), m_mong_client(mongocxx::uri{})
{
}

bool
node_manager::add_db(std::string& db_name)
{
    /* TODO: check for duplicates*/

    db_instance *__new_db = new db_instance(std::move(db_name), m_mong_client);
    if (__new_db == nullptr) {
        return false;
    }

    /* TODO: Register with blob manager */
    __new_db->set_id(++id);

    m_db_hash.insert((uint32_t*)(((char*)__new_db) + __new_db->get_id_offset()), __new_db);

    uint key = 1;
    if (m_db_hash.lookup(&key)) {
        std::cout << "Added successfully\n";
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
    
}
bool 
node_manager::insert(uint32_t _db_id, const char* _file_name, uint32_t _segment_id,
                        void* _segment_data, size_t _segment_size)
{
    db_instance* __db = nullptr;

    if ((__db = m_db_hash.lookup(&_db_id))) {
        return __db->insert_segment(_file_name, _segment_id, _segment_data,
                                    _segment_size);
    } else {
        std::cout << "Database not present\n";
        return false;
    }
}

std::shared_ptr<segment>
node_manager::lookup(uint32_t _db_id, const char* _file_name,
                     uint32_t _segment_id)
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
node_manager::remove(uint32_t _db_id, const char* _file_name,
                    uint32_t _segment_id)
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
    uint id = 1;

    if ((__db = m_db_hash.lookup(&id))) {
        std::cout << "Freed\n";
        delete __db;
    }
    std::cout << "node manager destroyed\n";
}
