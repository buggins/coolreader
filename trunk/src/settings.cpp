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

#include "settings.h"


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

DECL_DEF_CR_FONT_SIZES;

CRMenu * CRSettingsMenu::createFontSizeMenu( CRMenu * mainMenu, CRPropRef props )
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
    fontSizeMenu->setSkinName(lString16(L"SettingsMenu"));
    return fontSizeMenu;
}

void CRSettingsMenu::addMenuItems( CRMenu * menu, item_def_t values[], LVFontRef menuFont )
{
    for ( int i=0; values[i].translate_label; i++)
        menu->addItem( new CRMenuItem( menu, i,
            _wm->translateString(values[i].translate_label, values[i].translate_default),
            LVImageSourceRef(), 
            menuFont, Utf8ToUnicode(lString8(values[i].value)).c_str() ) );
    menu->setAccelerators( _menuAccelerators );
    menu->setSkinName(lString16(L"SettingsMenu"));
}


CRSettingsMenu::CRSettingsMenu( CRGUIWindowManager * wm, CRPropRef newProps, int id, LVFontRef font, CRGUIAcceleratorTableRef menuAccelerators )
: CRMenu( wm, NULL, id, lString16(L"Settings"),
            LVImageSourceRef(),
            font,
            font ),
  props( newProps ),
  _menuAccelerators( menuAccelerators )
{
        CRLog::trace("showSettingsMenu() - %d property values found", props->getCount() );

        setSkinName(lString16(L"SettingsMenu"));

        LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
        LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 300, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * mainMenu = this;
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

}


