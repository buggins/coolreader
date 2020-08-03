/** \file lvstringbuf16.h
    \brief string classes interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRINGBUF16_H_INCLUDED__
#define __LV_STRINGBUF16_H_INCLUDED__

#include "../include/lvstring.h"

/// fast 16-bit string character appender
template <int BUFSIZE> class lStringBuf16 {
    lString16 & str;
    lChar16 buf[BUFSIZE];
    int pos;
    lStringBuf16 & operator = (const lStringBuf16 & v)
    {
        CR_UNUSED(v);
        // not available
        return *this;
    }
public:
    lStringBuf16( lString16 & s )
    : str(s), pos(0)
    {
    }
    inline void append( lChar16 ch )
    {
        buf[ pos++ ] = ch;
        if ( pos==BUFSIZE )
            flush();
    }
    inline lStringBuf16& operator << ( lChar16 ch )
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
    ~lStringBuf16( )
    {
        flush();
    }
};

#endif // __LV_STRINGBUF16_H_INCLUDED__
