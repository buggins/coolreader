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



#include "../include/crsetup.h"
#include "../include/lvfntman.h"
#include "../include/lvstream.h"
#include "../include/lvdrawbuf.h"
#include "../include/lvstyles.h"
#include "../include/lvthread.h"

// define to filter out all fonts except .ttf
#define LOAD_TTF_FONTS_ONLY

#if (USE_FREETYPE==1)

//#include <ft2build.h>

#include <freetype/config/ftheader.h>
//#include FT_FREETYPE_H
#include <freetype/freetype.h>

#endif


#if COLOR_BACKBUFFER==0
//#define USE_BITMAP_FONT
#endif

#define MAX_LINE_CHARS 2048


//DEFINE_NULL_REF( LVFont )


LVFontManager * fontMan = NULL;

/**
 * Max width of -/./,/!/? to use for visial alignment by width
 */
int LVFont::getVisualAligmentWidth()
{
    if ( _visual_alignment_width==-1 ) {
        lChar16 chars[] = { getHyphChar(), ',', '.', '!', ':', ';', 0 };
        int maxw = 0;
        for ( int i=0; chars[i]; i++ ) {
            int w = getCharWidth( chars[i] );
            if ( w > maxw )
                maxw = w;
        }
        _visual_alignment_width = maxw;
    }
    return _visual_alignment_width;
}



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
    LVFontDef(const lString8 & name, int size, int weight, int italic, css_font_family_t family, const lString8 & typeface, int index=-1)
    : _size(size)
    , _weight(weight)
    , _italic(italic)
    , _family(family)
    , _typeface(typeface)
    , _name(name)
    , _index(index)
    {
    }
    LVFontDef(const LVFontDef & def)
    : _size(def._size)
    , _weight(def._weight)
    , _italic(def._italic)
    , _family(def._family)
    , _typeface(def._typeface)
    , _name(def._name)
    , _index(def._index)
    {
    }

    /// returns true if definitions are equal
    bool operator == ( const LVFontDef & def ) const 
    {
        return ( _size == def._size || _size == -1 || def._size == -1 )
            && ( _weight == def._weight || _weight==-1 || def._weight==-1 )
            && ( _italic == def._italic || _italic==-1 || def._italic==-1 )
            && _family == def._family
            && _typeface == def._typeface
            && _name == def._name
            && ( _index == def._index || def._index == -1 )
            ;
    }
    /// returns font file name
    lString8 getName() const { return _name; }
    void setName( lString8 name) {  _name = name; }
    int getIndex() const { return _index; }
    void setIndex( int index ) { _index = index; }
    int getSize() const { return _size; }
    void setSize( int size ) { _size = size; }
    int getWeight() const { return _weight; }
    void setWeight( int weight ) { _weight = weight; }
    bool getItalic() const { return _italic!=0; }
    void setItalic( bool italic ) { _italic=italic; }
    css_font_family_t getFamily() const { return _family; }
    void getFamily( css_font_family_t family ) { _family = family; }
    lString8 getTypeFace() const { return _typeface; }
    void setTypeFace(lString8 tf) { _typeface = tf; }
    ~LVFontDef() {}
    /// calculates difference between two fonts
    int CalcMatch( const LVFontDef & def ) const;
    int CalcDuplicateMatch( const LVFontDef & def ) const;
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
    void clear() { _registered_list.clear(); _instance_list.clear(); }
    void gc(); // garbage collector
    void update( const LVFontDef * def, LVFontRef ref );
    int  length() { return _registered_list.length(); }
    void addInstance( const LVFontDef * def, LVFontRef ref );
    LVPtrVector< LVFontCacheItem > * getInstances() { return &_instance_list; }
    LVFontCacheItem * find( const LVFontDef * def );
    LVFontCacheItem * findDuplicate( const LVFontDef * def );
    virtual void getFaceList( lString16Collection & list )
    {
        list.clear();
        for ( int i=0; i<_registered_list.length(); i++ ) {
            lString16 name = Utf8ToUnicode( _registered_list[i]->getDef()->getTypeFace() );
            if ( !list.contains(name) )
                list.add( name );
        }
    }
    LVFontCache( )
    { }
    virtual ~LVFontCache() { }
};


#if (USE_FREETYPE==1)


class LVFontGlyphWidthCache
{
private:
    lUInt8 * ptrs[128];
public:
    lUInt8 get( lChar16 ch )
    {
        int inx = (ch>>9) & 0x7f;
        lUInt8 * ptr = ptrs[inx];
        if ( !ptr )
            return 0xFF;
        return ptr[ch & 0x1FF ];
    }
    void put( lChar16 ch, lUInt8 w )
    {
        int inx = (ch>>9) & 0x7f;
        lUInt8 * ptr = ptrs[inx];
        if ( !ptr ) {
            ptr = new lUInt8[512];
            ptrs[inx] = ptr;
            memset( ptr, 0xFF, sizeof(lUInt8) * 512 );
        }
        ptr[ ch & 0x1FF ] = w;
    }
    void clear()
    {
        for ( int i=0; i<128; i++ ) {
            if ( ptrs[i] )
                delete [] ptrs[i];
            ptrs[i] = NULL;
        }
    }
    LVFontGlyphWidthCache()
    {
        memset( ptrs, 0, 128*sizeof(lUInt8*) );
    }
    ~LVFontGlyphWidthCache()
    {
        clear();
    }
};

class LVFreeTypeFace;
struct LVFontGlyphCacheItem;

class LVFontGlobalGlyphCache
{
private:
    LVFontGlyphCacheItem * head;
    LVFontGlyphCacheItem * tail;
    int size;
    int max_size;
public:
    LVFontGlobalGlyphCache( int maxSize )
        : head(NULL), tail(NULL), size(0), max_size(maxSize )
    {
    }
    ~LVFontGlobalGlyphCache()
    {
        clear();
    }
    void put( LVFontGlyphCacheItem * item );
    void remove( LVFontGlyphCacheItem * item );
    void refresh( LVFontGlyphCacheItem * item );
    void clear();
};

class LVFontLocalGlyphCache
{
private:
    LVFontGlyphCacheItem * head;
    LVFontGlyphCacheItem * tail;
    LVFontGlobalGlyphCache * global_cache;
    int size;
public:
    LVFontLocalGlyphCache( LVFontGlobalGlyphCache * globalCache )
        : head(NULL), tail(NULL), global_cache( globalCache )
    { }
    ~LVFontLocalGlyphCache()
    {
        clear();
    }
    void clear();
    LVFontGlyphCacheItem * get( lUInt16 ch );
    void put( LVFontGlyphCacheItem * item );
    void remove( LVFontGlyphCacheItem * item );
};

struct LVFontGlyphCacheItem
{
    LVFontGlyphCacheItem * prev_global;
    LVFontGlyphCacheItem * next_global;
    LVFontGlyphCacheItem * prev_local;
    LVFontGlyphCacheItem * next_local;
    LVFontLocalGlyphCache * local_cache;
    lChar16 ch;
    lUInt8 bmp_width;
    lUInt8 bmp_height;
    lInt8  origin_x;
    lInt8  origin_y;
    lUInt8 advance;
    lUInt8 bmp[1];
    //=======================================================================
    int getSize()
    { 
        return sizeof(LVFontGlyphCacheItem) 
            + (bmp_width * bmp_height - 1) * sizeof(lUInt8);
    }
    static LVFontGlyphCacheItem * newItem( LVFontLocalGlyphCache * local_cache, lChar16 ch, FT_GlyphSlot slot, bool drawMonochrome )
    {
        FT_Bitmap*  bitmap = &slot->bitmap;
        lUInt8 w = (lUInt8)(bitmap->width);
        lUInt8 h = (lUInt8)(bitmap->rows);
        LVFontGlyphCacheItem * item = (LVFontGlyphCacheItem *)malloc( sizeof(LVFontGlyphCacheItem) 
            + (w*h - 1)*sizeof(lUInt8) );
        if ( drawMonochrome ) {
            lUInt8 mask = 0x80;
            const lUInt8 * ptr = (const lUInt8 *)bitmap->buffer;
            lUInt8 * dst = item->bmp;
            int rowsize = ((w + 15) / 16) * 2;
            for ( int y=0; y<h; y++ ) {
                const lUInt8 * row = ptr;
                mask = 0x80;
                for ( int x=0; x<w; x++ ) {
                    *dst++ = (*row & mask) ? 0xFF : 00;
                    mask >>= 1;
                    if ( !mask && x!=w-1) {
                        mask = 0x80;
                        row++;
                    }
                }
                ptr += rowsize;
            }
        } else {
            memcpy( item->bmp, bitmap->buffer, w*h );
        }
        item->ch = ch;
        item->bmp_width = w;
        item->bmp_height = h;
        item->origin_x =   (lInt8)slot->bitmap_left;
        item->origin_y =   (lInt8)slot->bitmap_top;
        item->advance =    (lUInt8)(slot->metrics.horiAdvance >> 6);
        item->prev_global = NULL;
        item->next_global = NULL;
        item->prev_local = NULL;
        item->next_local = NULL;
        item->local_cache = local_cache;
        return item;
    }
    static void freeItem( LVFontGlyphCacheItem * item )
    {
        free( item );
    }
};

void LVFontLocalGlyphCache::clear()
{
    while ( head ) {
        LVFontGlyphCacheItem * ptr = head;
        remove( ptr );
        global_cache->remove( ptr );
        LVFontGlyphCacheItem::freeItem( ptr );
    }
}

LVFontGlyphCacheItem * LVFontLocalGlyphCache::get( lUInt16 ch )
{
    LVFontGlyphCacheItem * ptr = head;
    for ( ; ptr; ptr = ptr->next_local ) {
        if ( ptr->ch == ch ) {
            global_cache->refresh( ptr );
            return ptr;
        }
    }
    return NULL;
}

void LVFontLocalGlyphCache::put( LVFontGlyphCacheItem * item )
{
    global_cache->put( item );
    item->next_local = head;
    if ( head )
        head->prev_local = item;
    if ( !tail )
        tail = item;
    head = item;
}

/// remove from list, but don't delete
void LVFontLocalGlyphCache::remove( LVFontGlyphCacheItem * item )
{
    if ( item==head )
        head = item->next_local;
    if ( item==tail )
        tail = item->prev_local;
    if ( !head || !tail )
        return;
    if ( item->prev_local )
        item->prev_local->next_local = item->next_local;
    if ( item->next_local )
        item->next_local->prev_local = item->prev_local;
    item->next_local = NULL;
    item->prev_local = NULL;
}

void LVFontGlobalGlyphCache::refresh( LVFontGlyphCacheItem * item )
{
    if ( tail!=item ) {
        //move to head
        remove( item );
        put( item );
    }
}

void LVFontGlobalGlyphCache::put( LVFontGlyphCacheItem * item )
{
    int sz = item->getSize();
    // remove extra items from tail
    while ( sz + size > max_size ) {
        LVFontGlyphCacheItem * removed_item = tail;
        if ( !removed_item )
            break;
        remove( removed_item );
        removed_item->local_cache->remove( removed_item );
        LVFontGlyphCacheItem::freeItem( removed_item );
    }
    // add new item to head
    item->next_global = head;
    if ( head )
        head->prev_global = item;
    head = item;
    if ( !tail )
        tail = item;
    size += sz;
}

void LVFontGlobalGlyphCache::remove( LVFontGlyphCacheItem * item )
{
    if ( item==head )
        head = item->next_global;
    if ( item==tail )
        tail = item->prev_global;
    if ( !head || !tail )
        return;
    if ( item->prev_global )
        item->prev_global->next_global = item->next_global;
    if ( item->next_global )
        item->next_global->prev_global = item->prev_global;
    item->next_global = NULL;
    item->prev_global = NULL;
    size -= item->getSize();
}

void LVFontGlobalGlyphCache::clear()
{
    while ( head ) {
        LVFontGlyphCacheItem * ptr = head;
        remove( ptr );
        ptr->local_cache->remove( ptr );
        LVFontGlyphCacheItem::freeItem( ptr );
    }
}

class LVFreeTypeFace : public LVFont
{
private:
    LVMutex &      _mutex;
    lString8      _fileName;
    lString8      _faceName;
    css_font_family_t _fontFamily;
    FT_Library    _library;
    FT_Face       _face;
    FT_GlyphSlot  _slot;
//    FT_Matrix     _matrix;                 /* transformation matrix */
    int           _size; // height in pixels
    int           _hyphen_width;
    int           _baseline;
    LVFontGlyphWidthCache _wcache;
    LVFontLocalGlyphCache _glyph_cache;
    bool          _drawMonochrome;
    bool          _allowKerning;
public:
    LVFreeTypeFace( LVMutex &mutex, FT_Library  library, LVFontGlobalGlyphCache * globalCache )
    : _mutex(mutex), _fontFamily(css_ff_sans_serif), _library(library), _face(NULL), _size(0), _hyphen_width(0), _baseline(0)
    , _glyph_cache(globalCache), _drawMonochrome(false), _allowKerning(false)
    {
    }

    virtual ~LVFreeTypeFace()
    {
        Clear();
    }

    virtual int getHyphenWidth()
    {
        if ( !_hyphen_width ) {
            _hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );
        }
        return _hyphen_width;
    }

    /// get kerning mode: true==ON, false=OFF
    virtual bool getKerning() { return _allowKerning; }
    /// get kerning mode: true==ON, false=OFF
    virtual void setKerning( bool kerningEnabled ) { _allowKerning = kerningEnabled; }

    /// get bitmap mode (true=bitmap, false=antialiased)
    virtual bool getBitmapMode() { return _drawMonochrome; }
    /// set bitmap mode (true=bitmap, false=antialiased)
    virtual void setBitmapMode( bool drawBitmap )
    {
        if ( _drawMonochrome == drawBitmap )
            return;
        _drawMonochrome = drawBitmap;
        _glyph_cache.clear();
        _wcache.clear();
    }

    bool loadFromFile( const char * fname, int index, int size, css_font_family_t fontFamily, bool monochrome )
    {
        _drawMonochrome = monochrome;
        _fontFamily = fontFamily;
        if ( fname )
            _fileName = fname;
        if ( _fileName.empty() )
            return false;
        int error = FT_New_Face( _library, _fileName.c_str(), index, &_face ); /* create face object */
        if (error)
            return false;
        _slot = _face->glyph;
        _faceName = _face->family_name;
        CRLog::debug("Loaded font %s [%d]: faceName=%s, ", _fileName.c_str(), index, _face->family_name );
        //if ( !FT_IS_SCALABLE( _face ) ) {
        //    Clear();
        //    return false;
       // }
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
            (size * targetheight + nheight/2)/ nheight );  /* pixel_height          */

        _size = size; //(_face->size->metrics.height >> 6);
        _baseline = _size + (_face->size->metrics.descender >> 6);
        if ( error ) {
            // error
            return false;
        }
        return true;
    }

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found 
    */
    virtual bool getGlyphInfo( lUInt16 code, glyph_info_t * glyph )
    {
        LVLock lock(_mutex);
        int glyph_index = FT_Get_Char_Index( _face, code );
        if ( glyph_index==0 )
            return false;
        int error = FT_Load_Glyph(
            _face,          /* handle to face object */
            glyph_index,   /* glyph index           */
            FT_LOAD_DEFAULT );  /* load flags, see below */
        if ( error )
            return false;
        glyph->blackBoxX = (lUInt8)(_slot->metrics.width >> 6);
        glyph->blackBoxY = (lUInt8)(_slot->metrics.height >> 6);
        glyph->originX =   (lInt8)(_slot->metrics.horiBearingX >> 6);
        glyph->originY =   (lInt8)(_slot->metrics.horiBearingY >> 6);
        glyph->width =     (lUInt8)(_slot->metrics.horiAdvance >> 6);
        return true;
    }

    inline int calcCharFlags( lChar16 ch )
    {
        int bflags = 0;
        int isSpace = lvfontIsUnicodeSpace(ch);
        if (isSpace ||  ch == UNICODE_SOFT_HYPHEN_CODE )
            bflags |= LCHAR_ALLOW_WRAP_AFTER;
        if (ch == '-')
            bflags |= LCHAR_DEPRECATED_WRAP_AFTER;
        if (isSpace)
            bflags |= LCHAR_IS_SPACE;
        // TODO: add EOL support to other font engines
        if ( ch=='\r' || ch=='\n' )
            bflags |= LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER;
        return bflags;
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
                        lChar16 def_char,
                        int letter_spacing
                     )
    {
        LVLock lock(_mutex);
        if ( len <= 0 || _face==NULL )
            return 0;
        int error;

#if (ALLOW_KERNING==1)
        int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
        if ( letter_spacing<0 || letter_spacing>50 )
            letter_spacing = 0;

        //int i;

        FT_UInt previous = 0;
        lUInt16 prev_width = 0;
        int nchars = 0;
        int lastFitChar = 0;
        // measure character widths
        for ( nchars=0; nchars<len; nchars++) {
            lChar16 ch = text[nchars];
            bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
            FT_UInt ch_glyph_index = (FT_UInt)-1;
            int kerning = 0;
#if (ALLOW_KERNING==1)
            if ( use_kerning && previous ) {
                if ( ch_glyph_index==(FT_UInt)-1 ){
                    ch_glyph_index = FT_Get_Char_Index( _face, ch );
                    if ( ch_glyph_index==0 )
                        ch_glyph_index = FT_Get_Char_Index( _face, def_char );
                }
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

            flags[nchars] = calcCharFlags( ch );

            /* load glyph image into the slot (erase previous one) */
            int w = _wcache.get(ch);
            if ( w==0xFF ) {
                if ( ch_glyph_index==(FT_UInt)-1 ){
                    ch_glyph_index = FT_Get_Char_Index( _face, ch );
                    if ( ch_glyph_index==0 )
                        ch_glyph_index = FT_Get_Char_Index( _face, def_char );
                }
                error = FT_Load_Glyph( _face,          /* handle to face object */
                        ch_glyph_index,                /* glyph index           */
                        FT_LOAD_DEFAULT );             /* load flags, see below */
                if ( error ) {
                    widths[nchars] = prev_width;
                    continue;  /* ignore errors */
                }
                w = (_slot->metrics.horiAdvance >> 6);
                _wcache.put(ch, w);
            }
            widths[nchars] = prev_width + w + (kerning >> 6) + letter_spacing;
            previous = ch_glyph_index;
            if ( !isHyphen ) // avoid soft hyphens inside text string
                prev_width = widths[nchars];
            if ( prev_width > max_width ) {
                if ( lastFitChar < nchars + 7)
                    break;
            } else {
                lastFitChar = nchars + 1;
            }
        }

        // fill props for rest of chars
        for ( int ii=nchars; ii<len; ii++ ) {
            flags[nchars] = calcCharFlags( text[ii] );
        }

        //maxFit = nchars;

        if ( !_hyphen_width )
            _hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );

        // find last word
        if ( lastFitChar > 3 ) {
            int hwStart, hwEnd;
            lStr_findWordBounds( text, len, lastFitChar-1, hwStart, hwEnd );
            if ( hwStart < lastFitChar-1 && hwEnd > hwStart+3 )
                HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);
        }
        return lastFitChar; //nchars;
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
        LVLock lock(_mutex);
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
                        L' ',  // def_char
                        0
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
        int w = _wcache.get(ch);
        if ( w==0xFF ) {
            int ch_glyph_index = FT_Get_Char_Index( _face, ch );
            if ( ch_glyph_index==0 )
                ch_glyph_index = FT_Get_Char_Index( _face, '?' );
            int error = FT_Load_Glyph( _face,          /* handle to face object */
                    ch_glyph_index,                /* glyph index           */
                    FT_LOAD_DEFAULT );             /* load flags, see below */
            if ( error )
                w = 0;
            else
                w = (_slot->metrics.horiAdvance >> 6);
            _wcache.put(ch, w);
        }
        return w;
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
        return _fontFamily;
    }

    /// draws text string
    virtual void DrawTextString( LVDrawBuf * buf, int x, int y, 
                       const lChar16 * text, int len, 
                       lChar16 def_char, lUInt32 * palette, bool addHyphen, lUInt32 flags, int letter_spacing )
    {
        LVLock lock(_mutex);
        if ( len <= 0 || _face==NULL )
            return;
        if ( letter_spacing<0 || letter_spacing>50 )
            letter_spacing = 0;
        lvRect clip;
        buf->GetClipRect( &clip );
        if ( y + _size < clip.top || y >= clip.bottom )
            return;

        int error;

#if (ALLOW_KERNING==1)
        int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
        int i;

        FT_UInt previous = 0;
        //lUInt16 prev_width = 0;
        lChar16 ch;
        // measure character widths
        bool isHyphen = false;
        int x0 = x;
        for ( i=0; i<=len; i++) {
            if ( i==len && (!addHyphen || isHyphen) )
                break;
            if ( i<len ) {
                ch = text[i];
                isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE) && (i<len-1);
            } else {
                ch = UNICODE_SOFT_HYPHEN_CODE;
                isHyphen = 0;
            }
            FT_UInt ch_glyph_index = FT_Get_Char_Index( _face, ch );
            if ( ch_glyph_index==0 )
                ch_glyph_index = FT_Get_Char_Index( _face, def_char );
            int kerning = 0;
#if (ALLOW_KERNING==1)
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
#endif
            LVFontGlyphCacheItem * item = _glyph_cache.get( ch );
            if ( !item ) {

                int rend_flags = FT_LOAD_RENDER | ( !_drawMonochrome ? FT_LOAD_TARGET_NORMAL : (FT_LOAD_TARGET_MONO) ); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
                /* load glyph image into the slot (erase previous one) */
                error = FT_Load_Glyph( _face,          /* handle to face object */
                        ch_glyph_index,                /* glyph index           */
                        rend_flags );             /* load flags, see below */
                if ( error ) {
                    continue;  /* ignore errors */
                }

                item = LVFontGlyphCacheItem::newItem( &_glyph_cache, ch, _slot, _drawMonochrome );
                _glyph_cache.put( item );
            }
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
        if ( flags & LTEXT_TD_MASK ) {
            // text decoration: underline, etc.
            int h = _size > 30 ? 2 : 1;
            lUInt32 cl = buf->GetTextColor();
            if ( flags & LTEXT_TD_UNDERLINE || flags & LTEXT_TD_BLINK ) {
                int liney = y + _baseline + h;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
            if ( flags & LTEXT_TD_OVERLINE ) {
                int liney = y + h;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
            if ( flags & LTEXT_TD_LINE_THROUGH ) {
                int liney = y + _size/2 - h/2;
                buf->FillRect( x0, liney, x, liney+h, cl );
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
        LVLock lock(_mutex);
        if ( _face )
            FT_Done_Face( _face );
        _face = NULL;
    }

};

#define DEBUG_FONT_MAN 0
#define DEBUG_FONT_MAN_LOG_FILE "/tmp/font_man.log"

class LVFreeTypeFontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
    FT_Library  _library;
    LVFontGlobalGlyphCache _globalCache;
    lString16 _requiredChars;
    #if (DEBUG_FONT_MAN==1)
    FILE * _log;
    #endif
    LVMutex   _lock;
public:

    bool isBitmapModeForSize( int size )
    {
        bool bitmap = false;
        switch ( _antialiasMode ) {
        case font_aa_none:
            bitmap = true;
            break;
        case font_aa_big:
            bitmap = size<20 ? true : false;
            break;
        case font_aa_all:
        default:
            bitmap = false;
            break;
        }
        return bitmap;
    }

    /// set antialiasing mode
    virtual void SetAntialiasMode( int mode )
    { 
        _antialiasMode = mode; 
        gc(); 
        clearGlyphCache();
        LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
        for ( int i=0; i<fonts->length(); i++ ) {
            fonts->get(i)->getFont()->setBitmapMode( isBitmapModeForSize( fonts->get(i)->getFont()->getHeight() ) );
        }
    }

    /// set antialiasing mode
    virtual void setKerning( bool kerning )
    {
    
        _allowKerning = kerning; 
        gc(); 
        clearGlyphCache();
        LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
        for ( int i=0; i<fonts->length(); i++ ) {
            fonts->get(i)->getFont()->setKerning( kerning );
        }
    }
    /// clear glyph cache
    virtual void clearGlyphCache()
    {
        _globalCache.clear();
    }

    virtual int GetFontCount()
    {
        return _cache.length();
    }

    virtual ~LVFreeTypeFontManager() 
    {
        _globalCache.clear();
        _cache.clear();
        if ( _library )
            FT_Done_FreeType( _library );
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            fclose(_log);
        }
    #endif
    }

    LVFreeTypeFontManager()
    : _library(NULL), _globalCache(GLYPH_CACHE_SIZE)
    {
        int error = FT_Init_FreeType( &_library );
        if ( error ) {
            // error
        }
    #if (DEBUG_FONT_MAN==1)
        _log = fopen(DEBUG_FONT_MAN_LOG_FILE, "at");
        if ( _log ) {
            fprintf(_log, "=========================== LOGGING STARTED ===================\n");
        }
    #endif
        _requiredChars = L"azAZ\x0410\x042F\x0430\x044F";
    }

    virtual void gc() // garbage collector
    {
        _cache.gc();
    }

    lString8 makeFontFileName( lString8 name )
    {
        lString8 filename = _path;
        if (!filename.empty() && _path[_path.length()-1]!=PATH_SEPARATOR_CHAR)
            filename << PATH_SEPARATOR_CHAR;
        filename << name;
        return filename;
    }

    /// returns available typefaces
    virtual void getFaceList( lString16Collection & list )
    {
        _cache.getFaceList( list );
    }

    virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface )
    {
    //#if (DEBUG_FONT_MAN==1)
    //    if ( _log ) {
        //CRLog::debug("GetFont(size=%d, weight=%d, italic=%d, family=%d, typeface='%s')\n",
        //        size, weight, italic?1:0, (int)family, typeface.c_str() );
    //    }
    //#endif
        lString8 fontname;
        LVFontDef def( 
            fontname,
            size,
            weight,
            italic,
            family,
            typeface,
            -1
        );
        //fprintf( _log, "GetFont: %s %d %s %s\n",
        //    typeface.c_str(),
        //    size,
        //    weight>400?"bold":"",
        //    italic?"italic":"" );
        LVFontCacheItem * item = _cache.find( &def );
    //#if (DEBUG_FONT_MAN==1)
    //    if ( _log && item ) {
        /*
        CRLog::debug("   found item: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s) FontRef=%d\n",
                item->getDef()->getName().c_str(), item->getDef()->getIndex(), item->getDef()->getSize(), item->getDef()->getWeight(), item->getDef()->getItalic()?1:0, (int)item->getDef()->getFamily(), item->getDef()->getTypeFace().c_str(), item->getFont().isNull()?0:item->getFont()->getHeight()
            );
        */
    //    }
    //#endif
        if (!item->getFont().isNull())
        {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
        lString8 fname = item->getDef()->getName();
        //int index = item->getDef()->getIndex();
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            fprintf(_log, "   no instance: adding new one for filename=%s, index = %d\n", fname.c_str() );
        }
    #endif
        LVFreeTypeFace * font = new LVFreeTypeFace(_lock, _library, &_globalCache);
        lString8 pathname = makeFontFileName( fname );
        //def.setName( fname );
        //def.setIndex( index );

        //if ( fname.empty() || pathname.empty() ) {
        //    pathname = lString8("arial.ttf");
        //}

        //printf("going to load font file %s\n", fname.c_str());
        if (font->loadFromFile( pathname.c_str(), item->getDef()->getIndex(), size, family, isBitmapModeForSize(size) ) )
        {
            //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
            //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
            LVFontRef ref(font);
            font->setKerning( getKerning() );
            LVFontDef newDef(*item->getDef());
            newDef.setSize( size );
            //item->setFont( ref );
            //_cache.update( def, ref );
            _cache.update( &newDef, ref );
            int rsz = ref->getHeight();
            if ( rsz!=size ) {
                size++;
            }
            //delete def;
            return ref;
        }
        else
        {
            //printf("    not found!\n");
        }
        //delete def;
        delete font;
        return LVFontRef(NULL);
    }

    bool checkCharSet( FT_Face face )
    {
        // TODO: check existance of required characters (e.g. cyrillic)
        if (face==NULL)
            return false; // invalid face
        for ( unsigned i=0; i<_requiredChars.length(); i++ ) {
            lChar16 ch = _requiredChars[i];
            FT_UInt ch_glyph_index = FT_Get_Char_Index( face, ch );
            if ( ch_glyph_index==0 )
                return false; // no required char!!!
        }
        return true;
    }

    /*
    bool isMonoSpaced( FT_Face face )
    {
        // TODO: check existance of required characters (e.g. cyrillic)
        if (face==NULL)
            return false; // invalid face
        lChar16 ch1 = 'i';
        FT_UInt ch_glyph_index1 = FT_Get_Char_Index( face, ch1 );
        if ( ch_glyph_index1==0 )
            return false; // no required char!!!
        int w1, w2;
        int error1 = FT_Load_Glyph( face,  //        /* handle to face object 
                ch_glyph_index1,           //     /* glyph index           
                FT_LOAD_DEFAULT );         //    /* load flags, see below 
        if ( error1 )
            w1 = 0;
        else
            w1 = (face->glyph->metrics.horiAdvance >> 6);
        int error2 = FT_Load_Glyph( face,  //        /* handle to face object 
                ch_glyph_index2,           //     /* glyph index           
                FT_LOAD_DEFAULT );         //    /* load flags, see below 
        if ( error2 )
            w2 = 0;
        else
            w2 = (face->glyph->metrics.horiAdvance >> 6);

        lChar16 ch2 = 'W';
        FT_UInt ch_glyph_index2 = FT_Get_Char_Index( face, ch2 );
        if ( ch_glyph_index2==0 )
            return false; // no required char!!!
        return w1==w2;
    }
    */

    virtual bool RegisterFont( lString8 name )
    {
#ifdef LOAD_TTF_FONTS_ONLY
        if ( name.pos( lString8(".ttf") ) < 0 && name.pos( lString8(".TTF") ) < 0 )
            return false; // load ttf fonts only
#endif
        lString8 fname = makeFontFileName( name );
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            fprintf(_log, "RegisterFont( %s ) path=%s\n",
                name.c_str(), fname.c_str()
            );
        }
    #endif
        bool res = false;

        int index = 0;

        FT_Face face = NULL;

        // for all faces in file
        for ( ;; index++ ) {
            int error = FT_New_Face( _library, fname.c_str(), index, &face ); /* create face object */
            if ( error )
                break;
            bool scal = FT_IS_SCALABLE( face );
            bool charset = checkCharSet( face );
            //bool monospaced = isMonoSpaced( face );
            if ( !scal || !charset ) {
    //#if (DEBUG_FONT_MAN==1)
     //           if ( _log ) {
                CRLog::debug("    won't register font %s: %s",
                    name.c_str(), !charset?"no mandatory characters in charset" : "font is not scalable"
                    );
    //            }
    //#endif
                if ( face ) {
                    FT_Done_Face( face );
                    face = NULL;
                }
                break;
            }
            int num_faces = face->num_faces;

            css_font_family_t fontFamily = css_ff_sans_serif;
            if ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
                fontFamily = css_ff_monospace;
            lString8 familyName( face->family_name );
            if ( familyName=="Times" || familyName=="Times New Roman" )
                fontFamily = css_ff_serif;

            LVFontDef def(
                name,
                -1, // height==-1 for scalable fonts
                ( face->style_flags & FT_STYLE_FLAG_BOLD ) ? 700 : 300,
                ( face->style_flags & FT_STYLE_FLAG_ITALIC ) ? true : false,
                fontFamily,
                familyName,
                index
            );
    //#if (DEBUG_FONT_MAN==1)
    //    if ( _log ) {
            CRLog::debug("registering font: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                def.getName().c_str(), def.getIndex(), def.getSize(), def.getWeight(), def.getItalic()?1:0, (int)def.getFamily(), def.getTypeFace().c_str()
            );
    //    }
    //#endif
            if ( _cache.findDuplicate( &def ) )
                return false;
            _cache.update( &def, LVFontRef(NULL) );
            res = true;

            if ( face ) {
                FT_Done_Face( face );
                face = NULL;
            }

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
    
    virtual bool RegisterFont( const LOGFONTA * lf )
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
        LOGFONTA lf;
        memset(&lf, 0, sizeof(lf));
        lf.lfCharSet = ANSI_CHARSET;
        int res = 
        EnumFontFamiliesExA(
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

#if (USE_BITMAP_FONTS==1)

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

#endif

bool InitFontManager( lString8 path )
{
    if ( fontMan ) {
        delete fontMan;
    }
#if (USE_WIN32_FONTS==1)
    fontMan = new LVWin32FontManager;
#elif (USE_FREETYPE==1)
    fontMan = new LVFreeTypeFontManager;
#else
    fontMan = new LVBitmapFontManager;
#endif
    return fontMan->Init( path );
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

int LVFontDef::CalcDuplicateMatch( const LVFontDef & def ) const
{
    bool size_match = (_size==-1 || def._size==-1) ? true 
        : (def._size == _size);
    bool weight_match = (_weight==-1 || def._weight==-1) ? true 
        : (def._weight == _weight);
    bool italic_match = (_italic == def._italic || _italic==-1 || def._italic==-1);
    bool family_match = (_family==css_ff_inherit || def._family==css_ff_inherit || def._family == def._family);
    bool typeface_match = (_typeface == def._typeface);
    return size_match && weight_match && italic_match && family_match && typeface_match;
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
        + (size_match     * 1000)
        + (weight_match   * 100)
        + (italic_match   * 100)
        + (family_match   * 10000)
        + (typeface_match * 100000);
}








void LVBaseFont::DrawTextString( LVDrawBuf * buf, int x, int y, 
                   const lChar16 * text, int len, 
                   lChar16 def_char, lUInt32 * palette, bool addHyphen, lUInt32 flags, int letter_spacing )
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
                    lChar16 def_char,
                    int letter_spacing
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

LVFontCacheItem * LVFontCache::findDuplicate( const LVFontDef * def )
{
    for (int i=0; i<_registered_list.length(); i++)
    {
        if ( _registered_list[i]->_def.CalcDuplicateMatch( *def ) )
            return _registered_list[i];
    }
    return NULL;
}

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
    if ( ref.isNull() )
        printf("Adding null font instance!");
    LVFontCacheItem * item = new LVFontCacheItem(*def);
    item->_fnt = ref;
    _instance_list.add( item );
}

void LVFontCache::update( const LVFontDef * def, LVFontRef ref )
{
    int i;
    if ( !ref.isNull() ) {
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
        // add new
        //LVFontCacheItem * item;
        //item = new LVFontCacheItem(*def);
        addInstance( def, ref );
    } else {
        for (i=0; i<_registered_list.length(); i++)
        {
            if ( _registered_list[i]->_def == *def )
            {
                return;
            }
        }
        // add new
        LVFontCacheItem * item;
        item = new LVFontCacheItem(*def);
        _registered_list.add( item );
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

bool LVBaseWin32Font::Create( const LOGFONTA & lf )
{
    if (!IsNull())
        Clear();
    memcpy( &_logfont, &lf, sizeof(LOGFONTA));
    _hfont = CreateFontIndirectA( &lf );
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
    GetTextFaceA( _drawbuf.GetDC(), sizeof(_logfont.lfFaceName)-1, _logfont.lfFaceName );
    _height = tm.tmHeight;
    _baseline = _height - tm.tmDescent;
    return true;
}

bool LVBaseWin32Font::Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface )
{
    if (!IsNull())
        Clear();
    //
    LOGFONTA lf;
    memset(&lf, 0, sizeof(LOGFONTA));
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
    _hfont = CreateFontIndirectA( &lf );
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
    GetTextFaceA( _drawbuf.GetDC(), sizeof(_logfont.lfFaceName)-1, _logfont.lfFaceName );
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
                    lChar16 def_char,
                    int letter_spacing
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
        if (ch=='.' || ch==',' || ch=='!' || ch=='?' || ch=='?' || ch==':' || ch==';')
            break;
        
    }
    HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);

    return nchars;
}

/// draws text string
void LVWin32DrawFont::DrawTextString( LVDrawBuf * buf, int x, int y, 
                   const lChar16 * text, int len, 
                   lChar16 def_char, lUInt32 * palette, bool addHyphen, lUInt32 flags, int letter_spacing )
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
                    lChar16 def_char,
                    int letter_spacing
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

bool LVWin32Font::Create( const LOGFONTA & lf )
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
