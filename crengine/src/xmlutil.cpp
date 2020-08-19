#include "../include/fb2def.h"
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

void docx_titleHandler::onBodyStart()
{
    m_writer->OnTagOpen(L"", L"body");
}

void docx_titleHandler::onTitleStart(int level, bool noSection)
{
    CR_UNUSED(noSection);

    m_titleLevel = level;
    lString16 name = cs16("h") +  lString16::itoa(m_titleLevel);
    if( m_useClassName ) {
        m_writer->OnTagOpen(L"", L"p");
        m_writer->OnAttribute(L"", L"class", name.c_str());
    } else
        m_writer->OnTagOpen(L"", name.c_str());
}

void docx_titleHandler::onTitleEnd()
{
    if( !m_useClassName ) {
        lString16 tagName = cs16("h") +  lString16::itoa(m_titleLevel);
        m_writer->OnTagClose(L"", tagName.c_str());
    } else
        m_writer->OnTagClose(L"", L"p");
}

void docx_fb2TitleHandler::onBodyStart()
{
    m_section = m_writer->OnTagOpen(L"", L"body");
}

void docx_fb2TitleHandler::onTitleStart(int level, bool noSection)
{
    if( noSection )
        docx_titleHandler::onTitleStart(level, true);
    else {
        if( m_titleLevel < level ) {
            int startIndex = m_hasTitle ? 1 : 0;
            int contentCount = m_section->getChildCount();
            if(contentCount > startIndex)
                makeSection(startIndex);
        } else
            closeSection(m_titleLevel - level + 1);
        openSection(level);
        m_writer->OnTagOpen(L"", L"title");
        lString16 headingName = cs16("h") +  lString16::itoa(level);
        if( m_useClassName ) {
            m_writer->OnTagBody();
            m_writer->OnTagOpen(L"", L"p");
            m_writer->OnAttribute(L"", L"class", headingName.c_str());
        } else {
            m_writer->OnTagBody();
            m_writer->OnTagOpen(L"", headingName.c_str());
        }
    }
}

void docx_fb2TitleHandler::onTitleEnd()
{
    if( !m_useClassName ) {
        lString16 headingName = cs16("h") +  lString16::itoa(m_titleLevel);
        m_writer->OnTagClose(L"", headingName.c_str());
    } else
        m_writer->OnTagClose(L"", L"p");

    m_writer->OnTagClose(L"", L"title");
    m_hasTitle = true;
}

void docx_fb2TitleHandler::makeSection(int startIndex)
{
    ldomNode *newSection = m_section->insertChildElement(startIndex, LXML_NS_NONE, el_section);
    newSection->initNodeStyle();
    m_section->moveItemsTo(newSection, startIndex + 1, m_section->getChildCount() - 1);
    newSection->initNodeRendMethod( );
    m_section = newSection;
}

void docx_fb2TitleHandler::openSection(int level)
{
    for(int i = m_titleLevel; i < level; i++) {
        m_section = m_writer->OnTagOpen(L"", L"section");
        m_writer->OnTagBody();
    }
    m_titleLevel = level;
    m_hasTitle = false;
}

void docx_fb2TitleHandler::closeSection(int level)
{
    for(int i = 0; i < level; i++) {
        m_writer->OnTagClose(L"", L"section");
        m_titleLevel--;
    }
    m_hasTitle = false;
}
