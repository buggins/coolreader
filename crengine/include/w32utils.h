/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
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
 * \file w32utils.h
 * \brief misc windows utility functions
 */

#ifndef __W32_UTILS_H_INCLUDED__
#define __W32_UTILS_H_INCLUDED__

#if !defined(__SYMBIAN32__) && defined(_WIN32)

#include "lvfnt.h"
#include "lvdrawbuf.h"
#include "lvgraydrawbuf.h"

extern "C" {
#include <windows.h>
}

/// draw gray bitmap buffer to Windows device context
void DrawBuf2DC(HDC dc, int x, int y, LVDrawBuf * buf, COLORREF * palette, int scale=1 );
/// save gray bitmap to .BMP file
void SaveBitmapToFile( const char * fname, LVGrayDrawBuf * bmp );


#endif

#endif
