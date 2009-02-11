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


#ifdef WITH_DICT
#include "dictdlg.h"
#endif


#ifdef _WIN32
#define XK_Return   0xFF0D
#define XK_Up       0xFF52
#define XK_Down     0xFF54
#define XK_Escape   0xFF1B
#else

#ifdef CR_USE_XCB
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#else

#define XK_Return   0xFF0D
#define XK_Up       0xFF52
#define XK_Down     0xFF54
#define XK_Escape   0xFF1B

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
    MCMD_DICT,
    MCMD_CITE,
    MCMD_BOOKMARK_LIST,
};

class V3DocViewWin : public CRDocViewWindow
{
protected:
    CRPropRef _props;
    CRPropRef _newProps;
    lString16 _dataDir;
    lString16 _settingsFileName;
    lString16 _historyFileName;
    lString8  _css;
    TEncoding _t9encoding;
    lString16 _dictConfig;
	LVRef<CRDictionary> _dict;
public:
    virtual void flush(); // override
    bool loadDocument( lString16 filename );
    bool loadDefaultCover( lString16 filename );
    bool loadCSS( lString16 filename );
    bool loadSkin( lString16 pathname );
    bool loadSettings( lString16 filename );
    bool saveSettings( lString16 filename );
    bool loadHistory( lString16 filename );
    bool saveHistory( lString16 filename );
    bool loadDictConfig( lString16 filename );
    CRGUIAcceleratorTableRef getMenuAccelerators()
    {
        return  _wm->getAccTables().get("menu");
    }
    CRGUIAcceleratorTableRef getDialogAccelerators()
    {
        return  _wm->getAccTables().get("dialog");
    }

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

    void showBookmarksMenu();

    virtual bool onCommand( int command, int params );

    virtual void closing();
};


#endif
