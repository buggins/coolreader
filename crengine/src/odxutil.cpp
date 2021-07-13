/**
    CoolReader Engine

    odxutil.cpp: ODT/DOCX support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2019-2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#include "../include/fb2def.h"
#include "../include/crlog.h"
#include "odxutil.h"

ldomNode * docXMLreader::OnTagOpen( const lChar32 * nsname, const lChar32 * tagname)
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

void docXMLreader::OnTagClose(const lChar32 * nsname, const lChar32 * tagname , bool self_closing_tag)
{
    CR_UNUSED2(nsname, self_closing_tag);

    switch(m_state) {
    case xml_doc_in_xml_declaration:
        m_state = xml_doc_in_document;
        break;
    case xml_doc_in_document:
        if( isSkipping() )
            skipped();
        else if ( m_handler )
            m_handler->handleTagClose(U"", tagname);
        break;
    default:
        CRLog::error("Unexpected state");
        break;
    }
}

void docXMLreader::OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue )
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

void docXMLreader::OnText( const lChar32 * text, int len, lUInt32 flags )
{
    if( !isSkipping() && m_handler )
        m_handler->handleText(text, len, flags);
}

bool docXMLreader::OnBlob(lString32 name, const lUInt8 * data, int size)
{
    if ( !isSkipping() && m_writer )
        return m_writer->OnBlob(name, data, size);
    return false;
}

void xml_ElementHandler::setChildrenInfo(const struct item_def_t *tags)
{
    m_children = tags;
}

int xml_ElementHandler::parse_name(const struct item_def_t *tags, const lChar32 * nameValue)
{
    for (int i=0; tags[i].name; i++) {
        if ( !lStr_cmp( tags[i].name, nameValue )) {
            // found!
            return tags[i].id;
        }
    }
    return -1;
}

void xml_ElementHandler::parse_int(const lChar32 * attrValue, css_length_t & result)
{
    lString32 value = attrValue;

    result.type = css_val_unspecified;
    if ( value.atoi(result.value) )
        result.type = css_val_pt; //just to distinguish with unspecified value
}

ldomNode *xml_ElementHandler::handleTagOpen(const lChar32 *nsname, const lChar32 *tagname)
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

ldomNode *odx_titleHandler::onBodyStart()
{
    return m_writer->OnTagOpen(U"", U"body");
}

void odx_titleHandler::onTitleStart(int level, bool noSection)
{
    CR_UNUSED(noSection);

    m_titleLevel = level;
    lString32 name = cs32("h") +  lString32::itoa(m_titleLevel);
    if( m_useClassName ) {
        m_writer->OnTagOpen(U"", U"p");
        m_writer->OnAttribute(U"", U"class", name.c_str());
    } else
        m_writer->OnTagOpen(U"", name.c_str());
}

void odx_titleHandler::onTitleEnd()
{
    if( !m_useClassName ) {
        lString32 tagName = cs32("h") +  lString32::itoa(m_titleLevel);
        m_writer->OnTagClose(U"", tagName.c_str());
    } else
        m_writer->OnTagClose(U"", U"p");
}

ldomNode* odx_fb2TitleHandler::onBodyStart()
{
    m_section = m_writer->OnTagOpen(U"", U"body");
    return m_section;
}

void odx_fb2TitleHandler::onTitleStart(int level, bool noSection)
{
    if( noSection )
        odx_titleHandler::onTitleStart(level, true);
    else {
        if( m_titleLevel < level ) {
            int startIndex = m_hasTitle ? 1 : 0;
            int contentCount = m_section->getChildCount();
            if(contentCount > startIndex)
                makeSection(startIndex);
        } else
            closeSection(m_titleLevel - level + 1);
        openSection(level);
        m_writer->OnTagOpen(U"", U"title");
        lString32 headingName = cs32("h") +  lString32::itoa(level);
        if( m_useClassName ) {
            m_writer->OnTagBody();
            m_writer->OnTagOpen(U"", U"p");
            m_writer->OnAttribute(U"", U"class", headingName.c_str());
        } else {
            m_writer->OnTagBody();
            m_writer->OnTagOpen(U"", headingName.c_str());
        }
    }
}

void odx_fb2TitleHandler::onTitleEnd()
{
    if( !m_useClassName ) {
        lString32 headingName = cs32("h") +  lString32::itoa(m_titleLevel);
        m_writer->OnTagClose(U"", headingName.c_str());
    } else
        m_writer->OnTagClose(U"", U"p");

    m_writer->OnTagClose(U"", U"title");
    m_hasTitle = true;
}

void odx_fb2TitleHandler::makeSection(int startIndex)
{
    ldomNode *newSection = m_section->insertChildElement(startIndex, LXML_NS_NONE, el_section);
    newSection->initNodeStyle();
    m_section->moveItemsTo(newSection, startIndex + 1, m_section->getChildCount() - 1);
    newSection->initNodeRendMethod( );
    m_section = newSection;
}

void odx_fb2TitleHandler::openSection(int level)
{
    for(int i = m_titleLevel; i < level; i++) {
        m_section = m_writer->OnTagOpen(U"", U"section");
        m_writer->OnTagBody();
    }
    m_titleLevel = level;
    m_hasTitle = false;
}

void odx_fb2TitleHandler::closeSection(int level)
{
    for(int i = 0; i < level; i++) {
        m_writer->OnTagClose(U"", U"section");
        m_titleLevel--;
    }
    m_hasTitle = false;
}

odx_rPr::odx_rPr() : odx_StylePropertiesContainer(odx_character_style)
{
}

lString32 odx_rPr::getCss()
{
    lString32 style;

    if( isBold() )
        style << " font-weight: bold;";
    if( isItalic() )
        style << " font-style: italic;";
    if( isUnderline() )
        style << " text-decoration: underline;";
    if( isStrikeThrough() )
        style << " text-decoration: line-through;";
    return style;
}

odx_pPr::odx_pPr() : odx_StylePropertiesContainer(odx_paragraph_style)
{
}

lString32 odx_pPr::getCss()
{
    lString32 style;

    css_text_align_t align = getTextAlign();
    if(align != css_ta_inherit)
    {
        style << "text-align: ";
        switch(align)
        {
        case css_ta_left:
            style << "left;";
            break;
        case css_ta_right:
            style << "right;";
            break;
        case css_ta_center:
            style << "center;";
            break;
        case css_ta_justify:
            style << "justify;";
            break;
        case css_ta_end:
            style << "end;";
            break;
        case css_ta_start:
            style << "start;";
            break;
        default:
            style << "inherited;";
            break;
        }
    }
    if( isPageBreakBefore() )
        style << "page-break-before: always;";
    else if ( isKeepNext() )
        style << "page-break-before: avoid;";
    return style;
}

odx_Style::odx_Style() : m_type(odx_paragraph_style),
    m_pPrMerged(false), m_rPrMerged(false)
{
}

bool odx_Style::isValid() const
{
    return ( odx_invalid_style != m_type && !m_Id.empty() );
}

odx_Style *odx_Style::getBaseStyle(odx_ImportContext *context)
{
    lString32 basedOn = getBasedOn();
    if ( !basedOn.empty() ) {
        odx_Style *pStyle = context->getStyle(basedOn);
        if( pStyle && pStyle->getStyleType() == getStyleType() )
            return pStyle;
    }
    return NULL;
}

odx_pPr *odx_Style::get_pPr(odx_ImportContext *context)
{
    if( !m_pPrMerged ) {
        odx_Style* pStyle = getBaseStyle(context);
        if (pStyle ) {
            m_pPr.combineWith(pStyle->get_pPr(context));
        }
        m_pPrMerged = true;
    }
    return &m_pPr;
}

odx_rPr *odx_Style::get_rPr(odx_ImportContext *context)
{
    if( !m_rPrMerged ) {
        odx_Style* pStyle = getBaseStyle(context);
        if (pStyle ) {
            m_rPr.combineWith(pStyle->get_rPr(context));
        }
        m_rPrMerged = true;
    }
    return &m_rPr;
}

odx_StylePropertiesGetter *odx_Style::getStyleProperties(odx_ImportContext *context, odx_style_type styleType)
{
    switch(styleType) {
    case odx_paragraph_style:
        return get_pPr(context);
    case odx_character_style:
        return get_rPr(context);
    default:
        break;
    }
    return NULL;
}

void odx_ImportContext::addStyle(odx_StyleRef style)
{
    odx_Style *referenced = style.get();
    if ( NULL != referenced)
    {
        m_styles.set(referenced->getId(), style);
    }
}

void odx_ImportContext::setLanguage(const lChar32 *lang)
{
    lString32 language(lang);

    int p = language.pos(cs32("-"));
    if ( p > 0  ) {
        language = language.substr(0, p);
    }
    m_doc->getProps()->setString(DOC_PROP_LANGUAGE, language);
}

lString32 odx_ImportContext::getListStyleCss(css_list_style_type_t listType)
{
    switch(listType) {
    case css_lst_disc:
        return cs32("list-style-type: disc;");
    case css_lst_circle:
        return cs32("list-style-type: circle;");
    case css_lst_square:
        return cs32("list-style-type: square;");
    case css_lst_decimal:
        return cs32("list-style-type: decimal;");
    case css_lst_lower_roman:
        return cs32("list-style-type: lower-roman;");
    case css_lst_upper_roman:
        return cs32("list-style-type: upper-roman;");
    case css_lst_lower_alpha:
        return cs32("list-style-type: lower-alpha;");
    case css_lst_upper_alpha:
        return cs32("list-style-type: upper-alpha;");
    default:
        break;
    }
    return cs32("list-style-type: none;");
}

void odx_ImportContext::startDocument(ldomDocumentWriter &writer)
{
#ifdef DOCX_FB2_DOM_STRUCTURE
    writer.OnStart(NULL);
    writer.OnTagOpen(NULL, U"?xml");
    writer.OnAttribute(NULL, U"version", U"1.0");
    writer.OnAttribute(NULL, U"encoding", U"utf-8");
    writer.OnEncoding(U"utf-8", NULL);
    writer.OnTagBody();
    writer.OnTagClose(NULL, U"?xml");
    writer.OnTagOpenNoAttr(NULL, U"FictionBook");
    // DESCRIPTION
    writer.OnTagOpenNoAttr(NULL, U"description");
    writer.OnTagOpenNoAttr(NULL, U"title-info");
    writer.OnTagOpenNoAttr(NULL, U"book-title");
    writer.OnTagClose(NULL, U"book-title");
    writer.OnTagClose(NULL, U"title-info");
    writer.OnTagClose(NULL, U"description");
#else
    writer.OnStart(NULL);
    writer.OnTagOpen(NULL, U"?xml");
    writer.OnAttribute(NULL, U"version", U"1.0");
    writer.OnAttribute(NULL, U"encoding", U"utf-8");
    writer.OnEncoding(U"utf-8", NULL);
    writer.OnTagBody();
    writer.OnTagClose(NULL, U"?xml");
    writer.OnTagOpenNoAttr(NULL, U"html");
#endif
}

void odx_ImportContext::endDocument(ldomDocumentWriter &writer)
{
#ifdef DOCX_FB2_DOM_STRUCTURE
    writer.OnTagClose(NULL, U"FictionBook");
#else
    writer.OnTagClose(NULL, U"html");
#endif
}

int odx_styleTagsHandler::styleTagPos(lChar32 ch)
{
    for (int i=0; i < m_styleTags.length(); i++)
        if (m_styleTags[i] == ch)
            return i;
    return -1;
}

const lChar32 *odx_styleTagsHandler::getStyleTagName(lChar32 ch)
{
    switch ( ch ) {
    case 'b':
        return U"strong";
    case 'i':
        return U"em";
    case 'u':
        return U"u";
    case 's':
        return U"s";
    case 't':
        return U"sup";
    case 'd':
        return U"sub";
    default:
        return NULL;
    }
}

void odx_styleTagsHandler::closeStyleTag(lChar32 ch, ldomDocumentWriter *writer)
{
    int pos = styleTagPos( ch );
    if (pos >= 0) {
        for (int i = m_styleTags.length() - 1; i >= pos; i--) {
            const lChar32 * tag = getStyleTagName(m_styleTags[i]);
            m_styleTags.erase(m_styleTags.length() - 1, 1);
            if ( tag ) {
                writer->OnTagClose(U"", tag);
            }
        }
    }
}

void odx_styleTagsHandler::openStyleTag(lChar32 ch, ldomDocumentWriter *writer)
{
    int pos = styleTagPos( ch );
    if (pos < 0) {
        const lChar32 * tag = getStyleTagName(ch);
        if ( tag ) {
            writer->OnTagOpenNoAttr(U"", tag);
            m_styleTags.append( 1,  ch );
        }
    }
}

void odx_styleTagsHandler::openStyleTags(odx_rPr *runProps, ldomDocumentWriter *writer)
{
    if(runProps->isBold())
        openStyleTag('b', writer);
    if(runProps->isItalic())
        openStyleTag('i', writer);
    if(runProps->isUnderline())
        openStyleTag('u', writer);
    if(runProps->isStrikeThrough())
        openStyleTag('s', writer);
    if(runProps->isSubScript())
        openStyleTag('d', writer);
    if(runProps->isSuperScript())
        openStyleTag('t', writer);
}

void odx_styleTagsHandler::closeStyleTags(odx_rPr *runProps, ldomDocumentWriter *writer)
{
    if(!runProps->isBold())
        closeStyleTag('b', writer);
    if(!runProps->isItalic())
        closeStyleTag('i', writer);
    if(!runProps->isUnderline())
        closeStyleTag('u', writer);
    if(!runProps->isStrikeThrough())
        closeStyleTag('s', writer);
    if(!runProps->isSubScript())
        closeStyleTag('d', writer);
    if(!runProps->isSuperScript())
        closeStyleTag('t', writer);
}

void odx_styleTagsHandler::closeStyleTags(ldomDocumentWriter *writer)
{
    for(int i = m_styleTags.length() - 1; i >= 0; i--)
        closeStyleTag(m_styleTags[i], writer);
    m_styleTags.clear();
}
