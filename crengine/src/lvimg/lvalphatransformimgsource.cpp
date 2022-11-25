/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013,2014 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#include "lvalphatransformimgsource.h"

LVAlphaTransformImgSource::LVAlphaTransformImgSource(LVImageSourceRef src, int alpha)
    : _src( src )
    , _alpha(alpha ^ 0xFF)
{
}

LVAlphaTransformImgSource::~LVAlphaTransformImgSource() {
}

void LVAlphaTransformImgSource::OnStartDecode(LVImageSource *)
{
    _callback->OnStartDecode(this);
}

bool LVAlphaTransformImgSource::OnLineDecoded(LVImageSource *obj, int y, lUInt32 *data) {
    CR_UNUSED(obj);
    int dx = _src->GetWidth();
    
    for (int x = 0; x < dx; x++) {
        lUInt32 cl = data[x];
        int srcalpha = (cl >> 24) ^ 0xFF;
        if (srcalpha > 0) {
            srcalpha = _alpha * srcalpha;
            cl = (cl & 0xFFFFFF) | (((_alpha * srcalpha) ^ 0xFF)<<24);
        }
        data[x] = cl;
    }
    return _callback->OnLineDecoded(obj, y, data);
}

void LVAlphaTransformImgSource::OnEndDecode(LVImageSource *obj, bool res)
{
    CR_UNUSED(obj);
    _callback->OnEndDecode(this, res);
}

bool LVAlphaTransformImgSource::Decode(LVImageDecoderCallback *callback)
{
    _callback = callback;
    return _src->Decode( this );
}
