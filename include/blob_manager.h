#pragma once

#include "central_manager.h"
#include "hash_helper.h"
#include "rpc.h"
#include "segment.h"
#include "types.h"
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
#include <condition_variable>

/* Dependency: Hash Helper, RPC_helper
 */
class Blob_manager {
    public:
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
        
        enum operation_t {
            ADD = 1,
            LOOKUP,
            DEL
        };

    
    public:
        Blob_manager(Hash_helper& _hash_helper, RPC_helper& _rpc, Central_manager& _manager);
    public:
        template<typename T>
        class File {
            public:
            struct iter {
                const void* data;
                uint32_t len;
            };

            enum status_t {
                in_progress = 1,
                success,
                failure
            };
            
            explicit File(Blob_manager& owner, std::string_view name, operation_t operation);
            explicit File(Blob_manager& owner, std::string_view name, uint32_t total_segments, operation_t operation);
            virtual ~File();

            protected:
                bool init (uint32_t total_segments);
                bool set_segment_without_fill(std::unique_ptr<segment<T>>&& segment, uint32_t idx);
                const segment<T>* get_segment(uint32_t idx);
                bool handle_failure();
            public:
            /* block till the entire operation is completed
             */
            bool wait();
            std::string_view name();
            bool next(iter& ptr);
            virtual bool handle(std::unique_ptr<segment<T>>&& segment, status_t status, uint32_t idx) = 0;
            virtual void done(void* mssg) = 0;

            protected:
                struct m_meta_internal {
                    m_meta_internal()
                        :segment_(nullptr), segment_filled(in_progress)
                    {}

                    bool set_segment(std::unique_ptr<segment<T>>&& segment)
                    {
                        if (!segment) {
                            return false;
                        }

                        segment_ = std::move(segment);
                        return true;
                    }
                    bool set_segment(std::unique_ptr<segment<T>>&& segment, status_t status_)
                    {
                        if (!segment || status_ != success) {
                            set_segment_fill(failure);
                            return false;
                        }
                        segment_ = std::move(segment);
                        set_segment_fill(success);

                        return true;
                    }
                    inline void set_segment_fill(status_t status)
                    {
                        {
                            std::unique_lock<std::mutex> lock_(m_lock);
                            segment_filled = status;
                        }
                        m_cv.notify_one();
                    }
                    inline void handle_failure()
                    {
                        set_segment_fill(failure);
                    }
                    status_t is_filled_now()
                    {
                        return segment_filled;
                    }
                    bool is_filled_wait()
                    {
                        std::unique_lock<std::mutex> lock_(m_lock);
                        if (segment_filled != in_progress) {
                            return (segment_filled == success) ? true : false;
                        }
                        m_cv.wait(lock_, [this]
                                {
                                    return (segment_filled != in_progress);
                                });

                        return (segment_filled == success) ? true : false;
                    }
                    const void* data()
                    {
                        return segment_->get_data();
                    }
                    uint32_t len()
                    {
                        return segment_->get_len();
                    }
                    const segment<T>* full_segment()
                    {
                        return segment_.get();
                    }

                    private:
                        std::unique_ptr<segment<T>> segment_; 
                        status_t segment_filled;
                        std::mutex m_lock;
                        std::condition_variable m_cv;
                };

            protected:
                Blob_manager& m_owner;
                uint32_t m_total_segments{};
                std::string m_name;
                std::vector<m_meta_internal> m_segments;
                std::condition_variable m_cv;
                std::mutex m_condition_mutex;
                status_t m_op_result;
                operation_t m_operation;
                uint32_t m_iter;
                uint32_t m_filled_segments{};
        };

        class File_add: public File<char> {
            using segment_t = segment<char>;
            public:
                File_add(Blob_manager& owner, std::string_view file_path, operation_t operation);
                ~File_add();
                bool handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx) override;
                void done(void* mssg) override;
            private:
                void send_chunks();
            private:
                std::ifstream m_file;
                std::unordered_map<std::string, Blob_manager::file_meta_t>::iterator it;
                size_t m_length = 0;
                ID_helper<uint32_t> m_seg_id_helper{50000, 0};
                /* For multiple Servers
                 */
                std::unordered_map<uint32_t, RPC_helper::RPC_async_stream_handler<char>*> m_rpc_map{};
        };
        class File_lookup: public File<registration_apis::db_rsp> {
            using segment_t = segment<registration_apis::db_rsp>;
            public:
                File_lookup(Blob_manager&, std::string_view file_path, operation_t operation);
                ~File_lookup();
                bool handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx) override;
                void done(void* mssg) override;
            private:
                void lookup_chunks();
        };
        
        class File_del: public File<char> {
            using segment_t = segment<char>;
            public:
                File_del(Blob_manager&, std::string_view file_path, operation_t operation);
                ~File_del();
                bool handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx) override;
                void done(void* mssg) override;
            private:
                void del_chunks();
        };

    public:
        std::unique_ptr<File<char>> async_add(std::string_view _file_path);
        std::unique_ptr<File<char>> async_stream_add(std::string_view _file_path);
        std::unique_ptr<File<char>> async_remove(std::string_view file_path);
        

        /* Return pointer as well as file object should work. Right now returning
         * ptr as moving will be as efficient as possible
         */
        std::unique_ptr<File<registration_apis::db_rsp>> async_get(std::string_view file_path);
    public:
        /* Helper */
        inline auto insert_file(std::string_view file_path)
        {
            m_file_map.emplace(std::piecewise_construct,
                               std::forward_as_tuple(file_path.data()),
                               std::forward_as_tuple(file_path.data()));
            return m_file_map.find(file_path.data());
        }
        inline bool remove_file(std::string_view file_path)
        {
            m_file_map.erase(file_path.data());
            return true;
        }
        inline uint32_t get_total_segments(std::string_view file_path)
        {
            auto it = m_file_map.find(file_path.data());
            if (it == m_file_map.end()) {
                return 0;
            }

            return it->second.m_segs.size();
        }
        inline bool insert_seg(std::string_view file_path, pType::segment_ID id)
        {
            auto it = m_file_map.find(file_path.data());
            if (it == m_file_map.end()) {
                return false;
            }

            it->second.m_segs.push_back(id);
            return true;
        }

        inline pType::db_ID get_db(std::size_t hash)
        {
            return m_hash_helper.get_db(hash);
        }
        inline auto get_db_snapshot(pType::db_ID db)
        {
            return m_central_manager.get_db_snap(db);
        }
    
        template<typename T>
        auto create_handle(Blob_manager& owner, std::string_view file_path, operation_t operation)
        {
            switch(operation) {
                case ADD:
                    return std::make_unique<File_add>(owner, file_path, operation);
                //case LOOKUP:
                    //return std::make_unique<File_lookup>(owner, file_path, operation);
                default:
                    return nullptr;
            }
        }
    private:
        Hash_helper& m_hash_helper;
        RPC_helper& m_rpc_helper;
        Central_manager&  m_central_manager;
        std::unordered_map<std::string, file_meta_t> m_file_map{};
};

