#pragma once

#include <mutex>
class List {
    private:
        typedef struct _list_entry_t_ {
            struct _list_entry_t_* next;
            struct _list_entry_t_* prev;
            void *val;
            void *key;
        }_list_entry_t;
        bool (*m_lookup_func) (void*, void*);
        _list_entry_t_ *m_head;
        _list_entry_t_ *m_tail;
        std::recursive_mutex m_list_lock;
        bool m_is_multi_threaded;

    public:
        typedef struct _list_iter_t_{
            struct _list_entry_t_* cur;
            struct _list_entry_t_* next;
            struct _list_entry_t_* prev;
        } list_iter_t;

    private:
        void *__get_key(_list_entry_t* __entry)
        {
            if (__entry == nullptr) {
                return nullptr;
            }

            return __entry->key;
        }
        
        void *__get_val(_list_entry_t* __entry)
        {
            if (__entry == nullptr) {
                return nullptr;
            }

            return __entry->val;
        }

        _list_entry_t* __iter_deref (list_iter_t *__iter)
        {
            if (__iter == nullptr || __iter->cur == nullptr) {
                return nullptr;
            }
            _list_entry_t *_cur_entry = static_cast<_list_entry_t*>(__iter->cur);

            return _cur_entry;
        }
        
        _list_entry_t* __lookup_lockless(void *__key, void *__val)
        {
            list_iter_t *_iter = nullptr;
            _list_entry_t *_cur_entry = nullptr;

            _iter = iter_init();
            if (_iter == nullptr) {
                return nullptr;
            }

            while((_cur_entry = __iter_deref(_iter)) != nullptr) {
                if (__key) {
                    if (m_lookup_func(__get_key(_cur_entry), __key))
                    {
                        iter_clear(_iter);
                        return _cur_entry;
                    }
                } else {
                    if (m_lookup_func(__get_val(_cur_entry), __val))
                    {
                        iter_clear(_iter);
                        return _cur_entry;
                    }
                }
            }

            return nullptr;
        }
        bool __insert_head_lockless(void *__key, void *__val)
        {
            if (__key) {
                if (__lookup_lockless(__key, nullptr)) {
                    return false;
                }
            } else {
                if (__lookup_lockless(nullptr, __val)) {
                    return false;
                }

            }
           _list_entry_t *_new_entry = new _list_entry_t();
           if (_new_entry == nullptr) {
               return false;
           }

           _new_entry->next = m_head;
           _new_entry->prev = nullptr;
           _new_entry->key = __key;
           _new_entry->val = __val;
           if (m_head) {
               m_head->prev = _new_entry;
           } else {
               m_tail = _new_entry;
           }
           m_head = _new_entry;

           return true;
        }
        bool __insert_tail_lockless(void *__key, void *__val)
        {
            if (__key) {
                if (__lookup_lockless(__key, nullptr)) {
                    return false;
                }
            } else {
                if (__lookup_lockless(nullptr, __val)) {
                    return false;
                }

            }
           _list_entry_t *_new_entry = new _list_entry_t();
           if (_new_entry == nullptr) {
               return false;
           }

           _new_entry->next = nullptr;
           _new_entry->prev = m_tail;
           _new_entry->key = __key;
           _new_entry->val = __val;
           if (m_tail) {
               m_tail->next = _new_entry;
           } else {
               m_head = _new_entry;
           }
           m_tail = _new_entry;

           return true;
        }
        bool __remove_lockless(void *__key, void *__val)
        {
            _list_entry_t *_del_entry = nullptr;

            if (__key) {
               _del_entry = __lookup_lockless(__key, nullptr); 
               if (_del_entry) {
                   if (_del_entry->prev) {
                       _del_entry->prev->next = _del_entry->next;
                   } else {
                       m_head = _del_entry->next;
                   }
                   if (_del_entry->next) {
                       _del_entry->next->prev = _del_entry->prev;
                   } else {
                       m_tail = _del_entry->prev;
                   }

                   delete _del_entry;
                   return true;
               }

               return false;
            } else {
               _del_entry = __lookup_lockless(nullptr, __val); 
               if (_del_entry) {
                   if (_del_entry->prev) {
                       _del_entry->prev->next = _del_entry->next;
                   } else {
                       m_head = _del_entry->next;
                   }
                   if (_del_entry->next) {
                       _del_entry->next->prev = _del_entry->prev;
                   } else {
                       m_tail = _del_entry->prev;
                   }

                   delete _del_entry;
                   return true;
               }

               return false;
            }

            return false;
        }

    public:
        List(bool (*__lookup_func) (void*, void*))
            : m_lookup_func(__lookup_func), m_head(nullptr), m_tail(nullptr),
              m_is_multi_threaded(false)
        {
        }
        List(bool (*__lookup_func) (void*, void*), bool __is_multi_threaded)
            : m_lookup_func(__lookup_func), m_head(nullptr), m_tail(nullptr),
              m_is_multi_threaded(__is_multi_threaded)
        {
        }

        bool insert_head(void *__key, void *__val)
        {
            bool ret = false;

            if (m_is_multi_threaded) {
                std::lock_guard<std::recursive_mutex> guard(m_list_lock);
                ret = __insert_head_lockless(__key, __val);
            } else {
                ret = __insert_head_lockless(__key, __val);
            }

            return ret;
        }

        bool insert_tail(void *__key, void *__val)
        {
            bool ret = false;

            if (m_is_multi_threaded) {
                std::lock_guard<std::recursive_mutex> guard(m_list_lock);
                ret = __insert_tail_lockless(__key, __val);
            } else {
                ret = __insert_tail_lockless(__key, __val);
            }

            return ret;
        }

        bool remove(void *__key, void *__val)
        {
            bool ret = false;

            if (m_is_multi_threaded) {
                std::lock_guard<std::recursive_mutex> guard(m_list_lock);
                ret = __remove_lockless(__key, __val);
            } else {
                ret = __remove_lockless(__key, __val);
            }

            return ret;
        }

        void* lookup(void *__key, void *__val)
        {
            _list_entry_t *_cur_entry = nullptr;

            if (m_is_multi_threaded) {
                std::lock_guard<std::recursive_mutex> guard(m_list_lock);
                _cur_entry = __lookup_lockless(__key, __val);
                if (_cur_entry) {
                    return _cur_entry->val;
                }
            } else {
                _cur_entry = __lookup_lockless(__key, __val);
                if (_cur_entry) {
                    return _cur_entry->val;
                }
            }

            return nullptr;
        }

        list_iter_t* iter_init()
        {
            list_iter_t* _new_iter = new(list_iter_t);
            if (_new_iter == nullptr) {
                return nullptr;
            }

            _new_iter->cur = m_head;
            _new_iter->prev = nullptr;
            if (m_head) {
                _new_iter->next = m_head->next;
            } else {
                _new_iter->next = nullptr;
            }

            return _new_iter;
        }

        void iter_clear(list_iter_t *__iter)
        {
            if (__iter) {
                delete __iter;
            }
        }

        list_iter_t* iter_inc(list_iter_t *__iter)
        {
            if (__iter == nullptr || __iter->cur == nullptr) {
                return nullptr;
            }

            __iter->prev = __iter->cur;
            __iter->cur = __iter->next;
            if (__iter->next != nullptr) {
                __iter->next = __iter->next->next;
            }

            return __iter;
        }
        
        list_iter_t* iter_dec(list_iter_t *__iter)
        {
            if (__iter == nullptr || __iter->cur == nullptr) {
                return nullptr;
            }

            __iter->next = __iter->cur;
            __iter->cur = __iter->prev;
            if (__iter->prev != nullptr) {
                __iter->prev = __iter->prev->prev;
            }

            return __iter;
        }
};
