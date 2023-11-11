#include "list.h"
#include "iterator.h"
#include <functional>
#include <iterator>
#include <utility>

void *
List::__get_val(_list_entry_t* __entry)
{
    if (__entry == nullptr) {
        return nullptr;
    }

    return __entry->val;
}
List::_list_entry_t* 
List::__lookup_lockless(void *__val)
{
    for(List::Iterator it = begin(); it != end(); it++) {
        auto __cur_entry = *it;
        if (m_lookup_func(__get_val(__cur_entry), __val))
        {
            return __cur_entry;
        }
    }
    return nullptr;
}

bool 
List::__insert_head_lockless(void *__val)
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
    _new_entry->val = __val;
    
    m_head->prev = _new_entry;
    m_head = _new_entry;

    return true;
}


bool 
List::__insert_tail_lockless(void *__val)
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
    _new_entry->val = __val;
    if (m_head == m_tail) {
        m_head = _new_entry;
        m_tail->prev = _new_entry;
    }

    return true;
}

bool
List::__remove_internal(_list_entry_t* __val)
{

    _list_entry_t *_del_entry = nullptr;

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

bool 
List::__remove_lockless (void* __val)
{
    _list_entry_t *_del_entry = nullptr;
    _del_entry = __lookup_lockless(__val);

    return __remove_internal(_del_entry);
}
        
List::List(std::function<bool(void*, void*)> __func, bool __is_multi_threaded)
    : m_lookup_func(__func), m_tail(new _list_entry_t_()), m_head(m_tail),
      m_is_multi_threaded(__is_multi_threaded)
{
}
List::List(std::function<bool(void*, void*)> __func)
    : m_lookup_func(__func), m_tail(new _list_entry_t_()), m_head(m_tail),
      m_is_multi_threaded(false)
{}

bool
List::insert_head(void *__val)
{
    bool ret = false;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        ret = __insert_head_lockless(__val);
    } else {
        ret = __insert_head_lockless(__val);
    }

    return ret;
}

bool
List::insert_tail(void *__val)
{
    bool ret = false;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        ret = __insert_tail_lockless(__val);
    } else {
        ret = __insert_tail_lockless(__val);
    }

    return ret;
}

bool
List::remove_entry(void *__val)
{
    bool ret = false;
    _list_entry_t* __del_entry = nullptr;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        __del_entry = static_cast<_list_entry_t*>(__val);
        ret = __remove_internal(__del_entry);
    } else {
        ret = __remove_internal(__del_entry);
    }

    return ret;
}

bool
List::remove(void *__val)
{
    bool ret = false;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        ret = __remove_lockless(__val);
    } else {
        ret = __remove_lockless(__val);
    }

    return ret;
}

void* 
List::lookup(void *__val)
{
    _list_entry_t *_cur_entry = nullptr;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        _cur_entry = __lookup_lockless(__val);
        if (_cur_entry) {
            return _cur_entry->val;
        }
    } else {
        _cur_entry = __lookup_lockless(__val);
        if (_cur_entry) {
            return _cur_entry->val;
        }
    }

    return nullptr;
}

void**
List::lookup_mutable(void *__val)
{
    _list_entry_t *_cur_entry = nullptr;

    if (m_is_multi_threaded) {
        std::lock_guard<std::recursive_mutex> guard(m_list_lock);
        _cur_entry = __lookup_lockless(__val);
        if (_cur_entry) {
            return &_cur_entry->val;
        }
    } else {
        _cur_entry = __lookup_lockless(__val);
        if (_cur_entry) {
            return &_cur_entry->val;
        }
    }

    return nullptr;
}

List::Iterator
List::iter_init()
{
    return this->begin();
}

List::Iterator
List::iter_advance(List::Iterator& _iter)
{
    std::lock_guard<std::recursive_mutex> guard(m_list_lock);
    List::Iterator __ret(nullptr);
    if (_iter.is_same() == false) {
        _iter++;
        return _iter;
    } 

    __ret = _iter;
    _iter.update_old(nullptr);
    _iter++;
    return __ret;
}

List::~List()
{
    delete m_tail;
}
