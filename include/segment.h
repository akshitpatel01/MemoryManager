#pragma once

#include "bsoncxx/document/value.hpp"
#include "bsoncxx/document/view.hpp"
#include "bsoncxx/types.hpp"
#include <algorithm>
#include <cstdint>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

//#define LOGS
template<typename K, typename V>
class std::hash<std::pair<K,V>> {
    public:
    size_t operator() (std::pair<K,V>& _obj)
    {
        return hash_combine(std::hash<K>{}(_obj.first), std::hash<V>{}(_obj.second));
    }
};
template <typename T, size_t N>
class Hashfn {
    public:
    std::array<size_t, N> operator() (T& key)
   {
       std::minstd_rand0 rng(std::hash<T>{}(key));
       std::array<std::size_t, N> hashes;
       std::generate(std::begin(hashes), std::end(hashes), rng);
       return hashes;
   }
};

template<size_t N>
class Hashfn<void*, N> {
    uint64_t m_offset = 14695981039346656037U;
    uint64_t m_prime = 1099511628211;
    public:
        std::array<size_t, N> operator() (void* _data, size_t _size)
        {
            size_t _hash = m_offset; 
            size_t i = 0;

            for (; i < _size; i++) {
                _hash ^= static_cast<char*>(_data)[i];
                _hash *= m_prime;
            }

            std::minstd_rand0 rng(_hash);
            std::array<std::size_t, N> hashes;
            std::generate(std::begin(hashes), std::end(hashes), rng);
            return hashes;
        }
};
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
#if 0
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
#endif

template <typename T>
struct add_inner_const {
    using type = const T;
};

template<typename T>
struct add_inner_const<T*> {
    using type = const T*;
};

template<typename T>
using add_inner_const_t = typename add_inner_const<T>::type;
template<typename ret_t>
class segment {
    protected:
        using return_t = add_inner_const_t<ret_t>;
    public:
        virtual uint32_t get_len()
        {
            return m_len;
        }
        virtual uint32_t get_id()
        {
            return m_segment_id;
        }

        virtual std::size_t get_hash()
        {
            return m_hash;
        }

        virtual std::string_view get_file_name()
        {
            return m_file_path;
        }
        virtual return_t get_data() = 0;
        virtual ~segment() = default;

    protected:
        segment(uint32_t _data_len, std::string&& _file_path,
                    uint32_t _segment_id)
            :m_len(_data_len), m_file_path(std::move(_file_path)),
             m_segment_id(_segment_id), m_hash(Hashfn<uint32_t, 1>{}(_segment_id)[0])
             {}

    protected:
        uint32_t m_len;
        std::string m_file_path; 
        uint32_t m_segment_id;
        std::size_t m_hash;
};

template <typename data_t, typename ret_t>
class owning_segment: public segment<ret_t> {
    private:
        data_t m_data;
        
    public:
        explicit owning_segment(data_t&& _data, uint32_t _data_len, std::string&& _file_path,
                    uint32_t _segment_id = m_seg_id_helper.get_id()) noexcept
            : segment<ret_t>{_data_len, std::move(_file_path), _segment_id}, m_data(std::move(_data))
        {
        }

        ret_t get_data()
        {
            return m_data;
        }
        ~owning_segment() = default;
};


template<typename ret_t>
class owning_segment<bsoncxx::document::value, ret_t>: public segment<ret_t> {
    using holder_t = bsoncxx::document::value;
    using view_t = bsoncxx::document::view;
    using return_t = typename segment<ret_t>::return_t;
    private:
        holder_t m_data;
        return_t m_ptr;
        
    public:
        explicit owning_segment(holder_t&& _data, uint32_t _data_len, std::string&& _file_path,
                    uint32_t _segment_id = m_seg_id_helper.get_id())
            : segment<ret_t>{_data_len, std::move(_file_path), _segment_id}, m_data(std::move(_data)) 
        {
        }

        return_t get_data() override
        {
            return (m_data.view()["m_segment_data"].get_binary().bytes);
        }
        
        ~owning_segment() = default;
};

template <typename data_t, typename ret_t = data_t*>
class observer_segment: public segment<ret_t> {
    using pointer_t = data_t*;
    using return_t = typename segment<ret_t>::return_t;
    private:
        const pointer_t m_data;

    public:
        explicit observer_segment(pointer_t _data, uint32_t _data_len, std::string&& _file_path,
                uint32_t _segment_id = m_seg_id_helper.get_id())
            : segment<ret_t>(_data_len, std::move(_file_path), _segment_id), m_data(_data)
            {
            }

         return_t get_data()
        {
            return m_data;
        }
        ~observer_segment() = default;
};
