/*
    CR3 for OpenInkpot (XCB)
*/
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <crengine.h>
#include <crgui.h>
#include <crtrace.h>
#include <crtest.h>


#include "cr3main.h"
#include "mainwnd.h"

#include <locale.h>
#include <libintl.h>
#include <cri18n.h>

// Qt code ===================================================================

static V3DocViewWin * main_win = NULL;


/// WXWidget support: draw to wxImage
class CRQtScreen : public CRGUIScreenBase
{
    public:

    protected:
        int _bufDepth;

        /// sets new screen size
        virtual bool setSize( int dx, int dy )
        {
            if ( !CRGUIScreenBase::setSize( dx, dy ) )
                return false;
        }
        virtual void update( const lvRect & a_rc, bool full )
        {
            lvRect rc(a_rc);
            //CRLog::debug("update screen, bpp=%d width=%d, height=%d, rect={%d, %d, %d, %d} full=%d", (int)im->bpp,im->width,im->height, (int)rc.left, (int)rc.top, (int)rc.right, (int)rc.bottom, full?1:0 );
            // TODO
            if ( rc.isEmpty() )
                return;
        }
    public:
        /// creates compatible canvas of specified size
        virtual LVDrawBuf * createCanvas( int dx, int dy )
        {
            // bufDepth
            LVDrawBuf * buf;
            if (_bufDepth==32)
                buf = new LVColorDrawBuf( dx, dy );
            else
                buf = new LVGrayDrawBuf( dx, dy, _bufDepth );
            buf->Clear(0xFFFFFF);
            return buf;
        }
        virtual ~CRQtScreen()
        {
        }
        CRQtScreen( int width, int height, int bufDepth )
        :  CRGUIScreenBase( 0, 0, true ), _bufDepth(bufDepth)
        {
            _width = width;
            _height = height;
            //CRLog::info( "Device depth=%d, will use rendering depth=%d", (int)im->depth, d );

            _canvas = LVRef<LVDrawBuf>( createCanvas( _width, _height ) );
            _front = LVRef<LVDrawBuf>( createCanvas( _width, _height ) );
            printf("Created screen %d x %d, depth = %d\n", _width, _height, _bufDepth );
        }
};

class CRQtWindowManager : public CRGUIWindowManager
{
protected:

public:
    

    CRQtWindowManager( int dx, int dy, int bufDepth )
    : CRGUIWindowManager(NULL)
    {
        CRQtScreen * s = new CRQtScreen( dx, dy, bufDepth );
        _screen = s;
        _ownScreen = true;
//        cr_rotate_angle_t angle = readXCBScreenRotationAngle();
//        if ( angle!=0 )
//        CRLog::info("Setting rotation angle: %d", (int)angle );
//        setScreenOrientation( angle );
    }

    /// returns true if key is processed
    virtual bool onKeyPressed( int key, int flags = 0 )
    {
        // DEBUG ONLY
        if ( /* key == 65289 || */ key==1739 || key=='r' ) // 'r' == rotate
        {
            CRLog::info("Simulating rotation on R keypress...");
            cr_rotate_angle_t angle = (cr_rotate_angle_t)(getScreenOrientation() ^ 1);
            int dx = getScreen()->getWidth();
            int dy = getScreen()->getHeight();
            reconfigure(dy, dx, angle);
            update( true );
            reconfigure(dx, dy, angle);
            update( true );
            return true;
        }
        return CRGUIWindowManager::onKeyPressed( key, flags );
    }

    virtual bool getBatteryStatus( int & percent, bool & charging );

    /// idle actions
    virtual void idle();
    // runs event loop
    virtual int runEventLoop();
    /// forward events from system queue to application queue
    virtual void forwardSystemEvents( bool waitForEvent );
};

class QtDocViewWin : public V3DocViewWin
{
    public:
        /// file loading is finished successfully - drawCoveTo() may be called there
        virtual void OnLoadFileEnd()
        {
            V3DocViewWin::OnLoadFileEnd();
            //((CRQtWindowManager*)_wm)->updateProperties();
        }
        
        QtDocViewWin( CRGUIWindowManager * wm, lString16 dataDir )
        : V3DocViewWin( wm, dataDir )
        {
        }
        virtual ~QtDocViewWin()
        {
        }
};

bool CRQtWindowManager::getBatteryStatus( int & percent, bool & charging )
{
    percent = 100;
    charging = false;
    return false;
}

/// forward events from system queue to application queue
void CRQtWindowManager::forwardSystemEvents( bool waitForEvent )
{
    if ( _stopFlag )
        waitForEvent = false;
}

/// called when message queue is empty and application is going to wait for event
void CRQtWindowManager::idle()
{
    if ( !_stopFlag && getWindowCount() ) {
    }
}

// runs event loop
int CRQtWindowManager::runEventLoop()
{
}

int main(int argc, char **argv)
{
    int res = 0;

    {
    CRLog::setStdoutLogger();
#ifdef __i386__
    CRLog::setLogLevel( CRLog::LL_TRACE );
#else
    //CRLog::setLogLevel( CRLog::LL_ERROR );
    InitCREngineLog("/home/user/.crengine/crlog.ini");
#endif

       
    // gettext initialization
    setlocale (LC_ALL, "");
    #undef LOCALEDIR
    #define LOCALEDIR "/usr/share/locale"
    #ifndef PACKAGE
    #define PACKAGE "cr3"
    #endif
    const char * bindres = bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
    CRLog::info("Initializing gettext: dir=%s, LANGUAGE=%s, DOMAIN=%s, bindtxtdomain result = %s", LOCALEDIR, getenv("LANGUAGE"), PACKAGE, bindres);
    CRLog::info("Trying to translate: 'On'='%s'", gettext("On"));
    CRLog::info("Trying to translate: 'About...'='%s'", gettext("About..."));

    lString16Collection fontDirs;
    //fontDirs.add( lString16(L"/usr/local/share/cr3/fonts") );
    //fontDirs.add( lString16(L"/usr/local/share/fonts/truetype/freefont") );
    //fontDirs.add( lString16(L"/mnt/fonts") );
    //fontDirs.add( lString16(L"/usr/share/fonts/truetype") );
    //fontDirs.add( lString16(L"/usr/share/fonts/truetype/liberation") );
    //fontDirs.add( lString16(L"/usr/share/fonts/truetype/freefont") );
    //fontDirs.add( lString16(L"/root/fonts/truetype") );
    if ( !InitCREngine( argv[0], fontDirs ) ) {
        printf("Cannot init CREngine - exiting\n");
        return 2;
    }

    if ( argc<2 ) {
        printf("Usage: cr3 <filename_to_open>\n");
        return 3;
    }

    const char * fname = argv[1];
    const char * bmkdir = NULL;

    if ( !strcmp(fname, "unittest") ) {
        runCRUnitTests();
        return 0;
    }

    lString8 fn8( fname );
    lString16 fn16 = LocalToUnicode( fn8 );
    CRLog::info("Filename to open=\"%s\"", LCSTR(fn16) );
    if ( fn8.startsWith( lString8("/media/sd/") ) )
        bmkdir = "/media/sd/bookmarks/";
    //TODO: remove hardcoded
    int bitDepth = 4; //TODO
#ifdef __i386__
        CRQtWindowManager winman( 600, 680, bitDepth );
#else
        CRQtWindowManager winman( 600, 800, bitDepth );
#endif
    if ( !ldomDocCache::init( lString16(L"/media/sd/.cr3/cache"), 0x100000 * 64 ))
        ldomDocCache::init( lString16(L"/tmp/.cr3/cache"), 0x100000 * 64 ); /*64Mb*/

    {

        lString16 home = Utf8ToUnicode(lString8(( getenv("HOME") ) ));
        lString16 homecrengine = home + L"/.crengine/";

        lString8 home8 = UnicodeToUtf8( homecrengine );
        const char * keymap_locations [] = {
            "/etc/cr3",
            "/usr/share/cr3",
            home8.c_str(),
            "/media/sd/crengine/",
            NULL,
        };
        loadKeymaps( winman, keymap_locations );

        if ( !winman.loadSkin(  homecrengine + L"skin" ) )
            if ( !winman.loadSkin(  lString16( L"/media/sd/crengine/skin" ) ) )
            	winman.loadSkin( lString16( L"/usr/share/cr3/skins/default" ) );
        {
            const lChar16 * imgname =
                ( winman.getScreenOrientation()&1 ) ? L"cr3_logo_screen_landscape.png" : L"cr3_logo_screen.png";
            LVImageSourceRef img = winman.getSkin()->getImage(imgname);
            if ( !img.isNull() ) {
                winman.getScreen()->getCanvas()->Draw(img, 0, 0, winman.getScreen()->getWidth(), winman.getScreen()->getHeight(),  false );
            }
        }
        HyphMan::initDictionaries( lString16("/usr/share/cr3/hyph/") );
        //LVExtractPath(LocalToUnicode(lString8(fname)))
        main_win = new QtDocViewWin( &winman, lString16(CRSKIN) );
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if ( !main_win->loadDefaultCover( lString16( L"/media/sd/crengine/cr3_def_cover.png" ) ) )
            main_win->loadDefaultCover( lString16( L"/usr/share/cr3/cr3_def_cover.png" ) );
        if ( !main_win->loadCSS(  lString16( L"/media/sd/crengine/fb2.css" ) ) )
            main_win->loadCSS( lString16( L"/usr/share/cr3/fb2.css" ) );

        if ( !main_win->loadDictConfig(  lString16( L"/media/sd/crengine/dict/dictd.conf" ) ) )
            main_win->loadDictConfig( lString16( L"/usr/share/cr3/dict/dictd.conf" ) );
        if ( bmkdir!=NULL )
            main_win->setBookmarkDir( lString16(bmkdir) );

    #define SEPARATE_INI_FILES

        CRLog::trace("choosing init file...");
        const lChar16 * ini_fname = L"cr3.ini";
    #ifdef SEPARATE_INI_FILES
        if ( strstr(fname, ".txt")!=NULL || strstr(fname, ".tcr")!=NULL) {
            ini_fname = L"cr3-txt.ini";
        } else if ( strstr(fname, ".rtf")!=NULL ) {
            ini_fname = L"cr3-rtf.ini";
        } else if ( strstr(fname, ".htm")!=NULL ) {
            ini_fname = L"cr3-htm.ini";
        } else if ( strstr(fname, ".epub")!=NULL ) {
            ini_fname = L"cr3-epub.ini";
        } else {
            ini_fname = L"cr3-fb2.ini";
        }
    #endif
        const lChar16 * dirs[] = {
            L"/media/sd/crengine/",
            homecrengine.c_str(),
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
        lString16 hist;
        for ( i=0; dirs[i]; i++ ) {
            hist = lString16(dirs[i]) + L"cr3hist.bmk";
            if ( main_win->loadHistory( hist ) ) {
                break;
            }
        }

#if 0
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
#endif
        winman.activateWindow( main_win );
        if ( !main_win->loadDocument( LocalToUnicode((lString8(fname))) ) ) {
            printf("Cannot open book file %s\n", fname);
            res = 4;
        } else {
            winman.runEventLoop();
        }
        }
    }
    HyphMan::uninit();
    ldomDocCache::close();
    ShutdownCREngine();

    return res;
}

