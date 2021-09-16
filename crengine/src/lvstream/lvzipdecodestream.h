/** @file lvzipdecodestream.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVZIPDECODESTREAM_H_INCLUDED__
#define __LVZIPDECODESTREAM_H_INCLUDED__

#include "crsetup.h"

#if (USE_ZLIB==1)

#include "lvnamedstream.h"

#include <zlib.h>

class LVZipDecodeStream : public LVNamedStream
{
private:
    LVStreamRef m_stream;
    lvsize_t    m_start;
    lvsize_t    m_packsize;
    lvsize_t    m_unpacksize;
    z_stream_s  m_zstream;
    lvpos_t     m_inbytesleft; // bytes left
    lvpos_t     m_outbytesleft;
    bool        m_zInitialized;
    int         m_decodedpos;
    lUInt8 *    m_inbuf;
    lUInt8 *    m_outbuf;
    lUInt32     m_CRC;
    lUInt32     m_originalCRC;
    lUInt32     m_decodedCRC;


    LVZipDecodeStream( LVStreamRef stream, lvsize_t start, lvsize_t packsize, lvsize_t unpacksize, lUInt32 crc );

    ~LVZipDecodeStream();

    /// Get stream open mode
    /** \return lvopen_mode_t open mode */
    virtual lvopen_mode_t GetMode()
    {
        return LVOM_READ;
    }

    void zUninit();

    /// Fill input buffer: returns -1 if fails.
    int fillInBuf();

    bool rewind();
    // returns count of available decoded bytes in buffer
    inline int getAvailBytes()
    {
        return (int)(m_zstream.next_out - m_outbuf - m_decodedpos);
    }
    /// decode next portion of data, returns number of decoded bytes available, -1 if error
    int decodeNext();
    /// skip bytes from out stream
    bool skip( int bytesToSkip );
    /// decode bytes
    int read( lUInt8 * buf, int bytesToRead );
public:

    /// fastly return already known CRC
    virtual lverror_t getcrc32( lUInt32 & dst );

    virtual bool Eof()
    {
        return m_outbytesleft==0; //m_pos >= m_size;
    }
    virtual lvsize_t  GetSize()
    {
        return m_unpacksize;
    }
    virtual lvpos_t GetPos()
    {
        return m_unpacksize - m_outbytesleft;
    }
    virtual lverror_t GetPos( lvpos_t * pos )
    {
        if (pos)
            *pos = m_unpacksize - m_outbytesleft;
        return LVERR_OK;
    }
    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos);
    virtual lverror_t Write(const void*, lvsize_t, lvsize_t*)
    {
        return LVERR_NOTIMPL;
    }
    virtual lverror_t Read(void* buf, lvsize_t count, lvsize_t* bytesRead);
    virtual lverror_t SetSize(lvsize_t)
    {
        return LVERR_NOTIMPL;
    }
    static LVStream * Create( LVStreamRef stream, lvpos_t pos, lString32 name, lvsize_t srcPackSize, lvsize_t srcUnpSize );
};

#endif  // (USE_ZLIB==1)

#endif  // __LVZIPDECODESTREAM_H_INCLUDED__
