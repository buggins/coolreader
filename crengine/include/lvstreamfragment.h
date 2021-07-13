/** @file lvstreamfragment.h

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

#ifndef __LVSTREAMFRAGMENT_H_INCLUDED__
#define __LVSTREAMFRAGMENT_H_INCLUDED__

#include "lvnamedstream.h"

class LVStreamFragment : public LVNamedStream
{
private:
    LVStreamRef m_stream;
    lvsize_t    m_start;
    lvsize_t    m_size;
    lvpos_t     m_pos;
public:
    LVStreamFragment( LVStreamRef stream, lvsize_t start, lvsize_t size )
        : m_stream(stream), m_start(start), m_size(size), m_pos(0)
    {
    }
    virtual bool Eof()
    {
        return m_pos >= m_size;
    }
    virtual lvsize_t  GetSize()
    {
        return m_size;
    }

    virtual lverror_t Seek(lvoffset_t pos, lvseek_origin_t origin, lvpos_t* newPos)
    {
        if ( origin==LVSEEK_SET )
            pos += m_start;
        else if ( origin==LVSEEK_END ) {
            origin = LVSEEK_SET;
            pos = m_start + m_size;
        }
        lverror_t res = m_stream->Seek( pos, origin, &m_pos );
        if (res == LVERR_OK)
            m_pos -= m_start;
        if (newPos)
        {
            *newPos =  m_pos;
        }
        return res;
    }
    virtual lverror_t Write(const void*, lvsize_t, lvsize_t*)
    {
        return LVERR_NOTIMPL;
    }
    virtual lverror_t Read(void* buf, lvsize_t size, lvsize_t* pBytesRead)
    {
        lvsize_t bytesRead = 0;
        lvpos_t p;
        lverror_t res = m_stream->Seek( m_pos+m_start, LVSEEK_SET, &p );
        if ( res!=LVERR_OK )
            return res;
        res = m_stream->Read( buf, size, &bytesRead );
        if (res == LVERR_OK)
            m_pos += bytesRead;
        if (pBytesRead)
            *pBytesRead = bytesRead;
        return res;
    }
    virtual lverror_t SetSize(lvsize_t)
    {
        return LVERR_NOTIMPL;
    }
};

#endif  // __LVSTREAMFRAGMENT_H_INCLUDED__
