/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2010-2012 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#include "lvdrawbufimgsource.h"
#include "lvimagedecodercallback.h"
#include "lvcolordrawbuf.h"

// lvdrawbuff private stuff
#include "../lvdrawbuf/lvdrawbuf_utils.h"

LVDrawBufImgSource::LVDrawBufImgSource(LVColorDrawBuf *buf, bool own)
    : _buf(buf)
    , _own(own)
    , _dx( buf->GetWidth() )
    , _dy( buf->GetHeight() )
{
}

LVDrawBufImgSource::~LVDrawBufImgSource()
{
    if ( _own )
        delete _buf;
}

bool LVDrawBufImgSource::Decode(LVImageDecoderCallback *callback)
{
    callback->OnStartDecode( this );
    //bool res = false;
    if ( _buf->GetBitsPerPixel()==32 ) {
        // 32 bpp
        for ( int y=0; y<_dy; y++ ) {
            callback->OnLineDecoded( this, y, (lUInt32 *)_buf->GetScanLine(y) );
        }
    } else {
        // 16 bpp
        lUInt32 * row = new lUInt32[_dx];
        for ( int y=0; y<_dy; y++ ) {
            lUInt16 * src = (lUInt16 *)_buf->GetScanLine(y);
            for ( int x=0; x<_dx; x++ )
                row[x] = rgb565to888(src[x]);
            callback->OnLineDecoded( this, y, row );
        }
        delete[] row;
    }
    callback->OnEndDecode( this, false );
    return true;
}
