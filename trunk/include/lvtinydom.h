/** \file lvtinydom.h
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
#include "lvstsheet.h"
#include "lvpagesplitter.h"
#include "lvptrvec.h"
#include "lvhashtable.h"
#include "lvimg.h"
#include "props.h"

#define LXML_NO_DATA       0 ///< to mark data storage record as empty
#define LXML_ELEMENT_NODE  1 ///< element node
#define LXML_TEXT_NODE     2 ///< text node
//#define LXML_DOCUMENT_NODE 3 ///< document node (not implemented)
//#define LXML_COMMENT_NODE  4 ///< comment node (not implemented)


/// docFlag mask, enable internal stylesheet of document and style attribute of elements
#define DOC_FLAG_ENABLE_INTERNAL_STYLES 1
/// docFlag mask, enable paperbook-like footnotes
#define DOC_FLAG_ENABLE_FOOTNOTES       2
/// docFlag mask, enable paperbook-like footnotes
#define DOC_FLAG_PREFORMATTED_TEXT      4
/// default docFlag set
#define DOC_FLAG_DEFAULTS (DOC_FLAG_ENABLE_INTERNAL_STYLES|DOC_FLAG_ENABLE_FOOTNOTES)



#define LXML_NS_NONE 0       ///< no namespace specified
#define LXML_NS_ANY  0xFFFF  ///< any namespace can be specified
#define LXML_ATTR_VALUE_NONE  0xFFFF  ///< attribute not found

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
#define DOC_PROP_FILE_CRC32      "doc.file.crc32"
#define DOC_PROP_CODE_BASE       "doc.file.code.base"

#if BUILD_LITE!=1
/// final block cache
typedef LVRef<LFormattedText> LFormattedTextRef;
typedef LVCacheMap< ldomNode *, LFormattedTextRef> CVRendBlockCache;
#endif


//#define LDOM_USE_OWN_MEM_MAN 0

typedef enum {
	xpath_step_error = 0, // error
	xpath_step_element,   // element of type 'name' with 'index'        /elemname[N]/
	xpath_step_text,      // text node with 'index'                     /text()[N]/
	xpath_step_nodeindex, // node index                                 /N/
	xpath_step_point      // point index                                .N
} xpath_step_t;
xpath_step_t ParseXPathStep( const lChar8 * &path, lString8 & name, int & index );

struct DataStorageItemHeader;
struct TextDataStorageItem;
struct ElementDataStorageItem;
struct NodeItem;
class DataBuffer;


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
class lxmlDocBase
{
    friend class ldomNode;
    friend class ldomElement;
    friend class ldomPersistentElement;
    friend class ldomText;
    friend class ldomPersistentText;
	friend class ldomXPointer;
public:

    /// Default constructor
    lxmlDocBase( int dataBufSize = DEF_DOC_DATA_BUFFER_SIZE );
    /// Copy constructor - copies ID tables contents
    lxmlDocBase( lxmlDocBase & doc );
    /// Destructor
    virtual ~lxmlDocBase();

	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serializeMaps( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	bool deserializeMaps( SerialBuf & buf );

    //======================================================================
    // Name <-> Id maps functions

    /// Get namespace name by id
    /**
        \param id is numeric value of namespace
        \return string value of namespace
    */
    inline const lString16 & getNsName( lUInt16 id )
    {
        return _nsNameTable.nameById( id );
    }

    /// Get namespace id by name
    /**
        \param name is string value of namespace
        \return id of namespace
    */
    lUInt16 getNsNameIndex( const lChar16 * name );

    /// Get attribute name by id
    /**
        \param id is numeric value of attribute
        \return string value of attribute
    */
    inline const lString16 & getAttrName( lUInt16 id )
    {
        return _attrNameTable.nameById( id );
    }

    /// Get attribute id by name
    /**
        \param name is string value of attribute
        \return id of attribute
    */
    lUInt16 getAttrNameIndex( const lChar16 * name );

    /// helper: returns attribute value
    inline const lString16 & getAttrValue( lUInt16 index ) const
    {
        return _attrValueTable[index];
    }

    /// helper: returns attribute value index
    inline lUInt16 getAttrValueIndex( const lChar16 * value )
    {
        return (lUInt16)_attrValueTable.add( value );
    }

    /// helper: returns attribute value index, 0xffff if not found
    inline lUInt16 findAttrValueIndex( const lChar16 * value )
    {
        return (lUInt16)_attrValueTable.find( value );
    }

    /// Get element name by id
    /**
        \param id is numeric value of element name
        \return string value of element name
    */
    inline const lString16 & getElementName( lUInt16 id )
    {
        return _elementNameTable.nameById( id );
    }

    /// Get element id by name
    /**
        \param name is string value of element name
        \return id of element
    */
    lUInt16 getElementNameIndex( const lChar16 * name );

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

    inline void cacheStyle(css_style_ref_t & styleref )
    {
        _styleCache.cacheIt( styleref );
    }
    /// garbage collector
    virtual void gc()
    {
        _styleCache.gc();
#if BUILD_LITE!=1
        fontMan->gc();
#endif
    }

    //inline LVStyleSheet * getStyleSheet() { return &_stylesheet; }
    /// sets style sheet, clears old content of css if arg replace is true
    void setStyleSheet( const char * css, bool replace );
    /// apply document's stylesheet to element node
    inline void applyStyle( ldomNode * element, css_style_rec_t * pstyle)
    {
        _stylesheet.apply( element, pstyle );
    }


    void onAttributeSet( lUInt16 attrId, lUInt16 valueId, ldomNode * node );

    /// get element by id attribute value code
    inline ldomNode * getNodeById( lUInt16 attrValueId )
    {
        return getNodeInstance( _idNodeMap.get( attrValueId ) );
    }

    /// get element by id attribute value
    inline ldomNode * getElementById( const lChar16 * id )
    {
        lUInt16 attrValueId = getAttrValueIndex( id );
        ldomNode * node = getNodeById( attrValueId );
        return node;
    }
    /// returns doc properties collection
    inline CRPropRef getProps() { return _docProps; }
    /// returns doc properties collection
    void setProps( CRPropRef props ) { _docProps = props; }

    /// put all object into persistent storage
    virtual void persist();

    /// returns root element
    ldomNode * getRootNode();

    /// returns code base path relative to document container
    inline lString16 getCodeBase() { return getProps()->getStringDef(DOC_PROP_CODE_BASE, ""); }
    /// sets code base path relative to document container
    inline void setCodeBase(lString16 codeBase) { getProps()->setStringDef(DOC_PROP_CODE_BASE, codeBase); }

#ifdef _DEBUG
    ///debug method, for DOM tree consistency check, returns false if failed
    bool checkConsistency( bool requirePersistent );
#endif

    inline bool getDocFlag( lUInt32 mask )
    {
        return (_docFlags & mask) != 0;
    }

    inline void setDocFlag( lUInt32 mask, bool value )
    {
        if ( value )
            _docFlags |= mask;
        else
            _docFlags &= ~mask;
    }

    inline lUInt32 getDocFlags()
    {
        return _docFlags;
    }

    inline void setDocFlags( lUInt32 value )
    {
        _docFlags = value;
    }

    /// try opening from cache file, find by source file name (w/o path) and crc32
    virtual bool openFromCache( ) = 0;
    /// swap to cache file, find by source file name (w/o path) and crc32
    virtual bool swapToCache( lUInt32 reservedDataSize=0 ) = 0;
    /// saves recent changes to mapped file
    virtual bool updateMap() = 0;

    /// returns or creates object instance by index
    inline ldomNode * getNodeInstance( lInt32 dataIndex )
    {
        return _instanceMap[ dataIndex ].instance;
/*
        ldomNode * item = _instanceMap[ dataIndex ].instance;
        if ( item != NULL )
            return item;
        // TODO: try to create instance from data
        CRLog::error("NULL instance for index %d", dataIndex);
        return NULL;
*/
    }
protected:

    
//=========================================
//       NEW STORAGE MODEL METHODS
//=========================================
    struct NodeItem {
        // object's RAM instance
        ldomNode * instance;
        // object's data pointer
        DataStorageItemHeader * data;
        // empty item constructor
        NodeItem() : instance(NULL), data(NULL) { }
    };
	/// for persistent text node, return wide text by index, with caching (TODO)
    lString16 getTextNodeValue( lInt32 dataIndex );
	/// for persistent text node, return utf8 text by index, with caching (TODO)
    lString8 getTextNodeValue8( lInt32 dataIndex );
    /// used by object constructor, to assign ID for created object
    lInt32 registerNode( ldomNode * node );
    /// used by object destructor, to remove RAM reference; leave data as is
    void unregisterNode( ldomNode * node );
    /// used by persistance management constructors, to replace one instance with another, deleting old instance
    ldomNode * replaceInstance( lInt32 dataIndex, ldomNode * newInstance );
    /// used to create instances from mmapped file, returns passed node instance
    ldomNode * setNode( lInt32 dataIndex, ldomNode * instance, DataStorageItemHeader * data );
    /// used by object destructor, to remove RAM reference; mark data as deleted
    void deleteNode( ldomNode * node );
	/// returns pointer to node data block
	inline DataStorageItemHeader * getNodeData( lInt32 dataIndex ) { return _instanceMap[ dataIndex ].data; }
	/// returns pointer to text node data block
	inline TextDataStorageItem * getTextNodeData( lInt32 dataIndex ) { return (TextDataStorageItem *)_instanceMap[ dataIndex ].data;  }
	/// returns pointer to text node data block
	inline ElementDataStorageItem * getElementNodeData( lInt32 dataIndex ) { return (ElementDataStorageItem *)_instanceMap[ dataIndex ].data;  }
	/// allocate data block, return pointer to allocated block
	DataStorageItemHeader * allocData( lInt32 dataIndex, int size );
	/// allocate text block
	TextDataStorageItem * allocText( lInt32 dataIndex, lInt32 parentIndex, const lChar8 * text, int charCount );
	/// allocate element
	ElementDataStorageItem * allocElement( lInt32 dataIndex, lInt32 parentIndex, int attrCount, int childCount );

    bool keepData() { return _keepData; }
protected:
    struct DocFileHeader {
        //char magic[16]; //== doc_file_magic
        lUInt32 src_file_size;
        lUInt32 src_file_crc32;
        lUInt32 props_offset;
        lUInt32 props_size;
        lUInt32 idtable_offset;
        lUInt32 idtable_size;
        lUInt32 pagetable_offset;
        lUInt32 pagetable_size;
        lUInt32 data_offset;
        lUInt32 data_size;
        lUInt32 data_crc32;
        lUInt32 data_index_size;
        lUInt32 file_size;

        lUInt32 render_dx;
        lUInt32 render_dy;
        lUInt32 render_docflags;
        lUInt32 render_style_hash;
        //
        lString16 src_file_name;

        bool serialize( SerialBuf & buf );
        bool deserialize( SerialBuf & buf );
        DocFileHeader()
            : file_size(0), render_dx(0), render_dy(0), render_docflags(0), render_style_hash(0)
        {
        }
    };
    DocFileHeader hdr;


    LVPtrVector<DataBuffer> _dataBuffers; // node data buffers
	DataBuffer * _currentBuffer;
	int _dataBufferSize;       // single data buffer size
    NodeItem * _instanceMap;   // Id->Instance & Id->Data map
    int _instanceMapSize;      //
    int _instanceMapCount;     //
    LDOMNameIdMap _elementNameTable;    // Element Name<->Id map
    LDOMNameIdMap _attrNameTable;       // Attribute Name<->Id map
    LDOMNameIdMap _nsNameTable;          // Namespace Name<->Id map
    lUInt16       _nextUnknownElementId; // Next Id for unknown element
    lUInt16       _nextUnknownAttrId;    // Next Id for unknown attribute
    lUInt16       _nextUnknownNsId;      // Next Id for unknown namespace
    lvdomStyleCache _styleCache;         // Style cache
    LVStyleSheet  _stylesheet;
    lString16HashedCollection _attrValueTable;
    LVHashTable<lUInt16,lInt32> _idNodeMap; // id to data index map
    lUInt16 _idAttrId; // Id for "id" attribute name
    CRPropRef _docProps;
    bool _keepData; // if true, node deletion will not change persistent data

    LVStreamRef _map; // memory mapped file
    LVStreamBufferRef _mapbuf; // memory mapped file buffer
    bool _mapped; // true if document is mapped to file
    lUInt32 _docFlags; // document flags

    SerialBuf _pagesData;
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
    lUInt16 index;
    inline bool compare( lUInt16 nsId, lUInt16 attrId )
    {
        return (nsId == nsid || nsId == LXML_NS_ANY) && (id == attrId);
    }
    inline void setData( lUInt16 nsId, lUInt16 attrId, lUInt16 valueIndex )
    {
        nsid = nsId;
        id = attrId;
        index = valueIndex;
    }
};

class ldomDocument;


#define LDOM_ALLOW_NODE_INDEX 0

/// fastDOM NODE interface - base class for all text and element node implementations
class ldomNode
{
    friend class ldomElement;
    friend class lxmlDocBase;
    // vtable                    0: [4]
protected:
    /// document which owns this node
    ldomDocument * _document; // 4: [4]
    /// data index of parent node
    lInt32 _parentIndex;      // 8: [4]
    /// data index of this node
    lInt32 _dataIndex;        //12: [4]

#if (LDOM_ALLOW_NODE_INDEX==1)
	/// optional index of this node in parent's children collection
    lUInt32 _index;
#endif

    virtual void addChild( lInt32 ) { }

    ldomNode( const ldomNode * node )
    : _document(node->_document), _parentIndex( node->_parentIndex ), _dataIndex( node->_dataIndex )
#if (LDOM_ALLOW_NODE_INDEX==1)
    , _index( node->getNodeIndex() )
#endif
    {
	}

    ldomNode( ldomDocument * document, lInt32 parentIndex, lInt32 dataIndex )
        : _document( document ), _parentIndex( parentIndex ), _dataIndex( dataIndex )
    {
    }

public:
    ldomNode( ldomDocument * document, ldomNode * parent, lUInt32
#if (LDOM_ALLOW_NODE_INDEX==1)
              index
#endif
              )
    : _document(document), _parentIndex( parent ? parent->getDataIndex() : 0 )
#if (LDOM_ALLOW_NODE_INDEX==1)
    , _index(index)
#endif
    {
		_dataIndex  = ((lxmlDocBase*)document)->registerNode(this);
	}
	/// destructor
    virtual ~ldomNode();
    /// returns true if node is stored in persistent storage
    virtual bool isPersistent() { return false; }

    /// returns data index of node's registration in document data storage
    inline lInt32 getDataIndex() { return _dataIndex; }
    /// returns pointer to document
    inline ldomDocument * getDocument() const { return _document; }
	/// returns pointer to parent node, NULL if node has no parent
    inline ldomNode * getParentNode() const { return _parentIndex > 0 ? ((lxmlDocBase*)_document)->getNodeInstance(_parentIndex) : NULL; }
	/// returns node type, either LXML_TEXT_NODE or LXML_ELEMENT_NODE
    virtual lUInt8 getNodeType() const = 0;
	/// returns node level, 0 is root node
    virtual lUInt8 getNodeLevel() const;
    /// returns node index
#if (LDOM_ALLOW_NODE_INDEX==1)
    inline lUInt32 getNodeIndex() const { return _index; }
#else
    lUInt32 getNodeIndex() const;
#endif
	/// returns true if this pointer is NULL
    inline bool isNull() const { return this == NULL; }
	/// returns true if node is document's root
    inline bool isRoot() const { return _parentIndex <= 0; }
	/// returns true if node is text
    inline bool isText() const { return getNodeType() == LXML_TEXT_NODE; }
	/// returns true if node is element
    inline bool isElement() const { return getNodeType() == LXML_ELEMENT_NODE; }
    /// returns true if node is and element that has children
    inline bool hasChildren() { return getChildCount()!=0; }
    /// returns true if node is element has attributes
    inline bool hasAttributes() const { return getAttrCount()!=0; }
#if (LDOM_ALLOW_NODE_INDEX==1)
    inline void setIndex( lUInt32 index ) { _index = index; }
#endif

    // virtual functions

    /// returns element child count
    virtual lUInt32 getChildCount() const = 0;
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const = 0;
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const = 0;
    /// returns attribute value by attribute name
    inline const lString16 & getAttributeValue( const lChar16 * attrName ) const
    {
        return getAttributeValue( NULL, attrName );
    }
    /// returns attribute value by attribute name and namespace
    virtual const lString16 & getAttributeValue( const lChar16 * nsName, const lChar16 * attrName ) const;
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 ) const = 0;
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsId, lUInt16 attrId ) const = 0;
    /// returns attribute name by index
    virtual const lString16 & getAttributeName( lUInt32 ) const { return lString16::empty_str; }
    /// sets attribute value
    virtual void setAttributeValue( lUInt16 , lUInt16 , const lChar16 *  ) { }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const css_elem_def_props_t * getElementTypePtr() = 0;
    /// returns element name id
    virtual lUInt16 getNodeId() const = 0;
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const = 0;
    /// replace element name id with another value
    virtual void setNodeId( lUInt16 ) { }
    /// returns element name
    virtual const lString16 & getNodeName() const = 0;
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const = 0;
    /// returns text node text as wide string
    virtual lString16 getText( lChar16 blockDelimiter = 0 ) const;
    /// returns text node text as utf8 string
    virtual lString8 getText8( lChar8 blockDelimiter = 0 ) const;
    /// sets text node text as wide string
    virtual void setText( lString16 ) { }
    /// sets text node text as utf8 string
    virtual void setText8( lString8 ) { }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const = 0;
    /// returns node absolute rectangle
    virtual void getAbsRect( lvRect & rect );
    /// returns render data structure
    virtual lvdomElementFormatRec * getRenderData() { return NULL; }
    /// sets node rendering structure pointer
    virtual void clearRenderData() { }
    /// calls specified function recursively for all elements of DOM tree
    virtual void recurseElements( void (*pFun)( ldomNode * node ) );
    /// calls specified function recursively for all nodes of DOM tree
    virtual void recurseNodes( void (*pFun)( ldomNode * node ) );


    // inline, dummy namespace

    /// returns attribute value by attribute name id
    inline const lString16 & getAttributeValue( lUInt16 id ) const
    {
        return getAttributeValue( LXML_NS_ANY, id );
    }
    /// returns true if element node has attribute with specified name id
    inline bool hasAttribute( lUInt16 id ) const
    {
        return hasAttribute( LXML_NS_ANY, id );
    }
    /// returns first text child element
    virtual ldomNode * getFirstTextChild();
    /// returns last text child element
    virtual ldomNode * getLastTextChild();
#if BUILD_LITE!=1
    /// find node by coordinates of point in formatted document
    virtual ldomNode * elementFromPoint( lvPoint pt );
    /// find final node by coordinates of point in formatted document
    virtual ldomNode * finalBlockFromPoint( lvPoint pt );
#endif

    // rich interface stubs for supporting Element operations
    /// returns rendering method
    virtual lvdom_element_render_method  getRendMethod() { return erm_invisible; }
    /// sets rendering method
    virtual void setRendMethod( lvdom_element_render_method ) { }
    /// returns element style record
    virtual css_style_ref_t getStyle() { return css_style_ref_t(); }
    /// returns element font
    virtual font_ref_t getFont() { return font_ref_t(); }
    /// sets element font
    virtual void setFont( font_ref_t ) { }
    /// sets element style record
    virtual void setStyle( css_style_ref_t & ) { }

    /// returns first child node
    virtual ldomNode * getFirstChild() const { return NULL; }
    /// returns last child node
    virtual ldomNode * getLastChild() const { return NULL; }
    /// removes and deletes last child element
    virtual void removeLastChild() { }
    /// move range of children startChildIndex to endChildIndex inclusively to specified element
    virtual void moveItemsTo( ldomNode *, int , int ) { }
    /// find child element by tag id
    ldomNode * findChildElement( lUInt16 nsid, lUInt16 id, int index );
    /// find child element by id path
    ldomNode * findChildElement( lUInt16 idPath[] );
    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt32 index, lUInt16 nsid, lUInt16 id ) = 0;
    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt16 id ) = 0;
    /// inserts child text
    virtual ldomNode * insertChildText( lUInt32 index, lString16 value ) = 0;
    /// inserts child text
    virtual ldomNode * insertChildText( lString16 value ) = 0;
    /// remove child
    virtual ldomNode * removeChild( lUInt32 index ) = 0;
    /// creates stream to read base64 encoded data from element
    LVStreamRef createBase64Stream();
#if BUILD_LITE!=1
    /// returns object image source
    LVImageSourceRef getObjectImageSource();
    /// formats final block
    int renderFinalBlock(  LFormattedTextRef & frmtext, int width );
#endif
    /// replace node with r/o persistent implementation
    virtual ldomNode * persist() { return this; }
    /// replace node with r/w implementation
    virtual ldomNode * modify() { return this; }
protected:
    /// override to avoid deleting children while replacing
    virtual void prepareReplace() { }
};

class ldomDocument;


/**
 * @brief XPointer/XPath object with reference counting.
 * 
 */
class ldomXPointer
{
protected:
	struct XPointerData {
	protected:
		ldomDocument * _doc;
		lInt32 _dataIndex;
		int _offset;
		int _refCount;
	public:
		inline void addRef() { _refCount++; }
		inline void release() { if ( (--_refCount)==0 ) delete this; }
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
		inline bool isNull() { return _dataIndex==0; }
		inline ldomNode * getNode() { return _dataIndex>0 ? ((lxmlDocBase*)_doc)->getNodeInstance( _dataIndex ) : NULL; }
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
		~XPointerData() { }
	};
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
	XPointerData * _data;
	/// 
	inline ldomDocument * getDocument() { return _data->getDocument(); }
    /// returns node pointer
	inline ldomNode * getNode() const { return _data->getNode(); }
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
	~ldomXPointer() { _data->release(); }
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
		_data->release();
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
    ldomXPointer relative( lString16 relativePath );
    /// get pointer for relative path
    ldomXPointer relative( const lChar16 * relativePath )
    {
        return relative( lString16(relativePath) );
    }

    /// returns true for NULL pointer
	bool isNull() const
	{
		return _data->isNull();
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
#if BUILD_LITE!=1
    /// returns caret rectangle for pointer inside formatted document
    bool getRect(lvRect & rect) const;
    /// returns coordinates of pointer inside formatted document
    lvPoint toPoint() const;
#endif
    /// converts to string
	lString16 toString();
    /// returns XPath node text
    lString16 getText(  lChar16 blockDelimiter=0 )
    {
		ldomNode * node = getNode();
        if ( !node )
            return lString16();
        return node->getText( blockDelimiter );
    }
    /// returns href attribute of <A> element, null string if not found
    lString16 getHRef();
	/// create a copy of pointer data
	ldomXPointer * clone()
	{
		return new ldomXPointer( _data );
	}
    /// returns true if current node is element
    inline bool isElement() const { return !isNull() && getNode()->isElement(); }
    /// returns true if current node is element
    inline bool isText() const { return !isNull() && getNode()->isText(); }
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
		_data->release();
		_data = new XPointerData( *v._data );
        initIndex();
        return *this;
    }
    /// assignment operator
    ldomXPointerEx & operator =( const ldomXPointerEx& v )
    {
		if ( _data==v._data )
			return *this;
		_data->release();
		_data = new XPointerData( *v._data );
        _level = v._level;
        for ( int i=0; i<_level; i++ )
            _indexes[ i ] = v._indexes[i];
        return *this;
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
    /// move to next text node
    bool nextText();
    /// move to previous text node
    bool prevText();
    /// move to next visible text node
    bool nextVisibleText();
    /// move to previous visible text node
    bool prevVisibleText();
    /// forward iteration by elements of DOM three
    bool nextElement();
    /// backward iteration by elements of DOM three
    bool prevElement();
    /// calls specified function recursively for all elements of DOM tree
    void recurseElements( void (*pFun)( ldomXPointerEx & node ) );
    /// calls specified function recursively for all nodes of DOM tree
    void recurseNodes( void (*pFun)( ldomXPointerEx & node ) );
};

class ldomXRange;

/// callback for DOM tree iteration interface
class ldomNodeCallback {
public:
    /// destructor
    virtual ~ldomNodeCallback() { }
    /// called for each found text fragment in range
    virtual void onText( ldomXRange * ) { }
    /// called for each found node in range
    virtual bool onElement( ldomXPointerEx * ) { return true; }
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
    lString16 getText()
    {
        if ( isNull() )
            return lString16();
        lString16 txt = _node->getText();
        return txt.substr( _start, _end-_start );
    }
};

/// DOM range
class ldomXRange {
    ldomXPointerEx _start;
    ldomXPointerEx _end;
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
    /// create intersection of two ranges
    ldomXRange( const ldomXRange & v1,  const ldomXRange & v2 );
    /// copy constructor of full node range
    ldomXRange( ldomNode * p );
    /// copy assignment
    ldomXRange & operator = ( const ldomXRange & v )
    {
        _start = v._start;
        _end = v._end;
        return *this;
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
    lString16 getRangeText( lChar16 blockDelimiter='\n', int maxTextLen=0 );
    /// get all words from specified range
    void getRangeWords( LVArray<ldomWord> & list );
    /// returns href attribute of <A> element, null string if not found
    lString16 getHRef();
    /// sets range to nearest word bounds, returns true if success
    static bool getWordRange( ldomXRange & range, ldomXPointer & p );
    /// run callback for each node in range
    void forEach( ldomNodeCallback * callback );
#if BUILD_LITE!=1
    /// returns rectangle (in doc coordinates) for range. Returns true if found.
    bool getRect( lvRect & rect );
#endif
    /// returns nearest common element for start and end points
    ldomNode * getNearestCommonParent();

    /// searches for specified text inside range
    bool findText( lString16 pattern, bool caseInsensitive, LVArray<ldomWord> & words, int maxCount );
};

class ldomMarkedText
{
public:
    lString16 text;
    lUInt32   flags;
    int offset;
    ldomMarkedText( lString16 s, lUInt32 flg, int offs )
    : text(s), flags(flg), offset(offs)
    {
    }
    ldomMarkedText( const ldomMarkedText & v )
    : text(v.text), flags(v.flags)
    {
    }
};

typedef LVPtrVector<ldomMarkedText> ldomMarkedTextList;

/// range in document, marked with specified flags
class ldomMarkedRange
{
public:
    /// start document point
    lvPoint   start;
    /// end document point
    lvPoint   end;
    /// flags
    lUInt32   flags;
    bool empty()
    {
        return ( start.y>end.y || ( start.y == end.y && start.x >= end.x ) );
    }
    /// returns true if intersects specified line rectangle
    bool intersects( lvRect & rc, lvRect & intersection );
    /// constructor
    ldomMarkedRange( lvPoint _start, lvPoint _end, lUInt32 _flags )
    : start(_start), end(_end), flags(_flags)
    {
    }
    /// copy constructor
    ldomMarkedRange( const ldomMarkedRange & v )
    : start(v.start), end(v.end), flags(v.flags)
    {
    }
};

/// list of marked ranges
class ldomMarkedRangeList : public LVPtrVector<ldomMarkedRange>
{
public:
    ldomMarkedRangeList()
    {
    }
    /// create bounded by RC list, with (0,0) coordinates at left top corner
    ldomMarkedRangeList( const ldomMarkedRangeList * list, lvRect & rc );
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



class ldomNavigationHistory
{
    private:
        lString16Collection _links;
        int _pos;
        void clearTail()
        {
            if ( _links.length()-_pos > 0 )
                _links.erase(_pos, _links.length()-_pos);
        }
    public:
        void clear()
        {
            _links.clear();
            _pos = 0;
        }
        void save( lString16 link )
        {
            clearTail();
            _links.add( link );
            _pos = _links.length();
        }
        lString16 back()
        {
            if (_pos==0)
                return lString16();
            return _links[--_pos];
        }
        lString16 forward()
        {
            if (_pos>=(int)_links.length())
                return lString16();
            return _links[_pos++];
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

class ldomDocument : public lxmlDocBase
{
    friend class ldomDocumentWriter;
private:
    font_ref_t _def_font; // default font
    css_style_ref_t _def_style;
    int _page_height;
    ldomXRangeList _selections;

#if BUILD_LITE!=1
    /// final block cache
    CVRendBlockCache _renderedBlockCache;
#endif
    LVContainerRef _container;

protected:

    /// uniquie id of file format parsing option (usually 0, but 1 for preformatted text files)
    int getPersistenceFlags();

public:

    /// save document formatting parameters after render
    void updateRenderContext( LVRendPageList * pages, int dx, int dy );
    /// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
    bool checkRenderContext( LVRendPageList * pages, int dx, int dy );

    /// try opening from cache file, find by source file name (w/o path) and crc32
    virtual bool openFromCache( );
    /// swap to cache file, find by source file name (w/o path) and crc32
    virtual bool swapToCache( lUInt32 reservedDataSize=0 );
    /// saves recent changes to mapped file
    virtual bool updateMap();


    LVContainerRef getContainer() { return _container; }
    void setContainer( LVContainerRef cont ) { _container = cont; }


    ldomDocument();
    /// creates empty document which is ready to be copy target of doc partial contents
    ldomDocument( ldomDocument & doc );

    /// return selections collection
    ldomXRangeList & getSelections() { return _selections; }
    /// get full document height
    int getFullHeight();
    /// returns page height setting
    int getPageHeight() { return _page_height; }
    /// saves document contents as XML to stream with specified encoding
    bool saveToStream( LVStreamRef stream, const char * codepage );
    /// get default font reference
    font_ref_t getDefaultFont() { return _def_font; }
    /// get default style reference
    css_style_ref_t getDefaultStyle() { return _def_style; }
    /// destructor
    virtual ~ldomDocument();
#if BUILD_LITE!=1
    /// renders (formats) document in memory
    virtual int render( LVRendPageList * pages, int width, int dy, bool showCover, int y0, font_ref_t def_font, int def_interline_space );
#endif
    /// create xpointer from pointer string
    ldomXPointer createXPointer( const lString16 & xPointerStr );
    /// create xpointer from pointer string
    ldomNode * nodeFromXPath( const lString16 & xPointerStr )
    {
        return createXPointer( xPointerStr ).getNode();
    }
    /// get element text by pointer string
    lString16 textFromXPath( const lString16 & xPointerStr )
    {
        ldomNode * node = nodeFromXPath( xPointerStr );
        if ( !node )
            return lString16();
        return node->getText();
    }

    /// create xpointer from relative pointer string
    ldomXPointer createXPointer( ldomNode * baseNode, const lString16 & xPointerStr );
#if BUILD_LITE!=1
    /// create xpointer from doc point
    ldomXPointer createXPointer( lvPoint pt );
    /// get rendered block cache object
    CVRendBlockCache & getRendBlockCache() { return _renderedBlockCache; }

    bool findText( lString16 pattern, bool caseInsensitive, int minY, int maxY, LVArray<ldomWord> & words, int maxCount );
#endif
};


class ldomDocumentWriter;

class ldomElementWriter
{
    ldomElementWriter * _parent;
    ldomDocument * _document;

    ldomNode * _element;
    const css_elem_def_props_t * _typeDef;
    bool _allowText;
    lUInt32 getFlags();
    ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent);
    ldomNode * getElement()
    {
        return _element;
    }
    void onText( const lChar16 * text, int len, lUInt32 flags );
    void addAttribute( lUInt16 nsid, lUInt16 id, const wchar_t * value );
    //lxmlElementWriter * pop( lUInt16 id );

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
    lUInt16 _stopTagId;
    //============================
    lUInt32 _flags;
    virtual void ElementCloseHandler( ldomNode * ) { }
public:
    /// returns flags
    virtual lUInt32 getFlags() { return _flags; }
    /// sets flags
    virtual void setFlags( lUInt32 flags ) { _flags = flags; }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding( const lChar16 * name, const lChar16 * table );
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser * parser);
    /// called on parsing end
    virtual void OnStop();
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
    /// called on closing tag
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
    /// called on attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
    /// close tags
    ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 flags );
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
    bool _libRuDocumentDetected;
    bool _libRuParagraphStart;
    lUInt16 * _rules[MAX_ELEMENT_TYPE_ID];
    virtual void AutoClose( lUInt16 tag_id, bool open );
    virtual void ElementCloseHandler( ldomNode * elem );
public:
    /// called on attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
    /// called on closing tag
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 flags );
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
    lString16 baseTag;
    bool insideTag;
public:
    /// returns flags
    virtual lUInt32 getFlags() { return parent->getFlags(); }
    /// sets flags
    virtual void setFlags( lUInt32 flags ) { parent->setFlags(flags); }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding( const lChar16 * name, const lChar16 * table )
    { parent->OnEncoding( name, table ); }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser *)
    {
        insideTag = false;
    }
    /// called on parsing end
    virtual void OnStop()
    {
        insideTag = false;
    }
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
    {
        ldomNode * res = NULL;
        if ( insideTag ) {
            res = parent->OnTagOpen(nsname, tagname);
        }
        if ( !insideTag && baseTag==tagname )
            insideTag = true;
        return res;
    }
    /// called on closing tag
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
    {
        if ( insideTag && baseTag==tagname )
            insideTag = false;
        if ( insideTag )
            parent->OnTagClose(nsname, tagname);
    }
    /// called on attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
    {
        if ( insideTag )
            parent->OnAttribute(nsname, attrname, attrvalue);
    }
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 flags )
    {
        if ( insideTag )
            parent->OnText( text, len, flags );
    }
    /// constructor
    ldomDocumentFragmentWriter( LVXMLParserCallback * parentWriter, lString16 baseTagName )
    : parent(parentWriter), baseTag(baseTagName), insideTag(false)
    {
    }
    /// destructor
    virtual ~ldomDocumentFragmentWriter() { }
};

//utils
lString16 extractDocAuthors( ldomDocument * doc );
lString16 extractDocTitle( ldomDocument * doc );
lString16 extractDocSeries( ldomDocument * doc );

bool IsEmptySpace( const lChar16 * text, int len );

/// parse XML document from stream, returns NULL if failed
ldomDocument * LVParseXMLStream( LVStreamRef stream,
                              const elem_def_t * elem_table=NULL,
                              const attr_def_t * attr_table=NULL,
                              const ns_def_t * ns_table=NULL );

/// document cache
class ldomDocCache
{
public:
    /// open existing cache file stream
    static LVStreamRef openExisting( lString16 filename, lUInt32 crc, lUInt32 docFlags );
    /// create new cache file
    static LVStreamRef createNew( lString16 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize );
    /// init document cache
    static bool init( lString16 cacheDir, lvsize_t maxSize );
    /// close document cache manager
    static bool close();
    /// delete all cache files
    static bool clear();
    /// returns true if cache is enabled (successfully initialized)
    static bool enabled();
};

#endif
