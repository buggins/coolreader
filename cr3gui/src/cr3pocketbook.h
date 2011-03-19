#ifndef CR3_POCKETBOOK_H
#define CR3_POCKETBOOK_H

#ifdef __WINE__
#undef LoadBitmap
#define LoadBitmap PB_LoadBitmap

#undef EnumFonts
#define EnumFonts PB_EnumFonts
#endif

#define PB_COMMANDS_START 6200

enum CRPbCommands {
	PB_CMD_BEGIN = PB_COMMANDS_START,
	PB_QUICK_MENU,
	PB_QUICK_MENU_SELECT,
	PB_CMD_ROTATE,
	PB_CMD_ROTATE_ANGLE_SET,
	PB_CMD_CONTENTS
};

#define PB_QUICK_MENU_BMP_ID "fbreader_menu"
#define PB_QUICK_MENU_TEXT_ID "qmenu.fbreader.0.text"
#define PB_QUICK_MENU_TEXT_ID_IDX 15

#define PB_QUICK_MENU_ACTION_ID "qmenu.fbreader.0.action"
#define PB_QUICK_MENU_ACTION_ID_IDX 15

#define KEY_BUFFER_LEN 256

class CRPbMenu : public CRMenu {
private:
	void doCloseMenu(int command, int param = 0);
protected:
	int _selectedIndex;
	virtual int setInitialSelection();
	virtual void nextItem();
	virtual void prevItem();
	virtual void nextPage();
	virtual void prevPage();
	virtual bool onItemSelect(int command, int params = 0 );
	int getLastOnPage();
public:
	CRPbMenu(CRGUIWindowManager * wm, CRMenu * parentMenu, int id, lString16 label, LVImageSourceRef image,
		LVFontRef defFont, LVFontRef valueFont, CRPropRef props=CRPropRef(), const char * propName=NULL, int pageItems=8) 
		: CRMenu(wm, parentMenu, id, label, image, defFont, valueFont, props, propName, pageItems), 
			_selectedIndex(0) {	}
    CRPbMenu( CRGUIWindowManager * wm, CRMenu * parentMenu, int id, const char * label, LVImageSourceRef image, 
		LVFontRef defFont, LVFontRef valueFont, CRPropRef props=CRPropRef(), const char * propName=NULL, int pageItems=8 )
		: CRMenu(wm, parentMenu, id, label, image, defFont, valueFont, props, propName, pageItems), 
			_selectedIndex(0) {	}
	virtual void activated();
	virtual int getSelectedItemIndex();
    virtual bool onCommand( int command, int params = 0 );
};

#endif //CR3_POCKETBOOK_H

