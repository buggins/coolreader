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
#include <crgui.h>
#include "viewdlg.h"
#include "mainwnd.h"
//#include "fsmenu.h"
#ifdef CR_POCKETBOOK
#include "cr3pocketbook.h"
#endif

#include <cri18n.h>


class CRControlsMenu;
class CRControlsMenuItem : public CRMenuItem
{
private:
    int _key;
    int _flags;
    int _command;
    int _params;
    CRControlsMenu * _controlsMenu;
    lString16 _settingKey;
    int _defCommand;
    int _defParams;
public:
    bool getCommand( int & cmd, int & params )
    {
        lString16 v = _menu->getProps()->getStringDef(LCSTR(_settingKey), "");
        cmd = _defCommand;
        params = _defParams;
        return splitIntegerList( v, lString16(","), cmd, params );
   }
    /// submenu for options dialog support
    virtual lString16 getSubmenuValue()
    {
        int cmd;
        int params;
        bool isSet = getCommand( cmd, params );
        lString16 res = Utf8ToUnicode(lString8(getCommandName( cmd, params )));
        // TODO: use default flag
        return res;
    }
    /// called on item selection
    virtual int onSelect();
    CRControlsMenuItem( CRControlsMenu * menu, int id, int key, int flags, const CRGUIAccelerator * defAcc );
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected );
};

class CRControlsMenu : public CRFullScreenMenu
{
    lString16 _accelTableId;
    CRGUIAcceleratorTableRef _baseAccs;
    CRGUIAcceleratorTableRef _overrideCommands;
    lString16 _settingKey;
public:
    CRPropRef getProps() { return _props; }
    lString16 getSettingKey( int key, int flags )
    {
        lString16 res = _settingKey;
        if ( key!=0 )
            res << "." << fmt::decimal(key) << "." << fmt::decimal(flags);
        return res;
    }
    lString16 getSettingLabel( int key, int flags )
    {
        return lString16(getKeyName(key, flags));
    }
    CRMenu * createCommandsMenu(int key, int flags)
    {
        lString16 label = getSettingLabel( key, flags ) + " - " + lString16(_("choose command"));
        lString16 keyid = getSettingKey( key, flags );
        CRMenu * menu = new CRMenu(_wm, this, _id, label, LVImageSourceRef(), LVFontRef(), LVFontRef(), _props, LCSTR(keyid), 8);
        for ( int i=0; i<_overrideCommands->length(); i++ ) {
            const CRGUIAccelerator * acc = _overrideCommands->get(i);
            lString16 cmdLabel = lString16( getCommandName(acc->commandId, acc->commandParam) );
            lString16 cmdValue = lString16::itoa(acc->commandId) << "," << lString16::itoa(acc->commandParam);
            CRMenuItem * item = new CRMenuItem( menu, i, cmdLabel, LVImageSourceRef(), LVFontRef(), cmdValue.c_str());
            menu->addItem(item);
        }
        menu->setAccelerators( getAccelerators() );
        menu->setSkinName(lString16("#settings"));
        menu->setValueFont(_valueFont);
        menu->setFullscreen(true);
        menu->reconfigure( 0 );
        return menu;
    }
    CRControlsMenu(CRMenu * baseMenu, int id, CRPropRef props, lString16 accelTableId, int numItems, lvRect & rc)
    : CRFullScreenMenu( baseMenu->getWindowManager(), id, lString16(_("Controls")), numItems, rc )
    {
        _menu = baseMenu;
        _props = props;
        _baseAccs = _wm->getAccTables().get( accelTableId );
        if (_baseAccs.isNull()) {
            CRLog::error("CRControlsMenu: No accelerators %s", LCSTR(_accelTableId) );
        }
        _accelTableId = accelTableId;
        CRGUIAcceleratorTableRef _overrideKeys = _wm->getAccTables().get( accelTableId + "-override-keys" );
        if ( _overrideKeys.isNull() ) {
            CRLog::error("CRControlsMenu: No table of allowed keys for override accelerators %s", LCSTR(_accelTableId) );
            return;
        }
        _overrideCommands = _wm->getAccTables().get( accelTableId + "-override-commands" );
        if ( _overrideCommands.isNull() ) {
            CRLog::error("CRControlsMenu: No table of allowed commands to override accelerators %s", LCSTR(_accelTableId) );
            return;
        }
        _settingKey = lString16("keymap.") + _accelTableId;
        for ( int i=0; i<_overrideKeys->length(); i++ ) {
            const CRGUIAccelerator * acc = _overrideKeys->get(i);
            CRControlsMenuItem * item = new CRControlsMenuItem(this, i, acc->keyCode, acc->keyFlags,
                         _baseAccs->findKeyAccelerator( acc->keyCode, acc->keyFlags ) );
            addItem(item);
        }
    }

    virtual bool onCommand( int command, int params )
    {
        switch ( command ) {
        case mm_Controls:
            return true;
        default:
            return CRMenu::onCommand( command, params );
        }
    }
};

/// called on item selection
int CRControlsMenuItem::onSelect()
{
    CRMenu * menu = _controlsMenu->createCommandsMenu(_key, _flags);
    _menu->getWindowManager()->activateWindow(menu);
    return 1;
}

CRControlsMenuItem::CRControlsMenuItem( CRControlsMenu * menu, int id, int key, int flags, const CRGUIAccelerator * defAcc )
: CRMenuItem(menu, id, getKeyName(key, flags), LVImageSourceRef(), LVFontRef() ), _key( key ), _flags(flags)
{
    _defCommand = _defParams = 0;
    if ( defAcc ) {
        _defCommand = defAcc->commandId;
        _defParams = defAcc->commandParam;
    }
    _controlsMenu = menu;
    _settingKey = menu->getSettingKey( key, flags );
}

void CRControlsMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected )
{
    lvRect itemBorders = skin->getBorderWidths();
    skin->draw( buf, rc );
    buf.SetTextColor( skin->getTextColor() );
    buf.SetBackgroundColor( skin->getBackgroundColor() );
    lvRect textRect = rc;
    lvRect borders = skin->getBorderWidths();
    //textRect.shrinkBy(borders);
    skin->drawText(buf, textRect, _label);
    lString16 s = getSubmenuValue();
    if ( s.empty() )
        s = lString16(_("[Command is not assigned]"));
    //_menu->getValueFont()->DrawTextString( &buf, rc.right - w - 8, rc.top + hh/2 - _menu->getValueFont()->getHeight()/2, s.c_str(), s.length(), L'?', NULL, false, 0 );
    valueSkin->drawText(buf, textRect, s);
}

bool CRSettingsMenu::onCommand( int command, int params )
{
    switch ( command ) {
    case mm_Controls:
        {
        }
        return true;
    default:
        return CRMenu::onCommand( command, params );
    }
}

#if CR_INTERNAL_PAGE_ORIENTATION==1 || defined(CR_POCKETBOOK)
CRMenu * CRSettingsMenu::createOrientationMenu( CRMenu * mainMenu, CRPropRef props )
{
	item_def_t page_orientations[] = {
		{_("0` (Portrait)"), "0"},
		{_("90 `"), "1"},
		{_("180 `"), "2"},
		{_("270 `"), "3"},
		{NULL, NULL},
	};

    LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
#ifdef CR_POCKETBOOK
	const char * propName = PROP_POCKETBOOK_ORIENTATION;
#else
	const char * propName = PROP_ROTATE_ANGLE;
#endif
    CRMenu * orientationMenu = new CRMenu(_wm, mainMenu, mm_Orientation,
            lString16(_("Page orientation")),
                            LVImageSourceRef(), LVFontRef(), valueFont, props,  propName);
    addMenuItems( orientationMenu, page_orientations );
    orientationMenu->reconfigure( 0 );

    return orientationMenu;
}
#endif

DECL_DEF_CR_FONT_SIZES;

class FontSizeMenu : public CRMenu
{
public:
    FontSizeMenu(  CRGUIWindowManager * wm, CRMenu * parentMenu, LVFontRef valueFont, CRPropRef props  )
    : CRMenu( wm, parentMenu, mm_FontSize,
                                _("Default font size"),
                                        LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_SIZE, 10 )
    {
        _fullscreen = true;
    }

    /// submenu for options dialog support
    virtual lString16 getSubmenuValue()
    { 
        return getProps()->getStringDef(
            UnicodeToUtf8(getPropName()).c_str(), "32");
    }
};

class OnDemandFontMenuItem : public CRMenuItem
{
    int _size;
    int _weight;
    bool _italic;
    css_font_family_t _family;
    lString8 _typeface;
    lString8 _deftypeface;
    public:
    OnDemandFontMenuItem( CRMenu * menu, int id, lString16 label, LVImageSourceRef image, const lChar16 * propValue,
                          int size, int weight, bool italic, css_font_family_t family, lString8 typeface, lString8 deftypeface )
    : CRMenuItem(menu, id, LCSTR(label), image, LVFontRef(), propValue)
    , _size(size), _weight(weight), _italic(italic), _family(family), _typeface(typeface), _deftypeface(deftypeface)
    {
    }
    /// item label font
    virtual LVFontRef getFont()
    {
        if ( _defFont.isNull() ) {
            CRLog::trace("Creating font %s[%d] on demand", _typeface.c_str(), _size);
            _defFont = fontMan->GetFont( _size, _weight, _italic, _family, _typeface);
            LVFont::glyph_info_t glyph;
            if ( !_defFont->getGlyphInfo('A', &glyph) ) {
                _defFont = fontMan->GetFont( _size, _weight, _italic, _family, _deftypeface);
            }
        }
        return _defFont;
    }
};

CRMenu * CRSettingsMenu::createFontSizeMenu( CRGUIWindowManager * wm, CRMenu * mainMenu, CRPropRef props )
{
    lString16Collection list;
    fontMan->getFaceList( list );
    lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
    //LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, false, css_ff_sans_serif, lString8("Arial")) );
    CRMenuSkinRef skin = wm->getSkin()->getMenuSkin(L"#settings");
    LVFontRef valueFont = skin->getValueSkin()->getFont();//( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
    CRMenu * fontSizeMenu;
    fontSizeMenu = new FontSizeMenu(_wm, mainMenu, valueFont, props );
    for ( unsigned i=0; i<sizeof(cr_font_sizes)/sizeof(int); i++ ) {
        //char name[32];
        char defvalue[400];
        //sprintf( name, "VIEWER_DLG_FONT_SIZE_%d", cr_font_sizes[i] );
        sprintf( defvalue, "%d %s", cr_font_sizes[i], _("The quick brown fox jumps over lazy dog") );
        fontSizeMenu->addItem( new OnDemandFontMenuItem( fontSizeMenu, 0,
                        lString16(defvalue),
                        LVImageSourceRef(), lString16::itoa(cr_font_sizes[i]).c_str(),
                        cr_font_sizes[i], 400, false, css_ff_sans_serif, fontFace, UnicodeToUtf8(skin->getItemSkin()->getFontFace())) );
    }
    fontSizeMenu->setAccelerators( _wm->getAccTables().get("menu") );
    //fontSizeMenu->setAccelerators( _menuAccelerators );
    fontSizeMenu->setSkinName(lString16("#settings"));
    //fontSizeMenu->setSkinName(lString16("#main"));
    fontSizeMenu->reconfigure( 0 );
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
    menu->setSkinName(lString16("#settings"));
    menu->reconfigure( 0 );
}


CRSettingsMenu::CRSettingsMenu( CRGUIWindowManager * wm, CRPropRef newProps, int id, LVFontRef font, CRGUIAcceleratorTableRef menuAccelerators, lvRect &rc )
: CRFullScreenMenu( wm, id, lString16(_("Settings")), 8, rc ),
  props( newProps ),
  _menuAccelerators( menuAccelerators )
{
    setSkinName(lString16("#settings"));

    _fullscreen = true;

	item_def_t antialiasing_modes[] = {
		{_("On for all fonts"), "2"},
		{_("On for big fonts only"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

        item_def_t fonthinting_modes[] = {
                {_("Autohinting"), "2"},
                {_("Use bytecode"), "1"},
                {_("Disable"), "0"},
                {NULL, NULL},
        };

	item_def_t embedded_styles[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};

	item_def_t embedded_fonts[] = {
		{_("On"), "1"},
		{_("Off"), "0"},
		{NULL, NULL},
	};
        item_def_t highlight_bookmark[] = {
                {_("None"), "0"},
                {_("Solid"), "1"},
                {_("Underline"), "2"},
                {NULL, NULL},
        };

        item_def_t fontgamma_list[] = {
                {("0.30"), "0.30"},
                {("0.35"), "0.35"},
                {("0.40"), "0.40"},
                {("0.45"), "0.45"},
                {("0.50"), "0.50"},
                {("0.55"), "0.55"},
                {("0.60"), "0.60"},
                {("0.65"), "0.65"},
                {("0.70"), "0.70"},
                {("0.75"), "0.75"},
                {("0.80"), "0.80"},
                {("0.85"), "0.85"},
                {("0.90"), "0.90"},
                {("0.95"), "0.95"},
                {("0.98"), "0.98"},
                {("1.00"), "1.00"},
                {("1.02"), "1.02"},
                {("1.05"), "1.05"},
                {("1.10"), "1.10"},
                {("1.15"), "1.15"},
                {("1.20"), "1.20"},
                {("1.25"), "1.25"},
                {("1.30"), "1.30"},
                {("1.35"), "1.35"},
                {("1.40"), "1.40"},
                {("1.45"), "1.45"},
                {("1.50"), "1.50"},
                {("1.60"), "1.60"},
                {("1.70"), "1.70"},
                {("1.80"), "1.80"},
                {("1.90"), "1.90"},
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

    item_def_t floating_punctuation_options[] = {
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

    item_def_t embolden_mode[] = {
        {_("Normal"), "0"},
        {_("Bold"), "1"},
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
        {_("70%"), "70"},
        {_("75%"), "75"},
        {_("80%"), "80"},
        {_("85%"), "85"},
        {_("90%"), "90"},
        {_("95%"), "95"},
        {_("100%"), "100"},
        {_("105%"), "105"},
        {_("110%"), "110"},
        {_("115%"), "115"},
        {_("120%"), "120"},
        {_("125%"), "125"},
        {_("130%"), "130"},
        {_("135%"), "135"},
        {_("140%"), "140"},
        {_("145%"), "145"},
        {_("150%"), "150"},
        {_("160%"), "160"},
        {_("180%"), "180"},
        {_("200%"), "200"},
        {NULL, NULL},
	};

	item_def_t page_margins[] = {
#if BIG_PAGE_MARGINS==1
    //static int def_margin[] = { 8, 0, 5, 10, 20, 30, 50, 60 };
    {"0", "0"},
    {"1", "1"},
    {"2", "2"},
    {"3", "3"},
    {"4", "4"},
    {"5", "5"},
    {"8", "8"},
    {"10", "10"},
    {"12", "12"},
    {"14", "14"},
    {"16", "16"},
    {"20", "20"},
    {"30", "30"},
    {"50", "50"},
    {"60", "60"},
    {"80", "80"},
    {"100", "100"},
    {"130", "130"},
#else
        {"0", "0"},
		{"5", "5"},
		{"8", "8"},
		{"10", "10"},
		{"15", "15"},
		{"20", "20"},
		{"25", "25"},
		{"30", "30"},
#endif
		{NULL, NULL},
	};

    item_def_t screen_update_options[] = {
        {_("Always use fast updates"), "0"},
        {_("Don't use fast updates"), "1"},
        {_("Full updates every 2 pages"), "2"},
        {_("Full updates every 3 pages"), "3"},
        {_("Full updates every 4 pages"), "4"},
        {_("Full updates every 5 pages"), "5"},
        {_("Full updates every 6 pages"), "6"},
        {_("Full updates every 8 pages"), "8"},
        {_("Full updates every 10 pages"), "10"},
        {_("Full updates every 14 pages"), "14"},
        {NULL, NULL},
    };

    item_def_t turbo_update_options[] = {
        {_("Turbo mode disabled"), "0"},
        {_("Turbo mode enabled"), "1"},
        {NULL, NULL},
    };
#ifdef CR_POCKETBOOK
    item_def_t rotate_mode_options[] = {
        {"360°", "0"},
        {"180°", "1"},
        {_("180+slow next page"), "2"},
        {_("180+slow prev/next page"), "3"},
        {_("180+FAST next page"), "4"},
        {_("180+FAST prev/next page"), "5"},
        {_("180+FAST next/prev page"), "6"},
        {NULL, NULL}
    };
    item_def_t rotate_angle_options[] = {
        {"20°", "0"},
        {"25°", "1"},
        {"30°", "2"},
        {"35°", "3"},
        {"40°", "4"},
        {"45°", "5"},
        {"50°", "6"},
        {"55°", "7"},
        {"60°", "8"},
        {"65°", "9"},
        {"70°", "10"},
        {NULL, NULL}
    };
#endif

    item_def_t image_scaling_modes[] = {
        {_("Disabled (1:1)"), "0"},
        {_("Integer scale"), "1"},
        {_("Arbitrary scale"), "2"},
        {NULL, NULL}
    };

    item_def_t image_scaling_factors[] = {
        {_("Auto"), "0"},
        {_("*1"), "1"},
        {_("*2"), "2"},
        {_("*3"), "3"},
        {NULL, NULL}
    };

	CRLog::trace("showSettingsMenu() - %d property values found", props->getCount() );

        setSkinName(lString16("#settings"));
        //setSkinName(lString16(L"#main"));

        LVFontRef valueFont( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
        CRMenu * mainMenu = this;
        mainMenu->setAccelerators( _menuAccelerators );

        CRMenu * fontFaceMenu = new CRMenu(_wm, mainMenu, mm_FontFace,
                                            _("Default font face"),
                                                    LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_FACE );
        fontFaceMenu->setSkinName(lString16("#settings"));
        CRLog::trace("getting font face list");
        lString16Collection list;
        fontMan->getFaceList( list );
        CRLog::trace("faces found: %d", list.length());
        int i;
        lString8 defFont = UnicodeToUtf8(getSkin()->getItemSkin()->getFontFace());//( fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")) );
        for ( i=0; i<(int)list.length(); i++ ) {
            fontFaceMenu->addItem( new OnDemandFontMenuItem( fontFaceMenu, i,
                                    list[i], LVImageSourceRef(), list[i].c_str(),
                                    MENU_FONT_FACE_SIZE, 400,
                                    false, css_ff_sans_serif, UnicodeToUtf8(list[i]), defFont) );
            fontFaceMenu->setFullscreen( true );
        }
        fontFaceMenu->setAccelerators( _menuAccelerators );
        fontFaceMenu->reconfigure( 0 );

        //lString8 fontFace = UnicodeToUtf8(props->getStringDef( PROP_FONT_FACE, UnicodeToUtf8(list[0]).c_str() ));
        mainMenu->addItem( fontFaceMenu );

        CRMenu * fontFallbackFaceMenu = new CRMenu(_wm, mainMenu, mm_FontFallbackFace,
                                            _("Fallback font face"),
                                                    LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FALLBACK_FONT_FACE );
        fontFallbackFaceMenu->setSkinName(lString16("#settings"));

        for ( i=0; i<(int)list.length(); i++ ) {
            fontFallbackFaceMenu->addItem( new OnDemandFontMenuItem( fontFallbackFaceMenu, i,
                                    list[i], LVImageSourceRef(), list[i].c_str(),
                                    MENU_FONT_FACE_SIZE, 400,
                                    false, css_ff_sans_serif, UnicodeToUtf8(list[i]), defFont) );
            fontFallbackFaceMenu->setFullscreen( true );
        }

        fontFallbackFaceMenu->setAccelerators( _menuAccelerators );
        fontFallbackFaceMenu->reconfigure( 0 );
        mainMenu->addItem( fontFallbackFaceMenu );

        CRMenu * fontSizeMenu = createFontSizeMenu( _wm, mainMenu, props );
        mainMenu->addItem( fontSizeMenu );

        CRMenu * emboldenModeMenu = new CRMenu(_wm, mainMenu, mm_Embolden,
                _("Font weight"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_WEIGHT_EMBOLDEN );
        addMenuItems( emboldenModeMenu, embolden_mode );
        mainMenu->addItem( emboldenModeMenu );

        CRMenu * fontAntialiasingMenu = new CRMenu(_wm, mainMenu, mm_FontAntiAliasing,
                _("Font antialiasing"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_ANTIALIASING );
        addMenuItems( fontAntialiasingMenu, antialiasing_modes );
        mainMenu->addItem( fontAntialiasingMenu );

        CRMenu * fontHintingMenu = new CRMenu(_wm, mainMenu, mm_FontHinting,
                _("Font hinting"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_HINTING );
        addMenuItems( fontHintingMenu, fonthinting_modes );
        mainMenu->addItem( fontHintingMenu );

        CRMenu * fontGammaMenu = new CRMenu(_wm, mainMenu, mm_FontGamma,
                _("Font Gamma"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FONT_GAMMA );
        addMenuItems( fontGammaMenu, fontgamma_list );
        mainMenu->addItem( fontGammaMenu );

        CRMenu * interlineSpaceMenu = new CRMenu(_wm, mainMenu, mm_InterlineSpace,
                _("Interline space"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_INTERLINE_SPACE );
        addMenuItems( interlineSpaceMenu, interline_spaces );
        mainMenu->addItem( interlineSpaceMenu );

#if CR_INTERNAL_PAGE_ORIENTATION==1 || defined(CR_POCKETBOOK)
        CRMenu * orientationMenu = createOrientationMenu(mainMenu, props);
        mainMenu->addItem( orientationMenu );
#endif
#ifdef CR_POCKETBOOK
        CRMenu * rotateModeMenu = new CRMenu(_wm, mainMenu, mm_rotateMode,
                _("Rotate"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_POCKETBOOK_ROTATE_MODE );
        addMenuItems( rotateModeMenu, rotate_mode_options );
        mainMenu->addItem( rotateModeMenu );
        CRMenu * rotateAngleMenu = new CRMenu(_wm, mainMenu, mm_rotateAngle,
                _("Page turn angle"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_POCKETBOOK_ROTATE_ANGLE );
        addMenuItems( rotateAngleMenu, rotate_angle_options );
        mainMenu->addItem( rotateAngleMenu );
#endif
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

        CRMenu * embeddedFontsMenu = new CRMenu(_wm, mainMenu, mm_EmbeddedFonts,
                _("Document embedded fonts"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_EMBEDDED_FONTS );
        addMenuItems( embeddedFontsMenu, embedded_fonts );
        mainMenu->addItem( embeddedFontsMenu );

        CRMenu * inverseModeMenu = new CRMenu(_wm, mainMenu, mm_Inverse,
                _("Inverse display"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_DISPLAY_INVERSE );
        addMenuItems( inverseModeMenu, inverse_mode );
        mainMenu->addItem( inverseModeMenu );

#if ENABLE_UPDATE_MODE_SETTING==1
        CRMenu * fastUpdatesMenu = new CRMenu(_wm, mainMenu, mm_FastUpdates,
                _("Display update mode"),
                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_DISPLAY_FULL_UPDATE_INTERVAL );
        addMenuItems( fastUpdatesMenu, screen_update_options );
        mainMenu->addItem( fastUpdatesMenu );
#endif
        if ( _wm->getScreen()->getTurboUpdateSupported() ) {
            CRMenu * turboUpdatesMenu = new CRMenu(_wm, mainMenu, mm_TurboUpdateMode,
                    _("Turbo update mode"),
                    LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_DISPLAY_TURBO_UPDATE_MODE );
            addMenuItems( turboUpdatesMenu, turbo_update_options );
            mainMenu->addItem( turboUpdatesMenu );
        }

#if 0
        CRMenu * bookmarkIconsMenu = new CRMenu(_wm, mainMenu, mm_BookmarkIcons,
                _("Show bookmark icons"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_BOOKMARK_ICONS );
        addMenuItems( bookmarkIconsMenu, bookmark_icons );
        mainMenu->addItem( bookmarkIconsMenu );
#endif

        CRMenu * highlightbookmarksMenu = new CRMenu(_wm, mainMenu, mm_HighlightBookmarks,
                _("Highlight bookmarks"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_HIGHLIGHT_COMMENT_BOOKMARKS );
        addMenuItems( highlightbookmarksMenu, highlight_bookmark );
        mainMenu->addItem( highlightbookmarksMenu );

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

        //====== Hyphenation ==========
		if ( HyphMan::getDictList() ) {
            // strings from CREngine - just to catch by gettext tools
            _("[No Hyphenation]");
            _("[Algorythmic Hyphenation]");
			CRMenu * hyphMenu = new CRMenu(_wm, mainMenu, mm_Hyphenation,
					_("Hyphenation"),
					LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_HYPHENATION_DICT );
			for ( i=0; i<HyphMan::getDictList()->length(); i++ ) {
				HyphDictionary * item = HyphMan::getDictList()->get( i );
				hyphMenu->addItem( new CRMenuItem( hyphMenu, i,
					item->getTitle(),
					LVImageSourceRef(),
					LVFontRef(), item->getId().c_str() ) );
			}
			hyphMenu->setAccelerators( _menuAccelerators );
            hyphMenu->setSkinName(lString16("#settings"));
            hyphMenu->reconfigure( 0 );
            mainMenu->addItem( hyphMenu );
		}

        CRMenu * floatingPunctuationMenu = new CRMenu(_wm, mainMenu, mm_FloatingPunctuation,
                _("Floating punctuation"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_FLOATING_PUNCTUATION );
        addMenuItems( floatingPunctuationMenu, floating_punctuation_options );
        mainMenu->addItem( floatingPunctuationMenu );


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
        marginsMenu->setSkinName(lString16("#settings"));
        marginsMenu->reconfigure( 0 );
        mainMenu->addItem( marginsMenu );

#ifndef CR_POCKETBOOK
        CRControlsMenu * controlsMenu =
                new CRControlsMenu(this, mm_Controls, props, lString16("main"), 8, _rect);
        controlsMenu->setAccelerators( _menuAccelerators );
        controlsMenu->setSkinName(lString16("#settings"));
        controlsMenu->setValueFont(valueFont);
        controlsMenu->reconfigure( 0 );
        mainMenu->addItem( controlsMenu );
#endif

        //====== Image scaling ==============
        CRMenu * scalingMenu = new CRMenu(_wm, mainMenu, mm_ImageScaling,
                _("Image scaling"), LVImageSourceRef(), LVFontRef(), valueFont, props );
        CRMenu * blockImagesZoominModeMenu = new CRMenu(_wm, scalingMenu, mm_blockImagesZoominMode,
                _("Block image scaling mode"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE );
        addMenuItems( blockImagesZoominModeMenu, image_scaling_modes );
        scalingMenu->addItem( blockImagesZoominModeMenu );
        CRMenu * blockImagesZoominScaleMenu = new CRMenu(_wm, scalingMenu, mm_blockImagesZoominScale,
                _("Block image max zoom"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE );
        addMenuItems( blockImagesZoominScaleMenu, image_scaling_factors );
        scalingMenu->addItem( blockImagesZoominScaleMenu );

        CRMenu * inlineImagesZoominModeMenu = new CRMenu(_wm, scalingMenu, mm_inlineImagesZoominMode,
                _("Inline image scaling mode"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_IMG_SCALING_ZOOMIN_INLINE_MODE );
        addMenuItems( inlineImagesZoominModeMenu, image_scaling_modes );
        scalingMenu->addItem( inlineImagesZoominModeMenu );
        CRMenu * inlineImagesZoominScaleMenu = new CRMenu(_wm, scalingMenu, mm_inlineImagesZoominScale,
                _("Inline image max zoom"),
                                LVImageSourceRef(), LVFontRef(), valueFont, props, PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE );
        addMenuItems( inlineImagesZoominScaleMenu, image_scaling_factors );
        scalingMenu->addItem( inlineImagesZoominScaleMenu );
        scalingMenu->setAccelerators( _menuAccelerators );
        scalingMenu->setSkinName(lString16("#settings"));
        scalingMenu->reconfigure( 0 );
        mainMenu->addItem( scalingMenu );
        reconfigure(0);
}

/// use to override status text
lString16 CRSettingsMenu::getStatusText()
{
    /// find key by command
    int applyKey = 0;
    int applyFlags = 0;
    int cancelKey = 0;
    int cancelFlags = 0;
    if ( !_acceleratorTable->findCommandKey( MCMD_OK, 0, applyKey, applyFlags )
        || !_acceleratorTable->findCommandKey( MCMD_CANCEL, 0, cancelKey, cancelFlags ) )
        return _statusText;
    lString16 pattern(_("Press $1 to change option\n$2 to apply, $3 to cancel"));
#ifdef CR_POCKETBOOK
	pattern.replaceParam(1, getCommandKeyName(MCMD_SELECT) );
#else
    pattern.replaceParam(1, getItemNumberKeysName());
#endif
    pattern.replaceParam(2, getCommandKeyName(MCMD_OK) );
    pattern.replaceParam(3, getCommandKeyName(MCMD_CANCEL) );
    return pattern;
}
