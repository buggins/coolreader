#include "../include/crlog.h"
#include "xmlutil.h"

ldomNode * docXMLreader::OnTagOpen( const lChar16 * nsname, const lChar16 * tagname)
{
    if ( m_state == xml_doc_in_start && !lStr_cmp(tagname, "?xml") )
        m_state = xml_doc_in_xml_declaration;
    else if( !isSkipping() ) {
        if ( m_handler )
            return m_handler->handleTagOpen(nsname, tagname);
    } else
        // skip nested tag
        skip();
    return NULL;
}

void docXMLreader::OnStart(LVFileFormatParser *)
{
    m_skipTag = 0;
    m_state = xml_doc_in_start;
}

void docXMLreader::OnTagBody()
{
    if( m_state != xml_doc_in_xml_declaration && !isSkipping() && m_handler )
        m_handler->handleTagBody();
}

void docXMLreader::OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
{
    CR_UNUSED(nsname);

    switch(m_state) {
    case xml_doc_in_xml_declaration:
        m_state = xml_doc_in_document;
        break;
    case xml_doc_in_document:
        if( isSkipping() )
            skipped();
        else if ( m_handler )
            m_handler->handleTagClose(L"", tagname);
        break;
    default:
        CRLog::error("Unexpected state");
        break;
    }
}

void docXMLreader::OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
{
    switch(m_state) {
    case xml_doc_in_xml_declaration:
        if ( m_writer )
            m_writer->OnAttribute(nsname, attrname, attrvalue);
        break;
    case xml_doc_in_document:
        if ( !isSkipping() && m_handler )
            m_handler->handleAttribute(nsname, attrname, attrvalue);
        break;
    default:
        CRLog::error("Unexpected state");
    }
}

void docXMLreader::OnText( const lChar16 * text, int len, lUInt32 flags )
{
    if( !isSkipping() && m_handler )
        m_handler->handleText(text, len, flags);
}

bool docXMLreader::OnBlob(lString16 name, const lUInt8 * data, int size)
{
    if ( !isSkipping() && m_writer )
        return m_writer->OnBlob(name, data, size);
    return false;
}

ldomNode *xml_ElementHandler::handleTagOpen(const lChar16 *nsname, const lChar16 *tagname)
{
    int tag = parseTagName(tagname);

    CR_UNUSED(nsname);
    if( -1 == tag) {
        // skip the tag we are not interested in
        m_reader->skip();
        return NULL;
    }
    return handleTagOpen(tag);
}

ldomNode *xml_ElementHandler::handleTagOpen(int tagId)
{
    m_state = tagId;
    return NULL;
}

void xml_ElementHandler::start()
{
    m_savedHandler = m_reader->getHandler();
    reset();
    m_reader->setHandler(this);
}

void xml_ElementHandler::stop()
{
    m_reader->setHandler(m_savedHandler);
    m_savedHandler = NULL;
}

void xml_ElementHandler::reset()
{
}
