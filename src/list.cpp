#include "list.h"
#include <utility>

void *
List::__get_key(_list_entry_t* __entry)
{
    if (__entry == nullptr) {
        return nullptr;
    }

    return __entry->key;
}

void *
List::__get_val(_list_entry_t* __entry)
{
    if (__entry == nullptr) {
        return nullptr;
    }

    return __entry->val;
}
List::_list_entry_t* 
List::__lookup_lockless(void *__key, void *__val)
{
    for(List::Iterator it = begin(); it != end(); it++) {
        auto __cur_entry = *it;
        if (__get_key(__cur_entry)) {
            if (m_lookup_func(__get_key(__cur_entry), __key))
            {
                if (__cur_entry->m_is_moved) {
                    return nullptr;
                } else {
                    ;
                    return __cur_entry;
                }
            }
        } else {
            if (m_lookup_func(__get_val(__cur_entry), __val))
            {
                if (__cur_entry->m_is_moved) {
                    return nullptr;
                } else {
                    return __cur_entry;
                }
            }
        }
    }
    return nullptr;
}

bool 
List::__insert_head_lockless(void *__key, void *__val)
{
#ifdef LOOKUP_NEEDED
    if (__key) {
        if (__lookup_lockless(__key, nullptr)) {
            return false;
        }
    } else {
        if (__lookup_lockless(nullptr, __val)) {
            return false;
        }

    }
#endif
    _list_entry_t *_new_entry = new _list_entry_t();
    if (_new_entry == nullptr) {
        return false;
    }
    _new_entry->next = m_head;
    _new_entry->prev = nullptr;
    _new_entry->key = __key;
    _new_entry->val = __val;
    _new_entry->m_is_moved = false;
    
    m_head->prev = _new_entry;
    m_head = _new_entry;

    return true;
}


bool 
List::__insert_tail_lockless(void *__key, void *__val)
{
#ifdef LOOKUP_NEEDED
    if (__key) {
        if (__lookup_lockless(__key, nullptr)) {
            return false;
        }
    } else {
        if (__lookup_lockless(nullptr, __val)) {
            return false;
        }
    }
#endif
    _list_entry_t *_new_entry = new _list_entry_t();
    if (_new_entry == nullptr) {
        return false;
    }

    _new_entry->next = m_tail;
    _new_entry->prev = m_tail->prev;
    _new_entry->key = __key;
    _new_entry->val = __val;
    _new_entry->m_is_moved = false;
    if (m_head == m_tail) {
        m_head = _new_entry;
        m_tail->prev = _new_entry;
    }

    return true;
}
void
List::set_moved(void *_key, void *_val)
{
   _list_entry_t *__entry = nullptr;

    std::lock_guard<std::recursive_mutex> guard(m_list_lock);
   if (_key) {
        __entry = __lookup_lockless(_key, nullptr);
        if (__entry) {
            __entry->m_is_moved = true;
        }
   } else {
        __entry = __lookup_lockless(nullptr, _val);
        if (__entry) {
            __entry->m_is_moved = true;
        }
   } 
}

void
List::set_moved(Iterator& _iter)
{
    _list_entry_t *__entry = nullptr;

    std::lock_guard<std::recursive_mutex> guard(m_list_lock);
    __entry = *_iter;
    __entry->m_is_moved = true;
}
bool 
List::__remove_lockless (void *__key, void *__val)
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
            _del_entry->next->prev = _del_entry->prev;

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
            _del_entry->next->prev = _del_entry->prev;

            delete _del_entry;
            return true;
        }

        return false;
    }

    return false;
}
        
List::List (bool (*__lookup_func) (void*, void*))
    : m_lookup_func(__lookup_func), m_tail(new _list_entry_t_()), m_head(m_tail),
      m_is_multi_threaded(false)
{
}
List::List(bool (*__lookup_func) (void*, void*), bool __is_multi_threaded)
    : m_lookup_func(__lookup_func), m_tail(new _list_entry_t_()), m_head(m_tail),
      m_is_multi_threaded(__is_multi_threaded)
{
}

bool
List::insert_head(void *__key, void *__val)
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

bool
List::insert_tail(void *__key, void *__val)
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

bool
List::remove(void *__key, void *__val)
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

void* 
List::lookup(void *__key, void *__val)
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

void**
List::lookup_mutable(void *__key, void *__val)
{
    _list_entry_t *_cur_entry = nullptr;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        _cur_entry = __lookup_lockless(__key, __val);
        if (_cur_entry) {
            return &_cur_entry->val;
        }
    } else {
        _cur_entry = __lookup_lockless(__key, __val);
        if (_cur_entry) {
            return &_cur_entry->val;
        }
    }

    return nullptr;
}

List::~List()
{
    delete m_tail;
}
