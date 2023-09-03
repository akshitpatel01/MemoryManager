#pragma once

template <class T>
class Iterator {
    protected:
        using ValueType = typename T::ValueType;
        using ReferenceType = ValueType&;
        using PointerType = ValueType*;

    protected:
        PointerType m_ptr;
    public:
        Iterator(PointerType _ptr)
            :m_ptr(_ptr)
        {
        }

        virtual Iterator& operator++(int)
        {
            m_ptr = m_ptr->next;
            return *this;
        }

        virtual Iterator& operator--(int)
        {
            m_ptr = m_ptr->prev;
            return *this;
        }

        PointerType operator*()
        {
           return m_ptr;
        }

        bool operator==(const Iterator& _other)
        {
            return (m_ptr == _other.m_ptr);
        }

        bool operator!=(const Iterator& _other)
        {
            return (m_ptr != _other.m_ptr);
        }
};
