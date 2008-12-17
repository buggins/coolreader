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

class CRSkinBase : public LVRefCounter
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

#define SKIN_VALIGN_MASK    0x0003
#define SKIN_VALIGN_TOP     0x0001
#define SKIN_VALIGN_CENTER  0x0000
#define SKIN_VALIGN_BOTTOM  0x0002
#define SKIN_HALIGN_MASK    0x0030
#define SKIN_HALIGN_LEFT    0x0000
#define SKIN_HALIGN_CENTER  0x0010
#define SKIN_HALIGN_RIGHT   0x0020

class CRSkinnedItem : public LVRefCounter
{
protected:
	lUInt32 _textcolor;
	lUInt32 _bgcolor;
	LVImageSourceRef _bgimage;
	lvPoint _bgimagesplit;
    lString16 _fontFace;
    int _fontSize;
    bool _fontBold;
    bool _fontItalic;
	LVFontRef _font;
    int _textAlign;
public:
	CRSkinnedItem();
    virtual int getTextAlign() { return _textAlign; }
    virtual int getTextVAlign() { return _textAlign & SKIN_VALIGN_MASK; }
    virtual int getTextHAlign() { return _textAlign & SKIN_HALIGN_MASK; }
    virtual void setTextAlign( int align ) { _textAlign = align; }
    virtual void setTextVAlign( int align ) { _textAlign = (_textAlign & SKIN_VALIGN_MASK ) | (align & SKIN_VALIGN_MASK); }
    virtual void setTextHAlign( int align ) { _textAlign = (_textAlign & SKIN_HALIGN_MASK ) | (align & SKIN_HALIGN_MASK); }
    virtual lUInt32 getTextColor() { return _textcolor; }
    virtual lUInt32 getBackgroundColor() { return _bgcolor; }
    virtual LVImageSourceRef getBackgroundImage() { return _bgimage; }
    virtual lvPoint getBackgroundImageSplit() { return _bgimagesplit; }
    virtual lString16 getFontFace() { return _fontFace; }
    virtual int getFontSize() { return _fontSize; }
    virtual bool getFontBold() { return _fontBold; }
    virtual bool getFontItalic() { return _fontItalic; }
    virtual void setFontFace( lString16 face );
    virtual void setFontSize( int size );
    virtual void setFontBold( bool bold );
    virtual void setFontItalic( bool italic );
    virtual LVFontRef getFont();
    virtual void setTextColor( lUInt32 color ) { _textcolor = color; }
    virtual void setBackgroundColor( lUInt32 color ) { _bgcolor = color; }
    virtual void setBackgroundImage( LVImageSourceRef img ) { _bgimage = img; }
    virtual void setBackgroundImageSplit( lvPoint pt ) { _bgimagesplit = pt; }
    virtual void setFont( LVFontRef fnt ) { _font = fnt; }
    virtual void draw( LVDrawBuf & buf, const lvRect & rc );
    virtual void drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text, lUInt32 textColor, lUInt32 bgColor, int flags );
    virtual void drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text )
    {
        drawText(  buf, rc, text, getTextColor(), getBackgroundColor(), getTextAlign() );
    }
    virtual ~CRSkinnedItem() { }
};

class CRRectSkin : public CRSkinnedItem
{
protected:
	lvRect _margins;
public:
	CRRectSkin();
	virtual void setBorderWidths( const lvRect & rc) { _margins = rc; }
	virtual lvRect getBorderWidths() { return _margins; }
    virtual lvRect getClientRect( const lvRect &windowRect );
    virtual void drawText( LVDrawBuf & buf, const lvRect & rc, lString16 text );
};
typedef LVFastRef<CRRectSkin> CRRectSkinRef;

class CRWindowSkin : public CRRectSkin
{
protected:
	lvPoint _titleSize;
	CRRectSkinRef _titleSkin;
public:
	CRWindowSkin();
    /// returns necessary window size for specified client size
    virtual lvPoint getWindowSize( const lvPoint & clientSize );
	virtual lvPoint getTitleSize() { return _titleSize; }
	virtual void setTitleSize( lvPoint sz ) { _titleSize = sz; }
    virtual lvRect getTitleRect( const lvRect &windowRect );
    virtual lvRect getClientRect( const lvRect &windowRect );
	virtual CRRectSkinRef getTitleSkin() { return _titleSkin; }
	virtual void setTitleSkin( CRRectSkinRef skin ) { _titleSkin = skin; }
};
typedef LVFastRef<CRWindowSkin> CRWindowSkinRef;

class CRMenuSkin : public CRWindowSkin
{
protected:
	CRRectSkinRef _itemSkin;
	CRRectSkinRef _itemShortcutSkin;
public:
	CRMenuSkin();
	virtual CRRectSkinRef getItemSkin() { return _itemSkin; }
	virtual void setItemSkin( CRRectSkinRef skin ) { _itemSkin = skin; }
	virtual CRRectSkinRef getItemShortcutSkin() { return _itemShortcutSkin; }
	virtual void setItemShortcutSkin( CRRectSkinRef skin ) { _itemShortcutSkin = skin; }
};
typedef LVFastRef<CRMenuSkin> CRMenuSkinRef;

/// skin reference
typedef LVFastRef<CRSkinBase> CRSkinBaseRef;

class CRSkinItem : public CRSkinBase
{
protected:
    CRSkinBaseRef _skin;
    ldomXPointer _ptr;
public:
    /// constructor
    CRSkinItem( CRSkinBaseRef skin, const lChar16 * path ) : _skin(skin), _ptr( _skin->getXPointer( path )) { }
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
    /// returns window skin
    CRWindowSkinRef getWindowSkin( const lChar16 * path );
    /// returns menu skin
    CRMenuSkinRef getMenuSkin( const lChar16 * path );
    /// gets image from container
    virtual LVImageSourceRef getImage( const lChar16 * filename );
    /// gets doc pointer by asolute path
    virtual ldomXPointer getXPointer( const lString16 & xPointerStr ) { return _doc->createXPointer( xPointerStr ); }
    /// constructor does nothing
    CRSkin() { }
    virtual ~CRSkin() { }
    // open from container
    virtual bool open( LVContainerRef container );
    virtual bool open( lString8 simpleXml );
};

/// skin reference
typedef LVFastRef<CRSkin> CRSkinRef;

/// opens skin from directory or .zip file
CRSkinRef LVOpenSkin( const lString16 & pathname );
/// open simple skin, without image files, from string
CRSkinRef LVOpenSimpleSkin( const lString8 & xml );

#endif// CR_SKIN_INCLUDED
