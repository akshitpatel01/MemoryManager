#pragma once

#include "central_manager.h"
#include "hash_helper.h"
#include "rpc.h"
#include "segment.h"
#include "types.h"
#include "scope_profiler.h"
#include <cstddef>
#include <cstring>
#include <fstream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

/* Dependency: Hash Helper, RPC_helper
 */
class Blob_manager {
    using segment_t = segment<char>;
    struct file_meta_t {
        char* name;
        std::vector<pType::segment_ID> m_segs;

        file_meta_t(const char* _name)
            :name(new char[sizeof(_name)])
        {
            strncpy(name, _name, sizeof(*_name));
        }
        ~file_meta_t()
        {
            delete[] name;
        }
    };

    private:
        Hash_helper& m_hash_helper;
        RPC_helper& m_rpc_helper;
        Central_manager&  m_central_manager;
        std::unordered_map<const char*, file_meta_t> m_file_map;

    public:
        Blob_manager(Hash_helper& _hash_helper, RPC_helper& _rpc, Central_manager& _manager)
            :m_hash_helper(_hash_helper), m_rpc_helper(_rpc), m_central_manager(_manager)
        {}
    public:
       bool add(const char* _file_path)
       {
           std::ifstream _file(_file_path);
           size_t length, cur = 0;
           char* byte_array;
           m_file_map.emplace(std::piecewise_construct,
                                std::forward_as_tuple(_file_path),
                                std::forward_as_tuple(_file_path));
           auto it = m_file_map.find(_file_path);

           _file.seekg(0, std::ios::end);
           length = _file.tellg();
           std::cout << "File length: " << length << "\n";
           _file.seekg(0, std::ios::beg);

           PROFILE_SCOPE(); 
           while(cur < length) {
               size_t diff = (cur+SEGMENT_SIZE > length) ? length-cur : SEGMENT_SIZE;
               byte_array = new char[diff];
               _file.read(byte_array, diff);
               cur += diff;

               std::unique_ptr<segment_t> _seg = segment_t::create_segment(byte_array, diff, _file_path);
               pType::db_ID _dbID = m_hash_helper.get_db(_seg->get_hash());

               if (!m_rpc_helper.add(_seg, m_central_manager.get_db_snap(_dbID))) {
                   return false;
               }
               it->second.m_segs.push_back(_seg->get_id());
           } 

           return true;
       }

       bool remove(const char* _file_path)
       {
           std::string s(_file_path);
           auto it = m_file_map.find(_file_path);

           if (it == m_file_map.end()) {
               return false;
           }

           for (auto iter: it->second.m_segs) {
               auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
               pType::db_ID _dbID = m_hash_helper.get_db(seg_hash);
               std::cout << "Lookup db_id: " << _dbID << "\n";
               if (m_rpc_helper.del(s, iter, m_central_manager.get_db_snap(_dbID)) == false) {
                    std::cout << "del seg failed\n";
               }
           }
           
           return true;
       }

       std::vector<std::unique_ptr<segment<registration_apis::db_lookup_rsp>>> get(const char* _file_path)
       {
           std::vector<std::unique_ptr<segment<registration_apis::db_lookup_rsp>>> vec;
           auto it = m_file_map.find(_file_path);
           std::string s(_file_path);

           if (it == m_file_map.end()) {
               return std::vector<std::unique_ptr<segment<registration_apis::db_lookup_rsp>>>();
           }
           
           vec.reserve(it->second.m_segs.size());
           for (auto iter: it->second.m_segs) {
               auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
               pType::db_ID _dbID = m_hash_helper.get_db(seg_hash);
               std::cout << "Segment: " << iter << " Lookup db_id: " << _dbID << "\n";
               auto _seg = m_rpc_helper.lookup(s, iter, m_central_manager.get_db_snap(_dbID));
               if (_seg) {
                   vec.push_back(std::move(_seg));
               }
           }

           return vec;
       }
};
