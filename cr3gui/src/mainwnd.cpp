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

/// define to show only battery
//#define NO_BATTERY_GAUGE 1

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
#include <cr3version.h>

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

#ifdef CR_POCKETBOOK
#include "cr3pocketbook.h"
#endif


#define SECONDS_BEFORE_PROGRESS_BAR 2


// uncomment to trace document mem usage during load or format progress
//#define TRACE_DOC_MEM_STATS 1

DECL_DEF_CR_FONT_SIZES;


V3DocViewWin::V3DocViewWin( CRGUIWindowManager * wm, lString16 dataDir )
: CRViewDialog ( wm, lString16::empty_str, lString8::empty_str, lvRect(), false, false ), _dataDir(dataDir), _loadFileStart(0)
{
    CRLog::trace("V3DocViewWin()");
    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
	_fullscreen = true;
    _docview->setShowCover( true );
    _docview->setFontSizes( sizes, true );
    _docview->setCallback( this );
    _props = LVCreatePropsContainer();
    _newProps = _props;
    // TODO: move skin outside
    //lString16 skinfile = _dataDir;
    //LVAppendPathDelimiter( skinfile );
    //skinfile << L"skin";
    //lString8 s8 = UnicodeToLocal( skinfile );
    //CRLog::debug("Skin file is %s", s8.c_str() );
    //loadSkin( skinfile );

#define BATTERY_HEADER \
        "36 19 5 1", \
        "0 c #000000", \
        "X c #000000", \
        "o c #AAAAAA", \
        ". c #FFFFFF", \
        "  c None",


    LVRefVec<LVImageSource> icons;
    static const char * battery8[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        "....0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0000.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0000.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        "....0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        "   .0.XXXXXX.XXXXXX.XXXXXX.XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery7[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        "....0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0000.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        ".0000.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        "....0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        "   .0.oooooo.XXXXXX.XXXXXX.XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery6[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0........XXXXXX.XXXXXX.XXXXXX.0.",
        "....0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0000........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0..0........XXXXXX.XXXXXX.XXXXXX.0.",
        ".0000........XXXXXX.XXXXXX.XXXXXX.0.",
        "....0........XXXXXX.XXXXXX.XXXXXX.0.",
        "   .0........XXXXXX.XXXXXX.XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery5[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0........oooooo.XXXXXX.XXXXXX.0.",
        "....0........oooooo.XXXXXX.XXXXXX.0.",
        ".0000........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0..0........oooooo.XXXXXX.XXXXXX.0.",
        ".0000........oooooo.XXXXXX.XXXXXX.0.",
        "....0........oooooo.XXXXXX.XXXXXX.0.",
        "   .0........oooooo.XXXXXX.XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery4[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0...............XXXXXX.XXXXXX.0.",
        "....0...............XXXXXX.XXXXXX.0.",
        ".0000...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0..0...............XXXXXX.XXXXXX.0.",
        ".0000...............XXXXXX.XXXXXX.0.",
        "....0...............XXXXXX.XXXXXX.0.",
        "   .0...............XXXXXX.XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery3[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0...............oooooo.XXXXXX.0.",
        "....0...............oooooo.XXXXXX.0.",
        ".0000...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0..0...............oooooo.XXXXXX.0.",
        ".0000...............oooooo.XXXXXX.0.",
        "....0...............oooooo.XXXXXX.0.",
        "   .0...............oooooo.XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery2[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0......................XXXXXX.0.",
        "....0......................XXXXXX.0.",
        ".0000......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0..0......................XXXXXX.0.",
        ".0000......................XXXXXX.0.",
        "....0......................XXXXXX.0.",
        "   .0......................XXXXXX.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery1[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0......................oooooo.0.",
        "....0......................oooooo.0.",
        ".0000......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0..0......................oooooo.0.",
        ".0000......................oooooo.0.",
        "....0......................oooooo.0.",
        "   .0......................oooooo.0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery0[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0.............................0.",
        "....0.............................0.",
        ".0000.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0000.............................0.",
        "....0.............................0.",
        "   .0.............................0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery_charge[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0.............................0.",
        "....0.............................0.",
        ".0000.............................0.",
        ".0..0................XX...........0.",
        ".0..0...............XXXXX.........0.",
        ".0..0..XXXX........XXXXXXXX.......0.",
        ".0..0....XXXXXX...XXXX...XXXXX....0.",
        ".0..0.......XXXXXXXXX.......XXXX..0.",
        ".0..0.........XXXXXX..............0.",
        ".0..0............XX...............0.",
        ".0000.............................0.",
        "....0.............................0.",
        "   .0.............................0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    static const char * battery_frame[] = {
        BATTERY_HEADER
        "   .................................",
        "   .0000000000000000000000000000000.",
        "   .0.............................0.",
        "   .0.............................0.",
        "....0.............................0.",
        ".0000.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0..0.............................0.",
        ".0000.............................0.",
        "....0.............................0.",
        "   .0.............................0.",
        "   .0.............................0.",
        "   .0000000000000000000000000000000.",
        "   .................................",
    };
    icons.add( LVCreateXPMImageSource( battery_charge ) );
//#ifdef NO_BATTERY_GAUGE
//    icons.add( LVCreateXPMImageSource( battery_frame ) );
//#else
    icons.add( LVCreateXPMImageSource( battery0 ) );
    icons.add( LVCreateXPMImageSource( battery1 ) );
    icons.add( LVCreateXPMImageSource( battery2 ) );
    icons.add( LVCreateXPMImageSource( battery3 ) );
    icons.add( LVCreateXPMImageSource( battery4 ) );
    icons.add( LVCreateXPMImageSource( battery5 ) );
    icons.add( LVCreateXPMImageSource( battery6 ) );
    icons.add( LVCreateXPMImageSource( battery7 ) );
    icons.add( LVCreateXPMImageSource( battery8 ) );
//#endif
    icons.add( LVCreateXPMImageSource( battery_frame ) );
    _docview->setBatteryIcons( icons );
    _wm->setBatteryIcons(icons);
    setAccelerators( _wm->getAccTables().get("main") );
}

/// on starting file loading
void V3DocViewWin::OnLoadFileStart( lString16 filename )
{
    _loadFileStart = time((time_t)0);
}

/// format detection finished
void V3DocViewWin::OnLoadFileFormatDetected( doc_format_t fileFormat )
{
    CRLog::trace("OnLoadFileFormatDetected(%d)", (int)fileFormat);
    lString16 filename("fb2.css");
    if ( _cssDir.length() > 0 ) {
        switch ( fileFormat ) {
        case doc_format_txt:
            filename = "txt.css";
            break;
        case doc_format_rtf:
            filename = "rtf.css";
            break;
        case doc_format_epub:
            filename = "epub.css";
            break;
        case doc_format_html:
            filename = "htm.css";
            break;
        case doc_format_chm:
            filename = "chm.css";
            break;
        case doc_format_doc:
            filename = "doc.css";
			break;
        default:
            // do nothing
            ;
        }
        CRLog::debug( "CSS file to load: %s", UnicodeToUtf8(filename).c_str() );
        if ( LVFileExists( _cssDir + filename ) ) {
            loadCSS( _cssDir + filename );
        } else if ( LVFileExists( _cssDir + "fb2.css" ) ) {
            loadCSS( _cssDir + "fb2.css" );
        }
    }
}

/// file loading is finished successfully - drawCoveTo() may be called there
void V3DocViewWin::OnLoadFileEnd()
{
}

/// file progress indicator, called with values 0..100
void V3DocViewWin::OnLoadFileProgress( int percent )
{
    CRLog::trace("OnLoadFileProgress(%d)", percent);
    time_t t = time((time_t)0);
    if ( t - _loadFileStart >= SECONDS_BEFORE_PROGRESS_BAR ) {
        _wm->showProgress(lString16("cr3_wait_icon.png"), 10+percent/2);
#ifdef TRACE_DOC_MEM_STATS
        _docview->getDocument()->dumpStatistics();
#endif
    }
}

/// document formatting started
void V3DocViewWin::OnFormatStart()
{
    time_t t = time((time_t)0);
    if ( t - _loadFileStart >= SECONDS_BEFORE_PROGRESS_BAR )
        _wm->showProgress(lString16("cr3_wait_icon.png"), 60);
}

/// document formatting finished
void V3DocViewWin::OnFormatEnd()
{
    time_t t = time((time_t)0);
    if ( t - _loadFileStart >= SECONDS_BEFORE_PROGRESS_BAR )
        _wm->showProgress(lString16("cr3_wait_icon.png"), 100);
    // Background cache file saving is disabled when _docview->updateCache(infinite) is called here.
    // To implement background cache file saving, schedule on Idle state following task:
    // in each idle cycle call _docview->updateCache(timeOut) while it returns CR_TIMEOUT
#ifndef BACKGROUND_CACHE_FILE_CREATION
    CRTimerUtil infinite;
    _docview->updateCache(infinite);
#endif
}

/// format progress, called with values 0..100
void V3DocViewWin::OnFormatProgress( int percent )
{
    CRLog::trace("OnFormatProgress(%d)", percent);
    time_t t = time((time_t)0);
    if ( t - _loadFileStart >= SECONDS_BEFORE_PROGRESS_BAR ) {
        _wm->showProgress(lString16("cr3_wait_icon.png"), 60+percent*4/10);
#ifdef TRACE_DOC_MEM_STATS
        _docview->getDocument()->dumpStatistics();
#endif
    }
}

/// file load finiished with error
void V3DocViewWin::OnLoadFileError( lString16 message )
{
}

/// Override to handle external links
void V3DocViewWin::OnExternalLink( lString16 url, ldomNode * node )
{
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
            CRLog::info( "Using style sheet from %s", UnicodeToUtf8(filename).c_str() );
            _cssDir = LVExtractPath(filename);
            LVAppendPathDelimiter(_cssDir);
            _docview->setStyleSheet( css );
            _css = css;
            //CRLog::debug("Stylesheet found:\n%s", css.c_str() );
            return true;
        }
    } else {
        //CRLog::debug("Stylesheet file not found %s", UnicodeToUtf8(filename).c_str() );
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
    _docview->getHistory()->limit( 32 );
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
    if ( !_docview->getDocument() )
        return;
    //_docview->getDocument()->swapToCacheIfNecessary();
    //_docview->getDocument()->updateMap();


	CRLog::trace("V3DocViewWin::closing(), before docview->savePosition()");
	_dict = NULL;
    _docview->savePosition();
    CRLog::trace("after docview->savePosition()");
    saveHistory( lString16::empty_str, _props->getBoolDef( PROP_AUTOSAVE_BOOKMARKS, true ) );
}

bool V3DocViewWin::loadDocument( lString16 filename )
{
    if ( !_docview->LoadDocument( filename.c_str() ) ) {
    	CRLog::error("V3DocViewWin::loadDocument( %s ) - failed!", UnicodeToUtf8(filename).c_str() );
        return false;
    }
    //_docview->swapToCache();
    _docview->restorePosition();
    return true;
}

bool V3DocViewWin::saveHistory( lString16 filename, bool exportBookmarks )
{
    crtrace log;
    if ( filename.empty() )
        filename = _historyFileName;
    if ( filename.empty() ) {
        CRLog::info("Cannot write history file - no file name specified");
        return false;
    }
    if (exportBookmarks) {
        CRLog::debug("Exporting bookmarks to %s", UnicodeToUtf8(_bookmarkDir).c_str());
        _docview->exportBookmarks(_bookmarkDir); //use default filename
    }
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
        if ( charging )
            charge = -1;
        if ( _docview->getBatteryState() != charge )
            _docview->setBatteryState( charge );
    }
    CRDocViewWindow::flush();
}

bool V3DocViewWin::loadSettings( lString16 filename )
{
	CRLog::debug("loading settings from %s", LCSTR(filename));
    _settingsFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
		CRLog::debug("settings file not found: %s", LCSTR(filename));
        _docview->propsUpdateDefaults( _props );
        _docview->propsApply( _props );
        _wm->getScreen()->setFullUpdateInterval(_props->getIntDef(PROP_DISPLAY_FULL_UPDATE_INTERVAL, 1));
        _wm->getScreen()->setTurboUpdateEnabled(_props->getIntDef(PROP_DISPLAY_TURBO_UPDATE_MODE, 0));
        //setAccelerators( _wm->getAccTables().get(lString16("main"), _props) );
        return false;
    }
    if ( _props->loadFromStream( stream.get() ) ) {
        _props->setIntDef(PROP_FILE_PROPS_FONT_SIZE, 26);
        _docview->propsUpdateDefaults( _props );
        _docview->propsApply( _props );
        _wm->getScreen()->setFullUpdateInterval(_props->getIntDef(PROP_DISPLAY_FULL_UPDATE_INTERVAL, 1));
        _wm->getScreen()->setTurboUpdateEnabled(_props->getIntDef(PROP_DISPLAY_TURBO_UPDATE_MODE, 0));
        setAccelerators( _wm->getAccTables().get(lString16("main"), _props) );
        return true;
    }
    _docview->propsUpdateDefaults( _props );
    _docview->propsApply( _props );
    _wm->getScreen()->setFullUpdateInterval(_props->getIntDef(PROP_DISPLAY_FULL_UPDATE_INTERVAL, 1));
    _wm->getScreen()->setTurboUpdateEnabled(_props->getIntDef(PROP_DISPLAY_TURBO_UPDATE_MODE, 0));
    //setAccelerators( _wm->getAccTables().get(lString16("main"), _props) );
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
    //showWaitIcon();
    CRPropRef delta = _props ^ _newProps;
    CRLog::trace( "applySettings() - %d options changed", delta->getCount() );
#ifdef CR_POCKETBOOK
	if (delta->hasProperty(PROP_POCKETBOOK_ORIENTATION)) {
		CRLog::trace("PB orientation have changed");
		_wm->postCommand(mm_Orientation, 1525);
	}
#endif
    _docview->propsApply( delta );
    _props = _newProps; // | _props;
    _wm->getScreen()->setFullUpdateInterval(_props->getIntDef(PROP_DISPLAY_FULL_UPDATE_INTERVAL, 1));
    setAccelerators( _wm->getAccTables().get(lString16("main"), _props) );
}

void V3DocViewWin::showSettingsMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    lvRect rc = _wm->getScreen()->getRect();
    CRMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, getMenuAccelerators(), rc );
    _wm->activateWindow( mainMenu );
}

void V3DocViewWin::showFontSizeMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    lvRect rc = _wm->getScreen()->getRect();
    CRSettingsMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, getMenuAccelerators(), rc );
    CRMenu * menu = mainMenu->createFontSizeMenu( _wm, NULL, _newProps );
    _wm->activateWindow( menu );
}

#if CR_INTERNAL_PAGE_ORIENTATION==1
void V3DocViewWin::showOrientationMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    lvRect rc = _wm->getScreen()->getRect();
    CRSettingsMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, getMenuAccelerators(), rc );
    CRMenu * menu = mainMenu->createOrientationMenu( NULL, _newProps );
    _wm->activateWindow( menu );
}
#endif

void V3DocViewWin::showRecentBooksMenu()
{
    lvRect rc = _wm->getScreen()->getRect();
    CRRecentBooksMenu * menu_win = new CRRecentBooksMenu(_wm, _docview, 8, rc);
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
    _docview->savePosition();
    LVPtrVector<CRFileHistRecord> & files = _docview->getHistory()->getRecords();
    if ( index >= 1 && index < files.length() ) {
        CRFileHistRecord * file = files.get( index );
        lString16 fn = file->getFilePathName();
        // TODO: check error
        if ( LVFileExists(fn) ) {
            //showWaitIcon();
            loadDocument( fn );
        }
        //_docview->swapToCache();
    }
}

void V3DocViewWin::showHelpDialog()
{
	LVStreamRef stream = LVOpenFileStream( _helpFile.c_str(), LVOM_READ );
	lString8 help;
    if ( stream.isNull() ) {
        // show warning
        lString8 body;
        body << "<title><p>" << _("No manual currently available for this language, sorry!") << "</p></title>";
        help = CRViewDialog::makeFb2Xml( body );
    } else {
        int len = stream->GetSize();
        if ( len>100 && len <1000000 ) {
            help.append( len, ' ' );
            stream->Read( help.modify(), len, NULL );
        }
    }
	//lString8 help = UnicodeToUtf8( LVReadTextFile( _helpFile ) );
	if ( !help.empty() ) {
		CRViewDialog * dlg = new CRViewDialog( _wm, lString16(_("Help")), help, lvRect(), true, true );
                int fs = _props->getIntDef( PROP_FONT_SIZE, 22 );
                dlg->getDocView()->setFontSize(fs);
		_wm->activateWindow( dlg );
	}
}

void V3DocViewWin::showBookmarksMenu( bool goMode )
{
    lvRect rc = _wm->getScreen()->getRect();
    CRBookmarkMenu * menu_win = new CRBookmarkMenu(_wm, _docview, 8, rc, goMode);
    _wm->activateWindow( menu_win );
}

void V3DocViewWin::showCitesMenu()
{
    lvRect rc = _wm->getScreen()->getRect();
    CRCitesMenu * menu_win = new CRCitesMenu(_wm, _docview, 8, rc);
    _wm->activateWindow( menu_win );
}

void V3DocViewWin::showMainMenu()
{
    lvRect fullRc = _wm->getScreen()->getRect();
    CRFullScreenMenu * menu_win = new CRFullScreenMenu(
            _wm, 0, lString16(_("Main Menu")), 8, fullRc );
/*
VIEWER_MENU_GOTOFIRSTPAGE=Go to first page
VIEWER_MENU_GOTOENDPAGE=Go to last page
VIEWER_MENU_GOTOPAGE=Go to page...
VIEWER_MENU_GOTOINDEX=Go to index
VIEWER_MENU_5ABOUT=About...
VIEWER_MENU_4ABOUT=About...
*/
    menu_win->setSkinName(lString16("#main"));
	CRGUIAcceleratorTableRef menuItems = _wm->getAccTables().get(lString16("mainMenuItems"));
	if ( !menuItems.isNull() && menuItems->length()>1 ) {
		// get menu from file
        for (int i=0; i < menuItems->length(); i++) {
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
					label << "\t" << keyname;
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

		menu_win->addItem( new CRMenuItem( menu_win, MCMD_RECENT_BOOK_LIST,
					_("Open recent book"),
					LVImageSourceRef(),
					LVFontRef() ) );

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

    lString16 s(_("$1 - choose command\n$2, $3 - close"));
#ifdef CR_POCKETBOOK
	s.replaceParam(1, menu_win->getCommandKeyName( MCMD_SELECT ));
#else
    s.replaceParam(1, menu_win->getItemNumberKeysName());
#endif
    s.replaceParam(2, menu_win->getCommandKeyName(MCMD_OK));
    s.replaceParam(3, menu_win->getCommandKeyName(MCMD_CANCEL) );
    menu_win->setStatusText( s );
    menu_win->setFullscreen( true );

    menu_win->reconfigure(0);
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
            s << " " << middleName;
        if ( !lastName.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << lastName;
        }
        if ( !nickName.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << nickName;
        }
        if ( !homePage.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << homePage;
        }
        if ( !email.empty() ) {
            if ( !s.empty() )
                s << " ";
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
    lString16 title("Cool Reader ");
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION CR_ENGINE_VERSION
#endif
    title << Utf8ToUnicode(lString8(PACKAGE_VERSION));

    lString8 txt;
    //=========================================================
    txt << "<table><col width=\"30%\"/><col width=\"70%\"/>\n";
    CRPropRef props = _docview->getDocProps();

    lString8 statusInfo;
	addPropLine( statusInfo, _("Current page"), lString16::itoa(_docview->getCurPage()+1) );
	addPropLine( statusInfo, _("Total pages"), lString16::itoa(_docview->getPageCount()) );
    addPropLine( statusInfo, _("Battery state"), _docview->getBatteryState()==-1 ? lString16(_("charging...")) : lString16::itoa(_docview->getBatteryState()) + "%" );
	addPropLine( statusInfo, _("Current Time"), _docview->getTimeString() );
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
    addPropLine( bookInfo, _("Language"), props->getStringDef(DOC_PROP_LANGUAGE) );
    addPropLine( bookInfo, _("Translator"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/translator", ", " ) );
    addPropLine( bookInfo, _("Source language"), getDocText( getDocView()->getDocument(), "/FictionBook/description/title-info/src-lang", ", " ) );
    addInfoSection( txt, bookInfo, _("Book info") );

    lString8 docInfo;
    addPropLine( docInfo, _("Document author"), getDocAuthors( getDocView()->getDocument(), "/FictionBook/description/document-info/author", " " ) );
    addPropLine( docInfo, _("Document date"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/date", " " ) );
    addPropLine( docInfo, _("Document source URL"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/src-url", " " ) );
    addPropLine( docInfo, _("OCR by"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/src-ocr", " " ) );
    addPropLine( docInfo, _("Document version"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/version", " " ) );
    addPropLine( docInfo, _("Change history"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/history", " " ) );
    addPropLine( docInfo, _("ID"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/id", " " ) );
    addPropLine( docInfo, _("Program used"), getDocText( getDocView()->getDocument(), "/FictionBook/description/document-info/program-used", " " ) );
    addInfoSection( txt, docInfo, _("Document info") );

    lString8 pubInfo;
    addPropLine( pubInfo, _("Publication name"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/book-name", " " ) );
    addPropLine( pubInfo, _("Publisher"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/publisher", " " ) );
    addPropLine( pubInfo, _("Publisher city"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/city", " " ) );
    addPropLine( pubInfo, _("Publication year"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/year", " " ) );
    addPropLine( pubInfo, _("ISBN"), getDocText( getDocView()->getDocument(), "/FictionBook/description/publish-info/isbn", " " ) );
    addInfoSection( txt, pubInfo, _("Publication info") );

    addPropLine( txt, _("Custom info"), getDocText( getDocView()->getDocument(), "/FictionBook/description/custom-info", " " ) );

#if defined(CR_PB_VERSION) && defined(CR_PB_BUILD_DATE)
    lString8 progInfo;
    addPropLine( progInfo, _("CoolReader for PocketBook"), Utf8ToUnicode(lString8(CR_PB_VERSION)));
    addPropLine( progInfo, _("Build date"), Utf8ToUnicode(lString8(CR_PB_BUILD_DATE)));
    addInfoSection( txt, progInfo, _("About program") );
#endif
    txt << "</table>\n";

    //CRLog::trace(txt.c_str());
    //=========================================================
    txt = CRViewDialog::makeFb2Xml(txt);
    CRViewDialog * dlg = new CRViewDialog( _wm, title, txt, lvRect(), true, true );
    dlg->getDocView()->setVisiblePageCount(1);
    int fs = _props->getIntDef( PROP_FILE_PROPS_FONT_SIZE, 22 );
    dlg->getDocView()->setFontSize(fs);
    _wm->activateWindow( dlg );
}

/// returns true if command is processed
bool V3DocViewWin::onCommand( int command, int params )
{
    CRLog::info("V3DocViewWin::onCommand(%d [%s], %d)", command, getCommandName(command, params), params );
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
#if CR_INTERNAL_PAGE_ORIENTATION==1
    case MCMD_SETTINGS_ORIENTATION:
        showOrientationMenu();
        return true;
#endif
    case MCMD_SETTINGS:
        showSettingsMenu();
        return true;
    case MCMD_RECENT_BOOK_LIST:
        showRecentBooksMenu();
        return true;
    case MCMD_OPEN_RECENT_BOOK:
        _docview->swapToCache();
        _docview->updateCache();
        _docview->getDocument()->updateMap();
        openRecentBook( params );
        return true;
    case MCMD_SWITCH_TO_RECENT_BOOK:
        _docview->swapToCache();
        _docview->updateCache();
        _docview->getDocument()->updateMap();
        openRecentBook( 1 );
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
    case MCMD_GO_PERCENT_APPLY:
        _docview->doCommand( DCMD_GO_POS, params * _docview->GetFullHeight() / 100 );
        return true;
    case MCMD_SETTINGS_APPLY:
#if CR_INTERNAL_PAGE_ORIENTATION==1
    case mm_Orientation:
#endif
    case mm_FontSize:
        applySettings();
        saveSettings(lString16::empty_str);
        _wm->getSkin()->gc();
        return true;
    case DCMD_SAVE_HISTORY:
        saveHistory(lString16::empty_str);
        saveSettings(lString16::empty_str);
        return true;
    case DCMD_SAVE_TO_CACHE:
        _docview->swapToCache();
        _docview->updateCache();
        _docview->getDocument()->updateMap();
        return true;
    case MCMD_CITES_LIST:
        showCitesMenu();
        return true;
    case MCMD_BOOKMARK_LIST:
        showBookmarksMenu(false);
        return true;
    case MCMD_BOOKMARK_LIST_GO_MODE:
        showBookmarksMenu(true);
        return true;
    case DCMD_ZOOM_IN:
    case DCMD_ZOOM_OUT:
        showWaitIcon();
		CRViewDialog::onCommand( command, params );
        _props->setInt( PROP_FONT_SIZE, _docview->getFontSize() );
        saveSettings(lString16::empty_str);
        return true;
    case MCMD_HELP:
        showHelpDialog();
        return true;
    case DCMD_BOOKMARK_SAVE_N:
        _docview->doCommand( DCMD_BOOKMARK_SAVE_N, params );
        if ( _props->getBoolDef( PROP_AUTOSAVE_BOOKMARKS, true ) )
            saveHistory(lString16::empty_str);
        return true;
    default:
        // do nothing
        ;
    }
    return CRViewDialog::onCommand( command, params );
}

#define DISABLE_FIRST_PAGE_RENDER 1
/// first page is loaded from file an can be formatted for preview
void V3DocViewWin::OnLoadFileFirstPagesReady()
{
    if ( DISABLE_FIRST_PAGE_RENDER || !_props->getBoolDef( PROP_PROGRESS_SHOW_FIRST_PAGE, 1 ) ) {
        CRLog::info( "OnLoadFileFirstPagesReady() - don't paint first page because " PROP_PROGRESS_SHOW_FIRST_PAGE " setting is 0" );
        return;
    }
    CRLog::info( "OnLoadFileFirstPagesReady() - painting first page" );
    _docview->setPageHeaderOverride(lString16(_("Loading: please wait...")));
    //update();
    _wm->update(true);
    CRLog::info( "OnLoadFileFirstPagesReady() - painting done" );
    _docview->setPageHeaderOverride(lString16::empty_str);
    _docview->requestRender();
    // TODO: remove debug sleep
    //sleep(5);
}
