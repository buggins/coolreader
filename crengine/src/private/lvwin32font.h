/** \file lvwin32font.h
    \brief Win32 font interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_WIN32FONT_H_INCLUDED__
#define __LV_WIN32FONT_H_INCLUDED__

#include "../../include/crsetup.h"
#include "../../include/cssdef.h"
#include "../../include/lvstring.h"
#include "../../include/lvdrawbuf.h"
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

public:

    LVBaseWin32Font() : _hfont(NULL), _height(0), _baseline(0), _drawbuf(1,1)
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

    /// retrieves font handle
    virtual void * GetHandle()
    {
        return (void*)_hfont;
    }

    /// returns char width
    virtual int getCharWidth( lChar16 ch, lChar16 def_char=0 )
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

    virtual LVFontGlyphCacheItem * getGlyph(lUInt32 ch, lChar16 def_char=0) {
        return NULL;
    }

    virtual int getSize() const {
        return 0;
    }

};


class LVWin32DrawFont : public LVBaseWin32Font
{
private:
    int _hyphen_width;
public:

    LVWin32DrawFont() : _hyphen_width(0) { }

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo( lUInt32 code, glyph_info_t * glyph, lChar16 def_char=0 );

    /** \brief measure text
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyph was found
    */
    virtual lUInt16 measureText(
                        const lChar16 * text, int len,
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar16 def_char,
                        int letter_spacing=0,
                        bool allow_hyphenation=true,
                        lUInt32 hints=0
                     );
    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
                        const lChar16 * text, int len
        );

    /// returns char width
    virtual int getCharWidth( lChar16 ch, lChar16 def_char=0 );

    /// draws text string
    virtual void DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar16 * text, int len,
                       lChar16 def_char, lUInt32 * palette, bool addHyphen, lUInt32 flags=0, int letter_spacing=0
                       int width=-1, int text_decoration_back_gap=0 );

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found
    */
    virtual bool getGlyphImage(lUInt32 code, lUInt8 * buf, lChar16 def_char=0);

};

struct glyph_t {
    lUInt8 *     glyph;
    lChar16      ch;
    bool         flgNotExists;
    bool         flgValid;
    LVFont::glyph_info_t gi;
    glyph_t *    next;
    glyph_t(lChar16 c)
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
    glyph_t * find( lChar16 ch )
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
    glyph_t * get( lChar16 ch )
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
    lChar16 _unknown_glyph_index;
    GlyphCache _cache;

    static int GetGlyphIndex( HDC hdc, wchar_t code );

    glyph_t * GetGlyphRec( lChar16 ch );

public:
    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo( lUInt32 code, glyph_info_t * glyph, lChar16 def_char=0 );

    /** \brief measure text
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyph was found
    */
    virtual lUInt16 measureText(
                        const lChar16 * text, int len,
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar16 def_char,
                        int letter_spacing=0,
                        bool allow_hyphenation=true,
                        lUInt32 hints=0
                     );
    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
                        const lChar16 * text, int len
        );

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found
    */
    virtual bool getGlyphImage(lUInt32 code, lUInt8 * buf, lChar16 def_char=0);

    virtual void Clear();

    virtual bool Create( const LOGFONTA & lf );

    virtual bool Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface );

    LVWin32Font() : _cache(256) {  }

    virtual ~LVWin32Font() { }
};

#endif      // !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE!=1

#endif  // __LV_WIN32FONT_H_INCLUDED__
