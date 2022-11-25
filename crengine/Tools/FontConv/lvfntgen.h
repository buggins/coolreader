/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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

/*******************************************************
              LV Bitmap font interface
*******************************************************/

#ifndef __LVFNTGEN_H_INCLUDED__
#define __LVFNTGEN_H_INCLUDED__

#include "../../include/lvfnt.h"

struct glyph_range_buf
{
    tag_lvfont_range range;
    unsigned char buf[0x10000];
    int pos;
    glyph_range_buf();

    int getSize() { return (sizeof(range) + pos + 7)/8*8; }

    lvfont_glyph_t * addGlyph( unsigned short code );

    void commitGlyph();

    bool isEmpty() { return pos==0; }
};


struct font_gen_buf
{
    lvfont_header_t   hdr;
    glyph_range_buf * ranges[1024];
    glyph_range_buf * lastRange;
    hrle_decode_info_t * decodeTable;
    int decodeTableSize;

    lvfont_glyph_t *  addGlyph( unsigned short code );
    void commitGlyph();
    void setDecodeTable( hrle_decode_info_t * table );

    bool saveToFile( const char * fname );
    void init( int fntSize, int fntBaseline, int bitsPerPixel, const char * fontName, const char * fontCopyright );
    font_gen_buf();
    ~font_gen_buf();
};

#endif
