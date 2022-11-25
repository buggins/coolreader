/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2019,2021 NiLuJe <ninuje@gmail.com>                     *
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

#ifndef __LVCOLORTRANSFORMIMGSOURCE_H_INCLUDED__
#define __LVCOLORTRANSFORMIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"

class LVColorDrawBuf;

class LVColorTransformImgSource : public LVImageSource, public LVImageDecoderCallback
{
protected:
    LVImageSourceRef _src;
    lUInt32 _add;
    lUInt32 _multiply;
    LVImageDecoderCallback * _callback;
    LVColorDrawBuf * _drawbuf;
    int _sumR;
    int _sumG;
    int _sumB;
    int _countPixels;
public:
    LVColorTransformImgSource(LVImageSourceRef src, lUInt32 addRGB, lUInt32 multiplyRGB);
    virtual ~LVColorTransformImgSource();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource * obj, bool res);
    virtual ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _src->GetWidth(); }
    virtual int    GetHeight() const { return _src->GetHeight(); }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVCOLORTRANSFORMIMGSOURCE_H_INCLUDED__
