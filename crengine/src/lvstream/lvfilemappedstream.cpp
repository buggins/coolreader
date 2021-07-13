/*******************************************************

   CoolReader Engine

   lvfilemappedstream.cpp

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

#include "lvfilemappedstream.h"
#include "lvstreamutils.h"
#include "crlog.h"

#if !defined(__SYMBIAN32__) && defined(_WIN32)
extern "C" {
#include <windows.h>
}
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#endif

// To support "large files" on 32-bit platforms
// Since we have defined own types 'lvoffset_t', 'lvpos_t' and do not use the system type 'off_t'
// it is logical to define our own wrapper function 'lseek'.
static inline lvpos_t cr3_lseek(int fd, lvoffset_t offset, int whence) {
#if LVLONG_FILE_SUPPORT == 1
    return (lvpos_t)::lseek64(fd, (off64_t)offset, whence);
#else
    return (lvpos_t)::lseek(fd, (off_t)offset, whence);
#endif
}

LVStreamBufferRef LVFileMappedStream::GetReadBuffer(lvpos_t pos, lvpos_t size)
{
    LVStreamBufferRef res;
    if ( !m_map )
        return res;
    if ( (m_mode!=LVOM_APPEND && m_mode!=LVOM_READ) || pos + size > m_size || size==0 )
        return res;
    return LVStreamBufferRef ( new LVBuffer( LVStreamRef(this), m_map + pos, size, true ) );
}

LVStreamBufferRef LVFileMappedStream::GetWriteBuffer(lvpos_t pos, lvpos_t size)
{
    LVStreamBufferRef res;
    if ( !m_map )
        return res;
    if ( m_mode!=LVOM_APPEND || pos + size > m_size || size==0 )
        return res;
    return LVStreamBufferRef ( new LVBuffer( LVStreamRef(this), m_map + pos, size, false ) );
}

lverror_t LVFileMappedStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* pNewPos)
{
    //
    lvpos_t newpos = m_pos;
    switch ( origin )
    {
        case LVSEEK_SET:
            newpos = offset;
            break;
        case LVSEEK_CUR:
            newpos += offset;
            break;
        case LVSEEK_END:
            newpos = m_size + offset;
            break;
    }
    if ( newpos>m_size )
        return LVERR_FAIL;
    if ( pNewPos!=NULL )
        *pNewPos = newpos;
    m_pos = newpos;
    return LVERR_OK;
}

lverror_t LVFileMappedStream::Tell(lvpos_t* pPos)
{
    *pPos = m_pos;
    return LVERR_OK;
}

lvpos_t LVFileMappedStream::SetPos(lvpos_t p)
{
    if ( p<=m_size ) {
        m_pos = p;
        return m_pos;
    }
    return (lvpos_t)(~0);
}

lverror_t LVFileMappedStream::error()
{
#if defined(_WIN32)
    if ( m_hFile!=NULL ) {
        UnMap();
        if ( !CloseHandle(m_hFile) )
            CRLog::error("Error while closing file handle");
        m_hFile = NULL;
    }
#else
    if ( m_fd!= -1 ) {
        CRLog::trace("Closing mapped file %s", UnicodeToUtf8(GetName()).c_str() );
        UnMap();
        ::close(m_fd);
    }
    m_fd = -1;
#endif
    m_map = NULL;
    m_size = 0;
    m_mode = LVOM_ERROR;
    return LVERR_FAIL;
}

lverror_t LVFileMappedStream::Read(void* buf, lvsize_t count, lvsize_t* nBytesRead)
{
    if ( !m_map )
        return LVERR_FAIL;
    int cnt = (int)count;
    if ( m_pos + cnt > m_size )
        cnt = (int)(m_size - m_pos);
    if ( cnt <= 0 )
        return LVERR_FAIL;
    memcpy( buf, m_map + m_pos, cnt );
    m_pos += cnt;
    if (nBytesRead)
        *nBytesRead = cnt;
    return LVERR_OK;
}

bool LVFileMappedStream::Read(lUInt8* buf)
{
    if ( m_pos < m_size ) {
        *buf = m_map[ m_pos++ ];
        return true;
    }
    return false;
}

bool LVFileMappedStream::Read(lUInt16* buf)
{
    if ( m_pos+1 < m_size ) {
        *buf = m_map[ m_pos ] | ( ( (lUInt16)m_map[ m_pos+1 ] )<<8 );
        m_pos += 2;
        return true;
    }
    return false;
}

bool LVFileMappedStream::Read(lUInt32* buf)
{
    if ( m_pos+3 < m_size ) {
        *buf = m_map[ m_pos ] | ( ( (lUInt32)m_map[ m_pos+1 ] )<<8 )
                | ( ( (lUInt32)m_map[ m_pos+2 ] )<<16 )
                | ( ( (lUInt32)m_map[ m_pos+3 ] )<<24 )
                ;
        m_pos += 4;
        return true;
    }
    return false;
}

int LVFileMappedStream::ReadByte()
{
    if ( m_pos < m_size ) {
        return m_map[ m_pos++ ];
    }
    return -1;
}

lverror_t LVFileMappedStream::Write(const void* buf, lvsize_t count, lvsize_t* nBytesWritten)
{
    if ( m_mode!=LVOM_APPEND )
        return LVERR_FAIL;
    lvsize_t maxSize = (lvsize_t)(m_size - m_pos);
    if ( maxSize<=0 )
        return LVERR_FAIL; // end of file reached: resize is not supported yet
    if ( count > maxSize || count > m_size )
        count = maxSize;
    memcpy( m_map + m_pos, buf, count );
    m_pos += count;
    if ( nBytesWritten )
        *nBytesWritten = count;
    return LVERR_OK;
}

LVFileMappedStream* LVFileMappedStream::CreateFileStream(lString32 fname, lvopen_mode_t mode, int minSize)
{
    LVFileMappedStream * f = new LVFileMappedStream();
    if ( f->OpenFile( fname, mode, minSize )==LVERR_OK ) {
        return f;
    } else {
        delete f;
        return NULL;
    }
}

lverror_t LVFileMappedStream::Map()
{
#if defined(_WIN32)
    m_hMap = CreateFileMapping(
                 m_hFile,
                 NULL,
                 (m_mode==LVOM_READ)?PAGE_READONLY:PAGE_READWRITE, //flProtect,
                 0,
                 0,
                 NULL
                 );
    if ( m_hMap==NULL ) {
        DWORD err = GetLastError();
        CRLog::error( "LVFileMappedStream::Map() -- Cannot map file to memory, err=%08x, hFile=%p", err, m_hFile );
        return error();
    }
    m_map = (lUInt8*) MapViewOfFile(
                m_hMap,
                m_mode==LVOM_READ ? FILE_MAP_READ : FILE_MAP_READ|FILE_MAP_WRITE,
                0,
                0,
                m_size
                );
    if ( m_map==NULL ) {
        CRLog::error( "LVFileMappedStream::Map() -- Cannot map file to memory" );
        return error();
    }
    return LVERR_OK;
#else
    int mapFlags = (m_mode==LVOM_READ) ? PROT_READ : PROT_READ | PROT_WRITE;
    m_map = (lUInt8*)mmap( 0, m_size, mapFlags, MAP_SHARED, m_fd, 0 );
    if ( m_map == MAP_FAILED ) {
        CRLog::error( "LVFileMappedStream::Map() -- Cannot map file to memory" );
        return error();
    }
    return LVERR_OK;
#endif
}

lverror_t LVFileMappedStream::UnMap()
{
#if defined(_WIN32)
    lverror_t res = LVERR_OK;
    if ( m_map!=NULL ) {
        if ( !UnmapViewOfFile( m_map ) ) {
            CRLog::error("LVFileMappedStream::UnMap() -- Error while unmapping file");
            res = LVERR_FAIL;
        }
        m_map = NULL;
    }
    if ( m_hMap!=NULL ) {
        if ( !CloseHandle( m_hMap ) ) {
            CRLog::error("LVFileMappedStream::UnMap() -- Error while unmapping file");
            res = LVERR_FAIL;
        }
        m_hMap = NULL;
    }
    if ( res!=LVERR_OK )
        return error();
    return res;
#else
    if ( m_map!=NULL && munmap( m_map, m_size ) == -1 ) {
        m_map = NULL;
        CRLog::error("LVFileMappedStream::UnMap() -- Error while unmapping file");
        return error();
    }
    return LVERR_OK;
#endif
}

lverror_t LVFileMappedStream::SetSize(lvsize_t size)
{
    // support only size grow
    if ( m_mode!=LVOM_APPEND )
        return LVERR_FAIL;
    if ( size == m_size )
        return LVERR_OK;
    //if ( size < m_size )
    //    return LVERR_FAIL;

    bool wasMapped = false;
    if ( m_map!=NULL ) {
        wasMapped = true;
        if ( UnMap()!=LVERR_OK )
            return LVERR_FAIL;
    }
    m_size = size;

#if defined(_WIN32)
    // WIN32
    __int64 offset = size - 1;
    lUInt32 pos_low = (lUInt32)((__int64)offset & 0xFFFFFFFF);
    LONG pos_high = (long)(((__int64)offset >> 32) & 0xFFFFFFFF);
    pos_low = SetFilePointer(m_hFile, pos_low, &pos_high, FILE_BEGIN );
    if (pos_low == 0xFFFFFFFF) {
        lUInt32 err = GetLastError();
        if (err == ERROR_NOACCESS)
            pos_low = (lUInt32)offset;
        else if ( err != ERROR_SUCCESS)
            return error();
    }
    DWORD bytesWritten = 0;
    if ( !WriteFile( m_hFile, "", 1, &bytesWritten, NULL ) || bytesWritten!=1 )
        return error();
#else
    // LINUX
    if ( cr3_lseek( m_fd, size-1, SEEK_SET ) == (lvpos_t)-1 ) {
        CRLog::error("LVFileMappedStream::SetSize() -- Seek error");
        return error();
    }
    if ( write(m_fd, "", 1) != 1 ) {
        CRLog::error("LVFileMappedStream::SetSize() -- File resize error");
        return error();
    }
#endif
    if ( wasMapped ) {
        if ( Map() != LVERR_OK ) {
            return error();
        }
    }
    return LVERR_OK;
}

lverror_t LVFileMappedStream::OpenFile(lString32 fname, lvopen_mode_t mode, lvsize_t minSize)
{
    m_mode = mode;
    if ( mode!=LVOM_READ && mode!=LVOM_APPEND )
        return LVERR_FAIL; // not supported
    if ( minSize==(lvsize_t)-1 ) {
        if ( !LVFileExists(fname) )
            return LVERR_FAIL;
    }
    //if ( mode==LVOM_APPEND && minSize<=0 )
    //    return LVERR_FAIL;
    SetName(fname.c_str());
    lString8 fn8 = UnicodeToUtf8( fname );
#if defined(_WIN32)
    //========================================================
    // WIN32 IMPLEMENTATION
    lUInt32 m = 0;
    lUInt32 s = 0;
    lUInt32 c = 0;
    lString16 fn16 = UnicodeToUtf16( fname );
    switch (mode) {
        case LVOM_READWRITE:
            m |= GENERIC_WRITE|GENERIC_READ;
            s |= FILE_SHARE_WRITE|FILE_SHARE_READ;
            c |= OPEN_ALWAYS;
            break;
        case LVOM_READ:
            m |= GENERIC_READ;
            s |= FILE_SHARE_READ;
            c |= OPEN_EXISTING;
            break;
        case LVOM_WRITE:
            m |= GENERIC_WRITE;
            s |= FILE_SHARE_WRITE;
            c |= CREATE_ALWAYS;
            break;
        case LVOM_APPEND:
            m |= GENERIC_WRITE|GENERIC_READ;
            s |= FILE_SHARE_WRITE;
            c |= OPEN_ALWAYS;
            break;
        case LVOM_CLOSED:
        case LVOM_ERROR:
            crFatalError();
            break;
    }
    m_hFile = CreateFileW( fn16.c_str(), m, s, NULL, c, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE || !m_hFile) {
        // unicode not implemented?
        lUInt32 err = GetLastError();
        if (err==ERROR_CALL_NOT_IMPLEMENTED)
            m_hFile = CreateFileA( UnicodeToLocal(fname).c_str(), m, s, NULL, c, FILE_ATTRIBUTE_NORMAL, NULL);
        if ( (m_hFile == INVALID_HANDLE_VALUE) || (!m_hFile) ) {
            CRLog::error("Error opening file %s", fn8.c_str() );
            m_hFile = NULL;
            // error
            return error();
        }
    }
    // check size
    lUInt32 hw=0;
    m_size = GetFileSize( m_hFile, (LPDWORD)&hw );
#if LVLONG_FILE_SUPPORT
    if (hw)
        m_size |= (((lvsize_t)hw)<<32);
#endif

    if ( mode == LVOM_APPEND && m_size < minSize ) {
        if ( SetSize( minSize ) != LVERR_OK ) {
            CRLog::error( "Cannot set file size for %s", fn8.c_str() );
            return error();
        }
    }

    if ( Map()!=LVERR_OK )
        return error();

    return LVERR_OK;


#else
    //========================================================
    // LINUX IMPLEMENTATION
    m_fd = -1;

    int flags = (mode==LVOM_READ) ? O_RDONLY : O_RDWR | O_CREAT; // | O_SYNC
    m_fd = open( fn8.c_str(), flags, (mode_t)0666);
    if (m_fd == -1) {
        CRLog::error( "Error opening file %s for %s, errno=%d, msg=%s", fn8.c_str(), (mode==LVOM_READ) ? "reading" : "read/write",  (int)errno, strerror(errno) );
        return error();
    }
    struct stat stat;
    if ( fstat( m_fd, &stat ) ) {
        CRLog::error( "Cannot get file size for %s", fn8.c_str() );
        return error();
    }
    m_size = (lvsize_t) stat.st_size;
    if ( mode == LVOM_APPEND && m_size < minSize ) {
        if ( SetSize( minSize ) != LVERR_OK ) {
            CRLog::error( "Cannot set file size for %s", fn8.c_str() );
            return error();
        }
    }

    int mapFlags = (mode==LVOM_READ) ? PROT_READ : PROT_READ | PROT_WRITE;
    m_map = (lUInt8*)mmap( 0, m_size, mapFlags, MAP_SHARED, m_fd, 0 );
    if ( m_map == MAP_FAILED ) {
        CRLog::error( "Cannot map file %s to memory", fn8.c_str() );
        return error();
    }
    return LVERR_OK;
#endif
}

LVFileMappedStream::LVFileMappedStream()
#if defined(_WIN32)
    : m_hFile(NULL), m_hMap(NULL),
      #else
    : m_fd(-1),
      #endif
      m_map(NULL), m_size(0), m_pos(0)
{
    m_mode=LVOM_ERROR;
}

LVFileMappedStream::~LVFileMappedStream()
{
    // reuse error() to close file
    error();
}
