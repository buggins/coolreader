//
// C++ Implementation: bookmarks dialog
//
// Description: 
//      Allows to set or go to bookmarks
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// bmkdlg.cpp

#include "bmkdlg.h"

CRBookmarkMenuItem::CRBookmarkMenuItem( CRMenu * menu, int shortcut, CRBookmark * bookmark, int page )
: CRMenuItem(menu, shortcut, lString16(L"Empty slot"), LVImageSourceRef(), LVFontRef() ), _bookmark( bookmark ), _page(page)
{

}

void CRBookmarkMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected )
{
    if ( !_bookmark ) {
        CRMenuItem::Draw( buf, rc, skin, selected );
        return;
    }
    lvRect itemBorders = skin->getBorderWidths();
    skin->draw( buf, rc );
    buf.SetTextColor( 0x000000 );
    buf.SetBackgroundColor( 0xFFFFFF );
    int imgWidth = 0;
    int hh = rc.bottom - rc.top - itemBorders.top - itemBorders.bottom;
    if ( !_image.isNull() ) {
        int w = _image->GetWidth();
        int h = _image->GetHeight();
        buf.Draw( _image, rc.left + hh/2-w/2 + itemBorders.left, rc.top + hh/2 - h/2 + itemBorders.top, w, h );
        imgWidth = w + 8;
    }
    lvRect textRect = rc;
    textRect.left += imgWidth;
    lString16 postext(L"Page ");
    lvRect posRect = textRect;
    lString16 text = _bookmark->getPosText();
    if ( !text.empty() ) {
        posRect.bottom = posRect.top + skin->getFont()->getHeight() + 8 + 8;
        textRect.top = posRect.bottom - 8;
    }
    postext << lString16::itoa( _page+1 ) << L" (";
    postext << lString16::itoa( _bookmark->getPercent()/100 ) << L"." << lString16::itoa( _bookmark->getPercent()%100 ) << L"%)";
    postext << L"  " << _bookmark->getTitleText();
    skin->drawText( buf, posRect, postext, skin->getFont() );
    if ( !text.empty() )
        skin->drawText( buf, textRect, text, skin->getFont() );
}

CRBookmarkMenu::CRBookmarkMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc)
    : CRFullScreenMenu( wm, MCMD_BOOKMARK_LIST, lString16(L"Bookmarks"), numItems, rc )
{
    CRFileHistRecord * bookmarks = docview->getCurrentFileHistRecord();
    for ( int i=1; i<=_pageItems; i++ ) {
        CRBookmark * bm = bookmarks->getShortcutBookmark(i);
        int page = 0;
        if ( bm ) {
            ldomXPointer p = docview->getDocument()->createXPointer( bm->getStartPos() );
            if ( !p.isNull() ) {
                /// get page number by bookmark
                page = docview->getBookmarkPage( p );
                /// get bookmark position text
                if ( page<0 )
                    bm = NULL;
            }
        }
        CRBookmarkMenuItem * item = new CRBookmarkMenuItem( this, i, bm, page );
        addItem( item );
    }
    _helpText = L"Long press 1..8 = set, short press = go to";
    _helpHeight = 36;
}

/// returns true if command is processed
bool CRBookmarkMenu::onCommand( int command, int params )
{
    if ( command>=MCMD_SELECT_1 && command<=MCMD_SELECT_9 ) {
        int index = command - MCMD_SELECT_1 + 1;
        if ( index >=1 && index <= _pageItems ) {
            closeMenu( DCMD_BOOKMARK_GO_N, index );
            return true;
        }
    } else if ( command>=MCMD_SELECT_1_LONG && command<=MCMD_SELECT_9_LONG ) {
        int index = command - MCMD_SELECT_1_LONG + 1;
        if ( index >=1 && index <= _pageItems ) {
            closeMenu( DCMD_BOOKMARK_SAVE_N, index );
            return true;
        }
    }
    closeMenu( 0 );
    return true;
}

