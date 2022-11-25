/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2008 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __LVGIFFRAME_H_INCLUDED__
#define __LVGIFFRAME_H_INCLUDED__

#include "crsetup.h"

#if (USE_GIF==1)

#include "lvtypes.h"

class LVGifImageSource;
class LVImageDecoderCallback;

class LVGifFrame
{
protected:
    int        m_cx;
    int        m_cy;
    int m_left;
    int m_top;
    unsigned char m_bpp;     // bits per pixel
    unsigned char m_flg_ltc; // GTC (gobal table of colors) flag
    unsigned char m_flg_interlaced; // interlace flag

    LVGifImageSource * m_pImage;
    lUInt32 *    m_local_color_table;

    unsigned char * m_buffer;
public:
    int DecodeFromBuffer( unsigned char * buf, int buf_size, int &bytes_read );
    LVGifFrame(LVGifImageSource * pImage);
    ~LVGifFrame();
    void Clear();
    lUInt32 * GetColorTable();
    void Draw( LVImageDecoderCallback * callback );
};

#endif  // (USE_GIF==1)

#endif  // __LVGIFFRAME_H_INCLUDED__
