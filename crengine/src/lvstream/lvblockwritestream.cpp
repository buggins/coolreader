/*******************************************************

   CoolReader Engine

   lvblockwritestream.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*******************************************************/

#include "lvblockwritestream.h"
#include "crtimerutil.h"
#include "crlog.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define TRACE_BLOCK_WRITE_STREAM 0


LVBlockWriteStream::Block::Block(lvpos_t start, lvpos_t end, int block_size)
    : block_start( start/block_size*block_size ), block_end( end )
    , modified_start((lvpos_t)-1), modified_end((lvpos_t)-1)
    , size( block_size ), next(NULL)
{
    buf = (lUInt8*)calloc(size, sizeof(*buf));
    if ( buf ) {
        //            modified_start = 0;
        //            modified_end = size;
    }
    else {
        CRLog::error("buffer allocation failed");
    }
}

LVBlockWriteStream::Block::~Block()
{
    free(buf);
}

void LVBlockWriteStream::Block::save(const lUInt8 *ptr, lvpos_t pos, lvsize_t len)
{
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("block %x save %x, %x", (int)block_start, (int)pos, (int)len);
#endif
    int offset = (int)(pos - block_start);
    if (offset > size || offset < 0 || (int)len > size || offset + (int)len > size) {
        CRLog::error("Unaligned access to block %x", (int)block_start);
    }
    for (unsigned i = 0; i < len; i++ ) {
        lUInt8 ch1 = buf[offset+i];
        if ( pos+i>block_end || ch1!=ptr[i] ) {
            buf[offset+i] = ptr[i];
            if ( modified_start==(lvpos_t)-1 ) {
                modified_start = pos + i;
                modified_end = modified_start + 1;
            } else {
                if ( modified_start>pos+i )
                    modified_start = pos+i;
                if ( modified_end<pos+i+1)
                    modified_end = pos+i+1;
                if ( block_end<pos+i+1)
                    block_end = pos+i+1;
            }
        }
    }
    
}



void LVBlockWriteStream::setAutoSyncSize(lvsize_t sz) {
    _baseStream->setAutoSyncSize(sz);
    handleAutoSync(0);
}

lverror_t LVBlockWriteStream::readBlock(LVBlockWriteStream::Block *block)
{
    if ( !block->size ) {
        CRLog::error("Invalid block size");
    }
    lvpos_t start = block->block_start;
    lvpos_t end = start + _blockSize;
    lvpos_t ssize = 0;
    lverror_t res = LVERR_OK;
    res = _baseStream->GetSize( &ssize);
    if ( res!=LVERR_OK )
        return res;
    if ( end>ssize )
        end = ssize;
    if ( end<=start )
        return LVERR_OK;
    _baseStream->SetPos( start );
    lvsize_t bytesRead = 0;
    block->block_end = end;
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("block %x filling from stream %x, %x", (int)block->block_start, (int)block->block_start, (int)(block->block_end-block->block_start));
#endif
    res = _baseStream->Read( block->buf, end-start, &bytesRead );
    if ( res!=LVERR_OK )
        CRLog::error("Error while reading block %x from file of size %x", block->block_start, ssize);
    return res;
}

lverror_t LVBlockWriteStream::writeBlock(LVBlockWriteStream::Block *block)
{
    if ( block->modified_start < block->modified_end ) {
#if TRACE_BLOCK_WRITE_STREAM
        CRLog::trace("WRITE BLOCK %x (%x, %x)", (int)block->block_start, (int)block->modified_start, (int)(block->modified_end-block->modified_start));
#endif
        _baseStream->SetPos( block->modified_start );
        if (block->modified_end > _size) {
            block->modified_end = block->block_end;
        }
        lvpos_t bytesWritten = 0;
        lverror_t res = _baseStream->Write( block->buf + (block->modified_start-block->block_start), block->modified_end-block->modified_start, &bytesWritten );
        if ( res==LVERR_OK ) {
            if (_size < block->modified_end)
                _size = block->modified_end;
        }
        block->modified_end = block->modified_start = (lvpos_t)-1;
        return res;
    } else
        return LVERR_OK;
}

LVBlockWriteStream::Block *LVBlockWriteStream::newBlock(lvpos_t start, int len)
{
    Block * b = new Block( start, start+len, _blockSize );
    return b;
}

LVBlockWriteStream::Block *LVBlockWriteStream::findBlock(lvpos_t pos)
{
    for ( Block ** p = &_firstBlock; *p; p=&(*p)->next ) {
        Block * item = *p;
        if ( item->containsPos(pos) ) {
            if ( item!=_firstBlock ) {
#if TRACE_BLOCK_WRITE_STREAM
                dumpBlocks("before reorder");
#endif
                *p = item->next;
                item->next = _firstBlock;
                _firstBlock = item;
#if TRACE_BLOCK_WRITE_STREAM
                dumpBlocks("after reorder");
                CRLog::trace("found block %x (%x, %x)", (int)item->block_start, (int)item->modified_start, (int)(item->modified_end-item->modified_start));
#endif
            }
            return item;
        }
    }
    return NULL;
}

bool LVBlockWriteStream::readFromCache(void *buf, lvpos_t pos, lvsize_t count)
{
    Block * p = findBlock( pos );
    if ( p ) {
#if TRACE_BLOCK_WRITE_STREAM
        CRLog::trace("read from cache block %x (%x, %x)", (int)p->block_start, (int)pos, (int)(count));
#endif
        memcpy( buf, p->buf + (pos-p->block_start), count );
        return true;
    }
    return false;
}

lverror_t LVBlockWriteStream::writeToCache(const void *buf, lvpos_t pos, lvsize_t count)
{
    Block * p = findBlock( pos );
    if ( p ) {
#if TRACE_BLOCK_WRITE_STREAM
        CRLog::trace("saving data to existing block %x (%x, %x)", (int)p->block_start, (int)pos, (int)count);
#endif
        p->save( (const lUInt8 *)buf, pos, count );
        if ( pos + count > _size )
            _size = pos + count;
        return LVERR_OK;
    }
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("Block %x not found in cache", pos);
#endif
    if ( _count>=_blockCount-1 ) {
        // remove last
        for ( Block * p = _firstBlock; p; p=p->next ) {
            if ( p->next && !p->next->next ) {
#if TRACE_BLOCK_WRITE_STREAM
                dumpBlocks("before remove last");
                CRLog::trace("dropping block %x (%x, %x)", (int)p->next->block_start, (int)p->next->modified_start, (int)(p->next->modified_end-p->next->modified_start));
#endif
                writeBlock( p->next );
                delete p->next;
                _count--;
                p->next = NULL;
#if TRACE_BLOCK_WRITE_STREAM
                dumpBlocks("after remove last");
#endif
            }
        }
    }
    p = newBlock( pos, count );
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("creating block %x", (int)p->block_start);
#endif
    if ( readBlock( p )!=LVERR_OK ) {
        delete p;
        return LVERR_FAIL;
    }
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("saving data to new block %x (%x, %x)", (int)p->block_start, (int)pos, (int)count);
#endif
    p->save( (const lUInt8 *)buf, pos, count );
    p->next = _firstBlock;
    _firstBlock = p;
    _count++;
    if ( pos + count > _size ) {
        _size = pos + count;
        p->modified_start = p->block_start;
        p->modified_end = p->block_end;
    }
    return LVERR_OK;
}

lverror_t LVBlockWriteStream::Flush(bool sync) {
    CRTimerUtil infinite;
    return Flush(sync, infinite); // NOLINT: Call to virtual function during destruction
}

lverror_t LVBlockWriteStream::Flush(bool sync, CRTimerUtil &timeout)
{
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("flushing unsaved blocks");
#endif
    lverror_t res = LVERR_OK;
    for ( Block * p = _firstBlock; p; ) {
        Block * tmp = p;
        if ( writeBlock(p)!=LVERR_OK )
            res = LVERR_FAIL;
        p = p->next;
        delete tmp;
        if (!sync && timeout.expired()) {
            //CRLog::trace("LVBlockWriteStream::flush - timeout expired");
            _firstBlock = p;
            return LVERR_OK;
        }
        
    }
    _firstBlock = NULL;
    _baseStream->Flush( sync );
    return res;
}

LVBlockWriteStream::~LVBlockWriteStream()
{
    Flush( true ); // NOLINT: Call to virtual function during destruction
}

LVBlockWriteStream::LVBlockWriteStream(LVStreamRef baseStream, int blockSize, int blockCount)
    : _baseStream( baseStream ), _blockSize( blockSize ), _blockCount( blockCount ), _firstBlock(NULL), _count(0)
{
    _pos = _baseStream->GetPos();
    _size = _baseStream->GetSize();
}

lverror_t LVBlockWriteStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos)
{
    if ( origin==LVSEEK_CUR ) {
        origin = LVSEEK_SET;
        offset = _pos + offset;
    } else if ( origin==LVSEEK_END ) {
        origin = LVSEEK_SET;
        offset = _size + offset;
    }
    
    lvpos_t newpos = 0;
    lverror_t res = _baseStream->Seek(offset, origin, &newpos);
    if ( res==LVERR_OK ) {
        if ( pNewPos )
            *pNewPos = newpos;
        _pos = newpos;
    } else {
        CRLog::error("baseStream->Seek(%d,%x) failed: %d", (int)origin, (int)offset, (int)res);
    }
    return res;
}

lverror_t LVBlockWriteStream::Tell(lvpos_t *pPos)
{
    *pPos = _pos;
    return LVERR_OK;
}

lvpos_t LVBlockWriteStream::SetPos(lvpos_t p)
{
    lvpos_t res = _baseStream->SetPos(p);
    _pos = _baseStream->GetPos();
    //                if ( _size<_pos )
    //                    _size = _pos;
    return res;
}

lverror_t LVBlockWriteStream::SetSize(lvsize_t size)
{
    // TODO:
    lverror_t res = _baseStream->SetSize(size);
    if ( res==LVERR_OK )
        _size = size;
    return res;
}

void LVBlockWriteStream::dumpBlocks(const char *context)
{
    lString8 buf;
    for ( Block * p = _firstBlock; p; p = p->next ) {
        char s[1000];
        snprintf(s, 999, "%x ", (int)p->block_start);
        s[999] = 0;
        buf << s;
    }
    CRLog::trace("BLOCKS (%s): %s   count=%d", context, buf.c_str(), _count);
}

lverror_t LVBlockWriteStream::Read(void *buf, lvsize_t count, lvsize_t *nBytesRead)
{
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("stream::Read(%x, %x)", (int)_pos, (int)count);
    dumpBlocks("before read");
#endif
    // slice by block bounds
    lvsize_t bytesRead = 0;
    lverror_t res = LVERR_OK;
    if ( _pos > _size ) {
        if ( nBytesRead )
            *nBytesRead = bytesRead;
        return LVERR_FAIL;
    }
    if ( _pos + count > _size )
        count = (int)(_size - _pos);
    while ( (int)count>0 && res==LVERR_OK ) {
        lvpos_t blockSpaceLeft = _blockSize - (_pos % _blockSize);
        if ( blockSpaceLeft > count )
            blockSpaceLeft = count;
        lvsize_t blockBytesRead = 0;
        
        // read from Write buffers if possible, otherwise - from base stream
        if ( readFromCache( buf, _pos, blockSpaceLeft ) ) {
            blockBytesRead = blockSpaceLeft;
            res = LVERR_OK;
        } else {
            lvpos_t fsize = _baseStream->GetSize();
            if ( _pos + blockSpaceLeft > fsize && fsize < _size) {
#if TRACE_BLOCK_WRITE_STREAM
                CRLog::trace("stream::Read: inconsistent cache state detected: fsize=%d, _size=%d, force flush...", (int)fsize, (int)_size);
#endif
                // Workaround to exclude fatal error in ldomTextStorageChunk::ensureUnpacked()
                // Write cached data to a file stream if the required read block is larger than the rest of the file.
                // This is a very rare case.
                Flush(true);
            }
#if TRACE_BLOCK_WRITE_STREAM
            CRLog::trace("direct reading from stream (%x, %x)", (int)_pos, (int)blockSpaceLeft);
#endif
            _baseStream->SetPos(_pos);
            res = _baseStream->Read(buf, blockSpaceLeft, &blockBytesRead);
        }
        if ( res!=LVERR_OK )
            break;
        
        count -= blockBytesRead;
        buf = ((char*)buf) + blockBytesRead;
        _pos += blockBytesRead;
        bytesRead += blockBytesRead;
        if ( !blockBytesRead )
            break;
    }
    if ( nBytesRead && res==LVERR_OK )
        *nBytesRead = bytesRead;
    return res;
}

lverror_t LVBlockWriteStream::Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten)
{
#if TRACE_BLOCK_WRITE_STREAM
    CRLog::trace("stream::Write(%x, %x)", (int)_pos, (int)count);
    dumpBlocks("before write");
#endif
    // slice by block bounds
    lvsize_t bytesRead = 0;
    lverror_t res = LVERR_OK;
    //if ( _pos + count > _size )
    //    count = _size - _pos;
    while ( count>0 && res==LVERR_OK ) {
        lvpos_t blockSpaceLeft = _blockSize - (_pos % _blockSize);
        if ( blockSpaceLeft > count )
            blockSpaceLeft = count;
        lvsize_t blockBytesWritten = 0;
        
        // write to Write buffers
        res = writeToCache(buf, _pos, blockSpaceLeft);
        if ( res!=LVERR_OK )
            break;
        
        blockBytesWritten = blockSpaceLeft;
        
        count -= blockBytesWritten;
        buf = ((char*)buf) + blockBytesWritten;
        _pos += blockBytesWritten;
        bytesRead += blockBytesWritten;
        if ( _pos>_size )
            _size = _pos;
        if ( !blockBytesWritten )
            break;
    }
    if ( nBytesWritten && res==LVERR_OK )
        *nBytesWritten = bytesRead;
#if TRACE_BLOCK_WRITE_STREAM
    dumpBlocks("after write");
#endif
    return res;
}
