/** \file lvfntman.cpp
    \brief font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include <stdlib.h>
#include <stdio.h>



#include "../include/lvfntman.h"
#include "../include/lvstream.h"
#include "../include/lvdrawbuf.h"
#include "../include/lvstyles.h"

#if (USE_FREETYPE==1)

//#include <ft2build.h>

#include <freetype/config/ftheader.h>
#include FT_FREETYPE_H

#endif


#if COLOR_BACKBUFFER==0
//#define USE_BITMAP_FONT
#endif

#define MAX_LINE_CHARS 2048


//DEFINE_NULL_REF( LVFont )


LVFontManager * fontMan = NULL;

/**
    \brief Font properties definition
*/
class LVFontDef
{
private:
    int               _size;
    int               _weight;
    int               _italic;
    css_font_family_t _family;
    lString8          _typeface;
    lString8          _name;
    int               _index;
public:
    LVFontDef(lString8 name, int size, int weight, int italic, css_font_family_t family, lString8 typeface, int index=0)
    : _size(size)
    , _weight(weight)
    , _italic(italic)
    , _family(family)
    , _typeface(typeface)
    , _name(name)
    , _index(0)
    {
    }
    LVFontDef(const LVFontDef & def)
    : _size(def._size)
    , _weight(def._weight)
    , _italic(def._italic)
    , _family(def._family)
    , _typeface(def._typeface)
    , _name(def._name)
    {
    }

    /// returns true if definitions are equal
    bool operator == ( const LVFontDef & def ) const 
    {
        return (_size == def._size || _size == -1 || def._size == -1)
            && (_weight == def._weight || _weight==-1 || def._weight==-1)
            && (_italic == def._italic || _italic==-1 || def._italic==-1)
            && _family == def._family
            && _typeface == def._typeface
            && _name == def._name;
    }
    /// returns font typeface name
    lString8 getName() const { return _name; }
    int getIndex() const { return _index; }
    int getSize() const { return _size; }
    int getWeight() const { return _weight; }
    bool getItalic() const { return _italic!=0; }
    css_font_family_t getFamily() const { return _family; }
    lString8 getTypeFace() const { return _typeface; }
    void setTypeFace(lString8 tf) { _typeface = tf; }
    ~LVFontDef() {}
    /// calculates difference between two fonts
    int CalcMatch( const LVFontDef & def ) const;
};

/// font cache item
class LVFontCacheItem
{
    friend class LVFontCache;
    LVFontDef _def;
    LVFontRef _fnt;
public:
    LVFontDef * getDef() { return &_def; }
    LVFontRef & getFont() { return _fnt; }
    void setFont(LVFontRef & fnt) { _fnt = fnt; }
    LVFontCacheItem( const LVFontDef & def )
    : _def( def ) 
    { }
};

/// font cache
class LVFontCache
{
    LVPtrVector< LVFontCacheItem > _registered_list;
    LVPtrVector< LVFontCacheItem > _instance_list;
public:
    void gc(); // garbage collector
    void update( const LVFontDef * def, LVFontRef ref );
    int  length() { return _registered_list.length(); }
    void addInstance( const LVFontDef * def, LVFontRef ref );
    LVFontCacheItem * find( const LVFontDef * def );
    LVFontCache( )
    { }
};


#if (USE_FREETYPE==1)


class LVFreeTypeFace : public LVFont
{
private:
    lString8      _fileName;
    FT_Library    _library;
    FT_Face       _face;
    FT_GlyphSlot  _slot;
//    FT_Matrix     _matrix;                 /* transformation matrix */
    int           _size; // height in pixels
    int           _hyphen_width;
    int           _baseline;
public:
    LVFreeTypeFace( FT_Library  library )
    : _library(library), _face(NULL), _size(0), _hyphen_width(0), _baseline(0)
    {
    }

    virtual ~LVFreeTypeFace()
    {
        Clear();
    }

    bool loadFromFile( const char * fname, int index, int size )
    {
        if ( fname )
            _fileName = fname;
        if ( _fileName.empty() )
            return false;
        int error = FT_New_Face( _library, _fileName.c_str(), index, &_face ); /* create face object */
        if (error)
            return false;
        _slot = _face->glyph;
        if ( !FT_IS_SCALABLE( _face ) ) {
            Clear();
            return false;
        }
        error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            size );  /* pixel_height          */
        if (error) {
            Clear();
            return false;
        }
        int nheight = _face->size->metrics.height;
        int targetheight = size << 6;
        error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            size * targetheight / nheight );  /* pixel_height          */

        _size = (_face->size->metrics.height >> 6);
        _baseline = _size - (_face->size->metrics.descender >> 6);
        return (error==0);
    }

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found 
    */
    virtual bool getGlyphInfo( lUInt16 code, glyph_info_t * glyph )
    {
        int glyph_index = FT_Get_Char_Index( _face, code );
        if ( glyph_index==0 )
            return false;
        int error = FT_Load_Glyph(
            _face,          /* handle to face object */
            glyph_index,   /* glyph index           */
            FT_LOAD_DEFAULT );  /* load flags, see below */
        if ( error )
            return false;
        glyph->blackBoxX = _slot->metrics.width >> 6;
        glyph->blackBoxY = _slot->metrics.height >> 6;
        glyph->originX =   _slot->metrics.horiBearingX >> 6;
        glyph->originY =   _slot->metrics.horiBearingY >> 6;
        glyph->width =     _slot->metrics.horiAdvance >> 6;
        return false;
    }

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return number of characters before max_width reached 
    */
    virtual lUInt16 measureText( 
                        const lChar16 * text, int len, 
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar16 def_char
                     )
    {
        if ( len <= 0 || _face==NULL )
            return 0;
        int error;

        int use_kerning = FT_HAS_KERNING( _face );

        int i;

        FT_UInt previous = 0;
        lUInt16 prev_width = 0;
        int maxFit = 0;
        // measure character widths
        for ( i=0; i<len; i++) {
            lChar16 ch = text[i];
            bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
            FT_UInt ch_glyph_index = FT_Get_Char_Index( _face, ch );
            if ( ch_glyph_index==0 )
                ch_glyph_index = FT_Get_Char_Index( _face, def_char );
            int kerning = 0;
            if ( use_kerning && previous && ch_glyph_index ) {
                FT_Vector delta;
                error = FT_Get_Kerning( _face,          /* handle to face object */
                              previous,          /* left glyph index      */
                              ch_glyph_index,         /* right glyph index     */
                              FT_KERNING_DEFAULT,  /* kerning mode          */
                              &delta );    /* target vector         */
                if ( !error )
                    kerning = delta.x;
            }
            
            /* load glyph image into the slot (erase previous one) */
            error = FT_Load_Glyph( _face,          /* handle to face object */
                    ch_glyph_index,                /* glyph index           */
                    FT_LOAD_DEFAULT );             /* load flags, see below */
            if ( error ) {
                widths[i] = prev_width;
                continue;  /* ignore errors */
            }
            widths[i] = prev_width + ((_slot->metrics.horiAdvance + kerning) >> 6);
            previous = ch_glyph_index;
            if ( !isHyphen ) // avoid soft hyphens inside text string
                prev_width = widths[i];
            if ( prev_width > max_width )
                break;
            maxFit = i;
        }

        if ( !_hyphen_width )
            _hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );

        lUInt16 wsum = 0;
        lUInt16 nchars = 0;
        lUInt16 gwidth = 0;
        lUInt8 bflags;
        int isSpace;
        lChar16 ch;
        int hwStart, hwEnd;

        for ( ; nchars < len && nchars<maxFit; nchars++ ) 
        {
            bflags = 0;
            ch = text[nchars];
            isSpace = lvfontIsUnicodeSpace(ch);
            if (isSpace ||  ch == UNICODE_SOFT_HYPHEN_CODE )
                bflags |= LCHAR_ALLOW_WRAP_AFTER;
            if (ch == '-')
                bflags |= LCHAR_DEPRECATED_WRAP_AFTER;
            if (isSpace)
                bflags |= LCHAR_IS_SPACE;
            flags[nchars] = bflags;
        }

        //hyphwidth = glyph ? glyph->gi.width : 0;

        //try to add hyphen
        for (hwStart=nchars-1; hwStart>0; hwStart--)
        {
            if (lvfontIsUnicodeSpace(text[hwStart]))
            {
                hwStart++;
                break;
            }
        }
        for (hwEnd=nchars; hwEnd<len; hwEnd++)
        {
            lChar16 ch = text[hwEnd];
            if (lvfontIsUnicodeSpace(ch))
                break;
            if (flags[hwEnd-1]&LCHAR_ALLOW_WRAP_AFTER)
                break;
            if (ch=='.' || ch==',' || ch=='!' || ch=='?' || ch=='?')
                break;
        
        }
        HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);

        return nchars;
    }

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string 
    */
    virtual lUInt32 getTextWidth(
                        const lChar16 * text, int len
        )
    {
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
                        L' '  // def_char
                     );
        if ( res>0 && res<MAX_LINE_CHARS )
            return widths[res-1];
        return 0;
    }

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found 
    */
    virtual bool getGlyphImage(lUInt16 code, lUInt8 * buf)
    {
        return false;
    }

    /// returns font baseline offset
    virtual int getBaseline()
    {
        return _baseline;
    }

    /// returns font height
    virtual int getHeight()
    {
        return _size;
    }

    /// returns char width
    virtual int getCharWidth( lChar16 ch )
    {
        glyph_info_t glyph;
        if ( getGlyphInfo( ch, &glyph ) )
            return glyph.width;
        return 0;
    }

    /// retrieves font handle
    virtual void * GetHandle()
    {
        return NULL;
    }

    /// returns font typeface name
    virtual lString8 getTypeFace()
    {
        return lString8(_face->family_name);
    }

    /// returns font family id
    virtual css_font_family_t getFontFamily()
    {
        return css_ff_sans_serif;
    }

    /// draws text string
    virtual void DrawTextString( LVDrawBuf * buf, int x, int y, 
                       const lChar16 * text, int len, 
                       lChar16 def_char, lUInt32 * palette, bool addHyphen )
    {
        if ( len <= 0 || _face==NULL )
            return 0;
        int error;

        int use_kerning = FT_HAS_KERNING( _face );

        int i;

        FT_UInt previous = 0;
        lUInt16 prev_width = 0;
        // measure character widths
        bool isHyphen = false;
        for ( i=0; i<=len; i++) {
            if ( i==len && (!addHyphen || isHyphen) )
                break;
            lChar16 ch = text[i];
            isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
            FT_UInt ch_glyph_index = FT_Get_Char_Index( _face, ch );
            if ( ch_glyph_index==0 )
                ch_glyph_index = FT_Get_Char_Index( _face, def_char );
            int kerning = 0;
            if ( use_kerning && previous && ch_glyph_index ) {
                FT_Vector delta;
                error = FT_Get_Kerning( _face,          /* handle to face object */
                              previous,          /* left glyph index      */
                              ch_glyph_index,         /* right glyph index     */
                              FT_KERNING_DEFAULT,  /* kerning mode          */
                              &delta );    /* target vector         */
                if ( !error )
                    kerning = delta.x;
            }
            
            /* load glyph image into the slot (erase previous one) */
            error = FT_Load_Glyph( _face,          /* handle to face object */
                    ch_glyph_index,                /* glyph index           */
                    FT_LOAD_RENDER );             /* load flags, see below */
            if ( error ) {
                continue;  /* ignore errors */
            }
            if ( !isHyphen || i>=len-1 ) { // avoid soft hyphens inside text string
                int w = ((_slot->metrics.horiAdvance + kerning) >> 6);

                FT_Bitmap*  bitmap = &_slot->bitmap;
                buf->Draw( x + (kerning>>6) + _slot->bitmap_left,
                    y + _baseline - _slot->bitmap_top, 
                    bitmap->buffer,
                    bitmap->width, 
                    bitmap->rows,
                    palette);

                x  += w;
                previous = ch_glyph_index;
            }
        }
    }

    /// returns true if font is empty
    virtual bool IsNull() const
    {
        return _face == NULL;
    }

    virtual bool operator ! () const
    {
        return _face == NULL;
    }

    virtual void Clear()
    {
        if ( _face )
            FT_Done_Face( _face );
        _face = NULL;
    }

};

class LVFreeTypeFontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
    FT_Library  _library;
public:

    virtual int GetFontCount()
    {
        return _cache.length();
    }

    virtual ~LVFreeTypeFontManager() 
    {
        if ( _library )
            FT_Done_FreeType( _library );
    }

    LVFreeTypeFontManager()
    : _library(NULL)
    {
        int error = FT_Init_FreeType( &_library );
        if ( error ) {
            // error
        }
    }

    virtual void gc() // garbage collector
    {
        _cache.gc();
    }

    lString8 makeFontFileName( lString8 name )
    {
        lString8 filename = _path;
        if (!filename.empty() && _path[filename.length()-1]!=PATH_SEPARATOR_CHAR)
            filename << PATH_SEPARATOR_CHAR;
        filename << name;
        return filename;
    }

    virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface )
    {
        LVFontDef * def = new LVFontDef( 
            lString8(),
            size,
            weight,
            italic,
            family,
            typeface
        );
        //fprintf( _log, "GetFont: %s %d %s %s\n",
        //    typeface.c_str(),
        //    size,
        //    weight>400?"bold":"",
        //    italic?"italic":"" );
        LVFontCacheItem * item = _cache.find( def );
	delete def;
        if (!item->getFont().isNull())
        {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
        LVFreeTypeFace * font = new LVFreeTypeFace(_library);
        lString8 fname = makeFontFileName( item->getDef()->getName() );
        //printf("going to load font file %s\n", fname.c_str());
        if (font->loadFromFile( fname.c_str(), item->getDef()->getIndex(), size ) )
        {
            //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
            //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
            LVFontRef ref(font);
            item->setFont( ref );
            return ref;
        }
        else
        {
            //printf("    not found!\n");
        }
        delete font;
        return LVFontRef(NULL);
    }

    virtual bool RegisterFont( lString8 name )
    {
        lString8 fname = makeFontFileName( name );
        bool res = false;

        int index = 0;

        FT_Face face = NULL;

        // for all faces in file
        for ( ;; index++ ) {
            int error = FT_New_Face( _library, fname.c_str(), index, &face ); /* create face object */
            if ( error )
                break;
            int num_faces = face->num_faces;

            css_font_family_t fontFamily = css_ff_sans_serif;
            if ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
                fontFamily = css_ff_monospace;
        
            LVFontDef def( 
                name,
                0, // height==0 for saclable fonts
                ( face->style_flags & FT_STYLE_FLAG_BOLD ) ? 700 : 300,
                ( face->style_flags & FT_STYLE_FLAG_ITALIC ) ? true : false,
                fontFamily,
                lString8(face->family_name),
                index
            );
            _cache.update( &def, LVFontRef(NULL) );
            res = true;

            if ( face )
                FT_Done_Face( face );

            if ( index>=num_faces-1 )
                break;
        }

        return res;
    }

    virtual bool Init( lString8 path )
    {
        _path = path;
        return (_library != NULL);
    }
};
#endif

#if (USE_BITMAP_FONTS==1)
class LVBitmapFontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
    //FILE * _log;
public:
    virtual int GetFontCount()
    {
        return _cache.length();
    }
    virtual ~LVBitmapFontManager() 
    {
        //if (_log)
        //    fclose(_log);
    }
    LVBitmapFontManager()
    {
        //_log = fopen( "fonts.log", "wt" );
    }
    virtual void gc() // garbage collector
    {
        _cache.gc();
    }
    lString8 makeFontFileName( lString8 name )
    {
        lString8 filename = _path;
        if (!filename.empty() && _path[filename.length()-1]!=PATH_SEPARATOR_CHAR)
            filename << PATH_SEPARATOR_CHAR;
        filename << name;
        return filename;
    }
    virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface )
    {
        LVFontDef * def = new LVFontDef( 
            lString8(),
            size,
            weight,
            italic,
            family,
            typeface
        );
        //fprintf( _log, "GetFont: %s %d %s %s\n",
        //    typeface.c_str(),
        //    size,
        //    weight>400?"bold":"",
        //    italic?"italic":"" );
        LVFontCacheItem * item = _cache.find( def );
	delete def;
        if (!item->getFont().isNull())
        {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
        LBitmapFont * font = new LBitmapFont;
        lString8 fname = makeFontFileName( item->getDef()->getName() );
        //printf("going to load font file %s\n", fname.c_str());
        if (font->LoadFromFile( fname.c_str() ) )
        {
            //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
            //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
            LVFontRef ref(font);
            item->setFont( ref );
            return ref;
        }
        else
        {
            //printf("    not found!\n");
        }
        delete font;
        return LVFontRef(NULL);
    }
    virtual bool RegisterFont( lString8 name )
    {
        lString8 fname = makeFontFileName( name );
        //printf("going to load font file %s\n", fname.c_str());
        LVStreamRef stream = LVOpenFileStream( fname.c_str(), LVOM_READ );
        if (!stream)
        {
            //printf("    not found!\n");
            return false;
        }
        tag_lvfont_header hdr;
        bool res = false;
        lvsize_t bytes_read = 0;
        if ( stream->Read( &hdr, sizeof(hdr), &bytes_read ) == LVERR_OK && bytes_read == sizeof(hdr) )
        {
            LVFontDef def( 
                name,
                hdr.fontHeight,
                hdr.flgBold?700:300,
                hdr.flgItalic?true:false,
                (css_font_family_t)hdr.fontFamily,
                lString8(hdr.fontName)
            );
            //fprintf( _log, "Register: %s %s %d %s %s\n",
            //    name.c_str(), hdr.fontName,
            //    hdr.fontHeight,
            //    hdr.flgBold?"bold":"",
            //    hdr.flgItalic?"italic":"" );
            _cache.update( &def, LVFontRef(NULL) );
            res = true;
        }
        return res;
    }
    virtual bool Init( lString8 path )
    {
        _path = path;
        return true;
    }
};
#endif


#if !defined(__SYMBIAN32__) && defined(_WIN32)

// prototype
int CALLBACK LVWin32FontEnumFontFamExProc(
  const LOGFONTA *lpelfe,    // logical-font data
  const TEXTMETRICA *lpntme,  // physical-font data
  //ENUMLOGFONTEX *lpelfe,    // logical-font data
  //NEWTEXTMETRICEX *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
);

class LVWin32FontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
    //FILE * _log;
public:
    virtual int GetFontCount()
    {
        return _cache.length();
    }
    virtual ~LVWin32FontManager() 
    {
        //if (_log)
        //    fclose(_log);
    }
    LVWin32FontManager()
    {
        //_log = fopen( "fonts.log", "wt" );
    }
    virtual void gc() // garbage collector
    {
        _cache.gc();
    }
    virtual LVFontRef GetFont(int size, int weight, bool bitalic, css_font_family_t family, lString8 typeface )
    {
        int italic = bitalic?1:0;
        if (size<10)
            size = 10;
        if (size>52)
            size = 52;
        
        LVFontDef def( 
            lString8(),
            size,
            weight,
            italic,
            family,
            typeface
        );
        
        //fprintf( _log, "GetFont: %s %d %s %s\n",
        //    typeface.c_str(),
        //    size,
        //    weight>400?"bold":"",
        //    italic?"italic":"" );
        LVFontCacheItem * item = _cache.find( &def );
        if (!item->getFont().isNull())
        {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
        
#if COLOR_BACKBUFFER==0
        LVWin32Font * font = new LVWin32Font;
#else
        LVWin32DrawFont * font = new LVWin32DrawFont;
#endif
        
        LVFontDef * fdef = item->getDef();
        LVFontDef def2( fdef->getName(), size, weight, italic,
            fdef->getFamily(), fdef->getTypeFace() );
   
        if ( font->Create(size, weight, italic?true:false, fdef->getFamily(), fdef->getTypeFace()) )
        {
            //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
            //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
            LVFontRef ref(font);
            _cache.addInstance( &def2, ref );
            return ref;
        }
        delete font;
        return LVFontRef(NULL);
    }
    
    virtual bool RegisterFont( const LOGFONT * lf )
    {
        lString8 face(lf->lfFaceName);
        css_font_family_t ff;
        switch (lf->lfPitchAndFamily & 0x70)
        {
        case FF_ROMAN:
            ff = css_ff_serif;
            break;
        case FF_SWISS:
            ff = css_ff_sans_serif;
            break;
        case FF_SCRIPT:
            ff = css_ff_cursive;
            break;
        case FF_DECORATIVE:
            ff = css_ff_fantasy;
            break;
        case FF_MODERN:
            ff = css_ff_monospace;
            break;
        default:
            ff = css_ff_sans_serif;
            break;
        }
        LVFontDef def(
            face,
            -1, //lf->lfHeight>0 ? lf->lfHeight : -lf->lfHeight,
            -1, //lf->lfWeight,
            -1, //lf->lfItalic!=0,
            ff,
            face
        );
        _cache.update( &def, LVFontRef(NULL) );
        return true;
    }
    virtual bool RegisterFont( lString8 name )
    {
        return false;
    }
    virtual bool Init( lString8 path )
    {
        LVColorDrawBuf drawbuf(1,1);
        LOGFONT lf;
        memset(&lf, 0, sizeof(lf));
        lf.lfCharSet = ANSI_CHARSET;
        int res = 
        EnumFontFamiliesEx(
          drawbuf.GetDC(),                  // handle to DC
          &lf,                              // font information
          LVWin32FontEnumFontFamExProc, // callback function (FONTENUMPROC)
          (LPARAM)this,                    // additional data
          0                     // not used; must be 0
        );
        
        return res!=0;
    }
};

// definition
int CALLBACK LVWin32FontEnumFontFamExProc(
  const LOGFONTA *lf,    // logical-font data
  const TEXTMETRICA *lpntme,  // physical-font data
  //ENUMLOGFONTEX *lpelfe,    // logical-font data
  //NEWTEXTMETRICEX *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
)
{
    //
    if (FontType == TRUETYPE_FONTTYPE)
    {
        LVWin32FontManager * fontman = (LVWin32FontManager *)lParam;
        LVWin32Font fnt;
        //if (strcmp(lf->lfFaceName, "Courier New"))
        //    return 1;
        if ( fnt.Create( *lf ) )
        {
            //
            static lChar16 chars[] = {0, 0xBF, 0xE9, 0x106, 0x410, 0x44F, 0 };
            for (int i=0; chars[i]; i++)
            {
                LVFont::glyph_info_t glyph;
                if (!fnt.getGlyphInfo( chars[i], &glyph ))
                    return 1;
            }
            fontman->RegisterFont( lf ); //&lpelfe->elfLogFont
        }
    }
    return 1;
}


#endif

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

bool InitFontManager( lString8 path )
{
    if ( !fontMan )
    {
#if (USE_WIN32_FONTS==1)
        fontMan = new LVWin32FontManager;
#elif (USE_FREETYPE==1)
        fontMan = new LVFreeTypeFontManager;
#else
        fontMan = new LVBitmapFontManager;
#endif
        return fontMan->Init( path );
    }
    else
    {
        // already initialized
        crFatalError();
        return 0;
    }
}

bool ShutdownFontManager()
{
    if ( fontMan )
    {
        delete fontMan;
        fontMan = NULL;
        return true;
    }
    return false;
}

int LVFontDef::CalcMatch( const LVFontDef & def ) const
{
    int size_match = (_size==-1 || def._size==-1) ? 256 
        : (def._size>_size ? _size*256/def._size : def._size*256/_size );
    int weight_match = (_weight==-1 || def._weight==-1) ? 256 
        : (def._weight>_weight ? _weight*256/def._weight : def._weight*256/_weight );
    int italic_match = (_italic == def._italic || _italic==-1 || def._italic==-1) ? 256 : 0;
    int family_match = (_family==css_ff_inherit || def._family==css_ff_inherit || def._family == def._family) 
        ? 256 
        : ( (_family==css_ff_monospace)==(def._family==css_ff_monospace) ? 64 : 0 );
    int typeface_match = (_typeface == def._typeface) ? 256 : 0;
    return
        + (size_match     * 32)
        + (weight_match   * 4)
        + (italic_match   * 4)
        + (family_match   * 16)
        + (typeface_match * 32);
}








void LVBaseFont::DrawTextString( LVDrawBuf * buf, int x, int y, 
                   const lChar16 * text, int len, 
                   lChar16 def_char, lUInt32 * palette, bool addHyphen )
{
    static lUInt8 glyph_buf[16384];
    LVFont::glyph_info_t info;
    int baseline = getBaseline();
    while (len>=(addHyphen?0:1))
    {
      if (len<=1 || *text != UNICODE_SOFT_HYPHEN_CODE)
      {
          lChar16 ch = ((len==0)?UNICODE_SOFT_HYPHEN_CODE:*text);
          if ( !getGlyphInfo( ch, &info ) )
          {
              ch = def_char;
              if ( !getGlyphInfo( ch, &info ) )
                  ch = 0;
          }
          if (ch && getGlyphImage( ch, glyph_buf ))
          {
              if (info.blackBoxX && info.blackBoxY)
              {
                  buf->Draw( x + info.originX,
                      y + baseline - info.originY, 
                      glyph_buf,
                      info.blackBoxX, 
                      info.blackBoxY,
                      palette);
              }
              x += info.width;
          }
      }
      else if (*text != UNICODE_SOFT_HYPHEN_CODE)
      {
          len = len;
      }
      len--;
      text++;
    }
}

#if (USE_BITMAP_FONTS==1)
bool LBitmapFont::getGlyphInfo( lUInt16 code, LVFont::glyph_info_t * glyph )
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
                    const lChar16 * text, int len, 
                    lUInt16 * widths,
                    lUInt8 * flags,
                    int max_width,
                    lChar16 def_char
                 )
{
    return lvfontMeasureText( m_font, text, len, widths, flags, max_width, def_char );
}

lUInt32 LBitmapFont::getTextWidth( const lChar16 * text, int len )
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
                    L' '  // def_char
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
int LBitmapFont::getHeight()
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->fontHeight;
}
bool LBitmapFont::getGlyphImage(lUInt16 code, lUInt8 * buf)
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
#endif

LVFontCacheItem * LVFontCache::find( const LVFontDef * fntdef )
{
    int best_index = -1;
    int best_match = -1;
    int best_instance_index = -1;
    int best_instance_match = -1;
    int i;
    LVFontDef def(*fntdef);
    lString8Collection list;
    splitPropertyValueList( fntdef->getTypeFace().c_str(), list );
    for (unsigned nindex=0; nindex==0 || nindex<list.length(); nindex++)
    {
        if ( nindex<list.length() )
            def.setTypeFace( list[nindex] );
        else
            def.setTypeFace( lString8() );
        for (i=0; i<_instance_list.length(); i++)
        {
            int match = _instance_list[i]->_def.CalcMatch( def );
            if (match > best_instance_match)
            {
                best_instance_match = match;
                best_instance_index = i;
            }
        }
        for (i=0; i<_registered_list.length(); i++)
        {
            int match = _registered_list[i]->_def.CalcMatch( def );
            if (match > best_match)
            {
                best_match = match;
                best_index = i;
            }
        }
    }
    if (best_index<0)
        return NULL;
    if (best_instance_match >= best_match)
        return _instance_list[best_instance_index];
    return _registered_list[best_index];
}

void LVFontCache::addInstance( const LVFontDef * def, LVFontRef ref )
{
    LVFontCacheItem * item = new LVFontCacheItem(*def);
    item->_fnt = ref;
    _instance_list.add( item );
}

void LVFontCache::update( const LVFontDef * def, LVFontRef ref )
{
    int i;
    for (i=0; i<_instance_list.length(); i++)
    {
        if ( _instance_list[i]->_def == *def )
        {
            if (ref.isNull())
            {
                _instance_list.erase(i, 1);
            }
            else
            {
                _instance_list[i]->_fnt = ref;
            }
            return;
        }
    }
    for (i=0; i<_registered_list.length(); i++)
    {
        if ( _registered_list[i]->_def == *def )
        {
            if (!ref.isNull())
            {
                addInstance( def, ref );
            }
            return;
        }
    }
    // add new
    LVFontCacheItem * item;
    item = new LVFontCacheItem(*def);
    _registered_list.add( item );
    if (!ref.isNull())
    {
          addInstance( def, ref );
    }
}

// garbage collector
void LVFontCache::gc()
{
    for (int i=_instance_list.length()-1; i>=0; i--)
    {
        if ( _instance_list[i]->_fnt.getRefCount()==1 )
        {
            _instance_list.erase(i,1);
        }
    }
}

#if !defined(__SYMBIAN32__) && defined(_WIN32)
void LVBaseWin32Font::Clear() 
{
    if (_hfont)
    {
        DeleteObject(_hfont);
        _hfont = NULL;
        _height = 0;
        _baseline = 0;
    }
}

bool LVBaseWin32Font::Create( const LOGFONT & lf )
{
    if (!IsNull())
        Clear();
    memcpy( &_logfont, &lf, sizeof(LOGFONT));
    _hfont = CreateFontIndirect( &lf );
    if (!_hfont)
        return false;
    //memcpy( &_logfont, &lf, sizeof(LOGFONT) );
    // get text metrics
    SelectObject( _drawbuf.GetDC(), _hfont );
    TEXTMETRICW tm;
    GetTextMetricsW( _drawbuf.GetDC(), &tm );
    _logfont.lfHeight = tm.tmHeight;
    _logfont.lfWeight = tm.tmWeight;
    _logfont.lfItalic = tm.tmItalic;
    _logfont.lfCharSet = tm.tmCharSet;
    GetTextFace( _drawbuf.GetDC(), sizeof(_logfont.lfFaceName)-1, _logfont.lfFaceName );
    _height = tm.tmHeight;
    _baseline = _height - tm.tmDescent;
    return true;
}

bool LVBaseWin32Font::Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface )
{
    if (!IsNull())
        Clear();
    //
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = size;
    lf.lfWeight = weight;
    lf.lfItalic = italic?1:0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    //lf.lfQuality = ANTIALIASED_QUALITY; //PROOF_QUALITY;
#ifdef USE_BITMAP_FONT
    lf.lfQuality = NONANTIALIASED_QUALITY; //CLEARTYPE_QUALITY; //PROOF_QUALITY;
#else
    lf.lfQuality = 5; //CLEARTYPE_QUALITY; //PROOF_QUALITY;
#endif
    strcpy(lf.lfFaceName, typeface.c_str());
    _typeface = typeface;
    _family = family;
    switch (family)
    {
    case css_ff_serif:
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
        break;
    case css_ff_sans_serif:
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
        break;
    case css_ff_cursive:
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SCRIPT;
        break;
    case css_ff_fantasy:
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DECORATIVE;
        break;
    case css_ff_monospace:
        lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
        break;
    default:
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
        break;
    }
    _hfont = CreateFontIndirect( &lf );
    if (!_hfont)
        return false;
    //memcpy( &_logfont, &lf, sizeof(LOGFONT) );
    // get text metrics
    SelectObject( _drawbuf.GetDC(), _hfont );
    TEXTMETRICW tm;
    GetTextMetricsW( _drawbuf.GetDC(), &tm );
    memset(&_logfont, 0, sizeof(LOGFONT));
    _logfont.lfHeight = tm.tmHeight;
    _logfont.lfWeight = tm.tmWeight;
    _logfont.lfItalic = tm.tmItalic;
    _logfont.lfCharSet = tm.tmCharSet;
    GetTextFace( _drawbuf.GetDC(), sizeof(_logfont.lfFaceName)-1, _logfont.lfFaceName );
    _height = tm.tmHeight;
    _baseline = _height - tm.tmDescent;
    return true;
}


/** \brief get glyph info
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyh was found 
*/
bool LVWin32DrawFont::getGlyphInfo( lUInt16 code, glyph_info_t * glyph )
{
    return false;
}

/// returns char width
int LVWin32DrawFont::getCharWidth( lChar16 ch )
{
    if (_hfont==NULL)
        return 0;
    // measure character widths
    GCP_RESULTSW gcpres;
    memset( &gcpres, 0, sizeof(gcpres) );
    gcpres.lStructSize = sizeof(gcpres);
    lChar16 str[2];
    str[0] = ch;
    str[1] = 0;
    int dx[2];
    gcpres.lpDx = dx;
    gcpres.nMaxFit = 1;
    gcpres.nGlyphs = 1;

    lUInt32 res = GetCharacterPlacementW( 
        _drawbuf.GetDC(),
        str,
        1,
        100,
        &gcpres,
        GCP_MAXEXTENT); //|GCP_USEKERNING

    if (!res)
    {
        // ERROR
        return 0;
    }

    return dx[0];
}

lUInt32 LVWin32DrawFont::getTextWidth( const lChar16 * text, int len )
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
                    L' '  // def_char
                 );
    if ( res>0 && res<MAX_LINE_CHARS )
        return widths[res-1];
    return 0;
}

/** \brief measure text
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyph was found 
*/
lUInt16 LVWin32DrawFont::measureText( 
                    const lChar16 * text, int len, 
                    lUInt16 * widths,
                    lUInt8 * flags,
                    int max_width,
                    lChar16 def_char
                 )
{
    if (_hfont==NULL)
        return 0;

    if (len==5 && text[len]==65021)
        max_width=max_width;
    lString16 str(text, len);
    assert(str[len]==0);
    //str += L"         ";
    lChar16 * pstr = str.modify();
    assert(pstr[len]==0);
    // substitute soft hyphens with zero width spaces
    for (int i=0; i<len; i++)
    {
        if (pstr[i]==UNICODE_SOFT_HYPHEN_CODE)
            pstr[i] = UNICODE_ZERO_WIDTH_SPACE;
    }
    assert(pstr[len]==0);
    // measure character widths
    GCP_RESULTSW gcpres;
    memset( &gcpres, 0, sizeof(gcpres) );
    gcpres.lStructSize = sizeof(gcpres);
    LVArray<int> dx( len+1, 0 );
    gcpres.lpDx = dx.ptr();
    gcpres.nMaxFit = len;
    gcpres.nGlyphs = len;

    lUInt32 res = GetCharacterPlacementW( 
        _drawbuf.GetDC(),
        pstr,
        len,
        max_width,
        &gcpres,
        GCP_MAXEXTENT); //|GCP_USEKERNING
    if (!res)
    {
        // ERROR
        widths[0] = 0;
        flags[0] = 0;
        return 1;
    }

    if ( !_hyphen_width )
        _hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );

    lUInt16 wsum = 0;
    lUInt16 nchars = 0;
    lUInt16 gwidth = 0;
    lUInt8 bflags;
    int isSpace;
    lChar16 ch;
    int hwStart, hwEnd;

    assert(pstr[len]==0);

    for ( ; wsum < max_width && nchars < len && nchars<gcpres.nMaxFit; nchars++ ) 
    {
        bflags = 0;
        ch = text[nchars];
        isSpace = lvfontIsUnicodeSpace(ch);
        if (isSpace ||  ch == UNICODE_SOFT_HYPHEN_CODE )
            bflags |= LCHAR_ALLOW_WRAP_AFTER;
        if (ch == '-')
            bflags |= LCHAR_DEPRECATED_WRAP_AFTER;
        if (isSpace)
            bflags |= LCHAR_IS_SPACE;
        gwidth = gcpres.lpDx[nchars];
        widths[nchars] = wsum + gwidth;
        if ( ch != UNICODE_SOFT_HYPHEN_CODE ) 
            wsum += gwidth; /* don't include hyphens to width */
        flags[nchars] = bflags;
    }
    //hyphwidth = glyph ? glyph->gi.width : 0;

    //try to add hyphen
    for (hwStart=nchars-1; hwStart>0; hwStart--)
    {
        if (lvfontIsUnicodeSpace(text[hwStart]))
        {
            hwStart++;
            break;
        }
    }
    for (hwEnd=nchars; hwEnd<len; hwEnd++)
    {
        lChar16 ch = text[hwEnd];
        if (lvfontIsUnicodeSpace(ch))
            break;
        if (flags[hwEnd-1]&LCHAR_ALLOW_WRAP_AFTER)
            break;
        if (ch=='.' || ch==',' || ch=='!' || ch=='?' || ch=='?')
            break;
        
    }
    HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);

    return nchars;
}

/// draws text string
void LVWin32DrawFont::DrawTextString( LVDrawBuf * buf, int x, int y, 
                   const lChar16 * text, int len, 
                   lChar16 def_char, lUInt32 * palette, bool addHyphen )
{
    if (_hfont==NULL)
        return;

    lString16 str(text, len);
    // substitute soft hyphens with zero width spaces
    if (addHyphen)
        str += UNICODE_SOFT_HYPHEN_CODE;
    //str += L"       ";
    lChar16 * pstr = str.modify();
    for (int i=0; i<len-1; i++)
    {
        if (pstr[i]==UNICODE_SOFT_HYPHEN_CODE)
            pstr[i] = UNICODE_ZERO_WIDTH_SPACE;
    }

    lvRect clip;
    buf->GetClipRect(&clip);
    if (y > clip.bottom || y+_height < clip.top)
        return;
    if (buf->GetBitsPerPixel()<16)
    {
        // draw using backbuffer
        SIZE sz;
        if ( !GetTextExtentPoint32W(_drawbuf.GetDC(), 
                str.c_str(), str.length(), &sz) )
            return;
        LVColorDrawBuf colorbuf( sz.cx, sz.cy );
        colorbuf.Clear(0xFFFFFF);
        HDC dc = colorbuf.GetDC();
        SelectObject(dc, _hfont);
        SetTextColor(dc, 0x000000);
        SetBkMode(dc, TRANSPARENT);
        //ETO_OPAQUE
        if (ExtTextOutW( dc, 0, 0, 
                0, //ETO_OPAQUE
                NULL,
                str.c_str(), str.length(), NULL ))
        {
            // COPY colorbuf to buf with converting
            colorbuf.DrawTo( buf, x, y, 0, NULL );
        }
    } 
    else
    {
        // draw directly on DC
        //TODO
        HDC dc = ((LVColorDrawBuf*)buf)->GetDC();
        HFONT oldfont = (HFONT)SelectObject( dc, _hfont );
        SetTextColor( dc, RevRGB(buf->GetTextColor()) );
        SetBkMode(dc, TRANSPARENT);
        ExtTextOutW( dc, x, y, 
            0, //ETO_OPAQUE
            NULL,
            str.c_str(), str.length(), NULL );
        SelectObject( dc, oldfont );
    }
}
    
/** \brief get glyph image in 1 byte per pixel format
    \param code is unicode character
    \param buf is buffer [width*height] to place glyph data
    \return true if glyph was found 
*/
bool LVWin32DrawFont::getGlyphImage(lUInt16 code, lUInt8 * buf)
{
    return false;
}



int LVWin32Font::GetGlyphIndex( HDC hdc, wchar_t code )
{
    wchar_t s[2];
    wchar_t g[2];
    s[0] = code;
    s[1] = 0;
    g[0] = 0;
    GCP_RESULTSW gcp;
    gcp.lStructSize = sizeof(GCP_RESULTSW);
    gcp.lpOutString = NULL;
    gcp.lpOrder = NULL;
    gcp.lpDx = NULL;
    gcp.lpCaretPos = NULL;
    gcp.lpClass = NULL;
    gcp.lpGlyphs = g;
    gcp.nGlyphs = 2;
    gcp.nMaxFit = 2;

    DWORD res = GetCharacterPlacementW(
      hdc, s, 1,
      1000,
      &gcp,
      0
    );
    if (!res)
        return 0;
    return g[0];
}


glyph_t * LVWin32Font::GetGlyphRec( lChar16 ch )
{
    glyph_t * p = _cache.get( ch );
    if (p->flgNotExists)
        return NULL;
    if (p->flgValid)
        return p;
    p->flgNotExists = true;
    lUInt16 gi = GetGlyphIndex( _drawbuf.GetDC(), ch );
    if (gi==0 || gi==0xFFFF || (gi==_unknown_glyph_index && ch!=' '))
    {
        // glyph not found
        p->flgNotExists = true;
        return NULL;
    }
    GLYPHMETRICS metrics;
    p->glyph = NULL;
    
    MAT2 identity = { {0,1}, {0,0}, {0,0}, {0,1} };
    lUInt32 res;
    res = GetGlyphOutlineW( _drawbuf.GetDC(), ch,
        GGO_METRICS,
        &metrics,
        0,
        NULL,
        &identity );
    if (res==GDI_ERROR)
        return false;
    int gs = GetGlyphOutlineW( _drawbuf.GetDC(), ch,
#ifdef USE_BITMAP_FONT
        GGO_BITMAP, //GGO_METRICS
#else
        GGO_GRAY8_BITMAP, //GGO_METRICS
#endif
        &metrics,
        0,
        NULL,
        &identity );
    if (gs>0x10000 || gs<0)
        return false;
        
    p->gi.blackBoxX = metrics.gmBlackBoxX;
    p->gi.blackBoxY = metrics.gmBlackBoxY;
    p->gi.originX = (lInt8)metrics.gmptGlyphOrigin.x;
    p->gi.originY = (lInt8)metrics.gmptGlyphOrigin.y;
    p->gi.width = (lUInt8)metrics.gmCellIncX;
    
    if (p->gi.blackBoxX>0 && p->gi.blackBoxY>0)
    {
        p->glyph = new unsigned char[p->gi.blackBoxX * p->gi.blackBoxY];
        if (gs>0)
        {
            lUInt8 * glyph = new unsigned char[gs];
             res = GetGlyphOutlineW( _drawbuf.GetDC(), ch,
#ifdef USE_BITMAP_FONT
        GGO_BITMAP, //GGO_METRICS
#else
        GGO_GRAY8_BITMAP, //GGO_METRICS
#endif
               &metrics,
               gs,
               glyph,
               &identity );
            if (res==GDI_ERROR)
            {
                delete glyph;
                return NULL;
            }
#ifdef USE_BITMAP_FONT
            int glyph_row_size = (p->gi.blackBoxX + 31) / 8 / 4 * 4;
#else
            int glyph_row_size = (p->gi.blackBoxX + 3)/ 4 * 4;
#endif
            lUInt8 * src = glyph;
            lUInt8 * dst = p->glyph;
            for (int y=0; y<p->gi.blackBoxY; y++)
            {
                for (int x = 0; x<p->gi.blackBoxX; x++)
                {
#ifdef USE_BITMAP_FONT
                    lUInt8 b = (src[x>>3] & (0x80>>(x&7))) ? 0xFC : 0;
#else
                    lUInt8 b = src[x];
                    if (b>=64)
                       b = 63;
                    b = (b<<2) & 0xFC;
#endif
                    dst[x] = b;
                }
                src += glyph_row_size;
                dst += p->gi.blackBoxX;
            }
            delete glyph;
            //*(dst-1) = 0xFF;
        }
        else
        {
            // empty glyph
            for (int i=p->gi.blackBoxX * p->gi.blackBoxY-1; i>=0; i--)
                p->glyph[i] = 0;
        }
    }
    // found!
    p->flgValid = true;
    p->flgNotExists = false;
    return p;
}

/** \brief get glyph info
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyh was found 
*/
bool LVWin32Font::getGlyphInfo( lUInt16 code, glyph_info_t * glyph )
{
    if (_hfont==NULL)
        return false;
    glyph_t * p = GetGlyphRec( code );
    if (!p)
        return false;
    *glyph = p->gi;
    return true;
}

lUInt32 LVWin32Font::getTextWidth( const lChar16 * text, int len )
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
                    L' '  // def_char
                 );
    if ( res>0 && res<MAX_LINE_CHARS )
        return widths[res-1];
    return 0;
}

/** \brief measure text
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyph was found 
*/
lUInt16 LVWin32Font::measureText( 
                    const lChar16 * text, int len, 
                    lUInt16 * widths,
                    lUInt8 * flags,
                    int max_width,
                    lChar16 def_char
                 )
{
    if (_hfont==NULL)
        return 0;
        
    lUInt16 wsum = 0;
    lUInt16 nchars = 0;
    glyph_t * glyph; //GetGlyphRec( lChar16 ch )
    lUInt16 gwidth = 0;
    lUInt16 hyphwidth = 0;
    lUInt8 bflags;
    int isSpace;
    lChar16 ch;
    int hwStart, hwEnd;

    glyph = GetGlyphRec( UNICODE_SOFT_HYPHEN_CODE );
    hyphwidth = glyph ? glyph->gi.width : 0;

    for ( ; wsum < max_width && nchars < len; nchars++ ) 
    {
        bflags = 0;
        ch = text[nchars];
        isSpace = lvfontIsUnicodeSpace(ch);
        if (isSpace ||  ch == UNICODE_SOFT_HYPHEN_CODE )
            bflags |= LCHAR_ALLOW_WRAP_AFTER;
        if (ch == '-')
            bflags |= LCHAR_DEPRECATED_WRAP_AFTER;
        if (isSpace)
            bflags |= LCHAR_IS_SPACE;
        glyph = GetGlyphRec( ch );
        if (!glyph && def_char)
             glyph = GetGlyphRec( def_char );
        gwidth = glyph ? glyph->gi.width : 0;
        widths[nchars] = wsum + gwidth;
        if ( ch != UNICODE_SOFT_HYPHEN_CODE ) 
            wsum += gwidth; /* don't include hyphens to width */
        flags[nchars] = bflags;
    }

    //try to add hyphen
    for (hwStart=nchars-1; hwStart>0; hwStart--)
    {
        if (lvfontIsUnicodeSpace(text[hwStart]))
        {
            hwStart++;
            break;
        }
    }
    for (hwEnd=nchars; hwEnd<len; hwEnd++)
    {
        lChar16 ch = text[hwEnd];
        if (lvfontIsUnicodeSpace(ch))
            break;
        if (flags[hwEnd-1]&LCHAR_ALLOW_WRAP_AFTER)
            break;
        if (ch=='.' || ch==',' || ch=='!' || ch=='?' || ch=='?')
            break;
        
    }
    HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, hyphwidth, max_width);

    return nchars;
}

/** \brief get glyph image in 1 byte per pixel format
    \param code is unicode character
    \param buf is buffer [width*height] to place glyph data
    \return true if glyph was found 
*/
bool LVWin32Font::getGlyphImage(lUInt16 code, lUInt8 * buf)
{
    if (_hfont==NULL)
        return false;
    glyph_t * p = GetGlyphRec( code );
    if (!p)
        return false;
    int gs = p->gi.blackBoxX*p->gi.blackBoxY;
    if (gs>0)
        memcpy( buf, p->glyph, gs );
    return true;
}

void LVWin32Font::Clear() 
{
    LVBaseWin32Font::Clear();
    _cache.clear();
}

bool LVWin32Font::Create( const LOGFONT & lf )
{
    if (!LVBaseWin32Font::Create(lf))
        return false;
    _unknown_glyph_index = GetGlyphIndex( _drawbuf.GetDC(), 1 );
    return true;
}

bool LVWin32Font::Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface )
{
    if (!LVBaseWin32Font::Create(size, weight, italic, family, typeface))
        return false;
    _unknown_glyph_index = GetGlyphIndex( _drawbuf.GetDC(), 1 );
    return true;
}

#endif
