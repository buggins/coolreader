/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __STREAMPROXY_H_INCLUDED__
#define __STREAMPROXY_H_INCLUDED__

#include "lvstream.h"

/// base proxy class for streams: redirects all calls to base stream
class StreamProxy : public LVStream {
protected:
    LVStreamRef _base;
public:
    StreamProxy(LVStreamRef baseStream) : _base(baseStream) { }
    virtual ~StreamProxy() { }

    /// Seek (change file pos)
    /**
        \param offset is file offset (bytes) relateve to origin
        \param origin is offset base
        \param pNewPos points to place to store new file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos ) {
        return _base->Seek(offset, origin, pNewPos);
    }

    /// Tell current file position
    /**
        \param pNewPos points to place to store file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Tell( lvpos_t * pPos ) { return _base->Tell(pPos); }

    /// Set file position
    /**
        \param p is new position
        \return lverror_t status: LVERR_OK if success
    */
    //virtual lverror_t SetPos(lvpos_t p) { return Seek(p, LVSEEK_SET, NULL); }
    virtual lvpos_t   SetPos(lvpos_t p) { return _base->SetPos(p); }

    /// Get file position
    /**
        \return lvpos_t file position
    */
    virtual lvpos_t   GetPos()  { return _base->GetPos();  }

    virtual lvsize_t  GetSize()
    {
        return _base->GetSize();
    }

    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        return _base->GetSize(pSize);
    }

    virtual lverror_t SetSize( lvsize_t size ) { return _base->SetSize(size); }

    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead ) {
        return _base->Read(buf, count, nBytesRead);
    }

    /// Write
    /**
        \param buf is data to write to stream
        \param count is number of bytes to write
        \param nBytesWritten is place to store real number of bytes written to stream
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten ) {
        return _base->Write(buf, count, nBytesWritten);
    }

    virtual bool Eof() {
        return _base->Eof();
    }

};

#endif // __STREAMPROXY_H_INCLUDED__
