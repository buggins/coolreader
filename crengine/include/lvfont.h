/** @file lvfont.h
    @brief font interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_FONT_H_INCLUDED__
#define __LV_FONT_H_INCLUDED__

#include "crsetup.h"
#include "lvstring.h"
#include "lvref.h"
#include "lvdrawbuf.h"
#include "cssdef.h"

#define MAX_LINE_CHARS 2048

enum hinting_mode_t {
    HINTING_MODE_DISABLED,
    HINTING_MODE_BYTECODE_INTERPRETOR,
    HINTING_MODE_AUTOHINT
};

enum font_antialiasing_t {
    font_aa_none,
    font_aa_big,
    font_aa_all
};

class LVFontGlyphCacheItem;

/** \brief base class for fonts

    implements single interface for font of any engine
*/
class LVFont : public LVRefCounter {
protected:
    int _visual_alignment_width;
public:
    lUInt32 _hash;
    /// glyph properties structure
    struct glyph_info_t {
        lUInt8 blackBoxX;   ///< 0: width of glyph
        lUInt8 blackBoxY;   ///< 1: height of glyph black box
        lInt8 originX;     ///< 2: X origin for glyph
        lInt8 originY;     ///< 3: Y origin for glyph
        lUInt8 width;       ///< 4: full width of glyph
    };

    /// hyphenation character
    virtual lChar16 getHyphChar() { return UNICODE_SOFT_HYPHEN_CODE; }

    /// hyphen width
    virtual int getHyphenWidth() { return getCharWidth(getHyphChar()); }

    /**
     * Max width of -/./,/!/? to use for visial alignment by width
     */
    virtual int getVisualAligmentWidth();

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found 
    */
    virtual bool getGlyphInfo(lUInt16 code, glyph_info_t *glyph, lChar16 def_char = 0) = 0;

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \param max_width is maximum width to measure line 
        \param def_char is character to replace absent glyphs in font
        \param letter_spacing is number of pixels to add between letters
        \return number of characters before max_width reached 
    */
    virtual lUInt16 measureText(
            const lChar16 *text, int len,
            lUInt16 *widths,
            lUInt8 *flags,
            int max_width,
            lChar16 def_char,
            int letter_spacing = 0,
            bool allow_hyphenation = true
    ) = 0;

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string 
    */
    virtual lUInt32 getTextWidth(
            const lChar16 *text, int len
    ) = 0;

//    /** \brief get glyph image in 1 byte per pixel format
//        \param code is unicode character
//        \param buf is buffer [width*height] to place glyph data
//        \return true if glyph was found
//    */
//    virtual bool getGlyphImage(lUInt16 code, lUInt8 * buf, lChar16 def_char=0) = 0;
    /** \brief get glyph item
        \param code is unicode character
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem *getGlyph(lUInt16 ch, lChar16 def_char = 0) = 0;

    /// returns font baseline offset
    virtual int getBaseline() = 0;

    /// returns font height including normal interline space
    virtual int getHeight() const = 0;

    /// returns font character size
    virtual int getSize() const = 0;

    /// returns font weight
    virtual int getWeight() const = 0;

    /// returns italic flag
    virtual int getItalic() const = 0;

    /// returns char width
    virtual int getCharWidth(lChar16 ch, lChar16 def_char = 0) = 0;

    /// retrieves font handle
    virtual void *GetHandle() = 0;

    /// returns font typeface name
    virtual lString8 getTypeFace() const = 0;

    /// returns font family id
    virtual css_font_family_t getFontFamily() const = 0;

    /// draws text string
    virtual void DrawTextString(LVDrawBuf *buf, int x, int y,
                                const lChar16 *text, int len,
                                lChar16 def_char, lUInt32 *palette = NULL, bool addHyphen = false,
                                lUInt32 flags = 0, int letter_spacing = 0) = 0;

    /// constructor
    LVFont() : _visual_alignment_width(-1), _hash(0) {}

    /// get bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual bool getBitmapMode() { return false; }

    /// set bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual void setBitmapMode(bool) {}

    /// get kerning mode: true==ON, false=OFF
    virtual bool getKerning() const { return false; }

    /// set kerning mode: true==ON, false=OFF
    virtual void setKerning(bool) {}

    /// get ligatures mode: true==allowed, false=not allowed
    virtual bool getLigatures() const { return false; }

    /// set ligatures mode: true==allowed, false=not allowed
    virtual void setLigatures(bool) {}

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t /*mode*/) {}

    /// returns current hinting mode
    virtual hinting_mode_t getHintingMode() const { return HINTING_MODE_AUTOHINT; }

    /// returns true if font is empty
    virtual bool IsNull() const = 0;

    virtual bool operator!() const = 0;

    virtual void Clear() = 0;

    virtual ~LVFont() {}

    virtual bool kerningEnabled() { return false; }

    virtual int getKerningOffset(lChar16 ch1, lChar16 ch2, lChar16 def_char) {
        CR_UNUSED3(ch1, ch2, def_char);
        return 0;
    }

    virtual bool checkFontLangCompat(const lString8 &langCode) { return true; }

    /// set fallback font for this font
    void setFallbackFont(LVProtectedFastRef<LVFont> font) { CR_UNUSED(font); }

    /// get fallback font for this font
    LVFont *getFallbackFont() { return NULL; }
};

typedef LVProtectedFastRef<LVFont> LVFontRef;

/// to compare two fonts
bool operator==(const LVFont &r1, const LVFont &r2);

#endif //__LV_FONT_H_INCLUDED__
