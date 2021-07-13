/*******************************************************

   CoolReader Engine

   lvdirectorycontainer.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvdirectorycontainer.h"
#include "lvstreamutils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

LVStreamRef LVDirectoryContainer::OpenStream(const char32_t *fname, lvopen_mode_t mode)
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
    // make filename
    lString32 fn = m_fname;
    fn << fname;
    //const char * fb8 = UnicodeToUtf8( fn ).c_str();
    //printf("Opening directory container file %s : %s fname=%s path=%s\n", UnicodeToUtf8( lString32(fname) ).c_str(), UnicodeToUtf8( fn ).c_str(), UnicodeToUtf8( m_fname ).c_str(), UnicodeToUtf8( m_path ).c_str());
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

lverror_t LVDirectoryContainer::GetSize(lvsize_t *pSize)
{
    if (m_fname.empty())
        return LVERR_FAIL;
    *pSize = GetObjectCount();
    return LVERR_OK;
}

LVDirectoryContainer::LVDirectoryContainer() : m_parent(NULL)
{
}

LVDirectoryContainer::~LVDirectoryContainer()
{
    SetName(NULL);
    Clear();
}

LVDirectoryContainer *LVDirectoryContainer::OpenDirectory(const char32_t *path, const char32_t *mask)
{
    if (!path || !path[0])
        return NULL;
    
    
    // container object
    LVDirectoryContainer * dir = new LVDirectoryContainer;
    
    // make filename
    lString32 fn( path );
    lChar32 lastch = 0;
    if ( !fn.empty() )
        lastch = fn[fn.length()-1];
    if ( lastch!='\\' && lastch!='/' )
        fn << dir->m_path_separator;
    
    dir->SetName(fn.c_str());
    
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    // WIN32 API
    fn << mask;
    WIN32_FIND_DATAW data = { 0 };
    WIN32_FIND_DATAA dataa = { 0 };
    //lString8 bs = DOMString(path).ToAnsiString();
    lString16 fn16 = UnicodeToUtf16(fn);
    HANDLE hFind = FindFirstFileW(fn16.c_str(), &data);
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
        for (;;) {
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
                    item->m_name = Utf16ToUnicode(pfn);
                    item->m_is_container = true;
                    dir->Add(item);
                }
            } else {
                // file
                LVDirectoryContainerItemInfo * item = new LVDirectoryContainerItemInfo;
                item->m_name = Utf16ToUnicode(pfn);
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
        for (;;) {
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
    lString32 p( fn );
    p.erase( p.length()-1, 1 );
    lString8 p8 = UnicodeToLocal( p );
    if ( p8.empty() )
        p8 = ".";
    const char * p8s = p8.c_str();
    DIR * d = opendir(p8s);
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
    } else {
        delete dir;
        return NULL;
    }
    
    
#endif
    return dir;
}
