#pragma once

#include <cstdint>
#include <memory>
#include <vector>

template <typename T>
class ID_helper {
    private:
        std::vector<T> m_free_list;
        T m_max_id;
        T m_cur_id;
    public:
        ID_helper(T _max_id)
            :m_max_id(_max_id), m_cur_id{1}
        {
        }

        T get_id()
        {
            T __ret = m_max_id;

            if (m_free_list.size() > 0) {
                __ret = m_free_list.back();
                m_free_list.pop_back();
            } else if (m_cur_id <= m_max_id) {
                __ret = m_cur_id++;
            }

            return __ret;
        }

        void free_id(T& _id)
        {
            m_free_list.push_back(_id);
        }
};
static ID_helper<uint32_t> m_seg_id_helper{5000000};
class segment {
    private:
        const void* m_data;
        uint32_t m_len;
        const char* m_file_name; 
        uint32_t m_segment_id;
    private:
        friend class db_instance;
    private:
        segment(const void* _data, uint32_t _data_len, const char* _file_name,
                    uint32_t _segment_id = m_seg_id_helper.get_id())
            : m_data(_data), m_len(_data_len), m_file_name(_file_name),
              m_segment_id(_segment_id)
    {
    }

    public:
        template<typename... T>
        static std::shared_ptr<segment> create_segment(T &&...args)
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

