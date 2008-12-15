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

