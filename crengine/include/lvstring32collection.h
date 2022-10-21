/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2010,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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
