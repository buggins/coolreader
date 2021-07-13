/*******************************************************

   CoolReader Engine

   lvcachedstream.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvcachedstream.h"

#include <stdio.h>

LVCachedStream::BufItem *LVCachedStream::addNewItem(int start)
{
    //
    int index = (start >> CACHE_BUF_BLOCK_SHIFT);
    BufItem * item = new BufItem();
    if (!m_head)
    {
        m_head = m_tail = item;
    }
    else
    {
        item->next = m_head;
        m_head->prev = item;
        m_head = item;
    }
    item->start = start;
    int sz = CACHE_BUF_BLOCK_SIZE;
    if ( start + sz > (int)m_size )
        sz = (int)(m_size - start);
    item->size = sz;
    m_buf[ index ] = item;
    m_bufLen++;
    assert( !(m_head && !m_tail) );
    return item;
}

void LVCachedStream::moveToTop(int index)
{
    BufItem * item = m_buf[index];
    if ( !item || m_head == item )
        return;
    if ( m_tail == item )
        m_tail = item->prev;
    if ( item->next )
        item->next->prev = item->prev;
    if ( item->prev )
        item->prev->next = item->next;
    m_head->prev = item;
    item->next = m_head;
    item->prev = NULL;
    m_head = item;
    assert( !(m_head && !m_tail) );
}

LVCachedStream::BufItem *LVCachedStream::reuseItem(int start)
{
    //
    int rem_index = m_tail->start >> CACHE_BUF_BLOCK_SHIFT;
    if (m_tail->prev)
        m_tail->prev->next = NULL;
    m_tail = m_tail->prev;
    BufItem * item = m_buf[rem_index];
    m_buf[ rem_index ] = NULL;
    int index = (start >> CACHE_BUF_BLOCK_SHIFT);
    m_buf[ index ] = item;
    item->start = start;
    int sz = CACHE_BUF_BLOCK_SIZE;
    if ( start + sz > (int)m_size )
        sz = (int)(m_size - start);
    item->size = sz;
    item->next = m_head;
    item->prev = NULL;
    m_head->prev = item;
    m_head = item;
    assert( !(m_head && !m_tail) );
    return item;
}

bool LVCachedStream::fillItem(LVCachedStream::BufItem *item)
{
    //if ( m_stream->SetPos( item->start )==(lvpos_t)(~0) )
    if ( m_stream->SetPos( item->start )!=(lvpos_t)item->start )
        return false;
    //int streamSize=m_stream->GetSize(); int bytesLeft = m_stream->GetSize() - m_stream->GetPos();
    lvsize_t bytesRead = 0;
    if ( m_stream->Read( item->buf, item->size, &bytesRead )!=LVERR_OK || bytesRead!=item->size )
        return false;
    return true;
}

bool LVCachedStream::fillFragment(int startIndex, int count)
{
    if (count<=0 || startIndex<0 || startIndex+count>m_bufItems)
    {
        return false;
    }
    int firstne = -1;
    int lastne = -1;
    int i;
    for ( i=startIndex; i<startIndex+count; i++)
    {
        if ( m_buf[i] )
        {
            moveToTop( i );
        }
        else
        {
            if (firstne == -1)
                firstne = i;
            lastne = i;
        }
    }
    if ( firstne<0 )
        return true;
    for ( i=firstne; i<=lastne; i++)
    {
        if ( !m_buf[i] )
        {
            BufItem * item = addOrReuseItem( i << CACHE_BUF_BLOCK_SHIFT );
            if ( !fillItem ( item ) )
                return false;
        }
        else
        {
            moveToTop( i );
        }
    }
    return true;
}

LVCachedStream::LVCachedStream(LVStreamRef stream, int bufSize) : m_stream(stream), m_pos(0),
    m_head(NULL), m_tail(NULL), m_bufLen(0)
{
    m_size = m_stream->GetSize();
    m_bufItems = (int)((m_size + CACHE_BUF_BLOCK_SIZE - 1) >> CACHE_BUF_BLOCK_SHIFT);
    if (!m_bufItems)
        m_bufItems = 1;
    m_bufSize = (bufSize + CACHE_BUF_BLOCK_SIZE - 1) >> CACHE_BUF_BLOCK_SHIFT;
    if (m_bufSize<3)
        m_bufSize = 3;
    m_buf = new BufItem* [m_bufItems]();
    SetName( stream->GetName() );
}

LVCachedStream::~LVCachedStream()
{
    if (m_buf)
    {
        for (int i=0; i<m_bufItems; i++)
            if (m_buf[i])
                delete m_buf[i];
        delete[] m_buf;
    }
}

lverror_t LVCachedStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *newPos)
{
    lvpos_t npos = 0;
    lvpos_t currpos = m_pos;
    switch (origin) {
        case LVSEEK_SET:
            npos = offset;
            break;
        case LVSEEK_CUR:
            npos = currpos + offset;
            break;
        case LVSEEK_END:
            npos = m_size + offset;
            break;
    }
    if (npos > m_size)
        return LVERR_FAIL;
    m_pos = npos;
    if (newPos)
    {
        *newPos =  m_pos;
    }
    return LVERR_OK;
}

lverror_t LVCachedStream::Read(void *buf, lvsize_t size, lvsize_t *pBytesRead)
{
    if ( m_pos + size > m_size )
        size = m_size - m_pos;
    if ( size <= 0 ) {
        if ( pBytesRead )
            *pBytesRead = 0;
        return LVERR_FAIL;
    }
    int startIndex = (int)(m_pos >> CACHE_BUF_BLOCK_SHIFT);
    int endIndex = (int)((m_pos + size - 1) >> CACHE_BUF_BLOCK_SHIFT);
    int count = endIndex - startIndex + 1;
    int extraItems = (m_bufSize - count); // max move backward
    if (extraItems<0)
        extraItems = 0;
    char * flags = new char[ count ]();
    
    //if ( m_stream
    int start = (int)m_pos;
    lUInt8 * dst = (lUInt8 *) buf;
    int dstsz = (int)size;
    int i;
    int istart = start & (CACHE_BUF_BLOCK_SIZE - 1);
    for ( i=startIndex; i<=endIndex; i++ )
    {
        BufItem * item = m_buf[i];
        if (item)
        {
            int isz = item->size - istart;
            if (isz > dstsz)
                isz = dstsz;
            memcpy( dst, item->buf + istart, isz );
            flags[i - startIndex] = 1;
        }
        dstsz -= CACHE_BUF_BLOCK_SIZE - istart;
        dst += CACHE_BUF_BLOCK_SIZE - istart;
        istart = 0;
    }
    
    dst = (lUInt8 *) buf;
    
    bool flgFirstNE = true;
    istart = start & (CACHE_BUF_BLOCK_SIZE - 1);
    dstsz = (int)size;
    for ( i=startIndex; i<=endIndex; i++ )
    {
        if (!flags[ i - startIndex])
        {
            if ( !m_buf[i] )
            {
                int fillStart = i;
                if ( flgFirstNE )
                {
                    fillStart -= extraItems;
                }
                if (fillStart<0)
                    fillStart = 0;
                int fillEnd = fillStart + m_bufSize - 1;
                if (fillEnd>endIndex)
                    fillEnd = endIndex;
                bool res = fillFragment( fillStart, fillEnd - fillStart + 1 );
                if ( !res )
                {
                    fprintf( stderr, "cannot fill fragment %d .. %d\n", fillStart, fillEnd );
                    exit(-1);
                }
                flgFirstNE = false;
            }
            BufItem * item = m_buf[i];
            int isz = item->size - istart;
            if (isz > dstsz)
                isz = dstsz;
            memcpy( dst, item->buf + istart, isz );
        }
        dst += CACHE_BUF_BLOCK_SIZE - istart;
        dstsz -= CACHE_BUF_BLOCK_SIZE - istart;
        istart = 0;
    }
    delete[] flags;
    
    lvsize_t bytesRead = size;
    if ( m_pos + size > m_size )
        bytesRead = m_size - m_pos;
    m_pos += bytesRead;
    if (pBytesRead)
        *pBytesRead = bytesRead;
    return LVERR_OK;
}
