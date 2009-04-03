//
// C++ Interface: settings
//
// Description:
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MAINWND_H_INCLUDED
#define MAINWND_H_INCLUDED

#include <crengine.h>
#include <crgui.h>
#include <crtrace.h>
#include "settings.h"
#include "t9encoding.h"

#ifndef WITH_DICT
#define WITH_DICT
#endif


#include "viewdlg.h"

#if defined(_WIN32) || !defined(CR_USE_XCB)

//define key codes here
#define XK_Return   0xFF0D
#define XK_Up       0xFF52
#define XK_Down     0xFF54
#define XK_Escape   0xFF1B
#define XK_KP_Add   0xffab 
#define XK_KP_Subtract 0xffad
#define XK_Left     0xFF51
#define XK_Right    0xFF53
#define XK_Prior    0xFF55
#define XK_Next     0xFF56	
#define XK_KP_Enter 0xFF8D
#define XK_Menu	    0xFF67
#define XF86XK_RotateWindows    0x1008FF74
#define XF86XK_Search           0x1008FF1B
#define XF86XK_AudioPlay        0x1008FF14

#else

//use standard X11 key defs
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>

#endif

#define MAIN_MENU_COMMANDS_START 200
// don't forget to update keydefs.ini after editing of CRMainMenuCmd
enum CRMainMenuCmd
{
    MCMD_BEGIN = MAIN_MENU_COMMANDS_START,
    MCMD_QUIT,
    MCMD_MAIN_MENU,
    MCMD_GO_PAGE,
    MCMD_GO_PAGE_APPLY,
    MCMD_SETTINGS,
    MCMD_SETTINGS_APPLY,
    MCMD_SETTINGS_FONTSIZE,
    MCMD_SETTINGS_ORIENTATION,
    MCMD_GO_LINK,
    MCMD_GO_LINK_APPLY,
    MCMD_LONG_FORWARD,
    MCMD_LONG_BACK,
    MCMD_DICT,
    MCMD_BOOKMARK_LIST,
    MCMD_RECENT_BOOK_LIST,
    MCMD_OPEN_RECENT_BOOK,
    MCMD_ABOUT,
    MCMD_CITE,
    MCMD_SEARCH,
    MCMD_SEARCH_FINDFIRST,
    MCMD_DICT_VKEYBOARD,
    MCMD_DICT_FIND,
    MCMD_KBD_NEXTLAYOUT,
    MCMD_KBD_PREVLAYOUT,
    MCMD_HELP,
    MCMD_HELP_KEYS,
};

class V3DocViewWin : public CRViewDialog
{
protected:
    CRPropRef _props;
    CRPropRef _newProps;
    lString16 _dataDir;
    lString16 _settingsFileName;
    lString16 _historyFileName;
    lString8  _css;
    lString16 _dictConfig;
    lString16 _bookmarkDir;
	lString16 _helpFile;
public:
    lString16 getBookmarkDir() { return _bookmarkDir; }
    void setBookmarkDir( lString16 dir ) { _bookmarkDir = dir; }
    virtual void flush(); // override
    bool loadDocument( lString16 filename );
    bool loadDefaultCover( lString16 filename );
    bool loadCSS( lString16 filename );
    bool loadSettings( lString16 filename );
    bool saveSettings( lString16 filename );
    bool loadHistory( lString16 filename );
    bool saveHistory( lString16 filename );
    bool loadHistory( LVStreamRef stream );
    bool saveHistory( LVStreamRef stream );
    bool loadDictConfig( lString16 filename );
	bool setHelpFile( lString16 filename );
	lString16 getHelpFile( );

    /// returns current properties
    CRPropRef getProps() { return _props; }

    /// sets new properties
    void setProps( CRPropRef props )
    {
        _props = props;
        _docview->propsUpdateDefaults( _props );
    }

    V3DocViewWin( CRGUIWindowManager * wm, lString16 dataDir );

    void applySettings();

    void showSettingsMenu();

    void showOrientationMenu();

    void showFontSizeMenu();

    void showMainMenu();

    void showBookmarksMenu();

    void showRecentBooksMenu();

    void openRecentBook( int index );

    void showAboutDialog();

    void showHelpDialog();


    virtual bool onCommand( int command, int params );

    virtual void closing();
};


#endif
