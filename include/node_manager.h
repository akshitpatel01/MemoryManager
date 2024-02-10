#pragma once

#include "hash.h"
#include "mongocxx/options/auto_encryption.hpp"
#include "segment.h"
#include "db_instance.h"
#include <cstdint>
#include <memory>
#include <string>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/pool.hpp>
#include <sys/types.h>

#define MONGO_DB "test200"
#define MONGO_COLL "global_coll"
#define BASE_FOLDER std::filesystem::currentpath()
#define MAX_DBS 10

class node_manager {
    using segment_t = segment<uint8_t*>;
    public:
        node_manager();
        bool add_db(std::string& db_name);
        bool insert(const std::unique_ptr<segment_t>& _segment, uint32_t _db_id);
        std::unique_ptr<node_manager::segment_t> lookup(const char* _file_name, uint32_t _segment_id, uint32_t _db_id);
        bool remove(const char* _file_name, uint32_t _segment_id, uint32_t _db_id);
        void sync_segment();
        std::string test();
        void del_all(uint32_t db_id);
        ~node_manager();
    
    private:
        Hash<Hash_linear, db_instance, uint32_t> m_db_hash;
        List m_db_list;
        mongocxx::instance m_instance;
        mongocxx::pool m_client_pool;
        static bool m_db_hash_lookup(void*, void*);

};
