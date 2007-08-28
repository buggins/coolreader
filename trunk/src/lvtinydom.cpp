/*******************************************************

   CoolReader Engine

   lvtinydom.cpp:  compact read-only XML DOM tree

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <string.h>
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvrend.h"

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

lxmlDocBase::lxmlDocBase()
: _elementNameTable(MAX_ELEMENT_TYPE_ID)
, _attrNameTable(MAX_ATTRIBUTE_TYPE_ID)
, _nsNameTable(MAX_NAMESPACE_TYPE_ID)
, _nextUnknownElementId(UNKNOWN_ELEMENT_TYPE_ID)
, _nextUnknownAttrId(UNKNOWN_ATTRIBUTE_TYPE_ID)
, _nextUnknownNsId(UNKNOWN_NAMESPACE_TYPE_ID)
, _attrValueTable( DOC_STRING_HASH_SIZE )
,_idNodeMap(1024)
,_idAttrId(0)
{
    _stylesheet.setDocument( this );
}

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



// memory pools
#if (LDOM_USE_OWN_MEM_MAN==1)
ldomMemManStorage * ldomElement::pmsHeap = NULL;
ldomMemManStorage * ldomText::pmsHeap = NULL;
ldomMemManStorage * lvdomElementFormatRec::pmsHeap = NULL;
#if COMPACT_DOM == 1
ldomMemManStorage * ldomTextRef::pmsHeap = NULL;
#endif
#endif

ldomNode::~ldomNode() { }

/// returns main element (i.e. FictionBook for FB2)
ldomElement * ldomDocument::getMainNode()
{ 
    if (!_root || !_root->getChildCount())
        return NULL;
    ldomElement * elem = ((ldomElement *)_root->getChildNode(_root->getChildCount()-1));
    if ( elem->getNodeType() != LXML_ELEMENT_NODE )
        return NULL;
    return elem;
}

#if COMPACT_DOM == 1
ldomDocument::ldomDocument(LVStreamRef stream)
: _textcache(stream, COMPACT_DOM_MAX_TEXT_FRAGMENT_COUNT, COMPACT_DOM_MAX_TEXT_BUFFER_SIZE)
{
    _root = new ldomElement( this, NULL, 0, 0, 0, 0 );
}

#else
ldomDocument::ldomDocument()
{
    _root = new ldomElement( this, NULL, 0, 0, 0, 0 );
}
#endif

static void writeNode( LVStream * stream, ldomNode * node )
{
    if ( node->getNodeType() == LXML_TEXT_NODE )
    {
        lString16 txt = node->getText();
        *stream << txt;
    }
    else if (  node->getNodeType() == LXML_ELEMENT_NODE )
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

bool ldomDocument::saveToStream( LVStreamRef stream, const char * codepage )
{
    if (!stream || !getRootNode()->getChildCount())
        return false;

    *stream.get() << L"\xFEFF"; 
    writeNode( stream.get(), getRootNode() );
    return true;
}

ldomDocument::~ldomDocument()
{
    delete _root;
}

#if COMPACT_DOM == 1
lString16 ldomDocument::getTextNodeValue( const ldomTextRef * txt )
{
    return _textcache.getText( txt->fileOffset, txt->dataSize, txt->dataFormat );
}

void ldomTextRef::setText( lString16 value )
{
    // CAUTION! this node will be deleted and replaced by ldomText!!!
    int index = ldomNode::getNodeIndex();
    ldomNode * self = _parent->removeChild( index );
    _parent->insertChildText( index, value );
    delete self; // == this !!!
}
#endif

#ifndef BUILD_LITE
int ldomDocument::render( LVRendPageContext & context, int width, int y0, font_ref_t def_font, int def_interline_space )
{
    _page_height = context.getPageHeight();
    _def_font = def_font;
    _def_style = css_style_ref_t( new css_style_rec_t );
    _def_style->display = css_d_block;
    _def_style->white_space = css_ws_normal;
    _def_style->text_align = css_ta_left;
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
    getMainNode()->recurseElements( initFormatData );
    initRendMethod( getMainNode() );
    //updateStyles();
    int height = renderBlockElement( context, getMainNode(), 
        0, y0, width ) + y0;
    gc();
    context.Finalize();
    return height;
}
#endif

void lxmlDocBase::setNodeTypes( const elem_def_t * node_scheme )
{
    for ( ; node_scheme && node_scheme->id != 0; ++node_scheme )
    {
        _elementNameTable.AddItem( 
            node_scheme->id,               // ID
            lString16(node_scheme->name),  // Name
            (const void*)node_scheme );  // ptr
    }
}

// set attribute types from table
void lxmlDocBase::setAttributeTypes( const attr_def_t * attr_scheme )
{
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

/// returns node absolute rectangle
void ldomElement::getAbsRect( lvRect & rect )
{
    ldomElement * node = this;
    lvdomElementFormatRec * fmt = node->getRenderData();
    rect.left = 0;
    rect.top = 0;
    rect.right = fmt->getWidth();
    rect.bottom = fmt->getHeight();
    for (;!node->isRoot();node = node->getParentNode()) 
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

#ifndef BUILD_LITE
LVImageSourceRef ldomElement::getObjectImageSource()
{
    //printf("ldomElement::getObjectImageSource() ... ");
    LVImageSourceRef ref;
    const elem_def_t * et = _document->getElementTypePtr(_id);
    if (!et || !et->props.is_object)
        return ref;
    lUInt16 hrefId = _document->getAttrNameIndex(L"href");
    lString16 refName = getAttributeValue( _document->getNsNameIndex(L"xlink"),
        hrefId );
    if ( refName.empty() )
        refName = getAttributeValue( _document->getNsNameIndex(L"l"), hrefId );
    if ( refName.empty() )
        refName = getAttributeValue( LXML_NS_NONE, hrefId );
    if ( refName.length()<2 )
        return ref;
    if ( refName[0]!='#' )
        return ref;
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

/////////////////////////////////////////////////////////////////
/// lxmlNodeRef

#if 0
const lString16 & ldomNode *::getAttributeValue( lUInt16 nsid, lUInt16 id ) const
{
    DEF_ELEM_PTR(elem);
    if (elem->nodeType!=LXML_ELEMENT_NODE)
        return lString16::empty_str;
    lxmlAttribute * attrs = elem->getAttributes();
    for (lUInt32 i = elem->attrCount; i>0; i--, attrs++)
    {
        if (attrs->id == id && (nsid==LXML_NS_ANY) || (attrs->nsid==nsid) )
            return _document->getAttrValue( attrs->index );
    }
    return lString16::empty_str;
}

void ldomNode *::getAbsRect( lvRect & rect )
{
    ldomNode * node = *this;
    lvdomElementFormatRec * fmt = node->getRenderData();
    rect.left = 0;
    rect.top = 0;
    rect.right = fmt->getWidth();
    rect.bottom = fmt->getHeight();
    for (;!node->isRoot();node = node->getParentNode()) 
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

lxmlAttrRef ldomNode *::getAttribute( lUInt32 index ) const
{
    DEF_ELEM_PTR(elem);
    if (elem->nodeType!=LXML_ELEMENT_NODE || index>=elem->attrCount)
        return lxmlAttrRef();
    return lxmlAttrRef( _document, _offset + sizeof(lxmlElement) 
        + index*sizeof(lxmlAttribute) );
}

bool ldomNode *::hasAttribute( lUInt16 nsid, lUInt16 id ) const
{
    DEF_ELEM_PTR(elem);
    if (elem->nodeType != LXML_ELEMENT_NODE)
        return false;
    lxmlAttribute * attrs = elem->getAttributes();
    for (lUInt32 i = elem->attrCount; i>0; i--, attrs++)
    {
        if (attrs->id == id && ((nsid==LXML_NS_ANY) || (attrs->nsid==nsid)) )
            return true;
    }
    return false;
}

/// returns node index
lUInt32 ldomNode *::getNodeIndex() const
{
    if (isNull())
        return 0;
    ldomNode * parent = getParentNode();
    if (parent->isNull())
        return 0;
    lxmlElement * elem = (lxmlElement *)_document->getNodePtr(parent._offset);
    if ( elem->nodeType!=LXML_ELEMENT_NODE || elem->childIndex==0 )
        return 0;
    lUInt32 * children = _document->getOffsetArray( elem->childIndex );    
    for (lUInt32 i=0; i<elem->childCount; i++)
        if (children[i] == _offset)
            return i;
    return 0;
}


/////////////////////////////////////////////////////////////////
/// lxmlElementWriter

lxmlElementWriter::lxmlElementWriter(lxmlDocument * document, lUInt16 nsid, lUInt16 id, lxmlElementWriter * parent)
    : _parent(parent), _document(document), _childIndex(NULL), _childIndexSize(0)
{
    //logfile << "{c";
    _typeDef = _document->getElementTypePtr( id );
    _allowText = _typeDef ? _typeDef->props.allow_text : (_parent?true:false);
    _offset  = _document->allocElement();
    lxmlElement * _element = getElement();
    _level = _parent ? _parent->_level+1 : 0;
    _element->nsid = nsid;
    _element->nodeId = id;
    _element->nodeType = LXML_ELEMENT_NODE;
    _element->nodeLevel = (lUInt8)_level;
    _element->childCount = 0;
    _element->childIndex = 0;
    _element->attrCount = 0;
    _element->renderData = NULL;
    if (_parent)
        _parent->addChild( _offset );
    _element->parent = _parent ? _parent->_offset : 0;
    //logfile << "}";
}

lUInt32 lxmlElementWriter::addChild( lUInt32 childOffset )
{
    lxmlElement * _element = getElement();
    if ( _element->childCount >= _childIndexSize )
    {
        _childIndexSize += 16;
        _childIndex = (lUInt32 *) realloc( _childIndex, sizeof(lUInt32) * _childIndexSize );
    }
    _childIndex[_element->childCount] = childOffset;
    return _element->childCount++;
}

void lxmlElementWriter::onText( const lChar16 * text, int len, 
    lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
    //logfile << "{t";
    lUInt32 offset = _document->allocText();
    lxmlText * ptr = _document->getTextPtr( offset );
    ptr->nodeType = LXML_TEXT_NODE;
    ptr->nodeLevel = (lUInt8)_level;
    ptr->dataFormat = flags;
    ptr->fileOffset = fpos;
    ptr->dataSize = (lUInt16)fsize;
    addChild( offset );
    ptr->parent = _offset;
    //logfile << "}";
}

void lxmlElementWriter::addAttribute( lUInt16 nsid, lUInt16 id, const wchar_t * value )
{
    _document->allocAttribute( nsid, id, value );
    getElement()->attrCount++;
}

/*
lxmlElementWriter * lxmlElementWriter::pop( lUInt16 id )
{
    logfile << "{p";
    lxmlElementWriter * tmp = this;
    for ( ; tmp; tmp = tmp->_parent )
    {
        logfile << "-";
        if (tmp->getElement()->nodeId == id)
            break;
    }
    logfile << "1";
    if (!tmp)
    {
        logfile << "-err}";
        return this; // error!!!
    }
    lxmlElementWriter * tmp2 = NULL;
    logfile << "2";
    for ( tmp = this; tmp; tmp = tmp2 )
    {
        logfile << "-";
        tmp2 = tmp->_parent;
        if (tmp->getElement()->nodeId == id)
            break;
        delete tmp;
    }
    logfile << "3";
    delete tmp;
    logfile << "}";
    return tmp2;
}

  */

lxmlElementWriter * pop( lxmlElementWriter * obj, lUInt16 id )
{
    //logfile << "{p";
    lxmlElementWriter * tmp = obj;
    for ( ; tmp; tmp = tmp->_parent )
    {
        //logfile << "-";
        if (tmp->getElement()->nodeId == id)
            break;
    }
    //logfile << "1";
    if (!tmp)
    {
        //logfile << "-err}";
        return obj; // error!!!
    }
    lxmlElementWriter * tmp2 = NULL;
    //logfile << "2";
    for ( tmp = obj; tmp; tmp = tmp2 )
    {
        //logfile << "-";
        tmp2 = tmp->_parent;
        if (tmp->getElement()->nodeId == id)
            break;
        delete tmp;
    }
    /*
    logfile << "3 * ";
    logfile << (int)tmp << " - " << (int)tmp2 << " | cnt=";
    logfile << (int)tmp->getElement()->childCount << " - "
            << (int)tmp2->getElement()->childCount;
    */
    delete tmp;
    //logfile << "}";
    return tmp2;
}

lxmlElementWriter::~lxmlElementWriter()
{
    //logfile << "{~";
    if (getElement()->childCount)
    {
        // on delete, append child index
        //logfile << "ci+" << getElement()->childCount;
        lUInt32 offset = _document->allocOffsetArray( _childIndex, getElement()->childCount );
        //logfile << " ^ " << offset;
        getElement()->childIndex = offset;
        //logfile << "~";
        delete _childIndex;
    }
    //logfile << "}";
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
/// lxmlDocumentWriter

// overrides

#if 0
void lxmlDocumentWriter::OnStart(LVXMLParser * parser)
{ 
    //logfile << "lxmlDocumentWriter::OnStart()\n";
    // add document root node
    LVXMLParserCallback::OnStart( parser );
    _currNode = new lxmlElementWriter(_document, 0, 0, NULL);
}

void lxmlDocumentWriter::OnStop()
{ 
    //logfile << "lxmlDocumentWriter::OnStop()\n";
    while (_currNode)
        _currNode = pop( _currNode, _currNode->getElement()->nodeId );
    _document->pack();
}

void lxmlDocumentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    //logfile << "lxmlDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    _currNode = new lxmlElementWriter( _document, nsid, id, _currNode );
    //logfile << " !o!\n";
}

void lxmlDocumentWriter::OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
{
    //logfile << "lxmlDocumentWriter::OnTagClose() [" << nsname << ":" << tagname << "]";
    if (!_currNode)
    {
        _errFlag = true;
        //logfile << " !c-err!\n";
        return;
    }
    lUInt16 id = _document->getElementNameIndex(tagname);
    //lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    _errFlag |= (id != _currNode->getElement()->nodeId);
    _currNode = pop( _currNode, id );
    //logfile << " !c!\n";
}

void lxmlDocumentWriter::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    //logfile << "lxmlDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
    lUInt16 attr_id = _document->getAttrNameIndex( attrname );
    _currNode->addAttribute( (nsname && nsname[0]) ? _document->getNsNameIndex( nsname ):0, attr_id, attrvalue );
    //logfile << " !a!\n";
}

void lxmlDocumentWriter::OnText( const lChar16 * text, int len, 
    lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
    //logfile << "lxmlDocumentWriter::OnText() fpos=" << fpos;
    if (_currNode)
    {
        if ( (_flags & XML_FLAG_NO_SPACE_TEXT) 
             && IsEmptySpace(text, len) )
             return;
        if (_currNode->_allowText)
            _currNode->onText( text, len, fpos, fsize, flags );
    }
    //logfile << " !t!\n";
}

void lxmlDocumentWriter::OnEncoding( const lChar16 * name, const lChar16 * table )
{ 
    if (table)
        _document->_textcache.SetCharsetTable( table );
}

lxmlDocumentWriter::lxmlDocumentWriter(lxmlDocument * document)
    : _document(document), _currNode(NULL), _errFlag(false), _flags(0)
{
}

#endif

/////////////////////////////////////////////////////////////////
/// lxmlElementWriter

ldomElementWriter::ldomElementWriter(ldomDocument * document, lUInt16 nsid, lUInt16 id, ldomElementWriter * parent)
    : _parent(parent), _document(document)
{
    //logfile << "{c";
    _typeDef = _document->getElementTypePtr( id );
    _allowText = _typeDef ? _typeDef->props.allow_text : (_parent?true:false);
    if (_parent)
        _element = _parent->getElement()->insertChildElement( (lUInt32)-1, nsid, id );
    else
        _element = _document->getRootNode()->insertChildElement( (lUInt32)-1, nsid, id );
    //logfile << "}";
}

void ldomElementWriter::onText( const lChar16 * text, int len, 
    lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
    //logfile << "{t";
#if (COMPACT_DOM == 1)
    if ( len >= COMPACT_DOM_MIN_REF_TEXT_LENGTH)
    {
        // compact mode: store reference to file
        _element->insertChildText(fpos, fsize, flags);
    }
    else
#endif
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

/*
lxmlElementWriter * lxmlElementWriter::pop( lUInt16 id )
{
    logfile << "{p";
    lxmlElementWriter * tmp = this;
    for ( ; tmp; tmp = tmp->_parent )
    {
        logfile << "-";
        if (tmp->getElement()->nodeId == id)
            break;
    }
    logfile << "1";
    if (!tmp)
    {
        logfile << "-err}";
        return this; // error!!!
    }
    lxmlElementWriter * tmp2 = NULL;
    logfile << "2";
    for ( tmp = this; tmp; tmp = tmp2 )
    {
        logfile << "-";
        tmp2 = tmp->_parent;
        if (tmp->getElement()->nodeId == id)
            break;
        delete tmp;
    }
    logfile << "3";
    delete tmp;
    logfile << "}";
    return tmp2;
}

  */

ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id )
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
        delete tmp;
    }
    /*
    logfile << "3 * ";
    logfile << (int)tmp << " - " << (int)tmp2 << " | cnt=";
    logfile << (int)tmp->getElement()->childCount << " - "
            << (int)tmp2->getElement()->childCount;
    */
    delete tmp;
    //logfile << "}";
    return tmp2;
}

ldomElementWriter::~ldomElementWriter()
{
}




/////////////////////////////////////////////////////////////////
/// ldomDocumentWriter

// overrides
void ldomDocumentWriter::OnStart(LVXMLParser * parser)
{ 
    //logfile << "ldomDocumentWriter::OnStart()\n";
    // add document root node
    LVXMLParserCallback::OnStart( parser );
    _currNode = new ldomElementWriter(_document, 0, 0, NULL);
}

void ldomDocumentWriter::OnStop()
{ 
    //logfile << "ldomDocumentWriter::OnStop()\n";
    while (_currNode)
        _currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
}

void ldomDocumentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
{
    //logfile << "ldomDocumentWriter::OnTagOpen() [" << nsname << ":" << tagname << "]";
    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;

    if ( id==_stopTagId )
        _parser->Stop();
    _currNode = new ldomElementWriter( _document, nsid, id, _currNode );
    //logfile << " !o!\n";
}

void ldomDocumentWriter::OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
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

void ldomDocumentWriter::OnText( const lChar16 * text, int len, 
    lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
    //logfile << "ldomDocumentWriter::OnText() fpos=" << fpos;
    if (_currNode)
    {
        if ( (_flags & XML_FLAG_NO_SPACE_TEXT) 
             && IsEmptySpace(text, len) )
             return;
        if (_currNode->_allowText)
            _currNode->onText( text, len, fpos, fsize, flags );
    }
    //logfile << " !t!\n";
}

void ldomDocumentWriter::OnEncoding( const lChar16 * name, const lChar16 * table )
{ 
#if COMPACT_DOM == 1
    if (table)
        _document->_textcache.SetCharset( name );
#endif
}

ldomDocumentWriter::ldomDocumentWriter(ldomDocument * document, bool headerOnly)
    : _document(document), _currNode(NULL), _errFlag(false), _flags(0)
{
    if ( !headerOnly )
        _stopTagId = 0xFFFE;
    else
        _stopTagId = _document->getElementNameIndex(L"body");
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
    ldomElement * parent = node->getParentNode();
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
   -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, //96..111  60
   41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1  //112..127 70
};

#define BASE64_BUF_SIZE 128
class LVBase64NodeStream : public LVNamedStream
{
private:
    ldomElement * m_elem;
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
    LVBase64NodeStream( ldomElement * element )
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
        if (npos < 0 || npos > m_size)
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
LVStreamRef ldomElement::createBase64Stream()
{
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

ldomDocument * ldomText::getDocument() const
{
    return _parent->getDocument();
}

#if COMPACT_DOM == 1
ldomDocument * ldomTextRef::getDocument() const
{
    return _parent->getDocument();
}
#endif

#if (LDOM_ALLOW_NODE_INDEX!=1)
lUInt32 ldomNode::getNodeIndex() const
{
    for (int i=_parent->getChildCount()-1; i>=0; i--)
        if (_parent->getChildNode(i)==this)
            return i;
    return -1;
}
#endif

/// returns text node text
lString16 ldomElement::getText( lChar16 blockDelimiter ) const
{
    lString16 txt;
    for ( unsigned i=0; i<getChildCount(); i++ ) {
        txt += getChildNode(i)->getText(blockDelimiter);
        ldomNode * child = getChildNode(i);
        if ( i>=getChildCount()-1 )
            break;
        if ( blockDelimiter && child->getNodeType()==LXML_ELEMENT_NODE ) {
            if ( ((ldomElement*)child)->getStyle()->display == css_d_block )
                txt << blockDelimiter;
        }
    }
    return txt;
}

/// get pointer for relative path
ldomXPointer ldomXPointer::relative( lString16 relativePath )
{
    return _node->getDocument()->createXPointer( _node, relativePath );
}
/// create xpointer from pointer string
ldomXPointer ldomDocument::createXPointer( const lString16 & xPointerStr )
{
    return createXPointer( getRootNode(), xPointerStr );
}

#ifndef BUILD_LITE
/// formats final block
int ldomElement::renderFinalBlock( LFormattedText & txtform, int width )
{
    lvdomElementFormatRec * fmt = getRenderData();
    if ( !fmt || getRendMethod() != erm_final )
        return 0;
    // render whole node content as single formatted object
    int flags = styleToTextFmtFlags( getStyle(), 0 );
    ::renderFinalBlock( this, &txtform, fmt, flags, 0, 16 );
    int page_h = getDocument()->getPageHeight();
    int h = txtform.Format( width, page_h );
    return h;
}
#endif

/// returns first text child element
ldomText * ldomNode::getFirstTextChild()
{
    if ( isText() )
        return (ldomText *)this;
    else {
        for ( int i=0; i<(int)getChildCount(); i++ ) {
            ldomText * p = getChildNode(i)->getFirstTextChild();
            if (p)
                return p;
        }
    }
    return NULL;
}

/// returns last text child element
ldomText * ldomNode::getLastTextChild()
{
    if ( isText() )
        return (ldomText *)this;
    else {
        for ( int i=(int)getChildCount()-1; i>=0; i-- ) {
            ldomText * p = getChildNode(i)->getLastTextChild();
            if (p)
                return p;
        }
    }
    return NULL;
}

#ifndef BUILD_LITE
ldomElement * ldomNode::elementFromPoint( lvPoint pt )
{
    if ( !isElement() )
        return NULL;
    ldomElement * enode = (ldomElement*) this;
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
        return (ldomElement*)this;
    }
    int count = getChildCount();
    for ( int i=0; i<count; i++ ) {
        ldomNode * p = getChildNode( i );
        ldomElement * e = p->elementFromPoint( lvPoint( pt.x - fmt->getX(), 
                pt.y - fmt->getY() ) );
        if ( e )
            return e;
    }
    return (ldomElement*)this;
}

ldomElement * ldomNode::finalBlockFromPoint( lvPoint pt )
{
    ldomElement * elem = elementFromPoint( pt );
    if ( elem && elem->getRendMethod() == erm_final )
        return elem;
    return NULL;
}

/// create xpointer from doc point
ldomXPointer ldomDocument::createXPointer( lvPoint pt )
{
    //
    ldomXPointer ptr;
    if ( !getMainNode() )
        return ptr;
    ldomElement * finalNode = getMainNode()->elementFromPoint( pt );
    if ( !finalNode )
        return ptr;
    lvRect rc;
    finalNode->getAbsRect( rc );
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
    LFormattedText txtform;
    finalNode->renderFinalBlock( txtform, r->getWidth() );
    int lcount = txtform.GetLineCount();
    for ( int l = 0; l<lcount; l++ ) {
        const formatted_line_t * frmline = txtform.GetLineInfo(l);
        if ( pt.y >= (int)(frmline->y + frmline->height) && l<lcount-1 )
            continue;
        // found line, searching for word
        int wc = (int)frmline->word_count;
        int x = pt.x - frmline->x;
        for ( int w=0; w<wc; w++ ) {
            const formatted_word_t * word = &frmline->words[w];
            if ( x < word->x + word->width || w==wc-1 ) {
                // found word, searching for letters
                const src_text_fragment_t * src = txtform.GetSrcInfo(word->src_text_index);
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
                font->measureText( str.c_str()+word->t.start, word->t.len, w, flg, word->width+50, '?');
                for ( int i=0; i<word->t.len; i++ ) {
                    int xx = ( i>0 ) ? (w[i-1] + w[i])/2 : w[i]/2;
                    if ( x < word->x + xx ) {
                        return ldomXPointer( node, word->t.start + i );
                    }
                }
                return ldomXPointer( node, word->t.start + word->t.len );
            }
        }
    }
    return ptr;
}

/// returns coordinates of pointer inside formatted document
lvPoint ldomXPointer::toPoint() const
{
    lvPoint pt(-1, -1);
    if ( !_node )
        return pt;
    ldomElement * p = _node->isElement() ? (ldomElement *)_node : _node->getParentNode();
    ldomElement * finalNode = NULL;
    ldomElement * mainNode = p->getDocument()->getMainNode();
    for ( ; p; p = p->getParentNode() ) {
        if ( p->getRendMethod() == erm_final ) {
            finalNode = p; // found final block
        } else if ( p->getRendMethod() == erm_invisible ) {
            return pt; // invisible !!!
        }
        if ( p==mainNode )
            break;
    }
    if ( finalNode!=NULL ) {
        lvRect rc;
        finalNode->getAbsRect( rc );
        lvdomElementFormatRec * r = finalNode->getRenderData();
        if ( !r )
            return pt;
        LFormattedText txtform;
        finalNode->renderFinalBlock( txtform, r->getWidth() );

        ldomNode * node = _node;
        int offset = _offset;
        if ( node->isElement() ) {
            bool flgFirst = true;
            if ( offset>=0 ) {
                //
                if ( offset>= (int)node->getChildCount() ) {
                    node = node->getChildNode( node->getChildCount()-1 );
                    flgFirst = false;
                } else {
                    node = node->getChildNode( offset );
                }
            }
            if ( node->isElement() ) {
                if ( flgFirst ) {
                    node = node->getFirstTextChild();
                    offset = 0;
                } else {
                    node = node->getLastTextChild();
                    if ( node )
                        offset = node->getText().length();
                }
                if ( !node )
                    return pt;
            }
        }

        // text node
        int srcIndex = -1;
        int srcLen = -1;
        for ( int i=0; i<txtform.GetSrcCount(); i++ ) {
            const src_text_fragment_t * src = txtform.GetSrcInfo(i);
            if ( src->object == node ) {
                srcIndex = i;
                srcLen = src->t.len;
                break;
            }
        }
        if ( srcIndex == -1 )
            return pt;
        for ( int l = 0; l<txtform.GetLineCount(); l++ ) {
            const formatted_line_t * frmline = txtform.GetLineInfo(l);
            for ( int w=0; w<(int)frmline->word_count; w++ ) {
                const formatted_word_t * word = &frmline->words[w];
                if ( word->src_text_index==srcIndex ) {
                    // found word from same src line
                    if ( _offset<=word->t.start ) {
                        // before this word
                        pt.x = word->x + rc.left + frmline->x;
                        pt.y = word->y + rc.top + frmline->y + frmline->baseline;
                        return pt;
                    } else if ( (offset<word->t.start+word->t.len) || (offset==srcLen && offset==word->t.start+word->t.len) ) {
                        // pointer inside this word
                        LVFont * font = (LVFont *) txtform.GetSrcInfo(srcIndex)->t.font;
                        lUInt16 w[512];
                        lUInt8 flg[512];
                        lString16 str = node->getText();
                        font->measureText( str.c_str()+word->t.start, offset - word->t.start, w, flg, word->width+50, '?');
                        int chx = w[ _offset - word->t.start - 1 ];
                        pt.x = word->x + chx + rc.left + frmline->x;
                        pt.y = word->y + rc.top + frmline->y + frmline->baseline;
                        return pt;
                    }
                } else if ( word->src_text_index>srcIndex ) {
                    return pt;
                }
            }
        }
        return pt;
    } else {
        // no base final node, using blocks
        lvRect rc;
        if ( _offset<0 || _node->getChildCount()==0 ) {
            _node->getAbsRect( rc );
            return rc.topLeft();
        }
        if ( _offset < (int)_node->getChildCount() ) {
            _node->getChildNode(_offset)->getAbsRect( rc );
            return rc.topLeft();
        }
        _node->getChildNode(_node->getChildCount()-1)->getAbsRect( rc );
        return rc.bottomRight();
    }
}
#endif

/// create xpointer from relative pointer string
ldomXPointer ldomDocument::createXPointer( ldomNode * baseNode, const lString16 & xPointerStr )
{
	if ( xPointerStr.empty() )
		return ldomXPointer();
	const lChar16 * str = xPointerStr.c_str();
	int index = -1;
	ldomNode * currNode = baseNode;
	lString16 name;
    lString8 ptr8 = UnicodeToUtf8(xPointerStr);
    const char * ptr = ptr8.c_str();
	xpath_step_t step_type;

	while ( *str ) {
		step_type = ParseXPathStep( str, name, index );
		switch (step_type ) {
		case xpath_step_error:
			// error
			return ldomXPointer();
		case xpath_step_element:
			// element of type 'name' with 'index'        /elemname[N]/
			{
				lUInt16 id = getElementNameIndex( name.c_str() );
				ldomNode * foundItem = NULL;
				int foundCount = 0;
				for (unsigned i=0; i<currNode->getChildCount(); i++) {
					ldomNode * p = currNode->getChildNode(i);
					if ( p->isElement() && p->getNodeId()==id ) {
						foundCount++;
						if ( foundCount==index || index==-1 ) {
							foundItem = p;
						}
					}
				}
				if ( foundItem==NULL || (index==-1 && foundCount>1) )
					return ldomXPointer(); // node not found
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
			if ( index<0 || index>=(int)currNode->getChildCount() )
				return ldomXPointer(); // node not found: invalid index
			currNode = currNode->getChildNode( index );
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
    if ( _offset>=0 ) {
        path << L"." << lString16::itoa(_offset);
    }
    ldomNode * p = _node;
    ldomNode * mainNode = _node->getDocument()->getRootNode();
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
    lvdomElementFormatRec * rd = this ? this->getMainNode()->getRenderData() : NULL;
    return ( rd ? rd->getHeight() + rd->getY() : 0 ); 
}

/// returns text between two XPointer positions
lString16 ldomXPointer::getRangeText( ldomXPointer endpos, lChar16 blockDelimiter, int maxTextLen )
{
    ldomXPointer pos = *this;
    lString16 res;
    if ( pos.getNode() == endpos.getNode() ) {
        if ( pos.getOffset() >= endpos.getOffset() )
            return res;
        if ( pos.getNode()->getNodeType()==LXML_TEXT_NODE ) {
            return pos.getNode()->getText().substr( pos.getOffset(), endpos.getOffset()-pos.getOffset() );
        }
    }
    return res;
}





lString16 extractDocAuthors( ldomDocument * doc )
{
    lString16 authors;
    for ( int i=0; i<16; i++) {
        lString16 path = lString16(L"/FictionBook/description/title-info/author[") + lString16::itoa(i+1) + L"]";
        ldomXPointer pauthor = doc->createXPointer(path);
        if ( !pauthor )
            break;
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
    ldomElement * series = (ldomElement*)doc->createXPointer(L"/FictionBook/description/title-info/sequence").getNode();
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

