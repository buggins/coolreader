/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2009 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2019-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2019-2022 Aleksey Chernov <valexlin@gmail.com>          *
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

/**
 * \file lvwin32font.h
 * \brief Win32 font interface
 */

#ifndef __LV_WIN32FONT_H_INCLUDED__
#define __LV_WIN32FONT_H_INCLUDED__

#include "crsetup.h"
#include "cssdef.h"
#include "lvstring.h"
#include "lvcolordrawbuf.h"
#include "lvbasefont.h"

#if !defined(__SYMBIAN32__) && defined(_WIN32)
extern "C" {
#include <windows.h>
}
#endif


#if !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE != 1
class LVBaseWin32Font : public LVBaseFont
{
protected:
    HFONT   _hfont;
    LOGFONTA _logfont;
    int     _height;
    int     _baseline;
    LVColorDrawBuf _drawbuf;
    int _hyphen_width;

public:

    LVBaseWin32Font() : _hfont(NULL), _height(0), _baseline(0), _drawbuf(1,1), _hyphen_width(0)
        { }

    virtual ~LVBaseWin32Font() { Clear(); }

    /// returns font baseline offset
    virtual int getBaseline()
    {
        return _baseline;
    }

    /// returns font height
    virtual int getHeight() const
    {
        return _height;
    }

    /** \brief get extra glyph metric
    */
    virtual bool getGlyphExtraMetric( glyph_extra_metric_t metric, lUInt32 code, int & value, bool scaled_to_px=true, lChar32 def_char=0, lUInt32 fallbackPassMask=0 ) {
        return false;
    }

    /// returns char glyph left side bearing
    int getLeftSideBearing(lChar32 ch, bool negative_only = false, bool italic_only = false) {
        return 0;
    }

    /// returns char glyph right side bearing
    virtual int getRightSideBearing(lChar32 ch, bool negative_only = false, bool italic_only = false) {
        return 0;
    }

    /// returns extra metric
    virtual int getExtraMetric(font_extra_metric_t metric, bool scaled_to_px=true) {
        return 0;
    }

    /// returns if font has OpenType Math tables
    virtual bool hasOTMathSupport() const {
        return false;
    }

    /// retrieves font handle
    virtual void * GetHandle()
    {
        return (void*)_hfont;
    }

    /// returns char width
    virtual int getCharWidth( lChar32 ch, lChar32 def_char=0 )
    {
        glyph_info_t glyph;
        if ( getGlyphInfo(ch, &glyph, def_char) )
            return glyph.width;
        return 0;
    }
    /// returns true if font is empty
    virtual bool IsNull() const
    {
        return (_hfont == NULL);
    }

    virtual bool operator ! () const
    {
        return (_hfont == NULL);
    }

    virtual void Clear();

    virtual bool Create( const LOGFONTA & lf );

    virtual bool Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface );

    virtual int getWeight() const {
        return _logfont.lfWeight;
    }

    virtual int getItalic() const {
        return _logfont.lfItalic;
    }

    virtual lString8 getTypeFace() const {
        return lString8::empty_str;
    }

    virtual css_font_family_t getFontFamily() const {
        return css_ff_inherit;
    }

    virtual LVFontGlyphCacheItem * getGlyph(lUInt32 ch, lChar32 def_char=0, lUInt32 fallbackPassMask = 0) {
        return NULL;
    }

    virtual int getSize() const {
        return 0;
    }

};


class LVWin32DrawFont : public LVBaseWin32Font
{
public:

    LVWin32DrawFont() { }

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo( lUInt32 code, glyph_info_t * glyph, lChar32 def_char=0, lUInt32 fallbackPassMask = 0 );

    /** \brief measure text
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyph was found
    */
    virtual lUInt16 measureText(
                        const lChar32 * text, int len,
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar32 def_char,
                        TextLangCfg * lang_cfg = NULL,
                        int letter_spacing=0,
                        bool allow_hyphenation=true,
                        lUInt32 hints=0,
                        lUInt32 fallbackPassMask = 0
                     );
    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
                        const lChar32 * text, int len, TextLangCfg * lang_cfg = NULL
        );

    /// returns char width
    virtual int getCharWidth( lChar32 ch, lChar32 def_char=0 );

    /// draws text string
    virtual int DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar32 * text, int len,
                       lChar32 def_char, lUInt32 * palette,
                       bool addHyphen, TextLangCfg * lang_cfg = NULL,
                       lUInt32 flags=0, int letter_spacing=0, int width=-1,
                       int text_decoration_back_gap=0,
                       int target_w=-1, int target_h=-1,
                       lUInt32 fallbackPassMask = 0 );

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found
    */
    virtual bool getGlyphImage(lUInt32 code, lUInt8 * buf, lChar32 def_char=0);

};

struct glyph_t {
    lUInt8 *     glyph;
    lChar32      ch;
    bool         flgNotExists;
    bool         flgValid;
    LVFont::glyph_info_t gi;
    glyph_t *    next;
    glyph_t(lChar32 c)
    : glyph(NULL), ch(c), flgNotExists(false), flgValid(false), next(NULL)
    {
        memset( &gi, 0, sizeof(gi) );
    }
    ~glyph_t()
    {
        if (glyph)
            delete glyph;
    }
};

class GlyphCache
{
private:
    lUInt32 _size;
    glyph_t * * _hashtable;
public:
    GlyphCache( lUInt32 size )
    : _size(size)
    {
        _hashtable = new glyph_t * [_size];
        for (lUInt32 i=0; i<_size; i++)
            _hashtable[i] = NULL;
    }
    void clear()
    {
        for (lUInt32 i=0; i<_size; i++)
        {
            glyph_t * p = _hashtable[i];
            while (p)
            {
                glyph_t * next = p->next;
                delete p;
                p = next;
            }
            _hashtable[i] = NULL;
        }
    }
    ~GlyphCache()
    {
        if (_hashtable)
        {
            clear();
            delete _hashtable;
        }
    }
    glyph_t * find( lChar32 ch )
    {
        lUInt32 index = (((lUInt32)ch)*113) % _size;
        glyph_t * p = _hashtable[index];
        // 3 levels
        if (!p)
            return NULL;
        if (p->ch == ch)
            return p;
        p = p->next;
        if (!p)
            return NULL;
        if (p->ch == ch)
            return p;
        p = p->next;
        if (!p)
            return NULL;
        if (p->ch == ch)
            return p;
        return NULL;
    }
    /// returns found or creates new
    glyph_t * get( lChar32 ch )
    {
        lUInt32 index = (((lUInt32)ch)*113) % _size;
        glyph_t * * p = &_hashtable[index];
        // 3 levels
        if (!*p)
        {
            return (*p = new glyph_t(ch));
        }
        if ((*p)->ch == ch)
        {
            return *p;
        }
        p = &(*p)->next;
        if (!*p)
        {
            return (*p = new glyph_t(ch));
        }
        if ((*p)->ch == ch)
        {
            return *p;
        }
        p = &(*p)->next;
        if (!*p)
        {
            return (*p = new glyph_t(ch));
        }
        if ((*p)->ch == ch)
        {
            return *p;
        }

        delete (*p);
        *p = NULL;

        glyph_t * pp = new glyph_t(ch);
        pp->next = _hashtable[index];
        _hashtable[index] = pp;
        return pp;
    }

};

class LVWin32Font : public LVBaseWin32Font
{
private:
    lChar32 _unknown_glyph_index;
    GlyphCache _cache;

    static int GetGlyphIndex( HDC hdc, wchar_t code );

    glyph_t * GetGlyphRec( lChar32 ch );

public:
    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo( lUInt32 code, glyph_info_t * glyph, lChar32 def_char=0, lUInt32 fallbackPassMask = 0 );

    /** \brief measure text
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyph was found
    */
    virtual lUInt16 measureText(
                        const lChar32 * text, int len,
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar32 def_char,
                        TextLangCfg * lang_cfg = NULL,
                        int letter_spacing=0,
                        bool allow_hyphenation=true,
                        lUInt32 hints=0,
                        lUInt32 fallbackPassMask = 0
                     );
    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
                        const lChar32 * text, int len, TextLangCfg * lang_cfg = NULL
        );

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found
    */
    virtual bool getGlyphImage(lUInt32 code, lUInt8 * buf, lChar32 def_char=0);

    virtual void Clear();

    virtual bool Create( const LOGFONTA & lf );

    virtual bool Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface );

    LVWin32Font() : _cache(256) {  }

    virtual ~LVWin32Font() { }
};

#endif      // !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE!=1

#endif  // __LV_WIN32FONT_H_INCLUDED__
