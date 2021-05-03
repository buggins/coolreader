/** @file lvfilestream.h

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

#ifndef __LVFILESTREAM_H_INCLUDED__
#define __LVFILESTREAM_H_INCLUDED__

#include "crsetup.h"
#include "lvnamedstream.h"
#include "lvstream_types.h"

//#ifdef _LINUX
#undef USE_ANSI_FILES
//#endif

#if (USE_ANSI_FILES==1)

#include <stdio.h>

class LVFileStream : public LVNamedStream
{
private:
    FILE * m_file;
public:
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos );
    virtual lverror_t SetSize( lvsize_t );
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead );
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten );
    /// flushes unsaved data from buffers to file, with optional flush of OS buffers
    virtual lverror_t Flush( bool sync );
    virtual bool Eof();
    static LVFileStream * CreateFileStream( lString32 fname, lvopen_mode_t mode );
    lverror_t OpenFile( lString32 fname, lvopen_mode_t mode );
    LVFileStream();
    virtual ~LVFileStream();
};

#else   // (USE_ANSI_FILES==1)

#if !defined(__SYMBIAN32__) && defined(_WIN32)
extern "C" {
#include <windows.h>
}
#endif

class LVFileStream : public LVNamedStream
{
    friend class LVDirectoryContainer;
protected:
#if defined(_WIN32)
    HANDLE m_hFile;
#else
    int m_fd;
#endif
    //LVDirectoryContainer * m_parent;
    lvsize_t               m_size;
    lvpos_t                m_pos;
public:
    /// flushes unsaved data from buffers to file, with optional flush of OS buffers
    virtual lverror_t Flush( bool sync );
    virtual bool Eof();
//    virtual LVContainer * GetParentContainer()
//    {
//        return (LVContainer*)m_parent;
//    }
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead );
    virtual lverror_t GetSize( lvsize_t * pSize );
    virtual lvsize_t GetSize();
    virtual lverror_t SetSize( lvsize_t size );
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten );
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos );
    lverror_t Close();
    static LVFileStream * CreateFileStream( lString32 fname, lvopen_mode_t mode );
    lverror_t OpenFile( lString32 fname, int mode );
    LVFileStream();
    virtual ~LVFileStream();
};
#endif  // (USE_ANSI_FILES==1)

#endif  // __LVFILESTREAM_H_INCLUDED__
