#pragma once

#include "hash.h"
#include <cstdint>
#include <string>
typedef struct segment_t_{
    void* __data;
    uint32_t __data_len;
    const char* file_name; 
    uint32_t file_offset;
} segment_t;
#define BASE_FOLDER std::filesystem::currentpath()
class node_manager {
    private:
        Hash<Hash_linear, std::string, std::string> m_file_hash;
        static bool m_file_hash_lookup(void*, void*);
    public:
        node_manager();
        bool add_segment(std::string _filename, void* _segment, uint32_t _len);
        bool rem_segment();
        void lookup_segment();
        void sync_segment();
        std::string test();
        ~node_manager();
};
