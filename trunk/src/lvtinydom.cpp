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
,_docProps(LVCreatePropsContainer())
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

const lString16 & ldomNode::getAttributeValue( const lChar16 * nsName, const lChar16 * attrName ) const
{
    lUInt16 nsId = (nsName&&nsName[0]) ? getDocument()->getNsNameIndex( nsName ) : LXML_NS_ANY;
    lUInt16 attrId = getDocument()->getAttrNameIndex( attrName );
    return getAttributeValue( nsId, attrId );
}

ldomNode::~ldomNode() { }

/// returns main element (i.e. FictionBook for FB2)
ldomElement * ldomDocument::getMainNode()
{
    if (!_root || !_root->getChildCount())
        return NULL;
	return _root;
	/*
    //int elemCount = 0;
    ldomElement * lastElem = NULL;
    for ( unsigned i=0; i<_root->getChildCount(); i++) {
        ldomElement * el = ((ldomElement *)_root->getChildNode(i));
        if ( el->getNodeType() == LXML_ELEMENT_NODE )
            lastElem = el;
    }
    return lastElem;
	*/
}

#if COMPACT_DOM == 1
ldomDocument::ldomDocument(LVStreamRef stream, int min_ref_text_size)
: _textcache(stream, COMPACT_DOM_MAX_TEXT_FRAGMENT_COUNT, COMPACT_DOM_MAX_TEXT_BUFFER_SIZE),
             _min_ref_text_size(min_ref_text_size)
#if BUILD_LITE!=1
        , _renderedBlockCache( 32 )
#endif
        , _docFlags(DOC_FLAG_DEFAULTS)
{
    _root = new ldomElement( this, NULL, 0, 0, 0, 0 );
}

#else
ldomDocument::ldomDocument()
:
#if BUILD_LITE!=1
     _renderedBlockCache( 32 ),
#endif
        _docFlags(DOC_FLAG_DEFAULTS)
{
    _root = new ldomElement( this, NULL, 0, 0, 0, 0 );
}
#endif

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
{
}

/// creates empty document which is ready to be copy target of doc partial contents
ldomDocument::ldomDocument( ldomDocument & doc )
: lxmlDocBase(doc)
, _def_font(doc._def_font) // default font
, _def_style(doc._def_style)
, _page_height(doc._page_height)

#if COMPACT_DOM == 1
,_textcache(doc._textcache.getStream(), COMPACT_DOM_MAX_TEXT_FRAGMENT_COUNT, COMPACT_DOM_MAX_TEXT_BUFFER_SIZE)
    ,_min_ref_text_size(doc._min_ref_text_size)
#if BUILD_LITE!=1
        , _renderedBlockCache( 32 )
#endif
#else
    // COMPACT_DOM != 1
#if BUILD_LITE!=1
        , _renderedBlockCache( 32 )
#endif
#endif
, _docFlags(doc._docFlags)
, _container(doc._container)
, _codeBase(doc._codeBase)
{
}

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

#if 1
            if (!elemName.empty())
            {
				ldomElement * elem = ((ldomElement*)node);
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

bool ldomDocument::saveToStream( LVStreamRef stream, const char * codepage )
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
    delete _root;
}

#if COMPACT_DOM == 1
lString16 ldomDocument::getTextNodeValue( const ldomTextRef * txt )
{
    //CRLog::debug("getTextNodeValue(%d,%d,%d)", (int)txt->fileOffset, (int)txt->dataSize, (int)txt->dataFormat);
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

#if BUILD_LITE!=1
int ldomDocument::render( LVRendPageContext & context, int width, int y0, font_ref_t def_font, int def_interline_space )
{
    CRLog::trace("initializing default style...");
    _renderedBlockCache.clear();
    _page_height = context.getPageHeight();
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
    getMainNode()->recurseElements( initFormatData );
    CRLog::trace("init render method...");
    initRendMethod( getMainNode() );
    //updateStyles();
    CRLog::trace("rendering...");
    int height = renderBlockElement( context, getMainNode(),
        0, y0, width ) + y0;
#if 0 //def _DEBUG
    LVStreamRef ostream = LVOpenFileStream( "test_save_after_init_rend_method.xml", LVOM_WRITE );
    saveToStream( ostream, "utf-16" );
#endif
    gc();
    CRLog::trace("finalizing...");
    context.Finalize();
    return height;
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
            (const void*)node_scheme );  // ptr
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

/// returns node absolute rectangle
void ldomElement::getAbsRect( lvRect & rect )
{
    ldomElement * node = this;
    lvdomElementFormatRec * fmt = node->getRenderData();
    rect.left = 0;
    rect.top = 0;
    rect.right = fmt->getWidth();
    rect.bottom = fmt->getHeight();
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
LVImageSourceRef ldomElement::getObjectImageSource()
{
    //printf("ldomElement::getObjectImageSource() ... ");
    LVImageSourceRef ref;
    const elem_def_t * et = _document->getElementTypePtr(_id);
    if (!et || !et->props.is_object)
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
        _element = _document->getRootNode(); //->insertChildElement( (lUInt32)-1, nsid, id );
    //logfile << "}";
}

lUInt32 ldomElementWriter::getFlags()
{
    lUInt32 flags = 0;
    if ( _typeDef && _typeDef->props.white_space==css_ws_pre )
        flags |= TXTFLG_PRE;
    return flags;
}

void ldomElementWriter::onText( const lChar16 * text, int len,
    lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
    //logfile << "{t";
#if (COMPACT_DOM == 1)
    if ( _document->allowTextRefForSize( len ) )
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

ldomElement * ldomDocumentWriter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
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
    if ( !_parent )
        return 0;
    for (int i=_parent->getChildCount()-1; i>=0; i--)
        if (_parent->getChildNode(i)==this)
            return i;
    return (lUInt32)-1;
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

#if BUILD_LITE!=1
/// formats final block
int ldomElement::renderFinalBlock( LFormattedTextRef & txtform, int width )
{
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

#if BUILD_LITE!=1
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
    if ( !finalNode ) {
        if ( pt.y >= getFullHeight()) {
            ldomText * node = getMainNode()->getLastTextChild();
			return ldomXPointer(node,node ? node->getText().length() : 0);
        } else if ( pt.y <= 0 ) {
            ldomText * node = getMainNode()->getFirstTextChild();
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
    if ( !_node )
        return false;
    ldomElement * p = _node->isElement() ? (ldomElement *)_node : _node->getParentNode();
    ldomElement * finalNode = NULL;
    if ( !p ) {
        //CRLog::trace("ldomXPointer::getRect() - p==NULL");
    }
    //printf("getRect( p=%08X type=%d )\n", (unsigned)p, (int)p->getNodeType() );
    if ( !p->getDocument() ) {
        //CRLog::trace("ldomXPointer::getRect() - p->getDocument()==NULL");
    }
    ldomElement * mainNode = p->getDocument()->getMainNode();
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

        ldomNode * node = _node;
        int offset = _offset;
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
        if ( _offset<0 || _node->getChildCount()==0 ) {
            _node->getAbsRect( rect );
            return true;
            //return rc.topLeft();
        }
        if ( _offset < (int)_node->getChildCount() ) {
            _node->getChildNode(_offset)->getAbsRect( rect );
            return true;
            //return rc.topLeft();
        }
        _node->getChildNode(_node->getChildCount()-1)->getAbsRect( rect );
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
					if ( p->isElement() && p->getNodeId()==id ) {
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

void ldomXPointerEx::initIndex()
{
    int m[MAX_DOM_LEVEL];
    ldomNode * p = _node;
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
    ldomElement * p = _node->getParentNode();
    if ( !p || index < 0 || index >= (int)p->getChildCount() )
        return false;
    _node = p->getChildNode( index );
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
    ldomElement * p = _node->getParentNode();
    for ( int i=_indexes[_level-1] + 1; i<(int)_node->getChildCount(); i++ ) {
        if ( p->getChildNode( i )->getNodeType()==LXML_ELEMENT_NODE )
            return sibling( i );
    }
    return false;
}

/// move to previous sibling element
bool ldomXPointerEx::prevSiblingElement()
{
    if ( _level < 1 )
        return false;
    ldomElement * p = _node->getParentNode();
    for ( int i=_indexes[_level-1] - 1; i>=0; i-- ) {
        if ( p->getChildNode( i )->getNodeType()==LXML_ELEMENT_NODE )
            return sibling( i );
    }
    return false;
}

/// move to parent
bool ldomXPointerEx::parent()
{
    if ( _level<1 )
        return false;
    _node = _node->getParentNode();
    _level--;
    return true;
}

/// move to child #
bool ldomXPointerEx::child( int index )
{
    if ( _level >= MAX_DOM_LEVEL )
        return false;
    int count = _node->getChildCount();
    if ( index<0 || index>=count )
        return false;
    _indexes[ _level++ ] = index;
    _node = _node->getChildNode( index );
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
    if ( _node->getNodeType() != LXML_ELEMENT_NODE )
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
    if ( _node->getNodeType() != LXML_ELEMENT_NODE )
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
: _start( p, 0 ), _end( p, p->getNodeType()==LXML_TEXT_NODE ? p->getText().length() : p->getChildCount() ), _flags(1)
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
            words.add( ldomWord((ldomText*) _start.getNode(), offs, offs + pattern.length() ) );
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
    ldomElement * parent = getNearestCommonParent();
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
    if ( node->getNodeType() != LXML_TEXT_NODE )
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
ldomElement * ldomXRange::getNearestCommonParent()
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
        return (ldomElement *)start.getNode();
    return NULL;
}

bool ldomXPointerEx::ensureFinal()
{
    if ( ensureElement() )
        return false;
    int cnt = 0;
    int foundCnt = -1;
    ldomElement * e = (ldomElement*)_node;
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
    if ( !_node )
        return false;
    if ( _node->getNodeType() == LXML_TEXT_NODE && !parent() )
        return false;
    if ( !_node || _node->getNodeType() != LXML_ELEMENT_NODE )
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
    int count = _node->getChildCount();
    if ( count <=0 )
        return false;
    return child( count - 1 );
}

/// move to first element child of current node
bool ldomXPointerEx::firstElementChild()
{
    int count = _node->getChildCount();
    for ( int i=0; i<count; i++ ) {
        if ( _node->getChildNode( i )->getNodeType() == LXML_ELEMENT_NODE )
            return child( i );
    }
    return false;
}

/// move to last element child of current node
bool ldomXPointerEx::lastElementChild()
{
    int count = _node->getChildCount();
    for ( int i=count-1; i>=0; i-- ) {
        if ( _node->getChildNode( i )->getNodeType() == LXML_ELEMENT_NODE )
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
    ldomElement * e = (ldomElement*)_node;
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
    ldomElement * p;
    if ( _node && _node->isText() )
        p = _node->getParentNode();
    else
        p = (ldomElement*) _node;
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
    _offset = 0;
    while ( firstChild() ) {
        if ( _node->isText() )
            return true;
    }
    for (;;) {
        while ( nextSibling() ) {
            if ( _node->isText() )
                return true;
            while ( firstChild() ) {
                if ( _node->isText() )
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
    _offset = 0;
    for (;;) {
        while ( prevSibling() ) {
            if ( _node->isText() )
                return true;
            while ( lastChild() ) {
                if ( _node->isText() )
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
        if ( node->getNodeType()==LXML_ELEMENT_NODE ) {
            allowGoRecurse = callback->onElement( &pos.getStart() );
        } else if ( node->getNodeType()==LXML_TEXT_NODE ) {
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
        if ( !allowGoRecurse || !pos._start.child(0) ) {
            while ( !pos._start.nextSibling() ) {
                if ( !pos._start.parent() )
                    break;
            }
        }
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
            ldomText * node = (ldomText*) nodeRange->getStart().getNode();
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
            ldomElement * elem = (ldomElement *)ptr->getNode();
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
        ldomElement * elem = (ldomElement *)ptr->getNode();
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
#if COMPACT_DOM==1
    doc = new ldomDocument( stream, 0 );
#else
    doc = new ldomDocument();
#endif
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
                ldomElement * closedElement = _currNode->getElement();
                _currNode = pop( _currNode, closedElement->getNodeId() );
                //ElementCloseHandler( closedElement );
            }
        }
    } else {
        if ( !rule[0] )
            _currNode = pop( _currNode, _currNode->getElement()->getNodeId() );
    }
}

ldomElement * ldomDocumentWriterFilter::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname )
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

void ldomDocumentWriterFilter::ElementCloseHandler( ldomElement * node )
{
    ldomElement * parent = node->getParentNode();
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
                ldomElement * child = (ldomElement *)node->getLastChild();
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
    ldomElement * closedElement = _currNode->getElement();
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
void ldomDocumentWriterFilter::OnText( const lChar16 * text, int len,
    lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
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
                _currNode->onText( text, len, fpos, fsize, flags );
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

/// move range of children startChildIndex to endChildIndex inclusively to specified element
void ldomElement::moveItemsTo( ldomElement * destination, int startChildIndex, int endChildIndex )
{
    int len = endChildIndex - startChildIndex + 1;
    for ( int i=0; i<len; i++ ) {
        ldomNode * item = _children.remove( startChildIndex ); // + i
        item->_parent = destination;
        destination->_children.add( item );
    }
    // TODO: renumber rest of children in necessary
}
