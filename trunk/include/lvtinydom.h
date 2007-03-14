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

#define LXML_DOCUMENT_NODE 1 ///< document node
#define LXML_ELEMENT_NODE  2 ///< element node
#define LXML_TEXT_NODE     3 ///< text node
#define LXML_COMMENT_NODE  4 ///< comment node

#define LXML_NS_NONE 0       ///< no namespace specified
#define LXML_NS_ANY  0xFFFF  ///< any namespace can be specified
#define LXML_ATTR_VALUE_NONE  0xFFFF  ///< attribute not found

#define DOC_STRING_HASH_SIZE  256
#define RESERVED_DOC_SPACE    4096
#define MAX_ELEMENT_TYPE_ID   256
#define MAX_NAMESPACE_TYPE_ID 64
#define MAX_ATTRIBUTE_TYPE_ID 1024
#define UNKNOWN_ELEMENT_TYPE_ID   (MAX_ELEMENT_TYPE_ID>>1)
#define UNKNOWN_ATTRIBUTE_TYPE_ID (MAX_ATTRIBUTE_TYPE_ID>>1)
#define UNKNOWN_NAMESPACE_TYPE_ID (MAX_NAMESPACE_TYPE_ID>>1)

//#define LDOM_USE_OWN_MEM_MAN 0

typedef enum {
	xpath_step_error = 0, // error
	xpath_step_element,   // element of type 'name' with 'index'        /elemname[N]/
	xpath_step_text,      // text node with 'index'                     /text()[N]/
	xpath_step_nodeindex, // node index                                 /N/
	xpath_step_point      // point index                                .N
} xpath_step_t;
xpath_step_t ParseXPathStep( const lChar8 * &path, lString8 & name, int & index );



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
    virtual void gc() { _styleCache.gc(); fontMan->gc(); }

    LVStyleSheet * getStyleSheet() { return &_stylesheet; }

    inline void onAttributeSet( lUInt16 attrId, lUInt16 valueId, ldomNode * node )
    {
        if (attrId == _idAttrId)
        {
            _idNodeMap.set( valueId, node );
        }
    }

    inline ldomNode * getNodeById( lUInt16 attrValueId )
    {
        return _idNodeMap.get( attrValueId );
    }
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

/// fastDOM NODE interface
class ldomNode
{
protected:
    ldomDocument * _document;
    ldomElement * _parent;
    lUInt32 _index;
    lUInt8 _type;
    lUInt8 _level;
public:
    ldomNode( ldomDocument * document, ldomElement * parent, lUInt8 type, lUInt8 level, lUInt32 index )
    : _document(document), _parent(parent), _index(index), _type(type), _level(level) { }
    virtual ~ldomNode();
    // inline functions
    inline ldomDocument * getDocument() const { return _document; }
    inline ldomElement * getParentNode() const { return _parent; }
    inline lUInt8 getNodeType() const { return _type; }
    inline lUInt8 getNodeLevel() const { return _level; }
    /// returns node index
    inline lUInt32 getNodeIndex() const { return _index; }
    inline bool isNull() const { return this == NULL; }
    inline bool isRoot() const { return _parent == NULL; }
    inline bool isText() const { return _type == LXML_TEXT_NODE; }
    inline bool isElement() const { return _type == LXML_ELEMENT_NODE; }
    /// returns true if node is and element that has children
    inline bool hasChildren() { return getChildCount()!=0; }
    /// returns true if node is element has attributes
    inline bool hasAttributes() { return getAttrCount()!=0; }
    inline void setIndex( lUInt32 index ) { _index = index; }
    
    // virtual functions
    
    /// returns element child count
    virtual lUInt32 getChildCount() const = 0;
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const = 0;
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const = 0;
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
    virtual lString16 getText() const = 0;
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
    /// find node by coordinates of point in formatted document
    ldomElement * elementFromPoint( lvPoint pt );
    /// find final node by coordinates of point in formatted document
    ldomElement * finalBlockFromPoint( lvPoint pt );
};

class ldomDocument;

class ldomText : public ldomNode
{
private:
    lString16 _value;
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
    ldomText( ldomDocument * document, ldomElement * parent, lUInt8 level, lUInt32 index, lString16 value)
    : ldomNode( document, parent, LXML_TEXT_NODE, level, index ), _value(value)
    { }
    virtual ~ldomText() { }
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
    virtual lString16 getText() const { return _value; }
    /// sets text node text
    virtual void setText( lString16 value ) { _value = value; }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const { return NULL; }
};

/**
 * @brief XPointer/XPath object
 */
class ldomXPointer
{
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
    /// returns coordinates of pointer inside formatted document
    lvPoint toPoint() const;
    /// converts to string
	lString16 toString();
    /// returns XPath node text
    lString16 getText()
    {
        if ( !_node )
            return lString16();
        return _node->getText();
    }
};

class ldomElement;
#if COMPACT_DOM == 1
class ldomTextRef;
#endif

class ldomDocument : public lxmlDocBase
{
    friend class ldomDocumentWriter;
private:
    ldomElement * _root;
    font_ref_t _def_font; // default font
    css_style_ref_t _def_style;
    int _page_height;
    
#if COMPACT_DOM == 1
    LVXMLTextCache _textcache;
#endif

public:

#if COMPACT_DOM == 1
    ldomDocument(LVStreamRef stream);
    lString16 getTextNodeValue( const ldomTextRef * txt );
#else
    ldomDocument();
#endif

    int getPageHeight() { return _page_height; }
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
    /// renders (formats) document in memory
    virtual int render( LVRendPageContext & context, int width, font_ref_t def_font );
    /// create xpointer from pointer string
    ldomXPointer createXPointer( const lString16 & xPointerStr );
    /// create xpointer from pointer string
    ldomNode * nodeFromXPath( const lString16 & xPointerStr )
    {
        return createXPointer( xPointerStr ).getNode();
    }
    /// create xpointer from relative pointer string
    ldomXPointer createXPointer( ldomNode * baseNode, const lString16 & xPointerStr );
    /// create xpointer from doc point
    ldomXPointer createXPointer( lvPoint pt );
};

#if COMPACT_DOM == 1
class ldomTextRef : public ldomNode
{
    friend class ldomDocument;
private:


    lUInt32 dataSize;   // bytes
    lUInt32 fileOffset; // offset from beginning of stream
    lUInt32 dataFormat; // format flags
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
    ldomTextRef( ldomDocument * document, ldomElement * parent, lUInt8 level, lUInt32 index, lUInt32 pos, lUInt32 size, lUInt32 flags)
    : ldomNode( document, parent, LXML_TEXT_NODE, level, index ), dataSize(size), fileOffset(pos), dataFormat(flags)
    { }
    virtual ~ldomTextRef() { }
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
    virtual lString16 getText() const { return _document->getTextNodeValue(this); }
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
    : ldomNode( document, parent, LXML_ELEMENT_NODE, level, index ), _id(id), _nsid(nsid), _renderData(NULL)
    { }
    virtual ~ldomElement() { if (_renderData) delete _renderData; }
    /// returns element child count
    virtual lUInt32 getChildCount() const { return _children.length(); }
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
    /// sets attribute value
    virtual void setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value )
    {
        lUInt16 valueId = _document->getAttrValueIndex( value );
        _attrs.set(nsid, id, valueId);
        if (nsid == LXML_NS_NONE)
            _document->onAttributeSet( id, valueId, this );
    }
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
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return _nsid; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return _document->getElementName(_id); }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return _document->getNsName(_nsid); }
    /// returns concatenation of all child node text
    virtual lString16 getText() const;
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const
    {
        return _children[index];
    }
    /// returns render data structure
    virtual lvdomElementFormatRec * getRenderData()
    {   
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
        // reindex tail
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
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
        ldomTextRef * text = new ldomTextRef( _document, this, _level+1, index, (lUInt32)fpos, (lUInt32)fsize, flags );
        _children.insert( index, text );
        // reindex tail
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
        return text;
    }
    /// inserts text as reference to document file
    ldomTextRef * insertChildText( lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
    {
        ldomTextRef * text = new ldomTextRef( _document, this, _level+1, _children.length(), (lUInt32)fpos, (lUInt32)fsize, flags );
        _children.add( text );
        return text;
    }
#endif
    /// inserts child text
    ldomText * insertChildText( lUInt32 index, lString16 value )
    {
        if (index>(lUInt32)_children.length())
            index = _children.length();
        ldomText * text = new ldomText( _document, this, _level+1, index, value );
        _children.insert( index, text );
        // reindex tail
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
        return text;
    }
    /// inserts child text
    ldomText * insertChildText( lString16 value )
    {
        ldomText * text = new ldomText( _document, this, _level+1, _children.length(), value );
        _children.add( text );
        return text;
    }
    ldomNode * removeChild( lUInt32 index )
    {
        if ( index>(lUInt32)_children.length() )
            return NULL;
        ldomNode * node = _children.remove(index);
        for (int i=index; i<_children.length(); i++)
            _children[i]->setIndex( i );
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
    /// returns object image source
    LVImageSourceRef getObjectImageSource();
    /// formats final block
    int renderFinalBlock( LFormattedText & txtform, int width );
};

class ldomElementWriter
{
    ldomElementWriter * _parent;
    ldomDocument * _document;

    ldomElement * _element;
    const elem_def_t * _typeDef;
    bool _allowText;
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
    friend ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
};

/** \brief callback object to fill DOM tree

    To be used with XML parser as callback object.

    Creates document according to incoming events.
*/
class ldomDocumentWriter : public LVXMLParserCallback
{
private:
    //============================
    ldomDocument * _document;
    //ldomElement * _currNode;
    ldomElementWriter * _currNode;
    bool _errFlag;
    //============================
    lUInt32 _flags;
public:
    /// returns flags
    virtual lUInt32 getFlags() { return _flags; }
    /// sets flags
    virtual void setFlags( lUInt32 flags ) { _flags = flags; }
    // overrides
    /// called when encoding directive found in document
    virtual void OnEncoding( const lChar16 * name, const lChar16 * table );
    /// called on parsing start
    virtual void OnStart(LVXMLParser * parser);
    /// called on parsing end
    virtual void OnStop();
    /// called on opening tag 
    virtual void OnTagOpen( const lChar16 * nsname, const lChar16 * tagname );
    /// called on closing tag 
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
    /// called on attribute 
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );
    /// called on text 
    virtual void OnText( const lChar16 * text, int len, 
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags );
    /// constructor
    ldomDocumentWriter(ldomDocument * document);
};



#endif
