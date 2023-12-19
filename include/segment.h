#pragma once

#include "types.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

static ID_helper<uint32_t> m_seg_id_helper{5000000};
template <typename data_t>
class segment {
    using pointer_t = data_t*;
    using unique_pointer_t = std::unique_ptr<data_t>;
    private:
        unique_pointer_t m_data;
        uint32_t m_len;
        std::string_view m_file_path; 
        pType::segment_ID m_segment_id;
        std::size_t m_hash;

    private:
        friend class db_instance;
    private:
        explicit segment(pointer_t _data, uint32_t _data_len, std::string_view _file_path,
                    uint32_t _segment_id = m_seg_id_helper.get_id()) noexcept
            : m_data(std::unique_ptr<data_t>(_data)), m_len(_data_len), m_file_path(_file_path),
              m_segment_id(_segment_id), m_hash(Hashfn<pType::segment_ID, 1>{}(_segment_id)[0])
        {
        }

    public:
        template<typename... T>
        static std::unique_ptr<segment> create_segment(T &&...args)
        {
            return std::unique_ptr<segment>(new segment(std::forward<T>(args)...));
        }
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

        void const * get_data()
        {
            return m_data.get();
        }
};
