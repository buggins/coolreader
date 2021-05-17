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
#define LFNT_DRAW_UNDERLINE              0x1000 /// underlined text
#define LFNT_DRAW_OVERLINE               0x2000 /// overlined text
#define LFNT_DRAW_LINE_THROUGH           0x4000 /// striked through text
#define LFNT_DRAW_DECORATION_MASK        0x7000

// CSS font-variant and font-feature-settings properties:
//   https://drafts.csswg.org/css-fonts-3/#propdef-font-variant
//   https://developer.mozilla.org/en-US/docs/Web/CSS/font-variant
// OpenType feature tags (to be provided to HarfBuzz)
//   https://en.wikipedia.org/wiki/List_of_typographic_features
//   https://docs.microsoft.com/en-us/typography/opentype/spec/featurelist
// See https://github.com/koreader/koreader/issues/5821#issuecomment-596243758
// Random notes:
// - common-ligatures : 'liga' + 'clig'  (the keyword 'normal' activates these ligatures)
//   and 'rlig' ("required ligatures, e.g. for arabic) are enabled by default by Harfbuzz
// - discretionary-ligatures : 'dlig'   (type designer choices) is not enabled by default
// - diagonal-fractions : 'frac' (enabling also 'numr' + 'dnom' seems not needed,
//   numr or dnom standalone just make the / more oblique)
// - stacked-fractions: "/" becomes horizontal
// - jis90 'jp90' is said to be the default in fonts. Other jp* replace this one.
// - Not supported (because no room and because they require some args, which
//   would complicate parsing and storing):
//   CSS font-variant-alternates:
//       stylistic()
//       styleset()
//       character-variant()
//       swash()
//       ornaments()
//       annotation()

// OpenType features to request
// Max 31 bits. We need "signed int features" to have -1 for non-instantiated fonts)
#define LFNT_OT_FEATURES_NORMAL 0x00000000

// #define LFNT_OT_FEATURES_P_LIGA 0x000000XX // +liga +clig enabled by default  (font-variant-ligatures: common-ligatures)
// #define LFNT_OT_FEATURES_P_CALT 0x000000XX // +calt       enabled by default  (font-variant-ligatures: contextual)
#define LFNT_OT_FEATURES_M_LIGA 0x00000001 // -liga -clig   (font-variant-ligatures: no-common-ligatures)
#define LFNT_OT_FEATURES_M_CALT 0x00000002 // -calt         (font-variant-ligatures: no-contextual)
#define LFNT_OT_FEATURES_P_DLIG 0x00000004 // +dlig         (font-variant-ligatures: discretionary-ligatures)
#define LFNT_OT_FEATURES_M_DLIG 0x00000008 // -dlig         (font-variant-ligatures: no-discretionary-ligatures)
#define LFNT_OT_FEATURES_P_HLIG 0x00000010 // +hlig         (font-variant-ligatures: historical-ligatures)
#define LFNT_OT_FEATURES_M_HLIG 0x00000020 // -hlig         (font-variant-ligatures: no-historical-ligatures)

#define LFNT_OT_FEATURES_P_HIST 0x00000040 // +hist         (font-variant-alternates: historical-forms)
#define LFNT_OT_FEATURES_P_RUBY 0x00000080 // +ruby         (font-variant-east-asian: ruby)

#define LFNT_OT_FEATURES_P_SMCP 0x00000100 // +smcp         (font-variant-caps: small-caps)
#define LFNT_OT_FEATURES_P_C2SC 0x00000200 // +c2sc +smcp   (font-variant-caps: all-small-caps)
#define LFNT_OT_FEATURES_P_PCAP 0x00000400 // +pcap         (font-variant-caps: petite-caps)
#define LFNT_OT_FEATURES_P_C2PC 0x00000800 // +c2pc +pcap   (font-variant-caps: all-petite-caps)
#define LFNT_OT_FEATURES_P_UNIC 0x00001000 // +unic         (font-variant-caps: unicase)
#define LFNT_OT_FEATURES_P_TITL 0x00002000 // +titl         (font-variant-caps: titling-caps)
#define LFNT_OT_FEATURES_P_SUPS 0x00004000 // +sups         (font-variant-position: super)
#define LFNT_OT_FEATURES_P_SUBS 0x00008000 // +subs         (font-variant-position: sub)

#define LFNT_OT_FEATURES_P_LNUM 0x00010000 // +lnum         (font-variant-numeric: lining-nums)
#define LFNT_OT_FEATURES_P_ONUM 0x00020000 // +onum         (font-variant-numeric: oldstyle-nums)
#define LFNT_OT_FEATURES_P_PNUM 0x00040000 // +pnum         (font-variant-numeric: proportional-nums)
#define LFNT_OT_FEATURES_P_TNUM 0x00080000 // +tnum         (font-variant-numeric: tabular-nums)
#define LFNT_OT_FEATURES_P_ZERO 0x00100000 // +zero         (font-variant-numeric: slashed-zero)
#define LFNT_OT_FEATURES_P_ORDN 0x00200000 // +ordn         (font-variant-numeric: ordinal)
#define LFNT_OT_FEATURES_P_FRAC 0x00400000 // +frac         (font-variant-numeric: diagonal-fractions)
#define LFNT_OT_FEATURES_P_AFRC 0x00800000 // +afrc         (font-variant-numeric: stacked-fractions)

#define LFNT_OT_FEATURES_P_SMPL 0x01000000 // +smpl         (font-variant-east-asian: simplified)
#define LFNT_OT_FEATURES_P_TRAD 0x02000000 // +trad         (font-variant-east-asian: traditional)
#define LFNT_OT_FEATURES_P_FWID 0x04000000 // +fwid         (font-variant-east-asian: full-width)
#define LFNT_OT_FEATURES_P_PWID 0x08000000 // +pwid         (font-variant-east-asian: proportional-width)
#define LFNT_OT_FEATURES_P_JP78 0x10000000 // +jp78         (font-variant-east-asian: jis78)
#define LFNT_OT_FEATURES_P_JP83 0x20000000 // +jp83         (font-variant-east-asian: jis83)
#define LFNT_OT_FEATURES_P_JP04 0x40000000 // +jp04         (font-variant-east-asian: jis04)
// No more room for: (let's hope it's really the default in fonts)
// #define LFNT_OT_FEATURES_P_JP90 0x80000000 // +jp90      (font-variant-east-asian: jis90)

// Changing this enum also update method LVDocView::rotateFontAntialiasMode()
// & org.coolreader.crengine.OptionsDialog.mAntialias array.
enum font_antialiasing_t {
    font_aa_none = 0,
    font_aa_big,
    font_aa_all,
    font_aa_gray,
    font_aa_lcd_rgb,
    font_aa_lcd_bgr,
    font_aa_lcd_pentile,
    font_aa_lcd_pentile_m,  // mirrorred Pentile
    font_aa_lcd_v_rgb,
    font_aa_lcd_v_bgr,
    font_aa_lcd_v_pentile,
    font_aa_lcd_v_pentile_m
};

struct LVFontGlyphCacheItem;
class TextLangCfg;

class LVFont;
typedef LVProtectedFastRef<LVFont> LVFontRef;

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
    virtual lChar32 getHyphChar() { return UNICODE_SOFT_HYPHEN_CODE; }

    /// hyphen width
    virtual int getHyphenWidth() { return getCharWidth(getHyphChar()); }

    /**
     * Max width of -/./,/!/? to use for visial alignment by width
     */
    virtual int getVisualAligmentWidth();

    /** \brief get glyph info
        \param code is unicode character code
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \param def_char replacement char if glyph for code not found for this font
        \param fallbackPassMask bitmask of processed fallback fonts
        \return true if glyph was found 
    */
    virtual bool getGlyphInfo(lUInt32 code, glyph_info_t *glyph, lChar32 def_char = 0, lUInt32 fallbackPassMask = 0) = 0;

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \param max_width is maximum width to measure line 
        \param def_char is character to replace absent glyphs in font
        \param letter_spacing is number of pixels to add between letters
        \param hints: hint flags (direction, begin/end of paragraph, for Harfbuzz - unrelated to font hinting)
        \param fallbackPassMask bitmask of processed fallback fonts
        \return number of characters before max_width reached 
    */
    virtual lUInt16 measureText(
            const lChar32 *text, int len,
            lUInt16 *widths,
            lUInt8 *flags,
            int max_width,
            lChar32 def_char,
            TextLangCfg * lang_cfg = NULL,
            int letter_spacing = 0,
            bool allow_hyphenation = true,
            lUInt32 hints = 0,
            lUInt32 fallbackPassMask = 0
    ) = 0;

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string 
    */
    virtual lUInt32 getTextWidth(
            const lChar32 *text, int len, TextLangCfg * lang_cfg = NULL
    ) = 0;

//    /** \brief get glyph image in 1 byte per pixel format
//        \param code is unicode character
//        \param buf is buffer [width*height] to place glyph data
//        \return true if glyph was found
//    */
//    virtual bool getGlyphImage(lUInt32 code, lUInt8 * buf, lChar32 def_char=0) = 0;
    /** \brief get glyph item
        \param ch is unicode character code
        \param def_char replacement char if glyph for ch not found for this font
        \param fallbackPassMask bitmask of processed fallback fonts
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar32 def_char = 0, lUInt32 fallbackPassMask = 0) = 0;

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
    virtual int getCharWidth( lChar32 ch, lChar32 def_char=0 ) = 0;

    /// returns char glyph left side bearing
    virtual int getLeftSideBearing( lChar32 ch, bool negative_only=false, bool italic_only=false ) = 0;

    /// returns char glyph right side bearing
    virtual int getRightSideBearing( lChar32 ch, bool negative_only=false, bool italic_only=false ) = 0;

    /// retrieves font handle
    virtual void *GetHandle() = 0;

    /// returns font typeface name
    virtual lString8 getTypeFace() const = 0;

    /// returns font family id
    virtual css_font_family_t getFontFamily() const = 0;

    /// draws text string (returns x advance)
    virtual int DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar32 * text, int len,
                       lChar32 def_char, lUInt32 * palette = NULL,
                       bool addHyphen = false, TextLangCfg * lang_cfg = NULL,
                       lUInt32 flags=0, int letter_spacing=0, int width=-1,
                       int text_decoration_back_gap=0,
                       lUInt32 fallbackPassMask = 0) = 0;

    /// constructor
    LVFont() : _visual_alignment_width(-1), _hash(0) {}

    /// get bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual bool getBitmapMode() { return false; }

    /// set bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual void setBitmapMode(bool) {}

    /// get antialiasing mode
    virtual font_antialiasing_t GetAntialiasMode() { return font_aa_gray; }

    /// set antialiasing mode
    virtual void SetAntialiasMode(font_antialiasing_t mode) {}

    /// get OpenType features (bitmap)
    virtual int getFeatures() const { return 0; }

    /// set OpenType features (bitmap)
    virtual void setFeatures( int features ) { }

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

    virtual int getKerningOffset(lChar32 ch1, lChar32 ch2, lChar32 def_char) {
        CR_UNUSED3(ch1, ch2, def_char);
        return 0;
    }

    virtual bool checkFontLangCompat(const lString8 &langCode) { return true; }

    /// set fallback font for this font
    virtual void setFallbackFont(LVFontRef font) { CR_UNUSED(font); }

    /** \brief get fallback font for this font
     * \param bitmask of processed fallback fonts
     */
    virtual LVFont *getFallbackFont(lUInt32 fallbackPassMask) {
        CR_UNUSED(fallbackPassMask)
        return NULL;
    }
    
    virtual lUInt32 getFallbackMask() const { return 0; }
    virtual void setFallbackMask(lUInt32 mask) { CR_UNUSED(mask) }
};

/// to compare two fonts
bool operator==(const LVFont &r1, const LVFont &r2);

#endif //__LV_FONT_H_INCLUDED__
