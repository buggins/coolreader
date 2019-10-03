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
        memcpy(item->bmp, bitmap->buffer, w * h);
        // correct gamma
        if (gammaIndex != GAMMA_LEVELS / 2)
            cr_correct_gamma_buf(item->bmp, w * h, gammaIndex);
//            }
    }
    item->origin_x = (lInt16) slot->bitmap_left;
    item->origin_y = (lInt16) slot->bitmap_top;
    item->advance = (lUInt16) (myabs(slot->metrics.horiAdvance) >> 6);
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
        memcpy(item->bmp, bitmap->buffer, w * h);
        // correct gamma
        if (gammaIndex != GAMMA_LEVELS / 2)
            cr_correct_gamma_buf(item->bmp, w * h, gammaIndex);
//            }
    }
    item->origin_x = (lInt16) slot->bitmap_left;
    item->origin_y = (lInt16) slot->bitmap_top;
    item->advance = (lUInt16) (myabs(slot->metrics.horiAdvance) >> 6);
    return item;
}

#endif

static lUInt16 char_flags[] = {
        0, 0, 0, 0, 0, 0, 0, 0, // 0    00
        0, 0, LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER, 0, 0,
        LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER, 0, 0, // 8    08
        0, 0, 0, 0, 0, 0, 0, 0, // 16   10
        0, 0, 0, 0, 0, 0, 0, 0, // 24   18
        LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER, 0, 0, 0, 0, 0, 0, 0, // 32   20
        0, 0, 0, 0, 0, LCHAR_DEPRECATED_WRAP_AFTER, 0, 0, // 40   28
        0, 0, 0, 0, 0, 0, 0, 0, // 48   30
};

#define GET_CHAR_FLAGS(ch) \
     (ch<48?char_flags[ch]: \
        (ch==UNICODE_SOFT_HYPHEN_CODE?LCHAR_ALLOW_WRAP_AFTER: \
        (ch==UNICODE_NO_BREAK_SPACE?LCHAR_DEPRECATED_WRAP_AFTER|LCHAR_IS_SPACE: \
        (ch==UNICODE_HYPHEN?LCHAR_DEPRECATED_WRAP_AFTER:0))))

#if USE_HARFBUZZ == 1

struct LVCharTriplet {
    lChar16 prevChar;
    lChar16 Char;
    lChar16 nextChar;

    bool operator==(const struct LVCharTriplet &other) {
        return prevChar == other.prevChar && Char == other.Char && nextChar == other.nextChar;
    }
};

struct LVCharPosInfo {
    int offset;
    int width;
};

inline lUInt32 getHash(const struct LVCharTriplet &triplet) {
    //return (triplet.prevChar * 1975317 + 164521) ^ (triplet.Char * 1975317 + 164521) ^ (triplet.nextChar * 1975317 + 164521);
    return getHash((lUInt64) triplet.Char
                   + (((lUInt64) triplet.prevChar) << 16)
                   + (((lUInt64) triplet.nextChar) << 32));
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
    if (fontMan->GetFallbackFontFace() !=
        _faceName) // to avoid circular link, disable fallback for fallback font
        _fallbackFont = fontMan->GetFallbackFont(_size);
    _fallbackFontIsSet = true;
    return _fallbackFont.get();
}

LVFreeTypeFace::LVFreeTypeFace(LVMutex &mutex, FT_Library library,
                               LVFontGlobalGlyphCache *globalCache)
        : _mutex(mutex), _fontFamily(css_ff_sans_serif), _library(library), _face(NULL), _size(0),
          _hyphen_width(0), _baseline(0), _weight(400), _italic(0), _glyph_cache(globalCache),
          _drawMonochrome(false), _allowKerning(false), _allowLigatures(false),
          _hintingMode(HINTING_MODE_AUTOHINT), _fallbackFontIsSet(false)
#if USE_HARFBUZZ == 1
        , _glyph_cache2(globalCache), _width_cache2(1024)
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
    _hb_opt_kern_buffer = hb_buffer_create();
    // HarfBuzz features for full text shaping
    hb_feature_from_string("-kern", -1, &_hb_features[0]);      // font kerning
    hb_feature_from_string("-liga", -1, &_hb_features[1]);      // ligatures

    // HarfBuzz features for lighweight characters width calculating with caching
    hb_feature_from_string("-kern", -1,
                           &_hb_opt_kern_features[0]);  // Kerning: Fine horizontal positioning of one glyph to the next, based on the shapes of the glyphs
    // We can enable these ones:
    hb_feature_from_string("+mark", -1,
                           &_hb_opt_kern_features[1]);  // Mark Positioning: Fine positioning of a mark glyph to a base character
    hb_feature_from_string("+mkmk", -1,
                           &_hb_opt_kern_features[2]);  // Mark-to-mark Positioning: Fine positioning of a mark glyph to another mark character
    hb_feature_from_string("+curs", -1,
                           &_hb_opt_kern_features[3]);  // Cursive Positioning: Precise positioning of a letter's connection to an adjacent one
    hb_feature_from_string("+locl", -1,
                           &_hb_opt_kern_features[4]);  // Substitutes character with the preferred form based on script language

    // We should disable these ones:
    hb_feature_from_string("-liga", -1,
                           &_hb_opt_kern_features[5]);  // Standard Ligatures: replaces (by default) sequence of characters with a single ligature glyph
    hb_feature_from_string("-rlig", -1,
                           &_hb_opt_kern_features[6]);  // Ligatures required for correct text display (any script, but in cursive) - Arabic, semitic
    hb_feature_from_string("-clig", -1,
                           &_hb_opt_kern_features[7]);  // Applies a second ligature feature based on a match of a character pattern within a context of surrounding patterns
    hb_feature_from_string("-ccmp", -1,
                           &_hb_opt_kern_features[8]);  // Glyph composition/decomposition: either calls a ligature replacement on a sequence of characters or replaces a character with a sequence of glyphs
    // Provides logic that can for example effectively alter the order of input characters
    hb_feature_from_string("-calt", -1,
                           &_hb_opt_kern_features[9]);  // Contextual Alternates: Applies a second substitution feature based on a match of a character pattern within a context of surrounding patterns
    hb_feature_from_string("-rclt", -1,
                           &_hb_opt_kern_features[10]); // Required Contextual Alternates: Contextual alternates required for correct text display which differs from the default join for other letters, required especially important by Arabic
    hb_feature_from_string("-rvrn", -1,
                           &_hb_opt_kern_features[11]); // Required Variation Alternates: Special variants of a single character, which need apply to specific font variation, required by variable fonts
    hb_feature_from_string("-ltra", -1,
                           &_hb_opt_kern_features[12]); // Left-to-right glyph alternates: Replaces characters with forms befitting left-to-right presentation
    hb_feature_from_string("-ltrm", -1,
                           &_hb_opt_kern_features[13]); // Left-to-right mirrored forms: Replaces characters with possibly mirrored forms befitting left-to-right presentation
    hb_feature_from_string("-rtla", -1,
                           &_hb_opt_kern_features[14]); // Right-to-left glyph alternates: Replaces characters with forms befitting right-to-left presentation
    hb_feature_from_string("-rtlm", -1,
                           &_hb_opt_kern_features[15]); // Right-to-left mirrored forms: Replaces characters with possibly mirrored forms befitting right-to-left presentation
    hb_feature_from_string("-frac", -1,
                           &_hb_opt_kern_features[16]); // Fractions: Converts figures separated by slash with diagonal fraction
    hb_feature_from_string("-numr", -1,
                           &_hb_opt_kern_features[17]); // Numerator: Converts to appropriate fraction numerator form, invoked by frac
    hb_feature_from_string("-dnom", -1,
                           &_hb_opt_kern_features[18]); // Denominator: Converts to appropriate fraction denominator form, invoked by frac
    hb_feature_from_string("-rand", -1,
                           &_hb_opt_kern_features[19]); // Replaces character with random forms (meant to simulate handwriting)
    hb_feature_from_string("-trak", -1, &_hb_opt_kern_features[20]); // Tracking (?)
    hb_feature_from_string("-vert", -1, &_hb_opt_kern_features[21]); // Vertical (?)
    // Especially needed with FreeSerif and french texts: -ccmp
    // Especially needed with Fedra Serif and "The", "Thuringe": -calt
    // These tweaks seem fragile (adding here +smcp to experiment with small caps would break FreeSerif again).
    // So, when tuning these, please check it still behave well with FreeSerif.
#endif
}

LVFreeTypeFace::~LVFreeTypeFace() {
#if USE_HARFBUZZ == 1
    if (_hb_buffer)
        hb_buffer_destroy(_hb_buffer);
    if (_hb_opt_kern_buffer)
        hb_buffer_destroy(_hb_opt_kern_buffer);
#endif
    Clear();
}

void LVFreeTypeFace::clearCache() {
    _glyph_cache.clear();
    _wcache.clear();
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
#if USE_HARFBUZZ == 1
    if (_allowKerning) {
        hb_feature_from_string("+kern", -1, &_hb_features[0]);
        hb_feature_from_string("+kern", -1, &_hb_opt_kern_features[0]);
    } else {
        hb_feature_from_string("-kern", -1, &_hb_features[0]);
        hb_feature_from_string("-kern", -1, &_hb_opt_kern_features[0]);
    }
    // in cache may be found some ligatures, so clear it
    clearCache();
#endif
}

void LVFreeTypeFace::setLigatures(bool ligaturesAllowed) {
    // don't touch _hb_opt_kern_features here - lightweight characters width calculation always performed without any ligatures!
    _allowLigatures = ligaturesAllowed;
#if USE_HARFBUZZ == 1
    if (_allowLigatures)
        hb_feature_from_string("+liga", -1, &_hb_features[1]);
    else
        hb_feature_from_string("-liga", -1, &_hb_features[1]);
    // in cache may be found some ligatures, so clear it
    clearCache();
#endif
}

void LVFreeTypeFace::setHintingMode(hinting_mode_t mode) {
    if (_hintingMode == mode)
        return;
    _hintingMode = mode;
    clearCache();
}

void LVFreeTypeFace::setBitmapMode(bool drawBitmap) {
    if (_drawMonochrome == drawBitmap)
        return;
    _drawMonochrome = drawBitmap;
    clearCache();
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
        if (!_hb_font)
            error = FT_Err_Invalid_Argument;
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
    _height = _face->size->metrics.height >> 6;
    _size = size; //(_face->size->metrics.height >> 6);
    _baseline = _height + (_face->size->metrics.descender >> 6);
    _weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
    _italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;

    if (!error && italicize && !_italic) {
        _matrix.xy = 0x10000 * 3 / 10;
        FT_Set_Transform(_face, &_matrix, NULL);
        _italic = true;
    }

    if (error) {
        // error
        return false;
    }
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
        if (!_hb_font)
            error = FT_Err_Invalid_Argument;
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
    _height = _face->size->metrics.height >> 6;
    _size = size; //(_face->size->metrics.height >> 6);
    _baseline = _height + (_face->size->metrics.descender >> 6);
    _weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
    _italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;

    if (!error && italicize && !_italic) {
        _matrix.xy = 0x10000 * 3 / 10;
        FT_Set_Transform(_face, &_matrix, NULL);
        _italic = true;
    }

    if (error) {
        // error
        return false;
    }
    return true;
}

#if USE_HARFBUZZ == 1

lChar16 LVFreeTypeFace::filterChar(lChar16 code) {
    lChar16 res;
    if (code == '\t')
        code = ' ';
    FT_UInt ch_glyph_index = FT_Get_Char_Index(_face, code);
    if (0 != ch_glyph_index)
        res = code;
    else {
        res = getReplacementChar(code);
        if (0 == res)
            res = code;
    }
    return res;
}

bool LVFreeTypeFace::hbCalcCharWidth(LVCharPosInfo *posInfo, const LVCharTriplet &triplet,
                                     lChar16 def_char) {
    if (!posInfo)
        return false;
    unsigned int segLen = 0;
    int cluster;
    hb_buffer_clear_contents(_hb_opt_kern_buffer);
    if (0 != triplet.prevChar) {
        hb_buffer_add(_hb_opt_kern_buffer, (hb_codepoint_t) triplet.prevChar, segLen);
        segLen++;
    }
    hb_buffer_add(_hb_opt_kern_buffer, (hb_codepoint_t) triplet.Char, segLen);
    cluster = segLen;
    segLen++;
    if (0 != triplet.nextChar) {
        hb_buffer_add(_hb_opt_kern_buffer, (hb_codepoint_t) triplet.nextChar, segLen);
        segLen++;
    }
    hb_buffer_set_content_type(_hb_opt_kern_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
    hb_buffer_guess_segment_properties(_hb_opt_kern_buffer);
    hb_shape(_hb_font, _hb_opt_kern_buffer, _hb_opt_kern_features, 22);
    unsigned int glyph_count = hb_buffer_get_length(_hb_opt_kern_buffer);
    if (segLen == glyph_count) {
        hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(_hb_opt_kern_buffer, &glyph_count);
        hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(_hb_opt_kern_buffer,
                                                                       &glyph_count);
        if (0 != glyph_info[cluster].codepoint) {        // glyph found for this char in this font
            posInfo->offset = glyph_pos[cluster].x_offset >> 6;
            posInfo->width = glyph_pos[cluster].x_advance >> 6;
        } else {
            // hb_shape() failed or glyph omitted in this font, use fallback font
            glyph_info_t glyph;
            LVFont *fallback = getFallbackFont();
            if (fallback) {
                if (fallback->getGlyphInfo(triplet.Char, &glyph, def_char)) {
                    posInfo->offset = 0;
                    posInfo->width = glyph.width;
                } else {
                    posInfo->offset = 0;
                    posInfo->width = _size;
                }
            } else {
                if (getGlyphInfo(def_char, &glyph, 0)) {
                    posInfo->offset = 0;
                    posInfo->width = glyph.width;
                } else {
                    posInfo->offset = 0;
                    posInfo->width = _size;
                }
            }
        }
    } else {
#ifdef _DEBUG
        CRLog::debug("hbCalcCharWidthWithKerning(): hb_buffer_get_length() return %d, must be %d, return value (-1)", glyph_count, segLen);
#endif
        return false;
    }
    return true;
}

#endif  // USE_HARFBUZZ==1

FT_UInt LVFreeTypeFace::getCharIndex(lChar16 code, lChar16 def_char) {
    if (code == '\t')
        code = ' ';
    FT_UInt ch_glyph_index = FT_Get_Char_Index(_face, code);
    if (ch_glyph_index == 0) {
        lUInt16 replacement = getReplacementChar(code);
        if (replacement)
            ch_glyph_index = FT_Get_Char_Index(_face, replacement);
        if (ch_glyph_index == 0 && def_char)
            ch_glyph_index = FT_Get_Char_Index(_face, def_char);
    }
    return ch_glyph_index;
}

bool LVFreeTypeFace::getGlyphInfo(lUInt16 code, LVFont::glyph_info_t *glyph, lChar16 def_char) {
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
    if (_hintingMode == HINTING_MODE_AUTOHINT)
        flags |= FT_LOAD_FORCE_AUTOHINT;
    else if (_hintingMode == HINTING_MODE_DISABLED)
        flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
    updateTransform();
    int error = FT_Load_Glyph(
            _face,          /* handle to face object */
            glyph_index,   /* glyph index           */
            flags);  /* load flags, see below */
    if (error)
        return false;
    glyph->blackBoxX = (lUInt8) (_slot->metrics.width >> 6);
    glyph->blackBoxY = (lUInt8) (_slot->metrics.height >> 6);
    glyph->originX = (lInt8) (_slot->metrics.horiBearingX >> 6);
    glyph->originY = (lInt8) (_slot->metrics.horiBearingY >> 6);
    glyph->width = (lUInt8) (myabs(_slot->metrics.horiAdvance) >> 6);
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

lUInt16 LVFreeTypeFace::measureText(const lChar16 *text, int len, lUInt16 *widths, lUInt8 *flags,
                                    int max_width, lChar16 def_char, int letter_spacing,
                                    bool allow_hyphenation) {
    FONT_GUARD
    if (len <= 0 || _face == NULL)
        return 0;

    if (letter_spacing < 0 || letter_spacing > 50)
        letter_spacing = 0;

    int i;

    lUInt16 prev_width = 0;
    lUInt32 lastFitChar = 0;
    updateTransform();
    // measure character widths
#if USE_HARFBUZZ == 1
    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info = 0;
    hb_glyph_position_t *glyph_pos = 0;
    if (/*_allowKerning &&*/ _allowLigatures) {
        // Full shaping with HarfBuzz, very slow method but support ligatures.
        hb_buffer_clear_contents(_hb_buffer);
        hb_buffer_set_replacement_codepoint(_hb_buffer, def_char);
        // fill HarfBuzz buffer with filtering
        for (i = 0; i < len; i++)
            hb_buffer_add(_hb_buffer, (hb_codepoint_t) filterChar(text[i]), i);
        hb_buffer_set_content_type(_hb_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
        hb_buffer_guess_segment_properties(_hb_buffer);
        // shape
        hb_shape(_hb_font, _hb_buffer, _hb_features, 2);
        glyph_count = hb_buffer_get_length(_hb_buffer);
        glyph_info = hb_buffer_get_glyph_infos(_hb_buffer, 0);
        glyph_pos = hb_buffer_get_glyph_positions(_hb_buffer, 0);
#ifdef _DEBUG
        if ((int)glyph_count != len) {
            CRLog::debug(
                        "measureText(): glyph_count not equal source text length (ligature detected?), glyph_count=%d, len=%d",
                        glyph_count, len);
        }
#endif
        uint32_t j;
        uint32_t cluster;
        uint32_t prev_cluster = 0;
        int skipped_chars = 0; // to add to 'i' at end of loop, as 'i' is used later and should be accurate
        for (i = 0; i < (int) glyph_count; i++) {
            cluster = glyph_info[i].cluster;
            lChar16 ch = text[cluster];
            bool isHyphen = (ch == UNICODE_SOFT_HYPHEN_CODE);
            flags[cluster] = GET_CHAR_FLAGS(ch); //calcCharFlags( ch );
            hb_codepoint_t ch_glyph_index = glyph_info[i].codepoint;
            if (0 != ch_glyph_index)        // glyph found for this char in this font
                widths[cluster] = prev_width + (glyph_pos[i].x_advance >> 6) + letter_spacing;
            else {
                // hb_shape() failed or glyph skipped in this font, use fallback font
                int w = _wcache.get(ch);
                if (0xFF == w) {
                    glyph_info_t glyph;
                    LVFont *fallback = getFallbackFont();
                    if (fallback) {
                        if (fallback->getGlyphInfo(ch, &glyph, def_char)) {
                            w = glyph.width;
                            _wcache.put(ch, w);
                        } else {
                            w = _size;
                            _wcache.put(ch, w);
                        }
                    } else {
                        // use def_char if possible
                        if (getGlyphInfo(def_char, &glyph, 0)) {
                            w = glyph.width;
                            _wcache.put(ch, w);
                        } else {
                            w = _size;
                            _wcache.put(ch, w);
                        }
                    }
                }
                widths[cluster] = prev_width + w + letter_spacing;
            }
            for (j = prev_cluster + 1; j < cluster; j++) {
                flags[j] = GET_CHAR_FLAGS(text[j]);
                widths[j] = prev_width;        // for chars replaced by ligature, so next chars of a ligature has width=0
                skipped_chars++;
            }
            prev_cluster = cluster;
            if (!isHyphen) // avoid soft hyphens inside text string
                prev_width = widths[cluster];
            if (prev_width > max_width) {
                if (lastFitChar < cluster + 7)
                    break;
            } else {
                lastFitChar = cluster + 1;
            }
        }
        // For case when ligature is the last glyph in measured text
        if (prev_cluster < (uint32_t) (len - 1) && prev_width < (lUInt16) max_width) {
            for (j = prev_cluster + 1; j < (uint32_t) len; j++) {
                flags[j] = GET_CHAR_FLAGS(text[j]);
                widths[j] = prev_width;
                skipped_chars++;
            }
        }
        // i is used below to "fill props for rest of chars", so make it accurate
        i += skipped_chars;
    } else {
        // no ligatures, no HarfBuzz text shaping, only individual char shaping in hbCalcCharWidth()
        struct LVCharTriplet triplet;
        struct LVCharPosInfo posInfo;
        triplet.Char = 0;
        for (i = 0; i < len; i++) {
            lChar16 ch = text[i];
            bool isHyphen = (ch == UNICODE_SOFT_HYPHEN_CODE);

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
                else {
                    posInfo.offset = 0;
                    posInfo.width = prev_width;
                    lastFitChar = i + 1;
                    continue;  /* ignore errors */
                }
            }
            widths[i] = prev_width + posInfo.width + letter_spacing;
            if (!isHyphen) // avoid soft hyphens inside text string
                prev_width = widths[i];
            if (prev_width > max_width) {
                if (lastFitChar < (uint32_t) (i + 7))
                    break;
            } else {
                lastFitChar = i + 1;
            }
        }
    }
#else   // USE_HARFBUZZ==1
    FT_UInt previous = 0;
    int error;
#if (ALLOW_KERNING==1)
    int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
    for ( i=0; i<len; i++) {
        lChar16 ch = text[i];
        bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
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
        if ( w==0xFF ) {
            glyph_info_t glyph;
            if ( getGlyphInfo( ch, &glyph, def_char ) ) {
                w = glyph.width;
                _wcache.put(ch, w);
            } else {
                widths[i] = prev_width;
                lastFitChar = i + 1;
                continue;  /* ignore errors */
            }
            if ( ch_glyph_index==(FT_UInt)-1 )
                ch_glyph_index = getCharIndex( ch, 0 );
            //                error = FT_Load_Glyph( _face,          /* handle to face object */
            //                        ch_glyph_index,                /* glyph index           */
            //                        FT_LOAD_DEFAULT );             /* load flags, see below */
            //                if ( error ) {
            //                    widths[i] = prev_width;
            //                    continue;  /* ignore errors */
            //                }
        }
        widths[i] = prev_width + w + (kerning >> 6) + letter_spacing;
        previous = ch_glyph_index;
        if ( !isHyphen ) // avoid soft hyphens inside text string
            prev_width = widths[i];
        if ( prev_width > max_width ) {
            if ( lastFitChar < (lUInt32)(i + 7))
                break;
        } else {
            lastFitChar = i + 1;
        }
    }
#endif  // USE_HARFBUZZ==1

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
                HyphMan::hyphenate(text + hwStart, hwEnd - hwStart, widths + hwStart,
                                   flags + hwStart, _hyphen_width, max_width);
            }
        }
    }
    return lastFitChar; //i;
}

lUInt32 LVFreeTypeFace::getTextWidth(const lChar16 *text, int len) {
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
            2048, // max_width,
            L' ',  // def_char
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

LVFontGlyphCacheItem *LVFreeTypeFace::getGlyph(lUInt16 ch, lChar16 def_char) {
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
    LVFontGlyphCacheItem *item = _glyph_cache.getByChar(ch);
    if (!item) {

        int rend_flags = FT_LOAD_RENDER | (!_drawMonochrome ? FT_LOAD_TARGET_NORMAL
                                                            : (FT_LOAD_TARGET_MONO)); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
        if (_hintingMode == HINTING_MODE_AUTOHINT)
            rend_flags |= FT_LOAD_FORCE_AUTOHINT;
        else if (_hintingMode == HINTING_MODE_DISABLED)
            rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        /* load glyph image into the slot (erase previous one) */

        updateTransform();
        int error = FT_Load_Glyph(_face,          /* handle to face object */
                                  ch_glyph_index,                /* glyph index           */
                                  rend_flags);             /* load flags, see below */
        if (error) {
            return NULL;  /* ignore errors */
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
    LVFontGlyphCacheItem *item = _glyph_cache2.getByIndex(index);
    if (!item) {
        // glyph not found in cache, rendering...
        int rend_flags = FT_LOAD_RENDER | (!_drawMonochrome ? FT_LOAD_TARGET_NORMAL
                                                            : (FT_LOAD_TARGET_MONO)); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
        if (_hintingMode == HINTING_MODE_AUTOHINT)
            rend_flags |= FT_LOAD_FORCE_AUTOHINT;
        else if (_hintingMode == HINTING_MODE_DISABLED)
            rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        /* load glyph image into the slot (erase previous one) */

        updateTransform();
        int error = FT_Load_Glyph(_face,          /* handle to face object */
                                  index,                /* glyph index           */
                                  rend_flags);             /* load flags, see below */
        if (error) {
            return NULL;  /* ignore errors */
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
    if (w == 0xFF) {
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

void LVFreeTypeFace::DrawTextString(LVDrawBuf *buf, int x, int y, const lChar16 *text, int len,
                                    lChar16 def_char, lUInt32 *palette, bool addHyphen,
                                    lUInt32 flags, int letter_spacing) {
    FONT_GUARD
    if (len <= 0 || _face == NULL)
        return;
    if (letter_spacing < 0 || letter_spacing > 50)
        letter_spacing = 0;
    lvRect clip;
    buf->GetClipRect(&clip);
    updateTransform();
    if (y + _height < clip.top || y >= clip.bottom)
        return;

    //lUInt16 prev_width = 0;
    lChar16 ch;
    // measure character widths
    bool isHyphen = false;
    int x0 = x;
#if USE_HARFBUZZ == 1
    unsigned int i;
    hb_glyph_info_t *glyph_info = 0;
    hb_glyph_position_t *glyph_pos = 0;
    unsigned int glyph_count;
    int w;
    unsigned int len_new = 0;
    if (/*_allowKerning &&*/ _allowLigatures) {
        // Full shaping with HarfBuzz, very slow method but support ligatures.
        hb_buffer_clear_contents(_hb_buffer);
        hb_buffer_set_replacement_codepoint(_hb_buffer, 0);
        // fill HarfBuzz buffer with filtering
        for (i = 0; i < (unsigned int) len; i++) {
            ch = text[i];
            isHyphen = (ch == UNICODE_SOFT_HYPHEN_CODE) && (i < (unsigned int) (len - 1));
            if (!isHyphen) {        // avoid soft hyphens inside text string
                // Also replaced any chars to similar if the glyph is not found
                hb_buffer_add(_hb_buffer, (hb_codepoint_t) filterChar(ch), i);
                len_new++;
            }
        }
        hb_buffer_set_content_type(_hb_buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
        hb_buffer_guess_segment_properties(_hb_buffer);
        // shape
        hb_shape(_hb_font, _hb_buffer, _hb_features, 2);
        glyph_count = hb_buffer_get_length(_hb_buffer);
        glyph_info = hb_buffer_get_glyph_infos(_hb_buffer, 0);
        glyph_pos = hb_buffer_get_glyph_positions(_hb_buffer, 0);
#ifdef _DEBUG
        if (glyph_count != len_new) {
            CRLog::debug(
                        "DrawTextString(): glyph_count not equal source text length, glyph_count=%d, len=%d",
                        glyph_count, len_new);
        }
#endif
        for (i = 0; i < glyph_count; i++) {
            if (0 == glyph_info[i].codepoint) {
                // If HarfBuzz can't find glyph in current font
                // using fallback font that used in getGlyph()
                ch = text[glyph_info[i].cluster];
                LVFontGlyphCacheItem *item = getGlyph(ch, def_char);
                if (item) {
                    w = item->advance;
                    buf->Draw(x + item->origin_x,
                              y + _baseline - item->origin_y,
                              item->bmp,
                              item->bmp_width,
                              item->bmp_height,
                              palette);
                    x += w + letter_spacing;
                }
            } else {
                LVFontGlyphCacheItem *item = getGlyphByIndex(glyph_info[i].codepoint);
                if (item) {
                    w = glyph_pos[i].x_advance >> 6;
                    buf->Draw(x + item->origin_x + (glyph_pos[i].x_offset >> 6),
                              y + _baseline - item->origin_y + (glyph_pos[i].y_offset >> 6),
                              item->bmp,
                              item->bmp_width,
                              item->bmp_height,
                              palette);
                    x += w + letter_spacing;
                }
            }
        }
    } else {        // ligatures disabled, kerning any
        struct LVCharTriplet triplet;
        struct LVCharPosInfo posInfo;
        triplet.Char = 0;
        for (i = 0; i < (unsigned int) len; i++) {
            ch = text[i];
            isHyphen = (ch == UNICODE_SOFT_HYPHEN_CODE) && (i < (unsigned int) (len - 1));
            // avoid soft hyphens inside text string
            if (isHyphen)
                continue;
            LVFontGlyphCacheItem *item = getGlyph(ch, def_char);
            if (item) {
                triplet.prevChar = triplet.Char;
                triplet.Char = ch;
                if (i < (unsigned int) (len - 1))
                    triplet.nextChar = text[i + 1];
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
    }
    if (addHyphen) {
        ch = UNICODE_SOFT_HYPHEN_CODE;
        LVFontGlyphCacheItem *item = getGlyph(ch, def_char);
        if (item) {
            w = item->advance;
            buf->Draw(x + item->origin_x,
                      y + _baseline - item->origin_y,
                      item->bmp,
                      item->bmp_width,
                      item->bmp_height,
                      palette);
            x += w + letter_spacing;
        }
    }
#else
    FT_UInt previous = 0;
    int i;
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
#endif
    if (flags & LTEXT_TD_MASK) {
        // text decoration: underline, etc.
        int h = _size > 30 ? 2 : 1;
        lUInt32 cl = buf->GetTextColor();
        if ((flags & LTEXT_TD_UNDERLINE) || (flags & LTEXT_TD_BLINK)) {
            int liney = y + _baseline + h;
            buf->FillRect(x0, liney, x, liney + h, cl);
        }
        if (flags & LTEXT_TD_OVERLINE) {
            int liney = y + h;
            buf->FillRect(x0, liney, x, liney + h, cl);
        }
        if (flags & LTEXT_TD_LINE_THROUGH) {
            //                int liney = y + _baseline - _size/4 - h/2;
            int liney = y + _baseline - _size * 2 / 7;
            buf->FillRect(x0, liney, x, liney + h, cl);
        }
    }
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
