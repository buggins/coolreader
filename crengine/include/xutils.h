/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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
 * \file xutils.h
 * \brief misc X Window System utility functions
 */

#ifndef __X_UTILS_H_INCLUDED__
#define __X_UTILS_H_INCLUDED__

#ifdef LINUX

#include "lvfnt.h"
#include "lvdrawbuf.h"

/**
    \brief RGB offscreen image for X Window System
*/
class MyXImage
{
private:
    XImage * _img;
public:
    /// creates image buffer of specified size
    MyXImage( int dx, int dy );
    ~MyXImage();
    /// returns scanline pointer (pixel is 32bit unsigned)
    unsigned * getScanLine( int y );
    /// fills buffer with specified color
    void fill( unsigned pixel );
    /// returns XImage object
    XImage * getXImage()
    {
        return _img;
    }
};



/// draw gray bitmap buffer to X drawable
void DrawBuf2Drawable(Display *display, Drawable d, GC gc, int x, int y, LVDrawBuf * buf, unsigned * palette, int scale=1 );

#endif

#endif
