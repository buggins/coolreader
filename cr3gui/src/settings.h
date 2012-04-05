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

#ifndef CR3_SETTINGS_H_INCLUDED
#define CR3_SETTINGS_H_INCLUDED

#include <crgui.h>
#include "fsmenu.h"

#define MENU_FONT_SIZE 28
#define MENU_FONT_FACE_SIZE 36
#define VALUE_FONT_SIZE 24

typedef struct {
    const char * translate_default;
    const char * value;
} item_def_t;

#define SETTINGS_MENU_COMMANDS_START 300
enum MainMenuItems_t {
    mm_Settings = SETTINGS_MENU_COMMANDS_START,
    mm_FontFace,
    mm_FontFallbackFace,
    mm_FontSize,
    mm_FontAntiAliasing,
    mm_FontHinting,
    mm_FontGamma,
    mm_InterlineSpace,
    mm_Orientation,
    mm_EmbeddedStyles,
    mm_EmbeddedFonts,
    mm_Inverse,
    mm_HighlightBookmarks,
    mm_StatusLine,
    mm_BookmarkIcons,
    mm_Footnotes,
    mm_SetTime,
    mm_ShowTime,
    mm_Kerning,
    mm_LandscapePages,
    mm_PreformattedText,
    mm_PageMargins,
    mm_PageMarginTop,
    mm_PageMarginLeft,
    mm_PageMarginRight,
    mm_PageMarginBottom,
    mm_Hyphenation,
    mm_Controls,
    mm_Embolden,
    mm_FastUpdates,
    mm_TurboUpdateMode,
    mm_FloatingPunctuation
#ifdef CR_POCKETBOOK
    ,mm_rotateMode,
    mm_rotateAngle
#endif /* CR_POCKETBOOK*/
    ,mm_ImageScaling = 350,
    mm_blockImagesZoominMode,
    mm_blockImagesZoominScale,
    mm_inlineImagesZoominMode,
    mm_inlineImagesZoominScale
};


#define DECL_DEF_CR_FONT_SIZES static int cr_font_sizes[] = \
{ 16, 17, 18, 19, 20, 21, 22, 23, \
  24, 25, 26, 27, 28, 29, 30, 31, \
  32, 33, 34, 35, 36, 38, 39, 40, \
  42, 44, 46, 48, 50, 54, 58, 62 }


class CRSettingsMenu : public CRFullScreenMenu
{
    protected:
        CRPropRef props;
        CRGUIAcceleratorTableRef _menuAccelerators;
        void addMenuItems( CRMenu * menu, item_def_t values[] );
        lString16 getStatusText();
    public:
        CRMenu * createFontSizeMenu( CRGUIWindowManager * wm, CRMenu * mainMenu, CRPropRef props );
#if CR_INTERNAL_PAGE_ORIENTATION==1 || defined(CR_POCKETBOOK)
        CRMenu * createOrientationMenu( CRMenu * mainMenu, CRPropRef props );
#endif
        CRSettingsMenu( CRGUIWindowManager * wm, CRPropRef props, int id, LVFontRef font, CRGUIAcceleratorTableRef menuAccelerators, lvRect & rc );
        virtual bool onCommand( int command, int params );
        virtual ~CRSettingsMenu()
        {
            CRLog::trace("Calling fontMan->gc() on Settings menu destroy");
            fontMan->gc();
            CRLog::trace("Done fontMan->gc() on Settings menu destroy");
        }
};


#endif //CR3_SETTINGS_H_INCLUDED
