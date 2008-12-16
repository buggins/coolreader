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

/// returns necessary window size for specified client size
lvPoint CRWindowSkin::getWindowSize( const lvPoint & clientSize )
{
    lvRect borders = getBorderWidths();
    lvPoint tsz = getTitleSize();
    return lvPoint( clientSize.x + borders.left + borders.right + tsz.x, clientSize.y + borders.top + borders.bottom + tsz.y );
}

// WINDOW skin stub
class CRSimpleWindowSkin : public CRWindowSkin
{
public:
    virtual void draw( LVDrawBuf & buf, const lvRect & rc )
    {
        buf.Rect( rc, 8, 0xAAAAAA );
        buf.FillRect( getTitleRect( rc ), 0xAAAAAA );
        buf.FillRect( getClientRect( rc ), 0xAAAAAA );
    }
    virtual lvRect getBorderWidths()
    {
        return lvRect(8,8,8,8);
    }
    virtual lvRect getClientRect( const lvRect &windowRect )
    {
        lvRect rc = CRRectSkin::getClientRect( windowRect );
        rc.top += 25;
        return rc;
    }
    virtual lvPoint getTitleSize() { return lvPoint(0, 25); }
};

class CRSimpleMenuItemSkin : public CRRectSkin
{
public:
	CRSimpleMenuItemSkin()
	{
	}
    virtual lvRect getBorderWidths()
    {
        return lvRect(12,6,12,6);
    }
    virtual void draw( LVDrawBuf & buf, const lvRect & rc )
    {
		LVImageSourceRef img = LVCreateStretchFilledTransform( LVCreateXPMImageSource( menu_item_background ),
			rc.width(), rc.height() );
		buf.Draw( img, rc.left, rc.top, rc.width(), rc.height() );
    }
};

class CRSimpleMenuSkin : public CRMenuSkin
{
public:
    virtual void draw( LVDrawBuf & buf, const lvRect & rc )
    {
        buf.Rect( rc, 8, 0xAAAAAA );
        buf.FillRect( getTitleRect( rc ), 0xAAAAAA );
        buf.FillRect( getClientRect( rc ), 0xAAAAAA );
    }
    virtual lvRect getBorderWidths()
    {
        return lvRect(8,8,8,8);
    }
    virtual lvRect getClientRect( const lvRect &windowRect )
    {
		lvPoint tsz = getTitleSize();
        lvRect rc = CRRectSkin::getClientRect( windowRect );
		rc.top += tsz.y;
        return rc;
    }
    virtual lvPoint getTitleSize() { return lvPoint(0, 48); }
    virtual CRRectSkinRef getItemSkin()
    {
        return CRRectSkinRef( new CRSimpleMenuItemSkin() );
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
