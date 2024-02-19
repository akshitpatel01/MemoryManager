#include "blob_manager.h"
#include "rpc.h"
#include "segment.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

/* Blob Manager APIs
 */
Blob_manager::Blob_manager(Hash_helper& _hash_helper, RPC_helper& _rpc, Central_manager& _manager)
                            :m_hash_helper(_hash_helper), m_rpc_helper(_rpc), m_central_manager(_manager)
{}
            
std::unique_ptr<Blob_manager::File<char>> Blob_manager::async_stream_add(std::string_view _file_path)
{
    std::unique_ptr<File<char>> file_p = nullptr;
    file_p = std::make_unique<File_add>(*this, _file_path, ADD);

    return file_p;
}

std::unique_ptr<Blob_manager::File<registration_apis::db_rsp>>
Blob_manager::async_get(std::string_view file_path)
{
    std::unique_ptr<File<registration_apis::db_rsp>> file_p = nullptr;
    if (get_total_segments(file_path) > 0) {
        file_p = std::make_unique<File_lookup>(*this, file_path, LOOKUP);
    }

    return file_p;
}
std::unique_ptr<Blob_manager::File<char>> Blob_manager::async_remove(std::string_view file_path)
{
    std::unique_ptr<File<char>> file_p = nullptr;
    file_p = std::make_unique<File_del>(*this, file_path, DEL);

    return file_p;
}

/* File base class APIs
 */
template<typename T>
Blob_manager::File<T>::File(Blob_manager& owner, std::string_view name, Blob_manager::operation_t operation)
    :m_owner(owner), m_name{name}, m_segments{}, m_op_result(in_progress),
     m_operation(operation), m_iter(0)
     {}

template<typename T>
Blob_manager::File<T>::File(Blob_manager& owner, std::string_view name, uint32_t total_segments, operation_t operation)
    :m_owner(owner), m_total_segments(total_segments), m_name{name}, m_segments(total_segments),
     m_op_result(in_progress), m_operation(operation), m_iter(0)
{
    std::cout << "File constructed with name: " << m_name << " segments: " << m_segments.size() << " operation: " << m_operation << "\n"; 
}

template<typename T>
Blob_manager::File<T>::~File()
{
    wait();
}

template<typename T>
bool Blob_manager::File<T>::init (uint32_t total_segments)
{
    m_total_segments = total_segments;
    m_segments = std::vector<m_meta_internal>(total_segments);
    return true;
}

template<typename T>
bool Blob_manager::File<T>::wait()
{
#ifdef LOGS
    std::cout << "Waiting for operation to complete\n";
#endif
    std::unique_lock<std::mutex> lock_(m_condition_mutex);
    if (m_op_result != in_progress) {
        return (m_op_result == failure) ? false : true;
    }
    m_cv.wait(lock_, [this]{
            return (m_op_result != in_progress);
            });

#ifdef LOGS
    std::cout << "OPertion completed\n";
#endif
    return (m_op_result == failure) ? false : true;
}

template<typename T>
std::string_view Blob_manager::File<T>::name()
{
    return m_name;
}

template<typename T>
typename Blob_manager::File<T>::status_t Blob_manager::File<T>::next_segment(iter& ptr)
{
    bool ret_;
    if (m_operation != LOOKUP) {
        return failure;
    }

    if (m_iter >= m_segments.size()) {
        return success;
    }


    ret_ = m_segments[m_iter].is_filled_wait();
    if (ret_) {
        ptr = {m_segments[m_iter].data(), m_segments[m_iter++].len()};
    }

    return (ret_ == false) ? failure : in_progress;
}

template<typename T>
bool Blob_manager::File<T>::set_segment_without_fill(std::unique_ptr<segment<T>>&& segment,
                                                     uint32_t idx)
{
    return m_segments[idx].set_segment(std::move(segment));
}

template<typename T>
const segment<T>* Blob_manager::File<T>::get_segment(uint32_t idx)
{
    return m_segments[idx].full_segment();
}

template<typename T>
bool Blob_manager::File<T>::handle_failure(status_t status)
{
    std::cout << __FUNCTION__ << "\n";
    {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        m_filled_segments++;
        if (m_op_result == status) {
            return true;
        }
        m_op_result = status;
    }

    /* Wait till all ongoing requests complete. If we do not wait at this point
     * the wait() will pass after notify_one is triggered and the destructor will
     * successfully execute. This can (and will) result in crashes if the callback is 
     * invoked after the object has been deleted.
     */
    while(m_filled_segments < m_sent_segments){};
    for (auto& segment: m_segments) {
        segment.handle_failure();
    }

    /* This should be the last instruction to be executed in this function
     * as the object may be destructed right after the call to this
     */
    m_cv.notify_one();
    return true;
}


/* File insert operation APIs
 */
Blob_manager::File_add::File_add(Blob_manager& owner, std::string_view file_path, operation_t operation)
    :File<char>(owner, file_path, operation), m_file(file_path.data()), it(owner.insert_file(file_path))
{
    m_file.seekg(0, std::ios::end);
    m_length = m_file.tellg();
    uint32_t total_segments_ = std::ceil((float)m_length/SEGMENT_SIZE);
    m_file.seekg(0, std::ios::beg);
    init(total_segments_);

    /* Start send */
    send_chunks();
}
Blob_manager::File_add::~File_add()
{
    /* Extra safety purposes
     */
    wait();
    /* In case of any failure, we cannot call StartWritesDone after the failure 
     * has happened. Anyhow, as the File structure maintains the active requests
     * and waits till all the active requests are completed it should be safe
     * at this point to just delete the RPC handlers at this point.
     */
    /* Here as per current behaviour, calling RemoveHold() calls OnDone() which
     * inturn deletes the rpc_handler class. Assuming this behaviour, I can 
     * safely call done() and it is assumed to take care of freeing the class.
     */
    bool status = (m_op_result == rpc_failure) ? false : true;
    for (auto iter: m_rpc_map) {
        iter.second->done(status);
    }
}

bool Blob_manager::File_add::handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx)
{
    if (idx >= m_total_segments) {
        return false;
    }

    m_segments[idx].set_segment_fill(status);

    if (status == success) {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        m_filled_segments++;
        if (m_filled_segments == m_segments.size()) {
            /* unblock wait; */
            m_op_result = success;
            m_cv.notify_one();
        }
    } else {
        /* Fixme: Add retry 
         */
        handle_failure(status);
    }

    return (status == success);
}

/* Only called by one thread at a time
 */
void Blob_manager::File_add::send_chunks()
{
    size_t diff_ = 0, cur_ = 0;
    std::unique_ptr<segment_t> seg_ = nullptr;
    RPC_helper::RPC_async_stream_handler<char>* rpc_ = nullptr;
    uint32_t cur_seg_id_ = m_seg_id_helper.get_id();

    if (m_op_result != failure && cur_seg_id_ < m_total_segments) {
        cur_ = cur_seg_id_ * SEGMENT_SIZE;
        diff_ = (cur_+SEGMENT_SIZE > m_length) ? m_length-cur_ : SEGMENT_SIZE;
        char* byte_array_ = new char[diff_];
        m_file.read(byte_array_, diff_);
        cur_ += diff_;

        seg_ = segment_t::create_segment(byte_array_, diff_, m_name, cur_seg_id_);
        auto db_meta = m_owner.get_db_snapshot(m_owner.get_db(seg_->get_hash()));
        auto iter = m_rpc_map.find(db_meta.m_node.m_ip_addr);
        if (iter == m_rpc_map.end()) {
            rpc_ = m_owner.m_rpc_helper.stream_add([this](bool ok) -> void {
                        if (ok) {
                            send_chunks();
                        } else {
                            handle_failure(rpc_failure);
                        }
                    }, [this](bool ok, bool rsp_ok, uint32_t idx) -> void {
                        if (ok) {
                            if (rsp_ok) {
                                handle(nullptr, success, idx);
                            } else {
                                handle(nullptr, failure, idx);
                            }
                        } else {
                            handle(nullptr, rpc_failure, idx);
                        }
                    },
                    db_meta.m_node.m_ip_addr);
            m_rpc_map.insert({db_meta.m_node.m_ip_addr, rpc_});
        } else {
            rpc_ = iter->second;
        }

        auto id_ = seg_->get_id();
        m_owner.insert_seg(m_name, id_);
        set_segment_without_fill(std::move(seg_), id_);
        if (rpc_->write(get_segment(id_), db_meta.m_db.m_id, id_)) {
            m_sent_segments++;
        }
    }
}

void Blob_manager::File_add::done(void* mssg) 
{
    std::cout << __FUNCTION__ << " Done\n";
}

/* File lookup operation APIs
 */
Blob_manager::File_lookup::File_lookup(Blob_manager& owner, std::string_view file_path, operation_t operation)
    :File<registration_apis::db_rsp>(owner, file_path, owner.get_total_segments(file_path), operation)
{
    lookup_chunks();
}

Blob_manager::File_lookup::~File_lookup()
{
}

bool
Blob_manager::File_lookup::handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx)
{
    bool ret_ = false;

    ret_ = m_segments[idx].set_segment(std::move(segment), status);
    
    if ((status == success) && ret_) {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        m_filled_segments++;
        if (m_filled_segments == m_segments.size()) {
            /* unblock wait; */
            m_op_result = success;
            m_cv.notify_one();
        }
    } else {
        handle_failure(status);
    }

    return (status == success);
}

void
Blob_manager::File_lookup::lookup_chunks()
{
    auto file_meta_ = m_owner.m_file_map.find(m_name);
    uint32_t idx = 0;
    if (file_meta_ == m_owner.m_file_map.end()) {
        return;
    }

    for (auto iter: file_meta_->second.m_segs) {
        if (m_op_result == failure) {
            /* No need to call handle_failure as it will be called as part of
             * handle
             */
            break;
        }
        auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
        pType::db_ID _dbID = m_owner.m_hash_helper.get_db(seg_hash);
        m_owner.m_rpc_helper.lookup(m_name.data(), iter, m_owner.get_db_snapshot(_dbID), [this, idx](grpc::Status status, registration_apis::db_rsp* rsp){
                    if (!status.ok()) {
                        handle(nullptr, rpc_failure, idx);
                    } else if (rsp->rsp()) {
                        handle(segment_t::create_segment(rsp, rsp->data().length(), name()), success, idx);
                    } else {
                        handle(nullptr, failure, idx);
                    }
                });
        m_sent_segments++;
        idx++;
    }
}
void Blob_manager::File_lookup::done(void* mssg) 
{
    std::cout << __FUNCTION__ << " Done\n";
}

/* File delete operation APIs
 */
Blob_manager::File_del::File_del(Blob_manager& owner, std::string_view file_path, operation_t operation)
    :File<char>(owner, file_path, owner.get_total_segments(file_path), operation)
{
    del_chunks();
}

Blob_manager::File_del::~File_del()
{
}

bool Blob_manager::File_del::handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx)
{
    if (idx >= m_total_segments) {
        return false;
    }

    m_segments[idx].set_segment_fill(status);

    if (status == success) {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        m_filled_segments++;
        if (m_filled_segments == m_segments.size()) {
            /* unblock wait; */
            m_op_result = success;
            m_cv.notify_one();
        }
    } else {
        handle_failure(status);
    }
    
    return (status == success);
}

void
Blob_manager::File_del::del_chunks()
{
    auto file_meta_ = m_owner.m_file_map.find(m_name);
    uint32_t idx = 0;
    if (file_meta_ == m_owner.m_file_map.end()) {
        return;
    }

    for (auto iter: file_meta_->second.m_segs) {
        /* Loose check should be fine here
         */
        if (m_op_result == failure) {
            /* No need to call handle_failure as it will be called as part of
             * handle
             */
            return;
        }
        auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
        pType::db_ID _dbID = m_owner.m_hash_helper.get_db(seg_hash);
        m_owner.m_rpc_helper.del(m_name.data(), iter, m_owner.get_db_snapshot(_dbID), [this, idx](grpc::Status status, registration_apis::db_rsp& rsp){
                if (status.ok() && rsp.rsp()) {
                    handle(nullptr, success, idx);
                } else {
                    handle(nullptr, failure, idx);
                }
                });
        m_sent_segments++;
        idx++;
    }

    {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        if (m_op_result != failure) {
            m_owner.remove_file(m_name);
        }
    }
}

void Blob_manager::File_del::done(void* mssg) 
{
    std::cout << __FUNCTION__ << " Done\n";
}
template class Blob_manager::File<char>;
template class Blob_manager::File<registration_apis::db_rsp>;
