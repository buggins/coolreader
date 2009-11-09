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

#include "recentdlg.h"
#include <cri18n.h>

typedef LVPtrVector<CRFileHistRecord> FileList;

class CRRecentBookMenuItem : public CRMenuItem
{
private:
    CRFileHistRecord * _book;
public:
    CRRecentBookMenuItem( CRMenu * menu, int index, CRFileHistRecord * book );
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected );
};

CRRecentBookMenuItem::CRRecentBookMenuItem( CRMenu * menu, int index, CRFileHistRecord * book )
: CRMenuItem(menu, index, lString16(L""), LVImageSourceRef(), LVFontRef() ), _book( book )
{
}

void CRRecentBookMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected )
{
    if ( !_book ) {
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

    lString16 author = _book->getAuthor();
    lString16 title = _book->getTitle();
    lString16 series = _book->getSeries();
    if ( title.empty() )
        title = _book->getFileName();
    else if ( !series.empty() )
        title << L" - " << series;

    lvRect posRect = textRect;
    if ( !author.empty() ) {
        posRect.bottom = posRect.top + skin->getFont()->getHeight() + 8 + 8;
        textRect.top = posRect.bottom - 8;
        skin->drawText( buf, posRect, author, skin->getFont() );
    }
    if ( !title.empty() )
        skin->drawText( buf, textRect, title, skin->getFont() );
}

CRRecentBooksMenu::CRRecentBooksMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc)
    : CRFullScreenMenu( wm, MCMD_BOOKMARK_LIST, _16("Open recent book"), numItems, rc )
{
    docview->savePosition(); // to move current file to top
    LVPtrVector<CRFileHistRecord> & files = docview->getHistory()->getRecords();
    _files = &files;
    // skip Null
    for ( int i=1; i<files.length(); i++ ) {
        CRFileHistRecord * file = files.get( i );
        CRRecentBookMenuItem * item = new CRRecentBookMenuItem( this, i, file );
        addItem( item );
    }
    //_helpText = L"Long press 1..8 = set, short press = go to";
    //_helpHeight = 36;
}

/// returns true if command is processed
bool CRRecentBooksMenu::onCommand( int command, int params )
{
    if ( command==MCMD_SCROLL_FORWARD ) {
        setCurPage( getCurPage()+1 );
        return true;
    }
    if ( command==MCMD_SCROLL_BACK ) {
        setCurPage( getCurPage()-1 );
        return true;
    }
    if ( command>=MCMD_SELECT_1 && command<=MCMD_SELECT_9 ) {
        int index = command - MCMD_SELECT_1 + getTopItem();
        if ( index < 0 || index >= getItems().length() ) {
            closeMenu( 0 );
            return true;
        }
        CRMenuItem * item = getItems()[index];
        int n = item->getId();
        closeMenu( MCMD_OPEN_RECENT_BOOK, n );
        return true;
    } else if ( command>=MCMD_SELECT_1_LONG && command<=MCMD_SELECT_9_LONG ) {
        int index = command - MCMD_SELECT_1_LONG + getTopItem();
        if ( index >=0 && index < _pageItems ) {
            //TODO: allow removing book from history
            //closeMenu( MCMD_REMOVE_RECENT_BOOK, index );
            removeItem( index );
            setDirty();
            return true;
        }
    }
    closeMenu( 0 );
    return true;
}

bool CRRecentBooksMenu::removeItem( int index )
{
    if ( index <0 || index >= _items.length() )
        return false;
    CRMenuItem * item = getItems()[index];
    int n = item->getId();
    _files->erase( n, 1 );
    _items.erase( index, 1 );
    for ( int i=0; i<_items.length(); i++ ) {
        if ( _items[i]->getId() > n )
            _items[i]->setId( _items[i]->getId() - 1 );
    }
    setCurPage( (_items.length()-1) /  _pageItems );
    return true;
}
