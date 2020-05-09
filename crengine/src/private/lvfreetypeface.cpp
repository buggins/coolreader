/** \file lvfreetypeface.cpp
    \brief FreeType font implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfreetypeface.h"

#include "../../include/lvfntman.h"
#include "../../include/lvfnt.h"
#include "../../include/lvtextfm.h"
#include "../../include/crlog.h"
#include "lvfontglyphcache.h"
#include "lvfontdef.h"
#include "lvfontcache.h"


#include "../include/gammatbl.h"


// fc-lang database
#include "fc-lang-cat.h"

#if COLOR_BACKBUFFER == 0
//#define USE_BITMAP_FONT
#endif

//DEFINE_NULL_REF( LVFont )

#if (USE_FREETYPE == 1)

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H   // for FT_Outline_Embolden()
#include FT_SYNTHESIS_H // for FT_GlyphSlot_Embolden()

// Use Freetype embolden API instead of LVFontBoldTransform to
// make fake bold (for fonts that do not provide a bold face).
// This gives a chance to get them working with Harfbuzz, even if
// they won't look as nice as if they came with a real bold font.
#define USE_FT_EMBOLDEN

// Helpers with font metrics (units are 1/64 px)
// #define FONT_METRIC_FLOOR(x)    ((x) & -64)
// #define FONT_METRIC_CEIL(x)     (((x)+63) & -64)
// #define FONT_METRIC_ROUND(x)    (((x)+32) & -64)
// #define FONT_METRIC_TRUNC(x)    ((x) >> 6)
#define FONT_METRIC_TO_PX(x)    (((x)+32) >> 6) // ROUND + TRUNC
// Uncomment to use the former >>6 (trunc) with no rounding (instead of previous one)
// #define FONT_METRIC_TO_PX(x)    ((x) >> 6)

extern int gammaIndex;          // lvfntman.cpp

extern lString8 familyName(FT_Face face);


inline int myabs(int n) { return n < 0 ? -n : n; }

static lChar16 getReplacementChar(lUInt16 code) {
    switch (code) {
        case UNICODE_SOFT_HYPHEN_CODE:
            return '-';
        case 0x0401: // CYRILLIC CAPITAL LETTER IO
            return 0x0415; //CYRILLIC CAPITAL LETTER IE
        case 0x0451: // CYRILLIC SMALL LETTER IO
            return 0x0435; // CYRILLIC SMALL LETTER IE
        case UNICODE_NO_BREAK_SPACE:
            return ' ';
        case UNICODE_ZERO_WIDTH_SPACE:
            // If the font lacks a zero-width breaking space glyph (like
            // some Kindle built-ins) substitute a different zero-width
            // character instead of one with width.
            return UNICODE_ZERO_WIDTH_NO_BREAK_SPACE;
        case 0x2010:
        case 0x2011:
        case 0x2012:
        case 0x2013:
        case 0x2014:
        case 0x2015:
            return '-';
        case 0x2018:
        case 0x2019:
        case 0x201a:
        case 0x201b:
            return '\'';
        case 0x201c:
        case 0x201d:
        case 0x201e:
        case 0x201f:
        case 0x00ab:
        case 0x00bb:
            return '\"';
        case 0x2039:
            return '<';
        case 0x203A:
            return '>';
        case 0x2044:
            return '/';
        case 0x2022: // css_lst_disc:
            return '*';
        case 0x26AA: // css_lst_disc:
        case 0x25E6: // css_lst_disc:
        case 0x25CF: // css_lst_disc:
            return 'o';
        case 0x25CB: // css_lst_circle:
            return '*';
        case 0x25A0: // css_lst_square:
            return '-';
    }
    return 0;
}

#if USE_HARFBUZZ==1
bool isHBScriptCursive( hb_script_t script ) {
    // https://github.com/harfbuzz/harfbuzz/issues/64
    // From https://android.googlesource.com/platform/frameworks/minikin/
    //               +/refs/heads/experimental/libs/minikin/Layout.cpp
    return  script == HB_SCRIPT_ARABIC ||
            script == HB_SCRIPT_NKO ||
            script == HB_SCRIPT_PSALTER_PAHLAVI ||
            script == HB_SCRIPT_MANDAIC ||
            script == HB_SCRIPT_MONGOLIAN ||
            script == HB_SCRIPT_PHAGS_PA ||
            script == HB_SCRIPT_DEVANAGARI ||
            script == HB_SCRIPT_BENGALI ||
            script == HB_SCRIPT_GURMUKHI ||
            script == HB_SCRIPT_MODI ||
            script == HB_SCRIPT_SHARADA ||
            script == HB_SCRIPT_SYLOTI_NAGRI ||
            script == HB_SCRIPT_TIRHUTA ||
            script == HB_SCRIPT_OGHAM;
}
#endif

static LVFontGlyphCacheItem *newItem(LVFontLocalGlyphCache *local_cache, lChar16 ch, FT_GlyphSlot slot) // , bool drawMonochrome
{
    FONT_LOCAL_GLYPH_CACHE_GUARD
    FT_Bitmap *bitmap = &slot->bitmap;
    int w = bitmap->width;
    int h = bitmap->rows;
    LVFontGlyphCacheItem *item = LVFontGlyphCacheItem::newItem(local_cache, ch, w, h);
    if (!item)
        return 0;
    if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) { //drawMonochrome
        lUInt8 mask = 0x80;
        const lUInt8 *ptr = (const lUInt8 *) bitmap->buffer;
        lUInt8 *dst = item->bmp;
        //int rowsize = ((w + 15) / 16) * 2;
        for (int y = 0; y < h; y++) {
            const lUInt8 *row = ptr;
            mask = 0x80;
            for (int x = 0; x < w; x++) {
                *dst++ = (*row & mask) ? 0xFF : 00;
                mask >>= 1;
                if (!mask && x != w - 1) {
                    mask = 0x80;
                    row++;
                }
            }
            ptr += bitmap->pitch;//rowsize;
        }
    } else {
#if 0
        if ( bitmap->pixel_mode==FT_PIXEL_MODE_MONO ) {
            memset( item->bmp, 0, w*h );
            lUInt8 * srcrow = bitmap->buffer;
            lUInt8 * dstrow = item->bmp;
            for ( int y=0; y<h; y++ ) {
                lUInt8 * src = srcrow;
                for ( int x=0; x<w; x++ ) {
                    dstrow[x] =  ( (*src)&(0x80>>(x&7)) ) ? 255 : 0;
                    if ((x&7)==7)
                        src++;
                }
                srcrow += bitmap->pitch;
                dstrow += w;
            }
        } else {
#endif
        if (bitmap->buffer && w > 0 && h > 0)
        {
            memcpy(item->bmp, bitmap->buffer, w * h);
            // correct gamma
            if ( gammaIndex!=GAMMA_NO_CORRECTION_INDEX )
                cr_correct_gamma_buf(item->bmp, w * h, gammaIndex);
        }
    }
    item->origin_x = (lInt16) slot->bitmap_left;
    item->origin_y = (lInt16) slot->bitmap_top;
    item->advance  = (lUInt16)(FONT_METRIC_TO_PX( myabs(slot->metrics.horiAdvance) ));
    return item;
}

#if USE_HARFBUZZ == 1

static LVFontGlyphCacheItem *newItem(LVFontLocalGlyphCache *local_cache, lUInt32 index, FT_GlyphSlot slot) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    FT_Bitmap *bitmap = &slot->bitmap;
    int w = bitmap->width;
    int h = bitmap->rows;
    LVFontGlyphCacheItem *item = LVFontGlyphCacheItem::newItem(local_cache, index, w, h);
    if (!item)
        return 0;
    if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO) { //drawMonochrome
        lUInt8 mask = 0x80;
        const lUInt8 *ptr = (const lUInt8 *) bitmap->buffer;
        lUInt8 *dst = item->bmp;
        //int rowsize = ((w + 15) / 16) * 2;
        for (int y = 0; y < h; y++) {
            const lUInt8 *row = ptr;
            mask = 0x80;
            for (int x = 0; x < w; x++) {
                *dst++ = (*row & mask) ? 0xFF : 00;
                mask >>= 1;
                if (!mask && x != w - 1) {
                    mask = 0x80;
                    row++;
                }
            }
            ptr += bitmap->pitch;//rowsize;
        }
    } else {
        if (bitmap->buffer && w > 0 && h > 0)
        {
            memcpy(item->bmp, bitmap->buffer, w * h);
            // correct gamma
            if ( gammaIndex!=GAMMA_NO_CORRECTION_INDEX )
                cr_correct_gamma_buf(item->bmp, w * h, gammaIndex);
        }
    }
    item->origin_x = (lInt16) slot->bitmap_left;
    item->origin_y = (lInt16) slot->bitmap_top;
    item->advance =    (lUInt16)(FONT_METRIC_TO_PX( myabs(slot->metrics.horiAdvance) ));
    return item;
}

#endif

// The 2 slots with "LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER" on the 2nd line previously
// were: "LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER".
// LCHAR_IS_EOL was not used by any code, and has been replaced by LCHAR_IS_CLUSTER_TAIL
// (as flags were lUInt8, and the 8 bits were used, one needed to be dropped - they
// have since been upgraded to be lUInt16)
// (LCHAR_DEPRECATED_WRAP_AFTER for '-' and '/', as they may be used to
// separate words.)
static lUInt16 char_flags[] = {
        0, 0, 0, 0, 0, 0, 0, 0, // 0    00
    0, 0, LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER, 0, // 8    08
    0, LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER, 0, 0, // 12   0C
        0, 0, 0, 0, 0, 0, 0, 0, // 16   10
        0, 0, 0, 0, 0, 0, 0, 0, // 24   18
        LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER, 0, 0, 0, 0, 0, 0, 0, // 32   20
    0, 0, 0, 0, 0, LCHAR_DEPRECATED_WRAP_AFTER, 0, LCHAR_DEPRECATED_WRAP_AFTER, // 40   28
        0, 0, 0, 0, 0, 0, 0, 0, // 48   30
};

// removed, as soft hyphens are now exclusively handled by hyphman:
//      (ch==UNICODE_SOFT_HYPHEN_CODE?LCHAR_ALLOW_WRAP_AFTER:
#define GET_CHAR_FLAGS(ch) \
     (ch<48?char_flags[ch]: \
        (ch==UNICODE_NO_BREAK_SPACE ? LCHAR_IS_SPACE: \
        (ch==UNICODE_NO_BREAK_HYPHEN ? 0: \
        (ch>=UNICODE_HYPHEN && ch<=UNICODE_EM_DASH ? LCHAR_DEPRECATED_WRAP_AFTER: \
        (ch==UNICODE_ARMENIAN_HYPHEN ? LCHAR_DEPRECATED_WRAP_AFTER: \
        (ch==UNICODE_FIGURE_SPACE ? 0: \
        (ch>=UNICODE_EN_QUAD && ch<=UNICODE_ZERO_WIDTH_SPACE ? LCHAR_ALLOW_WRAP_AFTER: \
         0)))))))

#if USE_HARFBUZZ == 1
// For use with Harfbuzz light
struct LVCharTriplet
{
    lChar16 prevChar;
    lChar16 Char;
    lChar16 nextChar;
    bool operator==(const struct LVCharTriplet &other) {
        return prevChar == other.prevChar && Char == other.Char && nextChar == other.nextChar;
    }
};

struct LVCharPosInfo
{
    int offset;
    int width;
};

inline lUInt32 getHash( const struct LVCharTriplet& triplet )
{
    // lUInt32 hash = (((
    //       getHash((lUInt32)triplet.Char) )*31
    //     + getHash((lUInt32)triplet.prevChar) )*31
    //     + getHash((lUInt32)triplet.nextChar) );
    // Probably less expensive:
    lUInt32 hash = getHash( (lUInt64)triplet.Char
                   + (((lUInt64) triplet.prevChar) << 16)
                   + (((lUInt64) triplet.nextChar) << 32));
    return hash;
}

#endif  // USE_HARFBUZZ==1

void LVFreeTypeFace::setFallbackFont(LVFontRef font) {
    _fallbackFont = font;
    _fallbackFontIsSet = !font.isNull();
    clearCache();
}


LVFont *LVFreeTypeFace::getFallbackFont() {
    if (_fallbackFontIsSet)
        return _fallbackFont.get();
        // To avoid circular link, disable fallback for fallback font:
        if ( fontMan->GetFallbackFontFace()!=_faceName )
            _fallbackFont = fontMan->GetFallbackFont(_size, _weight, _italic);
    _fallbackFontIsSet = true;
    return _fallbackFont.get();
}

LVFreeTypeFace::LVFreeTypeFace(LVMutex &mutex, FT_Library library,
                               LVFontGlobalGlyphCache *globalCache)
        : _mutex(mutex), _fontFamily(css_ff_sans_serif), _library(library), _face(NULL),
          _size(0), _hyphen_width(0), _baseline(0),
          _weight(400), _italic(0), _embolden(false), _features(0),
          _glyph_cache(globalCache),
          _drawMonochrome(false),
          _hintingMode(HINTING_MODE_AUTOHINT),
          _shapingMode(SHAPING_MODE_FREETYPE),
          _fallbackFontIsSet(false)
#if USE_HARFBUZZ == 1
        , _glyph_cache2(globalCache),
          _width_cache2(1024)
#endif
{
    _matrix.xx = 0x10000;
    _matrix.yy = 0x10000;
    _matrix.xy = 0;
    _matrix.yx = 0;
    _hintingMode = fontMan->GetHintingMode();
#if USE_HARFBUZZ == 1
    _hb_font = 0;
    _hb_buffer = hb_buffer_create();
    _hb_features.reserve(22);
    setupHBFeatures();
#endif
}

LVFreeTypeFace::~LVFreeTypeFace() {
#if USE_HARFBUZZ == 1
    if (_hb_buffer)
        hb_buffer_destroy(_hb_buffer);
#endif
    Clear();
}

void LVFreeTypeFace::clearCache() {
    _glyph_cache.clear();
    _wcache.clear();
    _lsbcache.clear();
    _rsbcache.clear();
#if USE_HARFBUZZ == 1
    _glyph_cache2.clear();
    _width_cache2.clear();
#endif
}

int LVFreeTypeFace::getHyphenWidth() {
    FONT_GUARD
    if (!_hyphen_width) {
        _hyphen_width = getCharWidth(UNICODE_SOFT_HYPHEN_CODE);
    }
    return _hyphen_width;
}

void LVFreeTypeFace::setKerning(bool kerningEnabled) {
    _allowKerning = kerningEnabled;
    _hash = 0; // Force lvstyles.cpp calcHash(font_ref_t) to recompute the hash
#if USE_HARFBUZZ == 1
    if (_allowKerning)
        setHBFeatureValue("kern", 1);
    else
        setHBFeatureValue("kern", 0);
    // in cache may be found some ligatures, so clear it
    clearCache();
#endif
}

void LVFreeTypeFace::setHintingMode(hinting_mode_t mode) {
    if (_hintingMode == mode)
        return;
    _hintingMode = mode;
    _hash = 0; // Force lvstyles.cpp calcHash(font_ref_t) to recompute the hash
    clearCache();
    #if USE_HARFBUZZ==1
    // Also update HB load flags with the updated hinting mode.
    // We need this destroy/create, as only these will clear some internal HB caches
    // (ft_font->advance_cache, ft_font->cached_x_scale); hb_ft_font_set_load_flags will not.
    if (_hb_font)
        hb_font_destroy(_hb_font);
    _hb_font = hb_ft_font_create(_face, NULL);
    if (_hb_font) {
        // Use the same load flags as we do when using FT directly, to avoid mismatching advances & raster
        int flags = FT_LOAD_DEFAULT;
        flags |= (!_drawMonochrome ? FT_LOAD_TARGET_LIGHT : FT_LOAD_TARGET_MONO);
        if (_hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR) {
            flags |= FT_LOAD_NO_AUTOHINT;
        }
        else if (_hintingMode == HINTING_MODE_AUTOHINT) {
            flags |= FT_LOAD_FORCE_AUTOHINT;
        }
        else if (_hintingMode == HINTING_MODE_DISABLED) {
            flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        }
        hb_ft_font_set_load_flags(_hb_font, flags);
    }
    #endif
}

void LVFreeTypeFace::setShapingMode( shaping_mode_t shapingMode )
{
    _shapingMode = shapingMode;
    _hash = 0; // Force lvstyles.cpp calcHash(font_ref_t) to recompute the hash
#if USE_HARFBUZZ==1
    setupHBFeatures();
    // Reset buffer (to have it shrunk if HB full > light that will need only a 3 slots buffer)
    hb_buffer_reset(_hb_buffer);
    // in cache may be found some ligatures, so clear it
    clearCache();
#endif
}

void LVFreeTypeFace::setBitmapMode(bool drawBitmap) {
    if (_drawMonochrome == drawBitmap)
        return;
    _drawMonochrome = drawBitmap;
    clearCache();
}

void LVFreeTypeFace::setFeatures(int features)
{
    _features = features;
    _hash = 0; // Force lvstyles.cpp calcHash(font_ref_t) to recompute the hash
}

// Synthetized bold on a font that does not come with a bold variant.
void LVFreeTypeFace::setEmbolden()
{
    _embolden = true;
    // A real bold font has weight 700, vs 400 for the regular.
    // LVFontBoldTransform did +200, so we get 600 (demibold).
    // Let's do the same (even if I don't see why not +300).
    _weight = (_weight + 200 > 900) ? 900 : _weight + 200;
    // And add +1 so we can know it's a fake/synthetized font, so we
    // can avoid getting it (and get the original regular font instead)
    // when synthetizing an other variant of that font.
    _weight += 1;
    // When not using Harfbuzz, we will simply call FT_GlyphSlot_Embolden()
    // to get the glyphinfo and glyph with synthetized bold and increased
    // metrics, and everything should work naturally:
    //   "Embolden a glyph by a 'reasonable' value (which is highly a matter
    //   of taste) [...] For emboldened outlines the height, width, and
    //   advance metrics are increased by the strength of the emboldening".
    //
    // When using Harfbuzz, which uses itself the font metrics, that we
    // can't tweak at all from outside, we'll get positionning based on
    // the not-bolded font. We can't increase them as that would totally
    // mess HB work.
    // We can only do as MuPDF does (source/fitz/font.c): keep the HB
    // positions, offset and advances, embolden the glyph by some value
    // of 'strength', and shift left/bottom by 1/2 'strength', so the
    // boldened glyph is centered on its original: the glyph being a
    // bit larger, it will blend over its neighbour glyphs, but it
    // looks quite allright.
    // Caveat: words in fake bold will be bolder, but not larger than
    // the same word in the regular font (unlike with a real bold font
    // were they would be bolder and larger).
    // We need to compute the strength as done in FT_GlyphSlot_Embolden():
    //   xstr = FT_MulFix( face->units_per_EM, face->size->metrics.y_scale ) / 24;
    //   ystr = xstr;
    //   FT_Outline_EmboldenXY( &slot->outline, xstr, ystr );
    // and will do as MuPDF does (with some private value of 'strength'):
    //   FT_Outline_Embolden(&face->glyph->outline, strength);
    //   FT_Outline_Translate(&face->glyph->outline, -strength/2, -strength/2);
    // (with strength: 0=no change; 64=1px embolden; 128=2px embolden and 1px x/y translation)
    // int strength = (_face->units_per_EM * _face->size->metrics.y_scale) / 24;
    FT_Pos embolden_strength = FT_MulFix(_face->units_per_EM, _face->size->metrics.y_scale) / 24;
    // Make it slightly less bold than Freetype's bold, as we get less spacing
    // around glyphs with HarfBuzz, by getting the unbolded advances.
    embolden_strength = embolden_strength * 3/4; // (*1/2 is fine but a tad too light)
    _embolden_half_strength = embolden_strength / 2;
}

bool LVFreeTypeFace::loadFromBuffer(LVByteArrayRef buf, int index, int size,
                                    css_font_family_t fontFamily, bool monochrome, bool italicize) {
    FONT_GUARD
    _hintingMode = fontMan->GetHintingMode();
    _drawMonochrome = monochrome;
    _fontFamily = fontFamily;
    if (_face)
        FT_Done_Face(_face);
    int error = FT_New_Memory_Face(_library, buf->get(), buf->length(), index,
                                   &_face); /* create face object */
    if (error)
        return false;
    if (_fileName.endsWith(".pfb") || _fileName.endsWith(".pfa")) {
        lString8 kernFile = _fileName.substr(0, _fileName.length() - 4);
        if (LVFileExists(Utf8ToUnicode(kernFile) + ".afm")) {
            kernFile += ".afm";
        } else if (LVFileExists(Utf8ToUnicode(kernFile) + ".pfm")) {
            kernFile += ".pfm";
        } else {
            kernFile.clear();
        }
        if (!kernFile.empty())
            error = FT_Attach_File(_face, kernFile.c_str());
    }
    //FT_Face_SetUnpatentedHinting( _face, 1 );
    _slot = _face->glyph;
    _faceName = familyName(_face);
    CRLog::debug("Loaded font %s [%d]: faceName=%s, ", _fileName.c_str(), index, _faceName.c_str());
    //if ( !FT_IS_SCALABLE( _face ) ) {
    //    Clear();
    //    return false;
    // }
    error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            size);  /* pixel_height          */
#if USE_HARFBUZZ == 1
    if (FT_Err_Ok == error) {
        if (_hb_font)
            hb_font_destroy(_hb_font);
        _hb_font = hb_ft_font_create(_face, 0);
        if ( _hb_font ) {
            // Use the same load flags as we do when using FT directly, to avoid mismatching advances & raster
            int flags = FT_LOAD_DEFAULT;
            flags |= (!_drawMonochrome ? FT_LOAD_TARGET_LIGHT : FT_LOAD_TARGET_MONO);
            if (_hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR) {
                flags |= FT_LOAD_NO_AUTOHINT;
            }
            else if (_hintingMode == HINTING_MODE_AUTOHINT) {
                flags |= FT_LOAD_FORCE_AUTOHINT;
            }
            else if (_hintingMode == HINTING_MODE_DISABLED) {
                flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
            }
            hb_ft_font_set_load_flags(_hb_font, flags);
        } else {
            error = FT_Err_Invalid_Argument;
        }
    }

#endif
    if (error) {
        Clear();
        return false;
    }
#if 0
    int nheight = _face->size->metrics.height;
    int targetheight = size << 6;
    error = FT_Set_Pixel_Sizes(
                _face,    /* handle to face object */
                0,        /* pixel_width           */
                (size * targetheight + nheight/2)/ nheight );  /* pixel_height          */
#endif

    _height = FONT_METRIC_TO_PX( _face->size->metrics.height );
    _size = size; //(_face->size->metrics.height >> 6);
    _baseline = _height + FONT_METRIC_TO_PX( _face->size->metrics.descender );
    _weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
    _italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;

    if (!error && italicize && !_italic) {
        _matrix.xy = 0x10000 * 3 / 10;
        FT_Set_Transform(_face, &_matrix, NULL);
            _italic = 2;
    }

    if (error) {
        return false;
    }

    // If no unicode charmap, select any symbol charmap.
    // This is needed with Harfbuzz shaping (with Freetype, we switch charmap
    // when needed). It might not be needed with a Harfbuzz newer than 2.6.1
    // that will include https://github.com/harfbuzz/harfbuzz/pull/1948.
    if (FT_Select_Charmap(_face, FT_ENCODING_UNICODE)) // non-zero means failure
        // If no unicode charmap found, try symbol charmap
        FT_Select_Charmap(_face, FT_ENCODING_MS_SYMBOL);

    return true;
}

bool
LVFreeTypeFace::loadFromFile(const char *fname, int index, int size, css_font_family_t fontFamily,
                             bool monochrome, bool italicize) {
    FONT_GUARD
    _hintingMode = fontMan->GetHintingMode();
    _drawMonochrome = monochrome;
    _fontFamily = fontFamily;
    if (fname)
        _fileName = fname;
    if (_fileName.empty())
        return false;
    if (_face)
        FT_Done_Face(_face);
    int error = FT_New_Face(_library, _fileName.c_str(), index, &_face); /* create face object */
    if (error)
        return false;
    if (_fileName.endsWith(".pfb") || _fileName.endsWith(".pfa")) {
        lString8 kernFile = _fileName.substr(0, _fileName.length() - 4);
        if (LVFileExists(Utf8ToUnicode(kernFile) + ".afm")) {
            kernFile += ".afm";
        } else if (LVFileExists(Utf8ToUnicode(kernFile) + ".pfm")) {
            kernFile += ".pfm";
        } else {
            kernFile.clear();
        }
        if (!kernFile.empty())
            error = FT_Attach_File(_face, kernFile.c_str());
    }
    //FT_Face_SetUnpatentedHinting( _face, 1 );
    _slot = _face->glyph;
    _faceName = familyName(_face);
    CRLog::debug("Loaded font %s [%d]: faceName=%s, ", _fileName.c_str(), index, _faceName.c_str());
    //if ( !FT_IS_SCALABLE( _face ) ) {
    //    Clear();
    //    return false;
    // }
    error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            size);  /* pixel_height          */
#if USE_HARFBUZZ == 1
    if (FT_Err_Ok == error) {
        if (_hb_font)
            hb_font_destroy(_hb_font);
        _hb_font = hb_ft_font_create(_face, 0);
        if (!_hb_font) {
            error = FT_Err_Invalid_Argument;
        }
        else {
            // Use the same load flags as we do when using FT directly, to avoid mismatching advances & raster
            int flags = FT_LOAD_DEFAULT;
            flags |= (!_drawMonochrome ? FT_LOAD_TARGET_LIGHT : FT_LOAD_TARGET_MONO);
            if (_hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR) {
                flags |= FT_LOAD_NO_AUTOHINT;
            }
            else if (_hintingMode == HINTING_MODE_AUTOHINT) {
                flags |= FT_LOAD_FORCE_AUTOHINT;
            }
            else if (_hintingMode == HINTING_MODE_DISABLED) {
                flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
            }
            hb_ft_font_set_load_flags(_hb_font, flags);
        }
    }
#endif
    if (error) {
        Clear();
        return false;
    }
#if 0
    int nheight = _face->size->metrics.height;
    int targetheight = size << 6;
    error = FT_Set_Pixel_Sizes(
                _face,    /* handle to face object */
                0,        /* pixel_width           */
                (size * targetheight + nheight/2)/ nheight );  /* pixel_height          */
#endif

    _height = FONT_METRIC_TO_PX( _face->size->metrics.height );
    _size = size; //(_face->size->metrics.height >> 6);
    _baseline = _height + FONT_METRIC_TO_PX( _face->size->metrics.descender );
    _weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
    _italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;

    if (!error && italicize && !_italic) {
        _matrix.xy = 0x10000 * 3 / 10;
        FT_Set_Transform(_face, &_matrix, NULL);
            _italic = 2;
    }

    if (error) {
        // error
        return false;
    }

    // If no unicode charmap, select any symbol charmap.
    // This is needed with Harfbuzz shaping (with Freetype, we switch charmap
    // when needed). It might not be needed with a Harfbuzz newer than 2.6.1
    // that will include https://github.com/harfbuzz/harfbuzz/pull/1948.
    if (FT_Select_Charmap(_face, FT_ENCODING_UNICODE)) // non-zero means failure
        // If no unicode charmap found, try symbol charmap
        FT_Select_Charmap(_face, FT_ENCODING_MS_SYMBOL);

    return true;
}

#if USE_HARFBUZZ == 1

lChar16 LVFreeTypeFace::filterChar(lChar16 code, lChar16 def_char) {
    if (code == '\t')     // (FreeSerif doesn't have \t, get a space
        code = ' ';       // rather than a '?')
    FT_UInt ch_glyph_index = FT_Get_Char_Index(_face, code);
    if (ch_glyph_index != 0) { // found
        return code;
    }

    if ( code >= 0xF000 && code <= 0xF0FF) {
        // If no glyph found and code is among the private unicode
        // area classically used by symbol fonts (range U+F020-U+F0FF),
        // try to switch to FT_ENCODING_MS_SYMBOL
        if (!FT_Select_Charmap(_face, FT_ENCODING_MS_SYMBOL)) {
            ch_glyph_index = FT_Get_Char_Index( _face, code );
            // restore unicode charmap if there is one
            FT_Select_Charmap(_face, FT_ENCODING_UNICODE);
            if (ch_glyph_index != 0) { // glyph found: code is valid
                return code;
            }
        }
    }
    lChar16 res = getReplacementChar(code);
    if (res != 0)
        return res;
    if (def_char != 0)
        return def_char;
    // If nothing found, let code be
    return code;
}

bool LVFreeTypeFace::hbCalcCharWidth(LVCharPosInfo *posInfo, const LVCharTriplet &triplet,
                                     lChar16 def_char) {
    if (!posInfo)
        return false;
    unsigned int segLen = 0;
    int cluster;
    hb_buffer_clear_contents(_hb_buffer);
    if (0 != triplet.prevChar) {
        hb_buffer_add(_hb_buffer, (hb_codepoint_t) triplet.prevChar, segLen);
        segLen++;
    }
    hb_buffer_add(_hb_buffer, (hb_codepoint_t) triplet.Char, segLen);
    cluster = segLen;
    segLen++;
    if (0 != triplet.nextChar) {
        hb_buffer_add(_hb_buffer, (hb_codepoint_t) triplet.nextChar, segLen);
        segLen++;
    }
    hb_buffer_set_content_type(_hb_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
    hb_buffer_guess_segment_properties(_hb_buffer);
    hb_shape(_hb_font, _hb_buffer, _hb_features.ptr(), _hb_features.length());
    unsigned int glyph_count = hb_buffer_get_length(_hb_buffer);
    if (segLen == glyph_count) {
        hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(_hb_buffer, &glyph_count);
        hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(_hb_buffer,
                                                                       &glyph_count);
        // Ignore HB measurements when there is a single glyph not found,
        // as it may be found in a fallback font
        int codepoint_notfound_nb = 0;
        for (int i=0; i<glyph_count; i++) {
            if ( glyph_info[i].codepoint == 0 )
                codepoint_notfound_nb++;
            // This does not look like it's really needed to ignore
            // more measurements (I felt it was needed for hebrew with
            // many diacritics).
            // if ( glyph_pos[i].x_advance == 0 )
            //    zero_advance_nb++;
        }
        if ( codepoint_notfound_nb == 0 ) {
            // Be sure HB chosen glyph is the same as freetype chosen glyph,
            // which will be the one that will be rendered
            FT_UInt ch_glyph_index = FT_Get_Char_Index( _face, triplet.Char );
            if ( glyph_info[cluster].codepoint == ch_glyph_index ) {
                posInfo->offset = FONT_METRIC_TO_PX(glyph_pos[cluster].x_offset);
                posInfo->width = FONT_METRIC_TO_PX(glyph_pos[cluster].x_advance);
                return true;
            }
        }

    }
    // Otherwise, use plain Freetype getGlyphInfo() which will check
    // again with this font, or the fallback one
    glyph_info_t glyph;
    if ( getGlyphInfo(triplet.Char, &glyph, def_char) ) {
        posInfo->offset = 0;
        posInfo->width = glyph.width;
        return true;
    }
    return false;
}

#endif  // USE_HARFBUZZ==1

FT_UInt LVFreeTypeFace::getCharIndex(lUInt32 code, lChar16 def_char) {
    if (code == '\t')
        code = ' ';
    FT_UInt ch_glyph_index = FT_Get_Char_Index(_face, code);
    if ( ch_glyph_index==0 && code >= 0xF000 && code <= 0xF0FF) {
        // If no glyph found and code is among the private unicode
        // area classically used by symbol fonts (range U+F020-U+F0FF),
        // try to switch to FT_ENCODING_MS_SYMBOL
        if (!FT_Select_Charmap(_face, FT_ENCODING_MS_SYMBOL)) {
            ch_glyph_index = FT_Get_Char_Index( _face, code );
            // restore unicode charmap if there is one
            FT_Select_Charmap(_face, FT_ENCODING_UNICODE);
        }
    }
    if ( ch_glyph_index==0 ) {
        lUInt32 replacement = getReplacementChar( code );
        if ( replacement )
            ch_glyph_index = FT_Get_Char_Index( _face, replacement );
        if ( ch_glyph_index==0 && def_char )
            ch_glyph_index = FT_Get_Char_Index( _face, def_char );
    }
    return ch_glyph_index;
}

#if USE_HARFBUZZ == 1
bool LVFreeTypeFace::setHBFeatureValue(const char *tag, uint32_t value)
{
    hb_feature_t hb_feature;
    if (hb_feature_from_string(tag, -1, &hb_feature)) {
        int idx = -1;
        int i;
        for (i = 0; i < _hb_features.length(); i++) {
            if (_hb_features[i].tag == hb_feature.tag) {
                idx = i;
                break;
            }
        }
        if (idx < 0) {
            idx = _hb_features.length();
            _hb_features.add(hb_feature);
        }
        _hb_features[idx].value = value;
        return true;
    }
    return false;
}

bool LVFreeTypeFace::addHBFeature(const char *tag)
{
    hb_feature_t hb_feature;
    if (hb_feature_from_string(tag, -1, &hb_feature)) {
        for (int i = 0; i < _hb_features.length(); i++) {
            if (_hb_features[i].tag == hb_feature.tag) {
                _hb_features[i] = hb_feature;
                return true;
            }
        }
        _hb_features.add(hb_feature);
        return true;
    }
    return false;
}

bool LVFreeTypeFace::delHBFeature(const char *tag)
{
    hb_feature_t hb_feature;
    if (hb_feature_from_string(tag, -1, &hb_feature)) {
        for (int i = 0; i < _hb_features.length(); i++) {
            if (_hb_features[i].tag == hb_feature.tag) {
                _hb_features.remove(i);
                return true;
            }
        }
    }
    return false;
}

void LVFreeTypeFace::setupHBFeatures()
{
    _hb_features.clear();
    if ( _shapingMode == SHAPING_MODE_HARFBUZZ ) {
        setHBFeatureValue("kern", _allowKerning ? 1 : 0);
        addHBFeature("+liga");
    }
    else if ( _shapingMode == SHAPING_MODE_HARFBUZZ_LIGHT ) {
        // HarfBuzz features for lighweight characters width calculating with caching.
        // We need to disable all the features, enabled by default in Harfbuzz, that
        // may split a char into more glyphs, or merge chars into one glyph.
        // (see harfbuzz/src/hb-ot-shape.cc hb_ot_shape_collect_features() )
        //
        // We can enable these ones:
        setHBFeatureValue("kern", _allowKerning ? 1 : 0);
        addHBFeature("+mark");          // Mark Positioning: Fine positioning of a mark glyph to a base character
        addHBFeature("+mkmk");          // Mark-to-mark Positioning: Fine positioning of a mark glyph to another mark character
        addHBFeature("+curs");          // Cursive Positioning: Precise positioning of a letter's connection to an adjacent one
        addHBFeature("+locl");          // Substitutes character with the preferred form based on script language
        //
        // We should disable these ones:
        addHBFeature("-liga");          // Standard Ligatures: replaces (by default) sequence of characters with a single ligature glyph
        addHBFeature("-rlig");          // Ligatures required for correct text display (any script, but in cursive) - Arabic, semitic
        addHBFeature("-clig");          // Applies a second ligature feature based on a match of a character pattern within a context of surrounding patterns
        addHBFeature("-ccmp");          // Glyph composition/decomposition: either calls a ligature replacement on a sequence of characters or replaces a character with a sequence of glyphs
        // Provides logic that can for example effectively alter the order of input characters
        addHBFeature("-calt");          // Contextual Alternates: Applies a second substitution feature based on a match of a character pattern within a context of surrounding patterns
        addHBFeature("-rclt");          // Required Contextual Alternates: Contextual alternates required for correct text display which differs from the default join for other letters, required especially important by Arabic
        addHBFeature("-rvrn");          // Required Variation Alternates: Special variants of a single character, which need apply to specific font variation, required by variable fonts
        addHBFeature("-ltra");          // Left-to-right glyph alternates: Replaces characters with forms befitting left-to-right presentation
        addHBFeature("-ltrm");          // Left-to-right mirrored forms: Replaces characters with possibly mirrored forms befitting left-to-right presentation
        addHBFeature("-rtla");          // Right-to-left glyph alternates: Replaces characters with forms befitting right-to-left presentation
        addHBFeature("-rtlm");          // Right-to-left mirrored forms: Replaces characters with possibly mirrored forms befitting right-to-left presentation
        addHBFeature("-frac");          // Fractions: Converts figures separated by slash with diagonal fraction
        addHBFeature("-numr");          // Numerator: Converts to appropriate fraction numerator form, invoked by frac
        addHBFeature("-dnom");          // Denominator: Converts to appropriate fraction denominator form, invoked by frac
        addHBFeature("-rand");          // Replaces character with random forms (meant to simulate handwriting)
        addHBFeature("-trak");          // Tracking (?)
        addHBFeature("-vert");          // Vertical (?)
        // Especially needed with FreeSerif and french texts: -ccmp
        // Especially needed with Fedra Serif and "The", "Thuringe": -calt
        // These tweaks seem fragile (adding here +smcp to experiment with small caps would break FreeSerif again).
        // So, when tuning these, please check it still behave well with FreeSerif.
        //
        // The way KERNING_MODE_HARFBUZZ_LIGHT is implemented, we'll be drawing the
        // original codepoints, so there's no much use allowing additional HB features,
        // even the one-to-one substitutions like small-cap or oldstyle-nums...
        return;
    }
    else { // text shaping not use HarfBuzz features
        return;
    }
    if ( _features != LFNT_OT_FEATURES_NORMAL ) {
        // Add requested features
        if ( _features & LFNT_OT_FEATURES_M_LIGA ) { addHBFeature("-liga"); addHBFeature("-clig"); }
        if ( _features & LFNT_OT_FEATURES_M_CALT ) { addHBFeature("-calt"); }
        if ( _features & LFNT_OT_FEATURES_P_DLIG ) { addHBFeature("+dlig"); }
        if ( _features & LFNT_OT_FEATURES_M_DLIG ) { addHBFeature("-dlig"); }
        if ( _features & LFNT_OT_FEATURES_P_HLIG ) { addHBFeature("+hlig"); }
        if ( _features & LFNT_OT_FEATURES_M_HLIG ) { addHBFeature("-hlig"); }
        if ( _features & LFNT_OT_FEATURES_P_HIST ) { addHBFeature("+hist"); }
        if ( _features & LFNT_OT_FEATURES_P_RUBY ) { addHBFeature("+ruby"); }
        if ( _features & LFNT_OT_FEATURES_P_SMCP ) { addHBFeature("+smcp"); }
        if ( _features & LFNT_OT_FEATURES_P_C2SC ) { addHBFeature("+c2sc"); addHBFeature("+smcp"); }
        if ( _features & LFNT_OT_FEATURES_P_PCAP ) { addHBFeature("+pcap"); }
        if ( _features & LFNT_OT_FEATURES_P_C2PC ) { addHBFeature("+c2pc"); addHBFeature("+pcap"); }
        if ( _features & LFNT_OT_FEATURES_P_UNIC ) { addHBFeature("+unic"); }
        if ( _features & LFNT_OT_FEATURES_P_TITL ) { addHBFeature("+titl"); }
        if ( _features & LFNT_OT_FEATURES_P_SUPS ) { addHBFeature("+sups"); }
        if ( _features & LFNT_OT_FEATURES_P_SUBS ) { addHBFeature("+subs"); }
        if ( _features & LFNT_OT_FEATURES_P_LNUM ) { addHBFeature("+lnum"); }
        if ( _features & LFNT_OT_FEATURES_P_ONUM ) { addHBFeature("+onum"); }
        if ( _features & LFNT_OT_FEATURES_P_PNUM ) { addHBFeature("+pnum"); }
        if ( _features & LFNT_OT_FEATURES_P_TNUM ) { addHBFeature("+tnum"); }
        if ( _features & LFNT_OT_FEATURES_P_ZERO ) { addHBFeature("+zero"); }
        if ( _features & LFNT_OT_FEATURES_P_ORDN ) { addHBFeature("+ordn"); }
        if ( _features & LFNT_OT_FEATURES_P_FRAC ) { addHBFeature("+frac"); }
        if ( _features & LFNT_OT_FEATURES_P_AFRC ) { addHBFeature("+afrc"); }
        if ( _features & LFNT_OT_FEATURES_P_SMPL ) { addHBFeature("+smpl"); }
        if ( _features & LFNT_OT_FEATURES_P_TRAD ) { addHBFeature("+trad"); }
        if ( _features & LFNT_OT_FEATURES_P_FWID ) { addHBFeature("+fwid"); }
        if ( _features & LFNT_OT_FEATURES_P_PWID ) { addHBFeature("+pwid"); }
        if ( _features & LFNT_OT_FEATURES_P_JP78 ) { addHBFeature("+jp78"); }
        if ( _features & LFNT_OT_FEATURES_P_JP83 ) { addHBFeature("+jp83"); }
        if ( _features & LFNT_OT_FEATURES_P_JP04 ) { addHBFeature("+jp04"); }
    }
}
#endif

bool LVFreeTypeFace::getGlyphInfo(lUInt32 code, LVFont::glyph_info_t *glyph, lChar16 def_char) {
    //FONT_GUARD
    int glyph_index = getCharIndex(code, 0);
    if (glyph_index == 0) {
        LVFont *fallback = getFallbackFont();
        if (!fallback) {
            // No fallback
            glyph_index = getCharIndex(code, def_char);
            if (glyph_index == 0)
                return false;
        } else {
            // Fallback
            return fallback->getGlyphInfo(code, glyph, def_char);
        }
    }
    int flags = FT_LOAD_DEFAULT;
    flags |= (!_drawMonochrome ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO);
    if (_hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR) {
        flags |= FT_LOAD_NO_AUTOHINT;
    }
    else if (_hintingMode == HINTING_MODE_AUTOHINT) {
        flags |= FT_LOAD_FORCE_AUTOHINT;
    }
    else if (_hintingMode == HINTING_MODE_DISABLED) {
        flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
    }
    updateTransform(); // no-op
    int error = FT_Load_Glyph(
            _face,          /* handle to face object */
            glyph_index,   /* glyph index           */
            flags);  /* load flags, see below */
    if ( error == FT_Err_Execution_Too_Long && _hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR ) {
        // Native hinting bytecode may fail with some bad fonts: try again with no hinting
        flags |= FT_LOAD_NO_HINTING;
        error = FT_Load_Glyph( _face, glyph_index, flags );
    }
    if (error)
        return false;
    if (_embolden) { // Embolden so we get the real embolden metrics
        // See setEmbolden() for details
        FT_GlyphSlot_Embolden(_slot);
    }
    glyph->blackBoxX = (lUInt16)( FONT_METRIC_TO_PX( _slot->metrics.width ) );
    glyph->blackBoxY = (lUInt16)( FONT_METRIC_TO_PX( _slot->metrics.height ) );
    glyph->originX =   (lInt16)( FONT_METRIC_TO_PX( _slot->metrics.horiBearingX ) );
    glyph->originY =   (lInt16)( FONT_METRIC_TO_PX( _slot->metrics.horiBearingY ) );
    glyph->width =     (lUInt16)( FONT_METRIC_TO_PX( myabs(_slot->metrics.horiAdvance )) );
    if (glyph->blackBoxX == 0) // If a glyph has no blackbox (a spacing
        glyph->rsb =   0;      // character), there is no bearing
    else
        glyph->rsb =   (lInt16)(FONT_METRIC_TO_PX( (myabs(_slot->metrics.horiAdvance)
                                    - _slot->metrics.horiBearingX - _slot->metrics.width) ) );
    // printf("%c: %d + %d + %d = %d (y: %d + %d)\n", code, glyph->originX, glyph->blackBoxX,
    //                            glyph->rsb, glyph->width, glyph->originY, glyph->blackBoxY);
    // (Old) Note: these >>6 on a negative number will floor() it, so we'll get
    // a ceil()'ed value when considering negative numbers as some overflow,
    // which is good when we're using it for adding some padding.
    //
    // Note: when the font does not provide italic glyphs (_italic = 2), some fake
    // italic/oblique is obtained with FreeType transformation (_matrix.xy and
    // FT_Set_Transform()). freetype.h states about it:
    //     Note that this also transforms the `face.glyph.advance' field,
    //     but *not* the values in `face.glyph.metrics'.
    // So, with such fake italic, the values just computed above are wrong,
    // and may cause some wrong glyphs positionning or advance.
    // Some possible attempt at guessing the transformed values can be found in
    // http://code.qt.io/cgit/qt/qtbase.git/tree/src/platformsupport/fontdatabases/freetype/qfontengine_ft.cpp
    // (transformBoundingBox) but a straightforward port here does not give
    // the expected rendering...

    return true;
}

bool LVFreeTypeFace::checkFontLangCompat(const lString8 &langCode) {
#define FC_LANG_START_INTERVAL_CODE     2
    bool fullSupport = false;
    bool partialSupport = false;
    struct fc_lang_catalog *lang_ptr = fc_lang_cat;
    unsigned int i;
    bool found = false;
    for (i = 0; i < fc_lang_cat_sz; i++) {
        if (langCode.compare(lang_ptr->lang_code) == 0) {
            found = true;
            break;
        }
        lang_ptr++;
    }
    if (found) {
        unsigned int codePoint = 0;
        unsigned int tmp;
        unsigned int first, second = 0;
        bool inRange = false;
        FT_UInt glyphIndex;
        fullSupport = true;
        for (i = 0;;) {
            // get next codePoint
            if (inRange && codePoint < second) {
                codePoint++;
            } else {
                if (i >= lang_ptr->char_set_sz)
                    break;
                tmp = lang_ptr->char_set[i];
                if (FC_LANG_START_INTERVAL_CODE == tmp)        // code of start interval
                {
                    if (i + 2 < lang_ptr->char_set_sz) {
                        i++;
                        first = lang_ptr->char_set[i];
                        i++;
                        second = lang_ptr->char_set[i];
                        inRange = true;
                        codePoint = first;
                        i++;
                    } else {
                        // broken language char set
                        //qDebug() << "broken language char set";
                        fullSupport = false;
                        break;
                    }
                } else {
                    codePoint = tmp;
                    inRange = false;
                    i++;
                }
            }
            // check codePoint in this font
            glyphIndex = FT_Get_Char_Index(_face, codePoint);
            if (0 == glyphIndex) {
                fullSupport = false;
            } else {
                partialSupport = true;
            }
        }
        if (fullSupport)
            CRLog::debug("checkFontLangCompat(): Font have full support of language %s",
                         langCode.c_str());
        else if (partialSupport)
            CRLog::debug("checkFontLangCompat(): Font have partial support of language %s",
                         langCode.c_str());
        else
            CRLog::debug("checkFontLangCompat(): Font DON'T have support of language %s",
                         langCode.c_str());
    } else
        CRLog::debug("checkFontLangCompat(): Unsupported language code: %s", langCode.c_str());
    return fullSupport;
}

lUInt16 LVFreeTypeFace::measureText(const lChar16 *text,
                                    int len,
                                    lUInt16 *widths,
                                    lUInt8 *flags,
                                    int max_width,
                                    lChar16 def_char, TextLangCfg *lang_cfg,
                                    int letter_spacing,
                                    bool allow_hyphenation,
                                    lUInt32 hints) {
    FONT_GUARD
    if (len <= 0 || _face == NULL)
        return 0;

    if (letter_spacing < 0)
        letter_spacing = 0;
    else if ( letter_spacing > MAX_LETTER_SPACING ) {
        letter_spacing = MAX_LETTER_SPACING;
    }

    int i;

    lUInt16 prev_width = 0;
    lUInt32 lastFitChar = 0;
    updateTransform();  // no-op
    // measure character widths

#if USE_HARFBUZZ == 1
    if (_shapingMode == SHAPING_MODE_HARFBUZZ) {
        /** from harfbuzz/src/hb-buffer.h
         * hb_glyph_info_t:
         * @codepoint: either a Unicode code point (before shaping) or a glyph index
         *             (after shaping).
         * @cluster: the index of the character in the original text that corresponds
         *           to this #hb_glyph_info_t, or whatever the client passes to
         *           hb_buffer_add(). More than one #hb_glyph_info_t can have the same
         *           @cluster value, if they resulted from the same character (e.g. one
         *           to many glyph substitution), and when more than one character gets
         *           merged in the same glyph (e.g. many to one glyph substitution) the
         *           #hb_glyph_info_t will have the smallest cluster value of them.
         *           By default some characters are merged into the same cluster
         *           (e.g. combining marks have the same cluster as their bases)
         *           even if they are separate glyphs, hb_buffer_set_cluster_level()
         *           allow selecting more fine-grained cluster handling.
         */
        unsigned int glyph_count;
        hb_glyph_info_t* glyph_info = 0;
        hb_glyph_position_t* glyph_pos = 0;
        hb_buffer_clear_contents(_hb_buffer);

        // hb_buffer_set_replacement_codepoint(_hb_buffer, def_char);
        // /\ This would just set the codepoint to use when parsing
        // invalid utf8/16/32. As we provide codepoints, Harfbuzz
        // won't use it. This does NOT set the codepoint/glyph that
        // would be used when a glyph does not exist in that for that
        // codepoint. There is currently no way to specify that, and
        // it's always the .notdef/tofu glyph that is measured/drawn.

        // Fill HarfBuzz buffer
        // No need to call filterChar() on the input: HarfBuzz seems to do
        // the right thing with symbol fonts, and we'd better not replace
        // bullets & al unicode chars with generic equivalents, as they
        // may be found in the fallback font.
        // So, we don't, unless the current font has no fallback font,
        // in which case we need to get a replacement, in the worst case
        // def_char (?), because the glyph for 0/.notdef (tofu) has so
        // many different looks among fonts that it would mess the text.
        // We'll then get the '?' glyph of the fallback font only.
        // Note: not sure if Harfbuzz is able to be fine by using other
        // glyphs when the main codepoint does not exist by itself in
        // the font... in which case we'll mess things up.
        // todo: (if needed) might need a pre-pass in the fallback case:
        // full shaping without filterChar(), and if any .notdef
        // codepoint, re-shape with filterChar()...
        if ( getFallbackFont() ) { // It has a fallback font, add chars as-is
            for (i = 0; i < len; i++) {
                hb_buffer_add(_hb_buffer, (hb_codepoint_t)(text[i]), i);
            }
        }
        else { // No fallback font, check codepoint presence or get replacement char
            for (i = 0; i < len; i++) {
                hb_buffer_add(_hb_buffer, (hb_codepoint_t)filterChar(text[i], def_char), i);
            }
        }
        // Note: hb_buffer_add_codepoints(_hb_buffer, (hb_codepoint_t*)text, len, 0, len)
        // would do the same kind of loop we did above, so no speedup gain using it; and we
        // get to be sure of the cluster initial value we set to each of our added chars.
        hb_buffer_set_content_type(_hb_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);

        // If we are provided with direction and hints, let harfbuzz know
        if ( hints ) {
            if ( hints & LFNT_HINT_DIRECTION_KNOWN ) {
                if ( hints & LFNT_HINT_DIRECTION_IS_RTL )
                    hb_buffer_set_direction(_hb_buffer, HB_DIRECTION_RTL);
                else
                    hb_buffer_set_direction(_hb_buffer, HB_DIRECTION_LTR);
            }
            int hb_flags = HB_BUFFER_FLAG_DEFAULT; // (hb_buffer_flags_t won't let us do |= )
            if ( hints & LFNT_HINT_BEGINS_PARAGRAPH )
                hb_flags |= HB_BUFFER_FLAG_BOT;
            if ( hints & LFNT_HINT_ENDS_PARAGRAPH )
                hb_flags |= HB_BUFFER_FLAG_EOT;
            hb_buffer_set_flags(_hb_buffer, (hb_buffer_flags_t)hb_flags);
        }
        if ( lang_cfg ) {
            hb_buffer_set_language(_hb_buffer, lang_cfg->getHBLanguage());
        }
        // Let HB guess what's not been set (script, direction, language)
        hb_buffer_guess_segment_properties(_hb_buffer);

        // Some additional care might need to be taken, see:
        //   https://www.w3.org/TR/css-text-3/#letter-spacing-property
        if ( letter_spacing > 0 ) {
            // Don't apply letter-spacing if the script is cursive
            hb_script_t script = hb_buffer_get_script(_hb_buffer);
            if ( isHBScriptCursive(script) )
                letter_spacing = 0;
        }
        // todo: if letter_spacing, ligatures should be disabled (-liga, -clig)
        // todo: letter-spacing must not be applied at the beginning or at the end of a line
        // todo: it should be applied half-before/half-after each grapheme
        // cf in *some* minikin repositories: libs/minikin/Layout.cpp

        // Shape
        hb_shape(_hb_font, _hb_buffer, _hb_features.ptr(), _hb_features.length());

        // Harfbuzz has guessed and set a direction even if we did not provide one.
        bool is_rtl = false;
        if ( hb_buffer_get_direction(_hb_buffer) == HB_DIRECTION_RTL ) {
            is_rtl = true;
            // "For buffers in the right-to-left (RTL) or bottom-to-top (BTT) text
            // flow direction, the directionality of the buffer itself is reversed
            // for final output as a matter of design. Therefore, HarfBuzz inverts
            // the monotonic property: client programs are guaranteed that
            // monotonically increasing initial cluster values will be returned as
            // monotonically decreasing final cluster values."
            // hb_buffer_reverse_clusters() puts the advance on the last char of a
            // cluster, unlike hb_buffer_reverse() which puts it on the first, which
            // looks more natural (like it happens when LTR).
            // But hb_buffer_reverse_clusters() is required to have the clusters
            // ordered as our text indices, so we can map them back to our text.
            hb_buffer_reverse_clusters(_hb_buffer);
        }

        glyph_count = hb_buffer_get_length(_hb_buffer);
        glyph_info = hb_buffer_get_glyph_infos(_hb_buffer, 0);
        glyph_pos = hb_buffer_get_glyph_positions(_hb_buffer, 0);

        #ifdef DEBUG_MEASURE_TEXT
            printf("MTHB >>> measureText %x len %d is_rtl=%d [%s]\n", text, len, is_rtl, _faceName.c_str());
            for (i = 0; i < (int)glyph_count; i++) {
                char glyphname[32];
                hb_font_get_glyph_name(_hb_font, glyph_info[i].codepoint, glyphname, sizeof(glyphname));
                printf("MTHB g%d c%d(=t:%x) [%x %s]\tadvance=(%d,%d)", i, glyph_info[i].cluster,
                            text[glyph_info[i].cluster], glyph_info[i].codepoint, glyphname,
                            FONT_METRIC_TO_PX(glyph_pos[i].x_advance), FONT_METRIC_TO_PX(glyph_pos[i].y_advance)
                            );
                if (glyph_pos[i].x_offset || glyph_pos[i].y_offset)
                    printf("\toffset=(%d,%d)", FONT_METRIC_TO_PX(glyph_pos[i].x_offset),
                                               FONT_METRIC_TO_PX(glyph_pos[i].y_offset));
                printf("\n");
            }
            printf("MTHB ---\n");
        #endif

        // We need to set widths and flags on our original text.
        // hb_shape has modified buffer to contain glyphs, and text
        // and buffer may desync (because of clusters, ligatures...)
        // in both directions in a same run.
        // Also, a cluster must not be cut, so we want to set the same
        // width to all our original text chars that are part of the
        // same cluster (so 2nd+ chars in a cluster, will get a 0-width,
        // and, when splitting lines, will fit in a word with the first
        // char).
        // So run along our original text (chars, t), and try to follow
        // harfbuzz buffer (glyphs, hg), putting the advance of all
        // the glyphs that belong to the same cluster (hcl) on the
        // first char that started that cluster (and 0-width on the
        // followup chars).
        // It looks like Harfbuzz makes a cluster of combined glyphs
        // even when the font does not have any or all of the required
        // glyphs:
        // When meeting a not-found glyph (codepoint=0, name=".notdef"),
        // we record the original starting t of that cluster, and
        // keep processing (possibly other chars with .notdef glyphs,
        // giving them the width of the 'tofu' char), until we meet a char
        // with a found glyph. We then hold on on this one, while we go
        // measureText() the previous segment of text (that got .notdef
        // glyphs) with the fallback font, and update the wrongs width
        // and flags.

        int prev_width = 0;
        int cur_width = 0;
        int cur_cluster = 0;
        int hg = 0;  // index in glyph_info/glyph_pos
        int hcl = 0; // cluster glyph at hg
        int t_notdef_start = -1;
        int t_notdef_end = -1;
        for (int t = 0; t < len; t++) {
            #ifdef DEBUG_MEASURE_TEXT
                printf("MTHB t%d (=%x) ", t, text[t]);
            #endif
            // Grab all glyphs that do not belong to a cluster greater that our char position
            while ( hg < glyph_count ) {
                hcl = glyph_info[hg].cluster;
                if (hcl <= t) {
                    int advance = 0;
                    if ( glyph_info[hg].codepoint != 0 ) { // Codepoint found in this font
                        #ifdef DEBUG_MEASURE_TEXT
                            printf("(found cp=%x) ", glyph_info[hg].codepoint);
                        #endif
                        // Only process past notdef when the first glyph of a cluster is found.
                        // (It could happen that a cluster of 2 glyphs has its 1st one notdef
                        // while the 2nd one has a valid codepoint: we'll have to reprocess the
                        // whole cluster with the fallback font. If the 1st glyph is found but
                        // the 2nd is notdef, we'll process past notdef with the fallback font
                        // now, but we'll be processing this whole cluster with the fallback
                        // font when a later valid codepoint is found).
                        if ( t_notdef_start >= 0 && hcl > cur_cluster ) {
                            // We have a segment of previous ".notdef", and this glyph starts a new cluster
                            t_notdef_end = t;
                            LVFont *fallback = getFallbackFont();
                            // The code ensures the main fallback font has no fallback font
                            if ( fallback ) {
                                // Let the fallback font replace the wrong values in widths and flags
                                #ifdef DEBUG_MEASURE_TEXT
                                    printf("[...]\nMTHB ### measuring past failures with fallback font %d>%d\n",
                                                            t_notdef_start, t_notdef_end);
                                #endif
                                // Drop BOT/EOT flags if this segment is not at start/end
                                lUInt32 fb_hints = hints;
                                if ( t_notdef_start > 0 )
                                    fb_hints &= ~LFNT_HINT_BEGINS_PARAGRAPH;
                                if ( t_notdef_end < len )
                                    fb_hints &= ~LFNT_HINT_ENDS_PARAGRAPH;
                                fallback->measureText( text + t_notdef_start, t_notdef_end - t_notdef_start,
                                                widths + t_notdef_start, flags + t_notdef_start,
                                                max_width, def_char, lang_cfg, letter_spacing, allow_hyphenation,
                                                fb_hints );
                                // Fix previous bad measurements
                                int last_good_width = t_notdef_start > 0 ? widths[t_notdef_start-1] : 0;
                                for (int tn = t_notdef_start; tn < t_notdef_end; tn++) {
                                    widths[tn] += last_good_width;
                                }
                                // And fix our current width
                                cur_width = widths[t_notdef_end-1];
                                prev_width = cur_width;
                                #ifdef DEBUG_MEASURE_TEXT
                                    printf("MTHB ### measured past failures > W= %d\n[...]", cur_width);
                                #endif
                            }
                            else {
                                // No fallback font: stay with what's been measured: the notdef/tofu char
                                #ifdef DEBUG_MEASURE_TEXT
                                    printf("[...]\nMTHB no fallback font to measure past failures, keeping def_char\nMTHB [...]");
                                #endif
                            }
                            t_notdef_start = -1;
                            // And go on with the found glyph now that we fixed what was before
                        }
                        // Glyph found in this font
                        advance = FONT_METRIC_TO_PX(glyph_pos[hg].x_advance);
                    }
                    else {
                        #ifdef DEBUG_MEASURE_TEXT
                            printf("(glyph not found) ");
                        #endif
                        // Keep the advance of .notdef/tofu in case there is no fallback font to correct them
                        advance = FONT_METRIC_TO_PX(glyph_pos[hg].x_advance);
                        if ( t_notdef_start < 0 ) {
                            t_notdef_start = t;
                        }
                    }
                    #ifdef DEBUG_MEASURE_TEXT
                        printf("c%d+%d ", hcl, advance);
                    #endif
                    cur_width += advance;
                    cur_cluster = hcl;
                    hg++;
                    continue; // keep grabbing glyphs
                }
                break;
            }
            // Done grabbing clustered glyphs: they contributed to cur_width.
            // All 't' lower than the next cluster will have that same cur_width.
            if (cur_cluster < t) {
                // Our char is part of a cluster that started on a previous char
                flags[t] = LCHAR_IS_CLUSTER_TAIL;
                // todo: see at using HB_GLYPH_FLAG_UNSAFE_TO_BREAK to
                // set this flag instead/additionally
            }
            else {
                // We're either a single char cluster, or the start
                // of a multi chars cluster.
                flags[t] = GET_CHAR_FLAGS(text[t]);
                // It seems each soft-hyphen is in its own cluster, of length 1 and width 0,
                // so HarfBuzz must already deal correctly with soft-hyphens.
                if (cur_width == prev_width) {
                    // But if there is no advance (this happens with soft-hyphens),
                    // flag it and don't add any letter spacing.
                    flags[t] |= LCHAR_IS_CLUSTER_TAIL;
                }
                else {
                    cur_width += letter_spacing; // only between clusters/graphemes
                }            
            }
            widths[t] = cur_width;
            #ifdef DEBUG_MEASURE_TEXT
                printf("=> %d (flags=%d) => W=%d\n", cur_width - prev_width, flags[t], cur_width);
            #endif
            prev_width = cur_width;

            // (Not sure about how that max_width limit could play and if it could mess things)
            if (cur_width > max_width) {
                if (lastFitChar < hcl + 7)
                    break;
            }
            else {
                lastFitChar = t+1;
            }
        } // process next char t

        // Process .notdef glyphs at end of text (same logic as above)
        if ( t_notdef_start >= 0 ) {
            t_notdef_end = len;
            LVFont *fallback = getFallbackFont();
            if ( fallback ) {
                #ifdef DEBUG_MEASURE_TEXT
                    printf("[...]\nMTHB ### measuring past failures at EOT with fallback font %d>%d\n",
                                            t_notdef_start, t_notdef_end);
                #endif
                // Drop BOT flag if this segment is not at start (it is at end)
                lUInt32 fb_hints = hints;
                if ( t_notdef_start > 0 )
                    fb_hints &= ~LFNT_HINT_BEGINS_PARAGRAPH;
                int chars_measured = fallback->measureText( text + t_notdef_start, // start
                                t_notdef_end - t_notdef_start, // len
                                widths + t_notdef_start, flags + t_notdef_start,
                                max_width, def_char, lang_cfg, letter_spacing, allow_hyphenation,
                                fb_hints );
                lastFitChar = t_notdef_start + chars_measured;
                int last_good_width = t_notdef_start > 0 ? widths[t_notdef_start-1] : 0;
                for (int tn = t_notdef_start; tn < t_notdef_end; tn++) {
                    widths[tn] += last_good_width;
                }
                // And add all that to our current width
                cur_width = widths[t_notdef_end-1];
                #ifdef DEBUG_MEASURE_TEXT
                    printf("MTHB ### measured past failures at EOT > W= %d\n[...]", cur_width);
                #endif
            }
            else {
                #ifdef DEBUG_MEASURE_TEXT
                    printf("[...]\nMTHB no fallback font to measure past failures at EOT, keeping def_char\nMTHB [...]");
                #endif
            }
        }

        // i is used below to "fill props for rest of chars", so make it accurate
        i = len; // actually make it do nothing

        #ifdef DEBUG_MEASURE_TEXT
            printf("MTHB <<< W=%d [%s]\n", cur_width, _faceName.c_str());
            printf("MTHB dwidths[]: ");
            for (int t = 0; t < len; t++)
                printf("%d:%d ", t, widths[t] - (t>0?widths[t-1]:0));
            printf("\n");
        #endif
    } // _shapingMode == SHAPING_MODE_HARFBUZZ
    else if (_shapingMode == SHAPING_MODE_HARFBUZZ_LIGHT) {
        struct LVCharTriplet triplet;
        struct LVCharPosInfo posInfo;
        triplet.Char = 0;
        for ( i=0; i<len; i++) {
            lChar16 ch = text[i];
            bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
            if (isHyphen) {
                // do just what would be done below if zero width (no change
                // in prev_width), and don't get involved in kerning
                flags[i] = 0; // no LCHAR_ALLOW_WRAP_AFTER, will be dealt with by hyphenate()
                widths[i] = prev_width;
                lastFitChar = i + 1;
                continue;
            }
            flags[i] = GET_CHAR_FLAGS(ch); //calcCharFlags( ch );
            triplet.prevChar = triplet.Char;
            triplet.Char = ch;
            if (i < len - 1)
                triplet.nextChar = text[i + 1];
            else
                triplet.nextChar = 0;
            if (!_width_cache2.get(triplet, posInfo)) {
                if (hbCalcCharWidth(&posInfo, triplet, def_char))
                    _width_cache2.set(triplet, posInfo);
                else { // (seems this never happens, unlike with kerning disabled)
                    widths[i] = prev_width;
                    lastFitChar = i + 1;
                    continue;  /* ignore errors */
                }
            }
            widths[i] = prev_width + posInfo.width;
            if ( posInfo.width == 0 ) {
                // Assume zero advance means it's a diacritic, and we should not apply
                // any letter spacing on this char (now, and when justifying)
                flags[i] |= LCHAR_IS_CLUSTER_TAIL;
            }
            else {
                widths[i] += letter_spacing;
            }
            if ( !isHyphen ) // avoid soft hyphens inside text string
                prev_width = widths[i];
            if ( prev_width > max_width ) {
                if ( lastFitChar < i + 7)
                    break;
            }
            else {
                lastFitChar = i + 1;
            }
        }
    } else {
#endif   // USE_HARFBUZZ==1
    FT_UInt previous = 0;
    int error;
#if (ALLOW_KERNING==1)
    int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
    for ( i=0; i<len; i++) {
        lChar16 ch = text[i];
        bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
        if (isHyphen) {
            // do just what would be done below if zero width (no change
            // in prev_width), and don't get involved in kerning
            flags[i] = 0; // no LCHAR_ALLOW_WRAP_AFTER, will be dealt with by hyphenate()
            widths[i] = prev_width;
            lastFitChar = i + 1;
            continue;
        }
        FT_UInt ch_glyph_index = (FT_UInt)-1;
        int kerning = 0;
#if (ALLOW_KERNING==1)
        if ( use_kerning && previous>0  ) {
            if ( ch_glyph_index==(FT_UInt)-1 )
                ch_glyph_index = getCharIndex( ch, def_char );
            if ( ch_glyph_index != 0 ) {
                FT_Vector delta;
                error = FT_Get_Kerning( _face,          /* handle to face object */
                                        previous,          /* left glyph index      */
                                        ch_glyph_index,         /* right glyph index     */
                                        FT_KERNING_DEFAULT,  /* kerning mode          */
                                        &delta );    /* target vector         */
                if ( !error )
                    kerning = delta.x;
            }
        }
#endif
        flags[i] = GET_CHAR_FLAGS(ch); //calcCharFlags( ch );

        /* load glyph image into the slot (erase previous one) */
        int w = _wcache.get(ch);
        if ( w == CACHED_UNSIGNED_METRIC_NOT_SET ) {
            glyph_info_t glyph;
            if ( getGlyphInfo( ch, &glyph, def_char ) ) {
                w = glyph.width;
                _wcache.put(ch, w);
            } else {
                widths[i] = prev_width;
                lastFitChar = i + 1;
                continue;  /* ignore errors */
            }
        }
        if ( use_kerning ) {
            if ( ch_glyph_index==(FT_UInt)-1 )
                ch_glyph_index = getCharIndex( ch, 0 );
            previous = ch_glyph_index;
        }
        widths[i] = prev_width + w + FONT_METRIC_TO_PX(kerning);
        if ( w == 0 ) {
            // Assume zero advance means it's a diacritic, and we should not apply
            // any letter spacing on this char (now, and when justifying)
            flags[i] |= LCHAR_IS_CLUSTER_TAIL;
        }
        else {
            widths[i] += letter_spacing;
        }
        if ( !isHyphen ) // avoid soft hyphens inside text string
            prev_width = widths[i];
        if ( prev_width > max_width ) {
            if ( lastFitChar < i + 7)
                break;
        } else {
            lastFitChar = i + 1;
        }
    }

#if USE_HARFBUZZ==1
    } // else fallback to the non harfbuzz code
#endif


    // fill props for rest of chars
    for (int ii = i; ii < len; ii++) {
        flags[ii] = GET_CHAR_FLAGS(text[ii]);
    }

    //maxFit = i;

    // find last word
    if (allow_hyphenation) {
        if (!_hyphen_width)
            _hyphen_width = getCharWidth(UNICODE_SOFT_HYPHEN_CODE);
        if (lastFitChar > 3) {
            int hwStart, hwEnd;
            lStr_findWordBounds(text, len, lastFitChar - 1, hwStart, hwEnd);
            if (hwStart < (int) (lastFitChar - 1) && hwEnd > hwStart + 3) {
                //int maxw = max_width - (hwStart>0 ? widths[hwStart-1] : 0);
                if ( lang_cfg )
                    lang_cfg->getHyphMethod()->hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);
                else // Use global lang hyph method
                    HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);
            }
        }
    }
    return lastFitChar; //i;
}

lUInt32 LVFreeTypeFace::getTextWidth(const lChar16 *text, int len, TextLangCfg *lang_cfg) {
    static lUInt16 widths[MAX_LINE_CHARS + 1];
    static lUInt8 flags[MAX_LINE_CHARS + 1];
    if (len > MAX_LINE_CHARS)
        len = MAX_LINE_CHARS;
    if (len <= 0)
        return 0;
    lUInt16 res = measureText(
            text, len,
            widths,
            flags,
            MAX_LINE_WIDTH,
            L' ',  // def_char
            lang_cfg,
            0
    );
    if (res > 0 && res < MAX_LINE_CHARS)
        return widths[res - 1];
    return 0;
}

void LVFreeTypeFace::updateTransform() {
    //        static void * transformOwner = NULL;
    //        if ( transformOwner!=this ) {
    //            FT_Set_Transform(_face, &_matrix, NULL);
    //            transformOwner = this;
    //        }
}

LVFontGlyphCacheItem *LVFreeTypeFace::getGlyph(lUInt32 ch, lChar16 def_char) {
    //FONT_GUARD
    FT_UInt ch_glyph_index = getCharIndex(ch, 0);
    if (ch_glyph_index == 0) {
        LVFont *fallback = getFallbackFont();
        if (!fallback) {
            // No fallback
            ch_glyph_index = getCharIndex(ch, def_char);
            if (ch_glyph_index == 0)
                return NULL;
        } else {
            // Fallback
            return fallback->getGlyph(ch, def_char);
        }
    }
    LVFontGlyphCacheItem *item = _glyph_cache.get(ch);
    if (!item) {
        int rend_flags = FT_LOAD_RENDER | (!_drawMonochrome ? FT_LOAD_TARGET_LIGHT
                                                            : (FT_LOAD_TARGET_MONO)); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
        if (_hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR) {
            rend_flags |= FT_LOAD_NO_AUTOHINT;
        } else if (_hintingMode == HINTING_MODE_AUTOHINT) {
            rend_flags |= FT_LOAD_FORCE_AUTOHINT;
        } else if (_hintingMode == HINTING_MODE_DISABLED) {
            rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        }
        if (_embolden) { // Don't render yet
            rend_flags &= ~FT_LOAD_RENDER;
            // Also disable any hinting, as it would be wrong after embolden
            rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        }
        /* load glyph image into the slot (erase previous one) */
        updateTransform(); // no-op
        int error = FT_Load_Glyph( _face, /* handle to face object */
                ch_glyph_index,           /* glyph index           */
                rend_flags );             /* load flags, see below */
        if ( error == FT_Err_Execution_Too_Long && _hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR ) {
            // Native hinting bytecode may fail with some bad fonts: try again with no hinting
            rend_flags |= FT_LOAD_NO_HINTING;
            error = FT_Load_Glyph( _face, ch_glyph_index, rend_flags );
        }
        if ( error ) {
            return NULL;  /* ignore errors */
        }

        if (_embolden) { // Embolden and render
            // See setEmbolden() for details
            FT_GlyphSlot_Embolden(_slot);
            FT_Render_Glyph(_slot, _drawMonochrome?FT_RENDER_MODE_MONO:FT_RENDER_MODE_LIGHT);
        }

        item = newItem(&_glyph_cache, (lChar16)ch, _slot); //, _drawMonochrome
        if (item)
            _glyph_cache.put(item);
    }
    return item;
}

#if USE_HARFBUZZ == 1

LVFontGlyphCacheItem* LVFreeTypeFace::getGlyphByIndex(lUInt32 index) {
    //FONT_GUARD
    LVFontGlyphCacheItem *item = _glyph_cache2.get(index);
    if (!item) {
        // glyph not found in cache, rendering...
        int rend_flags = FT_LOAD_RENDER | (!_drawMonochrome ? FT_LOAD_TARGET_LIGHT
                                                            : (FT_LOAD_TARGET_MONO)); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
        if (_hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR) {
            rend_flags |= FT_LOAD_NO_AUTOHINT;
        }
        else if (_hintingMode == HINTING_MODE_AUTOHINT) {
            rend_flags |= FT_LOAD_FORCE_AUTOHINT;
        }
        else if (_hintingMode == HINTING_MODE_DISABLED) {
            rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        }

        if (_embolden) { // Don't render yet
            rend_flags &= ~FT_LOAD_RENDER;
            // Also disable any hinting, as it would be wrong after embolden
            rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        }

        /* load glyph image into the slot (erase previous one) */
        updateTransform(); // no-op
        int error = FT_Load_Glyph( _face, /* handle to face object */
                index,                    /* glyph index           */
                rend_flags );             /* load flags, see below */
        if ( error == FT_Err_Execution_Too_Long && _hintingMode == HINTING_MODE_BYTECODE_INTERPRETOR ) {
            // Native hinting bytecode may fail with some bad fonts: try again with no hinting
            rend_flags |= FT_LOAD_NO_HINTING;
            error = FT_Load_Glyph( _face, index, rend_flags );
        }
        if ( error ) {
            return NULL;  /* ignore errors */
        }

        if (_embolden) { // Embolden and render
            // See setEmbolden() for details
            if ( _slot->format == FT_GLYPH_FORMAT_OUTLINE ) {
                FT_Outline_Embolden(&_slot->outline, 2*_embolden_half_strength);
                FT_Outline_Translate(&_slot->outline, -_embolden_half_strength, -_embolden_half_strength);
            }
            FT_Render_Glyph(_slot, _drawMonochrome?FT_RENDER_MODE_MONO:FT_RENDER_MODE_LIGHT);
        }

        item = newItem(&_glyph_cache2, index, _slot);
        if (item)
            _glyph_cache2.put(item);
    }
    return item;
}

#endif  // USE_HARFBUZZ==1

int LVFreeTypeFace::getCharWidth(lChar16 ch, lChar16 def_char) {
    int w = _wcache.get(ch);
    if (w == CACHED_UNSIGNED_METRIC_NOT_SET) {
        glyph_info_t glyph;
        if (getGlyphInfo(ch, &glyph, def_char)) {
            w = glyph.width;
        } else {
            w = 0;
        }
        _wcache.put(ch, w);
    }
    return w;
}

int LVFreeTypeFace::getLeftSideBearing( lChar16 ch, bool negative_only, bool italic_only )
{
    if ( italic_only && !getItalic() )
        return 0;
    int b = _lsbcache.get(ch);
    if ( b == CACHED_SIGNED_METRIC_NOT_SET ) {
        glyph_info_t glyph;
        if ( getGlyphInfo( ch, &glyph, '?' ) ) {
            b = glyph.originX;
        }
        else {
            b = 0;
        }
        _lsbcache.put(ch, b);
    }
    if (negative_only && b >= 0)
        return 0;
    return b;
}

int LVFreeTypeFace::getRightSideBearing( lChar16 ch, bool negative_only, bool italic_only )
{
    if ( italic_only && !getItalic() )
        return 0;
    int b = _rsbcache.get(ch);
    if ( b == CACHED_SIGNED_METRIC_NOT_SET ) {
        glyph_info_t glyph;
        if ( getGlyphInfo( ch, &glyph, '?' ) ) {
            b = glyph.rsb;
        }
        else {
            b = 0;
        }
        _rsbcache.put(ch, b);
    }
    if (negative_only && b >= 0)
        return 0;
    return b;
}

int LVFreeTypeFace::DrawTextString(LVDrawBuf *buf, int x, int y, const lChar16 *text, int len,
                                    lChar16 def_char, lUInt32 *palette, bool addHyphen, TextLangCfg *lang_cfg,
                                    lUInt32 flags, int letter_spacing, int width, int text_decoration_back_gap) {
    FONT_GUARD
    if (len <= 0 || _face == NULL)
        return 0;
    if ( letter_spacing < 0 ) {
        letter_spacing = 0;
    }
    else if ( letter_spacing > MAX_LETTER_SPACING ) {
        letter_spacing = MAX_LETTER_SPACING;
    }
    lvRect clip;
    buf->GetClipRect(&clip);
    updateTransform(); // no-op
    if (y + _height < clip.top || y >= clip.bottom)
        return 0;

    unsigned int i;
    //lUInt16 prev_width = 0;
    lChar16 ch;
    // measure character widths
    bool isHyphen = false;
    int x0 = x;
#if USE_HARFBUZZ == 1
    if (_shapingMode == SHAPING_MODE_HARFBUZZ) {
        // See measureText() for more comments on how to work with Harfbuzz,
        // as we do and must work the same way here.
        unsigned int glyph_count;
        hb_glyph_info_t *glyph_info = 0;
        hb_glyph_position_t *glyph_pos = 0;
        hb_buffer_clear_contents(_hb_buffer);
        // Fill HarfBuzz buffer
        if ( getFallbackFont() ) { // It has a fallback font, add chars as-is
            for (i = 0; i < len; i++) {
                hb_buffer_add(_hb_buffer, (hb_codepoint_t)(text[i]), i);
            }
        }
        else { // No fallback font, check codepoint presence or get replacement char
            for (i = 0; i < len; i++) {
                hb_buffer_add(_hb_buffer, (hb_codepoint_t)filterChar(text[i], def_char), i);
            }
        }
        hb_buffer_set_content_type(_hb_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);

        // If we are provided with direction and hints, let harfbuzz know
        if ( flags ) {
            if ( flags & LFNT_HINT_DIRECTION_KNOWN ) {
                // Trust direction decided by fribidi: if we made a word containing just '(',
                // harfbuzz wouldn't be able to determine its direction and would render
                // it LTR - while it could be in some RTL text and needs to be mirrored.
                if ( flags & LFNT_HINT_DIRECTION_IS_RTL )
                    hb_buffer_set_direction(_hb_buffer, HB_DIRECTION_RTL);
                else
                    hb_buffer_set_direction(_hb_buffer, HB_DIRECTION_LTR);
            }
            int hb_flags = HB_BUFFER_FLAG_DEFAULT; // (hb_buffer_flags_t won't let us do |= )
            if ( flags & LFNT_HINT_BEGINS_PARAGRAPH )
                hb_flags |= HB_BUFFER_FLAG_BOT;
            if ( flags & LFNT_HINT_ENDS_PARAGRAPH )
                hb_flags |= HB_BUFFER_FLAG_EOT;
            hb_buffer_set_flags(_hb_buffer, (hb_buffer_flags_t)hb_flags);
        }
        if ( lang_cfg )
            hb_buffer_set_language(_hb_buffer, lang_cfg->getHBLanguage());
        // Let HB guess what's not been set (script, direction, language)
        hb_buffer_guess_segment_properties(_hb_buffer);

        // See measureText() for details
        if ( letter_spacing > 0 ) {
            // Don't apply letter-spacing if the script is cursive
            hb_script_t script = hb_buffer_get_script(_hb_buffer);
            if ( isHBScriptCursive(script) )
                letter_spacing = 0;
        }

        // Shape
        hb_shape(_hb_font, _hb_buffer, _hb_features.ptr(), _hb_features.length());

        // If direction is RTL, hb_shape() has reversed the order of the glyphs, so
        // they are in visual order and ready to be iterated and drawn. So,
        // we do not revert them, unlike in measureText().
        bool is_rtl = hb_buffer_get_direction(_hb_buffer) == HB_DIRECTION_RTL;

        glyph_count = hb_buffer_get_length(_hb_buffer);
        glyph_info = hb_buffer_get_glyph_infos(_hb_buffer, 0);
        glyph_pos = hb_buffer_get_glyph_positions(_hb_buffer, 0);

        #ifdef DEBUG_DRAW_TEXT
            printf("DTHB >>> drawTextString %x len %d is_rtl=%d [%s]\n", text, len, is_rtl, _faceName.c_str());
            for (i = 0; i < (int)glyph_count; i++) {
                char glyphname[32];
                hb_font_get_glyph_name(_hb_font, glyph_info[i].codepoint, glyphname, sizeof(glyphname));
                printf("DTHB g%d c%d(=t:%x) [%x %s]\tadvance=(%d,%d)", i, glyph_info[i].cluster,
                            text[glyph_info[i].cluster], glyph_info[i].codepoint, glyphname,
                            FONT_METRIC_TO_PX(glyph_pos[i].x_advance), FONT_METRIC_TO_PX(glyph_pos[i].y_advance));
                if (glyph_pos[i].x_offset || glyph_pos[i].y_offset)
                    printf("\toffset=(%d,%d)", FONT_METRIC_TO_PX(glyph_pos[i].x_offset),
                                               FONT_METRIC_TO_PX(glyph_pos[i].y_offset));
                printf("\n");
            }
            printf("DTHB ---\n");
        #endif

        // We want to do just like in measureText(): drawing found glyphs with
        // this font, and .notdef glyphs with the fallback font, as a single segment,
        // once a defined glyph is found, before drawing that defined glyph.
        // The code is different from in measureText(), as the glyphs might be
        // inverted for RTL drawing, and we can't uninvert them. We also loop
        // thru glyphs here rather than chars.
        int w;
        LVFont *fallback = getFallbackFont();
        bool has_fallback_font = (bool) fallback;

        // Cluster numbers may increase or decrease (if RTL) while we walk the glyphs.
        // We'll update fallback drawing text indices as we walk glyphs and cluster
        // (cluster numbers are boundaries in text indices, but it's quite tricky
        // to get right).
        int fb_t_start = 0;
        int fb_t_end = len;
        int hg = 0;  // index in glyph_info/glyph_pos
        while (hg < glyph_count) { // hg is the start of a new cluster at this point
            bool draw_with_fallback = false;
            int hcl = glyph_info[hg].cluster;
            fb_t_start = hcl; // if fb drawing needed from this glyph: t[hcl:..]
                // /\ Logical if !is_rtl, but also needed if is_rtl and immediately
                // followed by a found glyph (so, a single glyph to draw with the
                // fallback font): = hclbad
            #ifdef DEBUG_DRAW_TEXT
                printf("DTHB g%d c%d: ", hg, hcl);
            #endif
            int hg2 = hg;
            while ( hg2 < glyph_count ) {
                int hcl2 = glyph_info[hg2].cluster;
                if ( hcl2 != hcl ) { // New cluster starts at hg2: we can draw hg > hg2-1
                    #ifdef DEBUG_DRAW_TEXT
                        printf("all found, ");
                    #endif
                    if (is_rtl)
                        fb_t_end = hcl; // if fb drawing needed from next glyph: t[..:hcl]
                    break;
                }
                if ( glyph_info[hg2].codepoint != 0 || !has_fallback_font ) {
                    // Glyph found in this font, or not but we have no
                    // fallback font: we will draw the .notdef/tofu chars.
                    hg2++;
                    continue;
                }
                #ifdef DEBUG_DRAW_TEXT
                    printf("g%d c%d notdef, ", hg2, hcl2);
                #endif
                // Glyph notdef but we have a fallback font
                // Go look ahead for a complete cluster, or segment of notdef,
                // so we can draw it all with the fallback using harfbuzz
                draw_with_fallback = true;
                // We will update hg2 and hcl2 to be the last glyph of
                // a cluster/segment with notdef
                int hclbad = hcl2;
                int hclgood = -1;
                int hg3 = hg2+1;
                while ( hg3 < glyph_count ) {
                    int hcl3 = glyph_info[hg3].cluster;
                    if ( hclgood >=0 && hcl3 != hclgood ) {
                        // Found a complete cluster
                        // We can draw hg > hg2-1 with fallback font
                        #ifdef DEBUG_DRAW_TEXT
                            printf("c%d complete, need redraw up to g%d", hclgood, hg2);
                        #endif
                        if (!is_rtl)
                            fb_t_end = hclgood; // fb drawing t[..:hclgood]
                        hg2 += 1; // set hg2 to the first ok glyph
                        break;
                    }
                    if ( glyph_info[hg3].codepoint == 0 || hcl3 == hclbad) {
                        #ifdef DEBUG_DRAW_TEXT
                            printf("g%d c%d -, ", hg3, hcl3);
                        #endif
                        // notdef, or def but part of uncomplete previous cluster
                        hcl2 = hcl3;
                        hg2 = hg3; // move hg2 to this bad glyph
                        hclgood = -1; // un'good found good cluster
                        hclbad = hcl3;
                        if (is_rtl)
                            fb_t_start = hclbad; // fb drawing t[hclbad::..]
                        hg3++;
                        continue;
                    }
                    // Codepoint found, and we're not part of an uncomplete cluster
                    #ifdef DEBUG_DRAW_TEXT
                        printf("g%d c%d +, ", hg3, hcl3);
                    #endif
                    hclgood = hcl3;
                    hg3++;
                }
                if ( hg3 == glyph_count && hclgood >=0 ) { // last glyph was a good cluster
                    if (!is_rtl)
                        fb_t_end = hclgood; // fb drawing t[..:hclgood]
                    hg2 += 1; // set hg2 to the first ok glyph (so, the single last one)
                    break;
                }
                if ( hg3 == glyph_count ) { // no good cluster met till end of text
                    hg2 = glyph_count; // get out of hg2 loop
                    if (is_rtl)
                        fb_t_start = 0;
                    else
                        fb_t_end = len;
                }
                break;
            }
            // Draw glyphs from hg to hg2 excluded
            if (draw_with_fallback) {
                #ifdef DEBUG_DRAW_TEXT
                    printf("[...]\nDTHB ### drawing past notdef with fallback font %d>%d ", hg, hg2);
                    printf(" => %d > %d\n", fb_t_start, fb_t_end);
                #endif
                // Adjust DrawTextString() params for fallback drawing
                lUInt32 fb_flags = flags;
                fb_flags &= ~LFNT_DRAW_DECORATION_MASK; // main font will do text decoration
                // We must keep direction, but we should drop BOT/EOT flags
                // if this segment is not at start/end (this might be bogus
                // if the char at start or end is a space that could be drawn
                // with the main font).
                if (fb_t_start > 0)
                    fb_flags &= ~LFNT_HINT_BEGINS_PARAGRAPH;
                if (fb_t_end < len)
                    fb_flags &= ~LFNT_HINT_ENDS_PARAGRAPH;
                // Adjust fallback y so baselines of both fonts match
                int fb_y = y + _baseline - fallback->getBaseline();
                bool fb_addHyphen = false; // will be added by main font
                const lChar16 * fb_text = text + fb_t_start;
                int fb_len = fb_t_end - fb_t_start;
                // (width and text_decoration_back_gap are only used for
                // text decoration, that we dropped: no update needed)
                int fb_advance = fallback->DrawTextString( buf, x, fb_y,
                   fb_text, fb_len,
                   def_char, palette, fb_addHyphen, lang_cfg, fb_flags, letter_spacing,
                   width, text_decoration_back_gap );
                x += fb_advance;
                #ifdef DEBUG_DRAW_TEXT
                    printf("DTHB ### drawn past notdef > X+= %d\n[...]", fb_advance);
                #endif
            }
            else {
                #ifdef DEBUG_DRAW_TEXT
                    printf("regular g%d>%d: ", hg, hg2);
                #endif
                // Draw glyphs of this same cluster
                int prev_x = x;
                for (i = hg; i < hg2; i++) {
                    LVFontGlyphCacheItem *item = getGlyphByIndex(glyph_info[i].codepoint);
                    if (item) {
                        int w = FONT_METRIC_TO_PX(glyph_pos[i].x_advance);
                        #ifdef DEBUG_DRAW_TEXT
                            printf("%x(x=%d+%d,w=%d) ", glyph_info[i].codepoint, x,
                                    item->origin_x + FONT_METRIC_TO_PX(glyph_pos[i].x_offset), w);
                        #endif
                        buf->Draw(x + item->origin_x + FONT_METRIC_TO_PX(glyph_pos[i].x_offset),
                                  y + _baseline - item->origin_y - FONT_METRIC_TO_PX(glyph_pos[i].y_offset),
                                  item->bmp,
                                  item->bmp_width,
                                  item->bmp_height,
                                  palette);
                        x += w;
                    }
                    #ifdef DEBUG_DRAW_TEXT
                    else
                        printf("SKIPPED %x", glyph_info[i].codepoint);
                    #endif
                }
                // Whole cluster drawn: add letter spacing
                if ( x > prev_x ) {
                    // But only if this cluster has some advance
                    // (e.g. a soft-hyphen makes its own cluster, that
                    // draws a space glyph, but with no advance)
                    x += letter_spacing;
                }
            }
            hg = hg2;
            #ifdef DEBUG_DRAW_TEXT
                printf("\n");
            #endif
        }

        // Wondered if item->origin_x and glyph_pos[hg].x_offset must really
        // be added (harfbuzz' x_offset correcting Freetype's origin_x),
        // or are the same thing (harfbuzz' x_offset=0 replacing and
        // cancelling FreeType's origin_x) ?
        // Comparing screenshots seems to indicate they must be added.

        if (addHyphen) {
            ch = UNICODE_SOFT_HYPHEN_CODE;
            LVFontGlyphCacheItem *item = getGlyph(ch, def_char);
            if (item) {
                w = item->advance;
                buf->Draw( x + item->origin_x,
                           y + _baseline - item->origin_y,
                           item->bmp,
                           item->bmp_width,
                           item->bmp_height,
                           palette);
                x  += w; // + letter_spacing; (let's not add any letter-spacing after hyphen)
            }
        }
    } // _shapingMode == SHAPING_MODE_HARFBUZZ
    else if (_shapingMode == SHAPING_MODE_HARFBUZZ_LIGHT) {
        struct LVCharTriplet triplet;
        struct LVCharPosInfo posInfo;
        triplet.Char = 0;
        bool is_rtl = (flags & LFNT_HINT_DIRECTION_KNOWN) && (flags & LFNT_HINT_DIRECTION_IS_RTL);
        for ( i=0; i<=(unsigned int)len; i++) {
            if ( i==len && !addHyphen )
                break;
            if ( i<len ) {
                // If RTL, draw glyphs starting from the of the node text
                ch = is_rtl ? text[len-1-i] : text[i];
                if ( ch=='\t' )
                    ch = ' ';
                // don't draw any soft hyphens inside text string
                isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
            }
            else {
                ch = UNICODE_SOFT_HYPHEN_CODE;
                isHyphen = false; // an hyphen, but not one to not draw
            }
            LVFontGlyphCacheItem * item = getGlyph(ch, def_char);
            if ( !item )
                continue;
            if ( (item && !isHyphen) || i==len ) { // only draw soft hyphens at end of string
                triplet.prevChar = triplet.Char;
                triplet.Char = ch;
                if (i < (unsigned int)(len - 1))
                    triplet.nextChar = is_rtl ? text[len-1-i-1] : text[i + 1];
                else
                    triplet.nextChar = 0;
                if (!_width_cache2.get(triplet, posInfo)) {
                    if (!hbCalcCharWidth(&posInfo, triplet, def_char)) {
                        posInfo.offset = 0;
                        posInfo.width = item->advance;
                    }
                    _width_cache2.set(triplet, posInfo);
                }
                buf->Draw(x + item->origin_x + posInfo.offset,
                    y + _baseline - item->origin_y,
                    item->bmp,
                    item->bmp_width,
                    item->bmp_height,
                    palette);

                x += posInfo.width + letter_spacing;
            }
        }
    } else {
#endif // USE_HARFBUZZ
    FT_UInt previous = 0;
    int error;
#if (ALLOW_KERNING==1)
    int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
    for ( i=0; i<=len; i++) {
        if ( i==len && (!addHyphen || isHyphen) )
            break;
        if ( i<len ) {
            ch = text[i];
            if ( ch=='\t' )
                ch = ' ';
            isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE) && (i<len-1);
        } else {
            ch = UNICODE_SOFT_HYPHEN_CODE;
            isHyphen = 0;
        }
        FT_UInt ch_glyph_index = getCharIndex( ch, def_char );
        int kerning = 0;
#if (ALLOW_KERNING==1)
        if ( use_kerning && previous>0 && ch_glyph_index>0 ) {
            FT_Vector delta;
            error = FT_Get_Kerning( _face,          /* handle to face object */
                                    previous,          /* left glyph index      */
                                    ch_glyph_index,         /* right glyph index     */
                                    FT_KERNING_DEFAULT,  /* kerning mode          */
                                    &delta );    /* target vector         */
            if ( !error )
                kerning = delta.x;
        }
#endif
        LVFontGlyphCacheItem * item = getGlyph(ch, def_char);
        if ( !item )
            continue;
        if ( (item && !isHyphen) || i>=len-1 ) { // avoid soft hyphens inside text string
            int w = item->advance + (kerning >> 6);
            buf->Draw( x + (kerning>>6) + item->origin_x,
                       y + _baseline - item->origin_y,
                       item->bmp,
                       item->bmp_width,
                       item->bmp_height,
                       palette);

            x  += w + letter_spacing;
            previous = ch_glyph_index;
        }
    }

#if USE_HARFBUZZ==1
    } // else fallback to the non harfbuzz code
#endif

    int advance = x - x0;
    if ( flags & LFNT_DRAW_DECORATION_MASK ) {
        // text decoration: underline, etc.
        // Don't overflow the provided width (which may be lower than our
        // pen x if last glyph was a space not accounted in word width)
        if ( width >= 0 && x > x0 + width)
            x = x0 + width;
        // And start the decoration before x0 if it is continued
        // from previous word
        x0 -= text_decoration_back_gap;
        int h = _size > 30 ? 2 : 1;
        lUInt32 cl = buf->GetTextColor();
        if ( (flags & LFNT_DRAW_UNDERLINE) || (flags & LFNT_DRAW_BLINK) ) {
            int liney = y + _baseline + h;
            buf->FillRect( x0, liney, x, liney+h, cl );
        }
        if ( flags & LFNT_DRAW_OVERLINE ) {
            int liney = y + h;
            buf->FillRect( x0, liney, x, liney+h, cl );
        }
        if ( flags & LFNT_DRAW_LINE_THROUGH ) {
            // int liney = y + _baseline - _size/4 - h/2;
            int liney = y + _baseline - _size*2/7;
            buf->FillRect( x0, liney, x, liney+h, cl );
        }
    }
    return advance;
}

void LVFreeTypeFace::Clear() {
    LVLock lock(_mutex);
    clearCache();
#if USE_HARFBUZZ == 1
    if (_hb_font) {
        hb_font_destroy(_hb_font);
        _hb_font = 0;
    }
#endif
    if (_face) {
        FT_Done_Face(_face);
        _face = NULL;
    }
}

#endif  // (USE_FREETYPE==1)
