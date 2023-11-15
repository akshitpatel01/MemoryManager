#pragma once

#include <cstdint>
#include <memory>
class segment {
    private:
        void* m_data;
        uint32_t m_len;
        const char* m_file_name; 
        uint32_t m_segment_id;
    private:
        friend class db_instance;
    private:
        segment(void* _data, uint32_t _data_len, const char* _file_name,
                    uint32_t _segment_id)
            : m_data(_data), m_len(_data_len), m_file_name(_file_name),
              m_segment_id(_segment_id)
    {

    }

    public:
        template<typename... T>
        static std::shared_ptr<segment> create_shared(T &&...args)
        {
            return std::shared_ptr<segment>(new segment(std::forward<T>(args)...));
        }
        uint32_t get_len()
        {
            return m_len;
        }
        uint32_t get_id()
        {
            return m_segment_id;
        }
};
