/** \file lvstreamproxy.h

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

#ifndef __LVSTREAMPROXY_H_INCLUDED__
#define __LVSTREAMPROXY_H_INCLUDED__

#include "lvstream.h"

class LVStreamProxy : public LVStream
{
protected:
    LVStream * m_base_stream;
public:
    virtual const lChar32 * GetName()
            { return m_base_stream->GetName(); }
    virtual lvopen_mode_t GetMode()
            { return m_base_stream->GetMode(); }
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos )
            { return m_base_stream->Seek(offset, origin, pNewPos); }
    virtual lverror_t Tell( lvpos_t * pPos )
            { return m_base_stream->Tell(pPos); }
    //virtual lverror_t   SetPos(lvpos_t p)
    virtual lvpos_t   SetPos(lvpos_t p)
            { return m_base_stream->SetPos(p); }
    virtual lvpos_t   GetPos()
            { return m_base_stream->GetPos(); }
    virtual lverror_t SetSize( lvsize_t size )
            { return m_base_stream->SetSize(size); }
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead )
            { return m_base_stream->Read(buf, count, nBytesRead); }
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten )
            { return m_base_stream->Write(buf, count, nBytesWritten); }
    virtual bool Eof()
            { return m_base_stream->Eof(); }
    LVStreamProxy( LVStream * stream ) : m_base_stream(stream) { }
    ~LVStreamProxy() { delete m_base_stream; }
};

#endif  // __LVSTREAMPROXY_H_INCLUDED__
