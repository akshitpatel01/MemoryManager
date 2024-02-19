#pragma once

#include "types.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string_view>
#include "registration_apis.pb.h"

static ID_helper<uint32_t> m_seg_id_helper{5000000};
template <typename T>
struct default_delete
{
    void operator() (T* val)
    {
        delete val;
    }
};

template <>
struct default_delete<char> {
    void operator() (char* val) {
        delete[] val;
    }
};

/* Template to pass around any kind of data.
 * Takes ownership and manages a pointer
 */
template <typename data_t, class deleter>
class segment_base {
    protected:
        using pointer_t = data_t*;
        using unique_pointer_t = std::unique_ptr<data_t, deleter>;

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
        ~segment_base()
        {
            m_seg_id_helper.free_id(m_segment_id);
        }

    protected:
        const pointer_t get_data_pointer() const
        {
            return m_data.get();
        }
    public:
        uint32_t get_len() const
        {
            return m_len;
        }
        uint32_t get_id() const
        {
            return m_segment_id;
        }

        std::size_t get_hash() const
        {
            return m_hash;
        }

        std::string_view get_file_name() const
        {
            return m_file_path;
        }
    protected:
        unique_pointer_t m_data;
        uint32_t m_len;
        std::string_view m_file_path; 
        pType::segment_ID m_segment_id;
        std::size_t m_hash;

};

template <typename data_t, class deleter = default_delete<data_t>>
class segment: public segment_base<data_t, deleter> {
    using _base = segment_base<data_t, deleter>;
    using pointer_t = typename _base::pointer_t;
    private:
        explicit segment(pointer_t _data, uint32_t _data_len, std::string_view _file_path,
                            uint32_t _segment_id = m_seg_id_helper.get_id())
            : _base(std::unique_ptr<data_t, deleter>(_data), _data_len, _file_path, _segment_id)
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
        
        const void* get_data() const
        {
            return _base::get_data_pointer();
        }
};

template<>
class segment<registration_apis::db_rsp, default_delete<registration_apis::db_rsp>>:
    public segment_base<registration_apis::db_rsp, default_delete<registration_apis::db_rsp>> {
    using deleter = default_delete<registration_apis::db_rsp>;
    using data_t = registration_apis::db_rsp;
    using _base = segment_base<data_t, default_delete<registration_apis::db_rsp>>;
    using pointer_t = typename _base::pointer_t;
    private:
        explicit segment(pointer_t _data, uint32_t _data_len, std::string_view _file_path,
                            uint32_t _segment_id = m_seg_id_helper.get_id())
            : _base(std::unique_ptr<data_t, deleter>(_data), _data_len, _file_path, _segment_id)
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
