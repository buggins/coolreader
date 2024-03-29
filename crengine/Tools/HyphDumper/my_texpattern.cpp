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

#include "my_texpattern.h"

#include <string.h>

int MyTexPattern::cmp(MyTexPattern *v)
{
    return lStr_cmp( word, v->word );
}

bool MyTexPattern::match(const lChar32 *s, char *mask)
{
    MyTexPattern * p = this;
    bool found = false;
    while ( p ) {
        bool res = true;
        for ( int i=2; p->word[i]; i++ )
            if ( p->word[i]!=s[i] ) {
                res = false;
                break;
            }
        if ( res ) {
            if ( p->word[0]==s[0] && (p->word[1]==0 || p->word[1]==s[1]) ) {
#if DUMP_PATTERNS==1
                CRLog::debug("Pattern matched: %s %s on %s %s", LCSTR(lString32(p->word)), p->attr, LCSTR(lString32(s)), mask);
#endif
                p->apply(mask);
                found = true;
            }
        }
        p = p->next;
    }
    return found;
}

void MyTexPattern::apply(char *mask)
{
    ;
    for ( char * p = attr; *p && *mask; p++, mask++ ) {
        if ( *mask < *p )
            *mask = *p;
    }
}

MyTexPattern::MyTexPattern(const lString32 &s)
 : next( NULL )
{
    overflowed = 0;
    memset( word, 0, sizeof(word) );
    memset( attr, '0', sizeof(attr) );
    attr[sizeof(attr)-1] = 0;
    int n = 0;      // char number in pattern
    for ( int i=0; i<(int)s.length(); i++ ) {
        lChar32 ch = s[i];
        if (n > MAX_PATTERN_SIZE) {
            if ( ch<'0' || ch>'9' ) {
                overflowed = n++;
            }
            continue;
        }
        if ( ch>='0' && ch<='9' ) {
            attr[n] = (char)ch;
            //                if (n>0)
            //                    attr[n-1] = (char)ch;
        } else {
            if (n == MAX_PATTERN_SIZE) { // we previously reached max word size
                // Let the last 0 (string termination) in
                // word[MAX_PATTERN_SIZE] and mark it as overflowed
                overflowed = n++;
            }
            else {
                word[n++] = ch;
            }
        }
    }
    // if n==MAX_PATTERN_SIZE (or >), attr[MAX_PATTERN_SIZE] is either the
    // memset '0', or a 0-9 we got on next iteration, and
    // attr[MAX_PATTERN_SIZE+1] is the 0 set by attr[sizeof(attr)-1] = 0
    if (n < MAX_PATTERN_SIZE)
        attr[n+1] = 0;
    
    if (overflowed)
        overflowed = overflowed + 1; // convert counter to number of things counted
}

MyTexPattern::MyTexPattern(const unsigned char *s, int sz, const lChar32 *charMap)
 : next(NULL)
{
    overflowed = 0;
    if ( sz > MAX_PATTERN_SIZE ) {
        overflowed = sz;
        sz = MAX_PATTERN_SIZE;
    }
    memset( word, 0, sizeof(word) );
    memset( attr, 0, sizeof(attr) );
    for ( int i=0; i<sz; i++ )
        word[i] = charMap[ s[i] ];
    memcpy( attr, s+sz, sz+1 );
}
