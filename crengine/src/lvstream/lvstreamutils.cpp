/*******************************************************

   CoolReader Engine

   lvstreamutils.cpp

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvstreamutils.h"
#include "lvassetcontainerfactory.h"
#include "lvfilestream.h"
#include "lvmemorystream.h"
#include "lvcachedstream.h"
#include "lvdirectorycontainer.h"
#include "lvtcrstream.h"
#include "lvfilemappedstream.h"
#include "lvblockwritestream.h"
#include "crlog.h"

#if (USE_ZLIB==1)
#include "lvziparc.h"
#endif

#if (USE_UNRAR==1)
#include "lvrararc.h"
#endif

#include <stdio.h>

#if !defined(__SYMBIAN32__) && defined(_WIN32)
extern "C" {
#include <windows.h>
}
#else
#include <sys/stat.h>
#include <unistd.h>
#endif


static LVAssetContainerFactory * _assetContainerFactory = NULL;

lString32 LVExtractAssetPath(lString32 fn) {
	if (fn.length() < 2 || fn[0] != ASSET_PATH_PREFIX)
		return lString32();
	if (fn[1] == '/' || fn[1] == '\\')
		return fn.substr(2);
	return fn.substr(1);
}

/// Open memory mapped file
/**
    \param pathname is file name to open (unicode)
    \param mode is mode file should be opened in (LVOM_READ or LVOM_APPEND only)
	\param minSize is minimum file size for R/W mode
    \return reference to opened stream if success, NULL if error
*/
LVStreamRef LVMapFileStream( const lChar8 * pathname, lvopen_mode_t mode, lvsize_t minSize )
{
	lString32 fn = LocalToUnicode( lString8(pathname) );
	return LVMapFileStream( fn.c_str(), mode, minSize );
}

/// tries to split full path name into archive name and file name inside archive using separator "@/" or "@\"
bool LVSplitArcName( lString32 fullPathName, lString32 & arcPathName, lString32 & arcItemPathName )
{
    int p = fullPathName.pos(ASSET_PATH_PREFIX_S "/");
    if ( p<0 )
        p = fullPathName.pos(ASSET_PATH_PREFIX_S "\\");
    if ( p<0 )
        return false;
    arcPathName = fullPathName.substr(0, p);
    arcItemPathName = fullPathName.substr(p + 2);
    return !arcPathName.empty() && !arcItemPathName.empty();
}

/// tries to split full path name into archive name and file name inside archive using separator "@/" or "@\"
bool LVSplitArcName( lString8 fullPathName, lString8 & arcPathName, lString8 & arcItemPathName )
{
    int p = fullPathName.pos(ASSET_PATH_PREFIX_S "/");
    if ( p<0 )
        p = fullPathName.pos(ASSET_PATH_PREFIX_S "\\");
    if ( p<0 )
        return false;
    arcPathName = fullPathName.substr(0, p);
    arcItemPathName = fullPathName.substr(p + 2);
    return !arcPathName.empty() && !arcItemPathName.empty();
}

// facility functions
LVStreamRef LVOpenFileStream( const lChar32 * pathname, int mode )
{
    lString32 fn(pathname);
    if (fn.length() > 1 && fn[0] == ASSET_PATH_PREFIX) {
    	if (!_assetContainerFactory || mode != LVOM_READ)
    		return LVStreamRef();
    	lString32 assetPath = LVExtractAssetPath(fn);
    	return _assetContainerFactory->openAssetStream(assetPath);
    }
#if 0
    //defined(_LINUX) || defined(_WIN32)
    if ( mode==LVOM_READ ) {
        LVFileMappedStream * stream = LVFileMappedStream::CreateFileStream( fn, mode, 0 );
        if ( stream != NULL )
        {
            return LVStreamRef( stream );
        }
        return LVStreamRef();
    }
#endif

    LVFileStream * stream = LVFileStream::CreateFileStream( fn, (lvopen_mode_t)mode );
    if ( stream!=NULL )
    {
        return LVStreamRef( stream );
    }
    return LVStreamRef();
}

LVStreamRef LVOpenFileStream( const lChar8 * pathname, int mode )
{
    lString32 fn = Utf8ToUnicode(lString8(pathname));
    return LVOpenFileStream( fn.c_str(), mode );
}

LVContainerRef LVOpenArchieve( LVStreamRef stream )
{
    LVContainerRef ref;
    if (stream.isNull())
        return ref;

#if (USE_ZLIB==1)
    // try ZIP
    ref = LVZipArc::OpenArchieve( stream );
    if (!ref.isNull())
        return ref;
#endif

#if (USE_UNRAR==1)
    // try RAR
    ref = LVRarArc::OpenArchieve( stream );
    if (!ref.isNull())
        return ref;
#endif
    // not found: return null ref
    return ref;
}

/// Creates memory stream as copy of string contents
LVStreamRef LVCreateStringStream( lString8 data )
{
    LVMemoryStream * stream = new LVMemoryStream();
    stream->CreateCopy( (const lUInt8*)data.c_str(), data.length(), LVOM_READ );
    return LVStreamRef( stream );
}

/// Creates memory stream as copy of string contents
LVStreamRef LVCreateStringStream( lString32 data )
{
    return LVCreateStringStream( UnicodeToUtf8( data ) );
}

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

LVStreamRef LVCreateMemoryStream( LVStreamRef srcStream )
{
    LVMemoryStream * stream = new LVMemoryStream();
    if ( stream->CreateCopy(srcStream, LVOM_READ)==LVERR_OK )
        return LVStreamRef( stream );
    else
        delete stream;
    return LVStreamRef();
}

/// Creates memory stream as copy of file contents.
LVStreamRef LVCreateMemoryStream( lString32 filename )
{
    LVStreamRef fs = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( fs.isNull() )
        return fs;
    return LVCreateMemoryStream( fs );
}

LVStreamRef LVCreateBufferedStream( LVStreamRef stream, int bufSize )
{
    if ( stream.isNull() || bufSize < 512 )
        return stream;
    return LVStreamRef( new LVCachedStream( stream, bufSize ) );
}

lvsize_t LVPumpStream( LVStreamRef out, LVStreamRef in )
{
    return LVPumpStream( out.get(), in.get() );
}

lvsize_t LVPumpStream( LVStream * out, LVStream * in )
{
    char buf[5000];
    lvsize_t totalBytesRead = 0;
    lvsize_t bytesRead = 0;
    in->SetPos(0);
    lvsize_t bytesToRead = in->GetSize();
    while ( bytesToRead>0 )
    {
        unsigned blockSize = 5000;
        if (blockSize > bytesToRead)
            blockSize = bytesToRead;
        bytesRead = 0;
        if ( in->Read( buf, blockSize, &bytesRead )!=LVERR_OK )
            break;
        if ( !bytesRead )
            break;
        out->Write( buf, bytesRead, NULL );
        totalBytesRead += bytesRead;
        bytesToRead -= bytesRead;
    }
    return totalBytesRead;
}

bool LVDirectoryIsEmpty(const lString8& path) {
    return LVDirectoryIsEmpty(Utf8ToUnicode(path));
}

bool LVDirectoryIsEmpty(const lString32& path) {
    LVContainerRef dir = LVOpenDirectory(path);
    if (dir.isNull())
        return false;
    return dir->GetObjectCount() == 0;
}

LVContainerRef LVOpenDirectory(const lString32& path, const char32_t * mask) {
	return LVOpenDirectory(path.c_str(), mask);
}

LVContainerRef LVOpenDirectory(const lString8& path, const char32_t * mask) {
	return LVOpenDirectory(Utf8ToUnicode(path).c_str(), mask);
}

LVContainerRef LVOpenDirectory( const char32_t * path, const char32_t * mask )
{
	lString32 pathname(path);
    if (pathname.length() > 1 && pathname[0] == ASSET_PATH_PREFIX) {
    	if (!_assetContainerFactory)
    		return LVContainerRef();
    	lString32 assetPath = LVExtractAssetPath(pathname);
    	return _assetContainerFactory->openAssetContainer(assetPath);
    }
    LVContainerRef dir(LVDirectoryContainer::OpenDirectory(path, mask));
    return dir;
}

/// creates TCR decoder stream for stream
LVStreamRef LVCreateTCRDecoderStream( LVStreamRef stream )
{
    return LVTCRStream::create( stream, LVOM_READ );
}

/// returns path part of pathname (appended with / or \ delimiter)
lString8 LVExtractPath( lString8 pathName, bool appendEmptyPath) {
    return UnicodeToUtf8(LVExtractPath(Utf8ToUnicode(pathName), appendEmptyPath));
}

/// returns path part of pathname (appended with / or \ delimiter)
lString32 LVExtractPath( lString32 pathName, bool appendEmptyPath )
{
    int last_delim_pos = -1;
    for ( int i=0; i<pathName.length(); i++ )
        if ( pathName[i]=='/' || pathName[i]=='\\' )
            last_delim_pos = i;
    if ( last_delim_pos==-1 )
#ifdef _LINUX
        return lString32(appendEmptyPath ? U"./" : U"");
#else
        return lString32(appendEmptyPath ? U".\\" : U"");
#endif
    return pathName.substr( 0, last_delim_pos+1 );
}

/// returns filename part of pathname
lString8 LVExtractFilename( lString8 pathName ) {
    return UnicodeToUtf8(LVExtractFilename(Utf8ToUnicode(pathName)));
}

/// returns filename part of pathname
lString32 LVExtractFilename( lString32 pathName )
{
    int last_delim_pos = -1;
    for ( int i=0; i<pathName.length(); i++ )
        if ( pathName[i]=='/' || pathName[i]=='\\' )
            last_delim_pos = i;
    if ( last_delim_pos==-1 )
        return pathName;
    return pathName.substr( last_delim_pos+1 );
}

/// returns filename part of pathname without extension
lString32 LVExtractFilenameWithoutExtension( lString32 pathName )
{
    lString32 s = LVExtractFilename( pathName );
    int lastDot = -1;
    for ( int i=0; i<s.length(); i++ )
        if ( s[i]=='.' )
            lastDot = i;
    if ( lastDot<=0 || lastDot<(int)s.length()-7 )
        return s;
    return s.substr( 0, lastDot );
}

/// returns true if absolute path is specified
bool LVIsAbsolutePath( lString32 pathName )
{
    if ( pathName.empty() )
        return false;
    lChar32 c = pathName[0];
    if ( c=='\\' || c=='/' )
        return true;
#ifdef _WIN32
    if ( (c>='a' && c<='z') || (c>='A' && c<='Z') ) {
        return (pathName[1]==':');
    }
#endif
    return false;
}

/// removes first path part from pathname and returns it
lString32 LVExtractFirstPathElement( lString32 & pathName )
{
    if ( pathName.empty() )
        return lString32::empty_str;
    if ( pathName[0]=='/' || pathName[0]=='\\' )
        pathName.erase(0, 1);
    int first_delim_pos = -1;
    for ( int i=0; i<pathName.length(); i++ )
        if ( pathName[i]=='/' || pathName[i]=='\\' ) {
            first_delim_pos = i;
            break;
        }
    if ( first_delim_pos==-1 ) {
        lString32 res = pathName;
        pathName.clear();
        return res;
    }
    lString32 res = pathName.substr(0, first_delim_pos );
    pathName.erase(0, first_delim_pos+1 );
    return res;
}

/// appends path delimiter character to end of path, if absent
void LVAppendPathDelimiter( lString32 & pathName )
{
    if ( pathName.empty() || (pathName.length() == 1 && pathName[0] == ASSET_PATH_PREFIX))
        return;
    lChar32 delim = LVDetectPathDelimiter( pathName );
    if ( pathName[pathName.length()-1]!=delim )
        pathName << delim;
}

/// appends path delimiter character to end of path, if absent
void LVAppendPathDelimiter( lString8 & pathName )
{
    if ( pathName.empty() || (pathName.length() == 1 && pathName[0] == ASSET_PATH_PREFIX))
        return;
    lChar8 delim = LVDetectPathDelimiter(pathName);
    if ( pathName[pathName.length()-1]!=delim )
        pathName << delim;
}

/// removes path delimiter from end of path, if present
void LVRemoveLastPathDelimiter( lString32 & pathName ) {
    if (pathName.empty() || (pathName.length() == 1 && pathName[0] == ASSET_PATH_PREFIX))
        return;
    if (pathName.endsWith("/") || pathName.endsWith("\\"))
        pathName = pathName.substr(0, pathName.length() - 1);
}

/// removes path delimiter from end of path, if present
void LVRemoveLastPathDelimiter( lString8 & pathName )
{
    if (pathName.empty() || (pathName.length() == 1 && pathName[0] == ASSET_PATH_PREFIX))
        return;
    if (pathName.endsWith("/") || pathName.endsWith("\\"))
        pathName = pathName.substr(0, pathName.length() - 1);
}

/// replaces any found / or \\ separator with specified one
void LVReplacePathSeparator( lString32 & pathName, lChar32 separator )
{
    lChar32 * buf = pathName.modify();
    for ( ; *buf; buf++ )
        if ( *buf=='/' || *buf=='\\' )
            *buf = separator;
}

// resolve relative links
lString32 LVCombinePaths( lString32 basePath, lString32 newPath )
{
    if ( newPath[0]=='/' || newPath[0]=='\\' || (newPath.length()>0 && newPath[1]==':' && newPath[2]=='\\') )
        return newPath; // absolute path
    lChar32 separator = 0;
    if (!basePath.empty())
        LVAppendPathDelimiter(basePath);
    for ( int i=0; i<basePath.length(); i++ ) {
        if ( basePath[i]=='/' || basePath[i]=='\\' ) {
            separator = basePath[i];
            break;
        }
    }
    if ( separator == 0 )
        for ( int i=0; i<newPath.length(); i++ ) {
            if ( newPath[i]=='/' || newPath[i]=='\\' ) {
                separator = newPath[i];
                break;
            }
        }
    if ( separator == 0 )
        separator = '/';
    lString32 s = basePath;
    LVAppendPathDelimiter( s );
    s += newPath;
    //LVAppendPathDelimiter( s );
    LVReplacePathSeparator( s, separator );
    lString32 pattern;
    pattern << separator << ".." << separator;
    bool changed;
    do {
        changed = false;
        int lastElementStart = 0;
        for ( int i=0; i<(int)(s.length()-pattern.length()); i++ ) {
            if ( s[i]==separator && s[i+1]!='.' )
                lastElementStart = i + 1;
            else if ( s[i]==separator && s[i+1]=='.' && s[i+2]=='.' && s[i+3]==separator ) {
                if ( lastElementStart>=0 ) {
                    // /a/b/../c/
                    // 0123456789
                    //   ^ ^
                    s.erase( lastElementStart, i+4-lastElementStart );
                    changed = true;
                    // lastElementStart = -1;
                    break;
                }
            }
        }
    } while ( changed && s.length()>=pattern.length() );
    // Replace "/./" inside with "/"
    pattern.clear();
    pattern << separator << "." << separator;
    lString32 replacement;
    replacement << separator;
    while ( s.replace( pattern, replacement ) ) ;
    // Remove "./" at start
    if ( s.length()>2 && s[0]=='.' && s[1]==separator )
        s.erase(0, 2);
    return s;
}

/// removes last path part from pathname and returns it
lString32 LVExtractLastPathElement( lString32 & pathName )
{
    int l = pathName.length();
    if ( l==0 )
        return lString32::empty_str;
    if ( pathName[l-1]=='/' || pathName[l-1]=='\\' )
        pathName.erase(l-1, 1);
    int last_delim_pos = -1;
    for ( int i=0; i<pathName.length(); i++ )
        if ( pathName[i]=='/' || pathName[i]=='\\' )
            last_delim_pos = i;
    if ( last_delim_pos==-1 ) {
        lString32 res = pathName;
        pathName.clear();
        return res;
    }
    lString32 res = pathName.substr( last_delim_pos + 1, pathName.length()-last_delim_pos-1 );
    pathName.erase( last_delim_pos, pathName.length()-last_delim_pos );
    return res;
}

/// returns path delimiter character
lChar32 LVDetectPathDelimiter( lString32 pathName )
{
    for ( int i=0; i<pathName.length(); i++ )
        if ( pathName[i]=='/' || pathName[i]=='\\' )
            return pathName[i];
#ifdef _LINUX
        return '/';
#else
        return '\\';
#endif
}

/// returns path delimiter character
char LVDetectPathDelimiter( lString8 pathName ) {
    for ( int i=0; i<pathName.length(); i++ )
        if ( pathName[i]=='/' || pathName[i]=='\\' )
            return pathName[i];
#ifdef _LINUX
        return '/';
#else
        return '\\';
#endif
}

/// returns full path to file identified by pathName, with base directory == basePath
lString32 LVMakeRelativeFilename( lString32 basePath, lString32 pathName )
{
    if ( LVIsAbsolutePath( pathName ) )
        return pathName;
    lChar32 delim = LVDetectPathDelimiter( basePath );
    lString32 path = LVExtractPath( basePath );
    lString32 name = LVExtractFilename( pathName );
    lString32 dstpath = LVExtractPath( pathName );
    while ( !dstpath.empty() ) {
        lString32 element = LVExtractFirstPathElement( dstpath );
        if (element == ".") {
            // do nothing
        } else if (element == "..")
            LVExtractLastPathElement( path );
        else
            path << element << delim;
    }
    LVAppendPathDelimiter( path );
    path << name;
    return path;
}

/// removes path delimiter character from end of path, if exists
void LVRemovePathDelimiter( lString32 & pathName )
{
    int len = pathName.length();
    if ( len>0 && pathName != "/" && pathName != "\\" && !pathName.endsWith(":\\") && !pathName.endsWith("\\\\")) {
        if ( pathName.lastChar() == '/' || pathName.lastChar() == '\\' )
            pathName.erase( pathName.length()-1, 1 );
    }
}

/// removes path delimiter character from end of path, if exists
void LVRemovePathDelimiter( lString8 & pathName )
{
    int len = pathName.length();
    if ( len>0 && pathName != "/" && pathName != "\\" && !pathName.endsWith(":\\") && !pathName.endsWith("\\\\")) {
        if ( pathName.lastChar() == '/' || pathName.lastChar() == '\\' )
            pathName.erase( pathName.length()-1, 1 );
    }
}

/// returns true if specified file exists
bool LVFileExists( const lString8 & pathName ) {
    return LVFileExists(Utf8ToUnicode(pathName));
}

/// returns true if specified file exists
bool LVFileExists( const lString32 & pathName )
{
    lString32 fn(pathName);
    if (fn.length() > 1 && fn[0] == ASSET_PATH_PREFIX) {
    	if (!_assetContainerFactory)
    		return false;
    	lString32 assetPath = LVExtractAssetPath(fn);
    	return !_assetContainerFactory->openAssetStream(assetPath).isNull();
    }
#ifdef _WIN32
	LVStreamRef stream = LVOpenFileStream( pathName.c_str(), LVOM_READ );
	return !stream.isNull();
#else
    FILE * f = fopen(UnicodeToUtf8(pathName).c_str(), "rb");
    if ( f ) {
        fclose( f );
        return true;
    }
    return false;
#endif
}

/// returns true if directory exists and your app can write to directory
bool LVDirectoryIsWritable(const lString32 & pathName) {
    lString32 fn = pathName;
    LVAppendPathDelimiter(fn);
    fn << ".cr3_directory_write_test";
    bool res = false;
    bool created = false;
    {
        LVStreamRef stream = LVOpenFileStream(fn.c_str(), LVOM_WRITE);
        if (!stream.isNull()) {
            created = true;
            lvsize_t bytesWritten = 0;
            if (stream->Write("TEST", 4, &bytesWritten) == LVERR_OK && bytesWritten == 4) {
                res = true;
            }
        }
    }
    if (created)
        LVDeleteFile(fn);
    return res;
}

/// returns true if specified directory exists
bool LVDirectoryExists( const lString32 & pathName )
{
    lString32 fn(pathName);
    if (fn.length() > 1 && fn[0] == ASSET_PATH_PREFIX) {
    	if (!_assetContainerFactory)
    		return false;
    	lString32 assetPath = LVExtractAssetPath(fn);
    	return !_assetContainerFactory->openAssetContainer(assetPath).isNull();
    }
    LVContainerRef dir = LVOpenDirectory( pathName.c_str() );
    return !dir.isNull();
}

/// returns true if specified directory exists
bool LVDirectoryExists( const lString8 & pathName )
{
    lString32 fn(Utf8ToUnicode(pathName));
    if (fn.length() > 1 && fn[0] == ASSET_PATH_PREFIX) {
    	if (!_assetContainerFactory)
    		return false;
    	lString32 assetPath = LVExtractAssetPath(fn);
    	return !_assetContainerFactory->openAssetContainer(assetPath).isNull();
    }
    LVContainerRef dir = LVOpenDirectory(fn);
    return !dir.isNull();
}

/// Create directory if not exist
bool LVCreateDirectory( lString32 path )
{
    CRLog::trace("LVCreateDirectory(%s)", UnicodeToUtf8(path).c_str() );
    //LVRemovePathDelimiter(path);
    if ( path.length() <= 1 )
        return false;
    if (path[0] == ASSET_PATH_PREFIX) {
    	// cannot create directory in asset
    	return false;
    }
    LVContainerRef dir = LVOpenDirectory( path.c_str() );
    if ( dir.isNull() ) {
        CRLog::trace("Directory %s not found", UnicodeToUtf8(path).c_str());
        LVRemovePathDelimiter(path);
        lString32 basedir = LVExtractPath( path );
        CRLog::trace("Checking base directory %s", UnicodeToUtf8(basedir).c_str());
        if ( !LVCreateDirectory( basedir ) ) {
            CRLog::error("Failed to create directory %s", UnicodeToUtf8(basedir).c_str());
            return false;
        }
#ifdef _WIN32
        return CreateDirectoryW( UnicodeToUtf16(path).c_str(), NULL )!=0;
#else
        //LVRemovePathDelimiter( path );
        lString8 path8 = UnicodeToUtf8( path );
        CRLog::trace("Creating directory %s", path8.c_str() );
        if ( mkdir(path8.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) ) {
            CRLog::error("Cannot create directory %s", path8.c_str() );
            return false;
        }
        return true;
#endif
    }
    CRLog::trace("Directory %s exists", UnicodeToUtf8(path).c_str());
    return true;
}

/// Open memory mapped file
/**
    \param pathname is file name to open (unicode)
    \param mode is mode file should be opened in (LVOM_READ or LVOM_APPEND only)
        \param minSize is minimum file size for R/W mode
    \return reference to opened stream if success, NULL if error
*/
LVStreamRef LVMapFileStream( const lChar32 * pathname, lvopen_mode_t mode, lvsize_t minSize )
{
#if !defined(_WIN32) && !defined(_LINUX)
        // STUB for systems w/o mmap
    LVFileStream * stream = LVFileStream::CreateFileStream( pathname, mode );
    if ( stream!=NULL )
    {
        return LVStreamRef( stream );
    }
    return LVStreamRef();
#else
        LVFileMappedStream * stream = LVFileMappedStream::CreateFileStream( lString32(pathname), mode, (int)minSize );
        return LVStreamRef(stream);
#endif
}

/// delete file, return true if file found and successfully deleted
bool LVDeleteFile( lString32 filename )
{
#ifdef _WIN32
    return DeleteFileW( UnicodeToUtf16(filename).c_str() ) ? true : false;
#else
    if ( unlink( UnicodeToUtf8( filename ).c_str() ) )
        return false;
    return true;
#endif
}

/// rename file
bool LVRenameFile(lString32 oldname, lString32 newname) {
    lString8 oldname8 = UnicodeToLocal(oldname);
    lString8 newname8 = UnicodeToLocal(newname);
#ifdef _WIN32
    lString16 oldname16 = UnicodeToUtf16(oldname);
    lString16 newname16 = UnicodeToUtf16(newname);
    CRLog::trace("Renaming %s to %s", oldname8.c_str(), newname8.c_str());
    bool res = MoveFileW(oldname16.c_str(), newname16.c_str()) != 0;
    if (!res) {
        CRLog::error("Renaming result: %s for renaming of %s to %s", res ? "success" : "failed", oldname8.c_str(), newname8.c_str());
        CRLog::error("Last Error: %d", GetLastError());
    }
    return res;
#else
    return LVRenameFile(oldname8, newname8);
#endif
}

/// rename file
bool LVRenameFile(lString8 oldname, lString8 newname) {
#ifdef _WIN32
    return LVRenameFile(LocalToUnicode(oldname), LocalToUnicode(newname));
#else
    return !rename(oldname.c_str(), newname.c_str());
#endif
}

/// delete file, return true if file found and successfully deleted
bool LVDeleteFile( lString8 filename ) {
    return LVDeleteFile(Utf8ToUnicode(filename));
}

/// delete directory, return true if directory is found and successfully deleted
bool LVDeleteDirectory( lString32 filename ) {
#ifdef _WIN32
    return RemoveDirectoryW( UnicodeToUtf16(filename).c_str() ) ? true : false;
#else
    if ( unlink( UnicodeToUtf8( filename ).c_str() ) )
        return false;
    return true;
#endif
}

/// delete directory, return true if directory is found and successfully deleted
bool LVDeleteDirectory( lString8 filename ) {
    return LVDeleteDirectory(Utf8ToUnicode(filename));
}

LVStreamRef LVCreateBlockWriteStream( LVStreamRef baseStream, int blockSize, int blockCount )
{
    if ( baseStream.isNull() || baseStream->GetMode()==LVOM_READ )
        return baseStream;
    return LVStreamRef( new LVBlockWriteStream(baseStream, blockSize, blockCount) );
}

/// set container to handle filesystem access for paths started with ASSET_PATH_PREFIX (@ sign)
void LVSetAssetContainerFactory(LVAssetContainerFactory * asset) {
    _assetContainerFactory = asset;
}
