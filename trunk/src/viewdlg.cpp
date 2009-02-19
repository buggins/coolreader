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

/// adds XML and FictionBook tags for utf8 fb2 document
lString8 CRViewDialog::makeFb2Xml( const lString8 & body )
{
    lString8 res;
    res << "\0xef\0xbb\0xbf";
    res << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    res << "<FictionBook><body>";
    res << body;
    res << "</body></FictionBook>";
    return res;
}

static const char * def_view_css = 
"body { text-align: left; text-indent: 0px; font-family: \"DejaVu Sans\", \"FreeSans\", \"Arial\", sans-serif; }\n"
"p { text-align: justify; text-indent: 2em; margin-top:0em; margin-bottom: 0em }\n"
"empty-line { height: 1em }\n"
"hr { height: 1px; background-color: #808080; margin-top: 0.5em; margin-bottom: 0.5em }\n"
"sub { vertical-align: sub; font-size: 70% }\n"
"sup { vertical-align: super; font-size: 70% }\n"
"strong, b { font-weight: bold }\n"
"emphasis, i { font-style: italic }\n"
"a { text-decoration: underline }\n"
"a[type=\"note\"] { vertical-align: super; font-size: 70%; text-decoration: none }\n"
"image { text-align: center; text-indent: 0px }\n"
"p image { display: inline }\n"
"title p, subtitle p, h1 p, h2 p, h3 p, h4 p, h5 p, h6 p { text-align: center; text-indent: 0px }\n"
"cite p, epigraph p { text-align: left; text-indent: 0px }\n"
"v { text-align: left; text-indent: 0px }\n"
"stanza + stanza { margin-top: 1em; }\n"
"stanza { margin-left: 30%; text-align: left; font-style: italic  }\n"
"poem { margin-top: 1em; margin-bottom: 1em; text-indent: 0px }\n"
"text-author { font-weight: bold; font-style: italic; margin-left: 5%}\n"
"epigraph { margin-left: 25%; margin-right: 1em; text-align: left; text-indent: 1px; font-style: italic; margin-top: 15px; margin-bottom: 25px }\n"
"cite { font-style: italic; margin-left: 5%; margin-right: 5%; text-align: justyfy; margin-top: 20px; margin-bottom: 20px }\n"
"title, h1, h2 { text-align: center; text-indent: 0px; font-weight: bold; hyphenate: none; page-break-before: always; page-break-inside: avoid; page-break-after: avoid; }\n"
"subtitle, h3, h4, h5, h6 { text-align: center; text-indent: 0px; font-weight: bold; hyphenate: none; page-break-inside: avoid; page-break-after: avoid; }\n"
"title { font-size: 110%; margin-top: 0.7em; margin-bottom: 0.5em }\n"
"subtitle { font-style: italic; margin-top: 0.3em; margin-bottom: 0.3em }\n"
"h1 { font-size: 150% }\n"
"h2 { font-size: 140% }\n"
"h3 { font-size: 130% }\n"
"h4 { font-size: 120% }\n"
"h5 { font-size: 110% }\n"
"table { font-size: 80% }\n"
"td, th { text-indent: 0px; padding: 3px }\n"
"th {  font-weight: bold; background-color: #DDD  }\n"
"table > caption { text-indent: 0px; padding: 4px; background-color: #EEE }\n"
"code, pre { display: block; white-space: pre; text-align: left; font-family: \"Courier New\", \"Courier\", monospace; text-align: left }\n"
"description { display: none; }\n"
""
;

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
    //setRect( rect );
    //_clientRect = rect;
    getDocView()->setStyleSheet( lString8(def_view_css) );
    getDocView()->setBackgroundColor(0xFFFFFF);
    getDocView()->setTextColor(0x000000);
    getDocView()->setFontSize( 20 );
    getDocView()->setShowCover( false );
    getDocView()->setPageHeaderInfo( 0 ); // hide title bar
    getDocView()->setPageMargins( lvRect(8,8,8,8) );
    getDocView()->LoadDocument(_stream);
    setRect( rect );
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
        CRLog::trace("drawing scrollbar %d, %d, %d", page, pages, 1);
        sskin->drawScroll( *drawbuf, _scrollRect, false, page, pages, 1 );
    }
    LVDocImageRef pageImage = _docview->getPageImage(0);
    LVDrawBuf * pagedrawbuf = pageImage->getDrawBuf();
    _wm->getScreen()->draw( pagedrawbuf, _clientRect.left, _clientRect.top );
}

void CRViewDialog::setRect( const lvRect & rc )
{
    //if ( rc == _rect )
    //    return;
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
