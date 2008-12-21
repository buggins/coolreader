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

void SetCallbackFunction(struct CallbackFunction *cb)
{
    CRLog::trace("SetCallbackFunction()");
    v3_callbacks = cb;
    lastState.batteryState = v3_callbacks->GetBatteryState();
}

/// WXWidget support: draw to wxImage
class CRJinkeScreen : public CRGUIScreenBase
{
    public:
        static CRJinkeScreen * instance;
    protected:
        virtual void update( const lvRect & rc, bool full )
        {
/*
            CRLog::debug("crV3Screen::updateFull()");
            v3_callbacks->BlitBitmap( 0, 0, 600, getWindowHeight(), 0, 0, 600, getWindowHeight(), (unsigned char *)_buf.GetScanLine(0) );
            v3_callbacks->PartialPrint();
*/
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
        static char buf[2048];
        const char * res8 = v3_callbacks->GetString( (char *)key );
        if ( res8 && res8[0] ) {
            CRLog::trace("   found(%s)", res8);
            res = Utf8ToUnicode( lString8(res8) );
        } else {
            CRLog::trace("   not found", res8);
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
};

CRJinkeWindowManager * CRJinkeWindowManager::instance = NULL;

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
    LONG_KEY_0, '0', 0,
    LONG_KEY_1, '1', 0,
    LONG_KEY_2, '2', 0,
    LONG_KEY_3, '3', 0,
    LONG_KEY_4, '4', 0,
    LONG_KEY_5, '5', 0,
    LONG_KEY_6, '6', 0,
    LONG_KEY_7, '7', 0,
    LONG_KEY_8, '8', 0,
    LONG_KEY_9, '9', 0,
    KEY_CANCEL, XK_Escape, 0,
    KEY_OK, XK_Return, 0,
    KEY_DOWN, XK_Up, 0, 
    KEY_UP, XK_Down, 0, 
    LONG_KEY_CANCEL, XK_Escape, 0,
    LONG_KEY_OK, XK_Return, 0,
    LONG_KEY_DOWN, XK_Up, 0, 
    LONG_KEY_UP, XK_Down, 0, 
    0, 0, 0 // end marker
    };
    int code = 0;
    int flags = 0;
    for ( int i=0; convert_table[i]; i+=3 ) {
        code = convert_table[i+1];
        flags = convert_table[i+2];
    }
    if ( !code )
        return 0;
    CRJinkeWindowManager::instance->onKeyPressed( code, flags );
    return 2;

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
    // Return 0 always, ignore Jinke status
    return 0;
}

int InitDoc(char *fileName)
{
    CRLog::trace("InitDoc()");
    //CRLog::setFileLogger("/root/abook/crengine.log");
    //CRLog::setLogLevel(CRLog::LL_TRACE);
    //InitCREngineLog(NULL);
    InitCREngineLog("/root/abook/crengine/crlog.ini");
    lString16Collection fontDirs;
    fontDirs.add( lString16(L"/root/abook/fonts") );
    fontDirs.add( lString16(L"/root/crengine/fonts") );
    CRLog::info("INIT...");
    if ( !InitCREngine( "/root/abook/crengine/", fontDirs ) )
        return 0;



#ifdef ALLOW_RUN_EXE
    {
        __pid_t pid;
        if( strstr(fileName, ".exe.txt") || strstr(fileName, ".exe.fb2")) {
            pid = fork();
            if(!pid) {
                execve(fileName, NULL, NULL);
                exit(0);
            } else {
                waitpid(pid, NULL, 0);
                exit(0);
                //return 0;
            }
        }
    }
#endif
    {
        CRJinkeWindowManager * wm = new CRJinkeWindowManager(600,800);
        V3DocViewWin * main_win = new V3DocViewWin( wm, lString16(CRSKIN) );
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );

        const lChar16 * ini_fname = L"cr3.ini";
    #ifdef SEPARATE_INI_FILES
        if ( strstr(fileName, ".txt")!=NULL || strstr(fileName, ".tcr")!=NULL) {
            ini_fname = L"cr3-txt.ini";
        } else if ( strstr(fileName, ".rtf")!=NULL ) {
            ini_fname = L"cr3-rtf.ini";
        } else if ( strstr(fileName, ".htm")!=NULL ) {
            ini_fname = L"cr3-htm.ini";
        } else if ( strstr(fileName, ".epub")!=NULL ) {
            ini_fname = L"cr3-epub.ini";
        } else {
            ini_fname = L"cr3-fb2.ini";
        }
    #endif
        static const lChar16 * dirs[] = {
            L"/root/abook/crengine/",
            L"/home/crengine/",
            L"/root/appdata/",
            NULL
        };
        int i;
        for ( i=0; dirs[i]; i++ ) {
            lString16 ini = lString16(dirs[i]) + ini_fname;
            if ( main_win->loadSettings( ini ) )
                break;
        }
//        if ( !main_win->loadSkin(lString16(L"/root/abook/crengine/skins/default.c3s") )
  //          main_win->loadSkin(lString16(L"/root/crengine/skins/default.c3s");
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
        wm->activateWindow( main_win );
        if ( !main_win->getDocView()->LoadDocument(fileName) ) {
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


// stubs
int IsStandardStatusBarVisible() { return 0; }
#ifdef NANO_X
void vSetDisplayState(Apollo_State*state) { }
#endif
void vSetCurPage(int p) { }
int bGetRotate() { return 0; }
void vSetRotate(int rot) { }
void vGetTotalPage(int*iTotalPage) { *iTotalPage = 1; }
int GetPageIndex() { return 0; }
int Origin() { return 1; }
void vFontBigger() { }
int Bigger() { return 0; }
int Smaller() { return 0; }
int Rotate() { return 0; }
int Fit() { return 0; }
int Prev() { return 0; }
int Next() { return 0; }
int GotoPage(int index) { return 0; }
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
    //TODO:
    CRLog::trace("GetPageData() enter");
    //_docview->setBatteryState( ::getBatteryState() );
    //_docview->Draw();
    //LVDocImageRef pageImage = _docview->getPageImage(0);
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
    CRLog::trace("bGetUserData()");
    static int testData[] = {1, 2, 3, 4};
    //testData[0] = _docview ? _docview->getFontSize() : fontSize;
    *vUserData = testData;
    *iUserDataLength = 4;
}
void vSetUserData(void *vUserData, int iUserDataLength)
{
    CRLog::trace("vSetUserData()");
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
