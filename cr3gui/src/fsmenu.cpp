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

#include <cri18n.h>
#include "mainwnd.h"
#include "fsmenu.h"


CRFullScreenMenu::CRFullScreenMenu(CRGUIWindowManager * wm, int id, const lString16 & caption, int numItems, lvRect & rc)
    : CRMenu( wm, NULL, id, caption, LVImageSourceRef(), LVFontRef(), LVFontRef() )
{
    _rect = rc;
    _pageItems = numItems;
    _fullscreen = true;
}

lString16 CRFullScreenMenu::getCommandKeyName( int cmd, int param )
{
    int k, f;
    bool found = _acceleratorTable->findCommandKey( cmd, param, k, f );
    if ( !found )
        return lString16::empty_str;
    return lString16(getKeyName( k, f ));
}

lString16 CRFullScreenMenu::getItemNumberKeysName()
{
    int k9, f9;
    lString16 selKeyName;
    bool hasKey9 = _acceleratorTable->findCommandKey( MCMD_SELECT_9, 0, k9, f9 );
    if ( hasKey9 )
        selKeyName = lString16(_("1..9"));
    else
        selKeyName = lString16(_("1..8"));
    return selKeyName;
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

/*
void CRFullScreenMenu::Draw( LVDrawBuf & buf, int x, int y )
{
    CRGUIWindow::draw();
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
*/
