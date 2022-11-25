/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013,2015 Vadim Lopatin <coolreader.org@gmail.com>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#ifndef LVAUTOPTR_H
#define LVAUTOPTR_H

/// auto pointer
template <class T >
class LVAutoPtr {
    T * p;
    LVAutoPtr( const LVAutoPtr & v ) {
        CR_UNUSED(v);
    } // no copy allowed
    LVAutoPtr & operator = (const LVAutoPtr & v) {
        CR_UNUSED(v);
        return *this;
    } // no copy
public:
    LVAutoPtr()
        : p(NULL)
{
    }
    explicit LVAutoPtr( T* ptr )
        : p(ptr)
    {
    }
    bool isNull() const {
        return p == NULL;
    }
    bool operator !() const { return p == NULL; }
    inline void clear()
    {
        if (p)
            delete p;
        p = NULL;
    }
    ~LVAutoPtr()
    {
        clear();
    }
    inline T * operator -> ()
    {
        return p;
    }
    inline const T * operator -> () const
    {
        return p;
    }
    inline T & operator [] (int index) { return p[index]; }
    inline T * get() const { return p; }
    inline T & operator * ()
    {
        return *p;
    }
    inline const T & operator * () const
    {
        return *p;
    }
    inline LVAutoPtr & operator = ( T* ptr )
    {
        if ( p==ptr )
            return *this;
        if ( p )
            delete p;
        p = ptr;
        return *this;
    }
};

/// unique pointer
template <class T >
class LVUniquePtr {
    T * p;
public:
    LVUniquePtr()
        : p(NULL)
{
    }
    explicit LVUniquePtr( T* ptr )
        : p(ptr)
    {
    }
    LVUniquePtr( const LVUniquePtr & v ) { p = v.p; v.p = NULL; }
    LVUniquePtr & operator = (const LVUniquePtr & v) {
        clear();
        p = v.p; v.p = NULL;
        return *this;
    } // no copy
    bool isNull() const {
        return p == NULL;
    }
    bool operator !() const { return p == NULL; }
    inline void clear()
    {
        if (p)
            delete p;
        p = NULL;
    }
    ~LVUniquePtr()
    {
        clear();
    }
    inline T * operator -> ()
    {
        return p;
    }
    inline const T * operator -> () const
    {
        return p;
    }
    inline T * get() const { return p; }
    inline T & operator * ()
    {
        return *p;
    }
    inline const T & operator * () const
    {
        return *p;
    }
    inline LVUniquePtr & operator = ( T* ptr )
    {
        if ( p==ptr )
            return *this;
        if ( p )
            delete p;
        p = ptr;
        return *this;
    }
};

/// unique pointer
template <class T >
class LVClonePtr {
    T * p;
public:
    LVClonePtr()
        : p(NULL)
{
    }
    explicit LVClonePtr( T* ptr )
        : p(ptr ? (T*)ptr->clone() : NULL)
    {
    }
    LVClonePtr( const LVClonePtr & v ) { p = v.p ? (T*)v.p->clone() : NULL; }
    LVClonePtr & operator = (const LVClonePtr & v) {
        clear();
        p = v.p ? (T*)v.p->clone() : NULL;
        return *this;
    } // no copy
    bool isNull() const {
        return p == NULL;
    }
    bool operator !() const { return p == NULL; }
    inline void clear()
    {
        if (p)
            delete p;
        p = NULL;
    }
    ~LVClonePtr()
    {
        clear();
    }
    inline T * operator -> ()
    {
        return p;
    }
    inline const T * operator -> () const
    {
        return p;
    }
    inline T & operator [] (int index) { return p[index]; }
    inline T * get() const { return p; }
    inline T & operator * ()
    {
        return *p;
    }
    inline const T & operator * () const
    {
        return *p;
    }
    inline LVClonePtr & operator = ( T* ptr )
    {
        if ( p==ptr )
            return *this;
        if ( p )
            delete p;
        p = ptr ? (T*)ptr->clone() : NULL;
        return *this;
    }
};


#endif // LVAUTOPTR_H
