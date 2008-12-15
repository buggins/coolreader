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

#define MENU_FONT_SIZE 20
#define VALUE_FONT_SIZE 17

typedef struct {
    const char * translate_label;
    const char * translate_default;
    const char * value;
} item_def_t;

#define DECL_DEF_CR_FONT_SIZES static int cr_font_sizes[] = { 24, 29, 33, 39, 44 }

class CRSettingsMenu : public CRMenu
{
    protected:
        CRPropRef props;
        CRGUIAcceleratorTableRef _menuAccelerators;
        CRMenu * createFontSizeMenu( CRMenu * mainMenu, CRPropRef props );
        void addMenuItems( CRMenu * menu, item_def_t values[], LVFontRef menuFont );
    public:
        CRSettingsMenu( CRGUIWindowManager * wm, CRPropRef props, int id, LVFontRef font, CRGUIAcceleratorTableRef menuAccelerators );
};



#endif //CR3_SETTINGS_H_INCLUDED
