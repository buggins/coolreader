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

#include <cri18n.h>
#include "recentdlg.h"
#include "mainwnd.h"

typedef LVPtrVector<CRFileHistRecord> FileList;

class CRRecentBookMenuItem : public CRMenuItem
{
private:
    CRFileHistRecord * _book;
public:
    CRRecentBookMenuItem( CRMenu * menu, int index, CRFileHistRecord * book );
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected );
};

CRRecentBookMenuItem::CRRecentBookMenuItem( CRMenu * menu, int index, CRFileHistRecord * book )
: CRMenuItem(menu, index, lString16::empty_str, LVImageSourceRef(), LVFontRef() ), _book( book )
{
}

void CRRecentBookMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected )
{
    if ( !_book ) {
        CRMenuItem::Draw( buf, rc, skin, valueSkin, selected );
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
        title << " - " << series;

    lvRect posRect = textRect;
    if ( !author.empty() ) {
        posRect.bottom = posRect.top + skin->getFont()->getHeight() + itemBorders.top + itemBorders.bottom;
        textRect.top = posRect.bottom - itemBorders.bottom;
        skin->drawText( buf, posRect, author );
    }
    if ( !title.empty() )
        valueSkin->drawText( buf, textRect, title );
}

#ifdef CR_POCKETBOOK
#include "cr3pocketbook.h"
#include "inkview.h"

static CRRecentBooksMenu *bmkDialog = NULL;

static imenu _contextMenu[] = {
	{ITEM_ACTIVE, MCMD_OPEN_RECENT_BOOK, NULL, NULL},
	{ITEM_ACTIVE, PB_CMD_BOOKMARK_REMOVE, NULL, NULL},
	{ 0, 0, NULL, NULL }
};

static void handle_contextMenu(int index)
{
	bmkDialog->handleContextMenu(index);
}

void CRRecentBooksMenu::showContextMenu()
{
	CRRecentBookMenuItem *item = static_cast<CRRecentBookMenuItem *>(getItems()[_selectedItem]);
	CRMenuSkinRef skin = getSkin();
	CRRectSkinRef separatorSkin = skin->getSeparatorSkin();
    int separatorHeight = 0;
    if ( !separatorSkin.isNull() )
        separatorHeight = separatorSkin->getMinSize().y;

    lvRect clientRect;
    getClientRect(clientRect);
    lvPoint itemSize = getMaxItemSize();
        int y = clientRect.top + (itemSize.y + separatorHeight) * (_selectedItem - _topItem) +
			((itemSize.y + separatorHeight)/4);
	if (_contextMenu[0].text == NULL) {
		_contextMenu[0].text = (char *)_("Open book");
		_contextMenu[1].text = (char *)_("Delete record");
	}
	OpenMenu(_contextMenu, MCMD_OPEN_RECENT_BOOK,
		ScreenWidth()/4, 
		y, 
		handle_contextMenu);
}

void CRRecentBooksMenu::handleContextMenu(int index)
{
	_wm->postCommand(index, 0);
	_wm->processPostedEvents();
}
#endif

CRRecentBooksMenu::CRRecentBooksMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc)
    : CRFullScreenMenu( wm, MCMD_BOOKMARK_LIST, lString16(_("Open recent book")), numItems, rc )
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
    //_helpText = "Long press 1..8 = set, short press = go to";
    //_helpHeight = 36;
    CRGUIAcceleratorTableRef acc = _wm->getAccTables().get("bookmarks");
    if ( acc.isNull() )
        acc = _wm->getAccTables().get("menu");
    setAccelerators( acc );
    setSkinName(lString16("#bookmarks"));
    lString16 pattern(_("$1 - open book\n$2, $3 - close"));
#ifdef CR_POCKETBOOK
	pattern.replaceParam(1, getCommandKeyName( MCMD_SELECT ));
#else    
    pattern.replaceParam(1, getItemNumberKeysName());
#endif
    pattern.replaceParam(2, getCommandKeyName(MCMD_OK) );
    pattern.replaceParam(3, getCommandKeyName(MCMD_CANCEL) );
    _statusText = pattern;
#ifdef CR_POCKETBOOK
    bmkDialog = this;
#endif
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
        highlightCommandItem( n );
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
    } else if (command == MCMD_SELECT) {
		if (_selectedItem >= 0) {
			CRMenuItem * item = getItems()[_selectedItem];
			int n = item->getId();
			closeMenu( MCMD_OPEN_RECENT_BOOK, n );
			return true;
		}
		closeMenu( 0 );
		return true;
	} else if (command == MCMD_SELECT_LONG) {
		if (_selectedItem >= 0) {
#ifdef CR_POCKETBOOK
			showContextMenu();
#else
            removeItem( _selectedItem );
            setDirty();			
#endif
            return true;
		}
	} else if (command == MCMD_PREV_PAGE) {
		if (_topItem == 0) {
			closeMenu(0);
			return true;
		}
	}
#ifdef CR_POCKETBOOK
	else if (command == MCMD_OPEN_RECENT_BOOK) {
		closeMenu( command, _selectedItem + 1 );
		return true;
	} else if (command == PB_CMD_BOOKMARK_REMOVE && _selectedItem >= 0) {
		removeItem( _selectedItem );
		setDirty();
		return true;
	}
#endif
	return CRMenu::onCommand(command, params);
    //closeMenu( 0 );
    //return true;
}

bool CRRecentBooksMenu::removeItem( int index )
{
    if ( index <0 || index >= _items.length() )
        return false;
	int last = getLastOnPage();
    CRMenuItem * item = getItems()[index];
    int n = item->getId();
    _files->erase( n, 1 );
    _items.erase( index, 1 );
    for ( int i=0; i<_items.length(); i++ ) {
        if ( _items[i]->getId() > n ) {
            _items[i]->setId( _items[i]->getId() - 1 );
            _items[i]->setItemDirty();
		}
    }
    if (_items.length() < last) {
		_pageUpdate = true;
		_selectedItem = _items.length() -1;
	}
    setCurPage( (_items.length()-1) /  _pageItems );
    return true;
}
