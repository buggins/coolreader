/*
    First version of CR3 for EWL, based on etimetool example by Lunohod
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
//#include <Ewl.h>
#include <crengine.h>
#include <crgui.h>
#include <crtrace.h>

#ifdef WITH_DICT
#include "mod-dict.h"
#endif

#define CR_USE_XCB

#ifdef _WIN32
#define XK_Return   0xFF01
#define XK_Up       0xFF02
#define XK_Down     0xFF03
#define XK_Escape   0xFF04
#else

#ifdef CR_USE_XCB
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#endif
#endif


bool initHyph(const char * fname)
{
    //HyphMan hyphman;
    //return;

    LVStreamRef stream = LVOpenFileStream( fname, LVOM_READ);
    if (!stream)
    {
        printf("Cannot load hyphenation file %s\n", fname);
        return false;
    }
    return HyphMan::Open( stream.get() );
}

lString8 readFileToString( const char * fname )
{
    lString8 buf;
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return buf;
    int sz = stream->GetSize();
    if (sz>0)
    {
        buf.insert( 0, sz, ' ' );
        stream->Read( buf.modify(), sz, NULL );
    }
    return buf;
}

void ShutdownCREngine()
{
    HyphMan::Close();
#if LDOM_USE_OWN_MEM_MAN == 1
    ldomFreeStorage();
#endif
}

#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString16Collection & pathList, lString16 ext, lString16Collection & fonts, bool absPath )
{
    int foundCount = 0;
    lString16 path;
    for ( unsigned di=0; di<pathList.length();di++ ) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if ( !dir.isNull() ) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str() );
            for ( int i=0; i < dir->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = dir->GetObjectInfo(i);
                lString16 fileName = item->GetName();
                lString8 fn = UnicodeToLocal(fileName);
                    //printf(" test(%s) ", fn.c_str() );
                if ( !item->IsContainer() && fileName.length()>4 && lString16(fileName, fileName.length()-4, 4)==ext ) {
                    lString16 fn;
                    if ( absPath ) {
                        fn = path;
                        if ( !fn.empty() && fn[fn.length()-1]!=PATH_SEPARATOR_CHAR)
                            fn << PATH_SEPARATOR_CHAR;
                    }
                    fn << fileName;
                    foundCount++;
                    fonts.add( fn );
                }
            }
        }
    }
    return foundCount > 0;
}
#endif

bool InitCREngine( const char * exename )
{
    lString16 appname( exename );
    int lastSlash=-1;
    lChar16 slashChar = '/';
    for ( int p=0; p<(int)appname.length(); p++ ) {
        if ( appname[p]=='\\' ) {
            slashChar = '\\';
            lastSlash = p;
        } else if ( appname[p]=='/' ) {
            slashChar = '/';
            lastSlash=p;
        }
    }

    lString16 appPath;
    if ( lastSlash>=0 )
        appPath = appname.substr( 0, lastSlash+1 );

    lString16 fontDir = appPath + L"fonts";
    fontDir << slashChar;
    lString8 fontDir8 = UnicodeToLocal(fontDir);
    const char * fontDir8s = fontDir8.c_str();
    //InitFontManager( fontDir8 );
    InitFontManager( lString8() );

    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    if (!fontMan->GetFontCount()) {


    #if (USE_FREETYPE==1)
        lString16 fontExt = L".ttf";
    #else
        lString16 fontExt = L".lbf";
    #endif
    #if (USE_FREETYPE==1)
        lString16Collection fonts;
        lString16Collection fontDirs;
        fontDirs.add( fontDir );
        static const char * msfonts[] = {
            "arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf",
            "cour.ttf", "courbd.ttf", "couri.ttf", "courbi.ttf",
            "times.ttf", "timesbd.ttf", "timesi.ttf", "timesbi.ttf",
            NULL
        };
    #ifdef _LINUX
        fontDirs.add( lString16(L"/usr/local/share/crengine/fonts") );
        fontDirs.add( lString16(L"/usr/local/share/fonts/truetype/freefont") );
        fontDirs.add( lString16(L"/usr/share/crengine/fonts") );
        fontDirs.add( lString16(L"/usr/share/fonts/truetype/freefont") );
        fontDirs.add( lString16(L"/root/fonts/truetype") );
        //fontDirs.add( lString16(L"/usr/share/fonts/truetype/msttcorefonts") );
        for ( int fi=0; msfonts[fi]; fi++ )
            fonts.add( lString16(L"/usr/share/fonts/truetype/msttcorefonts/") + lString16(msfonts[fi]) );
    #endif
        getDirectoryFonts( fontDirs, fontExt, fonts, true );

        // load fonts from file
        CRLog::debug("%d font files found", fonts.length());
        if (!fontMan->GetFontCount()) {
            for ( unsigned fi=0; fi<fonts.length(); fi++ ) {
                lString8 fn = UnicodeToLocal(fonts[fi]);
                CRLog::trace("loading font: %s", fn.c_str());
                if ( !fontMan->RegisterFont(fn) ) {
                    CRLog::trace("    failed\n");
                }
            }
        }
    #else
            #define MAX_FONT_FILE 128
            for (int i=0; i<MAX_FONT_FILE; i++)
            {
                char fn[1024];
                sprintf( fn, "font%d.lbf", i );
                printf("try load font: %s\n", fn);
                fontMan->RegisterFont( lString8(fn) );
            }
    #endif
    }

    // init hyphenation manager
    char hyphfn[1024];
    sprintf(hyphfn, "Russian_EnUS_hyphen_(Alan).pdb" );
    if ( !initHyph( (UnicodeToLocal(appPath) + hyphfn).c_str() ) ) {
#ifdef _LINUX
        initHyph( "/usr/share/crengine/hyph/Russian_EnUS_hyphen_(Alan).pdb" );
#endif
    }

    if (!fontMan->GetFontCount())
    {
        //error
#if (USE_FREETYPE==1)
        printf("Fatal Error: Cannot open font file(s) .ttf \nCannot work without font\n" );
#else
        printf("Fatal Error: Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF\n" );
#endif
        return false;
    }

    printf("%d fonts loaded.\n", fontMan->GetFontCount());

    return true;

}

#define MAIN_MENU_COMMANDS_START 200
enum CRMainMenuCmd
{
    MCMD_BEGIN = MAIN_MENU_COMMANDS_START,
    MCMD_QUIT,
    MCMD_MAIN_MENU,
    MCMD_GO_PAGE,
    MCMD_SETTINGS,
#ifdef WITH_DICT
    MCMD_DICT,
#endif
};

#define SETTINGS_MENU_COMMANDS_START 300
enum MainMenuItems_t {
    mm_Settings = SETTINGS_MENU_COMMANDS_START,
    mm_FontFace,
    mm_FontSize,
    mm_FontAntiAliasing,
    mm_InterlineSpace,
    mm_Orientation,
    mm_EmbeddedStyles,
    mm_Inverse,
    mm_StatusLine,
    mm_BookmarkIcons,
    mm_Footnotes,
    mm_SetTime,
    mm_ShowTime,
    mm_Kerning,
    mm_LandscapePages,
    mm_PreformattedText,
};


typedef struct {
    const char * translate_label;
    const char * translate_default;
    const char * value;
} item_def_t;

static item_def_t antialiasing_modes[] = {
    {"VIEWER_DLG_ANTIALIASING_ALWAYS", "On for all fonts", "2"},
    {"VIEWER_DLG_ANTIALIASING_BIGFONTS", "On for big fonts only", "1"},
    {"VIEWER_DLG_ANTIALIASING_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t embedded_styles[] = {
    {"VIEWER_DLG_EMBEDDED_STYLES_ON", "On", "1"},
    {"VIEWER_DLG_EMBEDDED_STYLES_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t bookmark_icons[] = {
    {"VIEWER_DLG_BOOKMARK_ICONS_ON", "On", "1"},
    {"VIEWER_DLG_BOOKMARK_ICONS_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t kerning_options[] = {
    {"VIEWER_DLG_KERNING_ON", "On", "1"},
    {"VIEWER_DLG_KERNING_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t footnotes[] = {
    {"VIEWER_DLG_FOOTNOTES_ON", "On", "1"},
    {"VIEWER_DLG_FOOTNOTES_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t showtime[] = {
    {"VIEWER_DLG_TIME_ON", "On", "1"},
    {"VIEWER_DLG_TIME_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t preformatted_text[] = {
    {"VIEWER_DLG_PREFORMATTED_TEXT_ON", "On", "1"},
    {"VIEWER_DLG_PREFORMATTED_TEXT_OFF", "Off", "0"},
    {NULL, NULL, NULL},
};

static item_def_t inverse_mode[] = {
    {"VIEWER_DLG_INVERSE_OFF", "Normal", "0"},
    {"VIEWER_DLG_INVERSE_ON", "Inverse", "1"},
    {NULL, NULL, NULL},
};

static item_def_t landscape_pages[] = {
    {"VIEWER_DLG_LANDSCAPE_PAGES_1", "One", "1"},
    {"VIEWER_DLG_LANDSCAPE_PAGES_2", "Two", "2"},
    {NULL, NULL, NULL},
};

static item_def_t status_line[] = {
    {"VIEWER_DLG_STATUS_LINE_TOP", "Top", "0"},
    {"VIEWER_DLG_STATUS_LINE_BOTTOM", "Bottom", "1"},
    {"VIEWER_DLG_STATUS_LINE_OFF", "Off", "2"},
    {NULL, NULL, NULL},
};

static item_def_t interline_spaces[] = {
    {"VIEWER_DLG_INTERLINE_SPACE_0", "90%", "90"},
    {"VIEWER_DLG_INTERLINE_SPACE_1", "100%", "100"},
    {"VIEWER_DLG_INTERLINE_SPACE_2", "110%", "110"},
    {"VIEWER_DLG_INTERLINE_SPACE_3", "120%", "120"},
    {"VIEWER_DLG_INTERLINE_SPACE_4", "140%", "140"},
    {NULL, NULL, NULL},
};

static item_def_t page_orientations[] = {
    {"VIEWER_DLG_ORIENTATION_1", "Normal", "0"},
    {"VIEWER_DLG_ORIENTATION_2", "90 `", "1"},
    {"VIEWER_DLG_ORIENTATION_3", "180 `", "2"},
    {"VIEWER_DLG_ORIENTATION_4", "270 `", "3"},
    {NULL, NULL, NULL},
};

static item_def_t page_margins[] = {
    {"VIEWER_DLG_PAGE_MARGIN_0", "0", "0"},
    {"VIEWER_DLG_PAGE_MARGIN_1", "5", "5"},
    {"VIEWER_DLG_PAGE_MARGIN_2", "10", "10"},
    {"VIEWER_DLG_PAGE_MARGIN_3", "15", "15"},
    {"VIEWER_DLG_PAGE_MARGIN_4", "20", "20"},
    {"VIEWER_DLG_PAGE_MARGIN_5", "25", "25"},
    {NULL, NULL, NULL},
};

static int cr_font_sizes[] = { 24, 29, 33, 39, 44 };

// TODO: get from skin
#define MENU_FONT_SIZE 20
#define VALUE_FONT_SIZE 17
class V3DocViewWin : public CRDocViewWindow
{
protected:
    CRPropRef _props;
    CRPropRef _newProps;
    CRGUIAcceleratorTableRef _menuAccelerators;
public:
    /// returns current properties
    CRPropRef getProps() { return _props; }

    /// sets new properties
    void setProps( CRPropRef props )
    {
        _props = props;
        _docview->propsUpdateDefaults( _props );
    }

    V3DocViewWin( CRGUIWindowManager * wm )
    : CRDocViewWindow ( wm )
    {
         LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
        _docview->setFontSizes( sizes, true );
        _props = LVCreatePropsContainer();
        _newProps = _props;
        // TODO: move accelerator table outside
        static const int acc_table[] = {
            XK_Escape, 0, MCMD_CANCEL, 0,
            XK_Return, 0, MCMD_OK, 0, 
            '0', 0, MCMD_SCROLL_FORWARD, 0,
            XK_Down, 0, MCMD_SCROLL_FORWARD, 0,
            '9', 0, MCMD_SCROLL_BACK, 0,
            XK_Up, 0, MCMD_SCROLL_BACK, 0,
            '1', 0, MCMD_SELECT_1, 0,
            '2', 0, MCMD_SELECT_2, 0,
            '3', 0, MCMD_SELECT_3, 0,
            '4', 0, MCMD_SELECT_4, 0,
            '5', 0, MCMD_SELECT_5, 0,
            '6', 0, MCMD_SELECT_6, 0,
            '7', 0, MCMD_SELECT_7, 0,
            '8', 0, MCMD_SELECT_8, 0,
            0
        };
        _menuAccelerators = CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable( acc_table ) );
    }

    void addMenuItems( CRMenu * menu, item_def_t values[], LVFontRef menuFont )
    {
        for ( int i=0; values[i].translate_label; i++)
            menu->addItem( new CRMenuItem( menu, i,
               _wm->translateString(values[i].translate_label, values[i].translate_default),
               LVImageSourceRef(), 
               menuFont, Utf8ToUnicode(lString8(values[i].value)).c_str() ) );
        menu->setAccelerators( _menuAccelerators );
    }

    void applySettings()
    {
        CRPropRef delta = _props ^ _newProps;
        CRLog::trace( "applySettings() - %d options changed", delta->getCount() );
        _docview->propsApply( delta );
    }

    CRMenu * createFontSizeMenu( CRMenu * mainMenu, CRPropRef props )
    {
        lString16Collection list;
        fontMan->getFaceList( list );
        lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
        LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, false, css_ff_sans_serif, lString8("Arial")) );
        LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 300, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * fontSizeMenu;
        fontSizeMenu = new CRMenu(_wm, mainMenu, mm_FontSize,
                                    _wm->translateString("VIEWER_MENU_FONT_SIZE", "Default font size"),
                                            LVImageSourceRef(), menuFont, valueFont, props, PROP_FONT_SIZE );
        fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 0,
                                _wm->translateString("VIEWER_DLG_FONT_SIZE_1", "Smallest"),
                                        LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[0], 300, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[0]).c_str()  ) );
        fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 1,
                                _wm->translateString("VIEWER_DLG_FONT_SIZE_2", "Small"),
                                        LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[1], 300, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[1]).c_str()  ) );
        fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 2,
                                _wm->translateString("VIEWER_DLG_FONT_SIZE_3", "Medium"),
                                        LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[2], 300, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[2]).c_str()  ) );
        fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 3,
                                _wm->translateString("VIEWER_DLG_FONT_SIZE_4", "Big"),
                                        LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[3], 300, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[3]).c_str()  ) );
        fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 4,
                                _wm->translateString("VIEWER_DLG_FONT_SIZE_5", "Biggest"),
                                        LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[4], 300, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[4]).c_str()  ) );
        fontSizeMenu->setAccelerators( _menuAccelerators );
        return fontSizeMenu;
    }

    void showSettingsMenu()
    {
        _props->set( _docview->propsGetCurrent() );
        _newProps = LVClonePropsContainer( _props );
        CRPropRef props = _newProps;
        CRLog::trace("showSettingsMenu() - %d property values found", props->getCount() );

        LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
        LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 300, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * mainMenu = new CRMenu( _wm,
            NULL, //CRMenu * parentMenu,
            mm_Settings, // command to generate on OK
            lString16(L"Main Menu"),
            LVImageSourceRef(),
            menuFont,
            menuFont );
        mainMenu->setAccelerators( _menuAccelerators );

        CRMenu * fontFaceMenu = new CRMenu(_wm, mainMenu, mm_FontFace,
                                            _wm->translateString("VIEWER_MENU_FONT_FACE", "Default font face"),
                                                    LVImageSourceRef(), menuFont, valueFont, props, PROP_FONT_FACE );
        CRLog::trace("getting font face list");
        lString16Collection list;
        fontMan->getFaceList( list );
        CRLog::trace("faces found: %d", list.length());
        int i;
        for ( i=0; i<list.length(); i++ ) {
            fontFaceMenu->addItem( new CRMenuItem( fontFaceMenu, i,
                                    list[i], LVImageSourceRef(), fontMan->GetFont( MENU_FONT_SIZE, 300, false, css_ff_sans_serif, UnicodeToUtf8(list[i])), list[i].c_str() ) );
        }
        fontFaceMenu->setAccelerators( _menuAccelerators );
        //lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
        mainMenu->addItem( fontFaceMenu );

        CRMenu * fontSizeMenu = createFontSizeMenu( mainMenu, props );
        mainMenu->addItem( fontSizeMenu );

        CRMenu * fontAntialiasingMenu = new CRMenu(_wm, mainMenu, mm_FontAntiAliasing,
                _wm->translateString("VIEWER_MENU_FONT_ANTIALIASING", "Font antialiasing"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_FONT_ANTIALIASING );
        addMenuItems( fontAntialiasingMenu, antialiasing_modes, menuFont );
        mainMenu->addItem( fontAntialiasingMenu );
        CRMenu * interlineSpaceMenu = new CRMenu(_wm, mainMenu, mm_InterlineSpace,
                _wm->translateString("VIEWER_MENU_INTERLINE_SPACE", "Interline space"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_INTERLINE_SPACE );
        addMenuItems( interlineSpaceMenu, interline_spaces, menuFont );
        mainMenu->addItem( interlineSpaceMenu );
/*
        CRMenu * orientationMenu = createOrientationMenu(props);
        mainMenu->addItem( orientationMenu );
*/
        CRMenu * footnotesMenu = new CRMenu(_wm, mainMenu, mm_Footnotes,
                _wm->translateString("VIEWER_MENU_FOOTNOTES", "Footnotes at page bottom"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_FOOTNOTES );
        addMenuItems( footnotesMenu, footnotes, menuFont );
        mainMenu->addItem( footnotesMenu );

/*
        SetTimeMenuItem * setTime = new SetTimeMenuItem( mainMenu, mm_SetTime, _wm->translateString("VIEWER_MENU_SET_TIME", "Set time"),
                LVImageSourceRef(), menuFont, L"bla" );
        mainMenu->addItem( setTime );
*/
        CRMenu * showTimeMenu = new CRMenu(_wm, mainMenu, mm_ShowTime,
                _wm->translateString("VIEWER_MENU_SHOW_TIME", "Show time"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_SHOW_TIME );
        addMenuItems( showTimeMenu, showtime, menuFont );
        mainMenu->addItem( showTimeMenu );

        CRMenu * landscapePagesMenu = new CRMenu(_wm, mainMenu, mm_LandscapePages,
                _wm->translateString("VIEWER_MENU_LANDSCAPE_PAGES", "Landscape pages"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_LANDSCAPE_PAGES );
        addMenuItems( landscapePagesMenu, landscape_pages, menuFont );
        mainMenu->addItem( landscapePagesMenu );

        CRMenu * preformattedTextMenu = new CRMenu(_wm, mainMenu, mm_PreformattedText,
                _wm->translateString("VIEWER_MENU_TXT_OPTION_PREFORMATTED", "Preformatted text"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_TXT_OPTION_PREFORMATTED );
        addMenuItems( preformattedTextMenu, preformatted_text, menuFont );
        mainMenu->addItem( preformattedTextMenu );

        //

        CRMenu * embeddedStylesMenu = new CRMenu(_wm, mainMenu, mm_EmbeddedStyles,
                _wm->translateString("VIEWER_MENU_EMBEDDED_STYLES", "Document embedded styles"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_EMBEDDED_STYLES );
        addMenuItems( embeddedStylesMenu, embedded_styles, menuFont );
        mainMenu->addItem( embeddedStylesMenu );

        CRMenu * inverseModeMenu = new CRMenu(_wm, mainMenu, mm_Inverse,
                _wm->translateString("VIEWER_MENU_INVERSE", "Inverse display"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_DISPLAY_INVERSE );
        addMenuItems( inverseModeMenu, inverse_mode, menuFont );
        mainMenu->addItem( inverseModeMenu );

        CRMenu * bookmarkIconsMenu = new CRMenu(_wm, mainMenu, mm_BookmarkIcons,
                _wm->translateString("VIEWER_MENU_BOOKMARK_ICONS", "Show bookmark icons"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_BOOKMARK_ICONS );
        addMenuItems( bookmarkIconsMenu, bookmark_icons, menuFont );
        mainMenu->addItem( bookmarkIconsMenu );

        CRMenu * statusLineMenu = new CRMenu(_wm, mainMenu, mm_StatusLine,
                _wm->translateString("VIEWER_MENU_STATUS_LINE", "Status line"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_STATUS_LINE );
        addMenuItems( statusLineMenu, status_line, menuFont );
        mainMenu->addItem( statusLineMenu );

        CRMenu * kerningMenu = new CRMenu(_wm, mainMenu, mm_Kerning,
                _wm->translateString("VIEWER_MENU_FONT_KERNING", "Font kerning"),
                                LVImageSourceRef(), menuFont, valueFont, props, PROP_FONT_KERNING_ENABLED );
        addMenuItems( kerningMenu, kerning_options, menuFont );
        mainMenu->addItem( kerningMenu );

        // TODO: process submenus
        _wm->activateWindow( mainMenu );
    }

    void showMainMenu()
    {
        LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * menu_win = new CRMenu( _wm,
            NULL, //CRMenu * parentMenu,
            1,
            lString16(L"Main Menu"),
            LVImageSourceRef(),
            menuFont,
            menuFont );
        menu_win->addItem( new CRMenuItem( menu_win, DCMD_BEGIN,
                       lString16(L"Go to first page"),
                       LVImageSourceRef(),
                       menuFont ) );
        menu_win->addItem( new CRMenuItem( menu_win, MCMD_GO_PAGE,
                       lString16(L"Go to page ..."),
                       LVImageSourceRef(),
                       menuFont ) );
        menu_win->addItem( new CRMenuItem( menu_win, DCMD_END,
                       lString16(L"Go to last page"),
                       LVImageSourceRef(),
                       menuFont ) );
        menu_win->addItem( new CRMenuItem( menu_win, MCMD_SETTINGS,
                       lString16(L"Settings..."),
                       LVImageSourceRef(),
                       menuFont ) );
#ifdef WITH_DICT
        menu_win->addItem( new CRMenuItem( menu_win, MCMD_DICT,
                       lString16(L"Dictionary..."),
                       LVImageSourceRef(),
                       menuFont ) );
#endif
        menu_win->setAccelerators( _menuAccelerators );
        _wm->activateWindow( menu_win );
    }

    /// returns true if command is processed
    virtual bool onCommand( int command, int params )
    {
        switch ( command ) {
        case MCMD_QUIT:
            getWindowManager()->closeAllWindows();
            return true;
        case MCMD_MAIN_MENU:
            showMainMenu();
            return true;
        case MCMD_SETTINGS:
            showSettingsMenu();
            return true;
#ifdef WITH_DICT
        case MCMD_DICT:
            CRLog::info("MCMD_DICT activated\n");
            activate_dict(_wm,*_docview);
#endif
        case mm_Settings:
            applySettings();
            return true;
        default:
            // do nothing
            ;
        }
        return CRDocViewWindow::onCommand( command, params );
    }
};

//========= WIN32 =========================================================

#ifdef _WIN32


HWND g_hWnd = NULL;

// Global Variables:
HINSTANCE hInst;                                // current instance

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEXW);

    wcex.style            = 0; //CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = NULL; //LoadIcon(hInstance, (LPCTSTR)IDI_FONTTEST);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL;
    wcex.lpszClassName    = L"CoolReader";
    wcex.hIconSm        = NULL; //LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassExW(&wcex);
}

/// WXWidget support: draw to wxImage
class CRWin32Screen : public CRGUIScreenBase
{
    public:
        HWND _hWnd;
        virtual void draw( HDC hdc )
        {
            LVDrawBuf * drawBuf = getCanvas().get();
            drawBuf->DrawTo( hdc, 0, 0, 0, NULL);
        }
        virtual void paint()
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(_hWnd, &ps);
            draw( hdc );
            EndPaint(_hWnd, &ps);
        }
    protected:
        virtual void update( const lvRect & rc, bool full )
        {
            InvalidateRect(_hWnd, NULL, FALSE);
            UpdateWindow(_hWnd);
        }
    public:
        virtual ~CRWin32Screen()
        {
        }
        CRWin32Screen( HWND hwnd, int width, int height )
        :  CRGUIScreenBase( 0, 0, true )
        {
            _hWnd = hwnd;
            _width = width;
            _height = height;
            _canvas = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );
            _front = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );
        }
};

class CRWin32WindowManager : public CRGUIWindowManager
{
protected:
    HWND _hWnd;
public:
    static CRWin32WindowManager * instance;

    CRWin32WindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL)
    {
       int x=0;
       int y=0;
       lUInt32 flags = 0;
    #ifdef FIXED_JINKE_SIZE
          flags = WS_DLGFRAME | WS_MINIMIZEBOX | WS_SYSMENU | WS_VSCROLL; //WS_OVERLAPPEDWINDOW
          dx = 600 + GetSystemMetrics(SM_CXDLGFRAME)*2
               + GetSystemMetrics(SM_CXVSCROLL);
          dy = 800 + GetSystemMetrics(SM_CYDLGFRAME)*2
               + GetSystemMetrics(SM_CYCAPTION);
    #else
          flags = WS_OVERLAPPEDWINDOW;// | WS_VSCROLL; //
          dx = 500;
          dy = 600;
    #endif

       _hWnd = CreateWindowW(
           L"CoolReader",
           L"CREngine - Simple FB2 viewer",
          flags, //WS_OVERLAPPEDWINDOW
          x, y, dx, dy,
          NULL, NULL, hInst, NULL);

       if (!_hWnd)
       {
          return;
       }

       g_hWnd = _hWnd;

        CRWin32Screen * s = new CRWin32Screen( _hWnd, dx, dy );
        _screen = s;
        _ownScreen = true;
        instance = this;
    }
    // runs event loop
    virtual int runEventLoop()
    {
        ShowWindow( _hWnd, SW_SHOW );
        // Main message loop:
        MSG msg;
        bool stop = false;
        while (!stop && GetMessage(&msg, NULL, 0, 0))
        {
            processPostedEvents();
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            processPostedEvents();
            if ( !getWindowCount() )
                stop = true;
        }
        return 0;
    }
};

CRWin32WindowManager * CRWin32WindowManager::instance = NULL;
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //int wmId, wmEvent;
    switch (message)
    {
        case WM_CREATE:
            {
            }
            break;
        case WM_ERASEBKGND:
            break;
        case WM_VSCROLL:
            break;
        case WM_SIZE:
            {
                if (wParam!=SIZE_MINIMIZED)
                {
                    CRWin32WindowManager::instance->setSize( LOWORD(lParam), HIWORD(lParam) );
                    CRWin32WindowManager::instance->update(true);
                }
            }
            break;
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120     /* Value for rolling one detent */
#endif
        case WM_MOUSEWHEEL:
            {
                /*
              int delta = ((lInt16)HIWORD(wParam))/WHEEL_DELTA;
              if (delta<0)
                  DoCommand( hWnd, DCMD_LINEDOWN, 3 );
              else if (delta>0)
                  DoCommand( hWnd, DCMD_LINEUP, 3 );
                */
            }
            break;
        case WM_CHAR:
            {
                if ( wParam>=' ' && wParam<=127 ) {
                    CRWin32WindowManager::instance->onKeyPressed( wParam, 0 );
                    CRWin32WindowManager::instance->update(true);
                }
            }
            break;
        case WM_KEYDOWN:
            {
                int code = 0;
                switch( wParam )
                {
                case VK_RETURN:
                    code = XK_Return;
                    break;
                case VK_ESCAPE:
                    code = XK_Escape;
                    break;
                case VK_UP:
                    code = XK_Up;
                    break;
                case VK_DOWN:
                    code = XK_Down;
                    break;
                case VK_ADD:
                    code = '+';
                    break;
                case VK_SUBTRACT:
                    code = '-';
                    break;
                }
                if ( code ) {
                    CRWin32WindowManager::instance->onKeyPressed( code, 0 );
                    CRWin32WindowManager::instance->update(true);
                }
            }
            break;
        case WM_LBUTTONDOWN:
            {
                /*
                int xPos = lParam & 0xFFFF;
                int yPos = (lParam >> 16) & 0xFFFF;
                ldomXPointer ptr = text_view->getNodeByPoint( lvPoint( xPos, yPos ) );
                if ( !ptr.isNull() ) {
                    if ( ptr.getNode()->isText() ) {
                        ldomXRange * wordRange = new ldomXRange();
                        if ( ldomXRange::getWordRange( *wordRange, ptr ) ) {
                            wordRange->setFlags( 0x10000 );
                            text_view->getDocument()->getSelections().clear();
                            text_view->getDocument()->getSelections().add( wordRange );
                            text_view->updateSelections();
                            UpdateScrollBar( hWnd );
                        } else {
                            delete wordRange;
                        }
                    }
                }
            */
            }
            break;
        case WM_COMMAND:
            break;
        case WM_PAINT:
            {
                ((CRWin32Screen*)CRWin32WindowManager::instance->getScreen())->paint();
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    CRLog::setFileLogger( "crengine.log" );
    CRLog::setLogLevel( CRLog::LL_TRACE );

    lString8 exe_dir;
    char exe_fn[MAX_PATH+1];
    GetModuleFileNameA( NULL, exe_fn, MAX_PATH );
    InitCREngine( exe_fn );
    //LVCHECKPOINT("WinMain start");

    if (!fontMan->GetFontCount())
    {
        //error
        char str[100];
#if (USE_FREETYPE==1)
        sprintf(str, "Cannot open font file(s) fonts/*.ttf \nCannot work without font\nPlace some TTF files to font\\ directory" );
#else
        sprintf(str, "Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF" );
#endif
        MessageBoxA( NULL, str, "CR Engine :: Fb2Test -- fatal error!", MB_OK);
        return 1;
    }

    lString8 cmdline(lpCmdLine);
    if ( cmdline.empty() )
        return 2; // need filename

    hInst = hInstance;
    MyRegisterClass(hInstance);

    {
        CRWin32WindowManager winman(500, 700);

        V3DocViewWin * main_win = new V3DocViewWin( &winman );
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        static const int acc_table[] = {
            XK_Escape, 0, MCMD_QUIT, 0,
            XK_Return, 0, MCMD_MAIN_MENU, 0, 
            '0', 0, DCMD_PAGEDOWN, 0,
            XK_Down, 0, DCMD_PAGEDOWN, 0,
            '9', 0, DCMD_PAGEUP, 0,
            XK_Up, 0, DCMD_PAGEUP, 0,
            '+', 0, DCMD_ZOOM_IN, 0,
            '=', 0, DCMD_ZOOM_IN, 0,
            '-', 0, DCMD_ZOOM_OUT, 0,
            '_', 0, DCMD_ZOOM_OUT, 0,
            0
        };
        main_win->setAccelerators( CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable( acc_table ) ) );
        winman.activateWindow( main_win );
        if ( !main_win->getDocView()->LoadDocument(cmdline.c_str()) ) {
            char str[100];
            sprintf(str, "Cannot open document file %s", cmdline.c_str());
            MessageBoxA( NULL, str, "CR Engine :: Fb2Test -- fatal error!", MB_OK);
            return 1;
        } else {
            winman.runEventLoop();
        }
    }
    //ShutdownFontManager();

    return 0;
}


#else

#define CR_USE_XCB
#ifdef CR_USE_XCB

#include <unistd.h>      /* pause() */

#include <xcb/xcb.h>
extern "C" {
#include <xcb/shm.h>
};
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define XCB_ALL_PLANES ~0

static xcb_connection_t *connection;
static xcb_window_t window;
static xcb_screen_t *screen;


/// WXWidget support: draw to wxImage
class CRXCBScreen : public CRGUIScreenBase
{
    public:
        xcb_connection_t * getXcbConnection() { return connection; }
        xcb_window_t getXcbWindow() { return window; }
        xcb_screen_t * getXcbScreen() { return screen; }
    protected:
        xcb_gcontext_t      gc;
        xcb_gcontext_t      bgcolor;
        unsigned int pal_[4];
        xcb_drawable_t rect;
        xcb_shm_segment_info_t shminfo;
        xcb_image_t *im;
        unsigned int *pal;
        virtual void update( const lvRect & rc, bool full )
        {
            printf("update screen, bpp=%d width=%d, height=%d\n", (int)im->bpp,im->width,im->height);
            int i;
            i = xcb_image_shm_get (connection, window,
                    im, shminfo,
                    0, 0,
                    XCB_ALL_PLANES);
            if (!i) {
                printf("cannot get shm image\n");
                return;
            }
            printf("update screen, bpp=%d\n", (int)im->bpp);

            // pal
            static lUInt32 pal[4] = {0x000000, 0x555555, 0xaaaaaa, 0xffffff };
            switch ( im->bpp ) {
            case 32:
                {
                    for ( int y = rc.top; y<rc.bottom; y++ ) {
                        lUInt8 * src = _front->GetScanLine( y );
                        lUInt32 * dst = (lUInt32 *)(im->data + im->stride * y);
                        //printf("line %d : %08X -> %08X   ", y, src, dst);
                        int shift = 6;
                        for ( int x = 0; x< _width; x++ ) {
                            lUInt8 data = src[x>>2];
                            int pixel = (data>>shift) & 3;
                            lUInt32 color = pal[ pixel ]; // to check valgrind finding
                            dst[x] = color;
                            shift -= 2;
                            if ( shift < 0 )
                                shift = 6;
                        }
                    }
                }
                break;
            case 2:
                {
                    for ( int y = rc.top; y<rc.bottom; y++ ) {
                        lUInt8 * src = _front->GetScanLine( y );
                        lUInt8 * dst = (lUInt8 *)(im->data + im->stride * y);
                        memcpy( dst, src, _width>>2 );
                    }
                }
                break;
            }
            //pContext.image = im;
            printf("updated\n");

            //view()->paint();

            xcb_image_shm_put (connection, window, gc,
                    im, shminfo,
                    0, 0, 0, 0, _width, _height, 0);
            xcb_flush(connection);
        }
    public:
        virtual ~CRXCBScreen()
        {
            if ( connection )
                xcb_disconnect( connection );
        }
        CRXCBScreen( int width, int height )
        :  CRGUIScreenBase( 0, 0, true )
        {
            xcb_screen_iterator_t screen_iter;
            const xcb_setup_t    *setup;
            xcb_generic_event_t  *e;
            xcb_generic_error_t  *error;
            xcb_void_cookie_t     cookie_window;
            xcb_void_cookie_t     cookie_map;
            uint32_t              mask;
            uint32_t              values[2];
            int                   screen_number;
            uint8_t               is_hand = 0;

            /* getting the connection */
            connection = xcb_connect (NULL, &screen_number);
            if (xcb_connection_has_error(connection)) {
                fprintf (stderr, "ERROR: can't connect to an X server\n");
                exit(-1);
            }

            screen = xcb_aux_get_screen (connection, screen_number);
            if ( width <= 0 || width > screen->width_in_pixels )
                width = screen->width_in_pixels;
            if ( height <= 0 || height > screen->height_in_pixels )
                height = screen->height_in_pixels;
            xcb_rectangle_t rect_coord = { 0, 0, width, height};

            gc = xcb_generate_id (connection);
            mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
            values[0] = screen->black_pixel;
            values[1] = 0; /* no graphics exposures */
            xcb_create_gc (connection, gc, screen->root, mask, values);

            bgcolor = xcb_generate_id (connection);
            mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
            values[0] = screen->white_pixel;
            values[1] = 0; /* no graphics exposures */
            xcb_create_gc (connection, bgcolor, screen->root, mask, values);

            /* creating the window */
            window = xcb_generate_id(connection);
            mask =  XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
            xcb_params_cw_t params_cw;
            params_cw.back_pixel = screen->white_pixel;
            params_cw.event_mask =
                XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_EXPOSURE |
                XCB_EVENT_MASK_POINTER_MOTION;

            uint8_t depth = xcb_aux_get_depth (connection, screen);
            printf("depth = %d, root depth = %d\n",depth, screen->root_depth);
            xcb_aux_create_window(connection,
                    depth,
                    window, screen->root,
                    0, 0, width, height,
                    0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    mask, &params_cw);

            rect = xcb_generate_id (connection);
            
            xcb_void_cookie_t cookie;
            cookie = xcb_create_pixmap_checked (connection, depth,
                    rect, window,
                    width, height);
            if (xcb_request_check(connection,cookie)){
                printf("sucks, can't creae pixmap\n");
            }

            xcb_map_window(connection, window);

            xcb_colormap_t    colormap;
            colormap = screen->default_colormap;

            xcb_alloc_color_reply_t *rep;
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0, 0, 0), NULL);
            pal_[0] = rep->pixel;
            free(rep);
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0x55<<8, 0x55<<8, 0x55<<8), NULL);
            pal_[1] = rep->pixel;
            free(rep);
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xaa<<8, 0xaa<<8, 0xaa<<8), NULL);
            pal_[2] = rep->pixel;
            free(rep);
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xff<<8, 0xff<<8, 0xff<<8), NULL);
            pal_[3] = rep->pixel;
            free(rep);

            pal = pal_;

            xcb_shm_query_version_reply_t *rep_shm;

            rep_shm = xcb_shm_query_version_reply (connection,
                    xcb_shm_query_version (connection),
                    NULL);
            if(rep_shm) {
                xcb_image_format_t format;
                int shmctl_status;

                if (rep_shm->shared_pixmaps &&
                        (rep_shm->major_version > 1 || rep_shm->minor_version > 0))
                    format = (xcb_image_format_t)rep_shm->pixmap_format;
                else
                    format = (xcb_image_format_t)0;

                im = xcb_image_create_native (connection, width, height,
                        format, depth, NULL, ~0, NULL);
                assert(im);

                shminfo.shmid = shmget (IPC_PRIVATE,
                        im->stride*im->height,
                        IPC_CREAT | 0777);
                assert(shminfo.shmid != -1);
                shminfo.shmaddr = (uint8_t*)shmat (shminfo.shmid, 0, 0);
                assert(shminfo.shmaddr);
                im->data = shminfo.shmaddr;
                printf("Created image depth=%d bpp=%d stride=%d\n", (int)im->depth, (int)im->bpp, (int)im->stride );

                shminfo.shmseg = xcb_generate_id (connection);
                xcb_shm_attach (connection, shminfo.shmseg,
                        shminfo.shmid, 0);
                shmctl_status = shmctl(shminfo.shmid, IPC_RMID, 0);
                assert(shmctl_status != -1);
                free (rep_shm);

            } else {
                printf("Can't get shm\n");
            }
            _width = width;
            _height = height;
            _canvas = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );
            _front = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );

            xcb_flush(connection);
            printf("Created screen %d x %d, depth = %d\n", _width, _height, depth );
        }
};


class CRXCBWindowManager : public CRGUIWindowManager
{
protected:
    xcb_connection_t * _connection;
public:
    CRXCBWindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL)
    {
        CRXCBScreen * s = new CRXCBScreen( dx, dy );
        _screen = s;
        _connection = s->getXcbConnection();
        _ownScreen = true;
    }
    // runs event loop
    virtual int runEventLoop()
    {
        xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc( connection );

        xcb_generic_event_t *event;
        bool stop = false;
        while (!stop && (event = xcb_wait_for_event (connection)) ) {
            bool needUpdate = false;
            //processPostedEvents();
            switch (event->response_type & ~0x80) {
            case XCB_EXPOSE:
                // draw buffer
                {
                    update(true);
                }
                break;
            case XCB_KEY_RELEASE:
                {
                    xcb_key_press_event_t *release = (xcb_key_press_event_t *)event;
                    xcb_keycode_t key = release->detail;
                    int state = release->state;
                    xcb_keysym_t sym = xcb_key_symbols_get_keysym( keysyms,
                                            key,
                                            xcb_lookup_chars_t); //xcb_lookup_key_sym_t xcb_lookup_chars_t
                    printf("Key released keycode=%d char=%04x\n", (int)key, (int)sym );
                    if ( sym==XK_Escape ) {
                        stop = true;
                        break;
                    }
                    int cmd = 0;
#if 0
                    switch ( sym ) {
                    case '0':
                    case XK_Down:
                        cmd = DCMD_PAGEDOWN;
                        break;
                    case '9':
                    case XK_Up:
                        cmd = DCMD_PAGEUP;
                        break;
                    case '+':
                    case '-':
                        cmd = DCMD_ZOOM_IN;
                        break;
                    case '-':
                        cmd = DCMD_ZOOM_OUT;
                        break;
                    }
#endif
                    if ( cmd ) {
                        onCommand( cmd, 0 );
                    } else {
                        onKeyPressed( sym, state );
                    }
                    //printf("page number = %d\n", main_win->getDocView()->getCurPage());
                    needUpdate = true;
                }
                break;
            case XCB_BUTTON_PRESS:
                {
                    xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
                }
                break;
            default:
                /* Unknown event type, ignore it */
                break;
            }

            free (event);

            if ( processPostedEvents() || needUpdate )
                update(true);
            // stop loop if all windows are closed
            if ( !getWindowCount() )
                stop = true;

        }

        xcb_key_symbols_free( keysyms );
    }
};


int main(int argc, char **argv)
{
    CRLog::setStdoutLogger();
    CRLog::setLogLevel( CRLog::LL_TRACE );
    #if 0
    // memory leak test
    {
        {
            lString8 s;
            s << "bla bla bla";
            lString8 s2("xxxxx");
            s << s2;
            lString8 * array = new lString8[25];
            array[2] = lString8("text1");
            array[6] = lString8("text2");
            array[24] = lString8("text3");
            for ( int k=0; k<10000; k++ )
                array[7] << "123";
            typedef LVRef<int> IntRef;
            delete [] array;
            {
                LVCacheMap <int, IntRef> map( 20 );
                map.set(1, IntRef( new int(3) ));
                map.set(2, IntRef( new int(4) ));
            }
            lString8 buf;
            lStringBuf8<100> proxy( buf );
            for ( int i=0; i<5000; i++ )
                buf << 'A';
        }
        ShutdownCREngine();
        return 0;
    }
    #endif


    if ( !InitCREngine( argv[0] ) ) {
        printf("Cannot init CREngine - exiting\n");
        return 2;
    }

    if ( argc!=2 ) {
        printf("Usage: cr3 <filename_to_open>\n");
        return 3;
    }

    const char * fname = argv[1];

    int res = 0;

    {
        CRXCBWindowManager winman( 600, 700 );
        V3DocViewWin * main_win = new V3DocViewWin( &winman );
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        static const int acc_table[] = {
            XK_Escape, 0, MCMD_QUIT, 0,
            XK_Return, 0, MCMD_MAIN_MENU, 0, 
            '0', 0, DCMD_PAGEDOWN, 0,
            XK_Down, 0, DCMD_PAGEDOWN, 0,
            '9', 0, DCMD_PAGEUP, 0,
#ifdef WITH_DICT
            '2', 0, MCMD_DICT, 0,
#endif
            XK_Up, 0, DCMD_PAGEUP, 0,
            '+', 0, DCMD_ZOOM_IN, 0,
            '=', 0, DCMD_ZOOM_IN, 0,
            '-', 0, DCMD_ZOOM_OUT, 0,
            '_', 0, DCMD_ZOOM_OUT, 0,
            0
        };
        main_win->setAccelerators( CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable( acc_table ) ) );
        winman.activateWindow( main_win );
        if ( !main_win->getDocView()->LoadDocument(fname) ) {
            printf("Cannot open book file %s\n", fname);
            res = 4;
        } else {
            winman.runEventLoop();
        }
    }
    ShutdownCREngine();

    return res;
}

#endif
#endif
