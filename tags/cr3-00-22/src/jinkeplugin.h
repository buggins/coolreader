///////////////////////////////////////////////////////////
// Jinke/LBook parser/viewer interface header file

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef NANO_X
void vSetDisplayState(Apollo_State *);
#endif
void vSetCurPage(int);
int  bGetRotate();
void vSetRotate(int);

void vGetTotalPage(int *);
//used by browser
int Bigger();
int Smaller();
int Rotate();
int Fit();
int Prev();
int Next();
int GotoPage(int index);
int Origin();

//used by main functionsa
void Release();
void GetPageDimension(int *width, int *height);
void SetPageDimension(int width, int height);
void SetPageDimension_l(int width, int height);
double dGetResizePro();
void vSetResizePro(double dSetPro);
void GetPageData(void **data);
int  GetPageIndex();
int  GetPageNum();
void bGetUserData(void **vUserData, int *iUserDataLength);
void vSetUserData(void *vUserData, int iUserDataLength);
/*word2text.cpp*/
int iGetDocPageWidth();
int iGetDocPageHeight();

int iGetCurDirPage(int idx, int level);
/// initializes the directory
int iCreateDirList();
/// returns number of entries for current directory entry
int iGetDirNumber();
unsigned short* usGetCurDirNameAndLen(int pos, int * len);
unsigned short* usGetCurDirName(int level, int index);
int iGetCurDirLen(int level, int index);
void   vClearAllDirList();
int OpenLeaf( int pos );
/// returns 1 for shortcut, 0 for subdir
int  bCurItemIsLeaf(int pos);
void vEnterChildDir(int pos);
void vReturnParentDir();
void vFreeDir();

/*initDoc.cpp*/
unsigned short usGetLeftBarFlag();
void   vEndInit(int iEndStyle);
void   vEndDoc();
int   InitDoc(char*);
int    iInitDocF(char *, int, int);
void   vFirstBmp(char *, int);

/*dir.cpp*/
//void   vGetCurDir(int, int);
int       iGetCurDirPage(int, int);
int       iGetDirNumber();
unsigned short* usGetCurDirName(int, int);
int       iGetCurDirLen(int, int);
void   vClearAllDirList();
int   bCurItemIsLeaf(int pos);
void   vEnterChildDir(int pos);
void   vReturnParentDir();
void   vFreeDir();


// proposed new Viewer/Parser interface functions

/**************************************incluce/keyvalue.h***********************************/
#ifdef __arm__
#define KEY_BASE 30
#define KEY_0 (KEY_BASE)
#define KEY_1 (1+KEY_BASE)
#define KEY_2 (2+KEY_BASE)
#define KEY_3 (3+KEY_BASE)
#define KEY_4 (4+KEY_BASE)
#define KEY_5 (5+KEY_BASE)
#define KEY_6 (6+KEY_BASE)
#define KEY_7 (7+KEY_BASE)
#define KEY_8 (8+KEY_BASE)
#define KEY_9 (9+KEY_BASE)
#define KEY_BOOKMARK KEY_6			//alias name
#define KEY_CATALOG KEY_7
#define KEY_EXPANSION KEY_8
#define KEY_PREV KEY_9
#define KEY_NEXT KEY_0
#define KEY_CANCEL (10+KEY_BASE)
#define KEY_OK (11+KEY_BASE)
#define KEY_DOWN (12+KEY_BASE)     //page down key on the side
#define KEY_UP (13+KEY_BASE)   //page up key on the side
#define KEY_SHORTCUT_PREV KEY_UP	//shortcut key
#define KEY_SHORTCUT_NEXT KEY_DOWN
#define KEY_SHORTCUT_VOLUME_UP (14+KEY_BASE) 
#define KEY_SHORTCUT_VOLUME_DOWN (15+KEY_BASE)
#define KEY_POWEROFF (16+KEY_BASE)
#define KEY_CURSOR_UP (33+KEY_BASE) //prev key on the right
#define KEY_CURSOR_DOWN (34+KEY_BASE)   //next key on the right
#define KEY_CURSOR_OK (32+KEY_BASE) //OK

#define KEY_PREV_SONG KEY_9
#define KEY_NEXT_SONG KEY_0
#define KEY_FAST_FORWARD KEY_6
#define KEY_FAST_BACKWARD KEY_1
#define KEY_VOLUME_UP KEY_7
#define KEY_VOLUME_DOWN KEY_2
#define KEY_CIRCLE KEY_3
#define KEY_STYLE KEY_8

//key pressed for a long time
#define LONG_KEY_BASE 0x40+KEY_BASE
#define LONG_KEY_0 (LONG_KEY_BASE)
#define LONG_KEY_1 (1+LONG_KEY_BASE)
#define LONG_KEY_2 (2+LONG_KEY_BASE)
#define LONG_KEY_3 (3+LONG_KEY_BASE)
#define LONG_KEY_4 (4+LONG_KEY_BASE)
#define LONG_KEY_5 (5+LONG_KEY_BASE)
#define LONG_KEY_6 (6+LONG_KEY_BASE)
#define LONG_KEY_7 (7+LONG_KEY_BASE)
#define LONG_KEY_8 (8+LONG_KEY_BASE)
#define LONG_KEY_9 (9+LONG_KEY_BASE)
#define LONG_KEY_BOOKMARK LONG_KEY_6
#define LONG_KEY_CATALOG LONG_KEY_7
#define LONG_KEY_EXPANSION LONG_KEY_8
#define LONG_KEY_PREV LONG_KEY_9
#define LONG_KEY_NEXT LONG_KEY_0
#define LONG_KEY_CANCEL (10+LONG_KEY_BASE)
#define LONG_KEY_OK (11+LONG_KEY_BASE)
#define LONG_KEY_DOWN (12+LONG_KEY_BASE)     
#define LONG_KEY_UP (13+LONG_KEY_BASE)   
#define LONG_KEY_SHORTCUT_PREV LONG_KEY_UP	
#define LONG_KEY_SHORTCUT_NEXT LONG_KEY_DOWN
#define LONG_SHORTCUT_KEY_VOLUMN_UP (14+LONG_KEY_BASE) 
#define LONG_SHORTCUT_KEY_VOLUMN_DOWN (15+LONG_KEY_BASE)
#define LONG_KEY_POWEROFF (16+LONG_KEY_BASE)
#define LONG_KEY_CURSOR_UP (33+LONG_KEY_BASE)
#define LONG_KEY_CURSOR_DOWN (34+LONG_KEY_BASE)
#define LONG_KEY_CURSOR_OK (32+LONG_KEY_BASE)

#else
#define KEY_BASE 48
#define KEY_0 (KEY_BASE)
#define KEY_1 (1+KEY_BASE)
#define KEY_2 (2+KEY_BASE)
#define KEY_3 (3+KEY_BASE)
#define KEY_4 (4+KEY_BASE)
#define KEY_5 (5+KEY_BASE)
#define KEY_6 (6+KEY_BASE)
#define KEY_7 (7+KEY_BASE)
#define KEY_8 (8+KEY_BASE)
#define KEY_9 (9+KEY_BASE)
#define KEY_BOOKMARK KEY_6			
#define KEY_CATALOG KEY_7
#define KEY_EXPANSION KEY_8
#define KEY_PREV KEY_9
#define KEY_NEXT KEY_0
#define KEY_CANCEL 'n'
#define KEY_OK 'y'
#define KEY_UP ','     
#define KEY_DOWN '.'   
#define KEY_SHORTCUT_PREV KEY_UP
#define KEY_SHORTCUT_NEXT KEY_DOWN
#define KEY_SHORTCUT_VOLUME_UP 'u'
#define KEY_SHORTCUT_VOLUME_DOWN 'd' 
#define KEY_POWEROFF 'p'

#define KEY_PREV_SONG KEY_9
#define KEY_NEXT_SONG KEY_0
#define KEY_FAST_FORWARD KEY_6
#define KEY_FAST_BACKWARD KEY_1
#define KEY_VOLUME_UP KEY_7
#define KEY_VOLUME_DOWN KEY_2
#define KEY_CIRCLE KEY_3
#define KEY_STYLE KEY_8

//key pressed for a long time
#define LONG_KEY_0  ')'
#define LONG_KEY_1  '!'
#define LONG_KEY_2  '@'
#define LONG_KEY_3  '#'
#define LONG_KEY_4  '$'
#define LONG_KEY_5  '%'
#define LONG_KEY_6  '^'
#define LONG_KEY_7  '&'
#define LONG_KEY_8  '*'
#define LONG_KEY_9  '('
#define LONG_KEY_BOOKMARK LONG_KEY_6			//
#define LONG_KEY_CATALOG LONG_KEY_7
#define LONG_KEY_EXPANSION LONG_KEY_8
#define LONG_KEY_PREV LONG_KEY_9
#define LONG_KEY_NEXT LONG_KEY_0
#define LONG_KEY_CANCEL 'N'
#define LONG_KEY_OK 'Y'
#define LONG_KEY_UP '<'     //
#define LONG_KEY_DOWN '>'   //
#define LONG_KEY_SHORTCUT_PREV LONG_KEY_UP
#define LONG_KEY_SHORTCUT_NEXT LONG_KEY_DOWN
#define LONG_SHORTCUT_KEY_VOLUMN_UP 'U'
#define LONG_SHORTCUT_KEY_VOLUMN_DOWN 'D' 
#define LONG_KEY_POWEROFF 'P'

#endif
/************************************************include/tip.h**************************************/
enum{
    CHINESE,
    ENGLISH,
    CHINESE_TRADITIONAL,
    RUSSIAN,
    UKRAINIAN,
    KAZAKSTAN,
    SPANNISH,
    TURKEY,
    FRANCH,
    GRRMAN,
    BULGARIAN,
	ARABIC,
	BYELORUSSIAN,
	CATALAN,
	CZECH,
	DANISH,
	GREEK,
	ESTONIAN,
	FINNISH,
	CROATIAN,
	HUNGARIAN,
	ICELANDIC,
	ITALIAN,
	HEBREW,
	JAPANESE,
	KOREAN,
	LITHUANIAN,
	LATVIAN,
	MACEDONIAN,
	DUTCH,
	NORWEGIAN,
	POLISH,
	PORTUGUESE,
	ROMANIAN,
	SERBO_CROATIAN,
	SLOVAK,
	SLOVENIAN,
	ALBANIAN,
	SERBIAN,
	SWEDISH,
	THAI,	
    MAX_LANGUAGE
};
/**********************************************include/ViewerState.h*********************************/
enum
{
	NORMALSTATE,	//context state
	MENUSTATE,		//meun state
	INPUTSTATE,		//page input state
	CATALOGSTATE,	//catalog state
	BOOKMARKSTATE,	//bookmark state
	ABOUTSTATE,		//about state
	CUSTOMIZESTATE	//customize state
};
/**************************************ctlcallback.cpp**********************************************/
/**
* Call this function on key press.
*
* keyId - id of key. Key codes should be defined somewhere in SDK header file.
* state - the viewer state while received the key
*
* If return value is 1, this means that key has been processed in plugin and viewer should flush the screen.
* If return value is 2, this means that key has been processed in plugin and no more processing is required.
* If return value is 0, or no such function defined in plugin, default processing should be done by Viewer.
*/
int OnKeyPressed(int keyId, int state);

/**********************************CustomizeMenu.cpp,CustomizeMenu.h***************************************/
///////////////////////////////////////////////////////////
// Menu customization


/**
* Call this function on final (non submenu) menu item selection.
*
* actionId - id of menu action. Set of standard actions should be defined in SDK header file. 
*            Some range should be reserved for plugin items.
*            E.g. 1..999 for standard, Viewer-defined actions
*                 1000-1999 reserved for plugins
*
* If return value is 1, this means that action has been processed in plugin and viewer should flush the screen.
* If return value is 2, this means that action has been processed in plugin and no more processing is required.
* If return value is 0, or no such function defined in plugin, default processing should be done by Viewer.
*/
int OnMenuAction( int actionId );

/**
* Viewer menu item.
*
* viewer_menu_item_t * is usually a pointer to item array, actionId=0 for end-of-array marker.
* 
*/
struct viewer_menu_item_t {
    int actionId;                 // item action code. 0 to mark end of array. For submenu, it can be used to allow removing standard submenu item from root level.
    const char * captionLabelId;  // key string to find menu item label in language file, NULL to remove standard item with this actionId
    viewer_menu_item_t * submenu; // pointer to submenu items array, if any. Null for final item. If submenu is specified, 
                                  // submenu should be shown on item press, instead of calling OnMenuAction( int actionId )
	//Submenu not be supported currently
};

/**
* Returns custom menu items array, NULL to use standard menu items only.
*/
const viewer_menu_item_t * GetCustomViewerMenu();


/* Sample:

#define MY_MENU_ACTION_INTERILINE_SMALL  1000
#define MY_MENU_ACTION_INTERILINE_MEDIUM 1001
#define MY_MENU_ACTION_INTERILINE_BIG    1002
#define MY_MENU_ACTION_CHANGE_FONT       1003
#define MY_SUBMENU_INTERLINE_SPACING     1004

// submenu
static viewer_menu_item_t my_submenu_interline[]
{
{ MY_MENU_ACTION_INTERILINE_SMALL, "INTERLINE_SMALL", NULL },
{ MY_MENU_ACTION_INTERILINE_MEDIUM, "INTERLINE_MEDIUM", NULL },
{ MY_MENU_ACTION_INTERILINE_BIG, "INTERLINE_BIG", NULL },
{ 0, NULL, NULL } // end of array marker
};
// main menu
static viewer_menu_item_t my_plugin_menu[] = {
// standard items will be shown here
{ MENU_ACTION_ZOOM, NULL, NULL },  // to delete standard action ZOOM
{ MY_MENU_ACTION_CHANGE_FONT, "CHANGE_FONT_SIZE", NULL }, // new plugin defined menu item. CHANGE_FONT_SIZE should be added to language files.
{ MY_SUBMENU_INTERLINE_SPACING, "INTERLINE_SPACING", my_submenu_interline }, // plugin's own custom submenu
{ 0, NULL, NULL } // end of array marker
};


const viewer_menu_item_t * GetCustomViewerMenu()
{
    return my_plugin_menu;
}

*/
/******************************************MarkSearch.cpp,MarkSearch.h*********************************/
///////////////////////////////////////////////////////////
// Bookmarks should be stored as strings.

/**
* Call this function to store current position. Save returned string somewhere.
* This can be just a page number, file position or (for libfb2) XML pointer (Xpointer).
* It's necessary to be able restoring the same position even if number of pages has changed after zoom.
*/
const char * GetCurrentPositionBookmark();

/**
* Call this function to return to stored bookmark's position.
*/
void GoToBookmark( const char * bookmark );

/**
* Get page number by bookmark
*/
int GetBookmarkPage( const char * bookmark );


/***********************************tip.cpp,tip.h************************************************/
////////////////////////////////////////////////////////////
// Status bar customization


/*
On plugin start, IsStandardStatusBarVisible() is being called to determine whether standard or plugin's own
status bar should be used.

If 0 is returned, OnStatusInfoChange() should be called on status change.
If plugin redrawed status, it can inform Viewer that some are should be redrawn by returning 1, 
and setting updateRect to coordinates of are it wants to repaint. In this case viewer should get page image pointer from
plugin and then draw specified rectangle on screen.
*/

/**
* Return 0 to hide standard statusbar, 1 to show it. If no such function defined in plugin, assume as 1.
*/
int IsStandardStatusBarVisible();

/******************************************MarkSearch.cpp,MarkSearch.h*********************************/
struct status_info_t {
   int bookmarkLabelFlags; // bit set, (1, 2, 4, 8, 16) for bookmark icons 1, 2, 3, 4, 5 correspondingly
/*
*/
   int musicState;		   // 0 no music, 1 music
   int batteryState;       // e.g. 0..16 for current energy level, -1 for charging mode.
   int currentBookmarkFlags; // bit set, (1,2,4,8,16) for bookmarks on current page (1,2,3,4,5)
};

struct myRECT {
	int x;
	int y;
	int width;
	int height;
};

/**
* Call when some status information is changed.
* Plugin should return 1 and write rectangle coordinates to rectToUpdate if it wants to update part of screen to show new status.
*/
int OnStatusInfoChange( status_info_t * statusInfo, myRECT * rectToUpdate );

/*************************************CallbackFunc.cpp,CallbackFunc.h**********************************/
//encoding
enum
{
	TF_ASCII,
	TF_UTF8,
	TF_UC16,
};

//music state
enum
{
	MUSICSTOP,
	MUSICPLAY
};

//power state
enum{
    PowerLevel0,
    PowerLevel1,
    PowerLevel2,
    PowerLevel3,
    PowerLevel4,
    PowerLevelCharge		//usb plugged in
};

struct CallbackFunction{
	void (*BeginDialog)();		//enter the customize mode
	void (*EndDialog)();		//exit the customize mode, viewer will flush the screen in it.
	void (*SetFontSize)(int fontSize);	
	void (*SetFontAttr)(int fontAttr);	//0, black font, 1, white font
	void (*TextOut)(int x, int y , char *text, int length, int flags);	//flag can be TF_ASCII , TF_UTF8 or TF_UC16
			//to ascii and utf8, length is the length of string.
			//to UC16, length is the length of 
	void (*BlitBitmap)(int x, int y, int w, int h, int src_x, int src_y, int src_width, int src_height, unsigned char *buf);
	void (*Line)(int x1, int y1, int x2, int y2);
	void (*Point)(int x, int y);
	void (*Rect)(int x, int y, int width, int height);
	void (*ReadArea)(int x, int y, int width, int height, unsigned char *save);
	void (*ClearScreen)(unsigned char color);	//0x0, black , 0xFF, white
	int  (*GetBatteryState)();
	int  (*GetLanguage)();
	char*(*GetString)(char* stringName);
	void (*Print)();				//full screen flush
	void (*PartialPrint)();		//partial screen flush
};

void SetCallbackFunction(struct CallbackFunction *cb);

/***************************BookAbout.cpp,BookAbout.h**********************************************/
/** 
* returns About UTF8 text lines separated with \n character 
*/ 
const char * GetAboutInfoText(); 

unsigned short * szGetVoiceDataBlock( int iPage, int * numChars, int * encodingType );

#ifdef __cplusplus
}
#endif
////////////////////////////////////////////////////////////
// Customizable plugin set

/****************************************
It would be useful to make set of supported plugins flexible.
We can place list of file extensions and plugins to use for them into text file.

e.g. /root/plugins.cfg
*.txt libtxt.so
*.fb2 libfb2.so
*.fb2.zip libfb2.so

****************************************/
