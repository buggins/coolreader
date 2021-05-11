/** @file lvfileparserbase.h

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVFILEPARSERBASE_H_INCLUDED__
#define __LVFILEPARSERBASE_H_INCLUDED__

#include "lvfileformatparser.h"
#include "lvstream.h"

class LVFileParserBase : public LVFileFormatParser
{
protected:
    LVStreamRef m_stream;
    lUInt8 * m_buf;
    int      m_buf_size;
    lvsize_t m_stream_size;
    int      m_buf_len;
    int      m_buf_pos;
    lvpos_t  m_buf_fpos;
    bool     m_stopped; // true if Stop() is called
    LVDocViewCallback * m_progressCallback;
    time_t   m_lastProgressTime;
    int      m_progressLastPercent;
    int      m_progressUpdateCounter;
    int      m_firstPageTextCounter;
    /// fills buffer, to provide specified number of bytes for read
    bool FillBuffer( int bytesToRead );
    /// seek to specified stream position
    bool Seek( lvpos_t pos, int bytesToPrefetch=0 );
    /// override to return file reading position percent
    virtual int getProgressPercent();
public:
    /// call to send progress update to callback, if timeout expired
    void updateProgress();
    /// returns pointer to loading progress callback object
    virtual LVDocViewCallback * getProgressCallback();
    /// sets pointer to loading progress callback object
    virtual void setProgressCallback( LVDocViewCallback * callback );
    /// constructor
    LVFileParserBase( LVStreamRef stream );
    /// virtual destructor
    virtual ~LVFileParserBase();
    /// returns source stream
    LVStreamRef getStream() { return m_stream; }
    /// return stream file name
    lString32 getFileName();
    /// returns true if end of fle is reached, and there is no data left in buffer
    virtual bool Eof() { return m_buf_fpos + m_buf_pos >= m_stream_size; }
    /// resets parsing, moves to beginning of stream
    virtual void Reset();
    /// stops parsing in the middle of file, to read header only
    virtual void Stop();
};

#endif  // __LVFILEPARSERBASE_H_INCLUDED__
