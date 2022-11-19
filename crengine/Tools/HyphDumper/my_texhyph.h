/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
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

// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#ifndef MY_TEXHYPH_H
#define MY_TEXHYPH_H

#include "hyphman.h"
#include "my_texpattern.h"

class MyTexHyph : public HyphMethod
{
    MyTexPattern * table[PATTERN_HASH_SIZE];
    lUInt32 _hash;
    lUInt32 _pattern_count;
public:
    int largest_overflowed_word;
    bool match( const lChar32 * str, char * mask );
    virtual bool hyphenate( const lChar32 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize );
    void addPattern( MyTexPattern * pattern );
    MyTexHyph( lString32 id=HYPH_DICT_ID_DICTIONARY );
    virtual ~MyTexHyph();
    bool load( LVStreamRef stream );
    bool load( lString32 fileName );
    virtual lUInt32 getHash() { return _hash; }
    virtual lUInt32 getCount() { return _pattern_count; }
    virtual lUInt32 getSize();

    // added to dump content to stream
    bool dump(LVStreamRef stream, const lString8 &title);
};

#endif // MY_TEXHYPH_H
