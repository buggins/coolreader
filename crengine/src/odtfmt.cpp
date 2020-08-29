#include "../include/odtfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/crlog.h"
#include "xmlutil.h"

#define ODT_TAGS ODT_TAG(a) ODT_TAG(bookmark) ODT_TAG(body) ODT_TAG(note) ODT_TAG(text) \
    ODT_TAG(h) ODT_TAG(p) ODT_TAG(span) ODT_TAG(image) ODT_TAG(frame) ODT_TAG(table) \
    ODT_TAG(list)

#define ODT_TAG_NAME(itm) docx_el_##itm##_name
#define ODT_TAG_ID(itm) odt_el_##itm
#define ODT_TAG_CHILD(itm) { ODT_TAG_ID(itm), ODT_TAG_NAME(itm) }
#define ODT_LAST_ITEM { -1, NULL }

enum {
#define ODT_TAG(itm) 	ODT_TAG_ID(itm),
    odt_el_NULL = 0,
    odt_el_bookmarkRef,
    odt_el_bookmarkStart,
    odt_el_documentContent,
    odt_el_noteBody,
    odt_el_noteCitation,
    odt_el_tableRow,
    odt_el_tableCell,
    odt_el_listItem,
    odt_el_noteRef,
    ODT_TAGS
    odt_el_MAX_ID
};

#undef ODT_TAG
#define ODT_TAG(itm) static const lChar16 * const ODT_TAG_NAME(itm) = L ## #itm;
    ODT_TAGS

const struct item_def_t odt_elements[] = {
    { odt_el_bookmarkRef, L"bookmark-ref" },
    { odt_el_bookmarkStart, L"bookmark-start" },
    { odt_el_documentContent, L"document-content" },
    { odt_el_noteBody, L"note-body" },
    { odt_el_noteCitation, L"note-citation" },
    { odt_el_tableRow, L"table-row"},
    { odt_el_tableCell, L"table-cell"},
    { odt_el_listItem, L"list-item"},
    { odt_el_noteRef, L"note-ref"},
#undef ODT_TAG
#define ODT_TAG(itm) ODT_TAG_CHILD(itm),
    ODT_TAGS
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

class odt_documentHandler : public xml_ElementHandler
{
    LVArray<int> m_levels;
    LVArray<bool> m_listItems;
    LVArray<css_list_style_type_t> m_ListLevels;
    ldomDocumentWriter m_footNotesWriter;
    ldomDocumentWriter m_endNotesWriter;
    ldomDocumentWriter *m_saveWriter;
    ldomNode *m_footNotes;
    ldomNode *m_endNotes;
    ldomNode *m_body;
    lString16 m_noteId;
    lString16 m_noteRefText;
    bool m_isEndNote;
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
    docx_titleHandler* m_titleHandler;
    int m_outlineLevel;
    bool m_inTable;
    bool m_inListItem;
    bool m_listItemHadContent;
protected:
    int parseTagName(const lChar16 *tagname);
public:
    odt_documentHandler(ldomDocument * doc, docXMLreader * reader,
                        ldomDocumentWriter * writer, docx_titleHandler* titleHandler) :
        xml_ElementHandler(reader, writer, odt_el_NULL), m_footNotesWriter(doc),
        m_endNotesWriter(doc), m_saveWriter(NULL), m_footNotes(NULL), m_endNotes(NULL),
        m_body(NULL), m_isEndNote(false), m_titleHandler(titleHandler),
        m_outlineLevel(0), m_inTable(false), m_inListItem(false),
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

    //TODO: read manifest or whatever to get this file name
    LVStreamRef m_stream = arc->OpenStream(L"content.xml", LVOM_READ);
    if ( m_stream.isNull() )
        return false;

    ldomDocumentWriter writer(doc);
    docXMLreader docReader(&writer);

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
    docx_fb2TitleHandler titleHandler(&writer, DOCX_USE_CLASS_FOR_HEADING); //<section><title>..</title></section>
#else
    docx_titleHandler titleHandler(&writer);  //<hx>..</hx>
#endif

    odt_documentHandler documentHandler(doc, &docReader, &writer, &titleHandler);
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

int odt_documentHandler::parseTagName(const lChar16 *tagname)
{
    for (int i=0; odt_elements[i].name; i++) {
        if ( !lStr_cmp( odt_elements[i].name, tagname )) {
            // found!
            return odt_elements[i].id;
        }
    }
    return -1;
}

ldomNode *odt_documentHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
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
        if(m_inListItem) {
            m_listItemHadContent = true;
            m_writer->OnTagOpenNoAttr(L"", L"li");
        }
        m_writer->OnTagOpen(L"", L"p");
        break;
    case odt_el_span:
        m_writer->OnTagOpen(L"", L"span");
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
    m_state = tagId;
    m_levels.add(tagId);
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
        if(!lStr_cmp(attrname, "href"))
            m_writer->OnAttribute(L"", attrname, attrValue);
        break;
    default:
        break;
    }
}

void odt_documentHandler::handleTagBody()
{
    switch(m_state) {
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
    case odt_el_p:
    case odt_el_span:
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
    case odt_el_span:
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
