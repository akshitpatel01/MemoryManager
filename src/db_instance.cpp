#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include "db_instance.h"
#include "bsoncxx/document/value.hpp"
#include "mongocxx/database.hpp"
#include <mongocxx/exception/error_code.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/server_error_code.hpp>
#include "segment.h"

bsoncxx::document::view global_view;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

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
                            mongocxx::pool* _mongo_client)
    : m_name(std::move(_db_name)), m_id(0), m_id_offset(offsetof(db_instance, m_id)),
      m_client_pool(_mongo_client)
{
    auto client_ = m_client_helper(*m_client_pool, m_name);
    try {
        auto database_ = client_.get_database();
        const auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        database_.run_command(ping_cmd.view());
    } catch(const std::exception& e) {
        std::cout << "Cannot connect to database\n" << std::endl;
    }

    auto __index_specification = make_document(
            kvp("m_file_name", 1),
            kvp("m_segment_id", 1)
            );
    auto collection_ = client_.get_collection();
    collection_.create_index(std::move(__index_specification));
    
}

bool
db_instance::insert_segment(const char* _file_name, uint32_t _segment_id, const void* _segment_data,
                            size_t _segment_size)
{

    /* Add only if not already present.
     * If already present, ignore
     */
    mongocxx::options::update __opts;
    auto client_ = m_client_helper(*m_client_pool, m_name);
    auto collection_ = client_.get_collection();
#if 0
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

    auto __insert_one_result = collection_.update_one(
            __lookup_segment_doc.view(),
            make_document(
                kvp("$setOnInsert", __segment_doc)
                ).view(),
            __opts
            );
#endif
    auto __insert_one_result = collection_.insert_one(make_document(
                kvp("m_file_name", bsoncxx::types::b_string{_file_name}),
                kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
                kvp("m_segment_len", bsoncxx::types::b_int64{static_cast<int64_t>(_segment_size)}),
                kvp("m_segment_data", bsoncxx::types::b_binary{bsoncxx::binary_sub_type::k_binary, static_cast<uint32_t>(_segment_size), (uint8_t*)_segment_data})
                ));

#ifdef LOGS
    std::cout << "Inserted segment: " << _segment_id << " DBID: " << m_id << "\n";
#endif
    //if (_segment_id == 10485)
    //if(__insert_one_result)
    //std::cout << _segment_id << ": " << __insert_one_result->inserted_id().get_string().value << "\n";
    assert(__insert_one_result);
    //return (__insert_one_result->inserted_id() == 1);
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

std::unique_ptr<db_instance::segment_t>
db_instance::lookup_segment(const char* _file_name, uint32_t _segment_id)
{
    auto client_ = m_client_helper(*m_client_pool, m_name);
    auto collection_ = client_.get_collection();

    auto __lookup_segment = collection_.find_one(make_document(
                kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
                kvp("m_file_name", bsoncxx::types::b_string{_file_name})
                ));

    if (__lookup_segment) {
#ifdef LOGS
        std::cout << bsoncxx::to_json(__lookup_segment->view()) << std::endl;
#endif
        auto segment_view = __lookup_segment->view(); 
        global_view = segment_view;
        auto __file_name = segment_view["m_file_name"];
        auto __segment_len = segment_view["m_segment_len"];
        auto __segment_id = segment_view["m_segment_id"];
        std::string s{ __file_name.get_string().value};
        //auto test1 = segment_view["test"];

        //std::cout << "testing: " << test1.get_int64().value << "\n";
        //std::cout << "Looked up segment: " << _segment_id << "\n";
        //char *c = new char[__segment_len.get_int64().value];
        //memcpy(c, __segment.get_binary().bytes, __segment_len.get_int64().value);
        return std::unique_ptr<db_instance::segment_t>(new owning_segment<bsoncxx::document::value, uint8_t*>(std::move(*__lookup_segment),
                                                                                (uint32_t)__segment_len.get_int64().value,
                                                                                std::move(s),
                                                                                (uint32_t)__segment_id.get_int64().value));
    }
    
    std::cout << "Lookup failed for segment: " << _segment_id << " DB ID: " << m_id << "\n";
    return nullptr;
}

bool
db_instance::remove_segment(const char* _file_name, uint32_t _segment_id)
{
    auto client_ = m_client_helper(*m_client_pool, m_name);
    auto collection_ = client_.get_collection();
    auto __rem_doc = make_document(
            kvp("m_segment_id", bsoncxx::types::b_int64{_segment_id}),
            kvp("m_file_name", bsoncxx::types::b_string{_file_name})
            );

    auto __del_one_res = collection_.delete_one(__rem_doc.view());
#ifdef LOGS
    std::cout << "del segment: " << _segment_id << "\n";
#endif
    assert(__del_one_res->deleted_count() == 1);

    return true;
}
void
db_instance::del_all()
{
    auto client_ = m_client_helper(*m_client_pool, m_name);
    auto collection_ = client_.get_collection();
    collection_.delete_many({});
}
db_instance::~db_instance()
{
}
