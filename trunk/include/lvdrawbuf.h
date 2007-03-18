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

#if !defined(__SYMBIAN32__) && defined(_WIN32)
#include <windows.h>
#elif __SYMBIAN32__
#include <e32base.h>
#include <w32std.h>
#endif

#include "lvtypes.h"
#include "lvimg.h"

class LVFont;

/// Abstract drawing buffer
class LVDrawBuf
{
public:
    /// gets clip rect
    virtual void GetClipRect( lvRect * clipRect ) = 0;
    /// sets clip rect
    virtual void SetClipRect( lvRect * clipRect ) = 0;
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
    /// fills rectangle with specified color
    virtual void FillRect( int x0, int y0, int x1, int y1, lUInt32 color ) = 0;
    /// sets new size
    virtual void Resize( int dx, int dy ) = 0;
    /// draws bitmap (1 byte per pixel) using specified palette
    virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette ) = 0;
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height ) = 0;
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette ) = 0;
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette ) = 0;
#elif __SYMBIAN32__
    virtual void DrawTo(CWindowGc &dc, int x, int y, int options, lUInt32 * palette ) = 0;
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
    /// virtual destructor
    virtual ~LVDrawBuf() { }
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
public:
    /// gets clip rect
    virtual void GetClipRect( lvRect * clipRect ) { *clipRect = _clip; }
    /// sets clip rect
    virtual void SetClipRect( lvRect * clipRect );
    /// get buffer width, pixels
    virtual int  GetWidth();
    /// get buffer height, pixels
    virtual int  GetHeight();
    /// get row size (bytes)
    virtual int  GetRowSize() { return _rowsize; }
    /// draws text string
    /*
    virtual void DrawTextString( int x, int y, LVFont * pfont,
                       const lChar16 * text, int len, 
                       lChar16 def_char, 
                       lUInt32 * palette, bool addHyphen=false );
    */
    /// draws formatted text
    //virtual void DrawFormattedText( formatted_text_fragment_t * text, int x, int y );
    
    LVBaseDrawBuf() : _dx(0), _dy(0), _rowsize(0), _data(NULL) { }
    virtual ~LVBaseDrawBuf() { }
};

/// 2-bit gray bitmap buffer, partial support for 1-bit buffer
class LVGrayDrawBuf : public LVBaseDrawBuf
{
private:
    int _bpp;
public:
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette ) { }
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette );
#elif __SYMBIAN32__
    virtual void DrawTo(CWindowGc &dc, int x, int y, int options, lUInt32 * palette );    
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
    /// sets new size
    virtual void Resize( int dx, int dy );
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height );
    /// draws bitmap (1 byte per pixel) using specified palette
    virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette );
    /// constructor
    LVGrayDrawBuf(int dx, int dy, int bpp=2);
    /// destructor
    virtual ~LVGrayDrawBuf();
    /// convert to 1-bit bitmap
    void ConvertToBitmap(bool flgDither);
};

/// 32-bit RGB buffer
class LVColorDrawBuf : public LVBaseDrawBuf
{
private:
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    HDC _drawdc;
    HBITMAP _drawbmp;
#elif __SYMBIAN32__
    CGraphicsContext *_drawdc;
    CFbsBitmap *_drawbmp;
    CFbsBitmapDevice* bitmapDevice;
#endif

public:
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette );
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    /// draws buffer content to another buffer doing color conversion if necessary
    virtual void DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette );
#elif defined(__SYMBIAN32__)
    virtual void DrawTo(CWindowGc &dc, int x, int y, int options, lUInt32 * palette );    
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
    /// sets new size
    virtual void Resize( int dx, int dy );
    /// draws image
    virtual void Draw( LVImageSourceRef img, int x, int y, int width, int height );
    /// draws bitmap (1 byte per pixel) using specified palette
    virtual void Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette );
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y );
    /// constructor
    LVColorDrawBuf(int dx, int dy);
    /// destructor
    virtual ~LVColorDrawBuf();
    /// convert to 1-bit bitmap
    void ConvertToBitmap(bool flgDither);
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    /// returns device context for bitmap buffer
    HDC GetDC() { return _drawdc; }
#elif defined(__SYMBIAN32__)
    CGraphicsContext* GetDC() { return _drawdc; }
    CFbsBitmap* GetBitmap() {return _drawbmp;}
#endif
};


#endif

