/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2011,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2019,2021 NiLuJe <ninuje@gmail.com>                     *
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

/**
 * \file lvbasedrawbuf.h
 * \brief Drawing buffer, common bitmap buffer
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
    virtual lUInt32 GetBackgroundColor() const { return _backgroundColor; }
    /// sets current background color
    virtual void SetBackgroundColor( lUInt32 cl ) { _backgroundColor=cl; }
    /// returns current text color
    virtual lUInt32 GetTextColor() const { return _textColor; }
    /// sets current text color
    virtual void SetTextColor( lUInt32 cl ) { _textColor = cl; }
    /// gets clip rect
    virtual void GetClipRect( lvRect * clipRect ) const { *clipRect = _clip; }
    /// sets clip rect
    virtual void SetClipRect( const lvRect * clipRect );
    /// get average pixel value for area (coordinates are fixed floating points *16)
    virtual lUInt32 GetAvgColor(lvRect & rc16) const;
    /// get linearly interpolated pixel value (coordinates are fixed floating points *16)
    virtual lUInt32 GetInterpolatedColor(int x16, int y16) const;
    /// get buffer width, pixels
    virtual int  GetWidth() const { return _dx; }
    /// get buffer height, pixels
    virtual int  GetHeight() const { return _dy; }
    /// get row size (bytes)
    virtual int  GetRowSize() const { return _rowsize; }
    virtual void DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0,int length1,int length2,int direction)=0;
    /// Get nb of images drawn on buffer
    int getDrawnImagesCount() const { return _drawnImagesCount; }
    /// Get surface of images drawn on buffer
    int getDrawnImagesSurface() const { return _drawnImagesSurface; }

    LVBaseDrawBuf() : _dx(0), _dy(0), _rowsize(0), _data(NULL), _hidePartialGlyphs(true),
                        _invertImages(false), _ditherImages(false), _smoothImages(false),
                        _drawnImagesCount(0), _drawnImagesSurface(0) { }
    virtual ~LVBaseDrawBuf() { }
};

#endif  // __LVBASEDRAWBUF_H_INCLUDED__
