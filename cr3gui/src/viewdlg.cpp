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
#include <cri18n.h>

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

#ifdef CR_POCKETBOOK
#include "cr3pocketbook.h"
#include "inkview.h"
#define DICTD_CONF DIRPREFIX "/mnt/ext1/system/share/cr3/dict"
#define DICTD_CONF_ALT "/mnt/ext2/system/share/cr3/dict"
#elif _WIN32
#define DICTD_CONF "C:\\dict\\"
#define DICTD_CONF_ALT ".\\dict\\"
#else
#ifdef CR_USE_JINKE
#define DICTD_CONF "/root/abook/dict"
#define DICTD_CONF_ALT "/root/crengine/dict"
#else
#define DICTD_CONF "/media/sd/dict"
#define DICTD_CONF_ALT "/usr/share/cr3/dict"
#endif
#endif


#define USE_SEPARATE_GO_TO_PAGE_DIALOG 0

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
"image { text-align: center; text-indent: 0px; display: block }\n"
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
"th {  font-size: 120%; font-weight: bold; background-color: #DDD  }\n"
"table > caption { text-indent: 0px; padding: 4px; background-color: #EEE }\n"
"code, pre { display: block; white-space: pre; text-align: left; font-weight: bold; font-family: \"Courier New\", \"Courier\", monospace; text-align: left }\n"
"description { display: none; }\n"
""
;

CRViewDialog::CRViewDialog(CRGUIWindowManager * wm, lString16 title, lString8 text, lvRect rect, bool showScroll, bool showFrame )
    : CRDocViewWindow(wm), _text(text), _showScroll( showScroll ), _showFrame( showFrame ), _lastNavigationDirection(0)
{
    _passKeysToParent = _passCommandsToParent = false;
    if ( _showFrame ) {
        setSkinName( lString16("#dialog") );
        if ( !_wm->getSkin().isNull() )
            _skin = _wm->getSkin()->getWindowSkin(getSkinName().c_str());
    }
    setAccelerators( _wm->getAccTables().get("browse") );
    _caption = title;
    lvRect fsRect = _wm->getScreen()->getRect();
    _fullscreen = false;
    if ( rect.isEmpty() ) {
        rect = fsRect;
        _fullscreen = false;
    } else {
        _fullscreen = (rect == fsRect);
    }
    getDocView()->setDrawBufferBits(_wm->getScreen()->getCanvas()->GetBitsPerPixel());
    getDocView()->setStyleSheet( lString8(def_view_css) );
    getDocView()->setBackgroundColor(0xFFFFFF);
    getDocView()->setTextColor(0x000000);
    getDocView()->setFontSize( 20 );
    getDocView()->setPageMargins( lvRect(8,8,8,8) );
    if ( !_skin.isNull() ) {
        CRRectSkinRef clientSkin = _skin->getClientSkin();
        if ( !clientSkin.isNull() ) {
            getDocView()->setBackgroundColor(clientSkin->getBackgroundColor());
            getDocView()->setTextColor(clientSkin->getTextColor());
            getDocView()->setFontSize(clientSkin->getFontSize());
            getDocView()->setDefaultFontFace(UnicodeToUtf8(clientSkin->getFontFace()));
            getDocView()->setPageMargins( clientSkin->getBorderWidths() );
        }
    }
    getDocView()->setShowCover( false );
    getDocView()->setPageHeaderInfo( 0 ); // hide title bar
    if ( !text.empty() ) {
		_stream = LVCreateStringStream( text );
		getDocView()->LoadDocument(_stream);
    }
    setRect( rect );
}

bool CRViewDialog::hasDictionaries()
{

    const char * dict_path = DICTD_CONF;
    if ( !LVDirectoryExists(lString16(dict_path)) )
        dict_path = DICTD_CONF_ALT;
    if ( _dict.isNull() )
        _dict = LVRef<CRDictionary>( new CRTinyDict( Utf8ToUnicode(lString8(dict_path)) ) );
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
	CRViewDialog * dlg = new CRViewDialog( _wm, lString16(_("Dictionary")), xml, lvRect(), true, true );
	_wm->activateWindow( dlg );
	return false;
}

void CRViewDialog::showGoToPageDialog()
{
    LVTocItem * toc = _docview->getToc();
    CRNumberEditDialog * dlg;
#if USE_SEPARATE_GO_TO_PAGE_DIALOG==1
    if ( toc && toc->getChildCount()>0 ) {
#endif
        dlg = new CRTOCDialog( _wm,
            lString16( _("Table of contents") ),
            MCMD_GO_PAGE_APPLY,  _docview->getPageCount(), _docview );
#if USE_SEPARATE_GO_TO_PAGE_DIALOG==1
    } else {
        dlg = new CRNumberEditDialog( _wm,
            lString16( _("Enter page number") ),
            lString16::empty_str,
            MCMD_GO_PAGE_APPLY, 1, _docview->getPageCount() );
    }
#endif
    dlg->setAccelerators( getDialogAccelerators() );
    _wm->activateWindow( dlg );
}

void CRViewDialog::showGoToPercentDialog()
{
    LVTocItem * toc = _docview->getToc();
    CRNumberEditDialog * dlg;
    dlg = new CRNumberEditDialog( _wm,
        lString16( _("Enter position percent") ),
        lString16::empty_str,
        MCMD_GO_PERCENT_APPLY, 0, 100 );
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
    CRScreenKeyboard * dlg = new CRScreenKeyboard( _wm, MCMD_SEARCH_FINDFIRST, lString16(_("Search")), _searchPattern, rc );
    _wm->activateWindow( dlg );
}

#define EXTERNAL_DICTIONARY_EXECUTABLE_NAME "edict_launcher"

bool CRViewDialog::findInDictionary( lString16 pattern )
{
#if USE_EXTERNAL_EDICT_DICTIONARY==1
    // external dictionary
    lString8 cmd;
    cmd << EXTERNAL_DICTIONARY_EXECUTABLE_NAME;
    cmd << " \"";
    cmd << UnicodeToUtf8(pattern);
    cmd << "\" &";
    CRLog::info("Calling system %s", cmd.c_str());
    int res = system(cmd.c_str());
    return res!=-1;
#else
    if ( _dict.isNull() ) {
        showWaitIcon();
        _dict = LVRef<CRDictionary>( new CRTinyDict( Utf8ToUnicode(lString8(DICTD_CONF)) ) );
    }
	lString8 body = _dict->translate( UnicodeToUtf8( pattern ) );
    lString8 txt = CRViewDialog::makeFb2Xml( body );
    CRViewDialog * dlg = new CRViewDialog( _wm, pattern, txt, lvRect(), true, true );
    _wm->activateWindow( dlg );
	return true;
#endif
}

bool CRViewDialog::findText( lString16 pattern, int origin, int direction )
{
    if ( pattern.empty() )
        return false;
    LVArray<ldomWord> words;
    showWaitIcon();
    lvRect rc;
    _docview->GetPos( rc );
    int pageHeight = rc.height();
    int start = -1;
    int end = -1;
    bool reverse = direction<0;
    if ( reverse ) {
        // reverse
        if ( origin == 0 ) {
            // from end current page to first page
            end = rc.bottom;
        } else if ( origin == -1 ) {
            // from last page to end of current page
            start = rc.bottom;
        } else { // origin == 1
            // from prev page to first page
            end = rc.top;
        }
    } else {
        // forward
        if ( origin == 0 ) {
            // from current page to last page
            start = rc.top;
        } else if ( origin == -1 ) {
            // from first page to current page
            end = rc.top;
        } else { // origin == 1
            // from next page to last
            start = rc.bottom;
        }
    }
    CRLog::debug("CRViewDialog::findText: Current page: %d .. %d", rc.top, rc.bottom);
    CRLog::debug("CRViewDialog::findText: searching for text '%s' from %d to %d origin %d", LCSTR(pattern), start, end, origin );
    if ( _docview->getDocument()->findText( pattern, true, reverse, start, end, words, 200, pageHeight ) ) {
        CRLog::debug("CRViewDialog::findText: pattern found");
        _docview->clearSelection();
        _docview->selectWords( words );
        return true;
    }
    CRLog::debug("CRViewDialog::findText: pattern not found");
    return false;
}

int CRViewDialog::findPagesText( lString16 pattern, int origin, int direction )
{
    if ( pattern.empty() )
        return -1;
	if (!(origin == 0 || origin == -1 || origin == 1 || direction == 1 || direction == -1))
		return -1;
    showWaitIcon();
	int start, end;
	if (direction < 0) {
		//reverse search
		if (origin >= 0) {
			start = _docview->getCurPage() -origin;
			end = -1;
		} else {
			start = _docview->getPageCount() -1;
			end = _docview->getCurPage();
		}
	} else {
		// direct search
		if (origin >= 0) {
			start = _docview->getCurPage() + origin;
			end = _docview->getPageCount();
		} else {
			start = 0;
			end = _docview->getCurPage();
		}
	}
	LVArray<ldomWord> words;
	for (int i = start; i != end; i += direction) {
		LVRef<ldomXRange> range = _docview->getPageDocumentRange( i );
		if (!range.isNull()) {
			if (range->findText(pattern, true, false, words, 200, _docview->getPageHeight(i), true)) {
				CRLog::debug("CRViewDialog::findPagesText: pattern found");
				_docview->clearSelection();
				_docview->goToPage(i);
				_docview->selectWords( words );
				return i;
			}
		}
	}
    CRLog::debug("CRViewDialog::findPagesText: pattern not found");
	return -1;
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
    CRScreenKeyboard * dlg = new CRScreenKeyboard( _wm, MCMD_DICT_FIND, lString16(_("Find in dictionary")), _searchPattern, rc );
    _wm->activateWindow( dlg );
}

bool CRViewDialog::onCommand( int command, int params )
{
    _lastNavigationDirection = 0;
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
                if ( findText( _searchPattern, 0, 1 ) || findText( _searchPattern, -1, 1 )) {
                    CRSelNavigationDialog * dlg = new CRSelNavigationDialog( _wm, this, _searchPattern );
                    _wm->activateWindow( dlg );
                }
            }
			return true;
        case MCMD_CANCEL:
        case MCMD_OK:
            _wm->closeWindow( this );
            return true;
        case MCMD_GO_PAGE:
            showGoToPageDialog();
            return true;
        case MCMD_GO_PERCENT:
            showGoToPercentDialog();
            return true;
        case MCMD_GO_LINK:
            showLinksDialog();
            return true;
		case MCMD_HELP_KEYS:
			showKeymapDialog();
			break;
        case DCMD_PAGEDOWN:
            if ( params==0 || params==1 )
                _lastNavigationDirection = 1;
            break;
        case DCMD_PAGEUP:
            if ( params==0 || params==1 )
                _lastNavigationDirection = -1;
            break;
    }

    return CRDocViewWindow::onCommand( command, params );
}

static 
const char * getKeyName( int keyCode )
{
	static char name[256];
	if ( (keyCode>='0' && keyCode<='9') || keyCode=='+' || keyCode=='-' ) {
		sprintf( name, "'%c'", keyCode );
		return name;
	}
	switch ( keyCode ) {
#ifdef CR_POCKETBOOK
// FIXME: It would be more correct to map KEY_* to XK_* but PB people 
// already got used to these key definitions
	case KEY_BACK:
		return TR("@Key_back");
	case KEY_DELETE:
		return TR("@Key_delete");
	case KEY_OK:
		return TR("@Key_ok");
	case KEY_UP:
		return TR("@Key_up");
	case KEY_DOWN:
		return TR("@Key_down");
	case KEY_LEFT:
		return TR("@Key_left");
	case KEY_RIGHT:
		return TR("@Key_right");
	case KEY_MINUS:
		return TR("@Key_minus");
	case KEY_PLUS:
		return TR("@Key_plus");
	case KEY_MENU:
		return TR("@Key_plus");
	case KEY_MUSIC:
		return TR("@Key_music");
	case KEY_POWER:
		return TR("@Key_power");
	case KEY_PREV:
		return TR("@Key_prev");
	case KEY_NEXT:
		return TR("@Key_next");
	case KEY_PREV2:
		return TR("@Key_prev2");
	case KEY_NEXT2:
		return TR("@Key_next2");
#endif
	case XK_KP_Add:
		return "'+'";
	case XK_KP_Subtract:
		return "'-'";
    case ' ':
        return _("M");
    case XK_Return:
        return _("Ok");
	case XK_Up:
        return _("Up");
	case XK_Down:
        return _("Down");
	case XK_Escape:
        return _("C");
	case XK_Left:
        return _("Left");
	case XK_Right:
        return _("Right");
	case XK_Prior:
        return _("Side Up");
	case XK_Next:
        return _("Side Down");
	case XK_KP_Enter:
        return _("Side Press");
	case XK_Menu:
		return _("Menu");
    case XF86XK_Search:
        return _("Zoom");
    default:
		return "?";
	}
}

const char * getKeyName( int keyCode, int option )
{
	if ( !option )
		return ::getKeyName( keyCode );
	static char name[256];
	sprintf(name, "%s %s", _("Long"), ::getKeyName( keyCode ) );
	return name;
}

static const char * getCommandName( int command )
{
	switch ( command ) {
	case DCMD_BEGIN: return _("To first page");
	case DCMD_LINEUP:
	case DCMD_PAGEUP: return _("Back by page");
	case DCMD_PAGEDOWN:
	case DCMD_LINEDOWN: return _("Forward by page");
	case DCMD_LINK_FORWARD: return _("Forward in move history");
	case DCMD_LINK_BACK: return _("Back in move history");
	case DCMD_LINK_NEXT: return _("Next link");
	case DCMD_LINK_PREV: return _("Previous link");
	case DCMD_LINK_GO: return _("Go to link");
	case DCMD_END: return _("To last page");
	case DCMD_GO_POS: return _("Go to position");
	case DCMD_GO_PAGE: return _("Go to page");
	case DCMD_ZOOM_IN: return _("Zoom in");
	case DCMD_ZOOM_OUT: return _("Zoom out");
    case DCMD_TOGGLE_BOLD: return _("Toggle bold text");
	case DCMD_TOGGLE_TEXT_FORMAT: return _("Toggle text format");
	case DCMD_BOOKMARK_SAVE_N: return _("Save bookmark by number");
	case DCMD_BOOKMARK_GO_N: return _("Go to bookmark by number");
	case DCMD_MOVE_BY_CHAPTER: return _("Move by chapter");
    case DCMD_LINK_FIRST: return _("First link");
    case DCMD_ROTATE_BY: return _("Rotate by");
    case DCMD_ROTATE_SET: return _("Set rotate angle");
    case DCMD_SAVE_HISTORY: return _("Save history and bookmarks");
    case DCMD_SAVE_TO_CACHE: return _("Save document to cache");
    case MCMD_CANCEL: return _("Close dialog");
	case MCMD_OK: return ("Ok");
	case MCMD_SELECT: return ("Ok");
	case MCMD_SCROLL_FORWARD: return _("Scroll forward");
	case MCMD_SCROLL_BACK: return _("Scroll back");
	case MCMD_QUIT: return _("Close book");
	case MCMD_MAIN_MENU: return _("Main menu");
	case MCMD_GO_PAGE: return _("Go to page dialog");
	case MCMD_SETTINGS: return _("Settings menu");
	case MCMD_SETTINGS_FONTSIZE: return _("Font size settings");
	case MCMD_SETTINGS_ORIENTATION: return _("Page orientation settings");
	case MCMD_GO_LINK: return _("Go to link");
#ifdef CR_POCKETBOOK
	case MCMD_DICT: return TR("@Dictionary");
#else
	case MCMD_DICT: return _("Find in Dictionary (T5)");
#endif	
	case MCMD_BOOKMARK_LIST: return _("Bookmark list");
        case MCMD_CITES_LIST: return _("Citations list");
    case MCMD_BOOKMARK_LIST_GO_MODE: return _("Go to bookmark...");
	case MCMD_RECENT_BOOK_LIST: return _("Recent books list");
	case MCMD_OPEN_RECENT_BOOK: return _("Open recent book by number");
    case MCMD_SWITCH_TO_RECENT_BOOK: return _("Switch to recent book");
    case MCMD_ABOUT: return _("About");
	case MCMD_CITE: return _("Cite selection dialog");
	case MCMD_SEARCH: return _("Search dialog");
	case MCMD_DICT_VKEYBOARD: return _("Find in Dictionary (virtual keyboard)");
	case MCMD_KBD_NEXTLAYOUT: return _("Next keyboard layout");
	case MCMD_KBD_PREVLAYOUT: return _("Previous keyboard layout");
	case MCMD_HELP: return _("Show manual");
	case MCMD_HELP_KEYS: return _("Show key mapping");
    case MCMD_GO_PERCENT: return _("Go to percent...");
#ifdef CR_POCKETBOOK
	case PB_QUICK_MENU: return TR("@KA_menu");
	case PB_CMD_ROTATE: return TR("@KA_rtte");
	case PB_CMD_CONTENTS: return TR("@KA_cnts");
        case PB_CMD_MAIN_MENU: return _("PocketBook Main menu");
#endif
    default: return _("Unknown command");
	}
}

const char * getCommandName( int command, int param )
{
    if ( command==DCMD_PAGEUP && param == 0 )
        return _("Previous Page");
    if ( command==DCMD_PAGEDOWN && param == 0 )
        return _("Next Page");
    if ( command==DCMD_PAGEUP && param == 10 )
        return _("Back by 10 Pages");
    if ( command==DCMD_PAGEDOWN && param == 10 )
        return _("Forward by 10 Pages");
    if ( command==DCMD_MOVE_BY_CHAPTER && param == 1 )
        return _("Next Chapter");
    if ( command==DCMD_MOVE_BY_CHAPTER && param == -1 )
        return _("Previous Chapter");
    if ( command==DCMD_BOOKMARK_SAVE_N && param == 0 )
        return _("Add Bookmark");
    if ( command==DCMD_ROTATE_BY && param == 1 )
        return _("Rotate Clockwise");
    if ( command==DCMD_ROTATE_BY && param == -1 )
        return _("Rotate Anti-Clockwise");
    if ( command==DCMD_ROTATE_SET && param == 0 )
        return _("Portrait View");
    if ( command==DCMD_ROTATE_SET && param == 1 )
        return _("Landscape View");
#ifdef CR_POCKETBOOK
	if ( command == PB_CMD_MP3 && param == 1)
		return TR("@KA_mp3o");
	if ( command == PB_CMD_MP3 && param == 0)
		return TR("@KA_mp3p");	
	if ( command == PB_CMD_VOLUME && param == 3)
		return TR("@KA_volp");
	if ( command == PB_CMD_VOLUME && param == 3)
		return TR("@KA_volm");
#endif
    if ( !param )
		return ::getCommandName( command );
	static char buf[ 256 ];
	sprintf(buf, "%s (%d)", ::getCommandName( command ), param);
	return buf;
}

void CRViewDialog::showKeymapDialog()
{
	if ( _acceleratorTable.isNull() )
		return;
	lString8 txt;
	txt << "<table><tr><th>";
	txt << "<b>" << _("Key") << "</b></th><th><b>"<< _("Assigned function") <<"</b></th></tr>";
    for ( int i=0; i<_acceleratorTable->length(); i++ ) {
		const CRGUIAccelerator * acc = _acceleratorTable->get( i );
		txt << "<tr><td>";
		txt << getKeyName( acc->keyCode, acc->keyFlags );
		txt << "</td><td>";
		txt << getCommandName( acc->commandId, acc->commandParam );
		txt << "</td></tr>";
	}
	txt << "</table>";
	//============================================================
    txt = CRViewDialog::makeFb2Xml(txt);
    CRViewDialog * dlg = new CRViewDialog( _wm, lString16(_("Keyboard layout")), txt, lvRect(), true, true );
    dlg->getDocView()->setVisiblePageCount(1);
    _wm->activateWindow( dlg );
    //TODO:
}

void CRViewDialog::draw()
{
    draw(0);
}

void CRViewDialog::draw( int pageOffset )
{
    //if ( _skin.isNull() )
    //	return; // skin is not yet loaded
    _pages = _docview->getPageCount();
    _page = _docview->getCurPage() + pageOffset + 1;
    if ( _page<1 )
        _page = 1;
    if ( _page>_pages )
        _page = _pages;
    lvRect clientRect = _rect;
    lvRect borders;
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    if ( _showFrame && !_skin.isNull() ) {

        //CRRectSkinRef titleSkin = _skin->getTitleSkin();
        CRRectSkinRef clientSkin = _skin->getClientSkin();
        if ( !clientSkin.isNull() )
            borders = clientSkin->getBorderWidths();
        getClientRect( clientRect );
        if ( !clientSkin.isNull() )
            clientSkin->draw( *drawbuf, clientRect );
        _skin->draw( *drawbuf, _rect );
    }
    if ( _showFrame ) {
        drawTitleBar();
        drawStatusBar();
    }
    LVDocImageRef pageImage = _docview->getPageImage( pageOffset );
    if ( !pageImage.isNull() ) {
        LVDrawBuf * pagedrawbuf = pageImage->getDrawBuf();
        _wm->getScreen()->draw( pagedrawbuf, clientRect.left, clientRect.top );
    }
}

void CRViewDialog::setRect( const lvRect & rc )
{
    //if ( rc == _rect )
    //    return;
    CRLog::info("CRViewDialog::setRect(%d,%d,%d,%d)", rc.left, rc.top, rc.right, rc.bottom);
    _rect = rc;
    lvRect clientRect = _rect;
    if ( _showFrame ) {
        if ( !_skin.isNull() )
            getClientRect(clientRect);
    }
    _docview->Resize( clientRect.width(), clientRect.height() );
    _docview->checkRender();
    _pages = _docview->getPageCount();
    if ( !_skin.isNull() )
        getClientRect(clientRect);
    _docview->Resize( clientRect.width(), clientRect.height() );
    setDirty();
}

void CRViewDialog::prepareNextPageImage( int offset )
{
    if ( _wm->getScreen()->getTurboUpdateEnabled() && _wm->getScreen()->getTurboUpdateSupported() ) {
        CRLog::debug("CRViewDialog::prepareNextPageImage() in turbo mode");
        _wm->getScreen()->setTurboUpdateMode( CRGUIScreen::PrepareMode );
        draw(offset);
        _wm->getScreen()->flush(true);
        _wm->getScreen()->setTurboUpdateMode( CRGUIScreen::NormalMode );
        //_docview->getDocument()->swapToCacheIfNecessary();
    } else {
        CRLog::debug("CRViewDialog::prepareNextPageImage() in normal mode");
        LVDocImageRef pageImage = _docview->getPageImage(offset);
        //_docview->getDocument()->swapToCacheIfNecessary();
    }
}
