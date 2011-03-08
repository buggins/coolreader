

#include "cr3inkview.h"

#include "cr3main.h"
#include "mainwnd.h"

#include <inkview.h>

#define CR3PATH USERDATA "/share/crengine"

#define LOGFILE CR3PATH "/cr3log.ini"
#define SKINPATH CR3PATH "/skins/"
#define FONTPATH SYSTEMDATA "/fonts/"
#define HYPHPATH CR3PATH "/hyph/"
#define CRCACHEPATH CR3PATH "/.cache"
#define KEYMAPPATH CR3PATH "/keymaps/"


CRInkViewWindowManager * wm;

void CRInkViewScreen::update(const lvRect& rc2, bool full)
{
    char buf[2000];
    CRLog::trace("CRInkViewScreen::update()");
    icanvas * canvas = new(icanvas);
    lvRect * clipRect;
    canvas->width = _front->GetWidth();
    canvas->height = _front->GetHeight();
    canvas->scanline = _front->GetWidth();
    canvas->depth = _front->GetBitsPerPixel();
    _front->GetClipRect(clipRect);
    canvas->clipx1 = clipRect->left;
    canvas->clipx2 = clipRect->right-1;
    canvas->clipy1 = clipRect->top;
    canvas->clipy2 = clipRect->bottom-1;
/*    snprintf(buf, sizeof(buf), "w %d h %d s %d d %d", canvas->width, canvas->height, canvas->scanline, canvas->depth);
    CRLog::trace(buf);
    snprintf(buf, sizeof(buf), "x1 %d x2 %d y1 %d y2 %d", canvas->clipx1, canvas->clipx2, canvas->clipy1, canvas->clipy2);
    CRLog::trace(buf);
    snprintf(buf, sizeof(buf), "back %d txt %d b %d w %d",_front->GetBackgroundColor(), _front->GetTextColor(), _front->GetBlackColor(), _front->GetWhiteColor());
    CRLog::trace(buf);*/

    canvas->addr = _front->GetScanLine(0);
    // TODO: this should work!! Why not??
//    CRLog::trace("CRInkViewScreen::update() SetCanvas()");
//    SetCanvas(canvas);
    Stretch(canvas->addr, IMAGE_GRAY8, canvas->width, canvas->height, canvas->scanline, 0, 0, canvas->width, canvas->height, 0);
    CRLog::trace("CRInkViewScreen::update() FullUpdate()");
    DitherArea(0, 0, canvas->width, canvas->height, 16, DITHER_PATTERN);
    FullUpdate();
}

void CRInkViewWindowManager::closeAllWindows()
{
    CRGUIWindowManager::closeAllWindows();
    CloseApp();
}


CRInkViewScreen::CRInkViewScreen(int width, int height): CRGUIScreenBase(width, height, true)
{

}


CRInkViewWindowManager::CRInkViewWindowManager(int width, int height): CRGUIWindowManager(NULL)
{
    _screen = new CRInkViewScreen(width, height);
}

void CRInkViewWindowManager::update(bool fullScreenUpdate, bool forceFlushScreen)
{
    CRLog::trace("CRInkViewWindowManager::update()");
    CRGUIWindowManager::update(fullScreenUpdate, forceFlushScreen);
}


int InitDoc(char *fileName)
{
    static const lChar16 * css_file_name = L"fb2.css"; // fb2

    CRLog::trace("InitDoc()");
    CRLog::trace("creating window manager...");
    wm = new CRInkViewWindowManager(600,800);

    CRLog::trace("loading skin...");
    wm->loadSkin(  lString16( SKINPATH ) );

    lString16Collection fontDirs;
    fontDirs.add( lString16( FONTPATH ) );
    CRLog::info("INIT...");
    if ( !InitCREngine( CR3PATH, fontDirs ) )
        return 0;

    const char * keymap_locations [] = {
        KEYMAPPATH,
        NULL,
    };
    loadKeymaps( *wm, keymap_locations );
    HyphMan::initDictionaries( lString16( HYPHPATH ) );
    if ( !ldomDocCache::init( lString16( CRCACHEPATH ), 0x100000 * 64 ) ) {
        CRLog::error("Cannot initialize swap directory");
    }
    CRLog::trace("creating main window...");
    V3DocViewWin * main_win = new V3DocViewWin( wm, lString16(CR3PATH) );    
//    if ( manual_file[0] )
//        main_win->setHelpFile( lString16( manual_file ) );
//    main_win->loadDefaultCover( lString16( L"/root/abook/crengine/cr3_def_cover.png" ) )

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
    CRLog::trace("loading CSS...");
    main_win->loadCSS(  lString16( CR3PATH ) + lString16( L"/" ) + lString16(css_file_name) );

    CRLog::trace("loading Settings...");
    main_win->loadSettings( lString16( CR3PATH )  + lString16( L"/" ) + ini_fname );
//    main_win->setBookmarkDir( bookmarkDir );
    wm->activateWindow( main_win );
    if ( !main_win->loadDocument( lString16(fileName) ) ) {
        CRLog::error("Cannot open book file %s\n", fileName);
        delete wm;
        return 0;
    }
//    main_win->prepareNextPageImage(1);
    main_win->onCommand(MCMD_GO_PAGE_APPLY, 20);
//    wm->update(true, true);
    wm->handleAllEvents(false);
    Repaint();
    return 1;
}

char * fname;

int main_handler(int type, int par1, int par2)
{
    switch (type) {
        case EVT_INIT:
            CRLog::trace("EVT_INIT");
            if ( !InitDoc( fname ) ) {
                CloseApp();
            }
            break;
        case EVT_SHOW:
            CRLog::trace("EVT_SHOW");
            wm->update(true, true);
            break;
        case EVT_KEYRELEASE:
            CRLog::trace("EVT_KEYRELEASE");
            switch (par1) {
                default:
                    wm->postEvent( new CRGUIKeyDownEvent(par1, 0) );
                    wm->handleAllEvents(false);
                    break;
            }

    }
    
    
}

int main( int argc, const char * argv[] )
{
    if ( argc<2 ) {
        printf("usage: cr3 <filename>\n");
        return 1;
    }
//    InitCREngineLog(LOGFILE);
    CRLog::setFileLogger("/mnt/ext1/system/share/crengine/cr3.log", true);
    CRLog::setLogLevel(CRLog::LL_TRACE);
    
    fname = (char *)argv[1];

    InkViewMain(main_handler);

    return 0;
}
