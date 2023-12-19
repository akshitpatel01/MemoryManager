#pragma once

#include "bsoncxx/builder/basic/document.hpp"
#include "bsoncxx/builder/basic/kvp.hpp"
#include "bsoncxx/json.hpp"
#include "bsoncxx/types.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/options/update.hpp"
#include "segment.h"
#include <cstdint>
#include <string>

class db_instance {
    using segment_t = segment<char>;
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
        bool insert_segment(const char* file_name, uint32_t _segment_id, const void* _segment_data,
                            size_t _segment_size);
        std::unique_ptr<segment_t> lookup_segment(const char* _file_name, uint32_t _segment_id);
        bool remove_segment(const char* _file_name, uint32_t _segment_id);
        void del_all();

};
