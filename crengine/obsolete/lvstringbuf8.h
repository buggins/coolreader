/** \file lvstringbuf8.h
    \brief string classes interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRINGBUF8_H_INCLUDED__
#define __LV_STRINGBUF8_H_INCLUDED__

#include "../include/lvstring.h"

/// fast 8-bit string character appender
template <int BUFSIZE> class lStringBuf8 {
    lString8 & str;
    lChar8 buf[BUFSIZE];
    int pos;
public:
    lStringBuf8( lString8 & s )
    : str(s), pos(0)
    {
    }
    inline void append( lChar8 ch )
    {
        buf[ pos++ ] = ch;
        if ( pos==BUFSIZE )
            flush();
    }
    inline lStringBuf8& operator << ( lChar8 ch )
    {
        buf[ pos++ ] = ch;
        if ( pos==BUFSIZE )
            flush();
        return *this;
    }
    inline void flush()
    {
        str.append( buf, pos );
        pos = 0;
    }
    ~lStringBuf8( )
    {
        flush();
    }
};

#endif // __LV_STRINGBUF8_H_INCLUDED__
