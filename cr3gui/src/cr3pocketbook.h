#ifndef CR3_POCKETBOOK_H
#define CR3_POCKETBOOK_H

#ifdef __WINE__
#undef LoadBitmap
#define LoadBitmap PB_LoadBitmap

#undef EnumFonts
#define EnumFonts PB_EnumFonts
#endif

#include "inkview.h"

#define PB_COMMANDS_START 6200

enum CRPbCommands {
	PB_CMD_BEGIN = PB_COMMANDS_START,
	PB_QUICK_MENU,
	PB_QUICK_MENU_SELECT,
	PB_CMD_ROTATE,
	PB_CMD_ROTATE_ANGLE_SET
};

#define PB_QUICK_MENU_BMP_ID "fbreader_menu"
#define PB_QUICK_MENU_TEXT_ID "qmenu.fbreader.0.text"
#define PB_QUICK_MENU_TEXT_ID_IDX 15

#define PB_QUICK_MENU_ACTION_ID "qmenu.fbreader.0.action"
#define PB_QUICK_MENU_ACTION_ID_IDX 15

#define KEY_BUFFER_LEN 256

#endif //CR3_POCKETBOOK_H

