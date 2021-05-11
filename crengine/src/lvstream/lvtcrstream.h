/** @file lvtcrstream.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVTCRSTREAM_H_INCLUDED__
#define __LVTCRSTREAM_H_INCLUDED__

#include "lvnamedstream.h"

class LVTCRStream : public LVNamedStream
{
    class TCRCode {
    public:
        int len;
        char * str;
        TCRCode()
            : len(0), str(NULL)
        {
        }
        void set( const char * s, int sz );
        ~TCRCode();
    };
    LVStreamRef _stream;
    TCRCode _codes[256];
    lvpos_t _packedStart;
    lvsize_t _packedSize;
    lvsize_t _unpSize;
    lUInt32 * _index;
    lUInt8 * _decoded;
    int _decodedSize;
    int _decodedLen;
    unsigned _partIndex;
    lvpos_t _decodedStart;
    int _indexSize;
    lvpos_t _pos;
    //int _indexPos;
    #define TCR_READ_BUF_SIZE 4096
    lUInt8 _readbuf[TCR_READ_BUF_SIZE];
    LVTCRStream( LVStreamRef stream )
    : _stream(stream), _index(NULL), _decoded(NULL),
      _decodedSize(0), _decodedLen(0), _partIndex((unsigned)-1), _decodedStart(0), _indexSize(0), _pos(0)
    {
    }

    bool decodePart( unsigned index );
public:
    ~LVTCRStream();
    bool init();
    static LVStreamRef create( LVStreamRef stream, int mode );

    /// Get stream open mode
    /** \return lvopen_mode_t open mode */
    virtual lvopen_mode_t GetMode()
    {
        return LVOM_READ;
    }

    /// Seek (change file pos)
    /**
        \param offset is file offset (bytes) relateve to origin
        \param origin is offset base
        \param pNewPos points to place to store new file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos );

    /// Get file position
    /**
        \return lvpos_t file position
    */
    virtual lvpos_t   GetPos()
    {
        return _pos;
    }

    /// Get file size
    /**
        \return lvsize_t file size
    */
    virtual lvsize_t  GetSize()
    {
        return _unpSize;
    }

    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        *pSize = _unpSize;
        return LVERR_OK;
    }

    /// Set file size
    /**
        \param size is new file size
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t SetSize( lvsize_t )
    {
        return LVERR_FAIL;
    }

    /// Read
    /**
        \param buf is buffer to place bytes read from stream
        \param count is number of bytes to read from stream
        \param nBytesRead is place to store real number of bytes read from stream
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead );

    /// Write
    /**
        \param buf is data to write to stream
        \param count is number of bytes to write
        \param nBytesWritten is place to store real number of bytes written to stream
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Write( const void *, lvsize_t, lvsize_t *)
    {
        return LVERR_FAIL;
    }

    /// Check whether end of file is reached
    /**
        \return true if end of file reached
    */
    virtual bool Eof()
    {
        //TODO
        return false;
    }
};

#endif  // __LVTCRSTREAM_H_INCLUDED__
