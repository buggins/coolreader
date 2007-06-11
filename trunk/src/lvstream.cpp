/*******************************************************

   CoolReader Engine

   lvstream.h:  stream classes interface

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstream.h"
#include "../include/lvptrvec.h"
#include <stdio.h>

#if (USE_ZLIB==1)
#include <zlib.h>
#endif

#if !defined(__SYMBIAN32__) && defined(_WIN32)
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifdef _LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef USE_ANSI_FILES

#if !defined(__SYMBIAN32__) && defined(_WIN32)
#define USE_ANSI_FILES 0
#else
#define USE_ANSI_FILES 1
#endif

#endif

// LVStorageObject stubs
const lChar16 * LVStorageObject::GetName()
{
    return NULL;
}

LVContainer * LVStorageObject::GetParentContainer()
{
    return NULL;
}

void LVStorageObject::SetName(const lChar16 * name)
{
}

bool LVStorageObject::IsContainer()
{ 
    return false; 
}

lvsize_t LVStorageObject::GetSize( )
{
    lvsize_t sz;
    if ( GetSize( &sz )!=LVERR_OK )
        return LV_INVALID_SIZE;
    return sz;
}


/// returns stream/container name, may be NULL if unknown
const lChar16 * LVNamedStream::GetName()
{
    if (m_fname.empty())
        return NULL;
    return m_fname.c_str();
}
/// sets stream/container name, may be not implemented for some objects
void LVNamedStream::SetName(const lChar16 * name)
{
    m_fname = name;
    m_filename.clear();
    m_path.clear();
    if (m_fname.empty())
        return;
    const lChar16 * fn = m_fname.c_str();

    const lChar16 * p = fn + m_fname.length() - 1;
    for ( ;p>fn; p--) {
        if (p[-1] == '/' || p[-1]=='\\')
            break;
    }
    int pos = p-fn;
    if (p>fn)
        m_path = m_fname.substr(0, pos);
    m_filename = m_fname.substr(pos, m_fname.length() - pos);
}


#if (USE_ANSI_FILES==1)

class LVFileStream : public LVNamedStream
{
private:
    FILE * m_file;
public:
    
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos )
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
       return LVERR_FAIL;
    }
    virtual lverror_t SetSize( lvsize_t size )
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
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead )
    {
        lvsize_t sz = fread( buf, 1, count, m_file );
        if (nBytesRead)
            *nBytesRead = sz;
        if (sz < count)
        {
            return LVERR_FAIL;
        }
        return LVERR_OK;
    }
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten )
    {
        lvsize_t sz = fwrite( buf, 1, count, m_file );
        if (nBytesWritten)
            *nBytesWritten = sz;
        if (sz < count)
        {
            return LVERR_FAIL;
        }
        return LVERR_OK;
    }
    virtual bool Eof()
    {
        return feof(m_file)!=0;
    }
    static LVFileStream * CreateFileStream( lString16 fname, lvopen_mode_t mode )
    {
        LVFileStream * f = new LVFileStream;
        if (f->OpenFile( fname, mode )==LVERR_OK) {
            return f;
        } else {
            delete f;
            return NULL;
        }
    }
    lverror_t OpenFile( lString16 fname, lvopen_mode_t mode )
    {
        m_mode = mode;
        m_file = NULL;
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
    LVFileStream() : m_file(NULL) 
    { 
        m_mode=LVOM_ERROR; 
    }
    virtual ~LVFileStream()
    { 
        if (m_file)
            fclose(m_file);
    }
};

#else

class LVFileStream : public LVNamedStream
{
    friend class LVDirectoryContainer;
protected:
    HANDLE                 m_hFile;
    LVDirectoryContainer * m_parent;
    lvsize_t               m_size;
    lvpos_t                m_pos;
public:
    virtual bool Eof()
    {
        return m_size<=m_pos;
    }
    virtual LVContainer * GetParentContainer() 
    {  
        return (LVContainer*)m_parent; 
    }
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead )
    {
        //fprintf(stderr, "Read(%08x, %d)\n", buf, count);
        
        if (m_hFile == INVALID_HANDLE_VALUE || m_mode==LVOM_WRITE || m_mode==LVOM_APPEND )
            return LVERR_FAIL;
        //
        lUInt32 dwBytesRead = 0;
        if (ReadFile( m_hFile, buf, (lUInt32)count, &dwBytesRead, NULL )) {
            if (nBytesRead)
                *nBytesRead = dwBytesRead;
            m_pos += dwBytesRead;
        } else {
            return LVERR_FAIL;
        }

        return LVERR_OK;
    }
    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        if (m_hFile == INVALID_HANDLE_VALUE || !pSize)
            return LVERR_FAIL;
        if (m_size<m_pos)
            m_size = m_pos;
        *pSize = m_size;
        return LVERR_OK;
    }
    virtual lvsize_t GetSize()
    {
        if (m_hFile == INVALID_HANDLE_VALUE)
            return 0;
        if (m_size<m_pos)
            m_size = m_pos;
        return m_size;
    }
    virtual lverror_t SetSize( lvsize_t size )
    {
        //
        if (m_hFile == INVALID_HANDLE_VALUE || m_mode==LVOM_READ )
            return LVERR_FAIL;
        lvpos_t oldpos;
        Tell(&oldpos);
        if (!Seek(size, LVSEEK_SET, NULL))
            return LVERR_FAIL;
        SetEndOfFile( m_hFile);
        Seek(oldpos, LVSEEK_SET, NULL);
        return LVERR_OK;
    }
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten )
    {
        if (m_hFile == INVALID_HANDLE_VALUE || m_mode==LVOM_READ )
            return LVERR_FAIL;
        //
        lUInt32 dwBytesWritten = 0;
        if (WriteFile( m_hFile, buf, (lUInt32)count, &dwBytesWritten, NULL )) {
            if (nBytesWritten)
                *nBytesWritten = dwBytesWritten;
            m_pos += dwBytesWritten;
        } else {
            return LVERR_FAIL;
        }

        return LVERR_OK;
    }
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos )
    {
        //fprintf(stderr, "Seek(%d,%d)\n", offset, origin);
        if (m_hFile == INVALID_HANDLE_VALUE)
            return LVERR_FAIL;
        lUInt32 pos_low = (lUInt32)((__int64)offset & 0xFFFFFFFF);
        long pos_high = (long)(((__int64)offset >> 32) & 0xFFFFFFFF);
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
        if (pos_low == 0xFFFFFFFF) {
            lUInt32 err = GetLastError();
            if (err == ERROR_NOACCESS)
                pos_low = (lUInt32)offset;
            else if ( err != ERROR_SUCCESS)
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
    }
    lverror_t Close()
    {
        if (m_hFile == INVALID_HANDLE_VALUE)
            return LVERR_FAIL;
        CloseHandle( m_hFile );
        m_hFile = INVALID_HANDLE_VALUE;
        SetName(NULL);
        return LVERR_OK;
    }
    static LVFileStream * CreateFileStream( lString16 fname, lvopen_mode_t mode )
    {
        LVFileStream * f = new LVFileStream;
        if (f->OpenFile( fname, mode )==LVERR_OK) {
            return f;
        } else {
            delete f;
            return NULL;
        }
    }
    lverror_t OpenFile( lString16 fname, lvopen_mode_t mode )
    {
        lUInt32 m = 0;
        lUInt32 s = 0;
        lUInt32 c = 0;
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
            m |= GENERIC_WRITE;
            s |= FILE_SHARE_WRITE;
            c |= OPEN_ALWAYS;
            break;
        case LVOM_CLOSED:
        case LVOM_ERROR:
            crFatalError();
            break;
        }
        m_hFile = CreateFileW( fname.c_str(), m, s, NULL, c, FILE_ATTRIBUTE_NORMAL, NULL);
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
        m_mode = mode;
        lUInt32 hw=0;
        m_size = GetFileSize( m_hFile, &hw );
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

        return LVERR_OK;
    }
    LVFileStream() : m_hFile(INVALID_HANDLE_VALUE), m_parent(NULL), m_size(0), m_pos(0) 
    {
    }
    virtual ~LVFileStream()
    {
        Close();
        m_parent = NULL;
    }
};
#endif

// facility functions
LVStreamRef LVOpenFileStream( const lChar16 * pathname, lvopen_mode_t mode )
{
    lString16 fn(pathname);
    LVFileStream * stream = stream->CreateFileStream( fn, mode );
    if ( stream!=NULL )
    {
        return LVStreamRef( stream );
    }
    return LVStreamRef();
}

LVStreamRef LVOpenFileStream( const lChar8 * pathname, lvopen_mode_t mode )
{
    lString16 fn = LocalToUnicode(lString8(pathname));
    return LVOpenFileStream( fn.c_str(), mode );
}


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


class LVDirectoryContainerItemInfo : public LVCommonContainerItemInfo
{
    friend class LVDirectoryContainer;
};

class LVDirectoryContainer : public LVNamedContainer
{
protected:
    LVDirectoryContainer * m_parent;
public:
    virtual LVStreamRef OpenStream( const wchar_t * fname, lvopen_mode_t mode )
    {
        int found_index = -1;
        for (int i=0; i<m_list.length(); i++) {
            if ( !lStr_cmp( fname, m_list[i]->GetName() ) ) {
                if ( m_list[i]->IsContainer() ) {
                    // found directory with same name!!!
                    return NULL;
                }
                found_index = i;
                break;
            }
        }
        // make filename
        lString16 fn = m_fname;
        fn << m_path_separator << fname;
        LVStreamRef stream( LVOpenFileStream( fn.c_str(), mode ) );
        if (!stream) {
            return stream;
        }
        //stream->m_parent = this;
        if (found_index<0) {
            // add new info
            LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
            item->m_name = fname;
            stream->GetSize(&item->m_size);
            Add(item);
        }
        return stream;
    }    
    virtual LVContainer * GetParentContainer() 
    {  
        return (LVContainer*)m_parent; 
    }
    virtual const LVContainerItemInfo * GetObjectInfo(int index)
    {
        if (index>=0 && index<m_list.length())
            return m_list[index];
        return NULL;
    }
    virtual int GetObjectCount() const 
    {
        return m_list.length();
    }
    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        if (m_fname.empty())
            return LVERR_FAIL;
        *pSize = GetObjectCount();
        return LVERR_OK;
    }
    LVDirectoryContainer() : m_parent(NULL)
    {
    }
    virtual ~LVDirectoryContainer()
    {
        SetName(NULL);
        Clear();
    }
    static LVDirectoryContainer * OpenDirectory( const wchar_t * path, wchar_t * mask = L"*.*" )
    {
        if (!path || !path[0])
            return NULL;

        // container object
        LVDirectoryContainer * dir = new LVDirectoryContainer;

        dir->SetName(path);
        
#if !defined(__SYMBIAN32__) && defined(_WIN32)
        // WIN32 API
        lString16 fn(path);
        if (fn[fn.length()-1]!=dir->m_path_separator)
            fn << dir->m_path_separator;
        fn << mask;
        WIN32_FIND_DATAW data;
        WIN32_FIND_DATAA dataa;
        memset(&data, 0, sizeof(data));
        memset(&dataa, 0, sizeof(dataa));
        //lString8 bs = DOMString(path).ToAnsiString();
        HANDLE hFind = FindFirstFileW(fn.c_str(), &data);
        bool unicode=true;
        if (hFind == INVALID_HANDLE_VALUE || !hFind) {
            lUInt32 err=GetLastError();
            if (err == ERROR_CALL_NOT_IMPLEMENTED) {
                hFind = FindFirstFileA(UnicodeToLocal(fn).c_str(), &dataa);
                unicode=false;
                if (hFind == INVALID_HANDLE_VALUE || !hFind)
                {
                    delete dir;
                    return NULL;
                }
            } else {
                delete dir;
                return NULL;
            }
        }

        if (unicode) {
            // unicode
            while (1) {
                lUInt32 dwAttrs = data.dwFileAttributes;
                wchar_t * pfn = data.cFileName;
                for (int i=0; data.cFileName[i]; i++) {
                    if (data.cFileName[i]=='/' || data.cFileName[i]=='\\')
                        pfn = data.cFileName + i + 1;
                }

                if ( (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) ) {
                    // directory
                    if (!lStr_cmp(pfn, L"..") || !lStr_cmp(pfn, L".")) {
                        // .. or .
                    } else {
                        // normal directory
                        LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                        item->m_name = pfn;
                        item->m_is_container = true;
                        dir->Add(item);
                    }
                } else {
                    // file
                    LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                    item->m_name = pfn;
                    item->m_size = data.nFileSizeLow;
                    item->m_flags = data.dwFileAttributes;
                    dir->Add(item);
                }

                if (!FindNextFileW(hFind, &data)) {
                    // end of list
                    break;
                }

            }
        } else {
            // ANSI
            while (1) {
                lUInt32 dwAttrs = dataa.dwFileAttributes;
                char * pfn = dataa.cFileName;
                for (int i=0; dataa.cFileName[i]; i++) {
                    if (dataa.cFileName[i]=='/' || dataa.cFileName[i]=='\\')
                        pfn = dataa.cFileName + i + 1;
                }

                if ( (dwAttrs & FILE_ATTRIBUTE_DIRECTORY) ) {
                    // directory
                    if (!strcmp(pfn, "..") || !strcmp(pfn, ".")) {
                        // .. or .
                    } else {
                        // normal directory
                        LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                        item->m_name = LocalToUnicode( lString8( pfn ) );
                        item->m_is_container = true;
                        dir->Add(item);
                    }
                } else {
                    // file
                    LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                    item->m_name = LocalToUnicode( lString8( pfn ) );
                    item->m_size = data.nFileSizeLow;
                    item->m_flags = data.dwFileAttributes;
                    dir->Add(item);
                }

                if (!FindNextFileA(hFind, &dataa)) {
                    // end of list
                    break;
                }

            }
        }
        
        FindClose( hFind );
#else
        // POSIX
        lString16 p( path );
        lString8 p8 = UnicodeToLocal( p );
        DIR * d = opendir(p8.c_str());
        if ( d ) {
            struct dirent * pde;
            while ( (pde = readdir(d))!=NULL ) {
                lString8 fpath = p8 + "/" + pde->d_name;
                struct stat st;
                stat( fpath.c_str(), &st );
                if ( S_ISDIR(st.st_mode) ) {
                    // dir
                    if ( strcmp(pde->d_name, ".") && strcmp(pde->d_name, "..") ) {
                        // normal directory
                        LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                        item->m_name = LocalToUnicode(lString8(pde->d_name));
                        item->m_is_container = true;
                        dir->Add(item);
                    }
                } else if ( S_ISREG(st.st_mode) ) {
                    // file
                    LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                    item->m_name = LocalToUnicode(lString8(pde->d_name));
                    item->m_size = st.st_size;
                    item->m_flags = st.st_mode;
                    dir->Add(item);
                }
            }
            closedir(d);
        }

        
#endif
        return dir;
    }
};

class LVCachedStream : public LVNamedStream
{
private:

    #define CACHE_BUF_BLOCK_SHIFT 12
    #define CACHE_BUF_BLOCK_SIZE (1<<CACHE_BUF_BLOCK_SHIFT)
    class BufItem
    {
    public:
        lUInt32   start;
        lUInt32   size;
        BufItem * prev;
        BufItem * next;
        lUInt8    buf[CACHE_BUF_BLOCK_SIZE];
        
        int getIndex() { return start >> CACHE_BUF_BLOCK_SHIFT; }
        BufItem() : prev(NULL), next(NULL) { }
    };

    LVStreamRef m_stream;
    int m_bufSize;
    lvsize_t    m_size;
    lvpos_t     m_pos;
    BufItem * * m_buf;
    BufItem *   m_head;
    BufItem *   m_tail;
    int         m_bufItems;
    int         m_bufLen;

    /// add item to head
    BufItem * addNewItem( int start )
    {
        //
        int index = (start >> CACHE_BUF_BLOCK_SHIFT);
        BufItem * item = new BufItem();
        if (!m_head)
        {
            m_head = m_tail = item;
        }
        else
        {
            item->next = m_head;
            m_head->prev = item;
            m_head = item;
        }
        item->start = start;
        int sz = CACHE_BUF_BLOCK_SIZE;
        if ( start + sz > (int)m_size )
            sz = (int)(m_size - start);
        item->size = sz;
        m_buf[ index ] = item;
        m_bufLen++;
        assert( !(m_head && !m_tail) );
        return item;
    }
    /// move item to top
    void moveToTop( int index )
    {
        BufItem * item = m_buf[index];
        if ( !item || m_head == item )
            return;
        if ( m_tail == item )
            m_tail = item->prev;
        if ( item->next )
            item->next->prev = item->prev;
        if ( item->prev )
            item->prev->next = item->next;
        m_head->prev = item;
        item->next = m_head;
        item->prev = NULL;
        m_head = item;
        assert( !(m_head && !m_tail) );
    }
    /// reuse existing item from tail of list
    BufItem * reuseItem( int start )
    {
        //
        int rem_index = m_tail->start >> CACHE_BUF_BLOCK_SHIFT;
        if (m_tail->prev)
            m_tail->prev->next = NULL;
        m_tail = m_tail->prev;
        BufItem * item = m_buf[rem_index];
        m_buf[ rem_index ] = NULL;
        int index = (start >> CACHE_BUF_BLOCK_SHIFT);
        m_buf[ index ] = item;
        item->start = start;
        int sz = CACHE_BUF_BLOCK_SIZE;
        if ( start + sz > (int)m_size )
            sz = (int)(m_size - start);
        item->size = sz;
        item->next = m_head;
        item->prev = NULL;
        m_head->prev = item;
        m_head = item;
        assert( !(m_head && !m_tail) );
        return item;
    }
    /// read item content from base stream
    bool fillItem( BufItem * item )
    {
        if ( m_stream->SetPos( item->start )==(lvpos_t)(~0) )
            return false;
            
        lvsize_t bytesRead;
        if ( m_stream->Read( item->buf, item->size, &bytesRead )!=LVERR_OK || bytesRead!=item->size )
            return false;
        return true;
    }
    BufItem * addOrReuseItem( int start )
    {
        //assert( !(m_head && !m_tail) );
        if ( m_bufLen < m_bufSize )
            return addNewItem( start );
        else
            return reuseItem( start );
    }
    /// checks several items, loads if necessary
    bool fillFragment( int startIndex, int count )
    {
        if (count<=0 || startIndex<0 || startIndex+count>m_bufItems)
        {
            return false;
        }
        int firstne = -1;
        int lastne = -1;
        int i;
        for ( i=startIndex; i<startIndex+count; i++)
        {
            if ( m_buf[i] )
            {
                moveToTop( i );
            }
            else
            {
                if (firstne == -1)
                    firstne = i;
                lastne = i;
            }
        }
        if ( firstne<0 )
            return true;
        for ( i=firstne; i<=lastne; i++)
        {
            if ( !m_buf[i] )
            {
                BufItem * item = addOrReuseItem( i << CACHE_BUF_BLOCK_SHIFT );
                if ( !fillItem ( item ) )
                    return false;
            }
            else
            {
                moveToTop( i );
            }
        }
        return true;
    }
public:

    LVCachedStream( LVStreamRef stream, int bufSize ) : m_stream(stream), m_pos(0), 
            m_head(NULL), m_tail(NULL), m_bufLen(0)
    {
        m_size = m_stream->GetSize();
        m_bufItems = (int)((m_size + CACHE_BUF_BLOCK_SIZE - 1) >> CACHE_BUF_BLOCK_SHIFT);
        if (!m_bufItems)
            m_bufItems = 1;
        m_bufSize = (bufSize + CACHE_BUF_BLOCK_SIZE - 1) >> CACHE_BUF_BLOCK_SHIFT;
        if (m_bufSize<3)
            m_bufSize = 3;
        m_buf = new BufItem* [m_bufItems];
        memset( m_buf, 0, sizeof( BufItem*) * m_bufItems );
    }
    virtual ~LVCachedStream()
    {
        if (m_buf)
        {
            for (int i=0; i<m_bufItems; i++)
                if (m_buf[i])
                    delete m_buf[i];
            delete[] m_buf;
        }
    }
    
    virtual bool Eof()
    {
        return m_pos >= m_size;
    }
    virtual lvsize_t  GetSize()
    { 
        return m_size;
    }

    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos)
    {
        lvpos_t npos = 0;
        lvpos_t currpos = m_pos;
        switch (origin) {
        case LVSEEK_SET:
            npos = offset;
            break;
        case LVSEEK_CUR:
            npos = currpos + offset;
            break;
        case LVSEEK_END:
            npos = m_size + offset;
            break;
        }
        if (npos < 0 || npos > m_size)
            return LVERR_FAIL;
        m_pos = npos;
        if (newPos)
        {
            *newPos =  m_pos;
        }
        return LVERR_OK;
    }
    
    virtual lverror_t Write(const void*, lvsize_t, lvsize_t*)
    {
        return LVERR_NOTIMPL;
    }
    
    virtual lverror_t Read(void* buf, lvsize_t size, lvsize_t* pBytesRead)
    {
        int startIndex = (int)(m_pos >> CACHE_BUF_BLOCK_SHIFT);
        int endIndex = (int)((m_pos + size - 1) >> CACHE_BUF_BLOCK_SHIFT);
        int count = endIndex - startIndex + 1;
        int extraItems = (m_bufSize - count); // max move backward
        if (extraItems<0)
            extraItems = 0;
        char * flags = new char[ count ];
        memset( flags, 0, count );
        
        //if ( m_stream
        int start = (int)m_pos;
        lUInt8 * dst = (lUInt8 *) buf;
        int dstsz = (int)size;
        int i;
        int istart = start & (CACHE_BUF_BLOCK_SIZE - 1);
        for ( i=startIndex; i<=endIndex; i++ )
        {
            BufItem * item = m_buf[i];
            if (item)
            {
                int isz = item->size - istart;
                if (isz > dstsz)
                    isz = dstsz;
                memcpy( dst, item->buf + istart, isz );
                flags[i - startIndex] = 1;
            }
            dstsz -= CACHE_BUF_BLOCK_SIZE - istart;
            dst += CACHE_BUF_BLOCK_SIZE - istart;
            istart = 0;
        }
        
        dst = (lUInt8 *) buf;
        
        bool flgFirstNE = true;
        istart = start & (CACHE_BUF_BLOCK_SIZE - 1);
        dstsz = (int)size;
        for ( i=startIndex; i<=endIndex; i++ )
        {
            if (!flags[ i - startIndex])
            {
                if ( !m_buf[i] )
                {
                    int fillStart = i;
                    if ( flgFirstNE )
                    {
                        fillStart -= extraItems;
                    }
                    if (fillStart<0)
                        fillStart = 0;
                    int fillEnd = fillStart + m_bufSize - 1;
                    if (fillEnd>endIndex)
                        fillEnd = endIndex;
                    bool res = fillFragment( fillStart, fillEnd - fillStart + 1 );
                    if ( !res )
                    {
                        fprintf( stderr, "cannot fill fragment %d .. %d\n", fillStart, fillEnd );
                        exit(-1); 
                    }
                    flgFirstNE = false;
                }
                BufItem * item = m_buf[i];
                int isz = item->size - istart;
                if (isz > dstsz)
                    isz = dstsz;
                memcpy( dst, item->buf + istart, isz );
            }
            dst += CACHE_BUF_BLOCK_SIZE - istart;
            dstsz -= CACHE_BUF_BLOCK_SIZE - istart;
            istart = 0;
        }
        delete[] flags;
       
        lvsize_t bytesRead = size;
        if ( m_pos + size > m_size )
            bytesRead = m_size - m_pos;
        m_pos += bytesRead;
        if (pBytesRead)
            *pBytesRead = bytesRead;
        return LVERR_OK;
    }
    
    virtual lverror_t SetSize(lvsize_t)
    {
        return LVERR_NOTIMPL;
    }
};


#pragma pack(push, 1)
typedef struct {
    lUInt32  Mark;      // 0
    lUInt8   UnpVer;    // 4
    lUInt8   UnpOS;     // 5
    lUInt16  Flags;     // 6
    lUInt16  Method;    // 8
    lUInt32  ftime;     // A
    lUInt32  CRC;       // E
    lUInt32  PackSize;  //12
    lUInt32  UnpSize;   //16
    lUInt16  NameLen;   //1A
    lUInt16  AddLen;    //1C
    void byteOrderConv()
    {
        //
        lvByteOrderConv cnv;
        if ( cnv.msf() )
        {
            cnv.rev( &Mark );
            cnv.rev( &Flags );
            cnv.rev( &Method );
            cnv.rev( &ftime );
            cnv.rev( &CRC );
            cnv.rev( &PackSize );
            cnv.rev( &UnpSize );
            cnv.rev( &NameLen );
            cnv.rev( &AddLen );
        }
    }
} ZipLocalFileHdr;
#pragma pack(pop)

#pragma pack(push, 1)
struct ZipHd2
{
    lUInt32  Mark;      // 0
    lUInt8   PackVer;   // 4
    lUInt8   PackOS;
    lUInt8   UnpVer;
    lUInt8   UnpOS;
    lUInt16     Flags;  // 8
    lUInt16     Method; // A
    lUInt32    ftime;   // C
    lUInt32    CRC;     // 10
    lUInt32    PackSize;// 14
    lUInt32    UnpSize; // 18
    lUInt16     NameLen;// 1C
    lUInt16     AddLen; // 1E
    lUInt16     CommLen;// 20
    lUInt16     DiskNum;// 22
    //lUInt16     ZIPAttr;// 24
    //lUInt32     Attr;   // 26
    //lUInt32     Offset; // 30
    lUInt16     _Attr_and_Offset[5];   // 26
    lUInt16     getZIPAttr() { return _Attr_and_Offset[0]; }
    lUInt32     getAttr() { return _Attr_and_Offset[1] | ((lUInt32)_Attr_and_Offset[2]<<16); }
    lUInt32     getOffset() { return _Attr_and_Offset[3] | ((lUInt32)_Attr_and_Offset[4]<<16); }
    void byteOrderConv()
    {
        //
        lvByteOrderConv cnv;
        if ( cnv.msf() )
        {
            cnv.rev( &Mark );
            cnv.rev( &Flags );
            cnv.rev( &Method );
            cnv.rev( &ftime );
            cnv.rev( &CRC );
            cnv.rev( &PackSize );
            cnv.rev( &UnpSize );
            cnv.rev( &NameLen );
            cnv.rev( &AddLen );
            cnv.rev( &CommLen );
            cnv.rev( &DiskNum );
            cnv.rev( &_Attr_and_Offset[0] );
            cnv.rev( &_Attr_and_Offset[1] );
            cnv.rev( &_Attr_and_Offset[2] );
            cnv.rev( &_Attr_and_Offset[3] );
            cnv.rev( &_Attr_and_Offset[4] );
        }
    }
};
#pragma pack(pop)

#define ARC_INBUF_SIZE  8192
#define ARC_OUTBUF_SIZE 16384

#if (USE_ZLIB==1)

class LVZipDecodeStream : public LVNamedStream
{
private:
    LVStreamRef m_stream;
    lvsize_t    m_start;
    lvsize_t    m_packsize;
    lvsize_t    m_unpacksize;
    z_stream_s  m_zstream;
    lvpos_t     m_inbytesleft; // bytes left
    lvpos_t     m_outbytesleft;
    bool        m_zInitialized;
    int         m_decodedpos;
    lUInt8 *    m_inbuf;
    lUInt8 *    m_outbuf;
    lUInt32     m_CRC;
    lUInt32     m_originalCRC;

    
    LVZipDecodeStream( LVStreamRef stream, lvsize_t start, lvsize_t packsize, lvsize_t unpacksize, lUInt32 crc )
        : m_stream(stream), m_start(start), m_packsize(packsize), m_unpacksize(unpacksize), 
        m_inbytesleft(0), m_outbytesleft(0), m_zInitialized(false), m_decodedpos(0), 
        m_inbuf(NULL), m_outbuf(NULL), m_CRC(0), m_originalCRC(crc)
    {
        m_inbuf = new lUInt8[ARC_INBUF_SIZE];
        m_outbuf = new lUInt8[ARC_OUTBUF_SIZE];
        rewind();
    }
    
    ~LVZipDecodeStream()
    {
        zUninit();
        if (m_inbuf)
            delete m_inbuf;
        if (m_outbuf)
            delete m_outbuf;
    }
    
    void zUninit()
    {
        if (!m_zInitialized)
            return;
        inflateEnd(&m_zstream);
        m_zInitialized = false;
    }
    
    /// Fill input buffer: returns -1 if fails.
    int fillInBuf()
    {
        if (m_zstream.avail_in < ARC_INBUF_SIZE / 4 && m_inbytesleft > 0)
        {
            int inpos = m_zstream.next_in ? (m_zstream.next_in - m_inbuf) : 0;
            if ( inpos > ARC_INBUF_SIZE/2 )
            {
                // move rest of data to beginning of buffer
                for ( int i=0; i<(int)m_zstream.avail_in; i++)
                    m_inbuf[i] = m_inbuf[ i+inpos ];
                m_zstream.next_in = m_inbuf;
                inpos = 0;
            }
            int tailpos = inpos + m_zstream.avail_in;
            int bytes_to_read = ARC_INBUF_SIZE - tailpos;
            if ( bytes_to_read > (int)m_inbytesleft )
                bytes_to_read = (int)m_inbytesleft;
            if (bytes_to_read > 0)
            {
                lvsize_t bytesRead = 0;
                if ( m_stream->Read( m_inbuf + tailpos, bytes_to_read, &bytesRead ) != LVERR_OK )
                {
                    // read error
                    m_zstream.avail_in = 0;
                    return -1;
                }
                m_CRC = crc32( m_CRC, m_inbuf + tailpos, (int)(bytesRead) );
                m_zstream.avail_in += (int)bytesRead;
                m_inbytesleft -= bytesRead;
            }
            else
            {
                //check CRC
                if ( m_CRC != m_originalCRC )
                    return -1; // CRC error
            }
        }
        return m_zstream.avail_in;
    }

    bool rewind()
    {
        zUninit();
        // stream
        m_stream->SetPos( 0 );
        
        m_CRC = 0;
        memset( &m_zstream, 0, sizeof(m_zstream) );
        // inbuf
        m_inbytesleft = m_packsize;
        m_zstream.next_in = m_inbuf;
        fillInBuf();
        // outbuf
        m_zstream.next_out = m_outbuf;
        m_zstream.avail_out = ARC_OUTBUF_SIZE;
        m_decodedpos = 0;
        m_outbytesleft = m_unpacksize;
        // Z
        if ( inflateInit2( &m_zstream, -15 ) != Z_OK )
        {
            return false;
        }
        return true;
    }
    // returns count of available decoded bytes in buffer
    inline int getAvailBytes()
    {
        return m_zstream.next_out - m_outbuf - m_decodedpos;
    }
    /// decode next portion of data, returns number of decoded bytes available, -1 if error
    int decodeNext()
    {
        int avail = getAvailBytes();
        if (avail>0)
            return avail;
        // fill in buffer
        int in_bytes = fillInBuf();
        if (in_bytes<0)
            return -1;
        // reserve space for output
        if (m_zstream.avail_out < ARC_OUTBUF_SIZE / 4 && m_outbytesleft > 0)
        {
            
            int outpos = m_zstream.next_out - m_outbuf;
            if ( outpos > ARC_OUTBUF_SIZE*3/4 )
            {
                // move rest of data to beginning of buffer
                for ( int i=(int)m_decodedpos; i<outpos; i++)
                    m_inbuf[i - m_decodedpos] = m_inbuf[ i ];
                m_zstream.next_out -= m_decodedpos;
                outpos -= m_decodedpos;
                m_decodedpos = 0;
                m_zstream.avail_out = ARC_OUTBUF_SIZE - outpos;
            }
        }
        int res = inflate( &m_zstream, m_inbytesleft > 0 ? Z_NO_FLUSH : Z_FINISH );
        if (res == Z_STREAM_ERROR)
        {
            return -1;
        }
        avail = getAvailBytes();
        return avail;
    }
    /// skip bytes from out stream
    bool skip( int bytesToSkip )
    {
        while (bytesToSkip > 0)
        {
            int avail = decodeNext();
            
            if (avail < 0)
            {
                return false; // error
            }
            else if (avail==0)
            {
                return true;
            }

            if (avail >= bytesToSkip)
                avail = bytesToSkip;

            m_decodedpos += avail;
            m_outbytesleft -= avail;
            bytesToSkip -= avail;
        }
        if (bytesToSkip == 0)
            return true;
        return false;
    }
    /// decode bytes
    int read( lUInt8 * buf, int bytesToRead )
    {
        int bytesRead = 0;
        //
        while (bytesToRead > 0)
        {
            int avail = decodeNext();
            
            if (avail < 0)
            {
                return -1; // error
            }
            else if (avail==0)
            {
                return bytesRead;
            }

            if (avail >= bytesToRead)
                avail = bytesToRead;

            // copy data
            lUInt8 * src = m_outbuf + m_decodedpos;
            for (int i=avail; i>0; --i)
                *buf++ = *src++;
                
            m_decodedpos += avail;
            m_outbytesleft -= avail;
            bytesRead += avail;
            bytesToRead -= avail;
        }
        return bytesRead;
    }
public:
    virtual bool Eof()
    {
        return m_outbytesleft==0; //m_pos >= m_size;
    }
    virtual lvsize_t  GetSize()
    { 
        return m_unpacksize;
    }
    virtual lvpos_t GetPos()
    {
        return m_unpacksize - m_outbytesleft;
    }
    virtual lverror_t GetPos( lvpos_t * pos )
    {
        if (pos)
            *pos = m_unpacksize - m_outbytesleft;
        return LVERR_OK;
    }
    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos)
    {
        lvpos_t npos = 0;
        lvpos_t currpos = GetPos();
        switch (origin) {
        case LVSEEK_SET:
            npos = offset;
            break;
        case LVSEEK_CUR:
            npos = currpos + offset;
            break;
        case LVSEEK_END:
            npos = m_unpacksize + offset;
            break;
        }
        if (npos < 0 || npos > m_unpacksize)
            return LVERR_FAIL;
        if ( npos != currpos )
        {
            if (npos < currpos)
            {
                if ( !rewind() || !skip((int)npos) )
                    return LVERR_FAIL;
            }
            else
            {
                skip( (int)(npos - currpos) );
            }
        }
        if (newPos)
            *newPos = npos;                
        return LVERR_OK;
    }
    virtual lverror_t Write(const void*, lvsize_t, lvsize_t*)
    {
        return LVERR_NOTIMPL;
    }
    virtual lverror_t Read(void* buf, lvsize_t count, lvsize_t* bytesRead)
    {
        int readBytes = read( (lUInt8 *)buf, (int)count );
        if ( readBytes<0 )
            return LVERR_FAIL;
        if (bytesRead)
            *bytesRead = (lvsize_t)readBytes;
        return LVERR_OK;
    }
    virtual lverror_t SetSize(lvsize_t)
    {
        return LVERR_NOTIMPL;
    }
    static LVStream * Create( LVStreamRef stream, lvpos_t pos, lString16 name )
    {
        ZipLocalFileHdr hdr;
        if ( stream->Seek( pos, LVSEEK_SET, NULL )!=LVERR_OK )
            return NULL;
        lvsize_t sz = 0;
        if ( stream->Read( &hdr, sizeof(hdr), &sz)!=LVERR_OK || sz!=sizeof(hdr) )
            return NULL;
        hdr.byteOrderConv();
        pos += 0x1e + hdr.NameLen + hdr.AddLen;
        if ( stream->Seek( pos, LVSEEK_SET, NULL )!=LVERR_OK )
            return NULL;
        if ((lvpos_t)(pos + hdr.PackSize) > (lvpos_t)stream->GetSize())
            return NULL;
        if (hdr.Method == 0)
        {
            // store method, copy as is
            if ( hdr.PackSize != hdr.UnpSize )
                return NULL;
            LVStreamFragment * fragment = new LVStreamFragment( stream, pos, hdr.PackSize);
            fragment->SetName( name.c_str() );
            return fragment;
        }
        else if (hdr.Method == 8)
        {
            // deflate
            LVStreamRef srcStream( new LVStreamFragment( stream, pos, hdr.PackSize) );
            LVZipDecodeStream * res = new LVZipDecodeStream( srcStream, pos, 
                hdr.PackSize, hdr.UnpSize, hdr.CRC );
            res->SetName( name.c_str() );
            return res;
        }
        else
            return NULL;
    }
};

class LVZipArc : public LVArcContainerBase
{
public:
    virtual LVStreamRef OpenStream( const wchar_t * fname, lvopen_mode_t mode )
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
        // make filename
        lString16 fn = fname;
        LVStreamRef strm = m_stream; // fix strange arm-linux-g++ bug
        LVStreamRef stream( 
		LVZipDecodeStream::Create( 
			strm, 
			m_list[found_index]->GetSrcPos(), fn ) );
        if (!stream.isNull()) {
            return LVCreateBufferedStream( stream, ZIP_STREAM_BUFFER_SIZE );
        }
        return stream;
    }    
    LVZipArc( LVStreamRef stream ) : LVArcContainerBase(stream)
    {
    }
    virtual ~LVZipArc()
    {
    }
    
    virtual int ReadContents()
    {
        lvByteOrderConv cnv;
        bool arcComment = false;
        bool truncated = false;

        m_list.clear();

        if (!m_stream || m_stream->Seek(0, LVSEEK_SET, NULL)!=LVERR_OK)
            return 0;

        SetName( m_stream->GetName() );

                
        lvsize_t sz = 0;
        if (m_stream->GetSize( &sz )!=LVERR_OK)
                return 0;
        lvsize_t m_FileSize = (unsigned)sz;

        char ReadBuf[1024];
        lUInt32 NextPosition;
        lvpos_t CurPos;
        lvsize_t ReadSize;
        int Buf;
        bool found = false;
        CurPos=NextPosition=(int)m_FileSize;
        if (CurPos < sizeof(ReadBuf)-18)
            CurPos = 0;
        else
            CurPos -= sizeof(ReadBuf)-18;
        for ( Buf=0; Buf<64 && !found; Buf++ )
        {
            //SetFilePointer(ArcHandle,CurPos,NULL,FILE_BEGIN);
            m_stream->Seek( CurPos, LVSEEK_SET, NULL );
            m_stream->Read( ReadBuf, sizeof(ReadBuf), &ReadSize);
            if (ReadSize==0)
                break;
            for (int I=(int)ReadSize-4;I>=0;I--)
            {
                if (ReadBuf[I]==0x50 && ReadBuf[I+1]==0x4b && ReadBuf[I+2]==0x05 &&
                    ReadBuf[I+3]==0x06)
                {
                    m_stream->Seek( CurPos+I+16, LVSEEK_SET, NULL );
                    m_stream->Read( &NextPosition, sizeof(NextPosition), &ReadSize);
		    		cnv.lsf( &NextPosition );
                    found=true;
                    break;
                }
            }
            if (CurPos==0)
                break;
            if (CurPos<sizeof(ReadBuf)-4)
                CurPos=0;
            else
                CurPos-=sizeof(ReadBuf)-4;
        }

        truncated = !found;
        if (truncated)
            NextPosition=0;
        
        //================================================================
        // get files


        ZipLocalFileHdr ZipHd1;
        ZipHd2 ZipHeader;
        int ZipHeader_size = 0x34; //sizeof(ZipHd2)
          //lUInt32 ReadSize;

        while (1) {

            if (m_stream->Seek( NextPosition, LVSEEK_SET, NULL )!=LVERR_OK)
                return 0;

            if (truncated)
            {
                m_stream->Read( &ZipHd1, sizeof(ZipHd1), &ReadSize);
                ZipHd1.byteOrderConv();

                //ReadSize = fread(&ZipHd1, 1, sizeof(ZipHd1), f);
                if (ReadSize!=sizeof(ZipHd1)) {
                        //fclose(f);
                    if (ReadSize==0 && NextPosition==m_FileSize)
                        return m_list.length();
                    return 0;
                }

                memset(&ZipHeader,0,ZipHeader_size);
                ZipHeader.UnpVer=ZipHd1.UnpVer;
                ZipHeader.UnpOS=ZipHd1.UnpOS;
                ZipHeader.Flags=ZipHd1.Flags;
                ZipHeader.ftime=ZipHd1.ftime;
                ZipHeader.PackSize=ZipHd1.PackSize;
                ZipHeader.UnpSize=ZipHd1.UnpSize;
                ZipHeader.NameLen=ZipHd1.NameLen;
                ZipHeader.AddLen=ZipHd1.AddLen;
                ZipHeader.Method=ZipHd1.Method;
            } else {
                m_stream->Read( &ZipHeader, ZipHeader_size, &ReadSize);
                ZipHeader.byteOrderConv();
                    //ReadSize = fread(&ZipHeader, 1, sizeof(ZipHeader), f);
                if (ReadSize!=ZipHeader_size) {
                            if (ReadSize>16 && ZipHeader.Mark==0x06054B50 ) {
                                    break;
                            }
                            //fclose(f);
                            return 0;
                }
            }

            if (ReadSize==0 || ZipHeader.Mark==0x06054b50 ||
                    truncated && ZipHeader.Mark==0x02014b50)
            {
                if (!truncated && *(lUInt16 *)((char *)&ZipHeader+20)!=0)
                    arcComment=true;
                break; //(GETARC_EOF);
            }
          
            const int NM = 513;
            lUInt32 SizeToRead=(ZipHeader.NameLen<NM) ? ZipHeader.NameLen : NM;
            char fnbuf[1025];
            m_stream->Read( fnbuf, SizeToRead, &ReadSize);

            if (ReadSize!=SizeToRead) {
                return 0;
            }
            
            fnbuf[ZipHeader.NameLen]=0;

            long SeekLen=ZipHeader.AddLen+ZipHeader.CommLen;

            LVCommonContainerItemInfo * item = new LVCommonContainerItemInfo();
  
            if (truncated)
                SeekLen+=ZipHeader.PackSize;

            NextPosition = (lUInt32)m_stream->GetPos();
            NextPosition += SeekLen;
            m_stream->Seek(NextPosition, LVSEEK_SET, NULL);
  
            lString16 fName = LocalToUnicode( lString8(fnbuf) );

            item->SetItemInfo(fName.c_str(), ZipHeader.UnpSize, (ZipHeader.getAttr() & 0x3f));
            item->SetSrc( ZipHeader.getOffset(), ZipHeader.PackSize, ZipHeader.Method );
            m_list.add(item);
        }

        return m_list.length();
    }

    static LVArcContainerBase * OpenArchieve( LVStreamRef stream )
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
        if (hdr[0]!='P' || hdr[1]!='K' || hdr[2]!=3 || hdr[3]!=4)
                return NULL;
        LVZipArc * arc = new LVZipArc( stream );
        int itemCount = arc->ReadContents();
        if ( itemCount <= 0 )
        {
            delete arc;
            return NULL;
        }
        return arc;
    }

};
#endif

class LVMemoryStream : public LVNamedStream
{
protected:
	lUInt8 *               m_pBuffer;
	bool                   m_own_buffer;
	LVContainer *          m_parent;
	lvsize_t               m_size;
	lvsize_t               m_bufsize;
	lvpos_t                m_pos;
	lvopen_mode_t          m_mode;
public:
    /// Check whether end of file is reached
    /**
        \return true if end of file reached
    */
    virtual bool Eof()
    {
        return m_pos>=m_size;
    }
    virtual lvopen_mode_t GetMode() 
    { 
        return m_mode; 
    }
	virtual LVContainer * GetParentContainer()
	{  
		return (LVContainer*)m_parent; 
	}
	virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead )
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
	virtual lverror_t GetSize( lvsize_t * pSize )
	{
		if (!m_pBuffer || !pSize)
			return LVERR_FAIL;
		if (m_size<m_pos)
			m_size = m_pos;
		*pSize = m_size;
		return LVERR_OK;
	}
	// ensure that buffer is at least new_size long
	lverror_t SetBufSize( lvsize_t new_size )
	{
		if (!m_pBuffer || m_mode==LVOM_READ )
			return LVERR_FAIL;
		if (new_size<=m_bufsize)
			return LVERR_OK;
		if (m_own_buffer!=true)
			return LVERR_FAIL; // cannot resize foreign buffer
		//
		int newbufsize = (int)(new_size * 2 + 4096);
		m_pBuffer = (lUInt8*) realloc( m_pBuffer, newbufsize );
		m_bufsize = newbufsize;
		return LVERR_OK;
	}
	virtual lverror_t SetSize( lvsize_t size )
	{
		//
		if (SetBufSize( size )!=LVERR_OK)
			return LVERR_FAIL;
		m_size = size;
		if (m_pos>m_size)
			m_pos = m_size;
		return LVERR_OK;
	}
	virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten )
	{
		if (!m_pBuffer || m_mode==LVOM_READ )
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
	virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos )
	{
		if (!m_pBuffer)
			return LVERR_FAIL;
		lvpos_t newpos = m_pos;
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
		if (newpos<0 || newpos>m_size)
			return LVERR_FAIL;
		m_pos = newpos;
		if (pNewPos)
			*pNewPos = m_pos;
		return LVERR_OK;
	}
	virtual lverror_t Close()
	{
		if (!m_pBuffer)
			return LVERR_FAIL;
		if (m_pBuffer && m_own_buffer)
			delete m_pBuffer;
		m_pBuffer = NULL;
		m_size = 0;
		m_bufsize = 0;
		m_pos = 0;
		return LVERR_OK;
	}
	lverror_t Create( )
	{
		Close();
		m_bufsize = 4096;
		m_size = 0;
		m_pos = 0;
		m_pBuffer = new lUInt8[(int)m_bufsize];
		m_own_buffer = true;
		m_mode = LVOM_READWRITE;
		return LVERR_OK;
	}
	lverror_t CreateCopy( lUInt8 * pBuf, lvsize_t size, lvopen_mode_t mode )
	{
		Close();
		m_bufsize = size;
		m_size = 0;
		m_pos = 0;
		m_pBuffer = new lUInt8[(int)m_bufsize];
		if (m_pBuffer) {
			memcpy( m_pBuffer, pBuf, (int)size );
		}
		m_own_buffer = true;
		m_mode = mode;
		if (mode==LVOM_APPEND)
			m_pos = m_size;
		return LVERR_OK;
	}
	lverror_t Open( lUInt8 * pBuf, lvsize_t size )
	{
		if (!pBuf || size<0)
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
	LVMemoryStream() : m_parent(NULL), m_size(0), m_pos(0), m_own_buffer(false), m_pBuffer(NULL) 
	{
	}
	virtual ~LVMemoryStream()
	{
		Close();
		m_parent = NULL;
	}
};

#if (USE_ZLIB==1)
LVContainerRef LVOpenArchieve( LVStreamRef stream )
{
    LVContainerRef ref;
    if (stream.isNull())
        return ref;
    
    // try ZIP
    ref = LVZipArc::OpenArchieve( stream );
    if (!ref.isNull())
            return ref;
            
    // try RAR: todo
    
    // not found: return null ref
    return ref;
}
#endif

LVStreamRef LVCreateMemoryStream( void * buf, int bufSize, bool createCopy, lvopen_mode_t mode )
{
    LVMemoryStream * stream = new LVMemoryStream();
    if ( !buf )
        stream->Create();
    else if ( createCopy )
        stream->CreateCopy( (lUInt8*)buf, bufSize, mode );
    else
        stream->Open( (lUInt8*)buf, bufSize );
    return LVStreamRef( stream );
}

LVStreamRef LVCreateBufferedStream( LVStreamRef stream, int bufSize )
{
    if ( stream.isNull() || bufSize < 512 )
        return stream;
    return LVStreamRef( new LVCachedStream( stream, bufSize ) );
}

lvsize_t LVPumpStream( LVStreamRef out, LVStreamRef in )
{
    char buf[4096];
    lvsize_t totalBytesRead = 0;
    lvsize_t bytesRead = 0;
    in->SetPos(0);
    while ( in->Read( buf, 4096, &bytesRead )==LVERR_OK && bytesRead>0 )
    {
        out->Write( buf, bytesRead, NULL );
        totalBytesRead += bytesRead;
    }
    return totalBytesRead;
}


LVContainerRef LVOpenDirectory( const wchar_t * path )
{
    LVRef<LVContainer> dir = LVDirectoryContainer::OpenDirectory( path );
    return dir;
}

