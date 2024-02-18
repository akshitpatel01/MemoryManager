#include "blob_manager.h"
#include "rpc.h"
#include "segment.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <vector>

Blob_manager::Blob_manager(Hash_helper& _hash_helper, RPC_helper& _rpc, Central_manager& _manager)
                            :m_hash_helper(_hash_helper), m_rpc_helper(_rpc), m_central_manager(_manager)
{}
            
#if 0
std::unique_ptr<Blob_manager::File> Blob_manager::async_add(std::string_view _file_path)
{
    PROFILE_SCOPE();
    std::ifstream _file(_file_path.data());
    size_t length, cur = 0;
    char* byte_array;
    uint32_t total_segments_ = 0;
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
    total_segments_ = std::ceil((float)length/SEGMENT_SIZE);
#ifdef LOGS
    std::cout << "File length: " << length << "\n";
    std::cout << "total segment: " << total_segments_ << "\n";
#endif
    _file.seekg(0, std::ios::beg);

    std::unique_ptr<File> file_p = std::make_unique<File>(_file_path, total_segments_, File::ADD);
    while(total_segments_--) {
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
                    if (!status_.ok() || !reply_.rsp()) {
                    ptr->handle(nullptr, File::failure, idx_);
                    std::cout << status_.error_message() << "\n";
                    } else {
                    ptr->handle(nullptr, File::success, idx_);
#ifdef LOGS
                    std::cout << __FUNCTION__ << " got reply yes!!!!\n";
#endif
                    }
                    })) {
            /* file_p will go out of scope and will wait till all the 
             * outstanding async (success) operations are completed (success
             * or failure)
             */
            return nullptr;
        }
        idx_++;
    } 

    return file_p;
}
#endif

std::unique_ptr<Blob_manager::File<char>> Blob_manager::async_stream_add(std::string_view _file_path)
{
    std::unique_ptr<File<char>> file_p = nullptr;
    file_p = std::make_unique<File_add>(*this, _file_path, ADD);

    return file_p;
#if 0
    class Impl {
        public:
            Impl(Blob_manager& owner, std::string_view _file_path)
                :m_owner(owner), m_file(_file_path.data()), m_file_path(_file_path)
            {
                owner.m_file_map.emplace(std::piecewise_construct,
                        std::forward_as_tuple(_file_path.data()),
                        std::forward_as_tuple(_file_path.data()));

                it = owner.m_file_map.find(_file_path.data());
    
                m_file.seekg(0, std::ios::end);
                length = m_file.tellg();
                total_segments_ = std::ceil((float)length/SEGMENT_SIZE);
                m_file.seekg(0, std::ios::beg);
            }

            bool next_chunk(registration_apis::add_meta& req)
            {
                if (total_segments_--) {
                    size_t diff = (cur+SEGMENT_SIZE > length) ? length-cur : SEGMENT_SIZE;
                    byte_array = new char[diff];
                    m_file.read(byte_array, diff);
                    cur += diff;

                    std::unique_ptr<segment_t> _seg = segment_t::create_segment(nullptr, diff, m_file_path);
                    pType::db_ID _dbID = m_owner.m_hash_helper.get_db(_seg->get_hash());

                    it->second.m_segs.push_back(_seg->get_id());
                    auto db_ = m_owner.m_central_manager.get_db_snap(_dbID);

                    req.set_id(_seg->get_id());
                    req.set_db_id(db_.m_db.m_id);
                    req.set_data(byte_array, _seg->get_len());
                    req.set_data_size(_seg->get_len());
                    req.set_file_name(m_file_path.data());
                    idx_++;
                    return true;
                }

                return false;
            }
            void done(registration_apis::db_rsp& rsp)
            {
                std::cout << "Add done\n";
                delete this;
            }

        private:
            Blob_manager& m_owner;
            std::ifstream m_file;
            std::string m_file_path;
            size_t length, cur = 0;
            char* byte_array;
            uint32_t total_segments_ = 0;
            std::unordered_map<std::string, file_meta_t>::iterator it;
            uint idx_ = 0;
    };
    Impl* obj = new Impl{*this, _file_path};
    auto wr = [obj](registration_apis::add_meta& req)
    {
        return obj->next_chunk(req);
    };
    auto done = [obj](registration_apis::db_rsp& rsp)
    {
        obj->done(rsp);
    };
    m_rpc_helper.stream_add(std::move(wr), std::move(done));
#endif
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

/* Return pointer as well as file object should work. Right now returning
 * ptr as moving will be as efficient as possible
 */
#if 0
std::unique_ptr<Blob_manager::File> Blob_manager::async_get(std::string_view file_path)
{
    PROFILE_SCOPE();
    uint32_t idx_ = 0;
    auto it = m_file_map.find(file_path.data());

    if (it == m_file_map.end()) {
#ifdef LOGS
        std::cout << "File " << file_path << " Not present in DB" << "\n";
#endif
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
                    if (status_.ok() && _rsp->rsp()) {
                    ptr->handle(segment<registration_apis::db_rsp>::create_segment(_rsp,
                                _rsp->data().length(),
                                ptr->name()),
                            File::success, idx_);
                    } else {
                    ptr->handle(segment<registration_apis::db_rsp>::create_segment(_rsp,
                                _rsp->data().length(),
                                ptr->name()),
                            File::failure, idx_);
                    }
                    })) {
            /* file_p will go out of scope and will wait till all the 
             * outstanding async (success) operations are completed (success
             * or failure)
             */
            return nullptr;
        }
        idx_++;
    }

    return file;
}
#endif

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
bool Blob_manager::File<T>::next(iter& ptr)
{
    bool ret_;
    if (m_operation != LOOKUP) {
        return false;
    }

    if (m_iter >= m_segments.size()) {
        return false;
    }


    ret_ = m_segments[m_iter].is_filled_wait();
    if (ret_) {
        ptr = {m_segments[m_iter].data(), m_segments[m_iter++].len()};
    }

    return ret_;
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
bool Blob_manager::File<T>::handle_failure()
{
    std::cout << __FUNCTION__ << " " << "Handling failure\n";
    {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        if (m_op_result == failure) {
            return true;
        }
        m_op_result = failure;
    }
    
    m_cv.notify_one();
    for (auto& segment: m_segments) {
        segment.handle_failure();
    }

    return true;
}


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
    for (auto iter: m_rpc_map) {
        iter.second->done();
    }
}

bool Blob_manager::File_add::handle(std::unique_ptr<segment_t>&& segment, status_t status, uint32_t idx)
{
    bool do_notify_ = false;

    if (idx >= m_total_segments) {
        return false;
    }

    m_segments[idx].set_segment_fill(status);

    if (status == success) {
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        m_filled_segments++;
        if (m_filled_segments == m_segments.size()) {
            /* unblock wait; */
            do_notify_ = true;
            m_op_result = success;
        }
    } else {
        /* Fixme: Add retry 
         */
        std::scoped_lock<std::mutex> lock_(m_condition_mutex);
        do_notify_ = true;
        m_op_result = failure;
    }
    if (do_notify_) {
#ifdef LOGS
        std::cout << "notifying File operation done\n";
#endif
        m_cv.notify_one();
    }
    
    return (status == success);
}

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
                            handle_failure();
                            std::cout << "Not ok\n";
                        }
                    }, [this](bool ok, uint32_t idx) -> void {
                        if (ok) {
                            handle(nullptr, success, idx);
                        } else {
                            handle(nullptr, failure, idx);
                            std::cout << "NOT OK\n";
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
        rpc_->write(get_segment(id_), db_meta.m_db.m_id, id_);
    }
}

void Blob_manager::File_add::done(void* mssg) 
{
    std::cout << __FUNCTION__ << " Done\n";
}

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
        handle_failure();
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
        auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
        pType::db_ID _dbID = m_owner.m_hash_helper.get_db(seg_hash);
        m_owner.m_rpc_helper.lookup(m_name.data(), iter, m_owner.get_db_snapshot(_dbID), [this, idx](grpc::Status status, registration_apis::db_rsp* rsp){
                if (status.ok() && rsp->rsp()) {
                    handle(segment_t::create_segment(rsp, rsp->data().length(), name()), success, idx);
                } else {
                    handle(nullptr, failure, idx);
                }
                });
        idx++;
    }
}
void Blob_manager::File_lookup::done(void* mssg) 
{
    std::cout << __FUNCTION__ << " Done\n";
}

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
        handle_failure();
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
        auto seg_hash = Hashfn<pType::segment_ID, 1>{}(iter)[0];
        pType::db_ID _dbID = m_owner.m_hash_helper.get_db(seg_hash);
        m_owner.m_rpc_helper.del(m_name.data(), iter, m_owner.get_db_snapshot(_dbID), [this, idx](grpc::Status status, registration_apis::db_rsp& rsp){
                if (status.ok() && rsp.rsp()) {
                    handle(nullptr, success, idx);
                } else {
                    handle(nullptr, failure, idx);
                }
                });
        idx++;
    }
    m_owner.remove_file(m_name);
}

void Blob_manager::File_del::done(void* mssg) 
{
    std::cout << __FUNCTION__ << " Done\n";
}
template class Blob_manager::File<char>;
template class Blob_manager::File<registration_apis::db_rsp>;
