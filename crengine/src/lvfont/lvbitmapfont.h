/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2013 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2009-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2019-2021 Aleksey Chernov <valexlin@gmail.com>          *
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

/**
 * \file lvbitmapfont.h
 * \brief bitmap font interface
 */

#ifndef __LV_BITMAPFONT_H_INCLUDED__
#define __LV_BITMAPFONT_H_INCLUDED__

#include <stdlib.h>
#include "crsetup.h"
#include "lvfnt.h"
#include "lvbasefont.h"
#include "lvfontcache.h"

#if (USE_FREETYPE != 1) && (USE_BITMAP_FONTS == 1)

/* C++ wrapper class */
class LBitmapFont : public LVBaseFont {
private:
    lvfont_handle m_font;
public:
    LBitmapFont() : m_font(NULL) {}

    virtual bool getGlyphInfo(lUInt32 code, LVFont::glyph_info_t *glyph, lChar32 def_char = 0, lUInt32 fallbackPassMask = 0);

    virtual bool getGlyphExtraMetric( glyph_extra_metric_t metric, lUInt32 code, int & value, bool scaled_to_px=true, lChar32 def_char=0, lUInt32 fallbackPassMask=0 ) {
        return false;
    }

    virtual lUInt16
    measureText(const lChar32 *text, int len, lUInt16 *widths, lUInt8 *flags, int max_width,
                lChar32 def_char, TextLangCfg * lang_cfg = NULL, int letter_spacing = 0, bool allow_hyphenation = true, lUInt32 hints=0, lUInt32 fallbackPassMask = 0);

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string 
    */
    virtual lUInt32 getTextWidth(
            const lChar32 *text, int len, TextLangCfg * lang_cfg = NULL
    );

    /// returns font baseline offset
    virtual int getBaseline();

    /// returns font height
    virtual int getHeight() const;

    /// returns font character size
    virtual int getSize() const;

    /// returns font weight
    virtual int getWeight() const;

    /// returns italic flag
    virtual int getItalic() const;

    //virtual bool getGlyphImage(lUInt32 code, lUInt8 *buf, lChar32 def_char = 0);

    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar32 def_char = 0, lUInt32 fallbackPassMask = 0);

    /// returns char width
    virtual int getCharWidth(lChar32 ch, lChar32 def_char = 0) {
        glyph_info_t glyph;
        if (getGlyphInfo(ch, &glyph, def_char))
            return glyph.width;
        return 0;
    }

    virtual int getExtraMetric(font_extra_metric_t metric, bool scaled_to_px=true) { return 0; }

    virtual bool hasOTMathSupport() const { return false; }

    virtual lvfont_handle GetHandle() { return m_font; }

    int LoadFromFile(const char *fname);

    // LVFont functions overrides
    virtual void Clear() {
        if (m_font) lvfontClose(m_font);
        m_font = NULL;
    }

    virtual bool IsNull() const { return m_font == NULL; }

    virtual bool operator!() const { return IsNull(); }

    virtual ~LBitmapFont() {
        Clear(); // NOLINT: Call to virtual function during destruction
    }
};

#endif  // (USE_FREETYPE!=1) && (USE_BITMAP_FONTS==1)

#endif // __LV_BITMAPFONT_H_INCLUDED__
