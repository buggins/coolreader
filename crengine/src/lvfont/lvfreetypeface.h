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
#include "lvarray.h"

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
#include "lvhashtable.h"

#endif

#define CACHED_UNSIGNED_METRIC_NOT_SET 0xFFFF
class LVFontGlyphUnsignedMetricCache
{
private:
    static const int COUNT = 360;
    lUInt16 * ptrs[COUNT]; //support up to 0X2CFFF=360*512-1
public:
    lUInt16 get( lChar32 ch )
    {
        FONT_GLYPH_CACHE_GUARD
        int inx = (ch>>9) & 0x1ff;
        if (inx >= COUNT) return CACHED_UNSIGNED_METRIC_NOT_SET;
        lUInt16 * ptr = ptrs[inx];
        if ( !ptr )
            return CACHED_UNSIGNED_METRIC_NOT_SET;
        return ptr[ch & 0x1FF ];
    }
    void put( lChar32 ch, lUInt16 m )
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
    lInt16 get( lChar32 ch )
    {
        return (lInt16) ( LVFontGlyphUnsignedMetricCache::get(ch) - CACHED_SIGNED_METRIC_SHIFT );
    }
    void put( lChar32 ch, lInt16 m )
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
    FT_Matrix _matrix; // helper matrix for fake italic metrics
    int _size; // caracter height in pixels
    int _height; // full line height in pixels
    int _hyphen_width;
    int _baseline;
    int _weight; // original font weight 400: normal, 700: bold, 100..900 thin..black
    int _italic; // 0: regular, 1: italic, 2: fake/synthesized italic
    LVArray<int> _extra_metrics;
    LVFontGlyphUnsignedMetricCache _wcache;
    LVFontGlyphSignedMetricCache _lsbcache; // glyph left side bearing cache
    LVFontGlyphSignedMetricCache _rsbcache; // glyph right side bearing cache
    LVFontLocalGlyphCache _glyph_cache;
    bool _drawMonochrome;
    font_antialiasing_t _aa_mode;
    hinting_mode_t _hintingMode;
    shaping_mode_t _shapingMode;
    bool _fallbackFontIsSet;
    LVFontRef _fallbackFont;
    /**
     * @brief Fallback mask for this fallback font.
     * 0 - for normal (not fallback) font
     * <any> - A mask with only one bit set, the number of which corresponds to the number in the fallback font chain.
     */
    lUInt32 _fallback_mask;
    int            _synth_weight; // fake/synthesized weight
    bool           _allowKerning;
    FT_Pos         _synth_weight_strength;   // for emboldening with FT_Outline_Embolden()
    FT_Pos         _synth_weight_half_strength;
    FT_Pos         _scale_mul;                  // only for fixed-size color fonts
    FT_Pos         _scale_div;                  // only for fixed-size color fonts
    int _features; // requested OpenType features bitmap
#if USE_HARFBUZZ == 1
    hb_font_t *_hb_font;
    hb_buffer_t *_hb_buffer;
    LVArray<hb_feature_t> _hb_features;
    // For use with SHAPING_MODE_HARFBUZZ:
    LVFontLocalGlyphCache _glyph_cache2;
    // For use with SHAPING_MODE_HARFBUZZ_LIGHT:
    LVHashTable<struct LVCharTriplet, struct LVCharPosInfo> _width_cache2;
#endif
public:

    // fallback font support
    
    virtual lUInt32 getFallbackMask() const {
        return _fallback_mask;
    }

    virtual void setFallbackMask(lUInt32 mask) {
        _fallback_mask = mask;
    }

    /// set fallback font for this font
    virtual void setFallbackFont(LVFontRef font);

    /// get fallback font for this font
    virtual LVFont *getFallbackFont(lUInt32 fallbackPassMask);

    /// returns font weight
    virtual int getWeight() const { return _synth_weight > 0 ? _synth_weight : _weight; }

    /// returns italic flag
    virtual int getItalic() const { return _italic; }

    /// sets face name
    virtual void setFaceName(lString8 face) { _faceName = face; }

    LVMutex &getMutex() { return _mutex; }

    FT_Library getLibrary() { return _library; }

    LVFreeTypeFace(LVMutex &mutex, FT_Library library, LVFontGlobalGlyphCache *globalCache);

    virtual ~LVFreeTypeFace();

    virtual void clearCache();

    virtual int getHyphenWidth();

    /// get kerning mode: true==ON, false=OFF
    virtual bool getKerning() const { return _allowKerning; }

    /// set kerning mode: true==ON, false=OFF
    virtual void setKerning(bool kerningEnabled);

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t mode);

    /// returns current hinting mode
    virtual hinting_mode_t getHintingMode() const { return _hintingMode; }

    /// sets current shaping mode
    virtual void setShapingMode( shaping_mode_t shapingMode );

    /// returns current kerning mode
    virtual shaping_mode_t getShapingMode() const { return _shapingMode; }

    /// get bitmap mode (true=bitmap, false=antialiased)
    virtual bool getBitmapMode() { return _drawMonochrome; }

    /// set bitmap mode (true=bitmap, false=antialiased)
    virtual void setBitmapMode(bool drawBitmap);

    /// get antialiasing mode
    virtual font_antialiasing_t GetAntialiasMode() { return _aa_mode; }

    /// set antialiasing mode
    virtual void SetAntialiasMode(font_antialiasing_t mode);
    
    /// get OpenType features (bitmap)
    virtual int getFeatures() const { return _features; }

    /// set OpenType features (bitmap)
    virtual void setFeatures( int features );

    void setSynthWeight(int synth_weight);

    bool loadFromBuffer(LVByteArrayRef buf, int index, int size, css_font_family_t fontFamily,
                        bool monochrome, bool italicize, int weight = -1);

    bool loadFromFile(const char *fname, int index, int size, css_font_family_t fontFamily,
                      bool monochrome, bool italicize, int weight = -1);

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo(lUInt32 code, glyph_info_t *glyph, lChar32 def_char = 0, lUInt32 fallbackPassMask = 0);
/*
  // USE GET_CHAR_FLAGS instead
    inline int calcCharFlags( lChar32 ch )
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

    /** \brief get extra glyph metric
    */
    virtual bool getGlyphExtraMetric( glyph_extra_metric_t metric, lUInt32 code, int & value, bool scaled_to_px=true, lChar32 def_char=0, lUInt32 fallbackPassMask = 0 );

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
    );

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
            const lChar32 *text, int len, TextLangCfg * lang_cfg = NULL
    );

    /** \brief get glyph item
        \param code is unicode character
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem *getGlyph(lUInt32 ch, lChar32 def_char = 0, lUInt32 fallbackPassMask = 0);

//    /** \brief get glyph image in 1 byte per pixel format
//        \param code is unicode character
//        \param buf is buffer [width*height] to place glyph data
//        \return true if glyph was found
//    */
//    virtual bool getGlyphImage(lUInt16 ch, lUInt8 * bmp, lChar32 def_char=0)
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
    virtual int getCharWidth(lChar32 ch, lChar32 def_char = '?');

    /// returns char glyph left side bearing
    int getLeftSideBearing( lChar32 ch, bool negative_only=false, bool italic_only=false );

    /// returns char glyph right side bearing
    virtual int getRightSideBearing( lChar32 ch, bool negative_only=false, bool italic_only=false );

    /// returns extra metric
    virtual int getExtraMetric(font_extra_metric_t metric, bool scaled_to_px=true);

    /// returns if font has OpenType Math tables
    virtual bool hasOTMathSupport() const;

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
        return _allowKerning;
#else
        return _allowKerning && FT_HAS_KERNING( _face );
    #endif
#else
        return false;
#endif
    }

    /// draws text string
    virtual int DrawTextString(LVDrawBuf *buf, int x, int y,
                               const lChar32 *text, int len,
                               lChar32 def_char, lUInt32 *palette = NULL,
                               bool addHyphen = false, TextLangCfg * lang_cfg = NULL,
                               lUInt32 flags = 0, int letter_spacing = 0, int width=-1,
                               int text_decoration_back_gap = 0,
                               lUInt32 fallbackPassMask = 0);

    /// returns true if font is empty
    virtual bool IsNull() const {
        return _face == NULL;
    }

    virtual bool operator!() const {
        return _face == NULL;
    }

    virtual void Clear();
protected:
    void updateTransform();
    FT_UInt getCharIndex(lUInt32 code, lChar32 def_char);
#if USE_HARFBUZZ==1
    LVFontGlyphCacheItem *getGlyphByIndex(lUInt32 index);
    lChar32 filterChar(lChar32 code, lChar32 def_char=0);
    bool hbCalcCharWidth(struct LVCharPosInfo *posInfo, const struct LVCharTriplet &triplet,
                         lChar32 def_char, lUInt32 fallbackPassMask);
    bool setHBFeatureValue(const char * tag, uint32_t value);
    bool addHBFeature(const char * tag);
    bool delHBFeature(const char * tag);
    void setupHBFeatures();
#endif
};

#endif  // (USE_FREETYPE==1)

#endif  // __LV_FREETYPEFACE_H_INCLUDED__
