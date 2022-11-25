/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __CRNINEPATCHDECODER_H_INCLUDED__
#define __CRNINEPATCHDECODER_H_INCLUDED__

#include "lvimagedecodercallback.h"
#include "lvimagesource.h"

class CRNinePatchDecoder : public LVImageDecoderCallback {
    int _dx;
    int _dy;
    CR9PatchInfo *_info;
public:
    CRNinePatchDecoder(int dx, int dy, CR9PatchInfo *info) : _dx(dx), _dy(dy), _info(info) {
    }
    virtual ~CRNinePatchDecoder() {}
    virtual void OnStartDecode(LVImageSource *obj) {
        CR_UNUSED(obj);
    }
    bool isUsedPixel(lUInt32 pixel);
    void decodeHLine(lUInt32 *line, int &x0, int &x1);
    void decodeVLine(lUInt32 pixel, int y, int &y0, int &y1);
    virtual bool OnLineDecoded(LVImageSource *obj, int y, lUInt32 *data);
    virtual void OnEndDecode(LVImageSource *obj, bool errors) {
        CR_UNUSED2(obj, errors);
    }
};

#endif  // __CRNINEPATCHDECODER_H_INCLUDED__
