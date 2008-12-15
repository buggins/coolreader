/***************************************************************************
 *   Copyright (C) 2008 by Vadim Lopatin                                   *
 *   vadim.lopatin@coolreader.org                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef CR_SKIN_INCLUDED
#define CR_SKIN_INCLUDED

#include "lvtypes.h"
#include "lvptrvec.h"
#include "lvdrawbuf.h"
#include "lvdocview.h"

class CRSkinBase
{
public:
    /// gets image from container
    virtual LVImageSourceRef getImage( const lChar16 * filename ) = 0;
    /// gets image from container
    virtual LVImageSourceRef getImage( const lString16 & filename ) { return getImage( filename.c_str() ); }
    /// gets doc pointer by path
    virtual ldomXPointer getXPointer( const lString16 & xPointerStr ) = 0;
    /// gets doc pointer by path
    virtual ldomXPointer getXPointer( const lChar16 * xPointerStr ) { return getXPointer( lString16(xPointerStr) ); }
    /// reads color value from attrname attribute of element specified by path, returns defValue if not found
    virtual lUInt32 getColor( const lChar16 * path, const lChar16 * attrname, lUInt32 defValue );
    /// destructor
    virtual ~CRSkinBase() { }
};

/// skin reference
typedef LVRef<CRSkinBase> CRSkinRef;

class CRSkinItem : public CRSkinBase
{
protected:
    CRSkinRef _skin;
    ldomXPointer _ptr;
public:
    /// constructor
    CRSkinItem( CRSkinRef skin, const lChar16 * path ) : _skin(skin), _ptr( _skin->getXPointer( path )) { }
    /// gets image from container
    virtual LVImageSourceRef getImage( const lString16 & filename ) { return _skin->getImage( filename.c_str() ); }
    /// gets doc pointer by relative path
    virtual ldomXPointer getXPointer( const lChar16 * xPointerStr ) { return _ptr.relative( xPointerStr ); }
};

/// skin file support
class CRSkin : public CRSkinBase
{
protected:
    LVContainerRef _container;
    LVAutoPtr<ldomDocument> _doc;
public:
    /// gets image from container
    virtual LVImageSourceRef getImage( const lChar16 * filename );
    /// gets doc pointer by asolute path
    virtual ldomXPointer getXPointer( const lString16 & xPointerStr ) { return _doc->createXPointer( xPointerStr ); }
    /// constructor does nothing
    CRSkin() { }
    virtual ~CRSkin() { }
    // open from container
    virtual bool open( LVContainerRef container );
};


/// opens skin from directory or .zip file
CRSkinRef LVOpenSkin( const lString16 & pathname );

#endif// CR_SKIN_INCLUDED
