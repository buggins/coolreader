/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2008,2011 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef __LVRARARC_H_INCLUDED__
#define __LVRARARC_H_INCLUDED__

#include "crsetup.h"

#if (USE_UNRAR==1)

#include "lvarccontainerbase.h"

class LVRarArc : public LVArcContainerBase
{
public:
    LVRarArc( LVStreamRef stream ) : LVArcContainerBase(stream)
    {
    }
    virtual ~LVRarArc()
    {
    }

    virtual LVStreamRef OpenStream( const lChar32 * fname, lvopen_mode_t mode );
    virtual int ReadContents();

    static LVArcContainerBase * OpenArchieve( LVStreamRef stream );
};

#endif  // (USE_UNRAR==1)

#endif  // __LVRARARC_H_INCLUDED__
