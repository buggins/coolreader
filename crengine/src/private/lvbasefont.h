/** @file lvbasefont.h
    @brief base font interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_BASEFONT_H_INCLUDED__
#define __LV_BASEFONT_H_INCLUDED__

#include <stdlib.h>
#include "crsetup.h"
#include "lvfont.h"

class LVDrawBuf;

class LVBaseFont : public LVFont {
protected:
    lString8 _typeface;
    css_font_family_t _family;
public:
    /// returns font typeface name
    virtual lString8 getTypeFace() const { return _typeface; }

    /// returns font family id
    virtual css_font_family_t getFontFamily() const { return _family; }

    /// draws text string
    virtual int DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar16 * text, int len,
                       lChar16 def_char, lUInt32 * palette, bool addHyphen,
                       lUInt32 flags=0, int letter_spacing=0, int width=-1,
                       int text_decoration_back_gap=0 );
};

#endif  // __LV_BASEFONT_H_INCLUDED__
