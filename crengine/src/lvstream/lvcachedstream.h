/** @file lvcachedstream.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVCACHEDSTREAM_H_INCLUDED__
#define __LVCACHEDSTREAM_H_INCLUDED__

#include "lvnamedstream.h"

class LVCachedStream : public LVNamedStream
{
private:

    #define CACHE_BUF_BLOCK_SHIFT 12
    #define CACHE_BUF_BLOCK_SIZE (1<<CACHE_BUF_BLOCK_SHIFT)
    class BufItem
    {
    public:
        lUInt32   start;
        lUInt32   size;
        BufItem * prev;
        BufItem * next;
        lUInt8    buf[CACHE_BUF_BLOCK_SIZE];

        int getIndex() { return start >> CACHE_BUF_BLOCK_SHIFT; }
        BufItem() : prev(NULL), next(NULL) { }
    };

    LVStreamRef m_stream;
    int m_bufSize;
    lvsize_t    m_size;
    lvpos_t     m_pos;
    BufItem * * m_buf;
    BufItem *   m_head;
    BufItem *   m_tail;
    int         m_bufItems;
    int         m_bufLen;

    /// add item to head
    BufItem * addNewItem( int start );
    /// move item to top
    void moveToTop( int index );
    /// reuse existing item from tail of list
    BufItem * reuseItem( int start );
    /// read item content from base stream
    bool fillItem( BufItem * item );
    BufItem * addOrReuseItem( int start )
    {
        //assert( !(m_head && !m_tail) );
        if ( m_bufLen < m_bufSize )
            return addNewItem( start );
        else
            return reuseItem( start );
    }
    /// checks several items, loads if necessary
    bool fillFragment( int startIndex, int count );
public:

    LVCachedStream( LVStreamRef stream, int bufSize );
    virtual ~LVCachedStream();

    /// fastly return already known CRC
    virtual lverror_t getcrc32( lUInt32 & dst )
    {
        return m_stream->getcrc32( dst );
    }

    virtual bool Eof()
    {
        return m_pos >= m_size;
    }
    virtual lvsize_t  GetSize()
    {
        return m_size;
    }

    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos);

    virtual lverror_t Write(const void*, lvsize_t, lvsize_t*)
    {
        return LVERR_NOTIMPL;
    }

    virtual lverror_t Read(void* buf, lvsize_t size, lvsize_t* pBytesRead);

    virtual lverror_t SetSize(lvsize_t)
    {
        return LVERR_NOTIMPL;
    }
};

#endif  // __LVCACHEDSTREAM_H_INCLUDED__
