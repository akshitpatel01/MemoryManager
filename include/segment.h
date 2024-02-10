#pragma once

#include "types.h"
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>
#include "registration_apis.pb.h"

static ID_helper<uint32_t> m_seg_id_helper{5000000};
/* Takes ownership and manages a pointer
 */
template <typename data_t>
class segment_base {
    protected:
        using pointer_t = data_t*;
        using unique_pointer_t = std::unique_ptr<data_t>;
        unique_pointer_t m_data;
        uint32_t m_len;
        std::string_view m_file_path; 
        pType::segment_ID m_segment_id;
        std::size_t m_hash;

    protected:
        explicit segment_base(unique_pointer_t&& _data_p, uint32_t _data_len, std::string_view _file_path,
                                uint32_t _segment_id) noexcept
            : m_data(std::move(_data_p)), m_len(_data_len), m_file_path(_file_path),
              m_segment_id(_segment_id), m_hash(Hashfn<pType::segment_ID, 1>{}(_segment_id)[0])
        {
        }
        segment_base(const segment_base&) = delete;
        segment_base& operator=(const segment_base&) = delete;
        segment_base(segment_base&& seg) = default;
        segment_base& operator=(segment_base&& seg) = default;
        ~segment_base() = default;

    protected:
        const pointer_t get_data_pointer()
        {
            return m_data.get();
        }
    public:
        uint32_t get_len()
        {
            return m_len;
        }
        uint32_t get_id()
        {
            return m_segment_id;
        }

        std::size_t get_hash()
        {
            return m_hash;
        }

        std::string_view get_file_name()
        {
            return m_file_path;
        }
};

template <typename data_t>
class segment: public segment_base<data_t> {
    using _base = segment_base<data_t>;
    using pointer_t = typename _base::pointer_t;
    private:
        explicit segment(pointer_t _data, uint32_t _data_len, std::string_view _file_path,
                            uint32_t _segment_id = m_seg_id_helper.get_id())
            : _base(std::unique_ptr<data_t>(_data), _data_len, _file_path, _segment_id)
        {}

    public:
        segment(const segment&) = delete;
        segment& operator=(const segment&) = delete;
        segment(segment&& seg) = default;
        segment& operator=(segment&& seg) = default;
        ~segment() = default;

    public:
        template<typename... T>
        static std::unique_ptr<segment> create_segment(T &&...args)
        {
            return std::unique_ptr<segment>(new segment(std::forward<T>(args)...));
        }
        
        const void* get_data()
        {
            return _base::get_data_pointer();
        }
};

template<>
class segment<registration_apis::db_rsp>: public segment_base<registration_apis::db_rsp> {
    using data_t = registration_apis::db_rsp;
    using _base = segment_base<data_t>;
    using pointer_t = typename _base::pointer_t;
    private:
        explicit segment(pointer_t _data, uint32_t _data_len, std::string_view _file_path,
                            uint32_t _segment_id = m_seg_id_helper.get_id())
            : _base(std::unique_ptr<data_t>(_data), _data_len, _file_path, _segment_id)
        {}
    
    public:
        segment(const segment&) = delete;
        segment& operator=(const segment&) = delete;
        segment(segment&& seg) = default;
        segment& operator=(segment&& seg) = default;
        ~segment() = default;
    public:
        template<typename... T>
        static std::unique_ptr<segment> create_segment(T &&...args)
        {
            return std::unique_ptr<segment>(new segment(std::forward<T>(args)...));
        }
        
        const void* get_data()
        {
            pointer_t _ptr = _base::get_data_pointer();
            return _ptr->data().c_str();
        }
};


class File {
    public:
    struct iter {
        const void* data;
        uint32_t len;
    };

    enum operation_t {
        ADD = 1,
        LOOKUP,
        DEL
    };

    enum status_t {
        in_progress = 1,
        success,
        failure
    };
    
    explicit File(std::string_view name, uint32_t total_segments, operation_t operation)
        :m_name{name}, m_segments(total_segments), m_iter(0), m_operation(operation),
         m_op_result(in_progress)
    {
        std::cout << "File constructed with name: " << m_name << " segments: " << m_segments.size() << " operation: " << m_operation << "\n"; 
    }

    /* block till the entire operation is completed
     */
    bool wait()
    {
        std::cout << "Waiting for operation to complete\n";
        std::unique_lock<std::mutex> lock_(m_condition_mutex);
        m_cv.wait(lock_, [this]{
                return (m_op_result != in_progress);
                });

        std::cout << "OPertion completed\n";
        return (m_op_result == failure) ? false : true;
    }
    std::string_view name()
    {
        return m_name;
    }

    bool handle_operation(std::unique_ptr<segment<registration_apis::db_rsp>>&& segment, status_t status, uint32_t idx)
    {
        bool ret_ = false;
        bool do_notify_ = false;

        if (idx >= m_segments.size()) {
            return ret_;
        }

        switch (m_operation) {
            case ADD:
            case DEL:
                m_segments[idx].set_segment_fill(status);
                ret_ = true;
                break;
            case LOOKUP:
                ret_ = m_segments[idx].set_segment(std::move(segment));
                break;
        }

        if (ret_) {
#if 0
            m_segment_latch.count_down();
            
            if (m_segment_latch.try_wait()) {
                // unblock wait;
                do_notify_ = true;
                m_op_result = success;
            }
#endif
            std::scoped_lock<std::mutex> lock_(m_condition_mutex);
            m_filled_segments++;
            if (m_filled_segments == m_segments.size()) {
                // unblock wait;
                do_notify_ = true;
                m_op_result = success;
            }
        } else {
            std::scoped_lock<std::mutex> lock_(m_condition_mutex);
            do_notify_ = true;
            m_op_result = failure;

        }
        if (do_notify_) {
            std::cout << "notifying File operation done\n";
            m_cv.notify_one();
        }
        return ret_;
    }

    bool next(iter& ptr)
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

    private:
        struct m_meta_internal {
            using status_t = typename File::status_t;
            m_meta_internal()
                :segment_(nullptr), segment_filled(in_progress)
            {}

            bool set_segment(std::unique_ptr<segment<registration_apis::db_rsp>>&& segment)
            {
                if (!segment) {
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
            private:
                std::unique_ptr<segment<registration_apis::db_rsp>> segment_; 
                status_t segment_filled;
                std::mutex m_lock;
                std::condition_variable m_cv;
        };

        std::string m_name;
        std::vector<m_meta_internal> m_segments;
        uint32_t m_iter;
        operation_t m_operation;
        //std::latch m_segment_latch;
        uint32_t m_filled_segments{};
        std::condition_variable m_cv;
        std::mutex m_condition_mutex;
        status_t m_op_result;
};
