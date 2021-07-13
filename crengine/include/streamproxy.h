/** @file streamproxy.h
    @brief base proxy class for streams: redirects all calls to base stream

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*/

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
