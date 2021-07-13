/*******************************************************

   CoolReader Engine

   lvfileparserbase.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvfileparserbase.h"
#include "lvdocviewcallback.h"
#include "crlog.h"

#include <time.h>


#define BUF_SIZE_INCREMENT 4096


LVFileParserBase::LVFileParserBase( LVStreamRef stream )
    : m_stream(stream)
    , m_buf(NULL)
    , m_buf_size(0)
    , m_stream_size(0)
    , m_buf_len(0)
    , m_buf_pos(0)
    , m_buf_fpos(0)
    , m_stopped(false)
    , m_progressCallback(NULL)
    , m_lastProgressTime((time_t)0)
    , m_progressLastPercent(0)
    , m_progressUpdateCounter(0)
    , m_firstPageTextCounter(-1)

{
    m_stream_size = stream.isNull()?0:stream->GetSize();
}

/// returns pointer to loading progress callback object
LVDocViewCallback * LVFileParserBase::getProgressCallback()
{
    return m_progressCallback;
}

// should be (2^N - 1)
#define PROGRESS_UPDATE_RATE_MASK 63
/// call to send progress update to callback, if timeout expired
void LVFileParserBase::updateProgress()
{
    //CRLog::trace("LVFileParserBase::updateProgress() is called");
    if ( m_progressCallback == NULL )
        return;
    //CRLog::trace("LVFileParserBase::updateProgress() is called - 2");
    /// first page is loaded from file an can be formatted for preview
    if ( m_firstPageTextCounter>=0 ) {
        m_firstPageTextCounter--;
        if ( m_firstPageTextCounter==0 ) {
            if ( getProgressPercent()<30 )
                m_progressCallback->OnLoadFileFirstPagesReady();
            m_firstPageTextCounter=-1;
        }
    }
    m_progressUpdateCounter = (m_progressUpdateCounter + 1) & PROGRESS_UPDATE_RATE_MASK;
    if ( m_progressUpdateCounter!=0 )
        return; // to speed up checks
    time_t t = (time_t)time(NULL);
    if ( m_lastProgressTime==0 ) {
        m_lastProgressTime = t;
        return;
    }
    if ( t == m_lastProgressTime )
        return;
    int p = getProgressPercent();
    if ( p!= m_progressLastPercent ) {
        m_progressCallback->OnLoadFileProgress( p );
        m_progressLastPercent = p;
        m_lastProgressTime = t;
    }
}

/// sets pointer to loading progress callback object
void LVFileParserBase::setProgressCallback( LVDocViewCallback * callback )
{
    //CRLog::debug("LVFileParserBase::setProgressCallback is called");
    m_progressCallback = callback;
}

/// override to return file reading position percent
int LVFileParserBase::getProgressPercent()
{
    if ( m_stream_size<=0 )
        return 0;
    return (int)((lInt64)100 * (m_buf_pos + m_buf_fpos) / m_stream_size);
}

lString32 LVFileParserBase::getFileName()
{
    if ( m_stream.isNull() )
        return lString32::empty_str;
    lString32 name( m_stream->GetName() );
    int lastPathDelim = -1;
    for ( int i=0; i<name.length(); i++ ) {
        if ( name[i]=='\\' || name[i]=='/' ) {
            lastPathDelim = i;
        }
    }
    name = name.substr( lastPathDelim+1, name.length()-lastPathDelim-1 );
    return name;
}

/// stops parsing in the middle of file, to read header only
void LVFileParserBase::Stop()
{
    //CRLog::trace("LVTextFileBase::Stop() is called!");
    m_stopped = true;
}

/// destructor
LVFileParserBase::~LVFileParserBase()
{
    if (m_buf)
        free( m_buf );
}

/// seek to specified stream position
bool LVFileParserBase::Seek( lvpos_t pos, int bytesToPrefetch )
{
    if ( pos >= m_buf_fpos && pos+bytesToPrefetch <= (m_buf_fpos+m_buf_len) ) {
        m_buf_pos = (pos - m_buf_fpos);
        return true;
    }
    if ( pos>=m_stream_size )
        return false;
    unsigned bytesToRead = (bytesToPrefetch > m_buf_size) ? bytesToPrefetch : m_buf_size;
    if ( bytesToRead < BUF_SIZE_INCREMENT )
        bytesToRead = BUF_SIZE_INCREMENT;
    if ( bytesToRead > (m_stream_size - pos) )
        bytesToRead = (m_stream_size - pos);
    if ( (unsigned)m_buf_size < bytesToRead ) {
        m_buf_size = bytesToRead;
        m_buf = cr_realloc( m_buf, m_buf_size );
    }
    m_buf_fpos = pos;
    m_buf_pos = 0;
    m_buf_len = m_buf_size;
    // TODO: add error handing
    if ( m_stream->SetPos( m_buf_fpos ) != m_buf_fpos ) {
        CRLog::error("cannot set stream position to %d", (int)m_buf_pos );
        return false;
    }
    lvsize_t bytesRead = 0;
    if ( m_stream->Read( m_buf, bytesToRead, &bytesRead ) != LVERR_OK ) {
        CRLog::error("error while reading %d bytes from stream", (int)bytesToRead);
        return false;
    }
    return true;
}

bool LVFileParserBase::FillBuffer( int bytesToRead )
{
    lvoffset_t bytesleft = (lvoffset_t) (m_stream_size - (m_buf_fpos+m_buf_len));
    if (bytesleft<=0)
        return true; //FIX
    if (bytesToRead > bytesleft)
        bytesToRead = (int)bytesleft;
    int space = m_buf_size - m_buf_len;
    if (space < bytesToRead)
    {
        if ( m_buf_pos>bytesToRead || m_buf_pos>((m_buf_len*3)>>2) )
        {
            // just move
            int sz = (int)(m_buf_len -  m_buf_pos);
            for (int i=0; i<sz; i++)
            {
                m_buf[i] = m_buf[i+m_buf_pos];
            }
            m_buf_len = sz;
            m_buf_fpos += m_buf_pos;
            m_buf_pos = 0;
            space = m_buf_size - m_buf_len;
        }
        if (space < bytesToRead)
        {
            m_buf_size = m_buf_size + (bytesToRead - space + BUF_SIZE_INCREMENT);
            m_buf = cr_realloc( m_buf, m_buf_size );
        }
    }
    lvsize_t n = 0;
    if ( m_stream->Read(m_buf+m_buf_len, bytesToRead, &n) != LVERR_OK )
        return false;
//    if ( CRLog::isTraceEnabled() ) {
//        const lUInt8 * s = m_buf + m_buf_len;
//        const lUInt8 * s2 = m_buf + m_buf_len + (int)n - 8;
//        CRLog::trace("fpos=%06x+%06x, sz=%04x, data: %02x %02x %02x %02x %02x %02x %02x %02x .. %02x %02x %02x %02x %02x %02x %02x %02x",
//                     m_buf_fpos, m_buf_len, (int) n,
//                     s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
//                     s2[0], s2[1], s2[2], s2[3], s2[4], s2[5], s2[6], s2[7]
//                     );
//    }
    m_buf_len += (int)n;
    return (n>0);
}

void LVFileParserBase::Reset()
{
    m_stream->SetPos(0);
    m_buf_fpos = 0;
    m_buf_pos = 0;
    m_buf_len = 0;
    m_stream_size = m_stream->GetSize();
}
