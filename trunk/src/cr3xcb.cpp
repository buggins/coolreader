/*
    First version of CR3 for EWL, based on etimetool example by Lunohod
*/
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
//#include <Ewl.h>
#include <crengine.h>
#include <crgui.h>
#include <crtrace.h>


#include "cr3main.h"
#include "mainwnd.h"

// XCB code ===================================================================

#ifndef _WIN32

#ifdef CR_USE_XCB

#include <unistd.h>      /* pause() */
#include <sys/types.h>
#include <sys/stat.h>

#include <xcb/xcb.h>
int8_t i8sample = 0;
#include <xcb/xcb_aux.h>
extern "C" {
#include <xcb/shm.h>
};
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define XCB_ALL_PLANES ~0

static xcb_connection_t *connection;
static xcb_window_t window;
static xcb_screen_t *screen;


/// WXWidget support: draw to wxImage
class CRXCBScreen : public CRGUIScreenBase
{
    public:
        xcb_connection_t * getXcbConnection() { return connection; }
        xcb_window_t getXcbWindow() { return window; }
        xcb_screen_t * getXcbScreen() { return screen; }
    protected:
        xcb_gcontext_t      gc;
        xcb_gcontext_t      bgcolor;
        unsigned int pal_[4];
        xcb_drawable_t rect;
        xcb_shm_segment_info_t shminfo;
        xcb_image_t *im;
        unsigned int *pal;
        virtual void update( const lvRect & rc, bool full )
        {
            printf("update screen, bpp=%d width=%d, height=%d, rect={%d, %d, %d, %d} full=%d\n", (int)im->bpp,im->width,im->height, (int)rc.left, (int)rc.top, (int)rc.right, (int)rc.bottom, full?1:0 );
            if ( rc.isEmpty() )
                return;
            int i;
            i = xcb_image_shm_get (connection, window,
                    im, shminfo,
                    0, 0,
                    XCB_ALL_PLANES);
            if (!i) {
                printf("cannot get shm image\n");
                return;
            }
            printf("update screen, bpp=%d\n", (int)im->bpp);

            // pal
            //static lUInt32 pal[4] = {0x000000, 0x555555, 0xaaaaaa, 0xffffff };
            //static lUInt8 pal8[4] = {0x00, 0x55, 0xaa, 0xff };
            switch ( im->bpp ) {
            case 32:
                {
                    for ( int y = rc.top; y<rc.bottom; y++ ) {
                        lUInt8 * src = _front->GetScanLine( y );
                        lUInt32 * dst = (lUInt32 *)(im->data + im->stride * y);
                        //printf("line %d : %08X -> %08X   ", y, src, dst);
                        int shift = 6;
                        for ( int x = 0; x< _width; x++ ) {
                            lUInt8 data = src[x>>2];
                            int pixel = (data>>shift) & 3;
                            lUInt32 color = pal_[ pixel ]; // to check valgrind finding
                            dst[x] = color;
                            shift -= 2;
                            if ( shift < 0 )
                                shift = 6;
                        }
                    }
                }
                break;
            case 8:
                {
                    for ( int y = rc.top; y<rc.bottom; y++ ) {
                        lUInt8 * src = _front->GetScanLine( y );
                        lUInt8 * dst = (lUInt8 *)(im->data + im->stride * y);
                        //printf("line %d : %08X -> %08X   ", y, src, dst);
                        int shift = 6;
                        for ( int x = 0; x< _width; x++ ) {
                            lUInt8 data = src[x>>2];
                            int pixel = (data>>shift) & 3;
                            lUInt8 color = (lUInt8)pal_[ pixel ]; // to check valgrind finding
                            dst[x] = color;
                            shift -= 2;
                            if ( shift < 0 )
                                shift = 6;
                        }
                    }
                }
                break;
            case 2:
                {
                    for ( int y = rc.top; y<rc.bottom; y++ ) {
                        lUInt8 * src = _front->GetScanLine( y );
                        lUInt8 * dst = (lUInt8 *)(im->data + im->stride * y);
                        memcpy( dst, src, _width>>2 );
                    }
                }
                break;
            default:
                printf("unsupported bpp %d\n", im->bpp);
                break;
            }
            //pContext.image = im;
            printf("updated\n");

            //view()->paint();

            xcb_image_shm_put (connection, window, gc,
                    im, shminfo,
                    rc.left, rc.top, rc.left, rc.top, rc.width(), rc.height(), 0);
            xcb_flush(connection);
        }
    public:
        virtual ~CRXCBScreen()
        {
            if ( im )
                xcb_image_destroy( im );
            if ( connection )
                xcb_disconnect( connection );
        }
        CRXCBScreen( int width, int height )
        :  CRGUIScreenBase( 0, 0, true )
        {
            pal_[0] = 0x000000;
            pal_[1] = 0x555555;
            pal_[2] = 0xaaaaaa;
            pal_[3] = 0xffffff;
            //xcb_screen_iterator_t screen_iter;
            //const xcb_setup_t    *setup;
            //xcb_generic_event_t  *e;
            //xcb_generic_error_t  *error;
            //xcb_void_cookie_t     cookie_window;
            //xcb_void_cookie_t     cookie_map;
            uint32_t              mask;
            uint32_t              values[2];
            int                   screen_number;
            //uint8_t               is_hand = 0;

            /* getting the connection */
            connection = xcb_connect (NULL, &screen_number);
            if (xcb_connection_has_error(connection)) {
                fprintf (stderr, "ERROR: can't connect to an X server\n");
                exit(-1);
            }

            screen = xcb_aux_get_screen (connection, screen_number);
            if ( width <= 0 || width > screen->width_in_pixels )
                width = screen->width_in_pixels;
            if ( height <= 0 || height > screen->height_in_pixels )
                height = screen->height_in_pixels;
            //xcb_rectangle_t rect_coord = { 0, 0, width, height};

            gc = xcb_generate_id (connection);
            mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
            values[0] = screen->black_pixel;
            values[1] = 0; /* no graphics exposures */
            xcb_create_gc (connection, gc, screen->root, mask, values);

            bgcolor = xcb_generate_id (connection);
            mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
            values[0] = screen->white_pixel;
            values[1] = 0; /* no graphics exposures */
            xcb_create_gc (connection, bgcolor, screen->root, mask, values);

            /* creating the window */
            window = xcb_generate_id(connection);
            mask =  XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
            xcb_params_cw_t params_cw;
            params_cw.back_pixel = screen->white_pixel;
            params_cw.event_mask =
                XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_EXPOSURE |
                XCB_EVENT_MASK_POINTER_MOTION;

            uint8_t depth = xcb_aux_get_depth (connection, screen);
            printf("depth = %d, root depth = %d\n",depth, screen->root_depth);
            xcb_aux_create_window(connection,
                    depth,
                    window, screen->root,
                    0, 0, width, height,
                    0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    mask, &params_cw);

            rect = xcb_generate_id (connection);

            xcb_void_cookie_t cookie;
            cookie = xcb_create_pixmap_checked (connection, depth,
                    rect, window,
                    width, height);
            if (xcb_request_check(connection,cookie)){
                printf("sucks, can't creae pixmap\n");
            }

            xcb_map_window(connection, window);

            xcb_colormap_t    colormap;
            colormap = screen->default_colormap;

            xcb_alloc_color_reply_t *rep;
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0, 0, 0), NULL);
            pal_[0] = rep->pixel;
            free(rep);
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0x55<<8, 0x55<<8, 0x55<<8), NULL);
            pal_[1] = rep->pixel;
            free(rep);
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xaa<<8, 0xaa<<8, 0xaa<<8), NULL);
            pal_[2] = rep->pixel;
            free(rep);
            rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xff<<8, 0xff<<8, 0xff<<8), NULL);
            pal_[3] = rep->pixel;
            free(rep);

            pal = pal_;

            xcb_shm_query_version_reply_t *rep_shm;

            rep_shm = xcb_shm_query_version_reply (connection,
                    xcb_shm_query_version (connection),
                    NULL);
            if(rep_shm) {
                xcb_image_format_t format;
                int shmctl_status;

                if (rep_shm->shared_pixmaps &&
                        (rep_shm->major_version > 1 || rep_shm->minor_version > 0))
                    format = (xcb_image_format_t)rep_shm->pixmap_format;
                else
                    format = (xcb_image_format_t)0;

                im = xcb_image_create_native (connection, width, height,
                        format, depth, NULL, ~0, NULL);
                assert(im);

                shminfo.shmid = shmget (IPC_PRIVATE,
                        im->stride*im->height,
                        IPC_CREAT | 0777);
                assert(shminfo.shmid != (unsigned)-1);
                shminfo.shmaddr = (uint8_t*)shmat (shminfo.shmid, 0, 0);
                assert(shminfo.shmaddr);
                im->data = shminfo.shmaddr;
                printf("Created image depth=%d bpp=%d stride=%d\n", (int)im->depth, (int)im->bpp, (int)im->stride );

                shminfo.shmseg = xcb_generate_id (connection);
                xcb_shm_attach (connection, shminfo.shmseg,
                        shminfo.shmid, 0);
                shmctl_status = shmctl(shminfo.shmid, IPC_RMID, 0);
                assert(shmctl_status != -1);
                free (rep_shm);

            } else {
                printf("Can't get shm\n");
            }
            _width = width;
            _height = height;
            _canvas = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );
            _front = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );

            xcb_flush(connection);
            printf("Created screen %d x %d, depth = %d\n", _width, _height, depth );
        }
};


class CRXCBWindowManager : public CRGUIWindowManager
{
protected:
    xcb_connection_t * _connection;
public:
    CRXCBWindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL)
    {
        CRXCBScreen * s = new CRXCBScreen( dx, dy );
        _screen = s;
        _connection = s->getXcbConnection();
        _ownScreen = true;
    }

    virtual bool getBatteryStatus( int & percent, bool & charging )
    {
        charging = false;
        percent = 0;
//TODO: implement battery state conditional compilation for different devices
#ifdef __arm__

        int x, charge;
        FILE *f_cf, *f_cn;

        f_cn = fopen("/sys/class/power_supply/lbookv3_battery/charge_now", "r");
        f_cf = fopen("/sys/class/power_supply/lbookv3_battery/charge_full_design", "r");

        char b[11];
        if((f_cn != NULL) && (f_cf != NULL)) {
            fgets(b, 10, f_cn);
            charge = atoi(b);
            fgets(b, 10, f_cf);
            x = atoi(b);
            if(x > 0)
                charge = charge * 100 / x;
        } else
            charge = 0;

        if (f_cn != NULL)
            fclose(f_cn);
        if (f_cf != NULL)
            fclose(f_cf);
        percent = charge;
        return true;
#else
        return false;
#endif

    }
    // runs event loop
    virtual int runEventLoop()
    {
        xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc( connection );

        xcb_generic_event_t *event;
        bool stop = false;
        while (!stop && (event = xcb_wait_for_event (connection)) ) {
            bool needUpdate = false;
            //processPostedEvents();
            switch (event->response_type & ~0x80) {
            case XCB_EXPOSE:
                // draw buffer
                {
                    printf("EXPOSE\n");
                    update(true);
                }
                break;
            case XCB_KEY_RELEASE:
                {
                    xcb_key_press_event_t *release = (xcb_key_press_event_t *)event;
#define XCB_LOOKUP_CHARS_T 2
                    xcb_keycode_t key = release->detail;
                    int state = (release->state & XCB_MOD_MASK_1) ? KEY_FLAG_LONG_PRESS : 0;
                    xcb_keysym_t sym = xcb_key_symbols_get_keysym( keysyms,
                                            key,
                                            XCB_LOOKUP_CHARS_T); //xcb_lookup_key_sym_t xcb_lookup_chars_t
                    printf("Key released keycode=%d char=%04x state=%d\n", (int)key, (int)sym, state );
                    onKeyPressed( sym, state );
                    //printf("page number = %d\n", main_win->getDocView()->getCurPage());
                    needUpdate = true;
                }
                break;
            case XCB_BUTTON_PRESS:
                {
                    //xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
                }
                break;
            default:
                /* Unknown event type, ignore it */
                break;
            }

            free (event);

            if ( processPostedEvents() || needUpdate )
                update(false);
            // stop loop if all windows are closed
            if ( !getWindowCount() )
                stop = true;

        }

        xcb_key_symbols_free( keysyms );
        return 0;
    }
};


int main(int argc, char **argv)
{
    int res = 0;

    {
    CRLog::setStdoutLogger();
    CRLog::setLogLevel( CRLog::LL_TRACE );
    #if 0
    // memory leak test
    {
        {
            lString8 s;
            s << "bla bla bla";
            lString8 s2("xxxxx");
            s << s2;
            lString8 * array = new lString8[25];
            array[2] = lString8("text1");
            array[6] = lString8("text2");
            array[24] = lString8("text3");
            for ( int k=0; k<10000; k++ )
                array[7] << "123";
            typedef LVRef<int> IntRef;
            delete [] array;
            {
                LVCacheMap <int, IntRef> map( 20 );
                map.set(1, IntRef( new int(3) ));
                map.set(2, IntRef( new int(4) ));
            }
            lString8 buf;
            lStringBuf8<100> proxy( buf );
            for ( int i=0; i<5000; i++ )
                buf << 'A';
        }
        ShutdownCREngine();
        return 0;
    }
    #endif


    lString16Collection fontDirs;
    //fontDirs.add( lString16(L"/usr/local/share/crengine/fonts") );
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

    if ( argc!=2 ) {
        printf("Usage: cr3 <filename_to_open>\n");
        return 3;
    }

    const char * fname = argv[1];
    //TODO: remove hardcoded
#ifdef __i386__
        CRXCBWindowManager winman( 600, 700 );
#else
        CRXCBWindowManager winman( 600, 800 );

#endif
        lString16 home = Utf8ToUnicode(lString8(( getenv("HOME") ) ));
        lString16 homecrengine = home + L"/.crengine/";

        if ( !winman.getAccTables().openFromFile(  UnicodeToUtf8(homecrengine + L"/keydefs.ini").c_str(), 
                    UnicodeToUtf8(homecrengine + L"/keymaps.ini").c_str() ) )
            winman.getAccTables().openFromFile(  "/etc/cr3/keydefs.ini", "/etc/cr3/keymaps.ini" );
        if ( winman.getAccTables().empty() ) {
            CRLog::error("keymap files keydefs.ini and keymaps.ini were not found! please place them to ~/.crengine or /etc/cr3");
        }
        static const int menu_acc_table[] = {
            XK_Escape, 0, MCMD_CANCEL, 0,
            XK_Return, 0, MCMD_OK, 0, 
            XK_Return, 1, MCMD_OK, 0, 
            '0', 0, MCMD_SCROLL_FORWARD, 0,
            XK_Down, 0, MCMD_SCROLL_FORWARD, 0,
            '9', 0, MCMD_SCROLL_BACK, 0,
            XK_Up, 0, MCMD_SCROLL_BACK, 0,
            '0', 1, MCMD_LONG_FORWARD, 0,
            XK_Down, 1, MCMD_LONG_FORWARD, 0,
            '9', 1, MCMD_LONG_BACK, 0,
            XK_Up, 1, MCMD_LONG_BACK, 0,
            '1', 0, MCMD_SELECT_1, 0,
            '2', 0, MCMD_SELECT_2, 0,
            '3', 0, MCMD_SELECT_3, 0,
            '4', 0, MCMD_SELECT_4, 0,
            '5', 0, MCMD_SELECT_5, 0,
            '6', 0, MCMD_SELECT_6, 0,
            '7', 0, MCMD_SELECT_7, 0,
            '8', 0, MCMD_SELECT_8, 0,
            0
        };
        if ( winman.getAccTables().get("menu").isNull() )
            winman.getAccTables().add("menu", menu_acc_table );
        static const int acc_table_dialog[] = {
            XK_Escape, 0, MCMD_CANCEL, 0,
            XK_Return, 1, MCMD_OK, 0, 
            XK_Return, 0, MCMD_OK, 0, 
            XK_Down, 0, MCMD_SCROLL_FORWARD, 0,
            XK_Up, 0, MCMD_SCROLL_BACK, 0,
            '0', 0, MCMD_SELECT_0, 0,
            '1', 0, MCMD_SELECT_1, 0,
            '2', 0, MCMD_SELECT_2, 0,
            '3', 0, MCMD_SELECT_3, 0,
            '4', 0, MCMD_SELECT_4, 0,
            '5', 0, MCMD_SELECT_5, 0,
            '6', 0, MCMD_SELECT_6, 0,
            '7', 0, MCMD_SELECT_7, 0,
            '8', 0, MCMD_SELECT_8, 0,
            '9', 0, MCMD_SELECT_9, 0,
            0
        };
        if ( winman.getAccTables().get("dialog").isNull() )
            winman.getAccTables().add("dialog", acc_table_dialog );
        static const int default_acc_table[] = {
            '6', 0, MCMD_GO_LINK, 0,
            '8', 0, MCMD_SETTINGS_FONTSIZE, 0,
            '8', 1, MCMD_SETTINGS_ORIENTATION, 0,
            XK_Escape, 0, MCMD_QUIT, 0,
            XK_Return, 0, MCMD_MAIN_MENU, 0,
            XK_Return, 1, MCMD_SETTINGS, 0,
            '0', 0, DCMD_PAGEDOWN, 0,
            XK_Up, 0, DCMD_PAGEDOWN, 0,
            '0', KEY_FLAG_LONG_PRESS, DCMD_PAGEDOWN, 10,
            XK_Up, KEY_FLAG_LONG_PRESS, DCMD_PAGEDOWN, 10,
            XK_Down, 0, DCMD_PAGEUP, 0,
            XK_Down, KEY_FLAG_LONG_PRESS, DCMD_PAGEUP, 10,
            '9', 0, DCMD_PAGEUP, 0,
            '9', KEY_FLAG_LONG_PRESS, DCMD_PAGEUP, 10,
    #ifdef WITH_DICT
            '2', 0, MCMD_DICT, 0,
    #endif
            '+', 0, DCMD_ZOOM_IN, 0,
            '=', 0, DCMD_ZOOM_IN, 0,
            '-', 0, DCMD_ZOOM_OUT, 0,
            '_', 0, DCMD_ZOOM_OUT, 0,
            0
        };
        if ( winman.getAccTables().get("main").isNull() )
            winman.getAccTables().add("main", default_acc_table );

        //LVExtractPath(LocalToUnicode(lString8(fname)))
        V3DocViewWin * main_win = new V3DocViewWin( &winman, lString16(CRSKIN) );
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if ( !main_win->loadDefaultCover( lString16( L"/media/sd/crengine/cr3_def_cover.png" ) ) )
            main_win->loadDefaultCover( lString16( L"/usr/share/crengine/cr3_def_cover.png" ) );
        if ( !main_win->loadCSS(  lString16( L"/media/sd/crengine/fb2.css" ) ) )
            main_win->loadCSS( lString16( L"/usr/share/crengine/fb2.css" ) );
        if ( !main_win->loadSkin(  lString16( L"/media/sd/crengine/skin" ) ) )
            main_win->loadSkin( lString16( L"/usr/share/crengine/skin" ) );
        if ( !main_win->loadDictConfig(  lString16( L"/media/sd/crengine/dict/dictd.conf" ) ) )
            main_win->loadDictConfig( lString16( L"/usr/share/crengine/dict/dictd.conf" ) );

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
    ShutdownCREngine();

    return res;
}

#endif
#endif
