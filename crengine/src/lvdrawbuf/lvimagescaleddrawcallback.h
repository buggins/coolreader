/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2009,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2019 NiLuJe <ninuje@gmail.com>                          *
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

#ifndef __LVIMAGESCALEDDRAWCALLBACK_H_INCLUDED__
#define __LVIMAGESCALEDDRAWCALLBACK_H_INCLUDED__

#include "lvimagedecodercallback.h"
#include "lvbasedrawbuf.h"

class LVImageScaledDrawCallback : public LVImageDecoderCallback
{
private:
    LVImageSourceRef src;
    LVBaseDrawBuf * dst;
    int dst_x;
    int dst_y;
    int dst_dx;
    int dst_dy;
    int src_dx;
    int src_dy;
    int * xmap;
    int * ymap;
    bool dither;
    bool invert;
    bool smoothscale;
    lUInt8 * decoded;
    bool isNinePatch;
public:
    static int * GenMap( int src_len, int dst_len );
    static int * GenNinePatchMap( int src_len, int dst_len, int frame1, int frame2);
    LVImageScaledDrawCallback(LVBaseDrawBuf * dstbuf, LVImageSourceRef img, int x, int y, int width, int height, bool dith, bool inv, bool smooth );
    virtual ~LVImageScaledDrawCallback();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource *, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource * obj, bool );
};

#endif  // __LVIMAGESCALEDDRAWCALLBACK_H_INCLUDED__
