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

#define KEY_FLAG_LONG_PRESS 1

#ifdef _WIN32
#define XK_Return   0xFF01
#define XK_Up       0xFF02
#define XK_Down     0xFF03
#define XK_Escape   0xFF04
#else

#ifdef CR_USE_XCB
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#else

#define XK_Return   0xFF01
#define XK_Up       0xFF02
#define XK_Down     0xFF03
#define XK_Escape   0xFF04

#endif
#endif

#define MAIN_MENU_COMMANDS_START 200
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
#ifdef WITH_DICT
    MCMD_DICT,
#endif
};

class V3DocViewWin : public CRDocViewWindow
{
protected:
    CRPropRef _props;
    CRPropRef _newProps;
    CRGUIAcceleratorTableRef _menuAccelerators;
    CRGUIAcceleratorTableRef _dialogAccelerators;
    lString16 _dataDir;
    lString16 _settingsFileName;
    lString8  _css;
    TEncoding _t9encoding;
    lString16 _dictConfig;
public:
    bool loadDefaultCover( lString16 filename );
    bool loadCSS( lString16 filename );
    bool loadSkin( lString16 pathname );
    bool loadSettings( lString16 filename );
    bool saveSettings( lString16 filename );
    bool loadDictConfig( lString16 filename );
    CRGUIAcceleratorTableRef getMenuAccelerators() { return _menuAccelerators; }
    CRGUIAcceleratorTableRef getDialogAccelerators() { return _dialogAccelerators; }

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

    void showGoToPageDialog();

	bool showLinksDialog();

    virtual bool onCommand( int command, int params );
};


#endif
