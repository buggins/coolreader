/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
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

#ifndef __LVTEXTFILELINE_H_INCLUDED__
#define __LVTEXTFILELINE_H_INCLUDED__

#include "lvtextfilebase.h"

typedef enum {
    la_unknown,  // not detected
    la_empty,    // empty line
    la_left,     // left aligned
    la_indent,   // right aligned
    la_centered, // centered
    la_right,    // right aligned
    la_width     // justified width
} lineAlign_t;

class LVTextFileLine
{
public:
    //lvpos_t fpos;   // position of line in file
    //lvsize_t fsize;  // size of data in file
    lUInt32 flags;  // flags. 1=eoln
    lString32 text; // line text
    lUInt16 lpos;   // left non-space char position
    lUInt16 rpos;   // right non-space char posision + 1
    lineAlign_t align;
    bool empty() { return rpos==0; }
    bool isHeading() { return (flags & LINE_IS_HEADER)!=0; }
    LVTextFileLine( LVTextFileBase * file, int maxsize )
        : flags(0), lpos(0), rpos(0), align(la_unknown)
    {
        text = file->ReadLine( maxsize, flags );
        //CRLog::debug("  line read: %s", UnicodeToUtf8(text).c_str() );
        if ( !text.empty() ) {
            const lChar32 * s = text.c_str();
            for ( int p=0; *s; s++ ) {
                if ( *s == '\t' ) {
                    p = (p + 8)%8;
                } else {
                    if ( *s != ' ' ) {
                        if ( rpos==0 && p>0 ) {
                            //CRLog::debug("   lpos = %d", p);
                            lpos = (lUInt16)p;
                        }
                        rpos = (lUInt16)(p + 1);
                    }
                    p++;
                }
            }
        }
    }
};

#endif  // __LVTEXTFILELINE_H_INCLUDED__
