/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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

/**
 * \file lvbmpbuf.h
 * \brief Gray bitmap buffer
 */

#ifndef __LVBMPBUF_H_INCLUDED__
#define __LVBMPBUF_H_INCLUDED__

#include "lvfnt.h"
//#include "lvtextfm.h"

/** \brief Bitmap buffer to draw in (C API) */
typedef struct
tag_draw_buf 
{
    int height;
    int bitsPerPixel;
    int bytesPerRow;
    unsigned char * data;
} draw_buf_t;


/** \brief Init drawing structure using existing buffer (C API)*/
void lvdrawbufInit( draw_buf_t * buf, int bitsPerPixel, int width, int height, unsigned char * data );

/** \brief Init drawing structure and allocate new buffer (C API)*/
void lvdrawbufAlloc( draw_buf_t * buf, int bitsPerPixel, int width, int height );

/** \brief Free buffer allocated by lvdrawbufAlloc (C API) */
void lvdrawbufFree( draw_buf_t * buf );

/** \brief Fill the whole buffer with specified data (C API) */
void lvdrawbufFill( draw_buf_t * buf, unsigned char pixel );

/** \brief Fill rectangle with specified data (C API) */
void lvdrawbufFillRect( draw_buf_t * buf, int x0, int y0, int x1, int y1, unsigned char pixel );

/** \brief Draw bitmap into buffer of the same bit depth (logical OR) (C API)
   
   \param x, y are coordinates where to draw
   \param bitmap is src bitmap data
   \[param numRows is number of rows of source bitmap
   \param bytesPerRow is number of bytes per row of source bitmap
*/
void lvdrawbufDraw( draw_buf_t * buf, int x, int y, const unsigned char * bitmap, int numRows, int bytesPerRow );

/** \brief Draw text string into buffer (logical OR)
   \param x, y are coordinates where to draw
   \param text is string to draw
   \param len is number of chars from text to draw
*/
void lvdrawbufDrawText( draw_buf_t * buf, int x, int y, const lvfont_handle pfont, const lChar32 * text, int len, lChar32 def_char );


#endif
