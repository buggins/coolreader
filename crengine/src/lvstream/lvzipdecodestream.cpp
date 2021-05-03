/*******************************************************

   CoolReader Engine

   lvzipdecodestream.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvzipdecodestream.h"

#if (USE_ZLIB==1)

#include "lvstreamfragment.h"
#include "ziphdr.h"
#include "crlog.h"

//#define ARC_INBUF_SIZE  4096
//#define ARC_OUTBUF_SIZE 16384
#define ARC_INBUF_SIZE  5000
#define ARC_OUTBUF_SIZE 10000


LVZipDecodeStream::LVZipDecodeStream(LVStreamRef stream, lvsize_t start, lvsize_t packsize, lvsize_t unpacksize, lUInt32 crc)
    : m_stream(stream), m_start(start), m_packsize(packsize), m_unpacksize(unpacksize),
      m_inbytesleft(0), m_outbytesleft(0), m_zInitialized(false), m_decodedpos(0),
      m_inbuf(NULL), m_outbuf(NULL), m_CRC(0), m_originalCRC(crc), m_decodedCRC(0)
{
    m_inbuf = new lUInt8[ARC_INBUF_SIZE];
    m_outbuf = new lUInt8[ARC_OUTBUF_SIZE];
    rewind();
}

LVZipDecodeStream::~LVZipDecodeStream()
{
    zUninit();
    if (m_inbuf)
        delete[] m_inbuf;
    if (m_outbuf)
        delete[] m_outbuf;
}

void LVZipDecodeStream::zUninit()
{
    if (!m_zInitialized)
        return;
    inflateEnd(&m_zstream);
    m_zInitialized = false;
}

int LVZipDecodeStream::fillInBuf()
{
    if (m_zstream.avail_in < ARC_INBUF_SIZE / 4 && m_inbytesleft > 0)
    {
        int inpos = (int)(m_zstream.next_in ? (m_zstream.next_in - m_inbuf) : 0);
        if ( inpos > ARC_INBUF_SIZE/2 )
        {
            // move rest of data to beginning of buffer
            for ( int i=0; i<(int)m_zstream.avail_in; i++)
                m_inbuf[i] = m_inbuf[ i+inpos ];
            m_zstream.next_in = m_inbuf;
            inpos = 0;
        }
        int tailpos = inpos + m_zstream.avail_in;
        int bytes_to_read = ARC_INBUF_SIZE - tailpos;
        if ( bytes_to_read > (int)m_inbytesleft )
            bytes_to_read = (int)m_inbytesleft;
        if (bytes_to_read > 0)
        {
            lvsize_t bytesRead = 0;
            if ( m_stream->Read( m_inbuf + tailpos, bytes_to_read, &bytesRead ) != LVERR_OK )
            {
                // read error
                m_zstream.avail_in = 0;
                return -1;
            }
            m_CRC = lStr_crc32( m_CRC, m_inbuf + tailpos, (int)(bytesRead) );
            m_zstream.avail_in += (int)bytesRead;
            m_inbytesleft -= bytesRead;
        }
        else
        {
            //check CRC
            if ( m_CRC != m_originalCRC ) {
                CRLog::error("ZIP stream '%s': CRC doesn't match", LCSTR(lString32(GetName())) );
                return -1; // CRC error
            }
        }
    }
    return m_zstream.avail_in;
}

bool LVZipDecodeStream::rewind()
{
    zUninit();
    // stream
    m_stream->SetPos( 0 );
    
    m_CRC = 0;
    memset( &m_zstream, 0, sizeof(m_zstream) );
    // inbuf
    m_inbytesleft = m_packsize;
    m_zstream.next_in = m_inbuf;
    m_zstream.avail_in = 0;
    fillInBuf();
    // outbuf
    m_zstream.next_out = m_outbuf;
    m_zstream.avail_out = ARC_OUTBUF_SIZE;
    m_decodedpos = 0;
    m_outbytesleft = m_unpacksize;
    // Z
    if ( inflateInit2( &m_zstream, -15 ) != Z_OK )
    {
        return false;
    }
    m_zInitialized = true;
    return true;
}

int LVZipDecodeStream::decodeNext()
{
    int avail = getAvailBytes();
    if (avail>0)
        return avail;
    // fill in buffer
    int in_bytes = fillInBuf();
    if (in_bytes<0)
        return -1;
    // reserve space for output
    if (m_decodedpos > ARC_OUTBUF_SIZE/2 || (m_zstream.avail_out < ARC_OUTBUF_SIZE / 4 && m_outbytesleft > 0) )
    {
        
        int outpos = (int)(m_zstream.next_out - m_outbuf);
        if ( m_decodedpos > ARC_OUTBUF_SIZE/2 || outpos > ARC_OUTBUF_SIZE*2/4 || m_zstream.avail_out==0 || m_inbytesleft==0 )
        {
            // move rest of data to beginning of buffer
            for ( int i=(int)m_decodedpos; i<outpos; i++)
                m_outbuf[i - m_decodedpos] = m_outbuf[ i ];
            //m_inbuf[i - m_decodedpos] = m_inbuf[ i ];
            m_zstream.next_out -= m_decodedpos;
            outpos -= m_decodedpos;
            m_decodedpos = 0;
            m_zstream.avail_out = ARC_OUTBUF_SIZE - outpos;
        }
    }
    // int decoded = m_zstream.avail_out;
    int res = inflate( &m_zstream, m_inbytesleft > 0 ? Z_NO_FLUSH : Z_FINISH ); //m_inbytesleft | m_zstream.avail_in
    // decoded -= m_zstream.avail_out;
    if (res == Z_STREAM_ERROR)
    {
        return -1;
    }
    if (res == Z_BUF_ERROR)
    {
        //return -1;
        //res = 0; // DEBUG
    }
    avail = getAvailBytes();
    return avail;
}

bool LVZipDecodeStream::skip(int bytesToSkip)
{
    while (bytesToSkip > 0)
    {
        int avail = decodeNext();
        
        if (avail < 0)
        {
            return false; // error
        }
        else if (avail==0)
        {
            return true;
        }
        
        if (avail >= bytesToSkip)
            avail = bytesToSkip;
        
        m_decodedpos += avail;
        m_outbytesleft -= avail;
        bytesToSkip -= avail;
    }
    if (bytesToSkip == 0)
        return true;
    return false;
}

int LVZipDecodeStream::read(lUInt8 *buf, int bytesToRead)
{
    int bytesRead = 0;
    //
    while (bytesToRead > 0)
    {
        int avail = decodeNext();
        
        if (avail < 0)
        {
            return -1; // error
        }
        else if (avail==0)
        {
            avail = decodeNext();
            (void)avail; // never read
            return bytesRead;
        }
        
        int delta = avail;
        if (delta > bytesToRead)
            delta = bytesToRead;
        
        
        // copy data
        lUInt8 * src = m_outbuf + m_decodedpos;
        for (int i=delta; i>0; --i)
            *buf++ = *src++;
        
        m_decodedpos += delta;
        m_outbytesleft -= delta;
        bytesRead += delta;
        bytesToRead -= delta;
    }
    return bytesRead;
}

lverror_t LVZipDecodeStream::getcrc32(lUInt32 &dst)
{
    if (m_originalCRC != 0)
        dst = m_originalCRC;
    else {
        // invalid CRC in zip header, try to recalc CRC32
        if (m_decodedCRC != 0)
            dst = m_decodedCRC;
        else {
            lUInt8* tmp_buff = (lUInt8*)malloc(ARC_OUTBUF_SIZE);
            if (!tmp_buff) {
                dst = 0;
                return LVERR_FAIL;
            }
            lvpos_t curr_pos;
            Seek(0, LVSEEK_CUR, &curr_pos);
            Seek(0, LVSEEK_SET, 0);
            lvsize_t bytesRead = 0;
            while (Read(tmp_buff, ARC_OUTBUF_SIZE, &bytesRead) == LVERR_OK) {
                if (bytesRead > 0)
                    m_decodedCRC = lStr_crc32(m_decodedCRC, tmp_buff, bytesRead);
                else
                    break;
            }
            free(tmp_buff);
            Seek((lvoffset_t)curr_pos, LVSEEK_SET, 0);
            dst = m_decodedCRC;
        }
    }
    return LVERR_OK;
}

lverror_t LVZipDecodeStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *newPos)
{
    lvpos_t npos = 0;
    lvpos_t currpos = GetPos();
    switch (origin) {
        case LVSEEK_SET:
            npos = offset;
            break;
        case LVSEEK_CUR:
            npos = currpos + offset;
            break;
        case LVSEEK_END:
            npos = m_unpacksize + offset;
            break;
    }
    if (npos > m_unpacksize)
        return LVERR_FAIL;
    if ( npos != currpos )
    {
        if (npos < currpos)
        {
            if ( !rewind() || !skip((int)npos) )
                return LVERR_FAIL;
        }
        else
        {
            skip( (int)(npos - currpos) );
        }
    }
    if (newPos)
        *newPos = npos;
    return LVERR_OK;
}

lverror_t LVZipDecodeStream::Read(void *buf, lvsize_t count, lvsize_t *bytesRead)
{
    int readBytes = read( (lUInt8 *)buf, (int)count );
    if ( readBytes<0 )
        return LVERR_FAIL;
    if ( readBytes!=(int)count ) {
        CRLog::trace("ZIP stream: %d bytes read instead of %d", (int)readBytes, (int)count);
    }
    if (bytesRead)
        *bytesRead = (lvsize_t)readBytes;
    //CRLog::trace("%d bytes requested, %d bytes read, %d bytes left", count, readBytes, m_outbytesleft);
    return LVERR_OK;
}

LVStream *LVZipDecodeStream::Create(LVStreamRef stream, lvpos_t pos, lString32 name, lUInt32 srcPackSize, lUInt32 srcUnpSize)
{
    ZipLocalFileHdr hdr;
    unsigned hdr_size = 0x1E; //sizeof(hdr);
    if ( stream->Seek( pos, LVSEEK_SET, NULL )!=LVERR_OK )
        return NULL;
    lvsize_t sz = 0;
    if ( stream->Read( &hdr, hdr_size, &sz)!=LVERR_OK || sz!=hdr_size )
        return NULL;
    hdr.byteOrderConv();
    pos += 0x1e + hdr.getNameLen() + hdr.getAddLen();
    if ( stream->Seek( pos, LVSEEK_SET, NULL )!=LVERR_OK )
        return NULL;
    lUInt32 packSize = hdr.getPackSize();
    lUInt32 unpSize = hdr.getUnpSize();
    if ( packSize==0 && unpSize==0 ) {
        // Can happen when local header does not carry these sizes
        // Use the ones provided that come from zip central directory
        packSize = srcPackSize;
        unpSize = srcUnpSize;
    }
    if ((lvpos_t)(pos + packSize) > (lvpos_t)stream->GetSize())
        return NULL;
    if (hdr.getMethod() == 0)
    {
        // store method, copy as is
        if ( packSize != unpSize )
            return NULL;
        LVStreamFragment * fragment = new LVStreamFragment( stream, pos, packSize);
        fragment->SetName( name.c_str() );
        return fragment;
    }
    else if (hdr.getMethod() == 8)
    {
        // deflate
        LVStreamRef srcStream( new LVStreamFragment( stream, pos, packSize) );
        LVZipDecodeStream * res = new LVZipDecodeStream( srcStream, pos,
                                                         packSize, unpSize, hdr.getCRC() );
        res->SetName( name.c_str() );
        return res;
    }
    else
        return NULL;
}

#endif  // (USE_ZLIB==1)
