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
#include "bmkdlg.h"
#include "mainwnd.h"

CRBookmarkMenuItem::CRBookmarkMenuItem( CRMenu * menu, int shortcut, CRBookmark * bookmark, int page )
: CRMenuItem(menu, shortcut, lString16(_("Empty slot")), LVImageSourceRef(), LVFontRef() ), _bookmark( bookmark ), _page(page)
{

}

void CRBookmarkMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected )
{
    if ( !_bookmark ) {
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
    lvRect posRect = textRect;
    lString16 text = _bookmark->getPosText();
    if ( !text.empty() ) {
        posRect.bottom = posRect.top + skin->getFont()->getHeight() + itemBorders.top + itemBorders.bottom;
        textRect.top = posRect.bottom - itemBorders.bottom;
    }
    lString16 postext(_("Page $1 ($2%)"));
    postext.replaceIntParam(1, _page+1);
    postext.replaceParam(2, lString16::itoa( _bookmark->getPercent()/100 ) + L"." + lString16::itoa( _bookmark->getPercent()%100 ));
    postext << L"  " << _bookmark->getTitleText();
    skin->drawText( buf, posRect, postext );
    if ( !text.empty() )
        valueSkin->drawText( buf, textRect, text );
}

void CRBookmarkMenu::setMode( bool goToMode )
{
    //if ( _goToMode==goToMode )
    //    return;
    int k, f;
    lString16 selKeyName = getItemNumberKeysName();
    lString16 modeKeyName = getCommandKeyName( MCMD_NEXT_MODE );
    bool hasModeSwitch = !modeKeyName.empty();
    _goToMode = goToMode;
    if ( _goToMode ) {
        _caption = lString16(_("Go to bookmark"));
        _label = _caption;
        _statusText = lString16(
                hasModeSwitch
                ? _("Short press $1 - go to bookmark,\n$2 - switch to SET mode")
                : _("Short press $1 - go to bookmark,\nlong press - set bookmark")
                );
    } else {
        _caption = lString16(_("Set bookmark"));
        _label = _caption;
        _statusText = lString16(
                hasModeSwitch
                ? _("$1 - set bookmark,\n$2 - switch to GO mode")
                : _("Short press $1 - set bookmark,\nlong press - go to bookmark")
                );
    }
    _statusText.replaceParam(1, selKeyName);
    _statusText.replaceParam(2, modeKeyName);
    setDirty();
}

/// returns index of selected item, -1 if no item selected
int CRBookmarkMenu::getSelectedItemIndex()
{
    CRFileHistRecord * bookmarks = _docview->getCurrentFileHistRecord();
    int curPage = _docview->getCurPage();
    int n = bookmarks->getLastShortcutBookmark()+1;
    for ( int i=1; i<=n; i++ ) {
        CRBookmark * bm = bookmarks->getShortcutBookmark(i);
        int page = 0;
        if ( bm ) {
            ldomXPointer p = _docview->getDocument()->createXPointer( bm->getStartPos() );
            if ( !p.isNull() ) {
                /// get page number by bookmark
                page = _docview->getBookmarkPage( p );
                /// get bookmark position text
                if ( page>0 && page==curPage )
                    return i-1;
            }
        }
    }
    return -1;
}

#define MIN_BOOKMARK_ITEMS 32
CRBookmarkMenu::CRBookmarkMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc, bool goToMode)
    : CRFullScreenMenu( wm, MCMD_BOOKMARK_LIST, lString16(_("Bookmarks")), numItems, rc )
    , _docview(docview)
{
    CRFileHistRecord * bookmarks = docview->getCurrentFileHistRecord();
    CRGUIAcceleratorTableRef acc = _wm->getAccTables().get("bookmarks");
    if ( acc.isNull() )
        acc = _wm->getAccTables().get("menu");
    setAccelerators( acc );
    setSkinName(lString16(L"#bookmarks"));
    int mc = getSkin()->getMinItemCount();
    if ( _pageItems < mc )
        _pageItems = mc;
    int n = bookmarks->getLastShortcutBookmark()+1;
    n = (n + _pageItems - 1) / _pageItems * _pageItems;
    int minitems = (MIN_BOOKMARK_ITEMS + _pageItems - 1) / _pageItems * _pageItems;
    if ( n<minitems )
        n = minitems;
    for ( int i=1; i<=n; i++ ) {
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
    setMode( goToMode );
}

/// returns true if command is processed
bool CRBookmarkMenu::onCommand( int command, int params )
{
    if ( command>=MCMD_SELECT_1 && command<=MCMD_SELECT_9 ) {
        int index = command - MCMD_SELECT_1 + 1;
        if ( index >=1 && index <= _pageItems ) {
            index += _topItem;
            int cmd = _goToMode ? DCMD_BOOKMARK_GO_N : DCMD_BOOKMARK_SAVE_N;
            closeMenu( cmd, index );
            return true;
        }
    } else if ( command>=MCMD_SELECT_1_LONG && command<=MCMD_SELECT_9_LONG ) {
        int index = command - MCMD_SELECT_1_LONG + 1;
        if ( index >=1 && index <= _pageItems ) {
            index += _topItem;
            int cmd = _goToMode ? DCMD_BOOKMARK_SAVE_N : DCMD_BOOKMARK_GO_N;
            closeMenu( cmd, index );
            return true;
        }
    } else if ( command==MCMD_NEXT_MODE || command==MCMD_PREV_MODE ) {
        setMode( !_goToMode );
        return true;
    }
    return CRMenu::onCommand(command, params);
    //closeMenu( 0 );
    //return true;
}

