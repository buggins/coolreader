/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __LVFILEMAPPEDSTREAM_H_INCLUDED__
#define __LVFILEMAPPEDSTREAM_H_INCLUDED__

#include "lvnamedstream.h"
#include "lvstreambuffer.h"

#if defined(_WIN32) && !defined(__SYMBIAN32__)
extern "C" {
#include <windows.h>
}
#endif

//#if USE__FILES==1
#if defined(_LINUX) || defined(_WIN32)

class LVFileMappedStream : public LVNamedStream
{
private:
#if defined(_WIN32)
    HANDLE m_hFile;
    HANDLE m_hMap;
#else
    int m_fd;
#endif
    lUInt8* m_map;
    lvsize_t m_size;
    lvpos_t m_pos;

    /// Read or write buffer for stream region
    class LVBuffer : public LVStreamBuffer
    {
    protected:
        LVStreamRef m_stream;
        lUInt8 * m_buf;
        lvsize_t m_size;
        bool m_readonly;
    public:
        LVBuffer( LVStreamRef stream, lUInt8 * buf, lvsize_t size, bool readonly )
        : m_stream( stream ), m_buf( buf ), m_size( size ), m_readonly( readonly )
        {
        }
        /// get pointer to read-only buffer, returns NULL if unavailable
        virtual const lUInt8 * getReadOnly()
        {
            return m_buf;
        }
        /// get pointer to read-write buffer, returns NULL if unavailable
        virtual lUInt8 * getReadWrite()
        {
            return m_readonly ? NULL : m_buf;
        }
        /// get buffer size
        virtual lvsize_t getSize()
        {
            return m_size;
        }
        /// flush on destroy
        virtual ~LVBuffer() { }
    };
public:
    /// Get read buffer (optimal for )
    virtual LVStreamBufferRef GetReadBuffer( lvpos_t pos, lvpos_t size );
    /// Get read/write buffer (optimal for )
    virtual LVStreamBufferRef GetWriteBuffer( lvpos_t pos, lvpos_t size );
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos );
    /// Tell current file position
    /**
        \param pNewPos points to place to store file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Tell( lvpos_t * pPos );
    virtual lvpos_t SetPos(lvpos_t p);
    /// Get file position
    /**
        \return lvpos_t file position
    */
    virtual lvpos_t   GetPos()
    {
        return m_pos;
    }
    /// Get file size
    /**
        \return lvsize_t file size
    */
    virtual lvsize_t  GetSize()
    {
        return m_size;
    }
    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        *pSize = m_size;
        return LVERR_OK;
    }
    lverror_t error();
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead );
    virtual bool Read( lUInt8 * buf );
    virtual bool Read( lUInt16 * buf );
    virtual bool Read( lUInt32 * buf );
    virtual int ReadByte();
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten );
    virtual bool Eof()
    {
        return (m_pos >= m_size);
    }
    static LVFileMappedStream * CreateFileStream( lString32 fname, lvopen_mode_t mode, int minSize );
    lverror_t Map();
    lverror_t UnMap();
    virtual lverror_t SetSize( lvsize_t size );
    lverror_t OpenFile( lString32 fname, lvopen_mode_t mode, lvsize_t minSize = (lvsize_t)-1 );
    LVFileMappedStream();
    virtual ~LVFileMappedStream();
};
#endif  // #if defined(_LINUX) || defined(_WIN32)

#endif  // __LVFILEMAPPEDSTREAM_H_INCLUDED__
