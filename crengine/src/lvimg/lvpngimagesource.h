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

#ifndef __LVPNGIMAGESOURCE_H_INCLUDED__
#define __LVPNGIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_LIBPNG==1)

#include "lvnodeimagesource.h"

class LVPngImageSource : public LVNodeImageSource
{
protected:
public:
    LVPngImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVPngImageSource();
    virtual void   Compact();
    virtual bool   Decode( LVImageDecoderCallback * callback );
    static bool CheckPattern( const lUInt8 * buf, int len );
};

#endif  // (USE_LIBPNG==1)

#endif  // __LVPNGIMAGESOURCE_H_INCLUDED__
