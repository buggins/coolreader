/** \file lvbasedrawbuf.h
    \brief Drawing buffer, common bitmap buffer

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVBASEDRAWBUF_H_INCLUDED__
#define __LVBASEDRAWBUF_H_INCLUDED__

#include "lvdrawbuf.h"

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
    bool _invertImages;
    bool _ditherImages;
    bool _smoothImages;
    int _drawnImagesCount;
    int _drawnImagesSurface;
public:
    /// set to true for drawing in Paged mode, false for Scroll mode
    virtual void setHidePartialGlyphs( bool hide ) { _hidePartialGlyphs = hide; }
    /// set to true to invert images only (so they get inverted back to normal by nightmode)
    virtual void setInvertImages( bool invert ) { _invertImages = invert; }
    /// set to true to enforce dithering (only relevant for 8bpp Gray drawBuf)
    virtual void setDitherImages( bool dither ) { _ditherImages = dither; }
    /// set to true to switch to a more costly smooth scaler instead of nearest neighbor
    virtual void setSmoothScalingImages( bool smooth ) { _smoothImages = smooth; }
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
    virtual int  GetWidth() { return _dx; }
    /// get buffer height, pixels
    virtual int  GetHeight() { return _dy; }
    /// get row size (bytes)
    virtual int  GetRowSize() { return _rowsize; }
    virtual void DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0,int length1,int length2,int direction)=0;
    /// Get nb of images drawn on buffer
    int getDrawnImagesCount() { return _drawnImagesCount; }
    /// Get surface of images drawn on buffer
    int getDrawnImagesSurface() { return _drawnImagesSurface; }

    LVBaseDrawBuf() : _dx(0), _dy(0), _rowsize(0), _data(NULL), _hidePartialGlyphs(true),
                        _invertImages(false), _ditherImages(false), _smoothImages(false),
                        _drawnImagesCount(0), _drawnImagesSurface(0) { }
    virtual ~LVBaseDrawBuf() { }
};

#endif  // __LVBASEDRAWBUF_H_INCLUDED__
