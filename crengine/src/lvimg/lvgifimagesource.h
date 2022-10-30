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

#ifndef __LVGIFIMAGESOURCE_H_INCLUDED__
#define __LVGIFIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_GIF==1)

#include "lvnodeimagesource.h"

class LVGifFrame;

class LVGifImageSource : public LVNodeImageSource
{
    friend class LVGifFrame;
protected:
    LVGifFrame ** m_frames;
    int m_frame_count;
    unsigned char m_version;
    unsigned char m_bpp;     //
    unsigned char m_flg_gtc; // GTC (gobal table of colors) flag
    unsigned char m_transparent_color; // index
    unsigned char m_background_color;
    lUInt32 * m_global_color_table;
    bool defined_transparent_color;
public:
    LVGifImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVGifImageSource();

    static bool CheckPattern( const lUInt8 * buf, int );
    virtual void   Compact();
    virtual bool Decode( LVImageDecoderCallback * callback );

    int DecodeFromBuffer(unsigned char *buf, int buf_size, LVImageDecoderCallback * callback);
    //int LoadFromFile( const char * fname );
    void Clear();
    lUInt32 * GetColorTable();
};

#endif  // (USE_GIF==1)

#endif  // __LVGIFIMAGESOURCE_H_INCLUDED__
