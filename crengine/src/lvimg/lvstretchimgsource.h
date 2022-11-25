/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2008-2010 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2021 NiLuJe <ninuje@gmail.com>                          *
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

#ifndef __LVSTRETCHIMGSOURCE_H_INCLUDED__
#define __LVSTRETCHIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"
#include "lvarray.h"
#include "lvimg.h"

class LVStretchImgSource : public LVImageSource, public LVImageDecoderCallback
{
protected:
    LVImageSourceRef _src;
    int _src_dx;
    int _src_dy;
    int _dst_dx;
    int _dst_dy;
    ImageTransform _hTransform;
    ImageTransform _vTransform;
    int _split_x;
    int _split_y;
    LVArray<lUInt32> _line;
    LVImageDecoderCallback * _callback;
public:
    LVStretchImgSource( LVImageSourceRef src, int newWidth, int newHeight, ImageTransform hTransform, ImageTransform vTransform, int splitX, int splitY );
    virtual ~LVStretchImgSource();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource *, bool res);
    virtual ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _dst_dx; }
    virtual int    GetHeight() const { return _dst_dy; }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVSTRETCHIMGSOURCE_H_INCLUDED__
