#include "../include/odtfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/crlog.h"
#include "xmlutil.h"

#define ODT_TAGS ODT_TAG(body) ODT_TAG(text) ODT_TAG(h) ODT_TAG(p) ODT_TAG(span)

#define ODT_TAG_NAME(itm) docx_el_##itm##_name
#define ODT_TAG_ID(itm) odt_el_##itm
#define ODT_TAG_CHILD(itm) { ODT_TAG_ID(itm), ODT_TAG_NAME(itm) }
#define ODT_LAST_ITEM { -1, NULL }

enum {
#define ODT_TAG(itm) 	ODT_TAG_ID(itm),
    odt_el_NULL = 0,
    odt_el_documentContent,
    ODT_TAGS
    odt_el_MAX_ID
};

#undef ODT_TAG
#define ODT_TAG(itm) static const lChar16 * const ODT_TAG_NAME(itm) = L ## #itm;
    ODT_TAGS

const struct item_def_t odt_elements[] = {
    { odt_el_documentContent, L"document-content" },
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
protected:
    docx_titleHandler* m_titleHandler;
    int m_outlineLevel;
protected:
    int parseTagName(const lChar16 *tagname);
public:
    odt_documentHandler(docXMLreader * reader, ldomDocumentWriter *writer, docx_titleHandler* titleHandler) :
        xml_ElementHandler(reader, writer, odt_el_NULL), m_titleHandler(titleHandler), m_outlineLevel(0) {
    }
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

    odt_documentHandler documentHandler(&docReader, &writer, &titleHandler);
    docReader.setHandler(&documentHandler);

    LVXMLParser parser(m_stream, &docReader);

    if ( !parser.Parse() )
        return false;

    writer.OnTagClose(NULL, L"html");
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
    case odt_el_body:
        m_titleHandler->onBodyStart();
        m_writer->OnTagBody();
        break;
    case odt_el_h:
        m_outlineLevel = 0;
        break;
    case odt_el_p:
        m_writer->OnTagOpen(L"", L"p");
        m_writer->OnTagBody();
        break;
    case odt_el_span:
        m_writer->OnTagOpen(L"", L"span");
        m_writer->OnTagBody();
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
    if(m_state == odt_el_h && !lStr_cmp(attrname, "outline-level") ) {
        lString16 value = attrValue;
        int tmp;

        if(value.atoi(tmp))
            m_outlineLevel = tmp - 1;
    }
}

void odt_documentHandler::handleTagBody()
{
    if(m_state == odt_el_h) {
        m_titleHandler->onTitleStart(m_outlineLevel + 1);
        m_writer->OnTagBody();
    }
}

void odt_documentHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    switch(m_state) {
    case odt_el_body:
        m_titleHandler->onBodyEnd();
        m_writer->OnTagClose(nsname, tagname);
        break;
    case odt_el_h:
        m_titleHandler->onTitleEnd();
        break;
    case odt_el_p:
    case odt_el_span:
        m_writer->OnTagClose(nsname, tagname);
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
    case odt_el_h:
    case odt_el_p:
    case odt_el_span:
        m_writer->OnText(text, len, flags);
        break;
    default:
        break;
    }
}

void odt_documentHandler::reset()
{
    m_levels.clear();
    m_state = odt_el_NULL;
    m_outlineLevel = 0;
}
