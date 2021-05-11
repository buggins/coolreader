/*******************************************************

   CoolReader Engine

   lvtcrstream.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvtcrstream.h"

#include <stdlib.h>
#include <string.h>

void LVTCRStream::TCRCode::set(const char *s, int sz)
{
    if ( sz>0 ) {
        str = (char *)malloc( sz + 1 );
        memcpy( str, s, sz );
        str[sz] = 0;
        len = sz;
    }
}

LVTCRStream::TCRCode::~TCRCode()
{
    if ( str )
        free( str );
}




bool LVTCRStream::decodePart(unsigned index)
{
    if ( _partIndex==index )
        return true;
    lvsize_t bytesRead;
    int bytesToRead = TCR_READ_BUF_SIZE;
    if ( (index+1)*TCR_READ_BUF_SIZE > _packedSize )
        bytesToRead = TCR_READ_BUF_SIZE - ((index+1)*TCR_READ_BUF_SIZE - _packedSize);
    if ( bytesToRead<=0 || bytesToRead>TCR_READ_BUF_SIZE )
        return false;
    if ( _stream->SetPos(_packedStart + index * TCR_READ_BUF_SIZE)==(lvpos_t)(~0) )
        return false;
    if ( _stream->Read( _readbuf, bytesToRead, &bytesRead )!=LVERR_OK )
        return false;
    if ( bytesToRead!=(int)bytesRead )
        return false;
    //TODO
    if ( !_decoded ) {
        _decodedSize = TCR_READ_BUF_SIZE * 2;
        _decoded = (lUInt8 *)malloc( _decodedSize );
    }
    _decodedLen = 0;
    for ( unsigned i=0; i<bytesRead; i++ ) {
        TCRCode * item = &_codes[_readbuf[i]];
        for ( int j=0; j<item->len; j++ )
            _decoded[_decodedLen++] = item->str[j];
        if ( _decodedLen >= _decodedSize - 256 ) {
            _decodedSize += TCR_READ_BUF_SIZE / 2;
            _decoded = cr_realloc( _decoded, _decodedSize );
        }
    }
    _decodedStart = _index[index];
    _partIndex = index;
    return true;
}

LVTCRStream::~LVTCRStream()
{
    if ( _index )
        free(_index);
}

bool LVTCRStream::init()
{
    lUInt8 sz;
    char buf[256];
    lvsize_t bytesRead;
    for ( int i=0; i<256; i++ ) {
        if ( _stream->Read( &sz, 1, &bytesRead )!=LVERR_OK || bytesRead!=1 )
            return false;
        if ( sz==0 && i!=0 )
            return false; // only first entry may be 0
        if ( sz && (_stream->Read( buf, sz, &bytesRead )!=LVERR_OK || bytesRead!=sz) )
            return false;
        _codes[i].set( buf, sz );
    }
    _packedStart = _stream->GetPos();
    if ( _packedStart==(lvpos_t)(~0) )
        return false;
    _packedSize = _stream->GetSize() - _packedStart;
    if ( _packedSize<10 || _packedSize>0x8000000 )
        return false;
    _indexSize = (_packedSize + TCR_READ_BUF_SIZE - 1) / TCR_READ_BUF_SIZE;
    _index = (lUInt32*)malloc( sizeof(lUInt32) * (_indexSize + 1) );
    lvpos_t pos = 0;
    lvsize_t size = 0;
    for (;;) {
        bytesRead = 0;
        int res = _stream->Read( _readbuf, TCR_READ_BUF_SIZE, &bytesRead );
        if ( res!=LVERR_OK && res!=LVERR_EOF )
            return false;
        if ( bytesRead>0 ) {
            for ( unsigned i=0; i<bytesRead; i++ ) {
                int sz = _codes[_readbuf[i]].len;
                if ( (pos & (TCR_READ_BUF_SIZE-1)) == 0 ) {
                    // add pos index
                    int index = pos / TCR_READ_BUF_SIZE;
                    _index[index] = size;
                }
                size += sz;
                pos ++;
            }
        }
        if ( res==LVERR_EOF || bytesRead==0 ) {
            if ( _packedStart + pos != _stream->GetSize() )
                return false;
            break;
        }
    }
    _index[ _indexSize ] = size;
    _unpSize = size;
    return decodePart( 0 );
}

LVStreamRef LVTCRStream::create(LVStreamRef stream, int mode)
{
    LVStreamRef res;
    if ( stream.isNull() || mode != LVOM_READ )
        return res;
    static const char * signature = "!!8-Bit!!";
    char buf[9];
    if ( stream->SetPos(0)!=0 )
        return res;
    lvsize_t bytesRead = 0;
    if ( stream->Read(buf, 9, &bytesRead)!=LVERR_OK
         || bytesRead!=9 )
        return res;
    if (memcmp(signature, buf, 9) != 0)
        return res;
    LVTCRStream * decoder = new LVTCRStream( stream );
    if ( !decoder->init() ) {
        delete decoder;
        return res;
    }
    return LVStreamRef ( decoder );
}

lverror_t LVTCRStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos)
{
    lvpos_t npos = 0;
    lvpos_t currpos = _pos;
    switch (origin) {
        case LVSEEK_SET:
            npos = offset;
            break;
        case LVSEEK_CUR:
            npos = currpos + offset;
            break;
        case LVSEEK_END:
            npos = _unpSize + offset;
            break;
    }
    if (npos >= _unpSize)
        return LVERR_FAIL;
    _pos = npos;
    if ( _pos < _decodedStart || _pos >= _decodedStart + _decodedLen ) {
        // load another part
        int a = 0;
        int b = _indexSize;
        int c;
        for (;;) {
            c = (a + b) / 2;
            if ( a >= b-1 )
                break;
            if ( _index[c] > _pos )
                b = c;
            else if ( _index[c+1] <= _pos )
                a = c + 1;
            else
                break;
        }
        if ( _index[c]>_pos || _index[c+1]<=_pos )
            return LVERR_FAIL; // wrong algorithm?
        if ( !decodePart( c ) )
            return LVERR_FAIL;
    }
    if (pNewPos)
    {
        *pNewPos =  _pos;
    }
    return LVERR_OK;
}

lverror_t LVTCRStream::Read(void *buf, lvsize_t count, lvsize_t *nBytesRead)
{
    // TODO
    lvsize_t bytesRead = 0;
    lUInt8 * dst = (lUInt8*) buf;
    while (count) {
        int bytesLeft = _decodedLen - (int)(_pos - _decodedStart);
        if ( bytesLeft<=0 || bytesLeft>_decodedLen ) {
            SetPos(_pos);
            bytesLeft = _decodedLen - (int)(_pos - _decodedStart);
            if ( bytesLeft==0 && _pos==_decodedStart+_decodedLen) {
                if (nBytesRead)
                    *nBytesRead = bytesRead;
                return bytesRead ? LVERR_OK : LVERR_EOF;
            }
            if ( bytesLeft<=0 || bytesLeft>_decodedLen ) {
                if (nBytesRead)
                    *nBytesRead = bytesRead;
                return LVERR_FAIL;
            }
        }
        lUInt8 * src = _decoded + (_pos - _decodedStart);
        unsigned n = count;
        if ( n > (unsigned)bytesLeft )
            n = bytesLeft;
        for (unsigned i=0; i<n; i++) {
            *dst++ = *src++;
        }
        count -= n;
        // bytesLeft -= n;
        bytesRead += n;
        _pos += n;
    }
    if (nBytesRead)
        *nBytesRead = bytesRead;
    return LVERR_OK;
}
