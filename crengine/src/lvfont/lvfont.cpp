/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2008,2010,2011,2013,2015 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2020,2021 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "lvfont.h"

/**
 * Max width of -/./,/!/? to use for visial alignment by width
 */
int LVFont::getVisualAligmentWidth() {
    FONT_GUARD
    if (_visual_alignment_width == -1) {
        //lChar32 chars[] = { getHyphChar(), ',', '.', '!', ':', ';', 0 };
        lChar32 chars[] = {getHyphChar(), ',', '.', '!', ':', ';',
                           (lChar32) U'\xff0c', (lChar32) U'\x3302', (lChar32) U'\xff01', 0};
        //                  (lChar32)U'，', (lChar32)U'。', (lChar32)U'！', 0 };
        //                  65292 12290 65281
        //                  ff0c 3002 ff01
        int maxw = 0;
        for (int i = 0; chars[i]; i++) {
            int w = getCharWidth(chars[i]);
            if (w > maxw)
                maxw = w;
        }
        _visual_alignment_width = maxw;
    }
    return _visual_alignment_width;
}

/// to compare two fonts
bool operator==(const LVFont &r1, const LVFont &r2) {
    if (&r1 == &r2)
        return true;
    return r1.getSize() == r2.getSize()
           && r1.getWeight() == r2.getWeight()
           && r1.getItalic() == r2.getItalic()
           && r1.getFontFamily() == r2.getFontFamily()
           && r1.getTypeFace() == r2.getTypeFace()
           && r1.getShapingMode() == r2.getShapingMode()
           && r1.getKerning() == r2.getKerning()
           && r1.getHintingMode() == r2.getHintingMode();
}
