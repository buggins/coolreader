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
 * \file s32utils.h
 * \brief misc symbian utility functions
 */

#ifndef __S32_UTILS_H_INCLUDED__
#define __S32_UTILS_H_INCLUDED__

#ifdef __SYMBIAN32__

#include "lvfnt.h"
#include "lvdrawbuf.h"

#include <e32base.h>
#include <w32std.h>

/// draw gray bitmap buffer to Windows device context
void DrawBuf2DC(CWindowGc &dc, int x, int y, LVDrawBuf * buf, unsigned long * palette, int scale=1 );


#endif

#endif

