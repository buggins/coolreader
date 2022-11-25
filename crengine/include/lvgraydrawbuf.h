/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2009,2012,2013 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2015,2016 Yifei(Frank) ZHU <fredyifei@gmail.com>        *
 *   Copyright (C) 2021 NiLuJe <ninuje@gmail.com>                          *
 *   Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#ifndef __LVGRAYDRAWBUF_H_INCLUDED__
#define __LVGRAYDRAWBUF_H_INCLUDED__

#include "lvbasedrawbuf.h"

/**
 * Gray bitmap buffer, partial support for 1-bit buffer
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
    /// returns scanline pointer
    virtual lUInt8 * GetScanLine( int y ) const;
    /// fills buffer with specified color
    virtual void Clear( lUInt32 color );
    /// get pixel value
    virtual lUInt32 GetPixel( int x, int y ) const;
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
    /// blend font bitmap using specified palette
    virtual void BlendBitmap( int x, int y, const lUInt8 * bitmap, FontBmpPixelFormat bitmap_fmt, int width, int height, int bmp_pitch, lUInt32 * palette );
    /// constructor
    LVGrayDrawBuf(int dx, int dy, int bpp=2, void * auxdata = NULL );
    /// destructor
    virtual ~LVGrayDrawBuf();
    /// convert to 1-bit bitmap
    void ConvertToBitmap(bool flgDither);
    virtual void DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0,int length1,int length2,int direction=0);
};

#endif  // __LVGRAYDRAWBUF_H_INCLUDED__
