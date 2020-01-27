/** \file lvstring16collection.h
    \brief collection of wide strings

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRING16COLLECTION_H_INCLUDED__
#define __LV_STRING16COLLECTION_H_INCLUDED__

#include "lvstring.h"

/// collection of wide strings
class lString16Collection
{
private:
    lstring16_chunk_t * * chunks;
    int count;
    int size;
public:
    lString16Collection()
        : chunks(NULL), count(0), size(0)
    { }
    /// parse delimiter-separated string
    void parse( lString16 string, lChar16 delimiter, bool flgTrim );
    /// parse delimiter-separated string
    void parse( lString16 string, lString16 delimiter, bool flgTrim );
    void reserve(int space);
    int add( const lString16 & str );
    int add(const lChar16 * str) { return add(lString16(str)); }
    int add(const lChar8 * str) { return add(lString16(str)); }
    void addAll( const lString16Collection & v )
    {
        for (int i=0; i<v.length(); i++)
            add( v[i] );
    }
    int insert( int pos, const lString16 & str );
    void erase(int offset, int count);
    /// split into several lines by delimiter
    void split(const lString16 & str, const lString16 & delimiter);
    const lString16 & at(int index)
    {
        return ((lString16 *)chunks)[index];
    }
    const lString16 & operator [] (int index) const
    {
        return ((lString16 *)chunks)[index];
    }
    lString16 & operator [] (int index)
    {
        return ((lString16 *)chunks)[index];
    }
    int length() const { return count; }
    void clear();
    bool contains( lString16 value )
    {
        for (int i = 0; i < count; i++)
            if (value.compare(at(i)) == 0)
                return true;
        return false;
    }
    void sort();
    void sort(int(comparator)(lString16 & s1, lString16 & s2));
    ~lString16Collection()
    {
        clear();
    }
};

#endif // __LV_STRING16COLLECTION_H_INCLUDED__
