#include "../include/odtfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/crlog.h"
#include "odxutil.h"

#define ODT_TAGS \
    ODT_TAG(a)\
    ODT_TAG2(automaticStyles, "automatic-styles")\
    ODT_TAG(body)\
    ODT_TAG(bookmark)\
    ODT_TAG2(bookmarkRef, "bookmark-ref")\
    ODT_TAG2(bookmarkStart, "bookmark-start")\
    ODT_TAG2(defaultStyle, "default-style")\
    ODT_TAG2(documentContent, "document-content")\
    ODT_TAG2(documentStyles, "document-styles")\
    ODT_TAG(frame)\
    ODT_TAG(h)\
    ODT_TAG(image)\
    ODT_TAG(list)\
    ODT_TAG2(listItem, "list-item")\
    ODT_TAG(note)\
    ODT_TAG2(noteBody, "note-body")\
    ODT_TAG2(noteCitation, "note-citation")\
    ODT_TAG2(noteRef, "note-ref")\
    ODT_TAG(p)\
    ODT_TAG2(paragraphProperties, "paragraph-properties")\
    ODT_TAG(s)\
    ODT_TAG(span)\
    ODT_TAG(style)\
    ODT_TAG(styles)\
    ODT_TAG(tab)\
    ODT_TAG(table)\
    ODT_TAG2(tableRow, "table-row")\
    ODT_TAG2(tableCell, "table-cell")\
    ODT_TAG(text)\
    ODT_TAG2(textProperties, "text-properties")

#define ODT_TAG_NAME(itm) odt_el_##itm##_name
#define ODT_TAG_ID(itm) odt_el_##itm
#define ODT_TAG_CHILD(itm) { ODT_TAG_ID(itm), ODT_TAG_NAME(itm) }
#define ODT_TAG_CHILD2(itm, name) { ODT_TAG_ID(itm), L ## name }
#define ODT_LAST_ITEM { -1, NULL }

enum {
#define ODT_TAG(itm) ODT_TAG_ID(itm),
#define ODT_TAG2(itm, name) ODT_TAG_ID(itm),
    odt_el_NULL = 0,
    ODT_TAGS
    odt_el_MAX_ID
};

#undef ODT_TAG
#undef ODT_TAG2
#define ODT_TAG(itm) static const lChar16 * const ODT_TAG_NAME(itm) = L ## #itm;
#define ODT_TAG2(itm, name) static const lChar16 * const ODT_TAG_NAME(itm) = L ## name;
    ODT_TAGS

#undef ODT_TAG
#undef ODT_TAG2
#define ODT_TAG(itm) ODT_TAG_CHILD(itm),
#define ODT_TAG2(itm, name) ODT_TAG_CHILD2(itm, name),

const struct item_def_t odt_elements[] = {
    ODT_TAGS
    ODT_LAST_ITEM
};

const struct item_def_t odt_style_elements[] = {
    ODT_TAG_CHILD(automaticStyles),
    ODT_TAG_CHILD(defaultStyle),
    ODT_TAG_CHILD(documentStyles),
    ODT_TAG_CHILD(paragraphProperties),
    ODT_TAG_CHILD(style),
    ODT_TAG_CHILD(styles),
    ODT_TAG_CHILD(textProperties),
    ODT_LAST_ITEM
};

static const struct item_def_t styleFamily_attr_values[] = {
    { odx_paragraph_style, L"paragraph" },
    { odx_character_style, L"text"},
    ODT_LAST_ITEM
};

const struct item_def_t odt_fontWeigth_attr_values[] = {
    { 400, L"normal" },
    { 600, L"bold" },
    { 100, L"100" },
    { 200, L"200" },
    { 300, L"300" },
    { 400, L"400" },
    { 500, L"500" },
    { 600, L"600" },
    { 700, L"700" },
    { 800, L"800" },
    { 900, L"900" },
    ODT_LAST_ITEM
};

static const struct item_def_t odt_textAlign_attr_values[] =
{
    { css_ta_left, L"left" },
    { css_ta_right, L"right" },
    { css_ta_center, L"center" },
    { css_ta_justify, L"justify" },
    { css_ta_start, L"start" },
    { css_ta_end, L"end" },
    ODT_LAST_ITEM
};

bool DetectOpenDocumentFormat( LVStreamRef stream )
{
    LVContainerRef arc = LVOpenArchieve( stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive
    lString16 mimeType;
    {
        LVStreamRef mtStream = arc->OpenStream(L"mimetype", LVOM_READ );
        if ( !mtStream.isNull() ) {
            lvsize_t size = mtStream->GetSize();
            if ( size>4 && size<100 ) {
                LVArray<char> buf( size+1, '\0' );
                if ( mtStream->Read( buf.get(), size, NULL )==LVERR_OK ) {
                    for ( lvsize_t i=0; i<size; i++ )
                        if ( buf[i]<32 || ((unsigned char)buf[i])>127 )
                            buf[i] = 0;
                    buf[size] = 0;
                    if ( buf[0] )
                        mimeType = Utf8ToUnicode( lString8( buf.get() ) );
                }
            }
        }
    }
    return ( mimeType == L"application/vnd.oasis.opendocument.text" );
}

class odtImportContext : public odx_ImportContext
{
    LVContainerRef m_container;
public:
    odtImportContext(LVContainerRef container) : m_container(container) {
    }
    LVStreamRef openFile(const lChar16 * const fileName);
};


class odt_stylesHandler : public xml_ElementHandler
{
private:
    LVArray<int> m_levels;
    odx_StyleRef m_styleRef;
    odx_Style *m_style;
    odx_pPr* m_pPr;
    odx_rPr* m_rPr;
    odtImportContext *m_context;
public:
    /// constructor
    odt_stylesHandler(docXMLreader * reader, ldomDocumentWriter *writer, int element,
                      odtImportContext *context) :
        xml_ElementHandler(reader, writer, element, odt_style_elements),
            m_style(NULL), m_pPr(NULL), m_rPr(NULL), m_context(context)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
};

class odt_documentHandler : public xml_ElementHandler, odx_styleTagsHandler
{
    LVArray<int> m_levels;
    LVArray<bool> m_listItems;
    LVArray<css_list_style_type_t> m_ListLevels;
    ldomDocumentWriter m_footNotesWriter;
    ldomDocumentWriter m_endNotesWriter;
    ldomDocumentWriter *m_saveWriter;
    odtImportContext * m_context;
    ldomNode *m_footNotes;
    ldomNode *m_endNotes;
    ldomNode *m_body;
    lString16 m_noteId;
    lString16 m_noteRefText;
    lString16 m_StyleName;
    bool m_isEndNote;
    odt_stylesHandler m_stylesHandler;
private:
    ldomNode* startNotes(const lChar16 * notesKind) {
        m_writer->OnStart(NULL);
        ldomNode* notes = m_writer->OnTagOpen(L"", L"body");
        m_writer->OnAttribute(L"", L"name", notesKind);
        m_writer->OnTagBody();
        return notes;
    }
    void finishNotes(ldomNode* notes, ldomDocumentWriter& writer) {
        ldomNode* parent = notes->getParentNode();
        int index = notes->getNodeIndex();
        writer.OnTagClose(L"", L"body");
        writer.OnStop();
        parent->moveItemsTo(m_body->getParentNode(), index, index);
    }
protected:
    odx_titleHandler* m_titleHandler;
    int m_outlineLevel;
    bool m_inTable;
    bool m_inListItem;
    bool m_listItemHadContent;
public:
    odt_documentHandler(ldomDocument * doc, docXMLreader * reader,
                        ldomDocumentWriter * writer, odx_titleHandler* titleHandler,
                        odtImportContext * context) :
        xml_ElementHandler(reader, writer, odt_el_NULL, odt_elements),
        odx_styleTagsHandler(writer), m_footNotesWriter(doc),
        m_endNotesWriter(doc), m_saveWriter(NULL), m_context(context), m_footNotes(NULL),
        m_endNotes(NULL), m_body(NULL), m_isEndNote(false), m_titleHandler(titleHandler),
        m_outlineLevel(0), m_inTable(false), m_inListItem(false),
        m_stylesHandler(reader, NULL, odt_el_automaticStyles, context),
        m_listItemHadContent(false) {
    }
    inline bool isInList() { return m_ListLevels.length() != 0; }
    ldomNode *handleTagOpen(int tagId);
    void handleAttribute(const lChar16 *attrname, const lChar16 *attrValue);
    void handleTagBody();
    void handleTagClose(const lChar16 *nsname, const lChar16 *tagname);
    void handleText(const lChar16 *text, int len, lUInt32 flags);
    void reset();
};

static bool parseStyles(odtImportContext *context)
{
    LVStreamRef stream = context->openFile(L"styles.xml");
    if ( stream.isNull() )
        return false;

    docXMLreader docReader(NULL);
    odt_stylesHandler stylesHandler(&docReader, NULL, odt_el_documentStyles, context);
    docReader.setHandler(&stylesHandler);

    LVXMLParser parser(stream, &docReader);

    if ( !parser.Parse() )
        return false;
    return true;
}

bool ImportOpenDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    LVContainerRef arc = LVOpenArchieve( stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive

    doc->setContainer(arc);

#if BUILD_LITE!=1
    if ( doc->openFromCache(formatCallback) ) {
        if ( progressCallback ) {
            progressCallback->OnLoadFileEnd( );
        }
        return true;
    }
#endif

    ldomDocumentWriter writer(doc);
    docXMLreader docReader(&writer);

    odtImportContext importContext(arc);
    if( !parseStyles(&importContext) )
        return false;

    //TODO: read manifest or whatever to get this file name
    LVStreamRef m_stream = arc->OpenStream(L"content.xml", LVOM_READ);
    if ( m_stream.isNull() )
        return false;

#ifdef DOCX_FB2_DOM_STRUCTURE
    writer.OnStart(NULL);
    writer.OnTagOpen(NULL, L"?xml");
    writer.OnAttribute(NULL, L"version", L"1.0");
    writer.OnAttribute(NULL, L"encoding", L"utf-8");
    writer.OnEncoding(L"utf-8", NULL);
    writer.OnTagBody();
    writer.OnTagClose(NULL, L"?xml");
    writer.OnTagOpenNoAttr(NULL, L"FictionBook");
    // DESCRIPTION
    writer.OnTagOpenNoAttr(NULL, L"description");
    writer.OnTagOpenNoAttr(NULL, L"title-info");
    writer.OnTagOpenNoAttr(NULL, L"book-title");
    writer.OnTagClose(NULL, L"book-title");
    writer.OnTagClose(NULL, L"title-info");
    writer.OnTagClose(NULL, L"description");
#else
    writer.OnStart(NULL);
    writer.OnTagOpen(NULL, L"?xml");
    writer.OnAttribute(NULL, L"version", L"1.0");
    writer.OnAttribute(NULL, L"encoding", L"utf-8");
    writer.OnEncoding(L"utf-8", NULL);
    writer.OnTagBody();
    writer.OnTagClose(NULL, L"?xml");
    writer.OnTagOpenNoAttr(NULL, L"html");
#endif

#ifdef DOCX_FB2_DOM_STRUCTURE
    //Two options when dealing with titles: (FB2|HTML)
    odx_fb2TitleHandler titleHandler(&writer, DOCX_USE_CLASS_FOR_HEADING); //<section><title>..</title></section>
#else
    odx_titleHandler titleHandler(&writer);  //<hx>..</hx>
#endif

    odt_documentHandler documentHandler(doc, &docReader, &writer, &titleHandler, &importContext);
    docReader.setHandler(&documentHandler);

    LVXMLParser parser(m_stream, &docReader);

    if ( !parser.Parse() )
        return false;

    writer.OnTagClose(NULL, L"FictionBook");
    writer.OnStop();

    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        doc->compact();
        doc->dumpStatistics();
    }

#if 1
    // save compound XML document, for testing:
    doc->saveToStream(LVOpenFileStream("D:/Temp/odt_dump.xml", LVOM_WRITE), NULL, true);
#endif
    return true;
}

ldomNode *odt_documentHandler::handleTagOpen(int tagId)
{
    bool elementHandled = false;

    switch(tagId) {
    case odt_el_automaticStyles:
        m_stylesHandler.start();
        elementHandled = true;
        break;
    case odt_el_a:
    case odt_el_bookmark:
    case odt_el_bookmarkRef:
    case odt_el_bookmarkStart:
    case odt_el_note:
    case odt_el_noteRef:
        m_isEndNote = false;
        m_writer->OnTagOpen(L"", L"a");
        break;
    case odt_el_body:
        m_body = m_titleHandler->onBodyStart();
        m_writer->OnTagBody();
        break;
    case odt_el_h:
        m_outlineLevel = 0;
        break;
    case odt_el_p:
        m_StyleName.clear();
        if(m_inListItem) {
            m_listItemHadContent = true;
            m_writer->OnTagOpenNoAttr(L"", L"li");
        }
        m_writer->OnTagOpen(L"", L"p");
        break;
    case odt_el_span:
        m_StyleName.clear();
        break;
    case odt_el_list:
        m_writer->OnTagOpen(L"", L"ol");
        m_ListLevels.add(css_lst_decimal);
        if (isInList())
            m_listItems.add(m_listItemHadContent);
        break;
    case odt_el_listItem:
        m_inListItem = true;
        m_listItemHadContent = false;
        break;
    case odt_el_image:
        m_writer->OnTagOpen(L"", L"img");
        break;
    case odt_el_table:
        m_inTable = true;
        m_writer->OnTagOpen(L"", L"table");
        break;
    case odt_el_tableRow:
        m_writer->OnTagOpen(L"", L"tr");
        break;
    case odt_el_tableCell:
        m_writer->OnTagOpen(L"", L"td");
        break;
    case odt_el_tab:
    case odt_el_s:
        m_writer->OnText(L" ", 1, TXTFLG_PRE);
        break;
    case odt_el_noteBody:
        m_saveWriter = m_writer;
        if(m_isEndNote) {
            m_writer = &m_endNotesWriter;
            if(!m_endNotes)
                m_endNotes = startNotes(L"comments");
        } else {
            m_writer = &m_footNotesWriter;
            if(!m_footNotes)
                m_footNotes = startNotes(L"notes");;
        }
        m_writer->OnTagOpen(L"", L"section");
        m_writer->OnAttribute(L"", L"id", m_noteId.c_str());
        m_writer->OnAttribute(L"", L"role", m_isEndNote ? L"doc-rearnote" : L"doc-footnote");
        m_writer->OnTagBody();
        m_writer->OnTagOpenNoAttr(L"", L"title");
        m_writer->OnTagOpenNoAttr(L"", L"p");
        m_writer->OnText(m_noteRefText.c_str(), m_noteRefText.length(), 0);
        m_writer->OnTagClose(L"", L"p");
        m_writer->OnTagClose(L"", L"title");
        break;
    default:
        break;
    }
    if( !elementHandled ) {
        m_state = tagId;
        m_levels.add(tagId);
    }
    return NULL;
}

void odt_documentHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrValue)
{
    switch (m_state) {
    case odt_el_bookmark:
    case odt_el_bookmarkStart:
        if(!lStr_cmp(attrname, "name") ) {
            m_writer->OnAttribute(L"", L"id", attrValue);
        }
        break;
    case odt_el_bookmarkRef:
    case odt_el_noteRef:
        if(!lStr_cmp(attrname, "ref-name") ) {
            lString16 target = cs16("#") + lString16(attrValue);
            m_writer->OnAttribute(L"", L"href", target.c_str());
        }
        break;
    case odt_el_h:
        if(!lStr_cmp(attrname, "outline-level") ) {
            lString16 value = attrValue;
            int tmp;

            if(value.atoi(tmp))
                m_outlineLevel = tmp - 1;
        }
        break;
    case odt_el_note:
        if(!lStr_cmp(attrname, "note-class")) {
            if(!lStr_cmp(attrValue, "endnote")) {
                m_writer->OnAttribute(L"", L"type", L"comment");
                m_isEndNote = true;
            } else if(!lStr_cmp(attrValue, "footnote")) {
                m_writer->OnAttribute(L"", L"type", L"note");
            }
            m_writer->OnAttribute(L"", L"role", L"doc-noteref");
        } else if(!lStr_cmp(attrname, "id")) {
            m_noteId = lString16(attrValue);
            lString16 target = cs16("#") + m_noteId;
            m_writer->OnAttribute(L"", L"href", target.c_str());
        }
        break;
    case odt_el_tableCell:
        if(!lStr_cmp(attrname, "number-columns-spanned"))
            m_writer->OnAttribute(L"", L"colspan", attrValue);
        else if(!lStr_cmp(attrname, "number-rows-spanned"))
            m_writer->OnAttribute(L"", L"rowspan", attrValue);
        break;
    case odt_el_a:
    case odt_el_image:
        if( !lStr_cmp(attrname, "href") )
            m_writer->OnAttribute(L"", attrname, attrValue);
        break;
    case odt_el_p:
    case odt_el_span:
        if( !lStr_cmp(attrname, "style-name") )
            m_StyleName = attrValue;
        break;
    default:
        break;
    }
}

void odt_documentHandler::handleTagBody()
{
    switch(m_state) {
    case odt_el_span:
        if( !m_StyleName.empty() ) {
            odx_Style* style = m_context->getStyle(m_StyleName);
            if(style) {
                odx_rPr rPr;

                rPr.combineWith(style->get_rPr(m_context));
                rPr.combineWith(m_context->get_rPrDefault());
                closeStyleTags(&rPr);
                openStyleTags(&rPr);
            }
            /* FIXME: if no style specified we should look for paragraph style or some default style*/
        }
        break;
    case odt_el_p:
        if( !m_StyleName.empty() ) {
            odx_Style* style = m_context->getStyle(m_StyleName);
            if(style) {
                odx_pPr pPr;

                pPr.combineWith(style->get_pPr(m_context));
                pPr.combineWith(m_context->get_pPrDefault());
                lString16 style = pPr.getCss();
                if( !style.empty() )
                    m_writer->OnAttribute(L"", L"style", style.c_str());
            }
        }
        m_writer->OnTagBody();
        break;
    case odt_el_h:
        if(m_inListItem) {
            m_listItemHadContent = true;
            m_writer->OnTagOpenNoAttr(L"", L"li");
        }
        m_titleHandler->onTitleStart(m_outlineLevel + 1, m_inTable || m_inListItem);
    case odt_el_a:
    case odt_el_bookmark:
    case odt_el_bookmarkRef:
    case odt_el_bookmarkStart:
    case odt_el_note:
    case odt_el_image:
    case odt_el_table:
    case odt_el_tableCell:
    case odt_el_tableRow:
    case odt_el_list:
    case odt_el_noteRef:
        m_writer->OnTagBody();
        break;
    default:
        break;
    }
}

void odt_documentHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    switch(m_state) {
    case odt_el_bookmark:
    case odt_el_bookmarkRef:
    case odt_el_bookmarkStart:
    case odt_el_noteRef:
    case odt_el_a:
        m_writer->OnTagClose(L"", L"a");
        break;
    case odt_el_body:
        m_titleHandler->onBodyEnd();
        m_writer->OnTagClose(nsname, tagname);
        if(m_footNotes)
            finishNotes(m_footNotes, m_footNotesWriter);
        if(m_endNotes)
            finishNotes(m_endNotes, m_endNotesWriter);
        break;
    case odt_el_h:
        m_titleHandler->onTitleEnd();
        break;
    case odt_el_p:
        closeStyleTags();
        m_writer->OnTagClose(nsname, tagname);
        break;
    case odt_el_image:
        m_writer->OnTagClose(L"", L"img");
        break;
    case odt_el_table:
        m_writer->OnTagClose(L"", L"table");
        m_inTable = false;
        break;
    case odt_el_tableRow:
        m_writer->OnTagClose(L"", L"tr");
        break;
    case odt_el_tableCell:
        m_writer->OnTagClose(L"", L"td");
        break;
    case odt_el_list:
        m_ListLevels.remove(m_ListLevels.length() - 1);
        m_writer->OnTagClose(L"", L"ol");
        break;
    case odt_el_listItem:
        if(m_listItemHadContent)
            m_writer->OnTagClose(L"", L"li");
        if(!m_listItems.empty()) {
            m_listItemHadContent = m_listItems[m_listItems.length() - 1];
            m_listItems.erase(m_listItems.length() - 1, 1);
        }
        m_inListItem = false;
        break;
    case odt_el_noteBody:
        m_writer->OnTagClose(L"", L"section");
        m_writer = m_saveWriter;
        break;
    default:
        break;
    }
    m_levels.erase(m_levels.length() - 1, 1);
    if( !m_levels.empty() )
        m_state = m_levels[m_levels.length() - 1];
    else
        m_state = odt_el_NULL;
}

void odt_documentHandler::handleText(const lChar16 *text, int len, lUInt32 flags)
{
    switch(m_state) {
    case odt_el_noteCitation:
        m_noteRefText = text;
    case odt_el_h:
    case odt_el_p:
    case odt_el_span:
    case odt_el_bookmarkRef:
    case odt_el_noteRef:
        m_writer->OnText(text, len, flags);
        break;
    default:
        break;
    }
}

void odt_documentHandler::reset()
{
    m_levels.clear();
    m_ListLevels.clear();
    m_listItems.clear();
    m_state = odt_el_NULL;
    m_outlineLevel = 0;
    m_inTable = false;
    m_inListItem = false;
    m_listItemHadContent = false;
}

LVStreamRef odtImportContext::openFile(const lChar16 * const fileName)
{
    return m_container->OpenStream(fileName, LVOM_READ);
}

ldomNode *odt_stylesHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case odt_el_defaultStyle:
        m_style = NULL;
        break;
    case odt_el_paragraphProperties:
        m_pPr = m_style ? m_style->get_pPrPointer() : m_context->get_pPrDefault();
        break;
    case odt_el_textProperties:
        m_rPr = m_style ? m_style->get_rPrPointer() : m_context->get_rPrDefault();
        break;
    case odt_el_style:
        m_styleRef = odx_StyleRef( new odx_Style );
        m_style = m_styleRef.get();
    default:
        break;
    }
    m_state = tagId;
    m_levels.add(tagId);
    return NULL;
}

void odt_stylesHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
{
    switch(m_state) {
    case odt_el_style:
        if( !lStr_cmp(attrname, "name") ) {
            m_style->setId(attrvalue);
        } else if( !lStr_cmp(attrname, "display-name") ) {
            m_style->setName(attrvalue);
        } else if( !lStr_cmp(attrname, "family") ) {
            int n = parse_name(styleFamily_attr_values, attrvalue);
            if(n != -1)
                m_style->setStyleType((odx_style_type)n);
        } else if( !lStr_cmp(attrname, "parent-style-name") ) {
            m_style->setBasedOn(attrvalue);
        }
        break;
    case odt_el_paragraphProperties:
        if( !lStr_cmp(attrname, "break-before") ) {
            m_pPr->setPageBreakBefore( !lStr_cmp(attrvalue, "page") );
        } else if( !lStr_cmp(attrname, "text-align") ) {
            int n = parse_name(odt_textAlign_attr_values, attrvalue);
            if(n != -1)
                m_pPr->setTextAlign((css_text_align_t)n);
        } else if( !lStr_cmp(attrname, "keep-with-next") ) {
            m_pPr->setKeepNext( !lStr_cmp(attrvalue, "always") );
        }
        break;
    case odt_el_textProperties:
        if( !lStr_cmp(attrname, "font-style") ) {
            m_rPr->setItalic( lStr_cmp(attrvalue, "normal") !=0 );
        } else if( !lStr_cmp(attrname, "font-weight") ) {
            int n = parse_name(odt_fontWeigth_attr_values, attrvalue);
            if( n != -1 )
                m_rPr->setBold( n >= 600 );
        } else if( !lStr_cmp(attrname, "text-underline-type") ) {
            m_rPr->setUnderline( lStr_cmp( attrvalue, "none") !=0 );
        } else if( !lStr_cmp(attrname, "text-line-through-type") ) {
            m_rPr->setStrikeThrough( lStr_cmp( attrvalue, "none") !=0 );
        } else if( !lStr_cmp(attrname, "text-position") ) {
            lString16 val = attrvalue;

            if( val.startsWith(L"super") ) {
                m_rPr->setVertAlign(css_va_super);
            } else if( val.startsWith(L"sub") ) {
                m_rPr->setVertAlign(css_va_sub);
            }
        }
        break;
    default:
        break;
    }
}

void odt_stylesHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    switch(m_state) {
    case odt_el_style:
        if(m_style && m_style->isValid()) {
            m_context->addStyle(m_styleRef);
        }
    default:
        break;
    }
    if( !m_levels.empty() ) {
        m_levels.erase(m_levels.length() - 1, 1);
        if(! m_levels.empty() )
            m_state = m_levels[m_levels.length() - 1];
        else
            m_state = m_element;
    } else
        stop();
}
