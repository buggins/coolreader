//
// C++ Implementation: settings
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "mainwnd.h"

#ifdef _WIN32
#define DICTD_CONF "C:\\dict\\"
#else
#ifdef CR_USE_JINKE
#define DICTD_CONF "/root/crengine/dict/"
#else
#define DICTD_CONF "/media/sd/dict"
#endif
#endif

#include <cri18n.h>

#ifdef WITH_DICT
#include "dictdlg.h"
#endif
#include "numedit.h"
#include "linksdlg.h"
#include "tocdlg.h"
#include "bmkdlg.h"
#include "recentdlg.h"
#include "viewdlg.h"
#include "scrkbd.h"
#include "selnavig.h"

#include "citedlg.h"





DECL_DEF_CR_FONT_SIZES;

const char * cr_default_skin =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<CR3Skin>\n"
"  <menu id=\"main\">\n"
"        <text color=\"#000000\" face=\"Arial\" size=\"25\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"        <background color=\"#AAAAAA\"/>\n"
"        <border widths=\"0,8,8,8\"/>\n"
"        <!--icon image=\"filename\" valign=\"\" halign=\"\"/-->\n"
"        <title>\n"
"            <size minvalue=\"32,0\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"25\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background color=\"#AAAAAA\"/>\n"
"            <border widths=\"4,4,4,4\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </title>\n"
"        <item>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"left\"/>\n"
"            <background image=\"std_menu_item_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </item>\n"
"        <shortcut>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background image=\"std_menu_shortcut_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </shortcut>\n"
"  </menu>\n"
"  <menu id=\"settings\">\n"
"        <text color=\"#000000\" face=\"Arial\" size=\"25\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"        <background color=\"#AAAAAA\"/>\n"
"        <border widths=\"8,8,8,8\"/>\n"
"        <!--icon image=\"filename\" valign=\"\" halign=\"\"/-->\n"
"        <title>\n"
"            <size minvalue=\"0,40\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"28\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background color=\"#AAAAAA\"/>\n"
"            <border widths=\"4,4,4,4\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </title>\n"
"        <item>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"left\"/>\n"
"            <background image=\"std_menu_item_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </item>\n"
"        <shortcut>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background image=\"std_menu_shortcut_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </shortcut>\n"
"  </menu>\n"
"</CR3Skin>\n";


bool V3DocViewWin::loadSkin( lString16 pathname )
{
    CRSkinRef skin;
    if ( !pathname.empty() )
        skin = LVOpenSkin( pathname );
    if ( skin.isNull() ) {
        skin = LVOpenSimpleSkin( lString8( cr_default_skin ) );
        _wm->setSkin( skin );
        return false;
    }
    _wm->setSkin( skin );
    return true;
}

V3DocViewWin::V3DocViewWin( CRGUIWindowManager * wm, lString16 dataDir )
: CRDocViewWindow ( wm ), _dataDir(dataDir) //defT9encoding)
{
    CRLog::trace("V3DocViewWin()");
    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
    _docview->setFontSizes( sizes, true );
    _props = LVCreatePropsContainer();
    _newProps = _props;
    // TODO: move skin outside
    lString16 skinfile = _dataDir;
    LVAppendPathDelimiter( skinfile );
    skinfile << L"skin";
    lString8 s8 = UnicodeToLocal( skinfile );
    CRLog::debug("Skin file is %s", s8.c_str() );
    loadSkin( skinfile );


    LVRefVec<LVImageSource> icons;
    static const char * battery4[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0000.000.000.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery3[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0000.ooo.000.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery2[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0000.ooo.ooo.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery1[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0000.ooo.ooo.ooo.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery0[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0000.ooo.ooo.ooo.ooo.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    icons.add( LVCreateXPMImageSource( battery0 ) );
    icons.add( LVCreateXPMImageSource( battery1 ) );
    icons.add( LVCreateXPMImageSource( battery2 ) );
    icons.add( LVCreateXPMImageSource( battery3 ) );
    icons.add( LVCreateXPMImageSource( battery4 ) );
    _docview->setBatteryIcons( icons );

    setAccelerators( _wm->getAccTables().get("main") );
}

bool V3DocViewWin::loadDefaultCover( lString16 filename )
{
    LVImageSourceRef cover = LVCreateFileCopyImageSource( filename.c_str() );
    if ( !cover.isNull() ) {
        _docview->setDefaultCover( cover );
        return true;
    } else {
        IMAGE_SOURCE_FROM_BYTES(defCover, cr3_def_cover_gif);
        _docview->setDefaultCover( defCover );
        return false;
    }
}

bool V3DocViewWin::loadCSS( lString16 filename )
{
    lString8 css;
    if ( LVLoadStylesheetFile( filename, css ) ) {
        if ( !css.empty() ) {
            _docview->setStyleSheet( css );
            _css = css;
            return true;
        }
    }
    return false;
}

bool V3DocViewWin::loadDictConfig( lString16 filename )
{
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( !stream.isNull() ) {
        _dictConfig = filename;
        return true;
    }
    return false;
}

bool V3DocViewWin::loadHistory( lString16 filename )
{
    _historyFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
        return false;
    }
    if ( !_docview->getHistory()->loadFromStream( stream ) )
        return false;
    return true;
}

void V3DocViewWin::closing()
{
    _docview->savePosition();
    saveHistory( lString16() );
}

bool V3DocViewWin::loadDocument( lString16 filename )
{
    if ( !_docview->LoadDocument( filename.c_str() ) )
        return false;
    _docview->restorePosition();
    return true;
}

bool V3DocViewWin::saveHistory( lString16 filename )
{
    crtrace log;
    if ( filename.empty() )
        filename = _historyFileName;
    if ( filename.empty() )
        return false;
	_docview->exportBookmarks(lString16());//use default filename
    _historyFileName = filename;
    log << "V3DocViewWin::saveHistory(" << filename << ")";
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
    if ( !stream ) {
        lString16 path16 = LVExtractPath( filename );
        lString8 path = UnicodeToLocal( path16 );
#ifdef _WIN32
        if ( !CreateDirectoryW( path16.c_str(), NULL ) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#else
        path.erase( path.length()-1, 1 );
        CRLog::warn("Cannot create settings file, trying to create directory %s", path.c_str());
        if ( mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#endif
    }
    if ( stream.isNull() ) {
        lString8 fn = UnicodeToUtf8( filename );
        CRLog::error("Cannot open history file %s for write", fn.c_str() );
        return false;
    }
    return _docview->getHistory()->saveToStream( stream.get() );
}

void V3DocViewWin::flush()
{
    // override to update battery
    int charge = 0;
    bool charging = false;
    if ( _wm->getBatteryStatus( charge, charging ) ) {
        if ( _docview->getBatteryState() != charge )
            _docview->setBatteryState( charge );
    }
    CRDocViewWindow::flush();
}

bool V3DocViewWin::loadSettings( lString16 filename )
{
    _settingsFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
        _docview->propsUpdateDefaults( _props );
        _docview->propsApply( _props );
        return false;
    }
    if ( _props->loadFromStream( stream.get() ) ) {
        _docview->propsUpdateDefaults( _props );
        _docview->propsApply( _props );
        return true;
    }
    _docview->propsUpdateDefaults( _props );
    _docview->propsApply( _props );
    return false;
}

bool V3DocViewWin::saveSettings( lString16 filename )
{
    crtrace log;
    if ( filename.empty() )
        filename = _settingsFileName;
    if ( filename.empty() )
        return false;
    _settingsFileName = filename;
    log << "V3DocViewWin::saveSettings(" << filename << ")";
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
    if ( !stream ) {
        lString16 path16 = LVExtractPath( filename );
        lString8 path = UnicodeToLocal( path16 );
#ifdef _WIN32
        if ( !CreateDirectoryW( path16.c_str(), NULL ) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#else
        path.erase( path.length()-1, 1 );
        CRLog::warn("Cannot create settings file, trying to create directory %s", path.c_str());
        if ( mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#endif
    }
    if ( stream.isNull() ) {
        lString8 fn = UnicodeToUtf8( filename );
        CRLog::error("Cannot open settings file %s for write", fn.c_str() );
        return false;
    }
    return _props->saveToStream( stream.get() );
}

void V3DocViewWin::applySettings()
{
    CRPropRef delta = _props ^ _newProps;
    CRLog::trace( "applySettings() - %d options changed", delta->getCount() );
    _docview->propsApply( delta );
    _props = _newProps; // | _props;
}

void V3DocViewWin::showSettingsMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    CRMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, getMenuAccelerators() );
    _wm->activateWindow( mainMenu );
}

void V3DocViewWin::showFontSizeMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    CRSettingsMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, getMenuAccelerators() );
    CRMenu * menu = mainMenu->createFontSizeMenu( NULL, _newProps );
    _wm->activateWindow( menu );
}

void V3DocViewWin::showOrientationMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    CRSettingsMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, getMenuAccelerators() );
    CRMenu * menu = mainMenu->createOrientationMenu( NULL, _newProps );
    _wm->activateWindow( menu );
}

void V3DocViewWin::showRecentBooksMenu()
{
    lvRect rc = _wm->getScreen()->getRect();
    CRRecentBooksMenu * menu_win = new CRRecentBooksMenu(_wm, _docview, 8, rc);
    menu_win->setAccelerators( getMenuAccelerators() );
    menu_win->setSkinName(lString16(L"#bookmarks"));
    _wm->activateWindow( menu_win );
}

void V3DocViewWin::openRecentBook( int index )
{
    LVPtrVector<CRFileHistRecord> & files = _docview->getHistory()->getRecords();
    if ( index >= 1 && index < files.length() ) {
        CRFileHistRecord * file = files.get( index );
        lString16 fn = file->getFilePathName();
        // TODO: check error
        loadDocument( fn );
    }
}

void V3DocViewWin::showBookmarksMenu()
{
    lvRect rc = _wm->getScreen()->getRect();
    CRBookmarkMenu * menu_win = new CRBookmarkMenu(_wm, _docview, 8, rc);
    menu_win->setAccelerators( getMenuAccelerators() );
    menu_win->setSkinName(lString16(L"#bookmarks"));
    _wm->activateWindow( menu_win );
}

void V3DocViewWin::showMainMenu()
{
    CRMenu * menu_win = new CRMenu( _wm,
        NULL, //CRMenu * parentMenu,
        1,
        lString16(_("Main Menu")),
        LVImageSourceRef(),
        LVFontRef(),
        LVFontRef() );
/*
VIEWER_MENU_GOTOFIRSTPAGE=Go to first page
VIEWER_MENU_GOTOENDPAGE=Go to last page
VIEWER_MENU_GOTOPAGE=Go to page...
VIEWER_MENU_GOTOINDEX=Go to index
VIEWER_MENU_5ABOUT=About...
VIEWER_MENU_4ABOUT=About...
*/
    menu_win->setSkinName(lString16(L"#main"));
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_ABOUT,
                _("About..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#if 0
    menu_win->addItem( new CRMenuItem( menu_win, DCMD_BEGIN,
                _wm->translateString("VIEWER_MENU_GOTOFIRSTPAGE", "Go to first page"),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_GO_PAGE,
                _("Go to page ..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#if 0
    menu_win->addItem( new CRMenuItem( menu_win, DCMD_END,
                _wm->translateString("VIEWER_MENU_GOTOENDPAGE", "Go to last page"),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_RECENT_BOOK_LIST,
                _("Open recent book..."),
                LVImageSourceRef(),
                LVFontRef() ) );

#ifdef WITH_DICT
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_DICT,
                _("Dictionary..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_CITE,
                _("Cite.."),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_BOOKMARK_LIST,
                _("Bookmarks..."),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_SEARCH,
                _("Search..."),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_SETTINGS,
                _("Settings..."),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->setAccelerators( getMenuAccelerators() );
    _wm->activateWindow( menu_win );
}

void V3DocViewWin::showGoToPageDialog()
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

void V3DocViewWin::showSearchDialog()
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

void V3DocViewWin::showDictWithVKeyboard()
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

bool V3DocViewWin::showLinksDialog()
{
    CRLinksDialog * dlg = CRLinksDialog::create( _wm, this );
    if ( !dlg )
        return false;
    dlg->setAccelerators( getMenuAccelerators() );
    _wm->activateWindow( dlg );
    return true;
}

void addPropLine( lString8 & buf, const char * description, const lString16 & value )
{
    if ( !value.empty() ) {
        buf << "<tr><td>" << description << "</td><td>" << UnicodeToUtf8(value).c_str() << "</td></tr>\n";
    }
}

lString16 getDocText( ldomDocument * doc, const char * path, const char * delim )
{
    lString16 res;
    for ( int i=0; i<100; i++ ) {
        lString8 p = lString8(path) + "[" + lString8::itoa(i+1) + "]";
        //CRLog::trace("checking doc path %s", p.c_str() );
        lString16 p16 = Utf8ToUnicode(p);
        ldomXPointer ptr = doc->createXPointer( p16 );
        if ( ptr.isNull() )
            break;
        lString16 s = ptr.getText( L' ' );
        if ( s.empty() )
            continue;
        if ( !res.empty() && delim!=NULL )
            res << Utf8ToUnicode( lString8( delim ) );
        res << s;
    }
    return res;
}

lString16 getDocAuthors( ldomDocument * doc, const char * path, const char * delim )
{
    lString16 res;
    for ( int i=0; i<100; i++ ) {
        lString8 p = lString8(path) + "[" + lString8::itoa(i+1) + "]";
        //CRLog::trace("checking doc path %s", p.c_str() );
        lString16 firstName = getDocText( doc, (p + "/first-name").c_str(), " " );
        lString16 lastName = getDocText( doc, (p + "/last-name").c_str(), " " );
        lString16 middleName = getDocText( doc, (p + "/middle-name").c_str(), " " );
        lString16 nickName = getDocText( doc, (p + "/nickname").c_str(), " " );
        lString16 homePage = getDocText( doc, (p + "/homepage").c_str(), " " );
        lString16 email = getDocText( doc, (p + "/email").c_str(), " " );
        lString16 s = firstName;
        if ( !middleName.empty() )
            s << L" " << middleName;
        if ( !lastName.empty() ) {
            if ( !s.empty() )
                s << L" ";
            s << lastName;
        }
        if ( !nickName.empty() ) {
            if ( !s.empty() )
                s << L" ";
            s << nickName;
        }
        if ( !homePage.empty() ) {
            if ( !s.empty() )
                s << L" ";
            s << homePage;
        }
        if ( !email.empty() ) {
            if ( !s.empty() )
                s << L" ";
            s << email;
        }
        if ( s.empty() )
            continue;
        if ( !res.empty() && delim!=NULL )
            res << Utf8ToUnicode( lString8( delim ) );
        res << s;
    }
    return res;
}


void V3DocViewWin::showAboutDialog()
{
    lString16 title = L"Cool Reader ";
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "3.0"
#endif
    title << Utf8ToUnicode(lString8(PACKAGE_VERSION));

    lString8 txt;
    //=========================================================
    txt << "<table><col width=\"25%\"/><col width=\"75%\"/>\n";
    CRPropRef props = _docview->getDocProps();
    txt << "<tr><td colspan=\"2\" style=\"font-weight: bold; text-align: center\">File info</td></tr>";
    addPropLine( txt, _("Archive name"), props->getStringDef(DOC_PROP_ARC_NAME) );
    addPropLine( txt, _("Archive path"), props->getStringDef(DOC_PROP_ARC_PATH) );
    addPropLine( txt, _("Archive size"), props->getStringDef(DOC_PROP_ARC_SIZE) );
    addPropLine( txt, _("File name"), props->getStringDef(DOC_PROP_FILE_NAME) );
    addPropLine( txt, _("File path"), props->getStringDef(DOC_PROP_FILE_PATH) );
    addPropLine( txt, _("File size"), props->getStringDef(DOC_PROP_FILE_SIZE) );
    addPropLine( txt, _("File format"), props->getStringDef(DOC_PROP_FILE_FORMAT) );

    lString8 bookInfo;
    addPropLine( bookInfo, _("Title"), props->getStringDef(DOC_PROP_TITLE) );
    addPropLine( bookInfo, _("Author(s)"), props->getStringDef(DOC_PROP_AUTHORS) );
    addPropLine( bookInfo, _("Series name"), props->getStringDef(DOC_PROP_SERIES_NAME) );
    addPropLine( bookInfo, _("Series number"), props->getStringDef(DOC_PROP_SERIES_NUMBER) );
    addPropLine( bookInfo, _("Date"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/date", ", " ) );
    addPropLine( bookInfo, _("Genres"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/genre", ", " ) );
    addPropLine( bookInfo, _("Translator"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/translator", ", " ) );
    if ( !bookInfo.empty() )
        txt << "<tr><td colspan=\"2\" style=\"font-weight: bold; text-align: center\">Book info</td></tr>" << bookInfo;

    lString8 docInfo;
    addPropLine( docInfo, _("Document author"), getDocAuthors( getDocView()->getDocument(), "/FictionBook/description/document-info/author", " " ) );
    addPropLine( docInfo, _("Document date"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/date", " " ) );
    addPropLine( docInfo, _("Document source URL"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/src-url", " " ) );
    addPropLine( docInfo, _("OCR by"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/src-ocr", " " ) );
    addPropLine( docInfo, _("Document version"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/version", " " ) );
    if ( !docInfo.empty() )
        txt << "<tr><td colspan=\"2\" style=\"font-weight: bold; text-align: center\">Document info</td></tr>" << docInfo;

    lString8 pubInfo;
    addPropLine( pubInfo, _("Publication name"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/book-name", " " ) );
    addPropLine( pubInfo, _("Publisher"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/publisher", " " ) );
    addPropLine( pubInfo, _("Publisher city"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/city", " " ) );
    addPropLine( pubInfo, _("Publication year"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/year", " " ) );
    addPropLine( pubInfo, _("ISBN"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/isbn", " " ) );
    if ( !pubInfo.empty() )
        txt << "<tr><td colspan=\"2\" style=\"font-weight: bold; text-align: center\">Publication info</td></tr>" << pubInfo;

    addPropLine( txt, _("Custom info"), getDocText( getDocView()->getDocument(), "/FictionBook/description/custom-info", " " ) );

    txt << "</table>\n";

    //CRLog::trace(txt.c_str());
    //=========================================================
    txt = CRViewDialog::makeFb2Xml(txt);
    CRViewDialog * dlg = new CRViewDialog( _wm, title, txt, lvRect(), true, true );
    _wm->activateWindow( dlg );
}

bool V3DocViewWin::findInDictionary( lString16 pattern )
{
    if ( _dict.isNull() )
        _dict = LVRef<CRDictionary>( new CRTinyDict( Utf8ToUnicode(lString8(DICTD_CONF)) ) );
	lString8 body = _dict->translate( UnicodeToUtf8( pattern ) );
    lString8 txt = CRViewDialog::makeFb2Xml( body );
    CRViewDialog * dlg = new CRViewDialog( _wm, pattern, txt, lvRect(), true, true );
    _wm->activateWindow( dlg );
	return true;
}

bool V3DocViewWin::findText( lString16 pattern )
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

/// returns true if command is processed
bool V3DocViewWin::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_QUIT:
        getWindowManager()->closeAllWindows();
        return true;
    case MCMD_MAIN_MENU:
        showMainMenu();
        return true;
    case MCMD_SETTINGS_FONTSIZE:
        showFontSizeMenu();
        return true;
    case MCMD_SETTINGS_ORIENTATION:
        showOrientationMenu();
        return true;
    case MCMD_GO_PAGE:
        showGoToPageDialog();
        return true;
    case MCMD_GO_LINK:
        showLinksDialog();
        return true;
    case MCMD_SETTINGS:
        showSettingsMenu();
        return true;
    case MCMD_RECENT_BOOK_LIST:
        showRecentBooksMenu();
        return true;
    case MCMD_OPEN_RECENT_BOOK:
        openRecentBook( params );
        return true;

#ifdef WITH_DICT


    case MCMD_DICT:
		showT9Keyboard( _wm, this, MCMD_DICT_FIND, _searchPattern );
        return true;
	case MCMD_DICT_VKEYBOARD: 
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
    case MCMD_ABOUT:
        showAboutDialog();
        return true;
    case MCMD_CITE:
        activate_cite( _wm, this);
        return true;
    case MCMD_GO_PAGE_APPLY:
        _docview->doCommand( DCMD_GO_PAGE, params-1 );
        return true;
    case MCMD_SETTINGS_APPLY:
    case mm_Orientation:
    case mm_FontSize:
        applySettings();
        saveSettings( lString16() );
        return true;
    case MCMD_BOOKMARK_LIST:
        showBookmarksMenu();
        return true;
    default:
        // do nothing
        ;
    }
    return CRDocViewWindow::onCommand( command, params );
}
