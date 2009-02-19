//
// C++ Implementation: document view dialog
//
// Description: 
//      Allows to show (FB2) document on screen
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// viewdlg.cpp

#include "viewdlg.h"

CRViewDialog::CRViewDialog(CRGUIWindowManager * wm, lString16 title, lString8 text, lvRect rect, bool showScroll, bool showFrame )
    : CRDocViewWindow(wm), _text(text), _showScroll( showScroll ), _showFrame( showFrame )
{
    _passKeysToParent = _passCommandsToParent = false;
    _stream = LVCreateStringStream( text );
    _skin = _wm->getSkin()->getWindowSkin(L"#dialog");
    setAccelerators( _wm->getAccTables().get("browse") );
    _title = title;
    lvRect fsRect = _wm->getScreen()->getRect();
    _fullscreen = false;
    if ( rect.isEmpty() ) {
        rect = fsRect;
        _fullscreen = false;
    } else {
        _fullscreen = (rect == fsRect);
    }
    setRect( rect );
    _clientRect = rect;
    getDocView()->setBackgroundColor(0xFFFFFF);
    getDocView()->setTextColor(0x000000);
    getDocView()->setFontSize( 20 );
    getDocView()->setShowCover( false );
    getDocView()->setPageHeaderInfo( 0 ); // hide title bar
    getDocView()->setPageMargins( lvRect(8,8,8,8) );
    getDocView()->LoadDocument(_stream);
}

bool CRViewDialog::onCommand( int command, int params )
{
    switch ( command ) {
        case MCMD_CANCEL:
        case MCMD_OK:
            _wm->closeWindow( this );
            return true;
    }
    return CRDocViewWindow::onCommand( command, params );
}

void CRViewDialog::draw()
{
    CRRectSkinRef titleSkin = _skin->getTitleSkin();
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    lvRect borders = clientSkin->getBorderWidths();
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    if ( _showFrame ) {
        _skin->draw( *drawbuf, _rect );
        lvRect titleRect = _skin->getTitleRect( _rect );
        titleSkin->draw( *drawbuf, titleRect );
        titleSkin->drawText( *drawbuf, titleRect, _title );
    }
    // draw toc
    clientSkin->draw( *drawbuf, _clientRect );
    if ( !_scrollRect.isEmpty() ) {
        // draw scrollbar
        CRScrollSkinRef sskin = _skin->getScrollSkin();
        int pages = getDocView()->getPageCount();
        int page = getDocView()->getCurPage();
        sskin->drawScroll( *drawbuf, _scrollRect, false, page, pages, 1 );
    }
    LVDocImageRef pageImage = _docview->getPageImage(0);
    LVDrawBuf * pagedrawbuf = pageImage->getDrawBuf();
    _wm->getScreen()->draw( pagedrawbuf, _clientRect.left, _clientRect.top );
}

void CRViewDialog::setRect( const lvRect & rc )
{
    if ( rc == _rect )
        return;
    _rect = rc;
    _clientRect = _rect;
    if ( _showFrame ) {
        if ( !_skin.isNull() )
            _clientRect = _skin->getClientRect( rc );
    }
    _scrollRect = _clientRect;
    if ( _showScroll ) {
        _scrollRect.top = _scrollRect.bottom - 32; //TODO: scroll height
    } else {
        _scrollRect.top = _scrollRect.bottom;
    }
    _clientRect.bottom = _scrollRect.top;
    _docview->Resize( _clientRect.width(), _clientRect.height() );
    _docview->checkRender();
    int pages = _docview->getPageCount();
    if ( pages <= 1 && _showScroll ) {
        // hide scroll
        _scrollRect.top = _scrollRect.bottom;
        _clientRect.bottom = _scrollRect.top;
        _docview->Resize( _clientRect.width(), _clientRect.height() );
    }
    setDirty();
}
