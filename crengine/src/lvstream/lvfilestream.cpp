/*******************************************************

   CoolReader Engine

   lvfilestream.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvfilestream.h"
#include "crlog.h"

#if (USE_ANSI_FILES==1)

lverror_t LVFileStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos)
{
    //
    int res = -1;
    switch ( origin )
    {
        case LVSEEK_SET:
            res = fseek( m_file, offset, SEEK_SET );
            break;
        case LVSEEK_CUR:
            res = fseek( m_file, offset, SEEK_CUR );
            break;
        case LVSEEK_END:
            res = fseek( m_file, offset, SEEK_END );
            break;
    }
    if (res==0)
    {
        if ( pNewPos )
            * pNewPos = ftell(m_file);
        return LVERR_OK;
    }
    CRLog::error("error setting file position to %d (%d)", (int)offset, (int)origin );
    return LVERR_FAIL;
}

lverror_t LVFileStream::SetSize(lvsize_t)
{
    /*
        int64 sz = m_file->SetSize( size );
        if (sz==-1)
           return LVERR_FAIL;
        else
           return LVERR_OK;
        */
    return LVERR_FAIL;
}

lverror_t LVFileStream::Read(void *buf, lvsize_t count, lvsize_t *nBytesRead)
{
    lvsize_t sz = fread( buf, 1, count, m_file );
    if (nBytesRead)
        *nBytesRead = sz;
    if ( sz==0 )
    {
        return LVERR_FAIL;
    }
    return LVERR_OK;
}

lverror_t LVFileStream::Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten)
{
    lvsize_t sz = fwrite( buf, 1, count, m_file );
    if (nBytesWritten)
        *nBytesWritten = sz;
    handleAutoSync(sz);
    if (sz < count)
    {
        return LVERR_FAIL;
    }
    return LVERR_OK;
}

lverror_t LVFileStream::Flush(bool sync)
{
    if ( !m_file )
        return LVERR_FAIL;
    fflush( m_file );
    return LVERR_OK;
}

bool LVFileStream::Eof()
{
    return feof(m_file)!=0;
}

LVFileStream *LVFileStream::CreateFileStream(lString32 fname, lvopen_mode_t mode)
{
    LVFileStream * f = new LVFileStream;
    if (f->OpenFile( fname, mode )==LVERR_OK) {
        return f;
    } else {
        delete f;
        return NULL;
    }
}

lverror_t LVFileStream::OpenFile(lString32 fname, lvopen_mode_t mode)
{
    m_mode = mode;
    m_file = NULL;
    SetName(fname.c_str());
    const char * modestr = "r";
    switch (mode) {
        case LVOM_READ:
            modestr = "rb";
            break;
        case LVOM_WRITE:
            modestr = "wb";
            break;
        case LVOM_READWRITE:
        case LVOM_APPEND:
            modestr = "a+b";
            break;
        case LVOM_CLOSED:
        case LVOM_ERROR:
            break;
    }
    FILE * file = fopen(UnicodeToLocal(fname).c_str(), modestr);
    if (!file)
    {
        //printf("cannot open file %s\n", UnicodeToLocal(fname).c_str());
        m_mode = LVOM_ERROR;
        return LVERR_FAIL;
    }
    m_file = file;
    //printf("file %s opened ok\n", UnicodeToLocal(fname).c_str());
    // set filename
    SetName( fname.c_str() );
    return LVERR_OK;
}

LVFileStream::LVFileStream() : m_file(NULL)
{
    m_mode=LVOM_ERROR;
}

LVFileStream::~LVFileStream()
{
    if (m_file)
        fclose(m_file);
}

#else

#if !defined(__SYMBIAN32__) && defined(_WIN32)
extern "C" {
#include <windows.h>
}
#include "io.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
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


lverror_t LVFileStream::Flush(bool sync)
{
    CR_UNUSED(sync);
#ifdef _WIN32
    if ( m_hFile==INVALID_HANDLE_VALUE || !FlushFileBuffers( m_hFile ) )
        return LVERR_FAIL;
#else
    if ( m_fd==-1 )
        return LVERR_FAIL;
    if ( sync ) {
        //            CRTimerUtil timer;
        //            CRLog::trace("calling fsync");
        fsync( m_fd );
        //            CRLog::trace("fsync took %d ms", (int)timer.elapsed());
    }
#endif
    return LVERR_OK;
}

bool LVFileStream::Eof()
{
    return m_size<=m_pos;
}

lverror_t LVFileStream::Read(void *buf, lvsize_t count, lvsize_t *nBytesRead)
{
#ifdef _WIN32
    //fprintf(stderr, "Read(%08x, %d)\n", buf, count);
    
    if (m_hFile == INVALID_HANDLE_VALUE || m_mode==LVOM_WRITE ) // || m_mode==LVOM_APPEND
        return LVERR_FAIL;
    //
    if ( m_pos > m_size )
        return LVERR_FAIL; // EOF
    
    lUInt32 dwBytesRead = 0;
    if (ReadFile( m_hFile, buf, (lUInt32)count, (LPDWORD)&dwBytesRead, NULL )) {
        if (nBytesRead)
            *nBytesRead = dwBytesRead;
        m_pos += dwBytesRead;
        return LVERR_OK;
    } else {
        //DWORD err = GetLastError();
        if (nBytesRead)
            *nBytesRead = 0;
        return LVERR_FAIL;
    }
    
#else
    if (m_fd == -1)
        return LVERR_FAIL;
    ssize_t res = read( m_fd, buf, count );
    if ( res!=(ssize_t)-1 ) {
        if (nBytesRead)
            *nBytesRead = res;
        m_pos += res;
        return LVERR_OK;
    }
    if (nBytesRead)
        *nBytesRead = 0;
    return LVERR_FAIL;
#endif
}

lverror_t LVFileStream::GetSize(lvsize_t *pSize)
{
#ifdef _WIN32
    if (m_hFile == INVALID_HANDLE_VALUE || !pSize)
        return LVERR_FAIL;
#else
    if (m_fd == -1 || !pSize)
        return LVERR_FAIL;
#endif
    if (m_size<m_pos)
        m_size = m_pos;
    *pSize = m_size;
    return LVERR_OK;
}

lvsize_t LVFileStream::GetSize()
{
#ifdef _WIN32
    if (m_hFile == INVALID_HANDLE_VALUE)
        return 0;
    if (m_size<m_pos)
        m_size = m_pos;
    return m_size;
#else
    if (m_fd == -1)
        return 0;
    if (m_size<m_pos)
        m_size = m_pos;
    return m_size;
#endif
}

lverror_t LVFileStream::SetSize(lvsize_t size)
{
#ifdef _WIN32
    //
    if (m_hFile == INVALID_HANDLE_VALUE || m_mode==LVOM_READ )
        return LVERR_FAIL;
    lvpos_t oldpos = 0;
    if (!Tell(&oldpos))
        return LVERR_FAIL;
    if (!Seek(size, LVSEEK_SET, NULL))
        return LVERR_FAIL;
    SetEndOfFile( m_hFile);
    Seek(oldpos, LVSEEK_SET, NULL);
    return LVERR_OK;
#else
    if (m_fd == -1)
        return LVERR_FAIL;
    lvpos_t oldpos = 0;
    if (!Tell(&oldpos))
        return LVERR_FAIL;
    if (!Seek(size, LVSEEK_SET, NULL))
        return LVERR_FAIL;
    Seek(oldpos, LVSEEK_SET, NULL);
    return LVERR_OK;
#endif
}

lverror_t LVFileStream::Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten)
{
#ifdef _WIN32
    if (m_hFile == INVALID_HANDLE_VALUE || m_mode==LVOM_READ )
        return LVERR_FAIL;
    //
    lUInt32 dwBytesWritten = 0;
    if (WriteFile( m_hFile, buf, (lUInt32)count, (LPDWORD)&dwBytesWritten, NULL )) {
        if (nBytesWritten)
            *nBytesWritten = dwBytesWritten;
        m_pos += dwBytesWritten;
        if ( m_size < m_pos )
            m_size = m_pos;
        handleAutoSync(dwBytesWritten);
        return LVERR_OK;
    }
    if (nBytesWritten)
        *nBytesWritten = 0;
    return LVERR_FAIL;
    
#else
    if (m_fd == -1)
        return LVERR_FAIL;
    ssize_t res = write( m_fd, buf, count );
    if ( res!=(ssize_t)-1 ) {
        if (nBytesWritten)
            *nBytesWritten = res;
        m_pos += res;
        if ( m_size < m_pos )
            m_size = m_pos;
        handleAutoSync(res);
        return LVERR_OK;
    }
    if (nBytesWritten)
        *nBytesWritten = 0;
    return LVERR_FAIL;
#endif
}

lverror_t LVFileStream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos)
{
#ifdef _WIN32
    //fprintf(stderr, "Seek(%d,%d)\n", offset, origin);
    if (m_hFile == INVALID_HANDLE_VALUE)
        return LVERR_FAIL;
    lUInt32 pos_low = (lUInt32)((__int64)offset & 0xFFFFFFFF);
    LONG pos_high = (LONG)(((__int64)offset >> 32) & 0xFFFFFFFF);
    lUInt32 m=0;
    switch (origin) {
        case LVSEEK_SET:
            m = FILE_BEGIN;
            break;
        case LVSEEK_CUR:
            m = FILE_CURRENT;
            break;
        case LVSEEK_END:
            m = FILE_END;
            break;
    }
    
    pos_low = SetFilePointer(m_hFile, pos_low, &pos_high, m );
    lUInt32 err;
    if (pos_low == INVALID_SET_FILE_POINTER && (err = GetLastError())!=ERROR_SUCCESS ) {
        //if (err == ERROR_NOACCESS)
        //    pos_low = (lUInt32)offset;
        //else if ( err != ERROR_SUCCESS)
        return LVERR_FAIL;
    }
    m_pos = pos_low
        #if LVLONG_FILE_SUPPORT
            | ((lvpos_t)pos_high<<32)
        #endif
            ;
    if (pNewPos)
        *pNewPos = m_pos;
    return LVERR_OK;
#else
    if (m_fd == -1)
        return LVERR_FAIL;
    //
    lvpos_t res = (lvpos_t)-1;
    switch ( origin )
    {
        case LVSEEK_SET:
            res = cr3_lseek( m_fd, offset, SEEK_SET );
            break;
        case LVSEEK_CUR:
            res = cr3_lseek( m_fd, offset, SEEK_CUR );
            break;
        case LVSEEK_END:
            res = cr3_lseek( m_fd, offset, SEEK_END );
            break;
    }
    if (res!=(lvpos_t)-1)
    {
        m_pos = res;
        if ( pNewPos )
            * pNewPos = res;
        return LVERR_OK;
    }
    CRLog::error("error setting file position to %d (%d)", (int)offset, (int)origin );
    return LVERR_FAIL;
#endif
}

lverror_t LVFileStream::Close()
{
#if defined(_WIN32)
    if (m_hFile == INVALID_HANDLE_VALUE)
        return LVERR_FAIL;
    CloseHandle( m_hFile );
    m_hFile = INVALID_HANDLE_VALUE;
#else
    if ( m_fd!= -1 ) {
        close(m_fd);
        m_fd = -1;
    }
#endif
    SetName(NULL);
    return LVERR_OK;
}

LVFileStream *LVFileStream::CreateFileStream(lString32 fname, lvopen_mode_t mode)
{
    LVFileStream * f = new LVFileStream;
    if (f->OpenFile( fname, mode )==LVERR_OK) {
        return f;
    } else {
        delete f;
        return NULL;
    }
}

lverror_t LVFileStream::OpenFile(lString32 fname, int mode)
{
    mode = mode & LVOM_MASK;
#if defined(_WIN32)
    lUInt32 m = 0;
    lUInt32 s = 0;
    lUInt32 c = 0;
    SetName(fname.c_str());
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
            s |= FILE_SHARE_WRITE|FILE_SHARE_READ;
            c |= OPEN_ALWAYS;
            break;
        case LVOM_CLOSED:
        case LVOM_ERROR:
            crFatalError();
            break;
    }
    lString16 fn16 = UnicodeToUtf16(fname);
    m_hFile = CreateFileW( fn16.c_str(), m, s, NULL, c, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE || !m_hFile) {
        // unicode not implemented?
        lUInt32 err = GetLastError();
        if (err==ERROR_CALL_NOT_IMPLEMENTED)
            m_hFile = CreateFileA( UnicodeToLocal(fname).c_str(), m, s, NULL, c, FILE_ATTRIBUTE_NORMAL, NULL);
        if ( (m_hFile == INVALID_HANDLE_VALUE) || (!m_hFile) ) {
            // error
            return LVERR_FAIL;
        }
    }
    
    // set file size and position
    m_mode = (lvopen_mode_t)mode;
    lUInt32 hw=0;
    m_size = GetFileSize( m_hFile, (LPDWORD)&hw );
#if LVLONG_FILE_SUPPORT
    if (hw)
        m_size |= (((lvsize_t)hw)<<32);
#endif
    m_pos = 0;
    
    // set filename
    SetName( fname.c_str() );
    
    // move to end of file
    if (mode==LVOM_APPEND)
        Seek( 0, LVSEEK_END, NULL );
#else
    bool use_sync = (mode & LVOM_FLAG_SYNC)!=0;
    m_fd = -1;
    
    int flags = (mode==LVOM_READ) ? O_RDONLY : O_RDWR | O_CREAT | (use_sync ? O_SYNC : 0) | (mode==LVOM_WRITE ? O_TRUNC : 0);
    lString8 fn8 = UnicodeToUtf8(fname);
    m_fd = open( fn8.c_str(), flags, (mode_t)0666);
    if (m_fd == -1) {
#ifndef ANDROID
        CRLog::error( "Error opening file %s for %s", fn8.c_str(), (mode==LVOM_READ) ? "reading" : "read/write" );
        //CRLog::error( "Error opening file %s for %s, errno=%d, msg=%s", fn8.c_str(), (mode==LVOM_READ) ? "reading" : "read/write",  (int)errno, strerror(errno) );
#endif
        return LVERR_FAIL;
    }
    struct stat stat;
    if ( fstat( m_fd, &stat ) ) {
        CRLog::error( "Cannot get file size for %s", fn8.c_str() );
        return LVERR_FAIL;
    }
    m_mode = (lvopen_mode_t)mode;
    m_size = (lvsize_t) stat.st_size;
#endif
    
    SetName(fname.c_str());
    return LVERR_OK;
}

LVFileStream::LVFileStream() :
    #if defined(_WIN32)
    m_hFile(INVALID_HANDLE_VALUE),
    #else
    m_fd(-1),
    #endif
    //m_parent(NULL),
    m_size(0), m_pos(0)
{
}

LVFileStream::~LVFileStream()
{
    Close();
    //m_parent = NULL;
}

#endif
