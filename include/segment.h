#pragma once

#include "types.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
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
