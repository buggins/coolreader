//
// C++ Implementation: fullscreen menu dialog
//
// Description: 
//      Allows to set or go to bookmarks
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// bmkdlg.cpp

#include "fsmenu.h"


CRFullScreenMenu::CRFullScreenMenu(CRGUIWindowManager * wm, int id, const lString16 & caption, int numItems, lvRect & rc)
    : CRMenu( wm, NULL, id, caption, LVImageSourceRef(), LVFontRef(), LVFontRef() )
{
    _rect = rc;
    _pageItems = numItems;
    _helpHeight = 0;
    _fullscreen = true;
}

const lvRect & CRFullScreenMenu::getRect()
{
    return _rect;
}

lvPoint CRFullScreenMenu::getMaxItemSize()
{
    return lvPoint( _rect.width(), getItemHeight() );
}

lvPoint CRFullScreenMenu::getSize()
{
    return lvPoint( _rect.width(), _rect.height() );
}

void CRFullScreenMenu::Draw( LVDrawBuf & buf, int x, int y )
{
    CRMenu::Draw( buf, x, y );
    CRMenuSkinRef skin = getSkin();
    lvRect rc = skin->getClientRect( _rect );
    int ih = getItemHeight();
    rc.top += _pageItems * ih + 4;
    int scrollHeight = 0;
    if ( _items.length() > _pageItems )
        scrollHeight = 34;
    rc.bottom -= scrollHeight;
    //skin->getItemSkin()->draw( buf, rc );
    if ( !_helpText.empty() )
        skin->getItemSkin()->drawText( buf, rc, _helpText );
}
