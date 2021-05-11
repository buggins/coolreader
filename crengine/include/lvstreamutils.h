/** @file lvstreamutils.h
    @brief Various stream utilities funstions

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVSTREAMUTILS_H_INCLUDED__
#define __LVSTREAMUTILS_H_INCLUDED__

#include "lvstream.h"
#include "lvcontainer.h"
#include "lvassetcontainerfactory.h"

/// Open file stream
/**
    \param pathname is file name to open (unicode)
    \param mode is mode file should be opened in
    \return reference to opened stream if success, NULL if error
*/
LVStreamRef LVOpenFileStream( const lChar32 * pathname, int mode );

/// Open file stream
/**
    \param pathname is file name to open (utf8 codepage)
    \param mode is mode file should be opened in
    \return reference to opened stream if success, NULL if error
*/
LVStreamRef LVOpenFileStream( const lChar8 * pathname, int mode );

/// Open memory mapped file
/**
    \param pathname is file name to open (unicode)
    \param mode is mode file should be opened in (LVOM_READ or LVOM_APPEND only)
	\param minSize is minimum file size for R/W mode
    \return reference to opened stream if success, NULL if error
*/
LVStreamRef LVMapFileStream( const lChar32 * pathname, lvopen_mode_t mode, lvsize_t minSize );

/// Open memory mapped file
/**
    \param pathname is file name to open (unicode)
    \param mode is mode file should be opened in (LVOM_READ or LVOM_APPEND only)
	\param minSize is minimum file size for R/W mode
    \return reference to opened stream if success, NULL if error
*/
LVStreamRef LVMapFileStream( const lChar8 * pathname, lvopen_mode_t mode, lvsize_t minSize );

/// Open archieve from stream
/**
    \param stream is archieve file stream
    \return reference to opened archieve if success, NULL reference if error
*/
LVContainerRef LVOpenArchieve( LVStreamRef stream );

/// Creates memory stream
/**
    \param buf is pointer to buffer, if NULL, empty read/write memory stream will be created
    \param bufSize is buffer size, in bytes
    \param createCopy if true, read/write copy of specified data is being created, otherwise non-managed readonly buffer is being used as is
    \param mode is open mode
    \return reference to opened stream if success, NULL reference if error
*/
LVStreamRef LVCreateMemoryStream( void * buf = NULL, int bufSize = 0, bool createCopy = false, lvopen_mode_t mode = LVOM_READ );
/// Creates memory stream as copy of another stream.
LVStreamRef LVCreateMemoryStream( LVStreamRef srcStream );
/// Creates memory stream as copy of file contents.
LVStreamRef LVCreateMemoryStream( lString32 filename );
/// Creates memory stream as copy of string contents
LVStreamRef LVCreateStringStream( lString8 data );
/// Creates memory stream as copy of string contents
LVStreamRef LVCreateStringStream( lString32 data );

/// creates cache buffers for stream, to write data by big blocks to optimize Flash drives writing performance
LVStreamRef LVCreateBlockWriteStream( LVStreamRef baseStream, int blockSize, int blockCount );

LVContainerRef LVOpenDirectory( const lChar32 * path, const char32_t * mask = U"*.*" );
LVContainerRef LVOpenDirectory(const lString32& path, const char32_t * mask = U"*.*" );
LVContainerRef LVOpenDirectory(const lString8& path, const char32_t * mask = U"*.*" );

bool LVDirectoryIsEmpty(const lString8& path);
bool LVDirectoryIsEmpty(const lString32& path);

/// Create directory if not exist
bool LVCreateDirectory( lString32 path );
/// delete file, return true if file found and successfully deleted
bool LVDeleteFile( lString32 filename );
/// delete file, return true if file found and successfully deleted
bool LVDeleteFile( lString8 filename );
/// delete directory, return true if directory is found and successfully deleted
bool LVDeleteDirectory( lString32 filename );
/// delete directory, return true if directory is found and successfully deleted
bool LVDeleteDirectory( lString8 filename );
/// rename file
bool LVRenameFile(lString32 oldname, lString32 newname);
/// rename file
bool LVRenameFile(lString8 oldname, lString8 newname);

/// copies content of in stream to out stream
lvsize_t LVPumpStream( LVStreamRef out, LVStreamRef in );
/// copies content of in stream to out stream
lvsize_t LVPumpStream( LVStream * out, LVStream * in );

/// creates buffered stream object for stream
LVStreamRef LVCreateBufferedStream( LVStreamRef stream, int bufSize );
/// creates TCR decoder stream for stream
LVStreamRef LVCreateTCRDecoderStream( LVStreamRef stream );

/// returns path part of pathname (appended with / or \ delimiter)
lString32 LVExtractPath( lString32 pathName, bool appendEmptyPath=true );
/// returns path part of pathname (appended with / or \ delimiter)
lString8 LVExtractPath( lString8 pathName, bool appendEmptyPath=true );
/// removes first path part from pathname and returns it
lString32 LVExtractFirstPathElement( lString32 & pathName );
/// removes last path part from pathname and returns it
lString32 LVExtractLastPathElement( lString32 & pathName );
/// returns filename part of pathname
lString32 LVExtractFilename( lString32 pathName );
/// returns filename part of pathname
lString8 LVExtractFilename( lString8 pathName );
/// returns filename part of pathname without extension
lString32 LVExtractFilenameWithoutExtension( lString32 pathName );
/// appends path delimiter character to end of path, if absent
void LVAppendPathDelimiter( lString32 & pathName );
/// appends path delimiter character to end of path, if absent
void LVAppendPathDelimiter( lString8 & pathName );
/// removes path delimiter from end of path, if present
void LVRemoveLastPathDelimiter( lString8 & pathName );
/// removes path delimiter from end of path, if present
void LVRemoveLastPathDelimiter( lString32 & pathName );
/// replaces any found / or \\ separator with specified one
void LVReplacePathSeparator( lString32 & pathName, lChar32 separator );
/// removes path delimiter character from end of path, if exists
void LVRemovePathDelimiter( lString32 & pathName );
/// removes path delimiter character from end of path, if exists
void LVRemovePathDelimiter( lString8 & pathName );
/// returns path delimiter character
lChar32 LVDetectPathDelimiter( lString32 pathName );
/// returns path delimiter character
char LVDetectPathDelimiter( lString8 pathName );
/// returns true if absolute path is specified
bool LVIsAbsolutePath( lString32 pathName );
/// returns full path to file identified by pathName, with base directory == basePath
lString32 LVMakeRelativeFilename( lString32 basePath, lString32 pathName );
// resolve relative links
lString32 LVCombinePaths( lString32 basePath, lString32 newPath );

/// tries to split full path name into archive name and file name inside archive using separator "@/" or "@\"
bool LVSplitArcName(lString32 fullPathName, lString32 & arcPathName, lString32 & arcItemPathName);
/// tries to split full path name into archive name and file name inside archive using separator "@/" or "@\"
bool LVSplitArcName(lString8 fullPathName, lString8 & arcPathName, lString8 & arcItemPathName);

/// returns true if specified file exists
bool LVFileExists( const lString32 & pathName );
/// returns true if specified file exists
bool LVFileExists( const lString8 & pathName );
/// returns true if specified directory exists
bool LVDirectoryExists( const lString32 & pathName );
/// returns true if specified directory exists
bool LVDirectoryExists( const lString8 & pathName );
/// returns true if directory exists and your app can write to directory
bool LVDirectoryIsWritable(const lString32 & pathName);


#define ASSET_PATH_PREFIX_S "@"
#define ASSET_PATH_PREFIX '@'

/// set container to handle filesystem access for paths started with ASSET_PATH_PREFIX (@ sign)
void LVSetAssetContainerFactory(LVAssetContainerFactory * asset);

#endif  // __LVSTREAMUTILS_H_INCLUDED__
