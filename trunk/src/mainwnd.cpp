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



#ifdef WITH_DICT
#include "dictdlg.h"
#endif
#include "numedit.h"
#include "linksdlg.h"
#include "tocdlg.h"
#include "bmkdlg.h"
#include "recentdlg.h"

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


const lChar16 * defT9encoding[] = {
    L"",     // 0 STUB
    L"abc",  // 1
    L"def",  // 2
    L"ghi",  // 3
    L"jkl",  // 4
    L"mno",  // 5
    L"pqrs", // 6
    L"tuv",  // 7
    L"wxyz", // 8
    L"",      // 9 STUB
    NULL
};

const lChar16 * defT5encoding[] = {
    L"",     // 0 STUB
    L"abcde",  // 1
    L"fghij",  // 2
    L"klmno",  // 3
    L"pqrst",  // 4
    L"uvwxyz", // 5
    NULL
};

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
: CRDocViewWindow ( wm ), _dataDir(dataDir), _t9encoding(defT5encoding) //defT9encoding)
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
        lString16(L"Main Menu"),
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
#if 0
    menu_win->addItem( new CRMenuItem( menu_win, DCMD_BEGIN,
                _wm->translateString("VIEWER_MENU_GOTOFIRSTPAGE", "Go to first page"),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_GO_PAGE,
                _wm->translateString("VIEWER_MENU_GOTOPAGE", "Go to page ..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#if 0
    menu_win->addItem( new CRMenuItem( menu_win, DCMD_END,
                _wm->translateString("VIEWER_MENU_GOTOENDPAGE", "Go to last page"),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_RECENT_BOOK_LIST,
                _wm->translateString("VIEWER_MENU_RECENT_BOOKS_LIST", "Open recent book..."),
                LVImageSourceRef(),
                LVFontRef() ) );

#ifdef WITH_DICT
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_DICT,
                _wm->translateString("VIEWER_MENU_DICTIONARY", "Dictionary..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
#ifdef WITH_CITE
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_CITE,
                _wm->translateString("VIEVER_MENU_CITE", "Cite.."),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_BOOKMARK_LIST,
                _wm->translateString("VIEWER_MENU_BOOKMARK_LIST", "Bookmarks..."),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_SETTINGS,
                _wm->translateString("VIEWER_MENU_SETTINGS", "Settings..."),
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
            _wm->translateString("VIEWER_HINT_INPUTSKIPPAGENUM", "Table of contents"),
            MCMD_GO_PAGE_APPLY,  _docview->getPageCount(), _docview );
    } else {
        dlg = new CRNumberEditDialog( _wm, 
            _wm->translateString("VIEWER_HINT_INPUTSKIPPAGENUM", "Enter page number"),
            lString16(), 
            MCMD_GO_PAGE_APPLY, 1, _docview->getPageCount() );
    }
    dlg->setAccelerators( getDialogAccelerators() );
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

#ifdef _WIN32
#define DICTD_CONF "C:\\dict\\"
#else
#ifdef CR_USE_JINKE
#define DICTD_CONF "/root/crengine/dict/"
#else
#define DICTD_CONF "/media/sd/dict"
#endif
#endif

    case MCMD_DICT:
        CRLog::info("MCMD_DICT activated\n");
        if ( _dict.isNull() )
            _dict = LVRef<CRDictionary>( new CRTinyDict( Utf8ToUnicode(lString8(DICTD_CONF)) ) );
        activate_dict( _wm, this, _t9encoding, *_dict );
        return true;
#endif
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
