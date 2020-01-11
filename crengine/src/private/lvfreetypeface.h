/** \file lvfreetypeface.h
    \brief FreeType font interface

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_FREETYPEFACE_H_INCLUDED__
#define __LV_FREETYPEFACE_H_INCLUDED__


#include "../../include/crsetup.h"
#include "../../include/lvfont.h"
#include "../../include/lvthread.h"
#include "lvfontglyphcache.h"
#include "lvfontdef.h"
#include "lvfontcache.h"

// define to filter out all fonts except .ttf
//#define LOAD_TTF_FONTS_ONLY
// DEBUG ONLY
#if 0
#define USE_FREETYPE 1
#define USE_FONTCONFIG 1
//#define DEBUG_FONT_SYNTHESIS 1
//#define DEBUG_FONT_MAN 1
//#define DEBUG_FONT_MAN_LOG_FILE "/tmp/font_man.log"
#endif

#if (USE_FREETYPE == 1)

#include <ft2build.h>
#include FT_FREETYPE_H

#if USE_HARFBUZZ == 1

#include <hb.h>
#include <hb-ft.h>
#include "../../include/lvhashtable.h"

#endif

#define CACHED_UNSIGNED_METRIC_NOT_SET 0xFFFF
class LVFontGlyphUnsignedMetricCache
{
private:
    static const int COUNT = 360;
    lUInt16 * ptrs[COUNT]; //support up to 0X2CFFF=360*512-1
public:
    lUInt16 get( lChar16 ch )
    {
        FONT_GLYPH_CACHE_GUARD
        int inx = (ch>>9) & 0x1ff;
        if (inx >= COUNT) return CACHED_UNSIGNED_METRIC_NOT_SET;
        lUInt16 * ptr = ptrs[inx];
        if ( !ptr )
            return CACHED_UNSIGNED_METRIC_NOT_SET;
        return ptr[ch & 0x1FF ];
    }
    void put( lChar16 ch, lUInt16 m )
    {
        FONT_GLYPH_CACHE_GUARD
        int inx = (ch>>9) & 0x1ff;
        if (inx >= COUNT) return;
        lUInt16 * ptr = ptrs[inx];
        if ( !ptr ) {
            ptr = new lUInt16[512];
            ptrs[inx] = ptr;
            memset( ptr, CACHED_UNSIGNED_METRIC_NOT_SET, sizeof(lUInt16) * 512 );
        }
        ptr[ ch & 0x1FF ] = m;
    }
    void clear()
    {
        FONT_GLYPH_CACHE_GUARD
        for ( int i=0; i<360; i++ ) {
            if ( ptrs[i] )
                delete [] ptrs[i];
            ptrs[i] = NULL;
        }
    }
    LVFontGlyphUnsignedMetricCache()
    {
        memset( ptrs, 0, 360*sizeof(lUInt16*) );
    }
    ~LVFontGlyphUnsignedMetricCache()
    {
        clear();
    }
};

#define CACHED_SIGNED_METRIC_NOT_SET 0x7FFF
#define CACHED_SIGNED_METRIC_SHIFT 0x8000
class LVFontGlyphSignedMetricCache : public LVFontGlyphUnsignedMetricCache
{
public:
    lInt16 get( lChar16 ch )
    {
        return (lInt16) ( LVFontGlyphUnsignedMetricCache::get(ch) - CACHED_SIGNED_METRIC_SHIFT );
    }
    void put( lChar16 ch, lInt16 m )
    {
        LVFontGlyphUnsignedMetricCache::put(ch, (lUInt16)( m + CACHED_SIGNED_METRIC_SHIFT) );
    }
};

class LVFreeTypeFace : public LVFont {
protected:
    LVMutex &_mutex;
    lString8 _fileName;
    lString8 _faceName;
    css_font_family_t _fontFamily;
    FT_Library _library;
    FT_Face _face;
    FT_GlyphSlot _slot;
    FT_Matrix _matrix;                 /* transformation matrix */
    int _size; // caracter height in pixels
    int _height; // full line height in pixels
    int _hyphen_width;
    int _baseline;
    int _weight;
    int _italic;
    LVFontGlyphUnsignedMetricCache _wcache;
    LVFontGlyphSignedMetricCache _lsbcache; // glyph left side bearing cache
    LVFontGlyphSignedMetricCache _rsbcache; // glyph right side bearing cache
    LVFontLocalGlyphCache _glyph_cache;
    bool _drawMonochrome;
    hinting_mode_t _hintingMode;
    kerning_mode_t _kerningMode;
    bool _fallbackFontIsSet;
    LVFontRef _fallbackFont;
    bool           _embolden; // fake/synthetized bold
    FT_Pos         _embolden_half_strength; // for emboldening with Harfbuzz
#if USE_HARFBUZZ == 1
    hb_font_t *_hb_font;
    hb_buffer_t *_hb_buffer;
    //
    // For use with KERNING_MODE_HARFBUZZ:
    #define HARFBUZZ_FULL_FEATURES_NB 2
    hb_feature_t _hb_features[HARFBUZZ_FULL_FEATURES_NB];
    LVFontLocalGlyphCache _glyph_cache2;
    //
    // For use with KERNING_MODE_HARFBUZZ_LIGHT:
    #define HARFBUZZ_LIGHT_FEATURES_NB 22
    hb_buffer_t *_hb_light_buffer;
    hb_feature_t _hb_light_features[HARFBUZZ_LIGHT_FEATURES_NB];
    LVHashTable<struct LVCharTriplet, struct LVCharPosInfo> _width_cache2;
#endif
public:

    // fallback font support
    /// set fallback font for this font
    void setFallbackFont(LVFontRef font);

    /// get fallback font for this font
    LVFont *getFallbackFont();

    /// returns font weight
    virtual int getWeight() const { return _weight; }

    /// returns italic flag
    virtual int getItalic() const { return _italic; }

    /// sets face name
    virtual void setFaceName(lString8 face) { _faceName = face; }

    LVMutex &getMutex() { return _mutex; }

    FT_Library getLibrary() { return _library; }

    LVFreeTypeFace(LVMutex &mutex, FT_Library library, LVFontGlobalGlyphCache *globalCache);

    virtual ~LVFreeTypeFace();

    void clearCache();

    virtual int getHyphenWidth();

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t mode);

    /// returns current hinting mode
    virtual hinting_mode_t getHintingMode() const { return _hintingMode; }

    /// sets current kerning mode
    virtual void setKerningMode( kerning_mode_t kerningMode );

    /// returns current kerning mode
    virtual kerning_mode_t getKerningMode() const { return _kerningMode; }

    /// get bitmap mode (true=bitmap, false=antialiased)
    virtual bool getBitmapMode() { return _drawMonochrome; }

    /// set bitmap mode (true=bitmap, false=antialiased)
    virtual void setBitmapMode(bool drawBitmap);

    void setEmbolden();

    bool loadFromBuffer(LVByteArrayRef buf, int index, int size, css_font_family_t fontFamily,
                        bool monochrome, bool italicize);

    bool loadFromFile(const char *fname, int index, int size, css_font_family_t fontFamily,
                      bool monochrome, bool italicize);

#if USE_HARFBUZZ == 1

    lChar16 filterChar(lChar16 code, lChar16 def_char=0);

    bool hbCalcCharWidth(struct LVCharPosInfo *posInfo, const struct LVCharTriplet &triplet,
                         lChar16 def_char);

#endif  // USE_HARFBUZZ==1

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo(lUInt32 code, glyph_info_t *glyph, lChar16 def_char = 0);
/*
  // USE GET_CHAR_FLAGS instead
    inline int calcCharFlags( lChar16 ch )
    {
        switch ( ch ) {
        case 0x0020:
            return LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER;
        case UNICODE_SOFT_HYPHEN_CODE:
            return LCHAR_ALLOW_WRAP_AFTER;
        case '-':
            return LCHAR_DEPRECATED_WRAP_AFTER;
        case '\r':
        case '\n':
            return LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER;
        default:
            return 0;
        }
    }
  */

    /**
     * @brief Check font for compatibility with language with langCode
     * @param langCode language code, for example, "en" - English, "ru" - Russian
     * @return true if font contains all glyphs for given language, false otherwise.
     */
    virtual bool checkFontLangCompat(const lString8 &langCode);

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
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
    );

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
            const lChar16 *text, int len
    );

    void updateTransform();

    /** \brief get glyph item
        \param code is unicode character
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar16 def_char = 0);

#if USE_HARFBUZZ == 1

    LVFontGlyphCacheItem *getGlyphByIndex(lUInt32 index);

#endif

//    /** \brief get glyph image in 1 byte per pixel format
//        \param code is unicode character
//        \param buf is buffer [width*height] to place glyph data
//        \return true if glyph was found
//    */
//    virtual bool getGlyphImage(lUInt16 ch, lUInt8 * bmp, lChar16 def_char=0)
//    {
//        LVFontGlyphCacheItem * item = getGlyph(ch);
//        if ( item )
//            memcpy( bmp, item->bmp, item->bmp_width * item->bmp_height );
//        return item;
//    }

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
    virtual int getCharWidth(lChar16 ch, lChar16 def_char = '?');

    /// returns char glyph left side bearing
    int getLeftSideBearing( lChar16 ch, bool negative_only=false, bool italic_only=false );

    /// returns char glyph right side bearing
    virtual int getRightSideBearing( lChar16 ch, bool negative_only=false, bool italic_only=false );

    /// retrieves font handle
    virtual void *GetHandle() {
        return NULL;
    }

    /// returns font typeface name
    virtual lString8 getTypeFace() const {
        return _faceName;
    }

    /// returns font family id
    virtual css_font_family_t getFontFamily() const {
        return _fontFamily;
    }

    virtual bool kerningEnabled() {
#if (ALLOW_KERNING==1)
    #if USE_HARFBUZZ==1
        return _kerningMode == KERNING_MODE_HARFBUZZ
            || (_kerningMode == KERNING_MODE_FREETYPE && FT_HAS_KERNING( _face ));
    #else
        return _kerningMode != KERNING_MODE_DISABLED && FT_HAS_KERNING( _face );
    #endif
#else
        return false;
#endif
    }

    /// draws text string
    virtual int DrawTextString(LVDrawBuf *buf, int x, int y,
                                const lChar16 *text, int len,
                                lChar16 def_char, lUInt32 *palette, bool addHyphen, lUInt32 flags,
                                int letter_spacing, int width=-1, int text_decoration_back_gap = 0);

    /// returns true if font is empty
    virtual bool IsNull() const {
        return _face == NULL;
    }

    virtual bool operator!() const {
        return _face == NULL;
    }

    virtual void Clear();
protected:
    FT_UInt getCharIndex(lUInt32 code, lChar16 def_char);
};

#endif  // (USE_FREETYPE==1)

#endif  // __LV_FREETYPEFACE_H_INCLUDED__
