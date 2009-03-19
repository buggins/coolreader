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

#include <cri18n.h>



DECL_DEF_CR_FONT_SIZES;

CRMenu * CRSettingsMenu::createOrientationMenu( CRMenu * mainMenu, CRPropRef props )
{
	item_def_t page_orientations[] = {
		{_("Normal"), "0"},
		{_("90 `"), "1"},
		{_("180 `"), "2"},
		{_("270 `"), "3"},
		{NULL, NULL},
	};

    LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 300, true, css_ff_sans_serif, lString8("Arial")) );
    CRMenu * orientationMenu = new CRMenu(_wm, mainMenu, mm_Orientation,
            _16("Page orientation"),
                            LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_ROTATE_ANGLE );
    addMenuItems( orientationMenu, page_orientations );
    return orientationMenu;
}

CRMenu * CRSettingsMenu::createFontSizeMenu( CRMenu * mainMenu, CRPropRef props )
{
    lString16Collection list;
    fontMan->getFaceList( list );
    lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
    //LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, false, css_ff_sans_serif, lString8("Arial")) );
    LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 300, true, css_ff_sans_serif, lString8("Arial")) );
    CRMenu * fontSizeMenu;
    fontSizeMenu = new CRMenu(_wm, mainMenu, mm_FontSize,
                                _("Default font size"),
                                        LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_SIZE );
	for ( unsigned i=0; i<sizeof(cr_font_sizes)/sizeof(int); i++ ) {
		//char name[32];
		char defvalue[32];
		//sprintf( name, "VIEWER_DLG_FONT_SIZE_%d", cr_font_sizes[i] );
		sprintf( defvalue, "%d", cr_font_sizes[i] );
		fontSizeMenu->addItem( new CRMenuItem( fontSizeMenu, 0,
								lString16(defvalue),
										LVImageSourceRef(), fontMan->GetFont( cr_font_sizes[i], 300, false, css_ff_sans_serif, fontFace), lString16::itoa(cr_font_sizes[i]).c_str()  ) );
	}
    fontSizeMenu->setAccelerators( _menuAccelerators );
    fontSizeMenu->setSkinName(lString16(L"#settings"));
    return fontSizeMenu;
}

void CRSettingsMenu::addMenuItems( CRMenu * menu, item_def_t values[] )
{
    for ( int i=0; values[i].translate_default; i++)
        menu->addItem( new CRMenuItem( menu, i,
            lString16(values[i].translate_default),
            LVImageSourceRef(),
            LVFontRef(), Utf8ToUnicode(lString8(values[i].value)).c_str() ) );
    menu->setAccelerators( _menuAccelerators );
    menu->setSkinName(lString16(L"#settings"));
}


CRSettingsMenu::CRSettingsMenu( CRGUIWindowManager * wm, CRPropRef newProps, int id, LVFontRef font, CRGUIAcceleratorTableRef menuAccelerators )
: CRMenu( wm, NULL, id, lString16(_("Settings")),
            LVImageSourceRef(),
            font,
            font ),
  props( newProps ),
  _menuAccelerators( menuAccelerators )
{
	item_def_t antialiasing_modes[] = {
		{_("On for all fonts"), "2"},
		{_("On for big fonts only"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t embedded_styles[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t bookmark_icons[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t kerning_options[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t footnotes[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t showtime[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t preformatted_text[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t inverse_mode[] = {
		{_("Normal"), "0"},
		{_("Inverse"), "1"},
		{NULL, NULL},
	};

	item_def_t landscape_pages[] = {
		{_("One"), "1"},
		{_("Two"), "2"},
		{NULL, NULL},
	};

	item_def_t status_line[] = {
		{_("Top"), "0"},
	//    {"VIEWER_DLG_STATUS_LINE_BOTTOM", "Bottom", "1"},
		{_("Off"), "2"},
		{NULL, NULL},
	};

	item_def_t interline_spaces[] = {
		{_("90%"), "90"},
		{_("100%"), "100"},
		{_("110%"), "110"},
		{_("120%"), "120"},
		{_("140%"), "140"},
		{NULL, NULL},
	};

	item_def_t page_margins[] = {
		{"0", "0"},
		{"5", "5"},
		{"8", "8"},
		{"10", "10"},
		{"15", "15"},
		{"20", "20"},
		{"25", "25"},
		{"30", "30"},
		{NULL, NULL},
	};



	CRLog::trace("showSettingsMenu() - %d property values found", props->getCount() );

        setSkinName(lString16(L"#settings"));

        LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 300, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * mainMenu = this;
        mainMenu->setAccelerators( _menuAccelerators );

        CRMenu * fontFaceMenu = new CRMenu(_wm, mainMenu, mm_FontFace,
                                            _("Default font face"),
                                                    LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_FACE );
        fontFaceMenu->setSkinName(lString16(L"#settings"));
        CRLog::trace("getting font face list");
        lString16Collection list;
        fontMan->getFaceList( list );
        CRLog::trace("faces found: %d", list.length());
        int i;
        for ( i=0; i<(int)list.length(); i++ ) {
            fontFaceMenu->addItem( new CRMenuItem( fontFaceMenu, i,
                                    list[i], LVImageSourceRef(), fontMan->GetFont( MENU_FONT_SIZE, 300,
									false, css_ff_sans_serif, UnicodeToUtf8(list[i])), list[i].c_str() ) );
        }
        fontFaceMenu->setAccelerators( _menuAccelerators );
        //lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
        mainMenu->addItem( fontFaceMenu );

        CRMenu * fontSizeMenu = createFontSizeMenu( mainMenu, props );
        mainMenu->addItem( fontSizeMenu );

        CRMenu * fontAntialiasingMenu = new CRMenu(_wm, mainMenu, mm_FontAntiAliasing,
                _("Font antialiasing"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_ANTIALIASING );
        addMenuItems( fontAntialiasingMenu, antialiasing_modes );
        mainMenu->addItem( fontAntialiasingMenu );

        CRMenu * interlineSpaceMenu = new CRMenu(_wm, mainMenu, mm_InterlineSpace,
                _("Interline space"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_INTERLINE_SPACE );
        addMenuItems( interlineSpaceMenu, interline_spaces );
        mainMenu->addItem( interlineSpaceMenu );

        CRMenu * orientationMenu = createOrientationMenu(mainMenu, props);
        mainMenu->addItem( orientationMenu );

        CRMenu * footnotesMenu = new CRMenu(_wm, mainMenu, mm_Footnotes,
                _("Footnotes at page bottom"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FOOTNOTES );
        addMenuItems( footnotesMenu, footnotes );
        mainMenu->addItem( footnotesMenu );

/*
        SetTimeMenuItem * setTime = new SetTimeMenuItem( mainMenu, mm_SetTime, _wm->translateString("VIEWER_MENU_SET_TIME", "Set time"),
                LVImageSourceRef(), menuFont, L"bla" );
        mainMenu->addItem( setTime );
*/
        CRMenu * showTimeMenu = new CRMenu(_wm, mainMenu, mm_ShowTime,
                _("Show time"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_SHOW_TIME );
        addMenuItems( showTimeMenu, showtime );
        mainMenu->addItem( showTimeMenu );

        CRMenu * landscapePagesMenu = new CRMenu(_wm, mainMenu, mm_LandscapePages,
                _("Landscape pages"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_LANDSCAPE_PAGES );
        addMenuItems( landscapePagesMenu, landscape_pages );
        mainMenu->addItem( landscapePagesMenu );

        CRMenu * preformattedTextMenu = new CRMenu(_wm, mainMenu, mm_PreformattedText,
                _("Preformatted text"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_TXT_OPTION_PREFORMATTED );
        addMenuItems( preformattedTextMenu, preformatted_text );
        mainMenu->addItem( preformattedTextMenu );

        //

        CRMenu * embeddedStylesMenu = new CRMenu(_wm, mainMenu, mm_EmbeddedStyles,
                _("Document embedded styles"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_EMBEDDED_STYLES );
        addMenuItems( embeddedStylesMenu, embedded_styles );
        mainMenu->addItem( embeddedStylesMenu );

        CRMenu * inverseModeMenu = new CRMenu(_wm, mainMenu, mm_Inverse,
                _("Inverse display"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_DISPLAY_INVERSE );
        addMenuItems( inverseModeMenu, inverse_mode );
        mainMenu->addItem( inverseModeMenu );

        CRMenu * bookmarkIconsMenu = new CRMenu(_wm, mainMenu, mm_BookmarkIcons,
                _("Show bookmark icons"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_BOOKMARK_ICONS );
        addMenuItems( bookmarkIconsMenu, bookmark_icons );
        mainMenu->addItem( bookmarkIconsMenu );

        CRMenu * statusLineMenu = new CRMenu(_wm, mainMenu, mm_StatusLine,
                _("Status line"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_STATUS_LINE );
        addMenuItems( statusLineMenu, status_line );
        mainMenu->addItem( statusLineMenu );

        CRMenu * kerningMenu = new CRMenu(_wm, mainMenu, mm_Kerning,
                _("Font kerning"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_KERNING_ENABLED );
        addMenuItems( kerningMenu, kerning_options );
        mainMenu->addItem( kerningMenu );

        //====== Margins ==============
        CRMenu * marginsMenu = new CRMenu(_wm, mainMenu, mm_PageMargins,
                _("Page margins"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props );
        CRMenu * marginsMenuTop = new CRMenu(_wm, marginsMenu, mm_PageMarginTop,
                _("Top margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_TOP );
        addMenuItems( marginsMenuTop, page_margins );
        marginsMenu->addItem( marginsMenuTop );
        CRMenu * marginsMenuBottom = new CRMenu(_wm, marginsMenu, mm_PageMarginBottom,
                _("Bottom margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_BOTTOM );
        addMenuItems( marginsMenuBottom, page_margins );
        marginsMenu->addItem( marginsMenuBottom );
        CRMenu * marginsMenuLeft = new CRMenu(_wm, marginsMenu, mm_PageMarginLeft,
                _("Left margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_LEFT );
        addMenuItems( marginsMenuLeft, page_margins );
        marginsMenu->addItem( marginsMenuLeft );
        CRMenu * marginsMenuRight = new CRMenu(_wm, marginsMenu, mm_PageMarginRight,
                _("Right margin"),
                 LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_PAGE_MARGIN_RIGHT );
        addMenuItems( marginsMenuRight, page_margins );
        marginsMenu->addItem( marginsMenuRight );
		marginsMenu->setAccelerators( _menuAccelerators );
		marginsMenu->setSkinName(lString16(L"#settings"));
        mainMenu->addItem( marginsMenu );
        
}


