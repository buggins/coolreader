/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2017 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __LVSVGIMAGESOURCE_H_INCLUDED__
#define __LVSVGIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_NANOSVG==1)

#include "lvnodeimagesource.h"

class LVSvgImageSource : public LVNodeImageSource
{
protected:
public:
    LVSvgImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVSvgImageSource();
    virtual void   Compact();
    virtual bool   Decode( LVImageDecoderCallback * callback );
    int DecodeFromBuffer(unsigned char *buf, int buf_size, LVImageDecoderCallback * callback);
    static bool CheckPattern( const lUInt8 * buf, int len );
};

#endif  // (USE_NANOSVG==1)

#endif  // __LVSVGIMAGESOURCE_H_INCLUDED__
