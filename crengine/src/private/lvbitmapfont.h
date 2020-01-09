/** @file lvbitmapfont.h
    @brief bitmap font interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

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

    virtual bool getGlyphInfo(lUInt32 code, LVFont::glyph_info_t *glyph, lChar16 def_char = 0);

    virtual lUInt16
    measureText(const lChar16 *text, int len, lUInt16 *widths, lUInt8 *flags, int max_width,
                lChar16 def_char, int letter_spacing = 0, bool allow_hyphenation = true);

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string 
    */
    virtual lUInt32 getTextWidth(
            const lChar16 *text, int len
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

    //virtual bool getGlyphImage(lUInt32 code, lUInt8 *buf, lChar16 def_char = 0);

    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar16 def_char = 0);

    /// returns char width
    virtual int getCharWidth(lChar16 ch, lChar16 def_char = 0) {
        glyph_info_t glyph;
        if (getGlyphInfo(ch, &glyph, def_char))
            return glyph.width;
        return 0;
    }

    virtual lvfont_handle GetHandle() { return m_font; }

    int LoadFromFile(const char *fname);

    // LVFont functions overrides
    virtual void Clear() {
        if (m_font) lvfontClose(m_font);
        m_font = NULL;
    }

    virtual bool IsNull() const { return m_font == NULL; }

    virtual bool operator!() const { return IsNull(); }

    virtual ~LBitmapFont() { Clear(); }
};

#endif  // (USE_FREETYPE!=1) && (USE_BITMAP_FONTS==1)

#endif // __LV_BITMAPFONT_H_INCLUDED__
