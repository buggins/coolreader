/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009,2013,2014 Vadim Lopatin <coolreader.org@gmail.com> *
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

#include "lvstream.h"
#include "lvdefstreambuffer.h"

/// Get read buffer - default implementation, with RAM buffer
LVStreamBufferRef LVStream::GetReadBuffer( lvpos_t pos, lvpos_t size )
{
    LVStreamBufferRef res;
    res = LVDefStreamBuffer::create( LVStreamRef(this), pos, size, true );
    return res;
}

/// Get read/write buffer - default implementation, with RAM buffer
LVStreamBufferRef LVStream::GetWriteBuffer( lvpos_t pos, lvpos_t size )
{
    LVStreamBufferRef res;
    res = LVDefStreamBuffer::create( LVStreamRef(this), pos, size, false );
    return res;
}


#define CRC_BUF_SIZE 16384

/// calculate crc32 code for stream, if possible
lverror_t LVStream::getcrc32( lUInt32 & dst )
{
    dst = 0;
    if ( GetMode() == LVOM_READ || GetMode() == LVOM_APPEND ) {
        lvpos_t savepos = GetPos();
        lvsize_t size = GetSize();
        lUInt8 buf[CRC_BUF_SIZE];
        SetPos( 0 );
        lvsize_t bytesRead = 0;
        for ( lvpos_t pos = 0; pos<size; pos+=CRC_BUF_SIZE ) {
            lvsize_t sz = size - pos;
            if ( sz > CRC_BUF_SIZE )
                sz = CRC_BUF_SIZE;
            Read( buf, sz, &bytesRead );
            if ( bytesRead!=sz ) {
                SetPos(savepos);
                return LVERR_FAIL;
            }
            dst = lStr_crc32( dst, buf, sz );
        }
        SetPos( savepos );
        return LVERR_OK;
    } else {
        // not supported
        return LVERR_NOTIMPL;
    }
}
