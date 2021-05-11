/*******************************************************

   CoolReader Engine

   lvdefstreambuffer.cpp:  Universal Read or write buffer for stream region for non-meped streams

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

#include "lvdefstreambuffer.h"

LVStreamBufferRef LVDefStreamBuffer::create(LVStreamRef stream, lvpos_t pos, lvsize_t size, bool readonly)
{
    LVStreamBufferRef res;
    switch ( stream->GetMode() ) {
        case LVOM_ERROR:       ///< to indicate error state
        case LVOM_CLOSED:        ///< to indicate closed state
            return res;
        case LVOM_READ:          ///< readonly mode, use for r/o
            if ( !readonly )
                return res;
            break;
        case LVOM_WRITE:         ///< writeonly mode
        case LVOM_APPEND:        ///< append (readwrite) mode, use for r/w
        case LVOM_READWRITE:      ///< readwrite mode
            if ( readonly )
                return res;
            break;
    }
    lvsize_t sz;
    if ( stream->GetSize(&sz)!=LVERR_OK )
        return res;
    if ( pos + size > sz )
        return res; // wrong position/size
    LVDefStreamBuffer * buf = new LVDefStreamBuffer( stream, pos, size, readonly );
    if ( !buf->m_buf ) {
        delete buf;
        return res;
    }
    if ( stream->SetPos( pos )!=LVERR_OK ) {
        delete buf;
        return res;
    }
    lvsize_t bytesRead = 0;
    if ( stream->Read( buf->m_buf, size, &bytesRead )!=LVERR_OK || bytesRead!=size ) {
        delete buf;
        return res;
    }
    return LVStreamBufferRef( buf );
}

LVDefStreamBuffer::LVDefStreamBuffer(LVStreamRef stream, lvpos_t pos, lvsize_t size, bool readonly)
    : m_stream( stream ), m_buf( NULL ), m_pos(pos), m_size( size ), m_readonly( readonly )
{
    m_buf = (lUInt8*)malloc( size );
    m_writeonly = (m_stream->GetMode()==LVOM_WRITE);
}

bool LVDefStreamBuffer::close()
{
    bool res = true;
    if ( m_buf ) {
        if ( !m_readonly ) {
            if ( m_stream->SetPos( m_pos )!=LVERR_OK ) {
                res = false;
            } else {
                lvsize_t bytesWritten = 0;
                if ( m_stream->Write( m_buf, m_size, &bytesWritten )!=LVERR_OK || bytesWritten!=m_size ) {
                    res = false;
                }
            }
        }
        free( m_buf );
    }
    m_buf = NULL;
    m_stream = NULL;
    m_size = 0;
    m_pos = 0;
    return res;
}
