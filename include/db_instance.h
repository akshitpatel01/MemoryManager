#pragma once

#include "mongocxx/client.hpp"
#include "mongocxx/collection.hpp"
#include "mongocxx/database.hpp"
#include "mongocxx/pool.hpp"
#include "segment.h"
#include <cstdint>
#include <string>

class db_instance {
    using segment_t = segment<uint8_t*>;
    private:
        std::string m_name;
        uint32_t m_id;
        uint32_t m_id_offset;
        mongocxx::pool* m_client_pool;
        struct m_client_helper {
            m_client_helper(mongocxx::pool& pool_, std::string& name_)
                :m_client(pool_.acquire()), m_db((*m_client)[name_]), m_coll(m_db["m_segments"])
            {}
            ~m_client_helper() = default;
        
            inline mongocxx::database& get_database()
            {
                return m_db;
            }

            inline mongocxx::collection& get_collection()
            {
                return m_coll;
            }
            private:
                mongocxx::pool::entry m_client;
                mongocxx::database m_db;
                mongocxx::collection m_coll;
        };

    private:
        friend class node_manager;
    private:
        db_instance(std::string&&, mongocxx::pool*);
        ~db_instance();

    
    public:
        const uint32_t get_id();
        void set_id(uint32_t);
        uint32_t get_id_offset();

    public:
        bool insert_segment(const char* file_name, uint32_t _segment_id, const void* _segment_data,
                            size_t _segment_size);
        std::unique_ptr<segment_t> lookup_segment(const char* _file_name, uint32_t _segment_id);
        bool remove_segment(const char* _file_name, uint32_t _segment_id);
        void del_all();

};
