/** \file lvfontboldtransform.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_FONTBOLDTRANSFORM_H_INCLUDED__
#define __LV_FONTBOLDTRANSFORM_H_INCLUDED__

#include "../../include/lvfont.h"
#include "lvfontglyphcache.h"
#include "../../include/lvtextfm.h"

class LVFontBoldTransform : public LVFont {
    LVFontRef _baseFontRef;
    LVFont *_baseFont;
    int _hyphWidth;
    int _hShift;
    int _vShift;
    int _size;   // glyph height in pixels
    int _height; // line height in pixels
    //int           _hyphen_width;
    int _baseline;
    LVFontLocalGlyphCache _glyph_cache;
public:
    /// returns font weight
    virtual int getWeight() const;

    /// returns italic flag
    virtual int getItalic() const {
        return _baseFont->getItalic();
    }

    LVFontBoldTransform(LVFontRef baseFont, LVFontGlobalGlyphCache *globalCache);

    /// hyphenation character
    virtual lChar16 getHyphChar() { return UNICODE_SOFT_HYPHEN_CODE; }

    /// hyphen width
    virtual int getHyphenWidth();

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo(lUInt32 code, glyph_info_t *glyph, lChar16 def_char = 0, lUInt32 fallbackPassMask = 0);

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
            TextLangCfg * lang_cfg = NULL,
            int letter_spacing = 0,
            bool allow_hyphenation = true,
            lUInt32 hints=0,
            lUInt32 fallbackPassMask = 0
    );

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
            const lChar16 *text, int len, TextLangCfg * lang_cfg = NULL
    );

    /** \brief get glyph item
        \param code is unicode character
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar16 def_char = 0, lUInt32 fallbackPassMask = 0);

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found
    */
    //virtual bool getGlyphImage(lUInt16 code, lUInt8 * buf, lChar16 def_char=0 );

    /// returns font baseline offset
    virtual int getBaseline() {
        return _baseline;
    }

    /// returns font height
    virtual int getHeight() const {
        return _height;
    }

    /// returns font character size
    virtual int getSize() const {
        return _size;
    }

    /// returns char width
    virtual int getCharWidth(lChar16 ch, lChar16 def_char = 0) {
        int w = _baseFont->getCharWidth(ch, def_char) + _hShift;
        return w;
    }

    /// returns char glyph left side bearing
    virtual int getLeftSideBearing( lChar16 ch, bool negative_only=false, bool italic_only=false ) {
        return _baseFont->getLeftSideBearing( ch, negative_only, italic_only );
    }

    /// returns char glyph right side bearing
    virtual int getRightSideBearing( lChar16 ch, bool negative_only=false, bool italic_only=false ) {
        return _baseFont->getRightSideBearing( ch, negative_only, italic_only );
    }

    /// retrieves font handle
    virtual void *GetHandle() {
        return NULL;
    }

    /// returns font typeface name
    virtual lString8 getTypeFace() const {
        return _baseFont->getTypeFace();
    }

    /// returns font family id
    virtual css_font_family_t getFontFamily() const {
        return _baseFont->getFontFamily();
    }

    /// draws text string
    virtual int DrawTextString(LVDrawBuf *buf, int x, int y,
                               const lChar16 *text, int len,
                               lChar16 def_char, lUInt32 *palette = NULL,
                               bool addHyphen = false, TextLangCfg * lang_cfg = NULL,
                               lUInt32 flags = 0, int letter_spacing = 0,
                               int width = -1, int text_decoration_back_gap = 0,
                               lUInt32 fallbackPassMask = 0);

    /// get bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual bool getBitmapMode() {
        return _baseFont->getBitmapMode();
    }

    /// set bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual void setBitmapMode(bool m) {
        _baseFont->setBitmapMode(m);
    }

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t mode) { _baseFont->setHintingMode(mode); }

    /// returns current hinting mode
    virtual hinting_mode_t getHintingMode() const { return _baseFont->getHintingMode(); }

    /// get kerning mode: true==ON, false=OFF
    virtual bool getKerning() const { return _baseFont->getKerning(); }

    /// get kerning mode: true==ON, false=OFF
    virtual void setKerning(bool b) { _baseFont->setKerning(b); }

    /// get shaping mode
    virtual shaping_mode_t getShapingMode() const { return _baseFont->getShapingMode(); }

    /// get shaping mode
    virtual void setShapingMode( shaping_mode_t mode ) { _baseFont->setShapingMode( mode ); }

    /// clear cache
    virtual void clearCache() { _baseFont->clearCache(); }

    /// returns true if font is empty
    virtual bool IsNull() const {
        return _baseFont->IsNull();
    }

    virtual bool operator!() const {
        return !(*_baseFont);
    }

    virtual void Clear() {
        _baseFont->Clear();
    }

    virtual void setFallbackIndex(int index) {
        LVFont::setFallbackIndex(index);
        _baseFont->setFallbackIndex(index);
    }

    virtual ~LVFontBoldTransform() {
    }
};

#endif // __LV_FONTBOLDTRANSFORM_H_INCLUDED__
