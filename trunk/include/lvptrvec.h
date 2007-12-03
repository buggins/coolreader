/** \file lvptrvec.h
    \brief pointer vector template

    Implements vector of pointers.

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVPTRVEC_H_INCLUDED__
#define __LVPTRVEC_H_INCLUDED__

#include <stdlib.h>
#include "lvmemman.h"

/** \brief template which implements vector of pointer

    Automatically deletes objects when vector items are destroyed.
*/
template < class T >
class LVPtrVector
{
    T * * _list;
    int _size;
    int _count;
public:
    /// default constructor
    LVPtrVector() : _list(NULL), _size(0), _count(0) {}
    /// retrieves item from specified position
    T * operator [] ( int pos ) const { return _list[pos]; }
    /// retrieves item from specified position
    T * get( int pos ) const { return _list[pos]; }
    /// retrieves item reference from specified position
    T * & operator [] ( int pos ) { return _list[pos]; }
    /// ensures that size of vector is not less than specified value
    void reserve( int size )
    {
        if ( size > _size )
        {
            _list = (T**)realloc( _list, size * sizeof( T* ));
            for (int i=_size; i<size; i++)
                _list[i] = NULL;
            _size = size;
        }
    }
    /// sets item by index (extends vector if necessary)
    void set( int index, T * item )
    {
        reserve( index+1 );
        while (length()<index)
            add(NULL);
        if ( _list[index] )
            delete _list[index];
        _list[index] = item;
        if (_count<=index)
            _count = index + 1;
    }
    /// returns size of buffer
    int size() const { return _size; }
    /// returns number of items in vector
    int length() const { return _count; }
    /// returns true if there are no items in vector
    bool empty() const { return _count==0; }
    /// clears all items
    void clear()
    {
        if (_list)
        {
            for (int i=0; i<_count; ++i)
                delete _list[i];
            free( _list );
        }
        _list = NULL;
        _size = 0;
        _count = 0;
    }
    /// removes several items from vector
    void erase( int pos, int count )
    {
        if ( pos<0 || count<=0 || pos+count > _count )
            crFatalError();
        int i;
        for (i=0; i<count; i++)
        {
            if (_list[pos+i])
            {
                delete _list[pos+i];
                _list[pos+i] = NULL;
            }
        }
        for (i=pos+count; i<_count; i++)
        {
            _list[i-count] = _list[i];
            _list[i] = NULL;
        }
        _count -= count;
    }
    /// removes item from vector by index
    T * remove( int pos )
    {
        if ( pos < 0 || pos > _count )
            crFatalError();
        int i;
        T * item = _list[pos];
        for ( i=pos; i<_count; i++ )
        {
            _list[i] = _list[i];
            _list[i] = NULL;
        }
        _count--;
        return item;
    }
    /// returns vector index of specified pointer, -1 if not found
    int indexOf( T * p )
    {
        for ( int i=0; i<_count; i++ ) {
            if ( _list[i] == p )
                return i;
        }
        return -1;
    }
    /// removes item from vector by index
    T * remove( T * p )
    {
        int i;
        int pos = indexOf( p );
        if ( pos<0 )
            return NULL;
        T * item = _list[pos];
        for ( i=pos; i<_count; i++ )
        {
            _list[i] = _list[i];
            _list[i] = NULL;
        }
        _count--;
        return item;
    }
    /// adds new item to end of vector
    void add( T * item ) { insert( -1, item ); }
    /// inserts new item to specified position
    void insert( int pos, T * item )
    {
        if (pos<0 || pos>_count)
            pos = _count;
        if ( _count >= _size )
            reserve( _count * 3 / 2  + 8 );
        for (int i=_count; i>pos; --i)
            _list[i] = _list[i-1];
        _list[pos] = item;
        _count++;
    }
    /// copy constructor
    LVPtrVector( const LVPtrVector & v )
        : _list(NULL), _size(0), _count(0)
    {
        if ( v._count>0 ) {
            reserve( v._count );
            for ( int i=0; i<v._count; i++ )
                add( new T(*v[i]) );
        }
    }
    /// destructor
    ~LVPtrVector() { clear(); }
};



#endif
