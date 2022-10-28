/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2010,2011 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2019-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2019-2021 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include "lvbitmapfont.h"


#if (USE_BITMAP_FONTS == 1)

LVFontRef LoadFontFromFile( const char * fname )
{
    LVFontRef ref;
    LBitmapFont * font = new LBitmapFont;
    if (font->LoadFromFile( fname ) )
    {
        ref = font;
    }
    else
    {
        delete font;
    }
    return ref;
}

bool LBitmapFont::getGlyphInfo( lUInt32 code, LVFont::glyph_info_t * glyph, lChar32 def_char, lUInt32 fallbackPassMask )
{
    const lvfont_glyph_t * ptr = lvfontGetGlyph( m_font, code );
    if (!ptr)
        return false;
    glyph->blackBoxX = ptr->blackBoxX;
    glyph->blackBoxY = ptr->blackBoxY;
    glyph->originX = ptr->originX;
    glyph->originY = ptr->originY;
    glyph->width = ptr->width;
    return true;
}

lUInt16 LBitmapFont::measureText( 
                    const lChar32 * text, int len, 
                    lUInt16 * widths,
                    lUInt8 * flags,
                    int max_width,
                    lChar32 def_char,
                    TextLangCfg * lang_cfg,
                    int letter_spacing,
                    bool allow_hyphenation,
                    lUInt32 hints,
                    lUInt32 fallbackPassMask
                 )
{
    return lvfontMeasureText( m_font, text, len, widths, flags, max_width, def_char );
}

lUInt32 LBitmapFont::getTextWidth( const lChar32 * text, int len, TextLangCfg * lang_cfg )
{
    //
    static lUInt16 widths[MAX_LINE_CHARS+1];
    static lUInt8 flags[MAX_LINE_CHARS+1];
    if ( len>MAX_LINE_CHARS )
        len = MAX_LINE_CHARS;
    if ( len<=0 )
        return 0;
    lUInt16 res = measureText( 
                    text, len, 
                    widths,
                    flags,
                    2048, // max_width,
                    U' ',  // def_char
                    lang_cfg
                 );
    if ( res>0 && res<MAX_LINE_CHARS )
        return widths[res-1];
    return 0;
}

/// returns font baseline offset
int LBitmapFont::getBaseline()
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->fontBaseline;
}
/// returns font height
int LBitmapFont::getHeight() const
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->fontHeight;
}
/// returns font character size
int LBitmapFont::getSize() const
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->fontHeight;
}
/// returns font weight
int LBitmapFont::getWeight() const
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->flgBold ? 700 : 400;
}
/// returns italic flag
int LBitmapFont::getItalic() const
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->flgItalic;
}
/*
bool LBitmapFont::getGlyphImage(lUInt32 code, lUInt8 * buf, lChar32 def_char)
{
    const lvfont_glyph_t * ptr = lvfontGetGlyph( m_font, code );
    if (!ptr)
        return false;
    const hrle_decode_info_t * pDecodeTable = lvfontGetDecodeTable( m_font );
    int sz = ptr->blackBoxX*ptr->blackBoxY;
    if (sz)
        lvfontUnpackGlyph(ptr->glyph, pDecodeTable, buf, sz);
    return true;
}
*/
LVFontGlyphCacheItem *LBitmapFont::getGlyph(lUInt32 ch, lChar32 def_char, lUInt32 fallbackPassMask) {
    // TODO:
    return NULL;
}
int LBitmapFont::LoadFromFile( const char * fname )
{
    Clear();
    int res = (void*)lvfontOpen( fname, &m_font )!=NULL;
    if (!res)
        return 0;
    lvfont_header_t * hdr = (lvfont_header_t*) m_font;
    _typeface = lString8( hdr->fontName );
    _family = (css_font_family_t) hdr->fontFamily;
    return 1;
}

#endif  // (USE_BITMAP_FONTS==1)
