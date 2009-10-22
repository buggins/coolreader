//
// C++ Implementation: jinke/lbook V3 viewer plugin
//
// Description:
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <unistd.h>
#include <sys/wait.h>
#include "cr3jinke.h"
#include <crengine.h>
#include <crgui.h>
#include "cr3main.h"
#include "mainwnd.h"

// uncomment following line to allow running executables named .exe.txt
#define ALLOW_RUN_EXE 1
// uncomment to use separate .ini files for different formats
#define SEPARATE_INI_FILES 1

status_info_t lastState = {0,0,0};
static CallbackFunction * v3_callbacks = NULL;

static char last_bookmark[2048]= {0};
static int last_bookmark_page = 0;

static bool shuttingDown = false;

#define USE_JINKE_USER_DATA 0
#define USE_OWN_BATTERY_TEST 0

#ifdef ALLOW_RUN_EXE
static const char * EXE_FILE_NAME = NULL;
#endif

int getBatteryState()
{
#if USE_OWN_BATTERY_TEST==0
    lastState.batteryState = v3_callbacks->GetBatteryState();
    if ( lastState.batteryState >=0 && lastState.batteryState <=4 )
        return lastState.batteryState * 100 / 4;
    return 100;
#else
    FILE * f = fopen( "/dev/misc/s3c2410_batt", "rb" );
    if ( !f )
        return -1;
    int ch = fgetc( f );
    fclose(f);
    if ( ch == ' ' )
        return -1;
    if ( ch>=0 && ch <= 16 )
        return ch * 100 / 16;
    return 100;
#endif
}

#include <cri18n.h>


/// WXWidget support: draw to wxImage
class CRJinkeScreen : public CRGUIScreenBase
{
    public:
        static CRJinkeScreen * instance;
    protected:
        virtual void update( const lvRect & rc2, bool full )
        {
        	if ( rc2.isEmpty() && !full )
        		return;
        	lvRect rc = rc2;
        	rc.left &= ~3;
        	rc.right = (rc.right + 3) & ~3;
            CRLog::debug("CRJinkeScreen::update()");
            if ( rc.height()>400 )
            	full = true;
            else
            	full = false;
			CRLog::debug("CRJinkeScreen::update( %d, %d, %d, %d, %s )", rc.left, rc.top, rc.right, rc.bottom, full ? "full" : "partial");
            v3_callbacks->BlitBitmap( rc.left, rc.top, rc.width(), rc.height(), rc.left, rc.top, 600, 800, (unsigned char *)_front->GetScanLine(0) );
            //v3_callbacks->PartialPrint();
            if ( full )
            	v3_callbacks->Print();
            else
				v3_callbacks->PartialPrint();
        }
    public:
        virtual ~CRJinkeScreen()
        {
            instance = NULL;
        }
        CRJinkeScreen( int width, int height )
        :  CRGUIScreenBase( width, height, true )
        {
            instance = this;
        }
};
CRJinkeScreen * CRJinkeScreen::instance = NULL;


class CRJinkeWindowManager : public CRGUIWindowManager
{
protected:
public:
    /// translate string by key, return default value if not found
    virtual lString16 translateString( const char * key, const char * defValue )
    {
        CRLog::trace("Translate(%s)", key);
        lString16 res;
        //static char buf[2048];
        const char * res8 = v3_callbacks->GetString( (char *)key );
        if ( res8 && res8[0] ) {
            CRLog::trace("   found(%s)", res8);
            res = Utf8ToUnicode( lString8(res8) );
        } else {
            CRLog::trace("   not found");
            res = Utf8ToUnicode( lString8(defValue) );
        }
        return res;
    }
    static CRJinkeWindowManager * instance;
    CRJinkeWindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL)
    {
        CRJinkeScreen * s = new CRJinkeScreen( dx, dy );
        _screen = s;
        _ownScreen = true;
        instance = this;
    }
    // runs event loop
    virtual int runEventLoop()
    {
        return 0; // NO EVENT LOOP AVAILABLE
    }
    bool doCommand( int cmd, int params )
    {
        if ( !onCommand( cmd, params ) )
            return false;
        update( false );
        return true;
    }
};

CRJinkeWindowManager * CRJinkeWindowManager::instance = NULL;
V3DocViewWin * main_win = NULL;

class CRJinkeDocView : public V3DocViewWin {
public:
    static CRJinkeDocView * instance;
    CRJinkeDocView( CRGUIWindowManager * wm, lString16 dataDir )
    : V3DocViewWin( wm, dataDir )
    {
        instance = this;
    }
    virtual void closing()
    {
        strcpy( last_bookmark, GetCurrentPositionBookmark() );
        last_bookmark_page = CRJinkeDocView::instance->getDocView()->getCurPage();
        V3DocViewWin::closing();
    }
    virtual ~CRJinkeDocView()
    {
        instance = NULL;
    }
};
CRJinkeDocView * CRJinkeDocView::instance = NULL;


// some prototypes
//int InitDoc(char *fileName);


#ifdef JINKE_VIEWER

int main( int argc, const char * argv[] )
{
    if ( argc<2 ) {
        printf("usage: cr3 <filename>\n");
        return 1;
    }
    if ( !InitDoc( (char *)argv[1] ) ) {
        printf("Failed to show file %s\n", argv[1]);
        return 2;
    }
    return 0;
}

#else


void SetCallbackFunction(struct CallbackFunction *cb)
{
    CRLog::trace("SetCallbackFunction()");
    v3_callbacks = cb;
    lastState.batteryState = v3_callbacks->GetBatteryState();
}


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
int OnMenuAction( int actionId )
{
    CRLog::trace("OnMenuAction(%d)", actionId);
    return 0; // STUB
}


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
int OnKeyPressed(int keyId, int state)
{
    if ( shuttingDown )
        return 0;
    CRLog::debug("OnKeyPressed(%d, %d)", keyId, state);

#if 0
    FILE * f = fopen("/root/abook/keys.log","at");
    if ( f ) {
        fprintf(f,"key = %d \t  (%d) \n", keyId, state);
        fclose(f);
    }
#endif
    if ( keyId & 128 )
        return 2; // to ignore UP event after long keypress
    if ( state != CUSTOMIZESTATE )
        v3_callbacks->BeginDialog();
    static int convert_table[] = {
    KEY_0, '0', 0,
    KEY_1, '1', 0,
    KEY_2, '2', 0,
    KEY_3, '3', 0,
    KEY_4, '4', 0,
    KEY_5, '5', 0,
    KEY_6, '6', 0,
    KEY_7, '7', 0,
    KEY_8, '8', 0,
    KEY_9, '9', 0,
    LONG_KEY_0, '0', KEY_FLAG_LONG_PRESS,
    LONG_KEY_1, '1', KEY_FLAG_LONG_PRESS,
    LONG_KEY_2, '2', KEY_FLAG_LONG_PRESS,
    LONG_KEY_3, '3', KEY_FLAG_LONG_PRESS,
    LONG_KEY_4, '4', KEY_FLAG_LONG_PRESS,
    LONG_KEY_5, '5', KEY_FLAG_LONG_PRESS,
    LONG_KEY_6, '6', KEY_FLAG_LONG_PRESS,
    LONG_KEY_7, '7', KEY_FLAG_LONG_PRESS,
    LONG_KEY_8, '8', KEY_FLAG_LONG_PRESS,
    LONG_KEY_9, '9', KEY_FLAG_LONG_PRESS,
    KEY_CANCEL, XK_Escape, 0,
    KEY_OK, XK_Return, 0,
    KEY_DOWN, XK_Up, 0,
    KEY_UP, XK_Down, 0,
    LONG_KEY_CANCEL, XK_Escape, KEY_FLAG_LONG_PRESS,
    LONG_KEY_OK, XK_Return, KEY_FLAG_LONG_PRESS,
    LONG_KEY_DOWN, XK_Up, KEY_FLAG_LONG_PRESS,
    LONG_KEY_UP, XK_Down, KEY_FLAG_LONG_PRESS,
    KEY_SHORTCUT_VOLUME_UP, XK_KP_Add, 0,
    KEY_SHORTCUT_VOLUME_DOWN, XK_KP_Subtract, 0,
    LONG_SHORTCUT_KEY_VOLUMN_UP, XK_KP_Add, KEY_FLAG_LONG_PRESS,
    LONG_SHORTCUT_KEY_VOLUMN_DOWN, XK_KP_Subtract, KEY_FLAG_LONG_PRESS,
    0, 0, 0 // end marker
    };
    int code = 0;
    int flags = 0;
    for ( int i=0; convert_table[i]; i+=3 ) {
        if ( keyId==convert_table[i] ) {
            code = convert_table[i+1];
            flags = convert_table[i+2];
            CRLog::debug( "OnKeyPressed( %d (%04x) ) - converted to %04x, %d", keyId, keyId, code, flags );
        }
    }
    if ( !code ) {
        CRLog::debug( "Unknown key code in OnKeyPressed() : %d (%04x)", keyId, keyId );
        return 0;
    }
    bool needUpdate = CRJinkeWindowManager::instance->onKeyPressed( code, flags );
    needUpdate = CRJinkeWindowManager::instance->processPostedEvents() || needUpdate;
    if ( needUpdate )
    	CRJinkeWindowManager::instance->update( false );

    if ( CRJinkeWindowManager::instance->getWindowCount()==0 ) {
        shuttingDown = true;
        // QUIT
        CRLog::trace("windowCount==0, quitting");
        v3_callbacks->EndDialog();
        return 0;
    }
    return 2;

}

/**
* Get page number by bookmark.
*/
int GetBookmarkPage( const char * bookmark )
{
    CRLog::trace("GetBookmarkPage(%s)", bookmark);
    if ( !CRJinkeDocView::instance )
        return last_bookmark_page;
    ldomXPointer bm =  CRJinkeDocView::instance->getDocView()->getDocument()->createXPointer(Utf8ToUnicode(lString8(bookmark)));
    return CRJinkeDocView::instance->getDocView()->getBookmarkPage(bm);
}

/**
* Call this function to return to stored bookmark's position.
*/
void GoToBookmark( const char * bookmark )
{
    CRLog::trace("GoToBookmark(%s)", bookmark);
	if ( !bookmark || !bookmark[0] )
		return;
#if 1
    ldomXPointer bm = main_win->getDocView()->getDocument()->createXPointer(Utf8ToUnicode(lString8(bookmark)));
    if ( !bm.isNull() )
        main_win->getDocView()->goToBookmark(bm);
#endif
}


/**
* Get page bookmark description.
*/
unsigned short * szGetVoiceDataBlock( int iPage, int * numBytes, int * encodingType )
{
    LVDocView * _docview = main_win->getDocView();
    CRLog::trace("szGetVoiceDataBlock(%d)", iPage);
    lString16 text;
    ldomXPointer bm = _docview->getPageBookmark( iPage );
    if ( !bm.isNull() ) {
        lString16 titleText;
        lString16 posText;
        _docview->getBookmarkPosText( bm, titleText, posText );
        text = titleText;
        if ( !posText.empty() && !titleText.empty() )
            text += L" \n";
        text += posText;
    }
    if ( text.empty() ) {
        text = L"";
        LVRendPageList * pages = _docview->getPageList();
        int percent = 0;
        if ( iPage>=0 && iPage<pages->length() ) {
            percent = ( iPage * 100 ) / pages->length()-1;
        }
        text = lString16::itoa(percent);
        text += L"%";
    }
    *encodingType = 2;
    *numBytes = text.length(); // * 2;
    unsigned short * buf = ( unsigned short *) malloc( ( text.length() + 1 ) * sizeof(unsigned short ) );
    int i=0;
    for ( const lChar16 * str = text.c_str(); (buf[i++] = *str++) != 0; ) {
    }
    CRLog::trace(" return : \"%s\"\n", UnicodeToUtf8(text).c_str() );
    return buf; // caller should free this buffer
}



int OnStatusInfoChange( status_info_t * statusInfo, myRECT * rectToUpdate )
{
    bool bookmarkChanged = ( lastState.bookmarkLabelFlags!=statusInfo->bookmarkLabelFlags ||
         lastState.currentBookmarkFlags!=statusInfo->currentBookmarkFlags );
    bool batteryChanged = ( lastState.batteryState!=statusInfo->batteryState );
    memcpy( &lastState, statusInfo, sizeof( lastState ) );
    CRLog::trace("OnStatusInfoChange(bookmarks=%x, battery=%d)", statusInfo->bookmarkLabelFlags, statusInfo->batteryState);
    if ( !bookmarkChanged && !batteryChanged )
        return 0;
    if ( batteryChanged && main_win!=NULL ) {
        LVDocView * _docview = main_win->getDocView();
        _docview->setBatteryState( ::getBatteryState() );
        main_win->setDirty();
        main_win->getWindowManager()->update( false );
    }
    // Return 0 always, ignore Jinke status
    return 0;
}


const char * GetAboutInfoText()
{
    LVDocView * _docview = main_win->getDocView();
    CRLog::trace("GetAboutInfoText()");
    lString16 authors = _docview->getAuthors();
    lString16 title = _docview->getTitle();
    lString16 series = _docview->getSeries();
    lString16 text;
    static char about_text[10000];
    if ( !authors.empty() ) {
        text << L"Author(s):     " << authors << L"\n";
    }
    if ( !title.empty() ) {
        text << L"Title:     " << title << L"\n";
    }
    if ( !series.empty() ) {
        text << L"Series:     " << series << L"\n";
    }
    lString16 crengineVersion = Utf8ToUnicode(lString8(CR_ENGINE_VERSION));
    text << L"CoolReader:    " << crengineVersion << L"\n";

    lStr_cpy( about_text, UnicodeToUtf8( text ).c_str() );
    return about_text;
}


// stubs
int IsStandardStatusBarVisible() { return 0; }
#ifdef NANO_X
void vSetDisplayState(Apollo_State*state) { }
#endif

void vSetCurPage(int index)
{
    CRLog::trace("vSetCurPage(%d)", index);
#if 0
    if ( index < 0 ){
        index = 0;
    }
    if ( index<0 || index>CRJinkeDocView::instance->getDocView()->getPageCount() )
        return;
    CRJinkeWindowManager::instance->doCommand( DCMD_GO_PAGE, index );
#endif
}
int bGetRotate() { return 0; }
void vSetRotate(int rot) { }
void vGetTotalPage(int*iTotalPage)
{
    if (!shuttingDown)
        *iTotalPage = CRJinkeDocView::instance->getDocView()->getPageCount();
}
int GetPageIndex()
{
    CRLog::trace("GetPageIndex()");
    if (shuttingDown)
        return 0;
    return CRJinkeDocView::instance->getDocView()->getCurPage(); //pageNo
}
int Origin() { return 1; }
void vFontBigger() { }
int Bigger() { return 0; }
int Smaller() { return 0; }
int Rotate() { return 0; }
int Fit() { return 0; }
int Prev() { return 0; }
int Next() { return 0; }

int GotoPage(int index)
{
    CRLog::trace("GotoPage(%d)", index);
	/*
    if ( index<0 || index>CRJinkeDocView::instance->getDocView()->getPageCount() )
        return 0;
    CRJinkeWindowManager::instance->doCommand( DCMD_GO_PAGE, index );
	*/
    return 1;
}
void Release() { }

void GetPageDimension(int *width, int *height)
{
    *width = 600;
    *height = 800;
}

void SetPageDimension(int width, int height) { }
double dGetResizePro() { return 1.0; }
void vSetResizePro(double dSetPro) { }

void GetPageData(void **data)
{
#ifdef ALLOW_RUN_EXE
    if ( EXE_FILE_NAME!=NULL ) {
        __pid_t pid;
        pid = fork();
        if(!pid) {
            execve(EXE_FILE_NAME, NULL, NULL);
            exit(0);
        } else {
            waitpid(pid, NULL, 0);
            exit(0);
            //return 0;
        }
    }
#endif

    //TODO:
    CRLog::trace("GetPageData() enter");
    //_docview->setBatteryState( ::getBatteryState() );
    //_docview->Draw();
    //LVDocImageRef pageImage = _docview->getPageImage(0);
    CRJinkeWindowManager::instance->update( false );
    LVRef<LVDrawBuf> pageImage = CRJinkeWindowManager::instance->getScreen()->getCanvas();
    LVDrawBuf * drawbuf = pageImage.get();
    *data = drawbuf->GetScanLine(0);
    CRLog::trace("GetPageData() exit");
    //*data = _docview->GetDrawBuf()->GetScanLine(0);
}

int GetPageNum()
{
    return 1; //_docview->;
}
void bGetUserData(void **vUserData, int *iUserDataLength)
{
    printf("PLUGIN: bGetUserData()\n");
#if USE_JINKE_USER_DATA==1
    if ( !main_win ) {
    	CRLog::error("bGetUserData() - No main window yet created");
    	return;
    }
    LVStreamRef stream = LVCreateMemoryStream( NULL, 0, false, LVOM_READWRITE );
    if ( !main_win->saveHistory( stream ) ) {
    	CRLog::error( "Cannot write history file to buffer" );
    	return;
    }
    int sz = stream->GetSize();
    char * buf = (char*)malloc( sz );
    lvsize_t bytesRead = 0;
    if ( stream->Read( buf, sz, &bytesRead )!=LVERR_OK || bytesRead!=sz ) {
    	// NOTE: ignore this memory leak
    	*vUserData = buf;
    	*iUserDataLength = sz;
    }
#endif
}

void vSetUserData(void *vUserData, int iUserDataLength)
{
    CRLog::trace("vSetUserData()");
#if USE_JINKE_USER_DATA==1
    if ( !main_win ) {
    	CRLog::error("vSetUserData() - No main window yet created");
    	return;
    }
    LVStreamRef stream = LVCreateMemoryStream( vUserData, iUserDataLength, true, LVOM_READ );
    if ( !main_win->loadHistory( stream ) ) {
    	CRLog::error( "Cannot read history file from data block" );
    	return;
    }
    main_win->getDocView()->restorePosition();
#endif
}
int iGetDocPageWidth()
{
    return 600;
}
int iGetDocPageHeight()
{
    return 800;
}
unsigned short usGetLeftBarFlag() { return 4; }
void   vEndInit(int iEndStyle)
{
    CRLog::trace("vEndInit()");
    //main_win->getDocView()->swapToCache();
}

void   vEndDoc()
{
    CRLog::trace("vEndDoc()");
    CRLog::trace("uniniting CREngine");
    ShutdownCREngine();
}

int  iInitDocF(char *filename,int pageNo, int flag)
{
    return 0;
}
void   vFirstBmp(char *fileName, int pageNo) { }
/// returns number of doc page for entry
//int   iGetCurDirPage(int level, int idx)
int iGetCurDirPage(int idx, int level) { return 1; }
/// initializes the directory
int iCreateDirList() { return 0; }
/// returns number of entries for current directory entry
int iGetDirNumber() { return 0; }
unsigned short* usGetCurDirNameAndLen(int pos, int * len)
{
    *len = 1;
    static unsigned short buf[2]={'a', 0 };
    return buf; //(unsigned short *) L"dir entry\x0420";
}
unsigned short* usGetCurDirName(int level, int index)
{
    return (unsigned short *) L"dir entry\x0420";
}
int iGetCurDirLen(int level, int index) { return 11; }
void   vClearAllDirList() { }
/// return 1 for page of document, 0 for URL, image, etc...
int OpenLeaf( int pos ) { return 1; }
/// returns 1 for shortcut, 0 for subdir
int  bCurItemIsLeaf(int pos) { return 1; }
void vEnterChildDir(int pos) { }
void vReturnParentDir() { }
void vFreeDir() { }





#endif


static char history_file_name[1024] = "/root/abook/.cr3hist";

int InitDoc(char *fileName)
{
    static const lChar16 * css_file_name = L"fb2.css"; // fb2

    CRLog::trace("InitDoc()");
#ifdef __i386__
    //CRLog::setFileLogger("/root/abook/crengine.log");
    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);
#else
    //InitCREngineLog(NULL);
    InitCREngineLog("/root/abook/crengine/crlog.ini");
#endif

    lString16 bookmarkDir("/root/abook/bookmarks/");
    {
        lString8 fn(fileName);
        if ( fn.startsWith(lString8("/home")) ) {
            strcpy( history_file_name, "/home/.cr3hist" );
            bookmarkDir = lString16("/home/bookmarks/");
        }
        CRLog::info( "History file name: %s", history_file_name );
    }

    char manual_file[512] = "";
    {
        const char * lang = v3_callbacks->GetString( "CR3_LANG" );
        if ( lang && lang[0] ) {
            // set translator
            CRLog::info("Current language is %s, looking for translation file", lang);
            lString16 mofilename = L"/root/crengine/i18n/" + lString16(lang) + L".mo";
            CRMoFileTranslator * t = new CRMoFileTranslator();
            if ( t->openMoFile( mofilename ) ) {
                CRLog::info("translation file %s.mo found", lang);
                CRI18NTranslator::setTranslator( t );
            } else {
                CRLog::info("translation file %s.mo not found", lang);
                delete t;
            }
            sprintf( manual_file, "/root/crengine/manual/cr3-manual-%s.fb2", lang );
        }
    }

    const lChar16 * ini_fname = L"cr3.ini";
#ifdef SEPARATE_INI_FILES
    if ( strstr(fileName, ".txt")!=NULL || strstr(fileName, ".tcr")!=NULL) {
        ini_fname = L"cr3-txt.ini";
        css_file_name = L"txt.css";
    } else if ( strstr(fileName, ".rtf")!=NULL ) {
        ini_fname = L"cr3-rtf.ini";
        css_file_name = L"rtf.css";
    } else if ( strstr(fileName, ".htm")!=NULL ) {
        ini_fname = L"cr3-htm.ini";
        css_file_name = L"htm.css";
    } else if ( strstr(fileName, ".epub")!=NULL ) {
        ini_fname = L"cr3-epub.ini";
        css_file_name = L"epub.css";
    } else {
        ini_fname = L"cr3-fb2.ini";
        css_file_name = L"fb2.css";
    }
#endif

    lString16Collection fontDirs;
    fontDirs.add( lString16(L"/root/abook/fonts") );
    fontDirs.add( lString16(L"/home/fonts") );
    //fontDirs.add( lString16(L"/root/crengine/fonts") ); // will be added
    CRLog::info("INIT...");
    if ( !InitCREngine( "/root/crengine/", fontDirs ) )
        return 0;



    {
        CRLog::trace("creating window manager...");
        CRJinkeWindowManager * wm = new CRJinkeWindowManager(600,800);
        //main_win = new V3DocViewWin( wm, lString16(CRSKIN) );

        const char * keymap_locations [] = {
            "/root/crengine/",
            "/home/crengine/",
            "/root/abook/crengine/",
            NULL,
        };
        loadKeymaps( *wm, keymap_locations );
        HyphMan::initDictionaries( lString16("/root/crengine/hyph") );

        if ( !wm->loadSkin(  lString16( L"/root/abook/crengine/skin" ) ) )
            if ( !wm->loadSkin(  lString16( L"/home/crengine/skin" ) ) )
                wm->loadSkin( lString16( L"/root/crengine/skin" ) );

        ldomDocCache::init( lString16(L"/root/abook/crengine/.cache"), 0x100000 * 64 ); /*96Mb*/

        CRLog::trace("creating main window...");
        main_win = new CRJinkeDocView( wm, lString16(L"/root/crengine") );

#ifdef ALLOW_RUN_EXE
    {
        if( strstr(fileName, ".exe.txt") || strstr(fileName, ".exe.fb2")) {
            EXE_FILE_NAME = fileName;
            return true;
        }
    }
#endif

        CRLog::trace("setting colors...");
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if ( manual_file[0] )
            main_win->setHelpFile( lString16( manual_file ) );
        if ( !main_win->loadDefaultCover( lString16( L"/root/abook/crengine/cr3_def_cover.png" ) ) )
            if ( !main_win->loadDefaultCover( lString16( L"/home/crengine/cr3_def_cover.png" ) ) )
                main_win->loadDefaultCover( lString16( L"/root/crengine/cr3_def_cover.png" ) );
        if ( !main_win->loadCSS(  lString16( L"/root/abook/crengine/" ) + lString16(css_file_name) ) )
            if ( !main_win->loadCSS(  lString16( L"/home/crengine/" ) + lString16(css_file_name) ) )
                main_win->loadCSS( lString16( L"/root/crengine/" ) + lString16(css_file_name) );
        main_win->setBookmarkDir( bookmarkDir );
        CRLog::trace("choosing init file...");
        static const lChar16 * dirs[] = {
            L"/root/abook/crengine/",
            L"/home/crengine/",
            L"/root/appdata/",
            NULL
        };
        int i;
        CRLog::debug("Loading settings...");
        lString16 ini;
        for ( i=0; dirs[i]; i++ ) {
            ini = lString16(dirs[i]) + ini_fname;
            if ( main_win->loadSettings( ini ) ) {
                break;
            }
        }
        CRLog::debug("settings at %s", UnicodeToUtf8(ini).c_str() );
#if USE_JINKE_USER_DATA!=1
    if ( !main_win->loadHistory( lString16(history_file_name) ) ) {
        CRLog::error("Cannot read history file %s", history_file_name);
    }
#endif

        LVDocView * _docview = main_win->getDocView();
        _docview->setBatteryState( ::getBatteryState() );
        wm->activateWindow( main_win );
        if ( !main_win->loadDocument( lString16(fileName) ) ) {
            printf("Cannot open book file %s\n", fileName);
            delete wm;
            return 0;
        } else {
        }
    }

    //_docview->setVisiblePageCount( 1 );



    //tocDebugDump( _docview->getToc() );

    return 1;
}

const char * GetCurrentPositionBookmark()
{
    if ( !CRJinkeDocView::instance )
        return last_bookmark;
    CRLog::trace("GetCurrentPositionBookmark() - returning empty string");
    //ldomXPointer ptr = main_win->getDocView()->getBookmark();
    //lString16 bmtext( !ptr ? L"" : ptr.toString() );
    static char buf[1024];
    //strcpy( buf, UnicodeToUtf8( bmtext ).c_str() );
    strcpy( buf, "" );
    CRLog::trace("   return bookmark=%s", buf);
    return buf;
}

