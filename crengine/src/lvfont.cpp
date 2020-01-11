/** @file lvfntman.cpp
    @brief font implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "../include/lvfont.h"

/**
 * Max width of -/./,/!/? to use for visial alignment by width
 */
int LVFont::getVisualAligmentWidth() {
    FONT_GUARD
    if (_visual_alignment_width == -1) {
        //lChar16 chars[] = { getHyphChar(), ',', '.', '!', ':', ';', 0 };
        lChar16 chars[] = {getHyphChar(), ',', '.', '!', ':', ';',
                           (lChar16) L'\xff0c', (lChar16) L'\x3302', (lChar16) L'\xff01', 0};
        //                  (lChar16)L'，', (lChar16)L'。', (lChar16)L'！', 0 };
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
           && r1.getHintingMode() == r2.getHintingMode();
}
