/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2010,2011 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __LVBLOCKWRITESTREAM_H_INCLUDED__
#define __LVBLOCKWRITESTREAM_H_INCLUDED__

#include "lvnamedstream.h"

class LVBlockWriteStream : public LVNamedStream
{
    LVStreamRef _baseStream;
    int _blockSize;
    int _blockCount;
    lvpos_t _pos;
    lvpos_t _size;

    struct Block
    {
        lvpos_t block_start;
        lvpos_t block_end;
        lvpos_t modified_start;
        lvpos_t modified_end;
        lUInt8 * buf;
        int size;
        Block * next;

        Block( lvpos_t start, lvpos_t end, int block_size );
        ~Block();
        void save( const lUInt8 * ptr, lvpos_t pos, lvsize_t len );
        bool containsPos( lvpos_t pos )
        {
            return pos>=block_start && pos<block_start+size;
        }
    };

    // list of blocks
    Block * _firstBlock;
    int _count;

    /// set write bytes limit to call flush(true) automatically after writing of each sz bytes
    void setAutoSyncSize(lvsize_t sz);

    /// fills block with data existing in file
    lverror_t readBlock( Block * block );

    lverror_t writeBlock( Block * block );

    Block * newBlock( lvpos_t start, int len );

    /// find block, move to top if found
    Block * findBlock( lvpos_t pos );

    // try read block-aligned fragment from cache
    bool readFromCache( void * buf, lvpos_t pos, lvsize_t count );

    // write block-aligned fragment to cache
    lverror_t writeToCache( const void * buf, lvpos_t pos, lvsize_t count );
public:
    virtual lverror_t Flush( bool sync );
    /// flushes unsaved data from buffers to file, with optional flush of OS buffers
    virtual lverror_t Flush( bool sync, CRTimerUtil & timeout );

    virtual ~LVBlockWriteStream();

    virtual const lChar32 * GetName()
            { return _baseStream->GetName(); }
    virtual lvopen_mode_t GetMode()
            { return _baseStream->GetMode(); }

    LVBlockWriteStream( LVStreamRef baseStream, int blockSize, int blockCount );

    virtual lvpos_t GetSize()
    {
        return _size;
    }

    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos );

    virtual lverror_t Tell( lvpos_t * pPos );
    //virtual lverror_t   SetPos(lvpos_t p)
    virtual lvpos_t   SetPos(lvpos_t p);
    virtual lvpos_t   GetPos()
    {
        return _pos;
    }
    virtual lverror_t SetSize( lvsize_t size );

    void dumpBlocks( const char * context);

    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead );

    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten );
    virtual bool Eof()
    {
        return _pos >= _size;
    }
};

#endif  // __LVBLOCKWRITESTREAM_H_INCLUDED__
