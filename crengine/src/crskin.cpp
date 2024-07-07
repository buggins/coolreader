/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2008-2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2011 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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
 * \file crskin.cpp
 * \brief skinning file support
 */

#include <stdlib.h>
#include "../include/crskin.h"
#include "../include/lvstsheet.h"
#include "../include/crtrace.h"
#include "../include/lvdrawstatesaver.h"
#include "../include/lvstreamutils.h"

// uncomment to trace skin XML access errors / not found elements
//#define TRACE_SKIN_ERRORS


class RecursionLimit
{
static int counter;
public:
    bool test( int limit = 15 ) { return counter < limit; }
    RecursionLimit() { counter++; }
    ~RecursionLimit() { counter--; }
};
int RecursionLimit::counter = 0;


/// decodes skin percent
int fromSkinPercent( int x, int fullx )
{
    if ( x>0 && (x & SKIN_COORD_PERCENT_FLAG) ) {
        x ^= SKIN_COORD_PERCENT_FLAG;
        return x * fullx / 10000;
    } else {
        if ( x<0 ) {
            if ( !(x & SKIN_COORD_PERCENT_FLAG) ) {
                x ^= SKIN_COORD_PERCENT_FLAG;
                x = 10000-x;
                return x * fullx / 10000;
            }
            return fullx + x;
        }
        return x;
    }
}

/// decodes skin percent point to pixels (fullx is value corresponding to 100%)
lvPoint fromSkinPercent( lvPoint pt, lvPoint fullpt )
{
    lvPoint res;
    res.x = fromSkinPercent( pt.x, fullpt.x );
    res.y = fromSkinPercent( pt.y, fullpt.y );
    return res;
}

/// encodes percent value*100 (0..10000), to store in skin, from string like "75%" or "10"
int toSkinPercent( const lString32 & value, int defValue, bool * res )
{
    // "75%" format - in percent
    int p = value.pos("%");
    int pvalue;
    if ( p>0 ) {
        if ( value.substr(0, p).atoi(pvalue) ) {
            if ( res )
                *res = true;
            return toSkinPercent(pvalue*100);
        }
    }
    // "75px" format - in pixels
    p = value.pos("px");
    if ( p>0 ) {
        if ( value.substr(0, p).atoi(pvalue) ) {
            if ( res )
                *res = true;
            return pvalue;
        }
    }
    // simple "75" format
    if ( value.atoi(pvalue) ) {
        if ( res )
            *res = true;
        return pvalue;
    }
    return defValue;
}

CRPageSkinRef CRPageSkinList::findByName( const lString32 & name )
{
    for ( int i=0; i<length(); i++ ) {
        if ( get(i)->getName()==name )
            return get(i);
    }
    return CRPageSkinRef();
}

CRPageSkin::CRPageSkin()
: _scrollSkin(new CRRectSkin())
, _leftPageSkin(new CRRectSkin())
, _rightPageSkin(new CRRectSkin())
, _singlePageSkin(new CRRectSkin())
, _name("Default")
{

}

CRRectSkinRef CRPageSkin::getSkin( page_skin_type_t type )
{
    switch ( type ) {
    case PAGE_SKIN_SCROLL:
        return _scrollSkin;
    case PAGE_SKIN_LEFT_PAGE:
        return _leftPageSkin;
    case PAGE_SKIN_RIGHT_PAGE:
        return _rightPageSkin;
    case PAGE_SKIN_SINGLE_PAGE:
        return _singlePageSkin;
    default:
        return _scrollSkin;
    }
}


CRIconSkin::CRIconSkin()
: _bgcolor(0xFF000000) // transparent
, _hTransform(IMG_TRANSFORM_SPLIT)
, _vTransform(IMG_TRANSFORM_SPLIT)
, _splitPoint(-1, -1)
, _pos(0, 0)
, _size(toSkinPercent( 10000 ), toSkinPercent( 10000 )) // 100% x 100%
, _align(SKIN_VALIGN_TOP|SKIN_HALIGN_LEFT) // relative to top left
{
}

bool CRRectSkin::getRect( lvRect & rc, const lvRect & baseRect )
{
    rc = baseRect;
    lvPoint pos( fromSkinPercent( _pos.x, rc.width()),
                 fromSkinPercent( _pos.y, rc.height()) );
    lvPoint sz( fromSkinPercent( _size.x, rc.width()),
                fromSkinPercent( _size.y, rc.height()) );

    // left top corner -> origin point
    if ( getHAlign()==SKIN_HALIGN_RIGHT )
        pos.x = pos.x + sz.x;
    else if ( getHAlign()==SKIN_HALIGN_CENTER ) {
        pos.x = pos.x + sz.x / 2;
    }
    if ( getVAlign()==SKIN_VALIGN_BOTTOM )
        pos.y = pos.y + sz.y;
    else if ( getVAlign()==SKIN_VALIGN_CENTER ) {
        pos.y = pos.y + sz.y/2;
    }

    // apply size constraints
    if ( _minsize.x>0 && sz.x < _minsize.x )
        sz.x = _minsize.x;
    if ( _minsize.y>0 && sz.y < _minsize.y )
        sz.y = _minsize.y;
    if ( _maxsize.x>0 && sz.x > _maxsize.x )
        sz.x = _maxsize.x;
    if ( _maxsize.y>0 && sz.y > _maxsize.y )
        sz.y = _maxsize.y;

    // origin -> left top corner
    if ( getHAlign()==SKIN_HALIGN_RIGHT )
        pos.x = pos.x - sz.x;
    else if ( getHAlign()==SKIN_HALIGN_CENTER ) {
        pos.x = pos.x - sz.x / 2;
    }
    if ( getVAlign()==SKIN_VALIGN_BOTTOM )
        pos.y = pos.y - sz.y;
    else if ( getVAlign()==SKIN_VALIGN_CENTER ) {
        pos.y = pos.y - sz.y/2;
    }

    pos.x += baseRect.left;
    pos.y += baseRect.top;
    rc.left = pos.x;
    rc.top = pos.y;
    rc.right = pos.x + sz.x;
    rc.bottom = pos.y + sz.y;
    return true;
}

void CRIconSkin::draw( LVDrawBuf & buf, const lvRect & rc )
{
    int dx = _image.isNull() ? 0 : _image->GetWidth();
    int dy = _image.isNull() ? 0 : _image->GetHeight();
    lvRect rc2(rc);
    rc2.left = rc.left + fromSkinPercent( _pos.x, rc.width() );
    rc2.top = rc.top + fromSkinPercent( _pos.y, rc.height() );
    rc2.right = rc2.left + fromSkinPercent( _size.x, rc.width() );
    rc2.bottom = rc2.top + fromSkinPercent( _size.y, rc.height() );
    if ( _hTransform==IMG_TRANSFORM_NONE ) {
        int ddx = rc2.width()-dx;
        if ( getHAlign()==SKIN_HALIGN_RIGHT )
            rc2.left = rc2.right - dx;
        else if ( getHAlign()==SKIN_HALIGN_CENTER ) {
            rc2.left += ddx/2;
            rc2.right = rc2.left + dx;
        } else
            rc2.right = rc2.left + dx;
    }
    if ( _vTransform==IMG_TRANSFORM_NONE ) {
        int ddy = rc2.height()-dy;
        if ( getVAlign()==SKIN_VALIGN_BOTTOM )
            rc2.top = rc2.bottom - dy;
        else if ( getVAlign()==SKIN_VALIGN_CENTER ) {
            rc2.top += ddy/2;
            rc2.bottom = rc2.top + dy;
        } else
            rc2.bottom = rc2.top + dy;
    }
    if ( _image.isNull() ) {
        if ( ((_bgcolor>>24)&255) != 255 )
            buf.FillRect( rc2, _bgcolor );
    } else {
        LVImageSourceRef img = LVCreateStretchFilledTransform( _image,
            rc2.width(), rc2.height(), _hTransform, _vTransform, _splitPoint.x, _splitPoint.y );
        LVDrawStateSaver saver(buf);
		lvRect oldClip;
		buf.GetClipRect(&oldClip);
		if (oldClip.isEmpty())
			buf.SetClipRect(&rc);
		else if (oldClip.intersect(rc))
			buf.SetClipRect(&oldClip);
		else
			return;
        buf.Draw( img, rc2.left, rc2.top, rc2.width(), rc2.height(), false );
    }
}

void CRIconList::draw( LVDrawBuf & buf, const lvRect & rc )
{
    //CRLog::trace("enter CRIconList::draw(%d images)", _list.length());
    for ( int i=0; i<_list.length(); i++ )
        _list[i]->draw( buf, rc );
    //CRLog::trace("exit CRIconList::draw()");
}

/// retuns path to base definition, if attribute base="#nodeid" is specified for element of path
lString32 CRSkinContainer::getBasePath( const lChar32 * path )
{
    lString32 res;
    ldomXPointer p = getXPointer( lString32( path ) );
    if ( !p )
        return res;
    if ( !p.getNode()->isElement() )
        return res;
    lString32 value = p.getNode()->getAttributeValue("base");
    if (value.empty() || value[0] != '#')
        return res;
    res = pathById( value.c_str() + 1 );
    crtrace log;
    log << "CRSkinContainer::getBasePath( " << lString32( path ) << " ) = " << res;
    return res;
}

/// skin file support
class CRSkinImpl : public CRSkinContainer
{
protected:
    LVContainerRef _container;
    LVAutoPtr<ldomDocument> _doc;
    LVCacheMap<lString32,LVImageSourceRef> _imageCache;
    LVCacheMap<lString32,CRRectSkinRef> _rectCache;
    LVCacheMap<lString32,CRScrollSkinRef> _scrollCache;
    LVCacheMap<lString32,CRWindowSkinRef> _windowCache;
    LVCacheMap<lString32,CRMenuSkinRef> _menuCache;
    LVCacheMap<lString32,CRPageSkinRef> _pageCache;
    LVCacheMap<lString32,CRToolBarSkinRef> _toolbarCache;
    CRPageSkinListRef _pageSkinList;
public:
    /// returns scroll skin by path or #id
    virtual CRScrollSkinRef getScrollSkin( const lChar32 * path );
    /// returns rect skin by path or #id
    virtual CRRectSkinRef getRectSkin( const lChar32 * path );
    /// returns window skin by path or #id
    virtual CRWindowSkinRef getWindowSkin( const lChar32 * path );
    /// returns menu skin by path or #id
    virtual CRMenuSkinRef getMenuSkin( const lChar32 * path );
    /// returns book page skin by path or #id
    virtual CRPageSkinRef getPageSkin( const lChar32 * path );
    /// returns book page skin list
    virtual CRPageSkinListRef getPageSkinList();
    /// return ToolBar skin by path or #id
	virtual CRToolBarSkinRef getToolBarSkin( const lChar32 * path );
    /// get DOM path by id
    virtual lString32 pathById( const lChar32 * id );
    /// gets image from container
    virtual LVImageSourceRef getImage( const lChar32 * filename );
    /// gets doc pointer by asolute path
    virtual ldomXPointer getXPointer( const lString32 & xPointerStr ) { return _doc->createXPointer( xPointerStr ); }
    /// garbage collection
    virtual void gc()
    {
        _imageCache.clear();
    }
    /// constructor does nothing
    CRSkinImpl()  : _imageCache(8), _rectCache(8), _scrollCache(1), _windowCache(8), _menuCache(8), _pageCache(8), _toolbarCache(2) { }
    virtual ~CRSkinImpl(){ }
    // open from container
    virtual bool open( LVContainerRef container );
    virtual bool open( lString8 simpleXml );
};


/* XPM */
static const char *menu_item_background[] = {
/* width height num_colors chars_per_pixel */
"44 48 5 1",
/* colors */
"  c None",
". c #000000",
"o c #555555",
"0 c #AAAAAA",
"# c #ffffff",
/* pixels               ..                       */
"                                            ",
"                                            ",
"                                            ",
"                                            ",
"oooooooooooooooooooooooooooooooooooooooooooo",
"oooooooooooooooooooooooooooooooooooooooooooo",
"oooooooooooooooooooooooooooooooooooooooooooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"ooo######################################ooo",
"oooooooooooooooooooooooooooooooooooooooooooo",
"oooooooooooooooooooooooooooooooooooooooooooo",
"oooooooooooooooooooooooooooooooooooooooooooo",
"                                            ",
"                                            ",
"                                            ",
"                                            ",
};

/* XPM */
static const char *menu_shortcut_background[] = {
/* width height num_colors chars_per_pixel */
"36 48 5 1",
/* colors */
"  c None",
". c #000000",
"o c #555555",
"0 c #AAAAAA",
"# c #ffffff",
/* pixels               ..                       */
"                                    ",
"                                    ",
"                                    ",
"                                    ",
"                oooooooooooooooooooo",
"             ooooooooooooooooooooooo",
"          oooooooooooooooooooooooooo",
"        oooooooo####################",
"      oooooo########################",
"     oooo###########################",
"    oooo############################",
"   ooo##############################",
"   ooo##############################",
"  ooo###############################",
"  ooo###############################",
"  ooo###############################",
" ooo################################",
" ooo################################",
" ooo################################",
"ooo#################################",
"ooo#################################",
"ooo#################################",
"ooo#################################",
"ooo#################################",//==
"ooo#################################",//==
"ooo#################################",
"ooo#################################",
"ooo#################################",
"ooo#################################",
" ooo################################",
" ooo################################",
" ooo################################",
"  ooo###############################",
"  ooo###############################",
"  ooo###############################",
"   ooo##############################",
"   ooo##############################",
"    ooo#############################",
"     oooo###########################",
"      oooooo########################",
"       oooooooo#####################",
"         ooooooooooooooooooooooooooo",
"            oooooooooooooooooooooooo",
"               ooooooooooooooooooooo",
"                                    ",
"                                    ",
"                                    ",
"                                    ",
};


typedef struct {
    const lChar32 * filename;
    const char * * xpm;
} standard_image_item_t;

static standard_image_item_t standard_images [] = {
    { U"std_menu_shortcut_background.xpm", menu_shortcut_background },
    { U"std_menu_item_background.xpm", menu_item_background },
    { NULL, NULL }
};

/// gets image from container
LVImageSourceRef CRSkinImpl::getImage(  const lChar32 * filename  )
{
    LVImageSourceRef res;
    lString32 fn( filename );
    if ( _imageCache.get( fn, res ) )
        return res; // found in cache

    bool standard = false;
    for ( int i=0; standard_images[i].filename; i++ )
        if ( !lStr_cmp( filename, standard_images[i].filename ) ) {
            res = LVCreateXPMImageSource( standard_images[i].xpm );
            standard = true;
        }
    if ( !standard && !!_container ) {
        LVStreamRef stream = _container->OpenStream( filename, LVOM_READ );
        if ( !!stream ) {
            if ( stream->GetSize() < MAX_SKIN_IMAGE_CACHE_ITEM_RAM_COPY_PACKED_SIZE )
                res = LVCreateStreamCopyImageSource( stream );
            else
                res = LVCreateStreamImageSource( stream );
            // try to hold unpacked image, if small enough
            res = LVCreateUnpackedImageSource( res, MAX_SKIN_IMAGE_CACHE_ITEM_UNPACKED_SIZE, COLOR_BACKBUFFER==0 );
        }
    }
    // add found image to cache
    _imageCache.set( fn, res );
    return res;
}

// open from container
bool CRSkinImpl::open( LVContainerRef container )
{
    if ( container.isNull() )
        return false;
    LVStreamRef stream = container->OpenStream( U"cr3skin.xml", LVOM_READ );
    if ( stream.isNull() ) {
        CRLog::error("cannot open skin: cr3skin.xml not found");
        return false;
    }
    ldomDocument * doc = LVParseXMLStream( stream );
    if ( !doc ) {
        CRLog::error("cannot open skin: error while parsing cr3skin.xml");
        return false;
    }
    _doc = doc;
    _container = container;
    return true;
}

bool CRSkinImpl::open( lString8 simpleXml )
{
    LVStreamRef stream = LVCreateStringStream( simpleXml );
    ldomDocument * doc = LVParseXMLStream( stream );
    if ( !doc ) {
        CRLog::error("cannot open skin: error while parsing skin xml");
        return false;
    }
    _doc = doc;
    return true;
}

/// reads string value from attrname attribute of element specified by path, returns empty string if not found
lString32 CRSkinContainer::readString( const lChar32 * path, const lChar32 * attrname, bool * res )
{
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr )
        return lString32::empty_str;
    if ( !ptr.getNode()->isElement() )
        return lString32::empty_str;
	//lString32 pnname = ptr.getNode()->getParentNode()->getNodeName();
	//lString32 nname = ptr.getNode()->getNodeName();
    lString32 value = ptr.getNode()->getAttributeValue(attrname);
	if ( res )
		*res = true;
    return value;
}

/// reads string value from attrname attribute of element specified by path, returns defValue if not found
lString32 CRSkinContainer::readString( const lChar32 * path, const lChar32 * attrname, const lString32 & defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
	if ( res )
		*res = true;
    return value;
}

/// reads color value from attrname attribute of element specified by path, returns defValue if not found
lUInt32 CRSkinContainer::readColor( const lChar32 * path, const lChar32 * attrname, lUInt32 defValue, bool * res  )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    css_length_t cv;
    lString8 buf = UnicodeToUtf8(value);
    const char * bufptr = buf.modify();
    if ( !parse_color_value( bufptr, cv ) )
        return defValue;
	if ( res )
		*res = true;
    return cv.value;
}

/// reads rect value from attrname attribute of element specified by path, returns defValue if not found
lvRect CRSkinContainer::readRect( const lChar32 * path, const lChar32 * attrname, lvRect defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    lvRect p = defValue;
    lString32 s1, s2, s3, s4, s;
    s = value;
    if ( !s.split2(",", s1, s2) )
        return p;
    s1.trim();
    s2.trim();
    s = s2;
    if ( !s.split2(",", s2, s3) )
        return p;
    s2.trim();
    s3.trim();
    s = s3;
    if ( !s.split2(",", s3, s4) )
        return p;
    s3.trim();
    s4.trim();

    bool b1=false;
    bool b2=false;
    bool b3=false;
    bool b4=false;
    p.left = toSkinPercent( s1, defValue.left, &b1 );
    p.top = toSkinPercent( s2, defValue.top, &b2 );
    p.right = toSkinPercent( s3, defValue.right, &b3 );
    p.bottom = toSkinPercent( s4, defValue.bottom, &b4 );
    if ( b1 && b2 && b3 && b4) {
        if ( res )
            *res = true;
        return p;
    }
    return defValue;
}

/// reads boolean value from attrname attribute of element specified by path, returns defValue if not found
bool CRSkinContainer::readBool( const lChar32 * path, const lChar32 * attrname, bool defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if (value.empty())
        return defValue;
    if (value == "true" || value == "yes")
        return true;
    if (value == "false" || value == "no")
        return false;
	if ( res )
		*res = true;
    return defValue;
}

/// reads image transform value from attrname attribute of element specified by path, returns defValue if not found
ImageTransform CRSkinContainer::readTransform( const lChar32 * path, const lChar32 * attrname, ImageTransform defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    value.lowercase();
    if (value == "none") {
        if ( res )
            *res = true;
        return IMG_TRANSFORM_NONE;
    }
    if (value == "split") {
        if ( res )
            *res = true;
        return IMG_TRANSFORM_SPLIT;
    }
    if (value == "stretch") {
        if ( res )
            *res = true;
        return IMG_TRANSFORM_STRETCH;
    }
    if (value == "tile") {
        if ( res )
            *res = true;
        return IMG_TRANSFORM_TILE;
    }
    // invalid value
    return defValue;
}

/// reads h align value from attrname attribute of element specified by path, returns defValue if not found
int CRSkinContainer::readHAlign( const lChar32 * path, const lChar32 * attrname, int defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if (value.empty())
        return defValue;
    if (value == "left") {
		if ( res )
			*res = true;
        return SKIN_HALIGN_LEFT;
	}
    if (value == "center") {
		if ( res )
			*res = true;
        return SKIN_HALIGN_CENTER;
	}
    if (value == "right") {
		if ( res )
			*res = true;
        return SKIN_HALIGN_RIGHT;
	}
    // invalid value
    return defValue;
}

/// reads h align value from attrname attribute of element specified by path, returns defValue if not found
int CRSkinContainer::readVAlign( const lChar32 * path, const lChar32 * attrname, int defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if (value.empty())
        return defValue;
    if (value == "top") {
		if ( res )
			*res = true;
        return SKIN_VALIGN_TOP;
	}
    if (value == "center") {
		if ( res )
			*res = true;
        return SKIN_VALIGN_CENTER;
	}
    if (value == "bottom") {
		if ( res )
			*res = true;
        return SKIN_VALIGN_BOTTOM;
	}
    // invalid value
    return defValue;
}

/// reads int value from attrname attribute of element specified by path, returns defValue if not found
int CRSkinContainer::readInt( const lChar32 * path, const lChar32 * attrname, int defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    value.trim();
    return toSkinPercent( value, defValue, res);
}

/// reads point(size) value from attrname attribute of element specified by path, returns defValue if not found
lvPoint CRSkinContainer::readSize( const lChar32 * path, const lChar32 * attrname, lvPoint defValue, bool * res )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    lvPoint p = defValue;
    lString32 s1, s2;
    if ( !value.split2(",", s1, s2) )
        return p;
    s1.trim();
    s2.trim();
    bool b1=false;
    bool b2=false;
    p.x = toSkinPercent( s1, defValue.x, &b1 );
    p.y = toSkinPercent( s2, defValue.y, &b2 );
    if ( b1 && b2 ) {
        if ( res )
            *res = true;
        return p;
    }
    return defValue;
}

/// reads rect value from attrname attribute of element specified by path, returns null ref if not found
LVImageSourceRef CRSkinContainer::readImage( const lChar32 * path, const lChar32 * attrname, bool * r )
{
    lString32 value = readString( path, attrname );
    if ( value.empty() ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "CRSkinContainer::readImage( " << path << ", " << attrname << ") - attribute or element not found";
#endif
        return LVImageSourceRef();
    }
    LVImageSourceRef res = getImage( value );
    if ( res.isNull() ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Image " << value << " cannot be read";
#endif
	} else {
		if ( r )
			*r = true;
	}
    return res;
}

/// reads rect value from attrname attribute of element specified by path, returns null ref if not found
CRIconListRef CRSkinContainer::readIcons( const lChar32 * path, bool * r )
{
    CRIconListRef list = CRIconListRef(new CRIconList() );
    for ( int i=1; i<16; i++ ) {
        lString32 p = lString32(path) << "[" << fmt::decimal(i) << "]";
        CRIconSkin * icon = new CRIconSkin();
        if ( readIconSkin(p.c_str(), icon ) )
            list->add( CRIconSkinRef(icon) );
        else {
            delete icon;
            break;
        }
    }
    if ( list->length()==0 ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "CRSkinContainer::readIcons( " << path << ") - cannot read icon from specified path";
#endif
        return CRIconListRef();
    }
    if ( r )
        *r = true;
    return list;
}

/// open simple skin, without image files, from string
CRSkinRef LVOpenSimpleSkin( const lString8 & xml )
{
    CRSkinImpl * skin = new CRSkinImpl();
    CRSkinRef res( skin );
    if ( !skin->open( xml ) )
        return CRSkinRef();
    //CRLog::trace("skin xml opened ok");
    return res;
}

/// opens skin from directory or .zip file
CRSkinRef LVOpenSkin( const lString32 & pathname )
{
    LVContainerRef container = LVOpenDirectory( pathname.c_str() );
    if ( !container ) {
        LVStreamRef stream = LVOpenFileStream( pathname.c_str(), LVOM_READ );
        if ( stream.isNull() ) {
            CRLog::error("cannot open skin: specified archive or directory not found");
            return CRSkinRef();
        }
        container = LVOpenArchieve( stream );
        if ( !container ) {
            CRLog::error("cannot open skin: specified archive or directory not found");
            return CRSkinRef();
        }
    }
    CRSkinImpl * skin = new CRSkinImpl();
    CRSkinRef res( skin );
    if ( !skin->open( container ) )
        return CRSkinRef();
    CRLog::trace("skin container %s opened ok", LCSTR(pathname) );
    return res;
}

// default parameters
//LVFontRef CRSkinnedItem::getFont() { return fontMan->GetFont( 24, 300, false, css_ff_sans_serif, cs8("Arial")) }

void CRSkinnedItem::draw( LVDrawBuf & buf, const lvRect & rc )
{
    SAVE_DRAW_STATE( buf );
	buf.SetBackgroundColor( getBackgroundColor() );
	buf.SetTextColor( getTextColor() );
    //CRLog::trace("CRSkinnedItem::draw before getBgIcons()");
    CRIconListRef bgimg = getBgIcons();
	if ( bgimg.isNull() ) {
        //buf.FillRect( rc, getBackgroundColor() );
	} else {
        bgimg->draw( buf, rc );
	}
}


lvRect CRRectSkin::getClientRect( const lvRect &windowRect )
{
    lvRect rc = windowRect;
    lvRect border = getBorderWidths();
    rc.left += border.left;
    rc.top += border.top;
    rc.right -= border.right;
    rc.bottom -= border.bottom;
    return rc;
}

lvRect CRWindowSkin::getTitleRect( const lvRect &windowRect )
{
    lvRect rc = CRRectSkin::getClientRect( windowRect );
    lvPoint tsz = getTitleSize();
    rc.bottom = rc.top + tsz.y;
    rc.left = rc.left + tsz.x;
    return rc;
}

lvRect CRWindowSkin::getClientRect( const lvRect &windowRect )
{
	lvRect rc = CRRectSkin::getClientRect( windowRect );
    lvPoint tsz = getTitleSize();
	rc.top += tsz.y;
	rc.left += tsz.x;
	return rc;
}

/// returns necessary window size for specified client size
lvPoint CRWindowSkin::getWindowSize( const lvPoint & clientSize )
{
    lvRect borders = getBorderWidths();
    lvPoint tsz = getTitleSize();
    return lvPoint( clientSize.x + borders.left + borders.right + tsz.x, clientSize.y + borders.top + borders.bottom + tsz.y );
}

CRSkinnedItem::CRSkinnedItem()
:   _textcolor( 0x000000 )
,   _fontFace("Arial")
,   _fontSize( 24 )
,   _fontBold( false )
,   _fontItalic( false )
,   _textAlign( 0 )
{
}

void CRSkinnedItem::setFontFace( lString32 face )
{
    if ( _fontFace != face ) {
        _fontFace = face;
        _font.Clear();
    }
}

void CRSkinnedItem::setFontSize( int size )
{
    if ( _fontSize != size ) {
        _fontSize = size;
        _font.Clear();
    }
}

void CRSkinnedItem::setFontBold( bool bold )
{
    if ( _fontBold != bold ) {
        _fontBold = bold;
        _font.Clear();
    }
}

void CRSkinnedItem::setFontItalic( bool italic )
{
    if ( _fontItalic != italic ) {
        _fontItalic = italic;
        _font.Clear();
    }
}

LVFontRef CRSkinnedItem::getFont()
{
    if ( _font.isNull() ) {
        _font = fontMan->GetFont( _fontSize, _fontBold ? 700 : 400, _fontItalic, css_ff_sans_serif, UnicodeToUtf8(_fontFace) );
    }
    return _font;
}

lvPoint CRSkinnedItem::measureText( lString32 text )
{
    int th = getFont()->getHeight();
    int tw = getFont()->getTextWidth( text.c_str(), text.length() );
    return lvPoint( tw, th );
}

static void wrapLine( lString32Collection & dst, lString32 stringToSplit, int maxWidth, LVFontRef font )
{
    lString32 str = stringToSplit;
    int w = font->getTextWidth( str.c_str(), str.length() );
    if ( w<=maxWidth ) {
        dst.add( str );
        return;
    }
    for ( ;!str.empty(); ) {
        int wpos = 1;
        int wquality = 0;
        for ( int i=str.length(); i>=0; i-- ) {
            lChar32 ch = str[i];
            if ( ch!=' ' && ch!=0 && wpos>1 )
                continue;
            lChar32 prevChar = i>0 ? str[i-1] : 0;
            w = font->getTextWidth( str.c_str(), i );
            int q = 0;
            if ( ch!=' ' && ch!=0 )
                q = 1;
            else
                q = (prevChar=='.' || prevChar==',' || prevChar==':'
                     || prevChar==';' || prevChar=='!'  || prevChar=='?'
                     ) ? (w<maxWidth*2/3 ? 3 : 2) : 2;
            if ( q>wquality && w<maxWidth ) {
                wquality = q;
                wpos = i;
            }
            if ( wquality>1 && w<=maxWidth*2/3 )
                break;
        }
        lString32 s = str.substr(0, wpos);
        s.trim();
        if ( s.length()>0 )
            dst.add( s );
        str = str.substr( wpos );
        str.trim();
    }
}

lvPoint CRRectSkin::measureTextItem( lString32 text )
{
    lvPoint sz = CRSkinnedItem::measureText( text );
    sz.x += _margins.left + _margins.right;
    sz.y += _margins.top + _margins.bottom;
    if ( _minsize.x > 0 && sz.x < _minsize.x )
        sz.x = _minsize.x;
    if ( _minsize.y > 0 && sz.y < _minsize.y )
        sz.y = _minsize.y;
    return sz;
}

void CRSkinnedItem::drawText( LVDrawBuf & buf, const lvRect & rc, lString32 text, LVFontRef font, lUInt32 textColor, lUInt32 bgColor, int flags )
{
    SAVE_DRAW_STATE( buf );
    if ( font.isNull() )
        font = getFont();
    if ( font.isNull() )
        return;
    lString32Collection lines;
    lString32 tabText;
    int tabPos = text.pos("\t");
    if ( tabPos>=0 ) {
        if ( flags & SKIN_EXTEND_TAB ) {
            tabText = text.substr( tabPos+1 );
            text = text.substr( 0, tabPos );
        } else {
            text[tabPos] = U' ';
        }
    }
    lString32 cr("\n");
    if ( flags & SKIN_WORD_WRAP ) {
        lString32Collection crlines;
        lString32 s1, s2;
        while ( text.split2( cr, s1, s2 ) ) {
            crlines.add(s1);
            text = s2;
        }
        crlines.add( text );
        for ( int i=0; i<crlines.length(); i++ ) {
            wrapLine( lines, crlines[i], rc.width(), font );
        }
    } else {
        lString32 s = text;
        while ( s.replace( cr, cs32(" ") ) )
            ;
        lines.add( s );
    }
    buf.SetTextColor( textColor );
    buf.SetBackgroundColor( bgColor );
    lvRect oldRc;
    buf.GetClipRect( &oldRc );
    buf.SetClipRect( &rc );
    int lh = font->getHeight();
    int th = lh * lines.length();
    int ttw = tabText.empty() ? 0 : font->getTextWidth( tabText.c_str(), tabText.length() );

    int halign = tabText.empty() ? (flags & SKIN_HALIGN_MASK) : SKIN_HALIGN_LEFT;
    int valign = flags & SKIN_VALIGN_MASK;

    lvRect txtrc = rc;
    int y = txtrc.top;
    int dy = txtrc.height() - th;
    if ( valign == SKIN_VALIGN_CENTER )
        y += dy / 2;
    else if ( valign == SKIN_VALIGN_BOTTOM )
        y += dy;

    for ( int i=0; i<lines.length(); i++ ) {
        lString32 s = lines[i];
        int tw = font->getTextWidth( s.c_str(), s.length() );
        int x = txtrc.left;
        int dx = txtrc.width() - tw;
        if ( halign == SKIN_HALIGN_CENTER )
            x += dx / 2;
        else if ( halign == SKIN_HALIGN_RIGHT )
            x += dx;


        font->DrawTextString( &buf, x, y, s.c_str(), s.length(), U'?', NULL, false, 0 );
        if ( !tabText.empty() ) {
            font->DrawTextString( &buf, txtrc.right-ttw, y, tabText.c_str(), tabText.length(), U'?', NULL, false, 0 );
            tabText.clear();
        }
        y = y + lh;
    }
    buf.SetClipRect( &oldRc );
}

void CRRectSkin::drawText( LVDrawBuf & buf, const lvRect & rc, lString32 text, LVFontRef font )
{
    lvRect rect = getClientRect( rc );
    CRSkinnedItem::drawText( buf, rect, text, font );
}
void CRRectSkin::drawText( LVDrawBuf & buf, const lvRect & rc, lString32 text )
{
    lvRect rect = CRRectSkin::getClientRect( rc );
    CRSkinnedItem::drawText( buf, rect, text );
}

void CRButtonSkin::drawButton( LVDrawBuf & buf, const lvRect & rect, int flags )
{
    lvRect rc = rect;
    rc.shrinkBy( _margins );
    LVImageSourceRef btnImage = getImage(flags);
    if ( !btnImage.isNull() ) {
        LVImageSourceRef img = LVCreateStretchFilledTransform( btnImage,
            rc.width(), rc.height() );
        buf.Draw( btnImage, rc.left, rc.top, rc.width(), rc.height(), false );
    }
}

LVImageSourceRef CRButtonSkin::getImage(int flags)
{
    LVImageSourceRef btnImage;

    if ( flags & ENABLED ) {
        if ( flags & PRESSED )
            btnImage = _pressedimage;
        else if ( flags & SELECTED )
            btnImage = _selectedimage;
        else
            btnImage = _normalimage;
    } else
        btnImage = _disabledimage;
    if ( btnImage.isNull() )
        btnImage = _normalimage;
	return btnImage;
}

void CRScrollSkin::drawScroll( LVDrawBuf & buf, const lvRect & rect, bool vertical, int pos, int maxpos, int pagesize )
{
    lvRect rc = rect;

    draw( buf, rc );

    int pages = pagesize>0 ? (maxpos+pagesize-1)/pagesize : 0;
    int page = pages>0 ? pos/pagesize+1 : 0;

    if ( !_bottomTabSkin.isNull() && !_bottomPageBoundSkin.isNull() &&
         !_bottomActiveTabSkin.isNull() ) {
        // tabs
        if ( pages<=1 )
            return; // don't draw tabs if no other pages
        int tabwidth = _bottomTabSkin->getMinSize().x;
        if ( tabwidth<40 )
            tabwidth = 40;
        if ( tabwidth>_bottomTabSkin->getMaxSize().x && _bottomTabSkin->getMaxSize().x>0)
            tabwidth = _bottomTabSkin->getMaxSize().x;
        int maxtabs = rc.width()-_margins.left-_margins.right / tabwidth;
        if ( pages <= maxtabs ) {
            // can draw tabs
            lvRect r(rc);
            r.left += _margins.left;
            for ( int i=0; i<pages; i++ ) {
                r.right = r.left + tabwidth;
                if ( i+1!=page ) {
                    _bottomTabSkin->draw(buf, r);
                    lString32 label = lString32::itoa(i+1);
                    _bottomTabSkin->drawText(buf, r, label);
                }
                r.left += tabwidth - r.height()/6;
            }
            _bottomPageBoundSkin->draw(buf, rc);
            r = rc;
            r.left += _margins.left;
            for ( int i=0; i<pages; i++ ) {
                r.right = r.left + tabwidth;
                if ( i+1==page ) {
                    _bottomActiveTabSkin->draw(buf, r);
                    lString32 label = lString32::itoa(i+1);
                    _bottomActiveTabSkin->drawText(buf, r, label);
                }
                r.left += tabwidth - r.height()/6;
            }
            return;
        }
    }


    rc.shrinkBy( _margins );

    int btn1State = CRButtonSkin::ENABLED;
    int btn2State = CRButtonSkin::ENABLED;
    if ( pos <= 0 )
        btn1State = 0;
    if ( pos >= maxpos-pagesize )
        btn2State = 0;
    CRButtonSkinRef btn1Skin;
    CRButtonSkinRef btn2Skin;
    lvRect btn1Rect = rc;
    lvRect btn2Rect = rc;
    lvRect bodyRect = rc;
    lvRect sliderRect = rc;
    LVImageSourceRef bodyImg;
    LVImageSourceRef sliderImg;

    if ( _hBody.isNull() ) {
        // text label with optional arrows
        lString32 label;
        label << fmt::decimal(page) << " / " << fmt::decimal(pages);
        // calc label width
        int w = getFont()->getTextWidth( label.c_str(), label.length() );
        int margin = 4;
        btn1Skin = _leftButton;
        btn2Skin = _rightButton;
        // calc button widths
        int bw1 = btn1Skin.isNull() ? 0 : btn1Skin->getMinSize().x;
        int bw2 = btn1Skin.isNull() ? 0 : btn2Skin->getMinSize().x;
        // total width
        int ww = w + margin + margin + bw1 + bw2;
        int dw = rc.width() - ww;
        rc.left += dw*3/4;
        rc.right = rc.left + ww;
        // adjust rectangle size
        btn1Rect = rc;
        btn2Rect = rc;
        btn1Rect.right = btn1Rect.left + bw1;
        btn2Rect.left = btn2Rect.right - bw2;
        bodyRect.left = btn1Rect.right;
        bodyRect.right = btn2Rect.left;
        int dy = bodyRect.height() - btn1Skin->getMinSize().y;
        btn1Rect.top += dy/2;
        btn1Rect.bottom = btn1Rect.top + btn1Skin->getMinSize().y;
        dy = bodyRect.height() - btn2Skin->getMinSize().y;
        btn2Rect.top += dy/2;
        btn2Rect.bottom = btn2Rect.top + btn2Skin->getMinSize().y;
        btn1Skin->drawButton( buf, btn1Rect, btn1State );
        btn2Skin->drawButton( buf, btn2Rect, btn2State );
        drawText( buf, bodyRect, label );
        return;
    }

    if ( vertical ) {
        // draw vertical
        btn1Skin = _upButton;
        btn2Skin = _downButton;
        btn1Rect.bottom = btn1Rect.top + btn1Skin->getMinSize().y;
        btn2Rect.top = btn2Rect.bottom - btn2Skin->getMinSize().y;
        bodyRect.top = btn1Rect.bottom;
        bodyRect.bottom = btn2Rect.top;
        int sz = bodyRect.height();
        if ( pagesize < maxpos ) {
            sliderRect.top = bodyRect.top + sz * pos / maxpos;
            sliderRect.bottom = bodyRect.top + sz * (pos + pagesize) / maxpos;
        } else
            sliderRect = bodyRect;
        bodyImg = _vBody;
        sliderImg = _vSlider;
    } else {
        // draw horz
        btn1Skin = _leftButton;
        btn2Skin = _rightButton;
        btn1Rect.right = btn1Rect.left + btn1Skin->getMinSize().x;
        btn2Rect.left = btn2Rect.right - btn2Skin->getMinSize().x;
        bodyRect.left = btn1Rect.right;
        bodyRect.right = btn2Rect.left;
        int sz = bodyRect.width();
        if ( pagesize < maxpos ) {
            sliderRect.left = bodyRect.left + sz * pos / maxpos;
            sliderRect.right = bodyRect.left + sz * (pos + pagesize) / maxpos;
        } else
            sliderRect = bodyRect;
        bodyImg = _hBody;
        sliderImg = _hSlider;
    }
    btn1Skin->drawButton( buf, btn1Rect, btn1State );
    btn2Skin->drawButton( buf, btn2Rect, btn2State );
    if ( !bodyImg.isNull() ) {
        LVImageSourceRef img = LVCreateStretchFilledTransform( bodyImg,
            bodyRect.width(), bodyRect.height() );
        buf.Draw( img, bodyRect.left, bodyRect.top, bodyRect.width(), bodyRect.height(), false );
    }
    if ( !sliderImg.isNull() ) {
        LVImageSourceRef img = LVCreateStretchFilledTransform( sliderImg,
            sliderRect.width(), sliderRect.height() );
        buf.Draw( img, sliderRect.left, sliderRect.top, sliderRect.width(), sliderRect.height(), false );
        if ( this->getShowPageNumbers() ) {
            lString32 label;
            label << fmt::decimal(page) << " / " << fmt::decimal(pages);
            drawText( buf, sliderRect, label );
        }
    }
}

void CRScrollSkin::drawGauge( LVDrawBuf & buf, const lvRect & rect, int percent )
{
    lvRect rc = rect;
    rc.shrinkBy( _margins );
    bool vertical = rect.width()<rect.height();
    lvRect bodyRect = rc;
    lvRect sliderRect = rc;
    LVImageSourceRef bodyImg;
    LVImageSourceRef sliderImg;
    if ( vertical ) {
        // draw vertical
        int sz = bodyRect.height();
        sliderRect.bottom = bodyRect.top + sz * percent / 100;
        bodyImg = _vBody;
        sliderImg = _vSlider;
    } else {
        // draw horz
        int sz = bodyRect.width();
        sliderRect.right = bodyRect.left + sz * percent / 100;
        bodyImg = _hBody;
        sliderImg = _hSlider;
    }
    if ( !bodyImg.isNull() ) {
        LVImageSourceRef img = LVCreateStretchFilledTransform( bodyImg,
            bodyRect.width(), bodyRect.height() );
        buf.Draw( img, bodyRect.left, bodyRect.top, bodyRect.width(), bodyRect.height(), false );
    }
    if ( !sliderImg.isNull() ) {
        LVImageSourceRef img = LVCreateStretchFilledTransform( sliderImg,
            sliderRect.width(), sliderRect.height() );
        buf.Draw( img, sliderRect.left, sliderRect.top, sliderRect.width(), sliderRect.height(), false );
    }
}

void CRToolBarSkin::drawToolBar( LVDrawBuf & buf, const lvRect & rect, bool enabled, int selectedButton )
{
	draw(buf, rect);
    lvRect rc = rect;
    rc.shrinkBy( _margins );
    int width = 0;
	for ( int i=0; i<_buttons->length(); i++ ) {
		// int flags = enabled ? CRButtonSkin::ENABLED : 0;
		// if (i == selectedButton && enabled)
		// 	flags |= CRButtonSkin::SELECTED;
		LVRef<CRButtonSkin> button = _buttons->get(i);
		if (!button.isNull()) {
			width += button->getMinSize().x;
			int h = button->getMinSize().y;
			if (h > rc.height())
				return;
		}
	}
	if (width > rc.width())
		return; // That's all for now
	int offsetX = 0;
	if (getHAlign() == SKIN_HALIGN_RIGHT)
		offsetX = rc.width() - width;
	else if (getHAlign() == SKIN_HALIGN_CENTER ) 
		offsetX = rc.width() - width/2;
	int h = rc.height();
    for ( int i=0; i<_buttons->length(); i++ ) {
		lvRect rc2 = rc;
		int flags = enabled ? CRButtonSkin::ENABLED : 0;
		if (i == selectedButton && enabled)
			flags |= CRButtonSkin::SELECTED;
		LVRef<CRButtonSkin> button = _buttons->get(i);
		if (!button.isNull()) {
			LVImageSourceRef img = button->getImage(flags);
			rc2.left += offsetX;
			rc2.right = rc2.left + button->getMinSize().x;
			if ( getVAlign()==SKIN_VALIGN_BOTTOM )
				rc2.top = rc2.bottom - button->getMinSize().y;
			else if ( getVAlign()==SKIN_VALIGN_CENTER ) {
				int imgh = button->getMinSize().y;
				rc2.top += (h - imgh/2);
				rc2.bottom = rc2.top + imgh;
			} else
				rc2.bottom = rc2.top + button->getMinSize().y;
			button->drawButton( buf, rc2, flags );
			offsetX = rc2.right - rc.left;
		}
	}
}

void CRToolBarSkin::drawButton(LVDrawBuf & buf, const lvRect & rc, int index, int flags)
{
	
}

CRRectSkin::CRRectSkin()
: _margins( 0, 0, 0, 0 )
, _size(toSkinPercent( 10000 ), toSkinPercent( 10000 )) // 100% x 100%
, _pos(0, 0)
, _align(SKIN_VALIGN_TOP|SKIN_HALIGN_LEFT) // relative to top left
{
}

CRWindowSkin::CRWindowSkin()
{
    _fullscreen = false;
}

CRMenuSkin::CRMenuSkin()
: _minItemCount(-1)
, _maxItemCount(-1)
, _showShortcuts(true)
{
}


// WINDOW skin stub
class CRSimpleWindowSkin : public CRWindowSkin
{
public:
        CRSimpleWindowSkin( CRSkinImpl * )
	{
        //setBackgroundColor( 0xAAAAAA );
	}
};

class CRSimpleFrameSkin : public CRRectSkin
{
public:
        CRSimpleFrameSkin( CRSkinImpl * )
	{
        //setBackgroundColor( 0xAAAAAA );
	}
};

/*
    <item>
        <text color="" face="" size="" bold="" italic="" valign="" halign=""/>
        <background image="filename" color=""/>
        <border widths="left,top,right,bottom"/>
        <icon image="filename" valign="" halign=""/>
        <title>
            <size minvalue="x,y" maxvalue=""/>
            <text color="" face="" size="" bold="" italic="" valign="" halign=""/>
            <background image="filename" color=""/>
            <border widths="left,top,right,bottom"/>
            <icon image="filename" valign="" halign="">
        </title>
        <item>
            <size minvalue="x,y" maxvalue=""/>
            <text color="" face="" size="" bold="" italic="" valign="" halign=""/>
            <background image="filename" color=""/>
            <border widths="left,top,right,bottom"/>
            <icon image="filename" valign="" halign="">
        </item>
        <shortcut>
            <size minvalue="x,y" maxvalue=""/>
            <text color="" face="" size="" bold="" italic="" valign="" halign=""/>
            <background image="filename" color=""/>
            <border widths="left,top,right,bottom"/>
            <icon image="filename" valign="" halign="">
        </shortcut>
    </item>
*/
class CRSimpleMenuSkin : public CRMenuSkin
{
public:
    CRSimpleMenuSkin( CRSkinImpl * skin )
    {
        //setBackgroundColor( 0xAAAAAA );
        //setTitleSize( lvPoint( 0, 48 ) );
        setBorderWidths( lvRect( 8, 8, 8, 8 ) );
        _titleSkin = CRRectSkinRef( new CRRectSkin() );
        //_titleSkin->setBackgroundColor(0xAAAAAA);
        _titleSkin->setTextColor(0x000000);
        _titleSkin->setFontBold( true );
        _titleSkin->setFontSize( 28 );
        _itemSkin = CRRectSkinRef( new CRRectSkin() );
        _itemSkin->setBackgroundImage( skin->getImage( U"std_menu_item_background.xpm" ) );
        _itemSkin->setBorderWidths( lvRect( 8, 8, 8, 8 ) );
        _itemShortcutSkin = CRRectSkinRef( new CRRectSkin() );
        _itemShortcutSkin->setBackgroundImage( skin->getImage( U"std_menu_shortcut_background.xpm" ) );
        _itemShortcutSkin->setBorderWidths( lvRect( 12, 8, 8, 8 ) );
        _itemShortcutSkin->setTextColor( 0x555555 );
        _itemShortcutSkin->setTextHAlign( SKIN_HALIGN_CENTER );
        _itemShortcutSkin->setTextVAlign( SKIN_VALIGN_CENTER );
	}
};

CRButtonSkin::CRButtonSkin() { }

bool CRSkinContainer::readButtonSkin(  const lChar32 * path, CRButtonSkin * res )
{
    bool flg = false;
    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readButtonSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Button skin by path " << p << " was not found";
#endif
        return false;
    }

    flg = readRectSkin( path, res ) || flg;
    res->setNormalImage( readImage( path, U"normal", &flg ) );
    res->setDisabledImage( readImage( path, U"disabled", &flg ) );
    res->setPressedImage( readImage( path, U"pressed", &flg ) );
    res->setSelectedImage( readImage( path, U"selected", &flg ) );

    LVImageSourceRef img = res->getNormalImage();
    lvRect margins = res->getBorderWidths();
    if ( !img.isNull() ) {
        flg = true;
        res->setMinSize( lvPoint( margins.left + margins.right + img->GetWidth(), margins.top + margins.bottom + img->GetHeight() ) );
    }

    if ( !flg ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Button skin reading failed: " << path;
#endif
    }

    return flg;
}

CRScrollSkin::CRScrollSkin() : _autohide(false), _showPageNumbers(true), _location(CRScrollSkin::Status) { }

bool CRSkinContainer::readScrollSkin(  const lChar32 * path, CRScrollSkin * res )
{
    bool flg = false;
    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readScrollSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "ScrollBar skin by path " << p << " was not found";
#endif
        return false;
    }



    flg = readRectSkin( path, res ) || flg;

    res->setAutohide( readBool( (p).c_str(), U"autohide", res->getAutohide()) );
    res->setShowPageNumbers( readBool( (p).c_str(), U"show-page-numbers", res->getShowPageNumbers()) );
    lString32 l = readString( (p).c_str(), U"location", lString32::empty_str );
    if ( !l.empty() ) {
        l.lowercase();
        if (l == "title")
            res->setLocation( CRScrollSkin::Title );
    }
    CRButtonSkinRef upButton( new CRButtonSkin() );
    if ( readButtonSkin(  (p + "/upbutton").c_str(), upButton.get() ) ) {
        res->setUpButton( upButton );
        flg = true;
    }

    CRButtonSkinRef downButton( new CRButtonSkin() );
    if ( readButtonSkin(  (p + "/downbutton").c_str(), downButton.get() ) ) {
        res->setDownButton( downButton );
        flg = true;
    }

    CRButtonSkinRef leftButton( new CRButtonSkin() );
    if ( readButtonSkin(  (p + "/leftbutton").c_str(), leftButton.get() ) ) {
        res->setLeftButton( leftButton );
        flg = true;
    }

    CRButtonSkinRef rightButton( new CRButtonSkin() );
    if ( readButtonSkin(  (p + "/rightbutton").c_str(), rightButton.get() ) ) {
        res->setRightButton( rightButton );
        flg = true;
    }

    CRRectSkinRef tabSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/tab-bottom").c_str(), tabSkin.get() ) ) {
        res->setBottomTabSkin( tabSkin );
        flg = true;
    }

    CRRectSkinRef tabActiveSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/tab-bottom-active").c_str(), tabActiveSkin.get() ) ) {
        res->setBottomActiveTabSkin( tabActiveSkin );
        flg = true;
    }

    CRRectSkinRef pageBoundSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/page-bound-bottom").c_str(), pageBoundSkin.get() ) ) {
        res->setBottomPageBoundSkin( pageBoundSkin );
        flg = true;
    }

    LVImageSourceRef hf = readImage( (p + "/hbody").c_str(), U"frame", &flg );
    if ( !hf.isNull() )
        res->setHBody( hf );
    LVImageSourceRef hs = readImage( (p + "/hbody").c_str(), U"slider", &flg );
    if ( !hs.isNull() )
        res->setHSlider( hs );
    LVImageSourceRef vf = readImage( (p + "/vbody").c_str(), U"frame", &flg );
    if ( !vf.isNull() )
        res->setVBody( vf );
    LVImageSourceRef vs = readImage( (p + "/vbody").c_str(), U"slider", &flg );
    if ( !vs.isNull() )
        res->setVSlider(vs );

    if ( !flg ) {
        crtrace log;
        log << "Scroll skin reading failed: " << path;
    }

    return flg;
}

bool CRSkinContainer::readIconSkin(  const lChar32 * path, CRIconSkin * res )
{
    bool flg = false;
    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readIconSkin( base.c_str(), res ) || flg;
    }
    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Image skin by path " << p << " was not found";
#endif
        return false;
    }
    LVImageSourceRef image = readImage( path, U"image", &flg );
    if ( !image.isNull() )
        res->setImage( image );
    res->setHAlign( readHAlign( path, U"halign", res->getHAlign(), &flg) );
    res->setVAlign( readVAlign( path, U"valign", res->getVAlign(), &flg) );
    res->setBgColor( readColor( path, U"color", res->getBgColor(), &flg) );
    res->setHTransform( readTransform( path, U"htransform", res->getHTransform(), &flg) );
    res->setVTransform( readTransform( path, U"vtransform", res->getVTransform(), &flg) );
    res->setSplitPoint( readSize( path, U"split", res->getSplitPoint(), &flg) );
    res->setPos( readSize( path, U"pos", res->getPos(), &flg) );
    res->setSize( readSize( path, U"size", res->getSize(), &flg) );
    return flg;
}

bool CRSkinContainer::readRectSkin(  const lChar32 * path, CRRectSkin * res )
{
    bool flg = false;

    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readRectSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Rect skin by path " << p << " was not found";
#endif
        return false;
    }

    lString32 bgpath = p + "/background";
    lString32 borderpath = p + "/border";
    lString32 textpath = p + "/text";
    lString32 sizepath = p + "/size";

    CRIconListRef icons;
    bool bgIconsFlag = false;
    icons = readIcons( bgpath.c_str(), &bgIconsFlag);
    if ( bgIconsFlag ) {
        res->setBgIcons( icons );
        flg = true;
    }
    //res->setBackgroundColor( readColor( bgpath.c_str(), U"color", res->getBackgroundColor(), &flg ) );
    res->setBorderWidths( readRect( borderpath.c_str(), U"widths", res->getBorderWidths(), &flg ) );
    res->setMinSize( readSize( sizepath.c_str(), U"minvalue", res->getMinSize(), &flg ) );
    res->setMaxSize( readSize( sizepath.c_str(), U"maxvalue", res->getMaxSize(), &flg ) );
    res->setFontFace( readString( textpath.c_str(), U"face", res->getFontFace(), &flg ) );
    res->setTextColor( readColor( textpath.c_str(), U"color", res->getTextColor(), &flg ) );
    res->setFontBold( readBool( textpath.c_str(), U"bold", res->getFontBold(), &flg ) );
    res->setWordWrap( readBool( textpath.c_str(), U"wordwrap", res->getWordWrap(), &flg ) );
    res->setFontItalic( readBool( textpath.c_str(), U"italic", res->getFontItalic(), &flg ) );
    res->setFontSize( readInt( textpath.c_str(), U"size", res->getFontSize(), &flg ) );
    res->setTextHAlign( readHAlign( textpath.c_str(), U"halign", res->getTextHAlign(), &flg) );
    res->setTextVAlign( readVAlign( textpath.c_str(), U"valign", res->getTextVAlign(), &flg) );

    res->setHAlign( readHAlign( path, U"halign", res->getHAlign(), &flg) );
    res->setVAlign( readVAlign( path, U"valign", res->getVAlign(), &flg) );
    res->setPos( readSize( path, U"pos", res->getPos(), &flg) );
    res->setSize( readSize( path, U"size", res->getSize(), &flg) );

    if ( !flg ) {
        crtrace log;
        log << "Rect skin reading failed: " << path;
    }

    return flg;
}

lvPoint CRWindowSkin::getTitleSize()
{
    lvPoint minsize = _titleSkin.isNull() ? lvPoint(0, 0) : _titleSkin->getMinSize();
    return minsize;
}

bool CRSkinContainer::readPageSkin(  const lChar32 * path, CRPageSkin * res )
{
    bool flg = false;

    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readPageSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Book page skin by path " << p << " was not found";
#endif
        return false;
    }

    lString32 name = ptr.getNode()->getAttributeValue(ptr.getNode()->getDocument()->getAttrNameIndex("name"));
    if ( !name.empty() )
        res->setName(name);

    flg = readRectSkin( (p + "scroll-skin").c_str(),  res->getSkin( PAGE_SKIN_SCROLL ).get() ) || flg;
    flg = readRectSkin( (p + "left-page-skin").c_str(),  res->getSkin( PAGE_SKIN_LEFT_PAGE ).get() ) || flg;
    flg = readRectSkin( (p + "right-page-skin").c_str(),  res->getSkin( PAGE_SKIN_RIGHT_PAGE ).get() ) || flg;
    flg = readRectSkin( (p + "single-page-skin").c_str(),  res->getSkin( PAGE_SKIN_SINGLE_PAGE ).get() ) || flg;

    if ( !flg ) {
        crtrace log;
        log << "Book page skin reading failed: " << path;
    }

    return flg;
}

bool CRSkinContainer::readWindowSkin(  const lChar32 * path, CRWindowSkin * res )
{
    bool flg = false;

    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readWindowSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Window skin by path " << p << " was not found";
#endif
        return false;
    }

    res->setFullScreen(readBool(path, U"fullscreen", res->getFullScreen(), &flg));

    flg = readRectSkin(  path, res ) || flg;
    CRRectSkinRef titleSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/title").c_str(), titleSkin.get() ) ) {
        res->setTitleSkin( titleSkin );
        flg = true;
    }

    CRRectSkinRef clientSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/client").c_str(), clientSkin.get() ) ) {
        res->setClientSkin( clientSkin );
        flg = true;
    }

    CRRectSkinRef inputSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/input").c_str(), inputSkin.get() ) ) {
        res->setInputSkin( inputSkin );
        flg = true;
    }

    CRRectSkinRef statusSkin( new CRRectSkin() );
    if ( readRectSkin(  (p + "/status").c_str(), statusSkin.get() ) ) {
        res->setStatusSkin( statusSkin );
        flg = true;
    }

    CRScrollSkinRef scrollSkin( new CRScrollSkin() );
    if ( readScrollSkin(  (p + "/scroll").c_str(), scrollSkin.get() ) ) {
        res->setScrollSkin( scrollSkin );
        flg = true;
    }

    if ( !flg ) {
        crtrace log;
        log << "Window skin reading failed: " << path;
    }

    return flg;
}

bool CRSkinContainer::readMenuSkin(  const lChar32 * path, CRMenuSkin * res )
{
    bool flg = false;

    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readMenuSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "Menu skin by path " << p << " was not found";
#endif
        return false;
    }

    flg = readWindowSkin( path, res ) || flg;

    bool b;
    CRRectSkinRef separatorSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/separator").c_str(), separatorSkin.get() );
    flg = flg || b;
    if ( b || res->getSeparatorSkin().isNull() )
        res->setSeparatorSkin( separatorSkin );

    CRRectSkinRef valueSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/value").c_str(), valueSkin.get() );
    flg = flg || b;
    if ( b || res->getValueSkin().isNull() )
        res->setValueSkin( valueSkin );

    CRRectSkinRef itemSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/item").c_str(), itemSkin.get() );
    flg = flg || b;
    if ( b || res->getItemSkin().isNull() )
        res->setItemSkin( itemSkin );
    CRRectSkinRef shortcutSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/shortcut").c_str(), shortcutSkin.get() );
    flg = flg || b;
    if ( b || res->getItemShortcutSkin().isNull() )
        res->setItemShortcutSkin( shortcutSkin );

    CRRectSkinRef itemSelSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/selitem").c_str(), itemSelSkin.get() );
    flg = flg || b;
    if ( b || res->getSelItemSkin().isNull() )
        res->setSelItemSkin( itemSelSkin );
    CRRectSkinRef shortcutSelSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/selshortcut").c_str(), shortcutSelSkin.get() );
    flg = flg || b;
    if ( b || res->getSelItemShortcutSkin().isNull() )
        res->setSelItemShortcutSkin( shortcutSelSkin );

    CRRectSkinRef evenitemSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/item-even").c_str(), evenitemSkin.get() );
    flg = flg || b;
    if ( b )
        res->setEvenItemSkin( evenitemSkin );
    CRRectSkinRef evenshortcutSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/shortcut-even").c_str(), evenshortcutSkin.get() );
    flg = flg || b;
    if ( b )
        res->setEvenItemShortcutSkin( evenshortcutSkin );

    CRRectSkinRef evenitemSelSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/selitem-even").c_str(), evenitemSelSkin.get() );
    flg = flg || b;
    if ( b )
        res->setEvenSelItemSkin( evenitemSelSkin );
    CRRectSkinRef evenshortcutSelSkin( new CRRectSkin() );
    b = readRectSkin(  (p + "/selshortcut-even").c_str(), evenshortcutSelSkin.get() );
    flg = flg || b;
    if ( b )
        res->setEvenSelItemShortcutSkin( evenshortcutSelSkin );

    res->setMinItemCount( readInt( path, U"min-item-count", res->getMinItemCount()) );
    res->setMaxItemCount( readInt( path, U"max-item-count", res->getMaxItemCount()) );
    res->setShowShortcuts( readBool( path, U"show-shortcuts", res->getShowShortcuts() ) );

    return flg;
}

CRButtonListRef CRSkinContainer::readButtons( const lChar32 * path, bool * res )
{
    CRButtonListRef list = CRButtonListRef(new CRButtonList() );
    for ( int i=1; i<64; i++ ) {
        lString32 p = lString32(path) << "[" << fmt::decimal(i) << "]";
        CRButtonSkin * button = new CRButtonSkin();
        if ( readButtonSkin(p.c_str(), button ) )
            list->add( LVRef<CRButtonSkin>(button) );
        else {
            delete button;
            break;
        }
    }
    if ( list->length()==0 ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "CRSkinContainer::readButtons( " << path << ") - cannot read button from specified path";
#endif
		if ( res )
			*res = false;
        return CRButtonListRef();
    }
    if ( res )
        *res = true;
    return list;	
}

bool CRSkinContainer::readToolBarSkin(  const lChar32 * path, CRToolBarSkin * res )
{
    bool flg = false;
    lString32 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        flg = readToolBarSkin( base.c_str(), res ) || flg;
    }

    lString32 p( path );
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr ) {
#ifdef TRACE_SKIN_ERRORS
        crtrace log;
        log << "ToolBar skin by path " << p << " was not found";
#endif
        return false;
    }
    flg = readRectSkin( path, res ) || flg;

    lString32 buttonspath = p + "/button";
    bool buttonsFlag = false;
    CRButtonListRef buttons = readButtons( buttonspath.c_str(), &buttonsFlag);
    if ( buttonsFlag ) {
        res->setButtons( buttons );
        flg = true;
    }
	return flg;
}

lString32 CRSkinImpl::pathById( const lChar32 * id )
{
    ldomNode * elem = _doc->getElementById( id );
    if ( !elem )
        return lString32::empty_str;
    return ldomXPointer(elem, -1).toString();
}

/// returns rect skin
CRRectSkinRef CRSkinImpl::getRectSkin( const lChar32 * path )
{
    lString32 p(path);
    CRRectSkinRef res;
    if ( _rectCache.get( p, res ) )
        return res; // found in cache
    if ( *path == '#' ) {
        // find by id
        p = pathById( path+1 );
    }
    // create new one
    res = CRRectSkinRef( new CRRectSkin() );
    readRectSkin( p.c_str(), res.get() );
    _rectCache.set( lString32(path), res );
    return res;
}

/// returns scroll skin by path or #id
CRScrollSkinRef CRSkinImpl::getScrollSkin( const lChar32 * path )
{
    lString32 p(path);
    CRScrollSkinRef res;
    if ( _scrollCache.get( p, res ) )
        return res; // found in cache
    if ( *path == '#' ) {
        // find by id
        p = pathById( path+1 );
    }
    // create new one
    res = CRScrollSkinRef( new CRScrollSkin() );
    readScrollSkin( p.c_str(), res.get() );
    _scrollCache.set( lString32(path), res );
    return res;
}

/// returns book page skin list
CRPageSkinListRef CRSkinImpl::getPageSkinList()
{
    if ( _pageSkinList.isNull() ) {
        _pageSkinList = CRPageSkinListRef( new CRPageSkinList() );
        for ( int i=0; i<32; i++ ) {
            lString32 path("/CR3Skin/page-skins/page-skin[");
            path << (i+1) << U"]";
            CRPageSkinRef skin = CRPageSkinRef( new CRPageSkin() );
            if ( readPageSkin(path.c_str(), skin.get() ) ) {
                _pageSkinList->add( skin );
            } else {
                break;
            }
        }
    }
    return _pageSkinList;
}

/// returns book page skin by path or #id
CRPageSkinRef CRSkinImpl::getPageSkin( const lChar32 * path )
{
    lString32 p(path);
    CRPageSkinRef res;
    if ( _pageCache.get( p, res ) )
        return res; // found in cache
    if ( *path == '#' ) {
        // find by id
        p = pathById( path+1 );
    }
    // create new one
    res = CRPageSkinRef( new CRPageSkin() );
    readPageSkin( p.c_str(), res.get() );
    _pageCache.set( lString32(path), res );
    return res;
}

/// returns window skin
CRWindowSkinRef CRSkinImpl::getWindowSkin( const lChar32 * path )
{
    lString32 p(path);
    CRWindowSkinRef res;
    if ( _windowCache.get( p, res ) )
        return res; // found in cache
    if ( *path == '#' ) {
        // find by id
        p = pathById( path+1 );
    }
    // create new one
    res = CRWindowSkinRef( new CRWindowSkin() );
    readWindowSkin( p.c_str(), res.get() );
    _windowCache.set( lString32(path), res );
    return res;
}

/// returns menu skin
CRMenuSkinRef CRSkinImpl::getMenuSkin( const lChar32 * path )
{
    lString32 p(path);
    CRMenuSkinRef res;
    if ( _menuCache.get( p, res ) )
        return res; // found in cache
    if ( *path == '#' ) {
        // find by id
        p = pathById( path+1 );
    }
    // create new one
    res = CRMenuSkinRef( new CRMenuSkin() );
    readMenuSkin( p.c_str(), res.get() );
    _menuCache.set( lString32(path), res );
    return res;
}

CRToolBarSkinRef CRSkinImpl::getToolBarSkin( const lChar32 * path )
{
    lString32 p(path);
    CRToolBarSkinRef res;
    if ( _toolbarCache.get( p, res ) )
        return res; // found in cache
    if ( *path == '#' ) {
        // find by id
        p = pathById( path+1 );
    }
    res = CRToolBarSkinRef( new CRToolBarSkin() );
    readToolBarSkin( p.c_str(), res.get() );
    _toolbarCache.set( lString32(path), res );
    return res;
}

CRSkinListItem * CRSkinList::findByName( const lString32 & name )
{
    for ( int i=0; i<length(); i++ )
        if ( get(i)->getName()==name )
            return get(i);
    return NULL;
}

CRSkinListItem * CRSkinListItem::init( lString32 baseDir, lString32 fileName )
{
    CRSkinRef skin = LVOpenSkin( baseDir + fileName );
    if ( skin.isNull() )
        return NULL;
    CRSkinListItem * item = new CRSkinListItem();
    item->_baseDir = baseDir;
    item->_fileName = fileName;
    //item->_name = skin->get
    return item;
}

CRSkinRef CRSkinListItem::getSkin()
{
    return LVOpenSkin( getDirName() + getFileName() );
}

bool CRLoadSkinList( lString32 baseDir, CRSkinList & list )
{
    return false;
}
