/*******************************************************

   CoolReader Engine

   lvmemorystream.cpp

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

#include "lvmemorystream.h"
#include "lvmemman.h"

lverror_t LVMemoryStream::SetMode(lvopen_mode_t mode)
{
    if ( m_mode==mode )
        return LVERR_OK;
    if ( m_mode==LVOM_WRITE && mode==LVOM_READ ) {
        m_mode = LVOM_READ;
        m_pos = 0;
        return LVERR_OK;
    }
    // TODO: READ -> WRITE/APPEND
    return LVERR_FAIL;
}

lverror_t LVMemoryStream::Read(void *buf, lvsize_t count, lvsize_t *nBytesRead)
{
    if (!m_pBuffer || m_mode==LVOM_WRITE || m_mode==LVOM_APPEND )
        return LVERR_FAIL;
    //
    int bytesAvail = (int)(m_size - m_pos);
    if (bytesAvail>0) {
        int bytesRead = bytesAvail;
        if (bytesRead>(int)count)
            bytesRead = (int)count;
        if (bytesRead>0)
            memcpy( buf, m_pBuffer+(int)m_pos, bytesRead );
        if (nBytesRead)
            *nBytesRead = bytesRead;
        m_pos += bytesRead;
    } else {
        if (nBytesRead)
            *nBytesRead = 0; // EOF
    }
    return LVERR_OK;
}

lvsize_t LVMemoryStream::GetSize()
{
    if (!m_pBuffer)
        return (lvsize_t)(-1);
    if (m_size<m_pos)
        m_size = m_pos;
    return m_size;
}

lverror_t LVMemoryStream::GetSize(lvsize_t *pSize)
{
    if (!m_pBuffer || !pSize)
        return LVERR_FAIL;
    if (m_size<m_pos)
        m_size = m_pos;
    *pSize = m_size;
    return LVERR_OK;
}

lverror_t LVMemoryStream::SetBufSize(lvsize_t new_size)
{
    if (!m_pBuffer || m_mode==LVOM_READ )
        return LVERR_FAIL;
    if (new_size<=m_bufsize)
        return LVERR_OK;
    if (m_own_buffer!=true)
        return LVERR_FAIL; // cannot resize foreign buffer
    //
    int newbufsize = (int)(new_size * 2 + 4096);
    m_pBuffer = cr_realloc( m_pBuffer, newbufsize );
    m_bufsize = newbufsize;
    return LVERR_OK;
}

lverror_t LVMemoryStream::SetSize(lvsize_t size)
{
    //
    if (SetBufSize( size )!=LVERR_OK)
        return LVERR_FAIL;
    m_size = size;
    if (m_pos>m_size)
        m_pos = m_size;
    return LVERR_OK;
}

lverror_t LVMemoryStream::Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten)
{
    if (!m_pBuffer || !buf || m_mode==LVOM_READ )
        return LVERR_FAIL;
    SetBufSize( m_pos+count ); // check buf size
    int bytes_avail = (int)(m_bufsize-m_pos);
    if (bytes_avail>(int)count)
        bytes_avail = (int)count;
    if (bytes_avail>0) {
        memcpy( m_pBuffer+m_pos, buf, bytes_avail );
        m_pos+=bytes_avail;
        if (m_size<m_pos)
            m_size = m_pos;
    }
    if (nBytesWritten)
        *nBytesWritten = bytes_avail;
    return LVERR_OK;
}

lverror_t LVMemoryStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos)
{
    if (!m_pBuffer)
        return LVERR_FAIL;
    lvpos_t newpos;
    switch (origin) {
        case LVSEEK_SET:
            newpos = offset;
            break;
        case LVSEEK_CUR:
            newpos = m_pos + offset;
            break;
        case LVSEEK_END:
            newpos = m_size + offset;
            break;
    }
    if (newpos>m_size)
        return LVERR_FAIL;
    m_pos = newpos;
    if (pNewPos)
        *pNewPos = m_pos;
    return LVERR_OK;
}

lverror_t LVMemoryStream::Close()
{
    if (!m_pBuffer)
        return LVERR_FAIL;
    if (m_pBuffer && m_own_buffer)
        free(m_pBuffer);
    m_pBuffer = NULL;
    m_size = 0;
    m_bufsize = 0;
    m_pos = 0;
    return LVERR_OK;
}

lverror_t LVMemoryStream::Create()
{
    Close();
    m_bufsize = 4096;
    m_size = 0;
    m_pos = 0;
    m_pBuffer = (lUInt8*)malloc((int)m_bufsize);
    m_own_buffer = true;
    m_mode = LVOM_READWRITE;
    return LVERR_OK;
}

lverror_t LVMemoryStream::CreateCopy(LVStreamRef srcStream, lvopen_mode_t mode)
{
    Close();
    if ( mode!=LVOM_READ || srcStream.isNull() )
        return LVERR_FAIL;
    lvsize_t sz = srcStream->GetSize();
    if ( (int)sz <= 0 || sz > 0x200000 )
        return LVERR_FAIL;
    m_bufsize = sz;
    m_size = 0;
    m_pos = 0;
    m_pBuffer = (lUInt8*)malloc((int)m_bufsize);
    if (m_pBuffer) {
        lvsize_t bytesRead = 0;
        srcStream->Read( m_pBuffer, m_bufsize, &bytesRead );
        if ( bytesRead!=m_bufsize ) {
            free(m_pBuffer);
            m_pBuffer = 0;
            m_size = 0;
            m_pos = 0;
            m_bufsize = 0;
            return LVERR_FAIL;
        }
    }
    m_size = sz;
    m_own_buffer = true;
    m_mode = mode;
    return LVERR_OK;
}

lverror_t LVMemoryStream::CreateCopy(const lUInt8 *pBuf, lvsize_t size, lvopen_mode_t mode)
{
    Close();
    m_bufsize = size;
    m_pos = 0;
    m_pBuffer = (lUInt8*) malloc((int)m_bufsize);
    if (m_pBuffer) {
        memcpy( m_pBuffer, pBuf, (int)size );
    }
    m_own_buffer = true;
    m_mode = mode;
    m_size = size;
    if (mode==LVOM_APPEND)
        m_pos = m_size;
    return LVERR_OK;
}

lverror_t LVMemoryStream::Open(lUInt8 *pBuf, lvsize_t size)
{
    if (!pBuf)
        return LVERR_FAIL;
    m_own_buffer = false;
    m_pBuffer = pBuf;
    m_bufsize = size;
    // set file size and position
    m_pos = 0;
    m_size = size;
    m_mode = LVOM_READ;
    
    return LVERR_OK;
}

LVMemoryStream::LVMemoryStream() : m_pBuffer(NULL), m_own_buffer(false), m_parent(NULL), m_size(0), m_pos(0)
{
}

LVMemoryStream::~LVMemoryStream()
{
    Close();
    m_parent = NULL;
}
