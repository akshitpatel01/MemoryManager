#pragma once

#include "bsoncxx/types.hpp"
#include "hash.h"
#include "hash_linear.h"
#include "mongocxx/collection.hpp"
#include "mongocxx/database.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <sys/types.h>
#include <utility>

#define MONGO_DB "test200"
#define MONGO_COLL "global_coll"
#define BASE_FOLDER std::filesystem::currentpath()
#define MAX_DBS 10
class segment {
    private:
        void* m_data;
        uint32_t m_len;
        const char* m_file_name; 
        uint32_t m_segment_id;
    private:
        friend class db_instance;
    private:
        segment(void*, uint32_t, const char*, uint32_t);

    public:
        template<typename... T>
        static std::shared_ptr<segment> create_shared(T &&...args)
        {
            return std::shared_ptr<segment>(new segment(std::forward<T>(args)...));
        }
        uint32_t get_len()
        {
            return m_len;
        }
        uint32_t get_id()
        {
            return m_segment_id;
        }
};

class db_instance {
    private:
        std::string m_name;
        uint32_t m_id;
        uint32_t m_id_offset;
        mongocxx::database m_database;
        mongocxx::collection m_collection;

    private:
        friend class node_manager;
    private:
        db_instance(std::string&&, mongocxx::client&);
        ~db_instance();
    
    public:
        const uint32_t get_id();
        void set_id(uint32_t);
        uint32_t get_id_offset();

    public:
        bool insert_segment(const char* file_name, uint32_t _segment_id, void* _segment_data,
                            size_t _segment_size);
        std::shared_ptr<segment> lookup_segment(const char* _file_name, uint32_t _segment_id);
        bool remove_segment(const char* _file_name, uint32_t _segment_id);
        void del_all();

};

class node_manager {
    private:
        Hash<Hash_linear, db_instance, uint32_t> m_db_hash;
        mongocxx::instance m_instance;
        mongocxx::client m_mong_client;
        static bool m_db_hash_lookup(void*, void*);

    public:
        node_manager();
        bool add_db(std::string& db_name);
        bool insert(uint32_t _db_id, const char* file_name, uint32_t _segment_id,
                    void* _segment_data, size_t _segment_size);
        std::shared_ptr<segment> lookup(uint32_t _db_id, const char* _file_name,
                                        uint32_t _segment_id);
        bool remove(uint32_t _db_id, const char* _file_name,
                    uint32_t _segment_id);
        void sync_segment();
        std::string test();
        void del_all(uint32_t db_id);
        ~node_manager();
};
