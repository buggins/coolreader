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

// XCB code ===================================================================

#ifndef _WIN32

//#ifdef CR_USE_XCB

#include <unistd.h>      /* pause() */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <signal.h>

#include <xcb/xcb.h>
int8_t i8sample = 0;
#define class klass
extern "C" {
#include <xcb/xcb_aux.h>
#include <xcb/shm.h>
#include <xcb/randr.h>
#include <xcb/xcb_atom.h>
}
#undef class
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define XCB_ALL_PLANES ~0

#define PIDFILE "/tmp/cr3.pid"
#define CR3_FIFO "/tmp/.cr3-fifo"
#ifndef NAME_MAX
#define NAME_MAX 4096
#endif

// For Turbo Updates feature
#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC       _IOW('F', 0x20, uint32_t)
#endif
#define EINK_APOLLOFB_IOCTL_SET_AUTOREDRAW _IOW('F', 0x21, unsigned int)
#define EINK_APOLLOFB_IOCTL_FORCE_REDRAW _IO('F', 0x22)
#define EINK_APOLLOFB_IOCTL_SHOW_PREVIOUS _IO('F', 0x23)

//=================================
// START OF LIBEOI BATTERY SUPPORT
// OpenInkpot: libeoi-battery
//=================================
typedef enum {
    UNKNOWN,
    CHARGING,
    DISCHARGING,
    NOT_CHARGING,
    FULL_CHARGE,
    LOW_CHARGE
} battery_status_t;

typedef struct {
    battery_status_t status;

    /* This field is reliable only in DISCHARGING and LOW_CHARGE states */
    int charge;
} battery_info_t;

typedef struct {
    const char *now;
    const char *min;
    const char *max;
    const char *status;
} battery_loc_t;

static battery_loc_t batteries[] = {
    {
     "/sys/class/power_supply/n516-battery/charge_now",
     "/sys/class/power_supply/n516-battery/charge_empty_design",
     "/sys/class/power_supply/n516-battery/charge_full_design",
     "/sys/class/power_supply/n516-battery/status",
     },
    {
     "/sys/class/power_supply/lbookv3_battery/charge_now",
     "/sys/class/power_supply/lbookv3_battery/charge_empty_design",
     "/sys/class/power_supply/lbookv3_battery/charge_full_design",
     "/sys/class/power_supply/lbookv3_battery/status",
     },
};

static int
_find_battery()
{
    unsigned int i;
    for (i = 0; i < sizeof(batteries) / sizeof(batteries[0]); ++i)
        if (!access(batteries[i].now, R_OK))
            return i;
    return -1;
}

static battery_status_t
_read_status_file(const char *filename)
{
    char buf[256];
    FILE *f = fopen(filename, "r");
    if (!f)
        return UNKNOWN;
    if (!fgets(buf, 255, f))
        *buf = '\0';
    fclose(f);

    if (!strcmp(buf, "Charging\n"))
        return CHARGING;
    if (!strcmp(buf, "Discharging\n"))
        return DISCHARGING;
    if (!strcmp(buf, "Not charging\n"))
        return NOT_CHARGING;
    if (!strcmp(buf, "Full\n"))
        return FULL_CHARGE;
    if (!strcmp(buf, "Low charge\n"))
        return LOW_CHARGE;
    return UNKNOWN;
}

static int
_read_int_file(const char *filename)
{
    int res;
    FILE *f = fopen(filename, "r");
    if (1 != fscanf(f, "%d", &res))
        res = 0;
    fclose(f);
    return res;
}

void
eoi_get_battery_info(battery_info_t * info)
{
    static int batt = -1;
    if ( batt == -1 )
        batt = _find_battery();

    if (batt == -1) {
        info->status = UNKNOWN;
        info->charge = -1;
    } else {
        int now = _read_int_file(batteries[batt].now);
        int min = _read_int_file(batteries[batt].min);
        int max = _read_int_file(batteries[batt].max);
        info->status = _read_status_file(batteries[batt].status);
        if (max > min && now >= min && now <= max)
            info->charge = 100 * (now - min) / (max - min);
        else
            info->charge = -1;
    }
}

//=================================
// END OF LIBEOI BATTERY SUPPORT
//=================================


static xcb_connection_t *connection = NULL;
static xcb_window_t window = NULL;
static xcb_screen_t *screen = NULL;
static V3DocViewWin * main_win = NULL;


/// WXWidget support: draw to wxImage
class CRXCBScreen : public CRGUIScreenBase
{
    public:
        xcb_connection_t * getXcbConnection() { return connection; }
        xcb_window_t getXcbWindow() { return window; }
        xcb_screen_t * getXcbScreen() { return screen; }
        bool _turboModeSupported;


        bool enableAutoDraw()
        {
            int f;
            bool res = false;
            f = open("/dev/fb0", O_NONBLOCK);
            if ( f!=-1 ) {
                ioctl(f, FBIO_WAITFORVSYNC);
                res = ioctl(f, EINK_APOLLOFB_IOCTL_SET_AUTOREDRAW, 1)!=-1;
                close(f);
            }
            return res;
        }
        bool disableAutoDraw()
        {
            int f;
            bool res = false;
            f = open("/dev/fb0", O_NONBLOCK);
            if ( f!=-1 ) {
                ioctl(f, FBIO_WAITFORVSYNC);
                res = ioctl(f, EINK_APOLLOFB_IOCTL_SET_AUTOREDRAW, 0)!=-1;
                close(f);
            }
            return res;
        }
        bool forceDraw()
        {
            bool res = false;
            int f;
            f = open("/dev/fb0", O_NONBLOCK);
            if ( f!=-1 ) {
                ioctl(f, FBIO_WAITFORVSYNC);
                res = ioctl(f, EINK_APOLLOFB_IOCTL_FORCE_REDRAW)!=-1;
                close(f);
            }
            return res;
        }
        virtual void setTurboUpdateEnabled( bool flg ) { _turbo = flg && _turboModeSupported; }
        virtual bool getTurboUpdateEnabled() {  return _turbo; }
        virtual bool getTurboUpdateSupported() {  return _turboModeSupported; }
        virtual void setTurboUpdateMode( CRGUIScreen::UpdateMode mode )
        {
            if ( _mode==mode )
                return;
            if ( mode==CRGUIScreen::PrepareMode ) {
                _forceNextUpdate = true;
                _forceUpdateRect.clear();
            }
            _mode = mode;
        }
    protected:
        xcb_gcontext_t      gc;
        xcb_gcontext_t      bgcolor;
        unsigned int pal_[4];
        unsigned int pal8_[8];
        unsigned int pal16_[16];
        unsigned int pal256_[256];
        xcb_drawable_t rect;
        xcb_shm_segment_info_t shminfo;
        xcb_image_t *im;
        unsigned int *pal;
        uint8_t depth;
        int bufDepth;
        bool _turbo;
        UpdateMode _mode;
        bool _forceNextUpdate;
        lvRect _forceUpdateRect;
        /// sets new screen size
        virtual bool setSize( int dx, int dy )
        {
            if ( !CRGUIScreenBase::setSize( dx, dy ) )
                return false;
            createImage();
            return true;
        }
        void createImage()
        {
            CRLog::info("CRXCBScreen::createImage(%d, %d)", _width, _height);
            if ( im )
                xcb_image_destroy( im );
            im = NULL;
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
                    format = XCB_IMAGE_FORMAT_Z_PIXMAP;

                im = xcb_image_create_native (connection, _width, _height,
                     format, depth, NULL, ~0, NULL);
#if 0
                if ( !im ) {
                    CRLog::trace("Failed image format %d %d", format, depth);
                    int sz = _width*_height*4;
                    lUInt8 * data = (lUInt8*)malloc(sz);
                    int params[] = {
                        XCB_IMAGE_FORMAT_XY_BITMAP, 24,
                        XCB_IMAGE_FORMAT_XY_PIXMAP, 24,
                        XCB_IMAGE_FORMAT_Z_PIXMAP, 24,
                        XCB_IMAGE_FORMAT_XY_BITMAP, 32,
                        XCB_IMAGE_FORMAT_XY_PIXMAP, 32,
                        XCB_IMAGE_FORMAT_Z_PIXMAP, 32,
                        0, 0,
                    };
                    for ( int i=0; params[i+1]; i+=2 ) {
                        im = xcb_image_create_native (connection, _width, _height,
                             (xcb_image_format_t)params[i], params[i+1], NULL, ~0, NULL);
                        if ( im ) {
                            CRLog::trace("Passed image format %d %d", params[i], params[i+1]);
                            break;
                        }
                        CRLog::trace("Failed image format %d %d", params[i], params[i+1]);
                    }
                }
#endif
                //format, depth, NULL, ~0, NULL);
                //format, depth, NULL, ~0, NULL);
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
        }
        virtual void update( const lvRect & a_rc, bool full )
        {
            lvRect rc(a_rc);
            CRLog::debug("update screen, bpp=%d width=%d, height=%d, rect={%d, %d, %d, %d} full=%d", (int)im->bpp,im->width,im->height, (int)rc.left, (int)rc.top, (int)rc.right, (int)rc.bottom, full?1:0 );
            if ( _forceNextUpdate && !_forceUpdateRect.isEmpty() ) {
                if ( _mode==NormalMode ) {
                    if ( rc.isEmpty() ) {
                        // no changes since last update!
                        CRLog::debug("CRXCBScreen::update() : Showing already prepared image");
                        forceDraw();
                        _forceNextUpdate = false;
                        _forceUpdateRect.clear();
                        return;
                    }
                    // some changes detected
                    // expand update rect up to forced update rect
                    if ( rc.right < _forceUpdateRect.right )
                        rc.right = _forceUpdateRect.right;
                    if ( rc.left > _forceUpdateRect.left )
                        rc.left = _forceUpdateRect.left;
                    if ( rc.bottom < _forceUpdateRect.bottom )
                        rc.bottom = _forceUpdateRect.bottom;
                    if ( rc.top > _forceUpdateRect.top )
                        rc.top = _forceUpdateRect.top;
                    _forceNextUpdate = false;
                    _forceUpdateRect.clear();
                    CRLog::debug("CRXCBScreen::update() : prepared image is invalidated for rect(%d, %d, %d, %d)",
                                 rc.left, rc.top, rc.right, rc.bottom);
                }
            }
            if ( rc.isEmpty() )
                return;

#if 0
            // simulate slow e-ink update
            sleep(1);
#endif

            //if (
            int i;
            i = xcb_image_shm_get (connection, window,
                    im, shminfo,
                    0, 0,
                    XCB_ALL_PLANES);
            if (!i) {
                printf("cannot get shm image\n");
                return;
            }
            //CRLog::debug("update screen, bpp=%d", (int)im->bpp);

            if ( _mode==PrepareMode ) {
                CRLog::debug("CRXCBScreen::update() : disabling autodraw for prepare mode");
                disableAutoDraw();
            }
            // pal
            //static lUInt32 pal[4] = {0x000000, 0x555555, 0xaaaaaa, 0xffffff };
            //static lUInt8 pal8[4] = {0x00, 0x55, 0xaa, 0xff };
            switch ( im->bpp ) {
            case 32:
                {
                    int bpp = _front->GetBitsPerPixel();
                    if ( bpp==2 ) {
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
                    } else {
                        unsigned int * pal;
                        int shift;
                        int mask;
                        if ( bpp==4 ) {
                            pal = pal16_;
                            shift = 4;
                            mask = 15;
                        } else if ( bpp==8 ) {
                            pal = pal256_;
                            shift = 0;
                            mask = 255;
                        } else {
                            pal = pal8_;
                            shift = 5;
                            mask = 7;
                        }
                        for ( int y = rc.top; y<rc.bottom; y++ ) {
                            lUInt8 * src = _front->GetScanLine( y );
                            lUInt32 * dst = (lUInt32 *)(im->data + im->stride * y);
                            //printf("line %d : %08X -> %08X   ", y, src, dst);
                            for ( int x = 0; x< _width; x++ ) {
                                lUInt8 data = src[x];
                                int pixel = (data>>shift) & mask;
                                lUInt32 color = pal[ pixel ]; // to check valgrind finding
                                dst[x] = color;
                            }
                        }
                    }
                }
                break;
            case 8:
                {
                    if ( _front->GetBitsPerPixel()==2 ) {
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
                    } else {

                        for ( int y = rc.top; y<rc.bottom; y++ ) {
                            lUInt8 * src = _front->GetScanLine( y );
                            lUInt8 * dst = (lUInt8 *)(im->data + im->stride * y);
                            //printf("line %d : %08X -> %08X   ", y, src, dst);
                            for ( int x = 0; x< _width; x++ ) {
                                // TODO: support 8bits as well
                                int pixel = ((src[x]) >> 4) & 15;
                                lUInt8 color = (lUInt8)pal16_[ pixel ]; // to check valgrind finding
                                dst[x] = color;
                            }
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
                CRLog::error("unsupported bpp %d", im->bpp);
                break;
            }
            //pContext.image = im;
            //CRLog::debug("updated\n");

            //view()->paint();

            xcb_image_shm_put (connection, window, gc,
                    im, shminfo,
                    rc.left, rc.top, rc.left, rc.top, rc.width(), rc.height(), 0);
            xcb_flush(connection);
            if ( _mode == PrepareMode ) {
                enableAutoDraw();
                if ( _forceUpdateRect.left > rc.left || _forceUpdateRect.right==0)
                    _forceUpdateRect.left = rc.left;
                if ( _forceUpdateRect.right < rc.right )
                    _forceUpdateRect.right = rc.right;
                if ( _forceUpdateRect.top > rc.top || _forceUpdateRect.bottom==0 )
                    _forceUpdateRect.top = rc.top;
                if ( _forceUpdateRect.bottom < rc.bottom )
                    _forceUpdateRect.bottom = rc.bottom;
                CRLog::debug("CRXCBScreen::update() : prepared rect (%d, %d, %d, %d)",
                             _forceUpdateRect.left, _forceUpdateRect.top,
                             _forceUpdateRect.right, _forceUpdateRect.bottom );
            }
        }
    public:
        /// creates compatible canvas of specified size
        virtual LVDrawBuf * createCanvas( int dx, int dy )
        {
            LVDrawBuf * buf = new LVGrayDrawBuf( dx, dy, bufDepth );
            buf->Clear(0xFFFFFF);
            return buf;
        }
        virtual ~CRXCBScreen()
        {
            if ( im )
                xcb_image_destroy( im );
            if ( connection )
                xcb_disconnect( connection );
        }
        CRXCBScreen( int width, int height )
        :  CRGUIScreenBase( 0, 0, true )
        , _turbo(false), _mode(NormalMode)
        , _forceNextUpdate(false)
        , _forceUpdateRect(0,0,0,0)
        {
            // autodetect turbo mode - check whether enable auto draw is passed
            _turboModeSupported = enableAutoDraw();
            CRLog::info("CRXCBScreen : turbo mode is %s", (_turboModeSupported?"enabled":"disabled"));
            pal_[0] = 0x000000;
            pal_[1] = 0x555555;
            pal_[2] = 0xaaaaaa;
            pal_[3] = 0xffffff;
            for ( int k=0; k<16; k++ )
                pal16_[k] = k | (k<<4) | (k<<8) | (k<<12) | (k<<16) | (k<<20);
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
            im = NULL;

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
                XCB_EVENT_MASK_POINTER_MOTION |
                XCB_EVENT_MASK_VISIBILITY_CHANGE |
                XCB_EVENT_MASK_PROPERTY_CHANGE |
                XCB_EVENT_MASK_STRUCTURE_NOTIFY;

            depth = xcb_aux_get_depth (connection, screen);
            CRLog::trace("depth = %d, root depth = %d\n",depth, screen->root_depth);
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

            for ( int kk=0; kk<16; kk++ ) {
                int cc = kk | (kk<<4);
                rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, cc<<8, cc<<8, cc<<8), NULL);
                pal16_[kk] = rep->pixel;
                free(rep);
            }
            for ( int kk=0; kk<8; kk++ ) {
                int cc = (kk<<5) | (kk<<2) | (kk>>1);
                rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, cc<<8, cc<<8, cc<<8), NULL);
                pal8_[kk] = rep->pixel;
                free(rep);
            }
            for ( int kk=0; kk<256; kk++ ) {
                int cc = kk;
                rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, cc<<8, cc<<8, cc<<8), NULL);
                pal256_[kk] = rep->pixel;
                free(rep);
            }

            pal = pal_;

            _width = width;
            _height = height;
            createImage();

            // substitute GRAY_BACKBUFFER_BITS with depth of screen
            int d = im->depth;
            if ( d==32 || d==24 )
                d = GRAY_BACKBUFFER_BITS; // 8 colors for desktop simulation
            if ( d<2 )
                d = 2;
            if ( d>4 )
                d = 4;
            bufDepth = d;
            CRLog::info( "Device depth=%d, will use rendering depth=%d", (int)im->depth, d );

            _canvas = LVRef<LVDrawBuf>( createCanvas( _width, _height ) );
            _front = LVRef<LVDrawBuf>( createCanvas( _width, _height ) );


            xcb_flush(connection);
            printf("Created screen %d x %d, depth = %d\n", _width, _height, depth );
        }
};

static struct atom {
    const char *name;
    xcb_atom_t atom;
} atoms[] = {
    {"UTF8_STRING", 0},
    {"ACTIVE_DOC_AUTHOR", 0},
    {"ACTIVE_DOC_TITLE", 0},
    {"ACTIVE_DOC_FILENAME", 0},
    {"ACTIVE_DOC_FILEPATH", 0},
    {"ACTIVE_DOC_SERIES", 0},
    {"ACTIVE_DOC_SERIES_NUMBER", 0},
    {"ACTIVE_DOC_TYPE", 0},
    {"ACTIVE_DOC_SIZE", 0},
    {"ACTIVE_DOC_CURRENT_POSITION", 0},
    {"ACTIVE_DOC_CURRENT_PAGE", 0},
    {"ACTIVE_DOC_PAGES_COUNT", 0},
    {"ACTIVE_DOC_WINDOW_ID", 0},
    {"ACTIVE_DOC_COVER_IMAGE", 0}
};

cr_rotate_angle_t readXCBScreenRotationAngle()
{
    xcb_randr_get_screen_info_cookie_t cookie;
    xcb_randr_get_screen_info_reply_t *reply;
    xcb_randr_rotation_t rotation;

    cookie = xcb_randr_get_screen_info(connection, screen->root);
    reply = xcb_randr_get_screen_info_reply(connection, cookie, NULL);

    rotation = (xcb_randr_rotation_t)reply->rotation;
    free(reply);

    switch(rotation) {
        case XCB_RANDR_ROTATION_ROTATE_0:
            return CR_ROTATE_ANGLE_0;
        case XCB_RANDR_ROTATION_ROTATE_90:
            return CR_ROTATE_ANGLE_90;
        case XCB_RANDR_ROTATION_ROTATE_180:
            return CR_ROTATE_ANGLE_180;
        case XCB_RANDR_ROTATION_ROTATE_270:
            return CR_ROTATE_ANGLE_270;
        default:
            return CR_ROTATE_ANGLE_0;
    }
}

class CRXCBWindowManager : public CRGUIWindowManager
{
protected:
    xcb_atom_t wm_protocols_atom, wm_delete_window_atom;
    xcb_key_symbols_t * keysyms;
    xcb_visibility_t visibility;
    //xcb_connection_t * _connection;

    void init_properties()
    {
        if(!connection)
            return;

        xcb_intern_atom_cookie_t cookie;
        xcb_intern_atom_reply_t *reply = NULL;

        int atoms_cnt = sizeof(atoms) / sizeof(struct atom);
        for(int i = 0; i < atoms_cnt; i++) {
            cookie = xcb_intern_atom_unchecked(connection, 0, strlen(atoms[i].name), atoms[i].name);
            reply = xcb_intern_atom_reply(connection, cookie, NULL);
            atoms[i].atom = reply->atom;
            free(reply);
        }
    }

    void delete_properties()
    {
        xcb_delete_property(connection,
                screen->root,
                atoms[12].atom);

        xcb_flush(connection);
    }

    
public:
    
    void updateProperties()
    {
        if (!(window && connection))
            return;
        if ( !main_win || !main_win->getDocView() )
            return;

        CRLog::info("CRXCBWindowManager::updateProperties() -- updating properties on file load completion");
    #define set_prop_str(__i__, __prop__) \
        xcb_change_property(connection, \
                XCB_PROP_MODE_REPLACE, \
                window, \
                atoms[(__i__)].atom, \
                atoms[0].atom, \
                8, \
                strlen((__prop__)), \
                (__prop__));

    #define set_prop_int(__i__, __prop__) \
        { \
        int i = (__prop__); \
        xcb_change_property(connection, \
                XCB_PROP_MODE_REPLACE, \
                window, \
                atoms[(__i__)].atom, \
                XCB_ATOM_INTEGER, \
                32, \
                1, \
                (unsigned char*)&i); \
        }

        LVDocView* doc_view = main_win->getDocView();
        CRPropRef props = doc_view->getDocProps();
        lString16 series = props->getStringDef(DOC_PROP_SERIES_NUMBER);
        int seriesNumber = series.empty() ? 0 : series.atoi();
        LVArray<lChar8> cover_image_file;
        {
            LVStreamRef coverStream = doc_view->getCoverPageImageStream();
            if ( !coverStream.isNull() ) {
                //LVStreamRef out = LVOpenFileStream(COVER_FILE_NAME, LVOM_WRITE);
                int size = coverStream->GetSize();
                if ( size>0 && size<1000000 ) {
                    cover_image_file.addSpace(size);
                    lvsize_t bytesRead = 0;
                    coverStream->Read( cover_image_file.get(), size, &bytesRead );
                    if ( bytesRead!=size ) {
                        CRLog::error("Error while reading coverpage image");
                        cover_image_file.clear();
                    }
                }
            }
        }
        set_prop_str(1, UnicodeToUtf8(props->getStringDef(DOC_PROP_AUTHORS)).c_str());
        set_prop_str(2, UnicodeToUtf8(props->getStringDef(DOC_PROP_TITLE)).c_str());
        set_prop_str(3, UnicodeToUtf8(props->getStringDef(DOC_PROP_FILE_NAME)).c_str());
        set_prop_str(4, UnicodeToUtf8(props->getStringDef(DOC_PROP_FILE_PATH)).c_str());
        set_prop_str(5, UnicodeToUtf8(props->getStringDef(DOC_PROP_SERIES_NAME)).c_str() );
        set_prop_int(6, seriesNumber);
        set_prop_int(9, doc_view->getPosPercent()/100 ); //f->bookTextView().positionIndicator()->textPosition()
        set_prop_int(10, doc_view->getCurPage() );
        set_prop_int(11, doc_view->getPageCount() );

        xcb_change_property(connection,
                XCB_PROP_MODE_REPLACE,
                screen->root,
                atoms[12].atom,
                XCB_ATOM_WINDOW,
                sizeof(xcb_window_t) * 8,
                1,
                (unsigned char*)&window);

        xcb_change_property(connection,
                XCB_PROP_MODE_REPLACE,
                window,
                XCB_ATOM_WM_NAME,
                atoms[0].atom,
                8,
                strlen("CoolReader3"),
                "CoolReader3");

        xcb_change_property(connection,
                XCB_PROP_MODE_REPLACE,
                window,
                XCB_ATOM_WM_CLASS,
                XCB_ATOM_STRING,
                8,
                strlen("cr3") * 2 + 2,
                "cr3\0cr3");

        xcb_change_property(connection,
                XCB_PROP_MODE_REPLACE,
                window,
                atoms[13].atom,
                XCB_ATOM_STRING,
                8,
                cover_image_file.length(),
                cover_image_file.get());

        xcb_flush(connection);

        //free(myBookInfo);
    }

    void updatePositionProperty()
    {
        if ( !atoms[9].atom )
            return;
        if ( !main_win || !main_win->getDocView() )
            return;
        LVDocView* doc_view = main_win->getDocView();
        set_prop_int(9, doc_view->getPosPercent()/100 ); //f->bookTextView().positionIndicator()->textPosition()
        set_prop_int(10, doc_view->getCurPage() );
        set_prop_int(11, doc_view->getPageCount() );
    }


    CRXCBWindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL), keysyms(NULL)
    {
        CRXCBScreen * s = new CRXCBScreen( dx, dy );
        _screen = s;
        init_properties();
        //_connection = s->getXcbConnection();
        _ownScreen = true;
        cr_rotate_angle_t angle = readXCBScreenRotationAngle();
        if ( angle!=0 )
        CRLog::info("Setting rotation angle: %d", (int)angle );
        setScreenOrientation( angle );
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

    bool hasValidConnection()
    {
        return ( xcb_get_setup(connection) != NULL );
    }

    virtual bool getBatteryStatus( int & percent, bool & charging );

    /// idle actions
    virtual void idle();
    // runs event loop
    virtual int runEventLoop();
    /// forward events from system queue to application queue
    virtual void forwardSystemEvents( bool waitForEvent );
};

class XCBDocViewWin : public V3DocViewWin 
{
    public:
        /// file loading is finished successfully - drawCoveTo() may be called there
        virtual void OnLoadFileEnd()
        {
            V3DocViewWin::OnLoadFileEnd();
            ((CRXCBWindowManager*)_wm)->updateProperties();
        }
        
        XCBDocViewWin( CRGUIWindowManager * wm, lString16 dataDir )
        : V3DocViewWin( wm, dataDir )
        {
        }
        virtual ~XCBDocViewWin()
        {
        }
};

bool CRXCBWindowManager::getBatteryStatus( int & percent, bool & charging )
{

    battery_info_t info;
    eoi_get_battery_info(&info);
    charging = false;
    percent = 0;
    if ( info.status==DISCHARGING || info.status==LOW_CHARGE ) {
        charging = false;
        percent = info.charge;
        return true;
    } else {
#if 1
        //debug
        charging = false;
        percent = 100;
        return true;
#else
        // implementation
        charging = true;
        percent = 100;
        return true;
#endif
    }
#if 0
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

#endif
}

void sigint_handler(int)
{
    exit(1);
}

void sigusr1_handler(int)
{
    if (!(window && connection))
        return;

    //if (!in_main_loop)
    //    ecore_main_loop_quit();

    // raise window
    uint32_t value_list = XCB_STACK_MODE_ABOVE;
    xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_STACK_MODE, &value_list);
    xcb_flush(connection);

    int fifo = open(CR3_FIFO, O_RDONLY);
    if (!fifo)
        return;

    char buf[NAME_MAX];
    char *p = buf;
    int ret;
    int len = NAME_MAX - 1;
    while(len > 0 && (ret = read(fifo, p, len)) > 0) {
        len -= ret;
        p += ret;
    }
    *p = '\0';
    close(fifo);

    lString16 filename( Utf8ToUnicode(lString8(buf)) );
    if (filename.empty())
        return;
    
    // TODO: open document
/*
    FBReader *f = (FBReader*)myapplication;
    if(!f->myModel->fileName().compare(filename))
        return;

    BookDescriptionPtr description;
    f->createDescription(filename, description);
    if (!description.isNull()) {
        cover_image_file = "";

        f->openBook(description);
        f->refreshWindow();
        set_properties();
    }
*/
}

void sigalrm_handler(int)
{
}

/// forward events from system queue to application queue
void CRXCBWindowManager::forwardSystemEvents( bool waitForEvent )
{
    if ( _stopFlag )
        waitForEvent = false;
    xcb_generic_event_t *event = NULL;
    for (;;) {
        if ( waitForEvent )
            event = xcb_wait_for_event (connection);
        else
            event = xcb_poll_for_event (connection);
        waitForEvent = false;
        if ( !event )
            break;
        if(xcb_connection_has_error(connection)) {
            CRLog::error("Connection to server closed\n");
            break;
        }

        switch (event->response_type & ~0x80) {
        case XCB_EXPOSE:
            // draw buffer
            {
                if ( visibility != XCB_VISIBILITY_FULLY_OBSCURED ) {
                    printf("EXPOSE\n");
                    postEvent( new CRGUIUpdateEvent(true) );
                }
            }
            break;
        case XCB_VISIBILITY_NOTIFY:
            {
                xcb_visibility_notify_event_t *v = (xcb_visibility_notify_event_t *)event;
                visibility = (xcb_visibility_t)v->state;

                break;
            }
        case XCB_CONFIGURE_NOTIFY:
            {
                //if(count_if(efifo.begin(), efifo.end(), isConfigure) > 0)
                //    break;

                xcb_configure_notify_event_t *conf = (xcb_configure_notify_event_t *)event;
                if (_screen->getWidth() != conf->width || _screen->getHeight() != conf->height) {
                    cr_rotate_angle_t angle = readXCBScreenRotationAngle();
                    CRLog::info("Setting new window size: %d x %d, angle: %d", conf->width, conf->height, (int)angle );
                    postEvent( new CRGUIResizeEvent(conf->width, conf->height, angle) );
                }

                break;
            }
        case XCB_CLIENT_MESSAGE:
            {
                xcb_client_message_event_t *msg = (xcb_client_message_event_t *)event;
                if((msg->type == wm_protocols_atom) &&
                        (msg->format == 32) &&
                        (msg->data.data32[0] == (uint32_t)wm_delete_window_atom)) {
                    _stopFlag = true;
                }
            }
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
                postEvent( new CRGUIKeyDownEvent( sym, state ) );
                //printf("page number = %d\n", main_win->getDocView()->getCurPage());
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

    }
}

/// called when message queue is empty and application is going to wait for event
void CRXCBWindowManager::idle()
{
    if ( !_stopFlag && getWindowCount() )
        updatePositionProperty();
    if ( !_stopFlag && getWindowCount()==1 && (main_win->getLastNavigationDirection()==1 || main_win->getLastNavigationDirection()==-1)) {
        CRLog::debug("Last command is page down: preparing next page for fast navigation");
        main_win->prepareNextPageImage( main_win->getLastNavigationDirection() );
        main_win->unsetLastNavigationDirection();
    }
}

// runs event loop
int CRXCBWindowManager::runEventLoop()
{
    struct sigaction act;
    act.sa_handler = sigint_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    act.sa_handler = sigusr1_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);
    

    
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t *reply = NULL;


    cookie = xcb_intern_atom_unchecked(connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    reply = xcb_intern_atom_reply(connection, cookie, NULL);
    wm_delete_window_atom = reply->atom;
    free(reply);

    cookie = xcb_intern_atom_unchecked(connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    reply = xcb_intern_atom_reply(connection, cookie, NULL);
    wm_protocols_atom = reply->atom;
    free(reply);

    static bool alt_pressed = false;

    CRLog::trace("CRXCBWindowManager::runEventLoop()");
    keysyms = xcb_key_symbols_alloc( connection );

    int res = CRGUIWindowManager::runEventLoop();

    delete_properties();
    xcb_key_symbols_free( keysyms );
    return res;
}

int main(int argc, char **argv)
{

    int res = 0;

    int pid;
    struct stat pid_stat;
    FILE *pidfile;

    int done;

    do {
        done = 1;

        if(stat(PIDFILE, &pid_stat) == -1)
            break;

        pidfile = fopen(PIDFILE, "r");
        if(!pidfile)
            break;

        fscanf(pidfile, "%d", &pid);
        fclose(pidfile);

        if(!pid)
            break;

        if(pid <= 0 || pid == getpid() || kill(pid, 0))
            break;

        struct sigaction act;
        act.sa_handler = sigalrm_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        sigaction(SIGALRM, &act, NULL);

        if(mkfifo(CR3_FIFO, 0666) && errno != EEXIST)
            break;

        kill(pid, SIGUSR1);

        alarm(1);
        int fifo = open(CR3_FIFO, O_WRONLY);
        if(fifo < 0) {
            if(errno == EINTR) {
                done = 0;
                continue;
            }

            unlink(CR3_FIFO);
            break;
        }

        const char *p;
        if(argc > 1)
            p = argv[1];
        else
            p = "";
        int len = strlen(p);
        int ret;
        while(len) {
            ret = write(fifo, p, len);
            if(ret == -1 && errno != EINTR)
                break;

            len -= ret;
            p += ret;
        }

        close(fifo);
        unlink(CR3_FIFO);
        exit(0);
    } while (!done);
    
    pidfile = fopen(PIDFILE, "w");
    if(pidfile) {
        fprintf(pidfile, "%d", getpid());
        fclose(pidfile);
    }
    
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
#ifdef __i386__
        CRXCBWindowManager winman( 600, 680 );
#else
        CRXCBWindowManager winman( 600, 800 );

#endif
    if ( !ldomDocCache::init( lString16("/media/sd/.cr3/cache"), 0x100000 * 64 ))
        ldomDocCache::init( lString16("/tmp/.cr3/cache"), 0x100000 * 64 ); /*64Mb*/
    if ( !winman.hasValidConnection() ) {
        CRLog::error("connection has an error! exiting.");
    } else {

        lString16 home = Utf8ToUnicode(lString8(( getenv("HOME") ) ));
        lString16 homecrengine = home + "/.crengine/";

        lString8 home8 = UnicodeToUtf8( homecrengine );
        const char * keymap_locations [] = {
            "/etc/cr3",
            "/usr/share/cr3",
            home8.c_str(),
            "/media/sd/crengine/",
            NULL,
        };
        loadKeymaps( winman, keymap_locations );

        if ( !winman.loadSkin(homecrengine + "skin") )
            if ( !winman.loadSkin(  lString16("/media/sd/crengine/skin") ) )
                winman.loadSkin( lString16("/usr/share/cr3/skins/default") );
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
        main_win = new XCBDocViewWin( &winman, lString16(CRSKIN) );
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
            hist = lString16(dirs[i]) + "cr3hist.bmk";
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

//#endif
#endif
