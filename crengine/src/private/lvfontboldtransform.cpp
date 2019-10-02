/** \file lvfontboldtransform.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfontboldtransform.h"


int LVFontBoldTransform::getWeight() const {
    int w = _baseFont->getWeight() + 200;
    if (w > 900)
        w = 900;
    return w;
}

LVFontBoldTransform::LVFontBoldTransform(LVFontRef baseFont, LVFontGlobalGlyphCache *globalCache)
        : _baseFontRef(baseFont), _baseFont(baseFont.get()), _hyphWidth(-1),
          _glyph_cache(globalCache) {
    _size = _baseFont->getSize();
    _height = _baseFont->getHeight();
    _hShift = _size <= 36 ? 1 : 2;
    _vShift = _size <= 36 ? 0 : 1;
    _baseline = _baseFont->getBaseline();
}

int LVFontBoldTransform::getHyphenWidth() {
    FONT_GUARD
    if (_hyphWidth < 0)
        _hyphWidth = getCharWidth(getHyphChar());
    return _hyphWidth;
}

bool
LVFontBoldTransform::getGlyphInfo(lUInt16 code, LVFont::glyph_info_t *glyph, lChar16 def_char) {
    bool res = _baseFont->getGlyphInfo(code, glyph, def_char);
    if (!res)
        return res;
    glyph->blackBoxX += glyph->blackBoxX > 0 ? _hShift : 0;
    glyph->blackBoxY += _vShift;
    glyph->width += _hShift;

    return true;
}

lUInt16
LVFontBoldTransform::measureText(const lChar16 *text, int len, lUInt16 *widths, lUInt8 *flags,
                                 int max_width, lChar16 def_char, int letter_spacing,
                                 bool allow_hyphenation) {
    CR_UNUSED(allow_hyphenation);
    lUInt16 res = _baseFont->measureText(
            text, len,
            widths,
            flags,
            max_width,
            def_char,
            letter_spacing
    );
    int w = 0;
    for (int i = 0; i < res; i++) {
        w += _hShift;
        widths[i] += w;
    }
    return res;
}

lUInt32 LVFontBoldTransform::getTextWidth(const lChar16 *text, int len) {
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

LVFontGlyphCacheItem *LVFontBoldTransform::getGlyph(lUInt16 ch, lChar16 def_char) {

    LVFontGlyphCacheItem *item = _glyph_cache.get(ch);
    if (item)
        return item;

    LVFontGlyphCacheItem *olditem = _baseFont->getGlyph(ch, def_char);
    if (!olditem)
        return NULL;

    int oldx = olditem->bmp_width;
    int oldy = olditem->bmp_height;
    int dx = oldx ? oldx + _hShift : 0;
    int dy = oldy ? oldy + _vShift : 0;

    item = LVFontGlyphCacheItem::newItem(&_glyph_cache, ch, dx, dy); //, _drawMonochrome
    item->advance = olditem->advance + _hShift;
    item->origin_x = olditem->origin_x;
    item->origin_y = olditem->origin_y;

    if (dx && dy) {
        for (int y = 0; y < dy; y++) {
            lUInt8 *dst = item->bmp + y * dx;
            for (int x = 0; x < dx; x++) {
                int s = 0;
                for (int yy = -_vShift; yy <= 0; yy++) {
                    int srcy = y + yy;
                    if (srcy < 0 || srcy >= oldy)
                        continue;
                    lUInt8 *src = olditem->bmp + srcy * oldx;
                    for (int xx = -_hShift; xx <= 0; xx++) {
                        int srcx = x + xx;
                        if (srcx >= 0 && srcx < oldx && src[srcx] > s)
                            s = src[srcx];
                    }
                }
                dst[x] = s;
            }
        }
    }
    _glyph_cache.put(item);
    return item;
}

void LVFontBoldTransform::DrawTextString(LVDrawBuf *buf, int x, int y, const lChar16 *text, int len,
                                         lChar16 def_char, lUInt32 *palette, bool addHyphen,
                                         lUInt32 flags, int letter_spacing) {
    if (len <= 0)
        return;
    if (letter_spacing < 0 || letter_spacing > 50)
        letter_spacing = 0;
    lvRect clip;
    buf->GetClipRect(&clip);
    if (y + _height < clip.top || y >= clip.bottom)
        return;

    //int error;

    int i;

    //lUInt16 prev_width = 0;
    lChar16 ch;
    // measure character widths
    bool isHyphen = false;
    int x0 = x;
    for (i = 0; i <= len; i++) {
        if (i == len && (!addHyphen || isHyphen))
            break;
        if (i < len) {
            ch = text[i];
            isHyphen = (ch == UNICODE_SOFT_HYPHEN_CODE) && (i < len - 1);
        } else {
            ch = UNICODE_SOFT_HYPHEN_CODE;
            isHyphen = 0;
        }

        LVFontGlyphCacheItem *item = getGlyph(ch, def_char);
        int w = 0;
        if (item) {
            // avoid soft hyphens inside text string
            w = item->advance;
            if (item->bmp_width && item->bmp_height && (!isHyphen || i >= len - 1)) {
                buf->Draw(x + item->origin_x,
                          y + _baseline - item->origin_y,
                          item->bmp,
                          item->bmp_width,
                          item->bmp_height,
                          palette);
            }
        }
        x += w + letter_spacing;
    }
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
            int liney = y + _height / 2 - h / 2;
            buf->FillRect(x0, liney, x, liney + h, cl);
        }
    }
}

/*
bool LVFontBoldTransform::getGlyphImage(lUInt16 code, lUInt8 *buf, lChar16 def_char)
{
    LVFontGlyphCacheItem * item = getGlyph( code, def_char );
    if ( !item )
        return false;
    glyph_info_t glyph;
    if ( !_baseFont->getGlyphInfo( code, &glyph, def_char ) )
        return 0;
    int oldx = glyph.blackBoxX;
    int oldy = glyph.blackBoxY;
    int dx = oldx + _hShift;
    int dy = oldy + _vShift;
    if ( !oldx || !oldy )
        return true;
    LVAutoPtr<lUInt8> tmp( new lUInt8[oldx*oldy+2000] );
    memset(buf, 0, dx*dy);
    tmp[oldx*oldy]=123;
    bool res = _baseFont->getGlyphImage( code, tmp.get(), def_char );
    if ( tmp[oldx*oldy]!=123 ) {
        //CRLog::error("Glyph buffer corrupted!");
        // clear cache
        for ( int i=32; i<4000; i++ ) {
            _baseFont->getGlyphInfo( i, &glyph, def_char );
            _baseFont->getGlyphImage( i, tmp.get(), def_char );
        }
        _baseFont->getGlyphInfo( code, &glyph, def_char );
        _baseFont->getGlyphImage( code, tmp.get(), def_char );
    }
    for ( int y=0; y<dy; y++ ) {
        lUInt8 * dst = buf + y*dx;
        for ( int x=0; x<dx; x++ ) {
            int s = 0;
            for ( int yy=-_vShift; yy<=0; yy++ ) {
                int srcy = y+yy;
                if ( srcy<0 || srcy>=oldy )
                    continue;
                lUInt8 * src = tmp.get() + srcy*oldx;
                for ( int xx=-_hShift; xx<=0; xx++ ) {
                    int srcx = x+xx;
                    if ( srcx>=0 && srcx<oldx && src[srcx] > s )
                        s = src[srcx];
                }
            }
            dst[x] = s;
        }
    }
    return res;
    return false;
}
*/
