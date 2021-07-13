/** \file lvcolordrawbuf.h
    \brief Color drawing buffer

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVCOLORDRAWBUF_H_INCLUDED__
#define __LVCOLORDRAWBUF_H_INCLUDED__

#include "lvbasedrawbuf.h"

/// 16/32-bit RGB buffer
class LVColorDrawBuf : public LVBaseDrawBuf
{
private:
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
    HDC _drawdc;
    HBITMAP _drawbmp;
#endif
    int _bpp;
    bool _ownData;
public:
    /// rotates buffer contents by specified angle
    virtual void Rotate( cr_rotate_angle_t angle );
    /// returns white pixel value
    virtual lUInt32 GetWhiteColor() const;
    /// returns black pixel value
    virtual lUInt32 GetBlackColor() const;
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette );
    // draws buffer on top of another buffer to implement background
    virtual void DrawOnTop( LVDrawBuf * buf, int x, int y);
    /// draws rescaled buffer content to another buffer doing color conversion if necessary
    virtual void DrawRescaled(LVDrawBuf * src, int x, int y, int dx, int dy, int options);
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette );
#endif
    /// invert image
    virtual void  Invert();
    /// get buffer bits per pixel
    virtual int  GetBitsPerPixel() const;
    /// fills buffer with specified color
    virtual void Clear( lUInt32 color );
    /// get pixel value
    virtual lUInt32 GetPixel( int x, int y ) const;
    /// fills rectangle with specified color
    virtual void FillRect( int x0, int y0, int x1, int y1, lUInt32 color );
    /// fills rectangle with pattern
    virtual void FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern );
    /// inverts specified rectangle
    virtual void InvertRect( int x0, int y0, int x1, int y1 );
    /// sets new size
    virtual void Resize( int dx, int dy );
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither );
    /// blend font bitmap using specified palette
    virtual void BlendBitmap( int x, int y, const lUInt8 * bitmap, FontBmpPixelFormat bitmap_fmt, int width, int height, int bmp_pitch, lUInt32 * palette );
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y ) const;

    /// create own draw buffer
    LVColorDrawBuf(int dx, int dy, int bpp=32);
    /// creates wrapper around external RGBA buffer
    LVColorDrawBuf(int dx, int dy, lUInt8 * externalBuffer, int bpp=32 );
    /// destructor
    virtual ~LVColorDrawBuf();
    /// convert to 1-bit bitmap
    void ConvertToBitmap(bool flgDither);
    /// draw line
    virtual void DrawLine(int x0,int y0,int x1,int y1,lUInt32 color0 ,int length1=1,int length2=0,int direction=0);
#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
    /// returns device context for bitmap buffer
    HDC GetDC() { return _drawdc; }
#endif
};

#endif  // __LVCOLORDRAWBUF_H_INCLUDED__
