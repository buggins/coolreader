/*******************************************************

   CoolReader Engine

   lvtinydom.cpp: fast and compact XML DOM tree

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <string.h>
#include "../include/lvstring.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvrend.h"
#include <stddef.h>

//#define INDEX1 94
//#define INDEX2 96

//#define INDEX1 105
//#define INDEX2 106

#if BUILD_LITE!=1
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
    lvdomElementFormatRec renderData; // 4 * 4
    lUInt16 fontIndex;
    lUInt16 styleIndex;
    lInt32  children[1];
    lUInt16 * attrs() { return (lUInt16 *)(children + childCount); }
    lxmlAttribute * attr( int index ) { return (lxmlAttribute *)&(((lUInt16 *)(children + childCount))[index*3]); }
    // TODO: add items here
    //css_style_ref_t _style;
    //font_ref_t      _font;
};

class DataBuffer {
private:
    int _size;
    int _len;
    lUInt8 * _data;
    bool _own;
public :
    // return first non-empty item, NULL if no items found
    DataStorageItemHeader * first()
    {
        if ( _len==0 )
            return NULL;
        DataStorageItemHeader * item = (DataStorageItemHeader *)_data;
        if ( item->type == LXML_NO_DATA )
            item = next( item );
        return item;
    }
    // return next non-empty item, NULL if end of collection reached
    DataStorageItemHeader * next( DataStorageItemHeader * item )
    {
        if ( item==NULL )
            return NULL;
        for ( ;; ) {
            if ( !item->sizeDiv16 ) {
                CRLog::error("Zero size block at offset %d, data len=%d, total reserved size=%d", (int)(((lUInt8*)item) - _data), _len, _size );
                return NULL;
            }
            item = (DataStorageItemHeader*)(((lUInt8*)item) + ((lUInt32)item->sizeDiv16 * 16));
            if ( (lUInt8*)item >= _data + _len )
                return NULL;
            if ( item->type != LXML_NO_DATA )
                return item;
            //CRLog::trace("skipping no_data item of size %d at offset %x (dataIndex=%d)", item->sizeDiv16*16, (int)((lUInt8*)item - _data), item->dataIndex );
        }

    }
    bool isNull()
    {
        return _data==NULL;
    }
    DataBuffer( int size )
        : _size( size ), _len( 0 ), _own(true)
    {
        //_data = (lUInt8*)calloc( size, sizeof(lUInt8) );
        _data = (lUInt8*)malloc( size );
    memset( _data, 0, size );
    }
    DataBuffer( lUInt8 * data, int size, int len )
        : _size( size ), _len( len ), _data(data), _own(false)
    {
    }
    ~DataBuffer()
    {
        if ( _own )
            free( _data );
    }
    DataStorageItemHeader * alloc( int size );
    lUInt8 * ptr() { return _data; }
    int length() { return _len; }
    void relocatePtr( ptrdiff_t addrDiff, int newSize )
    {
        _data += addrDiff;
        _size = newSize;
    }
};

DataStorageItemHeader * DataBuffer::alloc( int size )
{
    if ( _len + size > _size )
        return NULL; // no room
    size = (size + 15) & 0xFFFFFF0;
    DataStorageItemHeader * item = (DataStorageItemHeader *) (_data + _len);
    item->sizeDiv16 = size >> 4;
    _len += size;
    return item;
}
#endif


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
    void add( lUInt16 nsId, lUInt16 attrId, lUInt16 valueIndex )
    {
        // find existing
        if (_len>=_size)
        {
            _size += 4;
            _list = (lxmlAttribute*) realloc( _list, _size*sizeof(lxmlAttribute) );
        }
        _list[ _len++ ].setData(nsId, attrId, valueIndex);
    }
    void add( const lxmlAttribute * v )
    {
        // find existing
        if (_len>=_size)
        {
            _size += 4;
            _list = (lxmlAttribute*) realloc( _list, _size*sizeof(lxmlAttribute) );
        }
        _list[ _len++ ] = *v;
    }
};

// non-persistent r/w instance of text
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
    void * operator new( size_t )
    {
        if (pmsHeap == NULL)
        {
            pmsHeap = new ldomMemManStorage(sizeof(ldomText));
        }
        return pmsHeap->alloc();
    }
    void operator delete( void * p )
    {
        pmsHeap->free((ldomMemBlock *)p);
    }
#endif
    ldomText( ldomNode * parent, lUInt32 index, lString16 value )
    : ldomNode( parent->getDocument(), parent, index )
    {
#if (USE_DOM_UTF8_STORAGE==1)
        _value = UnicodeToUtf8(value);
#else
        _value = value;
#endif
    }
    ldomText( ldomNode * parent, lUInt32 index, lString8 value )
    : ldomNode( parent->getDocument(), parent, index )
    {
#if (USE_DOM_UTF8_STORAGE==1)
        _value = value;
#else
        _value = Utf8ToUnicode(value);
#endif
    }
#if BUILD_LITE!=1
    ldomText( ldomPersistentText * v );
#endif
    virtual ~ldomText()
    {
        _document->unregisterNode( this );
    }
    /// returns LXML_TEXT_NODE for text node
    virtual lUInt8 getNodeType() const { return LXML_TEXT_NODE; }
    /// returns element child count
    virtual lUInt32 getChildCount() const { return 0; }
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return 0; }
    /// returns attribute value by attribute namespace id and name id
    virtual const lString16 & getAttributeValue( lUInt16, lUInt16 ) const
    {
        return lString16::empty_str;
    }
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 ) const { return NULL; }
    /// returns true if element node has attribute with specified namespace id and name id
    virtual bool hasAttribute( lUInt16, lUInt16 ) const { return false; }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const css_elem_def_props_t * getElementTypePtr() { return NULL; }
    /// returns element name id
    virtual lUInt16 getNodeId() const { return 0; }
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return 0; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return lString16::empty_str; }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return lString16::empty_str; }
    /// returns text node text
    virtual lString16 getText( lChar16 ) const
    {
#if (USE_DOM_UTF8_STORAGE==1)
        return Utf8ToUnicode(_value);
#else
        return _value;
#endif
    }
    virtual lString8 getText8( lChar8 = 0 ) const
    {
#if (USE_DOM_UTF8_STORAGE==1)
        return _value;
#else
        return UnicodeToUtf8(_value);
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
    /// sets text node text
    virtual void setText8( lString8 value )
    {
#if (USE_DOM_UTF8_STORAGE==1)
        _value = value;
#else
        _value = Utf8ToUnicode(value);
#endif
    }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 ) const { return NULL; }
#if BUILD_LITE!=1
    /// replace node with r/o persistent implementation
    virtual ldomNode * persist();
#endif

    // stubs

    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt32, lUInt16, lUInt16 ) { return NULL; }
    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt16 ) { return NULL; }
    /// inserts child text
    virtual ldomNode * insertChildText( lUInt32, lString16 ) { return NULL; }
    /// inserts child text
    virtual ldomNode * insertChildText( lString16 ) { return NULL; }
    /// remove child
    virtual ldomNode * removeChild( lUInt32 ) { return NULL; }
};

#if BUILD_LITE!=1
// persistent r/o instance of text
class ldomPersistentText : public ldomNode
{
    friend class ldomDocument;

    /// call to initiate fatal error due to modification of read only data
    void readOnlyError()
    {
        crFatalError( 124, "Text node is persistent (read-only)! Call modify() to get r/w instance." );
    }
public:
#if (LDOM_USE_OWN_MEM_MAN == 1)
    static ldomMemManStorage * pmsHeap;
    void * operator new( size_t )
    {
        if (pmsHeap == NULL)
        {
            pmsHeap = new ldomMemManStorage(sizeof(ldomPersistentText));
        }
        return pmsHeap->alloc();
    }
    void operator delete( void * p )
    {
        pmsHeap->free((ldomMemBlock *)p);
    }
#endif
    ldomPersistentText( ldomDocument * document, TextDataStorageItem * data )
        : ldomNode( document, data->parentIndex, data->dataIndex )
    {
        _document->setNode( data->dataIndex, this, data );
    }
    ldomPersistentText( ldomNode * parent, lUInt32 index, lString16 value );
    ldomPersistentText( ldomNode * parent, lUInt32 index, lString8 value );
    // create persistent r/o copy of r/w text node
    ldomPersistentText( ldomText * v );
    virtual ~ldomPersistentText()
    {
        _document->deleteNode( this );
    }
    /// returns true if node is stored in persistent storage
    virtual bool isPersistent() { return true; }
    virtual lUInt8 getNodeType() const { return LXML_TEXT_NODE; }
    /// returns element child count
    virtual lUInt32 getChildCount() const { return 0; }
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return 0; }
    /// returns attribute value by attribute namespace id and name id
    virtual const lString16 & getAttributeValue( lUInt16, lUInt16 ) const
    {
        return lString16::empty_str;
    }
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32) const { return NULL; }
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16, lUInt16 ) const { return false; }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const css_elem_def_props_t * getElementTypePtr() { return NULL; }
    /// returns element name id
    virtual lUInt16 getNodeId() const { return 0; }
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return 0; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return lString16::empty_str; }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return lString16::empty_str; }
    /// returns text node text
    virtual lString16 getText( lChar16 blockDelimiter=0 ) const;
    /// returns text node text
    virtual lString8 getText8( lChar8 blockDelimiter=0 ) const;
    /// sets text node text
    virtual void setText( lString16 ) { readOnlyError(); }
    /// sets text node text
    virtual void setText8( lString8 ) { readOnlyError(); }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 ) const { return NULL; }
    /// replace text node with r/w implementation
    virtual ldomNode * modify();

    // stubs

    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt32, lUInt16, lUInt16 ) { return NULL; }
    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt16 ) { return NULL; }
    /// inserts child text
    virtual ldomNode * insertChildText( lUInt32, lString16 ) { return NULL; }
    /// inserts child text
    virtual ldomNode * insertChildText( lString16 ) { return NULL; }
    /// remove child
    virtual ldomNode * removeChild( lUInt32 ) { return NULL; }
};
#endif


// ldomElement declaration placed here to hide DOM implementation
// use ldomNode rich interface instead
class ldomElement : public ldomNode
{
    friend class ldomPersistentElement;
private:
    ldomAttributeCollection _attrs;
    lUInt16 _id;
    lUInt16 _nsid;
    lvdomElementFormatRec * _renderData;   // used by rendering engine
    LVArray < lInt32 > _children;
    css_style_ref_t _style;
    font_ref_t      _font;
    lvdom_element_render_method _rendMethod;
protected:
    virtual void addChild( lInt32 dataIndex );
public:
#if (LDOM_USE_OWN_MEM_MAN == 1)
    static ldomMemManStorage * pmsHeap;
    void * operator new( size_t )
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
#if BUILD_LITE!=1
    ldomElement( ldomPersistentElement * v );
#endif
    ldomElement( ldomDocument * document, ldomNode * parent, lUInt32 index, lUInt16 nsid, lUInt16 id )
    : ldomNode( document, parent, index ), _id(id), _nsid(nsid), _renderData(NULL), _rendMethod(erm_invisible)
    { }
    /// destructor
    virtual ~ldomElement();
    /// returns LXML_ELEMENT_NODE
    virtual lUInt8 getNodeType() const { return LXML_ELEMENT_NODE; }
    /// returns rendering method
    virtual lvdom_element_render_method  getRendMethod() { return _rendMethod; }
    /// sets rendering method
    virtual void setRendMethod( lvdom_element_render_method  method ) { _rendMethod=method; }
    /// returns element style record
    virtual css_style_ref_t getStyle() { return _style; }
    /// returns element font
    virtual font_ref_t getFont() { return _font; }
    /// sets element font
    virtual void setFont( font_ref_t font ) { _font = font; }
    /// sets element style record
    virtual void setStyle( css_style_ref_t & style ) { _style = style; }
    /// returns element child count
    virtual lUInt32 getChildCount() const { return _children.length(); }
    /// returns first child node
    virtual ldomNode * getFirstChild() const;
    /// returns last child node
    virtual ldomNode * getLastChild() const;
    /// removes and deletes last child element
    virtual void removeLastChild();
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return _attrs.length(); }
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const;
    /// sets attribute value
    virtual void setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value );
    /// move range of children startChildIndex to endChildIndex inclusively to specified element
    virtual void moveItemsTo( ldomNode * destination, int startChildIndex, int endChildIndex );
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 index ) const { return _attrs[index]; }
    /// returns attribute value by attribute name id
    const lString16 & getAttributeName( lUInt32 index ) const { return _document->getAttrName(_attrs[index]->id); }
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsid, lUInt16 id ) const { return _attrs.get( nsid, id )!=LXML_ATTR_VALUE_NONE; }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const css_elem_def_props_t * getElementTypePtr() { return _document->getElementTypePtr(_id); }
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
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const;
    /// returns render data structure
    virtual lvdomElementFormatRec * getRenderData();
    /// sets node rendering structure pointer
    virtual void clearRenderData();

    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt32 index, lUInt16 nsid, lUInt16 id );
    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt16 id );
    /// inserts child text
    virtual ldomNode * insertChildText( lUInt32 index, lString16 value );
    /// inserts child text
    virtual ldomNode * insertChildText( lString16 value );
    /// remove child
    virtual ldomNode * removeChild( lUInt32 index );
#if BUILD_LITE!=1
    /// replace node with r/o persistent implementation
    virtual ldomNode * persist();
#endif
protected:
    /// override to avoid deleting children while replacing
    virtual void prepareReplace()
    {
        _children.clear();
    }
};


#if BUILD_LITE!=1

// ldomElement declaration placed here to hide DOM implementation
// use ldomNode rich interface instead
class ldomPersistentElement : public ldomNode
{
private:
    css_style_ref_t _style;
    font_ref_t      _font;
protected:

    inline ElementDataStorageItem * getData() const { return _document->getElementNodeData( _dataIndex ); }

    /// call to initiate fatal error due to modification of read only data
    void readOnlyError()
    {
        crFatalError( 123, "Element is persistent (read-only)! Call modify() to get r/w instance." );
    }
    virtual void addChild( lInt32 ) { readOnlyError(); }
public:
#if (LDOM_USE_OWN_MEM_MAN == 1)
    static ldomMemManStorage * pmsHeap;
    void * operator new( size_t )
    {
        if (pmsHeap == NULL)
        {
            pmsHeap = new ldomMemManStorage(sizeof(ldomPersistentElement));
        }
        return pmsHeap->alloc();
    }
    void operator delete( void * p )
    {
        pmsHeap->free((ldomMemBlock *)p);
    }
#endif

    ldomPersistentElement( ldomDocument * document, ElementDataStorageItem * data )
        : ldomNode( document, data->parentIndex, data->dataIndex )
    {
        _document->setNode( data->dataIndex, this, data );
    }

    ldomPersistentElement( ldomElement * v )
    : ldomNode( v )
    {
        int attrCount = v->getAttrCount();
        int childCount = v->getChildCount();
        ElementDataStorageItem * data = _document->allocElement( _dataIndex, _parentIndex, attrCount, childCount );
        data->nsid = v->_nsid;
        data->id = v->_id;
        lUInt16 * attrs = data->attrs();
        int i;
        for ( i=0; i<attrCount; i++ ) {
            const lxmlAttribute * attr = v->getAttribute(i);
            attrs[i * 3] = attr->nsid;     // namespace
            attrs[i * 3 + 1] = attr->id;   // id
            attrs[i * 3 + 2] = attr->index;// value
        }
        for ( i=0; i<childCount; i++ ) {
            data->children[i] = v->_children[i];
        }
        data->rendMethod = (lUInt8)v->_rendMethod;

        data->styleIndex = 0; // todo
        data->fontIndex = 0;  // todo

        lvdomElementFormatRec * rdata = v->getRenderData();
        data->renderData = *rdata;
        _style = v->_style;
        _font = v->_font;
        _document->replaceInstance( _dataIndex, this );
        //#ifdef _DEBUG
        //    _document->checkConsistency();
        //#endif

    }

    /// destructor
    virtual ~ldomPersistentElement()
    {
        _document->deleteNode( this );
    }
    /// returns true if node is stored in persistent storage
    virtual bool isPersistent() { return true; }

    /// returns LXML_ELEMENT_NODE
    virtual lUInt8 getNodeType() const { return LXML_ELEMENT_NODE; }
    /// returns rendering method
    virtual lvdom_element_render_method  getRendMethod() { return (lvdom_element_render_method)getData()->rendMethod; }
    /// sets rendering method
    virtual void setRendMethod( lvdom_element_render_method  method ) { getData()->rendMethod = (lUInt8)method; }
    /// returns element style record
    virtual css_style_ref_t getStyle() { return _style; }
    /// returns element font
    virtual font_ref_t getFont() { return _font; }
    /// sets element font
    virtual void setFont( font_ref_t font ) { _font = font; }
    /// sets element style record
    virtual void setStyle( css_style_ref_t & style ) { _style = style; }
    /// returns element child count
    virtual lUInt32 getChildCount() const { return getData()->childCount; }
    /// returns first child node
    virtual ldomNode * getFirstChild() const
    {
        ElementDataStorageItem * data = getData();
        if ( data->childCount <=0 )
            return NULL;
        return _document->getNodeInstance( data->children[0] );
    }

    /// returns last child node
    virtual ldomNode * getLastChild() const
    {
        ElementDataStorageItem * data = getData();
        if ( data->childCount <=0 )
            return NULL;
        return _document->getNodeInstance( data->children[data->childCount-1] );
    }
    /// removes and deletes last child element
    virtual void removeLastChild() { readOnlyError(); }
    /// returns element attribute count
    virtual lUInt32 getAttrCount() const { return getData()->attrCount; }
    /// returns attribute value by attribute name id and namespace id
    virtual const lString16 & getAttributeValue( lUInt16 nsid, lUInt16 id ) const
    {
        ElementDataStorageItem * data = getData();
        int attrCount = data->attrCount;
        lUInt16 * attrs = data->attrs();
        for ( int i=0; i<attrCount; i++ ) {
            lxmlAttribute * attr = (lxmlAttribute*)(&(attrs[i*3]));
            if ( !attr->compare( nsid, id ) )
                continue;
            lUInt16 val_id = attr->index;
            if (val_id != LXML_ATTR_VALUE_NONE)
                return _document->getAttrValue( val_id );
            else
                return lString16::empty_str;
        }
        return lString16::empty_str;
    }
    /// sets attribute value
    virtual void setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value )
    {
        lUInt16 valueId = _document->getAttrValueIndex( value );
        ElementDataStorageItem * data = getData();
        int attrCount = data->attrCount;
        lUInt16 * attrs = data->attrs();
        for ( int i=0; i<attrCount; i++ ) {
            lxmlAttribute * attr = (lxmlAttribute*)(&(attrs[i*3]));
            if ( attr->compare( nsid, id ) )
                continue;
            attr->index = valueId;
            if ( nsid == LXML_NS_NONE )
                _document->onAttributeSet( id, valueId, this );
        }
        // in persistent mode only modification of existing attribute allowed
        readOnlyError();
    }
    /// move range of children startChildIndex to endChildIndex inclusively to specified element
    virtual void moveItemsTo( ldomNode *, int, int ) { readOnlyError(); }
    /// returns attribute by index
    virtual const lxmlAttribute * getAttribute( lUInt32 index ) const
    {
        ElementDataStorageItem * data = getData();
        lUInt16 attrCount = data->attrCount;
        if ( index > attrCount )
            return NULL;
        return data->attr( index );
    }
    /// returns attribute value by attribute name id
    virtual const lString16 & getAttributeName( lUInt32 index ) const
    {
        const lxmlAttribute * attr = getAttribute( index );
        if ( attr!=NULL )
            return _document->getAttrName( attr->id );
        return lString16::empty_str;
    }
    /// returns true if element node has attribute with specified name id and namespace id
    virtual bool hasAttribute( lUInt16 nsid, lUInt16 id ) const
    {
        ElementDataStorageItem * data = getData();
        int attrCount = data->attrCount;
        lUInt16 * attrs = data->attrs();
        for ( int i=0; i<attrCount; i++ ) {
            lxmlAttribute * attr = (lxmlAttribute*)(&(attrs[i*3]));
            if ( attr->compare( nsid, id ) )
                continue;
            return true;
        }
        return false;
    }
    /// returns element type structure pointer if it was set in document for this element name
    virtual const css_elem_def_props_t * getElementTypePtr() { return _document->getElementTypePtr(getNodeId()); }
    /// returns element name id
    virtual lUInt16 getNodeId() const { return getData()->id; }
    /// replace element name id with another value
    virtual void setNodeId( lUInt16 id ) { getData()->id = id; }
    /// returns element namespace id
    virtual lUInt16 getNodeNsId() const { return getData()->nsid; }
    /// returns element name
    virtual const lString16 & getNodeName() const { return _document->getElementName(getData()->id); }
    /// returns element namespace name
    virtual const lString16 & getNodeNsName() const { return _document->getNsName(getData()->nsid); }
    /// returns child node by index
    virtual ldomNode * getChildNode( lUInt32 index ) const
    {
        ElementDataStorageItem * data = getData();
        return _document->getNodeInstance( data->children[index] );
    }
    /// returns render data structure
    virtual lvdomElementFormatRec * getRenderData()
    {
        return &getData()->renderData;
    }

    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt32, lUInt16, lUInt16 ) { readOnlyError(); return NULL; }
    /// inserts child element
    virtual ldomNode * insertChildElement( lUInt16 ) { readOnlyError(); return NULL; }
    /// inserts child text
    virtual ldomNode * insertChildText( lUInt32, lString16 ) { readOnlyError(); return NULL; }
    /// inserts child text
    virtual ldomNode * insertChildText( lString16 ) { readOnlyError(); return NULL; }
    /// remove child
    virtual ldomNode * removeChild( lUInt32 ) { readOnlyError(); return NULL; }
    /// replace node with r/w implementation
    virtual ldomNode * modify();
protected:
    /// override to avoid deleting children while replacing
    virtual void prepareReplace()
    {
        getData()->childCount = 0;
    }
};
#endif

#if BUILD_LITE!=1
ldomText::ldomText( ldomPersistentText * v )
: ldomNode( v )
{
#if (USE_DOM_UTF8_STORAGE==1)
    _value = v->getText8();
#else
    _value =  v->getText();
#endif
    _document->replaceInstance( _dataIndex, this );
}


ldomElement::ldomElement( ldomPersistentElement * v )
: ldomNode( v ), _id( v->getNodeId() ), _nsid( v->getNodeNsId() ), _renderData(NULL), _rendMethod(erm_invisible)
{
    int attrCount = v->getAttrCount();
    int childCount = v->getChildCount();
    int i;
    for ( i=0; i<attrCount; i++ )
        _attrs.add( v->getAttribute( i ) );
    for ( i=0; i<childCount; i++ )
        _children.add( v->getChildNode( i )->getDataIndex() );
    _style = v->getStyle();
    _font = v->getFont();
    _rendMethod = v->getRendMethod();
    memcpy( getRenderData(), v->getRenderData(), sizeof(lvdomElementFormatRec) );
    _document->replaceInstance( _dataIndex, this );
}

/// replace node with r/o persistent implementation
ldomNode * ldomElement::persist()
{
    //ldomDocument * doc = _document;
    //if ( !doc->checkConsistency(false) )
    //    CRLog::trace( "check failed before elem:persist()" );
    ldomNode * res = new ldomPersistentElement( this );
    //if ( !doc->checkConsistency(false) )
    //    CRLog::trace( "check failed after elem:persist()" );
    return res;
}

/// replace node with r/w implementation
ldomNode * ldomPersistentElement::modify()
{
    //ldomDocument * doc = _document;
    //if ( !doc->checkConsistency(false) )
    //    CRLog::trace( "check failed before elem:modify()" );
    ldomNode * res = new ldomElement( this );
    //if ( !doc->checkConsistency(false) )
    //    CRLog::trace( "check failed after elem:modify()" );
    return res;
}

/// replace node with r/o persistent implementation
ldomNode * ldomText::persist()
{
    return new ldomPersistentText( this );
}

/// replace node with r/w implementation
ldomNode * ldomPersistentText::modify()
{
    return new ldomText( this );
}
#endif


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


lxmlDocBase::lxmlDocBase( int dataBufSize )
:
#if BUILD_LITE!=1
  _dataBufferSize( dataBufSize ) // single data buffer size
, 
#endif
_instanceMap(NULL)
,_instanceMapSize(2048) // *8 = 16K
,_instanceMapCount(1)
,_elementNameTable(MAX_ELEMENT_TYPE_ID)
, _attrNameTable(MAX_ATTRIBUTE_TYPE_ID)
, _nsNameTable(MAX_NAMESPACE_TYPE_ID)
, _nextUnknownElementId(UNKNOWN_ELEMENT_TYPE_ID)
, _nextUnknownAttrId(UNKNOWN_ATTRIBUTE_TYPE_ID)
, _nextUnknownNsId(UNKNOWN_NAMESPACE_TYPE_ID)
, _attrValueTable( DOC_STRING_HASH_SIZE )
,_idNodeMap(1024)
,_idAttrId(0)
,_docProps(LVCreatePropsContainer())
#if BUILD_LITE!=1
,_keepData(false)
,_mapped(false)
#endif
,_docFlags(DOC_FLAG_DEFAULTS)
#if BUILD_LITE!=1
,_pagesData(8192)
#endif
{
    // create and add one data buffer
#if BUILD_LITE!=1
    _currentBuffer = new DataBuffer( _dataBufferSize );
    _dataBuffers.add( _currentBuffer );
#endif
    _instanceMap = (NodeItem *)malloc( sizeof(NodeItem) * _instanceMapSize );
    memset( _instanceMap, 0, sizeof(NodeItem) * _instanceMapSize );
    _stylesheet.setDocument( this );
}

/// Destructor
lxmlDocBase::~lxmlDocBase()
{
    for ( int i=0; i<_instanceMapCount; i++ ) {
        if ( _instanceMap[i].instance != NULL ) {
            delete _instanceMap[i].instance;
        }
    }
    free( _instanceMap );
}

void lxmlDocBase::onAttributeSet( lUInt16 attrId, lUInt16 valueId, ldomNode * node )
{
    if ( _idAttrId==0 )
        _idAttrId = _attrNameTable.idByName("id");
    if (attrId == _idAttrId)
    {
        _idNodeMap.set( valueId, node->getDataIndex() );
    }
}

#if BUILD_LITE!=1
/// put all object into persistent storage
void lxmlDocBase::persist()
{
    //CRLog::info("lxmlDocBase::persist() invoked - converting all nodes to persistent objects");
//#ifdef _DEBUG
    //if ( !checkConsistency(false) ) {
    //    CRLog::error( "- before lxmlDocBase::persist()" );
    //}
//#endif
    for ( int i=0; i<_instanceMapCount; i++ ) {
        ldomNode * old = _instanceMap[ i ].instance;
        if ( old ) {
            //_instanceMap[ i ].instance = 
            if ( old->persist() != old ) {
                //CRLog::trace("Item %d converted to persistent", i);
            }
        }
    }
#ifdef _DEBUG
    if ( !checkConsistency(true) ) {
        CRLog::error( "- after lxmlDocBase::persist()" );
    }
#endif
}
#endif

/// used by object constructor, to assign ID for created object
lInt32 lxmlDocBase::registerNode( ldomNode * node )
{
    //if ( node->getDataIndex()==INDEX2 || node->getDataIndex()==INDEX1 ) {
    //    CRLog::trace("register node %d", node->getDataIndex() );
   // }
    if ( _instanceMapCount >= _instanceMapSize ) {
        // resize
        _instanceMapSize = (_instanceMapSize < 1024) ? 1024 : (_instanceMapSize * 2); // 16K
        _instanceMap = (NodeItem *)realloc( _instanceMap, sizeof(NodeItem) * _instanceMapSize );
        memset( _instanceMap + _instanceMapCount, 0, sizeof(NodeItem) * (_instanceMapSize-_instanceMapCount) );
    }
    _instanceMap[_instanceMapCount].instance = node;
    return _instanceMapCount++;
}

/// used by object destructor, to remove RAM reference; leave data as is
void lxmlDocBase::unregisterNode( ldomNode * node )
{
    //if ( node->getDataIndex()==INDEX2 || node->getDataIndex()==INDEX1 ) {
    //    CRLog::trace("unregister node %d", node->getDataIndex() );
    //}
    lInt32 dataIndex = node->getDataIndex(); 
    NodeItem * p = &_instanceMap[ dataIndex ];
    if ( p->instance == node ) {
        p->instance = NULL;
    }
}

#if BUILD_LITE!=1
/// used to create instances from mmapped file
ldomNode * lxmlDocBase::setNode( lInt32 dataIndex, ldomNode * instance, DataStorageItemHeader * data )
{
    //if ( dataIndex==INDEX2 || dataIndex==INDEX1) {
    //    CRLog::trace("set node %d", dataIndex);
    //}
    NodeItem * p = &_instanceMap[ dataIndex ];
    if ( p->instance ) {
        CRLog::error( "lxmlDocBase::setNode() - Node %d already has instance (%d)", dataIndex, p->instance->getDataIndex() );
        delete p->instance;
    }
    if ( p->data ) {
        CRLog::error( "lxmlDocBase::setNode() - Node %d already has data", dataIndex );
        p->data->type = LXML_NO_DATA;
    }

    p->instance = instance;
    p->data = data;
    return instance;
}

/// used by persistance management constructors, to replace one instance with another
ldomNode * lxmlDocBase::replaceInstance( lInt32 dataIndex, ldomNode * newInstance )
{
    //if ( dataIndex==INDEX2 || dataIndex==INDEX1) {
    //    CRLog::trace("replace instance %d", dataIndex);
    //}
    NodeItem * p = &_instanceMap[ dataIndex ];
    if ( p->instance && p->instance!=newInstance ) {
        p->instance->prepareReplace();
        delete p->instance;
    }
    /*
    if ( p->data )
        p->data->type = LXML_NO_DATA;

    p->data = data;
    */
    //if ( p->instance )
    //    CRLog::error("removed instance still visible");
    p->instance = newInstance;
    return newInstance;
}

/// used by object destructor, to remove RAM reference and data block
void lxmlDocBase::deleteNode( ldomNode * node )
{
    //if ( node->getDataIndex()==INDEX2 || node->getDataIndex()==INDEX1 ) {
    //    CRLog::trace("delete node %d", node->getDataIndex() );
    //}
    lInt32 dataIndex = node->getDataIndex(); 
    NodeItem * p = &_instanceMap[ dataIndex ];
    if ( p->instance == node ) {
        p->instance = NULL;
        if ( !_keepData && p->data) {
            p->data->type  = LXML_NO_DATA;
            p->data = NULL;
        }
    }
}
#endif

/// returns or creates object instance by index
/*
ldomNode * lxmlDocBase::getNodeInstance( lInt32 dataIndex )
{
    ldomNode * item = _instanceMap[ dataIndex ].instance;
    if ( item != NULL )
        return item;
    // TODO: try to create instance from data
    CRLog::error("NULL instance for index %d", dataIndex);
    return NULL;
}
*/

lUInt16 lxmlDocBase::getNsNameIndex( const lChar16 * name )
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

lUInt16 lxmlDocBase::getElementNameIndex( const lChar16 * name )
{
    const LDOMNameIdMapItem * item = _elementNameTable.findItem( name );
    if (item)
        return item->id;
    _elementNameTable.AddItem( _nextUnknownElementId, lString16(name), NULL );
    return _nextUnknownElementId++;
}


#if BUILD_LITE!=1
/// allocate data block, return pointer to allocated block
DataStorageItemHeader * lxmlDocBase::allocData( lInt32 dataIndex, int size )
{
    //if ( dataIndex==INDEX2 || dataIndex==INDEX1) {
    //    CRLog::trace("data for Node %d is allocated", dataIndex);
    //}
    if ( _instanceMap[dataIndex].data != NULL ) {
        // mark data record as empty
        CRLog::warn( "Node with id=%d data is overwritten", dataIndex );
        _instanceMap[dataIndex].data->type = LXML_NO_DATA;
    }
    DataStorageItemHeader * item = _currentBuffer->alloc( size );
    if ( !item ) {
        if ( size >= _dataBufferSize )
            return NULL;
        if ( !_map.isNull() ) {
            if ( !resizeMap( hdr.data_offset + _dataBufferSize + _dataBufferSize / 4 +  size ) ) {
                // already has map file
                CRLog::error( "Too small swap file is reserved" );
                crFatalError(10, "Swap file is too small. Cannot allocate additional data. Exiting.");
            }
            item = _currentBuffer->alloc( size );
        } else {

            lUInt32 nsz = _dataBufferSize * (_dataBuffers.length()+1);
            if ( nsz > DOCUMENT_CACHING_MAX_RAM_USAGE ) {
                // swap to file
                lUInt32 sz = getProps()->getIntDef( DOC_PROP_FILE_SIZE, 0 ) + 0x8000;
                CRLog::info("Document data size is too big for RAM: swapping to disk, need to swap before allocating item %d[%d]", dataIndex, size);
                if ( nsz > sz )
                    sz = nsz;
#if BUILD_LITE!=1
                if ( !swapToCache( sz ) ) {
#endif
                    CRLog::error( "Cannot swap big document to disk" );
                    crFatalError(10, "Swapping big document is failed. Exiting.");
#if BUILD_LITE!=1
                }
                item = _currentBuffer->alloc( size );
#endif
            } else {
                // add one another buffer in RAM
                _currentBuffer = new DataBuffer( _dataBufferSize );
                if ( _currentBuffer->isNull() ) {
                    CRLog::error("Cannot create document data buffer #%d (size=%d)", _dataBuffers.length(), _dataBufferSize );
                    delete _currentBuffer;
                    return NULL; // OUT OF MEMORY
                }
                _dataBuffers.add( _currentBuffer );
                item = _currentBuffer->alloc( size );
            }
        }
    }
    item->dataIndex = dataIndex;
    //item->parentIndex = 0;
    //item->type = nodeType;
    _instanceMap[dataIndex].data = item;
    return item;
}

/// allocate text block
TextDataStorageItem * lxmlDocBase::allocText( lInt32 dataIndex, lInt32 parentIndex, const lChar8 * text, int charCount )
{
    int size = sizeof(TextDataStorageItem) + charCount - 2;
    TextDataStorageItem *item = (TextDataStorageItem *)allocData( dataIndex, size );
    if ( item ) {
        item->type = LXML_TEXT_NODE;
        item->parentIndex = parentIndex;
        item->length = charCount;
        if ( charCount>0 )
            memcpy( item->text, text, charCount );
    }
    return item;
}

/// allocate element
ElementDataStorageItem * lxmlDocBase::allocElement( lInt32 dataIndex, lInt32 parentIndex, int attrCount, int childCount )
{
    int size = sizeof(ElementDataStorageItem) + attrCount*sizeof(lUInt16)*3 + childCount*sizeof(lUInt32) - sizeof(lUInt32);
    ElementDataStorageItem *item = (ElementDataStorageItem *)allocData( dataIndex, size );
    if ( item ) {
        item->type = LXML_ELEMENT_NODE;
        item->parentIndex = parentIndex;
        item->attrCount = attrCount;
        item->childCount = childCount;
    }
    return item;
}
#endif

#if BUILD_LITE!=1
lString16 lxmlDocBase::getTextNodeValue( lInt32 dataIndex )
{
    // TODO: implement caching here
    TextDataStorageItem * data = getTextNodeData( dataIndex );
    if ( !data || data->type!=LXML_TEXT_NODE )
        return lString16();
    return data->getText();
}

lString8 lxmlDocBase::getTextNodeValue8( lInt32 dataIndex )
{
    // TODO: implement caching here
    TextDataStorageItem * data = getTextNodeData( dataIndex );
    if ( !data || data->type!=LXML_TEXT_NODE )
        return lString8();
    return data->getText8();
}

ldomPersistentText::ldomPersistentText( ldomNode * parent, lUInt32 index, lString16 value )
: ldomNode( parent->getDocument(), parent, index )
{
    lString8 s8 = UnicodeToUtf8( value );
    _document->allocText( _dataIndex, _parentIndex, s8.c_str(), s8.length() );
}

ldomPersistentText::ldomPersistentText( ldomNode * parent, lUInt32 index, lString8 value )
: ldomNode( parent->getDocument(), parent, index )
{
    _document->allocText( _dataIndex, _parentIndex, value.c_str(), value.length() );
}

// create persistent r/o copy of r/w text node
ldomPersistentText::ldomPersistentText( ldomText * v )
: ldomNode( v )
{
    lString8 value = v->getText8();
    _document->allocText( _dataIndex, _parentIndex, value.c_str(), value.length() );
    _document->replaceInstance( _dataIndex, this );
}

/// returns text node text
lString16 ldomPersistentText::getText( lChar16 ) const
{
    return ((lxmlDocBase*)_document)->getTextNodeValue( _dataIndex );
}

lString8 ldomPersistentText::getText8( lChar8 ) const
{
    return ((lxmlDocBase*)_document)->getTextNodeValue8( _dataIndex );
}
#endif

// memory pools
#if (LDOM_USE_OWN_MEM_MAN==1)
ldomMemManStorage * ldomElement::pmsHeap = NULL;
ldomMemManStorage * ldomText::pmsHeap = NULL;
ldomMemManStorage * lvdomElementFormatRec::pmsHeap = NULL;
#if BUILD_LITE!=1
ldomMemManStorage * ldomPersistentText::pmsHeap = NULL;
ldomMemManStorage * ldomPersistentElement::pmsHeap = NULL;
#endif
#endif

const lString16 & ldomNode::getAttributeValue( const lChar16 * nsName, const lChar16 * attrName ) const
{
    lUInt16 nsId = (nsName&&nsName[0]) ? getDocument()->getNsNameIndex( nsName ) : LXML_NS_ANY;
    lUInt16 attrId = getDocument()->getAttrNameIndex( attrName );
    return getAttributeValue( nsId, attrId );
}

ldomNode::~ldomNode()
{
}

// use iteration instead of storing in memory
lUInt8 ldomNode::getNodeLevel() const
{
    const ldomNode * node = this;
    int level = 0;
    for ( ; node; node = node->getParentNode() )
        level++;
    return level;
}

/// returns main element (i.e. FictionBook for FB2)
ldomNode * lxmlDocBase::getRootNode()
{
    if ( _instanceMapCount<2 )
        return NULL;
    return getNodeInstance( 1 );
}

ldomDocument::ldomDocument()
#if BUILD_LITE!=1
:
     _renderedBlockCache( 32 )
#endif
{
    new ldomElement( this, NULL, 0, 0, 0 );
    //assert( _instanceMapCount==2 );
}

/// Copy constructor - copies ID tables contents
lxmlDocBase::lxmlDocBase( lxmlDocBase & doc )
:    _elementNameTable(doc._elementNameTable)    // Element Name<->Id map
,    _attrNameTable(doc._attrNameTable)       // Attribute Name<->Id map
,   _nsNameTable(doc._nsNameTable)           // Namespace Name<->Id map
,   _nextUnknownElementId(doc._nextUnknownElementId) // Next Id for unknown element
,   _nextUnknownAttrId(doc._nextUnknownAttrId)    // Next Id for unknown attribute
,   _nextUnknownNsId(doc._nextUnknownNsId)      // Next Id for unknown namespace
    //lvdomStyleCache _styleCache;         // Style cache
,   _stylesheet(doc._stylesheet)
,   _attrValueTable(doc._attrValueTable)
,   _idNodeMap(doc._idNodeMap)
,   _idAttrId(doc._idAttrId) // Id for "id" attribute name
,   _docFlags(doc._docFlags)
#if BUILD_LITE!=1
,   _pagesData(8192)
#endif
{
}

/// creates empty document which is ready to be copy target of doc partial contents
ldomDocument::ldomDocument( ldomDocument & doc )
: lxmlDocBase(doc)
, _def_font(doc._def_font) // default font
, _def_style(doc._def_style)
, _page_height(doc._page_height)

#if BUILD_LITE!=1
        , _renderedBlockCache( 32 )
#endif
, _container(doc._container)
{
}

static void writeNode( LVStream * stream, ldomNode * node )
{
    if ( node->isText() )
    {
        lString16 txt = node->getText();
        *stream << txt;
    }
    else if (  node->isElement() )
    {
        lString16 elemName = node->getNodeName();
        lString16 elemNsName = node->getNodeNsName();
        if (!elemNsName.empty())
            elemName = elemNsName + L":" + elemName;
        if (!elemName.empty())
            *stream << L"<" << elemName;
        int i;
        for (i=0; i<(int)node->getAttrCount(); i++)
        {
            const lxmlAttribute * attr = node->getAttribute(i);
            if (attr)
            {
                lString16 attrName( node->getDocument()->getAttrName(attr->id) );
                lString16 nsName( node->getDocument()->getNsName(attr->nsid) );
                lString16 attrValue( node->getDocument()->getAttrValue(attr->index) );
                *stream << L" ";
                if ( nsName.length() > 0 )
                    *stream << nsName << L":";
                *stream << attrName << L"=\"" << attrValue << L"\"";
            }
        }

#if 1
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
                    *stream << L"?>";
                else
                    *stream << L"/>";
            }
        } else {
            if (!elemName.empty())
                *stream << L">";
            for (i=0; i<(int)node->getChildCount(); i++)
            {
                writeNode( stream, node->getChildNode(i) );
            }
            if (!elemName.empty())
                *stream << L"</" << elemName << L">";
        }
    }
}

bool ldomDocument::saveToStream( LVStreamRef stream, const char * )
{
    //CRLog::trace("ldomDocument::saveToStream()");
    if (!stream || !getRootNode()->getChildCount())
        return false;

    *stream.get() << L"\xFEFF";
    writeNode( stream.get(), getRootNode() );
    return true;
}

ldomDocument::~ldomDocument()
{
#if BUILD_LITE!=1
    updateMap();
#endif
    _keepData = true;
}

#if BUILD_LITE!=1
int ldomDocument::render( LVRendPageList * pages, int width, int dy, bool showCover, int y0, font_ref_t def_font, int def_interline_space )
{
    CRLog::info("Render is called for width %d, pageHeight=%d", width, dy );
    CRLog::trace("initializing default style...");
    //persist();
    _renderedBlockCache.clear();
    _page_height = dy;
    _def_font = def_font;
    _def_style = css_style_ref_t( new css_style_rec_t );
    _def_style->display = css_d_block;
    _def_style->white_space = css_ws_normal;
    _def_style->text_align = css_ta_left;
    _def_style->text_decoration = css_td_none;
    _def_style->hyphenate = css_hyph_auto;
    _def_style->color.type = css_val_unspecified;
    _def_style->color.value = 0x000000;
    _def_style->background_color.type = css_val_unspecified;
    _def_style->background_color.value = 0xFFFFFF;
    //_def_style->background_color.type = color;
    //_def_style->background_color.value = 0xFFFFFF;
    _def_style->page_break_before = css_pb_auto;
    _def_style->page_break_after = css_pb_auto;
    _def_style->page_break_inside = css_pb_auto;
    _def_style->vertical_align = css_va_baseline;
    _def_style->font_family = def_font->getFontFamily();
    _def_style->font_size.type = css_val_px;
    _def_style->font_size.value = def_font->getHeight();
    _def_style->font_name = def_font->getTypeFace();
    _def_style->text_indent.type = css_val_px;
    _def_style->text_indent.value = 0;
    _def_style->line_height.type = css_val_percent;
    _def_style->line_height.value = def_interline_space;
    // update styles
    CRLog::trace("init format data...");
    getRootNode()->recurseElements( initFormatData );

    if ( !checkRenderContext( pages, width, dy ) ) {
        pages->clear();
        if ( showCover )
            pages->add( new LVRendPageInfo( dy ) );
        LVRendPageContext context( pages, dy );
        CRLog::info("rendering context is changed - full render required...");
        CRLog::trace("init render method...");
        initRendMethod( getRootNode() );
        //updateStyles();
        CRLog::trace("rendering...");
        int height = renderBlockElement( context, getRootNode(),
            0, y0, width ) + y0;
    #if 0 //def _DEBUG
        LVStreamRef ostream = LVOpenFileStream( "test_save_after_init_rend_method.xml", LVOM_WRITE );
        saveToStream( ostream, "utf-16" );
    #endif
        gc();
        CRLog::trace("finalizing...");
        context.Finalize();
        updateRenderContext( pages, width, dy );
        //persist();
        return height;
    } else {
        CRLog::info("rendering context is not changed - no render!");
        return getFullHeight();
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

#if BUILD_LITE!=1
static const char * id_map_list_magic = "MAPS";
static const char * elem_id_map_magic = "ELEM";
static const char * attr_id_map_magic = "ATTR";
static const char * attr_value_map_magic = "ATTV";
static const char * ns_id_map_magic =   "NMSP";
static const char * node_by_id_map_magic = "NIDM";

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
    buf << (lUInt32)_idNodeMap.length();
    LVHashTable<lUInt16,lInt32>::iterator ii = _idNodeMap.forwardIterator();
    for ( LVHashTable<lUInt16,lInt32>::pair * p = ii.next(); p!=NULL; p = ii.next() ) {
        buf << p->key << p->value;
    }
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
    buf.checkMagic( attr_id_map_magic );
    _attrNameTable.deserialize( buf );
    buf >> _nextUnknownAttrId;    // Next Id for unknown attribute
    buf.checkMagic( ns_id_map_magic );
    _nsNameTable.deserialize( buf );
    buf >> _nextUnknownNsId;      // Next Id for unknown namespace
    buf.checkMagic( attr_value_map_magic );
    _attrValueTable.deserialize( buf );

    int start = buf.pos();
    buf.checkMagic( node_by_id_map_magic );
    lUInt32 idmsize;
    buf >> idmsize;
    _idNodeMap.clear();
    if ( idmsize < 20000 )
        _idNodeMap.resize( idmsize*2 );
    for ( unsigned i=0; i<idmsize; i++ ) {
        lUInt16 key;
        lUInt32 value;
        buf >> key;
        buf >> value;
        _idNodeMap.set( key, value );
        if ( buf.error() )
            return false;
    }
    buf.checkCRC( buf.pos() - start );

    buf.checkCRC( buf.pos() - pos );
    return !buf.error();
}
#endif

/// returns node absolute rectangle
void ldomNode::getAbsRect( lvRect & rect )
{
    ldomNode * node = this;
    lvdomElementFormatRec * fmt = node->getRenderData();
    rect.left = 0;
    rect.top = 0;
    rect.right = fmt ? fmt->getWidth() : 0;
    rect.bottom = fmt ? fmt->getHeight() : 0;
    if ( !fmt )
        return;
    for (; node; node = node->getParentNode())
    {
        lvdomElementFormatRec * fmt = node->getRenderData();
        if (fmt)
        {
            rect.left += fmt->getX();
            rect.top += fmt->getY();
        }
    }
    rect.bottom += rect.top;
    rect.right += rect.left;
}

#if BUILD_LITE!=1
LVImageSourceRef ldomNode::getObjectImageSource()
{
    if ( !this || !isElement() )
        return LVImageSourceRef();
    //printf("ldomElement::getObjectImageSource() ... ");
    LVImageSourceRef ref;
    const css_elem_def_props_t * et = _document->getElementTypePtr(getNodeId());
    if (!et || !et->is_object)
        return ref;
    lUInt16 hrefId = _document->getAttrNameIndex(L"href");
    lUInt16 srcId = _document->getAttrNameIndex(L"src");
    lString16 refName = getAttributeValue( _document->getNsNameIndex(L"xlink"),
        hrefId );
    if ( refName.empty() )
        refName = getAttributeValue( _document->getNsNameIndex(L"l"), hrefId );
    if ( refName.empty() )
        refName = getAttributeValue( LXML_NS_NONE, hrefId );
    if ( refName.empty() )
        refName = getAttributeValue( LXML_NS_NONE, srcId );
    if ( refName.length()<2 )
        return ref;
    if ( refName[0]!='#' ) {
        if ( !getDocument()->getContainer().isNull() ) {
            lString16 name = refName;
            if ( !getDocument()->getCodeBase().empty() )
                name = getDocument()->getCodeBase() + refName;
            LVStreamRef stream = getDocument()->getContainer()->OpenStream(name.c_str(), LVOM_READ);
            if ( !stream.isNull() )
                ref = LVCreateStreamImageSource( stream );
        }
        return ref;
    }
    lUInt16 refValueId = _document->findAttrValueIndex( refName.c_str() + 1 );
    if ( refValueId == (lUInt16)-1 )
        return ref;
    //printf(" refName=%s id=%d ", UnicodeToUtf8( refName ).c_str(), refValueId );
    ldomNode * objnode = _document->getNodeById( refValueId );
    if ( !objnode ) {
        //printf("no OBJ node found!!!\n" );
        return ref;
    }
    //printf(" (found) ");
    ref = LVCreateNodeImageSource( objnode );
    return ref;
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

ldomElementWriter::ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent)
    : _parent(parent), _document(document)
{
    //logfile << "{c";
    _typeDef = _document->getElementTypePtr( id );
    _allowText = _typeDef ? _typeDef->allow_text : (_parent?true:false);
    if (_parent)
        _element = _parent->getElement()->insertChildElement( (lUInt32)-1, nsid, id );
    else
        _element = _document->getRootNode(); //->insertChildElement( (lUInt32)-1, nsid, id );
    //logfile << "}";
}

lUInt32 ldomElementWriter::getFlags()
{
    lUInt32 flags = 0;
    if ( _typeDef && _typeDef->white_space==css_ws_pre )
        flags |= TXTFLG_PRE;
    return flags;
}

void ldomElementWriter::onText( const lChar16 * text, int len, lUInt32 )
{
    //logfile << "{t";
    {
        // normal mode: store text copy
        _element->insertChildText(lString16(text, len));
    }
    //logfile << "}";
}

void ldomElementWriter::addAttribute( lUInt16 nsid, lUInt16 id, const wchar_t * value )
{
    getElement()->setAttributeValue(nsid, id, value);
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
        if (tmp->getElement()->getNodeId() == id)
            break;
        ElementCloseHandler( tmp->getElement() );
        delete tmp;
    }
    /*
    logfile << "3 * ";
    logfile << (int)tmp << " - " << (int)tmp2 << " | cnt=";
    logfile << (int)tmp->getElement()->childCount << " - "
            << (int)tmp2->getElement()->childCount;
    */
    ElementCloseHandler( tmp->getElement() );
    delete tmp;
    //logfile << "}";
    return tmp2;
}

ldomElementWriter::~ldomElementWriter()
{
    //getElement()->persist();
}




/////////////////////////////////////////////////////////////////
/// ldomDocumentWriter

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

ldomNode * ldomDocumentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    //logfile << "ldomDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
    //CRLog::trace("OnTagOpen(%s)", UnicodeToUtf8(lString16(tagname)).c_str());
    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;

    //if ( id==_stopTagId ) {
        //CRLog::trace("stop tag found, stopping...");
    //    _parser->Stop();
    //}
    _currNode = new ldomElementWriter( _document, nsid, id, _currNode );
    _flags = _currNode->getFlags();
    //logfile << " !o!\n";
    return _currNode->getElement();
}

ldomDocumentWriter::~ldomDocumentWriter()
{
    while (_currNode)
        _currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
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
    if (_currNode)
    {
        if ( (_flags & XML_FLAG_NO_SPACE_TEXT)
             && IsEmptySpace(text, len) )
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
    : _document(document), _currNode(NULL), _errFlag(false), _headerOnly(headerOnly), _flags(0)
{
    _stopTagId = 0xFFFE;
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
                        m_text_pos--;
                        flgEof = true;
                        break;
                    }
                    else
                    {
                        int k = base64_decode_table[ch];
                        if ( !(k & 0x80) )
                        {
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

/// creates stream to read base64 encoded data from element
LVStreamRef ldomNode::createBase64Stream()
{
    if ( !this || !isElement() )
        return LVStreamRef();
#define DEBUG_BASE64_IMAGE 0
#if DEBUG_BASE64_IMAGE==1
    lString16 fname = getAttributeValue( attr_id );
    lString8 fname8 = UnicodeToUtf8( fname );
    LVStreamRef ostream = LVOpenFileStream( fname.empty()?L"image.png":fname.c_str(), LVOM_WRITE );
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
            if (s[pos] && s[pos!='/'] && s[pos]!='.')
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
            return (name==L"text()") ? xpath_step_text : xpath_step_element; // OK!
        }
        return xpath_step_error; // error
    }
    return xpath_step_error;
}

#if (LDOM_ALLOW_NODE_INDEX!=1)
lUInt32 ldomNode::getNodeIndex() const
{
    ldomNode * parent = getParentNode();
    if ( !parent )
        return 0;
    for (int i=parent->getChildCount()-1; i>=0; i--)
        if ( parent->getChildNode( i )==this )
            return i;
    return (lUInt32)-1;
}
#endif

/// returns first child node
ldomNode * ldomElement::getFirstChild() const
{
    return _children.length()>0?_document->getNodeInstance(_children[0]):NULL;
}
/// returns last child node
ldomNode * ldomElement::getLastChild() const
{
    return _children.length()>0?_document->getNodeInstance(_children[_children.length()-1]):NULL;
}

/// removes and deletes last child element
void ldomElement::removeLastChild()
{
    if ( _children.length()>0 ) {
        ldomNode * child = ldomElement::getLastChild();
        _children.remove( _children.length() - 1 );
        delete child;
    }
}

/// returns text node text
lString16 ldomNode::getText( lChar16 blockDelimiter ) const
{
    lString16 txt;
    for ( unsigned i=0; i<getChildCount(); i++ ) {
        txt += getChildNode(i)->getText(blockDelimiter);
        ldomNode * child = getChildNode(i);
        if ( i>=getChildCount()-1 )
            break;
        if ( blockDelimiter && child->isElement() ) {
            if ( child->getStyle()->display == css_d_block )
                txt << blockDelimiter;
        }
    }
    return txt;
}

/// returns text node text
lString8 ldomNode::getText8( lChar8 blockDelimiter ) const
{
    lString8 txt;
    for ( unsigned i=0; i<getChildCount(); i++ ) {
        txt += getChildNode(i)->getText8(blockDelimiter);
        ldomNode * child = getChildNode(i);
        if ( i>=getChildCount()-1 )
            break;
        if ( blockDelimiter && child->isElement() ) {
            if ( child->getStyle()->display == css_d_block )
                txt << blockDelimiter;
        }
    }
    return txt;
}

/// get pointer for relative path
ldomXPointer ldomXPointer::relative( lString16 relativePath )
{
    return getDocument()->createXPointer( getNode(), relativePath );
}
/// create xpointer from pointer string
ldomXPointer ldomDocument::createXPointer( const lString16 & xPointerStr )
{
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
        if ( node->getRendMethod()==erm_final )
            return node;
        node = node->getParentNode();
    }
}

/// formats final block again after change, returns true if size of block is changed
bool ldomNode::refreshFinalBlock()
{
    if ( getRendMethod() != erm_final )
        return false;
    // TODO: implement reformatting of one node
    CVRendBlockCache & cache = getDocument()->getRendBlockCache();
    cache.remove( this );
    lvdomElementFormatRec * fmt = getRenderData();
    if ( !fmt )
        return false;
    lvRect oldRect, newRect;
    fmt->getRect( oldRect );
    LFormattedTextRef txtform;
    int width = fmt->getWidth();
    int h = renderFinalBlock( txtform, width );
    fmt->getRect( newRect );
    if ( oldRect == newRect )
        return false;
    // TODO: relocate other blocks
    return true;
}

/// formats final block
int ldomNode::renderFinalBlock( LFormattedTextRef & txtform, int width )
{
    if ( !isElement() )
        return 0;
    CVRendBlockCache & cache = getDocument()->getRendBlockCache();
    LFormattedTextRef f;
    if ( cache.get( this, f ) ) {
        txtform = f;
        lvdomElementFormatRec * fmt = getRenderData();
        if ( !fmt || getRendMethod() != erm_final )
            return 0;
        //CRLog::trace("Found existing formatted object for node #%08X", (lUInt32)this);
        return fmt->getHeight();
    }
    f = new LFormattedText();
    lvdomElementFormatRec * fmt = getRenderData();
    if ( !fmt || (getRendMethod() != erm_final && getRendMethod() != erm_table_caption) )
        return 0;
    /// render whole node content as single formatted object
    int flags = styleToTextFmtFlags( getStyle(), 0 );
    ::renderFinalBlock( this, f.get(), fmt, flags, 0, 16 );
    int page_h = getDocument()->getPageHeight();
    cache.set( this, f );
    int h = f->Format( width, page_h );
    txtform = f;
    //CRLog::trace("Created new formatted object for node #%08X", (lUInt32)this);
    return h;
}
#endif

/// returns first text child element
ldomNode * ldomNode::getFirstTextChild()
{
    if ( isText() )
        return (ldomText *)this;
    else {
        for ( int i=0; i<(int)getChildCount(); i++ ) {
            ldomNode * p = getChildNode(i)->getFirstTextChild();
            if (p)
                return p;
        }
    }
    return NULL;
}

/// returns last text child element
ldomNode * ldomNode::getLastTextChild()
{
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
ldomNode * ldomNode::elementFromPoint( lvPoint pt )
{
    if ( !isElement() )
        return NULL;
    ldomNode * enode = this;
    lvdomElementFormatRec * fmt = getRenderData();
    if ( !fmt )
        return NULL;
    if ( enode->getRendMethod() == erm_invisible ) {
        return NULL;
    }
    if ( pt.y < fmt->getY() )
        return NULL;
    if ( pt.y >= fmt->getY() + fmt->getHeight() )
        return NULL;
    if ( enode->getRendMethod() == erm_final ) {
        return this;
    }
    int count = getChildCount();
    for ( int i=0; i<count; i++ ) {
        ldomNode * p = getChildNode( i );
        ldomNode * e = p->elementFromPoint( lvPoint( pt.x - fmt->getX(),
                pt.y - fmt->getY() ) );
        if ( e )
            return e;
    }
    return this;
}

ldomNode * ldomNode::finalBlockFromPoint( lvPoint pt )
{
    ldomNode * elem = elementFromPoint( pt );
    if ( elem && elem->getRendMethod() == erm_final )
        return elem;
    return NULL;
}

/// create xpointer from doc point
ldomXPointer ldomDocument::createXPointer( lvPoint pt )
{
    //
    ldomXPointer ptr;
    if ( !getRootNode() )
        return ptr;
    ldomNode * finalNode = getRootNode()->elementFromPoint( pt );
    if ( !finalNode ) {
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
    lvRect rc;
    finalNode->getAbsRect( rc );
    //CRLog::debug("ldomDocument::createXPointer point = (%d, %d), finalNode %08X rect = (%d,%d,%d,%d)", pt.x, pt.y, (lUInt32)finalNode, rc.left, rc.top, rc.right, rc.bottom );
    pt.x -= rc.left;
    pt.y -= rc.top;
    lvdomElementFormatRec * r = finalNode->getRenderData();
    if ( !r )
        return ptr;
    if ( finalNode->getRendMethod() != erm_final ) {
        // not final, use as is
        if ( pt.y < (rc.bottom + rc.top) / 2 )
            return ldomXPointer( finalNode, 0 );
        else
            return ldomXPointer( finalNode, finalNode->getChildCount() );
    }
    // final, format and search
    LFormattedTextRef txtform;
    finalNode->renderFinalBlock( txtform, r->getWidth() );
    int lcount = txtform->GetLineCount();
    for ( int l = 0; l<lcount; l++ ) {
        const formatted_line_t * frmline = txtform->GetLineInfo(l);
        if ( pt.y >= (int)(frmline->y + frmline->height) && l<lcount-1 )
            continue;
        //CRLog::debug("  point (%d, %d) line found [%d]: (%d..%d)", pt.x, pt.y, l, frmline->y, frmline->y+frmline->height);
        // found line, searching for word
        int wc = (int)frmline->word_count;
        int x = pt.x - frmline->x;
        for ( int w=0; w<wc; w++ ) {
            const formatted_word_t * word = &frmline->words[w];
            if ( x < word->x + word->width || w==wc-1 ) {
                const src_text_fragment_t * src = txtform->GetSrcInfo(word->src_text_index);
                //CRLog::debug(" word found [%d]: x=%d..%d, start=%d, len=%d  %08X", w, word->x, word->x + word->width, word->t.start, word->t.len, src->object);
                // found word, searching for letters
                ldomNode * node = (ldomNode *)src->object;
                if ( src->flags & LTEXT_SRC_IS_OBJECT ) {
                    // object (image)
                    return ldomXPointer( node->getParentNode(),
                        node->getNodeIndex() + (( x < word->x + word->width/2 ) ? 0 : 1) );
                }
                LVFont * font = (LVFont *) src->t.font;
                lUInt16 w[512];
                lUInt8 flg[512];

                lString16 str = node->getText();
                font->measureText( str.c_str()+word->t.start, word->t.len, w, flg, word->width+50, '?', src->letter_spacing);
                for ( int i=0; i<word->t.len; i++ ) {
                    int xx = ( i>0 ) ? (w[i-1] + w[i])/2 : w[i]/2;
                    if ( x < word->x + xx ) {
                        return ldomXPointer( node, src->t.offset + word->t.start + i );
                    }
                }
                return ldomXPointer( node, src->t.offset + word->t.start + word->t.len );
            }
        }
    }
    return ptr;
}

/// returns coordinates of pointer inside formatted document
lvPoint ldomXPointer::toPoint() const
{
    lvRect rc;
    if ( !getRect( rc ) )
        return lvPoint(-1, -1);
    return rc.topLeft();
}

/// returns caret rectangle for pointer inside formatted document
bool ldomXPointer::getRect(lvRect & rect) const
{
    //CRLog::trace("ldomXPointer::getRect()");
    if ( isNull() )
        return false;
    ldomNode * p = isElement() ? getNode() : getNode()->getParentNode();
    ldomNode * finalNode = NULL;
    if ( !p ) {
        //CRLog::trace("ldomXPointer::getRect() - p==NULL");
    }
    //printf("getRect( p=%08X type=%d )\n", (unsigned)p, (int)p->getNodeType() );
    if ( !p->getDocument() ) {
        //CRLog::trace("ldomXPointer::getRect() - p->getDocument()==NULL");
    }
    ldomNode * mainNode = p->getDocument()->getRootNode();
    for ( ; p; p = p->getParentNode() ) {
        if ( p->getRendMethod() == erm_final ) {
            finalNode = p; // found final block
        } else if ( p->getRendMethod() == erm_invisible ) {
            return false; // invisible !!!
        }
        if ( p==mainNode )
            break;
    }
    if ( finalNode!=NULL ) {
        lvRect rc;
        finalNode->getAbsRect( rc );
        lvdomElementFormatRec * r = finalNode->getRenderData();
        if ( !r )
            return false;
        LFormattedTextRef txtform;
        finalNode->renderFinalBlock( txtform, r->getWidth() );

        ldomNode * node = getNode();
        int offset = getOffset();
        if ( node->isElement() ) {
            if ( offset>=0 ) {
                //
                if ( offset>= (int)node->getChildCount() ) {
                    node = node->getLastTextChild();
                    if ( node )
                        offset = node->getText().length();
                    else
                        return false;
                } else {
                    for ( int ci=offset; ci<(int)node->getChildCount(); ci++ ) {
                        ldomNode * child = node->getChildNode( offset );
                        ldomNode * txt = child->getFirstTextChild();
                        if ( txt ) {
                            node = txt;
                            break;
                        }
                    }
                    if ( !node->isText() )
                        return false;
                    offset = 0;
                }
            }
        }

        // text node
        int srcIndex = -1;
        int srcLen = -1;
        for ( int i=0; i<txtform->GetSrcCount(); i++ ) {
            const src_text_fragment_t * src = txtform->GetSrcInfo(i);
            if ( src->object == node ) {
                srcIndex = i;
                srcLen = src->t.len;
                break;
            }
        }
        if ( srcIndex == -1 )
            return false;
        for ( int l = 0; l<txtform->GetLineCount(); l++ ) {
            const formatted_line_t * frmline = txtform->GetLineInfo(l);
            for ( int w=0; w<(int)frmline->word_count; w++ ) {
                const formatted_word_t * word = &frmline->words[w];
                if ( word->src_text_index==srcIndex ) {
                    // found word from same src line
                    if ( offset<=word->t.start ) {
                        // before this word
                        rect.left = word->x + rc.left + frmline->x;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        rect.right = rect.left + 1;
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    } else if ( (offset<word->t.start+word->t.len) || (offset==srcLen && offset==word->t.start+word->t.len) ) {
                        // pointer inside this word
                        LVFont * font = (LVFont *) txtform->GetSrcInfo(srcIndex)->t.font;
                        lUInt16 w[512];
                        lUInt8 flg[512];
                        lString16 str = node->getText();
                        font->measureText( str.c_str()+word->t.start, offset - word->t.start, w, flg, word->width+50, '?', txtform->GetSrcInfo(srcIndex)->letter_spacing);
                        int chx = w[ offset - word->t.start - 1 ];
                        rect.left = word->x + chx + rc.left + frmline->x;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        rect.right = rect.left + 1;
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    }
                } else if ( word->src_text_index>srcIndex ) {
                    return false;
                }
            }
        }
        return false;
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

/// create xpointer from relative pointer string
ldomXPointer ldomDocument::createXPointer( ldomNode * baseNode, const lString16 & xPointerStr )
{
    //CRLog::trace( "ldomDocument::createXPointer(%s)", UnicodeToUtf8(xPointerStr).c_str() );
    if ( xPointerStr.empty() )
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
                ldomNode * foundItem = NULL;
                int foundCount = 0;
                for (unsigned i=0; i<currNode->getChildCount(); i++) {
                    ldomNode * p = currNode->getChildNode(i);
                    //CRLog::trace( "        node[%d] = %d", i, p->getNodeId() );
                    if ( p && p->isElement() && p->getNodeId()==id ) {
                        foundCount++;
                        if ( foundCount==index || index==-1 ) {
                            foundItem = p;
                        }
                    }
                }
                if ( foundItem==NULL || (index==-1 && foundCount>1) ) {
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
                for (unsigned i=0; i<currNode->getChildCount(); i++) {
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

lString16 ldomXPointer::toString()
{
    lString16 path;
    if ( isNull() )
        return path;
    ldomNode * node = getNode();
    int offset = getOffset();
    if ( offset >= 0 ) {
        path << L"." << lString16::itoa(offset);
    }
    ldomNode * p = node;
    ldomNode * mainNode = node->getDocument()->getRootNode();
    while (p && p!=mainNode) {
        ldomNode * parent = p->getParentNode();
        if ( p->isElement() ) {
            // element
            lString16 name = p->getNodeName();
            int id = p->getNodeId();
            if ( !parent )
                return lString16(L"/") + name + path;
            int index = -1;
            int count = 0;
            for ( unsigned i=0; i<parent->getChildCount(); i++ ) {
                ldomNode * node = parent->getChildNode( i );
                if ( node->isElement() && node->getNodeId()==id ) {
                    count++;
                    if ( node==p )
                        index = count;
                }
            }
            if ( count>1 )
                path = lString16(L"/") + name + L"[" + lString16::itoa(index) + L"]" + path;
            else
                path = lString16(L"/") + name + path;
        } else {
            // text
            if ( !parent )
                return lString16(L"/text()") + path;
            int index = -1;
            int count = 0;
            for ( unsigned i=0; i<parent->getChildCount(); i++ ) {
                ldomNode * node = parent->getChildNode( i );
                if ( node->isText() ) {
                    count++;
                    if ( node==p )
                        index = count;
                }
            }
            if ( count>1 )
                path = lString16(L"/text()") + L"[" + lString16::itoa(index) + L"]" + path;
            else
                path = lString16(L"/text()") + path;
        }
        p = parent;
    }
    return path;
}

int ldomDocument::getFullHeight()
{
    lvdomElementFormatRec * rd = this ? this->getRootNode()->getRenderData() : NULL;
    return ( rd ? rd->getHeight() + rd->getY() : 0 );
}




lString16 extractDocAuthors( ldomDocument * doc )
{
    lString16 authors;
    for ( int i=0; i<16; i++) {
        lString16 path = lString16(L"/FictionBook/description/title-info/author[") + lString16::itoa(i+1) + L"]";
        ldomXPointer pauthor = doc->createXPointer(path);
        if ( !pauthor ) {
            //CRLog::trace( "xpath not found: %s", UnicodeToUtf8(path).c_str() );
            break;
        }
        lString16 firstName = pauthor.relative( L"/first-name" ).getText();
        lString16 lastName = pauthor.relative( L"/last-name" ).getText();
        lString16 middleName = pauthor.relative( L"/middle-name" ).getText();
        lString16 author = firstName;
        if ( !author.empty() )
            author += L" ";
        if ( !middleName.empty() )
            author += lString16(middleName, 0, 1) + L". ";
        author += lastName;
        if ( !authors.empty() )
            authors += L", ";
        authors += author;
    }
    return authors;
}

lString16 extractDocTitle( ldomDocument * doc )
{
    return doc->createXPointer(L"/FictionBook/description/title-info/book-title").getText();
}

lString16 extractDocSeries( ldomDocument * doc )
{
    lString16 res;
    ldomNode * series = doc->createXPointer(L"/FictionBook/description/title-info/sequence").getNode();
    if ( series ) {
        lString16 sname = series->getAttributeValue( attr_name );
        lString16 snumber = series->getAttributeValue( attr_number );
        if ( !sname.empty() ) {
            res << L"(" << sname;
            if ( !snumber.empty() )
                res << L" #" << snumber << L")";
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
    if ( _level < 1 )
        return false;
    ldomNode * p = getNode()->getParentNode();
    if ( !p || index < 0 || index >= (int)p->getChildCount() )
        return false;
    setNode( p->getChildNode( index ) );
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
    return sibling( _indexes[_level-1] - 1 );
}

/// move to next sibling element
bool ldomXPointerEx::nextSiblingElement()
{
    if ( _level < 1 )
        return false;
    ldomNode * node = getNode();
    ldomNode * p = node->getParentNode();
    for ( int i=_indexes[_level-1] + 1; i<(int)node->getChildCount(); i++ ) {
        if ( p->getChildNode( i )->isElement() )
            return sibling( i );
    }
    return false;
}

/// move to previous sibling element
bool ldomXPointerEx::prevSiblingElement()
{
    if ( _level < 1 )
        return false;
    ldomNode * node = getNode();
    ldomNode * p = node->getParentNode();
    for ( int i=_indexes[_level-1] - 1; i>=0; i-- ) {
        if ( p->getChildNode( i )->isElement() )
            return sibling( i );
    }
    return false;
}

/// move to parent
bool ldomXPointerEx::parent()
{
    if ( _level<=1 )
        return false;
    setNode( getNode()->getParentNode() );
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
        if ( getOffset() < v._indexes[i] )
            return -1;
        if ( getOffset() > v._indexes[i] )
            return 1;
        return -1;
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
ldomXRange::ldomXRange( ldomNode * p )
: _start( p, 0 ), _end( p, p->isText() ? p->getText().length() : p->getChildCount() ), _flags(1)
{
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

bool ldomDocument::findText( lString16 pattern, bool caseInsensitive, int minY, int maxY, LVArray<ldomWord> & words, int maxCount )
{
    if ( minY<0 )
        minY = 0;
    int fh = getFullHeight();
    if ( maxY<=0 || maxY>fh )
        maxY = fh;
    ldomXPointer start = createXPointer( lvPoint(minY, 0) );
    ldomXPointer end = createXPointer( lvPoint(maxY, 10000) );
    if ( start.isNull() || end.isNull() )
        return false;
    ldomXRange range( start, end );
    return range.findText( pattern, caseInsensitive, words, maxCount );
}

static bool findText( const lString16 & str, int & pos, const lString16 & pattern )
{
    int len = pattern.length();
    if ( pos < 0 || pos + len > (int)str.length() )
        return false;
    const lChar16 * s1 = str.c_str() + pos;
    const lChar16 * s2 = pattern.c_str();
    int nlen = str.length() - pos - len;
    for ( int j=0; j<nlen; j++ ) {
        bool matched = true;
        for ( int i=0; i<len; i++ ) {
            if ( s1[i] != s2[i] ) {
                matched = false;
                break;
            }
        }
        if ( matched )
            return true;
        s1++;
        pos++;
    }
    return false;
}

/// searches for specified text inside range
bool ldomXRange::findText( lString16 pattern, bool caseInsensitive, LVArray<ldomWord> & words, int maxCount )
{
    if ( caseInsensitive )
        pattern.lowercase();
    words.clear();
    if ( pattern.empty() )
        return false;
    if ( !_start.isText() )
        _start.nextVisibleText();
    while ( !isNull() ) {
        int offs = _start.getOffset();
        lString16 txt = _start.getNode()->getText();
        if ( caseInsensitive )
            txt.lowercase();

        while ( ::findText( txt, offs, pattern ) ) {
            words.add( ldomWord(_start.getNode(), offs, offs + pattern.length() ) );
            offs++;
        }
        if ( !_start.nextVisibleText() )
            break;
        if ( words.length() >= maxCount )
            break;
    }
    return words.length() > 0;
}

/// fill marked ranges list
void ldomXRangeList::getRanges( ldomMarkedRangeList &dst )
{
    dst.clear();
    if ( empty() )
        return;
    for ( int i=0; i<length(); i++ ) {
        ldomXRange * range = get(i);
        ldomMarkedRange * item = new ldomMarkedRange( range->getStart().toPoint(), range->getEnd().toPoint(), range->getFlags() );
        if ( !item->empty() )
            dst.add( item );
        else
            delete item;
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
bool ldomXRange::getRect( lvRect & rect )
{
    if ( isNull() )
        return false;
    // get start and end rects
    lvRect rc1;
    lvRect rc2;
    if ( !getStart().getRect(rc1) || !getEnd().getRect(rc2) )
        return false;
    if ( rc1.top == rc2.top && rc1.bottom == rc2.bottom ) {
        // on same line
        rect.left = rc1.left;
        rect.top = rc1.top;
        rect.right = rc2.right;
        rect.bottom = rc2.bottom;
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

/// create bounded by RC list, with (0,0) coordinates at left top corner
ldomMarkedRangeList::ldomMarkedRangeList( const ldomMarkedRangeList * list, lvRect & rc )
{
    if ( !list || list->empty() )
        return;
    if ( list->get(0)->start.y>rc.bottom )
        return;
    if ( list->get( list->length()-1 )->end.y < rc.top )
        return;
    for ( int i=0; i<list->length(); i++ ) {
        ldomMarkedRange * src = list->get(i);
        if ( src->start.y>=rc.bottom || src->end.y<rc.top )
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
    while ( start.getIndex()!=end.getIndex() && start.parent() && end.parent() )
        ;
    if ( start.getNode()==end.getNode() )
        return start.getNode();
    return NULL;
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
    if ( node->isText() && !parent() )
        return false;
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
bool ldomXPointerEx::nextVisibleText()
{
    while ( nextText() ) {
        if ( isVisible() )
            return true;
    }
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
bool ldomXPointerEx::nextText()
{
    setOffset( 0 );
    while ( firstChild() ) {
        if ( isText() )
            return true;
    }
    for (;;) {
        while ( nextSibling() ) {
            if ( isText() )
                return true;
            while ( firstChild() ) {
                if ( isText() )
                    return true;
            }
        }
        if ( !parent() )
            return false;
    }
}

/// move to previous text node
bool ldomXPointerEx::prevText()
{
    setOffset( 0 );
    for (;;) {
        while ( prevSibling() ) {
            if ( isText() )
                return true;
            while ( lastChild() ) {
                if ( isText() )
                    return true;
            }
        }
        if ( !parent() )
            return false;
    }
}

/// move to previous visible text node
bool ldomXPointerEx::prevVisibleText()
{
    while ( prevText() )
        if ( isVisible() )
            return true;
    return false;
}

// TODO: implement better behavior
static bool IsUnicodeSpace( lChar16 ch )
{
    return ch==' ';
}

/// move to previous visible word beginning
bool ldomXPointerEx::prevVisibleWordStart()
{
    if ( isNull() )
        return false;
    ldomNode * node = NULL;
    lString16 text;
    int textLen = 0;
    for ( ;; ) {
        if ( !isText() || !isVisible() || _data->getOffset()==0 ) {
            // move to previous text
            if ( !prevVisibleText() )
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
        while ( _data->getOffset() > 0 && IsUnicodeSpace(text[_data->getOffset()-1]) )
            _data->addOffset(-1);
        while ( _data->getOffset()>0 ) {
            if ( IsUnicodeSpace(text[ _data->getOffset()-1 ]) )
                break;
            foundNonSpace = true;
            _data->addOffset(-1);
        }
        if ( foundNonSpace )
            return true;
    }
}

/// move to previous visible word end
bool ldomXPointerEx::prevVisibleWordEnd()
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
            if ( !prevVisibleText() )
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
        while ( _data->getOffset() > 0 && IsUnicodeSpace(text[_data->getOffset()-1]) ) {
            _data->addOffset(-1);
            moved = true;
        }
        if ( moved && _data->getOffset()>0 )
            return true; // found!
        // skip non-spaces
        while ( _data->getOffset()>0 ) {
            if ( IsUnicodeSpace(text[ _data->getOffset()-1 ]) )
                break;
            _data->addOffset(-1);
        }
        // skip spaces
        while ( _data->getOffset() > 0 && IsUnicodeSpace(text[_data->getOffset()-1]) ) {
            _data->addOffset(-1);
            moved = true;
        }
        if ( moved && _data->getOffset()>0 )
            return true; // found!
    }
}

/// move to next visible word beginning
bool ldomXPointerEx::nextVisibleWordStart()
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
            if ( !nextVisibleText() )
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
                if ( !nextVisibleText() )
                    return false;
                _data->setOffset( 0 );
            }
        }
        // skip spaces
        while ( _data->getOffset()<textLen && IsUnicodeSpace(text[ _data->getOffset() ]) ) {
            _data->addOffset(1);
            moved = true;
        }
        if ( moved && _data->getOffset()<textLen )
            return true;
        // skip non-spaces
        while ( _data->getOffset()<textLen ) {
            if ( IsUnicodeSpace(text[ _data->getOffset() ]) )
                break;
            moved = true;
            _data->addOffset(1);
        }
        // skip spaces
        while ( _data->getOffset()<textLen && IsUnicodeSpace(text[ _data->getOffset() ]) ) {
            _data->addOffset(1);
            moved = true;
        }
        if ( moved && _data->getOffset()<textLen )
            return true;
    }
}

/// move to next visible word end
bool ldomXPointerEx::nextVisibleWordEnd()
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
            if ( !nextVisibleText() )
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
                if ( !nextVisibleText() )
                    return false;
                _data->setOffset( 0 );
            }
        }
        bool nonSpaceFound = false;
        // skip non-spaces
        while ( _data->getOffset()<textLen ) {
            if ( IsUnicodeSpace(text[ _data->getOffset() ]) )
                break;
            nonSpaceFound = true;
            _data->addOffset(1);
        }
        if ( nonSpaceFound )
            return true;
        // skip spaces
        while ( _data->getOffset()<textLen && IsUnicodeSpace(text[ _data->getOffset() ]) ) {
            _data->addOffset(1);
            moved = true;
        }
        // skip non-spaces
        while ( _data->getOffset()<textLen ) {
            if ( IsUnicodeSpace(text[ _data->getOffset() ]) )
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
    if ( (i==0 && i<textLen && !IsUnicodeSpace(text[i])) || (i<textLen && IsUnicodeSpace(text[i-1]) && !IsUnicodeSpace(text[i]) ) )
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
    if ( (i==textLen && i>0 && !IsUnicodeSpace(text[i-1]))
        || (i>0 && !IsUnicodeSpace(text[i-1]) && IsUnicodeSpace(text[i]) ) )
        return true;
    return false;
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

/// get all words from specified range
void ldomXRange::getRangeWords( LVArray<ldomWord> & list )
{
    class ldomWordsCollector : public ldomNodeCallback {
        LVArray<ldomWord> & _list;
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
            int beginOfWord = -1;
            for ( int i=0; i <= len; i++ ) {
                int alpha = lGetCharProps(text[i]) & CH_PROP_ALPHA;
                if (alpha && beginOfWord<0 ) {
                    beginOfWord = i;
                }
                if ( !alpha && beginOfWord>=0) {
                    _list.add( ldomWord( node, beginOfWord, i ) );
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
    ldomWordsCollector collector( list );
    forEach( &collector );
}

class ldomTextCollector : public ldomNodeCallback
{
private:
    bool lastText;
    bool newBlock;
    int  delimiter;
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
        ldomNode * elem = (ldomNode *)ptr->getNode();
        if ( elem->getRendMethod()==erm_invisible )
            return false;
        switch ( elem->getStyle()->display ) {
        /*
        case css_d_inherit:
        case css_d_block:
        case css_d_list_item:
        case css_d_compact:
        case css_d_marker:
        case css_d_table:
        case css_d_inline_table:
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
            newBlock = false;
            return true;
        }
    }
    /// get collected text
    lString16 getText() { return text; }
};

/// returns text between two XPointer positions
lString16 ldomXRange::getRangeText( lChar16 blockDelimiter, int maxTextLen )
{
    ldomTextCollector callback( blockDelimiter, maxTextLen );
    forEach( &callback );
    return callback.getText();
}

/// returns href attribute of <A> element, null string if not found
lString16 ldomXPointer::getHRef()
{
    if ( isNull() )
        return lString16();
    ldomNode * node = getNode();
    while ( node && !node->isElement() )
        node = node->getParentNode();
    while ( node && node->getNodeId()!=el_a )
        node = node->getParentNode();
    if ( !node )
        return lString16();
    lString16 ref = node->getAttributeValue( LXML_NS_ANY, attr_href );
    return ref;
}


/// returns href attribute of <A> element, null string if not found
lString16 ldomXRange::getHRef()
{
    if ( isNull() )
        return lString16();
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




/** \brief callback object to fill DOM tree

    To be used with XML parser as callback object.

    Creates document according to incoming events.

    Autoclose HTML tags.
*/
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
    //logfile << "lxmlDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
    if ( nsname && nsname[0] )
        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
    lStr_lowercase( const_cast<lChar16 *>(tagname), lStr_len(tagname) );

    // Patch for bad LIB.RU books - BR delimited paragraphs in "Fine HTML" format
    if ( tagname[0]=='b' && tagname[1]=='r' && tagname[2]==0 ) {
        // substitute to P
        tagname = L"p";
        _libRuParagraphStart = true; // to trim leading &nbsp;
    } else {
        _libRuParagraphStart = false;
    }

    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    AutoClose( id, true );
    _currNode = new ldomElementWriter( _document, nsid, id, _currNode );
    _flags = _currNode->getFlags();
    if ( _libRuDocumentDetected && (_flags & TXTFLG_PRE) )
        _flags |= TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM; // convert preformatted text into paragraphs
    //logfile << " !o!\n";
    return _currNode->getElement();
}

void ldomDocumentWriterFilter::ElementCloseHandler( ldomNode * node )
{
    ldomNode * parent = node->getParentNode();
    lUInt16 id = node->getNodeId();
    if ( parent ) {
        if ( parent->getLastChild() != node )
            return;
        if ( id==el_table ) {
            if ( node->getAttributeValue(attr_align)==L"right" && node->getAttributeValue(attr_width)==L"30%" ) {
                // LIB.RU TOC detected: remove it
                parent->removeLastChild();
            }
        } else if ( id==el_pre && _libRuDocumentDetected ) {
            // for LIB.ru - replace PRE element with DIV (section?)
            if ( node->getChildCount()==0 )
                parent->removeLastChild(); // remove empty PRE element
            //else if ( node->getLastChild()->getNodeId()==el_div && node->getLastChild()->getChildCount() &&
            //          ((ldomElement*)node->getLastChild())->getLastChild()->getNodeId()==el_form )
            //    parent->removeLastChild(); // remove lib.ru final section
            else
                node->setNodeId( el_div );
        } else if ( id==el_div ) {
            if ( node->getAttributeValue(attr_align)==L"right" ) {
                ldomNode * child = node->getLastChild();
                if ( child && child->getNodeId()==el_form )  {
                    // LIB.RU form detected: remove it
                    parent->removeLastChild();
                    _libRuDocumentDetected = true;
                }
            }
        }
    }
}

void ldomDocumentWriterFilter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    //logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
    if ( nsname && nsname[0] )
        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
    lStr_lowercase( const_cast<lChar16 *>(attrname), lStr_len(attrname) );

    lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex( nsname ) : 0;
    lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex( attrname ) : 0;
    _currNode->addAttribute( attr_ns, attr_id, attrvalue );

    //logfile << " !a!\n";
}

/// called on closing tag
void ldomDocumentWriterFilter::OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
{
    //logfile << "ldomDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
    if ( nsname && nsname[0] )
        lStr_lowercase( const_cast<lChar16 *>(nsname), lStr_len(nsname) );
    lStr_lowercase( const_cast<lChar16 *>(tagname), lStr_len(tagname) );
    if (!_currNode)
    {
        _errFlag = true;
        //logfile << " !c-err!\n";
        return;
    }
    lUInt16 id = _document->getElementNameIndex(tagname);

    // HTML title detection
    if ( id==el_title && _currNode->_element->getParentNode()!= NULL && _currNode->_element->getParentNode()->getNodeId()==el_head ) {
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
    //logfile << "lxmlDocumentWriter::OnText() fpos=" << fpos;
    if (_currNode)
    {
        AutoClose( _currNode->_element->getNodeId(), false );
        if ( (_flags & XML_FLAG_NO_SPACE_TEXT)
             && IsEmptySpace(text, len) )
             return;
        bool autoPara = _libRuDocumentDetected && (flags & TXTFLG_PRE);
        if (_currNode->_allowText) {
            if ( _libRuParagraphStart ) {
                while ( *text==160 && len > 0 ) {
                    text++;
                    len--;
                    while ( *text==' ' && len > 0 ) {
                        text++;
                        len--;
                    }
                }
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
                if ( ch=='-' || ch=='=' || ch=='_' || ch=='*' || ch=='#' )
                    isHr = true;
            }
            if ( isHr ) {
                OnTagOpen( NULL, L"hr" );
                OnTagClose( NULL, L"hr" );
            } else if ( len > 0 ) {
                if ( autoPara )
                    OnTagOpen( NULL, paraTag );
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

void ldomElement::addChild( lInt32 dataIndex )
{
    _children.add( dataIndex );
}

/// move range of children startChildIndex to endChildIndex inclusively to specified element
void ldomElement::moveItemsTo( ldomNode * destination, int startChildIndex, int endChildIndex )
{
    CRLog::warn( "moveItemsTo() invoked from %d to %d", getDataIndex(), destination->getDataIndex() );
    //if ( getDataIndex()==INDEX2 || getDataIndex()==INDEX1) {
    //    CRLog::trace("nodes from element %d are being moved", getDataIndex());
    //}
/*#ifdef _DEBUG
    if ( !_document->checkConsistency( false ) )
        CRLog::error("before moveItemsTo");
#endif*/
    int len = endChildIndex - startChildIndex + 1;
    for ( int i=0; i<len; i++ ) {
        ldomNode * item = getChildNode( startChildIndex );
        //if ( item->getDataIndex()==INDEX2 || item->getDataIndex()==INDEX1 ) {
        //    CRLog::trace("node %d is being moved", item->getDataIndex() );
        //}
        _children.remove( startChildIndex ); // + i
        item->_parentIndex = destination->getDataIndex();
        destination->addChild( item->getDataIndex() );
    }
    // TODO: renumber rest of children in necessary
/*#ifdef _DEBUG
    if ( !_document->checkConsistency( false ) )
        CRLog::error("after moveItemsTo");
#endif*/
}

ldomNode * ldomNode::findChildElement( lUInt16 idPath[] )
{
    if ( !this || !isElement() )
        return NULL;
    ldomNode * elem = this;
    for ( int i=0; idPath[i]; i++ ) {
        elem = elem->findChildElement( LXML_NS_ANY, idPath[i], -1 );
        if ( !elem )
            return NULL;
    }
    return elem;
}

ldomNode * ldomNode::findChildElement( lUInt16 nsid, lUInt16 id, int index )
{
    if ( !this || !isElement() )
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
ldomNode * ldomElement::insertChildElement( lUInt32 index, lUInt16 nsid, lUInt16 id )
{
    if (index>(lUInt32)_children.length())
        index = _children.length();
    ldomNode * elem = new ldomElement( _document, this, index, nsid, id );
    _children.insert( index, elem->getDataIndex() );
#if (LDOM_ALLOW_NODE_INDEX==1)
    // reindex tail
    for (int i=index; i<_children.length(); i++)
        _children[i]->setIndex( i );
#endif
    return elem;
}

/// inserts child element
ldomNode * ldomElement::insertChildElement( lUInt16 id )
{
    ldomNode * elem = new ldomElement( _document, this, _children.length(), LXML_NS_NONE, id );
    _children.add( elem->getDataIndex() );
    return elem;
}

/// inserts child text
ldomNode * ldomElement::insertChildText( lUInt32 index, lString16 value )
{
    if (index>(lUInt32)_children.length())
        index = _children.length();
#if BUILD_LITE!=1
    ldomPersistentText * text = new ldomPersistentText( this, index, value );
#else
    ldomText * text = new ldomText( this, index, value );
#endif
    _children.insert( index, text->getDataIndex() );
#if (LDOM_ALLOW_NODE_INDEX==1)
    // reindex tail
    for (int i=index; i<_children.length(); i++)
        _children[i]->setIndex( i );
#endif
    return text;
}

/// inserts child text
ldomNode * ldomElement::insertChildText( lString16 value )
{
#if BUILD_LITE!=1
    ldomPersistentText * text = new ldomPersistentText( this, _children.length(), value );
#else
    ldomText * text = new ldomText( this, _children.length(), value );
#endif
    _children.add( text->getDataIndex() );
    return text;
}

ldomNode * ldomElement::removeChild( lUInt32 index )
{
    if ( index>(lUInt32)_children.length() )
        return NULL;
    ldomNode * node = _document->getNodeInstance( _children[index] );
    _children.remove(index);
#if (LDOM_ALLOW_NODE_INDEX==1)
    for (int i=index; i<_children.length(); i++)
        _children[i]->setIndex( i );
#endif
    return node;
}

/// calls specified function recursively for all elements of DOM tree
void ldomNode::recurseElements( void (*pFun)( ldomNode * node ) )
{
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

/// calls specified function recursively for all nodes of DOM tree
void ldomNode::recurseNodes( void (*pFun)( ldomNode * node ) )
{
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

/// returns attribute value by attribute name id and namespace id
const lString16 & ldomElement::getAttributeValue( lUInt16 nsid, lUInt16 id ) const
{
    lUInt16 val_id = _attrs.get( nsid, id );
    if (val_id!=LXML_ATTR_VALUE_NONE)
        return _document->getAttrValue( val_id );
    else
        return lString16::empty_str;
}

/// sets attribute value
void ldomElement::setAttributeValue( lUInt16 nsid, lUInt16 id, const lChar16 * value )
{
    lUInt16 valueId = _document->getAttrValueIndex( value );
    _attrs.set(nsid, id, valueId);
    if (nsid == LXML_NS_NONE)
        _document->onAttributeSet( id, valueId, this );
}

/// returns child node by index
ldomNode * ldomElement::getChildNode( lUInt32 index ) const
{
    return _document->getNodeInstance( _children[index] );
}

/// returns render data structure
lvdomElementFormatRec * ldomElement::getRenderData()
{
    if ( !_renderData )
        _renderData = new lvdomElementFormatRec;
    return _renderData;
}

/// sets node rendering structure pointer
void ldomElement::clearRenderData()
{
    if (_renderData)
        delete _renderData;
}

ldomElement::~ldomElement()
{
    if (_renderData)
        delete _renderData;
    for ( int i=0; i<_children.length(); i++ ) {
        ldomNode * child = _document->getNodeInstance( _children[i] );
        if ( child )
            delete child;
    }
    _document->unregisterNode( this );
}


#if (LDOM_USE_OWN_MEM_MAN==1)
extern ldomMemManStorage * block_storages[LOCAL_STORAGE_COUNT];
static void freeStorage( ldomMemManStorage * & p )
{
    if ( p ) {
        delete p;
        p = NULL;
    }
}

void ldomFreeStorage()
{
    for ( int i=0; i<LOCAL_STORAGE_COUNT; i++ )
        freeStorage( block_storages[i] );
    freeStorage( pmsREF );
    freeStorage( ldomElement::pmsHeap );
    freeStorage( ldomText::pmsHeap );
#if BUILD_LITE!=1
    freeStorage( ldomPersistentText::pmsHeap );
    freeStorage( ldomPersistentElement::pmsHeap );
#endif
    freeStorage( lvdomElementFormatRec::pmsHeap );
    free_ls_storage();
}
#endif


#if BUILD_LITE!=1
static const char * doc_file_magic = "CoolReader3 Document Cache File\nformat version 3.01.04\n";


bool ldomDocument::DocFileHeader::serialize( SerialBuf & hdrbuf )
{
    int start = hdrbuf.pos();
    hdrbuf.putMagic( doc_file_magic );
    hdrbuf << src_file_size << src_file_crc32;
    hdrbuf << props_offset << props_size;
    hdrbuf << idtable_offset << idtable_size;
    hdrbuf << pagetable_offset << pagetable_size;
    hdrbuf << data_offset << data_size << data_crc32 << data_index_size << file_size;
    hdrbuf << render_dx << render_dy << render_docflags << render_style_hash;
    hdrbuf << src_file_name;

    hdrbuf.putCRC( hdrbuf.pos() - start );
    return !hdrbuf.error();
}

bool ldomDocument::DocFileHeader::deserialize( SerialBuf & hdrbuf )
{
    int start = hdrbuf.pos();
    hdrbuf.checkMagic( doc_file_magic );
    if ( hdrbuf.error() ) {
        CRLog::error("Swap file Magic signature doesn't match");
        return false;
    }
    hdrbuf >> src_file_size >> src_file_crc32;
    hdrbuf >> props_offset >> props_size;
    hdrbuf >> idtable_offset >> idtable_size;
    hdrbuf >> pagetable_offset >> pagetable_size;
    hdrbuf >> data_offset >> data_size >> data_crc32 >> data_index_size >> file_size;
    hdrbuf >> render_dx >> render_dy >> render_docflags >> render_style_hash;
    hdrbuf >> src_file_name;
    hdrbuf.checkCRC( hdrbuf.pos() - start );
    if ( hdrbuf.error() ) {
        CRLog::error("Swap file - header unpack error");
        return false;
    }
    return true;
}
#endif

#ifdef _DEBUG
#if BUILD_LITE!=1


bool testTreeConsistency( ldomNode * base, int & count, int * flags )
{
    bool res = true;
    int cnt = base->getChildCount();
    for ( int i=0; i<cnt; i++ ) {
        ldomNode * node = base->getChildNode(i);
        if ( node == NULL ) {
            // child node not found
            CRLog::error("child node %d of node %d not found!", i, base->getDataIndex() );
            res = false;
        } else if ( !testTreeConsistency( node, count, flags) ) {
            flags[ node->getDataIndex() ]++;
            // child node not found
            CRLog::error("inconsistency in child node %d of node %d", i, base->getDataIndex() );
            res = false;
        } else {
        }
    }
    if ( flags[ base->getDataIndex() ] ) {
        CRLog::error( "Node %d reached via tree twice!", base->getDataIndex() );
    }
    flags[ base->getDataIndex() ]++;
    count++;
    return res;
}

///debug method, for DOM tree consistency check, returns false if failed
bool lxmlDocBase::checkConsistency( bool requirePersistent )
{
    bool res = true;
    //test1: 
    for ( int i=_instanceMapCount; i<_instanceMapSize && i<_instanceMapCount+5; i++ ) {
        if ( _instanceMap[i].instance != NULL || _instanceMap[i].data != NULL ) {
            CRLog::error("instance map tail is not empty (index=%d, len=%d, size=%d)", i, _instanceMapCount, _instanceMapSize);
            res = false;
        }
    }

    LVArray<int> dataIndexCount( _instanceMapCount, 0 );
    LVArray<int> dataIndexInstanceFlag( _instanceMapCount, 0 );
    int elemcount = 0;
    int textcount = 0;
    int itemcount = 0;
    for ( int buf=0; buf<this->_dataBuffers.length(); buf++ )
    for ( DataStorageItemHeader * item = _dataBuffers[buf]->first(); item!=NULL; item = _dataBuffers[buf]->next(item) ) {
        itemcount++;
    //CRLog::trace("checking item %d: %08x", itemcount, (unsigned) item );
        if ( item->type==LXML_ELEMENT_NODE ) {
            //if ( item->dataIndex==INDEX1 || item->dataIndex==INDEX2 ) {
            //    CRLog::debug("item with index %d: element name=%s", item->dataIndex, UnicodeToUtf8(_instanceMap[item->dataIndex].instance->getNodeName()).c_str() );
            //}
            dataIndexCount[ item->dataIndex ]++;
            elemcount++;
            if ( _instanceMap[item->dataIndex].data != item ) {
                CRLog::error( "Data pointer doesn't match for element %d", item->dataIndex);
                res = false;
            }
            
        } else if ( item->type==LXML_TEXT_NODE ) {
            dataIndexCount[ item->dataIndex ]++;
            textcount++;
            if ( _instanceMap[item->dataIndex].data != item ) {
                CRLog::error( "Data pointer doesn't match for text %d, diff is %d  %p->%p", item->dataIndex, (int)((char *)_instanceMap[item->dataIndex].data - (char *)item), _instanceMap[item->dataIndex].data, item);
                res = false;
            }
        } else 
            continue;
        if ( dataIndexCount[ item->dataIndex ]>1 ) {
            CRLog::error("Number of data records is %d for index %d", dataIndexCount[ item->dataIndex ], item->dataIndex );
            res = false;
        }
    }
    for ( int i=0; i<_instanceMapCount; i++ ) {
        if ( dataIndexCount[ i ]>1 ) {
            CRLog::error("ldomDocument::checkConsistency() - item with index %d has %d data records", i, dataIndexCount[ i ] );
            res = false;
        }
    }
    int cnt = 0;
    testTreeConsistency( getRootNode(), cnt, dataIndexInstanceFlag.get() );
    if ( (requirePersistent && cnt != elemcount+textcount) || (!requirePersistent && cnt < elemcount+textcount) ) {
        CRLog::error( "Data storage item count is %d but tree item count is %d", elemcount + textcount, cnt );
        res = false;
    }
    int mapitemcount = 0;
    int persistentmapcount = 0;
    for ( int i=0; i<_instanceMapCount; i++ ) {
        if ( _instanceMap[i].instance && dataIndexInstanceFlag[i]!=1 ) {
            CRLog::error( "Instance for index %d exists in map, but reachable via tree %d times", i, dataIndexInstanceFlag[i] );
            res = false;
        }
        if ( requirePersistent && dataIndexInstanceFlag[i]!=dataIndexCount[i] ) {
            CRLog::error( "%s Instance for index %d exists in map, but is not reachable from data (flag=%d, persistent=%d)", _instanceMap[i].instance->isElement() ? "element" : "text",  i, dataIndexCount[i], _instanceMap[i].instance->isPersistent()?1:0 );
            res = false;
        }
        if ( requirePersistent && _instanceMap[i].instance && !_instanceMap[i].data ) {
            CRLog::error( "Instance for index %d exists in map, but doesn't have data pointer (not persistent?)", i );
            res = false;
        }
        if ( _instanceMap[i].instance ) {
            mapitemcount++;
            if ( _instanceMap[i].instance->isPersistent() ) {
                persistentmapcount++;
                if ( _instanceMap[i].data==NULL ) {
                    CRLog::error("No Data found for persistent item at index %d", i);
                    res = false;
                } else if ( _instanceMap[i].data->type==LXML_NO_DATA ) {
                    CRLog::error("Data pointer points to NO_DATA area for persistent item at index %d", i);
                    res = false;
                } else if (_instanceMap[i].data->type!=_instanceMap[i].instance->getNodeType()) {
                    CRLog::error("Node type doesn't match for index %d", i);
                    res = false;
                }
            } else {
                if ( requirePersistent ) {
                    CRLog::error("Non-persistent item found at index %d", i);
                    res = false;
                }
                if ( _instanceMap[i].data!=NULL ) {
                    CRLog::error("Data found for Non-persistent item at index %d", i);
                    res = false;
                }

            }
        }
    }
    if ( mapitemcount!=cnt ) {
        CRLog::error("Map item count=%d, tree item count=%d, data item count=%d", mapitemcount, cnt, elemcount+textcount);
        res = false;
    }
    if ( !res )
        CRLog::error( "checkConsistency() failed - %d elements and %d text nodes, Map item count=%d(%d persist), tree item count=%d, data item count=%d", elemcount, textcount, mapitemcount, persistentmapcount, cnt, elemcount+textcount );
    else if ( requirePersistent ) {
        //CRLog::warn( "checkConsistency() passed - %d elements and %d text nodes", elemcount, textcount );
    }
    return res;
}

#endif
#endif

int ldomDocument::getPersistenceFlags()
{
    int format = getProps()->getIntDef(DOC_PROP_FILE_FORMAT, 0);
    int flag = ( format==2 && getDocFlag(DOC_FLAG_PREFORMATTED_TEXT) ) ? 1 : 0;
    return flag;
}

#if BUILD_LITE!=1
bool ldomDocument::openFromCache( )
{
    lString16 fname = getProps()->getStringDef( DOC_PROP_FILE_NAME, "noname" );
    //lUInt32 sz = getProps()->getIntDef( DOC_PROP_FILE_SIZE, 0 );
    lUInt32 crc = getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);
    CRLog::info("ldomDocument::openFromCache() - Started restoring of document %s from cache file", UnicodeToUtf8(fname).c_str() );
    //doc_format_txt==2 TODO:
    LVStreamRef map = ldomDocCache::openExisting( fname, crc, getPersistenceFlags() );
    if ( map.isNull() ) {
        CRLog::error("Document %s is not found in cache", UnicodeToUtf8(fname).c_str() );
        return false;
    }
    lUInt32 fileSize = (lUInt32)map->GetSize();
    LVStreamBufferRef buf = map->GetWriteBuffer( 0, fileSize );
    if ( buf.isNull() ) {
        CRLog::error("Cannot map file to read/write buffer");
        return false;
    }
    lUInt8 * ptr = buf->getReadWrite();
    if ( ptr==NULL )
        return false;
    {
        SerialBuf hdrbuf( ptr, fileSize );
        if ( !hdr.deserialize( hdrbuf ) )
            return false;
        if ( hdr.file_size > fileSize ) {
            CRLog::error("Swap file - file size doesn't match");
            return false;
        }
        if ( hdr.data_offset >= fileSize || hdr.data_offset+hdr.data_size > fileSize )
            return false;
        // data crc32
        lUInt32 crc = lStr_crc32(0,  ptr + hdr.data_offset, hdr.data_size );
        if ( crc!=hdr.data_crc32 ) {
            CRLog::error("Swap file - CRC32 not matched for DOM data (%08x expected, %08x actual)", hdr.data_crc32, crc );
            return false;
        }
        // TODO: add more checks here
    }

    {
        SerialBuf propsbuf( ptr + hdr.props_offset, hdr.props_size );
        getProps()->deserialize( propsbuf );
        if ( propsbuf.error() ) {
            CRLog::error("Cannot read property table for document");
            return false;
        }

        SerialBuf idbuf( ptr + hdr.idtable_offset, hdr.idtable_size );
        deserializeMaps( idbuf );
        if ( idbuf.error() ) {
            CRLog::error("Cannot read ID table for document");
            return false;
        }

        SerialBuf pagebuf( ptr + hdr.pagetable_offset, hdr.pagetable_size );
        pagebuf.setPos( hdr.pagetable_size );
        _pagesData.setPos( 0 );
        _pagesData << pagebuf;
        _pagesData.setPos( 0 );
        LVRendPageList pages;
        pages.deserialize(_pagesData);
        if ( _pagesData.error() ) {
            CRLog::error("Page data deserialization is failed");
            return false;
        }
        _pagesData.setPos( 0 );
    }

    {
        _dataBuffers.clear();
        _dataBufferSize = fileSize-hdr.data_offset;
        _currentBuffer = new DataBuffer( ptr + hdr.data_offset, _dataBufferSize, hdr.data_size );
        _dataBuffers.add( _currentBuffer );
    }
    {
        //LVHashTable<lUInt16,lInt32> _idNodeMap
        if ( _instanceMap ) {
            for ( int i=0; i<_instanceMapCount; i++ ) {
                if ( _instanceMap[i].instance != NULL ) {
                    delete _instanceMap[i].instance;
                }
            }
            free( _instanceMap );
        }
        _instanceMapCount = hdr.data_index_size;
        _instanceMapSize = _instanceMapCount + 64;
        _instanceMap = (NodeItem *)(malloc( _instanceMapSize * sizeof(NodeItem) ));
        memset( _instanceMap, 0, _instanceMapSize * sizeof(NodeItem) );
        //_idNodeMap.clear();
        //_idNodeMap.resize( hdr.data_index_size );
        int elemcount = 0;
        int textcount = 0;
        for ( DataStorageItemHeader * item = _currentBuffer->first(); item!=NULL; item = _currentBuffer->next(item) ) {
            if ( item->type==LXML_ELEMENT_NODE ) {
                //ldomPersistentElement * elem = 
                new ldomPersistentElement( this, (ElementDataStorageItem*)item );
                //setNode( item->dataIndex, elem, item );
                elemcount++;
            } else if ( item->type==LXML_TEXT_NODE ) {
                //ldomPersistentText * text = 
                new ldomPersistentText( this, (TextDataStorageItem*)item );
                //setNode( item->dataIndex, text, item );
                textcount++;
            }
        }
        CRLog::trace("%d elements and %d text nodes (%d total) are read from disk (file size = %d)", elemcount, textcount, elemcount+textcount, (int)fileSize);
    }
#ifdef _DEBUG
    checkConsistency( true );
#endif

    _map = map;
    _mapbuf = buf;
    CRLog::info("ldomDocument::openFromCache() - read successfully");
    return true;
}

/// change size of memory mapped buffer
bool ldomDocument::resizeMap( lvsize_t newSize )
{
    if ( !_mapped || !_mapbuf || !_mapped )
        return false;
    lUInt8 * oldptr = _mapbuf->getReadWrite();
    _mapbuf.Clear();
    if ( _map->SetSize( newSize )!=LVERR_OK ) {
        _map.Clear();
        _mapped = false;
        CRLog::error("Error while resizing mmap file");
        return false;
    }
    _mapbuf = _map->GetWriteBuffer(0, newSize);
    lUInt8 * newptr = _mapbuf->getReadWrite();
    ptrdiff_t diff = newptr - oldptr;
    CRLog::debug("Relocating data pointers after mapped file resize: by %d (0x%x)  %p->%p", (int)(diff),(int)(diff), oldptr, newptr);
    for (int i=0; i<_instanceMapCount; i++ ) {
        if ( _instanceMap[ i ].data != NULL )
            _instanceMap[ i ].data = (DataStorageItemHeader*) ( (lUInt8*)_instanceMap[ i ].data + diff );
    }

    _dataBufferSize = newSize-hdr.data_offset;
    _currentBuffer->relocatePtr( diff, _dataBufferSize );

    return true;
    //_map = map; // memory mapped file
    //_mapbuf = buf; // memory mapped file buffer
    //_mapped = true;
}

bool ldomDocument::swapToCache( lUInt32 reservedSize )
{
    lString16 fname = getProps()->getStringDef( DOC_PROP_FILE_NAME, "noname" );
    //lUInt32 sz = getProps()->getIntDef( DOC_PROP_FILE_SIZE, 0 );
    lUInt32 crc = getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);
    if ( !_map.isNull() ) {
        // already in map file
        return true;
    }
    if ( !ldomDocCache::enabled() ) {
        return false;
    }

    //testTreeConsistency( getRootNode() );
    CRLog::info("ldomDocument::swapToCache() - Started swapping of document %s to cache file", UnicodeToUtf8(fname).c_str() );

    //if ( !reservedSize )
    //    persist(); //!!!


    //testTreeConsistency( getRootNode() );

    lvsize_t datasize = 0;
    for ( int i=0; i<_dataBuffers.length(); i++ ) {
        datasize += _dataBuffers[i]->length();
    }

    hdr.src_file_size = (lUInt32)getProps()->getInt64Def(DOC_PROP_FILE_SIZE, 0);
    hdr.src_file_crc32 = (lUInt32)getProps()->getIntDef(DOC_PROP_FILE_CRC32, 0);
    hdr.src_file_name = getProps()->getStringDef(DOC_PROP_FILE_NAME, "");

    SerialBuf propsbuf(4096);
    getProps()->serialize( propsbuf );
    int propssize = propsbuf.pos() + 4096;
    propssize = (propssize + 4095) / 4096 * 4096;

    SerialBuf idbuf(4096);
    serializeMaps( idbuf );
    int idsize = idbuf.pos() * 4 + 4096;
    idsize = (idsize + 4095) / 4096 * 4096;

    int pagesize = _pagesData.size();
    if ( (unsigned)pagesize < hdr.src_file_size/1000 * 20 )
        pagesize = hdr.src_file_size/1000 * 20 / 4096 * 4096 + 16384;

    int pos = 0;
    int hdrsize = 4096; // max header size
    pos += hdrsize;

    hdr.props_offset = pos;
    hdr.props_size = propssize;
    pos += propssize;

    hdr.idtable_offset = pos;
    hdr.idtable_size = idsize;
    pos += idsize;

    hdr.pagetable_offset = pos;
    hdr.pagetable_size = pagesize;
    pos += pagesize;

    hdr.data_offset = pos;
    hdr.data_size = datasize;
    hdr.data_index_size = _instanceMapCount;

    hdr.file_size = pos + (reservedSize > datasize ? reservedSize : datasize );
    lUInt32 reserved = (hdr.file_size) + 8192; // 8K + 1/16
    hdr.file_size += reserved;

    LVStreamRef map = ldomDocCache::createNew( fname, crc, getPersistenceFlags(), hdr.file_size );
    if ( map.isNull() )
        return false;

    LVStreamBufferRef buf = map->GetWriteBuffer( 0, hdr.file_size );
    if ( buf.isNull() )
        return false;

    lUInt8 * ptr = buf->getReadWrite();
    if ( ptr==NULL )
        return false;

    hdr.data_crc32 = 0;
    // copy data
    for ( int i=0; i<_dataBuffers.length(); i++ ) {
        lUInt32 len = _dataBuffers[i]->length();
        memcpy( ptr + pos, _dataBuffers[i]->ptr(), len );
        hdr.data_crc32 = lStr_crc32( hdr.data_crc32, ptr+pos, len );
        pos += len;
    }
    CRLog::info( "ldomDocument::swapToCache() - data CRC is %08x, max itemId=%d", hdr.data_crc32, _instanceMapCount );

    SerialBuf hdrbuf(4096);
    if ( !hdr.serialize( hdrbuf ) )
        return false;

    hdrbuf.copyTo( ptr, hdrbuf.pos() );
    propsbuf.copyTo( ptr + hdr.props_offset, hdr.props_size );
    if ( idbuf.pos() > (int)hdr.idtable_size ) {
        CRLog::error("ID buffer size is too small");
    }
    idbuf.copyTo( ptr + hdr.idtable_offset, hdr.idtable_size );
    if ( _pagesData.pos() > (int)hdr.pagetable_size ) {
        CRLog::error("Page buffer size is too small");
    }
    _pagesData.copyTo( ptr + hdr.pagetable_offset, hdr.pagetable_size );


    //
    _dataBufferSize = hdr.file_size - hdr.data_offset;
    _currentBuffer = new DataBuffer( ptr + hdr.data_offset, _dataBufferSize, hdr.data_size );
    _dataBuffers.clear();
    _dataBuffers.add( _currentBuffer );
    int elemcount = 0;
    int textcount = 0;
    for ( DataStorageItemHeader * item = _currentBuffer->first(); item!=NULL; item = _currentBuffer->next(item) ) {
        if ( item->type==LXML_ELEMENT_NODE || item->type==LXML_TEXT_NODE ) {
            if ( item->type==LXML_ELEMENT_NODE )
                elemcount++;
            else if ( item->type==LXML_TEXT_NODE )
                textcount++;
            else
                continue;
            if ( !_instanceMap[ item->dataIndex ].instance ) {
                CRLog::error("No instance found for dataIndex=%d", item->dataIndex);
                continue;
            }
            //if ( item->dataIndex==INDEX2 || item->dataIndex==INDEX1) {
            //    CRLog::trace("changing pointer to node %d from %08x to %08x", item->dataIndex, (unsigned)_instanceMap[ item->dataIndex ].data, (unsigned)item );
            //}
            if ( item->dataIndex < _instanceMapCount )
                _instanceMap[ item->dataIndex ].data = item;
            else {
                CRLog::error("Out of range dataIndex=%d", item->dataIndex);
                _instanceMap[ item->dataIndex ].data = NULL;
            }
        }
    }
    CRLog::trace("%d elements and %d text nodes (%d total) are swapped to disk (file size = %d)", elemcount, textcount, elemcount+textcount, (int)hdr.file_size);


#ifdef _DEBUG
    checkConsistency( false );
#endif

    _map = map; // memory mapped file
    _mapbuf = buf; // memory mapped file buffer
    _mapped = true;
    CRLog::info("ldomDocument::swapToCache() - swapping of document to cache file finished successfully");
    return true;
}

/// saves recent changes to mapped file
bool ldomDocument::updateMap()
{
    if ( !_mapped || !_mapbuf )
        return false;
    //testTreeConsistency( getRootNode() );
    CRLog::info("ldomDocument::updateMap() - Saving recent changes to cache file");
    persist();
#ifdef _DEBUG
    checkConsistency( true );
#endif

    SerialBuf propsbuf(4096);
    getProps()->serialize( propsbuf );
    unsigned propssize = propsbuf.pos() + 4096;
    propssize = (propssize + 4095) / 4096 * 4096;

    SerialBuf idbuf(4096);
    serializeMaps( idbuf );
    unsigned idsize = idbuf.pos() + 4096;
    idsize = (idsize + 4095) / 4096 * 4096;

    if ( hdr.props_size < propssize )
        return false;
    if ( hdr.idtable_size < idsize )
        return false;

    hdr.data_index_size = _instanceMapCount;

    hdr.data_size = _currentBuffer->length();
    lUInt8 * ptr = _mapbuf->getReadWrite();
    // update crc32
    {
        _currentBuffer->length();
        lUInt32 pos = hdr.data_offset;
        hdr.data_crc32 = 0;
        lUInt32 len = _currentBuffer->length();
        hdr.data_crc32 = lStr_crc32( hdr.data_crc32, ptr+pos, len );

        CRLog::info( "ldomDocument::updateMap() - data CRC is %08x", hdr.data_crc32 );
    }

    SerialBuf hdrbuf(4096);
    if ( !hdr.serialize( hdrbuf ) )
        return false;

#ifdef _DEBUG
    checkConsistency( true);
#endif

    hdrbuf.copyTo( ptr, hdrbuf.pos() );
    propsbuf.copyTo( ptr + hdr.props_offset, propssize );
    if ( idbuf.pos() > (int)hdr.idtable_size ) {
        CRLog::error("ID buffer size is too small");
    }
    idbuf.copyTo( ptr + hdr.idtable_offset, idsize );
    if ( _pagesData.pos() > (int)hdr.pagetable_size ) {
        CRLog::error("Page buffer size is too small");
    }
    _pagesData.copyTo( ptr + hdr.pagetable_offset, hdr.pagetable_size );

#ifdef _DEBUG
    checkConsistency( true);
#endif

    CRLog::info("ldomDocument::updateMap() - Changes saved");
    return true;
}

#endif

static const char * doccache_magic = "CoolReader3 Document Cache Directory Index\nV1.00\n";

/// document cache
class ldomDocCacheImpl : public ldomDocCache
{
    lString16 _cacheDir;
    lvsize_t _maxSize;

    struct FileItem {
        lString16 filename;
        lUInt32 size;
    };
    LVPtrVector<FileItem> _files;
public:
    ldomDocCacheImpl( lString16 cacheDir, lvsize_t maxSize )
        : _cacheDir( cacheDir ), _maxSize( maxSize )
    {
        LVAppendPathDelimiter( _cacheDir );
    }

    bool writeIndex()
    {
        lString16 filename = _cacheDir + L"cr3cache.inx";
        LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        if ( !stream )
            return false;
        SerialBuf buf( 16384, true );
        buf.putMagic( doccache_magic );

        lUInt32 start = buf.pos();
        lUInt32 count = _files.length();
        buf << count;
        for ( unsigned i=0; i<count && !buf.error(); i++ ) {
            FileItem * item = _files[i];
            buf << item->filename;
            buf << item->size;
        }
        buf.putCRC( buf.pos() - start );

        if ( buf.error() )
            return false;
        if ( stream->Write( buf.buf(), buf.pos(), NULL )!=LVERR_OK )
            return false;
        return true;
    }

    bool readIndex(  )
    {
        lString16 filename = _cacheDir + L"cr3cache.inx";
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
            for ( unsigned i=0; i<count && !buf.error(); i++ ) {
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
                if ( !fn.endsWith(L".cr3") )
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
        char s[16];
        sprintf(s, ".%08x.%d.cr3", (unsigned)crc, (int)docFlags);
        return filename + lString16( s ); //_cacheDir + 
    }

    /// open existing cache file stream
    LVStreamRef openExisting( lString16 filename, lUInt32 crc, lUInt32 docFlags )
    {
        lString16 fn = makeFileName( filename, crc, docFlags );
        LVStreamRef res;
        if ( findFileIndex( fn ) < 0 ) {
            CRLog::error( "ldomDocCache::openExisting - File %s is not found in cache index", UnicodeToUtf8(fn).c_str() );
            return res;
        }
        res = LVMapFileStream( (_cacheDir+fn).c_str(), LVOM_APPEND, 0 );
        if ( !res ) {
            CRLog::error( "ldomDocCache::openExisting - File %s is listed in cache index, but cannot be opened", UnicodeToUtf8(fn).c_str() );
            return res;
        }
        lUInt32 fileSize = (lUInt32) res->GetSize();
        moveFileToTop( fn, fileSize );
        return res;
    }

    /// create new cache file
    LVStreamRef createNew( lString16 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize )
    {
        lString16 fn = makeFileName( filename, crc, docFlags );
        LVStreamRef res;
        if ( findFileIndex( fn ) >= 0 )
            LVDeleteFile( fn );
        reserve( fileSize );
        res = LVMapFileStream( (_cacheDir+fn).c_str(), LVOM_APPEND, fileSize );
        if ( !res ) {
            CRLog::error( "ldomDocCache::createNew - file %s is cannot be created", UnicodeToUtf8(fn).c_str() );
            return res;
        }
        lUInt32 sz = (lUInt32) res->GetSize();
        moveFileToTop( fn, sz );
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
LVStreamRef ldomDocCache::openExisting( lString16 filename, lUInt32 crc, lUInt32 docFlags )
{
    if ( !_cacheInstance )
        return LVStreamRef();
    return _cacheInstance->openExisting( filename, crc, docFlags );
}

/// create new cache file
LVStreamRef ldomDocCache::createNew( lString16 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize )
{
    if ( !_cacheInstance )
        return LVStreamRef();
    return _cacheInstance->createNew( filename, crc, docFlags, fileSize );
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

void calcStyleHash( ldomNode * node, lUInt32 & value )
{
    if ( !node )
        return;

    if ( node->isText() || node->getRendMethod()==erm_invisible ) {
        value = value * 75 + 1673251;
        return; // don't go through invisible nodes
    }

    css_style_ref_t style = node->getStyle();
    font_ref_t font = node->getFont();
    lUInt32 styleHash = (!style) ? 4324324 : calcHash( style );
    lUInt32 fontHash = (!font) ? 256371 : calcHash( font );
    value = (value*75 + styleHash) * 75 + fontHash;

    int cnt = node->getChildCount();
    for ( int i=0; i<cnt; i++ ) {
        calcStyleHash( node->getChildNode(i), value );
    }
}

#if BUILD_LITE!=1

/// save document formatting parameters after render
void ldomDocument::updateRenderContext( LVRendPageList * pages, int dx, int dy )
{
    lUInt32 styleHash = 0;
    calcStyleHash( getRootNode(), styleHash );
    hdr.render_style_hash = styleHash;
    hdr.render_dx = dx;
    hdr.render_dy = dy;
    hdr.render_docflags = _docFlags;
    _pagesData.reset();
    pages->serialize( _pagesData );
}

/// check document formatting parameters before render - whether we need to reformat; returns false if render is necessary
bool ldomDocument::checkRenderContext( LVRendPageList * pages, int dx, int dy )
{
    lUInt32 styleHash = 0;
    calcStyleHash( getRootNode(), styleHash );
    if ( styleHash == hdr.render_style_hash 
        && _docFlags == hdr.render_docflags
        && dx == (int)hdr.render_dx
        && dy == (int)hdr.render_dy ) {

        //if ( pages->length()==0 ) {
            _pagesData.reset();
            pages->deserialize( _pagesData );
        //}

        return true;
    }
    hdr.render_style_hash = styleHash;
    hdr.render_dx = dx;
    hdr.render_dy = dy;
    hdr.render_docflags = _docFlags;
    return false;
}

#endif

void lxmlDocBase::setStyleSheet( const char * css, bool replace )
{
    if ( replace ) {
        //CRLog::debug("cleaning stylesheet contents");
        _stylesheet.clear();
    }
    if ( css && *css ) {
        //CRLog::debug("appending stylesheet contents: \n%s", css);
        _stylesheet.parse( css );
    }
}

