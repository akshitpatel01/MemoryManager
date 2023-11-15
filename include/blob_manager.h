#pragma once

#include "list.h"
#include "hash.h"
#include "segment.h"
#include <cstddef>
#include <cstdint>
#include <random>
#include <set>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>
#define SEGMENT_SIZE 1 /* bytes */
#define REPLICATION_COUNTER 1

template<typename K, typename V>
struct std::hash<std::pair<K, V>> {
    std::size_t operator() (const std::pair<K, V>& _param)  const
    {
        uint32_t __hash1 = std::hash<K>{}(_param.first);
        uint32_t __hash2 = std::hash<V>{}(_param.second);
        __hash1 ^= __hash2 + std::rand() + (__hash1 << 6) + (__hash2 >> 2);

        return __hash1;
    }
};
struct set_cmp {
    bool operator() (const std::pair<uint32_t, uint32_t>& a, const std::pair<uint32_t, uint32_t>& b) const
    {
        return a.first < b.first;
    }
};
class Blob_manager {
    private:
        typedef struct db_meta_data_ {
            uint32_t m_db_id;
            uint32_t m_node_id;
            uint32_t m_active_req;
            db_meta_data_(uint32_t _id, uint32_t _node_id)
                : m_db_id(_id), m_node_id(_node_id), m_active_req(0)
            {
            }
            uint32_t get_id_offset()
            {
                return offsetof(db_meta_data_, m_db_id);
            }
        } __db_meta_data_t;

        typedef struct file_meta_data_ {
            const char* m_file_name;
            List m_seg_meta;
            file_meta_data_()
                :m_file_name(nullptr), m_seg_meta([](void* _a, void*_b) -> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
                }){}
            file_meta_data_(const char* _path)
                :m_file_name(_path), m_seg_meta([](void* _a, void*_b) -> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
                })
            {
            }
            uint32_t get_id_offset()
            {
                //return offsetof(struct file_meta_data_, m_file_name);
                return 0;
            }
        } __file_meta_data_t;

        typedef struct  seg_meta_ {
            uint32_t m_seg_id;
            std::vector<uint32_t> m_db_id;
            seg_meta_ (uint32_t _id, std::vector<uint32_t>& db_list):
                m_seg_id(_id), m_db_id(db_list)
            {
            }
        }__seg_meta_t;

        typedef struct node_meta_ {
            uint32_t m_id;
            List m_dbs;
            node_meta_(uint32_t _id)
                : m_id(_id), m_dbs([](void* _a, void*_b) -> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
            })
            {}
            uint32_t get_id_offset()
            {
                //return offsetof(struct node_meta_, m_id);
                return 0;
            }
        } __node_meta_t;

        template <std::size_t N, typename T>
        std::array<std::size_t, N> hashNT(const T& key)
        {
            std::minstd_rand0 rng(std::hash<T>{}(key));
            std::array<std::size_t, N> hashes{};
            std::generate(std::begin(hashes), std::end(hashes), rng);
            return hashes;
        }
            
    private:
        uint32_t m_global_node_id;
        uint32_t m_global_db_id;
        List m_free_list_node;
        List m_free_list_db;

        Hash<Hash_linear, __db_meta_data_t, uint32_t> m_db_id_hash; 
        Hash<Hash_linear, __node_meta_t, uint32_t> m_node_id_hash; 
        //Hash<Hash_linear, __file_meta_data_t, char> m_file_name_hash; 
        std::unordered_map<const char*, __file_meta_data_t> m_file_name_hash;
        std::set<std::pair<uint32_t, uint32_t>, set_cmp> m_consistent_hash;
        List m_nodes;
        enum {
            DB_ID = 1,
            NODE_ID
        };
    private:
        /* internal */
        uint32_t __get_new_id(uint8_t);
        void __free_id(uint32_t, uint8_t);

        void* __split_file();
        uint32_t __get_db_id();
        uint32_t __get_node();

        template<typename T>
        bool __populate_db_list(std::set<T>& _s, typename std::set<T>::iterator& _iter, std::vector<uint32_t>& _vec);
    public:
        Blob_manager();

        /* Registration RPC calls */
        uint32_t register_node();
        uint32_t add_node_db(uint32_t _node_id);

        /* CRUD APIs */
        bool add(const char* _file_name, void*);
        bool del(const char* _file_name);
        void* lookup(const char* _file_name);
};
