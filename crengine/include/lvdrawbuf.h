/** \file lvdrawbuf.h
    \brief Abstract drawing buffer

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVDRAWBUF_H_INCLUDED__
#define __LVDRAWBUF_H_INCLUDED__

#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
extern "C" {
#include <windows.h>
}
#elif __SYMBIAN32__
#include <e32base.h>
#include <w32std.h>
#endif

#include "lvcacheableobject.h"
#include "lvimagesource.h"

enum cr_rotate_angle_t {
    CR_ROTATE_ANGLE_0 = 0,
    CR_ROTATE_ANGLE_90,
    CR_ROTATE_ANGLE_180,
    CR_ROTATE_ANGLE_270
};

enum DrawBufPixelFormat
{
    DRAW_BUF_1_BPP = 1, /// 1 bpp, 8 pixels per byte packed
    DRAW_BUF_2_BPP = 2, /// 2 bpp, 4 pixels per byte packed
    DRAW_BUF_3_BPP = 3, /// 3 bpp, 1 pixel per byte, higher 3 bits are significant
    DRAW_BUF_4_BPP = 4, /// 4 bpp, 1 pixel per byte, higher 4 bits are significant
    DRAW_BUF_8_BPP = 8, /// 8 bpp, 1 pixel per byte, all 8 bits are significant
    DRAW_BUF_16_BPP = 16, /// color 16bit RGB 565
    DRAW_BUF_32_BPP = 32  /// color 32bit RGB 888
};

class GLDrawBuf; // workaround for no-rtti builds

/// Abstract drawing buffer
class LVDrawBuf : public CacheableObject
{
public:
    // GL draw buffer support
    /// GL draw buffer compatibility - requires this call before any drawing
    virtual void beforeDrawing() {}
    /// GL draw buffer compatibility - requires this call after any drawing
    virtual void afterDrawing() {}

    // tiles support
    /// returns true if drawing buffer is tiled
    virtual bool isTiled() { return false; }
    /// returns tile width (or just width if no tiles)
    virtual int tileWidth() { return GetWidth(); }
    /// returns tile height (or just height if no tiles)
    virtual int tileHeight() { return GetHeight(); }
    /// returns tile drawbuf for tiled image, returns this for non tiled draw buffer
    virtual LVDrawBuf * getTile(int x, int y) {
        CR_UNUSED2(x, y);
        return this;
    }
    /// returns number of tiles in row
    virtual int getXtiles() {
        return 1;
    }
    /// returns number of tiles in column
    virtual int getYtiles() {
        return 1;
    }

    /// returns tile rectangle
    virtual void getTileRect(lvRect & rc, int x, int y) {
        CR_UNUSED2(x, y);
        rc.left = rc.top = 0;
        rc.right = GetWidth();
        rc.bottom = GetHeight();
    }

    /// rotates buffer contents by specified angle
    virtual void Rotate( cr_rotate_angle_t angle ) = 0;
    /// returns white pixel value
    virtual lUInt32 GetWhiteColor() = 0;
    /// returns black pixel value
    virtual lUInt32 GetBlackColor() = 0;
    /// returns current background color
    virtual lUInt32 GetBackgroundColor() = 0;
    /// sets current background color
    virtual void SetBackgroundColor( lUInt32 cl ) = 0;
    /// returns current text color
    virtual lUInt32 GetTextColor() = 0;
    /// sets current text color
    virtual void SetTextColor( lUInt32 cl ) = 0;
    /// gets clip rect
    virtual void GetClipRect( lvRect * clipRect ) = 0;
    /// sets clip rect
    virtual void SetClipRect( const lvRect * clipRect ) = 0;
    /// set to true for drawing in Paged mode, false for Scroll mode
    virtual void setHidePartialGlyphs( bool hide ) = 0;
    /// set to true to invert images only (so they get inverted back to normal by nightmode)
    virtual void setInvertImages( bool invert ) = 0;
    /// set to true to enforce dithering (only relevant for 8bpp Gray drawBuf)
    virtual void setDitherImages( bool dither ) = 0;
    /// set to true to switch to a more costly smooth scaler instead of nearest neighbor
    virtual void setSmoothScalingImages( bool smooth ) = 0;
    /// invert image
    virtual void  Invert() = 0;
    /// get buffer width, pixels
    virtual int  GetWidth() = 0;
    /// get buffer height, pixels
    virtual int  GetHeight() = 0;
    /// get buffer bits per pixel
    virtual int  GetBitsPerPixel() = 0;
    /// fills buffer with specified color
    virtual int  GetRowSize() = 0;
    /// fills buffer with specified color
    virtual void Clear( lUInt32 color ) = 0;
    /// get pixel value
    virtual lUInt32 GetPixel( int x, int y ) = 0;
    /// get average pixel value for area (coordinates are fixed floating points *16)
    virtual lUInt32 GetAvgColor(lvRect & rc16) = 0;
    /// get linearly interpolated pixel value (coordinates are fixed floating points *16)
    virtual lUInt32 GetInterpolatedColor(int x16, int y16) = 0;
    /// draw gradient filled rectangle with colors for top-left, top-right, bottom-right, bottom-left
    virtual void GradientRect(int x0, int y0, int x1, int y1, lUInt32 color1, lUInt32 color2, lUInt32 color3, lUInt32 color4) {
        CR_UNUSED8(x0, x1, y0, y1, color1, color2, color3, color4);
    }
    /// fills rectangle with specified color
    virtual void FillRect( int x0, int y0, int x1, int y1, lUInt32 color ) = 0;
    /// draw frame
    inline void DrawFrame(const lvRect & rc, lUInt32 color, int width = 1)
    {
        FillRect( rc.left, rc.top, rc.right, rc.top + width, color );
        FillRect( rc.left, rc.bottom - width, rc.right, rc.bottom, color );
        FillRect( rc.left, rc.top + width, rc.left + width, rc.bottom - width, color );
        FillRect( rc.right - width, rc.top + width, rc.right, rc.bottom - width, color );
    }
    /// fills rectangle with specified color
    inline void FillRect( const lvRect & rc, lUInt32 color )
    {
        FillRect( rc.left, rc.top, rc.right, rc.bottom, color );
    }
    /// draws rectangle with specified color
    inline void Rect( int x0, int y0, int x1, int y1, lUInt32 color )
    {
        FillRect( x0, y0, x1-1, y0+1, color );
        FillRect( x0, y0, x0+1, y1-1, color );
        FillRect( x1-1, y0, x1, y1, color );
        FillRect( x0, y1-1, x1, y1, color );
    }
    /// draws rectangle with specified width and color
    inline void Rect( int x0, int y0, int x1, int y1, int borderWidth, lUInt32 color )
    {
        FillRect( x0, y0, x1-1, y0+borderWidth, color );
        FillRect( x0, y0, x0+borderWidth, y1-1, color );
        FillRect( x1-borderWidth, y0, x1, y1, color );
        FillRect( x0, y1-borderWidth, x1, y1, color );
    }
    /// draws rounded rectangle with specified line width, rounding radius, and color
    void RoundRect( int x0, int y0, int x1, int y1, int borderWidth, int radius, lUInt32 color, int cornerFlags=0x0F )
    {
        FillRect( x0 + ((cornerFlags&1)?radius:0), y0, x1-1-((cornerFlags&2)?radius:0), y0+borderWidth, color );
        FillRect( x0, y0 + ((cornerFlags&1)?radius:0), x0+borderWidth, y1-1-((cornerFlags&4)?radius:0), color );
        FillRect( x1-borderWidth, y0 + ((cornerFlags&2)?radius:0), x1, y1-((cornerFlags&8)?radius:0), color );
        FillRect( x0 + ((cornerFlags&4)?radius:0), y1-borderWidth, x1-((cornerFlags&8)?radius:0), y1, color );
        // TODO: draw rounded corners
    }
    /// draws rectangle with specified color
    inline void Rect( const lvRect & rc, lUInt32 color )
    {
        Rect( rc.left, rc.top, rc.right, rc.bottom, color );
    }
    /// draws rectangle with specified color
    inline void Rect( const lvRect & rc, int borderWidth, lUInt32 color )
    {
        Rect( rc.left, rc.top, rc.right, rc.bottom, borderWidth, color );
    }
    /// fills rectangle with pattern
    virtual void FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern ) = 0;
    /// inverts image in specified rectangle
    virtual void InvertRect(int x0, int y0, int x1, int y1) = 0;
    /// sets new size
    virtual void Resize( int dx, int dy ) = 0;
    /// draws bitmap (1 byte per pixel) using specified palette
    virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette ) = 0;
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither=true ) = 0;
    /// draws part of source image, possible rescaled
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height, int srcx, int srcy, int srcwidth, int srcheight, bool dither=true ) { CR_UNUSED10(img, x, y, width, height, srcx, srcy, srcwidth, srcheight, dither); }
    /// for GL buf only - rotated drawing
    virtual void DrawRotated( LVImageSourceRef img, int x, int y, int width, int height, int rotationAngle) { Draw(img, x, y, width, height); CR_UNUSED(rotationAngle); }
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette ) = 0;
    // draws buffer on top of another buffer to implement background
    virtual void DrawOnTop( LVDrawBuf * buf, int x, int y) = 0;
    /// draws rescaled buffer content to another buffer doing color conversion if necessary
    virtual void DrawRescaled(LVDrawBuf * src, int x, int y, int dx, int dy, int options) = 0;
    /// draws rescaled buffer content to another buffer doing color conversion if necessary
    virtual void DrawFragment(LVDrawBuf * src, int srcx, int srcy, int srcdx, int srcdy, int x, int y, int dx, int dy, int options) {
        CR_UNUSED10(src, srcx, srcy, srcdx, srcdy, x, y, dx, dy, options);
    }
    /// draw lines
    virtual void DrawLine(int x0,int y0,int x1,int y1,lUInt32 color0 ,int length1,int length2,int direction)=0;
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette ) = 0;
#endif
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y ) = 0;


    virtual int getAlpha() { return 0; }
    virtual void setAlpha(int alpha) { CR_UNUSED(alpha); }
    virtual lUInt32 applyAlpha(lUInt32 cl) { return cl; }

    /// virtual destructor
    virtual ~LVDrawBuf() { }
    virtual GLDrawBuf * asGLDrawBuf() { return NULL; }
};

#endif
