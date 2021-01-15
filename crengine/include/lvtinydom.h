﻿/** \file lvtinydom.h
    \brief fast and compact XML DOM tree

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2009
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details


	Goal: make fast DOM implementation with small memory footprint.

    2009/04 : Introducing new storage model, optimized for mmap.
    All DOM objects are divided by 2 parts.
    1) Short RAM instance
    2) Data storage part, which could be placed to mmap buffer.

    Document object storage should handle object table and data buffer.
    Each object has DataIndex, index of entry in object table.
    Object table holds pointer to RAM instance and data storage for each object.
*/


#ifndef __LV_TINYDOM_H_INCLUDED__
#define __LV_TINYDOM_H_INCLUDED__

#include "lvmemman.h"
#include "lvstring.h"
#include "lstridmap.h"
#include "lvxml.h"
#include "dtddef.h"
#include "lvstyles.h"
#include "lvdrawbuf.h"
#include "lvembeddedfont.h"
#include "lvstsheet.h"
#include "lvpagesplitter.h"
#include "lvptrvec.h"
#include "lvhashtable.h"
#include "lvimg.h"
#include "props.h"
#include "bookformats.h"
#include "serialbuf.h"
#include "lvstring32hashedcollection.h"

// Allows for requesting older DOM building code (including bugs NOT fixed)
extern const int gDOMVersionCurrent;

// Also defined in src/lvtinydom.cpp
#define DOM_VERSION_WITH_NORMALIZED_XPOINTERS 20200223

#define LXML_NO_DATA       0 ///< to mark data storage record as empty
#define LXML_ELEMENT_NODE  1 ///< element node
#define LXML_TEXT_NODE     2 ///< text node
//#define LXML_DOCUMENT_NODE 3 ///< document node (not implemented)
//#define LXML_COMMENT_NODE  4 ///< comment node (not implemented)


/// docFlag mask, enable internal stylesheet of document and style attribute of elements
#define DOC_FLAG_ENABLE_INTERNAL_STYLES 1
/// docFlag mask, enable paperbook-like footnotes
#define DOC_FLAG_ENABLE_FOOTNOTES       2
/// docFlag mask, enable preformatted text
#define DOC_FLAG_PREFORMATTED_TEXT      4
/// docFlag mask, enable document embedded fonts (EPUB)
#define DOC_FLAG_ENABLE_DOC_FONTS       8
/// docFlag mask, force page breaks on non-linear fragments (EPUB)
#define DOC_FLAG_NONLINEAR_PAGEBREAK   16
/// default docFlag set
#define DOC_FLAG_DEFAULTS (DOC_FLAG_ENABLE_INTERNAL_STYLES|DOC_FLAG_ENABLE_FOOTNOTES|DOC_FLAG_ENABLE_DOC_FONTS)



#define LXML_NS_NONE 0       ///< no namespace specified
#define LXML_NS_ANY  0xFFFF  ///< any namespace can be specified
#define LXML_ATTR_VALUE_NONE  0xFFFFFFFF  ///< attribute not found

#define DOC_STRING_HASH_SIZE  256
#define RESERVED_DOC_SPACE    4096
#define MAX_TYPE_ID           1024 // max of element, ns, attr
#define MAX_ELEMENT_TYPE_ID   1024
#define MAX_NAMESPACE_TYPE_ID 64
#define MAX_ATTRIBUTE_TYPE_ID 1024
#define UNKNOWN_ELEMENT_TYPE_ID   (MAX_ELEMENT_TYPE_ID>>1)
#define UNKNOWN_ATTRIBUTE_TYPE_ID (MAX_ATTRIBUTE_TYPE_ID>>1)
#define UNKNOWN_NAMESPACE_TYPE_ID (MAX_NAMESPACE_TYPE_ID>>1)

// document property names
#define DOC_PROP_AUTHORS         "doc.authors"
#define DOC_PROP_TITLE           "doc.title"
#define DOC_PROP_LANGUAGE        "doc.language"
#define DOC_PROP_DESCRIPTION     "doc.description"
#define DOC_PROP_KEYWORDS        "doc.keywords"
#define DOC_PROP_SERIES_NAME     "doc.series.name"
#define DOC_PROP_SERIES_NUMBER   "doc.series.number"
#define DOC_PROP_ARC_NAME        "doc.archive.name"
#define DOC_PROP_ARC_PATH        "doc.archive.path"
#define DOC_PROP_ARC_SIZE        "doc.archive.size"
#define DOC_PROP_ARC_FILE_COUNT  "doc.archive.file.count"
#define DOC_PROP_FILE_NAME       "doc.file.name"
#define DOC_PROP_FILE_PATH       "doc.file.path"
#define DOC_PROP_FILE_SIZE       "doc.file.size"
#define DOC_PROP_FILE_FORMAT     "doc.file.format"
#define DOC_PROP_FILE_FORMAT_ID  "doc.file.format.id"
#define DOC_PROP_FILE_CRC32      "doc.file.crc32"
#define DOC_PROP_CODE_BASE       "doc.file.code.base"
#define DOC_PROP_COVER_FILE      "doc.cover.file"

#define DEF_SPACE_WIDTH_SCALE_PERCENT 100
#define DEF_MIN_SPACE_CONDENSING_PERCENT 50
#define DEF_UNUSED_SPACE_THRESHOLD_PERCENT 5
#define DEF_MAX_ADDED_LETTER_SPACING_PERCENT 0

#define NODE_DISPLAY_STYLE_HASH_UNINITIALIZED 0xFFFFFFFF

// To be used for 'direction' in ldomNode->elementFromPoint(lvPoint pt, int direction)
// and ldomDocument->createXPointer(lvPoint pt, int direction...) as a way to
// self-document what's expected (but the code does > and < comparisons, so
// don't change these values - some clients may also already use 0/1/-1).
// Use PT_DIR_EXACT to find the exact node at pt (with y AND x check),
// which is needed when selecting text or checking if tap is on a link,
// (necessary in table cells or floats, and in RTL text).
// Use PT_DIR_SCAN_* when interested only in finding the slice of a page
// at y (eg. to get the current page top), finding the nearest node in
// direction if pt.y happens to be in some node margin area.
// Use PT_DIR_SCAN_BACKWARD_LOGICAL_* when looking a whole page range
// xpointers, to not miss words on first or last line in bidi/RTL text.
#define PT_DIR_SCAN_BACKWARD_LOGICAL_LAST   -3
#define PT_DIR_SCAN_BACKWARD_LOGICAL_FIRST  -2
#define PT_DIR_SCAN_BACKWARD                -1
#define PT_DIR_EXACT                         0
#define PT_DIR_SCAN_FORWARD                  1
#define PT_DIR_SCAN_FORWARD_LOGICAL_FIRST    2
#define PT_DIR_SCAN_FORWARD_LOGICAL_LAST     3


//#if BUILD_LITE!=1
/// final block cache
typedef LVRef<LFormattedText> LFormattedTextRef;
typedef LVCacheMap< ldomNode *, LFormattedTextRef> CVRendBlockCache;
//#endif


//#define LDOM_USE_OWN_MEM_MAN 0
/// XPath step kind
typedef enum {
	xpath_step_error = 0, // error
	xpath_step_element,   // element of type 'name' with 'index'        /elemname[N]/
	xpath_step_text,      // text node with 'index'                     /text()[N]/
	xpath_step_nodeindex, // node index                                 /N/
	xpath_step_point      // point index                                .N
} xpath_step_t;
xpath_step_t ParseXPathStep( const lChar8 * &path, lString8 & name, int & index );

/// return value for continuous operations
typedef enum {
    CR_DONE,    ///< operation is finished successfully
    CR_TIMEOUT, ///< operation is incomplete - interrupted by timeout
    CR_ERROR    ///< error while executing operation
} ContinuousOperationResult;

/// type of image scaling
typedef enum {
    IMG_NO_SCALE,        /// scaling is disabled
    IMG_INTEGER_SCALING, /// integer multipier/divisor scaling -- *2, *3 only
    IMG_FREE_SCALING     /// free scaling, non-integer factor
} img_scaling_mode_t;

enum XPointerMode {
    XPATH_USE_NAMES = 0,
    XPATH_USE_INDEXES
};

/// image scaling option
struct img_scaling_option_t {
    img_scaling_mode_t mode;
    int max_scale;
    int getHash() { return (int)mode * 33 + max_scale; }
    // creates default option value
    img_scaling_option_t();
};

/// set of images scaling options for different kind of images
struct img_scaling_options_t {
    img_scaling_option_t zoom_in_inline;
    img_scaling_option_t zoom_in_block;
    img_scaling_option_t zoom_out_inline;
    img_scaling_option_t zoom_out_block;
    /// returns hash value
    int getHash() { return (((zoom_in_inline.getHash()*33 + zoom_in_block.getHash())*33 + zoom_out_inline.getHash())*33 + zoom_out_block.getHash()); }
    /// creates default options
    img_scaling_options_t();
    /// returns true if any changes occured
    bool update( CRPropRef props, int fontSize );
};

//#if BUILD_LITE!=1
struct DataStorageItemHeader;
struct TextDataStorageItem;
struct ElementDataStorageItem;
struct NodeItem;
class DataBuffer;
//#endif


/// DocView Callback interface - track progress, external links, etc.
class LVDocViewCallback {
public:
    /// on starting file loading
    virtual void OnLoadFileStart( lString32 filename ) { CR_UNUSED(filename); }
    /// format detection finished
    virtual void OnLoadFileFormatDetected( doc_format_t /*fileFormat*/) { }
    /// file loading is finished successfully - drawCoveTo() may be called there
    virtual void OnLoadFileEnd() { }
    /// first page is loaded from file an can be formatted for preview
    virtual void OnLoadFileFirstPagesReady() { }
    /// file progress indicator, called with values 0..100
    virtual void OnLoadFileProgress( int /*percent*/) { }
    /// file load finiished with error
    virtual void OnLoadFileError(lString32 /*message*/) { }
    /// node style update started
    virtual void OnNodeStylesUpdateStart() { }
    /// node style update finished
    virtual void OnNodeStylesUpdateEnd() { }
    /// node style update progress, called with values 0..100
    virtual void OnNodeStylesUpdateProgress(int /*percent*/) { }
    /// document formatting started
    virtual void OnFormatStart() { }
    /// document formatting finished
    virtual void OnFormatEnd() { }
    /// format progress, called with values 0..100
    virtual void OnFormatProgress(int /*percent*/) { }
    /// document fully loaded and rendered (follows OnFormatEnd(), or OnLoadFileEnd() when loaded from cache)
    virtual void OnDocumentReady() { }
    /// format progress, called with values 0..100
    virtual void OnExportProgress(int /*percent*/) { }
    /// Override to handle external links
    virtual void OnExternalLink(lString32 /*url*/, ldomNode * /*node*/) { }
    /// Called when page images should be invalidated (clearImageCache() called in LVDocView)
    virtual void OnImageCacheClear() { }
    /// return true if reload will be processed by external code, false to let internal code process it
    virtual bool OnRequestReload() { return false; }
    /// save cache file started
    virtual void OnSaveCacheFileStart() { }
    /// save cache file finished
    virtual void OnSaveCacheFileEnd() { }
    /// save cache file progress, called with values 0..100
    virtual void OnSaveCacheFileProgress(int /*percent*/) { }
    /// destructor
    virtual ~LVDocViewCallback() { }
};

class CacheLoadingCallback
{
public:
    /// called when format of document being loaded from cache became known
    virtual void OnCacheFileFormatDetected( doc_format_t ) = 0;
    virtual ~CacheLoadingCallback() { }
};


class ldomTextStorageChunk;
class ldomTextStorageChunkBuilder;
struct ElementDataStorageItem;
class CacheFile;
class tinyNodeCollection;

struct ldomNodeStyleInfo
{
    lUInt16 _fontIndex;
    lUInt16 _styleIndex;
};

class ldomBlobItem;
#define BLOB_NAME_PREFIX U"@blob#"
#define MOBI_IMAGE_NAME_PREFIX U"mobi_image_"
class ldomBlobCache
{
    CacheFile * _cacheFile;
    LVPtrVector<ldomBlobItem> _list;
    bool _changed;
    bool loadIndex();
    bool saveIndex();
public:
    ldomBlobCache();
    void setCacheFile( CacheFile * cacheFile );
    ContinuousOperationResult saveToCache(CRTimerUtil & timeout);
    bool addBlob( const lUInt8 * data, int size, lString32 name );
    LVStreamRef getBlob( lString32 name );
};

class ldomDataStorageManager
{
    friend class ldomTextStorageChunk;
protected:
    tinyNodeCollection * _owner;
    LVPtrVector<ldomTextStorageChunk> _chunks;
    ldomTextStorageChunk * _activeChunk;
    ldomTextStorageChunk * _recentChunk;
    CacheFile * _cache;
    lUInt32 _uncompressedSize;
    lUInt32 _maxUncompressedSize;
    lUInt32 _chunkSize;
    char _type;       /// type, to show in log
    bool _maxSizeReachedWarned;
    ldomTextStorageChunk * getChunk( lUInt32 address );
public:
    /// type
    lUInt16 cacheType();
    /// saves all unsaved chunks to cache file
    bool save( CRTimerUtil & maxTime );
    /// load chunk index from cache file
    bool load();
    /// sets cache file
    void setCache( CacheFile * cache );
    /// checks buffer sizes, compacts most unused chunks
    void compact( int reservedSpace , const ldomTextStorageChunk *excludedChunk = NULL );
    lUInt32 getUncompressedSize() { return _uncompressedSize; }
#if BUILD_LITE!=1
    /// allocates new text node, return its address inside storage
    lUInt32 allocText( lUInt32 dataIndex, lUInt32 parentIndex, const lString8 & text );
    /// allocates storage for new element, returns address address inside storage
    lUInt32 allocElem( lUInt32 dataIndex, lUInt32 parentIndex, int childCount, int attrCount );
    /// get text by address
    lString8 getText( lUInt32 address );
    /// get pointer to text data
    TextDataStorageItem * getTextItem( lUInt32 addr );
    /// get pointer to element data
    ElementDataStorageItem * getElem( lUInt32 addr );
    /// change node's parent, returns true if modified
    bool setParent( lUInt32 address, lUInt32 parent );
    /// returns node's parent by address
    lUInt32 getParent( lUInt32 address );
    /// free data item
    void freeNode( lUInt32 addr );
    /// call to invalidate chunk if content is modified
    void modified( lUInt32 addr );
    /// return true if some chunks have been allocated
    bool hasChunks() { return _chunks.length() > 0; }
#endif
    
    /// get or allocate space for rect data item
    void getRendRectData( lUInt32 elemDataIndex, lvdomElementFormatRec * dst );
    /// set rect data item
    void setRendRectData( lUInt32 elemDataIndex, const lvdomElementFormatRec * src );

    /// get or allocate space for element style data item
    void getStyleData( lUInt32 elemDataIndex, ldomNodeStyleInfo * dst );
    /// set element style data item
    void setStyleData( lUInt32 elemDataIndex, const ldomNodeStyleInfo * src );

    ldomDataStorageManager( tinyNodeCollection * owner, char type, lUInt32 maxUnpackedSize, lUInt32 chunkSize );
    ~ldomDataStorageManager();
};

/// class to store compressed/uncompressed text nodes chunk
class ldomTextStorageChunk
{
    friend class ldomDataStorageManager;
    ldomDataStorageManager * _manager;
    ldomTextStorageChunk * _nextRecent;
    ldomTextStorageChunk * _prevRecent;
    lUInt8 * _buf;     /// buffer for uncompressed data
    lUInt32 _bufsize;  /// _buf (uncompressed) area size, bytes
    lUInt32 _bufpos;  /// _buf (uncompressed) data write position (for appending of new data)
    lUInt16 _index;  /// ? index of chunk in storage
    char _type;       /// type, to show in log
    bool _saved;

    void setunpacked( const lUInt8 * buf, int bufsize );
    /// pack data, and remove unpacked
    void compact();
#if BUILD_LITE!=1
    /// pack data, and remove unpacked, put packed data to cache file
    bool swapToCache( bool removeFromMemory );
    /// read packed data from cache
    bool restoreFromCache();
#endif
    /// unpacks chunk, if packed; checks storage space, compact if necessary
    void ensureUnpacked();
#if BUILD_LITE!=1
    /// free data item
    void freeNode( int offset );
    /// saves data to cache file, if unsaved
    bool save();
#endif
public:
    /// call to invalidate chunk if content is modified
    void modified();
    /// returns chunk index inside collection
    int getIndex() { return _index; }
    /// returns free space in buffer
    int space();
    /// adds new text item to buffer, returns offset inside chunk of stored data
    int addText( lUInt32 dataIndex, lUInt32 parentIndex, const lString8 & text );
    /// adds new element item to buffer, returns offset inside chunk of stored data
    int addElem( lUInt32 dataIndex, lUInt32 parentIndex, int childCount, int attrCount );
    /// get text item from buffer by offset
    lString8 getText( int offset );
    /// get node parent by offset
    lUInt32 getParent( int offset );
    /// set node parent by offset
    bool setParent( int offset, lUInt32 parentIndex );
    /// get pointer to element data
    ElementDataStorageItem * getElem( int offset );
    /// get raw data bytes
    void getRaw( int offset, int size, lUInt8 * buf );
    /// set raw data bytes
    void setRaw( int offset, int size, const lUInt8 * buf );
    /// create empty buffer
    ldomTextStorageChunk(ldomDataStorageManager * manager, lUInt16 index);
    /// create chunk to be read from cache file
    ldomTextStorageChunk(ldomDataStorageManager * manager, lUInt16 index, lUInt32 compsize, lUInt32 uncompsize);
    /// create with preallocated buffer, for raw access
    ldomTextStorageChunk(lUInt32 preAllocSize, ldomDataStorageManager * manager, lUInt16 index);
    ~ldomTextStorageChunk();
};

// forward declaration
struct ldomNode;

// About these #define TNC_PART_* :
// A ldomNode unique reference is defined by:
//    struct ldomNodeHandle {     /// compact 32bit value for node
//        unsigned _docIndex:8;   // index in ldomNode::_documentInstances[MAX_DOCUMENT_INSTANCE_COUNT];
//        unsigned _dataIndex:24; // index of node in document's storage and type
//    };
// The 24 bits of _dataIndex are used that way:
//        return &(_elemList[index>>TNC_PART_INDEX_SHIFT][(index>>4)&TNC_PART_MASK]);
//        #define TNTYPE  (_handle._dataIndex&0x0F)
//        #define TNINDEX (_handle._dataIndex&(~0x0E))
//   24>15 10bits (1024 values) : index in the first-level _elemList[TNC_PART_COUNT]
//   14> 5 10bits (1024 values) : sub-index in second-level _elemList[first_index][]
//    4> 1  4bits (16 values) : type (bit 1: text | element, bit 2: mutable | permanent)
//                                   (bit 3 and 4 are not used, so we could grab 2 more bits from here if needed)
//
// We can update ldomNodeHandle to:
//    struct ldomNodeHandle {
//        unsigned _docIndex:4;   // decreasing MAX_DOCUMENT_INSTANCE_COUNT from 256 to 16
//        unsigned _dataIndex:28; // get 4 more bits that we can distribute to these indexes.
//    };
// The other #define below (and possibly the code too) assume the same TNC_PART_SHIFT for both indexes,
// so let's distribute 2 bits to each:
//   28>17 12bits (4096 values) : index in the first-level _elemList[TNC_PART_COUNT]
//   16> 5 12bits (4096 values) : sub-index in second-level _elemList[first_index][]
//    4> 1  4bits (16 values)
// With that, we have increased the max number of text nodes and the max number of
// element nodes from 1024x1024 (1M) to 4096x4096 (16M) which allows loading very large books.

//#define TNC_PART_COUNT 1024
//#define TNC_PART_SHIFT 10
#define TNC_PART_COUNT 4096
#define TNC_PART_SHIFT 12
#define TNC_PART_INDEX_SHIFT (TNC_PART_SHIFT+4)
#define TNC_PART_LEN (1<<TNC_PART_SHIFT)
#define TNC_PART_MASK (TNC_PART_LEN-1)
/// storage of ldomNode
class tinyNodeCollection
{
    friend struct ldomNode;
    friend class tinyElement;
    friend class ldomDocument;
private:
    int _textCount;
    lUInt32 _textNextFree;
    ldomNode * _textList[TNC_PART_COUNT];
    int _elemCount;
    lUInt32 _elemNextFree;
    ldomNode * _elemList[TNC_PART_COUNT];
    LVIndexedRefCache<css_style_ref_t> _styles;
    LVIndexedRefCache<font_ref_t> _fonts;
    int _tinyElementCount;
    int _itemCount;
    int _docIndex;

protected:
#if BUILD_LITE!=1
    /// final block cache
    CVRendBlockCache _renderedBlockCache;
    CacheFile * _cacheFile;
    bool _cacheFileStale;
    bool _cacheFileLeaveAsDirty;
    bool _mapped;
    bool _maperror;
    int  _mapSavingStage;

    img_scaling_options_t _imgScalingOptions;
    int  _spaceWidthScalePercent;
    int  _minSpaceCondensingPercent;
    int  _unusedSpaceThresholdPercent;
    int  _maxAddedLetterSpacingPercent;

    lUInt32 _nodeStyleHash;
    lUInt32 _nodeDisplayStyleHash;
    lUInt32 _nodeDisplayStyleHashInitial;
    bool _nodeStylesInvalidIfLoading;

    int calcFinalBlocks();
    void dropStyles();
#endif
    bool _hangingPunctuationEnabled;
    lUInt32 _renderBlockRenderingFlags;
    lUInt32 _DOMVersionRequested;
    int _interlineScaleFactor;

    ldomDataStorageManager _textStorage; // persistent text node data storage
    ldomDataStorageManager _elemStorage; // persistent element data storage
    ldomDataStorageManager _rectStorage; // element render rect storage
    ldomDataStorageManager _styleStorage;// element style storage (font & style indexes ldomNodeStyleInfo)

    CRPropRef _docProps;
    lUInt32 _docFlags; // document flags

    int _styleIndex;

    LVStyleSheet  _stylesheet;

    LVHashTable<lUInt16, lUInt16> _fontMap; // style index to font index

    /// checks buffer sizes, compacts most unused chunks
    ldomBlobCache _blobCache;

    /// uniquie id of file format parsing option (usually 0, but 1 for preformatted text files)
    int getPersistenceFlags();

#if BUILD_LITE!=1
    bool saveStylesData();
    bool loadStylesData();
    bool updateLoadedStyles( bool enabled );
    lUInt32 calcStyleHash(bool already_rendered);
    bool saveNodeData();
    bool saveNodeData( lUInt16 type, ldomNode ** list, int nodecount );
    bool loadNodeData();
    bool loadNodeData( lUInt16 type, ldomNode ** list, int nodecount );

    bool hasRenderData() { return _rectStorage.hasChunks(); }

    bool openCacheFile();

    void setNodeStyleIndex( lUInt32 dataIndex, lUInt16 index );
    void setNodeFontIndex( lUInt32 dataIndex, lUInt16 index );
    lUInt16 getNodeStyleIndex( lUInt32 dataIndex );
    lUInt16 getNodeFontIndex( lUInt32 dataIndex );
    css_style_ref_t getNodeStyle( lUInt32 dataIndex );
    font_ref_t getNodeFont( lUInt32 dataIndex );
    void setNodeStyle( lUInt32 dataIndex, css_style_ref_t & v );
    void setNodeFont( lUInt32 dataIndex, font_ref_t & v  );
    void clearNodeStyle( lUInt32 dataIndex );
    virtual void resetNodeNumberingProps() { }
#endif

    /// creates empty collection
    tinyNodeCollection();
    tinyNodeCollection( tinyNodeCollection & v );

public:

#if BUILD_LITE!=1
    int getSpaceWidthScalePercent() {
        return _spaceWidthScalePercent;
    }

    bool setSpaceWidthScalePercent(int spaceWidthScalePercent) {
        if (spaceWidthScalePercent == _spaceWidthScalePercent)
            return false;
        _spaceWidthScalePercent = spaceWidthScalePercent;
        return true;
    }

    bool setMinSpaceCondensingPercent(int minSpaceCondensingPercent) {
        if (minSpaceCondensingPercent == _minSpaceCondensingPercent)
            return false;
        _minSpaceCondensingPercent = minSpaceCondensingPercent;
        return true;
    }

    bool setUnusedSpaceThresholdPercent(int unusedSpaceThresholdPercent) {
        if (unusedSpaceThresholdPercent == _unusedSpaceThresholdPercent)
            return false;
        _unusedSpaceThresholdPercent = unusedSpaceThresholdPercent;
        return true;
    }

    bool setMaxAddedLetterSpacingPercent(int maxAddedLetterSpacingPercent) {
        if (maxAddedLetterSpacingPercent == _maxAddedLetterSpacingPercent)
            return false;
        _maxAddedLetterSpacingPercent = maxAddedLetterSpacingPercent;
        // This does not need to trigger a re-rendering, just
        // a re-formatting of the final blocks
        _renderedBlockCache.clear();
        return true;
    }

    /// add named BLOB data to document
    bool addBlob(lString32 name, const lUInt8 * data, int size) { _cacheFileStale = true ; return _blobCache.addBlob(data, size, name); }
    /// get BLOB by name
    LVStreamRef getBlob(lString32 name) { return _blobCache.getBlob(name); }

    /// called on document loading end
    bool validateDocument();

    /// swaps to cache file or saves changes, limited by time interval (can be called again to continue after TIMEOUT)
    virtual ContinuousOperationResult swapToCache(CRTimerUtil & maxTime) = 0;
    /// try opening from cache file, find by source file name (w/o path) and crc32
    virtual bool openFromCache( CacheLoadingCallback * formatCallback, LVDocViewCallback * progressCallback=NULL ) = 0;
    /// saves recent changes to mapped file, with timeout (can be called again to continue after TIMEOUT)
    virtual ContinuousOperationResult updateMap(CRTimerUtil & maxTime, LVDocViewCallback * progressCallback=NULL) = 0;
    /// saves recent changes to mapped file
    virtual bool updateMap(LVDocViewCallback * progressCallback=NULL) {
        CRTimerUtil infinite;
        return updateMap(infinite, progressCallback)!=CR_ERROR;
    }

    bool swapToCacheIfNecessary();


    bool createCacheFile();
#endif

    bool getHangingPunctiationEnabled() const {
        return _hangingPunctuationEnabled;
    }
    bool setHangingPunctiationEnabled(bool value);

    lUInt32 getRenderBlockRenderingFlags() const {
        return _renderBlockRenderingFlags;
    }
    bool setRenderBlockRenderingFlags(lUInt32 flags);

    lUInt32 getDOMVersionRequested() const {
        return _DOMVersionRequested;
    }
    bool setDOMVersionRequested(lUInt32 version);

    int getInterlineScaleFactor() const {
        return _interlineScaleFactor;
    }
    bool setInterlineScaleFactor(int value);

    inline bool getDocFlag( lUInt32 mask )
    {
        return (_docFlags & mask) != 0;
    }

    void setDocFlag( lUInt32 mask, bool value );

    inline lUInt32 getDocFlags()
    {
        return _docFlags;
    }

    inline int getDocIndex()
    {
        return _docIndex;
    }

    inline int getFontContextDocIndex()
    {
        return (_docFlags & DOC_FLAG_ENABLE_DOC_FONTS) && (_docFlags & DOC_FLAG_ENABLE_INTERNAL_STYLES) ? _docIndex : -1;
    }

    void setDocFlags( lUInt32 value );


    /// returns doc properties collection
    inline CRPropRef getProps() { return _docProps; }
    /// returns doc properties collection
    void setProps( CRPropRef props ) { _docProps = props; }

#if BUILD_LITE!=1
    /// set cache file stale flag
    void setCacheFileStale( bool stale ) { _cacheFileStale = stale; }

    /// is built (and cached) DOM possibly invalid (can happen when some nodes have changed display style)
    bool isBuiltDomStale() {
        return _nodeDisplayStyleHashInitial != NODE_DISPLAY_STYLE_HASH_UNINITIALIZED &&
                _nodeDisplayStyleHash != _nodeDisplayStyleHashInitial;
    }
    void setNodeStylesInvalidIfLoading() {
        _nodeStylesInvalidIfLoading = true;
    }

    /// if a cache file is in use
    bool hasCacheFile() { return _cacheFile != NULL; }
    /// set cache file as dirty, so it's not re-used on next load
    void invalidateCacheFile() { _cacheFileLeaveAsDirty = true; }
    /// get cache file full path
    lString32 getCacheFilePath();
#endif

    /// minimize memory consumption
    void compact();
    /// dumps memory usage statistics to debug log
    void dumpStatistics();
    /// get memory usage statistics
    lString32 getStatistics();

    /// get ldomNode instance pointer
    ldomNode * getTinyNode( lUInt32 index );
    /// allocate new ldomNode
    ldomNode * allocTinyNode( int type );
    /// allocate new tinyElement
    ldomNode * allocTinyElement( ldomNode * parent, lUInt16 nsid, lUInt16 id );
    /// recycle ldomNode on node removing
    void recycleTinyNode( lUInt32 index );



#if BUILD_LITE!=1
    /// put all object into persistent storage
    virtual void persist( CRTimerUtil & maxTime );
#endif


    /// destroys collection
    virtual ~tinyNodeCollection();
};

class ldomDocument;
class tinyElement;
struct lxmlAttribute;

#if BUILD_LITE!=1
class RenderRectAccessor : public lvdomElementFormatRec
{
    ldomNode * _node;
    bool _modified;
    bool _dirty;
public:
    //RenderRectAccessor & operator -> () { return *this; }
    int getX();
    int getY();
    int getWidth();
    int getHeight();
    void getRect( lvRect & rc );
    void setX( int x );
    void setY( int y );
    void setWidth( int w );
    void setHeight( int h );

    int getInnerWidth();
    int getInnerX();
    int getInnerY();
    void setInnerX( int x );
    void setInnerY( int y );
    void setInnerWidth( int w );

    int  getUsableLeftOverflow();
    int  getUsableRightOverflow();
    void setUsableLeftOverflow( int dx );
    void setUsableRightOverflow( int dx );

    int  getTopOverflow();
    int  getBottomOverflow();
    void setTopOverflow( int dy );
    void setBottomOverflow( int dy );

    int  getBaseline();
    void setBaseline( int baseline );
    int  getListPropNodeIndex();
    void setListPropNodeIndex( int idx );
    int  getLangNodeIndex();
    void setLangNodeIndex( int idx );

    unsigned short getFlags();
    void setFlags( unsigned short flags );

    void getTopRectsExcluded( int & lw, int & lh, int & rw, int & rh );
    void setTopRectsExcluded( int lw, int lh, int rw, int rh );
    void getNextFloatMinYs( int & left, int & right );
    void setNextFloatMinYs( int left, int right );
    void getInvolvedFloatIds( int & float_count, lUInt32 * float_ids );
    void setInvolvedFloatIds( int float_count, lUInt32 * float_ids );

    void push();
    void clear();
    RenderRectAccessor( ldomNode * node );
    ~RenderRectAccessor();
};
#endif

/// compact 32bit value for node
struct ldomNodeHandle {
    // See comment above around #define TNC_PART_COUNT and TNC_PART_SHIFT changes
    // Original crengine field sizes:
    // unsigned _docIndex:8;
    // unsigned _dataIndex:24;
    unsigned _docIndex:4;   // index in ldomNode::_documentInstances[MAX_DOCUMENT_INSTANCE_COUNT];
    unsigned _dataIndex:28; // index of node in document's storage and type
};

/// max number which could be stored in ldomNodeHandle._docIndex
// #define MAX_DOCUMENT_INSTANCE_COUNT 256
#define MAX_DOCUMENT_INSTANCE_COUNT 16


class ldomTextNode;
// no vtable, very small size (16 bytes)
// optimized for 32 bit systems
struct ldomNode
{
    friend class tinyNodeCollection;
    friend class RenderRectAccessor;
    friend class NodeImageProxy;
    friend class ldomDocument;

private:

    static ldomDocument * _documentInstances[MAX_DOCUMENT_INSTANCE_COUNT];

    /// adds document to list, returns ID of allocated document, -1 if no space in instance array
    static int registerDocument( ldomDocument * doc );
    /// removes document from list
    static void unregisterDocument( ldomDocument * doc );

    // types for _handle._type
    enum {
        NT_TEXT=0,       // mutable text node
        NT_ELEMENT=1    // mutable element node
#if BUILD_LITE!=1
        ,
        NT_PTEXT=2,      // immutable (persistent) text node
        NT_PELEMENT=3   // immutable (persistent) element node
#endif
    };

    /// 0: packed 32bit data field
    ldomNodeHandle _handle; // _docIndex, _dataIndex, _type

    /// 4: misc data 4 bytes (8 bytes on x64)
    union {                    // [8] 8 bytes (16 bytes on x64)
        ldomTextNode * _text_ptr;   // NT_TEXT: mutable text node pointer
        tinyElement * _elem_ptr;    // NT_ELEMENT: mutable element pointer
#if BUILD_LITE!=1
        lUInt32 _pelem_addr;        // NT_PELEMENT: element storage address: chunk+offset
        lUInt32 _ptext_addr;        // NT_PTEXT: persistent text storage address: chunk+offset
#endif
        lUInt32 _nextFreeIndex;     // NULL for removed items
    } _data;


    /// sets document for node
    inline void setDocumentIndex( int index ) { _handle._docIndex = index; }
    void setStyleIndexInternal( lUInt16 index );
    void setFontIndexInternal( lUInt16 index );


#define TNTYPE  (_handle._dataIndex&0x0F)
#define TNINDEX (_handle._dataIndex&(~0x0E))
#define TNCHUNK (_addr>>&(~0x0F))
    void onCollectionDestroy();
    inline ldomNode * getTinyNode( lUInt32 index ) const { return ((tinyNodeCollection*)getDocument())->getTinyNode(index); }

    void operator delete(void *)
    {
        // Do nothing. Just to disable delete.
    }

    /// changes parent of item
    void setParentNode( ldomNode * newParent );
    /// add child
    void addChild( lInt32 childNodeIndex );

    /// call to invalidate cache if persistent node content is modified
    void modified();

    /// returns copy of render data structure
    void getRenderData( lvdomElementFormatRec & dst);
    /// sets new value for render data structure
    void setRenderData( lvdomElementFormatRec & newData);

    void autoboxChildren( int startIndex, int endIndex, bool handleFloating=false );
    void removeChildren( int startIndex, int endIndex );
    bool cleanIfOnlyEmptyTextInline( bool handleFloating=false );
    /// returns true if element has inline content (non empty text, images, <BR>)
    bool hasNonEmptyInlineContent( bool ignoreFloats=false );

public:
#if BUILD_LITE!=1
    // Generic version of autoboxChildren() without any specific inline/block checking,
    // accepting any element id (from the enum el_*, like el_div, el_tabularBox) as
    // the wrapping element.
    ldomNode * boxWrapChildren( int startIndex, int endIndex, lUInt16 elementId );

    // Ensure this node has a ::before/::after pseudo element as
    // child, creating it if needed and possible
    void ensurePseudoElement( bool is_before );

    /// if stylesheet file name is set, and file is found, set stylesheet to its value
    bool applyNodeStylesheet();

    bool initNodeFont();
    void initNodeStyle();
    /// init render method for this node only (children should already have rend method set)
    void initNodeRendMethod();
    /// init render method for the whole subtree
    void initNodeRendMethodRecursive();
    /// init render method for the whole subtree
    void initNodeStyleRecursive( LVDocViewCallback * progressCallback );
#endif


    /// remove node, clear resources
    void destroy();

    /// returns true for invalid/deleted node ot NULL this pointer
    inline bool isNull() const { return _handle._dataIndex==0 || getDocument() == NULL; }
    /// returns true if node is stored in persistent storage
    inline bool isPersistent() const { return (_handle._dataIndex&2)!=0; }
    /// returns data index of node's registration in document data storage
    inline lInt32 getDataIndex() const { return TNINDEX; }
    /// returns pointer to document
    inline ldomDocument * getDocument() const { return _documentInstances[_handle._docIndex]; }
    /// returns pointer to parent node, NULL if node has no parent
    ldomNode * getParentNode() const;
    /// returns node type, either LXML_TEXT_NODE or LXML_ELEMENT_NODE
    inline lUInt8 getNodeType() const
    {
        return (_handle._dataIndex & 1) ? LXML_ELEMENT_NODE : LXML_TEXT_NODE;
    }
    /// returns node level, 0 is root node
    lUInt8 getNodeLevel() const;
    /// returns dataIndex of node's parent, 0 if no parent
    int getParentIndex() const;
    /// returns index of node inside parent's child collection
    int getNodeIndex() const;
    /// returns index of child node by dataIndex
    int getChildIndex( lUInt32 dataIndex ) const;
    /// returns true if node is document's root
    bool isRoot() const;
    /// returns true if node is text
    inline bool isText() const { return _handle._dataIndex && !(_handle._dataIndex&1); }
    /// returns true if node is element
    inline bool isElement() const { return _handle._dataIndex && (_handle._dataIndex&1); }
    /// returns true if node is and element that has children
    inline bool hasChildren() { return getChildCount()!=0; }
    /// returns true if node is element has attributes
    inline bool hasAttributes() const { return getAttrCount()!=0; }

    /// returns element child count
    int getChildCount() const;
    /// returns element attribute count
    int getAttrCount() const;
    /// returns attribute value by attribute name id and namespace id
    const lString32 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const;
    /// returns attribute value by attribute name
    inline const lString32 & getAttributeValue( const lChar32 * attrName ) const
    {
        return getAttributeValue( NULL, attrName );
    }
    /// returns attribute value by attribute name
    inline const lString32 & getAttributeValue( const lChar8 * attrName ) const
    {
        return getAttributeValue( NULL, attrName );
    }
    /// returns attribute value by attribute name and namespace
    const lString32 & getAttributeValue( const lChar32 * nsName, const lChar32 * attrName ) const;
    /// returns attribute value by attribute name and namespace
    const lString32 & getAttributeValue( const lChar8 * nsName, const lChar8 * attrName ) const;
    /// returns attribute by index
    const lxmlAttribute * getAttribute( lUInt32 ) const;
    /// returns true if element node has attribute with specified name id and namespace id
    bool hasAttribute( lUInt16 nsId, lUInt16 attrId ) const;
    /// returns attribute name by index
    const lString32 & getAttributeName( lUInt32 ) const;
    /// sets attribute value
    void setAttributeValue( lUInt16 , lUInt16 , const lChar32 *  );
    /// returns attribute value by attribute name id
    inline const lString32 & getAttributeValue( lUInt16 id ) const { return getAttributeValue( LXML_NS_ANY, id ); }
    /// returns true if element node has attribute with specified name id
    inline bool hasAttribute( lUInt16 id ) const  { return hasAttribute( LXML_NS_ANY, id ); }

    /// returns attribute value by attribute name id, looking at children if needed
    const lString32 & getFirstInnerAttributeValue( lUInt16 nsid, lUInt16 id ) const;
    const lString32 & getFirstInnerAttributeValue( lUInt16 id ) const { return getFirstInnerAttributeValue( LXML_NS_ANY, id ); }

    /// returns element type structure pointer if it was set in document for this element name
    const css_elem_def_props_t * getElementTypePtr();
    /// returns element name id
    lUInt16 getNodeId() const;
    /// returns element namespace id
    lUInt16 getNodeNsId() const;
    /// replace element name id with another value
    void setNodeId( lUInt16 );
    /// returns element name
    const lString32 & getNodeName() const;
    /// compares node name with value, returns true if matches
    bool isNodeName(const char * name) const;
    /// returns element namespace name
    const lString32 & getNodeNsName() const;

    /// returns child node by index
    ldomNode * getChildNode( lUInt32 index ) const;
    /// returns true child node is element
    bool isChildNodeElement( lUInt32 index ) const;
    /// returns true child node is text
    bool isChildNodeText( lUInt32 index ) const;
    /// returns child node by index, NULL if node with this index is not element or nodeId!=0 and element node id!=nodeId
    ldomNode * getChildElementNode( lUInt32 index, lUInt16 nodeId=0 ) const;
    /// returns child node by index, NULL if node with this index is not element or nodeTag!=0 and element node name!=nodeTag
    ldomNode * getChildElementNode( lUInt32 index, const lChar32 * nodeTag ) const;

    /// returns text node text as wide string
    lString32 getText( lChar32 blockDelimiter = 0, int maxSize=0 ) const;
    /// returns text node text as utf8 string
    lString8 getText8( lChar8 blockDelimiter = 0, int maxSize=0 ) const;
    /// sets text node text as wide string
    void setText( lString32 );
    /// sets text node text as utf8 string
    void setText8( lString8 );


    /// returns node absolute rectangle (with inner=true, for erm_final, additionally
    //  shifted by the inner paddings (exluding padding bottom) to get the absolute rect
    //  of the inner LFormattedText.
    void getAbsRect( lvRect & rect, bool inner=false );
    /// sets node rendering structure pointer
    void clearRenderData();
    /// reset node rendering structure pointer for sub-tree
    void clearRenderDataRecursive();
    /// calls specified function recursively for all elements of DOM tree
    void recurseElements( void (*pFun)( ldomNode * node ) );
    /// calls specified function recursively for all elements of DOM tree matched by matchFun
    void recurseMatchingElements( void (*pFun)( ldomNode * node ), bool (*matchFun)( ldomNode * node ) );
    /// calls specified function recursively for all elements of DOM tree, children before parent
    void recurseElementsDeepFirst( void (*pFun)( ldomNode * node ) );
    /// calls specified function recursively for all nodes of DOM tree
    void recurseNodes( void (*pFun)( ldomNode * node ) );


    /// returns first text child element
    ldomNode * getFirstTextChild( bool skipEmpty=false );
    /// returns last text child element
    ldomNode * getLastTextChild();

#if BUILD_LITE!=1
    /// find node by coordinates of point in formatted document
    ldomNode * elementFromPoint( lvPoint pt, int direction, bool strict_bounds_checking=false );
    /// find final node by coordinates of point in formatted document
    ldomNode * finalBlockFromPoint( lvPoint pt );
#endif

    // rich interface stubs for supporting Element operations
    /// returns rendering method
    lvdom_element_render_method getRendMethod();
    /// sets rendering method
    void setRendMethod( lvdom_element_render_method );
#if BUILD_LITE!=1
    /// returns element style record
    css_style_ref_t getStyle() const;
    /// returns element font
    font_ref_t getFont();
    /// sets element font
    void setFont( font_ref_t );
    /// sets element style record
    void setStyle( css_style_ref_t & );
#endif
    /// returns first child node
    ldomNode * getFirstChild() const;
    /// returns last child node
    ldomNode * getLastChild() const;
    /// removes and deletes last child element
    void removeLastChild();
    /// move range of children startChildIndex to endChildIndex inclusively to specified element
    void moveItemsTo( ldomNode *, int , int );
    /// find child element by tag id
    ldomNode * findChildElement( lUInt16 nsid, lUInt16 id, int index );
    /// find child element by id path
    ldomNode * findChildElement( lUInt16 idPath[] );
    /// inserts child element
    ldomNode * insertChildElement( lUInt32 index, lUInt16 nsid, lUInt16 id );
    /// inserts child element
    ldomNode * insertChildElement( lUInt16 id );
    /// inserts child text
    ldomNode * insertChildText( lUInt32 index, const lString32 & value );
    /// inserts child text
    ldomNode * insertChildText( const lString32 & value );
    /// inserts child text
    ldomNode * insertChildText(const lString8 & value, bool before_last_child=false);
    /// remove child
    ldomNode * removeChild( lUInt32 index );

    /// returns XPath segment for this element relative to parent element (e.g. "p[10]")
    lString32 getXPathSegment();

    /// creates stream to read base64 encoded data from element
    LVStreamRef createBase64Stream();
#if BUILD_LITE!=1
    /// returns object image source
    LVImageSourceRef getObjectImageSource();
    /// returns object image ref name
    lString32 getObjectImageRefName( bool percentDecode=true );
    /// returns object image stream
    LVStreamRef getObjectImageStream();
    /// returns the sum of this node and its parents' top and bottom margins, borders and paddings
    int getSurroundingAddedHeight();
    /// formats final block
    int renderFinalBlock( LFormattedTextRef & frmtext, RenderRectAccessor * fmt, int width,
                              BlockFloatFootprint * float_footprint=NULL );
    /// formats final block again after change, returns true if size of block is changed
    bool refreshFinalBlock();
#endif
    /// replace node with r/o persistent implementation
    ldomNode * persist();
    /// replace node with r/w implementation
    ldomNode * modify();

    /// for display:list-item node, get marker
    bool getNodeListMarker( int & counterValue, lString32 & marker );
    /// is node a floating floatBox
    bool isFloatingBox() const;
    /// is node an inlineBox that has not been re-inlined by having
    /// its child no more inline-block/inline-table
    bool isBoxingInlineBox() const;
    /// is node an inlineBox that wraps a bogus embedded block (not inline-block/inline-table)
    /// can be called with inline_box_checks_done=true when isBoxingInlineBox() has already
    /// been called to avoid rechecking what is known
    bool isEmbeddedBlockBoxingInlineBox(bool inline_box_checks_done=false) const;

    /// is node any of our internal boxing element (or, optionally, our pseudoElem)
    bool isBoxingNode( bool orPseudoElem=false ) const;

    /// return real (as in the original HTML) parent/siblings by skipping any internal
    /// boxing element up or down (returns NULL when no more sibling)
    ldomNode * getUnboxedParent() const;
    ldomNode * getUnboxedFirstChild( bool skip_text_nodes=false ) const;
    ldomNode * getUnboxedLastChild( bool skip_text_nodes=false ) const;
    ldomNode * getUnboxedPrevSibling( bool skip_text_nodes=false ) const;
    ldomNode * getUnboxedNextSibling( bool skip_text_nodes=false ) const;
};


// default: 512K
#define DEF_DOC_DATA_BUFFER_SIZE 0x80000

/// Base class for XML DOM documents
/**
    Helps to decrease memory usage and increase performance for DOM implementations.
    Maintains Name<->Id maps for element names, namespaces and attributes.
    It allows to use short IDs instead of strings in DOM internals,
    and avoid duplication of string values.

	Manages data storage.
*/
class lxmlDocBase : public tinyNodeCollection
{
    friend struct ldomNode;
	friend class ldomXPointer;
protected:


    /// Default constructor
    lxmlDocBase(int dataBufSize = DEF_DOC_DATA_BUFFER_SIZE);
    /// Copy constructor - copies ID tables contents
    lxmlDocBase( lxmlDocBase & doc );
public:
    /// Destructor
    virtual ~lxmlDocBase();

#if BUILD_LITE!=1
	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serializeMaps( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	bool deserializeMaps( SerialBuf & buf );

#endif

    //======================================================================
    // Name <-> Id maps functions

    /// Get namespace name by id
    /**
        \param id is numeric value of namespace
        \return string value of namespace
    */
    inline const lString32 & getNsName( lUInt16 id )
    {
        return _nsNameTable.nameById( id );
    }

    /// Get namespace id by name
    /**
        \param name is string value of namespace
        \return id of namespace
    */
    lUInt16 getNsNameIndex( const lChar32 * name );

    /// Get namespace id by name
    /**
        \param name is string value of namespace (ASCII only)
        \return id of namespace
    */
    lUInt16 getNsNameIndex( const lChar8 * name );

    /// Get attribute name by id
    /**
        \param id is numeric value of attribute
        \return string value of attribute
    */
    inline const lString32 & getAttrName( lUInt16 id )
    {
        return _attrNameTable.nameById( id );
    }

    /// Get attribute id by name
    /**
        \param name is string value of attribute
        \return id of attribute
    */
    lUInt16 getAttrNameIndex( const lChar32 * name );

    /// Get attribute id by name
    /**
        \param name is string value of attribute (8bit ASCII only)
        \return id of attribute
    */
    lUInt16 getAttrNameIndex( const lChar8 * name );

    /// helper: returns attribute value
    inline const lString32 & getAttrValue( lUInt32 index ) const
    {
        return _attrValueTable[index];
    }

    /// helper: returns attribute value index
    inline lUInt32 getAttrValueIndex( const lChar32 * value )
    {
        return (lUInt32)_attrValueTable.add( value );
    }

    /// helper: returns attribute value index, 0xffffffff if not found
    inline lUInt32 findAttrValueIndex( const lChar32 * value )
    {
        return (lUInt32)_attrValueTable.find( value );
    }

    /// Get element name by id
    /**
        \param id is numeric value of element name
        \return string value of element name
    */
    inline const lString32 & getElementName( lUInt16 id )
    {
        return _elementNameTable.nameById( id );
    }

    /// Get element id by name
    /**
        \param name is string value of element name
        \return id of element
    */
    lUInt16 getElementNameIndex( const lChar32 * name );

    /// Get element id by name
    /**
        \param name is string value of element name (8bit ASCII only)
        \return id of element, allocates new ID if not found
    */
    lUInt16 getElementNameIndex( const lChar8 * name );

    /// Get element id by name
    /**
        \param name is string value of element name (8bit ASCII only)
        \return id of element, 0 if not found
    */
    lUInt16 findElementNameIndex( const lChar8 * name );

    /// Get element type properties structure by id
    /**
        \param id is element id
        \return pointer to elem_def_t structure containing type properties
        \sa elem_def_t
    */
    inline const css_elem_def_props_t * getElementTypePtr( lUInt16 id )
    {
        return _elementNameTable.dataById( id );
    }

    // set node types from table
    void setNodeTypes( const elem_def_t * node_scheme );
    // set attribute types from table
    void setAttributeTypes( const attr_def_t * attr_scheme );
    // set namespace types from table
    void setNameSpaceTypes( const ns_def_t * ns_scheme );

    // debug dump
    void dumpUnknownEntities( const char * fname );
    lString32Collection getUnknownEntities();

    /// garbage collector
    virtual void gc()
    {
#if BUILD_LITE!=1
        fontMan->gc();
#endif
    }

    inline LVStyleSheet * getStyleSheet() { return &_stylesheet; }
    /// sets style sheet, clears old content of css if arg replace is true
    void setStyleSheet( const char * css, bool replace );

#if BUILD_LITE!=1
    /// apply document's stylesheet to element node
    inline void applyStyle( ldomNode * element, css_style_rec_t * pstyle)
    {
        _stylesheet.apply( element, pstyle );
    }
#endif

    void onAttributeSet( lUInt16 attrId, lUInt32 valueId, ldomNode * node );

    /// get element by id attribute value code
    inline ldomNode * getNodeById( lUInt32 attrValueId )
    {
        return getTinyNode( _idNodeMap.get( attrValueId ) );
    }

    /// get element by id attribute value
    inline ldomNode * getElementById( const lChar32 * id )
    {
        lUInt32 attrValueId = getAttrValueIndex( id );
        ldomNode * node = getNodeById( attrValueId );
        return node;
    }
    /// returns root element
    ldomNode * getRootNode();

    /// returns code base path relative to document container
    inline lString32 getCodeBase() { return getProps()->getStringDef(DOC_PROP_CODE_BASE, ""); }
    /// sets code base path relative to document container
    inline void setCodeBase(const lString32 & codeBase) { getProps()->setStringDef(DOC_PROP_CODE_BASE, codeBase); }

#ifdef _DEBUG
#if BUILD_LITE!=1
    ///debug method, for DOM tree consistency check, returns false if failed
    bool checkConsistency( bool requirePersistent );
#endif
#endif


    /// create formatted text object with options set
    LFormattedText * createFormattedText();

#if BUILD_LITE!=1
    void setHightlightOptions(text_highlight_options_t & options) {
        _highlightOptions = options;
    }
#endif

protected:
#if BUILD_LITE!=1
    struct DocFileHeader {
        lUInt32 render_dx;
        lUInt32 render_dy;
        lUInt32 render_docflags;
        lUInt32 render_style_hash;
        lUInt32 stylesheet_hash;
        lUInt32 node_displaystyle_hash;
        bool serialize( SerialBuf & buf );
        bool deserialize( SerialBuf & buf );
        DocFileHeader()
            : render_dx(0), render_dy(0), render_docflags(0), render_style_hash(0), stylesheet_hash(0),
                node_displaystyle_hash(NODE_DISPLAY_STYLE_HASH_UNINITIALIZED)
        {
        }
    };
    DocFileHeader _hdr;
    text_highlight_options_t _highlightOptions;
#endif

    LDOMNameIdMap _elementNameTable;    // Element Name<->Id map
    LDOMNameIdMap _attrNameTable;       // Attribute Name<->Id map
    LDOMNameIdMap _nsNameTable;          // Namespace Name<->Id map
    lUInt16       _nextUnknownElementId; // Next Id for unknown element
    lUInt16       _nextUnknownAttrId;    // Next Id for unknown attribute
    lUInt16       _nextUnknownNsId;      // Next Id for unknown namespace
    lString32HashedCollection _attrValueTable;
    LVHashTable<lUInt32,lInt32> _idNodeMap; // id to data index map
    LVHashTable<lString32,LVImageSourceRef> _urlImageMap; // url to image source map
    lUInt16 _idAttrId; // Id for "id" attribute name
    lUInt16 _nameAttrId; // Id for "name" attribute name

#if BUILD_LITE!=1
    SerialBuf _pagesData;
#endif

};

/*
struct lxmlNode
{
    lUInt32 parent;
    lUInt8  nodeType;
    lUInt8  nodeLevel;
};
*/

struct lxmlAttribute
{
    //
    lUInt16 nsid;
    lUInt16 id;
    lUInt32 index;
    inline bool compare( lUInt16 nsId, lUInt16 attrId )
    {
        return (nsId == nsid || nsId == LXML_NS_ANY) && (id == attrId);
    }
    inline void setData( lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex )
    {
        nsid = nsId;
        id = attrId;
        index = valueIndex;
    }
};

class ldomDocument;


#define LDOM_ALLOW_NODE_INDEX 0


class ldomDocument;

/**
 * @brief XPointer/XPath object with reference counting.
 * 
 */
class ldomXPointer
{
protected:
	friend class ldomXPointerEx;
	struct XPointerData {
	protected:
		ldomDocument * _doc;
		lInt32 _dataIndex;
		int _offset;
		int _refCount;
	public:
		inline void addRef() { _refCount++; }
		inline int decRef() { return --_refCount; }
		// create empty
		XPointerData() : _doc(NULL), _dataIndex(0), _offset(0), _refCount(1) { }
		// create instance
        XPointerData( ldomNode * node, int offset )
			: _doc(node?node->getDocument():NULL)
			, _dataIndex(node?node->getDataIndex():0)
			, _offset( offset )
			, _refCount( 1 )
		{ }
		// clone
		XPointerData( const XPointerData & v )  : _doc(v._doc), _dataIndex(v._dataIndex), _offset(v._offset), _refCount(1) { }
		inline ldomDocument * getDocument() { return _doc; }
        inline bool operator == (const XPointerData & v) const
		{
			return _doc==v._doc && _dataIndex == v._dataIndex && _offset == v._offset;
		}
		inline bool operator != (const XPointerData & v) const
		{
			return _doc!=v._doc || _dataIndex != v._dataIndex || _offset != v._offset;
		}
		inline bool isNull() { return _dataIndex==0 || _doc==NULL; }
        inline ldomNode * getNode() { return _dataIndex>0 ? ((lxmlDocBase*)_doc)->getTinyNode( _dataIndex ) : NULL; }
		inline int getOffset() { return _offset; }
        inline void setNode( ldomNode * node )
		{
			if ( node ) {
				_doc = node->getDocument();
				_dataIndex = node->getDataIndex();
			} else {
				_doc = NULL;
				_dataIndex = 0;
			}
		}
		inline void setOffset( int offset ) { _offset = offset; }
        inline void addOffset( int offset ) { _offset+=offset; }
        ~XPointerData() { }
	};
	XPointerData * _data;
	/// node pointer
    //ldomNode * _node;
	/// offset within node for pointer, -1 for xpath
	//int _offset;
	// cloning constructor
	ldomXPointer( const XPointerData * data )
		: _data( new XPointerData( *data ) )
	{
	}
public:
    /// clear pointer (make null)
    void clear()
    {
        if (_data->decRef() == 0)
            delete _data;
        _data = new XPointerData();
    }
    /// return document
	inline ldomDocument * getDocument() { return _data->getDocument(); }
    /// returns node pointer
    inline ldomNode * getNode() const { return _data->getNode(); }
#if BUILD_LITE!=1
    /// return parent final node, if found
    ldomNode * getFinalNode() const;
    /// return true is this node is a final node
    bool isFinalNode() const;
#endif
    /// returns offset within node
	inline int getOffset() const { return _data->getOffset(); }
	/// set pointer node
    inline void setNode( ldomNode * node ) { _data->setNode( node ); }
	/// set pointer offset within node
	inline void setOffset( int offset ) { _data->setOffset( offset ); }
    /// default constructor makes NULL pointer
	ldomXPointer()
		: _data( new XPointerData() )
	{
	}
	/// remove reference
	~ldomXPointer()
	{
		if (_data->decRef() == 0)
			delete _data;
	}
    /// copy constructor
	ldomXPointer( const ldomXPointer& v )
		: _data(v._data)
	{
		_data->addRef();
	}
    /// assignment operator
	ldomXPointer & operator =( const ldomXPointer& v )
	{
		if ( _data==v._data )
			return *this;
		if (_data->decRef() == 0)
			delete _data;
		_data = v._data;
		_data->addRef();
		return *this;
	}
    /// constructor
    ldomXPointer( ldomNode * node, int offset )
		: _data( new XPointerData( node, offset ) )
	{
	}
    /// get pointer for relative path
    ldomXPointer relative( lString32 relativePath );
    /// get pointer for relative path
    ldomXPointer relative( const lChar32 * relativePath )
    {
        return relative( lString32(relativePath) );
    }

    /// returns true for NULL pointer
	bool isNull() const
	{
        return !_data || _data->isNull();
	}
    /// returns true if object is pointer
	bool isPointer() const
	{
		return !_data->isNull() && getOffset()>=0;
	}
    /// returns true if object is path (no offset specified)
	bool isPath() const
	{
		return !_data->isNull() && getOffset()==-1;
	}
    /// returns true if pointer is NULL
	bool operator !() const
	{
		return _data->isNull();
	}
    /// returns true if pointers are equal
	bool operator == (const ldomXPointer & v) const
	{
		return *_data == *v._data;
	}
    /// returns true if pointers are not equal
	bool operator != (const ldomXPointer & v) const
	{
		return *_data != *v._data;
	}
//#if BUILD_LITE!=1
    /// returns caret rectangle for pointer inside formatted document
    bool getRect(lvRect & rect, bool extended=false, bool adjusted=false) const;
    /// returns glyph rectangle for pointer inside formatted document considering paddings and borders
    /// (with adjusted=true, adjust for left and right side bearing of the glyph, for cleaner highlighting)
    bool getRectEx(lvRect & rect, bool adjusted=false) const { return getRect(rect, true, adjusted); }
    /// returns coordinates of pointer inside formatted document
    lvPoint toPoint( bool extended=false ) const;
//#endif
    /// converts to string
    lString32 toString( XPointerMode mode = XPATH_USE_NAMES) {
        if( XPATH_USE_NAMES==mode ) {
            tinyNodeCollection* doc = (tinyNodeCollection*)_data->getDocument();
            if ( doc != NULL && doc->getDOMVersionRequested() >= DOM_VERSION_WITH_NORMALIZED_XPOINTERS )
                return toStringV2();
            return toStringV1();
        }
        return toStringV2AsIndexes();
    }
    lString32 toStringV1(); // Using names, old, with boxing elements (non-normalized)
    lString32 toStringV2(); // Using names, new, without boxing elements, so: normalized
    lString32 toStringV2AsIndexes(); // Without element names, normalized (not used)

    /// returns XPath node text
    lString32 getText(  lChar32 blockDelimiter=0 )
    {
        ldomNode * node = getNode();
        if ( !node )
            return lString32::empty_str;
        return node->getText( blockDelimiter );
    }
    /// returns href attribute of <A> element, null string if not found
    lString32 getHRef();
    /// returns href attribute of <A> element, plus xpointer of <A> element itself
    lString32 getHRef(ldomXPointer & a_xpointer);
    /// create a copy of pointer data
    ldomXPointer * clone()
    {
            return new ldomXPointer( _data );
    }
    /// returns true if current node is element
    inline bool isElement() const { return !isNull() && getNode()->isElement(); }
    /// returns true if current node is element
    inline bool isText() const { return !isNull() && getNode()->isText(); }
    /// returns HTML (serialized from the DOM, may be different from the source HTML)
    lString8 getHtml( lString32Collection & cssFiles, int wflags=0 );
    lString8 getHtml( int wflags=0 ) {
        lString32Collection cssFiles; return getHtml(cssFiles, wflags);
    }
};

#define MAX_DOM_LEVEL 64
/// Xpointer optimized to iterate through DOM tree
class ldomXPointerEx : public ldomXPointer
{
protected:
    int _indexes[MAX_DOM_LEVEL];
    int _level;
    void initIndex();
public:
    /// returns bottom level index
    int getIndex() { return _indexes[_level-1]; }
    /// returns node level
    int getLevel() { return _level; }
    /// default constructor
    ldomXPointerEx()
	    : ldomXPointer()
    {
        initIndex();
    }
    /// constructor by node pointer and offset
    ldomXPointerEx(  ldomNode * node, int offset )
		: ldomXPointer( node, offset )
    {
        initIndex();
    }
    /// copy constructor
    ldomXPointerEx( const ldomXPointer& v )
		: ldomXPointer( v._data )
    {
        initIndex();
    }
    /// copy constructor
    ldomXPointerEx( const ldomXPointerEx& v )
		: ldomXPointer( v._data )
    {
        _level = v._level;
        for ( int i=0; i<_level; i++ )
            _indexes[ i ] = v._indexes[i];
    }
    /// assignment operator
    ldomXPointerEx & operator =( const ldomXPointer& v )
    {
		if ( _data==v._data )
			return *this;
		if (_data->decRef() == 0)
			delete _data;
		_data = new XPointerData( *v._data );
        initIndex();
        return *this;
    }
    /// assignment operator
    ldomXPointerEx & operator =( const ldomXPointerEx& v )
    {
		if ( _data==v._data )
			return *this;
		if (_data->decRef() == 0)
			delete _data;
		_data = new XPointerData( *v._data );
        _level = v._level;
        for ( int i=0; i<_level; i++ )
            _indexes[ i ] = v._indexes[i];
        return *this;
    }
    /// returns true if ranges are equal
    bool operator == ( const ldomXPointerEx & v ) const
    {
        return _data->getDocument()==v._data->getDocument() && _data->getNode()==v._data->getNode() && _data->getOffset()==v._data->getOffset();
    }
    /// searches path for element with specific id, returns level at which element is founs, 0 if not found
    int findElementInPath( lUInt16 id );
    /// compare two pointers, returns -1, 0, +1
    int compare( const ldomXPointerEx& v ) const;
    /// move to next sibling
    bool nextSibling();
    /// move to previous sibling
    bool prevSibling();
    /// move to next sibling element
    bool nextSiblingElement();
    /// move to previous sibling element
    bool prevSiblingElement();
    /// move to parent
    bool parent();
    /// move to first child of current node
    bool firstChild();
    /// move to last child of current node
    bool lastChild();
    /// move to first element child of current node
    bool firstElementChild();
    /// move to last element child of current node
    bool lastElementChild();
    /// move to child #
    bool child( int index );
    /// move to sibling #
    bool sibling( int index );
    /// ensure that current node is element (move to parent, if not - from text node to element)
    bool ensureElement();
    /// moves pointer to parent element with FINAL render method, returns true if success
    bool ensureFinal();
    /// returns true if current node is visible element with render method == erm_final
    bool isVisibleFinal();
    /// move to next final visible node (~paragraph)
    bool nextVisibleFinal();
    /// move to previous final visible node (~paragraph)
    bool prevVisibleFinal();
    /// returns true if current node is visible element or text
    bool isVisible();
    // returns true if text node char at offset is part of a word
    bool isVisibleWordChar();
    /// move to next text node
    bool nextText( bool thisBlockOnly = false );
    /// move to previous text node
    bool prevText( bool thisBlockOnly = false );
    /// move to next visible text node
    bool nextVisibleText( bool thisBlockOnly = false );
    /// move to previous visible text node
    bool prevVisibleText( bool thisBlockOnly = false );

    /// move to prev visible char
    bool prevVisibleChar( bool thisBlockOnly = false );
    /// move to next visible char
    bool nextVisibleChar( bool thisBlockOnly = false );

    /// move to previous visible word beginning
    bool prevVisibleWordStart( bool thisBlockOnly = false );
    /// move to previous visible word end
    bool prevVisibleWordEnd( bool thisBlockOnly = false );
    /// move to next visible word beginning
    bool nextVisibleWordStart( bool thisBlockOnly = false );
    /// move to end of current word
    bool thisVisibleWordEnd( bool thisBlockOnly = false );
    /// move to next visible word end
    bool nextVisibleWordEnd( bool thisBlockOnly = false );

    /// move to previous visible word beginning (in sentence)
    bool prevVisibleWordStartInSentence();
    /// move to previous visible word end (in sentence)
    bool prevVisibleWordEndInSentence();
    /// move to next visible word beginning (in sentence)
    bool nextVisibleWordStartInSentence();
    /// move to end of current word (in sentence)
    bool thisVisibleWordEndInSentence();
    /// move to next visible word end (in sentence)
    bool nextVisibleWordEndInSentence();

    /// move to beginning of current visible text sentence
    bool thisSentenceStart();
    /// move to end of current visible text sentence
    bool thisSentenceEnd();
    /// move to beginning of next visible text sentence
    bool nextSentenceStart();
    /// move to beginning of next visible text sentence
    bool prevSentenceStart();
    /// move to end of next visible text sentence
    bool nextSentenceEnd();
    /// move to end of prev visible text sentence
    bool prevSentenceEnd();
    /// returns true if points to beginning of sentence
    bool isSentenceStart();
    /// returns true if points to end of sentence
    bool isSentenceEnd();

    /// returns true if points to last visible text inside block element
    bool isLastVisibleTextInBlock();
    /// returns true if points to first visible text inside block element
    bool isFirstVisibleTextInBlock();

    /// returns block owner node of current node (or current node if it's block)
    ldomNode * getThisBlockNode();

    /// returns true if current position is visible word beginning
    bool isVisibleWordStart();
    /// returns true if current position is visible word end
    bool isVisibleWordEnd();
    /// forward iteration by elements of DOM three
    bool nextElement();
    /// backward iteration by elements of DOM three
    bool prevElement();
    /// calls specified function recursively for all elements of DOM tree
    void recurseElements( void (*pFun)( ldomXPointerEx & node ) );
    /// calls specified function recursively for all nodes of DOM tree
    void recurseNodes( void (*pFun)( ldomXPointerEx & node ) );

    /// move to next sibling or parent's next sibling
    bool nextOuterElement();
    /// move to (end of) last and deepest child node descendant of current node
    bool lastInnerNode( bool toTextEnd=false );
    /// move to (end of) last and deepest child text node descendant of current node
    bool lastInnerTextNode( bool toTextEnd=false );
};

class ldomXRange;

/// callback for DOM tree iteration interface
class ldomNodeCallback {
public:
    /// destructor
    virtual ~ldomNodeCallback() { }
    /// called for each found text fragment in range
    virtual void onText( ldomXRange * ) = 0;
    /// called for each found node in range
    virtual bool onElement( ldomXPointerEx * ) = 0;
};

/// range for word inside text node
class ldomWord
{
    ldomNode * _node;
    int _start;
    int _end;
public:
    ldomWord( )
    : _node(NULL), _start(0), _end(0)
    { }
    ldomWord( ldomNode * node, int start, int end )
    : _node(node), _start(start), _end(end)
    { }
    ldomWord( const ldomWord & v )
    : _node(v._node), _start(v._start), _end(v._end)
    { }
    ldomWord & operator = ( const ldomWord & v )
    {
        _node = v._node;
        _start = v._start;
        _end = v._end;
        return *this;
    }
    /// returns true if object doesn't point valid word
    bool isNull() { return _node==NULL || _start<0 || _end<=_start; }
    /// get word text node pointer
    ldomNode * getNode() const { return _node; }
    /// get word start offset
    int getStart() const { return _start; }
    /// get word end offset
    int getEnd() const { return _end; }
    /// get word start XPointer
    ldomXPointer getStartXPointer() const { return ldomXPointer( _node, _start ); }
    /// get word start XPointer
    ldomXPointer getEndXPointer() const { return ldomXPointer( _node, _end ); }
    /// get word text
    lString32 getText()
    {
        if ( isNull() )
            return lString32::empty_str;
        lString32 txt = _node->getText();
        return txt.substr( _start, _end-_start );
    }
};

/// DOM range
class ldomXRange {
    ldomXPointerEx _start;
    ldomXPointerEx _end;
    /// _flags, only used by ldomXRangeList.getRanges() when making a ldomMarkedRangeList (for native
    //  highlighting of a text selection being made, and for crengine internal bookmarks):
    //  0: not shown (filtered out in LVDocView::updateSelections() by ldomXRangeList ranges(..., true))
    //  1,2,3: legacy drawing (will make a single ldomMarkedRange spanning multiple lines, assuming
    //         full width LTR paragraphs) (2 & 3 might be used for crengine internal bookmarks,
    //         see hist.h for enum bmk_type)
    //  0x11, 0x12, 0x13:  enhanced drawing (will make multiple segmented ldomMarkedRange,
    //                     each spanning a single line)
    lUInt32 _flags;
public:
    ldomXRange()
        : _flags(0)
    {
    }
    ldomXRange( const ldomXPointerEx & start, const ldomXPointerEx & end, lUInt32 flags=0 )
    : _start( start ), _end( end ), _flags(flags)
    {
    }
    ldomXRange( const ldomXPointer & start, const ldomXPointer & end )
    : _start( start ), _end( end ), _flags(0)
    {
    }
    /// copy constructor
    ldomXRange( const ldomXRange & v )
    : _start( v._start ), _end( v._end ), _flags(v._flags)
    {
    }
    ldomXRange( const ldomWord & word )
        : _start( word.getStartXPointer() ), _end( word.getEndXPointer() ), _flags(1)
    {
    }
    /// if start is after end, swap start and end
    void sort();
    /// create intersection of two ranges
    ldomXRange( const ldomXRange & v1,  const ldomXRange & v2 );
    /// copy constructor of full node range
    ldomXRange( ldomNode * p, bool fitEndToLastInnerChild=false );
    /// copy assignment
    ldomXRange & operator = ( const ldomXRange & v )
    {
        _start = v._start;
        _end = v._end;
        return *this;
    }
    /// returns true if ranges are equal
    bool operator == ( const ldomXRange & v ) const
    {
        return _start == v._start && _end == v._end && _flags==v._flags;
    }
    /// returns true if interval is invalid or empty
    bool isNull()
    {
        if ( _start.isNull() || _end.isNull() )
            return true;
        if ( _start.compare( _end ) > 0 )
            return true;
        return false;
    }
    /// makes range empty
    void clear()
    {
        _start.clear();
        _end.clear();
        _flags = 0;
    }
    /// returns true if pointer position is inside range
    bool isInside( const ldomXPointerEx & p ) const
    {
        return ( _start.compare( p ) <= 0 && _end.compare( p ) >= 0 );
    }
    /// returns interval start point
    ldomXPointerEx & getStart() { return _start; }
    /// returns interval end point
    ldomXPointerEx & getEnd() { return _end; }
    /// sets interval start point
    void setStart( ldomXPointerEx & start ) { _start = start; }
    /// sets interval end point
    void setEnd( ldomXPointerEx & end ) { _end = end; }
    /// returns flags value
    lUInt32 getFlags() { return _flags; }
    /// sets new flags value
    void setFlags( lUInt32 flags ) { _flags = flags; }
    /// returns true if this interval intersects specified interval
    bool checkIntersection( ldomXRange & v );
    /// returns text between two XPointer positions
    lString32 getRangeText( lChar32 blockDelimiter='\n', int maxTextLen=0 );
    /// get all words from specified range
    void getRangeWords( LVArray<ldomWord> & list );
    /// returns href attribute of <A> element, null string if not found
    lString32 getHRef();
    /// returns href attribute of <A> element, plus xpointer of <A> element itself
    lString32 getHRef(ldomXPointer & a_xpointer);
    /// sets range to nearest word bounds, returns true if success
    static bool getWordRange( ldomXRange & range, ldomXPointer & p );
    /// run callback for each node in range
    void forEach( ldomNodeCallback * callback );
#if BUILD_LITE!=1
    /// returns rectangle (in doc coordinates) for range. Returns true if found.
    bool getRectEx( lvRect & rect, bool & isSingleLine );
    bool getRectEx( lvRect & rect ) {
        bool isSingleLine; return getRectEx(rect, isSingleLine);
    };
    // returns multiple segments rects (one for each text line)
    // that the ldomXRange spans on the page.
    void getSegmentRects( LVArray<lvRect> & rects );
#endif
    /// returns nearest common element for start and end points
    ldomNode * getNearestCommonParent();
    /// returns HTML (serialized from the DOM, may be different from the source HTML)
    lString8 getHtml( lString32Collection & cssFiles, int wflags=0, bool fromRootNode=false );
    lString8 getHtml( int wflags=0, bool fromRootNode=false ) {
        lString32Collection cssFiles; return getHtml(cssFiles, wflags, fromRootNode);
    };

    /// searches for specified text inside range
    bool findText( lString32 pattern, bool caseInsensitive, bool reverse, LVArray<ldomWord> & words, int maxCount, int maxHeight, int maxHeightCheckStartY = -1, bool checkMaxFromStart = false );
};

class ldomMarkedText
{
public:
    lString32 text;
    lUInt32   flags;
    int offset;
    ldomMarkedText( lString32 s, lUInt32 flg, int offs )
    : text(s), flags(flg), offset(offs)
    {
    }
    ldomMarkedText( const ldomMarkedText & v )
    : text(v.text), flags(v.flags)
    {
    }
};

typedef LVPtrVector<ldomMarkedText> ldomMarkedTextList;

enum MoveDirection {
    DIR_ANY,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
};

/// range in document, marked with specified flags
class ldomMarkedRange
{
public:
    /// start document point
    lvPoint   start;
    /// end document point
    lvPoint   end;
    /// flags:
    //  0: not shown
    //  1,2,3: legacy drawing (a single mark may spans multiple lines, assuming full width
    //         LTR paragraphs) (2 & 3 might be used for crengine internal bookmarks,
    //         see hist.h for enum bmk_type)
    //  0x11, 0x12, 0x13:  enhanced drawing (segmented mark, spanning a single line)
    lUInt32   flags;
    bool empty()
    {
        return ( start.y>end.y || ( start.y == end.y && start.x >= end.x ) );
    }
    /// returns mark middle point for single line mark, or start point for multiline mark
    lvPoint getMiddlePoint();
    /// returns distance (dx+dy) from specified point to middle point
    int calcDistance( int x, int y, MoveDirection dir );
    /// returns true if intersects specified line rectangle
    bool intersects( lvRect & rc, lvRect & intersection );
    /// constructor
    ldomMarkedRange( lvPoint _start, lvPoint _end, lUInt32 _flags )
    : start(_start), end(_end), flags(_flags)
    {
    }
    /// constructor
    ldomMarkedRange( ldomWord & word ) {
        ldomXPointer startPos(word.getNode(), word.getStart() );
        ldomXPointer endPos(word.getNode(), word.getEnd() );
        start = startPos.toPoint();
        end = endPos.toPoint();
    }
    /// copy constructor
    ldomMarkedRange( const ldomMarkedRange & v )
    : start(v.start), end(v.end), flags(v.flags)
    {
    }
};

class ldomWordEx : public ldomWord
{
    ldomWord _word;
    ldomMarkedRange _mark;
    ldomXRange _range;
    lString32 _text;
public:
    ldomWordEx( ldomWord & word )
        :  _word(word), _mark(word), _range(word)
    {
        _text = removeSoftHyphens( _word.getText() );
    }
    ldomWord & getWord() { return _word; }
    ldomXRange & getRange() { return _range; }
    ldomMarkedRange & getMark() { return _mark; }
    lString32 & getText() { return _text; }
};

/// list of extended words
class ldomWordExList : public LVPtrVector<ldomWordEx>
{
    int minx;
    int maxx;
    int miny;
    int maxy;
    int x;
    int y;
    ldomWordEx * selWord;
    lString32Collection pattern;
    void init();
    ldomWordEx * findWordByPattern();
public:
    ldomWordExList()
        : minx(-1), maxx(-1), miny(-1), maxy(-1), x(-1), y(-1), selWord(NULL)
    {
    }
    /// adds all visible words from range, returns number of added words
    int addRangeWords( ldomXRange & range, bool trimPunctuation );
    /// find word nearest to specified point
    ldomWordEx * findNearestWord( int x, int y, MoveDirection dir );
    /// select word
    void selectWord( ldomWordEx * word, MoveDirection dir );
    /// select next word in specified direction
    ldomWordEx * selectNextWord( MoveDirection dir, int moveBy = 1 );
    /// select middle word in range
    ldomWordEx * selectMiddleWord();
    /// get selected word
    ldomWordEx * getSelWord() { return selWord; }
    /// try append search pattern and find word
    ldomWordEx * appendPattern(lString32 chars);
    /// remove last character from pattern and try to search
    ldomWordEx * reducePattern();
};


/// list of marked ranges
class ldomMarkedRangeList : public LVPtrVector<ldomMarkedRange>
{
public:
    ldomMarkedRangeList()
    {
    }
    /// create bounded by RC list, with (0,0) coordinates at left top corner
    // crop/discard elements outside of rc (or outside of crop_rc instead if provided)
    ldomMarkedRangeList( const ldomMarkedRangeList * list, lvRect & rc, lvRect * crop_rc=NULL );
};

class ldomXRangeList : public LVPtrVector<ldomXRange>
{
public:
    /// add ranges for words
    void addWords( const LVArray<ldomWord> & words )
    {
        for ( int i=0; i<words.length(); i++ )
            LVPtrVector<ldomXRange>::add( new ldomXRange( words[i] ) );
    }
    ldomXRangeList( const LVArray<ldomWord> & words )
    {
        addWords( words );
    }
    /// create list splittiny existing list into non-overlapping ranges
    ldomXRangeList( ldomXRangeList & srcList, bool splitIntersections );
    /// create list by filtering existing list, to get only values which intersect filter range
    ldomXRangeList( ldomXRangeList & srcList, ldomXRange & filter );
#if BUILD_LITE!=1
    /// fill text selection list by splitting text into monotonic flags ranges
    void splitText( ldomMarkedTextList &dst, ldomNode * textNodeToSplit );
    /// fill marked ranges list
    void getRanges( ldomMarkedRangeList &dst );
#endif
    /// split into subranges using intersection
    void split( ldomXRange * r );
    /// default constructor for empty list
    ldomXRangeList() {};
};

class LVTocItem;
class LVDocView;

/// TOC item
class LVTocItem
{
    friend class LVDocView;
private:
    LVTocItem *     _parent;
    ldomDocument *  _doc;
    lInt32          _level;
    lInt32          _index;
    lInt32          _page;
    lInt32          _percent;
    lString32       _name;
    lString32       _path;
    ldomXPointer    _position;
    LVPtrVector<LVTocItem> _children;
    //====================================================
    //LVTocItem( ldomXPointer pos, const lString32 & name ) : _parent(NULL), _level(0), _index(0), _page(0), _percent(0), _name(name), _path(pos.toString()), _position(pos) { }
    LVTocItem( ldomXPointer pos, lString32 path, const lString32 & name ) : _parent(NULL), _level(0), _index(0), _page(0), _percent(0), _name(name), _path(path), _position(pos) { }
    void addChild( LVTocItem * item ) { item->_level=_level+1; item->_parent=this; item->_index=_children.length(), item->_doc=_doc; _children.add(item); }
    //====================================================
    void setPage( int n ) { _page = n; }
    void setPercent( int n ) { _percent = n; }
public:
    /// serialize to byte array (pointer will be incremented by number of bytes written)
    bool serialize( SerialBuf & buf );
    /// deserialize from byte array (pointer will be incremented by number of bytes read)
    bool deserialize( ldomDocument * doc, SerialBuf & buf );
    /// get page number
    int getPage() { return _page; }
    /// get position percent * 100
    int getPercent() { return _percent; }
    /// returns parent node pointer
    LVTocItem * getParent() const { return _parent; }
    /// returns node level (0==root node, 1==top level)
    int getLevel() const { return _level; }
    /// returns node index
    int getIndex() const { return _index; }
    /// returns section title
    lString32 getName() const { return _name; }
    /// returns position pointer
    ldomXPointer getXPointer();
    /// set position pointer (for cases where we need to create a LVTocItem as a container, but
    /// we'll know the xpointer only later, mostly always the same xpointer as its first child)
    void setXPointer(ldomXPointer xp) { _position = xp; }
    /// returns position path
    lString32 getPath();
    /// returns Y position
    int getY();
    /// returns page number
    //int getPageNum( LVRendPageList & pages );
    /// returns child node count
    int getChildCount() const { return _children.length(); }
    /// returns child node by index
    LVTocItem * getChild( int index ) const { return _children[index]; }
    /// add child TOC node
    LVTocItem * addChild( const lString32 & name, ldomXPointer ptr, lString32 path )
    {
        LVTocItem * item = new LVTocItem( ptr, path, name );
        addChild( item );
        return item;
    }
    void clear() { _children.clear(); }
    // root node constructor
    LVTocItem( ldomDocument * doc ) : _parent(NULL), _doc(doc), _level(0), _index(0), _page(0) { }
    ~LVTocItem() { clear(); }

    /// For use on the root toc item only (_page, otherwise unused, can be used to store this flag)
    void setAlternativeTocFlag() { if (_level==0) _page = 1; }
    bool hasAlternativeTocFlag() { return _level==0 && _page==1; }

    /// When page numbers have been calculated, LVDocView::updatePageNumbers()
    /// sets the root toc item _percent to -1. So let's use it to know that fact.
    bool hasValidPageNumbers() { return _level==0 && _percent == -1; }
    void invalidatePageNumbers() { if (_level==0) _percent = 0; }
};

/// PageMapItem
class LVPageMapItem
{
    friend class LVDocView;
    friend class LVPageMap;
private:
    ldomDocument *  _doc;
    lInt32          _index;
    lInt32          _page;
    lInt32          _doc_y;
    lString32       _label;
    lString32       _path;
    ldomXPointer    _position;
    LVPageMapItem( ldomXPointer pos, lString32 path, const lString32 & label )
        : _index(0), _page(0), _doc_y(-1), _label(label), _path(path), _position(pos)
        { }
    void setPage( int n ) { _page = n; }
    void setDocY( int y ) { _doc_y = y; }
public:
    /// serialize to byte array (pointer will be incremented by number of bytes written)
    bool serialize( SerialBuf & buf );
    /// deserialize from byte array (pointer will be incremented by number of bytes read)
    bool deserialize( ldomDocument * doc, SerialBuf & buf );
    /// get rendered page number
    int getPage() { return _page; }
    /// returns node index
    int getIndex() const { return _index; }
    /// returns page label
    lString32 getLabel() const { return _label; }
    /// returns position pointer
    ldomXPointer getXPointer();
    /// returns position path
    lString32 getPath();
    /// returns Y position
    int getDocY(bool refresh=false);
    LVPageMapItem( ldomDocument * doc ) : _doc(doc), _index(0), _page(0), _doc_y(-1) { }
};

/// PageMapItems container
class LVPageMap
{
    friend class LVDocView;
private:
    ldomDocument *  _doc;
    bool            _page_info_valid;
    lString32       _source;
    LVPtrVector<LVPageMapItem> _children;
    void addPage( LVPageMapItem * item ) {
        item->_doc = _doc;
        item->_index = _children.length();
        _children.add(item);
    }
public:
    /// serialize to byte array (pointer will be incremented by number of bytes written)
    bool serialize( SerialBuf & buf );
    /// deserialize from byte array (pointer will be incremented by number of bytes read)
    bool deserialize( ldomDocument * doc, SerialBuf & buf );
    /// returns child node count
    int getChildCount() const { return _children.length(); }
    /// returns child node by index
    LVPageMapItem * getChild( int index ) const { return _children[index]; }
    /// add page item
    LVPageMapItem * addPage( const lString32 & label, ldomXPointer ptr, lString32 path )
    {
        LVPageMapItem * item = new LVPageMapItem( ptr, path, label );
        addPage( item );
        return item;
    }
    void clear() { _children.clear(); }
    bool hasValidPageInfo() { return _page_info_valid; }
    void invalidatePageInfo() { _page_info_valid = false; }
    // Page source (info about the book paper version the page labels reference)
    void setSource( lString32 source ) { _source = source; }
    lString32 getSource() const { return _source; }
    // root node constructor
    LVPageMap( ldomDocument * doc )
        : _doc(doc), _page_info_valid(false) { }
    ~LVPageMap() { clear(); }
};


class ldomNavigationHistory
{
    private:
        lString32Collection _links;
        int _pos;
        void clearTail()
        {
            if (_links.length() > _pos)
                _links.erase(_pos, _links.length() - _pos);
        }
    public:
        void clear()
        {
            _links.clear();
            _pos = 0;
        }
        bool save( lString32 link )
        {
            if (_pos==(int)_links.length() && _pos>0 && _links[_pos-1]==link )
                return false;
            if ( _pos>=(int)_links.length() || _links[_pos]!=link ) {
                clearTail();
                _links.add( link );
                _pos = _links.length();
                return true;
            } else if (_links[_pos]==link) {
                _pos++;
                return true;
            }
            return false;
        }
        lString32 back()
        {
            if (_pos==0)
                return lString32::empty_str;
            return _links[--_pos];
        }
        lString32 forward()
        {
            if (_pos>=(int)_links.length()-1)
                return lString32::empty_str;
            return _links[++_pos];
        }
        int backCount()
        {
            return _pos;
        }
        int forwardCount()
        {
            return _links.length() - _pos;
        }
};

class ListNumberingProps
{
public:
    int maxCounter;
    int maxWidth;
    ListNumberingProps( int c, int w )
        : maxCounter(c), maxWidth(w)
    {
    }
};
typedef LVRef<ListNumberingProps> ListNumberingPropsRef;

class ldomDocument : public lxmlDocBase
{
    friend class ldomDocumentWriter;
    friend class ldomDocumentWriterFilter;
private:
    LVTocItem m_toc;
    LVPageMap m_pagemap;
#if BUILD_LITE!=1
    font_ref_t _def_font; // default font
    css_style_ref_t _def_style;
    lUInt32 _last_docflags;
    int _page_height;
    int _page_width;
    bool _rendered;
    bool _just_rendered_from_cache;
    bool _toc_from_cache_valid;
    lUInt32 _warnings_seen_bitmap;
    ldomXRangeList _selections;
#endif

    lString32 _docStylesheetFileName;

    LVContainerRef _container;

    LVHashTable<lUInt32, ListNumberingPropsRef> lists;

    LVEmbeddedFontList _fontList;


#if BUILD_LITE!=1
    /// load document cache file content
    bool loadCacheFileContent(CacheLoadingCallback * formatCallback, LVDocViewCallback * progressCallback=NULL);

    /// save changes to cache file
    bool saveChanges();
    /// saves changes to cache file, limited by time interval (can be called again to continue after TIMEOUT)
    virtual ContinuousOperationResult saveChanges( CRTimerUtil & maxTime, LVDocViewCallback * progressCallback=NULL );
#endif

    /// create XPointer from a non-normalized string made by toStringV1()
    ldomXPointer createXPointerV1( ldomNode * baseNode, const lString32 & xPointerStr );
    /// create XPointer from a normalized string made by toStringV2()
    ldomXPointer createXPointerV2( ldomNode * baseNode, const lString32 & xPointerStr );
protected:

#if BUILD_LITE!=1
    void applyDocumentStyleSheet();
#endif

public:

#if BUILD_LITE!=1
    void forceReinitStyles() {
        dropStyles();
        _hdr.render_style_hash = 0;
        _rendered = false;
    }

    ListNumberingPropsRef getNodeNumberingProps( lUInt32 nodeDataIndex );
    void setNodeNumberingProps( lUInt32 nodeDataIndex, ListNumberingPropsRef v );
    void resetNodeNumberingProps();
#endif

#if BUILD_LITE!=1
    /// returns object image stream
    LVStreamRef getObjectImageStream( lString32 refName );
    /// returns object image source
    LVImageSourceRef getObjectImageSource( lString32 refName );

    bool isDefStyleSet()
    {
        return !_def_style.isNull();
    }

    /// return document's embedded font list
    LVEmbeddedFontList & getEmbeddedFontList() { return _fontList; }
    /// register embedded document fonts in font manager, if any exist in document
    void registerEmbeddedFonts();
    /// unregister embedded document fonts in font manager, if any exist in document
    void unregisterEmbeddedFonts();
#endif

    /// returns pointer to TOC root node
    LVTocItem * getToc() { return &m_toc; }
    /// build alternative TOC from document heading elements (H1 to H6) and cr-hints, or docFragments
    void buildAlternativeToc();
    bool isTocAlternativeToc() { return m_toc.hasAlternativeTocFlag(); }
    /// build TOC from headings
    void buildTocFromHeadings();

    /// returns pointer to PageMapItems container
    LVPageMap * getPageMap() { return &m_pagemap; }

#if BUILD_LITE!=1
    bool isTocFromCacheValid() { return _toc_from_cache_valid; }

    /// save document formatting parameters after render
    void updateRenderContext();
    /// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
    bool checkRenderContext();
#endif

#if BUILD_LITE!=1
    /// try opening from cache file, find by source file name (w/o path) and crc32
    virtual bool openFromCache( CacheLoadingCallback * formatCallback, LVDocViewCallback * progressCallback=NULL );
    /// saves recent changes to mapped file
    virtual ContinuousOperationResult updateMap(CRTimerUtil & maxTime, LVDocViewCallback * progressCallback=NULL);
    /// swaps to cache file or saves changes, limited by time interval
    virtual ContinuousOperationResult swapToCache( CRTimerUtil & maxTime );
    /// saves recent changes to mapped file
    virtual bool updateMap(LVDocViewCallback * progressCallback=NULL) {
        CRTimerUtil infinite;
        return updateMap(infinite, progressCallback)!=CR_ERROR; // NOLINT: Call to virtual function during destruction
    }
#endif


    LVContainerRef getContainer() { return _container; }
    void setContainer( LVContainerRef cont ) { _container = cont; }

#if BUILD_LITE!=1
    void clearRendBlockCache() { _renderedBlockCache.clear(); }
#endif
    void clear();
    lString32 getDocStylesheetFileName() { return _docStylesheetFileName; }
    void setDocStylesheetFileName(lString32 fileName) { _docStylesheetFileName = fileName; }

    ldomDocument();
    /// creates empty document which is ready to be copy target of doc partial contents
    ldomDocument( ldomDocument & doc );

#if BUILD_LITE!=1
    /// return selections collection
    ldomXRangeList & getSelections() { return _selections; }
    
    /// get full document height
    int getFullHeight();
    /// returns page height setting
    int getPageHeight() { return _page_height; }
    /// returns page width setting
    int getPageWidth() { return _page_width; }
#endif
    /// saves document contents as XML to stream with specified encoding
    bool saveToStream( LVStreamRef stream, const char * codepage, bool treeLayout=false );
    /// print a warning message (only once if warning_id provided, between 1 and 32)
    void printWarning(const char * msg, int warning_id=0);
#if BUILD_LITE!=1
    /// get default font reference
    font_ref_t getDefaultFont() { return _def_font; }
    /// get default style reference
    css_style_ref_t getDefaultStyle() { return _def_style; }

    inline bool parseStyleSheet(lString32 codeBase, lString32 css);
    inline bool parseStyleSheet(lString32 cssFile);
#endif
    /// destructor
    virtual ~ldomDocument();
#if BUILD_LITE!=1
    bool isRendered() { return _rendered; }
    /// renders (formats) document in memory: returns true if re-rendering needed, false if not
    virtual bool render( LVRendPageList * pages, LVDocViewCallback * callback, int width, int dy,
                         bool showCover, int y0, font_ref_t def_font, int def_interline_space,
                         CRPropRef props, int usable_left_overflow=0, int usable_right_overflow=0 );
    /// set global rendering properties
    virtual bool setRenderProps( int width, int dy, bool showCover, int y0, font_ref_t def_font,
                                 int def_interline_space, CRPropRef props );
#endif
    /// create xpointer from pointer string
    ldomXPointer createXPointer( const lString32 & xPointerStr );
    /// create xpointer from pointer string
    ldomNode * nodeFromXPath( const lString32 & xPointerStr )
    {
        return createXPointer( xPointerStr ).getNode();
    }
    /// get element text by pointer string
    lString32 textFromXPath( const lString32 & xPointerStr )
    {
        ldomNode * node = nodeFromXPath( xPointerStr );
        if ( !node )
            return lString32::empty_str;
        return node->getText();
    }

    /// create xpointer from relative pointer string
    ldomXPointer createXPointer( ldomNode * baseNode, const lString32 & xPointerStr )
    {
        if( _DOMVersionRequested >= DOM_VERSION_WITH_NORMALIZED_XPOINTERS)
            return createXPointerV2(baseNode, xPointerStr);
        return createXPointerV1(baseNode, xPointerStr);
    }

#if BUILD_LITE!=1
    /// create xpointer from doc point
    ldomXPointer createXPointer( lvPoint pt, int direction=PT_DIR_EXACT, bool strictBounds=false, ldomNode * from_node=NULL );
    /// get rendered block cache object
    CVRendBlockCache & getRendBlockCache() { return _renderedBlockCache; }

    bool findText( lString32 pattern, bool caseInsensitive, bool reverse, int minY, int maxY, LVArray<ldomWord> & words, int maxCount, int maxHeight, int maxHeightCheckStartY = -1 );
#endif
};


class ldomDocumentWriter;

class ldomElementWriter
{
    ldomElementWriter * _parent;
    ldomDocument * _document;

    ldomNode * _element;
    LVTocItem * _tocItem;
    lString32 _path;
    const css_elem_def_props_t * _typeDef;
    bool _allowText;
    bool _isBlock;
    bool _isSection;
    bool _stylesheetIsSet;
    bool _bodyEnterCalled;
    int _pseudoElementAfterChildIndex;
    lUInt32 _flags;
    lUInt32 getFlags();
    void updateTocItem();
    void onBodyEnter();
    void onBodyExit();
    ldomNode * getElement()
    {
        return _element;
    }
    lString32 getPath();
    void onText( const lChar32 * text, int len, lUInt32 flags, bool insert_before_last_child=false );
    void addAttribute( lUInt16 nsid, lUInt16 id, const lChar32 * value );
    //lxmlElementWriter * pop( lUInt16 id );

    ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent, bool insert_before_last_child=false);
    ~ldomElementWriter();

    friend class ldomDocumentWriter;
    friend class ldomDocumentWriterFilter;
    //friend ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
};

/** \brief callback object to fill DOM tree

    To be used with XML parser as callback object.

    Creates document according to incoming events.
*/
class ldomDocumentWriter : public LVXMLParserCallback
{
protected:
    //============================
    ldomDocument * _document;
    //ldomElement * _currNode;
    ldomElementWriter * _currNode;
    bool _errFlag;
    bool _headerOnly;
    bool _popStyleOnFinish;
    lUInt16 _stopTagId;
    //============================
    lUInt32 _flags;
    bool _inHeadStyle;
    lString32 _headStyleText;
    lString32Collection _stylesheetLinks;
    virtual void ElementCloseHandler( ldomNode * node ) { node->persist(); }
public:
    /// returns flags
    virtual lUInt32 getFlags() { return _flags; }
    /// sets flags
    virtual void setFlags( lUInt32 flags ) { _flags = flags; }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding( const lChar32 * name, const lChar32 * table );
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser * parser);
    /// called on parsing end
    virtual void OnStop();
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname );
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody();
    /// called on closing tag
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );
    /// called on attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue );
    /// close tags
    ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags );
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8 * data, int size) { 
#if BUILD_LITE!=1
        return _document->addBlob(name, data, size); 
#else
        return false;
#endif
    }
    /// set document property
    virtual void OnDocProperty(const char * name, lString8 value) { _document->getProps()->setString(name, value); }

    /// constructor
    ldomDocumentWriter(ldomDocument * document, bool headerOnly=false );
    /// destructor
    virtual ~ldomDocumentWriter();
};

/** \brief callback object to fill DOM tree

    To be used with XML parser as callback object.

    Creates document according to incoming events.

    Autoclose HTML tags.
*/
class ldomDocumentWriterFilter : public ldomDocumentWriter
{
protected:
    bool _libRuDocumentToDetect;
    bool _libRuDocumentDetected;
    bool _libRuParagraphStart;
    bool _libRuParseAsPre;
    lUInt16 _styleAttrId;
    lUInt16 _classAttrId;
    lUInt16 * _rules[MAX_ELEMENT_TYPE_ID];
    bool _tagBodyCalled;
    // Some states used when gDOMVersionRequested >= 20200824
    bool _htmlTagSeen;
    bool _headTagSeen;
    bool _bodyTagSeen;
    bool _curNodeIsSelfClosing;
    bool _curTagIsIgnored;
    ldomElementWriter * _curNodeBeforeFostering;
    ldomElementWriter * _curFosteredNode;
    ldomElementWriter * _lastP;
    virtual void AutoClose( lUInt16 tag_id, bool open );
    virtual bool AutoOpenClosePop( int step, lUInt16 tag_id );
    virtual lUInt16 popUpTo( ldomElementWriter * target, lUInt16 target_id=0, int scope=0 );
    virtual bool CheckAndEnsureFosterParenting(lUInt16 tag_id);
    virtual void ElementCloseHandler( ldomNode * node ) { node->persist(); }
    virtual void appendStyle( const lChar32 * style );
    virtual void setClass( const lChar32 * className, bool overrideExisting=false );
public:
    /// called on attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue );
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname );
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody();
    /// called on closing tag
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags );
    /// constructor
    ldomDocumentWriterFilter(ldomDocument * document, bool headerOnly, const char *** rules);
    /// destructor
    virtual ~ldomDocumentWriterFilter();
};

class ldomDocumentFragmentWriter : public LVXMLParserCallback
{
private:
    //============================
    LVXMLParserCallback * parent;
    lString32 baseTag;
    lString32 baseTagReplacement;
    lString32 codeBase;
    lString32 filePathName;
    lString32 codeBasePrefix;
    lString32 stylesheetFile;
    lString32 tmpStylesheetFile;
    lString32Collection stylesheetLinks;
    bool insideTag;
    int styleDetectionState;
    LVHashTable<lString32, lString32> pathSubstitutions;

    ldomNode * baseElement;
    ldomNode * lastBaseElement;

    lString8 headStyleText;
    int headStyleState;

    lString32 htmlDir;
    lString32 htmlLang;
    bool insideHtmlTag;

    bool m_nonlinear = false;

public:

    /// return content of html/head/style element
    lString8 getHeadStyleText() { return headStyleText; }

    ldomNode * getBaseElement() { return lastBaseElement; }

    lString32 convertId( lString32 id );
    lString32 convertHref( lString32 href );

    void addPathSubstitution( lString32 key, lString32 value )
    {
        pathSubstitutions.set(key, value);
    }

    virtual void setCodeBase( lString32 filePath );
    /// returns flags
    virtual lUInt32 getFlags() { return parent->getFlags(); }
    /// sets flags
    virtual void setFlags( lUInt32 flags ) { parent->setFlags(flags); }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding( const lChar32 * name, const lChar32 * table )
    { parent->OnEncoding( name, table ); }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser *)
    {
        insideTag = false;
        headStyleText.clear();
        headStyleState = 0;
        insideHtmlTag = false;
        htmlDir.clear();
        htmlLang.clear();
    }
    /// called on parsing end
    virtual void OnStop()
    {
        if ( insideTag ) {
            insideTag = false;
            if ( !baseTagReplacement.empty() ) {
                parent->OnTagClose(U"", baseTagReplacement.c_str());
            }
            baseElement = NULL;
            return;
        }
        insideTag = false;
    }
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname );
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody();
    /// called on closing tag
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );
    /// called on attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue );
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags )
    {
        if (headStyleState == 1) {
            headStyleText << UnicodeToUtf8(lString32(text).substr(0,len-1));
            return;
        }
        if ( insideTag )
            parent->OnText( text, len, flags );
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8 * data, int size) { return parent->OnBlob(name, data, size); }
    /// set document property
    virtual void OnDocProperty(const char * name, lString8 value) { parent->OnDocProperty(name, value); }
    // set non-linear flag
    virtual void setNonLinearFlag( bool nonlinear ) { m_nonlinear = nonlinear; }
    /// constructor
    ldomDocumentFragmentWriter( LVXMLParserCallback * parentWriter, lString32 baseTagName, lString32 baseTagReplacementName, lString32 fragmentFilePath )
    : parent(parentWriter), baseTag(baseTagName), baseTagReplacement(baseTagReplacementName),
    insideTag(false), styleDetectionState(0), pathSubstitutions(100), baseElement(NULL), lastBaseElement(NULL),
    headStyleState(0), insideHtmlTag(false)
    {
        setCodeBase( fragmentFilePath );
    }
    /// destructor
    virtual ~ldomDocumentFragmentWriter() { }
};

//utils
/// extract authors from FB2 document, delimiter is lString32 by default
lString32 extractDocAuthors( ldomDocument * doc, lString32 delimiter=lString32::empty_str, bool shortMiddleName=true );
lString32 extractDocTitle( ldomDocument * doc );
lString32 extractDocLanguage( ldomDocument * doc );
/// returns "(Series Name #number)" if pSeriesNumber is NULL, separate name and number otherwise
lString32 extractDocSeries( ldomDocument * doc, int * pSeriesNumber=NULL );
lString32 extractDocKeywords( ldomDocument * doc );
lString32 extractDocDescription( ldomDocument * doc );

bool IsEmptySpace( const lChar32 * text, int len );

/// parse XML document from stream, returns NULL if failed
ldomDocument * LVParseXMLStream( LVStreamRef stream,
                              const elem_def_t * elem_table=NULL,
                              const attr_def_t * attr_table=NULL,
                              const ns_def_t * ns_table=NULL );

/// parse XML document from stream, returns NULL if failed
ldomDocument * LVParseHTMLStream( LVStreamRef stream,
                              const elem_def_t * elem_table=NULL,
                              const attr_def_t * attr_table=NULL,
                              const ns_def_t * ns_table=NULL );

/// document cache
class ldomDocCache
{
public:
    /// open existing cache file stream
    static LVStreamRef openExisting( lString32 filename, lUInt32 crc, lUInt32 docFlags, lString32 &cachePath );
    /// create new cache file
    static LVStreamRef createNew( lString32 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString32 &cachePath );
    /// init document cache
    static bool init( lString32 cacheDir, lvsize_t maxSize );
    /// close document cache manager
    static bool close();
    /// delete all cache files
    static bool clear();
    /// returns true if cache is enabled (successfully initialized)
    static bool enabled();
};


/// unit test for DOM
void runTinyDomUnitTests();

/// pass true to enable CRC check for
void enableCacheFileContentsValidation(bool enable);

/// pass false to not compress data in cache files
void compressCachedData(bool enable);

/// increase the 4 hardcoded TEXT_CACHE_UNPACKED_SPACE, ELEM_CACHE_UNPACKED_SPACE,
// RECT_CACHE_UNPACKED_SPACE and STYLE_CACHE_UNPACKED_SPACE by this factor
void setStorageMaxUncompressedSizeFactor(float factor);

#endif
