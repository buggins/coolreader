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
#include "mainwnd.h"

#ifdef WITH_DICT
#include "dictdlg.h"
#endif
#include "viewdlg.h"
#include "scrkbd.h"
#include "numedit.h"
#include "linksdlg.h"
#include "tocdlg.h"
#include <cri18n.h>
#include "selnavig.h"


#ifdef _WIN32
#define DICTD_CONF "C:\\dict\\"
#else
#ifdef CR_USE_JINKE
#define DICTD_CONF "/root/abook/dict"
#else
#define DICTD_CONF "/media/sd/dict"
#endif
#endif

LVRef<CRDictionary> CRViewDialog::_dict;

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
"code, pre { display: block; white-space: pre; text-align: left; font-weight: bold; font-family: \"Courier New\", \"Courier\", monospace; text-align: left }\n"
"description { display: none; }\n"
""
;

CRViewDialog::CRViewDialog(CRGUIWindowManager * wm, lString16 title, lString8 text, lvRect rect, bool showScroll, bool showFrame )
    : CRDocViewWindow(wm), _text(text), _showScroll( showScroll ), _showFrame( showFrame )
{
    _passKeysToParent = _passCommandsToParent = false;
	if ( !_wm->getSkin().isNull() )
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
    getDocView()->setStyleSheet( lString8(def_view_css) );
    getDocView()->setBackgroundColor(0xFFFFFF);
    getDocView()->setTextColor(0x000000);
    getDocView()->setFontSize( 20 );
    getDocView()->setShowCover( false );
    getDocView()->setPageHeaderInfo( 0 ); // hide title bar
    getDocView()->setPageMargins( lvRect(8,8,8,8) );
    setRect( rect );
    if ( !text.empty() ) {
		_stream = LVCreateStringStream( text );
		getDocView()->LoadDocument(_stream);
    }
}

bool CRViewDialog::hasDictionaries()
{
    if ( _dict.isNull() )
        _dict = LVRef<CRDictionary>( new CRTinyDict( Utf8ToUnicode(lString8(DICTD_CONF)) ) );
    if ( !_dict->empty() ) {
    	return true;
    }
	// show warning
	lString8 body;
	body << "<title><p>" << _("No dictionaries found") << "</p></title>";
	body << "<p>" << _("Place dictionaries to directory 'dict' of SD card.") << "</p>";
	body << "<p>" << _("Dictionaries in standard unix .dict format are supported.") << "</p>";
	body << "<p>" << _("For each dictionary, pair of files should be provided: data file (with .dict or .dict.dz extension, and index file with .index extension") << "</p>";
	lString8 xml = CRViewDialog::makeFb2Xml( body );
	CRViewDialog * dlg = new CRViewDialog( _wm, _16("Dictionary"), xml, lvRect(), true, true );
	_wm->activateWindow( dlg );
	return false;
}

void CRViewDialog::showGoToPageDialog()
{
    LVTocItem * toc = _docview->getToc();
    CRNumberEditDialog * dlg;
    if ( toc && toc->getChildCount()>0 ) {
        dlg = new CRTOCDialog( _wm,
            lString16( _("Table of contents") ),
            MCMD_GO_PAGE_APPLY,  _docview->getPageCount(), _docview );
    } else {
        dlg = new CRNumberEditDialog( _wm,
            lString16( _("Enter page number") ),
            lString16(),
            MCMD_GO_PAGE_APPLY, 1, _docview->getPageCount() );
    }
    dlg->setAccelerators( getDialogAccelerators() );
    _wm->activateWindow( dlg );
}

bool CRViewDialog::showLinksDialog()
{
    CRLinksDialog * dlg = CRLinksDialog::create( _wm, this );
    if ( !dlg )
        return false;
    dlg->setAccelerators( getMenuAccelerators() );
    _wm->activateWindow( dlg );
    return true;
}

void CRViewDialog::showSearchDialog()
{
    lvRect rc = _wm->getScreen()->getRect();
    int h_margin = rc.width() / 12;
    int v_margin = rc.height() / 12;
    rc.left += h_margin;
    rc.right -= h_margin;
    rc.bottom -= v_margin;
    rc.top += rc.height() / 2;
    _searchPattern.clear();
    CRScreenKeyboard * dlg = new CRScreenKeyboard( _wm, MCMD_SEARCH_FINDFIRST, _16("Search"), _searchPattern, rc );
    _wm->activateWindow( dlg );
}

bool CRViewDialog::findInDictionary( lString16 pattern )
{
    if ( _dict.isNull() )
        _dict = LVRef<CRDictionary>( new CRTinyDict( Utf8ToUnicode(lString8(DICTD_CONF)) ) );
	lString8 body = _dict->translate( UnicodeToUtf8( pattern ) );
    lString8 txt = CRViewDialog::makeFb2Xml( body );
    CRViewDialog * dlg = new CRViewDialog( _wm, pattern, txt, lvRect(), true, true );
    _wm->activateWindow( dlg );
	return true;
}

bool CRViewDialog::findText( lString16 pattern )
{
    if ( pattern.empty() )
        return false;
    LVArray<ldomWord> words;
    if ( _docview->getDocument()->findText( pattern, true, -1, -1, words, 2000 ) ) {
        _docview->selectWords( words );
        CRSelNavigationDialog * dlg = new CRSelNavigationDialog( _wm, this );
        _wm->activateWindow( dlg );
        return true;
    }
    return false;
}

void CRViewDialog::showDictWithVKeyboard()
{
    lvRect rc = _wm->getScreen()->getRect();
    int h_margin = rc.width() / 12;
    int v_margin = rc.height() / 12;
    rc.left += h_margin;
    rc.right -= h_margin;
    rc.bottom -= v_margin;
    rc.top += rc.height() / 2;
    _searchPattern.clear();
    CRScreenKeyboard * dlg = new CRScreenKeyboard( _wm, MCMD_DICT_FIND, _16("Find in dictionary"), _searchPattern, rc );
    _wm->activateWindow( dlg );
}

bool CRViewDialog::onCommand( int command, int params )
{
    switch ( command ) {
	#ifdef WITH_DICT
		case MCMD_DICT:
			if ( hasDictionaries() )
				showT9Keyboard( _wm, this, MCMD_DICT_FIND, _searchPattern );
			return true;
		case MCMD_DICT_VKEYBOARD:
			if ( hasDictionaries() )
				showDictWithVKeyboard();
			return true;
		case MCMD_DICT_FIND:
			if ( !_searchPattern.empty() && params ) {
				findInDictionary( _searchPattern );
			}
			return true;
	#endif
		case MCMD_SEARCH:
			showSearchDialog();
			return true;
		case MCMD_SEARCH_FINDFIRST:
			if ( !_searchPattern.empty() && params ) {
				findText( _searchPattern );
			}
			return true;
        case MCMD_CANCEL:
        case MCMD_OK:
            _wm->closeWindow( this );
            return true;
        case MCMD_GO_PAGE:
            showGoToPageDialog();
            return true;
        case MCMD_GO_LINK:
            showLinksDialog();
            return true;
    }
    return CRDocViewWindow::onCommand( command, params );
}

void CRViewDialog::draw()
{
	if ( _skin.isNull() )
		return; // skin is not yet loaded
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
