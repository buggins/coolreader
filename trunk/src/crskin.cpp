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

/// gets image from container
LVImageSourceRef CRSkin::getImage(  const lChar16 * filename  )
{
    LVStreamRef stream = _container->OpenStream( filename, LVOM_READ );
    if ( !stream )
        return LVImageSourceRef();
    LVImageSourceRef img = LVCreateStreamImageSource( stream );
    return img;
}

// open from container
bool CRSkin::open( LVContainerRef container )
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

bool CRSkin::open( lString8 simpleXml )
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

/// reads color value from attrname attribute of element specified by path, returns defValue if not found
lUInt32 CRSkinBase::getColor( const lChar16 * path, const lChar16 * attrname, lUInt32 defValue )
{
    ldomXPointer ptr = getXPointer( path );
    if ( !ptr )
        return defValue;
    if ( ptr.getNode()->getNodeType() != LXML_ELEMENT_NODE )
        return defValue;
    lString16 value = ptr.getNode()->getAttributeValue( attrname );
    if ( value.empty() )
        return defValue;
    css_length_t cv;
    lString8 buf = UnicodeToUtf8(value);
    const char * bufptr = buf.modify();
    if ( !parse_color_value( bufptr, cv ) )
        return defValue;
    return cv.value;
}

/// open simple skin, without image files, from string
CRSkinRef LVOpenSimpleSkin( const lString8 & xml )
{
    CRSkin * skin = new CRSkin();
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
    CRSkin * skin = new CRSkin();
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
		buf.Draw( img, rc.left, rc.top, rc.width(), rc.height() );
	}
}

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

void CRSkinnedItem::drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text, lUInt32 textColor, lUInt32 bgColor, int flags )
{
    SAVE_DRAW_STATE( buf );
    LVFontRef font = getFont();
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
    if ( getTextVAlign() == SKIN_VALIGN_CENTER )
        y += dy / 2;
    else if ( getTextVAlign() == SKIN_VALIGN_BOTTOM )
        y += dy;
    if ( getTextHAlign() == SKIN_HALIGN_CENTER )
        x += dx / 2;
    else if ( getTextHAlign() == SKIN_HALIGN_RIGHT )
        x += dx;
    font->DrawTextString( &buf, x, y, text.c_str(), text.length(), L'?', NULL, false, 0 );
    buf.SetClipRect( &oldRc );
}

void CRRectSkin::drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text )
{
    SAVE_DRAW_STATE( buf );
    lvRect rect = getClientRect( rc );
    CRSkinnedItem::drawText( buf, rect, text );
}

CRRectSkin::CRRectSkin()
: _margins( 4, 4, 4, 4 )
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
	CRSimpleWindowSkin()
	{
		setBackgroundColor( 0xAAAAAA );
	}
};

class CRSimpleFrameSkin : public CRRectSkin
{
public:
	CRSimpleFrameSkin()
	{
		setBackgroundColor( 0xAAAAAA );
	}
};

class CRSimpleMenuSkin : public CRMenuSkin
{
public:
    CRSimpleMenuSkin()
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
        _itemSkin->setBackgroundImage( LVCreateXPMImageSource( menu_item_background ) );
        _itemSkin->setBorderWidths( lvRect( 8, 8, 8, 8 ) );
        _itemShortcutSkin = CRRectSkinRef( new CRRectSkin() );
        _itemShortcutSkin->setBackgroundImage( LVCreateXPMImageSource( menu_shortcut_background ) );
        _itemShortcutSkin->setBorderWidths( lvRect( 12, 8, 8, 8 ) );
        _itemShortcutSkin->setTextColor( 0x555555 );
        _itemShortcutSkin->setTextHAlign( SKIN_VALIGN_CENTER );
        _itemShortcutSkin->setTextVAlign( SKIN_VALIGN_CENTER );
	}
};

/// returns window skin
CRWindowSkinRef CRSkin::getWindowSkin( const lChar16 * path )
{
    return CRWindowSkinRef( new CRSimpleWindowSkin() );
}

/// returns menu skin
CRMenuSkinRef CRSkin::getMenuSkin( const lChar16 * path )
{
    return CRMenuSkinRef( new CRSimpleMenuSkin() );
}
