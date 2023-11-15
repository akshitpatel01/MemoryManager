#include "blob_manager.h"
#include "segment.h"
#include <algorithm>
#include <bits/types/FILE.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

Blob_manager::Blob_manager()
    : m_global_node_id(1), m_global_db_id(1), m_free_list_node([](void* _a, void* _b)-> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
            }),
            m_free_list_db([](void* _a, void* _b)-> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
            }), m_db_id_hash([](void* _a, void*_b) -> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
            }), m_node_id_hash([](void* _a, void*_b) -> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
            }), m_nodes([](void* _a, void*_b) -> bool {
                uint32_t* __id1 = static_cast<uint32_t*>(_a);
                uint32_t* __id2 = static_cast<uint32_t*>(_b);

                return *__id1 == *__id2;
            }){
}

uint32_t
Blob_manager::__get_new_id(uint8_t _type)
{
    uint32_t *__new_id;
    uint32_t __ret;
    List* __list = nullptr;
    uint32_t* __global_id = nullptr;
    
    switch (_type) {
        case DB_ID:
            __list = &m_free_list_db;
            __global_id = &m_global_db_id;
            break;
        case NODE_ID:
            __list = &m_free_list_node;
            __global_id = &m_global_node_id;
            break;
        default:
            return 0;
    }

    if ((__new_id = static_cast<uint32_t*>((*__list).get_head())) != nullptr) {
        __ret = *__new_id;
        (*__list).remove_head();
        delete __new_id;
        return __ret;
    }

    __ret = (*__global_id)++;
    return __ret;
}

void
Blob_manager::__free_id(uint32_t _id, uint8_t _type)
{
    uint32_t *__id = new uint32_t;
    List* __list = nullptr;
    
    switch (_type) {
        case DB_ID:
            __list = &m_free_list_db;
            break;
        case NODE_ID:
            __list = &m_free_list_node;
            break;
        default:
            return;
    }

    *__id = _id;
    (*__list).insert_tail(__id);
}

bool
Blob_manager::add(const char* _file_path, void* _file)
{
    auto fileSize = std::filesystem::file_size(std::string(_file_path));
    std::unique_ptr buf = std::make_unique<std::byte[]>(fileSize);
    std::basic_ifstream<std::byte> ifs(_file_path, std::ios::binary);
    ifs.read(buf.get(), fileSize);
    uint32_t __db_id;
    uint32_t __db_hash;
    //uint32_t __node_id;
    __db_meta_data_t* __db_meta = nullptr;
    //__file_meta_data_t* __file_meta = new __file_meta_data_t(_file_path);
    // duplicates allowed
    __file_meta_data_t& __file_meta = m_file_name_hash[_file_path];
    std::vector<uint32_t> _db_list;

    for (uint i = 0; i < fileSize-SEGMENT_SIZE; i+=SEGMENT_SIZE) {
        std::shared_ptr<segment> __seg = segment::create_shared((void*)buf[i], SEGMENT_SIZE*sizeof(buf[i]), _file_path, i);

        // get_db
        __db_hash = hashNT<1>(std::pair<std::string,uint32_t> {_file_path, i})[0];
        auto __iter = m_consistent_hash.lower_bound({__db_hash, std::numeric_limits<uint32_t>::min()});
        if (__iter == m_consistent_hash.end()) {
            __iter = m_consistent_hash.begin();
        }
        __db_id = __iter->second;
        //__populate_db_list<std::pair<uint32_t, uint32_t>, set_cmp>(m_consistent_hash, __iter, _db_list);
        _db_list.push_back(__db_id);

        // get node
        __db_meta = m_db_id_hash.lookup(&__db_id);
        if (__db_meta) {
            //__node_id = te->m_node_id;
            std::cout << __db_meta->m_node_id << " succ\n";
        }
        // send_mssg to add __seg to node

        __file_meta.m_seg_meta.insert_tail(new __seg_meta_t(i, _db_list));
    } 

    //m_file_name_hash.insert((char*)((std::byte*)__file_meta + __file_meta->get_id_offset()), __file_meta);
    return true;
}
bool
Blob_manager::del(const char* _file_path)
{
    auto __file_meta = m_file_name_hash.find(_file_path);
    if (__file_meta == m_file_name_hash.end()) {
        return false;
    } else {
        List& __list = __file_meta->second.m_seg_meta;
        for (auto it = __list.begin(); it != __list.end(); it++) {
            //send_mssg to del __seg to node
        }
        
        m_file_name_hash.erase(_file_path);
        return true;
    }
}
void*
Blob_manager::lookup(const char* _file_path)
{
    auto __file_meta = m_file_name_hash.find(_file_path);
    if (__file_meta == m_file_name_hash.end()) {
        return nullptr;
    } else {
        List& __list = __file_meta->second.m_seg_meta;
        for (auto it = __list.begin(); it != __list.end(); it++) {
            //send_mssg to del __seg to node
            std::cout << "Lookup: " << static_cast<__seg_meta_t*>(it.get_val())->m_db_id[0] << " " << static_cast<__seg_meta_t*>(it.get_val())->m_seg_id << "\n";
        }
        
        return nullptr;
    }
}

uint32_t
Blob_manager::register_node()
{
    uint32_t __node_id = __get_new_id(NODE_ID);
    __node_meta_t *__new_node = nullptr;

    __new_node = new __node_meta_t(__node_id);
    if (__new_node == nullptr) {
        __free_id(__node_id, NODE_ID);
        return 0;
    }

    m_nodes.insert_head(__new_node);
    m_node_id_hash.insert((uint32_t*)((std::byte*)__new_node + __new_node->get_id_offset()), __new_node);
    return __node_id;
}

uint32_t
Blob_manager::add_node_db(uint32_t _node_id)
{
    uint32_t __db_id = __get_new_id(DB_ID);
    __node_meta_t* __node_meta = nullptr;
    __db_meta_data_t* __new_db = nullptr;

    __node_meta = m_node_id_hash.lookup(&_node_id);
    if (__node_meta == nullptr) {
        __free_id(__db_id, DB_ID);
        return false;
    }

    __new_db = new __db_meta_data_t(__db_id, _node_id);
    m_db_id_hash.insert((uint32_t*)((std::byte*)__new_db + __new_db->get_id_offset()), __new_db);

    auto __hash_arr = hashNT<3>(__db_id);
    for (auto& hash: __hash_arr) {
        std::cout << "DB: " << __db_id << " hash: " << hash << " \n";
        m_consistent_hash.insert({hash, __db_id});
    }

    return __db_id;
}

template<typename T> bool
Blob_manager::__populate_db_list(std::set<T>& _s, typename std::set<T>::iterator& _iter, std::vector<uint32_t>& _vec)
{
    int k = REPLICATION_COUNTER;
    auto __iter = _iter;

    while(k-- && __iter != _s.end()) {
        _vec.push_back(__iter->second);
        __iter++;
    }

    __iter = _s.begin();
    while(k-- && __iter != _iter) {
        _vec.push_back(__iter->second);
        __iter++;
    }

    if (k > 0) {
        return false;
    }

    return true;
}
