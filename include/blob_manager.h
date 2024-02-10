#pragma once

#include "central_manager.h"
#include "hash_helper.h"
#include "rpc.h"
#include "segment.h"
#include "types.h"
#include "scope_profiler.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <grpcpp/support/status.h>
#include <memory>
#include <string_view>
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
            :name(new char[strlen(_name)])
        {
            //strncpy(name, _name, strlen(_name));
            memcpy(name, _name, strlen(_name));
        }
        ~file_meta_t()
        {
            delete[] name;
        }
    };

    public:
        Blob_manager(Hash_helper& _hash_helper, RPC_helper& _rpc, Central_manager& _manager)
            :m_hash_helper(_hash_helper), m_rpc_helper(_rpc), m_central_manager(_manager)
        {}
    public:
        std::unique_ptr<File> async_add(std::string_view _file_path)
       {
           PROFILE_SCOPE();
           std::ifstream _file(_file_path);
           size_t length, cur = 0;
           char* byte_array;
           /* Does map create a copy on insert?
            * If not, this will lead to a crash
            */
           m_file_map.emplace(std::piecewise_construct,
                                std::forward_as_tuple(_file_path.data()),
                                std::forward_as_tuple(_file_path.data()));
           auto it = m_file_map.find(_file_path.data());
           uint idx_ = 0;

           _file.seekg(0, std::ios::end);
           length = _file.tellg();
#ifdef LOGS
           std::cout << "File length: " << length << "\n";
           std::cout << "total segment: " << std::ceil((float)length/SEGMENT_SIZE) << "\n";
#endif
           _file.seekg(0, std::ios::beg);

           std::unique_ptr<File> file_p = std::make_unique<File>(_file_path, std::ceil((float)length/SEGMENT_SIZE), File::ADD);
           while(cur < length) {
               size_t diff = (cur+SEGMENT_SIZE > length) ? length-cur : SEGMENT_SIZE;
               byte_array = new char[diff];
               _file.read(byte_array, diff);
               cur += diff;

               std::unique_ptr<segment_t> _seg = segment_t::create_segment(byte_array, diff, _file_path);
               pType::db_ID _dbID = m_hash_helper.get_db(_seg->get_hash());

               it->second.m_segs.push_back(_seg->get_id());
               if (!m_rpc_helper.add(std::move(_seg), m_central_manager.get_db_snap(_dbID),
                                        [ptr = file_p.get(), idx_] (grpc::Status status_, registration_apis::db_rsp& reply_)
                                        {
                                            if (!status_.ok()) {
                                                ptr->handle_operation(nullptr, File::failure, idx_);
                                                std::cout << status_.error_message() << "\n";
                                            } else {
                                                ptr->handle_operation(nullptr, File::success, idx_);
                                                std::cout << "got reply yes!!!!\n";
                                            }
#ifdef LOGS
#endif
                                        })) {
                   return nullptr;
               }
               idx_++;
           } 

           return file_p;
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
#ifdef LOGS
               std::cout << "Lookup db_id: " << _dbID << "\n";
#endif
               if (m_rpc_helper.del(s, iter, m_central_manager.get_db_snap(_dbID), [](grpc::Status status_, registration_apis::db_rsp& reply_){}) == false) {
                    std::cout << "del seg failed\n";
               }
           }
           
           return true;
       }
       std::unique_ptr<File> async_remove(std::string_view file_path)
       {
           auto it = m_file_map.find(file_path.data());
           uint32_t idx_ = 0;

           if (it == m_file_map.end()) {
               return nullptr;
           }

           std::unique_ptr<File> file_p = std::make_unique<File>(file_path, it->second.m_segs.size(), File::DEL);
           for (auto iter: it->second.m_segs) {
               auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
               pType::db_ID _dbID = m_hash_helper.get_db(seg_hash);
#ifdef LOGS
               std::cout << "Lookup db_id: " << _dbID << "\n";
#endif
               if (m_rpc_helper.del(file_path, iter, m_central_manager.get_db_snap(_dbID),
                           [ptr = file_p.get(), idx_, this] (grpc::Status status_, registration_apis::db_rsp& reply_)
                                        {
                                            if (!status_.ok()) {
                                                ptr->handle_operation(nullptr, File::failure, idx_);
                                                std::cout << status_.error_message() << "\n";
                                            } else {
                                                m_file_map.erase(ptr->name().data());
                                                ptr->handle_operation(nullptr, File::success, idx_);
                                                std::cout << "got reply yes!!!!\n";
                                            }
#ifdef LOGS
#endif
                                        }) == false) {
                    std::cout << "del seg failed\n";
               }
               idx_++;
           }
           
           return file_p;

       }

       std::vector<std::unique_ptr<segment<registration_apis::db_rsp>>> get(const char* _file_path)
       {
           PROFILE_SCOPE();
           std::vector<std::unique_ptr<segment<registration_apis::db_rsp>>> vec;
           auto it = m_file_map.find(_file_path);
           std::string s(_file_path);

           if (it == m_file_map.end()) {
               return std::vector<std::unique_ptr<segment<registration_apis::db_rsp>>>();
           }

           vec.reserve(it->second.m_segs.size());
           for (auto iter: it->second.m_segs) {
               auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
               pType::db_ID _dbID = m_hash_helper.get_db(seg_hash);
#ifdef LOGS
               std::cout << "Segment: " << iter << " Lookup db_id: " << _dbID << "\n";
#endif
#if 0
               auto _seg = m_rpc_helper.lookup(s, iter, m_central_manager.get_db_snap(_dbID), [](grpc::Status status_, registration_apis::db_rsp* _rsp){});
               if (_seg) {
                   vec.push_back(std::move(_seg));
               }
#endif
           }

           return vec;
       }

       /* Return pointer as well as file object should work. Right now returning
        * ptr as moving will be as efficient as possible
        */
       std::unique_ptr<File> async_get(std::string_view file_path)
       {
           PROFILE_SCOPE();
           uint32_t idx_ = 0;
           auto it = m_file_map.find(file_path.data());

           if (it == m_file_map.end()) {
               return nullptr;
           }

           std::unique_ptr<File> file = std::make_unique<File>(file_path, it->second.m_segs.size(), File::LOOKUP);

           for (auto iter: it->second.m_segs) {
               auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
               pType::db_ID _dbID = m_hash_helper.get_db(seg_hash);


               /* Passing file by ref should be fine. copy elision will help here.
                */
               if (!m_rpc_helper.lookup(file_path, iter, m_central_manager.get_db_snap(_dbID),
                           [ptr = file.get(), idx_] (grpc::Status status_, registration_apis::db_rsp* _rsp) -> void
                           {
                           if (status_.ok()) {
                               ptr->handle_operation(segment<registration_apis::db_rsp>::create_segment(_rsp,
                                           _rsp->data().length(),
                                           ptr->name()),
                                       File::success, idx_);
                               std::cout << "Invoked handle lookup\n";
                           } else {
                               ptr->handle_operation(segment<registration_apis::db_rsp>::create_segment(_rsp,
                                           _rsp->data().length(),
                                           ptr->name()),
                                       File::failure, idx_);
                               std::cout << "Invoked handle lookup\n";
                           }
                           })) {
                   //std::cout << "Lookup internal error\n";
                   //return false;
               }
               idx_++;
           }

           return file;
       }

    public:
       struct file_comparator {
           bool operator() (std::string_view a, std::string_view b)
           {
               return (a.compare(b) == 0);
           }

       };
    
    private:
        Hash_helper& m_hash_helper;
        RPC_helper& m_rpc_helper;
        Central_manager&  m_central_manager;
        std::unordered_map<std::string, file_meta_t> m_file_map{};
};
