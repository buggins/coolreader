/** \file lvdrawbuf.h
    \brief Drawing buffer, gray bitmap buffer

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVDRAWBUF_H_INCLUDED__
#define __LVDRAWBUF_H_INCLUDED__

#include "crsetup.h"

#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
extern "C" {
#include <windows.h>
}
#elif __SYMBIAN32__
#include <e32base.h>
#include <w32std.h>
#endif

#include "lvtypes.h"
#include "lvimg.h"

enum cr_rotate_angle_t {
    CR_ROTATE_ANGLE_0 = 0,
    CR_ROTATE_ANGLE_90,
    CR_ROTATE_ANGLE_180,
    CR_ROTATE_ANGLE_270
};

class LVFont;
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
    void RoundRect( int x0, int y0, int x1, int y1, int borderWidth, int radius, lUInt32 color, int cornerFlags=0x0F  );
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
    /// draws text string
    /*
    virtual void DrawTextString( int x, int y, LVFont * pfont,
                       const lChar16 * text, int len, 
                       lChar16 def_char, lUInt32 * palette, bool addHyphen=false ) = 0;
    */                       
                      
/*
    /// draws formatted text
    virtual void DrawFormattedText( formatted_text_fragment_t * text, int x, int y ) = 0;
*/
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y ) = 0;


    virtual int getAlpha() { return 0; }
    virtual void setAlpha(int alpha) { CR_UNUSED(alpha); }
    virtual lUInt32 applyAlpha(lUInt32 cl) { return cl; }

    /// virtual destructor
    virtual ~LVDrawBuf() { }
    virtual GLDrawBuf * asGLDrawBuf() { return NULL; }
};

/// LVDrawBufferBase
class LVBaseDrawBuf : public LVDrawBuf
{
protected:
    int _dx;
    int _dy;
    int _rowsize;
    lvRect _clip;
    unsigned char * _data;
    lUInt32 _backgroundColor;
    lUInt32 _textColor;
    bool _hidePartialGlyphs;
public:
    virtual void setHidePartialGlyphs( bool hide ) { _hidePartialGlyphs = hide; }
    /// returns current background color
    virtual lUInt32 GetBackgroundColor() { return _backgroundColor; }
    /// sets current background color
    virtual void SetBackgroundColor( lUInt32 cl ) { _backgroundColor=cl; }
    /// returns current text color
    virtual lUInt32 GetTextColor() { return _textColor; }
    /// sets current text color
    virtual void SetTextColor( lUInt32 cl ) { _textColor = cl; }
    /// gets clip rect
    virtual void GetClipRect( lvRect * clipRect ) { *clipRect = _clip; }
    /// sets clip rect
    virtual void SetClipRect( const lvRect * clipRect );
    /// get average pixel value for area (coordinates are fixed floating points *16)
    virtual lUInt32 GetAvgColor(lvRect & rc16);
    /// get linearly interpolated pixel value (coordinates are fixed floating points *16)
    virtual lUInt32 GetInterpolatedColor(int x16, int y16);
    /// get buffer width, pixels
    virtual int  GetWidth();
    /// get buffer height, pixels
    virtual int  GetHeight();
    /// get row size (bytes)
    virtual int  GetRowSize() { return _rowsize; }
    virtual void DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0,int length1,int length2,int direction)=0;
    /// draws text string
    /*
    virtual void DrawTextString( int x, int y, LVFont * pfont,
                       const lChar16 * text, int len, 
                       lChar16 def_char, 
                       lUInt32 * palette, bool addHyphen=false );
    */
    /// draws formatted text
    //virtual void DrawFormattedText( formatted_text_fragment_t * text, int x, int y );
    
    LVBaseDrawBuf() : _dx(0), _dy(0), _rowsize(0), _data(NULL), _hidePartialGlyphs(true) { }
    virtual ~LVBaseDrawBuf() { }
};

/// use to simplify saving draw buffer state
class LVDrawStateSaver
{
    LVDrawBuf & _buf;
    lUInt32 _textColor;
    lUInt32 _backgroundColor;
    int _alpha;
    lvRect _clipRect;
	LVDrawStateSaver & operator = (LVDrawStateSaver &) {
		// no assignment
        return *this;
	}
public:
    /// save settings
    LVDrawStateSaver( LVDrawBuf & buf )
    : _buf( buf )
    , _textColor( buf.GetTextColor() )
    , _backgroundColor( buf.GetBackgroundColor() )
    , _alpha(buf.getAlpha())
    {
        _buf.GetClipRect( &_clipRect );
    }
    void restore()
    {
        _buf.SetTextColor( _textColor );
        _buf.SetBackgroundColor( _backgroundColor );
        _buf.setAlpha(_alpha);
        _buf.SetClipRect( &_clipRect );
    }
    /// restore settings on destroy
    ~LVDrawStateSaver()
    {
        restore();
    }
};

#define SAVE_DRAW_STATE( buf ) LVDrawStateSaver drawBufSaver( buf )

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

/**
 * 2-bit gray bitmap buffer, partial support for 1-bit buffer
 * Supported pixel formats for LVGrayDrawBuf :
 *    1 bpp, 8 pixels per byte packed
 *    2 bpp, 4 pixels per byte packed
 *    3 bpp, 1 pixel per byte, higher 3 bits are significant
 *    4 bpp, 1 pixel per byte, higher 4 bits are significant
 *    8 bpp, 1 pixel per byte, all 8 bits are significant
 *
 */
class LVGrayDrawBuf : public LVBaseDrawBuf
{
private:
    int _bpp;
    bool _ownData;
public:
    /// rotates buffer contents by specified angle
    virtual void Rotate( cr_rotate_angle_t angle );
    /// returns white pixel value
    virtual lUInt32 GetWhiteColor();
    /// returns black pixel value
    virtual lUInt32 GetBlackColor();
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
    virtual int  GetBitsPerPixel();
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y );
    /// fills buffer with specified color
    virtual void Clear( lUInt32 color );
    /// get pixel value
    virtual lUInt32 GetPixel( int x, int y );
    /// fills rectangle with specified color
    virtual void FillRect( int x0, int y0, int x1, int y1, lUInt32 color );
    /// inverts image in specified rectangle
    virtual void InvertRect( int x0, int y0, int x1, int y1 );
    /// fills rectangle with pattern
    virtual void FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern );
    /// sets new size
    virtual void Resize( int dx, int dy );
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither );
    /// draws bitmap (1 byte per pixel) using specified palette
    virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette );
    /// constructor
    LVGrayDrawBuf(int dx, int dy, int bpp=2, void * auxdata = NULL );
    /// destructor
    virtual ~LVGrayDrawBuf();
    /// convert to 1-bit bitmap
    void ConvertToBitmap(bool flgDither);
    virtual void DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0,int length1,int length2,int direction=0);
};

inline lUInt32 RevRGB( lUInt32 cl )
{
    return ((cl>>16)&255) | ((cl<<16)&0xFF0000) | (cl&0x00FF00);
}

inline lUInt32 rgb565to888(lUInt32 cl ) {
    return ((cl & 0xF800)<<8) | ((cl & 0x07E0)<<5) | ((cl & 0x001F)<<3);
}

inline lUInt16 rgb888to565(lUInt32 cl ) {
    return (lUInt16)(((cl>>8)& 0xF800) | ((cl>>5 )& 0x07E0) | ((cl>>3 )& 0x001F));
}

/// 32-bit RGB buffer
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
    virtual lUInt32 GetWhiteColor();
    /// returns black pixel value
    virtual lUInt32 GetBlackColor();
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
    virtual int  GetBitsPerPixel();
    /// fills buffer with specified color
    virtual void Clear( lUInt32 color );
    /// get pixel value
    virtual lUInt32 GetPixel( int x, int y );
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
    /// draws bitmap (1 byte per pixel) using specified palette
    virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette );
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y );

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



#endif

