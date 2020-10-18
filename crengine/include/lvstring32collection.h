/** \file lvstring16collection.h
    \brief collection of wide strings

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRING32COLLECTION_H_INCLUDED__
#define __LV_STRING32COLLECTION_H_INCLUDED__

#include "lvstring.h"

/// collection of wide strings
class lString32Collection
{
private:
    lstring32_chunk_t * * chunks;
    int count;
    int size;
public:
    lString32Collection()
        : chunks(NULL), count(0), size(0)
    { }
    /// parse delimiter-separated string
    void parse( lString32 string, lChar32 delimiter, bool flgTrim );
    /// parse delimiter-separated string
    void parse( lString32 string, lString32 delimiter, bool flgTrim );
    void reserve(int space);
    int add( const lString32 & str );
    int add(const lChar32 * str) { return add(lString32(str)); }
    int add(const lChar8 * str) { return add(lString32(str)); }
    void addAll( const lString32Collection & v )
    {
        for (int i=0; i<v.length(); i++)
            add( v[i] );
    }
    int insert( int pos, const lString32 & str );
    void erase(int offset, int count);
    /// split into several lines by delimiter
    void split(const lString32 & str, const lString32 & delimiter);
    const lString32 & at(int index)
    {
        return ((lString32 *)chunks)[index];
    }
    const lString32 & operator [] (int index) const
    {
        return ((lString32 *)chunks)[index];
    }
    lString32 & operator [] (int index)
    {
        return ((lString32 *)chunks)[index];
    }
    int length() const { return count; }
    void clear();
    bool contains( lString32 value )
    {
        for (int i = 0; i < count; i++)
            if (value.compare(at(i)) == 0)
                return true;
        return false;
    }
    void sort();
    void sort(int(comparator)(lString32 & s1, lString32 & s2));
    ~lString32Collection()
    {
        clear();
    }
};

#endif // __LV_STRING32COLLECTION_H_INCLUDED__
