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


V3DocViewWin::V3DocViewWin( CRGUIWindowManager * wm, lString16 dataDir )
: CRViewDialog ( wm, lString16(), lString8(), lvRect(), false, false ), _dataDir(dataDir)
{
    CRLog::trace("V3DocViewWin()");
    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
	_fullscreen = true;
    _docview->setShowCover( true );
    _docview->setFontSizes( sizes, true );
    _props = LVCreatePropsContainer();
    _newProps = _props;
    // TODO: move skin outside
    //lString16 skinfile = _dataDir;
    //LVAppendPathDelimiter( skinfile );
    //skinfile << L"skin";
    //lString8 s8 = UnicodeToLocal( skinfile );
    //CRLog::debug("Skin file is %s", s8.c_str() );
    //loadSkin( skinfile );


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

bool V3DocViewWin::loadHistory( LVStreamRef stream )
{
    if ( stream.isNull() ) {
        return false;
    }
    if ( !_docview->getHistory()->loadFromStream( stream ) )
        return false;
    return true;
}

bool V3DocViewWin::saveHistory( LVStreamRef stream )
{
    if ( stream.isNull() ) {
        CRLog::error("Cannot open history file for write" );
        return false;
    }
    return _docview->getHistory()->saveToStream( stream.get() );
}

bool V3DocViewWin::loadHistory( lString16 filename )
{
	CRLog::trace("V3DocViewWin::loadHistory( %s )", UnicodeToUtf8(filename).c_str());
    _historyFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    return loadHistory( stream );
}

void V3DocViewWin::closing()
{
	CRLog::trace("V3DocViewWin::closing()");
	_dict = NULL;
    _docview->savePosition();
    saveHistory( lString16() );
}

bool V3DocViewWin::loadDocument( lString16 filename )
{
    if ( !_docview->LoadDocument( filename.c_str() ) ) {
    	CRLog::error("V3DocViewWin::loadDocument( %s ) - failed!", UnicodeToUtf8(filename).c_str() );
        return false;
    }
    _docview->restorePosition();
    return true;
}

bool V3DocViewWin::saveHistory( lString16 filename )
{
    crtrace log;
    if ( filename.empty() )
        filename = _historyFileName;
    if ( filename.empty() ) {
        CRLog::info("Cannot write history file - no file name specified");
        return false;
    }
    CRLog::debug("Exporting bookmarks to %s", UnicodeToUtf8(_bookmarkDir).c_str());
    _docview->exportBookmarks(_bookmarkDir); //use default filename
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
    	CRLog::error("Error while creating history file %s - position will be lost", UnicodeToUtf8(filename).c_str() );
    	return false;
    }
    return saveHistory( stream );
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


bool V3DocViewWin::setHelpFile( lString16 filename )
{
	if ( LVFileExists( filename ) ) {
		_helpFile = filename;
		return true;
	}
	return false;
}

lString16 V3DocViewWin::getHelpFile( )
{
	return _helpFile;
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

void V3DocViewWin::showHelpDialog()
{
	LVStreamRef stream = LVOpenFileStream( _helpFile.c_str(), LVOM_READ );
	if ( stream.isNull() )
		return;
	int len = stream->GetSize();
	lString8 help;
	if ( len>100 && len <1000000 ) {
		help.append( len, ' ' );
		stream->Read( help.modify(), len, NULL );
	}
	//lString8 help = UnicodeToUtf8( LVReadTextFile( _helpFile ) );
	if ( !help.empty() ) {
		CRViewDialog * dlg = new CRViewDialog( _wm, _16("Help"), help, lvRect(), true, true );
		_wm->activateWindow( dlg );
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
        _16("Main Menu"),
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
	CRGUIAcceleratorTableRef menuItems = _wm->getAccTables().get(lString16("mainMenuItems"));
	if ( !menuItems.isNull() && menuItems->length()>1 ) {
		// get menu from file
		for ( unsigned i=0; i<menuItems->length(); i++ ) {
			const CRGUIAccelerator * acc = menuItems->get( i );
			int cmd = acc->commandId;
			int param = acc->commandParam;
			const char * name = getCommandName( cmd, param );
			int key = 0;
			int keyFlags = 0;
			lString8 label( name );
			if ( _acceleratorTable->findCommandKey( cmd, param, key, keyFlags ) ) {
				const char * keyname = getKeyName( key, keyFlags );
				if ( keyname && keyname[0] )
					label << "   (" << keyname << ")";
			}
			menu_win->addItem( new CRMenuItem( menu_win, cmd,
				label.c_str(),
				LVImageSourceRef(),
				LVFontRef() ) );
		}
	} else {
		// default menu
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_ABOUT,
					_("About"),
					LVImageSourceRef(),
					LVFontRef() ) );
	#if 0
		menu_win->addItem( new CRMenuItem( menu_win, DCMD_BEGIN,
					_wm->translateString("VIEWER_MENU_GOTOFIRSTPAGE", "Go to first page"),
					LVImageSourceRef(),
					LVFontRef() ) );
	#endif
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_GO_PAGE,
					_("Go to page"),
					LVImageSourceRef(),
					LVFontRef() ) );
	#if 0
		menu_win->addItem( new CRMenuItem( menu_win, DCMD_END,
					_wm->translateString("VIEWER_MENU_GOTOENDPAGE", "Go to last page"),
					LVImageSourceRef(),
					LVFontRef() ) );
	#endif

	#if USE_JINKE_USER_DATA!=1
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_RECENT_BOOK_LIST,
					_("Open recent book"),
					LVImageSourceRef(),
					LVFontRef() ) );
	#endif

	#ifdef WITH_DICT
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_DICT,
					_("Dictionary"),
					LVImageSourceRef(),
					LVFontRef() ) );
	#endif
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_CITE,
					_("Cite"),
					LVImageSourceRef(),
					LVFontRef() ) );
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_BOOKMARK_LIST,
					_("Bookmarks"),
					LVImageSourceRef(),
					LVFontRef() ) );
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_SEARCH,
					_("Search"),
					LVImageSourceRef(),
					LVFontRef() ) );
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_SETTINGS,
					_("Settings"),
					LVImageSourceRef(),
					LVFontRef() ) );
		if ( !_helpFile.empty() )
			menu_win->addItem( new CRMenuItem( menu_win, MCMD_HELP,
					_("Help"),
					LVImageSourceRef(),
					LVFontRef() ) );
		menu_win->addItem( new CRMenuItem( menu_win, MCMD_HELP_KEYS,
					_("Keyboard layout"),
					LVImageSourceRef(),
					LVFontRef() ) );
	}
	menu_win->setAccelerators( getMenuAccelerators() );
    _wm->activateWindow( menu_win );
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

static void addInfoSection( lString8 & buf, lString8 data, const char * caption )
{
    if ( !data.empty() )
        buf << "<tr><td colspan=\"2\" style=\"font-weight: bold; text-align: center\">" << caption << "</td></tr>" << data;
}

void V3DocViewWin::showAboutDialog()
{
	_docview->savePosition();
	CRFileHistRecord * hist = _docview->getCurrentFileHistRecord();
    lString16 title = L"Cool Reader ";
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "3.0"
#endif
    title << Utf8ToUnicode(lString8(PACKAGE_VERSION));

    lString8 txt;
    //=========================================================
    txt << "<table><col width=\"25%\"/><col width=\"75%\"/>\n";
    CRPropRef props = _docview->getDocProps();

    lString8 statusInfo;
	addPropLine( statusInfo, _("Current page"), lString16::itoa(_docview->getCurPage()) );
	addPropLine( statusInfo, _("Total pages"), lString16::itoa(_docview->getPageCount()) );
	addPropLine( statusInfo, _("Battery state"), lString16::itoa(_docview->getBatteryState()) + L"%" );
	// TODO:
	if ( hist ) {
		CRBookmark * lastpos = hist->getLastPos();
		if ( lastpos ) {
			addPropLine( statusInfo, _("Current chapter"), lastpos->getTitleText() );
		}
	}
    addInfoSection( txt, statusInfo, _("Status") );

    lString8 fileInfo;
    addPropLine( fileInfo, _("Archive name"), props->getStringDef(DOC_PROP_ARC_NAME) );
    addPropLine( fileInfo, _("Archive path"), props->getStringDef(DOC_PROP_ARC_PATH) );
    addPropLine( fileInfo, _("Archive size"), props->getStringDef(DOC_PROP_ARC_SIZE) );
    addPropLine( fileInfo, _("File name"), props->getStringDef(DOC_PROP_FILE_NAME) );
    addPropLine( fileInfo, _("File path"), props->getStringDef(DOC_PROP_FILE_PATH) );
    addPropLine( fileInfo, _("File size"), props->getStringDef(DOC_PROP_FILE_SIZE) );
    addPropLine( fileInfo, _("File format"), props->getStringDef(DOC_PROP_FILE_FORMAT) );
    addInfoSection( txt, fileInfo, _("File info") );

    lString8 bookInfo;
    addPropLine( bookInfo, _("Title"), props->getStringDef(DOC_PROP_TITLE) );
    addPropLine( bookInfo, _("Author(s)"), props->getStringDef(DOC_PROP_AUTHORS) );
    addPropLine( bookInfo, _("Series name"), props->getStringDef(DOC_PROP_SERIES_NAME) );
    addPropLine( bookInfo, _("Series number"), props->getStringDef(DOC_PROP_SERIES_NUMBER) );
    addPropLine( bookInfo, _("Date"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/date", ", " ) );
    addPropLine( bookInfo, _("Genres"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/genre", ", " ) );
    addPropLine( bookInfo, _("Translator"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/translator", ", " ) );
    addInfoSection( txt, bookInfo, _("Book info") );

    lString8 docInfo;
    addPropLine( docInfo, _("Document author"), getDocAuthors( getDocView()->getDocument(), "/FictionBook/description/document-info/author", " " ) );
    addPropLine( docInfo, _("Document date"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/date", " " ) );
    addPropLine( docInfo, _("Document source URL"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/src-url", " " ) );
    addPropLine( docInfo, _("OCR by"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/src-ocr", " " ) );
    addPropLine( docInfo, _("Document version"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/version", " " ) );
    addInfoSection( txt, docInfo, _("Document info") );

    lString8 pubInfo;
    addPropLine( pubInfo, _("Publication name"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/book-name", " " ) );
    addPropLine( pubInfo, _("Publisher"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/publisher", " " ) );
    addPropLine( pubInfo, _("Publisher city"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/city", " " ) );
    addPropLine( pubInfo, _("Publication year"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/year", " " ) );
    addPropLine( pubInfo, _("ISBN"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/isbn", " " ) );
    addInfoSection( txt, pubInfo, _("Publication info") );

    addPropLine( txt, _("Custom info"), getDocText( getDocView()->getDocument(), "/FictionBook/description/custom-info", " " ) );

    txt << "</table>\n";

    //CRLog::trace(txt.c_str());
    //=========================================================
    txt = CRViewDialog::makeFb2Xml(txt);
    CRViewDialog * dlg = new CRViewDialog( _wm, title, txt, lvRect(), true, true );
    _wm->activateWindow( dlg );
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
    case MCMD_SETTINGS:
        showSettingsMenu();
        return true;
    case MCMD_RECENT_BOOK_LIST:
        showRecentBooksMenu();
        return true;
#if USE_JINKE_USER_DATA!=1
    case MCMD_OPEN_RECENT_BOOK:
        openRecentBook( params );
        return true;
#endif

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
    case DCMD_ZOOM_IN:
    case DCMD_ZOOM_OUT:
		CRViewDialog::onCommand( command, params );
        _props->setInt( PROP_FONT_SIZE, _docview->getFontSize() );
        saveSettings( lString16() );
        return true;
    case MCMD_HELP:
        showHelpDialog();
        return true;
    default:
        // do nothing
        ;
    }
    return CRViewDialog::onCommand( command, params );
}
