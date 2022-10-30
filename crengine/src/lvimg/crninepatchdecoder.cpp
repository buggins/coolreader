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

#include "crninepatchdecoder.h"

bool CRNinePatchDecoder::isUsedPixel(lUInt32 pixel) {
    return (pixel == 0x000000); // black
}

void CRNinePatchDecoder::decodeHLine(lUInt32 *line, int &x0, int &x1) {
    bool foundUsed = false;
    for (int x = 0; x < _dx; x++) {
        if (isUsedPixel(line[x])) {
            if (!foundUsed) {
                x0 = x;
                foundUsed = true;
            }
            x1 = x + 1;
        }
    }
}

void CRNinePatchDecoder::decodeVLine(lUInt32 pixel, int y, int &y0, int &y1) {
    if (isUsedPixel(pixel)) {
        if (y0 == 0)
            y0 = y;
        y1 = y + 1;
    }
}

bool CRNinePatchDecoder::OnLineDecoded(LVImageSource *obj, int y, lUInt32 *data) {
    CR_UNUSED(obj);
    if (y == 0) {
        decodeHLine(data, _info->frame.left, _info->frame.right);
    } else if (y == _dy - 1) {
        decodeHLine(data, _info->padding.left, _info->padding.right);
    } else {
        decodeVLine(data[0], y, _info->frame.top, _info->frame.bottom);
        decodeVLine(data[_dx - 1], y, _info->padding.top, _info->padding.bottom);
    }
    return true;
}
