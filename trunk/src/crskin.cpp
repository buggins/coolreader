/*******************************************************

   CoolReader Engine

   crskin.cpp: skinning file support

   (c) Vadim Lopatin, 2000-2008
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include "../include/crskin.h"
#include "../include/lvstsheet.h"

class RecursionLimit
{
static int counter;
public:
    bool test( int limit = 10 ) { return counter < limit; }
    RecursionLimit() { counter++; }
    ~RecursionLimit() { counter--; }
};
int RecursionLimit::counter = 0;


/// retuns path to base definition, if attribute base="#nodeid" is specified for element of path
lString16 CRSkinContainer::getBasePath( const lChar16 * path )
{
    lString16 res;
    ldomXPointer p = getXPointer( lString16( path ) );
    if ( !p )
        return res;
    if ( p.getNode()->getNodeType() != LXML_ELEMENT_NODE )
        return res;
    lString16 value = p.getNode()->getAttributeValue( L"base" );
    if ( value.empty() || value[0]!=L'#' )
        return res;
    return pathById( value.c_str() + 1 );
}

/// skin file support
class CRSkinImpl : public CRSkinContainer
{
protected:
    LVContainerRef _container;
    LVAutoPtr<ldomDocument> _doc;
    LVCacheMap<lString16,LVImageSourceRef> _imageCache;
    LVCacheMap<lString16,CRRectSkinRef> _rectCache;
    LVCacheMap<lString16,CRWindowSkinRef> _windowCache;
    LVCacheMap<lString16,CRMenuSkinRef> _menuCache;
public:
    /// returns rect skin by path or #id
    virtual CRRectSkinRef getRectSkin( const lChar16 * path );
    /// returns window skin by path or #id
    virtual CRWindowSkinRef getWindowSkin( const lChar16 * path );
    /// returns menu skin by path or #id
    virtual CRMenuSkinRef getMenuSkin( const lChar16 * path );
    /// get DOM path by id
    virtual lString16 pathById( const lChar16 * id );
    /// gets image from container
    virtual LVImageSourceRef getImage( const lChar16 * filename );
    /// gets doc pointer by asolute path
    virtual ldomXPointer getXPointer( const lString16 & xPointerStr ) { return _doc->createXPointer( xPointerStr ); }
    /// constructor does nothing
    CRSkinImpl()  : _imageCache(8), _rectCache(8), _windowCache(8), _menuCache(8) { }
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
    const lChar16 * filename;
    const char * * xpm;
} standard_image_item_t;

static standard_image_item_t standard_images [] = {
    { L"std_menu_shortcut_background.xpm", menu_shortcut_background },
    { L"std_menu_item_background.xpm", menu_item_background },
    { NULL, NULL }
};

/// gets image from container
LVImageSourceRef CRSkinImpl::getImage(  const lChar16 * filename  )
{
    LVImageSourceRef res;
    lString16 fn( filename );
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
        if ( !!stream )
            res = LVCreateStreamImageSource( stream );
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
    LVStreamRef stream = container->OpenStream( L"cr3skin.xml", LVOM_READ );
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
lString16 CRSkinContainer::readString( const lChar16 * path, const lChar16 * attrname )
{
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr )
        return lString16();
    if ( ptr.getNode()->getNodeType() != LXML_ELEMENT_NODE )
        return lString16();
    lString16 value = ptr.getNode()->getAttributeValue( attrname );
    return value;
}

/// reads string value from attrname attribute of element specified by path, returns defValue if not found
lString16 CRSkinContainer::readString( const lChar16 * path, const lChar16 * attrname, const lString16 & defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    return value;
}

/// reads color value from attrname attribute of element specified by path, returns defValue if not found
lUInt32 CRSkinContainer::readColor( const lChar16 * path, const lChar16 * attrname, lUInt32 defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    css_length_t cv;
    lString8 buf = UnicodeToUtf8(value);
    const char * bufptr = buf.modify();
    if ( !parse_color_value( bufptr, cv ) )
        return defValue;
    return cv.value;
}

/// reads rect value from attrname attribute of element specified by path, returns defValue if not found
lvRect CRSkinContainer::readRect( const lChar16 * path, const lChar16 * attrname, lvRect defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    lString8 s8 = UnicodeToUtf8( value );
    int n1, n2, n3, n4;
    if ( sscanf( s8.c_str(), "%d,%d,%d,%d", &n1, &n2, &n3, &n4 )!=4 )
        return defValue;
    return lvRect( n1, n2, n3, n4 );
}

/// reads boolean value from attrname attribute of element specified by path, returns defValue if not found
bool CRSkinContainer::readBool( const lChar16 * path, const lChar16 * attrname, bool defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    if ( value == L"true" || value == L"yes" )
        return true;
    if ( value == L"false" || value == L"no" )
        return false;
    return defValue;
}

/// reads h align value from attrname attribute of element specified by path, returns defValue if not found
int CRSkinContainer::readHAlign( const lChar16 * path, const lChar16 * attrname, int defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    if ( value == L"left" )
        return SKIN_HALIGN_LEFT;
    if ( value == L"center" )
        return SKIN_HALIGN_CENTER;
    if ( value == L"right" )
        return SKIN_HALIGN_RIGHT;
    // invalid value
    return defValue;
}

/// reads h align value from attrname attribute of element specified by path, returns defValue if not found
int CRSkinContainer::readVAlign( const lChar16 * path, const lChar16 * attrname, int defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    if ( value == L"left" )
        return SKIN_VALIGN_TOP;
    if ( value == L"center" )
        return SKIN_VALIGN_CENTER;
    if ( value == L"bottom" )
        return SKIN_VALIGN_BOTTOM;
    // invalid value
    return defValue;
}

/// reads int value from attrname attribute of element specified by path, returns defValue if not found
int CRSkinContainer::readInt( const lChar16 * path, const lChar16 * attrname, int defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    lString8 s8 = UnicodeToUtf8( value );
    int n1;
    if ( sscanf( s8.c_str(), "%d", &n1 )!=1 )
        return defValue;
    return n1;
}

/// reads point(size) value from attrname attribute of element specified by path, returns defValue if not found
lvPoint CRSkinContainer::readSize( const lChar16 * path, const lChar16 * attrname, lvPoint defValue )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return defValue;
    lString8 s8 = UnicodeToUtf8( value );
    int n1, n2;
    if ( sscanf( s8.c_str(), "%d,%d", &n1, &n2 )!=2 )
        return defValue;
    return lvPoint( n1, n2 );
}

/// reads rect value from attrname attribute of element specified by path, returns null ref if not found
LVImageSourceRef CRSkinContainer::readImage( const lChar16 * path, const lChar16 * attrname )
{
    lString16 value = readString( path, attrname );
    if ( value.empty() )
        return LVImageSourceRef();
    return getImage( value );
}

/// open simple skin, without image files, from string
CRSkinRef LVOpenSimpleSkin( const lString8 & xml )
{
    CRSkinImpl * skin = new CRSkinImpl();
    CRSkinRef res( skin );
    if ( !skin->open( xml ) )
        return CRSkinRef();
    CRLog::trace("skin xml opened ok");
    return res;
}

/// opens skin from directory or .zip file
CRSkinRef LVOpenSkin( const lString16 & pathname )
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
    CRLog::trace("skin container opened ok");
    return res;
}

// default parameters
//LVFontRef CRSkinnedItem::getFont() { return fontMan->GetFont( 24, 300, false, css_ff_sans_serif, lString8("Arial")) }

void CRSkinnedItem::draw( LVDrawBuf & buf, const lvRect & rc )
{
    SAVE_DRAW_STATE( buf );
	buf.SetBackgroundColor( getBackgroundColor() );
	buf.SetTextColor( getTextColor() );
	LVImageSourceRef bgimg = getBackgroundImage();
	if ( bgimg.isNull() ) {
		buf.FillRect( rc, getBackgroundColor() );
	} else {
		lvPoint split = getBackgroundImageSplit();
		LVImageSourceRef img = LVCreateStretchFilledTransform( bgimg,
			rc.width(), rc.height() );
		buf.Draw( img, rc.left, rc.top, rc.width(), rc.height(), false );
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
,   _bgcolor( 0xFFFFFF )
,   _bgimagesplit(-1,-1)
,   _fontFace(L"Arial")
,   _fontSize( 24 )
,   _fontBold( false )
,   _fontItalic( false )
,   _textAlign( 0 )
{
}

void CRSkinnedItem::setFontFace( lString16 face )
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
        _font = fontMan->GetFont( _fontSize, _fontBold ? 600 : 300, _fontItalic, css_ff_sans_serif, UnicodeToUtf8(_fontFace) );
    }
    return _font;
}

void CRSkinnedItem::drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text, LVFontRef font, lUInt32 textColor, lUInt32 bgColor, int flags )
{
    SAVE_DRAW_STATE( buf );
    if ( font.isNull() )
        font = getFont();
    if ( font.isNull() )
        return;
    buf.SetTextColor( textColor );
    buf.SetBackgroundColor( bgColor );
    lvRect oldRc;
    buf.GetClipRect( &oldRc );
    buf.SetClipRect( &rc );
    int th = font->getHeight();
    int tw = font->getTextWidth( text.c_str(), text.length() );
    lvRect txtrc = rc;
    int x = txtrc.left;
    int dx = txtrc.width() - tw;
    int y = txtrc.top;
    int dy = txtrc.height() - th;
    int halign = flags & SKIN_HALIGN_MASK;
    int valign = flags & SKIN_VALIGN_MASK;
    if ( valign == SKIN_VALIGN_CENTER )
        y += dy / 2;
    else if ( valign == SKIN_VALIGN_BOTTOM )
        y += dy;
    if ( halign == SKIN_HALIGN_CENTER )
        x += dx / 2;
    else if ( halign == SKIN_HALIGN_RIGHT )
        x += dx;
    font->DrawTextString( &buf, x, y, text.c_str(), text.length(), L'?', NULL, false, 0 );
    buf.SetClipRect( &oldRc );
}

void CRRectSkin::drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text, LVFontRef font )
{
    lvRect rect = getClientRect( rc );
    CRSkinnedItem::drawText( buf, rect, text, font );
}
void CRRectSkin::drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text )
{
    lvRect rect = getClientRect( rc );
    CRSkinnedItem::drawText( buf, rect, text );
}

void CRButtonSkin::drawButton( LVDrawBuf & buf, const lvRect & rect, int flags )
{
    lvRect rc = rect;
    rc.shrinkBy( _margins );
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
    if ( !btnImage.isNull() ) {
        buf.Draw( btnImage, rc.left, rc.top, rc.width(), rc.height(), false );
    }
}

void CRScrollSkin::drawScroll( LVDrawBuf & buf, const lvRect & rect, bool vertical, int pos, int maxpos, int pagesize )
{
    lvRect rc = rect;
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
    buf.Draw( bodyImg, bodyRect.left, bodyRect.top, bodyRect.width(), bodyRect.height(), false );
    buf.Draw( sliderImg, sliderRect.left, sliderRect.top, sliderRect.width(), sliderRect.height(), false );
}

CRRectSkin::CRRectSkin()
: _margins( 0, 0, 0, 0 )
{
}

CRWindowSkin::CRWindowSkin()
: _titleSize( 0, 28 )
{
}

CRMenuSkin::CRMenuSkin()
{
}


// WINDOW skin stub
class CRSimpleWindowSkin : public CRWindowSkin
{
public:
	CRSimpleWindowSkin( CRSkinImpl * skin )
	{
		setBackgroundColor( 0xAAAAAA );
	}
};

class CRSimpleFrameSkin : public CRRectSkin
{
public:
	CRSimpleFrameSkin( CRSkinImpl * skin )
	{
		setBackgroundColor( 0xAAAAAA );
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
        setBackgroundColor( 0xAAAAAA );
        setTitleSize( lvPoint( 0, 48 ) );
        setBorderWidths( lvRect( 8, 8, 8, 8 ) );
        _titleSkin = CRRectSkinRef( new CRRectSkin() );
        _titleSkin->setBackgroundColor(0xAAAAAA);
        _titleSkin->setTextColor(0x000000);
        _titleSkin->setFontBold( true );
        _titleSkin->setFontSize( 28 );
        _itemSkin = CRRectSkinRef( new CRRectSkin() );
        _itemSkin->setBackgroundImage( skin->getImage( L"std_menu_item_background.xpm" ) );
        _itemSkin->setBorderWidths( lvRect( 8, 8, 8, 8 ) );
        _itemShortcutSkin = CRRectSkinRef( new CRRectSkin() );
        _itemShortcutSkin->setBackgroundImage( skin->getImage( L"std_menu_shortcut_background.xpm" ) );
        _itemShortcutSkin->setBorderWidths( lvRect( 12, 8, 8, 8 ) );
        _itemShortcutSkin->setTextColor( 0x555555 );
        _itemShortcutSkin->setTextHAlign( SKIN_HALIGN_CENTER );
        _itemShortcutSkin->setTextVAlign( SKIN_VALIGN_CENTER );
	}
};

CRButtonSkin::CRButtonSkin() { }

void CRSkinContainer::readButtonSkin(  const lChar16 * path, CRButtonSkin * res )
{
    lString16 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        readButtonSkin( base.c_str(), res );
    }

    readRectSkin( path, res );
    res->setNormalImage( readImage( path, L"normal" ) );
    res->setDisabledImage( readImage( path, L"disabled" ) );
    res->setPressedImage( readImage( path, L"pressed" ) );
    res->setSelectedImage( readImage( path, L"selected" ) );

    LVImageSourceRef img = res->getNormalImage();
    lvRect margins = res->getBorderWidths();
    if ( !img.isNull() ) {
        res->setMinSize( lvPoint( margins.left + margins.right + img->GetWidth(), margins.top + margins.bottom + img->GetHeight() ) );
    }
};

CRScrollSkin::CRScrollSkin() { }

void CRSkinContainer::readScrollSkin(  const lChar16 * path, CRScrollSkin * res )
{
    lString16 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        readScrollSkin( base.c_str(), res );
    }

    lString16 p( path );

    readRectSkin( path, res );

    CRButtonSkinRef upButton( new CRButtonSkin() );
    readButtonSkin(  (p + L"/upbutton").c_str(), upButton.get() );
    res->setUpButton( upButton );

    CRButtonSkinRef downButton( new CRButtonSkin() );
    readButtonSkin(  (p + L"/downbutton").c_str(), downButton.get() );
    res->setDownButton( downButton );

    CRButtonSkinRef leftButton( new CRButtonSkin() );
    readButtonSkin(  (p + L"/leftbutton").c_str(), leftButton.get() );
    res->setLeftButton( leftButton );

    CRButtonSkinRef rightButton( new CRButtonSkin() );
    readButtonSkin(  (p + L"/rightbutton").c_str(), rightButton.get() );
    res->setRightButton( rightButton );

    res->setHBody( readImage( (p + L"/hbody").c_str(), L"body" ) );
    res->setHSlider( readImage( (p + L"/hbody").c_str(), L"slider" ) );
    res->setVBody( readImage( (p + L"/vbody").c_str(), L"body" ) );
    res->setVSlider( readImage( (p + L"/vbody").c_str(), L"slider" ) );
};

void CRSkinContainer::readRectSkin(  const lChar16 * path, CRRectSkin * res )
{
    lString16 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        readRectSkin( base.c_str(), res );
    }

    lString16 p( path );
    lString16 bgpath = p + L"/background";
    lString16 borderpath = p + L"/border";
    lString16 textpath = p + L"/text";
    lString16 sizepath = p + L"/size";
    res->setBackgroundImage( readImage( bgpath.c_str(), L"image" ) );
    res->setBackgroundColor( readColor( bgpath.c_str(), L"color", 0xFFFFFF ) );
    res->setBorderWidths( readRect( borderpath.c_str(), L"widths", lvRect( 12, 8, 8, 8 ) ) );
    res->setMinSize( readSize( sizepath.c_str(), L"minvalue", lvPoint( 0, 0 ) ) );
    res->setMaxSize( readSize( sizepath.c_str(), L"maxvalue", lvPoint( 0, 0 ) ) );
    res->setFontFace( readString( textpath.c_str(), L"face", L"Arial" ) );
    res->setTextColor( readColor( textpath.c_str(), L"color", 0x000000 ) );
    res->setFontBold( readBool( textpath.c_str(), L"bold", false ) );
    res->setFontItalic( readBool( textpath.c_str(), L"italic", false ) );
    res->setFontSize( readInt( textpath.c_str(), L"size", 24 ) );
    res->setTextHAlign( readHAlign( textpath.c_str(), L"halign", SKIN_HALIGN_LEFT) );
    res->setTextVAlign( readVAlign( textpath.c_str(), L"valign", SKIN_VALIGN_CENTER) );
}

void CRSkinContainer::readWindowSkin(  const lChar16 * path, CRWindowSkin * res )
{
    lString16 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        readWindowSkin( base.c_str(), res );
    }

    lString16 p( path );
    readRectSkin(  path, res );
    CRRectSkinRef titleSkin( new CRRectSkin() );
    readRectSkin(  (p + L"/title").c_str(), titleSkin.get() );
    res->setTitleSkin( titleSkin );
    lvPoint minsize = titleSkin->getMinSize();
    res->setTitleSize( minsize );

    CRRectSkinRef clientSkin( new CRRectSkin() );
    readRectSkin(  (p + L"/client").c_str(), clientSkin.get() );
    res->setClientSkin( clientSkin );

    CRScrollSkinRef scrollSkin( new CRScrollSkin() );
    readScrollSkin(  (p + L"/scroll").c_str(), scrollSkin.get() );
    res->setScrollSkin( scrollSkin );
}

void CRSkinContainer::readMenuSkin(  const lChar16 * path, CRMenuSkin * res )
{
    lString16 base = getBasePath( path );
    RecursionLimit limit;
    if ( !base.empty() && limit.test() ) {
        // read base skin first
        readMenuSkin( base.c_str(), res );
    }

    lString16 p( path );
    readWindowSkin( path, res );
    CRRectSkinRef itemSkin( new CRRectSkin() );
    readRectSkin(  (p + L"/item").c_str(), itemSkin.get() );
    res->setItemSkin( itemSkin );
    CRRectSkinRef shortcutSkin( new CRRectSkin() );
    readRectSkin(  (p + L"/shortcut").c_str(), shortcutSkin.get() );
    res->setItemShortcutSkin( shortcutSkin );

    CRRectSkinRef itemSelSkin( new CRRectSkin() );
    readRectSkin(  (p + L"/selitem").c_str(), itemSelSkin.get() );
    res->setSelItemSkin( itemSelSkin );
    CRRectSkinRef shortcutSelSkin( new CRRectSkin() );
    readRectSkin(  (p + L"/selshortcut").c_str(), shortcutSelSkin.get() );
    res->setSelItemShortcutSkin( shortcutSelSkin );
}

lString16 CRSkinImpl::pathById( const lChar16 * id )
{
    ldomElement * elem = _doc->getElementById( id );
    if ( !elem )
        return lString16();
    return ldomXPointer(elem, -1).toString();
}

/// returns rect skin
CRRectSkinRef CRSkinImpl::getRectSkin( const lChar16 * path )
{
    lString16 p(path);
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
    _rectCache.set( lString16(path), res );
    return res;
}

/// returns window skin
CRWindowSkinRef CRSkinImpl::getWindowSkin( const lChar16 * path )
{
    lString16 p(path);
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
    _windowCache.set( lString16(path), res );
    return res;
}

/// returns menu skin
CRMenuSkinRef CRSkinImpl::getMenuSkin( const lChar16 * path )
{
    lString16 p(path);
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
    _menuCache.set( lString16(path), res );
    return res;
}
