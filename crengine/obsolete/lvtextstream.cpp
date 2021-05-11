/*******************************************************

   CoolReader Engine

   lvnamedstream.cpp:

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

#include "lvtextstream.h"

lvopen_mode_t LVTextStream::GetMode()
{
    return m_base_stream->GetMode();
}

lverror_t LVTextStream::Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos )
{
    return m_base_stream->Seek(offset, origin, pNewPos);
}

lverror_t LVTextStream::Tell( lvpos_t * pPos )
{
    return m_base_stream->Tell(pPos);
}

lvpos_t   LVTextStream::SetPos(lvpos_t p)
{
    return m_base_stream->SetPos(p);
}

lvpos_t   LVTextStream::GetPos()
{
    return m_base_stream->GetPos();
}

lverror_t LVTextStream::SetSize( lvsize_t size )
{
    return m_base_stream->SetSize(size);
}

lverror_t LVTextStream::Read( void * buf, lvsize_t count, lvsize_t * nBytesRead )
{
    return m_base_stream->Read(buf, count, nBytesRead);
}

lverror_t LVTextStream::Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten )
{
    return m_base_stream->Write(buf, count, nBytesWritten);
}

bool LVTextStream::Eof()
{
    return m_base_stream->Eof();
}
