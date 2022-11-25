/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) Alan <alan@alreader.com>                                *
 *   Copyright (C) 2009-2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
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

#ifndef MY_TEXPATTERN_H
#define MY_TEXPATTERN_H

#include "lvtypes.h"
#include "lvstring.h"

#define MAX_PATTERN_SIZE  35
#define PATTERN_HASH_SIZE 16384

class MyTexPattern {
public:
    lChar32 word[MAX_PATTERN_SIZE+1];
    char attr[MAX_PATTERN_SIZE+2];
    int overflowed; // 0, or size of complete word if larger than MAX_PATTERN_SIZE
    MyTexPattern * next;

    int cmp( MyTexPattern * v );

    static int hash( const lChar32 * s )
    {
        return ((lUInt32)(((s[0] *31 + s[1])*31 + s[2]) * 31 + s[3])) % PATTERN_HASH_SIZE;
    }

    static int hash3( const lChar32 * s )
    {
        return ((lUInt32)(((s[0] *31 + s[1])*31 + s[2]) * 31 + 0)) % PATTERN_HASH_SIZE;
    }

    static int hash2( const lChar32 * s )
    {
        return ((lUInt32)(((s[0] *31 + s[1])*31 + 0) * 31 + 0)) % PATTERN_HASH_SIZE;
    }

    static int hash1( const lChar32 * s )
    {
        return ((lUInt32)(((s[0] *31 + 0)*31 + 0) * 31 + 0)) % PATTERN_HASH_SIZE;
    }

    int hash()
    {
        return ((lUInt32)(((word[0] *31 + word[1])*31 + word[2]) * 31 + word[3])) % PATTERN_HASH_SIZE;
    }

    bool match( const lChar32 * s, char * mask );
    void apply( char * mask );

    MyTexPattern( const lString32 &s );
    MyTexPattern( const unsigned char * s, int sz, const lChar32 * charMap );
};

#endif	// MY_TEXPATTERN_H
