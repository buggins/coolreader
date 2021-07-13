/*******************************************************

   CoolReader Engine

   lvrararc.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvrararc.h"

#if (USE_UNRAR==1)

LVStreamRef LVRarArc::OpenStream(const lChar32 *fname, lvopen_mode_t mode)
{
    int found_index = -1;
    for (int i=0; i<m_list.length(); i++) {
        if ( !lStr_cmp( fname, m_list[i]->GetName() ) ) {
            if ( m_list[i]->IsContainer() ) {
                // found directory with same name!!!
                return LVStreamRef();
            }
            found_index = i;
            break;
        }
    }
    if (found_index<0)
        return LVStreamRef(); // not found
    
    // TODO
    return LVStreamRef(); // not found
    /*
        // make filename
        lString32 fn = fname;
        LVStreamRef strm = m_stream; // fix strange arm-linux-g++ bug
        LVStreamRef stream(
        LVZipDecodeStream::Create(
            strm,
            m_list[found_index]->GetSrcPos(), fn ) );
        if (!stream.isNull()) {
            return LVCreateBufferedStream( stream, ZIP_STREAM_BUFFER_SIZE );
        }
        stream->SetName(m_list[found_index]->GetName());
        return stream;
*/
}

int LVRarArc::ReadContents()
{
    lvByteOrderConv cnv;
    
    m_list.clear();
    
    if (!m_stream || m_stream->Seek(0, LVSEEK_SET, NULL)!=LVERR_OK)
        return 0;
    
    SetName( m_stream->GetName() );
    
    lvsize_t sz = 0;
    if (m_stream->GetSize( &sz )!=LVERR_OK)
        return 0;
    lvsize_t m_FileSize = (unsigned)sz;
    
    return m_list.length();
}

LVArcContainerBase *LVRarArc::OpenArchieve(LVStreamRef stream)
{
    // read beginning of file
    const lvsize_t hdrSize = 4;
    char hdr[hdrSize];
    stream->SetPos(0);
    lvsize_t bytesRead = 0;
    if (stream->Read(hdr, hdrSize, &bytesRead)!=LVERR_OK || bytesRead!=hdrSize)
        return NULL;
    stream->SetPos(0);
    // detect arc type
    if (hdr[0]!='R' || hdr[1]!='a' || hdr[2]!='r' || hdr[3]!='!')
        return NULL;
    LVRarArc * arc = new LVRarArc( stream );
    int itemCount = arc->ReadContents();
    if ( itemCount <= 0 )
    {
        delete arc;
        return NULL;
    }
    return arc;
}

#endif  // (USE_UNRAR==1)
