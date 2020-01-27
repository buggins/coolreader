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
#define MAX_LINE_WIDTH 2048
#define MAX_LETTER_SPACING MAX_LINE_WIDTH/2

enum hinting_mode_t {
    HINTING_MODE_DISABLED = 0,
    HINTING_MODE_BYTECODE_INTERPRETOR,
    HINTING_MODE_AUTOHINT
};

enum shaping_mode_t {
    SHAPING_MODE_FREETYPE = 0,
    SHAPING_MODE_HARFBUZZ_LIGHT,
    SHAPING_MODE_HARFBUZZ
};

// Hint flags for measuring and drawing (some used only with full Harfbuzz)
// These 4 translate (after mask & shift) from LTEXT_WORD_* equivalents
// (see lvtextfm.h). Keep them in sync.
#define LFNT_HINT_DIRECTION_KNOWN        0x0001 /// segment direction is known
#define LFNT_HINT_DIRECTION_IS_RTL       0x0002 /// segment direction is RTL
#define LFNT_HINT_BEGINS_PARAGRAPH       0x0004 /// segment is at start of paragraph
#define LFNT_HINT_ENDS_PARAGRAPH         0x0008 /// segment is at end of paragraph

// These 4 translate from LTEXT_TD_* equivalents (see lvtextfm.h). Keep them in sync.
#define LFNT_DRAW_UNDERLINE              0x0100 /// underlined text
#define LFNT_DRAW_OVERLINE               0x0200 /// overlined text
#define LFNT_DRAW_LINE_THROUGH           0x0400 /// striked through text
#define LFNT_DRAW_BLINK                  0x0800 /// blinking text (implemented as underline)
#define LFNT_DRAW_DECORATION_MASK        0x0F00

enum font_antialiasing_t {
    font_aa_none,
    font_aa_big,
    font_aa_all
};

struct LVFontGlyphCacheItem;

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
        lUInt16 blackBoxX;  ///< 0: width of glyph
        lUInt16 blackBoxY;  ///< 1: height of glyph black box
        lInt16  originX;    ///< 2: X origin for glyph (left side bearing)
        lInt16  originY;    ///< 3: Y origin for glyph
        lUInt16 width;      ///< 4: full advance width of glyph
        lInt16  rsb;        ///< 5: right side bearing
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
        \param code is unicode character code
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found 
    */
    virtual bool getGlyphInfo(lUInt32 code, glyph_info_t *glyph, lChar16 def_char = 0) = 0;

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \param max_width is maximum width to measure line 
        \param def_char is character to replace absent glyphs in font
        \param letter_spacing is number of pixels to add between letters
        \param hints: hint flags (direction, begin/end of paragraph, for Harfbuzz - unrelated to font hinting)
        \return number of characters before max_width reached 
    */
    virtual lUInt16 measureText(
            const lChar16 *text, int len,
            lUInt16 *widths,
            lUInt8 *flags,
            int max_width,
            lChar16 def_char,
            int letter_spacing = 0,
            bool allow_hyphenation = true,
            lUInt32 hints = 0
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
//    virtual bool getGlyphImage(lUInt32 code, lUInt8 * buf, lChar16 def_char=0) = 0;
    /** \brief get glyph item
        \param code is unicode character code
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar16 def_char = 0) = 0;

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

    /// returns char glyph advance width
    virtual int getCharWidth( lChar16 ch, lChar16 def_char=0 ) = 0;

    /// returns char glyph left side bearing
    virtual int getLeftSideBearing( lChar16 ch, bool negative_only=false, bool italic_only=false ) = 0;

    /// returns char glyph right side bearing
    virtual int getRightSideBearing( lChar16 ch, bool negative_only=false, bool italic_only=false ) = 0;

    /// retrieves font handle
    virtual void *GetHandle() = 0;

    /// returns font typeface name
    virtual lString8 getTypeFace() const = 0;

    /// returns font family id
    virtual css_font_family_t getFontFamily() const = 0;

    /// draws text string (returns x advance)
    virtual int DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar16 * text, int len,
                       lChar16 def_char, lUInt32 * palette = NULL, bool addHyphen = false,
                       lUInt32 flags=0, int letter_spacing=0, int width=-1,
                       int text_decoration_back_gap=0 ) = 0;

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

    /// sets current shaping mode
    virtual void setShapingMode( shaping_mode_t /*mode*/ ) { }

    /// returns current shaping mode
    virtual shaping_mode_t getShapingMode() const { return SHAPING_MODE_FREETYPE; }

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t /*mode*/) {}

    /// returns current hinting mode
    virtual hinting_mode_t getHintingMode() const { return HINTING_MODE_AUTOHINT; }

    /// clear cache
    virtual void clearCache() { }

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
