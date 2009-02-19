/** \file lvtinydom.h
    \brief compact read-only XML DOM tree header

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details
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

#define LXML_DOCUMENT_NODE 1 ///< document node
#define LXML_ELEMENT_NODE  2 ///< element node
#define LXML_TEXT_NODE     3 ///< text node
#define LXML_COMMENT_NODE  4 ///< comment node

#define LXML_NS_NONE 0       ///< no namespace specified
#define LXML_NS_ANY  0xFFFF  ///< any namespace can be specified
#define LXML_ATTR_VALUE_NONE  0xFFFF  ///< attribute not found

#define DOC_STRING_HASH_SIZE  256
#define RESERVED_DOC_SPACE    4096
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
#define DOC_PROP_FILE_NAME       "doc.file.name"
#define DOC_PROP_FILE_PATH       "doc.file.path"
#define DOC_PROP_FILE_SIZE       "doc.file.size"
#define DOC_PROP_FILE_FORMAT     "doc.file.format"

//#define LDOM_USE_OWN_MEM_MAN 0

typedef enum {
	xpath_step_error = 0, // error
	xpath_step_element,   // element of type 'name' with 'index'        /elemname[N]/
	xpath_step_text,      // text node with 'index'                     /text()[N]/
	xpath_step_nodeindex, // node index                                 /N/
	xpath_step_point      // point index                                .N
} xpath_step_t;
xpath_step_t ParseXPathStep( const lChar8 * &path, lString8 & name, int & index );

class ldomElement;

/// Base class for XML DOM documents
/**
    Helps to decrease memory usage and increase performance for DOM implementations.
    Maintains Name<->Id maps for element names, namespaces and attributes.
    It allows to use short IDs instead of strings in DOM internals,
    and avoid duplication of string values.
*/
class lxmlDocBase
{
public:

    /// Default constructor
    lxmlDocBase();
    /// Copy constructor - copies ID tables contents
    lxmlDocBase( lxmlDocBase & doc );
    /// Destructor
    virtual ~lxmlDocBase() { }

    //======================================================================
    // Name <-> Id maps functions

    /// Get namespace name by id
    /**
        \param id is numeric value of namespace
        \return string value of namespace
    */
    const lString16 & getNsName( lUInt16 id )
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
    const lString16 & getAttrName( lUInt16 id )
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
    const lString16 & getAttrValue( lUInt16 index ) const
    {
        return _attrValueTable[index];
    }

    /// helper: returns attribute value index
    lUInt16 getAttrValueIndex( const lChar16 * value )
    {
        return (lUInt16)_attrValueTable.add( value );
    }

    /// helper: returns attribute value index, 0xffff if not found
    lUInt16 findAttrValueIndex( const lChar16 * value )
    {
        return (lUInt16)_attrValueTable.find( value );
    }

    /// Get element name by id
    /**
        \param id is numeric value of element name
        \return string value of element name
    */
    const lString16 & getElementName( lUInt16 id )
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
    const elem_def_t * getElementTypePtr( lUInt16 id )
    {
        return (const elem_def_t *) _elementNameTable.dataById( id );
    }

    // set node types from table
    void setNodeTypes( const elem_def_t * node_scheme );
    // set attribute types from table
    void setAttributeTypes( const attr_def_t * attr_scheme );
    // set namespace types from table
    void setNameSpaceTypes( const ns_def_t * ns_scheme );

    // debug dump
    void dumpUnknownEntities( const char * fname );

    void cacheStyle(css_style_ref_t & styleref )
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

    LVStyleSheet * getStyleSheet() { return &_stylesheet; }

    inline void onAttributeSet( lUInt16 attrId, lUInt16 valueId, ldomNode * node )
    {
        if ( _idAttrId==0 )
            _idAttrId = _attrNameTable.idByName("id");
        if (attrId == _idAttrId)
        {
            _idNodeMap.set( valueId, node );
        }
    }

    /// get element by id attribute value code
    inline ldomNode * getNodeById( lUInt16 attrValueId )
    {
        return _idNodeMap.get( attrValueId );
    }

    /// get element by id attribute value
    inline ldomElement * getElementById( const lChar16 * id )
    {
        lUInt16 attrValueId = getAttrValueIndex( id );
        ldomNode * node = getNodeById( attrValueId );
        if ( node )
            return reinterpret_cast<ldomElement*>( node );
        return NULL;
    }
    /// returns doc properties collection
    CRPropRef getProps() { return _docProps; }
    /// returns doc properties collection
    void setProps( CRPropRef props ) { _docProps = props; }
private:
    LDOMNameIdMap _elementNameTable;    // Element Name<->Id map
    LDOMNameIdMap _attrNameTable;       // Attribute Name<->Id map
    LDOMNameIdMap _nsNameTable;         // Namespace Name<->Id map
    lUInt16       _nextUnknownElementId; // Next Id for unknown element
    lUInt16       _nextUnknownAttrId;    // Next Id for unknown attribute
    lUInt16       _nextUnknownNsId;      // Next Id for unknown namespace
    lvdomStyleCache _styleCache;         // Style cache
    LVStyleSheet  _stylesheet;
    lString16HashedCollection _attrValueTable;
    LVHashTable<lUInt16,ldomNode*> _idNodeMap;
    lUInt16 _idAttrId; // Id for "id" attribute name
    CRPropRef _docProps;
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
    bool compare( lUInt16 nsId, lUInt16 attrId )
    {
        return (nsId == nsid || nsId == LXML_NS_ANY) && (id == attrId);
    }
    void setData( lUInt16 nsId, lUInt16 attrId, lUInt16 valueIndex )
    {
        nsid = nsId;
        id = attrId;
        index = valueIndex;
    }
};

class ldomDocument;

class ldomElement;

class ldomText;

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
    lUInt16 get( lUInt16 nsId, lUInt16 attrId ) const
    {
        for (lUInt16 i=0; i<_len; i++)
        {
            if (_list[i].compare( nsId, attrId ))
                return _list[i].index;
        }
        return LXML_ATTR_VALUE_NONE;
    }
    void set( lUInt16 nsId, lUInt16 attrId, lUInt16 valueIndex )
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
            _list = (lxmlAttribute*) realloc( _list, _size*sizeof(lxmlAttribute) );
        }
        _list[ _len++ ].setData(nsId, attrId, valueIndex);
    }
};

#define LDOM_ALLOW_NODE_INDEX 0

/// fastDOM NODE interface
class ldomNode
{
    friend class ldomElement;
    // 0: this   [4]
protected:
    ldomElement * _parent;  // 4: [4]
#if (LDOM_ALLOW_NODE_INDEX==1)
    lUInt32 _index;
#endif
    lUInt8 _type;           // 8: [1]
    lUInt8 _level;          // 9: [1]
public:
    ldomNode( ldomElement * parent, lUInt8 type, lUInt8 level, lUInt32 index )
    : _parent(parent)
#if (LDOM_ALLOW_NODE_INDEX==1)
    , _index(index)
#endif
    , _type(type), _level(level) { }
    virtual ~ldomNode();
    // inline functions
    virtual ldomDocument * getDocument() const = 0;
    inline ldomElement * getParentNode() const { return _parent; }
    inline lUInt8 getNodeType() const { return _type; }
    inline lUInt8 getNodeLevel() const { return _level; }
    /// returns node index
#if (LDOM_ALLOW_NODE_INDEX==1)
    inline lUInt32 getNodeIndex() const { return _index; }
#else
    lUInt32 getNodeIndex() const;
#endif
    inline bool isNull() const { return this == NULL; }
    inline bool isRoot() const { return _parent == NULL; }
    inline bool isText() const { return _type == LXML_TEXT_NODE; }
    inline bool isElement() const { return _type == LXML_ELEMENT_NODE; }
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
    virtual const lString16 & getAttributeValue( const lChar16 * attrName ) const
    {
        return getAttributeValue( NULL, attrName );
    }
    /// returns attribute value by attribute name and namespace
    virtual const lString16 & getAttributeValue( const lChar16 * nsName, const lChar16 * attrName ) const;
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 index ) const = 0;
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsid, lUInt16 id ) const = 0;
    /// returns attribute name by index
    const lString16 & getAttributeName( lUInt32 index ) const { return lString16::empty_str; }
    /// sets attribute value
    virtual void setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value ) { }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const elem_def_t * getElementTypePtr() = 0;
    /// returns element name id
    virtual lUInt16 getNodeId() const = 0;
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const = 0;
    /// returns element name
    virtual const lString16 & getNodeName() const = 0;
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const = 0;
    /// returns text node text
    virtual lString16 getText( lChar16 blockDelimiter = 0 ) const = 0;
    /// sets text node text
    virtual void setText( lString16 value ) { }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const = 0;
    /// returns node absolute rectangle
    virtual void getAbsRect( lvRect & rect ) { }
    /// returns render data structure
    virtual lvdomElementFormatRec * getRenderData()
    {
        return NULL;
    }
    /// sets node rendering structure pointer
    virtual void setRenderData( lvdomElementFormatRec * pRenderData )
    {
    }
    /// calls specified function recursively for all elements of DOM tree
    virtual void recurseElements( void (*pFun)( ldomNode * node ) )
    {
    }
    /// calls specified function recursively for all nodes of DOM tree
    virtual void recurseNodes( void (*pFun)( ldomNode * node ) )
    {
        pFun( this );
    }


    // inline, dummy namespace

    /// returns attribute value by attribute name id
    const lString16 & getAttributeValue( lUInt16 id ) const
    {
        return getAttributeValue( LXML_NS_ANY, id );
    }
    /// returns true if element node has attribute with specified name id
    bool hasAttribute( lUInt16 id ) const
    {
        return hasAttribute( LXML_NS_ANY, id );
    }
    /// returns first text child element
    ldomText * getFirstTextChild();
    /// returns last text child element
    ldomText * getLastTextChild();
#if BUILD_LITE!=1
    /// find node by coordinates of point in formatted document
    ldomElement * elementFromPoint( lvPoint pt );
    /// find final node by coordinates of point in formatted document
    ldomElement * finalBlockFromPoint( lvPoint pt );
#endif
};

class ldomDocument;


class ldomText : public ldomNode
{
private:
#if (USE_DOM_UTF8_STORAGE==1)
    lString8 _value;
#else
    lString16 _value;
#endif

public:
#if (LDOM_USE_OWN_MEM_MAN==1)
    static ldomMemManStorage * pmsHeap;
    void * operator new( size_t size )
    {
        if (pmsHeap == NULL)
        {
            pmsHeap = new ldomMemManStorage(sizeof(ldomText));
        }
        return pmsHeap->alloc();
    }
    void operator delete( void * p )
    {
        pmsREF->free((ldomMemBlock *)p);
    }
#endif
    ldomText( ldomElement * parent, lUInt8 level, lUInt32 index, lString16 value)
    : ldomNode( parent, LXML_TEXT_NODE, level, index )
    {
#if (USE_DOM_UTF8_STORAGE==1)
        _value = UnicodeToUtf8(value);
#else
        _value = value;
#endif
    }
    virtual ~ldomText() { }
    virtual ldomDocument * getDocument() const;
    /// returns element child count
    virtual lUInt32 getChildCount() const { return 0; }
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return 0; }
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const
    {
        return lString16::empty_str;
    }
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 index ) const { return NULL; }
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsid, lUInt16 id ) const { return false; }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const elem_def_t * getElementTypePtr() { return NULL; }
    /// returns element name id
    virtual lUInt16 getNodeId() const { return 0; }
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return 0; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return lString16::empty_str; }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return lString16::empty_str; }
    /// returns text node text
    virtual lString16 getText( lChar16 blockDelimiter=0 ) const
    {
#if (USE_DOM_UTF8_STORAGE==1)
        return Utf8ToUnicode(_value);
#else
        return _value;
#endif
    }
    /// sets text node text
    virtual void setText( lString16 value )
    {
#if (USE_DOM_UTF8_STORAGE==1)
        _value = UnicodeToUtf8(value);
#else
        _value = value;
#endif
    }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const { return NULL; }
};

/**
 * @brief XPointer/XPath object
 */
class ldomXPointer
{
protected:
	/// node pointer
	ldomNode * _node;
	/// offset within node for pointer, -1 for xpath
	int _offset;
public:
    /// returns node pointer
	ldomNode * getNode() const { return _node; }
    /// returns offset within node
	int getOffset() const { return _offset; }
    /// default constructor makes NULL pointer
	ldomXPointer()
		: _node(NULL), _offset(-1)
	{
	}
    /// copy constructor
	ldomXPointer( const ldomXPointer& v )
		: _node(v._node), _offset(v._offset)
	{
	}
    /// assignment operator
	ldomXPointer & operator =( const ldomXPointer& v )
	{
        _node = v._node;
        _offset = v._offset;
        return *this;
	}
    /// constructor
	ldomXPointer( ldomNode * node, int offset )
		: _node(node), _offset(offset)
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
		return _node==NULL;
	}
    /// returns true if object is pointer
	bool isPointer() const
	{
		return _node!=NULL && _offset>=0;
	}
    /// returns true if object is path (no offset specified)
	bool isPath() const
	{
		return _node!=NULL && _offset==-1;
	}
    /// returns true if pointer is NULL
	bool operator !() const
	{
		return _node==NULL;
	}
    /// returns true if pointers are equal
	bool operator == (const ldomXPointer & v) const
	{
		return _node==v._node && _offset==v._offset;
	}
    /// returns true if pointers are not equal
	bool operator != (const ldomXPointer & v) const
	{
		return _node!=v._node || _offset!=v._offset;
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
        if ( !_node )
            return lString16();
        return _node->getText();
    }
    /// returns href attribute of <A> element, null string if not found
    lString16 getHRef();
};

#define MAX_DOM_LEVEL 64
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
    : ldomXPointer( v )
    {
        initIndex();
    }
    /// copy constructor
    ldomXPointerEx( const ldomXPointerEx& v )
    : ldomXPointer( v )
    {
        _level = v._level;
        for ( int i=0; i<_level; i++ )
            _indexes[ i ] = v._indexes[i];
    }
    /// assignment operator
    ldomXPointerEx & operator =( const ldomXPointer& v )
    {
        _node = v.getNode();
        _offset = v.getOffset();
        initIndex();
        return *this;
    }
    /// assignment operator
    ldomXPointerEx & operator =( const ldomXPointerEx& v )
    {
        _node = v._node;
        _offset = v._offset;
        _level = v._level;
        for ( int i=0; i<_level; i++ )
            _indexes[ i ] = v._indexes[i];
        return *this;
    }
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
    /// returns true if current node is element
    bool isElement() { return _node!=NULL && _node->isElement(); }
    /// move to next final visible node (~paragraph)
    bool nextVisibleFinal();
    /// move to previous final visible node (~paragraph)
    bool prevVisibleFinal();
    /// forward iteration by elements of DOM three
    bool nextElement();
    /// backward iteration by elements of DOM three
    bool prevElement();
    /// calls specified function recursively for all elements of DOM tree
    void recurseElements( void (*pFun)( ldomXPointerEx & node ) );
    /// calls specified function recursively for all nodes of DOM tree
    void recurseNodes( void (*pFun)( ldomXPointerEx & node ) );
    /// set new offset value
    void setOffset( int offset )
    {
        _offset = offset;
    }
};

class ldomXRange;

/// callback for DOM tree iteration interface
class ldomNodeCallback {
public:
    /// destructor
    virtual ~ldomNodeCallback() { }
    /// called for each found text fragment in range
    virtual void onText( ldomXRange * nodeRange ) { }
    /// called for each found node in range
    virtual bool onElement( ldomXPointerEx * ptr ) { return true; }
};

/// range for word inside text node
class ldomWord
{
    ldomText * _node;
    int _start;
    int _end;
public:
    ldomWord( )
    : _node(NULL), _start(0), _end(0)
    { }
    ldomWord( ldomText * node, int start, int end )
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
    ldomText * getNode() const { return _node; }
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
    ldomElement * getNearestCommonParent();
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



class ldomElement;
#if COMPACT_DOM == 1
class ldomTextRef;
#endif

#if BUILD_LITE!=1
/// final block cache
typedef LVRef<LFormattedText> LFormattedTextRef;
typedef LVCacheMap< ldomElement *, LFormattedTextRef> CVRendBlockCache;
#endif

/// docFlag mask, enable internal stylesheet of document and style attribute of elements
#define DOC_FLAG_ENABLE_INTERNAL_STYLES 1
/// docFlag mask, enable paperbook-like footnotes
#define DOC_FLAG_ENABLE_FOOTNOTES       2
/// default docFlag set
#define DOC_FLAG_DEFAULTS (DOC_FLAG_ENABLE_INTERNAL_STYLES|DOC_FLAG_ENABLE_FOOTNOTES)

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
    ldomElement * _root;
    font_ref_t _def_font; // default font
    css_style_ref_t _def_style;
    int _page_height;
    ldomXRangeList _selections;

#if COMPACT_DOM == 1
    LVXMLTextCache _textcache;
    int            _min_ref_text_size;
#endif

#if BUILD_LITE!=1
    /// final block cache
    CVRendBlockCache _renderedBlockCache;
#endif
    lUInt32 _docFlags;
    LVContainerRef _container;
    lString16 _codeBase;

public:

    lString16 getCodeBase() { return _codeBase; }
    void setCodeBase(lString16 codeBase) { _codeBase = codeBase; }
    LVContainerRef getContainer() { return _container; }
    void setContainer( LVContainerRef cont ) { _container = cont; }

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

#if COMPACT_DOM == 1
    ldomDocument(LVStreamRef stream, int min_ref_text_size);
    lString16 getTextNodeValue( const ldomTextRef * txt );
    void setMinRefTextSize( int size )
    {
        _min_ref_text_size = size;
    }
    bool allowTextRefForSize( int size )
    {
        return _min_ref_text_size>0 && size>_min_ref_text_size;
    }
#else
    ldomDocument();
#endif
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
    /// returns root element
    ldomElement * getRootNode() { return _root; }
    /// returns main element (i.e. FictionBook for FB2)
    ldomElement * getMainNode();
#if BUILD_LITE!=1
    /// renders (formats) document in memory
    virtual int render( LVRendPageContext & context, int width, int y0, font_ref_t def_font, int def_interline_space );
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
#endif
};

#if COMPACT_DOM == 1
class ldomTextRef : public ldomNode
{
    friend class ldomDocument;
private:

    lUInt16 dataFormat; // format flags
    lUInt32 dataSize;   // bytes
    lUInt32 fileOffset; // offset from beginning of stream
public:
#if (LDOM_USE_OWN_MEM_MAN == 1)
    static ldomMemManStorage * pmsHeap;
    void * operator new( size_t size )
    {
        if (pmsHeap == NULL)
        {
            pmsHeap = new ldomMemManStorage(sizeof(ldomTextRef));
        }
        return pmsHeap->alloc();
    }
    void operator delete( void * p )
    {
        pmsREF->free((ldomMemBlock *)p);
    }
#endif
    ldomTextRef( ldomElement * parent, lUInt8 level, lUInt32 index, lUInt32 pos, lUInt32 size, lUInt16 flags)
    : ldomNode( parent, LXML_TEXT_NODE, level, index ), dataFormat(flags), dataSize(size), fileOffset(pos)
    { }
    virtual ~ldomTextRef() { }
    virtual ldomDocument * getDocument() const;
    /// returns element child count
    virtual lUInt32 getChildCount() const { return 0; }
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return 0; }
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const
    {
        return lString16::empty_str;
    }
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 index ) const { return NULL; }
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsid, lUInt16 id ) const { return false; }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const elem_def_t * getElementTypePtr() { return NULL; }
    /// returns element name id
    virtual lUInt16 getNodeId() const { return 0; }
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return 0; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return lString16::empty_str; }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return lString16::empty_str; }
    /// returns text node text
    virtual lString16 getText( lChar16 blockDelimiter=0 ) const { return getDocument()->getTextNodeValue(this); }
    /// sets text node text
    virtual void setText( lString16 value );
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const { return NULL; }
};
#endif

class ldomElement : public ldomNode
{
private:
    ldomAttributeCollection _attrs;
    lUInt16 _id;
    lUInt16 _nsid;
    lvdomElementFormatRec * _renderData;   // used by rendering engine
    LVPtrVector < ldomNode > _children;
    ldomDocument * _document;
    css_style_ref_t _style;
    font_ref_t      _font;
    lvdom_element_render_method _rendMethod;
public:
#if (LDOM_USE_OWN_MEM_MAN == 1)
    static ldomMemManStorage * pmsHeap;
    void * operator new( size_t size )
    {
        if (pmsHeap == NULL)
        {
            pmsHeap = new ldomMemManStorage(sizeof(ldomElement));
        }
        return pmsHeap->alloc();
    }
    void operator delete( void * p )
    {
        pmsHeap->free((ldomMemBlock *)p);
    }
#endif
    ldomElement( ldomDocument * document, ldomElement * parent, lUInt8 level, lUInt32 index, lUInt16 nsid, lUInt16 id )
    : ldomNode( parent, LXML_ELEMENT_NODE, level, index ), _id(id), _nsid(nsid), _renderData(NULL), _document(document), _rendMethod(erm_invisible)
    { }
    virtual ~ldomElement() { if (_renderData) delete _renderData; }
    /// returns rendering method
    lvdom_element_render_method  getRendMethod() { return _rendMethod; }
    /// sets rendering method
    void setRendMethod( lvdom_element_render_method  method ) { _rendMethod=method; }
    /// returns element style record
    css_style_ref_t getStyle() { return _style; }
    /// returns element font
    font_ref_t getFont() { return _font; }
    /// sets element font
    void setFont( font_ref_t font ) { _font = font; }
    /// sets element style record
    void setStyle( css_style_ref_t & style ) { _style = style; }
    /// returns document
    virtual ldomDocument * getDocument() const { return _document; }
    /// returns element child count
    virtual lUInt32 getChildCount() const { return _children.length(); }
    /// returns first child node
    virtual ldomNode * getFirstChild() const { return _children.length()>0?_children[0]:NULL; }
    /// returns last child node
    virtual ldomNode * getLastChild() const { return _children.length()>0?_children[_children.length()-1]:NULL; }
    /// removes and deletes last child element
    virtual void removeLastChild()
    {
        if ( _children.length()>0 )
            delete _children.remove( _children.length() - 1 );
    }
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return _attrs.length(); }
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const
    {
        lUInt16 val_id = _attrs.get( nsid, id );
        if (val_id!=LXML_ATTR_VALUE_NONE)
            return _document->getAttrValue( val_id );
        else
            return lString16::empty_str;
    }
    /// returns attribute value by attribute name id
    const lString16 & getAttributeValue( lUInt16 id ) const
    {
        return getAttributeValue( LXML_NS_ANY, id );
    }
    /// sets attribute value
    virtual void setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value )
    {
        lUInt16 valueId = _document->getAttrValueIndex( value );
        _attrs.set(nsid, id, valueId);
        if (nsid == LXML_NS_NONE)
            _document->onAttributeSet( id, valueId, this );
    }
    /// move range of children startChildIndex to endChildIndex inclusively to specified element
    virtual void moveItemsTo( ldomElement * destination, int startChildIndex, int endChildIndex );
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 index ) const { return _attrs[index]; }
    /// returns attribute value by attribute name id
    const lString16 & getAttributeName( lUInt32 index ) const { return _document->getAttrName(_attrs[index]->id); }
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsid, lUInt16 id ) const { return _attrs.get( nsid, id )!=LXML_ATTR_VALUE_NONE; }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const elem_def_t * getElementTypePtr() { return _document->getElementTypePtr(_id); }
    /// returns element name id
    virtual lUInt16 getNodeId() const { return _id; }
    /// replace element name id with another value
    virtual void setNodeId( lUInt16 id ) { _id = id; }
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return _nsid; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return _document->getElementName(_id); }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return _document->getNsName(_nsid); }
    /// returns concatenation of all child node text
    virtual lString16 getText( lChar16 blockDelimiter=0 ) const;
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const
    {
        return _children[index];
    }
    /// returns render data structure
    virtual lvdomElementFormatRec * getRenderData()
    {
        if ( !_renderData )
            _renderData = new lvdomElementFormatRec;
        return _renderData;
    }
    /// sets node rendering structure pointer
    virtual void setRenderData( lvdomElementFormatRec * pRenderData )
    {
        if (_renderData)
            delete _renderData;
        _renderData = pRenderData;
    }
    /// returns node absolute rectangle
    virtual void getAbsRect( lvRect & rect );

    ldomElement * findChildElement( lUInt16 nsid, lUInt16 id, int index )
    {
        if ( !this )
            return NULL;
        ldomElement * res = NULL;
        int k = 0;
        for ( int i=0; i<_children.length(); i++ )
        {
            ldomElement * p = (ldomElement*)_children[i];
            if ( !p->isElement() )
                continue;
            if ( p->getNodeId() == id && ( (p->getNodeNsId() == nsid) || (nsid==LXML_NS_ANY) ) )
            {
                if ( k==index || index==-1 )
                    res = p;
                k++;
            }
        }
        if ( !res || (index==-1 && k>1) )
            return NULL;
        return res;
    }

    /// inserts child element
    ldomElement * insertChildElement( lUInt32 index, lUInt16 nsid, lUInt16 id )
    {
        if (index>(lUInt32)_children.length())
            index = _children.length();
        ldomElement * elem = new ldomElement( _document, this, _level+1, index, nsid, id );
        _children.insert( index, elem );
#if (LDOM_ALLOW_NODE_INDEX==1)
        // reindex tail
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
#endif
        return elem;
    }
    /// inserts child element
    ldomElement * insertChildElement( lUInt16 id )
    {
        ldomElement * elem = new ldomElement( _document, this, _level+1, _children.length(), LXML_NS_NONE, id );
        _children.add( elem );
        return elem;
    }
#if COMPACT_DOM == 1
    /// inserts text as reference to document file
    ldomTextRef * insertChildText( lUInt32 index, lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
    {
        if (index>(lUInt32)_children.length())
            index = _children.length();
        ldomTextRef * text = new ldomTextRef( this, _level+1, index, (lUInt32)fpos, (lUInt32)fsize, (lUInt16)flags );
        _children.insert( index, text );
#if (LDOM_ALLOW_NODE_INDEX==1)
        // reindex tail
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
#endif
        return text;
    }
    /// inserts text as reference to document file
    ldomTextRef * insertChildText( lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
    {
        ldomTextRef * text = new ldomTextRef( this, _level+1, (lUInt16)_children.length(), (lUInt32)fpos, (lUInt32)fsize, (lUInt16)flags );
        _children.add( text );
        return text;
    }
#endif
    /// inserts child text
    ldomText * insertChildText( lUInt32 index, lString16 value )
    {
        if (index>(lUInt32)_children.length())
            index = _children.length();
        ldomText * text = new ldomText( this, _level+1, index, value );
        _children.insert( index, text );
#if (LDOM_ALLOW_NODE_INDEX==1)
        // reindex tail
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
#endif
        return text;
    }
    /// inserts child text
    ldomText * insertChildText( lString16 value )
    {
        ldomText * text = new ldomText( this, _level+1, _children.length(), value );
        _children.add( text );
        return text;
    }
    ldomNode * removeChild( lUInt32 index )
    {
        if ( index>(lUInt32)_children.length() )
            return NULL;
        ldomNode * node = _children.remove(index);
#if (LDOM_ALLOW_NODE_INDEX==1)
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
#endif
        return node;
    }
    /// calls specified function recursively for all elements of DOM tree
    virtual void recurseElements( void (*pFun)( ldomNode * node ) )
    {
        pFun( this );
        int cnt = getChildCount();
        for (int i=0; i<cnt; i++)
        {
            ldomNode * child = getChildNode( i );
            if ( child->getNodeType()==LXML_ELEMENT_NODE )
            {
                child->recurseElements( pFun );
            }
        }
    }
    /// calls specified function recursively for all nodes of DOM tree
    virtual void recurseNodes( void (*pFun)( ldomNode * node ) )
    {
        pFun( this );
        if ( getNodeType()==LXML_ELEMENT_NODE )
        {
            int cnt = getChildCount();
            for (int i=0; i<cnt; i++)
            {
                ldomNode * child = getChildNode( i );
                child->recurseNodes( pFun );
            }
        }
    }
    /// creates stream to read base64 encoded data from element
    LVStreamRef createBase64Stream();
#if BUILD_LITE!=1
    /// returns object image source
    LVImageSourceRef getObjectImageSource();
    /// formats final block
    int renderFinalBlock(  LFormattedTextRef & frmtext, int width );
#endif
};

class ldomDocumentWriter;

class ldomElementWriter
{
    ldomElementWriter * _parent;
    ldomDocument * _document;

    ldomElement * _element;
    const elem_def_t * _typeDef;
    bool _allowText;
    lUInt32 getFlags();
    ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent);
    ldomElement * getElement()
    {
        return _element;
    }
    void onText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags );
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
    virtual void ElementCloseHandler( ldomElement * elem ) { }
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
    virtual ldomElement * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
    /// called on closing tag
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
    /// called on attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
    /// close tags
    ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
    /// called on text
    virtual void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags );
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
    virtual void ElementCloseHandler( ldomElement * elem );
public:
    /// called on attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
    /// called on opening tag
    virtual ldomElement * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
    /// called on closing tag
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
    /// called on text
    virtual void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags );
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
    virtual void OnStart(LVFileFormatParser * parser)
    {
        insideTag = false;
    }
    /// called on parsing end
    virtual void OnStop()
    {
        insideTag = false;
    }
    /// called on opening tag
    virtual ldomElement * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
    {
        ldomElement * res = NULL;
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
    virtual void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
    {
        if ( insideTag )
            parent->OnText( text, len, 0, 0, flags );
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

#endif
