/*******************************************************

   CoolReader Engine

   lvtinydom.cpp: fast and compact XML DOM tree

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/


/// Change this in case of incompatible changes in XML parsing or DOM
// building that could result in XPATHs being different than previously
// (this could make saved bookmarks and highlights, made with a previous
// version, not found in the DOM built with a newer version.
// Users of this library can request the old behaviour by setting
// gDOMVersionRequested to an older version to request the old (possibly
// buggy) behaviour.
#define DOM_VERSION_CURRENT 20200223

// Also defined in include/lvtinydom.h
#define DOM_VERSION_WITH_NORMALIZED_XPOINTERS 20200223

// Changes:
// 20100101 to 20180502: historical version
//
// 20180503: fixed <BR> that were previously converted to <P> because
// of a fix for "bad LIB.RU books" being applied in any case. This
// could prevent the style of the container to be applied on HTML
// sub-parts after a <BR>, or the style of <P> (with text-indent) to
// be applied after a <BR>.
//
// 20180524: changed default rendering of:
//   <li> (and css 'display:list-item') from css_d_list_item to css_d_list_item_block
//   <cite> from css_d_block to css_d_inline (inline in HTML, block in FB2, ensured by fb2.css)
//   <style> from css_d_inline to css_d_none (invisible in HTML)
// Changed also the default display: value for base elements (and so
// for unknown elements) from css_d_inherit to css_d_inline, and disable
// inheritance for the display: property, as per specs.
// See https://developer.mozilla.org/en-US/docs/Web/CSS/display
// (Initial value: inline; Inherited: no)
//
// 20180528: clean epub.css from class name based declarations
//   added support for style property -cr-ignore-if-dom-version-greater-or-equal: 20180528;
//   to ignore the whole declaration with newer gDOMVersionRequested.
//   Use it to keep class name based declarations that involve display:
//   so to not break previous DOM
// Also: fb2def.h updates
//   Changed some elements from XS_TAG1 to XS_TAG1T (<hr>, <ul>, <ol>,
//   <dl>, <output>, <section>, <svg>), so any text node direct child is
//   now displayed instead of just being dropped (all browsers do display
//   such child text nodes).
//   Also no more hide the <form> element content, as it may contain
//   textual information.
//   Also change <code> from 'white-space: pre' to 'normal', like browsers do
//   Added missing block elements from HTML specs so they are correctly
//   displayed as 'block' instead of the new default of 'inline'.
//
// (20190703: added support for CSS float: and clear: which may
// insert <floatBox> elements in the DOM tree. Bus as this is
// toggable, and legacy rendering is available, no need to limit
// their support to some DOM_VERSION. So no bump needed.)
//
// (20200110: added support for CSS display: inline-block and inline-table,
// which may insert <inlineBox> elements in the DOM tree. Bus as this is
// toggable, and legacy rendering is available, no need to limit
// their support to some DOM_VERSION. So no bump needed.)
//
// 20200223: normalized XPointers/XPATHs, by using createXPointerV2()
// and toStringV2(), that should ensure XPointers survive changes
// in style->display and the insertion or removal of autoBoxing,
// floatBox and inlineBox.
// (Older gDOMVersionRequested will keep using createXPointerV1()
// and toStringV1() to have non-normalized XPointers still working.)
// (20200223: added toggable auto completion of incomplete tables by
// wrapping some elements in a new <tabularBox>.)

extern const int gDOMVersionCurrent = DOM_VERSION_CURRENT;
int gDOMVersionRequested     = DOM_VERSION_CURRENT;


/// change in case of incompatible changes in swap/cache file format to avoid using incompatible swap file
// increment to force complete reload/reparsing of old file
#define CACHE_FILE_FORMAT_VERSION "3.12.63"

/// increment following value to force re-formatting of old book after load
#define FORMATTING_VERSION_ID 0x0023

#ifndef DOC_DATA_COMPRESSION_LEVEL
/// data compression level (0=no compression, 1=fast compressions, 3=normal compression)
// Note: keep this above 1, toggling between compression and no-compression
// can be done at run time by calling compressCachedData(false)
#define DOC_DATA_COMPRESSION_LEVEL 1 // 0, 1, 3 (0=no compression)
#endif

#ifndef STREAM_AUTO_SYNC_SIZE
#define STREAM_AUTO_SYNC_SIZE 300000
#endif //STREAM_AUTO_SYNC_SIZE

//=====================================================
// Document data caching parameters
//=====================================================

#ifndef DOC_BUFFER_SIZE
#define DOC_BUFFER_SIZE 0x00A00000UL // default buffer size
#endif

#if DOC_BUFFER_SIZE >= 0x7FFFFFFFUL
#error DOC_BUFFER_SIZE value is too large. This results in integer overflow.
#endif

//--------------------------------------------------------
// cache memory sizes
//--------------------------------------------------------
#ifndef ENABLED_BLOCK_WRITE_CACHE
#define ENABLED_BLOCK_WRITE_CACHE 1
#endif

#define WRITE_CACHE_TOTAL_SIZE    (10*DOC_BUFFER_SIZE/100)

#define TEXT_CACHE_UNPACKED_SPACE (25*DOC_BUFFER_SIZE/100)
#define TEXT_CACHE_CHUNK_SIZE     0x008000 // 32K
#define ELEM_CACHE_UNPACKED_SPACE (45*DOC_BUFFER_SIZE/100)
#define ELEM_CACHE_CHUNK_SIZE     0x004000 // 16K
#define RECT_CACHE_UNPACKED_SPACE (45*DOC_BUFFER_SIZE/100)
#define RECT_CACHE_CHUNK_SIZE     0x00F000 // 64K
#define STYLE_CACHE_UNPACKED_SPACE (10*DOC_BUFFER_SIZE/100)
#define STYLE_CACHE_CHUNK_SIZE    0x00C000 // 48K
//--------------------------------------------------------

#define COMPRESS_NODE_DATA          true
#define COMPRESS_NODE_STORAGE_DATA  true
#define COMPRESS_MISC_DATA          true
#define COMPRESS_PAGES_DATA         true
#define COMPRESS_TOC_DATA           true
#define COMPRESS_PAGEMAP_DATA       true
#define COMPRESS_STYLE_DATA         true

//#define CACHE_FILE_SECTOR_SIZE 4096
#define CACHE_FILE_SECTOR_SIZE 1024
#define CACHE_FILE_WRITE_BLOCK_PADDING 1

/// set t 1 to log storage reads/writes
#define DEBUG_DOM_STORAGE 0
//#define TRACE_AUTOBOX
/// set to 1 to enable crc check of all blocks of cache file on open
#ifndef ENABLE_CACHE_FILE_CONTENTS_VALIDATION
#define ENABLE_CACHE_FILE_CONTENTS_VALIDATION 1
#endif

#define RECT_DATA_CHUNK_ITEMS_SHIFT 11
#define STYLE_DATA_CHUNK_ITEMS_SHIFT 12

// calculated parameters
#define WRITE_CACHE_BLOCK_SIZE 0x4000
#define WRITE_CACHE_BLOCK_COUNT (WRITE_CACHE_TOTAL_SIZE/WRITE_CACHE_BLOCK_SIZE)
#define TEST_BLOCK_STREAM 0

#define PACK_BUF_SIZE 0x10000
#define UNPACK_BUF_SIZE 0x40000

#define RECT_DATA_CHUNK_ITEMS (1<<RECT_DATA_CHUNK_ITEMS_SHIFT)
#define RECT_DATA_CHUNK_SIZE (RECT_DATA_CHUNK_ITEMS*sizeof(lvdomElementFormatRec))
#define RECT_DATA_CHUNK_MASK (RECT_DATA_CHUNK_ITEMS-1)

#define STYLE_DATA_CHUNK_ITEMS (1<<STYLE_DATA_CHUNK_ITEMS_SHIFT)
#define STYLE_DATA_CHUNK_SIZE (STYLE_DATA_CHUNK_ITEMS*sizeof(ldomNodeStyleInfo))
#define STYLE_DATA_CHUNK_MASK (STYLE_DATA_CHUNK_ITEMS-1)


#define STYLE_HASH_TABLE_SIZE     512
#define FONT_HASH_TABLE_SIZE      256


static const char COMPRESSED_CACHE_FILE_MAGIC[] = "CoolReader 3 Cache"
                                       " File v" CACHE_FILE_FORMAT_VERSION ": "
                                       "c0"
                                       "m1"
                                        "\n";

static const char UNCOMPRESSED_CACHE_FILE_MAGIC[] = "CoolReader 3 Cache"
                                       " File v" CACHE_FILE_FORMAT_VERSION ": "
                                       "c0"
                                       "m0"
                                        "\n";

#define CACHE_FILE_MAGIC_SIZE 40

enum CacheFileBlockType {
    CBT_FREE = 0,
    CBT_INDEX = 1,
    CBT_TEXT_DATA,
    CBT_ELEM_DATA,
    CBT_RECT_DATA, //4
    CBT_ELEM_STYLE_DATA,
    CBT_MAPS_DATA,
    CBT_PAGE_DATA, //7
    CBT_PROP_DATA,
    CBT_NODE_INDEX,
    CBT_ELEM_NODE,
    CBT_TEXT_NODE,
    CBT_REND_PARAMS, //12
    CBT_TOC_DATA,
    CBT_PAGEMAP_DATA,
    CBT_STYLE_DATA,
    CBT_BLOB_INDEX, //16
    CBT_BLOB_DATA,
    CBT_FONT_DATA  //18
};


#include <stdlib.h>
#include <string.h>
#include "../include/crsetup.h"
#include "../include/lvstring.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#if BUILD_LITE!=1
#include "../include/lvrend.h"
#include "../include/chmfmt.h"
#endif
#include "../include/crtest.h"
#include "../include/crlog.h"
#include <stddef.h>
#include <math.h>
#include <zlib.h>
#include <xxhash.h>
#include <lvtextfm.h>

// define to store new text nodes as persistent text, instead of mutable
#define USE_PERSISTENT_TEXT 1


// default is to compress to use smaller cache files (but slower rendering
// and page turns with big documents)
static bool _compressCachedData = true;
void compressCachedData(bool enable) {
	_compressCachedData = enable;
}

// default is to use the TEXT_CACHE_UNPACKED_SPACE & co defined above as is
static float _storageMaxUncompressedSizeFactor = 1;
void setStorageMaxUncompressedSizeFactor(float factor) {
	_storageMaxUncompressedSizeFactor = factor;
}

static bool _enableCacheFileContentsValidation = (bool)ENABLE_CACHE_FILE_CONTENTS_VALIDATION;
void enableCacheFileContentsValidation(bool enable) {
	_enableCacheFileContentsValidation = enable;
}

static int _nextDocumentIndex = 0;
ldomDocument * ldomNode::_documentInstances[MAX_DOCUMENT_INSTANCE_COUNT] = {NULL,};

/// adds document to list, returns ID of allocated document, -1 if no space in instance array
int ldomNode::registerDocument( ldomDocument * doc )
{
    for ( int i=0; i<MAX_DOCUMENT_INSTANCE_COUNT; i++ ) {
        if ( _nextDocumentIndex<0 || _nextDocumentIndex>=MAX_DOCUMENT_INSTANCE_COUNT )
            _nextDocumentIndex = 0;
        if ( _documentInstances[_nextDocumentIndex]==NULL) {
            _documentInstances[_nextDocumentIndex] = doc;
            CRLog::info("ldomNode::registerDocument() - new index = %d", _nextDocumentIndex);
            return _nextDocumentIndex++;
        }
        _nextDocumentIndex++;
    }
    return -1;
}

/// removes document from list
void ldomNode::unregisterDocument( ldomDocument * doc )
{
    for ( int i=0; i<MAX_DOCUMENT_INSTANCE_COUNT; i++ ) {
        if ( _documentInstances[i]==doc ) {
            CRLog::info("ldomNode::unregisterDocument() - for index %d", i);
            _documentInstances[i] = NULL;
        }
    }
}

/// mutable text node
class ldomTextNode
{
    lUInt32 _parentIndex;
    lString8 _text;
public:

    lUInt32 getParentIndex()
    {
        return _parentIndex;
    }

    void setParentIndex( lUInt32 n )
    {
        _parentIndex = n;
    }

    ldomTextNode( lUInt32 parentIndex, const lString8 & text )
    : _parentIndex(parentIndex), _text(text)
    {
    }

    lString8 getText()
    {
        return _text;
    }

    lString16 getText16()
    {
        return Utf8ToUnicode(_text);
    }

    void setText( const lString8 & s )
    {
        _text = s;
    }

    void setText( const lString16 & s )
    {
        _text = UnicodeToUtf8(s);
    }
};

#define LASSERT(x) \
    if (!(x)) crFatalError(1111, "assertion failed: " #x)

//#define INDEX1 94
//#define INDEX2 96

//#define INDEX1 105
//#define INDEX2 106

/// pack data from _buf to _compbuf
bool ldomPack( const lUInt8 * buf, int bufsize, lUInt8 * &dstbuf, lUInt32 & dstsize );
/// unpack data from _compbuf to _buf
bool ldomUnpack( const lUInt8 * compbuf, int compsize, lUInt8 * &dstbuf, lUInt32 & dstsize  );


#if BUILD_LITE!=1

//static lUInt32 calcHash32( const lUInt8 * s, int len )
//{
//    lUInt32 res = 0;
//    for ( int i=0; i<len; i++ ) {
//        // res*31 + s
//        res = (((((((res<<1)+res)<<1)+res)<<1)+res)<<1)+res + s[i];
//    }
//    return res;
//}

// FNV 64bit hash function
// from http://isthe.com/chongo/tech/comp/fnv/#gcc-O3

//#define NO_FNV_GCC_OPTIMIZATION
/*#define FNV_64_PRIME ((lUInt64)0x100000001b3ULL)
static lUInt64 calcHash64( const lUInt8 * s, int len )
{
    const lUInt8 * endp = s + len;
    // 64 bit FNV hash function
    lUInt64 hval = 14695981039346656037ULL;
    for ( ; s<endp; s++ ) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
        hval *= FNV_64_PRIME;
#else *//* NO_FNV_GCC_OPTIMIZATION *//*
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
            (hval << 7) + (hval << 8) + (hval << 40);
#endif *//* NO_FNV_GCC_OPTIMIZATION *//*
        hval ^= *s;
    }
    return hval;
}*/
static lUInt32 calcHash(const lUInt8 * s, int len)
{
return XXH32(s,len,0);
}
lUInt32 calcGlobalSettingsHash(int documentId)
{
    lUInt32 hash = FORMATTING_VERSION_ID;
    hash = hash * 31 + (int)fontMan->GetShapingMode();
    if (fontMan->GetKerning())
        hash = hash * 75 + 1761;
    hash = hash * 31 + fontMan->GetFontListHash(documentId);
    hash = hash * 31 + (int)fontMan->GetHintingMode();
    if ( LVRendGetFontEmbolden() )
        hash = hash * 75 + 2384761;
    hash = hash * 31 + fontMan->GetFallbackFontFaces().getHash();
    if ( gFlgFloatingPunctuationEnabled )
        hash = hash * 75 + 1761;
    hash = hash * 31 + TextLangMan::getHash();
    hash = hash * 31 + HyphMan::getLeftHyphenMin();
    hash = hash * 31 + HyphMan::getRightHyphenMin();
    hash = hash * 31 + HyphMan::getTrustSoftHyphens();
    hash = hash * 31 + gRenderDPI;
    hash = hash * 31 + gRenderBlockRenderingFlags;
    hash = hash * 31 + gRootFontSize;
    hash = hash * 31 + gInterlineScaleFactor;
    return hash;
}

static void dumpRendMethods( ldomNode * node, lString16 prefix )
{
    lString16 name = prefix;
    if ( node->isText() )
        name << node->getText();
    else
        name << "<" << node->getNodeName() << ">   " << fmt::decimal(node->getRendMethod());
    CRLog::trace( "%s ",LCSTR(name) );
    for ( int i=0; i<node->getChildCount(); i++ ) {
        dumpRendMethods( node->getChildNode(i), prefix + "   ");
    }
}





#define CACHE_FILE_ITEM_MAGIC 0xC007B00C
struct CacheFileItem
{
    lUInt32 _magic;    // magic number
    lUInt16 _dataType;     // data type
    lUInt16 _dataIndex;    // additional data index, for internal usage for data type
    int _blockIndex;   // sequential number of block
    int _blockFilePos; // start of block
    int _blockSize;    // size of block within file
    int _dataSize;     // used data size inside block (<= block size)
    lUInt64 _dataHash; // additional hash of data
    lUInt64 _packedHash; // additional hash of packed data
    lUInt32 _uncompressedSize;   // size of uncompressed block, if compression is applied, 0 if no compression
    lUInt32 _padding;  // explicite padding (this struct would be implicitely padded from 44 bytes to 48 bytes)
                       // so we can set this padding value to 0 (instead of some random data with implicite padding)
                       // in order to get reproducible (same file checksum) cache files when this gets serialized
    bool validate( int fsize )
    {
        if ( _magic!=CACHE_FILE_ITEM_MAGIC ) {
            CRLog::error("CacheFileItem::validate: block magic doesn't match");
            return false;
        }
        if ( _dataSize>_blockSize || _blockSize<0 || _dataSize<0 || _blockFilePos+_dataSize>fsize || _blockFilePos<CACHE_FILE_SECTOR_SIZE) {
            CRLog::error("CacheFileItem::validate: invalid block size or position");
            return false;
        }
        return true;
    }
    CacheFileItem()
    {
    }
    CacheFileItem( lUInt16 dataType, lUInt16 dataIndex )
    : _magic(CACHE_FILE_ITEM_MAGIC)
    , _dataType(dataType)   // data type
    , _dataIndex(dataIndex) // additional data index, for internal usage for data type
    , _blockIndex(0)        // sequential number of block
    , _blockFilePos(0)      // start of block
    , _blockSize(0)         // size of block within file
    , _dataSize(0)          // used data size inside block (<= block size)
    , _dataHash(0)          // hash of data
    , _packedHash(0) // additional hash of packed data
    , _uncompressedSize(0)  // size of uncompressed block, if compression is applied, 0 if no compression
    , _padding(0)           // (padding)
    {
    }
};


struct SimpleCacheFileHeader
{
    char _magic[CACHE_FILE_MAGIC_SIZE] = { 0 }; // magic
    lUInt32 _dirty;
    lUInt32 _dom_version;
    SimpleCacheFileHeader( lUInt32 dirtyFlag ) {
        memcpy( _magic, _compressCachedData ? COMPRESSED_CACHE_FILE_MAGIC : UNCOMPRESSED_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE );
        _dirty = dirtyFlag;
        _dom_version = gDOMVersionRequested;
    }
};

struct CacheFileHeader : public SimpleCacheFileHeader
{
    lUInt32 _fsize;
    CacheFileItem _indexBlock; // index array block parameters,
    // duplicate of one of index records which contains
    bool validate()
    {
        if (memcmp(_magic, _compressCachedData ? COMPRESSED_CACHE_FILE_MAGIC : UNCOMPRESSED_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) != 0) {
            CRLog::error("CacheFileHeader::validate: magic doesn't match");
            return false;
        }
        if ( _dirty!=0 ) {
            CRLog::error("CacheFileHeader::validate: dirty flag is set");
            printf("CRE: ignoring cache file (marked as dirty)\n");
            return false;
        }
        if ( _dom_version != gDOMVersionRequested ) {
            CRLog::error("CacheFileHeader::validate: DOM version mismatch");
            printf("CRE: ignoring cache file (dom version mismatch)\n");
            return false;
        }
        return true;
    }
    CacheFileHeader( CacheFileItem * indexRec, int fsize, lUInt32 dirtyFlag )
    : SimpleCacheFileHeader(dirtyFlag), _indexBlock(0,0)
    {
        if ( indexRec ) {
            memcpy( &_indexBlock, indexRec, sizeof(CacheFileItem));
        } else
            memset( &_indexBlock, 0, sizeof(CacheFileItem));
        _fsize = fsize;
    }
};

/**
 * Cache file implementation.
 */
class CacheFile
{
    int _sectorSize; // block position and size granularity
    int _size;
    bool _indexChanged;
    bool _dirty;
    lString16 _cachePath;
    LVStreamRef _stream; // file stream
    LVPtrVector<CacheFileItem, true> _index; // full file block index
    LVPtrVector<CacheFileItem, false> _freeIndex; // free file block index
    LVHashTable<lUInt32, CacheFileItem*> _map; // hash map for fast search
    // searches for existing block
    CacheFileItem * findBlock( lUInt16 type, lUInt16 index );
    // alocates block at index, reuses existing one, if possible
    CacheFileItem * allocBlock( lUInt16 type, lUInt16 index, int size );
    // mark block as free, for later reusing
    void freeBlock( CacheFileItem * block );
    // writes file header
    bool updateHeader();
    // writes index block
    bool writeIndex();
    // reads index from file
    bool readIndex();
    // reads all blocks of index and checks CRCs
    bool validateContents();
public:
    // return current file size
    int getSize() { return _size; }
    // create uninitialized cache file, call open or create to initialize
    CacheFile();
    // free resources
    ~CacheFile();
    // try open existing cache file
    bool open( lString16 filename );
    // try open existing cache file from stream
    bool open( LVStreamRef stream );
    // create new cache file
    bool create( lString16 filename );
    // create new cache file in stream
    bool create( LVStreamRef stream );
    /// writes block to file
    bool write( lUInt16 type, lUInt16 dataIndex, const lUInt8 * buf, int size, bool compress );
    /// reads and allocates block in memory
    bool read( lUInt16 type, lUInt16 dataIndex, lUInt8 * &buf, int &size );
    /// reads and validates block
    bool validate( CacheFileItem * block );
    /// writes content of serial buffer
    bool write( lUInt16 type, lUInt16 index, SerialBuf & buf, bool compress );
    /// reads content of serial buffer
    bool read( lUInt16 type, lUInt16 index, SerialBuf & buf );
    /// writes content of serial buffer
    bool write( lUInt16 type, SerialBuf & buf, bool compress )
    {
        return write( type, 0, buf, compress);
    }
    /// reads content of serial buffer
    bool read( lUInt16 type, SerialBuf & buf )
    {
        return read(type, 0, buf);
    }
    /// reads block as a stream
    LVStreamRef readStream(lUInt16 type, lUInt16 index);

    /// sets dirty flag value, returns true if value is changed
    bool setDirtyFlag( bool dirty );
    // flushes index
    bool flush( bool clearDirtyFlag, CRTimerUtil & maxTime );
    int roundSector( int n )
    {
        return (n + (_sectorSize-1)) & ~(_sectorSize-1);
    }
    void setAutoSyncSize(int sz) {
        _stream->setAutoSyncSize(sz);
    }
    void setCachePath(const lString16 cachePath) {
        _cachePath = cachePath;
    }
    const lString16 getCachePath() {
        return _cachePath;
    }
};


// create uninitialized cache file, call open or create to initialize
CacheFile::CacheFile()
: _sectorSize( CACHE_FILE_SECTOR_SIZE ), _size(0), _indexChanged(false), _dirty(true), _map(1024), _cachePath(lString16::empty_str)
{
}

// free resources
CacheFile::~CacheFile()
{
    if ( !_stream.isNull() ) {
        // don't flush -- leave file dirty
        //CRTimerUtil infinite;
        //flush( true, infinite );
    }
}

/// sets dirty flag value, returns true if value is changed
bool CacheFile::setDirtyFlag( bool dirty )
{
    if ( _dirty==dirty )
        return false;
    if ( !dirty ) {
        CRLog::info("CacheFile::clearing Dirty flag");
        _stream->Flush(true);
    } else {
        CRLog::info("CacheFile::setting Dirty flag");
    }
    _dirty = dirty;
    SimpleCacheFileHeader hdr(_dirty?1:0);
    _stream->SetPos(0);
    lvsize_t bytesWritten = 0;
    _stream->Write(&hdr, sizeof(hdr), &bytesWritten );
    if ( bytesWritten!=sizeof(hdr) )
        return false;
    _stream->Flush(true);
    //CRLog::trace("setDirtyFlag : hdr is saved with Dirty flag = %d", hdr._dirty);
    return true;
}

// flushes index
bool CacheFile::flush( bool clearDirtyFlag, CRTimerUtil & maxTime )
{
    if ( clearDirtyFlag ) {
        //setDirtyFlag(true);
        if ( !writeIndex() )
            return false;
        setDirtyFlag(false);
    } else {
        _stream->Flush(false, maxTime);
        //CRLog::trace("CacheFile->flush() took %d ms ", (int)timer.elapsed());
    }
    return true;
}

// reads all blocks of index and checks CRCs
bool CacheFile::validateContents()
{
    CRLog::info("Started validation of cache file contents");
    LVHashTable<lUInt32, CacheFileItem*>::pair * pair;
    for ( LVHashTable<lUInt32, CacheFileItem*>::iterator p = _map.forwardIterator(); (pair=p.next())!=NULL; ) {
        if ( pair->value->_dataType==CBT_INDEX )
            continue;
        if ( !validate(pair->value) ) {
            CRLog::error("Contents validation is failed for block type=%d index=%d", (int)pair->value->_dataType, pair->value->_dataIndex );
            return false;
        }
    }
    CRLog::info("Finished validation of cache file contents -- successful");
    return true;
}

// reads index from file
bool CacheFile::readIndex()
{
    CacheFileHeader hdr(NULL, _size, 0);
    _stream->SetPos(0);
    lvsize_t bytesRead = 0;
    _stream->Read(&hdr, sizeof(hdr), &bytesRead );
    if ( bytesRead!=sizeof(hdr) )
        return false;
    CRLog::info("Header read: DirtyFlag=%d", hdr._dirty);
    if ( !hdr.validate() )
        return false;
    if ( (int)hdr._fsize > _size + 4096-1 ) {
        CRLog::error("CacheFile::readIndex: file size doesn't match with header");
        return false;
    }
    if ( !hdr._indexBlock._blockFilePos )
        return true; // empty index is ok
    if ( hdr._indexBlock._blockFilePos>=(int)hdr._fsize || hdr._indexBlock._blockFilePos+hdr._indexBlock._blockSize>(int)hdr._fsize+4096-1 ) {
        CRLog::error("CacheFile::readIndex: Wrong index file position specified in header");
        return false;
    }
    if ((int)_stream->SetPos(hdr._indexBlock._blockFilePos)!=hdr._indexBlock._blockFilePos ) {
        CRLog::error("CacheFile::readIndex: cannot move file position to index block");
        return false;
    }
    int count = hdr._indexBlock._dataSize / sizeof(CacheFileItem);
    if ( count<0 || count>100000 ) {
        CRLog::error("CacheFile::readIndex: invalid number of blocks in index");
        return false;
    }
    CacheFileItem * index = new CacheFileItem[count];
    bytesRead = 0;
    lvsize_t  sz = sizeof(CacheFileItem)*count;
    _stream->Read(index, sz, &bytesRead );
    if ( bytesRead!=sz )
        return false;
    // check CRC
    lUInt32 hash = calcHash( (lUInt8*)index, sz );
    if ( hdr._indexBlock._dataHash!=hash ) {
        CRLog::error("CacheFile::readIndex: CRC doesn't match found %08x expected %08x", hash, hdr._indexBlock._dataHash);
        delete[] index;
        return false;
    }
    for ( int i=0; i<count; i++ ) {
        if (index[i]._dataType == CBT_INDEX)
            index[i] = hdr._indexBlock;
        if ( !index[i].validate(_size) ) {
            delete[] index;
            return false;
        }
        CacheFileItem * item = new CacheFileItem();
        memcpy(item, &index[i], sizeof(CacheFileItem));
        _index.add( item );
        lUInt32 key = ((lUInt32)item->_dataType)<<16 | item->_dataIndex;
        if ( key==0 )
            _freeIndex.add( item );
        else
            _map.set( key, item );
    }
    delete[] index;
    CacheFileItem * indexitem = findBlock(CBT_INDEX, 0);
    if ( !indexitem ) {
        CRLog::error("CacheFile::readIndex: index block info doesn't match header");
        return false;
    }
    _dirty = hdr._dirty ? true : false;
    return true;
}

// writes index block
bool CacheFile::writeIndex()
{
    if ( !_indexChanged )
        return true; // no changes: no writes

    if ( _index.length()==0 )
        return updateHeader();

    // create copy of index in memory
    int count = _index.length();
    CacheFileItem * indexItem = findBlock(CBT_INDEX, 0);
    if (!indexItem) {
        int sz = sizeof(CacheFileItem) * (count * 2 + 100);
        allocBlock(CBT_INDEX, 0, sz);
        indexItem = findBlock(CBT_INDEX, 0);
        count = _index.length();
    }
    CacheFileItem * index = new CacheFileItem[count]();
    int sz = count * sizeof(CacheFileItem);
    for ( int i = 0; i < count; i++ ) {
        memcpy( &index[i], _index[i], sizeof(CacheFileItem) );
        if (index[i]._dataType == CBT_INDEX) {
            index[i]._dataHash = 0;
            index[i]._packedHash = 0;
            index[i]._dataSize = 0;
        }
    }
    bool res = write(CBT_INDEX, 0, (const lUInt8*)index, sz, false);
    delete[] index;

    indexItem = findBlock(CBT_INDEX, 0);
    if ( !res || !indexItem ) {
        CRLog::error("CacheFile::writeIndex: error while writing index!!!");
        return false;
    }

    updateHeader();
    _indexChanged = false;
    return true;
}

// writes file header
bool CacheFile::updateHeader()
{
    CacheFileItem * indexItem = NULL;
    indexItem = findBlock(CBT_INDEX, 0);
    CacheFileHeader hdr(indexItem, _size, _dirty?1:0);
    _stream->SetPos(0);
    lvsize_t bytesWritten = 0;
    _stream->Write(&hdr, sizeof(hdr), &bytesWritten );
    if ( bytesWritten!=sizeof(hdr) )
        return false;
    //CRLog::trace("updateHeader finished: Dirty flag = %d", hdr._dirty);
    return true;
}

//
void CacheFile::freeBlock( CacheFileItem * block )
{
    lUInt32 key = ((lUInt32)block->_dataType)<<16 | block->_dataIndex;
    _map.remove(key);
    block->_dataIndex = 0;
    block->_dataType = 0;
    block->_dataSize = 0;
    _freeIndex.add( block );
}

/// reads block as a stream
LVStreamRef CacheFile::readStream(lUInt16 type, lUInt16 index)
{
    CacheFileItem * block = findBlock(type, index);
    if (block && block->_dataSize) {
#if 0
        lUInt8 * buf = NULL;
        int size = 0;
        if (read(type, index, buf, size))
            return LVCreateMemoryStream(buf, size);
#else
        return LVStreamRef(new LVStreamFragment(_stream, block->_blockFilePos, block->_dataSize));
#endif
    }
    return LVStreamRef();
}

// searches for existing block
CacheFileItem * CacheFile::findBlock( lUInt16 type, lUInt16 index )
{
    lUInt32 key = ((lUInt32)type)<<16 | index;
    CacheFileItem * existing = _map.get( key );
    return existing;
}

// allocates index record for block, sets its new size
CacheFileItem * CacheFile::allocBlock( lUInt16 type, lUInt16 index, int size )
{
    lUInt32 key = ((lUInt32)type)<<16 | index;
    CacheFileItem * existing = _map.get( key );
    if ( existing ) {
        if ( existing->_blockSize >= size ) {
            if ( existing->_dataSize != size ) {
                existing->_dataSize = size;
                _indexChanged = true;
            }
            return existing;
        }
        // old block has not enough space: free it
        freeBlock( existing );
        existing = NULL;
    }
    // search for existing free block of proper size
    int bestSize = -1;
    //int bestIndex = -1;
    for ( int i=0; i<_freeIndex.length(); i++ ) {
        if ( _freeIndex[i] && (_freeIndex[i]->_blockSize>=size) && (bestSize==-1 || _freeIndex[i]->_blockSize<bestSize) ) {
            bestSize = _freeIndex[i]->_blockSize;
            //bestIndex = -1;
            existing = _freeIndex[i];
        }
    }
    if ( existing ) {
        _freeIndex.remove( existing );
        existing->_dataType = type;
        existing->_dataIndex = index;
        existing->_dataSize = size;
        _map.set( key, existing );
        _indexChanged = true;
        return existing;
    }
    // allocate new block
    CacheFileItem * block = new CacheFileItem( type, index );
    _map.set( key, block );
    block->_blockSize = roundSector(size);
    block->_dataSize = size;
    block->_blockIndex = _index.length();
    _index.add(block);
    block->_blockFilePos = _size;
    _size += block->_blockSize;
    _indexChanged = true;
    // really, file size is not extended
    return block;
}

/// reads and validates block
bool CacheFile::validate( CacheFileItem * block )
{
    lUInt8 * buf = NULL;
    unsigned size = 0;

    if ( (int)_stream->SetPos( block->_blockFilePos )!=block->_blockFilePos ) {
        CRLog::error("CacheFile::validate: Cannot set position for block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
        return false;
    }

    // read block from file
    size = block->_dataSize;
    buf = (lUInt8 *)malloc(size);
    lvsize_t bytesRead = 0;
    _stream->Read(buf, size, &bytesRead );
    if ( bytesRead!=size ) {
        CRLog::error("CacheFile::validate: Cannot read block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
        free(buf);
        return false;
    }

    // check CRC for file block
    lUInt32 packedhash = calcHash( buf, size );
    if ( packedhash!=block->_packedHash ) {
        CRLog::error("CacheFile::validate: packed data CRC doesn't match for block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
        free(buf);
        return false;
    }
    free(buf);
    return true;
}

// reads and allocates block in memory
bool CacheFile::read( lUInt16 type, lUInt16 dataIndex, lUInt8 * &buf, int &size )
{
    buf = NULL;
    size = 0;
    CacheFileItem * block = findBlock( type, dataIndex );
    if ( !block ) {
        CRLog::error("CacheFile::read: Block %d:%d not found in file", type, dataIndex);
        return false;
    }
    if ( (int)_stream->SetPos( block->_blockFilePos )!=block->_blockFilePos )
        return false;

    // read block from file
    size = block->_dataSize;
    buf = (lUInt8 *)malloc(size);
    lvsize_t bytesRead = 0;
    _stream->Read(buf, size, &bytesRead );
    if ( (int)bytesRead!=size ) {
        CRLog::error("CacheFile::read: Cannot read block %d:%d of size %d, bytesRead=%d", type, dataIndex, (int)size, (int)bytesRead);
        free(buf);
        buf = NULL;
        size = 0;
        return false;
    }

    bool compress = block->_uncompressedSize!=0;

    if ( compress ) {
        // block is compressed

        // check crc separately only for compressed data
        lUInt32 packedhash = calcHash( buf, size );
        if ( packedhash!=block->_packedHash ) {
            CRLog::error("CacheFile::read: packed data CRC doesn't match for block %d:%d of size %d", type, dataIndex, (int)size);
            free(buf);
            buf = NULL;
            size = 0;
            return false;
        }

        // uncompress block data
        lUInt8 * uncomp_buf = NULL;
        lUInt32 uncomp_size = 0;
        if ( ldomUnpack(buf, size, uncomp_buf, uncomp_size) && uncomp_size==block->_uncompressedSize ) {
            free( buf );
            buf = uncomp_buf;
            size = uncomp_size;
        } else {
            CRLog::error("CacheFile::read: error while uncompressing data for block %d:%d of size %d", type, dataIndex, (int)size);
            free(buf);
            buf = NULL;
            size = 0;
            return false;
        }
    }

    // check CRC
    lUInt32 hash = calcHash( buf, size );
    if (hash != block->_dataHash) {
        CRLog::error("CacheFile::read: CRC doesn't match for block %d:%d of size %d", type, dataIndex, (int)size);
        free(buf);
        buf = NULL;
        size = 0;
        return false;
    }
    // Success. Don't forget to free allocated block externally
    return true;
}

// writes block to file
bool CacheFile::write( lUInt16 type, lUInt16 dataIndex, const lUInt8 * buf, int size, bool compress )
{
    // check whether data is changed
    lUInt32 newhash = calcHash( buf, size );
    CacheFileItem * existingblock = findBlock( type, dataIndex );

    if (existingblock) {
        bool sameSize = ((int)existingblock->_uncompressedSize==size) || (existingblock->_uncompressedSize==0 && (int)existingblock->_dataSize==size);
        if (sameSize && existingblock->_dataHash == newhash ) {
            return true;
        }
    }

#if 0
    if (existingblock)
        CRLog::trace("*    oldsz=%d oldhash=%08x", (int)existingblock->_uncompressedSize, (int)existingblock->_dataHash);
    CRLog::trace("* wr block t=%d[%d] sz=%d hash=%08x", type, dataIndex, size, newhash);
#endif
    setDirtyFlag(true);

    lUInt32 uncompressedSize = 0;
    lUInt64 newpackedhash = newhash;
    if (!_compressCachedData)
        compress = false;
    if ( compress ) {
        lUInt8 * dstbuf = NULL;
        lUInt32 dstsize = 0;
        if ( !ldomPack( buf, size, dstbuf, dstsize ) ) {
            compress = false;
        } else {
            uncompressedSize = size;
            size = dstsize;
            buf = dstbuf;
            newpackedhash = calcHash( buf, size );
#if DEBUG_DOM_STORAGE==1
            //CRLog::trace("packed block %d:%d : %d to %d bytes (%d%%)", type, dataIndex, srcsize, dstsize, srcsize>0?(100*dstsize/srcsize):0 );
#endif
        }
    }

    CacheFileItem * block = NULL;
    if ( existingblock && existingblock->_dataSize>=size ) {
        // reuse existing block
        block = existingblock;
    } else {
        // allocate new block
        if ( existingblock )
            freeBlock( existingblock );
        block = allocBlock( type, dataIndex, size );
    }
    if ( !block )
    {
#if DOC_DATA_COMPRESSION_LEVEL!=0
        if ( compress ) {
            free( (void*)buf );
        }
#endif
        return false;
    }
    if ( (int)_stream->SetPos( block->_blockFilePos )!=block->_blockFilePos )
    {
#if DOC_DATA_COMPRESSION_LEVEL!=0
        if ( compress ) {
            free( (void*)buf );
        }
#endif
        return false;
    }
    // assert: size == block->_dataSize
    // actual writing of data
    block->_dataSize = size;
    lvsize_t bytesWritten = 0;
    _stream->Write(buf, size, &bytesWritten );
    if ( (int)bytesWritten!=size )
    {
#if DOC_DATA_COMPRESSION_LEVEL!=0
        if ( compress ) {
            free( (void*)buf );
        }
#endif
        return false;
    }
#if CACHE_FILE_WRITE_BLOCK_PADDING==1
    int paddingSize = block->_blockSize - size; //roundSector( size ) - size
    if ( paddingSize ) {
        if ((int)block->_blockFilePos + (int)block->_dataSize >= (int)_stream->GetSize() - _sectorSize) {
            LASSERT(size + paddingSize == block->_blockSize );
//            if (paddingSize > 16384) {
//                CRLog::error("paddingSize > 16384");
//            }
//            LASSERT(paddingSize <= 16384);
            lUInt8 tmp[16384];//paddingSize];
            memset(tmp, 0xFF, paddingSize < 16384 ? paddingSize : 16384);
            do {
                int blkSize = paddingSize < 16384 ? paddingSize : 16384;
                _stream->Write(tmp, blkSize, &bytesWritten );
                paddingSize -= blkSize;
            } while (paddingSize > 0);
        }
    }
#endif
    //_stream->Flush(true);
    // update CRC
    block->_dataHash = newhash;
    block->_packedHash = newpackedhash;
    block->_uncompressedSize = uncompressedSize;

    if ( compress ) {
        free( (void*)buf );
    }
    _indexChanged = true;

    //CRLog::error("CacheFile::write: block %d:%d (pos %ds, size %ds) is written (crc=%08x)", type, dataIndex, (int)block->_blockFilePos/_sectorSize, (int)(size+_sectorSize-1)/_sectorSize, block->_dataCRC);
    // success
    return true;
}

/// writes content of serial buffer
bool CacheFile::write( lUInt16 type, lUInt16 index, SerialBuf & buf, bool compress )
{
    return write( type, index, buf.buf(), buf.pos(), compress );
}

/// reads content of serial buffer
bool CacheFile::read( lUInt16 type, lUInt16 index, SerialBuf & buf )
{
    lUInt8 * tmp = NULL;
    int size = 0;
    bool res = read( type, index, tmp, size );
    if ( res ) {
        buf.set( tmp, size );
    }
    buf.setPos(0);
    return res;
}

// try open existing cache file
bool CacheFile::open( lString16 filename )
{
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_APPEND );
    if ( !stream ) {
        CRLog::error( "CacheFile::open: cannot open file %s", LCSTR(filename));
        return false;
    }
    crSetFileToRemoveOnFatalError(LCSTR(filename));
    return open(stream);
}


// try open existing cache file
bool CacheFile::open( LVStreamRef stream )
{
    _stream = stream;
    _size = _stream->GetSize();
    //_stream->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);

    if ( !readIndex() ) {
        CRLog::error("CacheFile::open : cannot read index from file");
        return false;
    }
    if (_enableCacheFileContentsValidation && !validateContents() ) {
        CRLog::error("CacheFile::open : file contents validation failed");
        return false;
    }
    return true;
}

bool CacheFile::create( lString16 filename )
{
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_APPEND );
    if ( _stream.isNull() ) {
        CRLog::error( "CacheFile::create: cannot create file %s", LCSTR(filename));
        return false;
    }
    crSetFileToRemoveOnFatalError(LCSTR(filename));
    return create(stream);
}

bool CacheFile::create( LVStreamRef stream )
{
    _stream = stream;
    //_stream->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);
    if ( _stream->SetPos(0)!=0 ) {
        CRLog::error( "CacheFile::create: cannot seek file");
        _stream.Clear();
        return false;
    }

    _size = _sectorSize;
    LVArray<lUInt8> sector0(_sectorSize, 0);
    lvsize_t bytesWritten = 0;
    _stream->Write(sector0.get(), _sectorSize, &bytesWritten );
    if ( (int)bytesWritten!=_sectorSize ) {
        _stream.Clear();
        return false;
    }
    if (!updateHeader()) {
        _stream.Clear();
        return false;
    }
    return true;
}

// BLOB storage

class ldomBlobItem {
    int _storageIndex;
    lString16 _name;
    int _size;
    lUInt8 * _data;
public:
    ldomBlobItem( lString16 name ) : _storageIndex(-1), _name(name), _size(0), _data(NULL) {

    }
    ~ldomBlobItem() {
        if ( _data )
            delete[] _data;
    }
    int getSize() { return _size; }
    int getIndex() { return _storageIndex; }
    lUInt8 * getData() { return _data; }
    lString16 getName() { return _name; }
    void setIndex(int index, int size) {
        if ( _data )
            delete[] _data;
        _data = NULL;
        _storageIndex = index;
        _size = size;
    }
    void setData( const lUInt8 * data, int size ) {
        if ( _data )
            delete[] _data;
        if (data && size>0) {
            _data = new lUInt8[size];
            memcpy(_data, data, size);
            _size = size;
        } else {
            _data = NULL;
            _size = -1;
        }
    }
};

ldomBlobCache::ldomBlobCache() : _cacheFile(NULL), _changed(false)
{

}

#define BLOB_INDEX_MAGIC "BLOBINDX"

bool ldomBlobCache::loadIndex()
{
    bool res;
    SerialBuf buf(0,true);
    res = _cacheFile->read(CBT_BLOB_INDEX, buf);
    if (!res) {
        _list.clear();
        return true; // missing blob index: treat as empty list of blobs
    }
    if (!buf.checkMagic(BLOB_INDEX_MAGIC))
        return false;
    lUInt32 len;
    buf >> len;
    for ( lUInt32 i = 0; i<len; i++ ) {
        lString16 name;
        buf >> name;
        lUInt32 size;
        buf >> size;
        if (buf.error())
            break;
        ldomBlobItem * item = new ldomBlobItem(name);
        item->setIndex(i, size);
        _list.add(item);
    }
    res = !buf.error();
    return res;
}

bool ldomBlobCache::saveIndex()
{
    bool res;
    SerialBuf buf(0,true);
    buf.putMagic(BLOB_INDEX_MAGIC);
    lUInt32 len = _list.length();
    buf << len;
    for ( lUInt32 i = 0; i<len; i++ ) {
        ldomBlobItem * item = _list[i];
        buf << item->getName();
        buf << (lUInt32)item->getSize();
    }
    res = _cacheFile->write( CBT_BLOB_INDEX, buf, false );
    return res;
}

ContinuousOperationResult ldomBlobCache::saveToCache(CRTimerUtil & timeout)
{
    if (!_list.length() || !_changed || _cacheFile==NULL)
        return CR_DONE;
    bool res = true;
    for ( int i=0; i<_list.length(); i++ ) {
        ldomBlobItem * item = _list[i];
        if ( item->getData() ) {
            res = _cacheFile->write(CBT_BLOB_DATA, i, item->getData(), item->getSize(), false) && res;
            if (res)
                item->setIndex(i, item->getSize());
        }
        if (timeout.expired())
            return CR_TIMEOUT;
    }
    res = saveIndex() && res;
    if ( res )
        _changed = false;
    return res ? CR_DONE : CR_ERROR;
}

void ldomBlobCache::setCacheFile( CacheFile * cacheFile )
{
    _cacheFile = cacheFile;
    CRTimerUtil infinite;
    if (_list.empty())
        loadIndex();
    else
        saveToCache(infinite);
}

bool ldomBlobCache::addBlob( const lUInt8 * data, int size, lString16 name )
{
    CRLog::debug("ldomBlobCache::addBlob( %s, size=%d, [%02x,%02x,%02x,%02x] )", LCSTR(name), size, data[0], data[1], data[2], data[3]);
    int index = _list.length();
    ldomBlobItem * item = new ldomBlobItem(name);
    if (_cacheFile != NULL) {
        _cacheFile->write(CBT_BLOB_DATA, index, data, size, false);
        item->setIndex(index, size);
    } else {
        item->setData(data, size);
    }
    _list.add(item);
    _changed = true;
    return true;
}

LVStreamRef ldomBlobCache::getBlob( lString16 name )
{
    ldomBlobItem * item = NULL;
    lUInt16 index = 0;
    for ( int i=0; i<_list.length(); i++ ) {
        if (_list[i]->getName() == name) {
            item = _list[i];
            index = i;
            break;
        }
    }
    if (item) {
        if (item->getData()) {
            // RAM
            return LVCreateMemoryStream(item->getData(), item->getSize(), true);
        } else {
            // CACHE FILE
            return _cacheFile->readStream(CBT_BLOB_DATA, index);
        }
    }
    return LVStreamRef();
}

#if BUILD_LITE!=1
//#define DEBUG_RENDER_RECT_ACCESS
#ifdef DEBUG_RENDER_RECT_ACCESS
  static signed char render_rect_flags[200000]={0};
  static void rr_lock( ldomNode * node )
  {
    int index = node->getDataIndex()>>4;
    CRLog::debug("RenderRectAccessor(%d) lock", index );
    if ( render_rect_flags[index] )
        crFatalError(123, "render rect accessor: cannot get lock");
    render_rect_flags[index] = 1;
  }
  static void rr_unlock( ldomNode * node )
  {
    int index = node->getDataIndex()>>4;
    CRLog::debug("RenderRectAccessor(%d) lock", index );
    if ( !render_rect_flags[index] )
        crFatalError(123, "render rect accessor: unlock w/o lock");
    render_rect_flags[index] = 0;
  }
#endif

RenderRectAccessor::RenderRectAccessor( ldomNode * node )
: _node(node), _modified(false), _dirty(false)
{
#ifdef DEBUG_RENDER_RECT_ACCESS
    rr_lock( _node );
#endif
    _node->getRenderData(*this);
}

RenderRectAccessor::~RenderRectAccessor()
{
    if ( _modified )
        _node->setRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
    if ( !_dirty )
        rr_unlock( _node );
#endif
}

void RenderRectAccessor::clear()
{
    lvdomElementFormatRec::clear(); // will clear every field
    _modified = true;
    _dirty = false;
}

void RenderRectAccessor::push()
{
    if ( _modified ) {
        _node->setRenderData(*this);
        _modified = false;
        _dirty = true;
        #ifdef DEBUG_RENDER_RECT_ACCESS
            rr_unlock( _node );
        #endif
    }
}

void RenderRectAccessor::setX( int x )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _x != x ) {
        _x = x;
        _modified = true;
    }
}
void RenderRectAccessor::setY( int y )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _y != y ) {
        _y = y;
        _modified = true;
    }
}
void RenderRectAccessor::setWidth( int w )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _width != w ) {
        _width = w;
        _modified = true;
    }
}
void RenderRectAccessor::setHeight( int h )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _height != h ) {
        _height = h;
        _modified = true;
    }
}

int RenderRectAccessor::getX()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _x;
}
int RenderRectAccessor::getY()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _y;
}
int RenderRectAccessor::getWidth()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _width;
}
int RenderRectAccessor::getHeight()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _height;
}
void RenderRectAccessor::getRect( lvRect & rc )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    rc.left = _x;
    rc.top = _y;
    rc.right = _x + _width;
    rc.bottom = _y + _height;
}

void RenderRectAccessor::setInnerX( int x )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _inner_x != x ) {
        _inner_x = x;
        _modified = true;
    }
}
void RenderRectAccessor::setInnerY( int y )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _inner_y != y ) {
        _inner_y = y;
        _modified = true;
    }
}
void RenderRectAccessor::setInnerWidth( int w )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _inner_width != w ) {
        _inner_width = w;
        _modified = true;
    }
}
int RenderRectAccessor::getInnerX()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _inner_x;
}
int RenderRectAccessor::getInnerY()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _inner_y;
}
int RenderRectAccessor::getInnerWidth()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _inner_width;
}
int RenderRectAccessor::getTopOverflow()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _top_overflow;
}
int RenderRectAccessor::getBottomOverflow()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _bottom_overflow;
}
void RenderRectAccessor::setTopOverflow( int dy )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( dy < 0 ) dy = 0; // don't allow a negative value
    if ( _top_overflow != dy ) {
        _top_overflow = dy;
        _modified = true;
    }
}
void RenderRectAccessor::setBottomOverflow( int dy )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( dy < 0 ) dy = 0; // don't allow a negative value
    if ( _bottom_overflow != dy ) {
        _bottom_overflow = dy;
        _modified = true;
    }
}
int RenderRectAccessor::getBaseline()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _baseline;
}
void RenderRectAccessor::setBaseline( int baseline )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _baseline != baseline ) {
        _baseline = baseline;
        _modified = true;
    }
}
int RenderRectAccessor::getListPropNodeIndex()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _listprop_node_idx;
}
void RenderRectAccessor::setListPropNodeIndex( int idx )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _listprop_node_idx != idx ) {
        _listprop_node_idx = idx;
        _modified = true;
    }
}
int RenderRectAccessor::getLangNodeIndex()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _lang_node_idx;
}
void RenderRectAccessor::setLangNodeIndex( int idx )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _lang_node_idx != idx ) {
        _lang_node_idx = idx;
        _modified = true;
    }
}
unsigned short RenderRectAccessor::getFlags()
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    return _flags;
}
void RenderRectAccessor::setFlags( unsigned short flags )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _flags != flags ) {
        _flags = flags;
        _modified = true;
    }
}
void RenderRectAccessor::getTopRectsExcluded( int & lw, int & lh, int & rw, int & rh )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    lw = _extra1 >> 16;    // Both stored in a single int slot (widths are
    rw = _extra1 & 0xFFFF; // constrained to lUint16 in many other places)
    lh = _extra2;
    rh = _extra3;
}
void RenderRectAccessor::setTopRectsExcluded( int lw, int lh, int rw, int rh )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _extra2 != lh || _extra3 != rh || (_extra1>>16) != lw || (_extra1&0xFFFF) != rw ) {
        _extra1 = (lw<<16) + rw;
        _extra2 = lh;
        _extra3 = rh;
        _modified = true;
    }
}
void RenderRectAccessor::getNextFloatMinYs( int & left, int & right )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    left = _extra4;
    right = _extra5;
}
void RenderRectAccessor::setNextFloatMinYs( int left, int right )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    if ( _extra4 != left || _extra5 != right ) {
        _extra4 = left;
        _extra5 = right;
        _modified = true;
    }
}
void RenderRectAccessor::getInvolvedFloatIds( int & float_count, lUInt32 * float_ids )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    float_count = _extra0;
    if (float_count > 0) float_ids[0] = _extra1;
    if (float_count > 1) float_ids[1] = _extra2;
    if (float_count > 2) float_ids[2] = _extra3;
    if (float_count > 3) float_ids[3] = _extra4;
    if (float_count > 4) float_ids[4] = _extra5;
}
void RenderRectAccessor::setInvolvedFloatIds( int float_count, lUInt32 * float_ids )
{
    if ( _dirty ) {
        _dirty = false;
        _node->getRenderData(*this);
#ifdef DEBUG_RENDER_RECT_ACCESS
        rr_lock( _node );
#endif
    }
    _extra0 = float_count;
    if (float_count > 0) _extra1 = float_ids[0];
    if (float_count > 1) _extra2 = float_ids[1];
    if (float_count > 2) _extra3 = float_ids[2];
    if (float_count > 3) _extra4 = float_ids[3];
    if (float_count > 4) _extra5 = float_ids[4];
    _modified = true;
}

#endif


class ldomPersistentText;
class ldomPersistentElement;

/// common header for data storage items
struct DataStorageItemHeader {
    /// item type: LXML_TEXT_NODE, LXML_ELEMENT_NODE, LXML_NO_DATA
    lUInt16 type;
    /// size of item / 16
    lUInt16 sizeDiv16;
    /// data index of this node in document
    lInt32 dataIndex;
    /// data index of parent node in document, 0 means no parent
    lInt32 parentIndex;
};

/// text node storage implementation
struct TextDataStorageItem : public DataStorageItemHeader {
    /// utf8 text length, characters
    lUInt16 length;
    /// utf8 text, w/o zero
    lChar8 text[2]; // utf8 text follows here, w/o zero byte at end
    /// return text
    inline lString16 getText() { return Utf8ToUnicode( text, length ); }
    inline lString8 getText8() { return lString8( text, length ); }
};

/// element node data storage
struct ElementDataStorageItem : public DataStorageItemHeader {
    lUInt16 id;
    lUInt16 nsid;
    lInt16  attrCount;
    lUInt8  rendMethod;
    lUInt8  reserved8;
    lInt32  childCount;
    lInt32  children[1];
    lUInt16 * attrs() { return (lUInt16 *)(children + childCount); }
    lxmlAttribute * attr( int index ) { return (lxmlAttribute *)&(((lUInt16 *)(children + childCount))[index*4]); }
    lUInt32 getAttrValueId( lUInt16 ns, lUInt16 id )
    {
        lUInt16 * a = attrs();
        for ( int i=0; i<attrCount; i++ ) {
            lxmlAttribute * attr = (lxmlAttribute *)(&a[i*4]);
            if ( !attr->compare( ns, id ) )
                continue;
            return  attr->index;
        }
        return LXML_ATTR_VALUE_NONE;
    }
    lxmlAttribute * findAttr( lUInt16 ns, lUInt16 id )
    {
        lUInt16 * a = attrs();
        for ( int i=0; i<attrCount; i++ ) {
            lxmlAttribute * attr = (lxmlAttribute *)(&a[i*4]);
            if ( attr->compare( ns, id ) )
                return attr;
        }
        return NULL;
    }
    // TODO: add items here
    //css_style_ref_t _style;
    //font_ref_t      _font;
};

#endif


//=================================================================
// tinyNodeCollection implementation
//=================================================================

tinyNodeCollection::tinyNodeCollection()
: _textCount(0)
, _textNextFree(0)
, _elemCount(0)
, _elemNextFree(0)
, _styles(STYLE_HASH_TABLE_SIZE)
, _fonts(FONT_HASH_TABLE_SIZE)
, _tinyElementCount(0)
, _itemCount(0)
#if BUILD_LITE!=1
, _renderedBlockCache( 256 )
, _cacheFile(NULL)
, _cacheFileStale(true)
, _cacheFileLeaveAsDirty(false)
, _mapped(false)
, _maperror(false)
, _mapSavingStage(0)
, _spaceWidthScalePercent(DEF_SPACE_WIDTH_SCALE_PERCENT)
, _minSpaceCondensingPercent(DEF_MIN_SPACE_CONDENSING_PERCENT)
, _unusedSpaceThresholdPercent(DEF_UNUSED_SPACE_THRESHOLD_PERCENT)
, _maxAddedLetterSpacingPercent(DEF_MAX_ADDED_LETTER_SPACING_PERCENT)
, _nodeStyleHash(0)
, _nodeDisplayStyleHash(NODE_DISPLAY_STYLE_HASH_UNITIALIZED)
, _nodeDisplayStyleHashInitial(NODE_DISPLAY_STYLE_HASH_UNITIALIZED)
, _nodeStylesInvalidIfLoading(false)
#endif
, _textStorage(this, 't', (lUInt32)(TEXT_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), TEXT_CACHE_CHUNK_SIZE ) // persistent text node data storage
, _elemStorage(this, 'e', (lUInt32)(ELEM_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), ELEM_CACHE_CHUNK_SIZE ) // persistent element data storage
, _rectStorage(this, 'r', (lUInt32)(RECT_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), RECT_CACHE_CHUNK_SIZE ) // element render rect storage
, _styleStorage(this, 's', (lUInt32)(STYLE_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), STYLE_CACHE_CHUNK_SIZE ) // element style info storage
,_docProps(LVCreatePropsContainer())
,_docFlags(DOC_FLAG_DEFAULTS)
,_fontMap(113)
{
    memset( _textList, 0, sizeof(_textList) );
    memset( _elemList, 0, sizeof(_elemList) );
    // _docIndex assigned in ldomDocument constructor
}

tinyNodeCollection::tinyNodeCollection( tinyNodeCollection & v )
: _textCount(0)
, _textNextFree(0)
, _elemCount(0)
, _elemNextFree(0)
, _styles(STYLE_HASH_TABLE_SIZE)
, _fonts(FONT_HASH_TABLE_SIZE)
, _tinyElementCount(0)
, _itemCount(0)
#if BUILD_LITE!=1
, _renderedBlockCache( 256 )
, _cacheFile(NULL)
, _cacheFileStale(true)
, _cacheFileLeaveAsDirty(false)
, _mapped(false)
, _maperror(false)
, _mapSavingStage(0)
, _spaceWidthScalePercent(DEF_SPACE_WIDTH_SCALE_PERCENT)
, _minSpaceCondensingPercent(DEF_MIN_SPACE_CONDENSING_PERCENT)
, _unusedSpaceThresholdPercent(DEF_UNUSED_SPACE_THRESHOLD_PERCENT)
, _maxAddedLetterSpacingPercent(DEF_MAX_ADDED_LETTER_SPACING_PERCENT)
, _nodeStyleHash(0)
, _nodeDisplayStyleHash(NODE_DISPLAY_STYLE_HASH_UNITIALIZED)
, _nodeDisplayStyleHashInitial(NODE_DISPLAY_STYLE_HASH_UNITIALIZED)
, _nodeStylesInvalidIfLoading(false)
#endif
, _textStorage(this, 't', (lUInt32)(TEXT_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), TEXT_CACHE_CHUNK_SIZE ) // persistent text node data storage
, _elemStorage(this, 'e', (lUInt32)(ELEM_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), ELEM_CACHE_CHUNK_SIZE ) // persistent element data storage
, _rectStorage(this, 'r', (lUInt32)(RECT_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), RECT_CACHE_CHUNK_SIZE ) // element render rect storage
, _styleStorage(this, 's', (lUInt32)(STYLE_CACHE_UNPACKED_SPACE*_storageMaxUncompressedSizeFactor), STYLE_CACHE_CHUNK_SIZE ) // element style info storage
,_docProps(LVCreatePropsContainer())
,_docFlags(v._docFlags)
,_stylesheet(v._stylesheet)
,_fontMap(113)
{
    memset( _textList, 0, sizeof(_textList) );
    memset( _elemList, 0, sizeof(_elemList) );
    // _docIndex assigned in ldomDocument constructor
}


#if BUILD_LITE!=1
bool tinyNodeCollection::openCacheFile()
{
    if ( _cacheFile )
        return true;
    CacheFile * f = new CacheFile();
    //lString16 cacheFileName("/tmp/cr3swap.tmp");

    lString16 fname = getProps()->getStringDef( DOC_PROP_FILE_NAME, "noname" );
    //lUInt32 sz = (lUInt32)getProps()->getInt64Def(DOC_PROP_FILE_SIZE, 0);
    lUInt32 crc = getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);

    if ( !ldomDocCache::enabled() ) {
        CRLog::error("Cannot open cached document: cache dir is not initialized");
        delete f;
        return false;
    }

    CRLog::info("ldomDocument::openCacheFile() - looking for cache file %s", UnicodeToUtf8(fname).c_str() );

    lString16 cache_path;
    LVStreamRef map = ldomDocCache::openExisting( fname, crc, getPersistenceFlags(), cache_path );
    if ( map.isNull() ) {
        delete f;
        return false;
    }
    CRLog::info("ldomDocument::openCacheFile() - cache file found, trying to read index %s", UnicodeToUtf8(fname).c_str() );

    if ( !f->open( map ) ) {
        delete f;
        return false;
    }
    CRLog::info("ldomDocument::openCacheFile() - index read successfully %s", UnicodeToUtf8(fname).c_str() );
    f->setCachePath(cache_path);
    _cacheFile = f;
    _textStorage.setCache( f );
    _elemStorage.setCache( f );
    _rectStorage.setCache( f );
    _styleStorage.setCache( f );
    _blobCache.setCacheFile( f );
    return true;
}

bool tinyNodeCollection::swapToCacheIfNecessary()
{
    if ( !_cacheFile || _mapped || _maperror)
        return false;
    return createCacheFile();
    //return swapToCache();
}

bool tinyNodeCollection::createCacheFile()
{
    if ( _cacheFile )
        return true;
    CacheFile * f = new CacheFile();
    //lString16 cacheFileName("/tmp/cr3swap.tmp");

    lString16 fname = getProps()->getStringDef( DOC_PROP_FILE_NAME, "noname" );
    lUInt32 sz = (lUInt32)getProps()->getInt64Def(DOC_PROP_FILE_SIZE, 0);
    lUInt32 crc = getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);

    if ( !ldomDocCache::enabled() ) {
        CRLog::error("Cannot swap: cache dir is not initialized");
        delete f;
        return false;
    }

    CRLog::info("ldomDocument::createCacheFile() - initialized swapping of document %s to cache file", UnicodeToUtf8(fname).c_str() );

    lString16 cache_path;
    LVStreamRef map = ldomDocCache::createNew( fname, crc, getPersistenceFlags(), sz, cache_path );
    if ( map.isNull() ) {
        delete f;
        return false;
    }

    if ( !f->create( map ) ) {
        delete f;
        return false;
    }
    f->setCachePath(cache_path);
    _cacheFile = f;
    _mapped = true;
    _textStorage.setCache( f );
    _elemStorage.setCache( f );
    _rectStorage.setCache( f );
    _styleStorage.setCache( f );
    _blobCache.setCacheFile( f );
    setCacheFileStale(true);
    return true;
}

lString16 tinyNodeCollection::getCacheFilePath() {
    return _cacheFile != NULL ? _cacheFile->getCachePath() : lString16::empty_str;
}

void tinyNodeCollection::clearNodeStyle( lUInt32 dataIndex )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    _styles.release( info._styleIndex );
    _fonts.release( info._fontIndex );
    info._fontIndex = info._styleIndex = 0;
    _styleStorage.setStyleData( dataIndex, &info );
    _nodeStyleHash = 0;
}

void tinyNodeCollection::setNodeStyleIndex( lUInt32 dataIndex, lUInt16 index )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    if ( info._styleIndex!=index ) {
        info._styleIndex = index;
        _styleStorage.setStyleData( dataIndex, &info );
        _nodeStyleHash = 0;
    }
}

void tinyNodeCollection::setNodeFontIndex( lUInt32 dataIndex, lUInt16 index )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    if ( info._fontIndex!=index ) {
        info._fontIndex = index;
        _styleStorage.setStyleData( dataIndex, &info );
        _nodeStyleHash = 0;
    }
}

lUInt16 tinyNodeCollection::getNodeStyleIndex( lUInt32 dataIndex )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    return info._styleIndex;
}

css_style_ref_t tinyNodeCollection::getNodeStyle( lUInt32 dataIndex )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    css_style_ref_t res =  _styles.get( info._styleIndex );
    if (!res.isNull())
    _styles.addIndexRef(info._styleIndex);
#if DEBUG_DOM_STORAGE==1
    if ( res.isNull() && info._styleIndex!=0 ) {
        CRLog::error("Null style returned for index %d", (int)info._styleIndex);
    }
#endif
    return res;
}

font_ref_t tinyNodeCollection::getNodeFont( lUInt32 dataIndex )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    return _fonts.get( info._fontIndex );
}

void tinyNodeCollection::setNodeStyle( lUInt32 dataIndex, css_style_ref_t & v )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    _styles.cache( info._styleIndex, v );
#if DEBUG_DOM_STORAGE==1
    if ( info._styleIndex==0 ) {
        CRLog::error("tinyNodeCollection::setNodeStyle() styleIndex is 0 after caching");
    }
#endif
    _styleStorage.setStyleData( dataIndex, &info );
    _nodeStyleHash = 0;
}

void tinyNodeCollection::setNodeFont( lUInt32 dataIndex, font_ref_t & v )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    _fonts.cache( info._fontIndex, v );
    _styleStorage.setStyleData( dataIndex, &info );
    _nodeStyleHash = 0;
}

lUInt16 tinyNodeCollection::getNodeFontIndex( lUInt32 dataIndex )
{
    ldomNodeStyleInfo info;
    _styleStorage.getStyleData( dataIndex, &info );
    return info._fontIndex;
}

bool tinyNodeCollection::loadNodeData(lUInt16 type, ldomNode ** list, int nodecount)
{
    int count = ((nodecount + TNC_PART_LEN - 1) >> TNC_PART_SHIFT);
    for (lUInt16 i=0; i<count; i++) {
        int offs = i*TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > nodecount) {
            sz = nodecount - offs;
        }

        lUInt8 * p;
        int buflen;
        if (!_cacheFile->read( type, i, p, buflen ))
            return false;
        if (!p || (unsigned)buflen != sizeof(ldomNode) * sz)
            return false;
        ldomNode * buf = (ldomNode *)p;
        if (sz == TNC_PART_LEN)
            list[i] = buf;
        else {
            // buf contains `sz' ldomNode items
            // _elemList, _textList (as `list' argument) must always be TNC_PART_LEN size
            // add into `list' zero filled (TNC_PART_LEN - sz) items
            list[i] = (ldomNode *)realloc(buf, TNC_PART_LEN * sizeof(ldomNode));
            if (NULL == list[i]) {
                free(buf);
                CRLog::error("Not enough memory!");
                return false;
            }
            memset( list[i] + sz, 0, (TNC_PART_LEN - sz) * sizeof(ldomNode) );
        }
        for (int j=0; j<sz; j++) {
            list[i][j].setDocumentIndex( _docIndex );
            // validate loaded nodes: all non-null nodes should be marked as persistent, i.e. the actual node data: _data._pelem_addr, _data._ptext_addr,
            // NOT _data._elem_ptr, _data._text_ptr.
            // So we check this flag, but after setting document so that isNull() works correctly.
            // If the node is not persistent now, then _data._elem_ptr will be used, which then generate SEGFAULT.
            if (!list[i][j].isNull() && !list[i][j].isPersistent()) {
                CRLog::error("Invalid cached node, flag PERSISTENT are NOT set: segment=%d, index=%d", i, j);
                // list[i] will be freed in the caller method.
                return false;
            }
            if ( list[i][j].isElement() ) {
                // will be set by loadStyles/updateStyles
                //list[i][j]._data._pelem._styleIndex = 0;
                setNodeFontIndex( list[i][j]._handle._dataIndex, 0 );
                //list[i][j]._data._pelem._fontIndex = 0;
            }
        }
    }
    return true;
}

bool tinyNodeCollection::saveNodeData( lUInt16 type, ldomNode ** list, int nodecount )
{
    int count = ((nodecount+TNC_PART_LEN-1) >> TNC_PART_SHIFT);
    for (lUInt16 i=0; i<count; i++) {
        if (!list[i])
            continue;
        int offs = i*TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if (offs + sz > nodecount) {
            sz = nodecount - offs;
        }
        ldomNode buf[TNC_PART_LEN];
        memcpy(buf, list[i], sizeof(ldomNode) * sz);
        for (int j = 0; j < sz; j++) {
            buf[j].setDocumentIndex(_docIndex);
            // On 64bits builds, this serialized ldomNode may have some
            // random data at the end, for being:
            //   union { [...] tinyElement * _elem_ptr; [...] lUInt32 _ptext_addr; [...] lUInt32 _nextFreeIndex }
            // To get "reproducible" cache files with a same file checksum, we'd
            // rather have the remains of the _elem_ptr sets to 0
            if (sizeof(int *) == 8) { // 64bits
                lUInt32 tmp = buf[j]._data._nextFreeIndex; // save 32bits part
                buf[j]._data._elem_ptr = 0;                // clear 64bits area
                buf[j]._data._nextFreeIndex = tmp;         // restore 32bits part
            }
        }
        if (!_cacheFile->write(type, i, (lUInt8*)buf, sizeof(ldomNode) * sz, COMPRESS_NODE_DATA))
            crFatalError(-1, "Cannot write node data");
    }
    return true;
}

#define NODE_INDEX_MAGIC 0x19283746
bool tinyNodeCollection::saveNodeData()
{
    SerialBuf buf(12, true);
    buf << (lUInt32)NODE_INDEX_MAGIC << (lUInt32)_elemCount << (lUInt32)_textCount;
    if ( !saveNodeData( CBT_ELEM_NODE, _elemList, _elemCount+1 ) )
        return false;
    if ( !saveNodeData( CBT_TEXT_NODE, _textList, _textCount+1 ) )
        return false;
    if ( !_cacheFile->write(CBT_NODE_INDEX, buf, COMPRESS_NODE_DATA) )
        return false;
    return true;
}

bool tinyNodeCollection::loadNodeData()
{
    SerialBuf buf(0, true);
    if ( !_cacheFile->read((lUInt16)CBT_NODE_INDEX, buf) )
        return false;
    lUInt32 magic;
    lInt32 elemcount;
    lInt32 textcount;
    buf >> magic >> elemcount >> textcount;
    if ( magic != NODE_INDEX_MAGIC ) {
        return false;
    }
    if ( elemcount<=0 )
        return false;
    if ( textcount<=0 )
        return false;
    ldomNode * elemList[TNC_PART_COUNT] = { 0 };
    ldomNode * textList[TNC_PART_COUNT] = { 0 };
    if ( !loadNodeData( CBT_ELEM_NODE, elemList, elemcount+1 ) ) {
        for ( int i=0; i<TNC_PART_COUNT; i++ )
            if ( elemList[i] )
                free( elemList[i] );
        return false;
    }
    if ( !loadNodeData( CBT_TEXT_NODE, textList, textcount+1 ) ) {
        for ( int i=0; i<TNC_PART_COUNT; i++ )
            if ( textList[i] )
                free( textList[i] );
        return false;
    }
    for ( int i=0; i<TNC_PART_COUNT; i++ ) {
        if ( _elemList[i] )
            free( _elemList[i] );
        if ( _textList[i] )
            free( _textList[i] );
    }
    memcpy( _elemList, elemList, sizeof(elemList) );
    memcpy( _textList, textList, sizeof(textList) );
    _elemCount = elemcount;
    _textCount = textcount;
    return true;
}
#endif

/// get ldomNode instance pointer
ldomNode * tinyNodeCollection::getTinyNode( lUInt32 index )
{
    if ( !index )
        return NULL;
    if ( index & 1 ) // element
        return &(_elemList[index>>TNC_PART_INDEX_SHIFT][(index>>4)&TNC_PART_MASK]);
    else // text
        return &(_textList[index>>TNC_PART_INDEX_SHIFT][(index>>4)&TNC_PART_MASK]);
}

/// allocate new tiny node
ldomNode * tinyNodeCollection::allocTinyNode( int type )
{
    ldomNode * res;
    if ( type & 1 ) {
        // allocate Element
        if ( _elemNextFree ) {
            // reuse existing free item
            int index = (_elemNextFree << 4) | type;
            res = getTinyNode(index);
            res->_handle._dataIndex = index;
            _elemNextFree = res->_data._nextFreeIndex;
        } else {
            // create new item
            _elemCount++;
            int idx = _elemCount >> TNC_PART_SHIFT;
            if (idx >= TNC_PART_COUNT)
                crFatalError(1003, "allocTinyNode: can't create any more element nodes (hard limit)");
            ldomNode * part = _elemList[idx];
            if ( !part ) {
                part = (ldomNode*)calloc(TNC_PART_LEN, sizeof(*part));
                _elemList[idx] = part;
            }
            res = &part[_elemCount & TNC_PART_MASK];
            res->setDocumentIndex( _docIndex );
            res->_handle._dataIndex = (_elemCount << 4) | type;
        }
        _itemCount++;
    } else {
        // allocate Text
        if ( _textNextFree ) {
            // reuse existing free item
            int index = (_textNextFree << 4) | type;
            res = getTinyNode(index);
            res->_handle._dataIndex = index;
            _textNextFree = res->_data._nextFreeIndex;
        } else {
            // create new item
            _textCount++;
            if (_textCount >= (TNC_PART_COUNT << TNC_PART_SHIFT))
                crFatalError(1003, "allocTinyNode: can't create any more text nodes (hard limit)");
            ldomNode * part = _textList[_textCount >> TNC_PART_SHIFT];
            if ( !part ) {
                part = (ldomNode*)calloc(TNC_PART_LEN, sizeof(*part));
                _textList[ _textCount >> TNC_PART_SHIFT ] = part;
            }
            res = &part[_textCount & TNC_PART_MASK];
            res->setDocumentIndex( _docIndex );
            res->_handle._dataIndex = (_textCount << 4) | type;
        }
        _itemCount++;
    }
    _nodeStyleHash = 0;
    return res;
}

void tinyNodeCollection::recycleTinyNode( lUInt32 index )
{
    if ( index & 1 ) {
        // element
        index >>= 4;
        ldomNode * part = _elemList[index >> TNC_PART_SHIFT];
        ldomNode * p = &part[index & TNC_PART_MASK];
        p->_handle._dataIndex = 0; // indicates NULL node
        p->_data._nextFreeIndex = _elemNextFree;
        _elemNextFree = index;
        _itemCount--;
    } else {
        // text
        index >>= 4;
        ldomNode * part = _textList[index >> TNC_PART_SHIFT];
        ldomNode * p = &part[index & TNC_PART_MASK];
        p->_handle._dataIndex = 0; // indicates NULL node
        p->_data._nextFreeIndex = _textNextFree;
        _textNextFree = index;
        _itemCount--;
    }
    _nodeStyleHash = 0;
}

tinyNodeCollection::~tinyNodeCollection()
{
#if BUILD_LITE!=1
    if ( _cacheFile )
        delete _cacheFile;
#endif
    // clear all elem parts
    for ( int partindex = 0; partindex<=(_elemCount>>TNC_PART_SHIFT); partindex++ ) {
        ldomNode * part = _elemList[partindex];
        if ( part ) {
            int n0 = TNC_PART_LEN * partindex;
            for ( int i=0; i<TNC_PART_LEN && n0+i<=_elemCount; i++ )
                part[i].onCollectionDestroy();
            free(part);
            _elemList[partindex] = NULL;
        }
    }
    // clear all text parts
    for ( int partindex = 0; partindex<=(_textCount>>TNC_PART_SHIFT); partindex++ ) {
        ldomNode * part = _textList[partindex];
        if ( part ) {
            int n0 = TNC_PART_LEN * partindex;
            for ( int i=0; i<TNC_PART_LEN && n0+i<=_textCount; i++ )
                part[i].onCollectionDestroy();
            free(part);
            _textList[partindex] = NULL;
        }
    }
    // document unregistered in ldomDocument destructor
}

#if BUILD_LITE!=1
/// put all objects into persistent storage
void tinyNodeCollection::persist( CRTimerUtil & maxTime )
{
    CRLog::info("lxmlDocBase::persist() invoked - converting all nodes to persistent objects");
    // elements
    for ( int partindex = 0; partindex<=(_elemCount>>TNC_PART_SHIFT); partindex++ ) {
        ldomNode * part = _elemList[partindex];
        if ( part ) {
            int n0 = TNC_PART_LEN * partindex;
            for ( int i=0; i<TNC_PART_LEN && n0+i<=_elemCount; i++ )
                if ( !part[i].isNull() && !part[i].isPersistent() ) {
                    part[i].persist();
                    if (maxTime.expired())
                        return;
                }
        }
    }
    //_cacheFile->flush(false); // intermediate flush
    if ( maxTime.expired() )
        return;
    // texts
    for ( int partindex = 0; partindex<=(_textCount>>TNC_PART_SHIFT); partindex++ ) {
        ldomNode * part = _textList[partindex];
        if ( part ) {
            int n0 = TNC_PART_LEN * partindex;
            for ( int i=0; i<TNC_PART_LEN && n0+i<=_textCount; i++ )
                if ( !part[i].isNull() && !part[i].isPersistent() ) {
                    //CRLog::trace("before persist");
                    part[i].persist();
                    //CRLog::trace("after persist");
                    if (maxTime.expired())
                        return;
                }
        }
    }
    //_cacheFile->flush(false); // intermediate flush
}
#endif


/*

  Struct Node
  { document, nodeid&type, address }

  Data Offset format

  Chunk index, offset, type.

  getDataPtr( lUInt32 address )
  {
     return (address & TYPE_MASK) ? textStorage.get( address & ~TYPE_MASK ) : elementStorage.get( address & ~TYPE_MASK );
  }

  index->instance, data
  >
  [index] { vtable, doc, id, dataptr } // 16 bytes per node


 */


/// saves all unsaved chunks to cache file
bool ldomDataStorageManager::save( CRTimerUtil & maxTime )
{
    bool res = true;
#if BUILD_LITE!=1
    if ( !_cache )
        return true;
    for ( int i=0; i<_chunks.length(); i++ ) {
        if ( !_chunks[i]->save() ) {
            res = false;
            break;
        }
        //CRLog::trace("time elapsed: %d", (int)maxTime.elapsed());
        if (maxTime.expired())
            return res;
//        if ( (i&3)==3 &&  maxTime.expired() )
//            return res;
    }
    if (!maxTime.infinite())
        _cache->flush(false, maxTime); // intermediate flush
    if ( maxTime.expired() )
        return res;
    if ( !res )
        return false;
    // save chunk index
    int n = _chunks.length();
    SerialBuf buf(n*4+4, true);
    buf << (lUInt32)n;
    for ( int i=0; i<n; i++ ) {
        buf << (lUInt32)_chunks[i]->_bufpos;
    }
    res = _cache->write( cacheType(), 0xFFFF, buf, COMPRESS_NODE_STORAGE_DATA );
    if ( !res ) {
        CRLog::error("ldomDataStorageManager::save() - Cannot write chunk index");
    }
#endif
    return res;
}

/// load chunk index from cache file
bool ldomDataStorageManager::load()
{
#if BUILD_LITE!=1
    if ( !_cache )
        return false;
    //load chunk index
    SerialBuf buf(0, true);
    if ( !_cache->read( cacheType(), 0xFFFF, buf ) ) {
        CRLog::error("ldomDataStorageManager::load() - Cannot read chunk index");
        return false;
    }
    lUInt32 n;
    buf >> n;
    if (n > 10000)
        return false; // invalid
    _recentChunk = NULL;
    _chunks.clear();
    lUInt32 compsize = 0;
    lUInt32 uncompsize = 0;
    for (lUInt32 i=0; i<n; i++ ) {
        buf >> uncompsize;
        if ( buf.error() ) {
            _chunks.clear();
            return false;
        }
        _chunks.add( new ldomTextStorageChunk( this, (lUInt16)i,compsize, uncompsize ) );
    }
    return true;
#else
    return false;
#endif
}

/// get chunk pointer and update usage data
ldomTextStorageChunk * ldomDataStorageManager::getChunk( lUInt32 address )
{
    ldomTextStorageChunk * chunk = _chunks[address>>16];
    if ( chunk!=_recentChunk ) {
        if ( chunk->_prevRecent )
            chunk->_prevRecent->_nextRecent = chunk->_nextRecent;
        if ( chunk->_nextRecent )
            chunk->_nextRecent->_prevRecent = chunk->_prevRecent;
        chunk->_prevRecent = NULL;
        if (((chunk->_nextRecent = _recentChunk)))
            _recentChunk->_prevRecent = chunk;
        _recentChunk = chunk;
    }
    chunk->ensureUnpacked();
    return chunk;
}

void ldomDataStorageManager::setCache( CacheFile * cache )
{
    _cache = cache;
}

/// type
lUInt16 ldomDataStorageManager::cacheType()
{
    switch ( _type ) {
    case 't':
        return CBT_TEXT_DATA;
    case 'e':
        return CBT_ELEM_DATA;
    case 'r':
        return CBT_RECT_DATA;
    case 's':
        return CBT_ELEM_STYLE_DATA;
    }
    return 0;
}

/// get or allocate space for element style data item
void ldomDataStorageManager::getStyleData( lUInt32 elemDataIndex, ldomNodeStyleInfo * dst )
{
    // assume storage has raw data chunks
    int index = elemDataIndex>>4; // element sequential index
    int chunkIndex = index >> STYLE_DATA_CHUNK_ITEMS_SHIFT;
    while ( _chunks.length() <= chunkIndex ) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add( new ldomTextStorageChunk(STYLE_DATA_CHUNK_SIZE, this, _chunks.length()) );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
    }
    ldomTextStorageChunk * chunk = getChunk( chunkIndex<<16 );
    int offsetIndex = index & STYLE_DATA_CHUNK_MASK;
    chunk->getRaw( offsetIndex * sizeof(ldomNodeStyleInfo), sizeof(ldomNodeStyleInfo), (lUInt8 *)dst );
}

/// set element style data item
void ldomDataStorageManager::setStyleData( lUInt32 elemDataIndex, const ldomNodeStyleInfo * src )
{
    // assume storage has raw data chunks
    int index = elemDataIndex>>4; // element sequential index
    int chunkIndex = index >> STYLE_DATA_CHUNK_ITEMS_SHIFT;
    while ( _chunks.length() < chunkIndex ) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add( new ldomTextStorageChunk(STYLE_DATA_CHUNK_SIZE, this, _chunks.length()) );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
    }
    ldomTextStorageChunk * chunk = getChunk( chunkIndex<<16 );
    int offsetIndex = index & STYLE_DATA_CHUNK_MASK;
    chunk->setRaw( offsetIndex * sizeof(ldomNodeStyleInfo), sizeof(ldomNodeStyleInfo), (const lUInt8 *)src );
}


/// get or allocate space for rect data item
void ldomDataStorageManager::getRendRectData( lUInt32 elemDataIndex, lvdomElementFormatRec * dst )
{
    // assume storage has raw data chunks
    int index = elemDataIndex>>4; // element sequential index
    int chunkIndex = index >> RECT_DATA_CHUNK_ITEMS_SHIFT;
    while ( _chunks.length() <= chunkIndex ) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add( new ldomTextStorageChunk(RECT_DATA_CHUNK_SIZE, this, _chunks.length()) );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
    }
    ldomTextStorageChunk * chunk = getChunk( chunkIndex<<16 );
    int offsetIndex = index & RECT_DATA_CHUNK_MASK;
    chunk->getRaw( offsetIndex * sizeof(lvdomElementFormatRec), sizeof(lvdomElementFormatRec), (lUInt8 *)dst );
}

/// set rect data item
void ldomDataStorageManager::setRendRectData( lUInt32 elemDataIndex, const lvdomElementFormatRec * src )
{
    // assume storage has raw data chunks
    int index = elemDataIndex>>4; // element sequential index
    int chunkIndex = index >> RECT_DATA_CHUNK_ITEMS_SHIFT;
    while ( _chunks.length() < chunkIndex ) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add( new ldomTextStorageChunk(RECT_DATA_CHUNK_SIZE, this, _chunks.length()) );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
    }
    ldomTextStorageChunk * chunk = getChunk( chunkIndex<<16 );
    int offsetIndex = index & RECT_DATA_CHUNK_MASK;
    chunk->setRaw( offsetIndex * sizeof(lvdomElementFormatRec), sizeof(lvdomElementFormatRec), (const lUInt8 *)src );
}

#if BUILD_LITE!=1
lUInt32 ldomDataStorageManager::allocText( lUInt32 dataIndex, lUInt32 parentIndex, const lString8 & text )
{
    if ( !_activeChunk ) {
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add( _activeChunk );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
    }
    int offset = _activeChunk->addText( dataIndex, parentIndex, text );
    if ( offset<0 ) {
        // no space in current chunk, add one more chunk
        //_activeChunk->compact();
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add( _activeChunk );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
        offset = _activeChunk->addText( dataIndex, parentIndex, text );
        if ( offset<0 )
            crFatalError(1001, "Unexpected error while allocation of text");
    }
    return offset | (_activeChunk->getIndex()<<16);
}

lUInt32 ldomDataStorageManager::allocElem( lUInt32 dataIndex, lUInt32 parentIndex, int childCount, int attrCount )
{
    if ( !_activeChunk ) {
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add( _activeChunk );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
    }
    int offset = _activeChunk->addElem( dataIndex, parentIndex, childCount, attrCount );
    if ( offset<0 ) {
        // no space in current chunk, add one more chunk
        //_activeChunk->compact();
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add( _activeChunk );
        getChunk( (_chunks.length()-1)<<16 );
        compact( 0 );
        offset = _activeChunk->addElem( dataIndex, parentIndex, childCount, attrCount );
        if ( offset<0 )
            crFatalError(1002, "Unexpected error while allocation of element");
    }
    return offset | (_activeChunk->getIndex()<<16);
}

/// call to invalidate chunk if content is modified
void ldomDataStorageManager::modified( lUInt32 addr )
{
    ldomTextStorageChunk * chunk = getChunk(addr);
    chunk->modified();
}

/// change node's parent
bool ldomDataStorageManager::setParent( lUInt32 address, lUInt32 parent )
{
    ldomTextStorageChunk * chunk = getChunk(address);
    return chunk->setParent(address&0xFFFF, parent);
}

/// free data item
void ldomDataStorageManager::freeNode( lUInt32 addr )
{
    ldomTextStorageChunk * chunk = getChunk(addr);
    chunk->freeNode(addr&0xFFFF);
}


lString8 ldomDataStorageManager::getText( lUInt32 address )
{
    ldomTextStorageChunk * chunk = getChunk(address);
    return chunk->getText(address&0xFFFF);
}

/// get pointer to element data
ElementDataStorageItem * ldomDataStorageManager::getElem( lUInt32 addr )
{
    ldomTextStorageChunk * chunk = getChunk(addr);
    return chunk->getElem(addr&0xFFFF);
}

/// returns node's parent by address
lUInt32 ldomDataStorageManager::getParent( lUInt32 addr )
{
    ldomTextStorageChunk * chunk = getChunk(addr);
    return chunk->getElem(addr&0xFFFF)->parentIndex;
}
#endif

void ldomDataStorageManager::compact( int reservedSpace, const ldomTextStorageChunk* excludedChunk )
{
#if BUILD_LITE!=1
    if ( _uncompressedSize + reservedSpace > _maxUncompressedSize + _maxUncompressedSize/10 ) { // allow +10% overflow
        if (!_maxSizeReachedWarned) {
            // Log once to stdout that we reached maxUncompressedSize, so we can know
            // of this fact and consider it as a possible cause for crengine bugs
            CRLog::warn("Storage for %s reached max allowed uncompressed size (%u > %u)",
                        (_type == 't' ? "TEXT NODES" : (_type == 'e' ? "ELEMENTS" : (_type == 'r' ? "RENDERED RECTS" : (_type == 's' ? "ELEMENTS' STYLE DATA" : "OTHER")))), 
                        _uncompressedSize, _maxUncompressedSize);
            CRLog::warn(" -> check settings.");
            _maxSizeReachedWarned = true; // warn only once
        }
        _owner->setCacheFileStale(true); // we may write: consider cache file stale
        // do compacting
        int sumsize = reservedSpace;
        for ( ldomTextStorageChunk * p = _recentChunk; p; p = p->_nextRecent ) {
            if ( (int)p->_bufsize + sumsize < _maxUncompressedSize ||
                 (p==_activeChunk && reservedSpace<0xFFFFFFF) || 
                 p == excludedChunk) {
				// fits
				sumsize += p->_bufsize;
			} else {
				if ( !_cache )
					_owner->createCacheFile();
				if ( _cache ) {
					if ( !p->swapToCache(true) ) {
						crFatalError(111, "Swap file writing error!");
					}
				}
			}
        }

    }
#endif
}

// max 512K of uncompressed data (~8 chunks)
#define DEF_MAX_UNCOMPRESSED_SIZE 0x80000
ldomDataStorageManager::ldomDataStorageManager( tinyNodeCollection * owner, char type, lUInt32 maxUnpackedSize, lUInt32 chunkSize )
: _owner( owner )
, _activeChunk(NULL)
, _recentChunk(NULL)
, _cache(NULL)
, _uncompressedSize(0)
, _maxUncompressedSize(maxUnpackedSize)
, _chunkSize(chunkSize)
, _type(type)
, _maxSizeReachedWarned(false)
{
}

ldomDataStorageManager::~ldomDataStorageManager()
{
}

/// create chunk to be read from cache file
ldomTextStorageChunk::ldomTextStorageChunk(ldomDataStorageManager * manager, lUInt16 index, lUInt32 compsize, lUInt32 uncompsize)
	: _manager(manager)
	, _nextRecent(NULL)
	, _prevRecent(NULL)
	, _buf(NULL)   /// buffer for uncompressed data
	, _bufsize(0)    /// _buf (uncompressed) area size, bytes
	, _bufpos(uncompsize)     /// _buf (uncompressed) data write position (for appending of new data)
	, _index(index)      /// ? index of chunk in storage
	, _type( manager->_type )
	, _saved(true)
{
    CR_UNUSED(compsize);
}

ldomTextStorageChunk::ldomTextStorageChunk(lUInt32 preAllocSize, ldomDataStorageManager * manager, lUInt16 index)
	: _manager(manager)
	, _nextRecent(NULL)
	, _prevRecent(NULL)
	, _buf(NULL)   /// buffer for uncompressed data
	, _bufsize(preAllocSize)    /// _buf (uncompressed) area size, bytes
	, _bufpos(preAllocSize)     /// _buf (uncompressed) data write position (for appending of new data)
	, _index(index)      /// ? index of chunk in storage
	, _type( manager->_type )
	, _saved(false)
{
    _buf = (lUInt8*)calloc(preAllocSize, sizeof(*_buf));
    _manager->_uncompressedSize += _bufsize;
}

ldomTextStorageChunk::ldomTextStorageChunk(ldomDataStorageManager * manager, lUInt16 index)
	: _manager(manager)
	, _nextRecent(NULL)
	, _prevRecent(NULL)
	, _buf(NULL)   /// buffer for uncompressed data
	, _bufsize(0)    /// _buf (uncompressed) area size, bytes
	, _bufpos(0)     /// _buf (uncompressed) data write position (for appending of new data)
	, _index(index)      /// ? index of chunk in storage
	, _type( manager->_type )
	, _saved(false)
{
}

#if BUILD_LITE!=1
/// saves data to cache file, if unsaved
bool ldomTextStorageChunk::save()
{
    if ( !_saved )
        return swapToCache(false);
    return true;
}
#endif

ldomTextStorageChunk::~ldomTextStorageChunk()
{
    setunpacked(NULL, 0);
}


#if BUILD_LITE!=1
/// pack data, and remove unpacked, put packed data to cache file
bool ldomTextStorageChunk::swapToCache( bool removeFromMemory )
{
    if ( !_manager->_cache )
        return true;
    if ( _buf ) {
        if ( !_saved && _manager->_cache) {
#if DEBUG_DOM_STORAGE==1
            CRLog::debug("Writing %d bytes of chunk %c%d to cache", _bufpos, _type, _index);
#endif
            if ( !_manager->_cache->write( _manager->cacheType(), _index, _buf, _bufpos, COMPRESS_NODE_STORAGE_DATA) ) {
                CRLog::error("Error while swapping of chunk %c%d to cache file", _type, _index);
                crFatalError(-1, "Error while swapping of chunk to cache file");
                return false;
            }
            _saved = true;
        }
    }
    if ( removeFromMemory ) {
        setunpacked(NULL, 0);
    }
    return true;
}

/// read packed data from cache
bool ldomTextStorageChunk::restoreFromCache()
{
    if ( _buf )
        return true;
    if ( !_saved )
        return false;
    int size;
    if ( !_manager->_cache->read( _manager->cacheType(), _index, _buf, size ) )
        return false;
    _bufsize = size;
    _manager->_uncompressedSize += _bufsize;
#if DEBUG_DOM_STORAGE==1
    CRLog::debug("Read %d bytes of chunk %c%d from cache", _bufsize, _type, _index);
#endif
    return true;
}
#endif

/// get raw data bytes
void ldomTextStorageChunk::getRaw( int offset, int size, lUInt8 * buf )
{
#ifdef _DEBUG
    if ( !_buf || offset+size>(int)_bufpos || offset+size>(int)_bufsize )
        crFatalError(123, "ldomTextStorageChunk: Invalid raw data buffer position");
#endif
    memcpy( buf, _buf+offset, size );
}

/// set raw data bytes
void ldomTextStorageChunk::setRaw( int offset, int size, const lUInt8 * buf )
{
#ifdef _DEBUG
    if ( !_buf || offset+size>(int)_bufpos || offset+size>(int)_bufsize )
        crFatalError(123, "ldomTextStorageChunk: Invalid raw data buffer position");
#endif
    if (memcmp(_buf+offset, buf, size) != 0) {
        memcpy(_buf+offset, buf, size);
        modified();
    }
}


/// returns free space in buffer
int ldomTextStorageChunk::space()
{
    return _bufsize - _bufpos;
}

#if BUILD_LITE!=1
/// returns free space in buffer
int ldomTextStorageChunk::addText( lUInt32 dataIndex, lUInt32 parentIndex, const lString8 & text )
{
    int itemsize = (sizeof(TextDataStorageItem)+text.length()-2 + 15) & 0xFFFFFFF0;
    if ( !_buf ) {
        // create new buffer, if necessary
        _bufsize = _manager->_chunkSize > itemsize ? _manager->_chunkSize : itemsize;
        _buf = (lUInt8*)calloc(_bufsize, sizeof(*_buf));
        _bufpos = 0;
        _manager->_uncompressedSize += _bufsize;
    }
    if ( (int)_bufsize - (int)_bufpos < itemsize )
        return -1;
    TextDataStorageItem * p = (TextDataStorageItem*)(_buf + _bufpos);
    p->sizeDiv16 = (lUInt16)(itemsize >> 4);
    p->dataIndex = dataIndex;
    p->parentIndex = parentIndex;
    p->type = LXML_TEXT_NODE;
    p->length = (lUInt16)text.length();
    memcpy(p->text, text.c_str(), p->length);
    int res = _bufpos >> 4;
    _bufpos += itemsize;
    return res;
}

/// adds new element item to buffer, returns offset inside chunk of stored data
int ldomTextStorageChunk::addElem(lUInt32 dataIndex, lUInt32 parentIndex, int childCount, int attrCount)
{
    int itemsize = (sizeof(ElementDataStorageItem) + attrCount*(sizeof(lUInt16)*2 + sizeof(lUInt32)) + childCount*sizeof(lUInt32) - sizeof(lUInt32) + 15) & 0xFFFFFFF0;
    if ( !_buf ) {
        // create new buffer, if necessary
        _bufsize = _manager->_chunkSize > itemsize ? _manager->_chunkSize : itemsize;
        _buf = (lUInt8*)calloc(_bufsize, sizeof(*_buf));
        _bufpos = 0;
        _manager->_uncompressedSize += _bufsize;
    }
    if ( _bufsize - _bufpos < (unsigned)itemsize )
        return -1;
    ElementDataStorageItem *item = (ElementDataStorageItem *)(_buf + _bufpos);
    if ( item ) {
        item->sizeDiv16 = (lUInt16)(itemsize >> 4);
        item->dataIndex = dataIndex;
        item->parentIndex = parentIndex;
        item->type = LXML_ELEMENT_NODE;
        item->parentIndex = parentIndex;
        item->attrCount = (lUInt16)attrCount;
        item->childCount = childCount;
    }
    int res = _bufpos >> 4;
    _bufpos += itemsize;
    return res;
}

/// set node parent by offset
bool ldomTextStorageChunk::setParent( int offset, lUInt32 parentIndex )
{
    offset <<= 4;
    if ( offset>=0 && offset<(int)_bufpos ) {
        TextDataStorageItem * item = (TextDataStorageItem *)(_buf+offset);
        if ( (int)parentIndex!=item->parentIndex ) {
            item->parentIndex = parentIndex;
            modified();
            return true;
        } else
            return false;
    }
    CRLog::error("Offset %d is out of bounds (%d) for storage chunk %c%d, chunkCount=%d", offset, this->_bufpos, this->_type, this->_index, _manager->_chunks.length() );
    return false;
}


/// get text node parent by offset
lUInt32 ldomTextStorageChunk::getParent( int offset )
{
    offset <<= 4;
    if ( offset>=0 && offset<(int)_bufpos ) {
        TextDataStorageItem * item = (TextDataStorageItem *)(_buf+offset);
        return item->parentIndex;
    }
    CRLog::error("Offset %d is out of bounds (%d) for storage chunk %c%d, chunkCount=%d", offset, this->_bufpos, this->_type, this->_index, _manager->_chunks.length() );
    return 0;
}

/// get pointer to element data
ElementDataStorageItem * ldomTextStorageChunk::getElem( int offset  )
{
    offset <<= 4;
    if ( offset>=0 && offset<(int)_bufpos ) {
        ElementDataStorageItem * item = (ElementDataStorageItem *)(_buf+offset);
        return item;
    }
    CRLog::error("Offset %d is out of bounds (%d) for storage chunk %c%d, chunkCount=%d", offset, this->_bufpos, this->_type, this->_index, _manager->_chunks.length() );
    return NULL;
}
#endif


/// call to invalidate chunk if content is modified
void ldomTextStorageChunk::modified()
{
    if ( !_buf ) {
        CRLog::error("Modified is called for node which is not in memory");
    }
    _saved = false;
}

#if BUILD_LITE!=1
/// free data item
void ldomTextStorageChunk::freeNode( int offset )
{
    offset <<= 4;
    if ( _buf && offset>=0 && offset<(int)_bufpos ) {
        TextDataStorageItem * item = (TextDataStorageItem *)(_buf+offset);
        if ( (item->type==LXML_TEXT_NODE || item->type==LXML_ELEMENT_NODE) && item->dataIndex ) {
            item->type = LXML_NO_DATA;
            item->dataIndex = 0;
            modified();
        }
    }
}

/// get text item from buffer by offset
lString8 ldomTextStorageChunk::getText( int offset )
{
    offset <<= 4;
    if ( _buf && offset>=0 && offset<(int)_bufpos ) {
        TextDataStorageItem * item = (TextDataStorageItem *)(_buf+offset);
        return item->getText8();
    }
    return lString8::empty_str;
}
#endif


/// pack data from _buf to _compbuf
bool ldomPack( const lUInt8 * buf, int bufsize, lUInt8 * &dstbuf, lUInt32 & dstsize )
{
    lUInt8 tmp[PACK_BUF_SIZE]; // 64K buffer for compressed data
    int ret;
    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    ret = deflateInit( &z, DOC_DATA_COMPRESSION_LEVEL );
    if ( ret != Z_OK )
        return false;
    z.avail_in = bufsize;
    z.next_in = (unsigned char *)buf;
    int compressed_size = 0;
    lUInt8 *compressed_buf = NULL;
    while (true) {
        z.avail_out = PACK_BUF_SIZE;
        z.next_out = tmp;
        ret = deflate( &z, Z_FINISH );
        if (ret == Z_STREAM_ERROR) { // some error occured while packing
            deflateEnd(&z);
            if (compressed_buf)
                free(compressed_buf);
            // printf("deflate() error: %d (%d > %d)\n", ret, bufsize, compressed_size);
            return false;
        }
        int have = PACK_BUF_SIZE - z.avail_out;
        compressed_buf = cr_realloc(compressed_buf, compressed_size + have);
        memcpy(compressed_buf + compressed_size, tmp, have );
        compressed_size += have;
        if (z.avail_out != 0) // buffer not fully filled = deflate is done
            break;
        // printf("deflate() additional call needed (%d > %d)\n", bufsize, compressed_size);
    }
    deflateEnd(&z);
    dstsize = compressed_size;
    dstbuf = compressed_buf;
    // printf("deflate() done: %d > %d\n", bufsize, compressed_size);
    return true;
}

/// unpack data from _compbuf to _buf
bool ldomUnpack( const lUInt8 * compbuf, int compsize, lUInt8 * &dstbuf, lUInt32 & dstsize  )
{
    lUInt8 tmp[UNPACK_BUF_SIZE]; // 256K buffer for uncompressed data
    int ret;
    z_stream z = { 0 };
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    ret = inflateInit( &z );
    if ( ret != Z_OK )
        return false;
    z.avail_in = compsize;
    z.next_in = (unsigned char *)compbuf;
    lUInt32 uncompressed_size = 0;
    lUInt8 *uncompressed_buf = NULL;
    while (true) {
        z.avail_out = UNPACK_BUF_SIZE;
        z.next_out = tmp;
        ret = inflate( &z, Z_SYNC_FLUSH );
        if (ret != Z_OK && ret != Z_STREAM_END) { // some error occured while unpacking
            inflateEnd(&z);
            if (uncompressed_buf)
                free(uncompressed_buf);
            // printf("inflate() error: %d (%d > %d)\n", ret, compsize, uncompressed_size);
            return false;
        }
        lUInt32 have = UNPACK_BUF_SIZE - z.avail_out;
        uncompressed_buf = cr_realloc(uncompressed_buf, uncompressed_size + have);
        memcpy(uncompressed_buf + uncompressed_size, tmp, have );
        uncompressed_size += have;
        if (ret == Z_STREAM_END) {
            break;
        }
        // printf("inflate() additional call needed (%d > %d)\n", compsize, uncompressed_size);
    }
    inflateEnd(&z);
    dstsize = uncompressed_size;
    dstbuf = uncompressed_buf;
    // printf("inflate() done %d > %d\n", compsize, uncompressed_size);
    return true;
}

void ldomTextStorageChunk::setunpacked( const lUInt8 * buf, int bufsize )
{
    if ( _buf ) {
        _manager->_uncompressedSize -= _bufsize;
        free(_buf);
        _buf = NULL;
        _bufsize = 0;
    }
    if ( buf && bufsize ) {
        _bufsize = bufsize;
        _bufpos = bufsize;
        _buf = (lUInt8 *)malloc( sizeof(lUInt8) * bufsize );
        _manager->_uncompressedSize += _bufsize;
        memcpy( _buf, buf, bufsize );
    }
}

/// unpacks chunk, if packed; checks storage space, compact if necessary
void ldomTextStorageChunk::ensureUnpacked()
{
#if BUILD_LITE!=1
    if ( !_buf ) {
        if ( _saved ) {
            if ( !restoreFromCache() ) {
                CRTimerUtil timer;
                timer.infinite();
                _manager->_cache->flush(false,timer);
                CRLog::warn( "restoreFromCache() failed for chunk %c%d,will try after flush", _type, _index);
            if ( !restoreFromCache() ) {
                CRLog::error( "restoreFromCache() failed for chunk %c%d", _type, _index);
                crFatalError( 111, "restoreFromCache() failed for chunk");
                }
            }
            _manager->compact( 0, this );
        }
    } else {
        // compact
    }
#endif
}









// moved to .cpp to hide implementation
// fastDOM
class ldomAttributeCollection
{
private:
    lUInt16 _len;
    lUInt16 _size;
    lxmlAttribute * _list;
public:
    ldomAttributeCollection()
    : _len(0), _size(0), _list(NULL)
    {
    }
    ~ldomAttributeCollection()
    {
        if (_list)
            free(_list);
    }
    lxmlAttribute * operator [] (int index) { return &_list[index]; }
    const lxmlAttribute * operator [] (int index) const { return &_list[index]; }
    lUInt16 length() const
    {
        return _len;
    }
    lUInt32 get( lUInt16 nsId, lUInt16 attrId ) const
    {
        for (lUInt16 i=0; i<_len; i++)
        {
            if (_list[i].compare( nsId, attrId ))
                return _list[i].index;
        }
        return LXML_ATTR_VALUE_NONE;
    }
    void set( lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex )
    {
        // find existing
        for (lUInt16 i=0; i<_len; i++)
        {
            if (_list[i].compare( nsId, attrId ))
            {
                _list[i].index = valueIndex;
                return;
            }
        }
        // add
        if (_len>=_size)
        {
            _size += 4;
            _list = cr_realloc( _list, _size );
        }
        _list[ _len++ ].setData(nsId, attrId, valueIndex);
    }
    void add( lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex )
    {
        // find existing
        if (_len>=_size)
        {
            _size += 4;
            _list = cr_realloc( _list, _size );
        }
        _list[ _len++ ].setData(nsId, attrId, valueIndex);
    }
    void add( const lxmlAttribute * v )
    {
        // find existing
        if (_len>=_size)
        {
            _size += 4;
            _list = cr_realloc( _list, _size );
        }
        _list[ _len++ ] = *v;
    }
};


/*
class simpleLogFile
{
public:
    FILE * f;
    simpleLogFile(const char * fname) { f = fopen( fname, "wt" ); }
    ~simpleLogFile() { if (f) fclose(f); }
    simpleLogFile & operator << ( const char * str ) { fprintf( f, "%s", str ); fflush( f ); return *this; }
    simpleLogFile & operator << ( int d ) { fprintf( f, "%d(0x%X) ", d, d ); fflush( f ); return *this; }
    simpleLogFile & operator << ( const wchar_t * str )
    {
        if (str)
        {
            for (; *str; str++ )
            {
                fputc( *str >= 32 && *str<127 ? *str : '?', f );
            }
        }
        fflush( f );
        return *this;
    }
};

simpleLogFile logfile("logfile.log");
*/



/////////////////////////////////////////////////////////////////
/// lxmlDocument


lxmlDocBase::lxmlDocBase(int /*dataBufSize*/)
: tinyNodeCollection(),
_elementNameTable(MAX_ELEMENT_TYPE_ID)
, _attrNameTable(MAX_ATTRIBUTE_TYPE_ID)
, _nsNameTable(MAX_NAMESPACE_TYPE_ID)
, _nextUnknownElementId(UNKNOWN_ELEMENT_TYPE_ID)
, _nextUnknownAttrId(UNKNOWN_ATTRIBUTE_TYPE_ID)
, _nextUnknownNsId(UNKNOWN_NAMESPACE_TYPE_ID)
, _attrValueTable( DOC_STRING_HASH_SIZE )
,_idNodeMap(8192)
,_urlImageMap(1024)
,_idAttrId(0)
,_nameAttrId(0)
#if BUILD_LITE!=1
//,_keepData(false)
//,_mapped(false)
#endif
#if BUILD_LITE!=1
,_pagesData(8192)
#endif
{
    // create and add one data buffer
    _stylesheet.setDocument( this );
}

/// Destructor
lxmlDocBase::~lxmlDocBase()
{
}

void lxmlDocBase::onAttributeSet( lUInt16 attrId, lUInt32 valueId, ldomNode * node )
{
    if ( _idAttrId==0 )
        _idAttrId = _attrNameTable.idByName("id");
    if ( _nameAttrId==0 )
        _nameAttrId = _attrNameTable.idByName("name");
    if (attrId == _idAttrId) {
        _idNodeMap.set( valueId, node->getDataIndex() );
    } else if ( attrId==_nameAttrId ) {
        lString16 nodeName = node->getNodeName();
        if (nodeName == "a")
            _idNodeMap.set( valueId, node->getDataIndex() );
    }
}

lUInt16 lxmlDocBase::getNsNameIndex( const lChar16 * name )
{
    const LDOMNameIdMapItem * item = _nsNameTable.findItem( name );
    if (item)
        return item->id;
    _nsNameTable.AddItem( _nextUnknownNsId, lString16(name), NULL );
    return _nextUnknownNsId++;
}

lUInt16 lxmlDocBase::getNsNameIndex( const lChar8 * name )
{
    const LDOMNameIdMapItem * item = _nsNameTable.findItem( name );
    if (item)
        return item->id;
    _nsNameTable.AddItem( _nextUnknownNsId, lString16(name), NULL );
    return _nextUnknownNsId++;
}

lUInt16 lxmlDocBase::getAttrNameIndex( const lChar16 * name )
{
    const LDOMNameIdMapItem * item = _attrNameTable.findItem( name );
    if (item)
        return item->id;
    _attrNameTable.AddItem( _nextUnknownAttrId, lString16(name), NULL );
    return _nextUnknownAttrId++;
}

lUInt16 lxmlDocBase::getAttrNameIndex( const lChar8 * name )
{
    const LDOMNameIdMapItem * item = _attrNameTable.findItem( name );
    if (item)
        return item->id;
    _attrNameTable.AddItem( _nextUnknownAttrId, lString16(name), NULL );
    return _nextUnknownAttrId++;
}

lUInt16 lxmlDocBase::getElementNameIndex( const lChar16 * name )
{
    const LDOMNameIdMapItem * item = _elementNameTable.findItem( name );
    if (item)
        return item->id;
    _elementNameTable.AddItem( _nextUnknownElementId, lString16(name), NULL );
    return _nextUnknownElementId++;
}

lUInt16 lxmlDocBase::findElementNameIndex( const lChar8 * name )
{
    const LDOMNameIdMapItem * item = _elementNameTable.findItem( name );
    if (item)
        return item->id;
    return 0;
}

lUInt16 lxmlDocBase::getElementNameIndex( const lChar8 * name )
{
    const LDOMNameIdMapItem * item = _elementNameTable.findItem( name );
    if (item)
        return item->id;
    _elementNameTable.AddItem( _nextUnknownElementId, lString16(name), NULL );
    return _nextUnknownElementId++;
}

/// create formatted text object with options set
LFormattedText * lxmlDocBase::createFormattedText()
{
    LFormattedText * p = new LFormattedText();
    p->setImageScalingOptions(&_imgScalingOptions);
    p->setSpaceWidthScalePercent(_spaceWidthScalePercent);
    p->setMinSpaceCondensingPercent(_minSpaceCondensingPercent);
    p->setUnusedSpaceThresholdPercent(_unusedSpaceThresholdPercent);
    p->setMaxAddedLetterSpacingPercent(_maxAddedLetterSpacingPercent);
    p->setHighlightOptions(&_highlightOptions);
    return p;
}

/// returns main element (i.e. FictionBook for FB2)
ldomNode * lxmlDocBase::getRootNode()
{
    return getTinyNode(17);
}

ldomDocument::ldomDocument()
: lxmlDocBase(DEF_DOC_DATA_BUFFER_SIZE),
  m_toc(this)
, m_pagemap(this)
#if BUILD_LITE!=1
, _last_docflags(0)
, _page_height(0)
, _page_width(0)
, _rendered(false)
, _just_rendered_from_cache(false)
, _toc_from_cache_valid(false)
#endif
, lists(100)
{
    _docIndex = ldomNode::registerDocument(this);
    allocTinyElement(NULL, 0, 0);
    // Note: valgrind reports (sometimes, when some document is opened or closed,
    // with metadataOnly or not) a memory leak (64 bytes in 1 blocks are definitely
    // lost), about this, created in allocTinyElement():
    //    tinyElement * elem = new tinyElement(...)
    // possibly because it's not anchored anywhere.
    // Attempt at anchoring into a _nullNode, and calling ->detroy()
    // in ~ldomDocument(), did not prevent this report, and caused other ones...

    //new ldomElement( this, NULL, 0, 0, 0 );
    //assert( _instanceMapCount==2 );
}

/// Copy constructor - copies ID tables contents
lxmlDocBase::lxmlDocBase( lxmlDocBase & doc )
:    tinyNodeCollection(doc)
,   _elementNameTable(doc._elementNameTable)    // Element Name<->Id map
,   _attrNameTable(doc._attrNameTable)       // Attribute Name<->Id map
,   _nsNameTable(doc._nsNameTable)           // Namespace Name<->Id map
,   _nextUnknownElementId(doc._nextUnknownElementId) // Next Id for unknown element
,   _nextUnknownAttrId(doc._nextUnknownAttrId)    // Next Id for unknown attribute
,   _nextUnknownNsId(doc._nextUnknownNsId)      // Next Id for unknown namespace
    //lvdomStyleCache _styleCache;         // Style cache
,   _attrValueTable(doc._attrValueTable)
,   _idNodeMap(doc._idNodeMap)
,   _urlImageMap(1024)
,   _idAttrId(doc._idAttrId) // Id for "id" attribute name
//,   _docFlags(doc._docFlags)
#if BUILD_LITE!=1
,   _pagesData(8192)
#endif
{
}

/// creates empty document which is ready to be copy target of doc partial contents
ldomDocument::ldomDocument( ldomDocument & doc )
: lxmlDocBase(doc)
, m_toc(this)
, m_pagemap(this)
#if BUILD_LITE!=1
, _def_font(doc._def_font) // default font
, _def_style(doc._def_style)
, _last_docflags(doc._last_docflags)
, _page_height(doc._page_height)
, _page_width(doc._page_width)
#endif
, _container(doc._container)
, lists(100)
{
    _docIndex = ldomNode::registerDocument(this);
}

static void writeNode( LVStream * stream, ldomNode * node, bool treeLayout )
{
    int level = 0;
    if ( treeLayout ) {
        level = node->getNodeLevel();
        for (int i=0; i<level; i++ )
            *stream << "  ";
    }
    if ( node->isText() )
    {
        lString8 txt = node->getText8();
        *stream << txt;
        if ( treeLayout )
            *stream << "\n";
    }
    else if (  node->isElement() )
    {
        lString8 elemName = UnicodeToUtf8(node->getNodeName());
        lString8 elemNsName = UnicodeToUtf8(node->getNodeNsName());
        if (!elemNsName.empty())
            elemName = elemNsName + ":" + elemName;
        if (!elemName.empty())
            *stream << "<" << elemName;
        int i;
        for (i=0; i<(int)node->getAttrCount(); i++)
        {
            const lxmlAttribute * attr = node->getAttribute(i);
            if (attr)
            {
                lString8 attrName( UnicodeToUtf8(node->getDocument()->getAttrName(attr->id)) );
                lString8 nsName( UnicodeToUtf8(node->getDocument()->getNsName(attr->nsid)) );
                lString8 attrValue( UnicodeToUtf8(node->getDocument()->getAttrValue(attr->index)) );
                *stream << " ";
                if ( nsName.length() > 0 )
                    *stream << nsName << ":";
                *stream << attrName << "=\"" << attrValue << "\"";
            }
        }

#if 0
            if (!elemName.empty())
            {
                ldomNode * elem = node;
                lvdomElementFormatRec * fmt = elem->getRenderData();
                css_style_ref_t style = elem->getStyle();
                if ( fmt ) {
                    lvRect rect;
                    elem->getAbsRect( rect );
                    *stream << L" fmt=\"";
                    *stream << L"rm:" << lString16::itoa( (int)elem->getRendMethod() ) << L" ";
                    if ( style.isNull() )
                        *stream << L"style: NULL ";
                    else {
                        *stream << L"disp:" << lString16::itoa( (int)style->display ) << L" ";
                    }
                    *stream << L"y:" << lString16::itoa( (int)fmt->getY() ) << L" ";
                    *stream << L"h:" << lString16::itoa( (int)fmt->getHeight() ) << L" ";
                    *stream << L"ay:" << lString16::itoa( (int)rect.top ) << L" ";
                    *stream << L"ah:" << lString16::itoa( (int)rect.height() ) << L" ";
                    *stream << L"\"";
                }
            }
#endif

        if ( node->getChildCount() == 0 ) {
            if (!elemName.empty())
            {
                if ( elemName[0] == '?' )
                    *stream << "?>";
                else
                    *stream << "/>";
            }
            if ( treeLayout )
                *stream << "\n";
        } else {
            if (!elemName.empty())
                *stream << ">";
            if ( treeLayout )
                *stream << "\n";
            for (i=0; i<(int)node->getChildCount(); i++)
            {
                writeNode( stream, node->getChildNode(i), treeLayout );
            }
            if ( treeLayout ) {
                for (int i=0; i<level; i++ )
                    *stream << "  ";
            }
            if (!elemName.empty())
                *stream << "</" << elemName << ">";
            if ( treeLayout )
                *stream << "\n";
        }
    }
}

// Extended version of previous function for displaying selection HTML, with tunable output
#define WRITENODEEX_TEXT_HYPHENATE               0x0001 ///< add soft-hyphens where hyphenation is allowed
#define WRITENODEEX_TEXT_MARK_NODE_BOUNDARIES    0x0002 ///< mark start and end of text nodes (useful when indented)
#define WRITENODEEX_TEXT_SHOW_UNICODE_CODEPOINT  0x0004 ///< show unicode codepoint after char
#define WRITENODEEX_TEXT_UNESCAPED               0x0008 ///< let &, < and > unescaped in text nodes (makes HTML invalid)
#define WRITENODEEX_INDENT_NEWLINE               0x0010 ///< indent newlines according to node level
#define WRITENODEEX_NEWLINE_BLOCK_NODES          0x0020 ///< start only nodes rendered as block/final on a new line,
                                                        ///  so inline elements and text nodes are stuck together
#define WRITENODEEX_NEWLINE_ALL_NODES            0x0040 ///< start all nodes on a new line
#define WRITENODEEX_UNUSED_1                     0x0080 ///<
#define WRITENODEEX_NB_SKIPPED_CHARS             0x0100 ///< show number of skipped chars in text nodes: (...43...)
#define WRITENODEEX_NB_SKIPPED_NODES             0x0200 ///< show number of skipped sibling nodes: [...17...]
#define WRITENODEEX_SHOW_REND_METHOD             0x0400 ///< show rendering method at end of tag (<div ~F> =Final, <b ~i>=Inline...)
#define WRITENODEEX_SHOW_MISC_INFO               0x0800 ///< show additional info (depend on context)
#define WRITENODEEX_ADD_UPPER_DIR_LANG_ATTR      0x1000 ///< add dir= and lang= grabbed from upper nodes
#define WRITENODEEX_GET_CSS_FILES                0x2000 ///< ensure css files that apply to initial node are returned
                                                        ///  in &cssFiles (needed when not starting from root node)
#define WRITENODEEX_INCLUDE_STYLESHEET_ELEMENT   0x4000 ///< includes crengine <stylesheet> element in HTML
                                                        ///  (not done if outside of sub-tree)
#define WRITENODEEX_COMPUTED_STYLES_AS_ATTR      0x8000 ///< set style='' from computed styles (not implemented)


#define WNEFLAG(x) ( wflags & WRITENODEEX_##x )

static void writeNodeEx( LVStream * stream, ldomNode * node, lString16Collection & cssFiles, int wflags=0,
    ldomXPointerEx startXP=ldomXPointerEx(), ldomXPointerEx endXP=ldomXPointerEx(), int indentBaseLevel=-1)
{
    bool isStartNode = false;
    bool isEndNode = false;
    bool isAfterStart = false;
    bool isBeforeEnd = false;
    bool containsStart = false;
    bool containsEnd = false;

    if ( !startXP.isNull() && !endXP.isNull() ) {
        ldomXPointerEx currentEXP = ldomXPointerEx(node, 0);
        // Use start (offset=0) of text node for comparisons, but keep original XPointers
        ldomXPointerEx startEXP = ldomXPointerEx( startXP );
        startEXP.setOffset(0);
        ldomXPointerEx endEXP = ldomXPointerEx( endXP );
        endEXP.setOffset(0);
        if (currentEXP == startEXP)
            isStartNode = true;
        if (currentEXP == endEXP)
            isEndNode = true;
        if ( currentEXP.compare( startEXP ) >= 0 ) {
            isAfterStart = true;
        }
        if ( currentEXP.compare( endEXP ) <= 0 ) {
            isBeforeEnd = true;
        }
        ldomNode *tmp;
        tmp = startXP.getNode();
        while (tmp) {
            if (tmp == node) {
                containsStart = true;
                break;
            }
            tmp = tmp->getParentNode();
        }
        tmp = endXP.getNode();
        while (tmp) {
            if (tmp == node) {
                containsEnd = true;
                break;
            }
            tmp = tmp->getParentNode();
        }
    }
    else {
        containsStart = true;
        containsEnd = true;
        isAfterStart = true;
        isBeforeEnd = true;
        // but not isStartNode nor isEndNode, as these use startXP and endXP
    }

    bool isInitialNode = false;
    lString16 initialDirAttribute = lString16::empty_str;
    lString16 initialLangAttribute = lString16::empty_str;
    if (indentBaseLevel < 0) { // initial call (recursive ones will have it >=0)
        indentBaseLevel = node->getNodeLevel();
        isInitialNode = true;
        if ( WNEFLAG(ADD_UPPER_DIR_LANG_ATTR) && !node->isRoot() ) {
            // Grab any dir="rtl" and lang="ar_AA" attributes from some parent node
            if ( !node->hasAttribute( attr_dir ) ) {
                ldomNode *pnode = node->getParentNode();
                for ( ; pnode && !pnode->isNull() && !pnode->isRoot(); pnode = pnode->getParentNode() ) {
                    if ( pnode->hasAttribute(attr_dir) ) {
                        initialDirAttribute = pnode->getAttributeValue(attr_dir);
                        break;
                    }
                }
            }
            if ( !node->hasAttribute( attr_lang ) ) {
                ldomNode *pnode = node->getParentNode();
                for ( ; pnode && !pnode->isNull() && !pnode->isRoot(); pnode = pnode->getParentNode() ) {
                    if ( pnode->hasAttribute(attr_lang) ) {
                        initialLangAttribute = pnode->getAttributeValue(attr_lang);
                        break;
                    }
                }
            }
        }
    }
    int level = node->getNodeLevel();
    if ( node->isText() && isAfterStart && isBeforeEnd ) {
        bool doNewLine =  WNEFLAG(NEWLINE_ALL_NODES);
        bool doIndent = doNewLine && WNEFLAG(INDENT_NEWLINE);
        lString16 txt = node->getText();
        lString8 prefix = lString8::empty_str;
        lString8 suffix = lString8::empty_str;

        if ( isEndNode ) {
            // show the number of chars not written after selection "(...n...)"
            int nodeLength = endXP.getText().length();
            int endOffset = endXP.getOffset();
            if (endOffset < nodeLength) {
                txt = txt.substr(0, endOffset);
                if ( WNEFLAG(NB_SKIPPED_CHARS) )
                    suffix << "(…" << lString8().appendDecimal(nodeLength-endOffset) << "…)";
            }
        }
        if ( WNEFLAG(TEXT_MARK_NODE_BOUNDARIES) ) {
            // We use non-ordinary chars to mark start and end of text
            // node, which can help noticing spaces at start or end
            // when NEWLINE_ALL_NODES and INDENT_NEWLINE are used.
            // Some candidates chars are:
            //   Greyish, discreet, but may be confused with parenthesis:
            //     prefix << "⟨"; // U+27E8 Mathematical Left Angle Bracket
            //     suffix << "⟩"; // U+27E9 Mathematical Right Angle Bracket
            //   Greyish, a bit less discreet, but won't be confused with any other casual char:
            //     prefix << "⟪"; // U+27EA Mathematical Left Double Angle Bracket
            //     suffix << "⟫"; // U+27EB Mathematical Right Double Angle Bracket
            //   A bit too dark, but won't be confused with any other casual char:
            //     prefix << "⎛"; // U+239B Left Parenthesis Upper Hook
            //     suffix << "⎠"; // U+23A0 Right Parenthesis Lower Hook (may have too much leading space)
            prefix << "⟪"; // U+27EA Mathematical Left Double Angle Bracket
            suffix << "⟫"; // U+27EB Mathematical Right Double Angle Bracket
        }
        if ( isStartNode ) {
            // show the number of chars not written before selection "(...n...)"
            int offset = startXP.getOffset();
            if (offset > 0) {
                txt = txt.substr(offset);
                if ( WNEFLAG(NB_SKIPPED_CHARS) )
                    prefix << "(…" << lString8().appendDecimal(offset) << "…)";
            }
            if ( WNEFLAG(NB_SKIPPED_NODES) ) {
                // show the number of sibling nodes not written before selection "[...n..]"
                int nbIgnoredPrecedingSiblings = node->getNodeIndex();
                if (nbIgnoredPrecedingSiblings) {
                    if (doIndent)
                        for ( int i=indentBaseLevel; i<level; i++ )
                            *stream << "  ";
                    *stream << "[…" << lString8().appendDecimal(nbIgnoredPrecedingSiblings) << "…]";
                    if (doNewLine)
                        *stream << "\n";
                }
            }
        }
        if (doIndent)
            for ( int i=indentBaseLevel; i<level; i++ )
                *stream << "  ";
        if ( ! WNEFLAG(TEXT_UNESCAPED) ) {
            // Use a temporary char we're not likely to find in the DOM
            // (see https://en.wikipedia.org/wiki/Specials_(Unicode_block) )
            // for 2-steps '&' replacement (to avoid infinite loop or the
            // need for more complicated code)
            while ( txt.replace( cs16("&"), cs16(L"\xFFFF") ) ) ;
            while ( txt.replace( cs16(L"\xFFFF"), cs16("&amp;") ) ) ;
            while ( txt.replace( cs16("<"), cs16("&lt;") ) ) ;
            while ( txt.replace( cs16(">"), cs16("&gt;") ) ) ;
        }
        #define HYPH_MIN_WORD_LEN_TO_HYPHENATE 4
        #define HYPH_MAX_WORD_SIZE 64
        // (No hyphenation if we are showing unicode codepoint)
        if ( WNEFLAG(TEXT_SHOW_UNICODE_CODEPOINT) ) {
            *stream << prefix;
            for ( int i=0; i<txt.length(); i++ )
                *stream << UnicodeToUtf8(txt.substr(i, 1)) << "⟨U+" << lString8().appendHex(txt[i]) << "⟩";
            *stream << suffix;
        }
        else if ( WNEFLAG(TEXT_HYPHENATE) && HyphMan::isEnabled() && txt.length() >= HYPH_MIN_WORD_LEN_TO_HYPHENATE ) {
            // Add soft-hyphens where HyphMan (with the user or language current hyphenation
            // settings) says hyphenation is allowed.
            // We do that here while we output the text to avoid the need
            // for temporary storage of a string with soft-hyphens added.
            const lChar16 * text16 = txt.c_str();
            int txtlen = txt.length();
            lUInt8 * flags = (lUInt8*)calloc(txtlen, sizeof(*flags));
            lUInt16 widths[HYPH_MAX_WORD_SIZE] = { 0 }; // array needed by hyphenate()
            // Lookup words starting from the end, just because lStr_findWordBounds()
            // will ensure the iteration that way.
            int wordpos = txtlen;
            while ( wordpos > 0 ) {
                // lStr_findWordBounds() will find the word contained at wordpos
                // (or the previous word if wordpos happens to be a space or some
                // punctuation) by looking only for alpha chars in m_text.
                int start, end;
                lStr_findWordBounds( text16, txtlen, wordpos, start, end );
                if ( end <= HYPH_MIN_WORD_LEN_TO_HYPHENATE ) {
                    // Too short word at start, we're done
                    break;
                }
                int len = end - start;
                if ( len < HYPH_MIN_WORD_LEN_TO_HYPHENATE ) {
                    // Too short word found, skip it
                    wordpos = start - 1;
                    continue;
                }
                if ( start >= wordpos ) {
                    // Shouldn't happen, but let's be sure we don't get stuck
                    wordpos = wordpos - HYPH_MIN_WORD_LEN_TO_HYPHENATE;
                    continue;
                }
                // We have a valid word to look for hyphenation
                if ( len > HYPH_MAX_WORD_SIZE ) // hyphenate() stops/truncates at 64 chars
                    len = HYPH_MAX_WORD_SIZE;
                // Have hyphenate() set flags inside 'flags'
                // (Fetching the lang_cfg for each text node is not really cheap, but
                // it's easier than having to pass it to each writeNodeEx())
                TextLangMan::getTextLangCfg(node)->getHyphMethod()->hyphenate(text16+start, len, widths, flags+start, 0, 0xFFFF, 1);
                // Continue with previous word
                wordpos = start - 1;
            }
            // Output text, and add a soft-hyphen where there are flags
            *stream << prefix;
            for ( int i=0; i<txt.length(); i++ ) {
                *stream << UnicodeToUtf8(txt.substr(i, 1));
                if ( flags[i] & LCHAR_ALLOW_HYPH_WRAP_AFTER )
                    *stream << "­";
            }
            *stream << suffix;
            free(flags);
        }
        else {
            *stream << prefix << UnicodeToUtf8(txt) << suffix;
        }
        if (doNewLine)
            *stream << "\n";
        if ( isEndNode && WNEFLAG(NB_SKIPPED_NODES) ) {
            // show the number of sibling nodes not written after selection "[...n..]"
            ldomNode * parent = node->getParentNode();
            int nbIgnoredFollowingSiblings = parent ? (parent->getChildCount() - 1 - node->getNodeIndex()) : 0;
            if (nbIgnoredFollowingSiblings) {
                if (doIndent)
                    for ( int i=indentBaseLevel; i<level; i++ )
                        *stream << "  ";
                *stream << "[…" << lString8().appendDecimal(nbIgnoredFollowingSiblings) << "…]";
                if (doNewLine)
                    *stream << "\n";
            }
        }
    }
    else if ( node->isElement() ) {
        lString8 elemName = UnicodeToUtf8(node->getNodeName());
        lString8 elemNsName = UnicodeToUtf8(node->getNodeNsName());
        // Write elements that are between start and end, but also those that
        // are parents of start and end nodes
        bool toWrite = (isAfterStart && isBeforeEnd) || containsStart || containsEnd;
        bool isStylesheetTag = false;
        if ( node->getNodeId() == el_stylesheet ) {
            toWrite = false;
            if ( WNEFLAG(INCLUDE_STYLESHEET_ELEMENT) ) {
                // We may meet a <stylesheet> tag that is not between startXP and endXP and
                // does not contain any of them, but its parent (body or DocFragment) does.
                // Write it if requested, as it's useful when inspecting HTML.
                toWrite = true;
                isStylesheetTag = true; // for specific parsing and writting
            }
        }
        if ( ! toWrite )
            return;

        bool doNewLineBeforeStartTag = false;
        bool doNewLineAfterStartTag = false;
        bool doNewLineBeforeEndTag = false; // always stays false, newline done by child elements
        bool doNewLineAfterEndTag = false;
        bool doIndentBeforeStartTag = false;
        bool doIndentBeforeEndTag = false;
        // Specific for floats and inline-blocks among inlines inside final, that
        // we want to show on their own lines:
        bool doNewlineBeforeIndentBeforeStartTag = false;
        bool doIndentAfterNewLineAfterEndTag = false;
        bool doIndentOneLevelLessAfterNewLineAfterEndTag = false;
        if ( WNEFLAG(NEWLINE_ALL_NODES) ) {
            doNewLineBeforeStartTag = true;
            doNewLineAfterStartTag = true;
            // doNewLineBeforeEndTag = false; // done by child elements
            doNewLineAfterEndTag = true;
            doIndentBeforeStartTag = WNEFLAG(INDENT_NEWLINE);
            doIndentBeforeEndTag = WNEFLAG(INDENT_NEWLINE);
        }
        else if ( WNEFLAG(NEWLINE_BLOCK_NODES) ) {
            // We consider block elements according to crengine decision for their
            // rendering method, which gives us a visual hint of it.
            lvdom_element_render_method rm = node->getRendMethod();
            // Text and inline nodes stay stuck together, but not all others
            if (rm == erm_invisible) {
                // We don't know how invisible nodes would be displayed if
                // they were visible. Make the invisible tree like inline
                // among finals, so they don't take too much height.
                if (node->getParentNode()) {
                    rm = node->getParentNode()->getRendMethod();
                    if (rm == erm_invisible || rm == erm_inline || rm == erm_final)
                        rm = erm_inline;
                    else
                        rm = erm_final;
                }
            }
            if ( (rm != erm_inline && rm != erm_runin) || node->isBoxingInlineBox()) {
                doNewLineBeforeStartTag = true;
                doNewLineAfterStartTag = true;
                // doNewLineBeforeEndTag = false; // done by child elements
                doNewLineAfterEndTag = true;
                doIndentBeforeStartTag = WNEFLAG(INDENT_NEWLINE);
                doIndentBeforeEndTag = WNEFLAG(INDENT_NEWLINE);
                if (rm == erm_final) {
                    // Nodes with rend method erm_final contain only text and inline nodes.
                    // We want these erm_final indented, but not their content
                    doNewLineAfterStartTag = false;
                    doIndentBeforeEndTag = false;
                }
                else if (node->isFloatingBox()) {
                    lvdom_element_render_method prm = node->getParentNode()->getRendMethod();
                    if (prm == erm_final || prm == erm_inline) {
                        doNewlineBeforeIndentBeforeStartTag = true;
                        doIndentAfterNewLineAfterEndTag = WNEFLAG(INDENT_NEWLINE);
                        // If we're the last node in parent collection, indent one level less,
                        // so that next node (the parent) is not at this node level
                        ldomNode * parent = node->getParentNode();
                        if ( parent && (node->getNodeIndex() == parent->getChildCount()-1) )
                            doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                        else if ( parent && (node->getNodeIndex() == parent->getChildCount()-2)
                                         && parent->getChildNode(parent->getChildCount()-1)->isText() )
                            doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                        else if ( containsEnd ) // same if next siblings won't be shown
                            doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                        // But if previous sibling node is a floating or boxing inline node
                        // that have done what we just did, cancel some of what we did
                        if ( node->getNodeIndex() > 0 ) {
                            ldomNode * prevsibling = parent->getChildNode(node->getNodeIndex()-1);
                            if ( prevsibling->isFloatingBox() || prevsibling->isBoxingInlineBox() ) {
                                doNewlineBeforeIndentBeforeStartTag = false;
                                doIndentBeforeStartTag = false;
                            }
                        }
                    }
                }
                else if (node->isBoxingInlineBox()) {
                    doNewlineBeforeIndentBeforeStartTag = true;
                    doIndentAfterNewLineAfterEndTag = WNEFLAG(INDENT_NEWLINE);
                    // Same as above
                    ldomNode * parent = node->getParentNode();
                    if ( parent && (node->getNodeIndex() == parent->getChildCount()-1) )
                        doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                    else if ( parent && (node->getNodeIndex() == parent->getChildCount()-2)
                                     && parent->getChildNode(parent->getChildCount()-1)->isText() )
                        doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                    else if ( containsEnd )
                        doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                    if ( node->getNodeIndex() > 0 ) {
                        ldomNode * prevsibling = parent->getChildNode(node->getNodeIndex()-1);
                        if ( prevsibling->isFloatingBox() || prevsibling->isBoxingInlineBox() ) {
                            doNewlineBeforeIndentBeforeStartTag = false;
                            doIndentBeforeStartTag = false;
                        }
                    }
                }
            }
        }

        if ( containsStart && WNEFLAG(NB_SKIPPED_NODES) ) {
            // Previous siblings did not contain startXP: show how many they are
            int nbIgnoredPrecedingSiblings = node->getNodeIndex();
            if (nbIgnoredPrecedingSiblings && WNEFLAG(INCLUDE_STYLESHEET_ELEMENT) &&
                    node->getParentNode()->getFirstChild()->isElement() &&
                    node->getParentNode()->getFirstChild()->getNodeId() == el_stylesheet) {
                nbIgnoredPrecedingSiblings--; // we have written the <stylesheet> tag
            }
            if (nbIgnoredPrecedingSiblings) {
                if (doIndentBeforeStartTag)
                    for ( int i=indentBaseLevel; i<level; i++ )
                        *stream << "  ";
                *stream << "[…" << lString8().appendDecimal(nbIgnoredPrecedingSiblings) << "…]";
                if (doNewLineBeforeStartTag)
                    *stream << "\n";
            }
        }
        if (doNewlineBeforeIndentBeforeStartTag)
            *stream << "\n";
        if (doIndentBeforeStartTag)
            for ( int i=indentBaseLevel; i<level; i++ )
                *stream << "  ";
        if ( elemName.empty() ) // should not happen (except for the root node, that we hopefully skipped)
            elemName = elemNsName + "???";
        if ( !elemNsName.empty() )
            elemName = elemNsName + ":" + elemName;
        *stream << "<" << elemName;
        if ( isInitialNode ) {
            // Add any dir="rtl" and lang="ar_AA" attributes grabbed from some parent node
            if ( !initialDirAttribute.empty() ) {
                *stream << " dir=\"" << UnicodeToUtf8(initialDirAttribute) << "\"";
            }
            if ( !initialLangAttribute.empty() ) {
                *stream << " lang=\"" << UnicodeToUtf8(initialLangAttribute) << "\"";
            }
        }
        for ( int i=0; i<(int)node->getAttrCount(); i++ ) {
            const lxmlAttribute * attr = node->getAttribute(i);
            if (attr) {
                lString8 attrName( UnicodeToUtf8(node->getDocument()->getAttrName(attr->id)) );
                lString8 nsName( UnicodeToUtf8(node->getDocument()->getNsName(attr->nsid)) );
                lString8 attrValue( UnicodeToUtf8(node->getDocument()->getAttrValue(attr->index)) );
                if ( WNEFLAG(SHOW_MISC_INFO) ) {
                    if ( node->getNodeId() == el_pseudoElem && (attr->id == attr_Before || attr->id == attr_After) ) {
                        // Show the rendered content as the otherwise empty Before/After attribute value
                        if ( WNEFLAG(TEXT_SHOW_UNICODE_CODEPOINT) ) {
                            lString16 content = get_applied_content_property(node);
                            attrValue.empty();
                            for ( int i=0; i<content.length(); i++ ) {
                                attrValue << UnicodeToUtf8(content.substr(i, 1)) << "⟨U+" << lString8().appendHex(content[i]) << "⟩";
                            }
                        }
                        else {
                            attrValue = UnicodeToUtf8(get_applied_content_property(node));
                        }
                    }
                }
                *stream << " ";
                if ( nsName.length() > 0 )
                    *stream << nsName << ":";
                *stream << attrName << "=\"" << attrValue << "\"";
                if ( attrName == "StyleSheet" ) { // gather linked css files
                    lString16 cssFile = node->getDocument()->getAttrValue(attr->index);
                    if (!cssFiles.contains(cssFile))
                        cssFiles.add(cssFile);
                }
            }
        }
        if ( WNEFLAG(SHOW_REND_METHOD) ) {
            *stream << " ~";
            switch ( node->getRendMethod() ) {
                case erm_invisible:          *stream << "X";     break;
                case erm_killed:             *stream << "K";     break;
                case erm_block:              *stream << "B";     break;
                case erm_final:              *stream << "F";     break;
                case erm_inline:             *stream << "i";     break;
                case erm_mixed:              *stream << "M";     break; // not implemented
                case erm_list_item:          *stream << "L";     break; // no more used
                case erm_table:              *stream << "T";     break;
                case erm_table_row_group:    *stream << "TRG";   break;
                case erm_table_header_group: *stream << "THG";   break;
                case erm_table_footer_group: *stream << "TFG";   break;
                case erm_table_row:          *stream << "TR";    break;
                case erm_table_column_group: *stream << "TCG";   break;
                case erm_table_column:       *stream << "TC";    break;
                case erm_table_cell:         *stream << "tcell"; break; // never stays erm_table_cell (becomes block or final)
                case erm_table_caption:      *stream << "tcap";  break;
                case erm_runin:              *stream << "R";     break;
                default:                     *stream << "?";     break;
            }
        }
        if ( node->getChildCount() == 0 ) {
            if ( elemName[0] == '?' )
                *stream << "?>";
            else
                *stream << "/>";
        }
        else {
            *stream << ">";
            if (doNewLineAfterStartTag)
                *stream << "\n";
            if ( ! isStylesheetTag ) {
                for ( int i=0; i<(int)node->getChildCount(); i++ ) {
                    writeNodeEx( stream, node->getChildNode(i), cssFiles, wflags, startXP, endXP, indentBaseLevel );
                }
            }
            else {
                // We need to parse the stylesheet tag text to extract css files path.
                // We write its content without indentation and add a \n for readability.
                lString8 txt = node->getText8();
                int txtlen = txt.length();
                if (txtlen && txt.substr(txtlen-1) != "\n") {
                    txt << "\n";
                }
                *stream << txt;
                // Parse @import'ed files to gather linked css files (we don't really need to
                // do recursive parsing of @import, which are very rare, we just want to get
                // the 2nd++ linked css files that were put there by crengine).
                const char * s = txt.c_str();
                while (true) {
                    lString8 import_file;
                    if ( ! LVProcessStyleSheetImport( s, import_file ) ) {
                        break;
                    }
                    lString16 cssFile = LVCombinePaths( node->getAttributeValue(attr_href), Utf8ToUnicode(import_file) );
                    if ( !cssFile.empty() && !cssFiles.contains(cssFile) ) {
                        cssFiles.add(cssFile);
                    }
                }
            }
            if (doNewLineBeforeEndTag)
                *stream << "\n";
            if (doIndentBeforeEndTag)
                for ( int i=indentBaseLevel; i<level; i++ )
                    *stream << "  ";
            *stream << "</" << elemName << ">";
        }
        if (doNewLineAfterEndTag)
            *stream << "\n";
        if (doIndentAfterNewLineAfterEndTag) {
            int ilevel = doIndentOneLevelLessAfterNewLineAfterEndTag ? level-1 : level;
            for ( int i=indentBaseLevel; i<ilevel; i++ )
                *stream << "  ";
        }
        if ( containsEnd && WNEFLAG(NB_SKIPPED_NODES) ) {
            // Next siblings will not contain endXP and won't be written: show how many they are
            ldomNode * parent = node->getParentNode();
            int nbIgnoredFollowingSiblings = parent ? (parent->getChildCount() - 1 - node->getNodeIndex()) : 0;
            if (nbIgnoredFollowingSiblings) {
                if (doIndentBeforeEndTag)
                    for ( int i=indentBaseLevel; i<level; i++ )
                        *stream << "  ";
                *stream << "[…" << lString8().appendDecimal(nbIgnoredFollowingSiblings) << "…]";
                if (doNewLineAfterEndTag)
                    *stream << "\n";
            }
        }
        if ( isInitialNode && cssFiles.length()==0 && WNEFLAG(GET_CSS_FILES) && !node->isRoot() ) {
            // We have gathered CSS files as we walked the DOM, which we usually
            // do from the root node if we want CSS files.
            // In case we started from an inner node, and we are requested for
            // CSS files - but we have none - walk the DOM back to gather them.
            ldomNode *pnode = node->getParentNode();
            for ( ; pnode && !pnode->isNull() && !pnode->isRoot(); pnode = pnode->getParentNode() ) {
                if ( pnode->getNodeId() == el_DocFragment || pnode->getNodeId() == el_body ) {
                    // The CSS file in StyleSheet="" attribute was the first one seen by
                    // crengine, so add it first to cssFiles
                    if (pnode->hasAttribute(attr_StyleSheet) ) {
                        lString16 cssFile = pnode->getAttributeValue(attr_StyleSheet);
                        if (!cssFiles.contains(cssFile))
                            cssFiles.add(cssFile);
                    }
                    // And then the CSS files in @import in the <stylesheet> element
                    if ( pnode->getChildCount() > 0 ) {
                        ldomNode *styleNode = pnode->getFirstChild();
                        if ( styleNode && styleNode->getNodeId()==el_stylesheet ) {
                            // Do as done above
                            lString8 txt = pnode->getText8();
                            const char * s = txt.c_str();
                            while (true) {
                                lString8 import_file;
                                if ( ! LVProcessStyleSheetImport( s, import_file ) ) {
                                    break;
                                }
                                lString16 cssFile = LVCombinePaths( pnode->getAttributeValue(attr_href), Utf8ToUnicode(import_file) );
                                if ( !cssFile.empty() && !cssFiles.contains(cssFile) ) {
                                    cssFiles.add(cssFile);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

bool ldomDocument::saveToStream( LVStreamRef stream, const char *, bool treeLayout )
{
    //CRLog::trace("ldomDocument::saveToStream()");
    if (!stream || !getRootNode()->getChildCount())
        return false;

    *stream.get() << UnicodeToLocal(cs16(L"\xFEFF"));
    writeNode( stream.get(), getRootNode(), treeLayout );
    return true;
}

ldomDocument::~ldomDocument()
{
    ldomNode::unregisterDocument(this);
    fontMan->UnregisterDocumentFonts(_docIndex);
#if BUILD_LITE!=1
    updateMap();
#endif
}

#if BUILD_LITE!=1

class LVImportStylesheetParser
{
public:
    LVImportStylesheetParser(ldomDocument *document) :
        _document(document), _nestingLevel(0)
    {
    }

    ~LVImportStylesheetParser()
    {
        _inProgress.clear();
    }

    bool Parse(lString16 cssFile)
    {
        bool ret = false;
        if ( cssFile.empty() )
            return ret;

        lString16 codeBase = cssFile;
        LVExtractLastPathElement(codeBase);
        LVStreamRef cssStream = _document->getContainer()->OpenStream(cssFile.c_str(), LVOM_READ);
        if ( !cssStream.isNull() ) {
            lString16 css;
            css << LVReadTextFile( cssStream );
            int offset = _inProgress.add(cssFile);
            ret = Parse(codeBase, css) || ret;
            _inProgress.erase(offset, 1);
        }
        return ret;
    }

    bool Parse(lString16 codeBase, lString16 css)
    {
        bool ret = false;
        if ( css.empty() )
            return ret;
        lString8 css8 = UnicodeToUtf8(css);
        const char * s = css8.c_str();

        _nestingLevel += 1;
        while (_nestingLevel < 11) { //arbitrary limit
            lString8 import_file;

            if ( LVProcessStyleSheetImport( s, import_file ) ) {
                lString16 importFilename = LVCombinePaths( codeBase, Utf8ToUnicode(import_file) );
                if ( !importFilename.empty() && !_inProgress.contains(importFilename) ) {
                    ret = Parse(importFilename) || ret;
                }
            } else {
                break;
            }
        }
        _nestingLevel -= 1;
        return (_document->getStyleSheet()->parse(s, false, codeBase) || ret);
    }
private:
    ldomDocument  *_document;
    lString16Collection _inProgress;
    int _nestingLevel;
};

/// renders (formats) document in memory
bool ldomDocument::setRenderProps( int width, int dy, bool /*showCover*/, int /*y0*/, font_ref_t def_font, int def_interline_space, CRPropRef props )
{
    // Note: def_interline_space is no more used here
    bool changed = false;
    // Don't clear this cache of LFormattedText if
    // render props don't change.
    //   _renderedBlockCache.clear();
    changed = _imgScalingOptions.update(props, def_font->getSize()) || changed;
    css_style_ref_t s( new css_style_rec_t );
    s->display = css_d_block;
    s->white_space = css_ws_normal;
    s->text_align = css_ta_start;
    s->text_align_last = css_ta_auto;
    s->text_decoration = css_td_none;
    s->text_transform = css_tt_none;
    s->hyphenate = css_hyph_auto;
    s->color.type = css_val_unspecified;
    s->color.value = 0x000000;
    s->background_color.type = css_val_unspecified;
    s->background_color.value = 0xFFFFFF;
    //_def_style->background_color.type = color;
    //_def_style->background_color.value = 0xFFFFFF;
    s->page_break_before = css_pb_auto;
    s->page_break_after = css_pb_auto;
    s->page_break_inside = css_pb_auto;
    s->list_style_type = css_lst_disc;
    s->list_style_position = css_lsp_outside;
    s->vertical_align.type = css_val_unspecified;
    s->vertical_align.value = css_va_baseline;
    s->font_family = def_font->getFontFamily();
    s->font_size.type = css_val_screen_px; // we use this type, as we got the real font size from FontManager
    s->font_size.value = def_font->getSize();
    s->font_name = def_font->getTypeFace();
    s->font_weight = css_fw_400;
    s->font_style = css_fs_normal;
    s->font_features.type = css_val_unspecified;
    s->font_features.value = 0;
    s->text_indent.type = css_val_px;
    s->text_indent.value = 0;
    // s->line_height.type = css_val_percent;
    // s->line_height.value = def_interline_space << 8;
    s->line_height.type = css_val_unspecified;
    s->line_height.value = css_generic_normal; // line-height: normal
    s->orphans = css_orphans_widows_1; // default to allow orphans and widows
    s->widows = css_orphans_widows_1;
    s->float_ = css_f_none;
    s->clear = css_c_none;
    s->direction = css_dir_inherit;
    s->cr_hint = css_cr_hint_none;
    //lUInt32 defStyleHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
    //defStyleHash = defStyleHash * 31 + getDocFlags();
    if ( _last_docflags != getDocFlags() ) {
        CRLog::trace("ldomDocument::setRenderProps() - doc flags changed");
        _last_docflags = getDocFlags();
        changed = true;
    }
    if ( calcHash(_def_style) != calcHash(s) ) {
        CRLog::trace("ldomDocument::setRenderProps() - style is changed");
        _def_style = s;
        changed = true;
    }
    if ( calcHash(_def_font) != calcHash(def_font)) {
        CRLog::trace("ldomDocument::setRenderProps() - font is changed");
        _def_font = def_font;
        changed = true;
    }
    if ( _page_height != dy ) {
        CRLog::trace("ldomDocument::setRenderProps() - page height is changed: %d != %d", _page_height, dy);
        _page_height = dy;
        changed = true;
    }
    if ( _page_width != width ) {
        CRLog::trace("ldomDocument::setRenderProps() - page width is changed");
        _page_width = width;
        changed = true;
    }
//    {
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash before set root style: %x", styleHash);
//    }
//    getRootNode()->setFont( _def_font );
//    getRootNode()->setStyle( _def_style );
//    {
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash after set root style: %x", styleHash);
//    }
    return changed;
}

void tinyNodeCollection::dropStyles()
{
    _styles.clear(-1);
    _fonts.clear(-1);
    resetNodeNumberingProps();

    int count = ((_elemCount+TNC_PART_LEN-1) >> TNC_PART_SHIFT);
    for ( int i=0; i<count; i++ ) {
        int offs = i*TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if ( offs + sz > _elemCount+1 ) {
            sz = _elemCount+1 - offs;
        }
        ldomNode * buf = _elemList[i];
        for ( int j=0; j<sz; j++ ) {
            if ( buf[j].isElement() ) {
                setNodeStyleIndex( buf[j]._handle._dataIndex, 0 );
                setNodeFontIndex( buf[j]._handle._dataIndex, 0 );
            }
        }
    }
    _nodeStyleHash = 0;
}

int tinyNodeCollection::calcFinalBlocks()
{
    int cnt = 0;
    int count = ((_elemCount+TNC_PART_LEN-1) >> TNC_PART_SHIFT);
    for ( int i=0; i<count; i++ ) {
        int offs = i*TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if ( offs + sz > _elemCount+1 ) {
            sz = _elemCount+1 - offs;
        }
        ldomNode * buf = _elemList[i];
        for ( int j=0; j<sz; j++ ) {
            if ( buf[j].isElement() ) {
                int rm = buf[j].getRendMethod();
                if ( rm==erm_final )
                    cnt++;
            }
        }
    }
    return cnt;
}

// This is mostly only useful for FB2 stylesheet, as we no more set
// anything in _docStylesheetFileName
void ldomDocument::applyDocumentStyleSheet()
{
    if ( !getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) ) {
        CRLog::trace("applyDocumentStyleSheet() : DOC_FLAG_ENABLE_INTERNAL_STYLES is disabled");
        return;
    }
    if ( !_docStylesheetFileName.empty() ) {
        if ( getContainer().isNull() )
            return;
        if ( parseStyleSheet(_docStylesheetFileName) ) {
            CRLog::debug("applyDocumentStyleSheet() : Using document stylesheet from link/stylesheet from %s",
                         LCSTR(_docStylesheetFileName));
        }
    } else {
        ldomXPointer ss = createXPointer(cs16("/FictionBook/stylesheet"));
        if ( !ss.isNull() ) {
            lString16 css = ss.getText('\n');
            if ( !css.empty() ) {
                CRLog::debug("applyDocumentStyleSheet() : Using internal FB2 document stylesheet:\n%s", LCSTR(css));
                _stylesheet.parse(LCSTR(css));
            } else {
                CRLog::trace("applyDocumentStyleSheet() : stylesheet under /FictionBook/stylesheet is empty");
            }
        } else {
            CRLog::trace("applyDocumentStyleSheet() : No internal FB2 stylesheet found under /FictionBook/stylesheet");
        }
    }
}

bool ldomDocument::parseStyleSheet(lString16 codeBase, lString16 css)
{
    LVImportStylesheetParser parser(this);
    return parser.Parse(codeBase, css);
}

bool ldomDocument::parseStyleSheet(lString16 cssFile)
{
    LVImportStylesheetParser parser(this);
    return parser.Parse(cssFile);
}

bool ldomDocument::render( LVRendPageList * pages, LVDocViewCallback * callback, int width, int dy, bool showCover, int y0, font_ref_t def_font, int def_interline_space, CRPropRef props )
{
    CRLog::info("Render is called for width %d, pageHeight=%d, fontFace=%s, docFlags=%d", width, dy, def_font->getTypeFace().c_str(), getDocFlags() );
    CRLog::trace("initializing default style...");
    //persist();
//    {
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash before setRenderProps: %x", styleHash);
//    } //bool propsChanged =
    setRenderProps( width, dy, showCover, y0, def_font, def_interline_space, props );

    // update styles
//    if ( getRootNode()->getStyle().isNull() || getRootNode()->getFont().isNull()
//        || _docFlags != _hdr.render_docflags
//        || width!=_hdr.render_dx || dy!=_hdr.render_dy || defStyleHash!=_hdr.stylesheet_hash ) {
//        CRLog::trace("init format data...");
//        getRootNode()->recurseElements( initFormatData );
//    } else {
//        CRLog::trace("reusing existing format data...");
//    }

    bool was_just_rendered_from_cache = _just_rendered_from_cache; // cleared by checkRenderContext()
    if ( !checkRenderContext() ) {
        if ( _nodeDisplayStyleHashInitial == NODE_DISPLAY_STYLE_HASH_UNITIALIZED ) { // happen when just loaded
            // For knowing/debugging cases when node styles set up during loading
            // is invalid (should happen now only when EPUB has embedded fonts
            // or some pseudoclass like :last-child has been met).
            printf("CRE: styles re-init needed after load, re-rendering\n");
        }
        CRLog::info("rendering context is changed - full render required...");
        // Clear LFormattedTextRef cache
        _renderedBlockCache.clear();
        CRLog::trace("init format data...");
        //CRLog::trace("validate 1...");
        //validateDocument();
        CRLog::trace("Dropping existing styles...");
        //CRLog::debug( "root style before drop style %d", getNodeStyleIndex(getRootNode()->getDataIndex()));
        dropStyles();
        //CRLog::debug( "root style after drop style %d", getNodeStyleIndex(getRootNode()->getDataIndex()));

        // After having dropped styles, which should have dropped most references
        // to fonts instances, we want to drop these fonts instances.
        // Mostly because some fallback fonts, possibly synthetized (fake bold and
        // italic) may have been instantiated in the late phase of text rendering.
        // We don't want such instances to be used for styles as it could cause some
        // cache check issues (perpetual "style hash mismatch", as these synthetised
        // fonts would not yet be there when loading from cache).
        // We need 2 gc() for a complete cleanup. The performance impact of
        // reinstantiating the fonts is minimal.
        gc(); // drop font instances that were only referenced by dropped styles
        gc(); // drop fallback font instances that were only referenced by dropped fonts

        //ldomNode * root = getRootNode();
        //css_style_ref_t roots = root->getStyle();
        //CRLog::trace("validate 2...");
        //validateDocument();

        // Reset counters (quotes nesting levels...)
        TextLangMan::resetCounters();

        CRLog::trace("Save stylesheet...");
        _stylesheet.push();
        CRLog::trace("Init node styles...");
        applyDocumentStyleSheet();
        getRootNode()->initNodeStyleRecursive( callback );
        CRLog::trace("Restoring stylesheet...");
        _stylesheet.pop();

        CRLog::trace("init render method...");
        getRootNode()->initNodeRendMethodRecursive();

//        getRootNode()->setFont( _def_font );
//        getRootNode()->setStyle( _def_style );
        updateRenderContext();

        // DEBUG dump of render methods
        //dumpRendMethods( getRootNode(), cs16(" - ") );
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Style hash: %x", styleHash);

        _rendered = false;
    }
    if ( !_rendered ) {
        if ( callback ) {
            callback->OnFormatStart();
        }
        _renderedBlockCache.reduceSize(1); // Reduce size to save some checking and trashing time
        setCacheFileStale(true); // new rendering: cache file will be updated
        _toc_from_cache_valid = false;
        // force recalculation of page numbers (even if not computed in this
        // session, they will be when loaded from cache next session)
        m_toc.invalidatePageNumbers();
        m_pagemap.invalidatePageInfo();
        pages->clear();
        if ( showCover )
            pages->add( new LVRendPageInfo( _page_height ) );
        LVRendPageContext context( pages, _page_height );
        int numFinalBlocks = calcFinalBlocks();
        CRLog::info("Final block count: %d", numFinalBlocks);
        context.setCallback(callback, numFinalBlocks);
        //updateStyles();
        CRLog::trace("rendering...");
        int height = renderBlockElement( context, getRootNode(),
            0, y0, width ) + y0;
        _rendered = true;
    #if 0 //def _DEBUG
        LVStreamRef ostream = LVOpenFileStream( "test_save_after_init_rend_method.xml", LVOM_WRITE );
        saveToStream( ostream, "utf-16" );
    #endif
        gc();
        CRLog::trace("finalizing... fonts.length=%d", _fonts.length());
        context.Finalize();
        updateRenderContext();
        _pagesData.reset();
        pages->serialize( _pagesData );
        _renderedBlockCache.restoreSize(); // Restore original cache size

        if ( _nodeDisplayStyleHashInitial == NODE_DISPLAY_STYLE_HASH_UNITIALIZED ) {
            // If _nodeDisplayStyleHashInitial has not been initialized from its
            // former value from the cache file, we use the one computed (just
            // above in updateRenderContext()) after the first full rendering
            // (which has applied styles and created the needed autoBoxing nodes
            // in the DOM). It is coherent with the DOM built up to now.
            _nodeDisplayStyleHashInitial = _nodeDisplayStyleHash;
            CRLog::info("Initializing _nodeDisplayStyleHashInitial after first rendering: %x", _nodeDisplayStyleHashInitial);
            // We also save it directly into DocFileHeader _hdr (normally,
            // updateRenderContext() does this, but doing it here avoids
            // a call and an expensive CalcStyleHash)
            _hdr.node_displaystyle_hash = _nodeDisplayStyleHashInitial;
        }

        if ( callback ) {
            callback->OnFormatEnd();
            callback->OnDocumentReady();
        }

        //saveChanges();

        //persist();
        dumpStatistics();

        return true; // full (re-)rendering done
        // return height;

    } else {
        CRLog::info("rendering context is not changed - no render!");
        if ( _pagesData.pos() ) {
            _pagesData.setPos(0);
            pages->deserialize( _pagesData );
        }
        CRLog::info("%d rendered pages found", pages->length() );

        if ( was_just_rendered_from_cache && callback )
            callback->OnDocumentReady();

        return false; // no (re-)rendering needed
        // return getFullHeight();
    }

}
#endif

void lxmlDocBase::setNodeTypes( const elem_def_t * node_scheme )
{
    if ( !node_scheme )
        return;
    for ( ; node_scheme && node_scheme->id != 0; ++node_scheme )
    {
        _elementNameTable.AddItem(
            node_scheme->id,               // ID
            lString16(node_scheme->name),  // Name
            &node_scheme->props );  // ptr
    }
}

// set attribute types from table
void lxmlDocBase::setAttributeTypes( const attr_def_t * attr_scheme )
{
    if ( !attr_scheme )
        return;
    for ( ; attr_scheme && attr_scheme->id != 0; ++attr_scheme )
    {
        _attrNameTable.AddItem(
            attr_scheme->id,               // ID
            lString16(attr_scheme->name),  // Name
            NULL);
    }
    _idAttrId = _attrNameTable.idByName("id");
}

// set namespace types from table
void lxmlDocBase::setNameSpaceTypes( const ns_def_t * ns_scheme )
{
    if ( !ns_scheme )
        return;
    for ( ; ns_scheme && ns_scheme->id != 0; ++ns_scheme )
    {
        _nsNameTable.AddItem(
            ns_scheme->id,                 // ID
            lString16(ns_scheme->name),    // Name
            NULL);
    }
}

void lxmlDocBase::dumpUnknownEntities( const char * fname )
{
    FILE * f = fopen( fname, "wt" );
    if ( !f )
        return;
    fprintf(f, "Unknown elements:\n");
    _elementNameTable.dumpUnknownItems(f, UNKNOWN_ELEMENT_TYPE_ID);
    fprintf(f, "-------------------------------\n");
    fprintf(f, "Unknown attributes:\n");
    _attrNameTable.dumpUnknownItems(f, UNKNOWN_ATTRIBUTE_TYPE_ID);
    fprintf(f, "-------------------------------\n");
    fprintf(f, "Unknown namespaces:\n");
    _nsNameTable.dumpUnknownItems(f, UNKNOWN_NAMESPACE_TYPE_ID);
    fprintf(f, "-------------------------------\n");
    fclose(f);
}

lString16Collection lxmlDocBase::getUnknownEntities()
{
    lString16Collection unknown_entities;
    unknown_entities.add( _elementNameTable.getUnknownItems(UNKNOWN_ELEMENT_TYPE_ID) );
    unknown_entities.add( _attrNameTable.getUnknownItems(UNKNOWN_ATTRIBUTE_TYPE_ID) );
    unknown_entities.add( _nsNameTable.getUnknownItems(UNKNOWN_NAMESPACE_TYPE_ID) );
    return unknown_entities;
}


#if BUILD_LITE!=1
static const char * id_map_list_magic = "MAPS";
static const char * elem_id_map_magic = "ELEM";
static const char * attr_id_map_magic = "ATTR";
static const char * attr_value_map_magic = "ATTV";
static const char * ns_id_map_magic =   "NMSP";
static const char * node_by_id_map_magic = "NIDM";

typedef struct {
    lUInt32 key;
    lUInt32 value;
} id_node_map_item;

int compare_id_node_map_items(const void * item1, const void * item2) {
    id_node_map_item * v1 = (id_node_map_item*)item1;
    id_node_map_item * v2 = (id_node_map_item*)item2;
    if (v1->key > v2->key)
        return 1;
    if (v1->key < v2->key)
        return -1;
    return 0;
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
void lxmlDocBase::serializeMaps( SerialBuf & buf )
{
    if ( buf.error() )
        return;
    int pos = buf.pos();
    buf.putMagic( id_map_list_magic );
    buf.putMagic( elem_id_map_magic );
    _elementNameTable.serialize( buf );
    buf << _nextUnknownElementId; // Next Id for unknown element
    buf.putMagic( attr_id_map_magic );
    _attrNameTable.serialize( buf );
    buf << _nextUnknownAttrId;    // Next Id for unknown attribute
    buf.putMagic( ns_id_map_magic );
    _nsNameTable.serialize( buf );
    buf << _nextUnknownNsId;      // Next Id for unknown namespace
    buf.putMagic( attr_value_map_magic );
    _attrValueTable.serialize( buf );

    int start = buf.pos();
    buf.putMagic( node_by_id_map_magic );
    lUInt32 cnt = 0;
    {
        LVHashTable<lUInt32,lInt32>::iterator ii = _idNodeMap.forwardIterator();
        for ( LVHashTable<lUInt32,lInt32>::pair * p = ii.next(); p!=NULL; p = ii.next() ) {
            cnt++;
        }
    }
    // TODO: investigate why length() doesn't work as count
    if ( (int)cnt!=_idNodeMap.length() )
        CRLog::error("_idNodeMap.length=%d doesn't match real item count %d", _idNodeMap.length(), cnt);
    buf << cnt;
    if (cnt > 0)
    {
        // sort items before serializing!
        id_node_map_item * array = new id_node_map_item[cnt];
        int i = 0;
        LVHashTable<lUInt32,lInt32>::iterator ii = _idNodeMap.forwardIterator();
        for ( LVHashTable<lUInt32,lInt32>::pair * p = ii.next(); p!=NULL; p = ii.next() ) {
            array[i].key = (lUInt32)p->key;
            array[i].value = (lUInt32)p->value;
            i++;
        }
        qsort(array, cnt, sizeof(id_node_map_item), &compare_id_node_map_items);
        for (i = 0; i < (int)cnt; i++)
            buf << array[i].key << array[i].value;
        delete[] array;
    }
    buf.putMagic( node_by_id_map_magic );
    buf.putCRC( buf.pos() - start );

    buf.putCRC( buf.pos() - pos );
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool lxmlDocBase::deserializeMaps( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    int pos = buf.pos();
    buf.checkMagic( id_map_list_magic );
    buf.checkMagic( elem_id_map_magic );
    _elementNameTable.deserialize( buf );
    buf >> _nextUnknownElementId; // Next Id for unknown element

    if ( buf.error() ) {
        CRLog::error("Error while deserialization of Element ID map");
        return false;
    }

    buf.checkMagic( attr_id_map_magic );
    _attrNameTable.deserialize( buf );
    buf >> _nextUnknownAttrId;    // Next Id for unknown attribute

    if ( buf.error() ) {
        CRLog::error("Error while deserialization of Attr ID map");
        return false;
    }


    buf.checkMagic( ns_id_map_magic );
    _nsNameTable.deserialize( buf );
    buf >> _nextUnknownNsId;      // Next Id for unknown namespace

    if ( buf.error() ) {
        CRLog::error("Error while deserialization of NS ID map");
        return false;
    }

    buf.checkMagic( attr_value_map_magic );
    _attrValueTable.deserialize( buf );

    if ( buf.error() ) {
        CRLog::error("Error while deserialization of AttrValue map");
        return false;
    }

    int start = buf.pos();
    buf.checkMagic( node_by_id_map_magic );
    lUInt32 idmsize;
    buf >> idmsize;
    _idNodeMap.clear();
    if ( idmsize < 20000 )
        _idNodeMap.resize( idmsize*2 );
    for ( unsigned i=0; i<idmsize; i++ ) {
        lUInt32 key;
        lUInt32 value;
        buf >> key;
        buf >> value;
        _idNodeMap.set( key, value );
        if ( buf.error() )
            return false;
    }
    buf.checkMagic( node_by_id_map_magic );

    if ( buf.error() ) {
        CRLog::error("Error while deserialization of ID->Node map");
        return false;
    }

    buf.checkCRC( buf.pos() - start );

    if ( buf.error() ) {
        CRLog::error("Error while deserialization of ID->Node map - CRC check failed");
        return false;
    }

    buf.checkCRC( buf.pos() - pos );

    return !buf.error();
}
#endif

bool IsEmptySpace( const lChar16 * text, int len )
{
   for (int i=0; i<len; i++)
      if ( text[i]!=' ' && text[i]!='\r' && text[i]!='\n' && text[i]!='\t')
         return false;
   return true;
}


/////////////////////////////////////////////////////////////////
/// lxmlElementWriter

static bool IS_FIRST_BODY = false;

ldomElementWriter::ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent)
    : _parent(parent), _document(document), _tocItem(NULL), _isBlock(true), _isSection(false),
      _stylesheetIsSet(false), _bodyEnterCalled(false), _pseudoElementAfterChildIndex(-1)
{
    //logfile << "{c";
    _typeDef = _document->getElementTypePtr( id );
    _flags = 0;
    if ( (_typeDef && _typeDef->white_space >= css_ws_pre_line) || (_parent && _parent->getFlags()&TXTFLG_PRE) )
        _flags |= TXTFLG_PRE; // Parse as PRE: pre-line, pre, pre-wrap and break-spaces
        // This will be updated in ldomElementWriter::onBodyEnter() after we have
        // set styles to this node, so we'll get the real white_space value to use.

    _isSection = (id==el_section);

    // Default (for elements not specified in fb2def.h) is to allow text
    // (except for the root node which must have children)
    _allowText = _typeDef ? _typeDef->allow_text : (_parent?true:false);
    if (gDOMVersionRequested < 20180528) { // revert what was changed 20180528
        // <hr>, <ul>, <ol>, <dl>, <output>, <section>, <svg> didn't allow text
        if ( id==el_hr || id==el_ul || id==el_ol || id==el_dl ||
                id==el_output || id==el_section || id==el_svg ) {
            _allowText = false;
        }
        // <code> was white-space: pre
        if ( id==el_code ) {
            _flags |= TXTFLG_PRE;
        }
    }

    if (_parent)
        _element = _parent->getElement()->insertChildElement( (lUInt32)-1, nsid, id );
    else
        _element = _document->getRootNode(); //->insertChildElement( (lUInt32)-1, nsid, id );
    if ( IS_FIRST_BODY && id==el_body ) {
        _tocItem = _document->getToc();
        //_tocItem->clear();
        IS_FIRST_BODY = false;
    }
    //logfile << "}";
}

lUInt32 ldomElementWriter::getFlags()
{
    return _flags;
}

static bool isBlockNode( ldomNode * node )
{
    if ( !node->isElement() )
        return false;
#if BUILD_LITE!=1
    switch ( node->getStyle()->display )
    {
    case css_d_block:
    case css_d_inline_block:
    case css_d_inline_table:
    case css_d_list_item:
    case css_d_list_item_block:
    case css_d_table:
    case css_d_table_row:
    case css_d_table_row_group:
    case css_d_table_header_group:
    case css_d_table_footer_group:
    case css_d_table_column_group:
    case css_d_table_column:
    case css_d_table_cell:
    case css_d_table_caption:
        return true;

    case css_d_inherit:
    case css_d_inline:
    case css_d_run_in:
    case css_d_compact:
    case css_d_marker:
    case css_d_none:
        break;
    }
    return false;
#else
    return true;
#endif
}

static bool isInlineNode( ldomNode * node )
{
    if ( node->isText() )
        return true;
    //int d = node->getStyle()->display;
    //return ( d==css_d_inline || d==css_d_run_in );
    int m = node->getRendMethod();
    return m==erm_inline || m==erm_runin;
}

static bool isFloatingNode( ldomNode * node )
{
    if ( node->isText() )
        return false;
    return node->getStyle()->float_ > css_f_none;
}

static bool isNotBoxWrappingNode( ldomNode * node )
{
    if ( BLOCK_RENDERING_G(PREPARE_FLOATBOXES) && node->getStyle()->float_ > css_f_none )
        return false; // floatBox
    // isBoxingInlineBox() already checks for BLOCK_RENDERING_G(BOX_INLINE_BLOCKS)
    return !node->isBoxingInlineBox();
}

static bool isNotBoxingInlineBoxNode( ldomNode * node )
{
    return !node->isBoxingInlineBox();
}

static lString16 getSectionHeader( ldomNode * section )
{
    lString16 header;
    if ( !section || section->getChildCount() == 0 )
        return header;
    ldomNode * child = section->getChildElementNode(0, L"title");
    if ( !child )
        return header;
    header = child->getText(L' ', 1024);
    return header;
}

lString16 ldomElementWriter::getPath()
{
    if ( !_path.empty() || _element->isRoot() )
        return _path;
    _path = _parent->getPath() + "/" + _element->getXPathSegment();
    return _path;
}

void ldomElementWriter::updateTocItem()
{
    if ( !_isSection )
        return;
    // TODO: update item
    if ( _parent && _parent->_tocItem ) {
        lString16 title = getSectionHeader( _element );
        //CRLog::trace("TOC ITEM: %s", LCSTR(title));
        _tocItem = _parent->_tocItem->addChild(title, ldomXPointer(_element,0), getPath() );
    }
    _isSection = false;
}

void ldomElementWriter::onBodyEnter()
{
    _bodyEnterCalled = true;
#if BUILD_LITE!=1
    //CRLog::trace("onBodyEnter() for node %04x %s", _element->getDataIndex(), LCSTR(_element->getNodeName()));
    if ( _document->isDefStyleSet() ) {
        _element->initNodeStyle();
//        if ( _element->getStyle().isNull() ) {
//            CRLog::error("error while style initialization of element %x %s", _element->getNodeIndex(), LCSTR(_element->getNodeName()) );
//            crFatalError();
//        }
        int nb_children = _element->getChildCount();
        if ( nb_children > 0 ) {
            // The only possibility for this element being built to have children
            // is if the above initNodeStyle() has applied to this node some
            // matching selectors that had ::before or ::after, which have then
            // created one or two pseudoElem children. But let's be sure of that.
            for ( int i=0; i<nb_children; i++ ) {
                ldomNode * child = _element->getChildNode(i);
                if ( child->getNodeId() == el_pseudoElem ) {
                    if ( child->hasAttribute(attr_Before) ) {
                        // The "Before" pseudo element (not part of the XML)
                        // needs to have its style applied. As it has no
                        // children, we can also init its rend method.
                        child->initNodeStyle();
                        child->initNodeRendMethod();
                    }
                    else if ( child->hasAttribute(attr_After) ) {
                        // For the "After" pseudo element, we need to wait
                        // for all real children to be added, to move it
                        // as its right position (last), to init its style
                        // (because of "content:close-quote", whose nested
                        // level need to have seen all previous nodes to
                        // be accurate) and its rendering method.
                        // We'll do that in onBodyExit() when called for
                        // this node.
                        _pseudoElementAfterChildIndex = i;
                    }
                }
            }
        }
        _isBlock = isBlockNode(_element);
        // If initNodeStyle() has set "white-space: pre" or alike, update _flags
        if ( _element->getStyle()->white_space >= css_ws_pre_line) {
            _flags |= TXTFLG_PRE;
        }
        else {
            _flags &= ~TXTFLG_PRE;
        }
    } else {
    }
    if ( _isSection ) {
        if ( _parent && _parent->_isSection ) {
            _parent->updateTocItem();
        }

    }
#endif
}

void ldomNode::ensurePseudoElement( bool is_before ) {
#if BUILD_LITE!=1
    // This node should have that pseudoElement, but it might already be there,
    // so check if there is already one, and if not, create it.
    // This happens usually in the initial loading phase, but it might in
    // a re-rendering if the pseudo element is introduced by a change in
    // styles (we won't be able to create a node if there's a cache file).
    int insertChildIndex = -1;
    int nb_children = getChildCount();
    if ( is_before ) { // ::before
        insertChildIndex = 0; // always to be inserted first, if not already there
        if ( nb_children > 0 ) {
            ldomNode * child = getChildNode(0); // should always be found as the first node
            // pseudoElem might have been wrapped by a inlineBox, autoBoxing, floatBox...
            while ( child && child->isBoxingNode() && child->getChildCount()>0 )
                child = child->getChildNode(0);
            if ( child && child->getNodeId() == el_pseudoElem && child->hasAttribute(attr_Before) ) {
                // Already there, no need to create it
                insertChildIndex = -1;
            }
        }
    }
    else { // ::after
        // In the XML loading phase, this one might be either first,
        // or second if there's already a Before. In the re-rendering
        // phase, it would have been moved as the last node. In all these
        // cases, it is always the last at the moment we are checking.
        insertChildIndex = nb_children; // always to be inserted last, if not already there
        if ( nb_children > 0 ) {
            ldomNode * child = getChildNode(nb_children-1); // should always be found as the last node
            // pseudoElem might have been wrapped by a inlineBox, autoBoxing, floatBox...
            while ( child && child->isBoxingNode() && child->getChildCount()>0 )
                child = child->getChildNode(0);
            if ( child && child->getNodeId() == el_pseudoElem && child->hasAttribute(attr_After) ) {
                // Already there, no need to create it
                insertChildIndex = -1;
            }
        }
    }
    if ( insertChildIndex >= 0 ) {
        ldomNode * pseudo = insertChildElement( insertChildIndex, LXML_NS_NONE, el_pseudoElem );
        lUInt16 attribute_id = is_before ? attr_Before : attr_After;
        pseudo->setAttributeValue(LXML_NS_NONE, attribute_id, L"");
        // We are called by lvrend.cpp setNodeStyle(), after the parent
        // style and font have been fully set up. We could set this pseudo
        // element style with pseudo->initNodeStyle(), as it can inherit
        // properly, but we should not:
        // - when re-rendering, initNodeStyleRecursive()/updateStyleDataRecursive()
        //   will iterate thru this node we just added as a child, and do it.
        // - when XML loading, we could do it for the "Before" pseudo element,
        //   but for the "After" one, we need to wait for all real children to be
        //   added and have their style applied - just because they can change
        //   open-quote/close-quote nesting levels - to be sure we get the
        //   proper nesting level quote char for the After node.
        // So, for the XML loading phase, we do that in onBodyEnter() and
        // onBodyExit() when called on the parent node.
    }

#endif
}

#if BUILD_LITE!=1
static void resetRendMethodToInline( ldomNode * node )
{
    // we shouldn't reset to inline (visible) if display: none
    // (using node->getRendMethod() != erm_invisible seems too greedy and may
    // hide other nodes)
    if (node->getStyle()->display != css_d_none)
        node->setRendMethod(erm_inline);
    else if (gDOMVersionRequested < 20180528) // do that in all cases
        node->setRendMethod(erm_inline);
}

static void resetRendMethodToInvisible( ldomNode * node )
{
    node->setRendMethod(erm_invisible);
}
#endif

void ldomNode::removeChildren( int startIndex, int endIndex )
{
    for ( int i=endIndex; i>=startIndex; i-- ) {
        removeChild(i)->destroy();
    }
}

void ldomNode::autoboxChildren( int startIndex, int endIndex, bool handleFloating )
{
#if BUILD_LITE!=1
    if ( !isElement() )
        return;
    css_style_ref_t style = getStyle();
    bool pre = ( style->white_space >= css_ws_pre_line );
        // (css_ws_pre_line might need special care?)
    int firstNonEmpty = startIndex;
    int lastNonEmpty = endIndex;

    bool hasInline = pre;
    bool hasNonEmptyInline = pre;
    bool hasFloating = false;
    // (Note: did not check how floats inside <PRE> are supposed to work)
    if ( !pre ) {
        while ( firstNonEmpty<=endIndex && getChildNode(firstNonEmpty)->isText() ) {
            lString16 s = getChildNode(firstNonEmpty)->getText();
            if ( !IsEmptySpace(s.c_str(), s.length() ) )
                break;
            firstNonEmpty++;
        }
        while ( lastNonEmpty>=endIndex && getChildNode(lastNonEmpty)->isText() ) {
            lString16 s = getChildNode(lastNonEmpty)->getText();
            if ( !IsEmptySpace(s.c_str(), s.length() ) )
                break;
            lastNonEmpty--;
        }

        for ( int i=firstNonEmpty; i<=lastNonEmpty; i++ ) {
            ldomNode * node = getChildNode(i);
            if ( isInlineNode( node ) ) {
                hasInline = true;
                if ( !hasNonEmptyInline ) {
                    if (node->isText()) {
                        lString16 s = node->getText();
                        if ( !IsEmptySpace(s.c_str(), s.length() ) ) {
                            hasNonEmptyInline = true;
                        }
                    }
                    else {
                        if ( handleFloating && isFloatingNode(node) ) {
                            // Ignore floatings
                        }
                        else {
                            hasNonEmptyInline = true;
                            // Note: when not using DO_NOT_CLEAR_OWN_FLOATS, we might
                            // want to be more agressive in the removal of empty
                            // elements, including nested empty elements which would
                            // have no effect on the rendering (eg, some empty <link/>
                            // or <span id="PageNumber123"/>), to avoid having the float
                            // in an autoBox element with nothing else, which would
                            // then be cleared and leave some blank space.
                            // We initially did:
                            //    // For now, assume any inline node with some content
                            //    // (text or other inlines) is non empty.
                            //    if ( node->getChildCount() > 0 )
                            //        hasNonEmptyInline = true;
                            //    else if (node->getNodeId() == el_br) {
                            //        hasNonEmptyInline = true;
                            //    }
                            //    else {
                            //        const css_elem_def_props_t * ntype = node->getElementTypePtr();
                            //        if (ntype && ntype->is_object) // standalone image
                            //            hasNonEmptyInline = true;
                            //    }
                            // and we could even use hasNonEmptyInlineContent() to get
                            // rid of any nested empty elements and be sure to have our
                            // float standalone and be able to have it rendered as block
                            // instead of in an erm_final.
                            //
                            // But this was for edge cases (but really noticable), and it has
                            // become less critical now that we have/ DO_NOT_CLEAR_OWN_FLOATS,
                            // so let's not remove any element from our DOM (those with some
                            // id= attribute might be the target of a link).
                            //
                            // Sample test case in China.EN at the top of the "Politics" section:
                            //   "...</div> <link/> (or any text) <div float>...</div> <div>..."
                            // gets turned into:
                            //   "...</div>
                            //   <autoBoxing>
                            //     <link/> (or any text)
                            //     <floatBox>
                            //       <div float>...</div>
                            //     </floatBox>
                            //   </autoBoxing>
                            //   <div>..."
                            // If the floatbox would be let outside of the autobox, it would
                            // be fine when not DO_NOT_CLEAR_OWN_FLOATS too.
                        }
                    }
                }
            }
            if ( handleFloating && isFloatingNode(node) )
                hasFloating = true;
            if ( hasNonEmptyInline && hasFloating )
                break; // We know, no need to look more
        }
    }

    if ( hasFloating && !hasNonEmptyInline) {
        // only multiple floats with empty spaces in between:
        // remove empty text nodes, and let the floats be blocks, don't autobox
        for ( int i=endIndex; i>=startIndex; i-- ) {
            if ( !isFloatingNode(getChildNode(i)) )
                removeChildren(i, i);
        }
    }
    else if ( hasInline ) { //&& firstNonEmpty<=lastNonEmpty

#ifdef TRACE_AUTOBOX
        CRLog::trace("Autobox children %d..%d of node <%s>  childCount=%d", firstNonEmpty, lastNonEmpty, LCSTR(getNodeName()), getChildCount());

        for ( int i=firstNonEmpty; i<=lastNonEmpty; i++ ) {
            ldomNode * node = getChildNode(i);
            if ( node->isText() )
                CRLog::trace("    text: %d '%s'", node->getDataIndex(), LCSTR(node->getText()));
            else
                CRLog::trace("    elem: %d <%s> rendMode=%d  display=%d", node->getDataIndex(), LCSTR(node->getNodeName()), node->getRendMethod(), node->getStyle()->display);
        }
#endif
        // remove trailing empty
        removeChildren(lastNonEmpty+1, endIndex);

        // inner inline
        ldomNode * abox = insertChildElement( firstNonEmpty, LXML_NS_NONE, el_autoBoxing );
        moveItemsTo( abox, firstNonEmpty+1, lastNonEmpty+1 );
        // remove starting empty
        removeChildren(startIndex, firstNonEmpty-1);
        abox->initNodeStyle();
        if ( !BLOCK_RENDERING_G(FLOAT_FLOATBOXES) ) {
            // If we don't want floatBoxes floating, reset them to be
            // rendered inline among inlines
            abox->recurseMatchingElements( resetRendMethodToInline, isNotBoxingInlineBoxNode );
        }
        abox->setRendMethod( erm_final );
    }
    else if ( hasFloating) {
        // only floats, don't autobox them (otherwise the autobox wouldn't be floating)
        // remove trailing empty
        removeChildren(lastNonEmpty+1, endIndex);
        // remove starting empty
        removeChildren(startIndex, firstNonEmpty-1);
    }
    else {
        // only empty items: remove them instead of autoboxing
        removeChildren(startIndex, endIndex);
    }
#endif
}

bool ldomNode::cleanIfOnlyEmptyTextInline( bool handleFloating )
{
#if BUILD_LITE!=1
    if ( !isElement() )
        return false;
    css_style_ref_t style = getStyle();
    if ( style->white_space >= css_ws_pre )
        return false; // Don't mess with PRE (css_ws_pre_line might need special care?)
    // We return false as soon as we find something non text, or text non empty
    int i = getChildCount()-1;
    for ( ; i>=0; i-- ) {
        ldomNode * node = getChildNode(i);
        if ( node->isText() ) {
            lString16 s = node->getText();
            if ( !IsEmptySpace(s.c_str(), s.length() ) ) {
                return false;
            }
        }
        else if ( handleFloating && isFloatingNode(node) ) {
            // Ignore floatings
        }
        else { // non-text non-float element
            return false;
        }
    }
    // Ok, only empty text inlines, with possible floats
    i = getChildCount()-1;
    for ( ; i>=0; i-- ) {
        // With the tests done above, we just need to remove text nodes
        if ( getChildNode(i)->isText() ) {
            removeChildren(i, i);
        }
    }
    return true;
#else
    return false;
#endif
}

/// returns true if element has inline content (non empty text, images, <BR>)
bool ldomNode::hasNonEmptyInlineContent( bool ignoreFloats )
{
    if ( getRendMethod() == erm_invisible ) {
        return false;
    }
    if ( ignoreFloats && BLOCK_RENDERING_G(FLOAT_FLOATBOXES) && getStyle()->float_ > css_f_none ) {
        return false;
    }
    // With some other bool param, we might want to also check for
    // padding top/bottom (and height if check ENSURE_STYLE_HEIGHT)
    // if these will introduce some content.
    if ( isText() ) {
        lString16 s = getText();
        return !IsEmptySpace(s.c_str(), s.length() );
    }
    if (getNodeId() == el_br) {
        return true;
    }
    const css_elem_def_props_t * ntype = getElementTypePtr();
    if (ntype && ntype->is_object) { // standalone image
        return true;
    }
    for ( int i=0; i<(int)getChildCount(); i++ ) {
        if ( getChildNode(i)->hasNonEmptyInlineContent() ) {
            return true;
        }
    }
    return false;
}

#if BUILD_LITE!=1
static void detectChildTypes( ldomNode * parent, bool & hasBlockItems, bool & hasInline,
                    bool & hasInternalTableItems, bool & hasFloating, bool detectFloating=false )
{
    hasBlockItems = false;
    hasInline = false;
    hasFloating = false;
    if ( parent->getNodeId() == el_pseudoElem ) {
        // pseudoElem (generated from CSS ::before and ::after), will have
        // some (possibly empty) plain text content.
        hasInline = true;
        return; // and it has no children
    }
    int len = parent->getChildCount();
    for ( int i=len-1; i>=0; i-- ) {
        ldomNode * node = parent->getChildNode(i);
        if ( !node->isElement() ) {
            // text
            hasInline = true;
        }
        else if ( detectFloating && node->getStyle()->float_ > css_f_none ) {
            hasFloating = true;
        }
        else {
            // element
            int d = node->getStyle()->display;
            int m = node->getRendMethod();
            if ( d==css_d_none || m==erm_invisible )
                continue;
            if ( m==erm_inline || m==erm_runin) { //d==css_d_inline || d==css_d_run_in
                hasInline = true;
            } else {
                hasBlockItems = true;
                // (Table internal elements are all block items in the context
                // where hasBlockItems is used, so account for them in both)
                if ( ( d > css_d_table && d <= css_d_table_caption ) ||
                     ( m > erm_table   && m <= erm_table_caption ) ) {
                    hasInternalTableItems = true;
                }
            }
        }
    }
}

// Generic version of autoboxChildren() without any specific inline/block checking,
// accepting any element id (from the enum el_*, like el_div, el_tabularBox) as
// the wrapping element.
ldomNode * ldomNode::boxWrapChildren( int startIndex, int endIndex, lUInt16 elementId )
{
    if ( !isElement() )
        return NULL;
    int firstNonEmpty = startIndex;
    int lastNonEmpty = endIndex;

    while ( firstNonEmpty<=endIndex && getChildNode(firstNonEmpty)->isText() ) {
        lString16 s = getChildNode(firstNonEmpty)->getText();
        if ( !IsEmptySpace(s.c_str(), s.length() ) )
            break;
        firstNonEmpty++;
    }
    while ( lastNonEmpty>=endIndex && getChildNode(lastNonEmpty)->isText() ) {
        lString16 s = getChildNode(lastNonEmpty)->getText();
        if ( !IsEmptySpace(s.c_str(), s.length() ) )
            break;
        lastNonEmpty--;
    }

    // printf("boxWrapChildren %d>%d | %d<%d\n", startIndex, firstNonEmpty, lastNonEmpty, endIndex);
    if ( firstNonEmpty<=lastNonEmpty ) {
        // remove trailing empty
        removeChildren(lastNonEmpty+1, endIndex);
        // create wrapping container
        ldomNode * box = insertChildElement( firstNonEmpty, LXML_NS_NONE, elementId );
        moveItemsTo( box, firstNonEmpty+1, lastNonEmpty+1 );
        // remove starting empty
        removeChildren(startIndex, firstNonEmpty-1);
        return box;
    }
    else {
        // Only empty items: remove them instead of box wrapping them
        removeChildren(startIndex, endIndex);
        return NULL;
    }
}

// Uncomment to debug COMPLETE_INCOMPLETE_TABLES tabularBox wrapping
// #define DEBUG_INCOMPLETE_TABLE_COMPLETION

// init table element render methods
// states: 0=table, 1=colgroup, 2=rowgroup, 3=row, 4=cell
// returns table cell count
// When BLOCK_RENDERING_G(COMPLETE_INCOMPLETE_TABLES), we follow rules
// from the "Generate missing child wrappers" section in:
//   https://www.w3.org/TR/CSS22/tables.html#anonymous-boxes
//   https://www.w3.org/TR/css-tables-3/#fixup (clearer than previous one)
// and we wrap unproper children in a tabularBox element.
int initTableRendMethods( ldomNode * enode, int state )
{
    //main node: table
    if ( state==0 && ( enode->getStyle()->display==css_d_table ||
                       enode->getStyle()->display==css_d_inline_table ||
                      (enode->getStyle()->display==css_d_inline_block && enode->getNodeId()==el_table) ) ) {
        enode->setRendMethod( erm_table );
    }
    int cellCount = 0; // (returned, but not used anywhere)
    int cnt = enode->getChildCount();
    int i;
    int first_unproper = -1; // keep track of consecutive unproper children that
    int last_unproper = -1;  // must all be wrapped in a single wrapper
    for (i=0; i<cnt; i++) {
        ldomNode * child = enode->getChildNode( i );
        css_display_t d;
        if ( child->isElement() ) {
            d = child->getStyle()->display;
        }
        else { // text node
            d = css_d_inline;
            // Not sure about what to do with whitespace only text nodes:
            // we shouldn't meet any alongside real elements (as whitespace
            // around and at start/end of block nodes are discarded), but
            // we may in case of style changes (inline > table) after
            // a book has been loaded.
            // Not sure if we should handle them differently when no unproper
            // elements yet (they will be discarded by the table render algo),
            // and when among unpropers (they could find their place in the
            // wrapped table cell).
            // Note that boxWrapChildren() called below will remove
            // them at start or end of an unproper elements sequence.
        }
        bool is_last = (i == cnt-1);
        bool is_proper = false;
        if ( state==0 ) { // in table
            if ( d==css_d_table_row ) {
                child->setRendMethod( erm_table_row );
                cellCount += initTableRendMethods( child, 3 ); // > row
                is_proper = true;
            }
            else if ( d==css_d_table_row_group ) {
                child->setRendMethod( erm_table_row_group );
                cellCount += initTableRendMethods( child, 2 ); // > rowgroup
                is_proper = true;
            }
            else if ( d==css_d_table_header_group ) {
                child->setRendMethod( erm_table_header_group );
                cellCount += initTableRendMethods( child, 2 ); // > rowgroup
                is_proper = true;
            }
            else if ( d==css_d_table_footer_group ) {
                child->setRendMethod( erm_table_footer_group );
                cellCount += initTableRendMethods( child, 2 ); // > rowgroup
                is_proper = true;
            }
            else if ( d==css_d_table_column_group ) {
                child->setRendMethod( erm_table_column_group );
                cellCount += initTableRendMethods( child, 1 ); // > colgroup
                is_proper = true;
            }
            else if ( d==css_d_table_column ) {
                child->setRendMethod( erm_table_column );
                is_proper = true;
            }
            else if ( d==css_d_table_caption ) {
                child->setRendMethod( erm_table_caption );
                is_proper = true;
            }
            else if ( d==css_d_none ) {
                child->setRendMethod( erm_invisible );
                is_proper = true;
            }
            else if ( child->getNodeId()==el_tabularBox ) {
                // Most probably added by us in a previous rendering
                #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                    printf("initTableRendMethods(0): (reused)wrapping unproper > row\n");
                #endif
                child->setRendMethod( erm_table_row );
                cellCount += initTableRendMethods( child, 3 ); // > row
                is_proper = true;
            }
        }
        else if ( state==2 ) { // in rowgroup
            if ( d==css_d_table_row ) {
                child->setRendMethod( erm_table_row );
                cellCount += initTableRendMethods( child, 3 ); // > row
                is_proper = true;
            }
            else if ( d==css_d_none ) {
                child->setRendMethod( erm_invisible );
                is_proper = true;
            }
            else if ( child->getNodeId()==el_tabularBox ) {
                // Most probably added by us in a previous rendering
                #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                    printf("initTableRendMethods(2): (reused)wrapping unproper > row\n");
                #endif
                child->setRendMethod( erm_table_row );
                cellCount += initTableRendMethods( child, 3 ); // > row
                is_proper = true;
            }
        }
        else if ( state==3 ) { // in row
            if ( d==css_d_table_cell ) {
                child->setRendMethod( erm_table_cell );
                cellCount++;
                is_proper = true;
                // This will reset the rend method we just set (erm_table_cell)
                // to the most appropriate one (erm_final or erm_block) for
                // rendering, depending on the cell content:
                child->initNodeRendMethodRecursive();
            }
            else if ( d==css_d_none ) {
                child->setRendMethod( erm_invisible );
                is_proper = true;
            }
            else if ( child->getNodeId()==el_tabularBox ) {
                // Most probably added by us in a previous rendering
                #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                    printf("initTableRendMethods(3): (reused)wrapping unproper > cell\n");
                #endif
                child->setRendMethod( erm_table_cell );
                cellCount++;
                child->initNodeRendMethodRecursive();
                is_proper = true;
            }
        }
        else if ( state==1 ) { // in colgroup
            if ( d==css_d_table_column ) {
                child->setRendMethod( erm_table_column );
                is_proper = true;
            }
            else {
                // No need to tabularBox invalid colgroup children:
                // they are not rendered, and should be considered
                // as if display: none.
                child->setRendMethod( erm_invisible );
                is_proper = true;
            }
        }
        else { // shouldn't be reached
            crFatalError(151, "initTableRendMethods state unexpected");
            // child->setRendMethod( erm_final );
        }

        // Check and deal with unproper children
        if ( !is_proper ) { // Unproper child met
            // printf("initTableRendMethods(%d): child %d is unproper\n", state, i);
            if ( BLOCK_RENDERING_G(COMPLETE_INCOMPLETE_TABLES) ) {
                // We can insert a tabularBox element to wrap unproper elements
                last_unproper = i;
                if (first_unproper < 0)
                    first_unproper = i;
            }
            else {
                // Asked to not complete incomplete tables, or we can't insert
                // tabularBox elements anymore
                if ( !BLOCK_RENDERING_G(ENHANCED) ) {
                    // Legacy behaviour was to just make invisible internal-table
                    // elements that were not found in their proper internal-table
                    // container, but let other non-internal-table elements be
                    // (which might be rendered and drawn quite correctly when
                    // they are erm_final/erm_block, but won't be if erm_inline).
                    if ( d > css_d_table ) {
                        child->setRendMethod( erm_invisible );
                    }
                }
                else {
                    // When in enhanced mode, we let the ones that could
                    // be rendered and drawn quite correctly be. But we'll
                    // have the others drawn as erm_killed, showing a small
                    // symbol so users know some content is missing.
                    if ( d > css_d_table || d == css_d_inline ) {
                        child->setRendMethod( erm_killed );
                    }
                    // Note that there are other situations where some content
                    // would not be shown when !COMPLETE_INCOMPLETE_TABLES, and
                    // for which we are not really able to set some node as
                    // erm_killed (for example, with TABLE > TABLE, the inner
                    // one will be rendered, but the outer one would have
                    // a height=0, and so the inner content will overflow
                    // its container and will not be drawn...)
                }
            }
        }
        if ( first_unproper >= 0 && (is_proper || is_last) ) {
            // We met unproper children, but we now have a proper child, or we're done:
            // wrap all these consecutive unproper nodes inside a single tabularBox
            // element with the proper rendering method.
            #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                printf("initTableRendMethods(%d): wrapping unproper %d>%d\n",
                            state, first_unproper, last_unproper);
            #endif
            int elems_removed = last_unproper - first_unproper + 1;
            ldomNode * tbox = enode->boxWrapChildren(first_unproper, last_unproper, el_tabularBox);
            if ( tbox && !tbox->isNull() ) {
                elems_removed -= 1; // tabularBox added
                if ( state==0 || state==2 ) { // in table or rowgroup
                    // No real need to store the style as an attribute: it would
                    // be remembered and re-used when styles change, and just
                    // setting the appropriate rendering method is all that is
                    // needed for rendering after this.
                    // tbox->setAttributeValue(LXML_NS_NONE, enode->getDocument()->getAttrNameIndex(L"style"), L"display: table-row");
                    tbox->initNodeStyle();
                    tbox->setRendMethod( erm_table_row );
                    cellCount += initTableRendMethods( tbox, 3 ); // > row
                }
                else if ( state==3 ) {
                    tbox->initNodeStyle();
                    tbox->setRendMethod( erm_table_cell );
                    cellCount++;
                    tbox->initNodeRendMethodRecursive(); // will reset rend method
                }
                else if ( state==1 ) { // should not happen, see above
                    tbox->initNodeStyle();
                    tbox->setRendMethod( erm_table_column );
                }
            }
            // If tbox is NULL, all unproper have been removed, and no element added
            if (is_last)
                break;
            // Account for what's been removed in our loop index and end
            i -= elems_removed;
            cnt -= elems_removed;
            first_unproper = -1;
            last_unproper = -1;
        }
    }
    // if ( state==0 ) {
    //     dumpRendMethods( enode, cs16("   ") );
    // }
    return cellCount;
}

bool hasInvisibleParent( ldomNode * node )
{
    for ( ; !node->isRoot(); node = node->getParentNode() )
        if ( node->getStyle()->display==css_d_none )
            return true;
    return false;
}

bool ldomNode::isFloatingBox() const
{
    // BLOCK_RENDERING_G(FLOAT_FLOATBOXES) is what triggers rendering
    // the floats floating. They are wrapped in a floatBox, possibly
    // not floating, when BLOCK_RENDERING_G(WRAP_FLOATS)).
    if ( BLOCK_RENDERING_G(FLOAT_FLOATBOXES) && getNodeId() == el_floatBox
                && getStyle()->float_ > css_f_none)
        return true;
    return false;
}

/// is node an inlineBox that has not been re-inlined by having
/// its child no more inline-block/inline-table
bool ldomNode::isBoxingInlineBox() const
{
    // BLOCK_RENDERING_G(BOX_INLINE_BLOCKS) is what ensures inline-block
    // are boxed and rendered as an inline block, but we may have them
    // wrapping a node that is no more inline-block (when some style
    // tweaks have changed the display: property).
    if ( getNodeId() == el_inlineBox && BLOCK_RENDERING_G(BOX_INLINE_BLOCKS) ) {
        if (getChildCount() == 1) {
            css_display_t d = getChildNode(0)->getStyle()->display;
            if (d == css_d_inline_block || d == css_d_inline_table) {
                return true;
            }
            return isEmbeddedBlockBoxingInlineBox(true); // avoid rechecking what we just checked
        }
    }
    return false;
}

/// is node an inlineBox that wraps a bogus embedded block (not inline-block/inline-table)
/// can be called with inline_box_checks_done=true when isBoxingInlineBox() has already
/// been called to avoid rechecking what is known
bool ldomNode::isEmbeddedBlockBoxingInlineBox(bool inline_box_checks_done) const
{
    if ( !inline_box_checks_done ) {
        if ( getNodeId() != el_inlineBox || !BLOCK_RENDERING_G(BOX_INLINE_BLOCKS) )
            return false;
        if (getChildCount() != 1)
            return false;
        css_display_t d = getChildNode(0)->getStyle()->display;
        if (d == css_d_inline_block || d == css_d_inline_table) {
            return false; // regular boxing inlineBox
        }
    }
    if ( hasAttribute( attr_type ) ) { // type="EmbeddedBlock"
            // (no other possible value yet, no need to compare strings)
        int cm = getChildNode(0)->getRendMethod();
        if ( cm == erm_inline || cm == erm_runin || cm == erm_invisible || cm == erm_killed )
            return false; // child has been reset to inline
        return true;
    }
    return false;
}

void ldomNode::initNodeRendMethod()
{
    // This method is called when re-rendering, but also while
    // initially loading a document.
    // On initial loading:
    //   A node's style is defined when the node element XML tag
    //   opening is processed (by lvrend.cpp setNodeStyle() which
    //   applies inheritance from its parent, which has
    //   already been parsed).
    //   This method is called when the node element XML tag is
    //   closed, so all its children are known, have styles, and
    //   have had this method called on them.
    // On re-rendering:
    //   Styles are first applied recursively, parents first (because
    //   of inheritance).
    //   This method is then called thru recurseElementsDeepFirst, so
    //   from deepest children up to their parents up to the root node.
    // So, this method should decide how this node is going to be
    // rendered (inline, block containing other blocks, or final block
    // containing only inlines), only from the node's own style, and
    // from the styles and rendering methods of its children.
    if ( !isElement() )
        return;
    if ( isRoot() ) {
        setRendMethod(erm_block);
        return;
    }

    // DEBUG TEST
    // if ( getParentNode()->getChildIndex( getDataIndex() )<0 ) {
    //     CRLog::error("Invalid parent->child relation for nodes %d->%d", getParentNode()->getDataIndex(), getDataIndex() );
    // }
    // if ( getNodeName() == "image" ) {
    //     CRLog::trace("Init log for image");
    // }

    // Needed if COMPLETE_INCOMPLETE_TABLES, so have it updated along
    // the way to avoid an extra loop for checking if we have some.
    bool hasInternalTableItems = false;

    int d = getStyle()->display;

    if ( hasInvisibleParent(this) ) { // (should be named isInvisibleOrHasInvisibleParent())
        // Note: we could avoid that up-to-root-node walk for each node
        // by inheriting css_d_none in setNodeStyle(), and just using
        // "if ( d==css_d_none )" instead of hasInvisibleParent(this).
        // But not certain this would have no side effect, and some
        // quick tests show no noticeable change in rendering timing.
        //
        //recurseElements( resetRendMethodToInvisible );
        setRendMethod(erm_invisible);
    } else if ( d==css_d_inline ) {
        // Used to be: an inline parent resets all its children to inline
        //   (so, if some block content is erroneously wrapped in a SPAN, all
        //   the content became inline...), except, depending on what's enabled:
        //   - nodes with float: which can stay block among inlines
        //   - the inner content of inlineBoxes (the inlineBox is already inline)
        //   recurseMatchingElements( resetRendMethodToInline, isNotBoxWrappingNode );
        //
        // But we don't want to "reset all its children to inline" when a bogus
        // spurious block element happens to be inside some inline one, as this
        // can be seen happening (<small> multiple <p>...</small>).
        // So, when BOX_INLINE_BLOCKS is enabled, we wrap such block elements inside
        // a <inlineBox> element, nearly just like if it were "display: inline-block",
        // with a few tweaks in its rendering (see below).
        // Or, if it contains only block elements, and empty text nodes, we can just
        // set this inline element to be erm_block.
        //
        // Some discussions about that "block inside inline" at:
        //   https://github.com/w3c/csswg-drafts/issues/1477
        //   https://stackoverflow.com/questions/1371307/displayblock-inside-displayinline
        //
        if ( !BLOCK_RENDERING_G(BOX_INLINE_BLOCKS) ) {
            // No support for anything but inline elements, and possibly embedded floats
            recurseMatchingElements( resetRendMethodToInline, isNotBoxWrappingNode );
        }
        else if ( !isNotBoxWrappingNode(this) ) {
            // If this node is already a box wrapping node (active floatBox or inlineBox,
            // possibly a <inlineBox type="EmbeddedBlock"> created here in a previous
            // rendering), just set it to erm_inline.
            setRendMethod(erm_inline);
        }
        else {
            // Set this inline element to be erm_inline, and look at its children
            setRendMethod(erm_inline);
            // Quick scan first, before going into more checks if needed
            bool has_block_nodes = false;
            bool has_inline_nodes = false;
            for ( int i=0; i < getChildCount(); i++ ) {
                ldomNode * child = getChildNode( i );
                if ( !child->isElement() ) // text node
                    continue;
                int cm = child->getRendMethod();
                if ( cm == erm_inline || cm == erm_runin ) {
                    has_inline_nodes = true; // We won't be able to make it erm_block
                    continue;
                }
                if ( cm == erm_invisible || cm == erm_killed )
                    continue;
                if ( !isNotBoxWrappingNode( child ) ) {
                    // This child is already wrapped by a floatBox or inlineBox
                    continue;
                }
                has_block_nodes = true;
                if ( has_inline_nodes )
                    break; // we know enough
            }
            if ( has_block_nodes ) {
                bool has_non_empty_text_nodes = false;
                bool do_wrap_blocks = true;
                if ( !has_inline_nodes ) {
                    // No real inline nodes. Inspect each text node to see if they
                    // are all empty text.
                    for ( int i=0; i < getChildCount(); i++ ) {
                        if ( getChildNode(i)->isText() ) {
                            lString16 s = getChildNode(i)->getText();
                            if ( !IsEmptySpace(s.c_str(), s.length() ) ) {
                                has_non_empty_text_nodes = true;
                                break;
                            }
                        }
                    }
                    if ( !has_non_empty_text_nodes ) {
                        // We can be a block wrapper (renderBlockElementEnhanced/Legacy will
                        // skip empty text nodes, no need to remove them)
                        setRendMethod(erm_block);
                        do_wrap_blocks = false;
                    }
                }
                if ( do_wrap_blocks ) {
                    // We have a mix of inline nodes or non-empty text, and block elements:
                    // wrap each block element in a <inlineBox type="EmbeddedBlock">.
                    for ( int i=getChildCount()-1; i >=0; i-- ) {
                        ldomNode * child = getChildNode( i );
                        if ( !child->isElement() ) // text node
                            continue;
                        int cm = child->getRendMethod();
                        if ( cm == erm_inline || cm == erm_runin || cm == erm_invisible || cm == erm_killed )
                            continue;
                        if ( !isNotBoxWrappingNode( child ) )
                            continue;
                        // This child is erm_block or erm_final (or some other erm_table like rend method).
                        // It will be inside a upper erm_final
                        // Wrap this element into an inlineBox, just as if it was display:inline-block,
                        // with a few differences that will be handled by lvrend.cpp/lvtextfm.cpp:
                        // - it should behave like if it has width: 100%, so preceeding
                        //   and following text/inlines element will be on their own line
                        // - the previous line should not be justified
                        // - in the matter of page splitting, lines (as they are 100%-width) should
                        //   be forwarded to the parent flow/context
                        // Remove any preceeding or following empty text nodes (there can't
                        // be consecutive text nodes) so we don't get spurious empty lines.
                        if ( i < getChildCount()-1 && getChildNode(i+1)->isText() ) {
                            lString16 s = getChildNode(i+1)->getText();
                            if ( IsEmptySpace(s.c_str(), s.length() ) ) {
                                removeChildren(i+1, i+1);
                            }
                        }
                        if ( i > 0 && getChildNode(i-1)->isText() ) {
                            lString16 s = getChildNode(i-1)->getText();
                            if ( IsEmptySpace(s.c_str(), s.length() ) ) {
                                removeChildren(i-1, i-1);
                                i--; // update our position
                            }
                        }
                        ldomNode * ibox = insertChildElement( i, LXML_NS_NONE, el_inlineBox );
                        moveItemsTo( ibox, i+1, i+1 ); // move this child from 'this' into ibox
                        // Mark this inlineBox so we can handle its pecularities
                        ibox->setAttributeValue(LXML_NS_NONE, getDocument()->getAttrNameIndex(L"type"), L"EmbeddedBlock");
                        setNodeStyle( ibox, getStyle(), getFont() );
                        ibox->setRendMethod( erm_inline );
                    }
                }
            }
        }
    } else if ( d==css_d_run_in ) {
        // runin
        //CRLog::trace("switch all children elements of <%s> to inline", LCSTR(getNodeName()));
        recurseElements( resetRendMethodToInline );
        setRendMethod(erm_runin);
    } else if ( d==css_d_list_item ) {
        // list item (no more used, obsolete rendering method)
        setRendMethod(erm_list_item);
    } else if ( d==css_d_table ) {
        // table: this will "Generate missing child wrappers" if needed
        initTableRendMethods( this, 0 );
        // Not sure if we should do the same for the other css_d_table_* and
        // call initTableRendMethods(this, 1/2/3) so that the "Generate missing
        // child wrappers" step is done before the "Generate missing parents" step
        // we might be doing below - to conform to the order of steps in the specs.
    } else if ( d==css_d_inline_table && ( BLOCK_RENDERING_G(COMPLETE_INCOMPLETE_TABLES) || getNodeId()==el_table ) ) {
        // Only if we're able to complete incomplete tables, or if this
        // node is itself a <TABLE>. Otherwise, fallback to the following
        // catch-all 'else' and render its content as block.
        //   (Note that we should skip that if the node is an image, as
        //   initTableRendMethods() would not be able to do anything with
        //   it as it can't add children to an IMG. Hopefully, the specs
        //   say replaced elements like IMG should not have table-like
        //   display: values - which setNodeStyle() ensures.)
        // Any element can have "display: inline-table", and if it's not
        // a TABLE, initTableRendMethods() will complete/wrap it to make
        // it possibly the single cell of a TABLE. This should naturally
        // ensure all the differences between inline-block and inline-table.
        // https://stackoverflow.com/questions/19352072/what-is-the-difference-between-inline-block-and-inline-table/19352149#19352149
        initTableRendMethods( this, 0 );
        // Note: if (d==css_d_inline_block && getNodeId()==el_table), we
        // should NOT call initTableRendMethods()! It should be rendered
        // as a block, and if its children are actually TRs, they will be
        // wrapped in a "missing parent" tabularBox wrapper that will
        // have initTableRendMethods() called on it.
    } else {
        // block or final
        // remove last empty space text nodes
        bool hasBlockItems = false;
        bool hasInline = false;
        bool hasFloating = false;
        // Floating nodes, thus block, are accounted apart from inlines
        // and blocks, as their behaviour is quite specific.
        // - When !PREPARE_FLOATBOXES, we just don't deal specifically with
        //   floats, for a rendering more similar to legacy rendering: SPANs
        //   with float: will be considered as non-floating inline, while
        //   DIVs with float: will be considered as block elements, possibly
        //   causing autoBoxing of surrounding content with only inlines.
        // - When PREPARE_FLOATBOXES (even if !FLOAT_FLOATBOXES), we do prepare
        //   floats and floatBoxes to be consistent, ready to be floating, or
        //   not and flat (with a rendering possibly not similar to legacy),
        //   without any display hash mismatch (so that toggling does not
        //   require a full reloading). SPANs and DIVs with float: mixed with
        //   inlines will be considered as inline when !FLOAT_FLOATBOXES, to
        //   avoid having autoBoxing elements that would mess with a correct
        //   floating rendering.
        // Note that FLOAT_FLOATBOXES requires having PREPARE_FLOATBOXES.
        bool handleFloating = BLOCK_RENDERING_G(PREPARE_FLOATBOXES);

        detectChildTypes( this, hasBlockItems, hasInline, hasInternalTableItems, hasFloating, handleFloating );
        const css_elem_def_props_t * ntype = getElementTypePtr();
        if (ntype && ntype->is_object) { // image
            // No reason to erm_invisible an image !
            // And it has to be erm_final to be drawn (or set to erm_inline
            // by some upper node).
            // (Note that setNodeStyle() made sure an image can't be
            // css_d_inline_table/css_d_table*, as per specs.)
            setRendMethod( erm_final );
            /* used to be:
            switch ( d )
            {
            case css_d_block:
            case css_d_list_item_block:
            case css_d_inline:
            case css_d_inline_block:
            case css_d_inline_table:
            case css_d_run_in:
                setRendMethod( erm_final );
                break;
            default:
                //setRendMethod( erm_invisible );
                recurseElements( resetRendMethodToInvisible );
                break;
            }
            */
        } else if ( hasBlockItems && !hasInline ) {
            // only blocks (or floating blocks) inside
            setRendMethod( erm_block );
        } else if ( !hasBlockItems && hasInline ) {
            // only inline (with possibly floating blocks that will
            // be dealt with by renderFinalBlock)
            if ( hasFloating ) {
                // If all the inline elements are empty space, we may as well
                // remove them and have our floats contained in a erm_block
                if ( cleanIfOnlyEmptyTextInline(true) ) {
                    setRendMethod( erm_block );
                }
                else {
                    if ( !BLOCK_RENDERING_G(FLOAT_FLOATBOXES) ) {
                        // If we don't want floatBoxes floating, reset them to be
                        // rendered inline among inlines
                        recurseMatchingElements( resetRendMethodToInline, isNotBoxingInlineBoxNode );
                    }
                    setRendMethod( erm_final );
                }
            }
            else {
                setRendMethod( erm_final );
            }
        } else if ( !hasBlockItems && !hasInline ) {
            // nothing (or only floating blocks)
            // (don't ignore it as it might be some HR with borders/padding,
            // even if no content)
            setRendMethod( erm_block );
        } else if ( hasBlockItems && hasInline ) {
            // Mixed content of blocks and inline elements:
            // the consecutive inline elements should be considered part
            // of an anonymous block element - non-anonymous for crengine,
            // as we create a <autoBoxing> element and add it to the DOM),
            // taking care of ignoring unvaluable inline elements consisting
            // of only spaces.
            //   Note: when there are blocks, inlines and floats mixed, we could
            //   choose to let the floats be blocks, or include them with the
            //   surrounding inlines into an autoBoxing:
            //   - blocks: they will just be footprints (so, only 2 squares at
            //   top left and right) over the inline/final content, and when
            //   there are many, the text may not wrap fully around the floats...
            //   - with inlines: they will wrap fully, but if the text is short,
            //   the floats will be cleared, and there will be blank vertical
            //   filling space...
            //   The rendering can be really different, and there's no real way
            //   of knowing which will be the best.
            //   So, for now, go with including them with inlines into the
            //   erm_final autoBoxing.
            // The above has become less critical after we added DO_NOT_CLEAR_OWN_FLOATS
            // and ALLOW_EXACT_FLOATS_FOOTPRINTS, and both options should render
            // similarly.
            // But still going with including them with inlines is best, as we
            // don't need to include them in the footprint (so, the limit of
            // 5 outer block float IDs is still available for real outer floats).
            if ( getParentNode()->getNodeId()==el_autoBoxing ) {
                // already autoboxed
                setRendMethod( erm_final );
                // This looks wrong: no reason to force child of autoBoxing to be
                // erm_final: most often, the autoBoxing has been created to contain
                // only inlines and set itself to be erm_final. So, it would have been
                // caught by the 'else if ( !hasBlockItems && hasInline )' above and
                // set to erm_final. If not, styles have changed, and it may contain
                // a mess of styles: it might be better to proceed with the following
                // cleanup (and have autoBoxing re-autoboxed... or not at all when
                // a cache file is used, and we'll end up being erm_final anyway).
                // But let's keep it, in case it handles some edge cases.
            } else {
                // cleanup or autobox
                int i=getChildCount()-1;
                for ( ; i>=0; i-- ) {
                    ldomNode * node = getChildNode(i);

                    // DEBUG TEST
                    // if ( getParentNode()->getChildIndex( getDataIndex() )<0 ) {
                    //    CRLog::error("Invalid parent->child relation for nodes %d->%d",
                    //              getParentNode()->getDataIndex(), getDataIndex() );
                    // }

                    // We want to keep float:'ing nodes with inline nodes, so they stick with their
                    // siblings inline nodes in an autoBox: the erm_final autoBox will deal
                    // with rendering the floating node, and the inline text around it
                    if ( isInlineNode(node) || (handleFloating && isFloatingNode(node)) ) {
                        int j = i-1;
                        for ( ; j>=0; j-- ) {
                            node = getChildNode(j);
                            if ( !isInlineNode(node) && !(handleFloating && isFloatingNode(node)) )
                                break;
                        }
                        j++;
                        // j..i are inline
                        if ( j>0 || i<(int)getChildCount()-1 )
                            autoboxChildren( j, i, handleFloating );
                        i = j;
                    }
                    else if ( i>0 ) {
                        // This node is not inline, but might be preceeded by a css_d_run_in node:
                        // https://css-tricks.com/run-in/
                        // https://developer.mozilla.org/en-US/docs/Web/CSS/display
                        //   "If the adjacent sibling of the element defined as "display: run-in" box
                        //   is a block box, the run-in box becomes the first inline box of the block
                        //   box that follows it. "
                        // Hopefully only used for footnotes in fb2 where the footnote number
                        // is in a block element, and the footnote text in another.
                        // fb2.css sets the first block to be "display: run-in" as an
                        // attempt to render both on the same line:
                        //   <section id="n1">
                        //     <title style="display: run-in; font-weight: bold;">
                        //       <p>1</p>
                        //     </title>
                        //     <p>Text footnote</p>
                        //   </section>
                        //
                        // This node might be that second block: look if preceeding node
                        // is "run-in", and if it is, bring them both in an autoBoxing.
                        ldomNode * prev = getChildNode(i-1);
                        ldomNode * inBetweenTextNode = NULL;
                        if ( prev->isText() && i-1>0 ) { // some possible empty text in between
                            inBetweenTextNode = prev;
                            prev = getChildNode(i-2);
                        }
                        if ( prev->isElement() && prev->getRendMethod()==erm_runin ) {
                            bool do_autoboxing = true;
                            int run_in_idx = inBetweenTextNode ? i-2 : i-1;
                            int block_idx = i;
                            if ( inBetweenTextNode ) {
                                lString16 text = inBetweenTextNode->getText();
                                if ( IsEmptySpace(text.c_str(), text.length() ) ) {
                                    removeChildren(i-1, i-1);
                                    block_idx = i-1;
                                }
                                else {
                                    do_autoboxing = false;
                                }
                            }
                            if ( do_autoboxing ) {
                                CRLog::debug("Autoboxing run-in items");
                                // Sadly, to avoid having an erm_final inside another erm_final,
                                // we need to reset the block node to be inline (but that second
                                // erm_final would have been handled as inline anyway, except
                                // for possibly updating the strut height/baseline).
                                node->recurseMatchingElements( resetRendMethodToInline, isNotBoxingInlineBoxNode );
                                // No need to autobox if there are only 2 children (the run-in and this box)
                                if ( getChildCount()!=2 ) { // autobox run-in
                                    autoboxChildren( run_in_idx, block_idx, handleFloating );
                                }
                            }
                            i = run_in_idx;
                        }
                    }
                }
                // check types after autobox
                detectChildTypes( this, hasBlockItems, hasInline, hasInternalTableItems, hasFloating, handleFloating );
                if ( hasInline ) {
                    // Should not happen when autoboxing has been done above - but
                    // if we couldn't, fallback to erm_final that will render all
                    // children as inline
                    setRendMethod( erm_final );
                } else {
                    // All inlines have been wrapped into block autoBoxing elements
                    // (themselves erm_final): we can be erm_block
                    setRendMethod( erm_block );
                }
            }
        }
    }

    if ( hasInternalTableItems && BLOCK_RENDERING_G(COMPLETE_INCOMPLETE_TABLES) && getRendMethod() == erm_block ) {
        // We have only block items, whether the original ones or the
        // autoBoxing nodes we created to wrap inlines, and all empty
        // inlines have been removed.
        // Some of these block items are css_d_table_cell, css_d_table_row...:
        // if this node (their parent) has not the expected css_d_table_row
        // or css_d_table display style, we are an unproper parent: we want
        // to add the missing parent(s) as wrapper(s) between this node and
        // these children.
        // (If we ended up not being erm_block, and we contain css_d_table_*
        // elements, everything is already messed up.)
        // Note: we first used the same <autoBoxing> element used to box
        // inlines as the table wrapper, which was fine, except in some edge
        // cases where some real autoBoxing were wrongly re-used as the tabular
        // wrapper (and we ended up having erm_final containing other erm_final
        // which were handled just like erm_inline with ugly side effects...)
        // So, best to introduce a decicated element: <tabularBox>.
        //
        // We follow rules from section "Generate missing parents" in:
        //   https://www.w3.org/TR/CSS22/tables.html#anonymous-boxes
        //   https://www.w3.org/TR/css-tables-3/#fixup (clearer than previous one)
        // Note: we do that not in the order given by the specs... As we walk
        // nodes deep first, we are here first "generating missing parents".
        // When walking up, and meeting a real css_d_table element, or
        // below when adding a generated erm_table tabularBox, we call
        // initTableRendMethods(0), which will "generate missing child wrappers".
        // Not really sure both orderings are equivalent, but let's hope it's ok...

        // So, let's generate missing parents:

        // "An anonymous table-row box must be generated around each sequence
        // of consecutive table-cell boxes whose parent is not a table-row."
        if ( d != css_d_table_row ) { // We're not a table row
            // Look if we have css_d_table_cell that we must wrap in a proper erm_table_row
            int last_table_cell = -1;
            int first_table_cell = -1;
            int last_visible_child = -1;
            bool did_wrap = false;
            int len = getChildCount();
            for ( int i=len-1; i>=0; i-- ) {
                ldomNode * child = getChildNode(i);
                int cd = child->getStyle()->display;
                int cm = child->getRendMethod();
                if ( cd == css_d_table_cell ) {
                    if ( last_table_cell < 0 ) {
                        last_table_cell = i;
                        // We've met a css_d_table_cell, see if it is followed by
                        // tabularBox siblings we might have passed by: they might
                        // have been added by initTableRendMethods as a missing
                        // children of a css_d_table_row: make them part of the row.
                        for (int j=i+1; j<getChildCount(); j++) {
                            if ( getChildNode(j)->getNodeId()==el_tabularBox )
                                last_table_cell = j;
                            else
                                break;
                        }
                    }
                    if ( i == 0 )
                        first_table_cell = 0;
                    if ( last_visible_child < 0 )
                        last_visible_child = i;
                }
                else if ( last_table_cell >= 0 && child->getNodeId()==el_tabularBox ) {
                    // We've seen a css_d_table_cell and we're seeing a tabularBox:
                    // it might have been added by initTableRendMethods as a missing
                    // children of a css_d_table_row: make it part of the row
                    if ( i == 0 )
                        first_table_cell = 0;
                    if ( last_visible_child < 0 )
                        last_visible_child = i;
                }
                else if ( cd == css_d_none || cm == erm_invisible ) {
                    // Can be left inside or outside the wrap
                    if ( i == 0 && last_table_cell >= 0 ) {
                        // Include it if first and we're wrapping
                        first_table_cell = 0;
                    }
                }
                else {
                    if ( last_table_cell >= 0)
                        first_table_cell = i+1;
                    if ( last_visible_child < 0 )
                        last_visible_child = i;
                }
                if ( first_table_cell >= 0 ) {
                    if ( first_table_cell == 0 && last_table_cell == last_visible_child
                                && getNodeId()==el_tabularBox && !did_wrap ) {
                        // All children are table cells, and we're not css_d_table_row,
                        // but we are a tabularBox!
                        // We were most probably created here in a previous rendering,
                        // so just set us to be the anonymous table row.
                        #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                            printf("initNodeRendMethod: (reused)wrapping unproper table cells %d>%d\n",
                                        first_table_cell, last_table_cell);
                        #endif
                        setRendMethod( erm_table_row );
                    }
                    #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                        printf("initNodeRendMethod: wrapping unproper table cells %d>%d\n",
                               first_table_cell, last_table_cell);
                    #endif
                    ldomNode * tbox = boxWrapChildren(first_table_cell, last_table_cell, el_tabularBox);
                    if ( tbox && !tbox->isNull() ) {
                        tbox->initNodeStyle();
                        tbox->setRendMethod( erm_table_row );
                    }
                    did_wrap = true;
                    last_table_cell = -1;
                    first_table_cell = -1;
                }
            }
        }

        // "An anonymous table or inline-table box must be generated around each
        // sequence of consecutive proper table child boxes which are misparented."
        // Not sure if we should skip that for some values of this node's
        // style->display among css_d_table*. Let's do as litterally as the specs.
        int last_misparented = -1;
        int first_misparented = -1;
        int last_visible_child = -1;
        bool did_wrap = false;
        int len = getChildCount();
        for ( int i=len-1; i>=0; i-- ) {
            ldomNode * child = getChildNode(i);
            int cd = child->getStyle()->display;
            int cm = child->getRendMethod();
            bool is_misparented = false;
            if ( (cd == css_d_table_row || cm == erm_table_row)
                            && d != css_d_table && d != css_d_table_row_group
                            && d != css_d_table_header_group && d != css_d_table_footer_group ) {
                // A table-row is misparented if its parent is neither a table-row-group
                // nor a table-root box (we include by checking cm==erm_table_row any
                // anonymous table row created just above).
                is_misparented = true;
            }
            else if ( cd == css_d_table_column && d != css_d_table && d != css_d_table_column_group ) {
                // A table-column box is misparented if its parent is neither
                // a table-column-group box nor a table-root box.
                is_misparented = true;
            }
            else if ( d != css_d_table && (cd == css_d_table_row_group || cd == css_d_table_header_group
                                            || cd == css_d_table_footer_group || cd == css_d_table_column_group
                                            || cd == css_d_table_caption ) ) {
                // A table-row-group, table-column-group, or table-caption box is misparented
                // if its parent is not a table-root box.
                is_misparented = true;
            }
            if ( is_misparented ) {
                if ( last_misparented < 0 ) {
                    last_misparented = i;
                    // As above for table cells: grab passed-by tabularBox siblings
                    // to include them in the wrap
                    for (int j=i+1; j<getChildCount(); j++) {
                        if ( getChildNode(j)->getNodeId()==el_tabularBox )
                            last_misparented = j;
                        else
                            break;
                    }
                }
                if (i == 0)
                    first_misparented = 0;
                if ( last_visible_child < 0 )
                    last_visible_child = i;
            }
            else if ( last_misparented >= 0 && child->getNodeId()==el_tabularBox ) {
                // As above for table cells: include tabularBox siblings in the wrap
                if (i == 0)
                    first_misparented = 0;
                if ( last_visible_child < 0 )
                    last_visible_child = i;
            }
            else if ( cd == css_d_none || cm == erm_invisible ) {
                // Can be left inside or outside the wrap
                if ( i == 0 && last_misparented >= 0 ) {
                    // Include it if first and we're wrapping
                    first_misparented = 0;
                }
            }
            else {
                if ( last_misparented >= 0 )
                    first_misparented = i+1;
                if ( last_visible_child < 0 )
                    last_visible_child = i;
            }
            if ( first_misparented >= 0 ) {
                if ( first_misparented == 0 && last_misparented == last_visible_child
                            && getNodeId()==el_tabularBox && !did_wrap ) {
                    // All children are misparented, and we're not css_d_table,
                    // but we are a tabularBox!
                    // We were most probably created here in a previous rendering,
                    // so just set us to be the anonymous table.
                    #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                        printf("initNodeRendMethod: (reused)wrapping unproper table children %d>%d\n",
                                    first_misparented, last_misparented);
                    #endif
                    setRendMethod( erm_table );
                    initTableRendMethods( this, 0 );
                }
                #ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                    printf("initNodeRendMethod: wrapping unproper table children %d>%d\n",
                                first_misparented, last_misparented);
                #endif
                ldomNode * tbox = boxWrapChildren(first_misparented, last_misparented, el_tabularBox);
                if ( tbox && !tbox->isNull() ) {
                    tbox->initNodeStyle();
                    tbox->setRendMethod( erm_table );
                    initTableRendMethods( tbox, 0 );
                }
                did_wrap = true;
                last_misparented = -1;
                first_misparented = -1;
                // Note:
                //   https://www.w3.org/TR/css-tables-3/#fixup
                //   "An anonymous table or inline-table box must be generated
                //    around [...] If the box's parent is an inline, run-in, or
                //    ruby box (or any box that would perform inlinification of
                //    its children), then an inline-table box must be generated;
                //    otherwise it must be a table box."
                // We don't handle the "inline parent > inline-table" rule,
                // because of one of the first checks at top of this function:
                // if this node (the parent) is css_d_inline, we didn't have
                // any detectChildTypes() and autoBoxing happening, stayed erm_inline
                // and didn't enter this section to do the tabularBox wrapping.
                // Changing this (incorrect) rule for css_d_inline opens many
                // bigger issues, so let's not support this (rare) case here.
                // So:
                //   <div>Some text <span style="display: table-cell">table-cell</span> and more text.</div>
                // will properly have the cell tabularBoxes'ed, which will be
                // inserted between 2 autoBoxing (the text nodes), because their
                // container is css_d_block DIV.
                // While:
                //   <div><span>Some text <span style="display: table-cell">table-cell</span> and more text.</span></div>
                // as the container is a css_d_inline SPAN, nothing will happen
                // and everything will be reset to erm_inline. The parent DIV
                // will just see that it contains a single erm_inline SPAN,
                // and won't do any boxing.
            }
        }
    }

    bool handled_as_float = false;
    if (BLOCK_RENDERING_G(WRAP_FLOATS)) {
        // While loading the document, we want to put any element with float:left/right
        // inside an internal floatBox element with no margin in its style: this
        // floatBox's RenderRectAccessor will have the position and width/height
        // of the outer element (with margins inside), while the RenderRectAccessor
        // of the wrapped original element itself will have the w/h of the element,
        // including borders but excluding margins (as it is done for all elements
        // by crengine).
        // That makes out the following rules:
        // - a floatBox has a single child: the original floating element.
        // - a non-floatBox element with style->float_ must be wrapped in a floatBox
        //   which will get the same style->float_ (happens in the initial document
        //   loading)
        // - if it already has a floatBox parent, no need to do it again, just ensure
        //   the style->float_ are the same (happens when re-rendering)
        // - if the element has lost its style->float_ (style tweak applied), or
        //   WRAP_FLOATS disabled, as we can't remove the floatBox (we can't
        //   modify the DOM once a cache has been made): update the floatBox's
        //   style->float_ and style->display and rendering method to be the same
        //   as the element: this will limit the display degradation when such
        //   change happen (but a full re-loading will still be suggested to the
        //   user, and should probably be accepted).
        // So, to allow toggling FLOAT_FLOATBOXES with less chance of getting
        // a _nodeDisplayStyleHash change (and so, a need for document reloading),
        // it's best to use WRAP_FLOATS even when flat rendering is requested.
        //
        // Note that, when called in the XML loading phase, we can't update
        // a node style (with getStyle(), copystyle(), setStyle()) as, for some reason
        // not pinpointed, it could affect and mess with the upcoming node parsing.
        // We can just set the style of an element we add (and only once, setting it
        // twice would cause the same mess). But in the re-rendering phase, we can
        // update a node style as much as we want.
        bool isFloating = getStyle()->float_ > css_f_none;
        bool isFloatBox = (getNodeId() == el_floatBox);
        if ( isFloating || isFloatBox ) {
            handled_as_float = true;
            ldomNode * parent = getParentNode();
            bool isFloatBoxChild = (parent && (parent->getNodeId() == el_floatBox));
            if ( isFloatBox ) {
                // Wrapping floatBox already made
                if (getChildCount() != 1) {
                    CRLog::error("floatBox with zero or more than one child");
                    crFatalError();
                }
                // Update floatBox style according to child's one
                ldomNode * child = getChildNode(0);
                css_style_ref_t child_style = child->getStyle();
                css_style_ref_t my_style = getStyle();
                css_style_ref_t my_new_style( new css_style_rec_t );
                copystyle(my_style, my_new_style);
                my_new_style->float_ = child_style->float_;
                if (child_style->display == css_d_inline) { // when !PREPARE_FLOATBOXES
                    my_new_style->display = css_d_inline; // become an inline wrapper
                }
                else if (child_style->display == css_d_none) {
                    my_new_style->display = css_d_none; // stay invisible
                }
                else { // everything else (including tables) must be wrapped by a block
                    my_new_style->display = css_d_block;
                }
                setStyle(my_new_style);
                // When re-rendering, setNodeStyle() has already been called to set
                // our style and font, so no need for calling initNodeFont() here,
                // as we didn't change anything related to font in the style (and
                // calling it can cause a style hash mismatch for some reason).

                // Update floatBox rendering method according to child's one
                // It should be erm_block by default (the child can be erm_final
                // if it contains text), except if the child has stayed inline
                // when !PREPARE_FLOATBOXES
                if (child->getRendMethod() == erm_inline)
                    setRendMethod( erm_inline );
                else if (child->getRendMethod() == erm_invisible)
                    setRendMethod( erm_invisible );
                else
                    setRendMethod( erm_block );
            }
            else if ( isFloatBoxChild ) {
                // Already floatBox'ed, nothing special to do
            }
            else if ( parent ) { // !isFloatBox && !isFloatBoxChild
                // Element with float:, that has not been yet wrapped in a floatBox.
                // Replace this element with a floatBox in its parent children collection,
                // and move it inside, as the single child of this floatBox.
                int pos = getNodeIndex();
                ldomNode * fbox = parent->insertChildElement( pos, LXML_NS_NONE, el_floatBox );
                parent->moveItemsTo( fbox, pos+1, pos+1 ); // move this element from parent into fbox
                
                // If we have float:, this just-created floatBox should be erm_block,
                // unless the child has been kept inline
                if ( !BLOCK_RENDERING_G(PREPARE_FLOATBOXES) && getRendMethod() == erm_inline)
                    fbox->setRendMethod( erm_inline );
                else
                    fbox->setRendMethod( erm_block );
                
                // We want this floatBox to have no real style (and it surely
                // should not have the margins of the child), but it should probably
                // have the inherited properties of the node parent, just like the child
                // had them. We can't just copy the parent style into this floatBox, as
                // we don't want its non-inherited properties like background-color which
                // could be drawn over some other content if this float has some negative
                // margins.
                // So, we can't really do this:
                //    // Move float and display from me into my new fbox parent
                //    css_style_ref_t mystyle = getStyle();
                //    css_style_ref_t parentstyle = parent->getStyle();
                //    css_style_ref_t fboxstyle( new css_style_rec_t );
                //    copystyle(parentstyle, fboxstyle);
                //    fboxstyle->float_ = mystyle->float_;
                //    fboxstyle->display = mystyle->display;
                //    fbox->setStyle(fboxstyle);
                //    fbox->initNodeFont();
                //
                // Best to use lvrend.cpp setNodeStyle(), which will properly set
                // this new node style with inherited properties from its parent,
                // and we made it do this specific propagation of float_ and
                // display from its single children, only when it has styles
                // defined (so, only on initial loading and not on re-renderings).
                setNodeStyle( fbox, parent->getStyle(), parent->getFont() );
                
                // We would have liked to reset style->float_ to none in the
                // node we moved in the floatBox, for correctness sake.
                //    css_style_ref_t mynewstyle( new css_style_rec_t );
                //    copystyle(mystyle, mynewstyle);
                //    mynewstyle->float_ = css_f_none;
                //    mynewstyle->display = css_d_block;
                //    setStyle(mynewstyle);
                //    initNodeFont();
                // Unfortunatly, we can't yet re-set a style while the DOM
                // is still being built (as we may be called during the loading
                // phase) without many font glitches.
                // So, we'll have a floatBox with float: that contains a span
                // or div with float: - the rendering code may have to check
                // for that: ->isFloatingBox() was added for that.
            }
        }
    }

    // (If a node is both inline-block and float: left/right, float wins.)
    if (BLOCK_RENDERING_G(BOX_INLINE_BLOCKS) && !handled_as_float) {
        // (Similar to what we do above for floats, but simpler.)
        // While loading the document, we want to put any element with
        // display: inline-block or inline-table inside an internal inlineBox
        // element with no margin in its style: this inlineBox's RenderRectAccessor
        // will have the width/height of the outer element (with margins inside),
        // while the RenderRectAccessor of the wrapped original element itself
        // will have the w/h of the element, including borders but excluding
        // margins (as it is done for all elements by crengine).
        // That makes out the following rules:
        // - a inlineBox has a single child: the original inline-block element.
        // - an element with style->display: inline-block/inline-table must be
        //   wrapped in a inlineBox, which will get the same style->vertical_align
        //   (happens in the initial document loading)
        // - if it already has a inlineBox parent, no need to do it again, just ensure
        //   the style->vertical_align are the same (happens when re-rendering)
        // - if the element has lost its style->display: inline-block (style tweak
        //   applied), or BOX_INLINE_BLOCKS disabled, as we can't remove the
        //   inlineBox (we can't modify the DOM once a cache has been made):
        //   the inlineBox and its children will both be set to erm_inline
        //   (but as ->display has changed, a full re-loading will be suggested
        //   to the user, and should probably be accepted).
        // - a inlineBox has ALWAYS ->display=css_d_inline and erm_method=erm_inline
        // - a inlineBox child keeps its original ->display, and may have
        //   erm_method = erm_final or erm_block (depending on its content)
        bool isInlineBlock = (d == css_d_inline_block || d == css_d_inline_table);
        bool isInlineBox = (getNodeId() == el_inlineBox);
        if ( isInlineBlock || isInlineBox ) {
            ldomNode * parent = getParentNode();
            bool isInlineBoxChild = (parent && (parent->getNodeId() == el_inlineBox));
            if ( isInlineBox ) {
                // Wrapping inlineBox already made
                if (getChildCount() != 1) {
                    CRLog::error("inlineBox with zero or more than one child");
                    crFatalError();
                }
                // Update inlineBox style according to child's one
                ldomNode * child = getChildNode(0);
                css_style_ref_t child_style = child->getStyle();
                css_style_ref_t my_style = getStyle();
                css_style_ref_t my_new_style( new css_style_rec_t );
                copystyle(my_style, my_new_style);
                if (child_style->display == css_d_inline_block || child_style->display == css_d_inline_table) {
                    my_new_style->display = css_d_inline; // become an inline wrapper
                    // We need it to have the vertical_align from the child
                    // (it's the only style we need for proper inline layout).
                    my_new_style->vertical_align = child_style->vertical_align;
                    setRendMethod( erm_inline );
                }
                else if ( isEmbeddedBlockBoxingInlineBox(true) ) {
                    my_new_style->display = css_d_inline; // wrap bogus "block among inlines" in inline
                    setRendMethod( erm_inline );
                }
                else if (child_style->display == css_d_inline) {
                    my_new_style->display = css_d_inline; // wrap inline in inline
                    setRendMethod( erm_inline );
                }
                else if (child_style->display == css_d_none) {
                    my_new_style->display = css_d_none; // stay invisible
                    setRendMethod( erm_invisible );
                }
                else { // everything else must be wrapped by a block
                    my_new_style->display = css_d_block;
                    setRendMethod( erm_block );
                }
                setStyle(my_new_style);
                // When re-rendering, setNodeStyle() has already been called to set
                // our style and font, so no need for calling initNodeFont() here,
                // as we didn't change anything related to font in the style (and
                // calling it can cause a style hash mismatch for some reason).
            }
            else if ( isInlineBoxChild ) {
                // Already inlineBox'ed, nothing special to do
            }
            else if ( parent ) { // !isInlineBox && !isInlineBoxChild
                // Element with display: inline-block/inline-table, that has not yet
                // been wrapped in a inlineBox.
                // Replace this element with a inlineBox in its parent children collection,
                // and move it inside, as the single child of this inlineBox.
                int pos = getNodeIndex();
                ldomNode * ibox = parent->insertChildElement( pos, LXML_NS_NONE, el_inlineBox );
                parent->moveItemsTo( ibox, pos+1, pos+1 ); // move this element from parent into ibox
                ibox->setRendMethod( erm_inline );
                
                // We want this inlineBox to have no real style (and it surely
                // should not have the margins of the child), but it should probably
                // have the inherited properties of the node parent, just like the child
                // had them. We can't just copy the parent style into this inlineBox, as
                // we don't want its non-inherited properties like background-color which
                // could be drawn over some other content if this float has some negative
                // margins.
                // Best to use lvrend.cpp setNodeStyle(), which will properly set
                // this new node style with inherited properties from its parent,
                // and we made it do this specific propagation of vertical_align
                // from its single child, only when it has styles defined (so,
                // only on initial loading and not on re-renderings).
                setNodeStyle( ibox, parent->getStyle(), parent->getFont() );
            }
        }
    }
}
#endif

void ldomElementWriter::onBodyExit()
{
    if ( _isSection )
        updateTocItem();

#if BUILD_LITE!=1
    if ( !_document->isDefStyleSet() )
        return;
    if ( !_bodyEnterCalled ) {
        onBodyEnter();
    }
    if ( _pseudoElementAfterChildIndex >= 0 ) {
        if ( _pseudoElementAfterChildIndex != _element->getChildCount()-1 ) {
            // Not the last child: move it there
            // (moveItemsTo() works just fine when the source node is also the
            // target node: remove it, and re-add it, so, adding it at the end)
            _element->moveItemsTo( _element, _pseudoElementAfterChildIndex, _pseudoElementAfterChildIndex);
        }
        // Now that all the real children of this node have had their
        // style set, we can init the style of the "After" pseudo
        // element, and its rend method as it has no children.
        ldomNode * child = _element->getChildNode(_element->getChildCount()-1);
        child->initNodeStyle();
        child->initNodeRendMethod();
    }
//    if ( _element->getStyle().isNull() ) {
//        lString16 path;
//        ldomNode * p = _element->getParentNode();
//        while (p) {
//            path = p->getNodeName() + L"/" + path;
//            p = p->getParentNode();
//        }
//        //CRLog::error("style not initialized for element 0x%04x %s path %s", _element->getDataIndex(), LCSTR(_element->getNodeName()), LCSTR(path));
//        crFatalError();
//    }
    _element->initNodeRendMethod();

    if ( _stylesheetIsSet )
        _document->getStyleSheet()->pop();
#endif
}

void ldomElementWriter::onText( const lChar16 * text, int len, lUInt32 )
{
    //logfile << "{t";
    {
        // normal mode: store text copy
        // add text node, if not first empty space string of block node
        if ( !_isBlock || _element->getChildCount()!=0 || !IsEmptySpace( text, len ) || (_flags&TXTFLG_PRE) ) {
            lString8 s8 = UnicodeToUtf8(text, len);
            _element->insertChildText(s8);
        } else {
            //CRLog::trace("ldomElementWriter::onText: Ignoring first empty space of block item");
        }
    }
    //logfile << "}";
}


//#define DISABLE_STYLESHEET_REL
#if BUILD_LITE!=1
/// if stylesheet file name is set, and file is found, set stylesheet to its value
bool ldomNode::applyNodeStylesheet()
{
#ifndef DISABLE_STYLESHEET_REL
    CRLog::trace("ldomNode::applyNodeStylesheet()");
    if ( !getDocument()->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) ) // internal styles are disabled
        return false;

    if ( getNodeId() != el_DocFragment && getNodeId() != el_body )
        return false;
    if ( getNodeId() == el_DocFragment && getDocument()->getContainer().isNull() )
        return false;

    // Here, we apply internal stylesheets that have been saved as attribute or
    // child element by the HTML parser for EPUB or plain HTML documents.

    // For epub documents, for each included .html in the epub, the first css
    // file link may have been put as the value of an added attribute to
    // the <DocFragment> element:
    //     <DocFragment StyleSheet="path to css file">
    //
    // For epub and html documents, the content of one or more <head><style>
    // elements, as well as all (only the 2nd++ for epub) linked css files,
    // with @import url(), have been put into an added child element:
    //     <DocFragment><stylesheet>css content</stylesheet><body>...</body></DocFragment>
    //     <body><stylesheet>css content</stylesheet>...</body>

    bool stylesheetChanged = false;

    if ( getNodeId() == el_DocFragment && hasAttribute(attr_StyleSheet) ) {
        getDocument()->_stylesheet.push();
        stylesheetChanged = getDocument()->parseStyleSheet(getAttributeValue(attr_StyleSheet));
        if ( !stylesheetChanged )
            getDocument()->_stylesheet.pop();
    }
    if ( getChildCount() > 0 ) {
        ldomNode *styleNode = getChildNode(0);

        if ( styleNode && styleNode->getNodeId()==el_stylesheet ) {
            if ( false == stylesheetChanged) {
                getDocument()->_stylesheet.push();
            }
            if ( getDocument()->parseStyleSheet(styleNode->getAttributeValue(attr_href),
                                                styleNode->getText()) ) {
                stylesheetChanged = true;
            } else if (false == stylesheetChanged) {
                getDocument()->_stylesheet.pop();
            }
        }
    }
    return stylesheetChanged;
#endif
    return false;
}
#endif

void ldomElementWriter::addAttribute( lUInt16 nsid, lUInt16 id, const wchar_t * value )
{
    getElement()->setAttributeValue(nsid, id, value);
#if BUILD_LITE!=1
    /* This is now done by ldomDocumentFragmentWriter::OnTagOpen() directly,
     * as we need to do it too for <DocFragment><stylesheet> tag, and not
     * only for <DocFragment StyleSheet="path_to_css_1st_file"> attribute.
    if ( id==attr_StyleSheet ) {
        _stylesheetIsSet = _element->applyNodeStylesheet();
    }
    */
#endif
}

ldomElementWriter * ldomDocumentWriter::pop( ldomElementWriter * obj, lUInt16 id )
{
    //logfile << "{p";
    ldomElementWriter * tmp = obj;
    for ( ; tmp; tmp = tmp->_parent )
    {
        //logfile << "-";
        if (tmp->getElement()->getNodeId() == id)
            break;
    }
    //logfile << "1";
    if (!tmp)
    {
        //logfile << "-err}";
        return obj; // error!!!
    }
    ldomElementWriter * tmp2 = NULL;
    //logfile << "2";
    for ( tmp = obj; tmp; tmp = tmp2 )
    {
        //logfile << "-";
        tmp2 = tmp->_parent;
        bool stop = (tmp->getElement()->getNodeId() == id);
        ElementCloseHandler( tmp->getElement() );
        delete tmp;
        if ( stop )
            return tmp2;
    }
    /*
    logfile << "3 * ";
    logfile << (int)tmp << " - " << (int)tmp2 << " | cnt=";
    logfile << (int)tmp->getElement()->childCount << " - "
            << (int)tmp2->getElement()->childCount;
    */
    //logfile << "}";
    return tmp2;
}

ldomElementWriter::~ldomElementWriter()
{
    //CRLog::trace("~ldomElementWriter for element 0x%04x %s", _element->getDataIndex(), LCSTR(_element->getNodeName()));
    //getElement()->persist();
    onBodyExit();
}




/////////////////////////////////////////////////////////////////
/// ldomDocumentWriter
// Used to parse expected XHTML (possibly made by crengine or helpers) for
// formats: FB2, RTF, WORD, plain text, PDB(txt)
// Also used for EPUB to build a single document, but driven by ldomDocumentFragmentWriter
// for each individual HTML files in the EPUB.
// For all these document formats, it is fed by HTMLParser that does
// convert to lowercase the tag names and attributes.
// ldomDocumentWriter does not do any auto-close of unbalanced tags and
// expect a fully correct and balanced XHTML.

// overrides
void ldomDocumentWriter::OnStart(LVFileFormatParser * parser)
{
    //logfile << "ldomDocumentWriter::OnStart()\n";
    // add document root node
    //CRLog::trace("ldomDocumentWriter::OnStart()");
    if ( !_headerOnly )
        _stopTagId = 0xFFFE;
    else {
        _stopTagId = _document->getElementNameIndex(L"description");
        //CRLog::trace( "ldomDocumentWriter() : header only, tag id=%d", _stopTagId );
    }
    LVXMLParserCallback::OnStart( parser );
    _currNode = new ldomElementWriter(_document, 0, 0, NULL);
}

void ldomDocumentWriter::OnStop()
{
    //logfile << "ldomDocumentWriter::OnStop()\n";
    while (_currNode)
        _currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
}

/// called after > of opening tag (when entering tag body)
// Note to avoid confusion: all tags HAVE a body (their content), so this
// is called on all tags.
// But in this, we do some specifics for tags that ARE a <BODY> tag.
void ldomDocumentWriter::OnTagBody()
{
    // Specific if we meet the <BODY> tag and we have styles to apply and
    // store in the DOM
    // (This can't happen with EPUBs: the ldomDocumentFragmentWriter that
    // drives this ldomDocumentWriter has parsed the HEAD STYLEs and LINKs
    // of each individual HTML file, and we see from them only their BODY:
    // _headStyleText and _stylesheetLinks are then empty. Styles for EPUB
    // are handled in :OnTagOpen() when being a DocFragment and meeting
    // the BODY.)
    if ( _currNode && _currNode->getElement() && _currNode->getElement()->isNodeName("body") &&
            ( !_headStyleText.empty() || _stylesheetLinks.length() > 0 ) ) {
        // If we're BODY, and we have meet styles in the previous HEAD
        // (links to css files or <STYLE> content), we need to save them
        // in an added <body><stylesheet> element so they are in the DOM
        // and saved in the cache, and found again when loading from cache
        // and applied again when a re-rendering is needed.

        // Make out an aggregated single stylesheet text.
        // @import's need to be first in the final stylesheet text
        lString16 imports;
        for (int i = 0; i < _stylesheetLinks.length(); i++) {
            lString16 import("@import url(\"");
            import << _stylesheetLinks.at(i);
            import << "\");\n";
            imports << import;
        }
        lString16 styleText = imports + _headStyleText.c_str();
        _stylesheetLinks.clear();
        _headStyleText.clear();

        // It's only at this point that we push() the previous stylesheet state
        // and apply the combined style text we made to the document:
        if ( _document->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) ) {
            _document->getStyleSheet()->push();
            _popStyleOnFinish = true; // superclass ~ldomDocumentWriter() will do the ->pop()
            _document->parseStyleSheet(lString16(), styleText);
            // printf("applied: %s\n", LCSTR(styleText));
            // apply any FB2 stylesheet too, so it's removed too when pop()
            _document->applyDocumentStyleSheet();
        }
        // We needed to add that /\ to the _document->_stylesheet before this
        // onBodyEnter \/, for any body {} css declaration to be available
        // as this onBodyEnter will apply the current _stylesheet to this BODY node.
        _currNode->onBodyEnter();
        _flags = _currNode->getFlags(); // _flags may have been updated (if white-space: pre)
        // And only after this we can add the <stylesheet> as a first child
        // element of this BODY node. It will not be displayed thanks to fb2def.h:
        //   XS_TAG1D( stylesheet, true, css_d_none, css_ws_normal )
        OnTagOpen(L"", L"stylesheet");
        OnTagBody();
        OnText(styleText.c_str(), styleText.length(), 0);
        OnTagClose(L"", L"stylesheet");
        CRLog::trace("added BODY>stylesheet child element with HEAD>STYLE&LINKS content");
    }
    else if ( _currNode ) { // for all other tags (including BODY when no style)
        _currNode->onBodyEnter();
        _flags = _currNode->getFlags(); // _flags may have been updated (if white-space: pre)
    }
}

ldomNode * ldomDocumentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    //logfile << "ldomDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
    //CRLog::trace("OnTagOpen(%s)", UnicodeToUtf8(lString16(tagname)).c_str());
    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;

    // Set a flag for OnText to accumulate the content of any <HEAD><STYLE>
    if ( tagname[0] == 's' && !lStr_cmp(tagname, "style") && _currNode && _currNode->getElement()->isNodeName("head") ) {
        _inHeadStyle = true;
    }

    // For EPUB, when ldomDocumentWriter is driven by ldomDocumentFragmentWriter:
    // if we see a BODY coming and we are a DocFragment, its time to apply the
    // styles set to the DocFragment before switching to BODY (so the styles can
    // be applied to BODY)
    if (id == el_body && _currNode && _currNode->_element->getNodeId() == el_DocFragment) {
        _currNode->_stylesheetIsSet = _currNode->getElement()->applyNodeStylesheet();
        // _stylesheetIsSet will be used to pop() the stylesheet when
        // leaving/destroying this DocFragment ldomElementWriter
    }

    //if ( id==_stopTagId ) {
        //CRLog::trace("stop tag found, stopping...");
    //    _parser->Stop();
    //}
    _currNode = new ldomElementWriter( _document, nsid, id, _currNode );
    _flags = _currNode->getFlags();
    //logfile << " !o!\n";
    //return _currNode->getElement();
    return _currNode->getElement();
}

ldomDocumentWriter::~ldomDocumentWriter()
{
    while (_currNode)
        _currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
#if BUILD_LITE!=1
    if ( _document->isDefStyleSet() ) {
        if ( _popStyleOnFinish )
            // pop any added styles to the original stylesheet so we get
            // the original one back and avoid a stylesheet hash mismatch
            _document->getStyleSheet()->pop();
        // Not sure why we would do that at end of parsing, but ok: it's
        // not recursive, so not expensive:
        _document->getRootNode()->initNodeStyle();
        _document->getRootNode()->initNodeFont();
        //if ( !_document->validateDocument() )
        //    CRLog::error("*** document style validation failed!!!");
        _document->updateRenderContext();
        _document->dumpStatistics();
        if ( _document->_nodeStylesInvalidIfLoading ) {
            // Some pseudoclass like :last-child has been met which has set this flag
            printf("CRE: document loaded, but styles re-init needed (cause: peculiar CSS pseudoclasses met)\n");
            _document->forceReinitStyles();
        }
    }

#endif
}

void ldomDocumentWriter::OnTagClose( const lChar16 *, const lChar16 * tagname )
{
    //logfile << "ldomDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
    if (!_currNode)
    {
        _errFlag = true;
        //logfile << " !c-err!\n";
        return;
    }

    // Parse <link rel="stylesheet">, put the css file link in _stylesheetLinks.
    // They will be added to <body><stylesheet> when we meet <BODY>
    // (duplicated in ldomDocumentWriterFilter::OnTagClose)
    if (tagname[0] == 'l' && _currNode && !lStr_cmp(tagname, "link")) {
        // link node
        if ( _currNode && _currNode->getElement() && _currNode->getElement()->isNodeName("link") &&
             _currNode->getElement()->getParentNode() && _currNode->getElement()->getParentNode()->isNodeName("head") &&
             lString16(_currNode->getElement()->getAttributeValue("rel")).lowercase() == L"stylesheet" &&
             lString16(_currNode->getElement()->getAttributeValue("type")).lowercase() == L"text/css" ) {
            lString16 href = _currNode->getElement()->getAttributeValue("href");
            lString16 stylesheetFile = LVCombinePaths( _document->getCodeBase(), href );
            CRLog::debug("Internal stylesheet file: %s", LCSTR(stylesheetFile));
            // We no more apply it immediately: it will be when <BODY> is met
            // _document->setDocStylesheetFileName(stylesheetFile);
            // _document->applyDocumentStyleSheet();
            _stylesheetLinks.add(stylesheetFile);
        }
    }

    /* This is now dealt with in :OnTagBody(), just before creating this <stylesheet> tag
    bool isStyleSheetTag = !lStr_cmp(tagname, "stylesheet");
    if ( isStyleSheetTag ) {
        ldomNode *parentNode = _currNode->getElement()->getParentNode();
        if (parentNode && parentNode->isNodeName("DocFragment")) {
            _document->parseStyleSheet(_currNode->getElement()->getAttributeValue(attr_href),
                                       _currNode->getElement()->getText());
            isStyleSheetTag = false;
        }
    }
    */

    lUInt16 id = _document->getElementNameIndex(tagname);
    //lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    _errFlag |= (id != _currNode->getElement()->getNodeId());
    _currNode = pop( _currNode, id );

    if ( _currNode )
        _flags = _currNode->getFlags();

    if ( id==_stopTagId ) {
        //CRLog::trace("stop tag found, stopping...");
        _parser->Stop();
    }

    /* This is now dealt with in :OnTagBody(), just before creating this <stylesheet> tag
    if ( isStyleSheetTag ) {
        //CRLog::trace("</stylesheet> found");
#if BUILD_LITE!=1
        if ( !_popStyleOnFinish ) {
            //CRLog::trace("saving current stylesheet before applying of document stylesheet");
            _document->getStyleSheet()->push();
            _popStyleOnFinish = true;
            _document->applyDocumentStyleSheet();
        }
#endif
    }
    */

    //logfile << " !c!\n";
}

void ldomDocumentWriter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    //logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
    lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex( nsname ) : 0;
    lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex( attrname ) : 0;
    _currNode->addAttribute( attr_ns, attr_id, attrvalue );

    //logfile << " !a!\n";
}

void ldomDocumentWriter::OnText( const lChar16 * text, int len, lUInt32 flags )
{
    //logfile << "ldomDocumentWriter::OnText() fpos=" << fpos;

    // Accumulate <HEAD><STYLE> content
    if (_inHeadStyle) {
        _headStyleText << lString16(text, len);
        _inHeadStyle = false;
        return;
    }

    if (_currNode)
    {
        if ( (_flags & XML_FLAG_NO_SPACE_TEXT)
             && IsEmptySpace(text, len)  && !(flags & TXTFLG_PRE))
             return;
        if (_currNode->_allowText)
            _currNode->onText( text, len, flags );
    }
    //logfile << " !t!\n";
}

void ldomDocumentWriter::OnEncoding( const lChar16 *, const lChar16 *)
{
}

ldomDocumentWriter::ldomDocumentWriter(ldomDocument * document, bool headerOnly)
    : _document(document), _currNode(NULL), _errFlag(false), _headerOnly(headerOnly), _popStyleOnFinish(false), _flags(0), _inHeadStyle(false)
{
    _headStyleText.clear();
    _stylesheetLinks.clear();
    _stopTagId = 0xFFFE;
    IS_FIRST_BODY = true;

#if BUILD_LITE!=1
    if ( _document->isDefStyleSet() ) {
        _document->getRootNode()->initNodeStyle();
        _document->getRootNode()->setRendMethod(erm_block);
    }
#endif

    //CRLog::trace("ldomDocumentWriter() headerOnly=%s", _headerOnly?"true":"false");
}








bool FindNextNode( ldomNode * & node, ldomNode * root )
{
    if ( node->getChildCount()>0 ) {
        // first child
        node = node->getChildNode(0);
        return true;
    }
    if (node->isRoot() || node == root )
        return false; // root node reached
    int index = node->getNodeIndex();
    ldomNode * parent = node->getParentNode();
    while (parent)
    {
        if ( index < (int)parent->getChildCount()-1 ) {
            // next sibling
            node = parent->getChildNode( index + 1 );
            return true;
        }
        if (parent->isRoot() || parent == root )
            return false; // root node reached
        // up one level
        index = parent->getNodeIndex();
        parent = parent->getParentNode();
    }
    //if ( node->getNodeType() == LXML_TEXT_NODE )
    return false;
}

// base64 decode table
static const signed char base64_decode_table[] = {
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, //0..15
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, //16..31   10
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63, //32..47   20
   52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1, //48..63   30
   -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, //64..79   40
   15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1, //80..95   50
   -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, //INDEX2..111  60
   41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1  //112..127 70
};

#define BASE64_BUF_SIZE 128
class LVBase64NodeStream : public LVNamedStream
{
private:
    ldomNode *  m_elem;
    ldomNode *  m_curr_node;
    lString16   m_curr_text;
    int         m_text_pos;
    lvsize_t    m_size;
    lvpos_t     m_pos;

    int         m_iteration;
    lUInt32     m_value;

    lUInt8      m_bytes[BASE64_BUF_SIZE];
    int         m_bytes_count;
    int         m_bytes_pos;

    int readNextBytes()
    {
        int bytesRead = 0;
        bool flgEof = false;
        while ( bytesRead == 0 && !flgEof )
        {
            while ( m_text_pos >= (int)m_curr_text.length() )
            {
                if ( !findNextTextNode() )
                    return bytesRead;
            }
            int len = m_curr_text.length();
            const lChar16 * txt = m_curr_text.c_str();
            for ( ; m_text_pos<len && m_bytes_count < BASE64_BUF_SIZE - 3; m_text_pos++ )
            {
                lChar16 ch = txt[ m_text_pos ];
                if ( ch < 128 )
                {
                    if ( ch == '=' )
                    {
                        // end of stream
                        if ( m_iteration == 2 )
                        {
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value>>4) & 0xFF);
                            bytesRead++;
                        }
                        else if ( m_iteration == 3 )
                        {
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value>>10) & 0xFF);
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value>>2) & 0xFF);
                            bytesRead += 2;
                        }
                        // stop!!!
                        //m_text_pos--;
                        m_iteration = 0;
                        flgEof = true;
                        break;
                    }
                    else
                    {
                        int k = base64_decode_table[ch];
                        if ( !(k & 0x80) ) {
                            // next base-64 digit
                            m_value = (m_value << 6) | (k);
                            m_iteration++;
                            if (m_iteration==4)
                            {
                                //
                                m_bytes[m_bytes_count++] = (lUInt8)((m_value>>16) & 0xFF);
                                m_bytes[m_bytes_count++] = (lUInt8)((m_value>>8) & 0xFF);
                                m_bytes[m_bytes_count++] = (lUInt8)((m_value>>0) & 0xFF);
                                m_iteration = 0;
                                m_value = 0;
                                bytesRead+=3;
                            }
                        } else {
                            //m_text_pos++;
                        }
                    }
                }
            }
        }
        return bytesRead;
    }

    bool findNextTextNode()
    {
        while ( FindNextNode( m_curr_node, m_elem ) ) {
            if ( m_curr_node->isText() ) {
                m_curr_text = m_curr_node->getText();
                m_text_pos = 0;
                return true;
            }
        }
        return false;
    }

    int bytesAvailable() { return m_bytes_count - m_bytes_pos; }

    bool rewind()
    {
        m_curr_node = m_elem;
        m_pos = 0;
        m_bytes_count = 0;
        m_bytes_pos = 0;
        m_iteration = 0;
        m_value = 0;
        return findNextTextNode();
    }

    bool skip( lvsize_t count )
    {
        while ( count )
        {
            if ( m_bytes_pos >= m_bytes_count )
            {
                m_bytes_pos = 0;
                m_bytes_count = 0;
                int bytesRead = readNextBytes();
                if ( bytesRead == 0 )
                    return false;
            }
            int diff = (int) (m_bytes_count - m_bytes_pos);
            if (diff > (int)count)
                diff = (int)count;
            m_pos += diff;
            count -= diff;
        }
        return true;
    }

public:
    virtual ~LVBase64NodeStream() { }
    LVBase64NodeStream( ldomNode * element )
        : m_elem(element), m_curr_node(element), m_size(0), m_pos(0)
    {
        // calculate size
        rewind();
        m_size = bytesAvailable();
        for (;;) {
            int bytesRead = readNextBytes();
            if ( !bytesRead )
                break;
            m_bytes_count = 0;
            m_bytes_pos = 0;
            m_size += bytesRead;
        }
        // rewind
        rewind();
    }
    virtual bool Eof()
    {
        return m_pos >= m_size;
    }
    virtual lvsize_t  GetSize()
    {
        return m_size;
    }

    virtual lvpos_t GetPos()
    {
        return m_pos;
    }

    virtual lverror_t GetPos( lvpos_t * pos )
    {
        if (pos)
            *pos = m_pos;
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
            npos = m_size + offset;
            break;
        }
        if (npos > m_size)
            return LVERR_FAIL;
        if ( npos != currpos )
        {
            if (npos < currpos)
            {
                if ( !rewind() || !skip(npos) )
                    return LVERR_FAIL;
            }
            else
            {
                skip( npos - currpos );
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
    virtual lverror_t Read(void* buf, lvsize_t size, lvsize_t* pBytesRead)
    {
        lvsize_t bytesRead = 0;
        //fprintf( stderr, "Read()\n" );

        lUInt8 * out = (lUInt8 *)buf;

        while (size>0)
        {
            int sz = bytesAvailable();
            if (!sz) {
                m_bytes_pos = m_bytes_count = 0;
                sz = readNextBytes();
                if (!sz) {
                    if ( !bytesRead || m_pos!=m_size) //
                        return LVERR_FAIL;
                    break;
                }
            }
            if (sz>(int)size)
                sz = (int)size;
            for (int i=0; i<sz; i++)
                *out++ = m_bytes[m_bytes_pos++];
            size -= sz;
            bytesRead += sz;
            m_pos += sz;
        }

        if (pBytesRead)
            *pBytesRead = bytesRead;
        //fprintf( stderr, "    %d bytes read...\n", (int)bytesRead );
        return LVERR_OK;
    }
    virtual lverror_t SetSize(lvsize_t)
    {
        return LVERR_NOTIMPL;
    }
};

img_scaling_option_t::img_scaling_option_t()
{
    mode = (MAX_IMAGE_SCALE_MUL>1) ? (ARBITRARY_IMAGE_SCALE_ENABLED==1 ? IMG_FREE_SCALING : IMG_INTEGER_SCALING) : IMG_NO_SCALE;
    max_scale = (MAX_IMAGE_SCALE_MUL>1) ? MAX_IMAGE_SCALE_MUL : 1;
}

img_scaling_options_t::img_scaling_options_t()
{
    img_scaling_option_t option;
    zoom_in_inline = option;
    zoom_in_block = option;
    zoom_out_inline = option;
    zoom_out_block = option;
}

#define FONT_SIZE_BIG 32
#define FONT_SIZE_VERY_BIG 50
static bool updateScalingOption( img_scaling_option_t & v, CRPropRef props, int fontSize, bool zoomin, bool isInline )
{
    lString8 propName("crengine.image.scaling.");
    propName << (zoomin ? "zoomin." : "zoomout.");
    propName << (isInline ? "inline." : "block.");
    lString8 propNameMode = propName + "mode";
    lString8 propNameScale = propName + "scale";
    img_scaling_option_t def;
    int currMode = props->getIntDef(propNameMode.c_str(), (int)def.mode);
    int currScale = props->getIntDef(propNameScale.c_str(), (int)def.max_scale);
    if ( currScale==0 ) {
        if ( fontSize>=FONT_SIZE_VERY_BIG )
            currScale = 3;
        else if ( fontSize>=FONT_SIZE_BIG )
            currScale = 2;
        else
            currScale = 1;
    }
    if ( currScale==1 )
        currMode = 0;
    bool updated = false;
    if ( v.max_scale!=currScale ) {
        updated = true;
        v.max_scale = currScale;
    }
    if ( v.mode!=(img_scaling_mode_t)currMode ) {
        updated = true;
        v.mode = (img_scaling_mode_t)currMode;
    }
    props->setIntDef(propNameMode.c_str(), currMode);
    props->setIntDef(propNameScale.c_str(), currScale);
    return updated;
}

/// returns true if any changes occured
bool img_scaling_options_t::update( CRPropRef props, int fontSize )
{
    bool updated = false;
    updated = updateScalingOption( zoom_in_inline, props, fontSize, true, true ) || updated;
    updated = updateScalingOption( zoom_in_block, props, fontSize, true, false ) || updated;
    updated = updateScalingOption( zoom_out_inline, props, fontSize, false, true ) || updated;
    updated = updateScalingOption( zoom_out_block, props, fontSize, false, false ) || updated;
    return updated;
}

xpath_step_t ParseXPathStep( const lChar16 * &path, lString16 & name, int & index )
{
    int pos = 0;
    const lChar16 * s = path;
    //int len = path.GetLength();
    name.clear();
    index = -1;
    int flgPrefix = 0;
    if (s && s[pos]) {
        lChar16 ch = s[pos];
        // prefix: none, '/' or '.'
        if (ch=='/') {
            flgPrefix = 1;
            ch = s[++pos];
        } else if (ch=='.') {
            flgPrefix = 2;
            ch = s[++pos];
        }
        int nstart = pos;
        if (ch>='0' && ch<='9') {
            // node or point index
            pos++;
            while (s[pos]>='0' && s[pos]<='9')
                pos++;
            if (s[pos] && s[pos]!='/' && s[pos]!='.')
                return xpath_step_error;
            lString16 sindex( path+nstart, pos-nstart );
            index = sindex.atoi();
            if (index<((flgPrefix==2)?0:1))
                return xpath_step_error;
            path += pos;
            return (flgPrefix==2) ? xpath_step_point : xpath_step_nodeindex;
        }
        while (s[pos] && s[pos]!='[' && s[pos]!='/' && s[pos]!='.')
            pos++;
        if (pos==nstart)
            return xpath_step_error;
        name = lString16( path+ nstart, pos-nstart );
        if (s[pos]=='[') {
            // index
            pos++;
            int istart = pos;
            while (s[pos] && s[pos]!=']' && s[pos]!='/' && s[pos]!='.')
                pos++;
            if (!s[pos] || pos==istart)
                return xpath_step_error;

            lString16 sindex( path+istart, pos-istart );
            index = sindex.atoi();
            pos++;
        }
        if (!s[pos] || s[pos]=='/' || s[pos]=='.') {
            path += pos;
            return (name == "text()") ? xpath_step_text : xpath_step_element; // OK!
        }
        return xpath_step_error; // error
    }
    return xpath_step_error;
}


/// get pointer for relative path
ldomXPointer ldomXPointer::relative( lString16 relativePath )
{
    return getDocument()->createXPointer( getNode(), relativePath );
}
/// create xpointer from pointer string
ldomXPointer ldomDocument::createXPointer( const lString16 & xPointerStr )
{
    if ( xPointerStr[0]=='#' ) {
        lString16 id = xPointerStr.substr(1);
        lUInt32 idid = getAttrValueIndex(id.c_str());
        lInt32 nodeIndex;
        if ( _idNodeMap.get(idid, nodeIndex) ) {
            ldomNode * node = getTinyNode(nodeIndex);
            if ( node && node->isElement() ) {
                return ldomXPointer(node, -1);
            }
        }
        return ldomXPointer();
    }
    return createXPointer( getRootNode(), xPointerStr );
}

#if BUILD_LITE!=1

/// return parent final node, if found
ldomNode * ldomXPointer::getFinalNode() const
{
    ldomNode * node = getNode();
    for (;;) {
        if ( !node )
            return NULL;
        if ( node->getRendMethod()==erm_final || node->getRendMethod()==erm_list_item || node->getRendMethod() == erm_table_caption )
            return node;
        node = node->getParentNode();
    }
}

/// return true is this node is a final node
bool ldomXPointer::isFinalNode() const
{
    ldomNode * node = getNode();
    if ( !node )
        return false;
    if ( node->getRendMethod()==erm_final || node->getRendMethod()==erm_list_item || node->getRendMethod() == erm_table_caption )
        return true;
    return false;
}

/// create xpointer from doc point
ldomXPointer ldomDocument::createXPointer( lvPoint pt, int direction, bool strictBounds, ldomNode * fromNode )
{
    //
    lvPoint orig_pt = lvPoint(pt);
    ldomXPointer ptr;
    if ( !getRootNode() )
        return ptr;
    ldomNode * startNode;
    if ( fromNode ) {
        // Start looking from the fromNode provided - only used when we are
        // looking inside a floatBox or an inlineBox below and we have this
        // recursive call to createXPointer().
        // Even with a provided fromNode, pt must be provided in full absolute
        // coordinates. But we need to give to startNode->elementFromPoint()
        // a pt with coordinates relative to fromNode.
        // And because elementFromPoint() uses the fmt x/y offsets of the
        // start node (relative to the containing final block), we would
        // need to have pt relative to that containing final block - and so,
        // we'd need to lookup the final node from here (or have it provided
        // as an additional parameter if it's known by caller).
        // But because we're called only for floatBox and inlineBox, which
        // have only a single child, we can use the trick of calling
        // ->elementFromPoint() on that first child, while still getting
        // pt relative to fromNode itself:
        startNode = fromNode->getChildNode(0);
        lvRect rc;
        fromNode->getAbsRect( rc, true );
        pt.x -= rc.left;
        pt.y -= rc.top;
    }
    else {
        startNode = getRootNode();
    }
    ldomNode * finalNode = startNode->elementFromPoint( pt, direction );
    if ( fromNode )
        pt = orig_pt; // restore orig pt
    if ( !finalNode ) {
        // printf("no finalNode found from %s\n", UnicodeToLocal(ldomXPointer(fromNode, 0).toString()).c_str());
        // No node found, return start or end of document if pt overflows it, otherwise NULL
        if ( pt.y >= getFullHeight()) {
            ldomNode * node = getRootNode()->getLastTextChild();
            return ldomXPointer(node,node ? node->getText().length() : 0);
        } else if ( pt.y <= 0 ) {
            ldomNode * node = getRootNode()->getFirstTextChild();
            return ldomXPointer(node, 0);
        }
        CRLog::trace("not final node");
        return ptr;
    }
    // printf("finalNode %s\n", UnicodeToLocal(ldomXPointer(finalNode, 0).toString()).c_str());

    lvdom_element_render_method rm = finalNode->getRendMethod();
    if ( rm != erm_final && rm != erm_list_item && rm != erm_table_caption ) {
        // Not final, return XPointer to first or last child
        lvRect rc;
        finalNode->getAbsRect( rc );
        if ( pt.y < (rc.bottom + rc.top) / 2 )
            return ldomXPointer( finalNode, 0 );
        else
            return ldomXPointer( finalNode, finalNode->getChildCount() );
    }

    // Final node found
    // Adjust pt in coordinates of the FormattedText
    RenderRectAccessor fmt( finalNode );
    lvRect rc;
    // When in enhanced rendering mode, we can get the FormattedText coordinates
    // and its width (inner_width) directly
    finalNode->getAbsRect( rc, true ); // inner=true
    pt.x -= rc.left;
    pt.y -= rc.top;
    int inner_width;
    if ( RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET) ) {
        inner_width = fmt.getInnerWidth();
    }
    else {
        // In legacy mode, we just got the erm_final coordinates, and we must
        // compute and remove left/top border and padding (using rc.width() as
        // the base for % is wrong here, and so is rc.height() for padding top)
        int em = finalNode->getFont()->getSize();
        int padding_left = measureBorder(finalNode,3)+lengthToPx(finalNode->getStyle()->padding[0],rc.width(),em);
        int padding_right = measureBorder(finalNode,1)+lengthToPx(finalNode->getStyle()->padding[1],rc.width(),em);
        int padding_top = measureBorder(finalNode,0)+lengthToPx(finalNode->getStyle()->padding[2],rc.height(),em);
        pt.x -= padding_left;
        pt.y -= padding_top;
        // As well as the inner width
        inner_width  = fmt.getWidth() - padding_left - padding_right;
    }

    // Get the formatted text, so we can look for 'pt' line by line, word by word,
    // (and embedded float by embedded float if there are some).
    LFormattedTextRef txtform;
    {
        // This will possibly return it from CVRendBlockCache
        finalNode->renderFinalBlock( txtform, &fmt, inner_width );
    }

    // First, look if pt happens to be in some float
    // (this may not work with floats with negative margins)
    int fcount = txtform->GetFloatCount();
    for (int f=0; f<fcount; f++) {
        const embedded_float_t * flt = txtform->GetFloatInfo(f);
        // Ignore fake floats (no srctext) made from outer floats footprint
        if ( flt->srctext == NULL )
            continue;
        if (pt.x >= flt->x && pt.x < flt->x + flt->width && pt.y >= flt->y && pt.y < flt->y + flt->height ) {
            // pt is inside this float.
            ldomNode * node = (ldomNode *) flt->srctext->object; // floatBox node
            ldomXPointer inside_ptr = createXPointer( orig_pt, direction, strictBounds, node );
            if ( !inside_ptr.isNull() ) {
                return inside_ptr;
            }
            // Otherwise, return xpointer to the floatNode itself
            return ldomXPointer(node, 0);
            // (Or should we let just go on looking only at the text in the original final node?)
        }
        // If no containing float, go on looking at the text of the original final node
    }

    // Look at words in the rendered final node (whether it's the original
    // main final node, or the one found in a float)
    int lcount = txtform->GetLineCount();
    for ( int l = 0; l<lcount; l++ ) {
        const formatted_line_t * frmline = txtform->GetLineInfo(l);
        if ( pt.y >= (int)(frmline->y + frmline->height) && l<lcount-1 )
            continue;
        // CRLog::debug("  point (%d, %d) line found [%d]: (%d..%d)",
        //      pt.x, pt.y, l, frmline->y, frmline->y+frmline->height);
        bool line_is_bidi = frmline->flags & LTEXT_LINE_IS_BIDI;
        int wc = (int)frmline->word_count;

        if ( direction >= PT_DIR_SCAN_FORWARD_LOGICAL_FIRST || direction <= PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST ) {
            // Only used by LVDocView::getBookmark(), LVDocView::getPageDocumentRange()
            // and ldomDocument::findText(), to not miss any content or text from
            // the page.
            // The SCAN_ part has been done done: a line has been found, and we want
            // to find node/chars from it in the logical (HTML) order, and not in the
            // visual order (that PT_DIR_SCAN_FORWARD/PT_DIR_SCAN_BACKWARD do), which
            // might not be the same in bidi lines:
            bool find_first = direction == PT_DIR_SCAN_FORWARD_LOGICAL_FIRST ||
                              direction == PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST;
                         // so, false when PT_DIR_SCAN_FORWARD_LOGICAL_LAST
                         //             or PT_DIR_SCAN_BACKWARD_LOGICAL_LAST

            const formatted_word_t * word = NULL;
            for ( int w=0; w<wc; w++ ) {
                const formatted_word_t * tmpword = &frmline->words[w];
                const src_text_fragment_t * src = txtform->GetSrcInfo(tmpword->src_text_index);
                ldomNode * node = (ldomNode *)src->object;
                if ( !node ) // ignore crengine added text (spacing, list item bullets...)
                    continue;
                if ( !line_is_bidi ) {
                    word = tmpword;
                    if ( find_first )
                        break; // found logical first real word
                    // otherwise, go to the end, word will be logical last real word
                }
                else {
                    if (!word) { // first word seen: first candidate
                        word = tmpword;
                    }
                    else { // compare current word to the current candidate
                        if ( find_first && tmpword->src_text_index < word->src_text_index ) {
                            word = tmpword;
                        }
                        else if ( !find_first && tmpword->src_text_index > word->src_text_index ) {
                            word = tmpword;
                        }
                        else if (tmpword->src_text_index == word->src_text_index ) {
                            // (Same src_text_fragment_t, same src->t.offset, skip in when comparing)
                            if ( find_first && tmpword->t.start < word->t.start ) {
                                word = tmpword;
                            }
                            else if ( !find_first && tmpword->t.start > word->t.start ) {
                                word = tmpword;
                            }
                        }
                    }
                }
            }
            if ( !word ) // no word: no xpointer (should not happen?)
                return ptr;
            // Found right word/image
            const src_text_fragment_t * src = txtform->GetSrcInfo(word->src_text_index);
            ldomNode * node = (ldomNode *)src->object;
            if ( word->flags & LTEXT_WORD_IS_INLINE_BOX ) {
                // pt is inside this inline-block inlineBox node
                ldomXPointer inside_ptr = createXPointer( orig_pt, direction, strictBounds, node );
                if ( !inside_ptr.isNull() ) {
                    return inside_ptr;
                }
                // Otherwise, return xpointer to the inlineBox itself
                return ldomXPointer(node, 0);
            }
            if ( word->flags & LTEXT_WORD_IS_OBJECT ) {
                return ldomXPointer(node, 0);
            }
            // It is a word
            if ( find_first ) // return xpointer to logical start of word
                return ldomXPointer( node, src->t.offset + word->t.start );
            else // return xpointer to logical end of word
                return ldomXPointer( node, src->t.offset + word->t.start + word->t.len );
        }

        // Found line, searching for word (words are in visual order)
        int x = pt.x - frmline->x;
        // frmline->x is text indentation (+ possibly leading space if text
        // centered or right aligned)
        if (strictBounds) {
            if (x < 0 || x > frmline->width) { // pt is before or after formatted text: nothing there
                return ptr;
            }
        }

        for ( int w=0; w<wc; w++ ) {
            const formatted_word_t * word = &frmline->words[w];
            if ( ( !line_is_bidi && x < word->x + word->width ) ||
                 ( line_is_bidi && x >= word->x && x < word->x + word->width ) ||
                 ( w == wc-1 ) ) {
                const src_text_fragment_t * src = txtform->GetSrcInfo(word->src_text_index);
                // CRLog::debug(" word found [%d]: x=%d..%d, start=%d, len=%d  %08X",
                //      w, word->x, word->x + word->width, word->t.start, word->t.len, src->object);

                ldomNode * node = (ldomNode *)src->object;
                if ( !node ) // Ignore crengine added text (spacing, list item bullets...)
                    continue;

                if ( word->flags & LTEXT_WORD_IS_INLINE_BOX ) {
                    // pt is inside this inline-block inlineBox node
                    ldomXPointer inside_ptr = createXPointer( orig_pt, direction, strictBounds, node );
                    if ( !inside_ptr.isNull() ) {
                        return inside_ptr;
                    }
                    // Otherwise, return xpointer to the inlineBox itself
                    return ldomXPointer(node, 0);
                }
                if ( word->flags & LTEXT_WORD_IS_OBJECT ) {
                    // Object (image)
                    #if 1
                    // return image object itself
                    return ldomXPointer(node, 0);
                    #else
                    return ldomXPointer( node->getParentNode(),
                        node->getNodeIndex() + (( x < word->x + word->width/2 ) ? 0 : 1) );
                    #endif
                }

                // Found word, searching for letters
                LVFont * font = (LVFont *) src->t.font;
                lUInt16 width[512];
                lUInt8 flg[512];

                lString16 str = node->getText();
                // We need to transform the node text as it had been when
                // rendered (the transform may change chars widths) for the
                // XPointer offset to be correct
                switch ( node->getParentNode()->getStyle()->text_transform ) {
                    case css_tt_uppercase:
                        str.uppercase();
                        break;
                    case css_tt_lowercase:
                        str.lowercase();
                        break;
                    case css_tt_capitalize:
                        str.capitalize();
                        break;
                    case css_tt_full_width:
                        // str.fullWidthChars(); // disabled for now in lvrend.cpp
                        break;
                    default:
                        break;
                }

                lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                font->measureText( str.c_str()+word->t.start, word->t.len, width, flg, word->width+50, '?',
                            src->lang_cfg, src->letter_spacing + word->added_letter_spacing, false, hints);

                bool word_is_rtl = word->flags & LTEXT_WORD_DIRECTION_IS_RTL;
                if ( word_is_rtl ) {
                    for ( int i=word->t.len-1; i>=0; i-- ) {
                        int xx = ( i>0 ) ? (width[i-1] + width[i])/2 : width[i]/2;
                        xx = word->width - xx;
                        if ( x < word->x + xx ) {
                            return ldomXPointer( node, src->t.offset + word->t.start + i );
                        }
                    }
                    return ldomXPointer( node, src->t.offset + word->t.start );
                }
                else {
                    for ( int i=0; i<word->t.len; i++ ) {
                        int xx = ( i>0 ) ? (width[i-1] + width[i])/2 : width[i]/2;
                        if ( x < word->x + xx ) {
                            return ldomXPointer( node, src->t.offset + word->t.start + i );
                        }
                    }
                    return ldomXPointer( node, src->t.offset + word->t.start + word->t.len );
                }
            }
        }
    }
    return ptr;
}

/// returns coordinates of pointer inside formatted document
lvPoint ldomXPointer::toPoint(bool extended) const
{
    lvRect rc;
    if ( !getRect( rc, extended ) )
        return lvPoint(-1, -1);
    return rc.topLeft();
}

/// returns caret rectangle for pointer inside formatted document
// (with extended=true, consider paddings and borders)
// Note that extended / ldomXPointer::getRectEx() is only used (by cre.cpp)
// when dealing with hyphenated words, getting each char width, char by char.
// So we return the char width (and no more the word width) of the char
// pointed to by this XPointer (unlike ldomXRange::getRectEx() which deals
// with a range between 2 XPointers).
bool ldomXPointer::getRect(lvRect & rect, bool extended, bool adjusted) const
{
    //CRLog::trace("ldomXPointer::getRect()");
    if ( isNull() )
        return false;
    ldomNode * p = isElement() ? getNode() : getNode()->getParentNode();
    ldomNode * p0 = p;
    ldomNode * finalNode = NULL;
    if ( !p ) {
        //CRLog::trace("ldomXPointer::getRect() - p==NULL");
    }
    //printf("getRect( p=%08X type=%d )\n", (unsigned)p, (int)p->getNodeType() );
    else if ( !p->getDocument() ) {
        //CRLog::trace("ldomXPointer::getRect() - p->getDocument()==NULL");
    }
    ldomNode * mainNode = p->getDocument()->getRootNode();
    for ( ; p; p = p->getParentNode() ) {
        int rm = p->getRendMethod();
        if ( rm == erm_final || rm == erm_table_caption ) {
            // With floats, we may get multiple erm_final when walking up
            // to root node: keep the first one met (but go on up to the
            // root node in case we're in some upper erm_invisible).
            if (!finalNode)
                finalNode = p; // found final block
        }
        else if (rm == erm_list_item) {
            // This obsolete rendering method is considered just like erm_final
            // for many purposes, but can contain real erm_final nodes.
            // So, if we found an erm_final, and if we find an erm_list_item
            // when going up, we should use it (unlike in previous case).
            // (This is needed to correctly display highlights on books opened
            // with some older DOM_VERSION.)
            finalNode = p;
        }
        else if ( p->getRendMethod() == erm_invisible ) {
            return false; // invisible !!!
        }
        if ( p==mainNode )
            break;
    }

    if ( finalNode==NULL ) {
        lvRect rc;
        p0->getAbsRect( rc );
        CRLog::debug("node w/o final parent: %d..%d", rc.top, rc.bottom);
    }

    if ( finalNode!=NULL ) {
        lvRect rc;
        finalNode->getAbsRect( rc, extended ); // inner=true if extended=true
        if (rc.height() == 0 && rc.width() > 0) {
            rect = rc;
            rect.bottom++;
            return true;
        }
        RenderRectAccessor fmt( finalNode );
        //if ( !fmt )
        //    return false;

        // When in enhanced rendering mode, we can get the FormattedText coordinates
        // and its width (inner_width) directly
        int inner_width;
        if ( RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET) ) {
            inner_width = fmt.getInnerWidth();
            // if extended=true, we got directly the adjusted rc.top and rc.left
        }
        else {
            // In legacy mode, we just got the erm_final coordinates, and we must
            // compute and remove left/top border and padding (using rc.width() as
            // the base for % is wrong here)
            int em = finalNode->getFont()->getSize();
            int padding_left = measureBorder(finalNode,3) + lengthToPx(finalNode->getStyle()->padding[0], rc.width(), em);
            int padding_right = measureBorder(finalNode,1) + lengthToPx(finalNode->getStyle()->padding[1], rc.width(), em);
            inner_width  = fmt.getWidth() - padding_left - padding_right;
            if (extended) {
                int padding_top = measureBorder(finalNode,0) + lengthToPx(finalNode->getStyle()->padding[2], rc.width(), em);
                rc.top += padding_top;
                rc.left += padding_left;
                // rc.right += padding_left; // wrong, but not used
                // rc.bottom += padding_top; // wrong, but not used
            }
        }

        // Get the formatted text, so we can look where in it is this XPointer
        LFormattedTextRef txtform;
        finalNode->renderFinalBlock( txtform, &fmt, inner_width );

        ldomNode *node = getNode();
        int offset = getOffset();
////        ldomXPointerEx xp(node, offset);
////        if ( !node->isText() ) {
////            //ldomXPointerEx xp(node, offset);
////            xp.nextVisibleText();
////            node = xp.getNode();
////            offset = xp.getOffset();
////        }
//        if ( node->isElement() ) {
//            if ( offset>=0 ) {
//                //
//                if ( offset>= (int)node->getChildCount() ) {
//                    node = node->getLastTextChild();
//                    if ( node )
//                        offset = node->getText().length();
//                    else
//                        return false;
//                } else {
//                    for ( int ci=offset; ci<(int)node->getChildCount(); ci++ ) {
//                        ldomNode * child = node->getChildNode( offset );
//                        ldomNode * txt = txt = child->getFirstTextChild( true );
//                        if ( txt ) {
//                            node = txt;
////                            lString16 s = txt->getText();
////                            CRLog::debug("text: [%d] '%s'", s.length(), LCSTR(s));
//                            break;
//                        }
//                    }
//                    if ( !node->isText() )
//                        return false;
//                    offset = 0;
//                }
//            }
//        }

        // text node
        int srcIndex = -1;
        int srcLen = -1;
        int lastIndex = -1;
        int lastLen = -1;
        int lastOffset = -1;
        ldomXPointerEx xp(node, offset);
        for ( int i=0; i<txtform->GetSrcCount(); i++ ) {
            const src_text_fragment_t * src = txtform->GetSrcInfo(i);
            if ( src->flags & LTEXT_SRC_IS_FLOAT ) // skip floats
                continue;
            bool isObject = (src->flags&LTEXT_SRC_IS_OBJECT)!=0;
            if ( src->object == node ) {
                srcIndex = i;
                srcLen = isObject ? 0 : src->t.len;
                break;
            }
            lastIndex = i;
            lastLen =  isObject ? 0 : src->t.len;
            lastOffset = isObject ? 0 : src->t.offset;
            ldomXPointerEx xp2((ldomNode*)src->object, lastOffset);
            if ( xp2.compare(xp)>0 ) {
                srcIndex = i;
                srcLen = lastLen;
                offset = lastOffset;
                break;
            }
        }
        if ( srcIndex == -1 ) {
            if ( lastIndex<0 )
                return false;
            srcIndex = lastIndex;
            srcLen = lastLen;
            offset = lastOffset;
        }

        // Some state for non-linear bidi word search
        int nearestForwardSrcIndex = -1;
        int nearestForwardSrcOffset = -1;
        lvRect bestBidiRect = lvRect();
        bool hasBestBidiRect = false;

        for ( int l = 0; l<txtform->GetLineCount(); l++ ) {
            const formatted_line_t * frmline = txtform->GetLineInfo(l);
            bool line_is_bidi = frmline->flags & LTEXT_LINE_IS_BIDI;
            for ( int w=0; w<(int)frmline->word_count; w++ ) {
                const formatted_word_t * word = &frmline->words[w];
                bool word_is_rtl = word->flags & LTEXT_WORD_DIRECTION_IS_RTL;
                bool lastWord = (l == txtform->GetLineCount() - 1
                                 && w == frmline->word_count - 1);

                if ( line_is_bidi ) {
                    // When line is bidi, src text nodes may be shuffled, so we can't
                    // just be done when meeting a forward src in logical order.
                    // We'd better have a dedicated searching code to not mess with
                    // the visual=logical order generic code below.
                    // todo: see if additional tweaks according to
                    // frmline->flags&LTEXT_LINE_PARA_IS_RTL may help adjusting
                    // char rects depending on it vs word_is_rtl.
                    if ( word->src_text_index>=srcIndex || lastWord ) {
                        // Found word from same or forward src line
                        if (word->src_text_index > srcIndex &&
                               ( nearestForwardSrcIndex == -1 ||
                                 word->src_text_index < nearestForwardSrcIndex ||
                                 (word->src_text_index == nearestForwardSrcIndex &&
                                    word->t.start < nearestForwardSrcOffset ) ) ) {
                            // Found some word from a forward src that is nearest than previously found one:
                            // get its start as a possible best result.
                            bestBidiRect.top = rc.top + frmline->y;
                            bestBidiRect.bottom = bestBidiRect.top + frmline->height;
                            if ( word_is_rtl ) {
                                bestBidiRect.right = word->x + word->width + rc.left + frmline->x;
                                bestBidiRect.left = bestBidiRect.right - 1;
                            }
                            else {
                                bestBidiRect.left = word->x + rc.left + frmline->x;
                                if (extended) {
                                    if (word->flags & (LTEXT_WORD_IS_OBJECT|LTEXT_WORD_IS_INLINE_BOX) && word->width > 0)
                                        bestBidiRect.right = bestBidiRect.left + word->width; // width of image
                                    else
                                        bestBidiRect.right = bestBidiRect.left + 1;
                                }
                            }
                            hasBestBidiRect = true;
                            nearestForwardSrcIndex = word->src_text_index;
                            if (word->flags & (LTEXT_WORD_IS_OBJECT|LTEXT_WORD_IS_INLINE_BOX))
                                nearestForwardSrcOffset = 0;
                            else
                                nearestForwardSrcOffset = word->t.start;
                        }
                        else if (word->src_text_index == srcIndex) {
                            // Found word in that exact source text node
                            if ( word->flags & (LTEXT_WORD_IS_OBJECT|LTEXT_WORD_IS_INLINE_BOX) ) {
                                // An image is the single thing in its srcIndex
                                rect.top = rc.top + frmline->y;
                                rect.bottom = rect.top + frmline->height;
                                rect.left = word->x + rc.left + frmline->x;
                                if (word->width > 0)
                                    rect.right = rect.left + word->width; // width of image
                                else
                                    rect.right = rect.left + 1;
                                return true;
                            }
                            // Target is in this text node. We may not find it part
                            // of a word, so look at all words and keep the nearest
                            // (forward if possible) in case we don't find an exact one
                            if ( word->t.start > offset ) { // later word in logical order
                                if (nearestForwardSrcIndex != word->src_text_index ||
                                          word->t.start <= nearestForwardSrcOffset ) {
                                    bestBidiRect.top = rc.top + frmline->y;
                                    bestBidiRect.bottom = bestBidiRect.top + frmline->height;
                                    if ( word_is_rtl ) { // right edge of next logical word, as it is drawn on the left
                                        bestBidiRect.right = word->x + word->width + rc.left + frmline->x;
                                        bestBidiRect.left = bestBidiRect.right - 1;
                                    }
                                    else { // left edge of next logical word, as it is drawn on the right
                                        bestBidiRect.left = word->x + rc.left + frmline->x;
                                        bestBidiRect.right = bestBidiRect.left + 1;
                                    }
                                    hasBestBidiRect = true;
                                    nearestForwardSrcIndex = word->src_text_index;
                                    nearestForwardSrcOffset = word->t.start;
                                }
                            }
                            else if ( word->t.start+word->t.len <= offset ) { // past word in logical order
                                // Only if/while we haven't yet found one with the right src index and
                                // a forward offset
                                if (nearestForwardSrcIndex != word->src_text_index ||
                                        ( nearestForwardSrcOffset < word->t.start &&
                                          word->t.start+word->t.len > nearestForwardSrcOffset ) ) {
                                    bestBidiRect.top = rc.top + frmline->y;
                                    bestBidiRect.bottom = bestBidiRect.top + frmline->height;
                                    if ( word_is_rtl ) { // left edge of previous logical word, as it is drawn on the right
                                        bestBidiRect.left = word->x + rc.left + frmline->x;
                                        bestBidiRect.right = bestBidiRect.left + 1;
                                    }
                                    else { // right edge of previous logical word, as it is drawn on the left
                                        bestBidiRect.right = word->x + word->width + rc.left + frmline->x;
                                        bestBidiRect.left = bestBidiRect.right - 1;
                                    }
                                    hasBestBidiRect = true;
                                    nearestForwardSrcIndex = word->src_text_index;
                                    nearestForwardSrcOffset = word->t.start+word->t.len;
                                }
                            }
                            else { // exact word found
                                // Measure word
                                LVFont *font = (LVFont *) txtform->GetSrcInfo(srcIndex)->t.font;
                                lUInt16 w[512];
                                lUInt8 flg[512];
                                lString16 str = node->getText();
                                if (offset == word->t.start && str.empty()) {
                                    rect.left = word->x + rc.left + frmline->x;
                                    rect.top = rc.top + frmline->y;
                                    rect.right = rect.left + 1;
                                    rect.bottom = rect.top + frmline->height;
                                    return true;
                                }
                                // We need to transform the node text as it had been when
                                // rendered (the transform may change chars widths) for the
                                // rect to be correct
                                switch ( node->getParentNode()->getStyle()->text_transform ) {
                                    case css_tt_uppercase:
                                        str.uppercase();
                                        break;
                                    case css_tt_lowercase:
                                        str.lowercase();
                                        break;
                                    case css_tt_capitalize:
                                        str.capitalize();
                                        break;
                                    case css_tt_full_width:
                                        // str.fullWidthChars(); // disabled for now in lvrend.cpp
                                        break;
                                    default:
                                        break;
                                }
                                lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                                font->measureText(
                                    str.c_str()+word->t.start,
                                    word->t.len,
                                    w,
                                    flg,
                                    word->width+50,
                                    '?',
                                    txtform->GetSrcInfo(srcIndex)->lang_cfg,
                                    txtform->GetSrcInfo(srcIndex)->letter_spacing + word->added_letter_spacing,
                                    false,
                                    hints);
                                rect.top = rc.top + frmline->y;
                                rect.bottom = rect.top + frmline->height;
                                // chx is the width of previous chars in the word
                                int chx = (offset > word->t.start) ? w[ offset - word->t.start - 1 ] : 0;
                                if ( word_is_rtl ) {
                                    rect.right = word->x + word->width - chx + rc.left + frmline->x;
                                    rect.left = rect.right - 1;
                                }
                                else {
                                    rect.left = word->x + chx + rc.left + frmline->x;
                                    rect.right = rect.left + 1;
                                }
                                if (extended) { // get width of char at offset
                                    if (offset == word->t.start && word->t.len == 1) {
                                        // With CJK chars, the measured width seems
                                        // less correct than the one measured while
                                        // making words. So use the calculated word
                                        // width for one-char-long words instead
                                        if ( word_is_rtl )
                                            rect.left = rect.right - word->width;
                                        else
                                            rect.right = rect.left + word->width;
                                    }
                                    else {
                                        int chw = w[ offset - word->t.start ] - chx;
                                        bool hyphen_added = false;
                                        if ( offset == word->t.start + word->t.len - 1
                                                && (word->flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER)
                                                && !gFlgFloatingPunctuationEnabled ) {
                                            // if offset is the end of word, and this word has
                                            // been hyphenated, includes the hyphen width
                                            // (but not when floating punctuation is enabled,
                                            // to keep nice looking rectangles on multi lines
                                            // text selection)
                                            chw += font->getHyphenWidth();
                                            // We then should not account for the right side
                                            // bearing below
                                            hyphen_added = true;
                                        }
                                        if ( word_is_rtl ) {
                                            rect.left = rect.right - chw;
                                            if ( !hyphen_added ) {
                                                // Also remove our added letter spacing for justification
                                                // from the left, to have cleaner highlights.
                                                rect.left += word->added_letter_spacing;
                                            }
                                        }
                                        else {
                                            rect.right = rect.left + chw;
                                            if ( !hyphen_added ) {
                                                // Also remove our added letter spacing for justification
                                                // from the right, to have cleaner highlights.
                                                rect.right -= word->added_letter_spacing;
                                            }
                                        }
                                        if (adjusted) {
                                            // Extend left or right if this glyph overflows its
                                            // origin/advance box (can happen with an italic font,
                                            // or with a regular font on the right of the letter 'f'
                                            // or on the left of the letter 'J').
                                            // Only when negative (overflow) and not when positive
                                            // (which are more frequent), mostly to keep some good
                                            // looking rectangles on the sides when highlighting
                                            // multiple lines.
                                            rect.left += font->getLeftSideBearing(str[offset], true);
                                            if ( !hyphen_added )
                                                rect.right -= font->getRightSideBearing(str[offset], true);
                                            // Should work wheter rtl or ltr
                                        }
                                    }
                                    // Ensure we always return a non-zero width, even for zero-width
                                    // chars or collapsed spaces (to avoid isEmpty() returning true
                                    // which could be considered as a failure)
                                    if ( rect.right <= rect.left ) {
                                        if ( word_is_rtl )
                                            rect.left = rect.right - 1;
                                        else
                                            rect.right = rect.left + 1;
                                    }
                                }
                                return true;
                            }
                        }
                        if ( lastWord ) {
                            // If no exact word found, return best candidate
                            if (hasBestBidiRect) {
                                rect = bestBidiRect;
                                return true;
                            }
                            // Otherwise, return end of last word (?)
                            rect.top = rc.top + frmline->y;
                            rect.bottom = rect.top + frmline->height;
                            rect.left = word->x + rc.left + frmline->x + word->width;
                            rect.right = rect.left + 1;
                            return true;
                        }
                    }
                    continue;
                } // end if line_is_bidi

                // ================================
                // Generic code when visual order = logical order
                if ( word->src_text_index>=srcIndex || lastWord ) {
                    // found word from same src line
                    if ( word->flags & (LTEXT_WORD_IS_OBJECT|LTEXT_WORD_IS_INLINE_BOX)
                            || word->src_text_index > srcIndex
                            || (!extended && offset <= word->t.start)
                            || (extended && offset < word->t.start)
                            // if extended, and offset = word->t.start, we want to
                            // measure the first char, which is done in the next else
                            ) {
                        // before this word
                        rect.left = word->x + rc.left + frmline->x;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        if (extended) {
                            if (word->flags & (LTEXT_WORD_IS_OBJECT|LTEXT_WORD_IS_INLINE_BOX) && word->width > 0)
                                rect.right = rect.left + word->width; // width of image
                            else
                                rect.right = rect.left + 1; // not the right word: no char width
                        }
                        else {
                            rect.right = rect.left + 1;
                        }
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    } else if ( (word->src_text_index == srcIndex) &&
                                ( (offset < word->t.start+word->t.len) ||
                                  (offset==srcLen && offset == word->t.start+word->t.len) ) ) {
                        // pointer inside this word
                        LVFont *font = (LVFont *) txtform->GetSrcInfo(srcIndex)->t.font;
                        lUInt16 w[512];
                        lUInt8 flg[512];
                        lString16 str = node->getText();
                        // With "|| (extended && offset < word->t.start)" added to the first if
                        // above, we may now be here with: offset = word->t.start = 0
                        // and a node->getText() returning THE lString16::empty_str:
                        // font->measureText() would segfault on it because its just a dummy
                        // pointer. Not really sure why that happens.
                        // It happens when node is the <a> in:
                        //     <div><span> <a id="someId"/>Anciens </span> <b>...
                        // and offset=0, word->t.start=0, word->t.len=8 .
                        // We can just do as in the first 'if'.
                        if (offset == word->t.start && str.empty()) {
                            rect.left = word->x + rc.left + frmline->x;
                            rect.top = rc.top + frmline->y;
                            rect.right = rect.left + 1;
                            rect.bottom = rect.top + frmline->height;
                            return true;
                        }
                        // We need to transform the node text as it had been when
                        // rendered (the transform may change chars widths) for the
                        // rect to be correct
                        switch ( node->getParentNode()->getStyle()->text_transform ) {
                            case css_tt_uppercase:
                                str.uppercase();
                                break;
                            case css_tt_lowercase:
                                str.lowercase();
                                break;
                            case css_tt_capitalize:
                                str.capitalize();
                                break;
                            case css_tt_full_width:
                                // str.fullWidthChars(); // disabled for now in lvrend.cpp
                                break;
                            default:
                                break;
                        }
                        lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                        font->measureText(
                            str.c_str()+word->t.start,
                            word->t.len,
                            w,
                            flg,
                            word->width+50,
                            '?',
                            txtform->GetSrcInfo(srcIndex)->lang_cfg,
                            txtform->GetSrcInfo(srcIndex)->letter_spacing + word->added_letter_spacing,
                            false,
                            hints );
                        // chx is the width of previous chars in the word
                        int chx = (offset > word->t.start) ? w[ offset - word->t.start - 1 ] : 0;
                        rect.left = word->x + chx + rc.left + frmline->x;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        if (extended) { // get width of char at offset
                            if (offset == word->t.start && word->t.len == 1) {
                                // With CJK chars, the measured width seems
                                // less correct than the one measured while
                                // making words. So use the calculated word
                                // width for one-char-long words instead
                                rect.right = rect.left + word->width;
                            }
                            else {
                                int chw = w[ offset - word->t.start ] - chx;
                                bool hyphen_added = false;
                                if ( offset == word->t.start + word->t.len - 1
                                        && (word->flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER)
                                        && !gFlgFloatingPunctuationEnabled ) {
                                    // if offset is the end of word, and this word has
                                    // been hyphenated, includes the hyphen width
                                    // (but not when floating punctuation is enabled,
                                    // to keep nice looking rectangles on multi lines
                                    // text selection)
                                    chw += font->getHyphenWidth();
                                    // We then should not account for the right side
                                    // bearing below
                                    hyphen_added = true;
                                }
                                rect.right = rect.left + chw;
                                if ( !hyphen_added ) {
                                    // Also remove our added letter spacing for justification
                                    // from the right, to have cleaner highlights.
                                    rect.right -= word->added_letter_spacing;
                                }
                                if (adjusted) {
                                    // Extend left or right if this glyph overflows its
                                    // origin/advance box (can happen with an italic font,
                                    // or with a regular font on the right of the letter 'f'
                                    // or on the left of the letter 'J').
                                    // Only when negative (overflow) and not when positive
                                    // (which are more frequent), mostly to keep some good
                                    // looking rectangles on the sides when highlighting
                                    // multiple lines.
                                    rect.left += font->getLeftSideBearing(str[offset], true);
                                    if ( !hyphen_added )
                                        rect.right -= font->getRightSideBearing(str[offset], true);
                                }
                            }
                            // Ensure we always return a non-zero width, even for zero-width
                            // chars or collapsed spaces (to avoid isEmpty() returning true
                            // which could be considered as a failure)
                            if ( rect.right <= rect.left )
                                rect.right = rect.left + 1;
                        }
                        else
                            rect.right = rect.left + 1;
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    } else if (lastWord) {
                        // after last word
                        rect.left = word->x + rc.left + frmline->x + word->width;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        if (extended)
                            rect.right = rect.left + 1; // not the right word: no char width
                        else
                            rect.right = rect.left + 1;
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    }
                }
            }
        }
        // return false;
        // Not found, which is possible with a final node with only empty
        // elements. This final node has a rect, so use it.
        rect = rc;
        return true;
    } else {
        // no base final node, using blocks
        //lvRect rc;
        ldomNode * node = getNode();
        int offset = getOffset();
        if ( offset<0 || node->getChildCount()==0 ) {
            node->getAbsRect( rect );
            return true;
            //return rc.topLeft();
        }
        if ( offset < (int)node->getChildCount() ) {
            node->getChildNode(offset)->getAbsRect( rect );
            return true;
            //return rc.topLeft();
        }
        node->getChildNode(node->getChildCount()-1)->getAbsRect( rect );
        return true;
        //return rc.bottomRight();
    }
}
#endif

static bool isBoxingNode(ldomNode * node)
{
    // In the context this is used (xpointers), handle pseudoElems (that don't
    // box anything) just as boxing nodes: ignoring them in XPointers.
    return node->isBoxingNode(true);
}

static bool isTextNode(ldomNode * node)
{
    return (node && node->isText());
}

struct ldomNodeIdPredicate
{
    lUInt16 m_id;
    ldomNodeIdPredicate(lUInt16 id) : m_id(id) {}
    bool operator() (ldomNode * node) {
        return (node && node->getNodeId() == m_id);
    }
};

static bool notNull(ldomNode * node)
{
    return (NULL != node);
}

template<typename T>
static ldomNode * getNodeByIndex(ldomNode *parent, int index, T predicat, int& count)
{
    ldomNode *foundNode = NULL;

    for( int i=0; i < (int)parent->getChildCount(); i++) {
        ldomNode * p = parent->getChildNode(i);
        if( isBoxingNode(p) ) {
            foundNode = getNodeByIndex(p, index, predicat, count);
            if( foundNode )
                return foundNode;
        } else if(predicat(p)) {
            count++;
            if(index == -1 || count == index) {
                if( !foundNode )
                    foundNode = p;
                return foundNode;
            }
        }
    }
    return NULL;
}

/// create XPointer from relative pointer non-normalized string made by toStringV1()
ldomXPointer ldomDocument::createXPointerV1( ldomNode * baseNode, const lString16 & xPointerStr )
{
    //CRLog::trace( "ldomDocument::createXPointer(%s)", UnicodeToUtf8(xPointerStr).c_str() );
    if ( xPointerStr.empty() || !baseNode )
        return ldomXPointer();
    const lChar16 * str = xPointerStr.c_str();
    int index = -1;
    ldomNode * currNode = baseNode;
    lString16 name;
    lString8 ptr8 = UnicodeToUtf8(xPointerStr);
    //const char * ptr = ptr8.c_str();
    xpath_step_t step_type;

    while ( *str ) {
        //CRLog::trace( "    %s", UnicodeToUtf8(lString16(str)).c_str() );
        step_type = ParseXPathStep( str, name, index );
        //CRLog::trace( "        name=%s index=%d", UnicodeToUtf8(lString16(name)).c_str(), index );
        switch (step_type ) {
        case xpath_step_error:
            // error
            //CRLog::trace("    xpath_step_error");
            return ldomXPointer();
        case xpath_step_element:
            // element of type 'name' with 'index'        /elemname[N]/
            {
                lUInt16 id = getElementNameIndex( name.c_str() );
                ldomNode * foundItem = currNode->findChildElement(LXML_NS_ANY, id, index > 0 ? index - 1 : -1);
                if (foundItem == NULL && currNode->getChildCount() == 1) {
                    // make saved pointers work properly even after moving of some part of path one element deeper
                    foundItem = currNode->getChildNode(0)->findChildElement(LXML_NS_ANY, id, index > 0 ? index - 1 : -1);
                }
//                int foundCount = 0;
//                for (unsigned i=0; i<currNode->getChildCount(); i++) {
//                    ldomNode * p = currNode->getChildNode(i);
//                    //CRLog::trace( "        node[%d] = %d %s", i, p->getNodeId(), LCSTR(p->getNodeName()) );
//                    if ( p && p->isElement() && p->getNodeId()==id ) {
//                        foundCount++;
//                        if ( foundCount==index || index==-1 ) {
//                            foundItem = p;
//                            break; // DON'T CHECK WHETHER OTHER ELEMENTS EXIST
//                        }
//                    }
//                }
//                if ( foundItem==NULL || (index==-1 && foundCount>1) ) {
//                    //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
//                    return ldomXPointer(); // node not found
//                }
                if (foundItem == NULL) {
                    //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
                    return ldomXPointer(); // node not found
                }
                // found element node
                currNode = foundItem;
            }
            break;
        case xpath_step_text:
            // text node with 'index'                     /text()[N]/
            {
                ldomNode * foundItem = NULL;
                int foundCount = 0;
                for (int i=0; i<currNode->getChildCount(); i++) {
                    ldomNode * p = currNode->getChildNode(i);
                    if ( p->isText() ) {
                        foundCount++;
                        if ( foundCount==index || index==-1 ) {
                            foundItem = p;
                        }
                    }
                }
                if ( foundItem==NULL || (index==-1 && foundCount>1) )
                    return ldomXPointer(); // node not found
                // found text node
                currNode = foundItem;
            }
            break;
        case xpath_step_nodeindex:
            // node index                                 /N/
            if ( index<=0 || index>(int)currNode->getChildCount() )
                return ldomXPointer(); // node not found: invalid index
            currNode = currNode->getChildNode( index-1 );
            break;
        case xpath_step_point:
            // point index                                .N
            if (*str)
                return ldomXPointer(); // not at end of string
            if ( currNode->isElement() ) {
                // element point
                if ( index<0 || index>(int)currNode->getChildCount() )
                    return ldomXPointer();
                return ldomXPointer(currNode, index);
            } else {
                // text point
                if ( index<0 || index>(int)currNode->getText().length() )
                    return ldomXPointer();
                return ldomXPointer(currNode, index);
            }
            break;
        }
    }
    return ldomXPointer( currNode, -1 ); // XPath: index==-1
}

/// create XPointer from relative pointer normalized string made by toStringV2()
ldomXPointer ldomDocument::createXPointerV2( ldomNode * baseNode, const lString16 & xPointerStr )
{
    //CRLog::trace( "ldomDocument::createXPointer(%s)", UnicodeToUtf8(xPointerStr).c_str() );
    if ( xPointerStr.empty() || !baseNode )
        return ldomXPointer();
    const lChar16 * str = xPointerStr.c_str();
    int index = -1;
    int count;
    ldomNode * currNode = baseNode;
    ldomNode * foundNode;
    lString16 name;
    xpath_step_t step_type;

    while ( *str ) {
        //CRLog::trace( "    %s", UnicodeToUtf8(lString16(str)).c_str() );
        step_type = ParseXPathStep( str, name, index );
        //CRLog::trace( "        name=%s index=%d", UnicodeToUtf8(lString16(name)).c_str(), index );
        switch (step_type ) {
        case xpath_step_error:
            // error
            //CRLog::trace("    xpath_step_error");
            return ldomXPointer();
        case xpath_step_element:
            // element of type 'name' with 'index'        /elemname[N]/
            {
                ldomNodeIdPredicate predicat(getElementNameIndex( name.c_str() ));
                count = 0;
                foundNode = getNodeByIndex(currNode, index, predicat, count);
                if (foundNode == NULL) {
                    //CRLog::trace("    Element %d is not found. foundCount=%d", id, foundCount);
                    return ldomXPointer(); // node not found
                }
                // found element node
                currNode = foundNode;
                lString16 nm = currNode->getNodeName();
                CRLog::trace("%d -> %s", index, LCSTR(nm));
            }
            break;
        case xpath_step_text:
            //
            count = 0;
            foundNode = getNodeByIndex(currNode, index, isTextNode, count);

            if ( foundNode==NULL )
                return ldomXPointer(); // node not found
            // found text node
            currNode = foundNode;
            break;
        case xpath_step_nodeindex:
            // node index                                 /N/
            count = 0;
            foundNode = getNodeByIndex(currNode, index, notNull, count);
            if ( foundNode == NULL )
                return ldomXPointer(); // node not found: invalid index
            currNode = foundNode;
            break;
        case xpath_step_point:
            // point index                                .N
            if (*str)
                return ldomXPointer(); // not at end of string
            if ( currNode->isElement() ) {
                // element point
                if ( index<0 || index>(int)currNode->getChildCount() )
                    return ldomXPointer();
                return ldomXPointer(currNode, index);
            } else {
                // text point
                if ( index<0 || index>(int)currNode->getText().length() )
                    return ldomXPointer();
                return ldomXPointer(currNode, index);
            }
            break;
        }
    }
    return ldomXPointer( currNode, -1 ); // XPath: index==-1
}

/// returns XPath segment for this element relative to parent element (e.g. "p[10]")
lString16 ldomNode::getXPathSegment()
{
    if ( isNull() || isRoot() )
        return lString16::empty_str;
    ldomNode * parent = getParentNode();
    int cnt = parent->getChildCount();
    int index = 0;
    if ( isElement() ) {
        int id = getNodeId();
        for ( int i=0; i<cnt; i++ ) {
            ldomNode * node = parent->getChildNode(i);
            if ( node == this ) {
                return getNodeName() + "[" + fmt::decimal(index+1) + "]";
            }
            if ( node->isElement() && node->getNodeId()==id )
                index++;
        }
    } else {
        for ( int i=0; i<cnt; i++ ) {
            ldomNode * node = parent->getChildNode(i);
            if ( node == this ) {
                return "text()[" + lString16::itoa(index+1) + "]";
            }
            if ( node->isText() )
                index++;
        }
    }
    return lString16::empty_str;
}

// Using names, old, with boxing elements (non-normalized)
lString16 ldomXPointer::toStringV1()
{
    lString16 path;
    if ( isNull() )
        return path;
    ldomNode * node = getNode();
    int offset = getOffset();
    if ( offset >= 0 ) {
        path << "." << fmt::decimal(offset);
    }
    ldomNode * p = node;
    ldomNode * mainNode = node->getDocument()->getRootNode();
    while (p && p!=mainNode) {
        ldomNode * parent = p->getParentNode();
        if ( p->isElement() ) {
            // element
            lString16 name = p->getNodeName();
            lUInt16 id = p->getNodeId();
            if ( !parent )
                return "/" + name + path;
            int index = -1;
            int count = 0;
            for ( int i=0; i<parent->getChildCount(); i++ ) {
                ldomNode * node = parent->getChildElementNode( i, id );
                if ( node ) {
                    count++;
                    if ( node==p )
                        index = count;
                }
            }
            if ( count>1 )
                path = cs16("/") + name + "[" + fmt::decimal(index) + "]" + path;
            else
                path = cs16("/") + name + path;
        } else {
            // text
            if ( !parent )
                return cs16("/text()") + path;
            int index = -1;
            int count = 0;
            for ( int i=0; i<parent->getChildCount(); i++ ) {
                ldomNode * node = parent->getChildNode( i );
                if ( node->isText() ) {
                    count++;
                    if ( node==p )
                        index = count;
                }
            }
            if ( count>1 )
                path = cs16("/text()") + "[" + fmt::decimal(index) + "]" + path;
            else
                path = "/text()" + path;
        }
        p = parent;
    }
    return path;
}

template<typename T>
static int getElementIndex(ldomNode* parent, ldomNode *targetNode, T predicat, int& count)
{
    for ( int i=0; i<parent->getChildCount(); i++ ) {
        ldomNode * node = parent->getChildNode( i );
        if( isBoxingNode(node) && targetNode != node ) {
            int index = getElementIndex(node, targetNode, predicat, count);
            if(index > 0)
                return index;
        } else if (predicat(node))
           count++;
        if ( node==targetNode )
            return count;
    }
    return -1;
}

// Using names, new, without boxing elements, so: normalized
lString16 ldomXPointer::toStringV2()
{
    lString16 path;
    if ( isNull() )
        return path;
    ldomNode * node = getNode();
    int offset = getOffset();
    ldomNode * p = node;
    if ( !node->isBoxingNode(true) ) { // (nor pseudoElem)
        if ( offset >= 0 ) {
            path << "." << fmt::decimal(offset);
        }
    }
    else {
        if ( offset < p->getChildCount() )
            p = p->getChildNode(offset);
        else
            p = p->getParentNode();
    }
    ldomNode * mainNode = node->getDocument()->getRootNode();
    while (p && p!=mainNode) {
        ldomNode * parent = p->getParentNode();
        while( isBoxingNode(parent) )
            parent = parent->getParentNode();
        if ( p->isElement() ) {
            // element
            lString16 name = p->getNodeName();
            if ( !parent )
                return "/" + name + path;
            int count = 0;
            ldomNodeIdPredicate predicat(p->getNodeId());
            int index = getElementIndex(parent, p, predicat, count);
            if ( count == 1 ) {
                // We're first, but see if we have following siblings with the
                // same element name, so we can have "div[1]" instead of "div"
                // when parent has more than one of it (as toStringV1 does).
                ldomNode * n = p;
                while ( ( n = n->getUnboxedNextSibling(true) ) ) {
                    if ( predicat(n) ) { // We have such a followup sibling
                        count = 2; // there's at least 2 of them
                        break;
                    }
                }
            }
            if ( count>1 )
                path = cs16("/") + name + "[" + fmt::decimal(index) + "]" + path;
            else
                path = cs16("/") + name + path;
        } else {
            // text
            if ( !parent )
                return cs16("/text()") + path;
            int count = 0;
            int index = getElementIndex(parent, p, isTextNode, count);
            if ( count == 1 ) {
                // We're first, but see if we have following text siblings,
                // so we can have "text()[1]" instead of "text()" when
                // parent has more than one text node (as toStringV1 does).
                ldomNode * n = p;
                while ( ( n = n->getUnboxedNextSibling(false) ) ) {
                    if ( isTextNode(n) ) { // We have such a followup sibling
                        count = 2; // there's at least 2 of them
                        break;
                    }
                }
            }
            if ( count>1 )
                path = cs16("/text()") + "[" + fmt::decimal(index) + "]" + path;
            else
                path = "/text()" + path;
        }
        p = parent;
    }
    return path;
}

// Without element names, normalized (not used)
lString16 ldomXPointer::toStringV2AsIndexes()
{
    lString16 path;
    if ( isNull() )
        return path;
    int offset = getOffset();
    if ( offset >= 0 ) {
        path << "." << fmt::decimal(offset);
    }
    ldomNode * p = getNode();
    ldomNode * rootNode = p->getDocument()->getRootNode();
    while( p && p!=rootNode ) {
        ldomNode * parent = p->getParentNode();
        if ( !parent )
            return "/" + (p->isElement() ? p->getNodeName() : cs16("/text()")) + path;

        while( isBoxingNode(parent) )
            parent = parent->getParentNode();

        int count = 0;
        int index = getElementIndex(parent, p, notNull, count);

        if( index>0 ) {
            path = cs16("/") + fmt::decimal(index) + path;
        } else {
            CRLog::error("!!! child node not found in a parent");
        }
        p = parent;
    }
    return path;
}

#if BUILD_LITE!=1
int ldomDocument::getFullHeight()
{
    RenderRectAccessor rd( this->getRootNode() );
    return rd.getHeight() + rd.getY();
}
#endif




lString16 extractDocAuthors( ldomDocument * doc, lString16 delimiter, bool shortMiddleName )
{
    if ( delimiter.empty() )
        delimiter = ", ";
    lString16 authors;
    for ( int i=0; i<16; i++) {
        lString16 path = cs16("/FictionBook/description/title-info/author[") + fmt::decimal(i+1) + "]";
        ldomXPointer pauthor = doc->createXPointer(path);
        if ( !pauthor ) {
            //CRLog::trace( "xpath not found: %s", UnicodeToUtf8(path).c_str() );
            break;
        }
        lString16 firstName = pauthor.relative( L"/first-name" ).getText().trim();
        lString16 lastName = pauthor.relative( L"/last-name" ).getText().trim();
        lString16 middleName = pauthor.relative( L"/middle-name" ).getText().trim();
        lString16 author = firstName;
        if ( !author.empty() )
            author += " ";
        if ( !middleName.empty() )
            author += shortMiddleName ? lString16(middleName, 0, 1) + "." : middleName;
        if ( !lastName.empty() && !author.empty() )
            author += " ";
        author += lastName;
        if ( !authors.empty() )
            authors += delimiter;
        authors += author;
    }
    return authors;
}

lString16 extractDocTitle( ldomDocument * doc )
{
    return doc->createXPointer(L"/FictionBook/description/title-info/book-title").getText().trim();
}

lString16 extractDocLanguage( ldomDocument * doc )
{
    return doc->createXPointer(L"/FictionBook/description/title-info/lang").getText();
}

lString16 extractDocSeries( ldomDocument * doc, int * pSeriesNumber )
{
    lString16 res;
    ldomNode * series = doc->createXPointer(L"/FictionBook/description/title-info/sequence").getNode();
    if ( series ) {
        lString16 sname = lString16(series->getAttributeValue(attr_name)).trim();
        lString16 snumber = series->getAttributeValue(attr_number);
        if ( !sname.empty() ) {
            if ( pSeriesNumber ) {
                *pSeriesNumber = snumber.atoi();
                res = sname;
            } else {
                res << "(" << sname;
                if ( !snumber.empty() )
                    res << " #" << snumber << ")";
            }
        }
    }
    return res;
}

lString16 extractDocKeywords( ldomDocument * doc )
{
    lString16 res;
    // Year
    res << doc->createXPointer(L"/FictionBook/description/title-info/date").getText().trim();
    // Genres
    for ( int i=0; i<16; i++) {
        lString16 path = cs16("/FictionBook/description/title-info/genre[") + fmt::decimal(i+1) + "]";
        ldomXPointer genre = doc->createXPointer(path);
        if ( !genre ) {
            break;
        }
        if ( !res.empty() )
            res << "\n";
        res << genre.getText().trim();
    }
    return res;
}

lString16 extractDocDescription( ldomDocument * doc )
{
    // We put all other FB2 meta info in this description
    lString16 res;

    // Annotation (description)
    res << doc->createXPointer(L"/FictionBook/description/title-info/annotation").getText().trim();

    // Translators
    lString16 translators;
    int nbTranslators = 0;
    for ( int i=0; i<16; i++) {
        lString16 path = cs16("/FictionBook/description/title-info/translator[") + fmt::decimal(i+1) + "]";
        ldomXPointer ptranslator = doc->createXPointer(path);
        if ( !ptranslator ) {
            break;
        }
        lString16 firstName = ptranslator.relative( L"/first-name" ).getText().trim();
        lString16 lastName = ptranslator.relative( L"/last-name" ).getText().trim();
        lString16 middleName = ptranslator.relative( L"/middle-name" ).getText().trim();
        lString16 translator = firstName;
        if ( !translator.empty() )
            translator += " ";
        if ( !middleName.empty() )
            translator += middleName;
        if ( !lastName.empty() && !translator.empty() )
            translator += " ";
        translator += lastName;
        if ( !translators.empty() )
            translators << "\n";
        translators << translator;
        nbTranslators++;
    }
    if ( !translators.empty() ) {
        if ( !res.empty() )
            res << "\n\n";
        if ( nbTranslators > 1 )
            res << "Translators:\n" << translators;
        else
            res << "Translator: " << translators;
    }

    // Publication info & publisher
    ldomXPointer publishInfo = doc->createXPointer(L"/FictionBook/description/publish-info");
    if ( !publishInfo.isNull() ) {
        lString16 publisher = publishInfo.relative( L"/publisher" ).getText().trim();
        lString16 pubcity = publishInfo.relative( L"/city" ).getText().trim();
        lString16 pubyear = publishInfo.relative( L"/year" ).getText().trim();
        lString16 isbn = publishInfo.relative( L"/isbn" ).getText().trim();
        lString16 bookName = publishInfo.relative( L"/book-name" ).getText().trim();
        lString16 publication;
        if ( !publisher.empty() || !pubcity.empty() ) {
            if ( !publisher.empty() ) {
                publication << publisher;
            }
            if ( !pubcity.empty() ) {
                if ( !!publisher.empty() ) {
                    publication << ", ";
                }
                publication << pubcity;
            }
        }
        if ( !pubyear.empty() || !isbn.empty() ) {
            if ( !publication.empty() )
                publication << "\n";
            if ( !pubyear.empty() ) {
                publication << pubyear;
            }
            if ( !isbn.empty() ) {
                if ( !pubyear.empty() ) {
                    publication << ", ";
                }
                publication << isbn;
            }
        }
        if ( !bookName.empty() ) {
            if ( !publication.empty() )
                publication << "\n";
            publication << bookName;
        }
        if ( !publication.empty() ) {
            if ( !res.empty() )
                res << "\n\n";
            res << "Publication:\n" << publication;
        }
    }

    // Document info
    ldomXPointer pDocInfo = doc->createXPointer(L"/FictionBook/description/document-info");
    if ( !pDocInfo.isNull() ) {
        lString16 docInfo;
        lString16 docAuthors;
        int nbAuthors = 0;
        for ( int i=0; i<16; i++) {
            lString16 path = cs16("/FictionBook/description/document-info/author[") + fmt::decimal(i+1) + "]";
            ldomXPointer pdocAuthor = doc->createXPointer(path);
            if ( !pdocAuthor ) {
                break;
            }
            lString16 firstName = pdocAuthor.relative( L"/first-name" ).getText().trim();
            lString16 lastName = pdocAuthor.relative( L"/last-name" ).getText().trim();
            lString16 middleName = pdocAuthor.relative( L"/middle-name" ).getText().trim();
            lString16 docAuthor = firstName;
            if ( !docAuthor.empty() )
                docAuthor += " ";
            if ( !middleName.empty() )
                docAuthor += middleName;
            if ( !lastName.empty() && !docAuthor.empty() )
                docAuthor += " ";
            docAuthor += lastName;
            if ( !docAuthors.empty() )
                docAuthors << "\n";
            docAuthors << docAuthor;
            nbAuthors++;
        }
        if ( !docAuthors.empty() ) {
            if ( nbAuthors > 1 )
                docInfo << "Authors:\n" << docAuthors;
            else
                docInfo << "Author: " << docAuthors;
        }
        lString16 docPublisher = pDocInfo.relative( L"/publisher" ).getText().trim();
        lString16 docId = pDocInfo.relative( L"/id" ).getText().trim();
        lString16 docVersion = pDocInfo.relative( L"/version" ).getText().trim();
        lString16 docDate = pDocInfo.relative( L"/date" ).getText().trim();
        lString16 docHistory = pDocInfo.relative( L"/history" ).getText().trim();
        lString16 docSrcUrl = pDocInfo.relative( L"/src-url" ).getText().trim();
        lString16 docSrcOcr = pDocInfo.relative( L"/src-ocr" ).getText().trim();
        lString16 docProgramUsed = pDocInfo.relative( L"/program-used" ).getText().trim();
        if ( !docPublisher.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "Publisher: " << docPublisher;
        }
        if ( !docId.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "Id: " << docId;
        }
        if ( !docVersion.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "Version: " << docVersion;
        }
        if ( !docDate.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "Date: " << docDate;
        }
        if ( !docHistory.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "History: " << docHistory;
        }
        if ( !docSrcUrl.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "URL: " << docSrcUrl;
        }
        if ( !docSrcOcr.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "OCR: " << docSrcOcr;
        }
        if ( !docProgramUsed.empty() ) {
            if ( !docInfo.empty() )
                docInfo << "\n";
            docInfo << "Application: " << docProgramUsed;
        }
        if ( !docInfo.empty() ) {
            if ( !res.empty() )
                res << "\n\n";
            res << "Document:\n" << docInfo;
        }
    }

    return res;
}

void ldomXPointerEx::initIndex()
{
    int m[MAX_DOM_LEVEL];
    ldomNode * p = getNode();
    _level = 0;
    while ( p ) {
        m[_level] = p->getNodeIndex();
        _level++;
        p = p->getParentNode();
    }
    for ( int i=0; i<_level; i++ ) {
        _indexes[ i ] = m[ _level - i - 1 ];
    }
}

/// move to sibling #
bool ldomXPointerEx::sibling( int index )
{
    if ( _level <= 1 )
        return false;
    ldomNode * p = getNode()->getParentNode();
    if ( !p || index < 0 || index >= (int)p->getChildCount() )
        return false;
    setNode( p->getChildNode( index ) );
    setOffset(0);
    _indexes[ _level-1 ] = index;
    return true;
}

/// move to next sibling
bool ldomXPointerEx::nextSibling()
{
    return sibling( _indexes[_level-1] + 1 );
}

/// move to previous sibling
bool ldomXPointerEx::prevSibling()
{
    if ( _level <= 1 )
        return false;
    return sibling( _indexes[_level-1] - 1 );
}

/// move to next sibling element
bool ldomXPointerEx::nextSiblingElement()
{
    if ( _level <= 1 )
        return false;
    ldomNode * node = getNode();
    ldomNode * p = node->getParentNode();
    for ( int i=_indexes[_level-1] + 1; i<(int)p->getChildCount(); i++ ) {
        if ( p->getChildNode( i )->isElement() )
            return sibling( i );
    }
    return false;
}

/// move to previous sibling element
bool ldomXPointerEx::prevSiblingElement()
{
    if ( _level <= 1 )
        return false;
    ldomNode * node = getNode();
    ldomNode * p = node->getParentNode();
    for ( int i=_indexes[_level-1] - 1; i>=0; i-- ) {
        if ( p->getChildNode( i )->isElement() )
            return sibling( i );
    }
    return false;
}

/// move to next sibling or parent's next sibling
bool ldomXPointerEx::nextOuterElement()
{
    if ( !ensureElement() )
        return false;
    for (;;) {
        if ( nextSiblingElement() )
            return true;
        if ( !parent() )
            return false;
    }
}

/// move to (end of) last and deepest child node descendant of current node
bool ldomXPointerEx::lastInnerNode(bool toTextEnd)
{
    if ( !getNode() )
        return false;
    while ( lastChild() ) {}
    if ( isText() && toTextEnd ) {
        setOffset(getNode()->getText().length());
    }
    return true;
}

/// move to (end of) last and deepest child text node descendant of current node
bool ldomXPointerEx::lastInnerTextNode(bool toTextEnd)
{
    if ( !getNode() )
        return false;
    if ( isText() ) {
        if (toTextEnd)
            setOffset(getNode()->getText().length());
        return true;
    }
    if ( lastChild() ) {
        do {
            if (lastInnerTextNode(toTextEnd))
                return true;
        } while ( prevSibling() );
        parent();
    }
    return false;

}

/// move to parent
bool ldomXPointerEx::parent()
{
    if ( _level<=1 )
        return false;
    setNode( getNode()->getParentNode() );
    setOffset(0);
    _level--;
    return true;
}

/// move to child #
bool ldomXPointerEx::child( int index )
{
    if ( _level >= MAX_DOM_LEVEL )
        return false;
    int count = getNode()->getChildCount();
    if ( index<0 || index>=count )
        return false;
    _indexes[ _level++ ] = index;
    setNode( getNode()->getChildNode( index ) );
    setOffset(0);
    return true;
}

/// compare two pointers, returns -1, 0, +1
int ldomXPointerEx::compare( const ldomXPointerEx& v ) const
{
    int i;
    for ( i=0; i<_level && i<v._level; i++ ) {
        if ( _indexes[i] < v._indexes[i] )
            return -1;
        if ( _indexes[i] > v._indexes[i] )
            return 1;
    }
    if ( _level < v._level ) {
        return -1;
//        if ( getOffset() < v._indexes[i] )
//            return -1;
//        if ( getOffset() > v._indexes[i] )
//            return 1;
//        return -1;
    }
    if ( _level > v._level ) {
        if ( _indexes[i] < v.getOffset() )
            return -1;
        if ( _indexes[i] > v.getOffset() )
            return 1;
        return 1;
    }
    if ( getOffset() < v.getOffset() )
        return -1;
    if ( getOffset() > v.getOffset() )
        return 1;
    return 0;
}

/// calls specified function recursively for all elements of DOM tree
void ldomXPointerEx::recurseElements( void (*pFun)( ldomXPointerEx & node ) )
{
    if ( !isElement() )
        return;
    pFun( *this );
    if ( child( 0 ) ) {
        do {
            recurseElements( pFun );
        } while ( nextSibling() );
        parent();
    }
}

/// calls specified function recursively for all nodes of DOM tree
void ldomXPointerEx::recurseNodes( void (*pFun)( ldomXPointerEx & node ) )
{
    if ( !isElement() )
        return;
    pFun( *this );
    if ( child( 0 ) ) {
        do {
            recurseElements( pFun );
        } while ( nextSibling() );
        parent();
    }
}

/// returns true if this interval intersects specified interval
bool ldomXRange::checkIntersection( ldomXRange & v )
{
    if ( isNull() || v.isNull() )
        return false;
    if ( _end.compare( v._start ) < 0 )
        return false;
    if ( _start.compare( v._end ) > 0 )
        return false;
    return true;
}

/// create list by filtering existing list, to get only values which intersect filter range
ldomXRangeList::ldomXRangeList( ldomXRangeList & srcList, ldomXRange & filter )
{
    for ( int i=0; i<srcList.length(); i++ ) {
        if ( srcList[i]->checkIntersection( filter ) )
            LVPtrVector<ldomXRange>::add( new ldomXRange( *srcList[i] ) );
    }
}

/// copy constructor of full node range
ldomXRange::ldomXRange( ldomNode * p, bool fitEndToLastInnerChild )
: _start( p, 0 ), _end( p, p->isText() ? p->getText().length() : p->getChildCount() ), _flags(1)
{
    // Note: the above initialization seems wrong: for a non-text
    // node, offset seems of no-use, and setting it to the number
    // of children wouldn't matter (and if the original aim was to
    // extend end to include the last child, the range would ignore
    // this last child descendants).
    // The following change might well be the right behaviour expected
    // from ldomXRange(ldomNode) and fixing a bug, but let's keep
    // this "fixed" behaviour an option
    if (fitEndToLastInnerChild && !p->isText()) {
        // Update _end to point to the last deepest inner child node,
        // and to the end of its text if it is a text npde.
        ldomXPointerEx tmp = _start;
        if (tmp.lastInnerNode(true)) {
            _end = tmp;
        }
    }
    // Note: code that walks or compare a ldomXRange may include or
    // exclude the _end: most often, it's excluded.
    // If it is a text node, the end points to text.length(), so after the
    // last char, and it then includes the last char.
    // If it is a non-text node, we could choose to include or exclude it
    // in XPointers comparisons. Including it would have the node included,
    // but not its children (because a child is after its parent in
    // comparisons), which feels strange.
    // So, excluding it looks like the sanest choice.
    // But then, with fitEndToLastInnerChild, if that last inner child
    // is a <IMG> node, it will be the _end, but won't then be included
    // in the range... The proper way to include it then would be to use
    // ldomXPointerEx::nextOuterElement(), but this is just a trick (it
    // would fail if that node is the last in the document, and
    // getNearestParent() would move up unnecessary ancestors...)
    // So, better check the functions that we use to see how they would
    // cope with that case.
}

static const ldomXPointerEx & _max( const ldomXPointerEx & v1,  const ldomXPointerEx & v2 )
{
    int c = v1.compare( v2 );
    if ( c>=0 )
        return v1;
    else
        return v2;
}

static const ldomXPointerEx & _min( const ldomXPointerEx & v1,  const ldomXPointerEx & v2 )
{
    int c = v1.compare( v2 );
    if ( c<=0 )
        return v1;
    else
        return v2;
}

/// create intersection of two ranges
ldomXRange::ldomXRange( const ldomXRange & v1,  const ldomXRange & v2 )
    : _start( _max( v1._start, v2._start ) ), _end( _min( v1._end, v2._end ) )
{
}

/// create list splittiny existing list into non-overlapping ranges
ldomXRangeList::ldomXRangeList( ldomXRangeList & srcList, bool splitIntersections )
{
    if ( srcList.empty() )
        return;
    int i;
    if ( splitIntersections ) {
        ldomXRange * maxRange = new ldomXRange( *srcList[0] );
        for ( i=1; i<srcList.length(); i++ ) {
            if ( srcList[i]->getStart().compare( maxRange->getStart() ) < 0 )
                maxRange->setStart( srcList[i]->getStart() );
            if ( srcList[i]->getEnd().compare( maxRange->getEnd() ) > 0 )
                maxRange->setEnd( srcList[i]->getEnd() );
        }
        maxRange->setFlags(0);
        add( maxRange );
        for ( i=0; i<srcList.length(); i++ )
            split( srcList[i] );
        for ( int i=length()-1; i>=0; i-- ) {
            if ( get(i)->getFlags()==0 )
                erase( i, 1 );
        }
    } else {
        for ( i=0; i<srcList.length(); i++ )
            add( new ldomXRange( *srcList[i] ) );
    }
}

/// split into subranges using intersection
void ldomXRangeList::split( ldomXRange * r )
{
    int i;
    for ( i=0; i<length(); i++ ) {
        if ( r->checkIntersection( *get(i) ) ) {
            ldomXRange * src = remove( i );
            int cmp1 = src->getStart().compare( r->getStart() );
            int cmp2 = src->getEnd().compare( r->getEnd() );
            //TODO: add intersections
            if ( cmp1 < 0 && cmp2 < 0 ) {
                //   0====== src ======0
                //        X======= r=========X
                //   1111122222222222222
                ldomXRange * r1 = new ldomXRange( src->getStart(), r->getStart(), src->getFlags() );
                ldomXRange * r2 = new ldomXRange( r->getStart(), src->getEnd(), src->getFlags() | r->getFlags() );
                insert( i++, r1 );
                insert( i, r2 );
                delete src;
            } else if ( cmp1 > 0 && cmp2 > 0 ) {
                //           0====== src ======0
                //     X======= r=========X
                //           2222222222222233333
                ldomXRange * r2 = new ldomXRange( src->getStart(), r->getEnd(), src->getFlags() | r->getFlags() );
                ldomXRange * r3 = new ldomXRange( r->getEnd(), src->getEnd(), src->getFlags() );
                insert( i++, r2 );
                insert( i, r3 );
                delete src;
            } else if ( cmp1 < 0 && cmp2 > 0 ) {
                // 0====== src ================0
                //     X======= r=========X
                ldomXRange * r1 = new ldomXRange( src->getStart(), r->getStart(), src->getFlags() );
                ldomXRange * r2 = new ldomXRange( r->getStart(), r->getEnd(), src->getFlags() | r->getFlags() );
                ldomXRange * r3 = new ldomXRange( r->getEnd(), src->getEnd(), src->getFlags() );
                insert( i++, r1 );
                insert( i++, r2 );
                insert( i, r3 );
                delete src;
            } else if ( cmp1 == 0 && cmp2 > 0 ) {
                //   0====== src ========0
                //   X====== r=====X
                ldomXRange * r1 = new ldomXRange( src->getStart(), r->getEnd(), src->getFlags() | r->getFlags() );
                ldomXRange * r2 = new ldomXRange( r->getEnd(), src->getEnd(), src->getFlags() );
                insert( i++, r1 );
                insert( i, r2 );
                delete src;
            } else if ( cmp1 < 0 && cmp2 == 0 ) {
                //   0====== src =====0
                //      X====== r=====X
                ldomXRange * r1 = new ldomXRange( src->getStart(), r->getStart(), src->getFlags() );
                ldomXRange * r2 = new ldomXRange( r->getStart(), r->getEnd(), src->getFlags() | r->getFlags() );
                insert( i++, r1 );
                insert( i, r2 );
                delete src;
            } else {
                //        0====== src =====0
                //   X============== r===========X
                //
                //        0====== src =====0
                //   X============== r=====X
                //
                //   0====== src =====0
                //   X============== r=====X
                //
                //   0====== src ========0
                //   X========== r=======X
                src->setFlags( src->getFlags() | r->getFlags() );
                insert( i, src );
            }
        }
    }
}

#if BUILD_LITE!=1

bool ldomDocument::findText( lString16 pattern, bool caseInsensitive, bool reverse, int minY, int maxY, LVArray<ldomWord> & words, int maxCount, int maxHeight, int maxHeightCheckStartY )
{
    if ( minY<0 )
        minY = 0;
    int fh = getFullHeight();
    if ( maxY<=0 || maxY>fh )
        maxY = fh;
    // ldomXPointer start = createXPointer( lvPoint(0, minY), reverse?-1:1 );
    // ldomXPointer end = createXPointer( lvPoint(10000, maxY), reverse?-1:1 );
    // If we're provided with minY or maxY in some empty space (margins, empty
    // elements...), they may not resolve to a XPointer.
    // Find a valid y near each of them that does resolve to a XPointer:
    // We also want to get start/end point to logical-order HTML nodes,
    // which might be different from visual-order in bidi text.
    ldomXPointer start;
    ldomXPointer end;
    for (int y = minY; y >= 0; y--) {
        start = createXPointer( lvPoint(0, y), reverse ? PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST
                                                       : PT_DIR_SCAN_FORWARD_LOGICAL_FIRST );
        if (!start.isNull())
            break;
    }
    for (int y = maxY; y <= fh; y++) {
        end = createXPointer( lvPoint(10000, y), reverse ? PT_DIR_SCAN_BACKWARD_LOGICAL_LAST
                                                         : PT_DIR_SCAN_FORWARD_LOGICAL_LAST );
        if (!end.isNull())
            break;
    }

    if ( start.isNull() || end.isNull() )
        return false;
    ldomXRange range( start, end );
    CRLog::debug("ldomDocument::findText() for Y %d..%d, range %d..%d",
                 minY, maxY, start.toPoint().y, end.toPoint().y);
    if ( range.getStart().toPoint().y==-1 ) {
        range.getStart().nextVisibleText();
        CRLog::debug("ldomDocument::findText() updated range %d..%d",
                     range.getStart().toPoint().y, range.getEnd().toPoint().y);
    }
    if ( range.getEnd().toPoint().y==-1 ) {
        range.getEnd().prevVisibleText();
        CRLog::debug("ldomDocument::findText() updated range %d..%d",
                     range.getStart().toPoint().y, range.getEnd().toPoint().y);
    }
    if ( range.isNull() ) {
        CRLog::debug("No text found: Range is empty");
        return false;
    }
    return range.findText( pattern, caseInsensitive, reverse, words, maxCount, maxHeight, maxHeightCheckStartY );
}

static bool findText( const lString16 & str, int & pos, int & endpos, const lString16 & pattern )
{
    int len = pattern.length();
    if ( pos < 0 || pos + len > (int)str.length() )
        return false;
    const lChar16 * s1 = str.c_str() + pos;
    const lChar16 * s2 = pattern.c_str();
    int nlen = str.length() - pos - len;
    for ( int j=0; j<=nlen; j++ ) {
        bool matched = true;
        int nsofthyphens = 0; // There can be soft-hyphen in str, but not in pattern
        for ( int i=0; i<len; i++ ) {
            while ( i+nsofthyphens < nlen && s1[i+nsofthyphens] == UNICODE_SOFT_HYPHEN_CODE ) {
                nsofthyphens += 1;
            }
            if ( s1[i+nsofthyphens] != s2[i] ) {
                matched = false;
                break;
            }
        }
        if ( matched ) {
            endpos = pos + len + nsofthyphens;
            return true;
        }
        s1++;
        pos++;
    }
    return false;
}

static bool findTextRev( const lString16 & str, int & pos, int & endpos, const lString16 & pattern )
{
    int len = pattern.length();
    if ( pos+len>(int)str.length() )
        pos = str.length()-len;
    if ( pos < 0 )
        return false;
    const lChar16 * s1 = str.c_str() + pos;
    const lChar16 * s2 = pattern.c_str();
    int nlen = pos;
    for ( int j=nlen; j>=0; j-- ) {
        bool matched = true;
        int nsofthyphens = 0; // There can be soft-hyphen in str, but not in pattern
        for ( int i=0; i<len; i++ ) {
            while ( i+nsofthyphens < nlen && s1[i+nsofthyphens] == UNICODE_SOFT_HYPHEN_CODE ) {
                nsofthyphens += 1;
            }
            if ( s1[i+nsofthyphens] != s2[i] ) {
                matched = false;
                break;
            }
        }
        if ( matched ) {
            endpos = pos + len + nsofthyphens;
            return true;
        }
        s1--;
        pos--;
    }
    return false;
}

/// searches for specified text inside range
bool ldomXRange::findText( lString16 pattern, bool caseInsensitive, bool reverse, LVArray<ldomWord> & words, int maxCount, int maxHeight, int maxHeightCheckStartY, bool checkMaxFromStart )
{
    if ( caseInsensitive )
        pattern.lowercase();
    words.clear();
    if ( pattern.empty() )
        return false;
    if ( reverse ) {
        // reverse search
        if ( !_end.isText() ) {
            _end.prevVisibleText();
            lString16 txt = _end.getNode()->getText();
            _end.setOffset(txt.length());
        }
        int firstFoundTextY = -1;
        while ( !isNull() ) {

            lString16 txt = _end.getNode()->getText();
            int offs = _end.getOffset();
            int endpos;

            if ( firstFoundTextY!=-1 && maxHeight>0 ) {
                ldomXPointer p( _end.getNode(), offs );
                int currentTextY = p.toPoint().y;
                if ( currentTextY<firstFoundTextY-maxHeight )
                    return words.length()>0;
            }

            if ( caseInsensitive )
                txt.lowercase();

            while ( ::findTextRev( txt, offs, endpos, pattern ) ) {
                if ( firstFoundTextY==-1 && maxHeight>0 ) {
                    ldomXPointer p( _end.getNode(), offs );
                    int currentTextY = p.toPoint().y;
                    if (maxHeightCheckStartY == -1 || currentTextY <= maxHeightCheckStartY)
                        firstFoundTextY = currentTextY;
                }
                words.add( ldomWord(_end.getNode(), offs, endpos ) );
                offs--;
            }
            if ( !_end.prevVisibleText() )
                break;
            txt = _end.getNode()->getText();
            _end.setOffset(txt.length());
            if ( words.length() >= maxCount )
                break;
        }
    } else {
        // direct search
        if ( !_start.isText() )
            _start.nextVisibleText();
        int firstFoundTextY = -1;
        if (checkMaxFromStart) {
			ldomXPointer p( _start.getNode(), _start.getOffset() );
			firstFoundTextY = p.toPoint().y;
		}
        while ( !isNull() ) {
            int offs = _start.getOffset();
            int endpos;

            if ( firstFoundTextY!=-1 && maxHeight>0 ) {
                ldomXPointer p( _start.getNode(), offs );
                int currentTextY = p.toPoint().y;
                if ( (checkMaxFromStart && currentTextY>=firstFoundTextY+maxHeight) ||
					currentTextY>firstFoundTextY+maxHeight )
                    return words.length()>0;
            }

            lString16 txt = _start.getNode()->getText();
            if ( caseInsensitive )
                txt.lowercase();

            while ( ::findText( txt, offs, endpos, pattern ) ) {
                if ( firstFoundTextY==-1 && maxHeight>0 ) {
                    ldomXPointer p( _start.getNode(), offs );
                    int currentTextY = p.toPoint().y;
                    if (checkMaxFromStart) {
                        if ( currentTextY>=firstFoundTextY+maxHeight )
                            return words.length()>0;
                    } else {
                        if (maxHeightCheckStartY == -1 || currentTextY >= maxHeightCheckStartY)
                            firstFoundTextY = currentTextY;
                    }
                }
                words.add( ldomWord(_start.getNode(), offs, endpos ) );
                offs++;
            }
            if ( !_start.nextVisibleText() )
                break;
            if ( words.length() >= maxCount )
                break;
        }
    }
    return words.length() > 0;
}

/// fill marked ranges list
// Transform a list of ldomXRange (start and end xpointers) into a list
// of ldomMarkedRange (start and end point coordinates) for native
// drawing of highlights
void ldomXRangeList::getRanges( ldomMarkedRangeList &dst )
{
    dst.clear();
    if ( empty() )
        return;
    for ( int i=0; i<length(); i++ ) {
        ldomXRange * range = get(i);
        if (range->getFlags() < 0x10) {
            // Legacy marks drawing: make a single ldomMarkedRange spanning
            // multiple lines, assuming full width LTR paragraphs)
            // (Updated to use toPoint(extended=true) to have them shifted
            // by the margins and paddings of final blocks, to be compatible
            // with getSegmentRects() below that does that internally.)
            lvPoint ptStart = range->getStart().toPoint(true);
            lvPoint ptEnd = range->getEnd().toPoint(true);
            // LVE:DEBUG
            // CRLog::trace("selectRange( %d,%d : %d,%d : %s, %s )", ptStart.x, ptStart.y, ptEnd.x, ptEnd.y,
            //              LCSTR(range->getStart().toString()), LCSTR(range->getEnd().toString()) );
            if ( ptStart.y > ptEnd.y || ( ptStart.y == ptEnd.y && ptStart.x >= ptEnd.x ) ) {
                // Swap ptStart and ptEnd if coordinates seems inverted (or we would
                // get item->empty()), which is needed for bidi/rtl.
                // Hoping this has no side effect.
                lvPoint ptTmp = ptStart;
                ptStart = ptEnd;
                ptEnd = ptTmp;
            }
            ldomMarkedRange * item = new ldomMarkedRange( ptStart, ptEnd, range->getFlags() );
            if ( !item->empty() )
                dst.add( item );
            else
                delete item;
        }
        else {
            // Enhanced marks drawing: from a single ldomXRange, make multiple segmented
            // ldomMarkedRange, each spanning a single line.
            LVArray<lvRect> rects;
            range->getSegmentRects(rects);
            for (int i=0; i<rects.length(); i++) {
                lvRect r = rects[i];
                // printf("r %d %dx%d %dx%d\n", i, r.topLeft().x, r.topLeft().y, r.bottomRight().x, r.bottomRight().y);
                ldomMarkedRange * item = new ldomMarkedRange( r.topLeft(), r.bottomRight(), range->getFlags() );
                if ( !item->empty() )
                    dst.add( item );
                else
                    delete item;
            }
        }
    }
}

/// fill text selection list by splitting text into monotonic flags ranges
void ldomXRangeList::splitText( ldomMarkedTextList &dst, ldomNode * textNodeToSplit )
{
    lString16 text = textNodeToSplit->getText();
    if ( length()==0 ) {
        dst.add( new ldomMarkedText( text, 0, 0 ) );
        return;
    }
    ldomXRange textRange( textNodeToSplit );
    ldomXRangeList ranges;
    ranges.add( new ldomXRange(textRange) );
    int i;
    for ( i=0; i<length(); i++ ) {
        ranges.split( get(i) );
    }
    for ( i=0; i<ranges.length(); i++ ) {
        ldomXRange * r = ranges[i];
        int start = r->getStart().getOffset();
        int end = r->getEnd().getOffset();
        if ( end>start )
            dst.add( new ldomMarkedText( text.substr(start, end-start), r->getFlags(), start ) );
    }
    /*
    if ( dst.length() ) {
        CRLog::debug(" splitted: ");
        for ( int k=0; k<dst.length(); k++ ) {
            CRLog::debug("    (%d, %d) %s", dst[k]->offset, dst[k]->flags, UnicodeToUtf8(dst[k]->text).c_str());
        }
    }
    */
}

/// returns rectangle (in doc coordinates) for range. Returns true if found.
// Note that this works correctly only when start and end are in the
// same text node.
bool ldomXRange::getRectEx( lvRect & rect, bool & isSingleLine )
{
    isSingleLine = false;
    if ( isNull() )
        return false;
    // get start and end rects
    lvRect rc1;
    lvRect rc2;
    // inner=true if enhanced rendering, to directly get the inner coordinates,
    // so no need to compute paddings (as done below for legacy rendering)
    if ( !getStart().getRect(rc1, true) || !getEnd().getRect(rc2, true) )
        return false;
    ldomNode * finalNode1 = getStart().getFinalNode();
    ldomNode * finalNode2 = getEnd().getFinalNode();
    if ( !finalNode1 || !finalNode2 ) {
        // Shouldn't happen, but prevent a segfault in case some other bug
        // in initNodeRendMethod made some text not having a erm_final-like
        // ancestor.
        if ( !finalNode1 )
            printf("CRE WARNING: no final parent for range start %s\n", UnicodeToLocal(getStart().toString()).c_str());
        if ( !finalNode2 )
            printf("CRE WARNING: no final parent for range end %s\n", UnicodeToLocal(getEnd().toString()).c_str());
        return false;
    }
    RenderRectAccessor fmt1(finalNode1);
    RenderRectAccessor fmt2(finalNode2);
    // In legacy mode, we just got the erm_final coordinates, and we must
    // compute and add left/top border and padding (using rc.width() as
    // the base for % is wrong here, and so is rc.height() for padding top)
    if ( ! RENDER_RECT_HAS_FLAG(fmt1, INNER_FIELDS_SET) ) {
        int em = finalNode1->getFont()->getSize();
        int padding_left = measureBorder(finalNode1,3) + lengthToPx(finalNode1->getStyle()->padding[0], fmt1.getWidth(), em);
        int padding_top = measureBorder(finalNode1,0) + lengthToPx(finalNode1->getStyle()->padding[2], fmt1.getWidth(), em);
        rc1.top += padding_top;
        rc1.left += padding_left;
        rc1.right += padding_left;
        rc1.bottom += padding_top;
    }
    if ( ! RENDER_RECT_HAS_FLAG(fmt2, INNER_FIELDS_SET) ) {
        int em = finalNode2->getFont()->getSize();
        int padding_left = measureBorder(finalNode2,3) + lengthToPx(finalNode2->getStyle()->padding[0], fmt2.getWidth(), em);
        int padding_top = measureBorder(finalNode2,0) + lengthToPx(finalNode2->getStyle()->padding[2], fmt2.getWidth(), em);
        rc2.top += padding_top;
        rc2.left += padding_left;
        rc2.right += padding_left;
        rc2.bottom += padding_top;
    }
    if ( rc1.top == rc2.top && rc1.bottom == rc2.bottom ) {
        // on same line
        rect.left = rc1.left;
        rect.top = rc1.top;
        rect.right = rc2.right;
        rect.bottom = rc2.bottom;
        isSingleLine = true;
        return !rect.isEmpty();
    }
    // on different lines
    ldomNode * parent = getNearestCommonParent();
    if ( !parent )
        return false;
    parent->getAbsRect(rect);
    rect.top = rc1.top;
    rect.bottom = rc2.bottom;
    return !rect.isEmpty();
}

// Returns the multiple segments (rectangle for each text line) that
// this ldomXRange spans on the page.
// The text content from S to E on this page will push 4 segments:
//   ......
//   ...S==
//   ======
//   ======
//   ==E..
//   ......
void ldomXRange::getSegmentRects( LVArray<lvRect> & rects )
{
    bool go_on = true;
    int lcount = 1;
    lvRect lineStartRect = lvRect();
    lvRect nodeStartRect = lvRect();
    lvRect curCharRect = lvRect();
    lvRect prevCharRect = lvRect();
    ldomNode *prevFinalNode = NULL; // to add rect when we cross final nodes

    // We process range text node by text node (I thought rects' y-coordinates
    // comparisons were valid only for a same text node, but it seems all
    // text on a line get the same .top and .bottom, even if they have a
    // smaller font size - but using ldomXRange.getRectEx() on multiple
    // text nodes gives wrong rects for the last chars on a line...)

    // Note: someRect.extend(someOtherRect) and !someRect.isEmpty() expect
    // a rect to have both width and height non-zero. So, make sure
    // in getRectEx() that we always get a rect of width at least 1px,
    // otherwise some lines may not be highlighted.

    // Note: the range end offset is NOT part of the range (it points to the
    // char after, or last char + 1 if it includes the whole text node text)
    ldomXPointerEx rangeEnd = getEnd();
    ldomXPointerEx curPos = ldomXPointerEx( getStart() ); // copy, will change
    if (!curPos.isText()) // we only deal with text nodes: get the first
        go_on = curPos.nextText();

    while (go_on) { // new line or new/continued text node
        // We may have (empty or not if not yet pushed) from previous iteration:
        // lineStartRect : char rect for first char of line, even if from another text node
        // nodeStartRect : char rect of current char at curPos (calculated but not included
        //   in previous line), that is now the start of the line
        // The curPos.getRectEx(charRect) we use returns a rect for a single char, with
        // the width of the char. We then "extend" it to the char at end of line (or end
        // of range) to make a segment that we add to the provided &rects.
        // We use getRectEx() with adjusted=true, for fine tuned glyph rectangles
        // that include the excessive left or right side bearing.

        if (!curPos || curPos.isNull() || curPos.compare(rangeEnd) >= 0) {
            // no more text node, or after end of range: we're done
            go_on = false;
            break;
        }

        ldomNode *curFinalNode = curPos.getFinalNode();
        if (curFinalNode != prevFinalNode) {
            // Force a new segment if we're crossing final nodes, that is, when
            // we're no more in the same inline context (so we get a new segment
            // for each table cells that may happen to be rendered on the same line)
            if (! lineStartRect.isEmpty()) {
                rects.add( lineStartRect );
                lineStartRect = lvRect(); // reset
            }
            prevFinalNode = curFinalNode;
        }

        int startOffset = curPos.getOffset();
        lString16 nodeText = curPos.getText();
        int textLen = nodeText.length();

        if (startOffset == 0) { // new text node
            nodeStartRect = lvRect(); // reset
            if (textLen == 0) { // empty text node (not sure that can happen)
                go_on = curPos.nextText();
                continue;
            }
        }
        // Skip space at start of node or at start of new line
        // (the XML parser made sure we always have a single space
        // at boundaries)
        if (nodeText[startOffset] == ' ') {
            startOffset += 1;
            nodeStartRect = lvRect(); // reset
        }
        if (startOffset >= textLen) { // no more text in this node (or single space node)
            go_on = curPos.nextText();
            nodeStartRect = lvRect(); // reset
            continue;
        }
        curPos.setOffset(startOffset);
        if (nodeStartRect.isEmpty()) { // otherwise, we re-use the one left from previous loop
            // getRectEx() seems to fail on a single no-break-space, but we
            // are not supposed to see a no-br space at start of line.
            // Anyway, try next chars if first one(s) fails
            while (startOffset <= textLen-2 && !curPos.getRectEx(nodeStartRect, true)) {
                // printf("#### curPos.getRectEx(nodeStartRect:%d) failed\n", startOffset);
                startOffset++;
                curPos.setOffset(startOffset);
                nodeStartRect = lvRect(); // reset
            }
            // last try with the last char (startOffset = textLen-1):
            if (!curPos.getRectEx(nodeStartRect, true)) {
                // printf("#### curPos.getRectEx(nodeStartRect) failed\n");
                // getRectEx() returns false when a node is invisible, so we just
                // go processing next text node on failure (it may fail for other
                // reasons that we won't notice, except for may be holes in the
                // highlighting)
                go_on = curPos.nextText(); // skip this text node
                nodeStartRect = lvRect(); // reset
                continue;
            }
        }
        if (lineStartRect.isEmpty()) {
            lineStartRect = nodeStartRect; // re-use the one already computed
        }
        // This would help noticing a line-feed-back-to-start-of-line:
        //   else if (nodeStartRect.left < lineStartRect.right)
        // but it makes a 2-lines-tall single segment if text-indent is larger
        // than previous line end.
        // So, use .top comparison
        else if (nodeStartRect.top > lineStartRect.top) {
            // We ended last node on a line, but a new node starts (or previous
            // one continues) on a different line.
            // And we have a not-yet-added lineStartRect: add it as it is
            rects.add( lineStartRect );
            lineStartRect = nodeStartRect; // start line on current node
        }

        // 1) Look if text node contains end of range (probably the case
        // when only a few words are highlighted)
        if (curPos.getNode() == rangeEnd.getNode() && rangeEnd.getOffset() <= textLen) {
            curCharRect = lvRect();
            curPos.setOffset(rangeEnd.getOffset() - 1); // Range end is not part of the range
            if (!curPos.getRectEx(curCharRect, true)) {
                // printf("#### curPos.getRectEx(textLen=%d) failed\n", textLen);
                go_on = curPos.nextText(); // skip this text node
                nodeStartRect = lvRect(); // reset
                continue;
            }
            if (curCharRect.top == nodeStartRect.top) { // end of range is on current line
                // (Two offsets in a same text node with the same tops are on the same line)
                lineStartRect.extend(curCharRect);
                // lineStartRect will be added after loop exit
                go_on = false;
                break; // we're done
            }
        }

        // 2) Look if the full text node is contained on the line
        // Ignore (possibly collapsed) space at end of text node
        curPos.setOffset(nodeText[textLen-1] == ' ' ? textLen-2 : textLen-1 );
        curCharRect = lvRect();
        if (!curPos.getRectEx(curCharRect, true)) {
            // printf("#### curPos.getRectEx(textLen=%d) failed\n", textLen);
            go_on = curPos.nextText(); // skip this text node
            nodeStartRect = lvRect(); // reset
            continue;
        }
        if (curCharRect.top == nodeStartRect.top) {
            // Extend line up to the end of this node, but don't add it yet,
            // lineStartRect can still be extended with (parts of) next text nodes
            lineStartRect.extend(curCharRect);
            nodeStartRect  = lvRect(); // reset
            go_on = curPos.nextText(); // go processing next text node
            continue;
        }

        // 3) Current text node's end is not on our line:
        // scan it char by char to see where it changes line
        // (we could use binary search to reduce the number of iterations)
        curPos.setOffset(startOffset);
        prevCharRect = nodeStartRect;
        for (int i=startOffset+1; i<=textLen-1; i++) {
            // skip spaces (but let soft-hyphens in, so they are part of the
            // highlight when they are shown at end of line)
            lChar16 c = nodeText[i];
            if (c == ' ') // || c == 0x00AD)
                continue;
            curPos.setOffset(i);
            curCharRect = lvRect(); // reset
            if (!curPos.getRectEx(curCharRect, true)) {
                // printf("#### curPos.getRectEx(char=%d) failed\n", i);
                // Can happen with non-break-space and may be others,
                // just try with next char
                continue;
            }
            if (curPos.compare(rangeEnd) >= 0) {
                // should not happen, we should have dealt with it as 1)
                // printf("??????????? curPos.getRectEx(char=%d) end of range\n", i);
                go_on = false;        // don't break yet, need to add what we met before
                curCharRect.top = -1; // force adding prevCharRect
            }
            if (curCharRect.top != nodeStartRect.top) { // no more on the same line
                if ( ! prevCharRect.isEmpty() ) { // (should never be empty)
                    // We got previously a rect on this line: it's the end of line
                    lineStartRect.extend(prevCharRect);
                    rects.add( lineStartRect );
                }
                // Continue with this text node, but on a new line
                nodeStartRect = curCharRect;
                lineStartRect = lvRect(); // reset
                break; // break for (i<textLen) loop
            }
            prevCharRect = curCharRect; // still on the line: candidate for end of line
            if (! go_on)
                break; // we're done
        }
    }
    // Add any lineStartRect not yet added
    if (! lineStartRect.isEmpty()) {
        rects.add( lineStartRect );
    }
}

/// sets range to nearest word bounds, returns true if success
bool ldomXRange::getWordRange( ldomXRange & range, ldomXPointer & p )
{
    ldomNode * node = p.getNode();
    if ( !node->isText() )
        return false;
    int pos = p.getOffset();
    lString16 txt = node->getText();
    if ( pos<0 )
        pos = 0;
    if ( pos>(int)txt.length() )
        pos = txt.length();
    int endpos = pos;
    for (;;) {
        lChar16 ch = txt[endpos];
        if ( ch==0 || ch==' ' )
            break;
        endpos++;
    }
    /*
    // include trailing space
    for (;;) {
        lChar16 ch = txt[endpos];
        if ( ch==0 || ch!=' ' )
            break;
        endpos++;
    }
    */
    for ( ;; ) {
        if ( pos==0 )
            break;
        if ( txt[pos]!=' ' )
            break;
        pos--;
    }
    for ( ;; ) {
        if ( pos==0 )
            break;
        if ( txt[pos-1]==' ' )
            break;
        pos--;
    }
    ldomXRange r( ldomXPointer( node, pos ), ldomXPointer( node, endpos ) );
    range = r;
    return true;
}
#endif

/// returns true if intersects specified line rectangle
bool ldomMarkedRange::intersects( lvRect & rc, lvRect & intersection )
{
    if ( flags < 0x10 ) {
        // This assumes lines (rc) are from full-width LTR paragraphs, and
        // takes some shortcuts when checking intersection (it can be wrong
        // when floats, table cells, or RTL/BiDi text are involved).
        if ( start.y>=rc.bottom )
            return false;
        if ( end.y<rc.top )
            return false;
        intersection = rc;
        if ( start.y>=rc.top && start.y<rc.bottom ) {
            if ( start.x > rc.right )
                return false;
            intersection.left = rc.left > start.x ? rc.left : start.x;
        }
        if ( end.y>=rc.top && end.y<rc.bottom ) {
            if ( end.x < rc.left )
                return false;
            intersection.right = rc.right < end.x ? rc.right : end.x;
        }
        return true;
    }
    else {
        // Don't take any shortcut and check the full intersection
        if ( rc.bottom <= start.y || rc.top >= end.y || rc.right <= start.x || rc.left >= end.x ) {
            return false; // no intersection
        }
        intersection.top = rc.top > start.y ? rc.top : start.y;
        intersection.bottom = rc.bottom < end.y ? rc.bottom : end.y;
        intersection.left = rc.left > start.x ? rc.left : start.x;
        intersection.right = rc.right < end.x ? rc.right : end.x;
        return !intersection.isEmpty();
    }
}

/// create bounded by RC list, with (0,0) coordinates at left top corner
// crop/discard elements outside of rc (or outside of crop_rc instead if provided)
ldomMarkedRangeList::ldomMarkedRangeList( const ldomMarkedRangeList * list, lvRect & rc, lvRect * crop_rc )
{
    if ( !list || list->empty() )
        return;
//    if ( list->get(0)->start.y>rc.bottom )
//        return;
//    if ( list->get( list->length()-1 )->end.y < rc.top )
//        return;
    if ( !crop_rc ) {
        // If no alternate crop_rc provided, crop to the rc anchor
        crop_rc = &rc;
    }
    for ( int i=0; i<list->length(); i++ ) {
        ldomMarkedRange * src = list->get(i);
        if ( src->start.y >= crop_rc->bottom || src->end.y < crop_rc->top )
            continue;
        add( new ldomMarkedRange(
            lvPoint(src->start.x-rc.left, src->start.y-rc.top ),
            lvPoint(src->end.x-rc.left, src->end.y-rc.top ),
            src->flags ) );
    }
}

/// returns nearest common element for start and end points
ldomNode * ldomXRange::getNearestCommonParent()
{
    ldomXPointerEx start(getStart());
    ldomXPointerEx end(getEnd());
    while ( start.getLevel() > end.getLevel() && start.parent() )
        ;
    while ( start.getLevel() < end.getLevel() && end.parent() )
        ;
    /*
    while ( start.getIndex()!=end.getIndex() && start.parent() && end.parent() )
        ;
    if ( start.getNode()==end.getNode() )
        return start.getNode();
    return NULL;
    */
    // This above seems wrong: we could have start and end on the same level,
    // but in different parent nodes, with still the same index among these
    // different parent nodes' children.
    // Best to check for node identity, till we find the same parent,
    // or the root node
    while ( start.getNode()!=end.getNode() && start.parent() && end.parent() )
        ;
    return start.getNode();
}

/// returns HTML (serialized from the DOM, may be different from the source HTML)
/// puts the paths of the linked css files met into the provided lString16Collection cssFiles
lString8 ldomXPointer::getHtml(lString16Collection & cssFiles, int wflags)
{
    if ( isNull() )
        return lString8::empty_str;
    ldomNode * startNode = getNode();
    LVStreamRef stream = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
    writeNodeEx( stream.get(), startNode, cssFiles, wflags );
    int size = stream->GetSize();
    LVArray<char> buf( size+1, '\0' );
    stream->Seek(0, LVSEEK_SET, NULL);
    stream->Read( buf.get(), size, NULL );
    buf[size] = 0;
    lString8 html = lString8( buf.get() );
    return html;
}

/// returns HTML (serialized from the DOM, may be different from the source HTML)
/// puts the paths of the linked css files met into the provided lString16Collection cssFiles
lString8 ldomXRange::getHtml(lString16Collection & cssFiles, int wflags, bool fromRootNode)
{
    if ( isNull() )
        return lString8::empty_str;
    sort();
    ldomNode * startNode;
    if (fromRootNode) {
        startNode = getStart().getNode()->getDocument()->getRootNode();
        if (startNode->getChildCount() == 1) // start HTML with first child (<body>)
            startNode = startNode->getFirstChild();
    }
    else {
        // We need to start from the nearest common parent, to get balanced HTML
        startNode = getNearestCommonParent();
    }
    LVStreamRef stream = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
    writeNodeEx( stream.get(), startNode, cssFiles, wflags, getStart(), getEnd() );
    int size = stream->GetSize();
    LVArray<char> buf( size+1, '\0' );
    stream->Seek(0, LVSEEK_SET, NULL);
    stream->Read( buf.get(), size, NULL );
    buf[size] = 0;
    lString8 html = lString8( buf.get() );
    return html;
}

/// searches path for element with specific id, returns level at which element is founs, 0 if not found
int ldomXPointerEx::findElementInPath( lUInt16 id )
{
    if ( !ensureElement() )
        return 0;
    ldomNode * e = getNode();
    for ( ; e!=NULL; e = e->getParentNode() ) {
        if ( e->getNodeId()==id ) {
            return e->getNodeLevel();
        }
    }
    return 0;
}

bool ldomXPointerEx::ensureFinal()
{
    if ( !ensureElement() )
        return false;
    int cnt = 0;
    int foundCnt = -1;
    ldomNode * e = getNode();
    for ( ; e!=NULL; e = e->getParentNode() ) {
        if ( e->getRendMethod() == erm_final ) {
            foundCnt = cnt;
        }
        cnt++;
    }
    if ( foundCnt<0 )
        return false;
    for ( int i=0; i<foundCnt; i++ )
        parent();
    // curent node is final formatted element (e.g. paragraph)
    return true;
}

/// ensure that current node is element (move to parent, if not - from text node to element)
bool ldomXPointerEx::ensureElement()
{
    ldomNode * node = getNode();
    if ( !node )
        return false;
    if ( node->isText()) {
        if (!parent())
            return false;
        node = getNode();
    }
    if ( !node || !node->isElement() )
        return false;
    return true;
}

/// move to first child of current node
bool ldomXPointerEx::firstChild()
{
    return child(0);
}

/// move to last child of current node
bool ldomXPointerEx::lastChild()
{
    int count = getNode()->getChildCount();
    if ( count <=0 )
        return false;
    return child( count - 1 );
}

/// move to first element child of current node
bool ldomXPointerEx::firstElementChild()
{
    ldomNode * node = getNode();
    int count = node->getChildCount();
    for ( int i=0; i<count; i++ ) {
        if ( node->getChildNode( i )->isElement() )
            return child( i );
    }
    return false;
}

/// move to last element child of current node
bool ldomXPointerEx::lastElementChild()
{
    ldomNode * node = getNode();
    int count = node->getChildCount();
    for ( int i=count-1; i>=0; i-- ) {
        if ( node->getChildNode( i )->isElement() )
            return child( i );
    }
    return false;
}

/// forward iteration by elements of DOM three
bool ldomXPointerEx::nextElement()
{
    if ( !ensureElement() )
        return false;
    if ( firstElementChild() )
        return true;
    for (;;) {
        if ( nextSiblingElement() )
            return true;
        if ( !parent() )
            return false;
    }
}

/// returns true if current node is visible element with render method == erm_final
bool ldomXPointerEx::isVisibleFinal()
{
    if ( !isElement() )
        return false;
    int cnt = 0;
    int foundCnt = -1;
    ldomNode * e = getNode();
    for ( ; e!=NULL; e = e->getParentNode() ) {
        switch ( e->getRendMethod() ) {
        case erm_final:
            foundCnt = cnt;
            break;
        case erm_invisible:
            foundCnt = -1;
            break;
        default:
            break;
        }
        cnt++;
    }
    if ( foundCnt != 0 )
        return false;
    // curent node is visible final formatted element (e.g. paragraph)
    return true;
}

/// move to next visible text node
bool ldomXPointerEx::nextVisibleText( bool thisBlockOnly )
{
    ldomXPointerEx backup;
    if ( thisBlockOnly )
        backup = *this;
    while ( nextText(thisBlockOnly) ) {
        if ( isVisible() )
            return true;
    }
    if ( thisBlockOnly )
        *this = backup;
    return false;
}

/// returns true if current node is visible element or text
bool ldomXPointerEx::isVisible()
{
    ldomNode * p;
    ldomNode * node = getNode();
    if ( node && node->isText() )
        p = node->getParentNode();
    else
        p = node;
    while ( p ) {
        if ( p->getRendMethod() == erm_invisible )
            return false;
        p = p->getParentNode();
    }
    return true;
}

/// move to next text node
bool ldomXPointerEx::nextText( bool thisBlockOnly )
{
    ldomNode * block = NULL;
    if ( thisBlockOnly )
        block = getThisBlockNode();
    setOffset( 0 );
    while ( firstChild() ) {
        if ( isText() )
            return (!thisBlockOnly || getThisBlockNode()==block);
    }
    for (;;) {
        while ( nextSibling() ) {
            if ( isText() )
                return (!thisBlockOnly || getThisBlockNode()==block);
            while ( firstChild() ) {
                if ( isText() )
                    return (!thisBlockOnly || getThisBlockNode()==block);
            }
        }
        if ( !parent() )
            return false;
    }
}

/// move to previous text node
bool ldomXPointerEx::prevText( bool thisBlockOnly )
{
    ldomNode * block = NULL;
    if ( thisBlockOnly )
        block = getThisBlockNode();
    setOffset( 0 );
    for (;;) {
        while ( prevSibling() ) {
            if ( isText() )
                return  (!thisBlockOnly || getThisBlockNode()==block);
            while ( lastChild() ) {
                if ( isText() )
                    return (!thisBlockOnly || getThisBlockNode()==block);
            }
        }
        if ( !parent() )
            return false;
    }
}

/// move to previous visible text node
bool ldomXPointerEx::prevVisibleText( bool thisBlockOnly )
{
    ldomXPointerEx backup;
    if ( thisBlockOnly )
        backup = *this;
    while ( prevText( thisBlockOnly ) )
        if ( isVisible() )
            return true;
    if ( thisBlockOnly )
        *this = backup;
    return false;
}

/// move to previous visible char
bool ldomXPointerEx::prevVisibleChar( bool thisBlockOnly )
{
    if ( isNull() )
        return false;
    if ( !isText() || !isVisible() || _data->getOffset()==0 ) {
        // move to previous text
        if ( !prevVisibleText(thisBlockOnly) )
            return false;
        ldomNode * node = getNode();
        lString16 text = node->getText();
        int textLen = text.length();
        _data->setOffset( textLen );
    }
    _data->addOffset(-1);
    return true;
}

/// move to next visible char
bool ldomXPointerEx::nextVisibleChar( bool thisBlockOnly )
{
    if ( isNull() )
        return false;
    if ( !isText() || !isVisible() ) {
        // move to next text
        if ( !nextVisibleText(thisBlockOnly) )
            return false;
        _data->setOffset( 0 );
        return true;
    }
    ldomNode * node = getNode();
    lString16 text = node->getText();
    int textLen = text.length();
    if ( _data->getOffset() == textLen ) {
        // move to next text
        if ( !nextVisibleText(thisBlockOnly) )
            return false;
        _data->setOffset( 0 );
        return true;
    }
    _data->addOffset(1);
    return true;
}

// TODO: implement better behavior
inline bool IsUnicodeSpace( lChar16 ch )
{
    //return ch==' ';
    switch ((unsigned short)ch) {
        case 0x0020:        // SPACE
        case 0x00A0:        // NO-BREAK SPACE
        case 0x2000:        // EN QUAD
        case 0x2001:        // EM QUAD
        case 0x2002:        // EN SPACE
        case 0x2003:        // EM SPACE
        case 0x2004:        // THREE-PER-EM SPACE
        case 0x2005:        // FOUR-PER-EM SPACE
        case 0x202F:        // NARROW NO-BREAK SPACE
        case 0x3000:        // IDEOGRAPHIC SPACE
            return true;
    }
    return false;
}

// TODO: implement better behavior
inline bool IsUnicodeSpaceOrNull( lChar16 ch )
{
    return ch==0 || IsUnicodeSpace(ch);
}

// Note:
//  ALL calls to IsUnicodeSpace and IsUnicodeSpaceOrNull in
//  the *VisibleWord* functions below have been replaced with
//  calls to IsWordSeparator and IsWordSeparatorOrNull.
//  The *Sentence* functions have not beed modified, and have not been
//  tested against this change to the *VisibleWord* functions that
//  they use (but KOReader does not use these *Sentence* functions).

// For better accuracy than IsUnicodeSpace for detecting words
inline bool IsWordSeparator( lChar16 ch )
{
    return lStr_isWordSeparator(ch);
}

inline bool IsWordSeparatorOrNull( lChar16 ch )
{
    if (ch==0) return true;
    return IsWordSeparator(ch);
}

inline bool canWrapWordBefore( lChar16 ch ) {
    return ch>=0x2e80 && ch<0x2CEAF;
}

inline bool canWrapWordAfter( lChar16 ch ) {
    return ch>=0x2e80 && ch<0x2CEAF;
}

bool ldomXPointerEx::isVisibleWordChar() {
    if ( isNull() )
        return false;
    if ( !isText() || !isVisible() )
        return false;
    ldomNode * node = getNode();
    lString16 text = node->getText();
    return !IsWordSeparator(text[_data->getOffset()]);
}

/// move to previous visible word beginning
bool ldomXPointerEx::prevVisibleWordStart( bool thisBlockOnly )
{
    if ( isNull() )
        return false;
    ldomNode * node = NULL;
    lString16 text;
    int textLen = 0;
    for ( ;; ) {
        if ( !isText() || !isVisible() || _data->getOffset()==0 ) {
            // move to previous text
            if ( !prevVisibleText(thisBlockOnly) )
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset( textLen );
        } else {
            node = getNode();
            text = node->getText();
            textLen = text.length();
        }
        bool foundNonSpace = false;
        while ( _data->getOffset() > 0 && IsWordSeparator(text[_data->getOffset()-1]) )
            _data->addOffset(-1);
        while ( _data->getOffset()>0 ) {
            if ( IsWordSeparator(text[ _data->getOffset()-1 ]) )
                break;
            foundNonSpace = true;
            _data->addOffset(-1);
        }
        if ( foundNonSpace )
            return true;
    }
}

/// move to previous visible word end
bool ldomXPointerEx::prevVisibleWordEnd( bool thisBlockOnly )
{
    if ( isNull() )
        return false;
    ldomNode * node = NULL;
    lString16 text;
    int textLen = 0;
    bool moved = false;
    for ( ;; ) {
        if ( !isText() || !isVisible() || _data->getOffset()==0 ) {
            // move to previous text
            if ( !prevVisibleText(thisBlockOnly) )
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset( textLen );
            moved = true;
        } else {
            node = getNode();
            text = node->getText();
            textLen = text.length();
        }
        // skip spaces
        while ( _data->getOffset() > 0 && IsWordSeparator(text[_data->getOffset()-1]) ) {
            _data->addOffset(-1);
            moved = true;
        }
        if ( moved && _data->getOffset()>0 )
            return true; // found!
        // skip non-spaces
        while ( _data->getOffset()>0 ) {
            if ( IsWordSeparator(text[ _data->getOffset()-1 ]) )
                break;
            _data->addOffset(-1);
        }
        // skip spaces
        while ( _data->getOffset() > 0 && IsWordSeparator(text[_data->getOffset()-1]) ) {
            _data->addOffset(-1);
            moved = true;
        }
        if ( moved && _data->getOffset()>0 )
            return true; // found!
    }
}

/// move to next visible word beginning
bool ldomXPointerEx::nextVisibleWordStart( bool thisBlockOnly )
{
    if ( isNull() )
        return false;
    ldomNode * node = NULL;
    lString16 text;
    int textLen = 0;
    bool moved = false;
    for ( ;; ) {
        if ( !isText() || !isVisible() ) {
            // move to previous text
            if ( !nextVisibleText(thisBlockOnly) )
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset( 0 );
            moved = true;
        } else {
            for (;;) {
                node = getNode();
                text = node->getText();
                textLen = text.length();
                if ( _data->getOffset() < textLen )
                    break;
                if ( !nextVisibleText(thisBlockOnly) )
                    return false;
                _data->setOffset( 0 );
                moved = true;
            }
        }
        // skip spaces
        while ( _data->getOffset()<textLen && IsWordSeparator(text[ _data->getOffset() ]) ) {
            _data->addOffset(1);
            moved = true;
        }
        if ( moved && _data->getOffset()<textLen )
            return true;
        // skip non-spaces
        while ( _data->getOffset()<textLen ) {
            if ( IsWordSeparator(text[ _data->getOffset() ]) )
                break;
            moved = true;
            _data->addOffset(1);
        }
        // skip spaces
        while ( _data->getOffset()<textLen && IsWordSeparator(text[ _data->getOffset() ]) ) {
            _data->addOffset(1);
            moved = true;
        }
        if ( moved && _data->getOffset()<textLen )
            return true;
    }
}

/// move to end of current word
bool ldomXPointerEx::thisVisibleWordEnd(bool thisBlockOnly)
{
    CR_UNUSED(thisBlockOnly);
    if ( isNull() )
        return false;
    ldomNode * node = NULL;
    lString16 text;
    int textLen = 0;
    bool moved = false;
    if ( !isText() || !isVisible() )
        return false;
    node = getNode();
    text = node->getText();
    textLen = text.length();
    if ( _data->getOffset() >= textLen )
        return false;
    // skip spaces
    while ( _data->getOffset()<textLen && IsWordSeparator(text[ _data->getOffset() ]) ) {
        _data->addOffset(1);
        //moved = true;
    }
    // skip non-spaces
    while ( _data->getOffset()<textLen ) {
        if ( IsWordSeparator(text[ _data->getOffset() ]) )
            break;
        moved = true;
        _data->addOffset(1);
    }
    return moved;
}

/// move to next visible word end
bool ldomXPointerEx::nextVisibleWordEnd( bool thisBlockOnly )
{
    if ( isNull() )
        return false;
    ldomNode * node = NULL;
    lString16 text;
    int textLen = 0;
    //bool moved = false;
    for ( ;; ) {
        if ( !isText() || !isVisible() ) {
            // move to previous text
            if ( !nextVisibleText(thisBlockOnly) )
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset( 0 );
            //moved = true;
        } else {
            for (;;) {
                node = getNode();
                text = node->getText();
                textLen = text.length();
                if ( _data->getOffset() < textLen )
                    break;
                if ( !nextVisibleText(thisBlockOnly) )
                    return false;
                _data->setOffset( 0 );
            }
        }
        bool nonSpaceFound = false;
        // skip non-spaces
        while ( _data->getOffset()<textLen ) {
            if ( IsWordSeparator(text[ _data->getOffset() ]) )
                break;
            nonSpaceFound = true;
            _data->addOffset(1);
        }
        if ( nonSpaceFound )
            return true;
        // skip spaces
        while ( _data->getOffset()<textLen && IsWordSeparator(text[ _data->getOffset() ]) ) {
            _data->addOffset(1);
            //moved = true;
        }
        // skip non-spaces
        while ( _data->getOffset()<textLen ) {
            if ( IsWordSeparator(text[ _data->getOffset() ]) )
                break;
            nonSpaceFound = true;
            _data->addOffset(1);
        }
        if ( nonSpaceFound )
            return true;
    }
}

/// returns true if current position is visible word beginning
bool ldomXPointerEx::isVisibleWordStart()
{
   if ( isNull() )
        return false;
    if ( !isText() || !isVisible() )
        return false;
    ldomNode * node = getNode();
    lString16 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    lChar16 currCh = i<textLen ? text[i] : 0;
    lChar16 prevCh = i<textLen && i>0 ? text[i-1] : 0;
    if (canWrapWordBefore(currCh) || (IsWordSeparatorOrNull(prevCh) && !IsWordSeparator(currCh)))
        return true;
    return false;
 }

/// returns true if current position is visible word end
bool ldomXPointerEx::isVisibleWordEnd()
{
    if ( isNull() )
        return false;
    if ( !isText() || !isVisible() )
        return false;
    ldomNode * node = getNode();
    lString16 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    lChar16 currCh = i>0 ? text[i-1] : 0;
    lChar16 nextCh = i<textLen ? text[i] : 0;
    if (canWrapWordAfter(currCh) || (!IsWordSeparator(currCh) && IsWordSeparatorOrNull(nextCh)))
        return true;
    return false;
}

/// returns block owner node of current node (or current node if it's block)
ldomNode * ldomXPointerEx::getThisBlockNode()
{
    if ( isNull() )
        return NULL;
    ldomNode * node = getNode();
    if ( node->isText() )
        node = node->getParentNode();
    for (;;) {
        if ( !node )
            return NULL;
        lvdom_element_render_method rm = node->getRendMethod();
        switch ( rm ) {
        case erm_runin: // treat as separate block
        case erm_block:
        case erm_final:
        case erm_mixed:
        case erm_list_item: // no more used (obsolete rendering method)
        case erm_table:
        case erm_table_row_group:
        case erm_table_row:
        case erm_table_caption:
            return node;
        default:
            break; // ignore
        }
        node = node->getParentNode();
    }
}

/// returns true if points to last visible text inside block element
bool ldomXPointerEx::isLastVisibleTextInBlock()
{
    if ( !isText() )
        return false;
    ldomXPointerEx pos(*this);
    return !pos.nextVisibleText(true);
}

/// returns true if points to first visible text inside block element
bool ldomXPointerEx::isFirstVisibleTextInBlock()
{
    if ( !isText() )
        return false;
    ldomXPointerEx pos(*this);
    return !pos.prevVisibleText(true);
}

// sentence navigation

/// returns true if points to beginning of sentence
bool ldomXPointerEx::isSentenceStart()
{
    if ( isNull() )
        return false;
    if ( !isText() || !isVisible() )
        return false;
    ldomNode * node = getNode();
    lString16 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    lChar16 currCh = i<textLen ? text[i] : 0;
    lChar16 prevCh = i>0 ? text[i-1] : 0;
    lChar16 prevNonSpace = 0;
    for ( ;i>0; i-- ) {
        lChar16 ch = text[i-1];
        if ( !IsUnicodeSpace(ch) ) {
            prevNonSpace = ch;
            break;
        }
    }
#if 0
    // At this implementation it's a wrong to check previous node
    if ( !prevNonSpace ) {
        ldomXPointerEx pos(*this);
        while ( !prevNonSpace && pos.prevVisibleText(true) ) {
            lString16 prevText = pos.getText();
            for ( int j=prevText.length()-1; j>=0; j-- ) {
                lChar16 ch = prevText[j];
                if ( !IsUnicodeSpace(ch) ) {
                    prevNonSpace = ch;
                    break;
                }
            }
        }
    }
#endif

    // skip separated separator.
    if (1 == textLen) {
        switch (currCh) {
            case '.':
            case '?':
            case '!':
            case L'\x2026': // horizontal ellypsis
                return false;
        }
    }

    if ( !IsUnicodeSpace(currCh) && IsUnicodeSpaceOrNull(prevCh) ) {
        switch (prevNonSpace) {
        case 0:
        case '.':
        case '?':
        case '!':
        case L'\x2026': // horizontal ellypsis
            return true;
        default:
            return false;
        }
    }
    return false;
}

/// returns true if points to end of sentence
bool ldomXPointerEx::isSentenceEnd()
{
    if ( isNull() )
        return false;
    if ( !isText() || !isVisible() )
        return false;
    ldomNode * node = getNode();
    lString16 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    lChar16 currCh = i<textLen ? text[i] : 0;
    lChar16 prevCh = i>0 ? text[i-1] : 0;
    if ( IsUnicodeSpaceOrNull(currCh) ) {
        switch (prevCh) {
        case 0:
        case '.':
        case '?':
        case '!':
        case L'\x2026': // horizontal ellypsis
            return true;
        default:
            break;
        }
    }
    // word is not ended with . ! ?
    // check whether it's last word of block
    ldomXPointerEx pos(*this);
    //return !pos.nextVisibleWordStart(true);
    return !pos.thisVisibleWordEnd(true);
}

/// move to beginning of current visible text sentence
bool ldomXPointerEx::thisSentenceStart()
{
    if ( isNull() )
        return false;
    if ( !isText() && !nextVisibleText() && !prevVisibleText() )
        return false;
    for (;;) {
        if ( isSentenceStart() )
            return true;
        if ( !prevVisibleWordStart(true) )
            return false;
    }
}

/// move to end of current visible text sentence
bool ldomXPointerEx::thisSentenceEnd()
{
    if ( isNull() )
        return false;
    if ( !isText() && !nextVisibleText() && !prevVisibleText() )
        return false;
    for (;;) {
        if ( isSentenceEnd() )
            return true;
        if ( !nextVisibleWordEnd(true) )
            return false;
    }
}

/// move to beginning of next visible text sentence
bool ldomXPointerEx::nextSentenceStart()
{
    if ( !isSentenceStart() && !thisSentenceEnd() )
        return false;
    for (;;) {
        if ( !nextVisibleWordStart() )
            return false;
        if ( isSentenceStart() )
            return true;
    }
}

/// move to beginning of prev visible text sentence
bool ldomXPointerEx::prevSentenceStart()
{
    if ( !thisSentenceStart() )
        return false;
    for (;;) {
        if ( !prevVisibleWordStart() )
            return false;
        if ( isSentenceStart() )
            return true;
    }
}

/// move to end of next visible text sentence
bool ldomXPointerEx::nextSentenceEnd()
{
    if ( !nextSentenceStart() )
        return false;
    return thisSentenceEnd();
}

/// move to end of next visible text sentence
bool ldomXPointerEx::prevSentenceEnd()
{
    if ( !thisSentenceStart() )
        return false;
    for (;;) {
        if ( !prevVisibleWordEnd() )
            return false;
        if ( isSentenceEnd() )
            return true;
    }
}

/// if start is after end, swap start and end
void ldomXRange::sort()
{
    if ( _start.isNull() || _end.isNull() )
        return;
    if ( _start.compare(_end) > 0 ) {
        ldomXPointer p1( _start );
        ldomXPointer p2( _end );
        _start = p2;
        _end = p1;
    }
}

/// backward iteration by elements of DOM three
bool ldomXPointerEx::prevElement()
{
    if ( !ensureElement() )
        return false;
    for (;;) {
        if ( prevSiblingElement() ) {
            while ( lastElementChild() )
                ;
            return true;
        }
        if ( !parent() )
            return false;
        return true;
    }
}

/// move to next final visible node (~paragraph)
bool ldomXPointerEx::nextVisibleFinal()
{
    for ( ;; ) {
        if ( !nextElement() )
            return false;
        if ( isVisibleFinal() )
            return true;
    }
}

/// move to previous final visible node (~paragraph)
bool ldomXPointerEx::prevVisibleFinal()
{
    for ( ;; ) {
        if ( !prevElement() )
            return false;
        if ( isVisibleFinal() )
            return true;
    }
}

/// run callback for each node in range
void ldomXRange::forEach( ldomNodeCallback * callback )
{
    if ( isNull() )
        return;
    ldomXRange pos( _start, _end, 0 );
    bool allowGoRecurse = true;
    while ( !pos._start.isNull() && pos._start.compare( _end ) < 0 ) {
        // do something
        ldomNode * node = pos._start.getNode();
        //lString16 path = pos._start.toString();
        //CRLog::trace( "%s", UnicodeToUtf8(path).c_str() );
        if ( node->isElement() ) {
            allowGoRecurse = callback->onElement( &pos.getStart() );
        } else if ( node->isText() ) {
            lString16 txt = node->getText();
            pos._end = pos._start;
            pos._start.setOffset( 0 );
            pos._end.setOffset( txt.length() );
            if ( _start.getNode() == node ) {
                pos._start.setOffset( _start.getOffset() );
            }
            if ( _end.getNode() == node && pos._end.getOffset() > _end.getOffset()) {
                pos._end.setOffset( _end.getOffset() );
            }
            callback->onText( &pos );
            allowGoRecurse = false;
        }
        // move to next item
        bool stop = false;
        if ( !allowGoRecurse || !pos._start.child(0) ) {
             while ( !pos._start.nextSibling() ) {
                if ( !pos._start.parent() ) {
                    stop = true;
                    break;
                }
            }
        }
        if ( stop )
            break;
    }
}

class ldomWordsCollector : public ldomNodeCallback {
    LVArray<ldomWord> & _list;
	ldomWordsCollector & operator = (ldomWordsCollector&) {
		// no assignment
        return *this;
    }
public:
    ldomWordsCollector( LVArray<ldomWord> & list )
        : _list( list )
    {
    }
    /// called for each found text fragment in range
    virtual void onText( ldomXRange * nodeRange )
    {
        ldomNode * node = nodeRange->getStart().getNode();
        lString16 text = node->getText();
        int len = text.length();
        int end = nodeRange->getEnd().getOffset();
        if ( len>end )
            len = end;
        int beginOfWord = -1;
        for ( int i=nodeRange->getStart().getOffset(); i <= len; i++ ) {
            // int alpha = lGetCharProps(text[i]) & CH_PROP_ALPHA;
            // Also allow digits (years, page numbers) to be considered words
            // int alpha = lGetCharProps(text[i]) & (CH_PROP_ALPHA|CH_PROP_DIGIT|CH_PROP_HYPHEN);
            // We use lStr_isWordSeparator() as the other word finding/skipping functions do,
            // so they all share the same notion of what a word is.
            int alpha = !lStr_isWordSeparator(text[i]); // alpha, number, CJK char
            if (alpha && beginOfWord<0 ) {
                beginOfWord = i;
            }
            if ( !alpha && beginOfWord>=0) { // space, punctuation, sign, paren...
                _list.add( ldomWord( node, beginOfWord, i ) );
                beginOfWord = -1;
            }
            if (lGetCharProps(text[i]) == CH_PROP_CJK && i < len) { // a CJK char makes its own word
                _list.add( ldomWord( node, i, i+1 ) );
                beginOfWord = -1;
            }
        }
    }
    /// called for each found node in range
    virtual bool onElement( ldomXPointerEx * ptr )
    {
        ldomNode * elem = ptr->getNode();
        if ( elem->getRendMethod()==erm_invisible )
            return false;
        return true;
    }
};

/// get all words from specified range
void ldomXRange::getRangeWords( LVArray<ldomWord> & list )
{
    ldomWordsCollector collector( list );
    forEach( &collector );
}

/// adds all visible words from range, returns number of added words
int ldomWordExList::addRangeWords( ldomXRange & range, bool /*trimPunctuation*/ ) {
    LVArray<ldomWord> list;
    range.getRangeWords( list );
    for ( int i=0; i<list.length(); i++ )
        add( new ldomWordEx(list[i]) );
    init();
    return list.length();
}

lvPoint ldomMarkedRange::getMiddlePoint() {
    if ( start.y==end.y ) {
        return lvPoint( ((start.x + end.x)>>1), start.y );
    } else {
        return start;
    }
}

/// returns distance (dx+dy) from specified point to middle point
int ldomMarkedRange::calcDistance( int x, int y, MoveDirection dir ) {
    lvPoint middle = getMiddlePoint();
    int dx = middle.x - x;
    int dy = middle.y - y;
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    switch (dir) {
    case DIR_LEFT:
    case DIR_RIGHT:
        return dx + dy;
    case DIR_UP:
    case DIR_DOWN:
        return dx + dy*100;
    case DIR_ANY:
        return dx + dy;
    }


    return dx + dy;
}

/// select word
void ldomWordExList::selectWord( ldomWordEx * word, MoveDirection dir )
{
    selWord = word;
    if ( selWord ) {
        lvPoint middle = word->getMark().getMiddlePoint();
        if ( x==-1 || (dir!=DIR_UP && dir!=DIR_DOWN) )
            x = middle.x;
        y = middle.y;
    } else {
        x = y = -1;
    }
}

/// select next word in specified direction
ldomWordEx * ldomWordExList::selectNextWord( MoveDirection dir, int moveBy )
{
    if ( !selWord )
        return selectMiddleWord();
    pattern.clear();
    for ( int i=0; i<moveBy; i++ ) {
        ldomWordEx * word = findNearestWord( x, y, dir );
        if ( word )
            selectWord( word, dir );
    }
    return selWord;
}

/// select middle word in range
ldomWordEx * ldomWordExList::selectMiddleWord() {
    if ( minx==-1 )
        init();
    ldomWordEx * word = findNearestWord( (maxx+minx)/2, (miny+maxy)/2, DIR_ANY );
    selectWord(word, DIR_ANY);
    return word;
}

ldomWordEx * ldomWordExList::findWordByPattern()
{
    ldomWordEx * lastBefore = NULL;
    ldomWordEx * firstAfter = NULL;
    bool selReached = false;
    for ( int i=0; i<length(); i++ ) {
        ldomWordEx * item = get(i);
        if ( item==selWord )
            selReached = true;
        lString16 text = item->getText();
        text.lowercase();
        bool flg = true;
        for ( int j=0; j<pattern.length(); j++ ) {
            if ( j>=text.length() ) {
                flg = false;
                break;
            }
            lString16 chars = pattern[j];
            chars.lowercase();
            bool charFound = false;
            for ( int k=0; k<chars.length(); k++ ) {
                if ( chars[k]==text[j] ) {
                    charFound = true;
                    break;
                }
            }
            if ( !charFound ) {
                flg = false;
                break;
            }
        }
        if ( !flg )
            continue;
        if ( selReached ) {
            if ( firstAfter==NULL )
                firstAfter = item;
        } else {
            lastBefore = item;
        }
    }

    if ( firstAfter )
        return firstAfter;
    else
        return lastBefore;
}

/// try append search pattern and find word
ldomWordEx * ldomWordExList::appendPattern(lString16 chars)
{
    pattern.add(chars);
    ldomWordEx * foundWord = findWordByPattern();

    if ( foundWord ) {
        selectWord(foundWord, DIR_ANY);
    } else {
        pattern.erase(pattern.length()-1, 1);
    }
    return foundWord;
}

/// remove last character from pattern and try to search
ldomWordEx * ldomWordExList::reducePattern()
{
    if ( pattern.length()==0 )
        return NULL;
    pattern.erase(pattern.length()-1, 1);
    ldomWordEx * foundWord = findWordByPattern();

    if ( foundWord )
        selectWord(foundWord, DIR_ANY);
    return foundWord;
}

/// find word nearest to specified point
ldomWordEx * ldomWordExList::findNearestWord( int x, int y, MoveDirection dir ) {
    if ( !length() )
        return NULL;
    int bestDistance = -1;
    ldomWordEx * bestWord = NULL;
    ldomWordEx * defWord = (dir==DIR_LEFT || dir==DIR_UP) ? get(length()-1) : get(0);
    int i;
    if ( dir==DIR_LEFT || dir==DIR_RIGHT ) {
        int thisLineY = -1;
        int thisLineDy = -1;
        for ( i=0; i<length(); i++ ) {
            ldomWordEx * item = get(i);
            lvPoint middle = item->getMark().getMiddlePoint();
            int dy = middle.y - y;
            if ( dy<0 ) dy = -dy;
            if ( thisLineY==-1 || thisLineDy>dy ) {
                thisLineY = middle.y;
                thisLineDy = dy;
            }
        }
        ldomWordEx * nextLineWord = NULL;
        for ( i=0; i<length(); i++ ) {
            ldomWordEx * item = get(i);
            if ( dir!=DIR_ANY && item==selWord )
                continue;
            ldomMarkedRange * mark = &item->getMark();
            lvPoint middle = mark->getMiddlePoint();
            switch ( dir ) {
            case DIR_LEFT:
                if ( middle.y<thisLineY )
                    nextLineWord = item; // last word of prev line
                if ( middle.x>=x )
                    continue;
                break;
            case DIR_RIGHT:
                if ( nextLineWord==NULL && middle.y>thisLineY )
                    nextLineWord = item; // first word of next line
                if ( middle.x<=x )
                    continue;
                break;
            case DIR_UP:
            case DIR_DOWN:
            case DIR_ANY:
                // none
                break;
            }
            if ( middle.y!=thisLineY )
                continue;
            int dist = mark->calcDistance(x, y, dir);
            if ( bestDistance==-1 || dist<bestDistance ) {
                bestWord = item;
                bestDistance = dist;
            }
        }
        if ( bestWord!=NULL )
            return bestWord; // found in the same line
        if ( nextLineWord!=NULL  )
            return nextLineWord;
        return defWord;
    }
    for ( i=0; i<length(); i++ ) {
        ldomWordEx * item = get(i);
        if ( dir!=DIR_ANY && item==selWord )
            continue;
        ldomMarkedRange * mark = &item->getMark();
        lvPoint middle = mark->getMiddlePoint();
        if ( dir==DIR_UP && middle.y >= y )
            continue;
        if ( dir==DIR_DOWN && middle.y <= y )
            continue;

        int dist = mark->calcDistance(x, y, dir);
        if ( bestDistance==-1 || dist<bestDistance ) {
            bestWord = item;
            bestDistance = dist;
        }
    }
    if ( bestWord!=NULL )
        return bestWord;
    return defWord;
}

void ldomWordExList::init()
{
    if ( !length() )
        return;
    for ( int i=0; i<length(); i++ ) {
        ldomWordEx * item = get(i);
        lvPoint middle = item->getMark().getMiddlePoint();
        if ( i==0 || minx > middle.x )
            minx = middle.x;
        if ( i==0 || maxx < middle.x )
            maxx = middle.x;
        if ( i==0 || miny > middle.y )
            miny = middle.y;
        if ( i==0 || maxy < middle.y )
            maxy = middle.y;
    }
}


class ldomTextCollector : public ldomNodeCallback
{
private:
    bool lastText;
    bool newBlock;
    lChar16  delimiter;
    int  maxLen;
    lString16 text;
public:
    ldomTextCollector( lChar16 blockDelimiter, int maxTextLen )
        : lastText(false), newBlock(true), delimiter( blockDelimiter), maxLen( maxTextLen )
    {
    }
    /// destructor
    virtual ~ldomTextCollector() { }
    /// called for each found text fragment in range
    virtual void onText( ldomXRange * nodeRange )
    {
        if ( newBlock && !text.empty()) {
            text << delimiter;
        }
        lString16 txt = nodeRange->getStart().getNode()->getText();
        int start = nodeRange->getStart().getOffset();
        int end = nodeRange->getEnd().getOffset();
        if ( start < end ) {
            text << txt.substr( start, end-start );
        }
        lastText = true;
        newBlock = false;
    }
    /// called for each found node in range
    virtual bool onElement( ldomXPointerEx * ptr )
    {
#if BUILD_LITE!=1
        ldomNode * elem = (ldomNode *)ptr->getNode();
        if ( elem->getRendMethod() == erm_invisible )
            return false;
        // Allow tweaking that with hints
        css_cr_hint_t hint = elem->getStyle()->cr_hint;
        if ( hint == css_cr_hint_text_selection_skip ) {
            return false;
        }
        else if ( hint == css_cr_hint_text_selection_inline ) {
            newBlock = false;
            return true;
        }
        else if ( hint == css_cr_hint_text_selection_block ) {
            newBlock = true;
            return true;
        }
        switch ( elem->getStyle()->display ) {
        /*
        case css_d_inherit:
        case css_d_block:
        case css_d_list_item:
        case css_d_list_item_block:
        case css_d_compact:
        case css_d_marker:
        case css_d_table:
        case css_d_table_row_group:
        case css_d_table_header_group:
        case css_d_table_footer_group:
        case css_d_table_row:
        case css_d_table_column_group:
        case css_d_table_column:
        case css_d_table_cell:
        case css_d_table_caption:
        */
        default:
            newBlock = true;
            return true;
        case css_d_none:
            return false;
        case css_d_inline:
        case css_d_run_in:
        case css_d_inline_block: // Make these behave as inline, in case they don't contain much
        case css_d_inline_table: // (if they do, some inner block element will give newBlock=true)
            newBlock = false;
            return true;
        }
#else
        newBlock = true;
        return true;
#endif
    }
    /// get collected text
    lString16 getText() { return text; }
};

/// returns text between two XPointer positions
lString16 ldomXRange::getRangeText( lChar16 blockDelimiter, int maxTextLen )
{
    ldomTextCollector callback( blockDelimiter, maxTextLen );
    forEach( &callback );
    return removeSoftHyphens( callback.getText() );
}

/// returns href attribute of <A> element, plus xpointer of <A> element itself
lString16 ldomXPointer::getHRef(ldomXPointer & a_xpointer)
{
    if ( isNull() )
        return lString16::empty_str;
    ldomNode * node = getNode();
    while ( node && !node->isElement() )
        node = node->getParentNode();
    while ( node && node->getNodeId()!=el_a )
        node = node->getParentNode();
    if ( !node )
        return lString16::empty_str;
    a_xpointer.setNode(node);
    a_xpointer.setOffset(0);
    lString16 ref = node->getAttributeValue( LXML_NS_ANY, attr_href );
    if (!ref.empty() && ref[0] != '#')
        ref = DecodeHTMLUrlString(ref);
    return ref;
}

/// returns href attribute of <A> element, null string if not found
lString16 ldomXPointer::getHRef()
{
    ldomXPointer unused_a_xpointer;
    return getHRef(unused_a_xpointer);
}

/// returns href attribute of <A> element, plus xpointer of <A> element itself
lString16 ldomXRange::getHRef(ldomXPointer & a_xpointer)
{
    if ( isNull() )
        return lString16::empty_str;
    return _start.getHRef(a_xpointer);
}

/// returns href attribute of <A> element, null string if not found
lString16 ldomXRange::getHRef()
{
    if ( isNull() )
        return lString16::empty_str;
    return _start.getHRef();
}


ldomDocument * LVParseXMLStream( LVStreamRef stream,
                              const elem_def_t * elem_table,
                              const attr_def_t * attr_table,
                              const ns_def_t * ns_table )
{
    if ( stream.isNull() )
        return NULL;
    bool error = true;
    ldomDocument * doc;
    doc = new ldomDocument();
    doc->setDocFlags( 0 );

    ldomDocumentWriter writer(doc);
    doc->setNodeTypes( elem_table );
    doc->setAttributeTypes( attr_table );
    doc->setNameSpaceTypes( ns_table );

    /// FB2 format
    LVFileFormatParser * parser = new LVXMLParser(stream, &writer);
    if ( parser->CheckFormat() ) {
        if ( parser->Parse() ) {
            error = false;
        }
    }
    delete parser;
    if ( error ) {
        delete doc;
        doc = NULL;
    }
    return doc;
}

ldomDocument * LVParseHTMLStream( LVStreamRef stream,
                              const elem_def_t * elem_table,
                              const attr_def_t * attr_table,
                              const ns_def_t * ns_table )
{
    if ( stream.isNull() )
        return NULL;
    bool error = true;
    ldomDocument * doc;
    doc = new ldomDocument();
    doc->setDocFlags( 0 );

    ldomDocumentWriterFilter writerFilter(doc, false, HTML_AUTOCLOSE_TABLE);
    doc->setNodeTypes( elem_table );
    doc->setAttributeTypes( attr_table );
    doc->setNameSpaceTypes( ns_table );

    /// FB2 format
    LVFileFormatParser * parser = new LVHTMLParser(stream, &writerFilter);
    if ( parser->CheckFormat() ) {
        if ( parser->Parse() ) {
            error = false;
        }
    }
    delete parser;
    if ( error ) {
        delete doc;
        doc = NULL;
    }
    return doc;
}

#if 0
static lString16 escapeDocPath( lString16 path )
{
    for ( int i=0; i<path.length(); i++ ) {
        lChar16 ch = path[i];
        if ( ch=='/' || ch=='\\')
            path[i] = '_';
    }
    return path;
}
#endif

/////////////////////////////////////////////////////////////////
/// ldomDocumentFragmentWriter
// Used for EPUB with each individual HTML files in the EPUB,
// drives ldomDocumentWriter to build one single document from them.

lString16 ldomDocumentFragmentWriter::convertId( lString16 id )
{
    if ( !codeBasePrefix.empty() ) {
        return codeBasePrefix + "_" + " " + id;//add a space for later
    }
    return id;
}

lString16 ldomDocumentFragmentWriter::convertHref( lString16 href )
{
    if ( href.pos("://")>=0 )
        return href; // fully qualified href: no conversion

    //CRLog::trace("convertHref(%s, codeBase=%s, filePathName=%s)", LCSTR(href), LCSTR(codeBase), LCSTR(filePathName));

    if (href[0] == '#') {
        // Link to anchor in the same docFragment
        lString16 replacement = pathSubstitutions.get(filePathName);
        if (replacement.empty())
            return href;
        lString16 p = cs16("#") + replacement + "_" + " " + href.substr(1);
        //CRLog::trace("href %s -> %s", LCSTR(href), LCSTR(p));
        return p;
    }

    // href = LVCombinePaths(codeBase, href);

    // Depending on what's calling us, href may or may not have
    // gone thru DecodeHTMLUrlString() to decode %-encoded bits.
    // We'll need to try again with DecodeHTMLUrlString() if not
    // initially found in "pathSubstitutions" (whose filenames went
    // thru DecodeHTMLUrlString(), and so did 'codeBase').

    // resolve relative links
    lString16 p, id; // path, id
    if ( !href.split2(cs16("#"), p, id) )
        p = href;
    if ( p.empty() ) {
        //CRLog::trace("codebase = %s -> href = %s", LCSTR(codeBase), LCSTR(href));
        if ( codeBasePrefix.empty() )
            return LVCombinePaths(codeBase, href);
        p = codeBasePrefix;
    }
    else {
        lString16 replacement = pathSubstitutions.get(LVCombinePaths(codeBase, p));
        //CRLog::trace("href %s -> %s", LCSTR(p), LCSTR(replacement));
        if ( !replacement.empty() )
            p = replacement;
        else {
            // Try again after DecodeHTMLUrlString()
            p = DecodeHTMLUrlString(p);
            replacement = pathSubstitutions.get(LVCombinePaths(codeBase, p));
            if ( !replacement.empty() )
                p = replacement;
            else
                return LVCombinePaths(codeBase, href);
        }
        //else
        //    p = codeBasePrefix;
        //p = LVCombinePaths( codeBase, p ); // relative to absolute path
    }
    if ( !id.empty() )
        p = p + "_" + " " + id;

    p = cs16("#") + p;

    //CRLog::debug("converted href=%s to %s", LCSTR(href), LCSTR(p) );

    return p;
}

void ldomDocumentFragmentWriter::setCodeBase( lString16 fileName )
{
    filePathName = fileName;
    codeBasePrefix = pathSubstitutions.get(fileName);
    codeBase = LVExtractPath(filePathName);
    if ( codeBasePrefix.empty() ) {
        CRLog::trace("codeBasePrefix is empty for path %s", LCSTR(fileName));
        codeBasePrefix = pathSubstitutions.get(fileName);
    }
    stylesheetFile.clear();
}

/// called on attribute
void ldomDocumentFragmentWriter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    if ( insideTag ) {
        if ( !lStr_cmp(attrname, "href") || !lStr_cmp(attrname, "src") ) {
            parent->OnAttribute(nsname, attrname, convertHref(lString16(attrvalue)).c_str() );
        } else if ( !lStr_cmp(attrname, "id") ) {
            parent->OnAttribute(nsname, attrname, convertId(lString16(attrvalue)).c_str() );
        } else if ( !lStr_cmp(attrname, "name") ) {
            //CRLog::trace("name attribute = %s", LCSTR(lString16(attrvalue)));
            parent->OnAttribute(nsname, attrname, convertId(lString16(attrvalue)).c_str() );
        } else {
            parent->OnAttribute(nsname, attrname, attrvalue);
        }
    } else {
        if (insideHtmlTag) {
            // Grab attributes from <html dir="rtl" lang="he"> (not included in the DOM)
            // to reinject them in <DocFragment>
            if ( !lStr_cmp(attrname, "dir") )
                htmlDir = attrvalue;
            else if ( !lStr_cmp(attrname, "lang") )
                htmlLang = attrvalue;
        }
        else if ( styleDetectionState ) {
            if ( !lStr_cmp(attrname, "rel") && lString16(attrvalue).lowercase() == L"stylesheet" )
                styleDetectionState |= 2;
            else if ( !lStr_cmp(attrname, "type") ) {
                if ( lString16(attrvalue).lowercase() == L"text/css")
                    styleDetectionState |= 4;
                else
                    styleDetectionState = 0;  // text/css type supported only
            } else if ( !lStr_cmp(attrname, "href") ) {
                styleDetectionState |= 8;
                lString16 href = attrvalue;
                if ( stylesheetFile.empty() )
                    tmpStylesheetFile = LVCombinePaths( codeBase, href );
                else
                    tmpStylesheetFile = href;
            }
            if (styleDetectionState == 15) {
                if ( !stylesheetFile.empty() )
                    stylesheetLinks.add(tmpStylesheetFile);
                else
                    stylesheetFile = tmpStylesheetFile;
                styleDetectionState = 0;
                CRLog::trace("CSS file href: %s", LCSTR(stylesheetFile));
            }
        }
    }
}

/// called on opening tag
ldomNode * ldomDocumentFragmentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    if ( insideTag ) {
        return parent->OnTagOpen(nsname, tagname);
    } else {
        if ( !lStr_cmp(tagname, "link") )
            styleDetectionState = 1;
        else if ( !lStr_cmp(tagname, "style") )
            headStyleState = 1;
        else if ( !lStr_cmp(tagname, "html") ) {
            insideHtmlTag = true;
            htmlDir.clear();
            htmlLang.clear();
        }
    }

    // When meeting the <body> of each of an EPUB's embedded HTML files,
    // we will insert into parent (the ldomDocumentWriter that makes out a single
    // document) a <DocFragment> wrapping that <body>. It may end up as:
    //
    //   <DocFragment StyleSheet="OEBPS/Styles/main.css" id="_doc_fragment_2">
    //     <stylesheet href="OEBPS/Text/">
    //       @import url("../Styles/other.css");
    //       @import url(path_to_3rd_css_file)
    //       here is <HEAD><STYLE> content
    //     </stylesheet>
    //     <body>
    //       here is original <BODY> content
    //     </body>
    //   </DocFragment>
    //
    // (Why one css file link in an attribute and others in the tag?
    // I suppose it's because attribute values are hashed and stored only
    // once, so it saves space in the DOM/cache for documents with many
    // fragments and a single CSS link, which is the most usual case.)

    if ( !insideTag && baseTag==tagname ) { // with EPUBs: baseTag="body"
        insideTag = true;
        if ( !baseTagReplacement.empty() ) { // with EPUBs: baseTagReplacement="DocFragment"
            baseElement = parent->OnTagOpen(L"", baseTagReplacement.c_str()); // start <DocFragment
            lastBaseElement = baseElement;
            if ( !stylesheetFile.empty() ) {
                // add attribute <DocFragment StyleSheet="path_to_css_1st_file"
                parent->OnAttribute(L"", L"StyleSheet", stylesheetFile.c_str() );
                CRLog::debug("Setting StyleSheet attribute to %s for document fragment", LCSTR(stylesheetFile) );
            }
            if ( !codeBasePrefix.empty() ) // add attribute <DocFragment id="..html_file_name"
                parent->OnAttribute(L"", L"id", codeBasePrefix.c_str() );
            if ( !htmlDir.empty() ) // add attribute <DocFragment dir="rtl" from <html dir="rtl"> tag
                parent->OnAttribute(L"", L"dir", htmlDir.c_str() );
            if ( !htmlLang.empty() ) // add attribute <DocFragment lang="ar" from <html lang="ar"> tag
                parent->OnAttribute(L"", L"lang", htmlLang.c_str() );

            parent->OnTagBody(); // inside <DocFragment>
            if ( !headStyleText.empty() || stylesheetLinks.length() > 0 ) {
                // add stylesheet element as child of <DocFragment>: <stylesheet href="...">
                parent->OnTagOpen(L"", L"stylesheet");
                parent->OnAttribute(L"", L"href", codeBase.c_str() );
                lString16 imports;
                for (int i = 0; i < stylesheetLinks.length(); i++) {
                    lString16 import("@import url(\"");
                    import << stylesheetLinks.at(i);
                    import << "\");\n";
                    imports << import;
                }
                stylesheetLinks.clear();
                lString16 styleText = imports + headStyleText.c_str();
                // Add it to <DocFragment><stylesheet>, so it becomes:
                //   <stylesheet href="...">
                //     @import url(path_to_css_2nd_file)
                //     @import url(path_to_css_3rd_file)
                //     here is <HEAD><STYLE> content
                //   </stylesheet>
                parent->OnTagBody();
                parent->OnText(styleText.c_str(), styleText.length(), 0);
                parent->OnTagClose(L"", L"stylesheet");
                // done with <DocFragment><stylesheet>...</stylesheet>
            }
            // Finally, create <body> and go on.
            // The styles we have just set via <stylesheet> element and
            // StyleSheet= attribute will be applied by this OnTagOpen("body")
            // (including those that may apply to body itself), push()'ing
            // the previous stylesheet state, that will be pop()'ed when the
            // ldomElementWriter for DocFragment is left/destroyed (by onBodyExit(),
            // because this OnTagOpen has set to it _stylesheetIsSet).
            parent->OnTagOpen(L"", baseTag.c_str());
            parent->OnTagBody();
            return baseElement;
        }
    }
    return NULL;
}

/// called on closing tag
void ldomDocumentFragmentWriter::OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
{
    styleDetectionState = headStyleState = 0;
    if ( insideTag && baseTag==tagname ) {
        insideTag = false;
        if ( !baseTagReplacement.empty() ) {
            parent->OnTagClose(L"", baseTag.c_str());
            parent->OnTagClose(L"", baseTagReplacement.c_str());
        }
        baseElement = NULL;
        return;
    }
    if ( insideTag )
        parent->OnTagClose(nsname, tagname);
}

/// called after > of opening tag (when entering tag body) or just before /> closing tag for empty tags
void ldomDocumentFragmentWriter::OnTagBody()
{
    if ( insideTag ) {
        parent->OnTagBody();
    }
    else if ( insideHtmlTag ) {
        insideHtmlTag = false;
    }
    if ( styleDetectionState == 11 ) {
        // incomplete <link rel="stylesheet", href="..." />; assuming type="text/css"
        if ( !stylesheetFile.empty() )
            stylesheetLinks.add(tmpStylesheetFile);
        else
            stylesheetFile = tmpStylesheetFile;
        styleDetectionState = 0;
    } else
        styleDetectionState = 0;
}



/////////////////////////////////////////////////////////////////
/// ldomDocumentWriterFilter
// Used to parse loosy HTML in formats: HTML, CHM, PDB(html)
// For all these document formats, it is fed by HTMLParser that does
// convert to lowercase the tag names and attributes.
// ldomDocumentWriterFilter does then deal with auto-closing unbalanced
// HTML tags according to the rules set in crengine/src/lvxml.cpp HTML_AUTOCLOSE_TABLE[]

/** \brief callback object to fill DOM tree

    To be used with XML parser as callback object.

    Creates document according to incoming events.

    Autoclose HTML tags.
*/

void ldomDocumentWriterFilter::setClass( const lChar16 * className, bool overrideExisting )
{
    ldomNode * node = _currNode->_element;
    if ( _classAttrId==0 ) {
        _classAttrId = _document->getAttrNameIndex(L"class");
    }
    if ( overrideExisting || !node->hasAttribute(_classAttrId) ) {
        node->setAttributeValue(LXML_NS_NONE, _classAttrId, className);
    }
}

void ldomDocumentWriterFilter::appendStyle( const lChar16 * style )
{
    ldomNode * node = _currNode->_element;
    if ( _styleAttrId==0 ) {
        _styleAttrId = _document->getAttrNameIndex(L"style");
    }
    // Append to the style attribute even if embedded styles are disabled
    // at loading time, otherwise it won't be there if we enable them later
    // if (!_document->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES))
    //     return; // disabled

    lString16 oldStyle = node->getAttributeValue(_styleAttrId);
    if ( !oldStyle.empty() && oldStyle.at(oldStyle.length()-1)!=';' )
        oldStyle << "; ";
    oldStyle << style;
    node->setAttributeValue(LXML_NS_NONE, _styleAttrId, oldStyle.c_str());
}

void ldomDocumentWriterFilter::AutoClose( lUInt16 tag_id, bool open )
{
    lUInt16 * rule = _rules[tag_id];
    if ( !rule )
        return;
    if ( open ) {
        ldomElementWriter * found = NULL;
        ldomElementWriter * p = _currNode;
        while ( p && !found ) {
            lUInt16 id = p->_element->getNodeId();
            for ( int i=0; rule[i]; i++ ) {
                if ( rule[i]==id ) {
                    found = p;
                    break;
                }
            }
            p = p->_parent;
        }
        // found auto-close target
        if ( found != NULL ) {
            bool done = false;
            while ( !done && _currNode ) {
                if ( _currNode == found )
                    done = true;
                ldomNode * closedElement = _currNode->getElement();
                _currNode = pop( _currNode, closedElement->getNodeId() );
                //ElementCloseHandler( closedElement );
            }
        }
    } else {
        if ( !rule[0] )
            _currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
    }
}

ldomNode * ldomDocumentWriterFilter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    //CRLog::trace("OnTagOpen(%s, %s)", LCSTR(lString16(nsname)), LCSTR(lString16(tagname)));
    if ( !_tagBodyCalled ) {
        CRLog::error("OnTagOpen w/o parent's OnTagBody : %s", LCSTR(lString16(tagname)));
        crFatalError();
    }
    _tagBodyCalled = false;
    //logfile << "lxmlDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
//    if ( nsname && nsname[0] )
//        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
//    lStr_lowercase( const_cast<lChar16 *>(tagname), lStr_len(tagname) );

    // Set a flag for OnText to accumulate the content of any <HEAD><STYLE>
    if ( tagname[0] == 's' && !lStr_cmp(tagname, "style") && _currNode && _currNode->getElement()->isNodeName("head") ) {
        _inHeadStyle = true;
    }

    // Fixed 20180503: this was done previously in any case, but now only
    // if _libRuDocumentDetected. We still allow the old behaviour if
    // requested to keep previously recorded XPATHs valid.
    if ( _libRuDocumentDetected || gDOMVersionRequested < 20180503) {
        // Patch for bad LIB.RU books - BR delimited paragraphs in "Fine HTML" format
        if ((tagname[0] == 'b' && tagname[1] == 'r' && tagname[2] == 0)
            || (tagname[0] == 'd' && tagname[1] == 'd' && tagname[2] == 0)) {
            // substitute to P
            tagname = L"p";
            _libRuParagraphStart = true; // to trim leading &nbsp;
        } else {
            _libRuParagraphStart = false;
        }
    }

    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    AutoClose( id, true );
    _currNode = new ldomElementWriter( _document, nsid, id, _currNode );
    _flags = _currNode->getFlags();
    if ( _libRuDocumentDetected && (_flags & TXTFLG_PRE) )
        _flags |= TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM; // convert preformatted text into paragraphs
    //logfile << " !o!\n";
    //return _currNode->getElement();
    return _currNode->getElement();
}

/// called after > of opening tag (when entering tag body)
// Note to avoid confusion: all tags HAVE a body (their content), so this
// is called on all tags.
void ldomDocumentWriterFilter::OnTagBody()
{
    _tagBodyCalled = true;

    // Some specific handling for the <BODY> tag to deal with HEAD STYLE and
    // LINK is done in super class (ldomDocumentWriter)
    ldomDocumentWriter::OnTagBody();
}

bool isRightAligned(ldomNode * node) {
    lString16 style = node->getAttributeValue(attr_style);
    if (style.empty())
        return false;
    int p = style.pos("text-align: right", 0);
    return (p >= 0);
}

void ldomDocumentWriterFilter::ElementCloseHandler( ldomNode * node )
{
    ldomNode * parent = node->getParentNode();
    lUInt16 id = node->getNodeId();
    if ( parent ) {
        if ( parent->getLastChild() != node )
            return;
        if ( id==el_table ) {
            if (isRightAligned(node) && node->getAttributeValue(attr_width) == "30%") {
                // LIB.RU TOC detected: remove it
                //parent = parent->modify();

                //parent->removeLastChild();
            }
        } else if ( id==el_pre && _libRuDocumentDetected ) {
            // for LIB.ru - replace PRE element with DIV (section?)
            if ( node->getChildCount()==0 ) {
                //parent = parent->modify();

                //parent->removeLastChild(); // remove empty PRE element
            }
            //else if ( node->getLastChild()->getNodeId()==el_div && node->getLastChild()->getChildCount() &&
            //          ((ldomElement*)node->getLastChild())->getLastChild()->getNodeId()==el_form )
            //    parent->removeLastChild(); // remove lib.ru final section
            else
                node->setNodeId( el_div );
        } else if ( id==el_div ) {
//            CRLog::trace("DIV attr align = %s", LCSTR(node->getAttributeValue(attr_align)));
//            CRLog::trace("DIV attr count = %d", node->getAttrCount());
//            int alignId = node->getDocument()->getAttrNameIndex("align");
//            CRLog::trace("align= %d %d", alignId, attr_align);
//            for (int i = 0; i < node->getAttrCount(); i++)
//                CRLog::trace("DIV attr %s", LCSTR(node->getAttributeName(i)));
            if (isRightAligned(node)) {
                ldomNode * child = node->getLastChild();
                if ( child && child->getNodeId()==el_form )  {
                    // LIB.RU form detected: remove it
                    //parent = parent->modify();

                    parent->removeLastChild();
                    _libRuDocumentDetected = true;
                }
            }
        }
    }
    if (!_libRuDocumentDetected)
        node->persist();
}

void ldomDocumentWriterFilter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    //logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
    //if ( nsname && nsname[0] )
    //    lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
    //lStr_lowercase( const_cast<lChar16 *>(attrname), lStr_len(attrname) );

    //CRLog::trace("OnAttribute(%s, %s)", LCSTR(lString16(attrname)), LCSTR(lString16(attrvalue)));

    // ldomDocumentWriterFilter is used for HTML/CHM/PDB (not with EPUBs).
    // We translate some attributes (now possibly deprecated) to their
    // CSS style equivalent, globally or for some elements only.
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes
    lUInt16 id = _currNode->_element->getNodeId();

    // Not sure this is to be done here: we get attributes as they are read,
    // so possibly before or after a style=, that the attribute may override.
    // Hopefully, a document use either one or the other.
    // (Alternative: in lvrend.cpp when used, as fallback when there is
    // none specified in node->getStyle().)

    // HTML align= => CSS text-align:
    // Done for all elements, except IMG and TABLE (for those, it should
    // translate to float:left/right, which is ensured by epub.css)
    // Should this be restricted to some specific elements?
    if ( !lStr_cmp(attrname, "align") && (id != el_img) && (id != el_table) ) {
        lString16 align = lString16(attrvalue).lowercase();
        if ( align == L"justify")
            appendStyle( L"text-align: justify" );
        else if ( align == L"left")
            appendStyle( L"text-align: left" );
        else if ( align == L"right")
            appendStyle( L"text-align: right" );
        else if ( align == L"center")
            appendStyle( L"text-align: center" );
       return;
    }

    // For the table & friends elements where we do support the following styles,
    // we translate these deprecated attributes to their style equivalents:
    //
    // HTML valign= => CSS vertical-align: only for TH & TD (as lvrend.cpp
    // only uses it with erm_table_cell)
    if (id == el_th || id == el_td) {
        // Default rendering for cells is valign=top
        // There is no support for valign=baseline.
        if ( !lStr_cmp(attrname, "valign") ) {
            lString16 valign = lString16(attrvalue).lowercase();
            if ( valign == L"middle" )
                appendStyle( L"vertical-align: middle" );
            else if ( valign == L"bottom")
                appendStyle( L"vertical-align: bottom" );
           return;
        }
    }
    // HTML width= => CSS width: only for TH, TD and COL (as lvrend.cpp
    // only uses it with erm_table_cell and erm_table_column)
    // Note: with IMG, lvtextfm LFormattedText::AddSourceObject() only uses
    // style, and not attributes: <img width=100 height=50> would not be used.
    if (id == el_th || id == el_td || id == el_col) {
        if ( !lStr_cmp(attrname, "width") ) {
            lString16 val = lString16(attrvalue);
            const wchar_t * s = val.c_str();
            bool is_pct = false;
            int n=0;
            if (s && s[0]) {
                for (int i=0; s[i]; i++) {
                    if (s[i]>='0' && s[i]<='9') {
                        n = n*10 + (s[i]-'0');
                    } else if (s[i] == '%') {
                        is_pct = true;
                        break;
                    }
                }
                if (n > 0) {
                    val = lString16("width: ");
                    val.appendDecimal(n);
                    val += is_pct ? "%" : "px"; // CSS pixels
                    appendStyle(val.c_str());
                }
            }
            return;
        }
    }

    // Othewise, add the attribute
    lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex( nsname ) : 0;
    lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex( attrname ) : 0;

    _currNode->addAttribute( attr_ns, attr_id, attrvalue );

    //logfile << " !a!\n";
}

/// called on closing tag
void ldomDocumentWriterFilter::OnTagClose( const lChar16 * /*nsname*/, const lChar16 * tagname )
{
    if ( !_tagBodyCalled ) {
        CRLog::error("OnTagClose w/o parent's OnTagBody : %s", LCSTR(lString16(tagname)));
        crFatalError();
    }
    //logfile << "ldomDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
//    if ( nsname && nsname[0] )
//        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
//    lStr_lowercase( const_cast<lChar16 *>(tagname), lStr_len(tagname) );
    if (!_currNode)
    {
        _errFlag = true;
        //logfile << " !c-err!\n";
        return;
    }

    // Parse <link rel="stylesheet">, put the css file link in _stylesheetLinks,
    // they will be added to <body><stylesheet> when we meet <BODY>
    // (duplicated in ldomDocumentWriter::OnTagClose)
    if (tagname[0] == 'l' && _currNode && !lStr_cmp(tagname, "link")) {
        // link node
        if ( _currNode && _currNode->getElement() && _currNode->getElement()->isNodeName("link") &&
             _currNode->getElement()->getParentNode() && _currNode->getElement()->getParentNode()->isNodeName("head") &&
             lString16(_currNode->getElement()->getAttributeValue("rel")).lowercase() == L"stylesheet" &&
             lString16(_currNode->getElement()->getAttributeValue("type")).lowercase() == L"text/css" ) {
            lString16 href = _currNode->getElement()->getAttributeValue("href");
            lString16 stylesheetFile = LVCombinePaths( _document->getCodeBase(), href );
            CRLog::debug("Internal stylesheet file: %s", LCSTR(stylesheetFile));
            // We no more apply it immediately: it will be when <BODY> is met
            // _document->setDocStylesheetFileName(stylesheetFile);
            // _document->applyDocumentStyleSheet();
            _stylesheetLinks.add(stylesheetFile);
        }
    }

    lUInt16 id = _document->getElementNameIndex(tagname);

    // HTML title detection
    if ( id==el_title && _currNode && _currNode->_element && _currNode->_element->getParentNode() != NULL
                                   && _currNode->_element->getParentNode()->getNodeId() == el_head ) {
        lString16 s = _currNode->_element->getText();
        s.trim();
        if ( !s.empty() ) {
            // TODO: split authors, title & series
            _document->getProps()->setString( DOC_PROP_TITLE, s );
        }
    }
    //======== START FILTER CODE ============
    AutoClose( _currNode->_element->getNodeId(), false );
    //======== END FILTER CODE ==============
    //lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    // save closed element
    ldomNode * closedElement = _currNode->getElement();
    _errFlag |= (id != closedElement->getNodeId());
    _currNode = pop( _currNode, id );


    if ( _currNode ) {
        _flags = _currNode->getFlags();
        if ( _libRuDocumentDetected && (_flags & TXTFLG_PRE) )
            _flags |= TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM; // convert preformatted text into paragraphs
    }

    //=============================================================
    // LIB.RU patch: remove table of contents
    //ElementCloseHandler( closedElement );
    //=============================================================

    if ( id==_stopTagId ) {
        //CRLog::trace("stop tag found, stopping...");
        _parser->Stop();
    }
    //logfile << " !c!\n";
}

/// called on text
void ldomDocumentWriterFilter::OnText( const lChar16 * text, int len, lUInt32 flags )
{
    // Accumulate <HEAD><STYLE> content
    if (_inHeadStyle) {
        _headStyleText << lString16(text, len);
        _inHeadStyle = false;
        return;
    }

    //logfile << "lxmlDocumentWriter::OnText() fpos=" << fpos;
    if (_currNode)
    {
        AutoClose( _currNode->_element->getNodeId(), false );
        if ( (_flags & XML_FLAG_NO_SPACE_TEXT)
             && IsEmptySpace(text, len) && !(flags & TXTFLG_PRE))
             return;
        bool autoPara = _libRuDocumentDetected && (flags & TXTFLG_PRE);
        if (_currNode->_allowText) {
            if ( _libRuParagraphStart ) {
                bool cleaned = false;
                while ( *text==160 && len > 0 ) {
                    cleaned = true;
                    text++;
                    len--;
                    while ( *text==' ' && len > 0 ) {
                        text++;
                        len--;
                    }
                }
                if ( cleaned ) {
                    setClass(L"justindent");
                    //appendStyle(L"text-indent: 1.3em; text-align: justify");
                }
                _libRuParagraphStart = false;
            }
            int leftSpace = 0;
            const lChar16 * paraTag = NULL;
            bool isHr = false;
            if ( autoPara ) {
                while ( (*text==' ' || *text=='\t' || *text==160) && len > 0 ) {
                    text++;
                    len--;
                    leftSpace += (*text == '\t') ? 8 : 1;
                }
                paraTag = leftSpace > 8 ? L"h2" : L"p";
                lChar16 ch = 0;
                bool sameCh = true;
                for ( int i=0; i<len; i++ ) {
                    if ( !ch )
                        ch = text[i];
                    else if ( ch != text[i] ) {
                        sameCh = false;
                        break;
                    }
                }
                if ( !ch )
                    sameCh = false;
                if ( (ch=='-' || ch=='=' || ch=='_' || ch=='*' || ch=='#') && sameCh )
                    isHr = true;
            }
            if ( isHr ) {
                OnTagOpen( NULL, L"hr" );
                OnTagBody();
                OnTagClose( NULL, L"hr" );
            } else if ( len > 0 ) {
                if ( autoPara ) {
                    OnTagOpen( NULL, paraTag );
                    OnTagBody();
                }
                _currNode->onText( text, len, flags );
                if ( autoPara )
                    OnTagClose( NULL, paraTag );
            }
        }
    }
    //logfile << " !t!\n";
}

ldomDocumentWriterFilter::ldomDocumentWriterFilter(ldomDocument * document, bool headerOnly, const char *** rules )
: ldomDocumentWriter( document, headerOnly )
, _libRuDocumentDetected(false)
, _libRuParagraphStart(false)
, _styleAttrId(0)
, _classAttrId(0)
, _tagBodyCalled(true)
{
    lUInt16 i;
    for ( i=0; i<MAX_ELEMENT_TYPE_ID; i++ )
        _rules[i] = NULL;
    lUInt16 items[MAX_ELEMENT_TYPE_ID];
    for ( i=0; rules[i]; i++ ) {
        const char ** rule = rules[i];
        lUInt16 j;
        for ( j=0; rule[j] && j<MAX_ELEMENT_TYPE_ID; j++ ) {
            const char * s = rule[j];
            items[j] = _document->getElementNameIndex( lString16(s).c_str() );
        }
        if ( j>=1 ) {
            lUInt16 id = items[0];
            _rules[ id ] = new lUInt16[j];
            for ( int k=0; k<j; k++ ) {
                _rules[id][k] = k==j-1 ? 0 : items[k+1];
            }
        }
    }
}

ldomDocumentWriterFilter::~ldomDocumentWriterFilter()
{

    for ( int i=0; i<MAX_ELEMENT_TYPE_ID; i++ ) {
        if ( _rules[i] )
            delete[] _rules[i];
    }
}

#if BUILD_LITE!=1
static const char * doc_file_magic = "CR3\n";


bool lxmlDocBase::DocFileHeader::serialize( SerialBuf & hdrbuf )
{
    int start = hdrbuf.pos();
    hdrbuf.putMagic( doc_file_magic );
    //CRLog::trace("Serializing render data: %d %d %d %d", render_dx, render_dy, render_docflags, render_style_hash);
    hdrbuf << render_dx << render_dy << render_docflags << render_style_hash << stylesheet_hash << node_displaystyle_hash;

    hdrbuf.putCRC( hdrbuf.pos() - start );

#if 0
    {
        lString8 s;
        s<<"SERIALIZED HDR BUF: ";
        for ( int i=0; i<hdrbuf.pos(); i++ ) {
            char tmp[20];
            sprintf(tmp, "%02x ", hdrbuf.buf()[i]);
            s<<tmp;
        }
        CRLog::trace(s.c_str());
    }
#endif
    return !hdrbuf.error();
}

bool lxmlDocBase::DocFileHeader::deserialize( SerialBuf & hdrbuf )
{
    int start = hdrbuf.pos();
    hdrbuf.checkMagic( doc_file_magic );
    if ( hdrbuf.error() ) {
        CRLog::error("Swap file Magic signature doesn't match");
        return false;
    }
    hdrbuf >> render_dx >> render_dy >> render_docflags >> render_style_hash >> stylesheet_hash >> node_displaystyle_hash;
    //CRLog::trace("Deserialized render data: %d %d %d %d", render_dx, render_dy, render_docflags, render_style_hash);
    hdrbuf.checkCRC( hdrbuf.pos() - start );
    if ( hdrbuf.error() ) {
        CRLog::error("Swap file - header unpack error");
        return false;
    }
    return true;
}
#endif

void tinyNodeCollection::setDocFlag( lUInt32 mask, bool value )
{
    CRLog::debug("setDocFlag(%04x, %s)", mask, value?"true":"false");
    if ( value )
        _docFlags |= mask;
    else
        _docFlags &= ~mask;
}

void tinyNodeCollection::setDocFlags( lUInt32 value )
{
    CRLog::debug("setDocFlags(%04x)", value);
    _docFlags = value;
}

int tinyNodeCollection::getPersistenceFlags()
{
    int format = 2; //getProps()->getIntDef(DOC_PROP_FILE_FORMAT, 0);
    int flag = ( format==2 && getDocFlag(DOC_FLAG_PREFORMATTED_TEXT) ) ? 1 : 0;
    CRLog::trace("getPersistenceFlags() returned %d", flag);
    return flag;
}

void ldomDocument::clear()
{
#if BUILD_LITE!=1
    clearRendBlockCache();
    _rendered = false;
    _urlImageMap.clear();
    _fontList.clear();
    fontMan->UnregisterDocumentFonts(_docIndex);
#endif
    //TODO: implement clear
    //_elemStorage.
}

#if BUILD_LITE!=1
bool ldomDocument::openFromCache( CacheLoadingCallback * formatCallback, LVDocViewCallback * progressCallback )
{
    setCacheFileStale(true);
    if ( !openCacheFile() ) {
        CRLog::info("Cannot open document from cache. Need to read fully");
        clear();
        return false;
    }
    if ( !loadCacheFileContent(formatCallback, progressCallback) ) {
        CRLog::info("Error while loading document content from cache file.");
        clear();
        return false;
    }
#if 0
    LVStreamRef s = LVOpenFileStream("/tmp/test.xml", LVOM_WRITE);
    if ( !s.isNull() )
        saveToStream(s, "UTF8");
#endif
    _mapped = true;
    _rendered = true;
    _just_rendered_from_cache = true;
    _toc_from_cache_valid = true;
    // Use cached node_displaystyle_hash as _nodeDisplayStyleHashInitial, as it
    // should be in sync with the DOM stored in the cache
    _nodeDisplayStyleHashInitial = _hdr.node_displaystyle_hash;
    CRLog::info("Initializing _nodeDisplayStyleHashInitial from cache file: %x", _nodeDisplayStyleHashInitial);

    setCacheFileStale(false);
    return true;
}

/// load document cache file content, @see saveChanges()
bool ldomDocument::loadCacheFileContent(CacheLoadingCallback * formatCallback, LVDocViewCallback * progressCallback)
{

    CRLog::trace("ldomDocument::loadCacheFileContent()");
    {
        if (progressCallback) progressCallback->OnLoadFileProgress(5);
        SerialBuf propsbuf(0, true);
        if ( !_cacheFile->read( CBT_PROP_DATA, propsbuf ) ) {
            CRLog::error("Error while reading props data");
            return false;
        }
        getProps()->deserialize( propsbuf );
        if ( propsbuf.error() ) {
            CRLog::error("Cannot decode property table for document");
            return false;
        }

        if ( formatCallback ) {
            int fmt = getProps()->getIntDef(DOC_PROP_FILE_FORMAT_ID,
                    doc_format_fb2);
            if (fmt < doc_format_fb2 || fmt > doc_format_max)
                fmt = doc_format_fb2;
            // notify about format detection, to allow setting format-specific CSS
            formatCallback->OnCacheFileFormatDetected((doc_format_t)fmt);
        }

        if (progressCallback) progressCallback->OnLoadFileProgress(10);
        CRLog::trace("ldomDocument::loadCacheFileContent() - ID data");
        SerialBuf idbuf(0, true);
        if ( !_cacheFile->read( CBT_MAPS_DATA, idbuf ) ) {
            CRLog::error("Error while reading Id data");
            return false;
        }
        deserializeMaps( idbuf );
        if ( idbuf.error() ) {
            CRLog::error("Cannot decode ID table for document");
            return false;
        }

        if (progressCallback) progressCallback->OnLoadFileProgress(15);
        CRLog::trace("ldomDocument::loadCacheFileContent() - page data");
        SerialBuf pagebuf(0, true);
        if ( !_cacheFile->read( CBT_PAGE_DATA, pagebuf ) ) {
            CRLog::error("Error while reading pages data");
            return false;
        }
        pagebuf.swap( _pagesData );
        _pagesData.setPos( 0 );
        LVRendPageList pages;
        pages.deserialize(_pagesData);
        if ( _pagesData.error() ) {
            CRLog::error("Page data deserialization is failed");
            return false;
        }
        CRLog::info("%d pages read from cache file", pages.length());
        //_pagesData.setPos( 0 );

        if (progressCallback) progressCallback->OnLoadFileProgress(20);
        CRLog::trace("ldomDocument::loadCacheFileContent() - embedded font data");
        {
            SerialBuf buf(0, true);
            if ( !_cacheFile->read(CBT_FONT_DATA, buf)) {
                CRLog::error("Error while reading font data");
                return false;
            }
            if (!_fontList.deserialize(buf)) {
                CRLog::error("Error while parsing font data");
                return false;
            }
            registerEmbeddedFonts();
        }

        if (progressCallback) progressCallback->OnLoadFileProgress(25);
        DocFileHeader h = {};
        SerialBuf hdrbuf(0,true);
        if ( !_cacheFile->read( CBT_REND_PARAMS, hdrbuf ) ) {
            CRLog::error("Error while reading header data");
            return false;
        } else if ( !h.deserialize(hdrbuf) ) {
            CRLog::error("Header data deserialization is failed");
            return false;
        }
        _hdr = h;
        CRLog::info("Loaded render properties: styleHash=%x, stylesheetHash=%x, docflags=%x, width=%x, height=%x, nodeDisplayStyleHash=%x",
                _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy, _hdr.node_displaystyle_hash);
    }

    if (progressCallback) progressCallback->OnLoadFileProgress(30);
    CRLog::trace("ldomDocument::loadCacheFileContent() - node data");
    if ( !loadNodeData() ) {
        CRLog::error("Error while reading node instance data");
        return false;
    }

    if (progressCallback) progressCallback->OnLoadFileProgress(40);
    CRLog::trace("ldomDocument::loadCacheFileContent() - element storage");
    if ( !_elemStorage.load() ) {
        CRLog::error("Error while loading element data");
        return false;
    }
    if (progressCallback) progressCallback->OnLoadFileProgress(50);
    CRLog::trace("ldomDocument::loadCacheFileContent() - text storage");
    if ( !_textStorage.load() ) {
        CRLog::error("Error while loading text data");
        return false;
    }
    if (progressCallback) progressCallback->OnLoadFileProgress(60);
    CRLog::trace("ldomDocument::loadCacheFileContent() - rect storage");
    if ( !_rectStorage.load() ) {
        CRLog::error("Error while loading rect data");
        return false;
    }
    if (progressCallback) progressCallback->OnLoadFileProgress(70);
    CRLog::trace("ldomDocument::loadCacheFileContent() - node style storage");
    if ( !_styleStorage.load() ) {
        CRLog::error("Error while loading node style data");
        return false;
    }

    if (progressCallback) progressCallback->OnLoadFileProgress(80);
    CRLog::trace("ldomDocument::loadCacheFileContent() - TOC");
    {
        SerialBuf tocbuf(0,true);
        if ( !_cacheFile->read( CBT_TOC_DATA, tocbuf ) ) {
            CRLog::error("Error while reading TOC data");
            return false;
        } else if ( !m_toc.deserialize(this, tocbuf) ) {
            CRLog::error("TOC data deserialization is failed");
            return false;
        }
    }
    if (progressCallback) progressCallback->OnLoadFileProgress(85);
    CRLog::trace("ldomDocument::loadCacheFileContent() - PageMap");
    {
        SerialBuf pagemapbuf(0,true);
        if ( !_cacheFile->read( CBT_PAGEMAP_DATA, pagemapbuf ) ) {
            CRLog::error("Error while reading PageMap data");
            return false;
        } else if ( !m_pagemap.deserialize(this, pagemapbuf) ) {
            CRLog::error("PageMap data deserialization is failed");
            return false;
        }
    }


    if (progressCallback) progressCallback->OnLoadFileProgress(90);
    if ( loadStylesData() ) {
        CRLog::trace("ldomDocument::loadCacheFileContent() - using loaded styles");
        updateLoadedStyles( true );
//        lUInt32 styleHash = calcStyleHash();
//        styleHash = styleHash * 31 + calcGlobalSettingsHash();
//        CRLog::debug("Loaded style hash: %x", styleHash);
//        lUInt32 styleHash = calcStyleHash();
//        CRLog::info("Loaded style hash = %08x", styleHash);
    } else {
        CRLog::trace("ldomDocument::loadCacheFileContent() - style loading failed: will reinit ");
        updateLoadedStyles( false );
    }

    CRLog::trace("ldomDocument::loadCacheFileContent() - completed successfully");
    if (progressCallback) progressCallback->OnLoadFileProgress(95);

    return true;
}

static const char * styles_magic = "CRSTYLES";

#define CHECK_EXPIRATION(s) \
    if ( maxTime.expired() ) { CRLog::info("timer expired while " s); return CR_TIMEOUT; }

/// saves changes to cache file, limited by time interval (can be called again to continue after TIMEOUT)
ContinuousOperationResult ldomDocument::saveChanges( CRTimerUtil & maxTime, LVDocViewCallback * progressCallback )
{
    if ( !_cacheFile )
        return CR_DONE;

    if (progressCallback) progressCallback->OnSaveCacheFileStart();

    if (maxTime.infinite()) {
        _mapSavingStage = 0; // all stages from the beginning
        _cacheFile->setAutoSyncSize(0);
    } else {
        //CRLog::trace("setting autosync");
        _cacheFile->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);
        //CRLog::trace("setting autosync - done");
    }

    CRLog::trace("ldomDocument::saveChanges(timeout=%d stage=%d)", maxTime.interval(), _mapSavingStage);
    setCacheFileStale(true);

    switch (_mapSavingStage) {
    default:
    case 0:

        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime);
        CHECK_EXPIRATION("flushing of stream")

        persist( maxTime );
        CHECK_EXPIRATION("persisting of node data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(0);

        // fall through
    case 1:
        _mapSavingStage = 1;
        CRLog::trace("ldomDocument::saveChanges() - element storage");

        if ( !_elemStorage.save(maxTime) ) {
            CRLog::error("Error while saving element data");
            return CR_ERROR;
        }
        CHECK_EXPIRATION("saving element storate")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(10);
        // fall through
    case 2:
        _mapSavingStage = 2;
        CRLog::trace("ldomDocument::saveChanges() - text storage");
        if ( !_textStorage.save(maxTime) ) {
            CRLog::error("Error while saving text data");
            return CR_ERROR;
        }
        CHECK_EXPIRATION("saving text storate")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(20);
        // fall through
    case 3:
        _mapSavingStage = 3;
        CRLog::trace("ldomDocument::saveChanges() - rect storage");

        if ( !_rectStorage.save(maxTime) ) {
            CRLog::error("Error while saving rect data");
            return CR_ERROR;
        }
        CHECK_EXPIRATION("saving rect storate")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(30);
        // fall through
    case 41:
        _mapSavingStage = 41;
        CRLog::trace("ldomDocument::saveChanges() - blob storage data");

        if ( _blobCache.saveToCache(maxTime) == CR_ERROR ) {
            CRLog::error("Error while saving blob storage data");
            return CR_ERROR;
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving blob storage data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(35);
        // fall through
    case 4:
        _mapSavingStage = 4;
        CRLog::trace("ldomDocument::saveChanges() - node style storage");

        if ( !_styleStorage.save(maxTime) ) {
            CRLog::error("Error while saving node style data");
            return CR_ERROR;
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving node style storage")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(40);
        // fall through
    case 5:
        _mapSavingStage = 5;
        CRLog::trace("ldomDocument::saveChanges() - misc data");
        {
            SerialBuf propsbuf(4096);
            getProps()->serialize( propsbuf );
            if ( !_cacheFile->write( CBT_PROP_DATA, propsbuf, COMPRESS_MISC_DATA ) ) {
                CRLog::error("Error while saving props data");
                return CR_ERROR;
            }
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving props data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(45);
        // fall through
    case 6:
        _mapSavingStage = 6;
        CRLog::trace("ldomDocument::saveChanges() - ID data");
        {
            SerialBuf idbuf(4096);
            serializeMaps( idbuf );
            if ( !_cacheFile->write( CBT_MAPS_DATA, idbuf, COMPRESS_MISC_DATA ) ) {
                CRLog::error("Error while saving Id data");
                return CR_ERROR;
            }
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving ID data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(50);
        // fall through
    case 7:
        _mapSavingStage = 7;
        if ( _pagesData.pos() ) {
            CRLog::trace("ldomDocument::saveChanges() - page data (%d bytes)", _pagesData.pos());
            if ( !_cacheFile->write( CBT_PAGE_DATA, _pagesData, COMPRESS_PAGES_DATA  ) ) {
                CRLog::error("Error while saving pages data");
                return CR_ERROR;
            }
        } else {
            CRLog::trace("ldomDocument::saveChanges() - no page data");
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving page data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(60);
        // fall through
    case 8:
        _mapSavingStage = 8;

        CRLog::trace("ldomDocument::saveChanges() - node data");
        if ( !saveNodeData() ) {
            CRLog::error("Error while node instance data");
            return CR_ERROR;
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving node data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(70);
        // fall through
    case 9:
        _mapSavingStage = 9;
        CRLog::trace("ldomDocument::saveChanges() - render info");
        {
            SerialBuf hdrbuf(0,true);
            if ( !_hdr.serialize(hdrbuf) ) {
                CRLog::error("Header data serialization is failed");
                return CR_ERROR;
            } else if ( !_cacheFile->write( CBT_REND_PARAMS, hdrbuf, false ) ) {
                CRLog::error("Error while writing header data");
                return CR_ERROR;
            }
        }
        CRLog::info("Saving render properties: styleHash=%x, stylesheetHash=%x, docflags=%x, width=%x, height=%x, nodeDisplayStyleHash=%x",
                    _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy, _hdr.node_displaystyle_hash);
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(73);

        CRLog::trace("ldomDocument::saveChanges() - TOC");
        {
            SerialBuf tocbuf(0,true);
            if ( !m_toc.serialize(tocbuf) ) {
                CRLog::error("TOC data serialization is failed");
                return CR_ERROR;
            } else if ( !_cacheFile->write( CBT_TOC_DATA, tocbuf, COMPRESS_TOC_DATA ) ) {
                CRLog::error("Error while writing TOC data");
                return CR_ERROR;
            }
        }
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(76);

        CRLog::trace("ldomDocument::saveChanges() - PageMap");
        {
            SerialBuf pagemapbuf(0,true);
            if ( !m_pagemap.serialize(pagemapbuf) ) {
                CRLog::error("PageMap data serialization is failed");
                return CR_ERROR;
            } else if ( !_cacheFile->write( CBT_PAGEMAP_DATA, pagemapbuf, COMPRESS_PAGEMAP_DATA ) ) {
                CRLog::error("Error while writing PageMap data");
                return CR_ERROR;
            }
        }
        if (!maxTime.infinite())
            _cacheFile->flush(false, maxTime); // intermediate flush
        CHECK_EXPIRATION("saving TOC data")
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(80);
        // fall through
    case 10:
        _mapSavingStage = 10;

        if ( !saveStylesData() ) {
            CRLog::error("Error while writing style data");
            return CR_ERROR;
        }
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(90);
        // fall through
    case 11:
        _mapSavingStage = 11;
        CRLog::trace("ldomDocument::saveChanges() - embedded fonts");
        {
            SerialBuf buf(4096);
            _fontList.serialize(buf);
            if (!_cacheFile->write(CBT_FONT_DATA, buf, COMPRESS_MISC_DATA) ) {
                CRLog::error("Error while saving embedded font data");
                return CR_ERROR;
            }
            CHECK_EXPIRATION("saving embedded fonts")
        }
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(95);
        // fall through
    case 12:
        _mapSavingStage = 12;
        CRLog::trace("ldomDocument::saveChanges() - flush");
        {
            CRTimerUtil infinite;
            if ( !_cacheFile->flush(true, infinite) ) {
                CRLog::error("Error while updating index of cache file");
                return CR_ERROR;
            }
            CHECK_EXPIRATION("flushing")
        }
        if (progressCallback) progressCallback->OnSaveCacheFileProgress(100);
        // fall through
    case 13:
        _mapSavingStage = 13;
        setCacheFileStale(false);
    }
    CRLog::trace("ldomDocument::saveChanges() - done");
    if (progressCallback) progressCallback->OnSaveCacheFileEnd();
    return CR_DONE;
}

/// save changes to cache file, @see loadCacheFileContent()
bool ldomDocument::saveChanges()
{
    if ( !_cacheFile )
        return true;
    CRLog::debug("ldomDocument::saveChanges() - infinite");
    CRTimerUtil timerNoLimit;
    ContinuousOperationResult res = saveChanges(timerNoLimit);
    return res!=CR_ERROR;
}

bool tinyNodeCollection::saveStylesData()
{
    SerialBuf stylebuf(0, true);
    lUInt32 stHash = _stylesheet.getHash();
    LVArray<css_style_ref_t> * list = _styles.getIndex();
    stylebuf.putMagic(styles_magic);
    stylebuf << stHash;
    stylebuf << (lUInt32)list->length(); // index
    for ( int i=0; i<list->length(); i++ ) {
        css_style_ref_t rec = list->get(i);
        if ( !rec.isNull() ) {
            stylebuf << (lUInt32)i; // index
            rec->serialize( stylebuf ); // style
        }
    }
    stylebuf << (lUInt32)0; // index=0 is end list mark
    stylebuf.putMagic(styles_magic);
    delete list;
    if ( stylebuf.error() )
        return false;
    CRLog::trace("Writing style data: %d bytes", stylebuf.pos());
    if ( !_cacheFile->write( CBT_STYLE_DATA, stylebuf, COMPRESS_STYLE_DATA) ) {
        return false;
    }
    return !stylebuf.error();
}

bool tinyNodeCollection::loadStylesData()
{
    SerialBuf stylebuf(0, true);
    if ( !_cacheFile->read( CBT_STYLE_DATA, stylebuf ) ) {
        CRLog::error("Error while reading style data");
        return false;
    }
    lUInt32 stHash = 0;
    lInt32 len = 0;

    // lUInt32 myHash = _stylesheet.getHash();
    // When loading from cache, this stylesheet was built with the
    // initial element name ids, which may have been replaced by
    // the one restored from the cache. So, its hash may be different
    // from the one we're going to load from cache.
    // This is not a failure, but a sign the stylesheet will have
    // to be regenerated (later, no need for it currently as we're
    // loading previously applied style data): this will be checked
    // in checkRenderContext() when comparing a combo hash
    // against _hdr.stylesheet_hash fetched from the cache.

    //LVArray<css_style_ref_t> * list = _styles.getIndex();
    stylebuf.checkMagic(styles_magic);
    stylebuf >> stHash;
    // Don't check for this:
    // if ( stHash != myHash ) {
    //     CRLog::info("tinyNodeCollection::loadStylesData() - stylesheet hash is changed: skip loading styles");
    //     return false;
    // }
    stylebuf >> len; // index
    if ( stylebuf.error() )
        return false;
    LVArray<css_style_ref_t> list(len, css_style_ref_t());
    for ( int i=0; i<list.length(); i++ ) {
        lUInt32 index = 0;
        stylebuf >> index; // index
        if ( index<=0 || (int)index>=len || stylebuf.error() )
            break;
        css_style_ref_t rec( new css_style_rec_t() );
        if ( !rec->deserialize(stylebuf) )
            break;
        list.set( index, rec );
    }
    stylebuf.checkMagic(styles_magic);
    if ( stylebuf.error() )
        return false;

    CRLog::trace("Setting style data: %d bytes", stylebuf.size());
    _styles.setIndex( list );

    return !stylebuf.error();
}

lUInt32 tinyNodeCollection::calcStyleHash()
{
    CRLog::debug("calcStyleHash start");
//    int maxlog = 20;
    lUInt32 res = 0; //_elemCount;
    lUInt32 globalHash = calcGlobalSettingsHash(getFontContextDocIndex());
    lUInt32 docFlags = getDocFlags();
    //CRLog::info("Calculating style hash...  elemCount=%d, globalHash=%08x, docFlags=%08x", _elemCount, globalHash, docFlags);
    if (_nodeStyleHash) {
        // Re-use saved _nodeStyleHash if it has not been invalidated,
        // as the following loop can be expensive
        res = _nodeStyleHash;
        CRLog::debug("  using saved _nodeStyleHash %x", res);
    }
    else {
        // We also compute _nodeDisplayStyleHash from each node style->display. It
        // may not change as often as _nodeStyleHash, but if it does, it means
        // some nodes switched between 'block' and 'inline', and that some autoBoxing
        // that may have been added should no more be in the DOM for a correct
        // rendering: in that case, the user will have to reload the document, and
        // we should invalidate the cache so a new correct DOM is build on load.
        _nodeDisplayStyleHash = 0;

        int count = ((_elemCount+TNC_PART_LEN-1) >> TNC_PART_SHIFT);
        for ( int i=0; i<count; i++ ) {
            int offs = i*TNC_PART_LEN;
            int sz = TNC_PART_LEN;
            if ( offs + sz > _elemCount+1 ) {
                sz = _elemCount+1 - offs;
            }
            ldomNode * buf = _elemList[i];
            for ( int j=0; j<sz; j++ ) {
                if ( buf[j].isElement() ) {
                    css_style_ref_t style = buf[j].getStyle();
                    lUInt32 sh = calcHash( style );
                    res = res * 31 + sh;
                    if (!style.isNull()) {
                        _nodeDisplayStyleHash = _nodeDisplayStyleHash * 31 + style.get()->display;
                        // Also account in this hash if this node is "white_space: pre" or alike.
                        // If white_space changes from/to "pre"-like to/from "normal"-like,
                        // the document will need to be reloaded so that the HTML text parts
                        // are parsed according the the PRE/not-PRE rules
                        if (style.get()->white_space >= css_ws_pre_line)
                            _nodeDisplayStyleHash += 29;
                        // Also account for style->float_, as it should create/remove new floatBox
                        // elements wrapping floats when toggling BLOCK_RENDERING_G(ENHANCED)
                        if (style.get()->float_ > css_f_none)
                            _nodeDisplayStyleHash += 123;
                    }
                    //printf("element %d %d style hash: %x\n", i, j, sh);
                    LVFontRef font = buf[j].getFont();
                    lUInt32 fh = calcHash( font );
                    res = res * 31 + fh;
                    //printf("element %d %d font hash: %x\n", i, j, fh);
//                    if ( maxlog>0 && sh==0 ) {
//                        style = buf[j].getStyle();
//                        CRLog::trace("[%06d] : s=%08x f=%08x  res=%08x", offs+j, sh, fh, res);
//                        maxlog--;
//                    }
                }
            }
        }

        CRLog::debug("  COMPUTED _nodeStyleHash %x", res);
        _nodeStyleHash = res;
        CRLog::debug("  COMPUTED _nodeDisplayStyleHash %x (initial: %x)", _nodeDisplayStyleHash, _nodeDisplayStyleHashInitial);
    }
    CRLog::info("Calculating style hash...  elemCount=%d, globalHash=%08x, docFlags=%08x, nodeStyleHash=%08x", _elemCount, globalHash, docFlags, res);
    res = res * 31 + _imgScalingOptions.getHash();
    res = res * 31 + _spaceWidthScalePercent;
    res = res * 31 + _minSpaceCondensingPercent;
    res = res * 31 + _unusedSpaceThresholdPercent;
    // _maxAddedLetterSpacingPercent does not need to be accounted, as, working
    // only on a laid out line, it does not need a re-rendering, but just
    // a _renderedBlockCache.clear() to reformat paragraphs and have the
    // word re-positionned (the paragraphs width & height do not change)
    res = (res * 31 + globalHash) * 31 + docFlags;
//    CRLog::info("Calculated style hash = %08x", res);
    CRLog::debug("calcStyleHash done");
    return res;
}

static void validateChild( ldomNode * node )
{
    // DEBUG TEST
    if ( !node->isRoot() && node->getParentNode()->getChildIndex( node->getDataIndex() )<0 ) {
        CRLog::error("Invalid parent->child relation for nodes %d->%d", node->getParentNode()->getDataIndex(), node->getParentNode()->getDataIndex() );
    }
}

/// called on document loading end
bool tinyNodeCollection::validateDocument()
{
    ((ldomDocument*)this)->getRootNode()->recurseElements(validateChild);
    int count = ((_elemCount+TNC_PART_LEN-1) >> TNC_PART_SHIFT);
    bool res = true;
    for ( int i=0; i<count; i++ ) {
        int offs = i*TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if ( offs + sz > _elemCount+1 ) {
            sz = _elemCount+1 - offs;
        }
        ldomNode * buf = _elemList[i];
        for ( int j=0; j<sz; j++ ) {
            buf[j].setDocumentIndex( _docIndex );
            if ( buf[j].isElement() ) {
                lUInt16 style = getNodeStyleIndex( buf[j]._handle._dataIndex );
                lUInt16 font = getNodeFontIndex( buf[j]._handle._dataIndex );;
                if ( !style ) {
                    if ( !buf[j].isRoot() ) {
                        CRLog::error("styleId=0 for node <%s> %d", LCSTR(buf[j].getNodeName()), buf[j].getDataIndex());
                        res = false;
                    }
                } else if ( _styles.get(style).isNull() ) {
                    CRLog::error("styleId!=0, but absent in cache for node <%s> %d", LCSTR(buf[j].getNodeName()), buf[j].getDataIndex());
                    res = false;
                }
                if ( !font ) {
                    if ( !buf[j].isRoot() ) {
                        CRLog::error("fontId=0 for node <%s>", LCSTR(buf[j].getNodeName()));
                        res = false;
                    }
                } else if ( _fonts.get(font).isNull() ) {
                    CRLog::error("fontId!=0, but absent in cache for node <%s>", LCSTR(buf[j].getNodeName()));
                    res = false;
                }
            }
        }
    }
    return res;
}

bool tinyNodeCollection::updateLoadedStyles( bool enabled )
{
    int count = ((_elemCount+TNC_PART_LEN-1) >> TNC_PART_SHIFT);
    bool res = true;
    LVArray<css_style_ref_t> * list = _styles.getIndex();

    _fontMap.clear(); // style index to font index

    for ( int i=0; i<count; i++ ) {
        int offs = i*TNC_PART_LEN;
        int sz = TNC_PART_LEN;
        if ( offs + sz > _elemCount+1 ) {
            sz = _elemCount+1 - offs;
        }
        ldomNode * buf = _elemList[i];
        for ( int j=0; j<sz; j++ ) {
            buf[j].setDocumentIndex( _docIndex );
            if ( buf[j].isElement() ) {
                lUInt16 style = getNodeStyleIndex( buf[j]._handle._dataIndex );
                if ( enabled && style!=0 ) {
                    css_style_ref_t s = list->get( style );
                    if ( !s.isNull() ) {
                        lUInt16 fntIndex = _fontMap.get( style );
                        if ( fntIndex==0 ) {
                            LVFontRef fnt = getFont(s.get(), getFontContextDocIndex());
                            fntIndex = (lUInt16)_fonts.cache( fnt );
                            if ( fnt.isNull() ) {
                                CRLog::error("font not found for style!");
                            } else {
                                _fontMap.set(style, fntIndex);
                            }
                        } else {
                            _fonts.addIndexRef( fntIndex );
                        }
                        if ( fntIndex<=0 ) {
                            CRLog::error("font caching failed for style!");
                            res = false;
                        } else {
                            setNodeFontIndex( buf[j]._handle._dataIndex, fntIndex );
                            //buf[j]._data._pelem._fontIndex = fntIndex;
                        }
                    } else {
                        CRLog::error("Loaded style index %d not found in style collection", (int)style);
                        setNodeFontIndex( buf[j]._handle._dataIndex, 0 );
                        setNodeStyleIndex( buf[j]._handle._dataIndex, 0 );
//                        buf[j]._data._pelem._styleIndex = 0;
//                        buf[j]._data._pelem._fontIndex = 0;
                        res = false;
                    }
                } else {
                    setNodeFontIndex( buf[j]._handle._dataIndex, 0 );
                    setNodeStyleIndex( buf[j]._handle._dataIndex, 0 );
//                    buf[j]._data._pelem._styleIndex = 0;
//                    buf[j]._data._pelem._fontIndex = 0;
                }
            }
        }
    }
#ifdef TODO_INVESTIGATE
    if ( enabled && res) {
        //_styles.setIndex( *list );
        // correct list reference counters

        for ( int i=0; i<list->length(); i++ ) {
            if ( !list->get(i).isNull() ) {
                // decrease reference counter
                // TODO:
                //_styles.release( list->get(i) );
            }
        }
    }
#endif
    delete list;
//    getRootNode()->setFont( _def_font );
//    getRootNode()->setStyle( _def_style );
    _nodeStyleHash = 0;
    return res;
}

/// swaps to cache file or saves changes, limited by time interval
ContinuousOperationResult ldomDocument::swapToCache( CRTimerUtil & maxTime )
{
    CRLog::trace("ldomDocument::swapToCache entered");
    if ( _maperror )
        return CR_ERROR;
    if ( !_mapped ) {
        CRLog::trace("ldomDocument::swapToCache creating cache file");
        if ( !createCacheFile() ) {
            CRLog::error("ldomDocument::swapToCache: failed: cannot create cache file");
            _maperror = true;
            return CR_ERROR;
        }
    }
    _mapped = true;
    if (!maxTime.infinite()) {
        CRLog::info("Cache file is created, but document saving is postponed");
        return CR_TIMEOUT;
    }
    ContinuousOperationResult res = saveChanges(maxTime);
    if ( res==CR_ERROR )
    {
        CRLog::error("Error while saving changes to cache file");
        _maperror = true;
        return CR_ERROR;
    }
    CRLog::info("Successfully saved document to cache file: %dK", _cacheFile->getSize()/1024 );
    return res;
}

/// saves recent changes to mapped file
ContinuousOperationResult ldomDocument::updateMap(CRTimerUtil & maxTime, LVDocViewCallback * progressCallback)
{
    if ( !_cacheFile || !_mapped )
        return CR_DONE;

    if ( _cacheFileLeaveAsDirty ) {
        CRLog::info("requested to set cache file as dirty without any update");
        _cacheFile->setDirtyFlag(true);
        return CR_DONE;
    }

    if ( !_cacheFileStale) {
        CRLog::info("No change, cache file update not needed");
        return CR_DONE;
    }
    CRLog::info("Updating cache file");

    ContinuousOperationResult res = saveChanges(maxTime, progressCallback);
    if ( res==CR_ERROR )
    {
        CRLog::error("Error while saving changes to cache file");
        return CR_ERROR;
    }

    if ( res==CR_DONE ) {
        CRLog::info("Cache file updated successfully");
        dumpStatistics();
    }
    return res;
}

#endif

static const char * doccache_magic = "CoolReader3 Document Cache Directory Index\nV1.00\n";

/// document cache
class ldomDocCacheImpl : public ldomDocCache
{
    lString16 _cacheDir;
    lvsize_t _maxSize;
    lUInt32 _oldStreamSize;
    lUInt32 _oldStreamCRC;

    struct FileItem {
        lString16 filename;
        lUInt32 size;
    };
    LVPtrVector<FileItem> _files;
public:
    ldomDocCacheImpl( lString16 cacheDir, lvsize_t maxSize )
        : _cacheDir( cacheDir ), _maxSize( maxSize ), _oldStreamSize(0), _oldStreamCRC(0)
    {
        LVAppendPathDelimiter( _cacheDir );
        CRLog::trace("ldomDocCacheImpl(%s maxSize=%d)", LCSTR(_cacheDir), (int)maxSize);
    }

    bool writeIndex()
    {
        lString16 filename = _cacheDir + "cr3cache.inx";
        if (_oldStreamSize == 0)
        {
            LVStreamRef oldStream = LVOpenFileStream(filename.c_str(), LVOM_READ);
            if (!oldStream.isNull()) {
                _oldStreamSize = (lUInt32)oldStream->GetSize();
                _oldStreamCRC = (lUInt32)oldStream->getcrc32();
            }
        }

        // fill buffer
        SerialBuf buf( 16384, true );
        buf.putMagic( doccache_magic );
        lUInt32 start = buf.pos();
        int count = _files.length();
        buf << (lUInt32)count;
        for ( int i=0; i<count && !buf.error(); i++ ) {
            FileItem * item = _files[i];
            buf << item->filename;
            buf << item->size;
            CRLog::trace("cache item: %s %d", LCSTR(item->filename), (int)item->size);
        }
        buf.putCRC( buf.pos() - start );
        if ( buf.error() )
            return false;
        lUInt32 newCRC = buf.getCRC();
        lUInt32 newSize = buf.pos();

        // check to avoid rewritting of identical file
        if (newCRC != _oldStreamCRC || newSize != _oldStreamSize) {
            // changed: need to write
            CRLog::trace("Writing cache index");
            LVStreamRef stream = LVOpenFileStream(filename.c_str(), LVOM_WRITE);
            if ( !stream )
                return false;
            if ( stream->Write( buf.buf(), buf.pos(), NULL )!=LVERR_OK )
                return false;
            _oldStreamCRC = newCRC;
            _oldStreamSize = newSize;
        }
        return true;
    }

    bool readIndex(  )
    {
        lString16 filename = _cacheDir + "cr3cache.inx";
        // read index
        lUInt32 totalSize = 0;
        LVStreamRef instream = LVOpenFileStream( filename.c_str(), LVOM_READ );
        if ( !instream.isNull() ) {
            LVStreamBufferRef sb = instream->GetReadBuffer(0, instream->GetSize() );
            if ( !sb )
                return false;
            SerialBuf buf( sb->getReadOnly(), sb->getSize() );
            if ( !buf.checkMagic( doccache_magic ) ) {
                CRLog::error("wrong cache index file format");
                return false;
            }

            lUInt32 start = buf.pos();
            lUInt32 count;
            buf >> count;
            for (lUInt32 i=0; i < count && !buf.error(); i++) {
                FileItem * item = new FileItem();
                _files.add( item );
                buf >> item->filename;
                buf >> item->size;
                CRLog::trace("cache %d: %s [%d]", i, UnicodeToUtf8(item->filename).c_str(), (int)item->size );
                totalSize += item->size;
            }
            if ( !buf.checkCRC( buf.pos() - start ) ) {
                CRLog::error("CRC32 doesn't match in cache index file");
                return false;
            }

            if ( buf.error() )
                return false;

            CRLog::info( "Document cache index file read ok, %d files in cache, %d bytes", _files.length(), totalSize );
            return true;
        } else {
            CRLog::error( "Document cache index file cannot be read" );
            return false;
        }
    }

    /// remove all .cr3 files which are not listed in index
    bool removeExtraFiles( )
    {
        LVContainerRef container;
        container = LVOpenDirectory( _cacheDir.c_str(), L"*.cr3" );
        if ( container.isNull() ) {
            if ( !LVCreateDirectory( _cacheDir ) ) {
                CRLog::error("Cannot create directory %s", UnicodeToUtf8(_cacheDir).c_str() );
                return false;
            }
            container = LVOpenDirectory( _cacheDir.c_str(), L"*.cr3" );
            if ( container.isNull() ) {
                CRLog::error("Cannot open directory %s", UnicodeToUtf8(_cacheDir).c_str() );
                return false;
            }
        }
        for ( int i=0; i<container->GetObjectCount(); i++ ) {
            const LVContainerItemInfo * item = container->GetObjectInfo( i );
            if ( !item->IsContainer() ) {
                lString16 fn = item->GetName();
                if ( !fn.endsWith(".cr3") )
                    continue;
                if ( findFileIndex(fn)<0 ) {
                    // delete file
                    CRLog::info("Removing cache file not specified in index: %s", UnicodeToUtf8(fn).c_str() );
                    if ( !LVDeleteFile( _cacheDir + fn ) ) {
                        CRLog::error("Error while removing cache file not specified in index: %s", UnicodeToUtf8(fn).c_str() );
                    }
                }
            }
        }
        return true;
    }

    // remove all extra files to add new one of specified size
    bool reserve( lvsize_t allocSize )
    {
        bool res = true;
        // remove extra files specified in list
        lvsize_t dirsize = allocSize;
        for ( int i=0; i<_files.length(); ) {
            if ( LVFileExists( _cacheDir + _files[i]->filename ) ) {
                if ( (i>0 || allocSize>0) && dirsize+_files[i]->size > _maxSize ) {
                    if ( LVDeleteFile( _cacheDir + _files[i]->filename ) ) {
                        _files.erase(i, 1);
                    } else {
                        CRLog::error("Cannot delete cache file %s", UnicodeToUtf8(_files[i]->filename).c_str() );
                        dirsize += _files[i]->size;
                        res = false;
                        i++;
                    }
                } else {
                    dirsize += _files[i]->size;
                    i++;
                }
            } else {
                CRLog::error("File %s is found in cache index, but does not exist", UnicodeToUtf8(_files[i]->filename).c_str() );
                _files.erase(i, 1);
            }
        }
        return res;
    }

    int findFileIndex( lString16 filename )
    {
        for ( int i=0; i<_files.length(); i++ ) {
            if ( _files[i]->filename == filename )
                return i;
        }
        return -1;
    }

    bool moveFileToTop( lString16 filename, lUInt32 size )
    {
        int index = findFileIndex( filename );
        if ( index<0 ) {
            FileItem * item = new FileItem();
            item->filename = filename;
            item->size = size;
            _files.insert( 0, item );
        } else {
            _files.move( 0, index );
            _files[0]->size = size;
        }
        return writeIndex();
    }

    bool init()
    {
        CRLog::info("Initialize document cache in directory %s", UnicodeToUtf8(_cacheDir).c_str() );
        // read index
        if ( readIndex(  ) ) {
            // read successfully
            // remove files not specified in list
            removeExtraFiles( );
        } else {
            if ( !LVCreateDirectory( _cacheDir ) ) {
                CRLog::error("Document Cache: cannot create cache directory %s, disabling cache", UnicodeToUtf8(_cacheDir).c_str() );
                return false;
            }
            _files.clear();

        }
        reserve(0);
        if ( !writeIndex() )
            return false; // cannot write index: read only?
        return true;
    }

    /// remove all files
    bool clear()
    {
        for ( int i=0; i<_files.length(); i++ )
            LVDeleteFile( _files[i]->filename );
        _files.clear();
        return writeIndex();
    }

    // dir/filename.{crc32}.cr3
    lString16 makeFileName( lString16 filename, lUInt32 crc, lUInt32 docFlags )
    {
        lString16 fn;
        lString8 filename8 = UnicodeToTranslit(filename);
        bool lastUnderscore = false;
        int goodCount = 0;
        int badCount = 0;
        for (int i = 0; i < filename8.length(); i++) {
            lChar16 ch = filename8[i];

            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
                fn << ch;
                lastUnderscore = false;
                goodCount++;
            } else {
                if (!lastUnderscore) {
                    fn << L"_";
                    lastUnderscore = true;
                }
                badCount++;
            }
        }
        if (goodCount < 2 || badCount > goodCount * 2)
            fn << "_noname";
        if (fn.length() > 25)
            fn = fn.substr(0, 12) + "-" + fn.substr(fn.length()-12, 12);
        char s[16];
        sprintf(s, ".%08x.%d.cr3", (unsigned)crc, (int)docFlags);
        return fn + lString16( s ); //_cacheDir +
    }

    /// open existing cache file stream
    LVStreamRef openExisting( lString16 filename, lUInt32 crc, lUInt32 docFlags, lString16 &cachePath )
    {
        lString16 fn = makeFileName( filename, crc, docFlags );
        CRLog::debug("ldomDocCache::openExisting(%s)", LCSTR(fn));
        // Try filename with ".keep" extension (that a user can manually add
        // to a .cr3 cache file, for it to no more be maintained by crengine
        // in its index, thus not subject to _maxSize enforcement, so sure
        // to not be deleted by crengine)
        lString16 fn_keep = _cacheDir + fn + ".keep";
        if ( LVFileExists(fn_keep) ) {
            LVStreamRef stream = LVOpenFileStream( fn_keep.c_str(), LVOM_APPEND|LVOM_FLAG_SYNC );
            if ( !stream.isNull() ) {
                CRLog::info( "ldomDocCache::openExisting - opening user renamed cache file %s", UnicodeToUtf8(fn_keep).c_str() );
                cachePath = fn_keep;
#if ENABLED_BLOCK_WRITE_CACHE
                stream = LVCreateBlockWriteStream( stream, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT );
#endif
                return stream;
            }
        }
        LVStreamRef res;
        if ( findFileIndex( fn ) < 0 ) {
            CRLog::error( "ldomDocCache::openExisting - File %s is not found in cache index", UnicodeToUtf8(fn).c_str() );
            return res;
        }
        lString16 pathname = _cacheDir + fn;
        res = LVOpenFileStream( pathname.c_str(), LVOM_APPEND|LVOM_FLAG_SYNC );
        if ( !res ) {
            CRLog::error( "ldomDocCache::openExisting - File %s is listed in cache index, but cannot be opened", UnicodeToUtf8(fn).c_str() );
            return res;
        }
        cachePath = pathname;

#if ENABLED_BLOCK_WRITE_CACHE
        res = LVCreateBlockWriteStream( res, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT );
#if TEST_BLOCK_STREAM

        LVStreamRef stream2 = LVOpenFileStream( (_cacheDir + fn + "_c").c_str(), LVOM_APPEND );
        if ( !stream2 ) {
            CRLog::error( "ldomDocCache::openExisting - file %s is cannot be created", UnicodeToUtf8(fn).c_str() );
            return stream2;
        }
        res = LVCreateCompareTestStream(res, stream2);
#endif
#endif

        lUInt32 fileSize = (lUInt32) res->GetSize();
        moveFileToTop( fn, fileSize );
        return res;
    }

    /// create new cache file
    LVStreamRef createNew( lString16 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString16 &cachePath )
    {
        lString16 fn = makeFileName( filename, crc, docFlags );
        LVStreamRef res;
        lString16 pathname = _cacheDir + fn;
        // If this cache filename exists with a ".keep" extension (manually
        // added by the user), and we were going to create a new one (because
        // this .keep is invalid, or cache file format version has changed),
        // remove it and create the new one with this same .keep extension,
        // so it stays (as wished by the user) not maintained by crengine.
        lString16 fn_keep = pathname + ".keep";
        if ( LVFileExists( fn_keep ) ) {
            LVDeleteFile( pathname ); // delete .cr3 if any
            LVDeleteFile( fn_keep ); // delete invalid .cr3.keep
            LVStreamRef stream = LVOpenFileStream( fn_keep.c_str(), LVOM_APPEND|LVOM_FLAG_SYNC );
            if ( !stream.isNull() ) {
                CRLog::info( "ldomDocCache::createNew - re-creating user renamed cache file %s", UnicodeToUtf8(fn_keep).c_str() );
                cachePath = fn_keep;
#if ENABLED_BLOCK_WRITE_CACHE
                stream = LVCreateBlockWriteStream( stream, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT );
#endif
                return stream;
            }
        }
        if ( findFileIndex( pathname ) >= 0 )
            LVDeleteFile( pathname );
        reserve( fileSize/10 );
        //res = LVMapFileStream( (_cacheDir+fn).c_str(), LVOM_APPEND, fileSize );
        LVDeleteFile( pathname ); // try to delete, ignore errors
        res = LVOpenFileStream( pathname.c_str(), LVOM_APPEND|LVOM_FLAG_SYNC );
        if ( !res ) {
            CRLog::error( "ldomDocCache::createNew - file %s is cannot be created", UnicodeToUtf8(fn).c_str() );
            return res;
        }
        cachePath = pathname;
#if ENABLED_BLOCK_WRITE_CACHE
        res = LVCreateBlockWriteStream( res, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT );
#if TEST_BLOCK_STREAM
        LVStreamRef stream2 = LVOpenFileStream( (pathname+L"_c").c_str(), LVOM_APPEND );
        if ( !stream2 ) {
            CRLog::error( "ldomDocCache::createNew - file %s is cannot be created", UnicodeToUtf8(fn).c_str() );
            return stream2;
        }
        res = LVCreateCompareTestStream(res, stream2);
#endif
#endif
        moveFileToTop( fn, fileSize );
        return res;
    }

    virtual ~ldomDocCacheImpl()
    {
    }
};

static ldomDocCacheImpl * _cacheInstance = NULL;

bool ldomDocCache::init( lString16 cacheDir, lvsize_t maxSize )
{
    if ( _cacheInstance )
        delete _cacheInstance;
    CRLog::info("Initialize document cache at %s (max size = %d)", UnicodeToUtf8(cacheDir).c_str(), (int)maxSize );
    _cacheInstance = new ldomDocCacheImpl( cacheDir, maxSize );
    if ( !_cacheInstance->init() ) {
        delete _cacheInstance;
        _cacheInstance = NULL;
        return false;
    }
    return true;
}

bool ldomDocCache::close()
{
    if ( !_cacheInstance )
        return false;
    delete _cacheInstance;
    _cacheInstance = NULL;
    return true;
}

/// open existing cache file stream
LVStreamRef ldomDocCache::openExisting( lString16 filename, lUInt32 crc, lUInt32 docFlags, lString16 &cachePath )
{
    if ( !_cacheInstance )
        return LVStreamRef();
    return _cacheInstance->openExisting( filename, crc, docFlags, cachePath );
}

/// create new cache file
LVStreamRef ldomDocCache::createNew( lString16 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString16 &cachePath )
{
    if ( !_cacheInstance )
        return LVStreamRef();
    return _cacheInstance->createNew( filename, crc, docFlags, fileSize, cachePath );
}

/// delete all cache files
bool ldomDocCache::clear()
{
    if ( !_cacheInstance )
        return false;
    return _cacheInstance->clear();
}

/// returns true if cache is enabled (successfully initialized)
bool ldomDocCache::enabled()
{
    return _cacheInstance!=NULL;
}

//void calcStyleHash( ldomNode * node, lUInt32 & value )
//{
//    if ( !node )
//        return;
//
//    if ( node->isText() || node->getRendMethod()==erm_invisible ) {
//        value = value * 75 + 1673251;
//        return; // don't go through invisible nodes
//    }
//
//    css_style_ref_t style = node->getStyle();
//    font_ref_t font = node->getFont();
//    lUInt32 styleHash = (!style) ? 4324324 : calcHash( style );
//    lUInt32 fontHash = (!font) ? 256371 : calcHash( font );
//    value = (value*75 + styleHash) * 75 + fontHash;
//
//    int cnt = node->getChildCount();
//    for ( int i=0; i<cnt; i++ ) {
//        calcStyleHash( node->getChildNode(i), value );
//    }
//}


#if BUILD_LITE!=1

/// save document formatting parameters after render
void ldomDocument::updateRenderContext()
{
    int dx = _page_width;
    int dy = _page_height;
    _nodeStyleHash = 0; // force recalculation by calcStyleHash()
    lUInt32 styleHash = calcStyleHash();
    lUInt32 stylesheetHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
    //calcStyleHash( getRootNode(), styleHash );
    _hdr.render_style_hash = styleHash;
    _hdr.stylesheet_hash = stylesheetHash;
    _hdr.render_dx = dx;
    _hdr.render_dy = dy;
    _hdr.render_docflags = _docFlags;
    _hdr.node_displaystyle_hash = _nodeDisplayStyleHashInitial; // we keep using the initial one
    CRLog::info("Updating render properties: styleHash=%x, stylesheetHash=%x, docflags=%x, width=%x, height=%x, nodeDisplayStyleHash=%x",
                _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy, _hdr.node_displaystyle_hash);
}

/// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
bool ldomDocument::checkRenderContext()
{
    bool res = true;
    ldomNode * node = getRootNode();
    if (node != NULL && node->getFont().isNull()) {
        // This may happen when epubfmt.cpp has called forceReinitStyles()
        // because the EPUB contains embedded fonts: a full nodes styles
        // re-init is needed to use the new fonts (only available at end
        // of loading)
        CRLog::info("checkRenderContext: style is not set for root node");
        res = false;
    }
    int dx = _page_width;
    int dy = _page_height;
    lUInt32 styleHash = calcStyleHash();
    lUInt32 stylesheetHash = (((_stylesheet.getHash() * 31) + calcHash(_def_style))*31 + calcHash(_def_font));
    //calcStyleHash( getRootNode(), styleHash );
    if ( styleHash != _hdr.render_style_hash ) {
        CRLog::info("checkRenderContext: Style hash doesn't match %x!=%x", styleHash, _hdr.render_style_hash);
        res = false;
        if (_just_rendered_from_cache)
            printf("CRE WARNING: cached rendering is invalid (style hash mismatch): doing full rendering\n");
    } else if ( stylesheetHash != _hdr.stylesheet_hash ) {
        CRLog::info("checkRenderContext: Stylesheet hash doesn't match %x!=%x", stylesheetHash, _hdr.stylesheet_hash);
        res = false;
        if (_just_rendered_from_cache)
            printf("CRE WARNING: cached rendering is invalid (stylesheet hash mismatch): doing full rendering\n");
    } else if ( _docFlags != _hdr.render_docflags ) {
        CRLog::info("checkRenderContext: Doc flags don't match %x!=%x", _docFlags, _hdr.render_docflags);
        res = false;
        if (_just_rendered_from_cache)
            printf("CRE WARNING: cached rendering is invalid (doc flags mismatch): doing full rendering\n");
    } else if ( dx != (int)_hdr.render_dx ) {
        CRLog::info("checkRenderContext: Width doesn't match %x!=%x", dx, (int)_hdr.render_dx);
        res = false;
        if (_just_rendered_from_cache)
            printf("CRE WARNING: cached rendering is invalid (page width mismatch): doing full rendering\n");
    } else if ( dy != (int)_hdr.render_dy ) {
        CRLog::info("checkRenderContext: Page height doesn't match %x!=%x", dy, (int)_hdr.render_dy);
        res = false;
        if (_just_rendered_from_cache)
            printf("CRE WARNING: cached rendering is invalid (page height mismatch): doing full rendering\n");
    }
    // no need to check for _nodeDisplayStyleHash != _hdr.node_displaystyle_hash:
    // this is implicitely done by styleHash != _hdr.render_style_hash (whose _nodeDisplayStyleHash is a subset)
    _just_rendered_from_cache = false;
    if ( res ) {

        //if ( pages->length()==0 ) {
//            _pagesData.reset();
//            pages->deserialize( _pagesData );
        //}

        return true;
    }
//    _hdr.render_style_hash = styleHash;
//    _hdr.stylesheet_hash = stylesheetHash;
//    _hdr.render_dx = dx;
//    _hdr.render_dy = dy;
//    _hdr.render_docflags = _docFlags;
//    CRLog::info("New render properties: styleHash=%x, stylesheetHash=%x, docflags=%04x, width=%d, height=%d",
//                _hdr.render_style_hash, _hdr.stylesheet_hash, _hdr.render_docflags, _hdr.render_dx, _hdr.render_dy);
    return false;
}

#endif

void lxmlDocBase::setStyleSheet( const char * css, bool replace )
{
    lString8 s(css);

    //CRLog::trace("lxmlDocBase::setStyleSheet(length:%d replace:%s css text hash: %x)", strlen(css), replace ? "yes" : "no", s.getHash());
    lUInt32 oldHash = _stylesheet.getHash();
    if ( replace ) {
        //CRLog::debug("cleaning stylesheet contents");
        _stylesheet.clear();
    }
    if ( css && *css ) {
        //CRLog::debug("appending stylesheet contents: \n%s", css);
        _stylesheet.parse( css, true );
        // We use override_important=true: we are the only code
        // that sets the main CSS (including style tweaks). We allow
        // any !important to override any previous !important.
        // Other calls to _stylesheet.parse() elsewhere are used to
        // include document embedded or inline CSS, with the default
        // of override_important=false, so they won't override
        // the ones we set here.
    }
    lUInt32 newHash = _stylesheet.getHash();
    if (oldHash != newHash) {
        CRLog::debug("New stylesheet hash: %08x", newHash);
    }
}






//=====================================================
// ldomElement declaration placed here to hide DOM implementation
// use ldomNode rich interface instead
class tinyElement
{
    friend struct ldomNode;
private:
    ldomDocument * _document;
    ldomNode * _parentNode;
    lUInt16 _id;
    lUInt16 _nsid;
    LVArray < lInt32 > _children;
    ldomAttributeCollection _attrs;
    lvdom_element_render_method _rendMethod;
public:
    tinyElement( ldomDocument * document, ldomNode * parentNode, lUInt16 nsid, lUInt16 id )
    : _document(document), _parentNode(parentNode), _id(id), _nsid(nsid), _rendMethod(erm_invisible)
    { _document->_tinyElementCount++; }
    /// destructor
    ~tinyElement() { _document->_tinyElementCount--; }
};


#define NPELEM _data._elem_ptr
#define NPTEXT _data._text_ptr._str

//=====================================================

/// minimize memory consumption
void tinyNodeCollection::compact()
{
    _textStorage.compact(0xFFFFFF);
    _elemStorage.compact(0xFFFFFF);
    _rectStorage.compact(0xFFFFFF);
    _styleStorage.compact(0xFFFFFF);
}

/// allocate new tinyElement
ldomNode * tinyNodeCollection::allocTinyElement( ldomNode * parent, lUInt16 nsid, lUInt16 id )
{
    ldomNode * node = allocTinyNode( ldomNode::NT_ELEMENT );
    tinyElement * elem = new tinyElement( (ldomDocument*)this, parent, nsid, id );
    node->NPELEM = elem;
    return node;
}

static void readOnlyError()
{
    crFatalError( 125, "Text node is persistent (read-only)! Call modify() to get r/w instance." );
}

//=====================================================

// shortcut for dynamic element accessor
#ifdef _DEBUG
  #define ASSERT_NODE_NOT_NULL \
    if ( isNull() ) \
		crFatalError( 1313, "Access to null node" )
#else
  #define ASSERT_NODE_NOT_NULL
#endif

/// returns node level, 0 is root node
lUInt8 ldomNode::getNodeLevel() const
{
    const ldomNode * node = this;
    int level = 0;
    for ( ; node; node = node->getParentNode() )
        level++;
    return (lUInt8)level;
}

void ldomNode::onCollectionDestroy()
{
    if ( isNull() )
        return;
    //CRLog::trace("ldomNode::onCollectionDestroy(%d) type=%d", this->_handle._dataIndex, TNTYPE);
    switch ( TNTYPE ) {
    case NT_TEXT:
        delete _data._text_ptr;
        _data._text_ptr = NULL;
        break;
    case NT_ELEMENT:
        // ???
#if BUILD_LITE!=1
        getDocument()->clearNodeStyle( _handle._dataIndex );
#endif
        delete NPELEM;
        NPELEM = NULL;
        break;
#if BUILD_LITE!=1
    case NT_PTEXT:      // immutable (persistent) text node
        // do nothing
        break;
    case NT_PELEMENT:   // immutable (persistent) element node
        // do nothing
        break;
#endif
    }
}

void ldomNode::destroy()
{
    if ( isNull() )
        return;
    //CRLog::trace("ldomNode::destroy(%d) type=%d", this->_handle._dataIndex, TNTYPE);
    switch ( TNTYPE ) {
    case NT_TEXT:
        delete _data._text_ptr;
        break;
    case NT_ELEMENT:
        {
#if BUILD_LITE!=1
            getDocument()->clearNodeStyle(_handle._dataIndex);
#endif
            tinyElement * me = NPELEM;
            // delete children
            for ( int i=0; i<me->_children.length(); i++ ) {
                ldomNode * child = getDocument()->getTinyNode(me->_children[i]);
                if ( child )
                    child->destroy();
            }
            delete me;
            NPELEM = NULL;
        }
        delete NPELEM;
        break;
#if BUILD_LITE!=1
    case NT_PTEXT:
        // disable removing from storage: to minimize modifications
        //_document->_textStorage.freeNode( _data._ptext_addr._addr );
        break;
    case NT_PELEMENT:   // immutable (persistent) element node
        {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            for ( int i=0; i<me->childCount; i++ )
                getDocument()->getTinyNode( me->children[i] )->destroy();
            getDocument()->clearNodeStyle( _handle._dataIndex );
//            getDocument()->_styles.release( _data._pelem._styleIndex );
//            getDocument()->_fonts.release( _data._pelem._fontIndex );
//            _data._pelem._styleIndex = 0;
//            _data._pelem._fontIndex = 0;
            getDocument()->_elemStorage.freeNode( _data._pelem_addr );
        }
        break;
#endif
    }
    getDocument()->recycleTinyNode( _handle._dataIndex );
}

/// returns index of child node by dataIndex
int ldomNode::getChildIndex( lUInt32 dataIndex ) const
{
    // was here and twice below: dataIndex &= 0xFFFFFFF0;
    // The lowest bits of a dataIndex carry properties about the node:
    //   bit 0: 0 = text node / 1 = element node
    //   bit 1: 0 = mutable node / 1 = immutable (persistent, cached)
    // (So, all Text nodes have an even dataIndex, and Element nodes
    // all have a odd dataIndex.)
    // This '& 0xFFFFFFF0' was to clear these properties so a same
    // node can be found if these properties change (mostly useful
    // with mutable<>persistent).
    // But text nodes and Element nodes use different independant counters
    // (see tinyNodeCollection::allocTinyNode(): _elemCount++, _textCount++)
    // and we may have a text node with dataIndex 8528, and an element
    // node with dataIndex 8529, that would be confused with each other
    // if we use 0xFFFFFFF0.
    // This could cause finding the wrong node, and strange side effects.
    // With '& 0xFFFFFFF1' keep the lowest bit.
    dataIndex &= 0xFFFFFFF1;
    ASSERT_NODE_NOT_NULL;
    int parentIndex = -1;
    switch ( TNTYPE ) {
    case NT_ELEMENT:
        {
            tinyElement * me = NPELEM;
            for ( int i=0; i<me->_children.length(); i++ ) {
                if ( (me->_children[i] & 0xFFFFFFF1) == dataIndex ) {
                    // found
                    parentIndex = i;
                    break;
                }
            }
        }
        break;
#if BUILD_LITE!=1
    case NT_PELEMENT:
        {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            for ( int i=0; i<me->childCount; i++ ) {
                if ( (me->children[i] & 0xFFFFFFF1) == dataIndex ) {
                    // found
                    parentIndex = i;
                    break;
                }
            }
        }
        break;
    case NT_PTEXT:      // immutable (persistent) text node
#endif
    case NT_TEXT:
        break;
    }
    return parentIndex;
}

/// returns index of node inside parent's child collection
int ldomNode::getNodeIndex() const
{
    ASSERT_NODE_NOT_NULL;
    ldomNode * parent = getParentNode();
    if ( parent )
        return parent->getChildIndex( getDataIndex() );
    return 0;
}

/// returns true if node is document's root
bool ldomNode::isRoot() const
{
    ASSERT_NODE_NOT_NULL;
    switch ( TNTYPE ) {
    case NT_ELEMENT:
        return !NPELEM->_parentNode;
#if BUILD_LITE!=1
    case NT_PELEMENT:   // immutable (persistent) element node
        {
             ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
             return me->parentIndex==0;
        }
        break;
    case NT_PTEXT:      // immutable (persistent) text node
        {
            return getDocument()->_textStorage.getParent( _data._ptext_addr )==0;
        }
#endif
    case NT_TEXT:
        return _data._text_ptr->getParentIndex()==0;
    }
    return false;
}

/// call to invalidate cache if persistent node content is modified
void ldomNode::modified()
{
#if BUILD_LITE!=1
    if ( isPersistent() ) {
        if ( isElement() )
            getDocument()->_elemStorage.modified( _data._pelem_addr );
        else
            getDocument()->_textStorage.modified( _data._ptext_addr );
    }
#endif
}

/// changes parent of item
void ldomNode::setParentNode( ldomNode * parent )
{
    ASSERT_NODE_NOT_NULL;
#ifdef TRACE_AUTOBOX
    if ( getParentNode()!=NULL && parent != NULL )
        CRLog::trace("Changing parent of %d from %d to %d", getDataIndex(), getParentNode()->getDataIndex(), parent->getDataIndex());
#endif
    switch ( TNTYPE ) {
    case NT_ELEMENT:
        NPELEM->_parentNode = parent;
        break;
#if BUILD_LITE!=1
    case NT_PELEMENT:   // immutable (persistent) element node
        {
            lUInt32 parentIndex = parent->_handle._dataIndex;
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            if ( me->parentIndex != (int)parentIndex ) {
                me->parentIndex = parentIndex;
                modified();
            }
        }
        break;
    case NT_PTEXT:      // immutable (persistent) text node
        {
            lUInt32 parentIndex = parent->_handle._dataIndex;
            getDocument()->_textStorage.setParent(_data._ptext_addr, parentIndex);
            //_data._ptext_addr._parentIndex = parentIndex;
            //_document->_textStorage.setTextParent( _data._ptext_addr._addr, parentIndex );
        }
        break;
#endif
    case NT_TEXT:
        {
            lUInt32 parentIndex = parent->_handle._dataIndex;
            _data._text_ptr->setParentIndex( parentIndex );
        }
        break;
    }
}

/// returns dataIndex of node's parent, 0 if no parent
int ldomNode::getParentIndex() const
{
    ASSERT_NODE_NOT_NULL;

    switch ( TNTYPE ) {
    case NT_ELEMENT:
        return NPELEM->_parentNode ? NPELEM->_parentNode->getDataIndex() : 0;
#if BUILD_LITE!=1
    case NT_PELEMENT:   // immutable (persistent) element node
        {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            return me->parentIndex;
        }
        break;
    case NT_PTEXT:      // immutable (persistent) text node
        return getDocument()->_textStorage.getParent(_data._ptext_addr);
#endif
    case NT_TEXT:
        return _data._text_ptr->getParentIndex();
    }
    return 0;
}

/// returns pointer to parent node, NULL if node has no parent
ldomNode * ldomNode::getParentNode() const
{
    ASSERT_NODE_NOT_NULL;
    int parentIndex = 0;
    switch ( TNTYPE ) {
    case NT_ELEMENT:
        return NPELEM->_parentNode;
#if BUILD_LITE!=1
    case NT_PELEMENT:   // immutable (persistent) element node
        {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            parentIndex = me->parentIndex;
        }
        break;
    case NT_PTEXT:      // immutable (persistent) text node
        parentIndex = getDocument()->_textStorage.getParent(_data._ptext_addr);
        break;
#endif
    case NT_TEXT:
        parentIndex = _data._text_ptr->getParentIndex();
        break;
    }
    return parentIndex ? getTinyNode(parentIndex) : NULL;
}

/// returns true child node is element
bool ldomNode::isChildNodeElement( lUInt32 index ) const
{
    ASSERT_NODE_NOT_NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        int n = me->_children[index];
        return ( (n & 1)==1 );
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        int n = me->children[index];
        return ( (n & 1)==1 );
    }
#endif
}

/// returns true child node is text
bool ldomNode::isChildNodeText( lUInt32 index ) const
{
    ASSERT_NODE_NOT_NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        int n = me->_children[index];
        return ( (n & 1)==0 );
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        int n = me->children[index];
        return ( (n & 1)==0 );
    }
#endif
}

/// returns child node by index, NULL if node with this index is not element or nodeTag!=0 and element node name!=nodeTag
ldomNode * ldomNode::getChildElementNode( lUInt32 index, const lChar16 * nodeTag ) const
{
    lUInt16 nodeId = getDocument()->getElementNameIndex(nodeTag);
    return getChildElementNode( index, nodeId );
}

/// returns child node by index, NULL if node with this index is not element or nodeId!=0 and element node id!=nodeId
ldomNode * ldomNode::getChildElementNode( lUInt32 index, lUInt16 nodeId ) const
{
    ASSERT_NODE_NOT_NULL;
    ldomNode * res = NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        int n = me->_children[index];
        if ( (n & 1)==0 ) // not element
            return NULL;
        res = getTinyNode( n );
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        int n = me->children[index];
        if ( (n & 1)==0 ) // not element
            return NULL;
        res = getTinyNode( n );
    }
#endif
    if ( res && nodeId!=0 && res->getNodeId()!=nodeId )
        res = NULL;
    return res;
}

/// returns child node by index
ldomNode * ldomNode::getChildNode( lUInt32 index ) const
{
    ASSERT_NODE_NOT_NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        return getTinyNode( me->_children[index] );
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return getTinyNode( me->children[index] );
    }
#endif
}

/// returns element child count
int ldomNode::getChildCount() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return 0;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        return me->_children.length();
#if BUILD_LITE!=1
    } else {
        // persistent element
        // persistent element
        {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
//            if ( me==NULL ) { // DEBUG
//                me = _document->_elemStorage.getElem( _data._pelem_addr );
//            }
            return me->childCount;
        }
    }
#endif
}

/// returns element attribute count
int ldomNode::getAttrCount() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return 0;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        return me->_attrs.length();
#if BUILD_LITE!=1
    } else {
        // persistent element
        {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            return me->attrCount;
        }
    }
#endif
}

/// returns attribute value by attribute name id and namespace id
const lString16 & ldomNode::getAttributeValue( lUInt16 nsid, lUInt16 id ) const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return lString16::empty_str;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        lUInt32 valueId = me->_attrs.get( nsid, id );
        if ( valueId==LXML_ATTR_VALUE_NONE )
            return lString16::empty_str;
        return getDocument()->getAttrValue(valueId);
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        lUInt32 valueId = me->getAttrValueId( nsid, id );
        if ( valueId==LXML_ATTR_VALUE_NONE )
            return lString16::empty_str;
        return getDocument()->getAttrValue(valueId);
    }
#endif
}

/// returns attribute value by attribute name and namespace
const lString16 & ldomNode::getAttributeValue( const lChar16 * nsName, const lChar16 * attrName ) const
{
    ASSERT_NODE_NOT_NULL;
    lUInt16 nsId = (nsName && nsName[0]) ? getDocument()->getNsNameIndex( nsName ) : LXML_NS_ANY;
    lUInt16 attrId = getDocument()->getAttrNameIndex( attrName );
    return getAttributeValue( nsId, attrId );
}

/// returns attribute value by attribute name and namespace
const lString16 & ldomNode::getAttributeValue( const lChar8 * nsName, const lChar8 * attrName ) const
{
    ASSERT_NODE_NOT_NULL;
    lUInt16 nsId = (nsName && nsName[0]) ? getDocument()->getNsNameIndex( nsName ) : LXML_NS_ANY;
    lUInt16 attrId = getDocument()->getAttrNameIndex( attrName );
    return getAttributeValue( nsId, attrId );
}

/// returns attribute by index
const lxmlAttribute * ldomNode::getAttribute( lUInt32 index ) const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        return me->_attrs[index];
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return me->attr( index );
    }
#endif
}

/// returns true if element node has attribute with specified name id and namespace id
bool ldomNode::hasAttribute( lUInt16 nsid, lUInt16 id ) const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return false;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        tinyElement * me = NPELEM;
        lUInt32 valueId = me->_attrs.get( nsid, id );
        return ( valueId!=LXML_ATTR_VALUE_NONE );
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return (me->findAttr( nsid, id ) != NULL);
    }
#endif
}

/// returns attribute name by index
const lString16 & ldomNode::getAttributeName( lUInt32 index ) const
{
    ASSERT_NODE_NOT_NULL;
    const lxmlAttribute * attr = getAttribute( index );
    if ( attr )
        return getDocument()->getAttrName( attr->id );
    return lString16::empty_str;
}

/// sets attribute value
void ldomNode::setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    lUInt32 valueIndex = getDocument()->getAttrValueIndex(value);
#if BUILD_LITE!=1
    if ( isPersistent() ) {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        lxmlAttribute * attr = me->findAttr( nsid, id );
        if ( attr ) {
            attr->index = valueIndex;
            modified();
            return;
        }
        // else: convert to modifable and continue as non-persistent
        modify();
    }
#endif
    // element
    tinyElement * me = NPELEM;
    me->_attrs.set(nsid, id, valueIndex);
    if (nsid == LXML_NS_NONE)
        getDocument()->onAttributeSet( id, valueIndex, this );
}

/// returns attribute value by attribute name id, looking at children if needed
const lString16 & ldomNode::getFirstInnerAttributeValue( lUInt16 nsid, lUInt16 id ) const
{
    ASSERT_NODE_NOT_NULL;
    if (hasAttribute(nsid, id))
        return getAttributeValue(nsid, id);
    ldomNode * n = (ldomNode *) this;
    if (n->isElement() && n->getChildCount() > 0) {
        int nextChildIndex = 0;
        n = n->getChildNode(nextChildIndex);
        while (true) {
            // Check only the first time we met a node (nextChildIndex == 0)
            // and not when we get back to it from a child to process next sibling
            if (nextChildIndex == 0) {
                if (n->isElement() && n->hasAttribute(nsid, id))
                    return n->getAttributeValue(nsid, id);
            }
            // Process next child
            if (n->isElement() && nextChildIndex < n->getChildCount()) {
                n = n->getChildNode(nextChildIndex);
                nextChildIndex = 0;
                continue;
            }
            // No more child, get back to parent and have it process our sibling
            nextChildIndex = n->getNodeIndex() + 1;
            n = n->getParentNode();
            if (!n) // back to root node
                break;
            if (n == this && nextChildIndex >= n->getChildCount())
                // back to this node, and done with its children
                break;
        }
    }
    return lString16::empty_str;
}

/// returns element type structure pointer if it was set in document for this element name
const css_elem_def_props_t * ldomNode::getElementTypePtr()
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        const css_elem_def_props_t * res = getDocument()->getElementTypePtr(NPELEM->_id);
//        if ( res && res->is_object ) {
//            CRLog::trace("Object found");
//        }
        return res;
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        const css_elem_def_props_t * res = getDocument()->getElementTypePtr(me->id);
//        if ( res && res->is_object ) {
//            CRLog::trace("Object found");
//        }
        return res;
    }
#endif
}

/// returns element name id
lUInt16 ldomNode::getNodeId() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return 0;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
        // element
#endif
        return NPELEM->_id;
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return me->id;
    }
#endif
}

/// returns element namespace id
lUInt16 ldomNode::getNodeNsId() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return 0;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
        // element
#endif
        return NPELEM->_nsid;
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return me->nsid;
    }
#endif
}

/// replace element name id with another value
void ldomNode::setNodeId( lUInt16 id )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
        // element
#endif
        NPELEM->_id = id;
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        me->id = id;
        modified();
    }
#endif
}

/// returns element name
const lString16 & ldomNode::getNodeName() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return lString16::empty_str;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
        // element
#endif
        return getDocument()->getElementName(NPELEM->_id);
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return getDocument()->getElementName(me->id);
    }
#endif
}

/// returns element name
bool ldomNode::isNodeName(const char * s) const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return false;
    lUInt16 index = getDocument()->findElementNameIndex(s);
    if (!index)
        return false;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
        // element
#endif
        return index == NPELEM->_id;
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return index == me->id;
    }
#endif
}

/// returns element namespace name
const lString16 & ldomNode::getNodeNsName() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return lString16::empty_str;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
#endif
        // element
        return getDocument()->getNsName(NPELEM->_nsid);
#if BUILD_LITE!=1
    } else {
        // persistent element
        ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
        return getDocument()->getNsName(me->nsid);
    }
#endif
}



/// returns text node text as wide string
lString16 ldomNode::getText( lChar16 blockDelimiter, int maxSize ) const
{
    ASSERT_NODE_NOT_NULL;
    switch ( TNTYPE ) {
#if BUILD_LITE!=1
    case NT_PELEMENT:
#endif
    case NT_ELEMENT:
        {
            lString16 txt;
            unsigned cc = getChildCount();
            for ( unsigned i=0; i<cc; i++ ) {
                ldomNode * child = getChildNode(i);
                txt += child->getText(blockDelimiter, maxSize);
                if (maxSize != 0 && txt.length() > maxSize)
                    break;
                if (i >= cc - 1)
                    break;
#if BUILD_LITE!=1
                if ( blockDelimiter && child->isElement() ) {
                    if ( !child->getStyle().isNull() && child->getStyle()->display == css_d_block )
                        txt << blockDelimiter;
                }
#endif
            }
            return txt;
        }
        break;
#if BUILD_LITE!=1
    case NT_PTEXT:
        return Utf8ToUnicode(getDocument()->_textStorage.getText( _data._ptext_addr ));
#endif
    case NT_TEXT:
        return _data._text_ptr->getText16();
    }
    return lString16::empty_str;
}

/// returns text node text as utf8 string
lString8 ldomNode::getText8( lChar8 blockDelimiter, int maxSize ) const
{
    ASSERT_NODE_NOT_NULL;
    switch ( TNTYPE ) {
    case NT_ELEMENT:
#if BUILD_LITE!=1
    case NT_PELEMENT:
        {
            lString8 txt;
            int cc = getChildCount();
            for (int i = 0; i < cc; i++) {
                ldomNode * child = getChildNode(i);
                txt += child->getText8(blockDelimiter, maxSize);
                if (maxSize != 0 && txt.length() > maxSize)
                    break;
                if (i >= getChildCount() - 1)
                    break;
                if ( blockDelimiter && child->isElement() ) {
                    if ( child->getStyle()->display == css_d_block )
                        txt << blockDelimiter;
                }
            }
            return txt;
        }
        break;
    case NT_PTEXT:
        return getDocument()->_textStorage.getText( _data._ptext_addr );
#endif
    case NT_TEXT:
        return _data._text_ptr->getText();
    }
    return lString8::empty_str;
}

/// sets text node text as wide string
void ldomNode::setText( lString16 str )
{
    ASSERT_NODE_NOT_NULL;
    switch ( TNTYPE ) {
    case NT_ELEMENT:
        readOnlyError();
        break;
#if BUILD_LITE!=1
    case NT_PELEMENT:
        readOnlyError();
        break;
    case NT_PTEXT:
        {
            // convert persistent text to mutable
            lUInt32 parentIndex = getDocument()->_textStorage.getParent(_data._ptext_addr);
            getDocument()->_textStorage.freeNode( _data._ptext_addr );
            _data._text_ptr = new ldomTextNode( parentIndex, UnicodeToUtf8(str) );
            // change type from PTEXT to TEXT
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_TEXT;
        }
        break;
#endif
    case NT_TEXT:
        {
            _data._text_ptr->setText( str );
        }
        break;
    }
}

/// sets text node text as utf8 string
void ldomNode::setText8( lString8 utf8 )
{
    ASSERT_NODE_NOT_NULL;
    switch ( TNTYPE ) {
    case NT_ELEMENT:
        readOnlyError();
        break;
#if BUILD_LITE!=1
    case NT_PELEMENT:
        readOnlyError();
        break;
    case NT_PTEXT:
        {
            // convert persistent text to mutable
            lUInt32 parentIndex = getDocument()->_textStorage.getParent(_data._ptext_addr);
            getDocument()->_textStorage.freeNode( _data._ptext_addr );
            _data._text_ptr = new ldomTextNode( parentIndex, utf8 );
            // change type from PTEXT to TEXT
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_TEXT;
        }
        break;
#endif
    case NT_TEXT:
        {
            _data._text_ptr->setText( utf8 );
        }
        break;
    }
}

#if BUILD_LITE!=1
/// returns node absolute rectangle
void ldomNode::getAbsRect( lvRect & rect, bool inner )
{
    ASSERT_NODE_NOT_NULL;
    ldomNode * node = this;
    RenderRectAccessor fmt( node );
    rect.left = fmt.getX();
    rect.top = fmt.getY();
    rect.right = fmt.getWidth();
    rect.bottom = fmt.getHeight();
    if ( inner && RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET) ) {
        // This flag is set only when in enhanced rendering mode, and
        // only on erm_final-like nodes.
        rect.left += fmt.getInnerX();     // add padding left
        rect.top += fmt.getInnerY();      // add padding top
        rect.right = fmt.getInnerWidth(); // replace by inner width
    }
    node = node->getParentNode();
    for (; node; node = node->getParentNode())
    {
        RenderRectAccessor fmt( node );
        rect.left += fmt.getX();
        rect.top += fmt.getY();
        if ( RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET) ) {
            // getAbsRect() is mostly used on erm_final nodes. So,
            // if we meet another erm_final node in our parent, we are
            // probably an embedded floatBox or inlineBox. Embedded
            // floatBoxes or inlineBoxes are positioned according
            // to the inner LFormattedText, so we need to account
            // for these padding shifts.
            rect.left += fmt.getInnerX();     // add padding left
            rect.top += fmt.getInnerY();      // add padding top
        }
    }
    rect.bottom += rect.top;
    rect.right += rect.left;
}

/// returns render data structure
void ldomNode::getRenderData( lvdomElementFormatRec & dst)
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() ) {
        dst.clear();
        return;
    }
    getDocument()->_rectStorage.getRendRectData(_handle._dataIndex, &dst);
}

/// sets new value for render data structure
void ldomNode::setRenderData( lvdomElementFormatRec & newData)
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    getDocument()->_rectStorage.setRendRectData(_handle._dataIndex, &newData);
}

/// sets node rendering structure pointer
void ldomNode::clearRenderData()
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    lvdomElementFormatRec rec;
    getDocument()->_rectStorage.setRendRectData(_handle._dataIndex, &rec);
}
#endif

/// calls specified function recursively for all elements of DOM tree, children before parent
void ldomNode::recurseElementsDeepFirst( void (*pFun)( ldomNode * node ) )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    int cnt = getChildCount();
    for (int i=0; i<cnt; i++)
    {
        ldomNode * child = getChildNode( i );
        if ( child && child->isElement() )
        {
            child->recurseElementsDeepFirst( pFun );
        }
    }
    pFun( this );
}

#if BUILD_LITE!=1
static void updateRendMethod( ldomNode * node )
{
    node->initNodeRendMethod();
    // Also clean up node previous positionings (they were set while in
    // a previous page drawing phase), that could otherwise have negative
    // impact on the coming rendering (noticeable with table elements).
    RenderRectAccessor fmt( node );
    fmt.clear();
    fmt.push();
}

/// init render method for the whole subtree
void ldomNode::initNodeRendMethodRecursive()
{
    recurseElementsDeepFirst( updateRendMethod );
}
#endif

#if 0
static void updateStyleData( ldomNode * node )
{
    if ( node->getNodeId()==el_DocFragment )
        node->applyNodeStylesheet();
    node->initNodeStyle();
}
#endif

#if BUILD_LITE!=1
static void updateStyleDataRecursive( ldomNode * node, LVDocViewCallback * progressCallback, int & lastProgressPercent )
{
    if ( !node->isElement() )
        return;
    bool styleSheetChanged = false;

    // DocFragment (for epub) and body (for html) may hold some stylesheet
    // as first child or a link to stylesheet file in attribute
    if ( node->getNodeId()==el_DocFragment || node->getNodeId()==el_body ) {
        styleSheetChanged = node->applyNodeStylesheet();
        // We don't have access to much metric to show the progress of
        // this recursive phase. Do that anyway as we progress among
        // the collection of DocFragments.
        if ( progressCallback && node->getNodeId()==el_DocFragment ) {
            int nbDocFragments = node->getParentNode()->getChildCount();
            if (nbDocFragments == 0) // should not happen (but avoid clang-tidy warning)
                nbDocFragments = 1;
            int percent = 100 * node->getNodeIndex() / nbDocFragments;
            if ( percent != lastProgressPercent ) {
                progressCallback->OnNodeStylesUpdateProgress( percent );
                lastProgressPercent = percent;
            }
        }
    }

    node->initNodeStyle();
    int n = node->getChildCount();
    for ( int i=0; i<n; i++ ) {
        ldomNode * child = node->getChildNode(i);
        if ( child->isElement() )
            updateStyleDataRecursive( child, progressCallback, lastProgressPercent );
    }
    if ( styleSheetChanged )
        node->getDocument()->getStyleSheet()->pop();
}

/// init render method for the whole subtree
void ldomNode::initNodeStyleRecursive( LVDocViewCallback * progressCallback )
{
    if (progressCallback)
        progressCallback->OnNodeStylesUpdateStart();
    getDocument()->_fontMap.clear();
    int lastProgressPercent = -1;
    updateStyleDataRecursive( this, progressCallback, lastProgressPercent );
    //recurseElements( updateStyleData );
    if (progressCallback)
        progressCallback->OnNodeStylesUpdateEnd();
}
#endif

/// calls specified function recursively for all elements of DOM tree
void ldomNode::recurseElements( void (*pFun)( ldomNode * node ) )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    pFun( this );
    int cnt = getChildCount();
    for (int i=0; i<cnt; i++)
    {
        ldomNode * child = getChildNode( i );
        if ( child->isElement() )
        {
            child->recurseElements( pFun );
        }
    }
}

/// calls specified function recursively for all elements of DOM tree
void ldomNode::recurseMatchingElements( void (*pFun)( ldomNode * node ), bool (*matchFun)( ldomNode * node ) )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    if ( !matchFun( this ) ) {
        return;
    }
    pFun( this );
    int cnt = getChildCount();
    for (int i=0; i<cnt; i++)
    {
        ldomNode * child = getChildNode( i );
        if ( child->isElement() )
        {
            child->recurseMatchingElements( pFun, matchFun );
        }
    }
}

/// calls specified function recursively for all nodes of DOM tree
void ldomNode::recurseNodes( void (*pFun)( ldomNode * node ) )
{
    ASSERT_NODE_NOT_NULL;
    pFun( this );
    if ( isElement() )
    {
        int cnt = getChildCount();
        for (int i=0; i<cnt; i++)
        {
            ldomNode * child = getChildNode( i );
            child->recurseNodes( pFun );
        }
    }
}

/// returns first text child element
ldomNode * ldomNode::getFirstTextChild(bool skipEmpty)
{
    ASSERT_NODE_NOT_NULL;
    if ( isText() ) {
        if ( !skipEmpty )
            return this;
        lString16 txt = getText();
        bool nonSpaceFound = false;
        for ( int i=0; i<txt.length(); i++ ) {
            lChar16 ch = txt[i];
            if ( ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n' ) {
                nonSpaceFound = true;
                break;
            }
        }
        if ( nonSpaceFound )
            return this;
        return NULL;
    }
    for ( int i=0; i<(int)getChildCount(); i++ ) {
        ldomNode * p = getChildNode(i)->getFirstTextChild(skipEmpty);
        if (p)
            return p;
    }
    return NULL;
}

/// returns last text child element
ldomNode * ldomNode::getLastTextChild()
{
    ASSERT_NODE_NOT_NULL;
    if ( isText() )
        return this;
    else {
        for ( int i=(int)getChildCount()-1; i>=0; i-- ) {
            ldomNode * p = getChildNode(i)->getLastTextChild();
            if (p)
                return p;
        }
    }
    return NULL;
}


#if BUILD_LITE!=1
/// find node by coordinates of point in formatted document
ldomNode * ldomNode::elementFromPoint( lvPoint pt, int direction )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return NULL;
    ldomNode * enode = this;
    lvdom_element_render_method rm = enode->getRendMethod();
    if ( rm == erm_invisible ) {
        return NULL;
    }

    if ( rm == erm_inline ) {
        // We shouldn't meet erm_inline here, as our purpose is to return
        // a final node (so, the container of inlines), and not look further
        // (it's ldomDocument::createXPointer(pt) job to look at this final
        // node rendered content to find the exact text node and char at pt).
        // Except in the "pt.y is inside the box bottom overflow" case below,
        // and that box is erm_final (see there for more comments).
        // We should navigate all the erm_inline nodes, looking for
        // non-erm_inline ones that may be in that overflow and containt pt.
        // erm_inline nodes don't have a RenderRectAccessor(), so their x/y
        // shifts are 0, and any inner block node had its RenderRectAccessor
        // x/y offsets positioned related to the final block. So, no need
        // to shift pt: just recursively call elementFromPoint() as-is,
        // and we'll be recursively navigating inline nodes here.
        int count = getChildCount();
        for ( int i=0; i<count; i++ ) {
            ldomNode * p = getChildNode( i );
            ldomNode * e = p->elementFromPoint( pt, direction );
            if ( e ) // found it!
                return e;
        }
        return NULL; // nothing found
    }

    RenderRectAccessor fmt( this );

    if ( BLOCK_RENDERING_G(ENHANCED) ) {
        // In enhanced rendering mode, because of collpasing of vertical margins
        // and the fact that we did not update style margins to their computed
        // values, a children box with margins can overlap its parent box, if
        // the child bigger margin collapsed with the parent smaller margin.
        // So, if we ignore the margins, there can be holes along the vertical
        // axis (these holes are the collapsed margins). But the content boxes
        // (without margins) don't overlap.
        if ( direction >= PT_DIR_EXACT ) { // PT_DIR_EXACT or PT_DIR_SCAN_FORWARD*
            // We get the parent node's children in ascending order
            // It could just be:
            //   if ( pt.y >= fmt.getY() + fmt.getHeight() )
            //       // Box fully before pt.y: not a candidate, next one may be
            //       return NULL;
            // but, because of possible floats overflowing their container element,
            // and we want to check if pt is inside any of them, we directly
            // check with bottom overflow included (just to avoid 2 tests
            // in the most common case when there is no overflow).
            if ( pt.y >= fmt.getY() + fmt.getHeight() + fmt.getBottomOverflow() ) {
                // Box (with overflow) fully before pt.y: not a candidate, next one may be
                return NULL;
            }
            if ( pt.y >= fmt.getY() + fmt.getHeight() ) { // pt.y is inside the box bottom overflow
                // Get back absolute coordinates of pt
                lvRect rc;
                getParentNode()->getAbsRect( rc );
                lvPoint pt0 = lvPoint(rc.left+pt.x, rc.top+pt.y );
                // Check each of this element's children if pt is inside it (so, we'll
                // go by here for each of them that has some overflow too, and that
                // contributed to making this element's overflow.)
                // Note that if this node is erm_final, its bottom overflow must have
                // been set by some inner embedded float. But this final block's children
                // are erm_inline, and the float might be deep among inlines' children.
                // erm_inline nodes don't have their RenderRectAccessor set, so the
                // bottom overflow is not propagated thru them, and we would be in
                // the above case ("Box (with overflow) fully before pt.y"), not
                // looking at inlines' children. We handle this case above (at the
                // start of this function) by looking at erm_inline's children for
                // non-erm_inline nodes before checking any x/y or bottom overflow.
                int count = getChildCount();
                for ( int i=0; i<count; i++ ) {
                    ldomNode * p = getChildNode( i );
                    // Find an inner erm_final element that has pt in it: for now, it can
                    // only be a float. Use PT_DIR_EXACT to really check for x boundaries.
                    ldomNode * e = p->elementFromPoint( lvPoint(pt.x-fmt.getX(), pt.y-fmt.getY()), PT_DIR_EXACT );
                    if ( e ) {
                        // Just to be sure, as elementFromPoint() may be a bit fuzzy in its
                        // checks, double check that pt is really inside that e rect.
                        lvRect erc;
                        e->getAbsRect( erc );
                        if ( erc.isPointInside(pt0) ) {
                            return e; // return this inner erm_final
                        }
                    }
                }
                return NULL; // Nothing found in the overflow
            }
            // There is one special case to skip: floats that may have been
            // positioned after their normal y (because of clear:, or because
            // of not enough width). Their following non-float siblings (after
            // in the HTML/DOM tree) may have a lower fmt.getY().
            if ( isFloatingBox() && pt.y < fmt.getY() ) {
                // Float starts after pt.y: next non-float siblings may contain pt.y
                return NULL;
            }
            // pt.y is inside the box (without overflows), go on with it.
            // Note: we don't check for next elements which may have a top
            // overflow and have pt.y inside it, because it would be a bit
            // more twisted here, and it's less common that floats overflow
            // their container's top (they need to have negative margins).
        }
        else { // PT_DIR_SCAN_BACKWARD*
            // We get the parent node's children in descending order
            if ( pt.y < fmt.getY() ) {
                // Box fully before pt.y: not a candidate, next one may be
                return NULL;
            }
        }
    }
    else {
        // In legacy rendering mode, all boxes (with their margins added) touch
        // each other, and the boxes of children are fully contained (with
        // their margins added) in their parent box.

        // Styles margins set on <TR>, <THEAD> and the like are ignored
        // by table layout algorithm (as per CSS specs)
        bool ignore_margins = ( rm == erm_table_row || rm == erm_table_row_group ||
                                rm == erm_table_header_group || rm == erm_table_footer_group );

        int top_margin = ignore_margins ? 0 : lengthToPx(enode->getStyle()->margin[2], fmt.getWidth(), enode->getFont()->getSize());
        if ( pt.y < fmt.getY() - top_margin) {
            if ( direction >= PT_DIR_SCAN_FORWARD && (rm == erm_final || rm == erm_list_item || rm == erm_table_caption) )
                return this;
            return NULL;
        }
        int bottom_margin = ignore_margins ? 0 : lengthToPx(enode->getStyle()->margin[3], fmt.getWidth(), enode->getFont()->getSize());
        if ( pt.y >= fmt.getY() + fmt.getHeight() + bottom_margin ) {
            if ( direction <= PT_DIR_SCAN_BACKWARD && (rm == erm_final || rm == erm_list_item || rm == erm_table_caption) )
                return this;
            return NULL;
        }
    }

    if ( direction == PT_DIR_EXACT ) {
        // (We shouldn't check for pt.x when we are given PT_DIR_SCAN_*.
        // In full text search, we might not find any and get locked
        // on some page.)
        if ( pt.x >= fmt.getX() + fmt.getWidth() ) {
            return NULL;
        }
        if ( pt.x < fmt.getX() ) {
            return NULL;
        }
        // We now do this above check in all cases.
        // Previously:
        //
        //   We also don't need to do it if pt.x=0, which is often used
        //   to get current page top or range xpointers.
        //   We are given a x>0 when tap/hold to highlight text or find
        //   a link, and checking x vs fmt.x and width allows for doing
        //   that correctly in 2nd+ table cells.
        //
        //   No need to check if ( pt.x < fmt.getX() ): we probably
        //   meet the multiple elements that can be formatted on a same
        //   line in the order they appear as children of their parent,
        //   we can simply just ignore those who end before our pt.x.
        //   But check x if we happen to be on a floating node (which,
        //   with float:right, can appear first in the DOM but be
        //   displayed at a higher x)
        //    if ( pt.x < fmt.getX() && enode->isFloatingBox() ) {
        //        return NULL;
        //    }
        // This is no more true, now that we support RTL tables and
        // we can meet cells in the reverse of their logical order.
        // We could add more conditions (like parentNode->getRendMethod()>=erm_table),
        // but let's just check this in all cases when direction=0.
    }
    if ( rm == erm_final || rm == erm_list_item || rm == erm_table_caption ) {
        // Final node, that's what we looked for
        return this;
    }
    // Not a final node, but a block container node that must contain
    // the final node we look for: check its children.
    int count = getChildCount();
    if ( direction >= PT_DIR_EXACT ) { // PT_DIR_EXACT or PT_DIR_SCAN_FORWARD*
        for ( int i=0; i<count; i++ ) {
            ldomNode * p = getChildNode( i );
            ldomNode * e = p->elementFromPoint( lvPoint(pt.x-fmt.getX(), pt.y-fmt.getY()), direction );
            if ( e )
                return e;
        }
    } else {
        for ( int i=count-1; i>=0; i-- ) {
            ldomNode * p = getChildNode( i );
            ldomNode * e = p->elementFromPoint( lvPoint(pt.x-fmt.getX(), pt.y-fmt.getY()), direction );
            if ( e )
                return e;
        }
    }
    return this;
}

/// find final node by coordinates of point in formatted document
ldomNode * ldomNode::finalBlockFromPoint( lvPoint pt )
{
    ASSERT_NODE_NOT_NULL;
    ldomNode * elem = elementFromPoint( pt, PT_DIR_EXACT );
    if ( elem && elem->getRendMethod() == erm_final )
        return elem;
    return NULL;
}
#endif

/// returns rendering method
lvdom_element_render_method ldomNode::getRendMethod()
{
    ASSERT_NODE_NOT_NULL;
    if ( isElement() ) {
#if BUILD_LITE!=1
        if ( !isPersistent() ) {
#endif
            return NPELEM->_rendMethod;
#if BUILD_LITE!=1
        } else {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            return (lvdom_element_render_method)me->rendMethod;
        }
#endif
    }
    return erm_invisible;
}

/// sets rendering method
void ldomNode::setRendMethod( lvdom_element_render_method method )
{
    ASSERT_NODE_NOT_NULL;
    if ( isElement() ) {
#if BUILD_LITE!=1
        if ( !isPersistent() ) {
#endif
            NPELEM->_rendMethod = method;
#if BUILD_LITE!=1
        } else {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            if ( me->rendMethod != method ) {
                me->rendMethod = (lUInt8)method;
                modified();
            }
        }
#endif
    }
}

#if BUILD_LITE!=1
/// returns element style record
css_style_ref_t ldomNode::getStyle() const
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return css_style_ref_t();
    css_style_ref_t res = getDocument()->getNodeStyle( _handle._dataIndex );
    return res;
}

/// returns element font
font_ref_t ldomNode::getFont()
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return font_ref_t();
    return getDocument()->getNodeFont( _handle._dataIndex );
}

/// sets element font
void ldomNode::setFont( font_ref_t font )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        getDocument()->setNodeFont( _handle._dataIndex, font );
    }
}

/// sets element style record
void ldomNode::setStyle( css_style_ref_t & style )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        getDocument()->setNodeStyle( _handle._dataIndex, style );
    }
}

bool ldomNode::initNodeFont()
{
    if ( !isElement() )
        return false;
    lUInt16 style = getDocument()->getNodeStyleIndex( _handle._dataIndex );
    lUInt16 font = getDocument()->getNodeFontIndex( _handle._dataIndex );
    lUInt16 fntIndex = getDocument()->_fontMap.get( style );
    if ( fntIndex==0 ) {
        css_style_ref_t s = getDocument()->_styles.get( style );
        if ( s.isNull() ) {
            CRLog::error("style not found for index %d", style);
            s = getDocument()->_styles.get( style );
        }
        LVFontRef fnt = ::getFont(s.get(), getDocument()->getFontContextDocIndex());
        fntIndex = (lUInt16)getDocument()->_fonts.cache( fnt );
        if ( fnt.isNull() ) {
            CRLog::error("font not found for style!");
            return false;
        } else {
            getDocument()->_fontMap.set(style, fntIndex);
        }
        if ( font != 0 ) {
            if ( font!=fntIndex ) // ???
                getDocument()->_fonts.release(font);
        }
        getDocument()->setNodeFontIndex( _handle._dataIndex, fntIndex);
        return true;
    } else {
        if ( font!=fntIndex )
            getDocument()->_fonts.addIndexRef( fntIndex );
    }
    if ( fntIndex<=0 ) {
        CRLog::error("font caching failed for style!");
        return false;;
    } else {
        getDocument()->setNodeFontIndex( _handle._dataIndex, fntIndex);
    }
    return true;
}

void ldomNode::initNodeStyle()
{
    // assume all parent styles already initialized
    if ( !getDocument()->isDefStyleSet() )
        return;
    if ( isElement() ) {
        if ( isRoot() || getParentNode()->isRoot() )
        {
            setNodeStyle( this,
                getDocument()->getDefaultStyle(),
                getDocument()->getDefaultFont()
            );
        }
        else
        {
            ldomNode * parent = getParentNode();

            // DEBUG TEST
            if ( parent->getChildIndex( getDataIndex() )<0 ) {
                CRLog::error("Invalid parent->child relation for nodes %d->%d", parent->getDataIndex(), getDataIndex() );
            }


            //lvdomElementFormatRec * parent_fmt = node->getParentNode()->getRenderData();
            css_style_ref_t style = parent->getStyle();
            LVFontRef font = parent->getFont();
#if DEBUG_DOM_STORAGE==1
            if ( style.isNull() ) {
                // for debugging
                CRLog::error("NULL style is returned for node <%s> %d level=%d  "
                             "parent <%s> %d level=%d children %d childIndex=%d",
                             LCSTR(getNodeName()), getDataIndex(), getNodeLevel(),
                             LCSTR(parent->getNodeName()), parent->getDataIndex(),
                             parent->getNodeLevel(), parent->getChildCount(), parent->getChildIndex(getDataIndex()) );

                style = parent->getStyle();
            }
#endif
            setNodeStyle( this,
                style,
                font
                );
#if DEBUG_DOM_STORAGE==1
            if ( this->getStyle().isNull() ) {
                CRLog::error("NULL style is set for <%s>", LCSTR(getNodeName()) );
                style = this->getStyle();
            }
#endif
        }
    }
}
#endif

bool ldomNode::isBoxingNode( bool orPseudoElem ) const
{
    if( isElement() ) {
        lUInt16 id = getNodeId();
        if( id >= el_autoBoxing && id <= el_inlineBox ) {
            return true;
        }
        if ( orPseudoElem && id == el_pseudoElem ) {
            return true;
        }
    }
    return false;
}

ldomNode * ldomNode::getUnboxedParent() const
{
    ldomNode * parent = getParentNode();
    while ( parent && parent->isBoxingNode() )
        parent = parent->getParentNode();
    return parent;
}

// The following 4 methods are mostly used when checking CSS siblings/child
// rules and counting list items siblings: we have them skip pseudoElems by
// using isBoxingNode(orPseudoElem=true).
ldomNode * ldomNode::getUnboxedFirstChild( bool skip_text_nodes ) const
{
    for ( int i=0; i<getChildCount(); i++ ) {
        ldomNode * child = getChildNode(i);
        if ( child && child->isBoxingNode(true) ) {
            child = child->getUnboxedFirstChild( skip_text_nodes );
            // (child will then be NULL if it was a pseudoElem)
        }
        if ( child && (!skip_text_nodes || !child->isText()) )
            return child;
    }
    return NULL;
}

ldomNode * ldomNode::getUnboxedLastChild( bool skip_text_nodes ) const
{
    for ( int i=getChildCount()-1; i>=0; i-- ) {
        ldomNode * child = getChildNode(i);
        if ( child && child->isBoxingNode(true) ) {
            child = child->getUnboxedLastChild( skip_text_nodes );
        }
        if ( child && (!skip_text_nodes || !child->isText()) )
            return child;
    }
    return NULL;
}

/* For reference, a non-recursive node subtree walker:
    ldomNode * n = topNode;
    if ( n && n->getChildCount() > 0 ) {
        int index = 0;
        n = n->getChildNode(index);
        while ( true ) {
            // Check the node only the first time we meet it (index == 0) and
            // not when we get back to it from a child to process next sibling
            if ( index == 0 ) {
                // Check n, process it, return it...
            }
            // Process next child
            if ( index < n->getChildCount() ) {
                n = n->getChildNode(index);
                index = 0;
                continue;
            }
            // No more child, get back to parent and have it process our sibling
            index = n->getNodeIndex() + 1;
            n = n->getParentNode();
            if ( n == topNode ) // all children done and back to top node
                break;
        }
    }
*/

ldomNode * ldomNode::getUnboxedNextSibling( bool skip_text_nodes ) const
{
    // We use a variation of the above non-recursive node subtree walker,
    // but with an arbitrary starting node (this) inside the unboxed_parent
    // tree, and checks to not walk down non-boxing nodes - but still
    // walking up any node (which ought to be a boxing node).
    ldomNode * unboxed_parent = getUnboxedParent(); // don't walk outside of it
    ldomNode * n = (ldomNode *) this;
    int index = 0;
    bool node_entered = true; // bootstrap loop
    // We may meet a same node as 'n' multiple times:
    // - once with node_entered=false and index being its real position inside
    //   its parent children collection, and we'll be "entering" it
    // - once with node_entered=true and index=0, meaning we have "entered" it to
    //   check if it's a candidate, and to possibly go on checking its own children.
    // - once when back from its children, with node_entered=false and index
    //   being that previous child index + 1, to go process its next sibling
    //   (or parent if no more sibling)
    while ( true ) {
        // printf("      %s\n", LCSTR(ldomXPointer(n,0).toStringV1()));
        if ( node_entered && n != this ) { // Don't check the starting node
            // Check if this node is a candidate
            if ( n->isText() ) { // Text nodes are not boxing nodes
                if ( !skip_text_nodes )
                    return n;
            }
            else if ( !n->isBoxingNode(true) ) // Not a boxing node nor pseudoElem
                return n;
            // Otherwise, this node is a boxing node (or a text node or a pseudoElem
            // with no child, and we'll get back to its parent)
        }
        // Enter next node, and re-loop to have it checked
        // - if !node_entered : n is the parent and index points to the next child
        //   we want to check
        // - if n->isBoxingNode() (and node_entered=true, and index=0): enter the first
        //   child of this boxingNode (not if pseudoElem, that doesn't box anything)
        if ( (!node_entered || n->isBoxingNode()) && index < n->getChildCount() ) {
            n = n->getChildNode(index);
            index = 0;
            node_entered = true;
            continue;
        }
        // No more sibling/child to check, get back to parent and have it
        // process n's next sibling
        index = n->getNodeIndex() + 1;
        n = n->getParentNode();
        node_entered = false;
        if ( n == unboxed_parent && index >= n->getChildCount() ) {
            // back to real parent node and no more child to check
            break;
        }
    }
    return NULL;
}

ldomNode * ldomNode::getUnboxedPrevSibling( bool skip_text_nodes ) const
{
    // Similar to getUnboxedNextSibling(), but walking backward
    ldomNode * unboxed_parent = getUnboxedParent();
    ldomNode * n = (ldomNode *) this;
    int index = 0;
    bool node_entered = true; // bootstrap loop
    while ( true ) {
        // printf("      %s\n", LCSTR(ldomXPointer(n,0).toStringV1()));
        if ( node_entered && n != this ) {
            if ( n->isText() ) {
                if ( !skip_text_nodes )
                    return n;
            }
            else if ( !n->isBoxingNode(true) )
                return n;
        }
        if ( (!node_entered || n->isBoxingNode()) && index >= 0 && index < n->getChildCount() ) {
            n = n->getChildNode(index);
            index = n->getChildCount() - 1;
            node_entered = true;
            continue;
        }
        index = n->getNodeIndex() - 1;
        n = n->getParentNode();
        node_entered = false;
        if ( n == unboxed_parent && index < 0 ) {
            break;
        }
    }
    return NULL;
}

/// for display:list-item node, get marker
bool ldomNode::getNodeListMarker( int & counterValue, lString16 & marker, int & markerWidth )
{
#if BUILD_LITE!=1
    css_style_ref_t s = getStyle();
    marker.clear();
    markerWidth = 0;
    if ( s.isNull() )
        return false;
    css_list_style_type_t st = s->list_style_type;
    switch ( st ) {
    default:
        // treat default as disc
    case css_lst_disc:
        marker = L"\x2022"; //L"\x25CF"; // 25e6
        break;
    case css_lst_circle:
        marker = L"\x2022"; //L"\x25CB";
        break;
    case css_lst_square:
        marker = L"\x25A0";
        break;
    case css_lst_none:
        // When css_lsp_inside, no space is used by the invisible marker
        if ( s->list_style_position != css_lsp_inside ) {
            marker = L"\x0020";
        }
        break;
    case css_lst_decimal:
    case css_lst_lower_roman:
    case css_lst_upper_roman:
    case css_lst_lower_alpha:
    case css_lst_upper_alpha:
        if ( counterValue<=0 ) {
            // calculate counter
            // The UL > LI parent-child chain may have had some of our Boxing elements inserted
            ldomNode * parent = getUnboxedParent();
            counterValue = 0;
            // See if parent has a 'start' attribute that overrides this 0
            // https://www.w3.org/TR/html5/grouping-content.html#the-ol-element
            // "The start attribute, if present, must be a valid integer giving the ordinal value of the first list item."
            lString16 value = parent->getAttributeValue(attr_start);
            if ( !value.empty() ) {
                int ivalue;
                if (value.atoi(ivalue))
                    counterValue = ivalue - 1;
            }
            // iterate parent's real children from start up to this node
            ldomNode * sibling = parent->getUnboxedFirstChild(true);
            while ( sibling ) {
                css_style_ref_t cs = sibling->getStyle();
                if ( cs.isNull() ) { // Should not happen, but let's be sure
                    if ( sibling == this )
                        break;
                    sibling = sibling->getUnboxedNextSibling(true);
                    continue;
                }
                if ( cs->display != css_d_list_item_block && cs->display != css_d_list_item) {
                    // Alien element among list item nodes, skip it to not mess numbering
                    if ( sibling == this ) // Should not happen, but let's be sure
                        break;
                    sibling = sibling->getUnboxedNextSibling(true);
                    continue;
                }
                switch ( cs->list_style_type ) {
                    case css_lst_decimal:
                    case css_lst_lower_roman:
                    case css_lst_upper_roman:
                    case css_lst_lower_alpha:
                    case css_lst_upper_alpha:
                        counterValue++;
                        break;
                    default:
                        // do nothing
                        ;
                }
                // See if it has a 'value' attribute that overrides the incremented value
                // https://www.w3.org/TR/html5/grouping-content.html#the-li-element
                // "The value attribute, if present, must be a valid integer giving the ordinal value of the list item."
                lString16 value = sibling->getAttributeValue(attr_value);
                if ( !value.empty() ) {
                    int ivalue;
                    if ( value.atoi(ivalue) )
                        counterValue = ivalue;
                }
                if ( sibling == this )
                    break;
                sibling = sibling->getUnboxedNextSibling(true); // skip text nodes
            }
        }
        else {
            counterValue++;
        }
        static const char * lower_roman[] = {"i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix",
                                             "x", "xi", "xii", "xiii", "xiv", "xv", "xvi", "xvii", "xviii", "xix",
                                         "xx", "xxi", "xxii", "xxiii"};
        if (counterValue > 0 || st == css_lst_decimal) {
            switch (st) {
            case css_lst_decimal:
                marker = lString16::itoa(counterValue);
                break;
            case css_lst_lower_roman:
                if (counterValue - 1 < (int)(sizeof(lower_roman) / sizeof(lower_roman[0])))
                    marker = lString16(lower_roman[counterValue-1]);
                else
                    marker = lString16::itoa(counterValue); // fallback to simple counter
                break;
            case css_lst_upper_roman:
                if (counterValue - 1 < (int)(sizeof(lower_roman) / sizeof(lower_roman[0])))
                    marker = lString16(lower_roman[counterValue-1]);
                else
                    marker = lString16::itoa(counterValue); // fallback to simple digital counter
                marker.uppercase();
                break;
            case css_lst_lower_alpha:
                if ( counterValue<=26 )
                    marker.append(1, (lChar16)('a' + counterValue - 1));
                else
                    marker = lString16::itoa(counterValue); // fallback to simple digital counter
                break;
            case css_lst_upper_alpha:
                if ( counterValue<=26 )
                    marker.append(1, (lChar16)('A' + counterValue - 1));
                else
                    marker = lString16::itoa(counterValue); // fallback to simple digital counter
                break;
            case css_lst_disc:
            case css_lst_circle:
            case css_lst_square:
            case css_lst_none:
            case css_lst_inherit:
                // do nothing
                break;
            }
        }
        break;
    }
    bool res = false;
    if ( !marker.empty() ) {
        LVFont * font = getFont().get();
        if ( font ) {
            TextLangCfg * lang_cfg = TextLangMan::getTextLangCfg( this );
            markerWidth = font->getTextWidth((marker + "  ").c_str(), marker.length()+2, lang_cfg) + font->getSize()/8;
            res = true;
        } else {
            marker.clear();
        }
    }
    return res;
#else
    marker = cs16("*");
    return true;
#endif
}


/// returns first child node
ldomNode * ldomNode::getFirstChild() const
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
#if BUILD_LITE!=1
        if ( !isPersistent() ) {
#endif
            tinyElement * me = NPELEM;
            if ( me->_children.length() )
                return getDocument()->getTinyNode(me->_children[0]);
#if BUILD_LITE!=1
        } else {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            if ( me->childCount )
                return getDocument()->getTinyNode(me->children[0]);
        }
#endif
    }
    return NULL;
}

/// returns last child node
ldomNode * ldomNode::getLastChild() const
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
#if BUILD_LITE!=1
        if ( !isPersistent() ) {
#endif
            tinyElement * me = NPELEM;
            if ( me->_children.length() )
                return getDocument()->getTinyNode(me->_children[me->_children.length()-1]);
#if BUILD_LITE!=1
        } else {
            ElementDataStorageItem * me = getDocument()->_elemStorage.getElem( _data._pelem_addr );
            if ( me->childCount )
                return getDocument()->getTinyNode(me->children[me->childCount-1]);
        }
#endif
    }
    return NULL;
}

/// removes and deletes last child element
void ldomNode::removeLastChild()
{
    ASSERT_NODE_NOT_NULL;
    if ( hasChildren() ) {
        ldomNode * lastChild = removeChild( getChildCount() - 1 );
        lastChild->destroy();
    }
}

/// add child
void ldomNode::addChild( lInt32 childNodeIndex )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    if ( isPersistent() )
        modify(); // convert to mutable element
    tinyElement * me = NPELEM;
    me->_children.add( childNodeIndex );
}

/// move range of children startChildIndex to endChildIndex inclusively to specified element
void ldomNode::moveItemsTo( ldomNode * destination, int startChildIndex, int endChildIndex )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return;
    if ( isPersistent() )
        modify();

#ifdef TRACE_AUTOBOX
    CRLog::debug( "moveItemsTo() invoked from %d to %d", getDataIndex(), destination->getDataIndex() );
#endif
    //if ( getDataIndex()==INDEX2 || getDataIndex()==INDEX1) {
    //    CRLog::trace("nodes from element %d are being moved", getDataIndex());
    //}
/*#ifdef _DEBUG
    if ( !_document->checkConsistency( false ) )
        CRLog::error("before moveItemsTo");
#endif*/
    int len = endChildIndex - startChildIndex + 1;
    tinyElement * me = NPELEM;
    for ( int i=0; i<len; i++ ) {
        ldomNode * item = getChildNode( startChildIndex );
        //if ( item->getDataIndex()==INDEX2 || item->getDataIndex()==INDEX1 ) {
        //    CRLog::trace("node %d is being moved", item->getDataIndex() );
        //}
        me->_children.remove( startChildIndex ); // + i
        item->setParentNode(destination);
        destination->addChild( item->getDataIndex() );
    }
    // TODO: renumber rest of children in necessary
/*#ifdef _DEBUG
    if ( !_document->checkConsistency( false ) )
        CRLog::error("after moveItemsTo");
#endif*/

}

/// find child element by tag id
ldomNode * ldomNode::findChildElement( lUInt16 nsid, lUInt16 id, int index )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return NULL;
    ldomNode * res = NULL;
    int k = 0;
    int childCount = getChildCount();
    for ( int i=0; i<childCount; i++ )
    {
        ldomNode * p = getChildNode( i );
        if ( !p->isElement() )
            continue;
        if ( p->getNodeId() == id && ( (p->getNodeNsId() == nsid) || (nsid==LXML_NS_ANY) ) )
        {
            if ( k==index || index==-1 ) {
                res = p;
                break;
            }
            k++;
        }
    }
    if (!res) //  || (index==-1 && k>1)  // DON'T CHECK WHETHER OTHER ELEMENTS EXIST
        return NULL;
    return res;
}

/// find child element by id path
ldomNode * ldomNode::findChildElement( lUInt16 idPath[] )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return NULL;
    ldomNode * elem = this;
    for ( int i=0; idPath[i]; i++ ) {
        elem = elem->findChildElement( LXML_NS_ANY, idPath[i], -1 );
        if ( !elem )
            return NULL;
    }
    return elem;
}

/// inserts child element
ldomNode * ldomNode::insertChildElement( lUInt32 index, lUInt16 nsid, lUInt16 id )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        if ( isPersistent() )
            modify();
        tinyElement * me = NPELEM;
        if (index>(lUInt32)me->_children.length())
            index = me->_children.length();
        ldomNode * node = getDocument()->allocTinyElement( this, nsid, id );
        me->_children.insert( index, node->getDataIndex() );
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child element
ldomNode * ldomNode::insertChildElement( lUInt16 id )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        if ( isPersistent() )
            modify();
        ldomNode * node = getDocument()->allocTinyElement( this, LXML_NS_NONE, id );
        NPELEM->_children.insert( NPELEM->_children.length(), node->getDataIndex() );
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child text
ldomNode * ldomNode::insertChildText( lUInt32 index, const lString16 & value )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        if ( isPersistent() )
            modify();
        tinyElement * me = NPELEM;
        if (index>(lUInt32)me->_children.length())
            index = me->_children.length();
#if !defined(USE_PERSISTENT_TEXT) || BUILD_LITE==1
        ldomNode * node = getDocument()->allocTinyNode( NT_TEXT );
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._text_ptr = new ldomTextNode(_handle._dataIndex, s8);
#else
        ldomNode * node = getDocument()->allocTinyNode( NT_PTEXT );
        //node->_data._ptext_addr._parentIndex = _handle._dataIndex;
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._ptext_addr = getDocument()->_textStorage.allocText( node->_handle._dataIndex, _handle._dataIndex, s8 );
#endif
        me->_children.insert( index, node->getDataIndex() );
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child text
ldomNode * ldomNode::insertChildText( const lString16 & value )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        if ( isPersistent() )
            modify();
        tinyElement * me = NPELEM;
#if !defined(USE_PERSISTENT_TEXT) || BUILD_LITE==1
        ldomNode * node = getDocument()->allocTinyNode( NT_TEXT );
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._text_ptr = new ldomTextNode(_handle._dataIndex, s8);
#else
        ldomNode * node = getDocument()->allocTinyNode( NT_PTEXT );
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._ptext_addr = getDocument()->_textStorage.allocText( node->_handle._dataIndex, _handle._dataIndex, s8 );
#endif
        me->_children.insert( me->_children.length(), node->getDataIndex() );
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child text
ldomNode * ldomNode::insertChildText(const lString8 & s8)
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        if ( isPersistent() )
            modify();
        tinyElement * me = NPELEM;
#if !defined(USE_PERSISTENT_TEXT) || BUILD_LITE==1
        ldomNode * node = getDocument()->allocTinyNode( NT_TEXT );
        node->_data._text_ptr = new ldomTextNode(_handle._dataIndex, s8);
#else
        ldomNode * node = getDocument()->allocTinyNode( NT_PTEXT );
        node->_data._ptext_addr = getDocument()->_textStorage.allocText( node->_handle._dataIndex, _handle._dataIndex, s8 );
#endif
        me->_children.insert( me->_children.length(), node->getDataIndex() );
        return node;
    }
    readOnlyError();
    return NULL;
}

/// remove child
ldomNode * ldomNode::removeChild( lUInt32 index )
{
    ASSERT_NODE_NOT_NULL;
    if  ( isElement() ) {
        if ( isPersistent() )
            modify();
        lUInt32 removedIndex = NPELEM->_children.remove(index);
        ldomNode * node = getTinyNode( removedIndex );
        return node;
    }
    readOnlyError();
    return NULL;
}

/// creates stream to read base64 encoded data from element
LVStreamRef ldomNode::createBase64Stream()
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return LVStreamRef();
#define DEBUG_BASE64_IMAGE 0
#if DEBUG_BASE64_IMAGE==1
    lString16 fname = getAttributeValue( attr_id );
    lString8 fname8 = UnicodeToUtf8( fname );
    LVStreamRef ostream = LVOpenFileStream( fname.empty() ? L"image.png" : fname.c_str(), LVOM_WRITE );
    printf("createBase64Stream(%s)\n", fname8.c_str());
#endif
    LVStream * stream = new LVBase64NodeStream( this );
    if ( stream->GetSize()==0 )
    {
#if DEBUG_BASE64_IMAGE==1
        printf("    cannot create base64 decoder stream!!!\n");
#endif
        delete stream;
        return LVStreamRef();
    }
    LVStreamRef istream( stream );

#if DEBUG_BASE64_IMAGE==1
    LVPumpStream( ostream, istream );
    istream->SetPos(0);
#endif

    return istream;
}

#if BUILD_LITE!=1

class NodeImageProxy : public LVImageSource
{
    ldomNode * _node;
    lString16 _refName;
    int _dx;
    int _dy;
public:
    NodeImageProxy( ldomNode * node, lString16 refName, int dx, int dy )
        : _node(node), _refName(refName), _dx(dx), _dy(dy)
    {

    }

    virtual ldomNode * GetSourceNode()
    {
        return NULL;
    }
    virtual LVStream * GetSourceStream()
    {
        return NULL;
    }

    virtual void   Compact() { }
    virtual int    GetWidth() { return _dx; }
    virtual int    GetHeight() { return _dy; }
    virtual bool   Decode( LVImageDecoderCallback * callback )
    {
        LVImageSourceRef img = _node->getDocument()->getObjectImageSource(_refName);
        if ( img.isNull() )
            return false;
        return img->Decode(callback);
    }
    virtual ~NodeImageProxy()
    {

    }
};

/// returns object image ref name
lString16 ldomNode::getObjectImageRefName(bool percentDecode)
{
    if (!isElement())
        return lString16::empty_str;
    //printf("ldomElement::getObjectImageSource() ... ");
    const css_elem_def_props_t * et = getDocument()->getElementTypePtr(getNodeId());
    if (!et || !et->is_object)
        return lString16::empty_str;
    lUInt16 hrefId = getDocument()->getAttrNameIndex("href");
    lUInt16 srcId = getDocument()->getAttrNameIndex("src");
    lUInt16 recIndexId = getDocument()->getAttrNameIndex("recindex");
    lString16 refName = getAttributeValue( getDocument()->getNsNameIndex("xlink"),
        hrefId );

    if ( refName.empty() )
        refName = getAttributeValue( getDocument()->getNsNameIndex("l"), hrefId );
    if ( refName.empty() )
        refName = getAttributeValue( LXML_NS_ANY, hrefId ); //LXML_NS_NONE
    if ( refName.empty() )
        refName = getAttributeValue( LXML_NS_ANY, srcId ); //LXML_NS_NONE
    if (refName.empty()) {
        lString16 recindex = getAttributeValue( LXML_NS_ANY, recIndexId );
        if (!recindex.empty()) {
            int n;
            if (recindex.atoi(n)) {
                refName = lString16(MOBI_IMAGE_NAME_PREFIX) + fmt::decimal(n);
                //CRLog::debug("get mobi image %s", LCSTR(refName));
            }
        }
//        else {
//            for (int k=0; k<getAttrCount(); k++) {
//                CRLog::debug("attr %s=%s", LCSTR(getAttributeName(k)), LCSTR(getAttributeValue(getAttributeName(k).c_str())));
//            }
//        }
    }
    if ( refName.length()<2 )
        return lString16::empty_str;
    if (percentDecode)
        refName = DecodeHTMLUrlString(refName);
    return refName;
}


/// returns object image stream
LVStreamRef ldomNode::getObjectImageStream()
{
    lString16 refName = getObjectImageRefName();
    if ( refName.empty() )
        return LVStreamRef();
    return getDocument()->getObjectImageStream( refName );
}


/// returns object image source
LVImageSourceRef ldomNode::getObjectImageSource()
{
    lString16 refName = getObjectImageRefName(true);
    LVImageSourceRef ref;
    if ( refName.empty() )
        return ref;
    ref = getDocument()->getObjectImageSource( refName );
    if (ref.isNull()) {
        // try again without percent decoding (for fb3)
        refName = getObjectImageRefName(false);
        if ( refName.empty() )
            return ref;
        ref = getDocument()->getObjectImageSource( refName );
    }
    if ( !ref.isNull() ) {
        int dx = ref->GetWidth();
        int dy = ref->GetHeight();
        ref = LVImageSourceRef( new NodeImageProxy(this, refName, dx, dy) );
    } else {
        CRLog::error("ObjectImageSource cannot be opened by name %s", LCSTR(refName));
    }

    getDocument()->_urlImageMap.set( refName, ref );
    return ref;
}

/// register embedded document fonts in font manager, if any exist in document
void ldomDocument::registerEmbeddedFonts()
{
    if (_fontList.empty())
        return;
    int list = _fontList.length();
    lString8 x=lString8("");
    lString16Collection flist;
    fontMan->getFaceList(flist);
    int cnt = flist.length();
    for (int i = 0; i < list; i++) {
        LVEmbeddedFontDef *item = _fontList.get(i);
        lString16 url = item->getUrl();
        lString8 face = item->getFace();
        if (face.empty()) {
            for (int a=i+1;a<list;a++){
                lString8 tmp=_fontList.get(a)->getFace();
                if (!tmp.empty()) {face=tmp;break;}
            }
        }
        if ((!x.empty() && x.pos(face)!=-1) || url.empty()) {
            continue;
        }
        if (url.startsWithNoCase(lString16("res://")) || url.startsWithNoCase(lString16("file://"))) {
            if (!fontMan->RegisterExternalFont(item->getUrl(), item->getFace(), item->getBold(), item->getItalic())) {
                //CRLog::error("Failed to register external font face: %s file: %s", item->getFace().c_str(), LCSTR(item->getUrl()));
            }
            continue;
        }
        else {
            if (!fontMan->RegisterDocumentFont(getDocIndex(), _container, item->getUrl(), item->getFace(), item->getBold(), item->getItalic())) {
                //CRLog::error("Failed to register document font face: %s file: %s", item->getFace().c_str(), LCSTR(item->getUrl()));
                lString16 fontface = lString16("");
                for (int j = 0; j < cnt; j = j + 1) {
                fontface = flist[j];
                do { (fontface.replace(lString16(" "), lString16("\0"))); }
                while (fontface.pos(lString16(" ")) != -1);
                do { (url.replace(lString16(" "), lString16("\0"))); }
                while (url.pos(lString16(" ")) != -1);
                 if (fontface.lowercase().pos(url.lowercase()) != -1) {
                    if(fontMan->SetAlias(face, UnicodeToLocal(flist[j]), getDocIndex(),item->getBold(),item->getItalic())){
                    x.append(face).append(lString8(","));
                        CRLog::debug("font-face %s matches local font %s",face.c_str(),LCSTR(flist[j]));
                    break;}
                 }
                }
            }
        }
    }
}
/// unregister embedded document fonts in font manager, if any exist in document
void ldomDocument::unregisterEmbeddedFonts()
{
    fontMan->UnregisterDocumentFonts(_docIndex);
}

/// returns object image stream
LVStreamRef ldomDocument::getObjectImageStream( lString16 refName )
{
    LVStreamRef ref;
    if ( refName.startsWith(lString16(BLOB_NAME_PREFIX)) ) {
        return _blobCache.getBlob(refName);
    }
    if ( refName.length() > 10 && refName[4] == ':' && refName.startsWith(lString16("data:image/")) ) {
        // <img src="data:image/png;base64,iVBORw0KG...>
        lString16 data = refName.substr(0, 50);
        int pos = data.pos(L";base64,");
        if ( pos > 0 ) {
            lString8 b64data = UnicodeToLocal(refName.substr(pos+8));
            ref = LVStreamRef(new LVBase64Stream(b64data));
            return ref;
        }
    }
    if ( refName[0]!='#' ) {
        if ( !getContainer().isNull() ) {
            lString16 name = refName;
            if ( !getCodeBase().empty() )
                name = getCodeBase() + refName;
            ref = getContainer()->OpenStream(name.c_str(), LVOM_READ);
            if ( ref.isNull() ) {
                lString16 fname = getProps()->getStringDef( DOC_PROP_FILE_NAME, "" );
                fname = LVExtractFilenameWithoutExtension(fname);
                if ( !fname.empty() ) {
                    lString16 fn = fname + "_img";
//                    if ( getContainer()->GetObjectInfo(fn) ) {

//                    }
                    lString16 name = fn + "/" + refName;
                    if ( !getCodeBase().empty() )
                        name = getCodeBase() + name;
                    ref = getContainer()->OpenStream(name.c_str(), LVOM_READ);
                }
            }
            if ( ref.isNull() )
                CRLog::error("Cannot open stream by name %s", LCSTR(name));
        }
        return ref;
    }
    lUInt32 refValueId = findAttrValueIndex( refName.c_str() + 1 );
    if ( refValueId == (lUInt32)-1 ) {
        return ref;
    }
    ldomNode * objnode = getNodeById( refValueId );
    if ( !objnode || !objnode->isElement())
        return ref;
    ref = objnode->createBase64Stream();
    return ref;
}

/// returns object image source
LVImageSourceRef ldomDocument::getObjectImageSource( lString16 refName )
{
    LVStreamRef stream = getObjectImageStream( refName );
    if (stream.isNull())
         return LVImageSourceRef();
    return LVCreateStreamImageSource( stream );
}

void ldomDocument::resetNodeNumberingProps()
{
    lists.clear();
}

ListNumberingPropsRef ldomDocument::getNodeNumberingProps( lUInt32 nodeDataIndex )
{
    return lists.get(nodeDataIndex);
}

void ldomDocument::setNodeNumberingProps( lUInt32 nodeDataIndex, ListNumberingPropsRef v )
{
    lists.set(nodeDataIndex, v);
}

/// returns the sum of this node and its parents' top and bottom margins, borders and paddings
int ldomNode::getSurroundingAddedHeight()
{
    int h = 0;
    ldomNode * n = this;
    while (true) {
        ldomNode * parent = n->getParentNode();
        lvdom_element_render_method rm = n->getRendMethod();
        if ( rm != erm_inline && rm != erm_invisible && rm != erm_killed) {
            // Add offset of border and padding
            int base_width = 0;
            if ( parent && !(parent->isNull()) ) {
                // margins and padding in % are scaled according to parent's width
                RenderRectAccessor fmt( parent );
                base_width = fmt.getWidth();
            }
            int em = n->getFont()->getSize();
            css_style_ref_t style = n->getStyle();
            h += lengthToPx( style->margin[2], base_width, em );  // top margin
            h += lengthToPx( style->margin[3], base_width, em );  // bottom margin
            h += lengthToPx( style->padding[2], base_width, em ); // top padding
            h += lengthToPx( style->padding[3], base_width, em ); // bottom padding
            h += measureBorder(n, 0); // top border
            h += measureBorder(n, 2); // bottom border
        }
        if ( !parent || parent->isNull() )
            break;
        n = parent;
    }
    return h;
}

/// formats final block
// 'fmt' is the rect of the block node, and MUST have its width set
// (as ::renderFinalBlock( this, f.get(), fmt...) needs it to compute text-indent in %
// 'int width' is the available width for the inner content, and so
// caller must exclude block node padding from it.
int ldomNode::renderFinalBlock(  LFormattedTextRef & frmtext, RenderRectAccessor * fmt, int width, BlockFloatFootprint * float_footprint )
{
    ASSERT_NODE_NOT_NULL;
    if ( !isElement() )
        return 0;
    //CRLog::trace("renderFinalBlock()");
    CVRendBlockCache & cache = getDocument()->getRendBlockCache();
    LFormattedTextRef f;
    lvdom_element_render_method rm = getRendMethod();

    if ( cache.get( this, f ) ) {
        if ( f->isReusable() ) {
            frmtext = f;
            if ( rm != erm_final && rm != erm_list_item && rm != erm_table_caption )
                return 0;
            //RenderRectAccessor fmt( this );
            //CRLog::trace("Found existing formatted object for node #%08X", (lUInt32)this);
            return fmt->getHeight();
        }
        // Not resuable: remove it, just to be sure it's properly freed
        cache.remove( this );
    }
    f = getDocument()->createFormattedText();
    if ( (rm != erm_final && rm != erm_list_item && rm != erm_table_caption) )
        return 0;
    //RenderRectAccessor fmt( this );
    /// render whole node content as single formatted object
    int direction = RENDER_RECT_PTR_GET_DIRECTION(fmt);
    lUInt32 flags = styleToTextFmtFlags( getStyle(), 0, direction );
    int lang_node_idx = fmt->getLangNodeIndex();
    TextLangCfg * lang_cfg = TextLangMan::getTextLangCfg(lang_node_idx>0 ? getDocument()->getTinyNode(lang_node_idx) : NULL);
    ::renderFinalBlock( this, f.get(), fmt, flags, 0, -1, lang_cfg );
    // We need to store this LFormattedTextRef in the cache for it to
    // survive when leaving this function (some callers do use it).
    cache.set( this, f );
    bool flg=gFlgFloatingPunctuationEnabled;
    if (this->getNodeName()=="th"||this->getNodeName()=="td"||
            (!this->getParentNode()->isNull()&&this->getParentNode()->getNodeName()=="td")||
            (!this->getParentNode()->isNull()&&this->getParentNode()->getNodeName()=="th")) {
        gFlgFloatingPunctuationEnabled=false;
    }
    // This page_h we provide to f->Format() is only used to enforce a max height to images
    int page_h = getDocument()->getPageHeight();
    // Save or restore outer floats footprint (it is only provided
    // when rendering the document - when this is called to draw the
    // node, or search for text and links, we need to get it from
    // the cached RenderRectAccessor).
    BlockFloatFootprint restored_float_footprint; // (need to be available when we exit the else {})
    if (float_footprint) { // Save it in this node's RenderRectAccessor
        float_footprint->store( this );
    }
    else { // Restore it from this node's RenderRectAccessor
        float_footprint = &restored_float_footprint;
        float_footprint->restore( this, (lUInt16)width );
    }
    if ( !getDocument()->isRendered() ) {
        // Full rendering in progress: avoid some uneeded work that
        // is only needed when we'll be drawing the formatted text
        // (like alignLign()): this will mark it as not reusable, and
        // one that is on a page to be drawn will be reformatted .
        f->requestLightFormatting();
    }
    int h = f->Format((lUInt16)width, (lUInt16)page_h, direction, float_footprint);
    gFlgFloatingPunctuationEnabled=flg;
    frmtext = f;
    //CRLog::trace("Created new formatted object for node #%08X", (lUInt32)this);
    return h;
}

/// formats final block again after change, returns true if size of block is changed
/// (not used anywhere, not updated to use RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET)
bool ldomNode::refreshFinalBlock()
{
    ASSERT_NODE_NOT_NULL;
    if ( getRendMethod() != erm_final )
        return false;
    // TODO: implement reformatting of one node
    CVRendBlockCache & cache = getDocument()->getRendBlockCache();
    cache.remove( this );
    RenderRectAccessor fmt( this );
    lvRect oldRect, newRect;
    fmt.getRect( oldRect );
    LFormattedTextRef txtform;
    int width = fmt.getWidth();
    renderFinalBlock( txtform, &fmt, width-measureBorder(this,1)-measureBorder(this,3)
         -lengthToPx(this->getStyle()->padding[0],fmt.getWidth(),this->getFont()->getSize())
         -lengthToPx(this->getStyle()->padding[1],fmt.getWidth(),this->getFont()->getSize()));
    fmt.getRect( newRect );
    if ( oldRect == newRect )
        return false;
    // TODO: relocate other blocks
    return true;
}

#endif

/// replace node with r/o persistent implementation
ldomNode * ldomNode::persist()
{
    ASSERT_NODE_NOT_NULL;
#if BUILD_LITE!=1
    if ( !isPersistent() ) {
        if ( isElement() ) {
            // ELEM->PELEM
            tinyElement * elem = NPELEM;
            int attrCount = elem->_attrs.length();
            int childCount = elem->_children.length();
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_PELEMENT;
            _data._pelem_addr = getDocument()->_elemStorage.allocElem(_handle._dataIndex, elem->_parentNode ? elem->_parentNode->_handle._dataIndex : 0, elem->_children.length(), elem->_attrs.length() );
            ElementDataStorageItem * data = getDocument()->_elemStorage.getElem(_data._pelem_addr);
            data->nsid = elem->_nsid;
            data->id = elem->_id;
            lUInt16 * attrs = data->attrs();
            int i;
            for ( i=0; i<attrCount; i++ ) {
                const lxmlAttribute * attr = elem->_attrs[i];
                attrs[i * 4] = attr->nsid;     // namespace
                attrs[i * 4 + 1] = attr->id;   // id
                attrs[i * 4 + 2] = (lUInt16)(attr->index & 0xFFFF);// value lower 2-bytes
                attrs[i * 4 + 3] = (lUInt16)(attr->index >> 16);// value higher 2-bytes
            }
            for ( i=0; i<childCount; i++ ) {
                data->children[i] = elem->_children[i];
            }
            data->rendMethod = (lUInt8)elem->_rendMethod;
            delete elem;
        } else {
            // TEXT->PTEXT
            lString8 utf8 = _data._text_ptr->getText();
            lUInt32 parentIndex = _data._text_ptr->getParentIndex();
            delete _data._text_ptr;
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_PTEXT;
            _data._ptext_addr = getDocument()->_textStorage.allocText(_handle._dataIndex, parentIndex, utf8 );
            // change type
        }
    }
#endif
    return this;
}

/// replace node with r/w implementation
ldomNode * ldomNode::modify()
{
    ASSERT_NODE_NOT_NULL;
#if BUILD_LITE!=1
    if ( isPersistent() ) {
        if ( isElement() ) {
            // PELEM->ELEM
            ElementDataStorageItem * data = getDocument()->_elemStorage.getElem(_data._pelem_addr);
            tinyElement * elem = new tinyElement(getDocument(), getParentNode(), data->nsid, data->id );
            for ( int i=0; i<data->childCount; i++ )
                elem->_children.add( data->children[i] );
            for ( int i=0; i<data->attrCount; i++ )
                elem->_attrs.add( data->attr(i) );
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_ELEMENT;
            elem->_rendMethod = (lvdom_element_render_method)data->rendMethod;
            getDocument()->_elemStorage.freeNode( _data._pelem_addr );
            NPELEM = elem;
        } else {
            // PTEXT->TEXT
            // convert persistent text to mutable
            lString8 utf8 = getDocument()->_textStorage.getText(_data._ptext_addr);
            lUInt32 parentIndex = getDocument()->_textStorage.getParent(_data._ptext_addr);
            getDocument()->_textStorage.freeNode( _data._ptext_addr );
            _data._text_ptr = new ldomTextNode( parentIndex, utf8 );
            // change type
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_TEXT;
        }
    }
#endif
    return this;
}


/// dumps memory usage statistics to debug log
void tinyNodeCollection::dumpStatistics()
{
    CRLog::info("*** Document memory usage: "
                "elements:%d, textNodes:%d, "
                "ptext=("
                "%d uncompressed), "
                "ptelems=("
                "%d uncompressed), "
                "rects=("
                "%d uncompressed), "
                "nodestyles=("
                "%d uncompressed), "
                "styles:%d, fonts:%d, renderedNodes:%d, "
                "totalNodes:%d(%dKb), mutableElements:%d(~%dKb)",
                _elemCount, _textCount,
                _textStorage.getUncompressedSize(),
                _elemStorage.getUncompressedSize(),
                _rectStorage.getUncompressedSize(),
                _styleStorage.getUncompressedSize(),
                _styles.length(), _fonts.length(),
#if BUILD_LITE!=1
                ((ldomDocument*)this)->_renderedBlockCache.length(),
#else
                0,
#endif
                _itemCount, _itemCount*16/1024,
                _tinyElementCount, _tinyElementCount*(sizeof(tinyElement)+8*4)/1024 );
}
lString16 tinyNodeCollection::getStatistics()
{
    lString16 s;
    s << "Elements: " << fmt::decimal(_elemCount) << ", " << fmt::decimal(_elemStorage.getUncompressedSize()/1024) << " KB\n";
    s << "Text nodes: " << fmt::decimal(_textCount) << ", " << fmt::decimal(_textStorage.getUncompressedSize()/1024) << " KB\n";
    s << "Styles: " << fmt::decimal(_styles.length()) << ", " << fmt::decimal(_styleStorage.getUncompressedSize()/1024) << " KB\n";
    s << "Font instances: " << fmt::decimal(_fonts.length()) << "\n";
    s << "Rects: " << fmt::decimal(_rectStorage.getUncompressedSize()/1024) << " KB\n";
    #if BUILD_LITE!=1
    s << "Cached rendered blocks: " << fmt::decimal(((ldomDocument*)this)->_renderedBlockCache.length()) << "\n";
    #endif
    s << "Total nodes: " << fmt::decimal(_itemCount) << ", " << fmt::decimal(_itemCount*16/1024) << " KB\n";
    s << "Mutable elements: " << fmt::decimal(_tinyElementCount) << ", " << fmt::decimal(_tinyElementCount*(sizeof(tinyElement)+8*4)/1024) << " KB";
    return s;
}


/// returns position pointer
ldomXPointer LVTocItem::getXPointer()
{
    if ( _position.isNull() && !_path.empty() ) {
        _position = _doc->createXPointer( _path );
        if ( _position.isNull() ) {
            CRLog::trace("TOC node is not found for path %s", LCSTR(_path) );
        } else {
            CRLog::trace("TOC node is found for path %s", LCSTR(_path) );
            // CRLog::trace("           gives xpointer: %s", UnicodeToLocal(_position.toString()).c_str());
        }
    }
    return _position;
}

/// returns position path
lString16 LVTocItem::getPath()
{
    if ( _path.empty() && !_position.isNull())
        _path = _position.toString();
    return _path;
}

/// returns Y position
int LVTocItem::getY()
{
#if BUILD_LITE!=1
    return getXPointer().toPoint().y;
#else
    return 0;
#endif
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
bool LVTocItem::serialize( SerialBuf & buf )
{
//    LVTocItem *     _parent;
//    int             _level;
//    int             _index;
//    int             _page;
//    int             _percent;
//    lString16       _name;
//    ldomXPointer    _position;
//    LVPtrVector<LVTocItem> _children;

    buf << (lUInt32)_level << (lUInt32)_index << (lUInt32)_page << (lUInt32)_percent << (lUInt32)_children.length() << _name << getPath();
    if ( buf.error() )
        return false;
    for ( int i=0; i<_children.length(); i++ ) {
        _children[i]->serialize( buf );
        if ( buf.error() )
            return false;
    }
    return !buf.error();
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool LVTocItem::deserialize( ldomDocument * doc, SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    lInt32 childCount = 0;
    buf >> _level >> _index >> _page >> _percent >> childCount >> _name >> _path;
//    CRLog::trace("[%d] %05d  %s  %s", _level, _page, LCSTR(_name), LCSTR(_path));
    if ( buf.error() )
        return false;
//    if ( _level>0 ) {
//        _position = doc->createXPointer( _path );
//        if ( _position.isNull() ) {
//            CRLog::error("Cannot find TOC node by path %s", LCSTR(_path) );
//            buf.seterror();
//            return false;
//        }
//    }
    for ( int i=0; i<childCount; i++ ) {
        LVTocItem * item = new LVTocItem(doc);
        if ( !item->deserialize( doc, buf ) ) {
            delete item;
            return false;
        }
        item->_parent = this;
        _children.add( item );
        if ( buf.error() )
            return false;
    }
    return true;
}

/// returns page number
//int LVTocItem::getPageNum( LVRendPageList & pages )
//{
//    return getSectionPage( _position.getNode(), pages );
//}


static inline void makeTocFromCrHintsOrHeadings( ldomNode * node, bool ensure_cr_hints )
{
    int level;
    if ( ensure_cr_hints ) {
        css_cr_hint_t hint = node->getStyle()->cr_hint;
        if ( hint == css_cr_hint_toc_ignore )
            return; // requested to be ignored via style tweaks
        if ( hint >= css_cr_hint_toc_level1 && hint <= css_cr_hint_toc_level6 )
            level = hint - css_cr_hint_toc_level1 + 1;
        else if ( node->getNodeId() >= el_h1 && node->getNodeId() <= el_h6 )
            // el_h1 .. el_h6 are consecutive and ordered in include/fb2def.h
            level = node->getNodeId() - el_h1 + 1;
        else
            return;
    }
    else {
        if ( node->getNodeId() >= el_h1 && node->getNodeId() <= el_h6 )
            // el_h1 .. el_h6 are consecutive and ordered in include/fb2def.h
            level = node->getNodeId() - el_h1 + 1;
        else
            return;
    }
    lString16 title = removeSoftHyphens( node->getText(' ') );
    ldomXPointer xp = ldomXPointer(node, 0);
    LVTocItem * root = node->getDocument()->getToc();
    LVTocItem * parent = root;
    // Find adequate parent, or create intermediates
    int plevel = 1;
    while (plevel < level) {
        int nbc = parent->getChildCount();
        if (nbc) { // use the latest child
            parent = parent->getChild(nbc-1);
        }
        else {
            // If we'd like to stick it to the last parent found, even if
            // of wrong level, just do: break;
            // But it is cleaner to create intermediate(s)
            parent = parent->addChild(L"", xp, lString16::empty_str);
        }
        plevel++;
    }
    parent->addChild(title, xp, lString16::empty_str);
}

static void makeTocFromHeadings( ldomNode * node )
{
    makeTocFromCrHintsOrHeadings( node, false );
}

static void makeTocFromCrHintsOrHeadings( ldomNode * node )
{
    makeTocFromCrHintsOrHeadings( node, true );
}

static void makeTocFromDocFragments( ldomNode * node )
{
    if ( node->getNodeId() != el_DocFragment )
        return;
    // No title, and only level 1 with DocFragments
    ldomXPointer xp = ldomXPointer(node, 0);
    LVTocItem * root = node->getDocument()->getToc();
    root->addChild(L"", xp, lString16::empty_str);
}

void ldomDocument::buildTocFromHeadings()
{
    m_toc.clear();
    getRootNode()->recurseElements(makeTocFromHeadings);
}

void ldomDocument::buildAlternativeToc()
{
    m_toc.clear();
    // Look first for style tweaks specified -cr-hint: toc-level1 ... toc-level6
    // and/or headings (H1...H6)
    getRootNode()->recurseElements(makeTocFromCrHintsOrHeadings);
    // If no heading or hints found, fall back to gathering DocFraments
    if ( !m_toc.getChildCount() )
        getRootNode()->recurseElements(makeTocFromDocFragments);
    // m_toc.setAlternativeTocFlag() uses the root toc item _page property
    // (never used for the root node) to store the fact this is an
    // alternatve TOC. This info can then be serialized to cache and
    // retrieved without any additional work or space overhead.
    m_toc.setAlternativeTocFlag();
    // cache file will have to be updated with the alt TOC
    setCacheFileStale(true);
    _toc_from_cache_valid = false; // to force update of page numbers
}

/// returns position pointer
ldomXPointer LVPageMapItem::getXPointer()
{
    if ( _position.isNull() && !_path.empty() ) {
        _position = _doc->createXPointer( _path );
        if ( _position.isNull() ) {
            CRLog::trace("LVPageMapItem node is not found for path %s", LCSTR(_path) );
        } else {
            CRLog::trace("LVPageMapItem node is found for path %s", LCSTR(_path) );
        }
    }
    return _position;
}

/// returns position path
lString16 LVPageMapItem::getPath()
{
    if ( _path.empty() && !_position.isNull())
        _path = _position.toString();
    return _path;
}

/// returns Y position
int LVPageMapItem::getDocY(bool refresh)
{
#if BUILD_LITE!=1
    if ( _doc_y < 0 || refresh )
        _doc_y = getXPointer().toPoint().y;
    if ( _doc_y < 0 && !_position.isNull() ) {
        // We got a xpointer, that did not resolve to a point.
        // It may be because the node it points to is invisible,
        // which may happen with pagebreak spans (that may not
        // be empty, and were set to "display: none").
        ldomXPointerEx xp = _position;
        if ( !xp.isVisible() ) {
            if ( xp.nextVisibleText() ) {
                _doc_y = xp.toPoint().y;
            }
            else {
                xp = _position;
                if ( xp.prevVisibleText() ) {
                    _doc_y = xp.toPoint().y;
                }
            }
        }
    }
    return _doc_y;
#else
    return 0;
#endif
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
bool LVPageMapItem::serialize( SerialBuf & buf )
{
    buf << (lUInt32)_index << (lUInt32)_page << (lUInt32)_doc_y << _label << getPath();
    return !buf.error();
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool LVPageMapItem::deserialize( ldomDocument * doc, SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    buf >> _index >> _page >> _doc_y >> _label >> _path;
    return !buf.error();

}
/// serialize to byte array (pointer will be incremented by number of bytes written)
bool LVPageMap::serialize( SerialBuf & buf )
{
    buf << (lUInt32)_page_info_valid << (lUInt32)_children.length() << _source;
    if ( buf.error() )
        return false;
    for ( int i=0; i<_children.length(); i++ ) {
        _children[i]->serialize( buf );
        if ( buf.error() )
            return false;
    }
    return !buf.error();
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool LVPageMap::deserialize( ldomDocument * doc, SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    lUInt32 childCount = 0;
    lUInt32 pageInfoValid = 0;
    buf >> pageInfoValid >> childCount >> _source;
    if ( buf.error() )
        return false;
    _page_info_valid = (bool)pageInfoValid;
    for ( int i=0; i<childCount; i++ ) {
        LVPageMapItem * item = new LVPageMapItem(doc);
        if ( !item->deserialize( doc, buf ) ) {
            delete item;
            return false;
        }
        _children.add( item );
        if ( buf.error() )
            return false;
    }
    return true;
}


#if 0 && defined(_DEBUG)

#define TEST_FILE_NAME "/tmp/test-cache-file.dat"

#include <lvdocview.h>

void testCacheFile()
{
#if BUILD_LITE!=1
    CRLog::info("Starting CacheFile unit test");
    lUInt8 data1[] = {'T', 'e', 's', 't', 'd', 'a', 't', 'a', 1, 2, 3, 4, 5, 6, 7};
    lUInt8 data2[] = {'T', 'e', 's', 't', 'd', 'a', 't', 'a', '2', 1, 2, 3, 4, 5, 6, 7};
    lUInt8 * buf1;
    lUInt8 * buf2;
    int sz1;
    int sz2;
    lString16 fn(TEST_FILE_NAME);

    {
        lUInt8 data1[] = {'T', 'e', 's', 't', 'D', 'a', 't', 'a', '1'};
        lUInt8 data2[] = {'T', 'e', 's', 't', 'D', 'a', 't', 'a', '2', 1, 2, 3, 4, 5, 6, 7};
        LVStreamRef s = LVOpenFileStream( fn.c_str(), LVOM_APPEND );
        s->SetPos(0);
        s->Write(data1, sizeof(data1), NULL);
        s->SetPos(4096);
        s->Write(data1, sizeof(data1), NULL);
        s->SetPos(8192);
        s->Write(data2, sizeof(data2), NULL);
        s->SetPos(4096);
        s->Write(data2, sizeof(data2), NULL);
        lUInt8 buf[16];
        s->SetPos(0);
        s->Read(buf, sizeof(data1), NULL);
        MYASSERT(!memcmp(buf, data1, sizeof(data1)), "read 1 content");
        s->SetPos(4096);
        s->Read(buf, sizeof(data2), NULL);
        MYASSERT(!memcmp(buf, data2, sizeof(data2)), "read 2 content");

        //return;
    }

    // write
    {
        CacheFile f;
        MYASSERT(f.open(cs16("/tmp/blabla-not-exits-file-name"))==false, "Wrong failed open result");
        MYASSERT(f.create( fn )==true, "new file created");
        MYASSERT(f.write(CBT_TEXT_DATA, 1, data1, sizeof(data1), true)==true, "write 1");
        MYASSERT(f.write(CBT_ELEM_DATA, 3, data2, sizeof(data2), false)==true, "write 2");

        MYASSERT(f.read(CBT_TEXT_DATA, 1, buf1, sz1)==true, "read 1");
        MYASSERT(f.read(CBT_ELEM_DATA, 3, buf2, sz2)==true, "read 2");
        MYASSERT(sz1==sizeof(data1), "read 1 size");
        MYASSERT(!memcmp(buf1, data1, sizeof(data1)), "read 1 content");
        MYASSERT(sz2==sizeof(data2), "read 2 size");
        MYASSERT(!memcmp(buf2, data2, sizeof(data2)), "read 2 content");
    }
    // write
    {
        CacheFile f;
        MYASSERT(f.open(fn)==true, "Wrong failed open result");
        MYASSERT(f.read(CBT_TEXT_DATA, 1, buf1, sz1)==true, "read 1");
        MYASSERT(f.read(CBT_ELEM_DATA, 3, buf2, sz2)==true, "read 2");
        MYASSERT(sz1==sizeof(data1), "read 1 size");
        MYASSERT(!memcmp(buf1, data1, sizeof(data1)), "read 1 content");
        MYASSERT(sz2==sizeof(data2), "read 2 size");
        MYASSERT(!memcmp(buf2, data2, sizeof(data2)), "read 2 content");
    }

    CRLog::info("Finished CacheFile unit test");
#endif
}

#ifdef _WIN32
#define TEST_FN_TO_OPEN "/projects/test/bibl.fb2.zip"
#else
#define TEST_FN_TO_OPEN "/home/lve/src/test/bibl.fb2.zip"
#endif

void runFileCacheTest()
{
#if BUILD_LITE!=1
    CRLog::info("====Cache test started =====");

    // init and clear cache
    ldomDocCache::init(cs16("/tmp/cr3cache"), 100);
    MYASSERT(ldomDocCache::enabled(), "clear cache");

    {
        CRLog::info("====Open document and save to cache=====");
        LVDocView view(4);
        view.Resize(600, 800);
        bool res = view.LoadDocument(TEST_FN_TO_OPEN);
        MYASSERT(res, "load document");
        view.getPageImage(0);
        view.getDocProps()->setInt(PROP_FORCED_MIN_FILE_SIZE_TO_CACHE, 30000);
        view.swapToCache();
        //MYASSERT(res, "swap to cache");
        view.getDocument()->dumpStatistics();
    }
    {
        CRLog::info("====Open document from cache=====");
        LVDocView view(4);
        view.Resize(600, 800);
        bool res = view.LoadDocument(TEST_FN_TO_OPEN);
        MYASSERT(res, "load document");
        view.getDocument()->dumpStatistics();
        view.getPageImage(0);
    }
    CRLog::info("====Cache test finished=====");
#endif
}

void runBasicTinyDomUnitTests()
{
    CRLog::info("==========================");
    CRLog::info("Starting tinyDOM unit test");
    ldomDocument * doc = new ldomDocument();
    ldomNode * root = doc->getRootNode();//doc->allocTinyElement( NULL, 0, 0 );
    MYASSERT(root!=NULL,"root != NULL");

    int el_p = doc->getElementNameIndex(L"p");
    int el_title = doc->getElementNameIndex(L"title");
    int el_strong = doc->getElementNameIndex(L"strong");
    int el_emphasis = doc->getElementNameIndex(L"emphasis");
    int attr_id = doc->getAttrNameIndex(L"id");
    int attr_name = doc->getAttrNameIndex(L"name");
    static lUInt16 path1[] = {el_title, el_p, 0};
    static lUInt16 path2[] = {el_title, el_p, el_strong, 0};

    CRLog::info("* simple DOM operations, tinyElement");
    MYASSERT(root->isRoot(),"root isRoot");
    MYASSERT(root->getParentNode()==NULL,"root parent is null");
    MYASSERT(root->getParentIndex()==0,"root parent index == 0");
    MYASSERT(root->getChildCount()==0,"empty root child count");
    ldomNode * el1 = root->insertChildElement(el_p);
    MYASSERT(root->getChildCount()==1,"root child count 1");
    MYASSERT(el1->getParentNode()==root,"element parent node");
    MYASSERT(el1->getParentIndex()==root->getDataIndex(),"element parent node index");
    MYASSERT(el1->getNodeId()==el_p, "node id");
    MYASSERT(el1->getNodeNsId()==LXML_NS_NONE, "node nsid");
    MYASSERT(!el1->isRoot(),"elem not isRoot");
    ldomNode * el2 = root->insertChildElement(el_title);
    MYASSERT(root->getChildCount()==2,"root child count 2");
    MYASSERT(el2->getNodeId()==el_title, "node id");
    MYASSERT(el2->getNodeNsId()==LXML_NS_NONE, "node nsid");
    lString16 nodename = el2->getNodeName();
    //CRLog::debug("node name: %s", LCSTR(nodename));
    MYASSERT(nodename==L"title","node name");
    ldomNode * el21 = el2->insertChildElement(el_p);
    MYASSERT(root->getNodeLevel()==1,"node level 1");
    MYASSERT(el2->getNodeLevel()==2,"node level 2");
    MYASSERT(el21->getNodeLevel()==3,"node level 3");
    MYASSERT(el21->getNodeIndex()==0,"node index single");
    MYASSERT(el1->getNodeIndex()==0,"node index first");
    MYASSERT(el2->getNodeIndex()==1,"node index last");
    MYASSERT(root->getNodeIndex()==0,"node index for root");
    MYASSERT(root->getFirstChild()==el1,"first child");
    MYASSERT(root->getLastChild()==el2,"last child");
    MYASSERT(el2->getFirstChild()==el21,"first single child");
    MYASSERT(el2->getLastChild()==el21,"last single child");
    MYASSERT(el21->getFirstChild()==NULL,"first child - no children");
    MYASSERT(el21->getLastChild()==NULL,"last child - no children");
    ldomNode * el0 = root->insertChildElement(1, LXML_NS_NONE, el_title);
    MYASSERT(el1->getNodeIndex()==0,"insert in the middle");
    MYASSERT(el0->getNodeIndex()==1,"insert in the middle");
    MYASSERT(el2->getNodeIndex()==2,"insert in the middle");
    MYASSERT(root->getChildNode(0)==el1,"child node 0");
    MYASSERT(root->getChildNode(1)==el0,"child node 1");
    MYASSERT(root->getChildNode(2)==el2,"child node 2");
    ldomNode * removedNode = root->removeChild( 1 );
    MYASSERT(removedNode==el0,"removed node");
    el0->destroy();
    MYASSERT(el0->isNull(),"destroyed node isNull");
    MYASSERT(root->getChildNode(0)==el1,"child node 0, after removal");
    MYASSERT(root->getChildNode(1)==el2,"child node 1, after removal");
    ldomNode * el02 = root->insertChildElement(5, LXML_NS_NONE, el_emphasis);
    MYASSERT(el02==el0,"removed node reusage");

    {
        ldomNode * f1 = root->findChildElement(path1);
        MYASSERT(f1==el21, "find 1 on mutable - is el21");
        MYASSERT(f1->getNodeId()==el_p, "find 1 on mutable");
        //ldomNode * f2 = root->findChildElement(path2);
        //MYASSERT(f2!=NULL, "find 2 on mutable - not null");
        //MYASSERT(f2==el21, "find 2 on mutable - is el21");
        //MYASSERT(f2->getNodeId()==el_strong, "find 2 on mutable");
    }

    CRLog::info("* simple DOM operations, mutable text");
    lString16 sampleText("Some sample text.");
    lString16 sampleText2("Some sample text 2.");
    lString16 sampleText3("Some sample text 3.");
    ldomNode * text1 = el1->insertChildText(sampleText);
    MYASSERT(text1->getText()==sampleText, "sample text 1 match unicode");
    MYASSERT(text1->getNodeLevel()==3,"text node level");
    MYASSERT(text1->getNodeIndex()==0,"text node index");
    MYASSERT(text1->isText(),"text node isText");
    MYASSERT(!text1->isElement(),"text node isElement");
    MYASSERT(!text1->isNull(),"text node isNull");
    ldomNode * text2 = el1->insertChildText(0, sampleText2);
    MYASSERT(text2->getNodeIndex()==0,"text node index, insert at beginning");
    MYASSERT(text2->getText()==sampleText2, "sample text 2 match unicode");
    MYASSERT(text2->getText8()==UnicodeToUtf8(sampleText2), "sample text 2 match utf8");
    text1->setText(sampleText2);
    MYASSERT(text1->getText()==sampleText2, "sample text 1 match unicode, changed");
    text1->setText8(UnicodeToUtf8(sampleText3));
    MYASSERT(text1->getText()==sampleText3, "sample text 1 match unicode, changed 8");
    MYASSERT(text1->getText8()==UnicodeToUtf8(sampleText3), "sample text 1 match utf8, changed");

    MYASSERT(el1->getFirstTextChild()==text2, "firstTextNode");
    MYASSERT(el1->getLastTextChild()==text1, "lastTextNode");
    MYASSERT(el21->getLastTextChild()==NULL, "lastTextNode NULL");

#if BUILD_LITE!=1
    CRLog::info("* style cache");
    {
        css_style_ref_t style1;
        style1 = css_style_ref_t( new css_style_rec_t );
        style1->display = css_d_block;
        style1->white_space = css_ws_normal;
        style1->text_align = css_ta_left;
        style1->text_align_last = css_ta_left;
        style1->text_decoration = css_td_none;
        style1->text_transform = css_tt_none;
        style1->hyphenate = css_hyph_auto;
        style1->color.type = css_val_unspecified;
        style1->color.value = 0x000000;
        style1->background_color.type = css_val_unspecified;
        style1->background_color.value = 0xFFFFFF;
        style1->page_break_before = css_pb_auto;
        style1->page_break_after = css_pb_auto;
        style1->page_break_inside = css_pb_auto;
        style1->vertical_align.type = css_val_unspecified;
        style1->vertical_align.value = css_va_baseline;
        style1->font_family = css_ff_sans_serif;
        style1->font_size.type = css_val_px;
        style1->font_size.value = 24 << 8;
        style1->font_name = cs8("Arial");
        style1->font_weight = css_fw_400;
        style1->font_style = css_fs_normal;
        style1->font_features.type = css_val_unspecified;
        style1->font_features.value = 0;
        style1->text_indent.type = css_val_px;
        style1->text_indent.value = 0;
        style1->line_height.type = css_val_unspecified;
        style1->line_height.value = css_generic_normal; // line-height: normal
        style1->cr_hint = css_cr_hint_none;

        css_style_ref_t style2;
        style2 = css_style_ref_t( new css_style_rec_t );
        style2->display = css_d_block;
        style2->white_space = css_ws_normal;
        style2->text_align = css_ta_left;
        style2->text_align_last = css_ta_left;
        style2->text_decoration = css_td_none;
        style2->text_transform = css_tt_none;
        style2->hyphenate = css_hyph_auto;
        style2->color.type = css_val_unspecified;
        style2->color.value = 0x000000;
        style2->background_color.type = css_val_unspecified;
        style2->background_color.value = 0xFFFFFF;
        style2->page_break_before = css_pb_auto;
        style2->page_break_after = css_pb_auto;
        style2->page_break_inside = css_pb_auto;
        style2->vertical_align.type = css_val_unspecified;
        style2->vertical_align.value = css_va_baseline;
        style2->font_family = css_ff_sans_serif;
        style2->font_size.type = css_val_px;
        style2->font_size.value = 24 << 8;
        style2->font_name = cs8("Arial");
        style2->font_weight = css_fw_400;
        style2->font_style = css_fs_normal;
        style2->font_features.type = css_val_unspecified;
        style2->font_features.value = 0;
        style2->text_indent.type = css_val_px;
        style2->text_indent.value = 0;
        style2->line_height.type = css_val_unspecified;
        style2->line_height.value = css_generic_normal; // line-height: normal
        style2->cr_hint = css_cr_hint_none;

        css_style_ref_t style3;
        style3 = css_style_ref_t( new css_style_rec_t );
        style3->display = css_d_block;
        style3->white_space = css_ws_normal;
        style3->text_align = css_ta_right;
        style3->text_align_last = css_ta_left;
        style3->text_decoration = css_td_none;
        style3->text_transform = css_tt_none;
        style3->hyphenate = css_hyph_auto;
        style3->color.type = css_val_unspecified;
        style3->color.value = 0x000000;
        style3->background_color.type = css_val_unspecified;
        style3->background_color.value = 0xFFFFFF;
        style3->page_break_before = css_pb_auto;
        style3->page_break_after = css_pb_auto;
        style3->page_break_inside = css_pb_auto;
        style3->vertical_align.type = css_val_unspecified;
        style3->vertical_align.value = css_va_baseline;
        style3->font_family = css_ff_sans_serif;
        style3->font_size.type = css_val_px;
        style3->font_size.value = 24 << 8;
        style3->font_name = cs8("Arial");
        style3->font_weight = css_fw_400;
        style3->font_style = css_fs_normal;
        style3->font_features.type = css_val_unspecified;
        style3->font_features.value = 0;
        style3->text_indent.type = css_val_px;
        style3->text_indent.value = 0;
        style3->line_height.type = css_val_unspecified;
        style3->line_height.value = css_generic_normal; // line-height: normal
        style3->cr_hint = css_cr_hint_none;

        el1->setStyle(style1);
        css_style_ref_t s1 = el1->getStyle();
        MYASSERT(!s1.isNull(), "style is set");
        el2->setStyle(style2);
        MYASSERT(*style1==*style2, "identical styles : == is true");
        MYASSERT(calcHash(*style1)==calcHash(*style2), "identical styles have the same hashes");
        MYASSERT(el1->getStyle().get()==el2->getStyle().get(), "identical styles reused");
        el21->setStyle(style3);
        MYASSERT(el1->getStyle().get()!=el21->getStyle().get(), "different styles not reused");
    }

    CRLog::info("* font cache");
    {
        font_ref_t font1 = fontMan->GetFont(24, 400, false, css_ff_sans_serif, cs8("DejaVu Sans"));
        font_ref_t font2 = fontMan->GetFont(24, 400, false, css_ff_sans_serif, cs8("DejaVu Sans"));
        font_ref_t font3 = fontMan->GetFont(28, 800, false, css_ff_serif, cs8("DejaVu Sans Condensed"));
        MYASSERT(el1->getFont().isNull(), "font is not set");
        el1->setFont(font1);
        MYASSERT(!el1->getFont().isNull(), "font is set");
        el2->setFont(font2);
        MYASSERT(*font1==*font2, "identical fonts : == is true");
        MYASSERT(calcHash(font1)==calcHash(font2), "identical styles have the same hashes");
        MYASSERT(el1->getFont().get()==el2->getFont().get(), "identical fonts reused");
        el21->setFont(font3);
        MYASSERT(el1->getFont().get()!=el21->getFont().get(), "different fonts not reused");
    }

    CRLog::info("* persistance test");

    el2->setAttributeValue(LXML_NS_NONE, attr_id, L"id1");
    el2->setAttributeValue(LXML_NS_NONE, attr_name, L"name1");
    MYASSERT(el2->getNodeId()==el_title, "mutable node id");
    MYASSERT(el2->getNodeNsId()==LXML_NS_NONE, "mutable node nsid");
    MYASSERT(el2->getAttributeValue(attr_id)==L"id1", "attr id1 mutable");
    MYASSERT(el2->getAttributeValue(attr_name)==L"name1", "attr name1 mutable");
    MYASSERT(el2->getAttrCount()==2, "attr count mutable");
    el2->persist();
    MYASSERT(el2->getAttributeValue(attr_id)==L"id1", "attr id1 pers");
    MYASSERT(el2->getAttributeValue(attr_name)==L"name1", "attr name1 pers");
    MYASSERT(el2->getNodeId()==el_title, "persistent node id");
    MYASSERT(el2->getNodeNsId()==LXML_NS_NONE, "persistent node nsid");
    MYASSERT(el2->getAttrCount()==2, "attr count persist");

    {
        ldomNode * f1 = root->findChildElement(path1);
        MYASSERT(f1==el21, "find 1 on mutable - is el21");
        MYASSERT(f1->getNodeId()==el_p, "find 1 on mutable");
    }

    el2->modify();
    MYASSERT(el2->getNodeId()==el_title, "mutable 2 node id");
    MYASSERT(el2->getNodeNsId()==LXML_NS_NONE, "mutable 2 node nsid");
    MYASSERT(el2->getAttributeValue(attr_id)==L"id1", "attr id1 mutable 2");
    MYASSERT(el2->getAttributeValue(attr_name)==L"name1", "attr name1 mutable 2");
    MYASSERT(el2->getAttrCount()==2, "attr count mutable 2");

    {
        ldomNode * f1 = root->findChildElement(path1);
        MYASSERT(f1==el21, "find 1 on mutable - is el21");
        MYASSERT(f1->getNodeId()==el_p, "find 1 on mutable");
    }

    CRLog::info("* convert to persistent");
    CRTimerUtil infinite;
    doc->persist(infinite);
    doc->dumpStatistics();

    MYASSERT(el21->getFirstChild()==NULL,"first child - no children");
    MYASSERT(el21->isPersistent(), "persistent before insertChildElement");
    ldomNode * el211 = el21->insertChildElement(el_strong);
    MYASSERT(!el21->isPersistent(), "mutable after insertChildElement");
    el211->persist();
    MYASSERT(el211->isPersistent(), "persistent before insertChildText");
    el211->insertChildText(cs16(L"bla bla bla"));
    el211->insertChildText(cs16(L"bla bla blaw"));
    MYASSERT(!el211->isPersistent(), "modifable after insertChildText");
    //el21->insertChildElement(el_strong);
    MYASSERT(el211->getChildCount()==2, "child count, in mutable");
    el211->persist();
    MYASSERT(el211->getChildCount()==2, "child count, in persistent");
    el211->modify();
    MYASSERT(el211->getChildCount()==2, "child count, in mutable again");
    CRTimerUtil infinite2;
    doc->persist(infinite2);

    ldomNode * f1 = root->findChildElement(path1);
    MYASSERT(f1->getNodeId()==el_p, "find 1");
    ldomNode * f2 = root->findChildElement(path2);
    MYASSERT(f2->getNodeId()==el_strong, "find 2");
    MYASSERT(f2 == el211, "find 2, ref");


    CRLog::info("* compacting");
    doc->compact();
    doc->dumpStatistics();
#endif

    delete doc;


    CRLog::info("Finished tinyDOM unit test");

    CRLog::info("==========================");

}

void runCHMUnitTest()
{
#if CHM_SUPPORT_ENABLED==1
#if BUILD_LITE!=1
    LVStreamRef stream = LVOpenFileStream("/home/lve/src/test/mysql.chm", LVOM_READ);
    MYASSERT ( !stream.isNull(), "container stream opened" );
    CRLog::trace("runCHMUnitTest() -- file stream opened ok");
    LVContainerRef dir = LVOpenCHMContainer( stream );
    MYASSERT ( !dir.isNull(), "container opened" );
    CRLog::trace("runCHMUnitTest() -- container opened ok");
    LVStreamRef s = dir->OpenStream(L"/index.html", LVOM_READ);
    MYASSERT ( !s.isNull(), "item opened" );
    CRLog::trace("runCHMUnitTest() -- index.html opened ok: size=%d", (int)s->GetSize());
    lvsize_t bytesRead = 0;
    char buf[1000];
    MYASSERT( s->SetPos(100)==100, "SetPos()" );
    MYASSERT( s->Read(buf, 1000, &bytesRead)==LVERR_OK, "Read()" );
    MYASSERT( bytesRead==1000, "Read() -- bytesRead" );
    buf[999] = 0;
    CRLog::trace("CHM/index.html Contents 1000: %s", buf);

    MYASSERT( s->SetPos(0)==0, "SetPos() 2" );
    MYASSERT( s->Read(buf, 1000, &bytesRead)==LVERR_OK, "Read() 2" );
    MYASSERT( bytesRead==1000, "Read() -- bytesRead 2" );
    buf[999] = 0;
    CRLog::trace("CHM/index.html Contents 0: %s", buf);
#endif
#endif
}

static void makeTestFile( const char * fname, int size )
{
    LVStreamRef s = LVOpenFileStream( fname, LVOM_WRITE );
    MYASSERT( !s.isNull(), "makeTestFile create" );
    int seed = 0;
    lUInt8 * buf = new lUInt8[size];
    for ( int i=0; i<size; i++ ) {
        buf[i] = (seed >> 9) & 255;
        seed = seed * 31 + 14323;
    }
    MYASSERT( s->Write(buf, size, NULL)==LVERR_OK, "makeTestFile write" );
    delete[] buf;
}

void runBlockWriteCacheTest()
{



    int sz = 2000000;
    const char * fn1 = "/tmp/tf1.dat";
    const char * fn2 = "/tmp/tf2.dat";
    //makeTestFile( fn1, sz );
    //makeTestFile( fn2, sz );

    CRLog::debug("BlockCache test started");

    LVStreamRef s1 = LVOpenFileStream( fn1, LVOM_APPEND );
    LVStreamRef s2 =  LVCreateBlockWriteStream( LVOpenFileStream( fn2, LVOM_APPEND ), 0x8000, 16);
    MYASSERT(! s1.isNull(), "s1");
    MYASSERT(! s2.isNull(), "s2");
    LVStreamRef ss = LVCreateCompareTestStream(s1, s2);
    lUInt8 buf[0x100000];
    for ( int i=0; i<sizeof(buf); i++ ) {
        buf[i] = (lUInt8)(rand() & 0xFF);
    }
    //memset( buf, 0xAD, 1000000 );
    ss->SetPos( 0 );
    ss->Write( buf, 150, NULL );
    ss->SetPos( 0 );
    ss->Write( buf, 150, NULL );
    ss->SetPos( 0 );
    ss->Write( buf, 150, NULL );


    ss->SetPos( 1000 );
    ss->Read( buf, 5000, NULL );
    ss->SetPos( 100000 );
    ss->Read( buf+10000, 150000, NULL );

    ss->SetPos( 1000 );
    ss->Write( buf, 15000, NULL );
    ss->SetPos( 1000 );
    ss->Read( buf+100000, 15000, NULL );
    ss->Read( buf, 1000000, NULL );


    ss->SetPos( 1000 );
    ss->Write( buf, 15000, NULL );
    ss->Write( buf, 15000, NULL );
    ss->Write( buf, 15000, NULL );
    ss->Write( buf, 15000, NULL );


    ss->SetPos( 100000 );
    ss->Write( buf+15000, 150000, NULL );
    ss->SetPos( 100000 );
    ss->Read( buf+25000, 200000, NULL );

    ss->SetPos( 100000 );
    ss->Read( buf+55000, 200000, NULL );

    ss->SetPos( 100000 );
    ss->Write( buf+1000, 250000, NULL );
    ss->SetPos( 150000 );
    ss->Read( buf, 50000, NULL );
    ss->SetPos( 1000000 );
    ss->Write( buf, 500000, NULL );
    for ( int i=0; i<10; i++ )
        ss->Write( buf, 5000, NULL );
    ss->Read( buf, 50000, NULL );

    ss->SetPos( 5000000 );
    ss->Write( buf, 500000, NULL );
    ss->SetPos( 4800000 );
    ss->Read( buf, 500000, NULL );

    for ( int i=0; i<20; i++ ) {
        int op = (rand() & 15) < 5;
        long offset = (rand()&0x7FFFF);
        long foffset = (rand()&0x3FFFFF);
        long size = (rand()&0x3FFFF);
        ss->SetPos(foffset);
        if ( op==0 ) {
            // read
            ss->Read(buf+offset, size, NULL);
        } else {
            ss->Write(buf+offset, size, NULL);
        }
    }

    CRLog::debug("BlockCache test finished");

}

void runTinyDomUnitTests()
{
    CRLog::info("runTinyDomUnitTests()");
    runBlockWriteCacheTest();

    runBasicTinyDomUnitTests();

    CRLog::info("==========================");
    testCacheFile();

    runFileCacheTest();
    CRLog::info("==========================");

}

#endif
