/**
    CoolReader Engine

    odtfmt.cpp: ODT support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#include "../include/odtfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/lvstreamutils.h"
#include "../include/lvxmlparser.h"
#include "../include/lvxmlutils.h"
#include "../include/crlog.h"
#include "odxutil.h"

// If you add new element - update odt_elements_mapping table below
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
    ODT_TAG2(indexBody, "index-body")\
    ODT_TAG2(lineBreak, "line-break")\
    ODT_TAG(list)\
    ODT_TAG2(listStyle, "list-style")\
    ODT_TAG2(listLevelStyleBullet, "list-level-style-bullet")\
    ODT_TAG2(listLevelStyleNumber, "list-level-style-number")\
    ODT_TAG2(listItem, "list-item")\
    ODT_TAG(note)\
    ODT_TAG2(noteBody, "note-body")\
    ODT_TAG2(noteCitation, "note-citation")\
    ODT_TAG2(noteRef, "note-ref")\
    ODT_TAG(p)\
    ODT_TAG2(paragraphProperties, "paragraph-properties")\
    ODT_TAG2(referenceMark, "reference-mark")\
    ODT_TAG2(referenceMarkStart, "reference-mark-start")\
    ODT_TAG2(referenceRef, "reference-ref")\
    ODT_TAG(s)\
    ODT_TAG(span)\
    ODT_TAG(style)\
    ODT_TAG(styles)\
    ODT_TAG(tab)\
    ODT_TAG(table)\
    ODT_TAG2(tableHeaderRows, "table-header-rows")\
    ODT_TAG2(tableOfContent, "table-of-content")\
    ODT_TAG2(tableRow, "table-row")\
    ODT_TAG2(tableCell, "table-cell")\
    ODT_TAG(text)\
    ODT_TAG2(textBox, "text-box")\
    ODT_TAG2(textProperties, "text-properties")

#define ODT_TAG_NAME(itm) odt_el_##itm##_name
#define ODT_TAG_ID(itm) odt_el_##itm
#define ODT_TAG_CHILD(itm) { ODT_TAG_ID(itm), ODT_TAG_NAME(itm) }
#define ODT_TAG_CHILD2(itm, name) { ODT_TAG_ID(itm), U ## name }
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
#define ODT_TAG(itm) static const lChar32 * const ODT_TAG_NAME(itm) = U ## #itm;
#define ODT_TAG2(itm, name) static const lChar32 * const ODT_TAG_NAME(itm) = U ## name;
    ODT_TAGS

#undef ODT_TAG
#undef ODT_TAG2
#define ODT_TAG(itm) ODT_TAG_CHILD(itm),
#define ODT_TAG2(itm, name) ODT_TAG_CHILD2(itm, name),

const struct item_def_t odt_elements_mapping[] = {
    { odt_el_NULL, NULL },
    { odt_el_a, U"a" },
    { odt_el_automaticStyles, NULL },
    { odt_el_body, U"body" },
    { odt_el_bookmark, U"a" },
    { odt_el_bookmarkRef, U"a" },
    { odt_el_bookmarkStart, U"a" },
    { odt_el_defaultStyle, NULL },
    { odt_el_documentContent, NULL },
    { odt_el_documentStyles, NULL },
    { odt_el_frame, NULL },
    { odt_el_h, NULL /*Special processing*/},
    { odt_el_image, U"img" },
    { odt_el_indexBody, NULL },
    { odt_el_lineBreak, U"br" },
    { odt_el_list, U"ol" },
    { odt_el_listStyle, NULL },
    { odt_el_listLevelStyleBullet, NULL },
    { odt_el_listLevelStyleNumber, NULL },
    { odt_el_listItem, U"li" },
    { odt_el_note, NULL },
    { odt_el_noteBody, NULL /*Special Processing */ },
    { odt_el_noteCitation, NULL /*Special Processing */ },
    { odt_el_noteRef, U"a" },
    { odt_el_p, U"p" },
    { odt_el_paragraphProperties, NULL },
    { odt_el_referenceMark, U"a" },
    { odt_el_referenceMarkStart, U"a" },
    { odt_el_referenceRef, U"a" },
    { odt_el_s, NULL },
    { odt_el_span, NULL },
    { odt_el_style, NULL },
    { odt_el_styles, NULL },
    { odt_el_tab, NULL },
    { odt_el_table, U"table" },
    { odt_el_tableHeaderRows, U"thead" },
    { odt_el_tableOfContent, NULL },
    { odt_el_tableRow, U"tr" },
    { odt_el_tableCell, U"td" },
    { odt_el_text, NULL },
    { odt_el_textBox, NULL },
    { odt_el_textProperties, NULL }
};

const struct item_def_t odt_elements[] = {
    ODT_TAG_CHILD(a),
    ODT_TAG_CHILD(automaticStyles),
    ODT_TAG_CHILD(body),
    ODT_TAG_CHILD(bookmark),
    ODT_TAG_CHILD(bookmarkRef),
    ODT_TAG_CHILD(bookmarkStart),
    ODT_TAG_CHILD(documentContent),
    ODT_TAG_CHILD(frame),
    ODT_TAG_CHILD(h),
    ODT_TAG_CHILD(image),
    ODT_TAG_CHILD(indexBody),
    ODT_TAG_CHILD(lineBreak),
    ODT_TAG_CHILD(list),
    ODT_TAG_CHILD(listItem),
    ODT_TAG_CHILD(note),
    ODT_TAG_CHILD(noteBody),
    ODT_TAG_CHILD(noteCitation),
    ODT_TAG_CHILD(noteRef),
    ODT_TAG_CHILD(p),
    ODT_TAG_CHILD(referenceMark),
    ODT_TAG_CHILD(referenceMarkStart),
    ODT_TAG_CHILD(referenceRef),
    ODT_TAG_CHILD(s),
    ODT_TAG_CHILD(span),
    ODT_TAG_CHILD(tab),
    ODT_TAG_CHILD(table),
    ODT_TAG_CHILD(tableHeaderRows),
    ODT_TAG_CHILD(tableOfContent),
    ODT_TAG_CHILD(tableRow),
    ODT_TAG_CHILD(tableCell),
    ODT_TAG_CHILD(text),
    ODT_TAG_CHILD(textBox),
    ODT_LAST_ITEM
};

const struct item_def_t odt_style_elements[] = {
    ODT_TAG_CHILD(automaticStyles),
    ODT_TAG_CHILD(defaultStyle),
    ODT_TAG_CHILD(documentStyles),
    ODT_TAG_CHILD(listStyle),
    ODT_TAG_CHILD(listLevelStyleBullet),
    ODT_TAG_CHILD(listLevelStyleNumber),
    ODT_TAG_CHILD(paragraphProperties),
    ODT_TAG_CHILD(style),
    ODT_TAG_CHILD(styles),
    ODT_TAG_CHILD(textProperties),
    ODT_LAST_ITEM
};

static const struct item_def_t styleFamily_attr_values[] = {
    { odx_paragraph_style, U"paragraph" },
    { odx_character_style, U"text"},
    ODT_LAST_ITEM
};

const struct item_def_t odt_fontWeigth_attr_values[] = {
    { 400, U"normal" },
    { 600, U"bold" },
    { 100, U"100" },
    { 200, U"200" },
    { 300, U"300" },
    { 400, U"400" },
    { 500, U"500" },
    { 600, U"600" },
    { 700, U"700" },
    { 800, U"800" },
    { 900, U"900" },
    ODT_LAST_ITEM
};

static const struct item_def_t odt_textAlign_attr_values[] =
{
    { css_ta_left, U"left" },
    { css_ta_right, U"right" },
    { css_ta_center, U"center" },
    { css_ta_justify, U"justify" },
    { css_ta_start, U"start" },
    { css_ta_end, U"end" },
    ODT_LAST_ITEM
};

bool DetectOpenDocumentFormat( LVStreamRef stream )
{
    LVContainerRef arc = LVOpenArchieve( stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive
    lString32 mimeType;
    {
        LVStreamRef mtStream = arc->OpenStream(U"mimetype", LVOM_READ );
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
    return ( mimeType == U"application/vnd.oasis.opendocument.text" );
}

class odt_ListLevelStyle : public LVRefCounter
{
    css_length_t m_lvlStart;
    css_list_style_type_t m_levelType;
    int m_level;
public:
    odt_ListLevelStyle() :
        m_lvlStart(css_val_unspecified, 0), m_levelType(css_lst_disc), m_level(0) { }
    virtual ~odt_ListLevelStyle() {}
    inline css_length_t getLevelStart() const { return m_lvlStart; }
    inline void setLevelStart( int value) {
        m_lvlStart.type = css_val_pt;
        m_lvlStart.value = value;
    }
    inline int getLevel() { return m_level; }
    inline void setLevel( const lChar32* value) { m_level = lString32(value).atoi(); }
    inline css_list_style_type_t getLevelType() { return m_levelType; }
    inline void setLevelType(css_list_style_type_t levelType) { m_levelType = levelType; }
    void reset() {}
};

typedef LVFastRef< odt_ListLevelStyle > odt_ListLevelStyleRef;

class odt_ListStyle : public LVRefCounter
{
    LVHashTable<lUInt32, odt_ListLevelStyleRef> m_levels;
    bool m_consecutiveNumbering;
    lString32 m_Id;
public:
    odt_ListStyle() : m_levels(10), m_consecutiveNumbering(false) {}
    virtual ~odt_ListStyle() {}
    odt_ListLevelStyle* getLevel(int level) {
        return m_levels.get(level).get();
    }
    inline lString32 getId() const { return m_Id; }
    inline void setId(const lChar32 * value) { m_Id = value; }
    bool getConsecutiveNumbering() { return m_consecutiveNumbering; }
    void setConsecutiveNumbering(bool value) {
        m_consecutiveNumbering = value;
    }
    void addLevel(odt_ListLevelStyleRef listLevel);
    void reset() { m_levels.clear(); }
};

typedef LVFastRef< odt_ListStyle > odt_ListStyleRef;

class odtImportContext : public odx_ImportContext
{
    LVHashTable<lString32, odt_ListStyleRef> m_ListStyles;
public:
    explicit odtImportContext(ldomDocument* doc) : odx_ImportContext(doc), m_ListStyles(64) { }
    void addListStyle(odt_ListStyleRef listStyle);
    odt_ListStyle* getListStyle(lString32 id) {
        if( !id.empty() )
            return m_ListStyles.get(id).get();
        return NULL;
    }
    LVStreamRef openFile(const lChar32 * const fileName);
};

class odt_stylesHandler : public xml_ElementHandler
{
private:
    LVArray<int> m_levels;
    odx_StyleRef m_styleRef;
    odx_Style *m_style;
    odt_ListStyleRef m_ListStyleRef;
    odt_ListStyle *m_ListStyle;
    odt_ListLevelStyleRef m_ListLevelStyleRef;
    odt_ListLevelStyle *m_ListLevelStyle;
    odx_pPr* m_pPr;
    odx_rPr* m_rPr;
    odtImportContext *m_context;
public:
    /// constructor
    odt_stylesHandler(docXMLreader * reader, ldomDocumentWriter *writer, int element,
                      odtImportContext *context) :
        xml_ElementHandler(reader, writer, element, odt_style_elements),
            m_style(NULL), m_ListStyle(NULL), m_pPr(NULL), m_rPr(NULL), m_context(context)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
};

class odt_documentHandler : public xml_ElementHandler, odx_styleTagsHandler
{
    LVArray<int> m_levels;
    LVArray<bool> m_listItems;
    LVArray<odt_ListStyle*> m_ListLevels;
    ldomDocumentWriter m_footNotesWriter;
    ldomDocumentWriter m_endNotesWriter;
    ldomDocumentWriter *m_saveWriter;
    odtImportContext * m_context;
    ldomNode *m_footNotes;
    ldomNode *m_endNotes;
    ldomNode *m_body;
    lString32 m_noteId;
    lString32 m_noteRefText;
    lString32 m_StyleName;
    lString32 m_spanStyleName;
    bool m_isEndNote;
    bool m_paragraphStarted;
    odt_stylesHandler m_stylesHandler;
private:
    ldomNode* startNotes(const lChar32 * notesKind) {
        m_writer->OnStart(NULL);
#ifdef ODX_CRENGINE_IN_PAGE_FOOTNOTES
        ldomNode* notes = m_writer->OnTagOpen(U"", U"body");
        m_writer->OnAttribute(U"", U"name", notesKind);
#else
        ldomNode* notes = m_writer->OnTagOpen(U"", U"div");
        m_writer->OnAttribute(U"", U"style", U"page-break-before: always");
#endif
        m_writer->OnTagBody();
        return notes;
    }
    void finishNotes(ldomNode* notes, ldomDocumentWriter& writer) {
        ldomNode* parent = notes->getParentNode();
        int index = notes->getNodeIndex();
#ifdef ODX_CRENGINE_IN_PAGE_FOOTNOTES
        writer.OnTagClose(U"", U"body");
#else
        writer.OnTagClose(U"", U"div");
#endif
        writer.OnStop();
        parent->moveItemsTo(m_body->getParentNode(), index, index);
    }
    inline void checkParagraphStart() {
        if( !m_paragraphStarted ) {
            startParagraph();
        }
    }
    void startParagraph();
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
        m_footNotesWriter(doc), m_endNotesWriter(doc), m_saveWriter(NULL),
        m_context(context), m_footNotes(NULL), m_endNotes(NULL), m_body(NULL),
        m_isEndNote(false), m_titleHandler(titleHandler),
        m_outlineLevel(0), m_inTable(false), m_inListItem(false),
        m_stylesHandler(reader, NULL, odt_el_automaticStyles, context),
        m_listItemHadContent(false), m_paragraphStarted(false) {
    }
    inline bool isInList() { return m_ListLevels.length() != 0; }
    ldomNode *handleTagOpen(int tagId);
    void handleAttribute(const lChar32 *attrname, const lChar32 *attrValue);
    void handleTagBody();
    void handleTagClose(const lChar32 *nsname, const lChar32 *tagname);
    void handleText(const lChar32 *text, int len, lUInt32 flags);
    void reset();
};

static bool parseStyles(odtImportContext *context)
{
    LVStreamRef stream = context->openFile(U"styles.xml");
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

    //Read document metadata
    LVStreamRef meta_stream = arc->OpenStream(U"meta.xml", LVOM_READ);
    if ( meta_stream.isNull() )
        return false;
    ldomDocument * metaDoc = LVParseXMLStream( meta_stream );
    if ( metaDoc ) {
        CRPropRef doc_props = doc->getProps();

        lString32 author = metaDoc->textFromXPath( cs32("document-meta/meta/creator") );
        lString32 title = metaDoc->textFromXPath( cs32("document-meta/meta/title") );
        lString32 description = metaDoc->textFromXPath( cs32("document-meta/meta/description") );
        doc_props->setString(DOC_PROP_TITLE, title);
        doc_props->setString(DOC_PROP_AUTHORS, author );
        doc_props->setString(DOC_PROP_DESCRIPTION, description );
        delete metaDoc;
    } else {
        CRLog::error("Couldn't parse document meta data");
    }

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

    odtImportContext importContext(doc);
    if( !parseStyles(&importContext) )
        return false;

    LVStreamRef m_stream = arc->OpenStream(U"content.xml", LVOM_READ);
    if ( m_stream.isNull() )
        return false;

    importContext.startDocument(writer);

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

    importContext.endDocument(writer);
    writer.OnStop();

    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        doc->compact();
        doc->dumpStatistics();
    }

    return true;
}

void odt_documentHandler::startParagraph()
{
    if(m_inListItem) {
        m_listItemHadContent = true;
        m_writer->OnTagOpenNoAttr(U"", U"li");
    }
    m_writer->OnTagOpen(U"", U"p");
    odx_Style* style = m_context->getStyle(m_StyleName);
    if( style ) {
        odx_pPr pPr;

        pPr.combineWith(style->get_pPr(m_context));
        pPr.combineWith(m_context->get_pPrDefault());
        lString32 css = pPr.getCss();
        if( !css.empty() )
            m_writer->OnAttribute(U"", U"style", css.c_str());
    }
    m_writer->OnTagBody();
#ifndef DOCX_FB2_DOM_STRUCTURE
    if(m_state == odt_el_noteBody) {
        m_writer->OnTagOpen(U"", U"sup");
        m_writer->OnTagBody();
        m_writer->OnText(m_noteRefText.c_str(), m_noteRefText.length(), 0);
        m_writer->OnTagClose(U"", U"sup");
    }
#endif
    m_paragraphStarted = true;
}

ldomNode *odt_documentHandler::handleTagOpen(int tagId)
{
    bool elementHandled = false;
    switch(tagId) {
    case odt_el_automaticStyles:
        m_stylesHandler.start();
        elementHandled = true;
        break;
    case odt_el_note:
        m_isEndNote = false;
        m_writer->OnTagOpen(U"", U"a");
        break;
    case odt_el_body:
        m_body = m_titleHandler->onBodyStart();
        m_writer->OnTagBody();
        break;
    case odt_el_h:
        if( odt_el_p == m_state)
            checkParagraphStart();
        m_StyleName.clear();
        m_outlineLevel = 0;
        break;
    case odt_el_p:
#ifndef DOCX_FB2_DOM_STRUCTURE
        if( odt_el_p == m_state || odt_el_noteBody == m_state)
            checkParagraphStart();
        m_paragraphStarted = odt_el_noteBody == m_state;
#else
        if( odt_el_p == m_state)
            checkParagraphStart();
        m_paragraphStarted = false;
#endif
        m_StyleName.clear();
        break;
    case odt_el_span:
        m_spanStyleName.clear();
        break;
    case odt_el_list:
        if( odt_el_p == m_state)
            checkParagraphStart();
        m_StyleName.clear();
        m_writer->OnTagOpen(U"", U"ol");
        if (isInList())
            m_listItems.add(m_listItemHadContent);
        break;
    case odt_el_listItem:
        m_inListItem = true;
        m_listItemHadContent = false;
        break;
    case odt_el_tab:
    case odt_el_s:
        if( odt_el_p == m_state)
            checkParagraphStart();
        m_writer->OnText(U" ", 1, TXTFLG_PRE);
        break;
    case odt_el_noteBody:
        m_saveWriter = m_writer;
        if(m_isEndNote) {
            m_writer = &m_endNotesWriter;
            if(!m_endNotes)
                m_endNotes = startNotes(U"comments");
        } else {
            m_writer = &m_footNotesWriter;
            if(!m_footNotes)
                m_footNotes = startNotes(U"notes");;
        }
        m_writer->OnTagOpen(U"", U"section");
        m_writer->OnAttribute(U"", U"id", m_noteId.c_str());
        m_writer->OnAttribute(U"", U"role", m_isEndNote ? U"doc-rearnote" : U"doc-footnote");
        m_writer->OnTagBody();
#ifdef DOCX_FB2_DOM_STRUCTURE
        m_writer->OnTagOpenNoAttr(U"", U"title");
        m_writer->OnTagOpenNoAttr(U"", U"p");
        m_writer->OnText(m_noteRefText.c_str(), m_noteRefText.length(), 0);
        m_writer->OnTagClose(U"", U"p");
        m_writer->OnTagClose(U"", U"title");
#else
        m_paragraphStarted = false;
#endif
        break;
    case odt_el_table:
        m_inTable = true;
    default:
        if( odt_elements_mapping[tagId].name ) {
            if( odt_el_p == m_state)
                checkParagraphStart();
            m_writer->OnTagOpen(U"", odt_elements_mapping[tagId].name);
        }
        break;
    }
    if( !elementHandled ) {
        m_state = tagId;
        m_levels.add(tagId);
    }
    return NULL;
}

void odt_documentHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrValue)
{
    switch (m_state) {
    case odt_el_bookmark:
    case odt_el_bookmarkStart:
    case odt_el_referenceMark:
    case odt_el_referenceMarkStart:
        if(!lStr_cmp(attrname, "name") ) {
            m_writer->OnAttribute(U"", U"id", attrValue);
        }
        break;
    case odt_el_bookmarkRef:
    case odt_el_noteRef:
    case odt_el_referenceRef:
        if(!lStr_cmp(attrname, "ref-name") ) {
            lString32 target = cs32("#") + lString32(attrValue);
            m_writer->OnAttribute(U"", U"href", target.c_str());
        }
        break;
    case odt_el_h:
        if(!lStr_cmp(attrname, "outline-level") ) {
            lString32 value = attrValue;
            int tmp;

            if(value.atoi(tmp))
                m_outlineLevel = tmp - 1;
            break;
        }
        if( !lStr_cmp(attrname, "style-name") ) {
            m_StyleName = attrValue;
        }
        break;
    case odt_el_note:
        if(!lStr_cmp(attrname, "note-class")) {
            if(!lStr_cmp(attrValue, "endnote")) {
                m_writer->OnAttribute(U"", U"type", U"comment");
                m_isEndNote = true;
            } else if(!lStr_cmp(attrValue, "footnote")) {
                m_writer->OnAttribute(U"", U"type", U"note");
            }
            m_writer->OnAttribute(U"", U"role", U"doc-noteref");
        } else if(!lStr_cmp(attrname, "id")) {
            m_noteId = lString32(attrValue);
            lString32 target = cs32("#") + m_noteId;
            m_writer->OnAttribute(U"", U"href", target.c_str());
        }
        break;
    case odt_el_tableCell:
        if(!lStr_cmp(attrname, "number-columns-spanned"))
            m_writer->OnAttribute(U"", U"colspan", attrValue);
        else if(!lStr_cmp(attrname, "number-rows-spanned"))
            m_writer->OnAttribute(U"", U"rowspan", attrValue);
        break;
    case odt_el_a:
    case odt_el_image:
        if( !lStr_cmp(attrname, "href") )
            m_writer->OnAttribute(U"", attrname, attrValue);
        break;
    case odt_el_p:
    case odt_el_list:
    case odt_el_span:
        if( !lStr_cmp(attrname, "style-name") ) {
            if( m_state == odt_el_span)
                m_spanStyleName = attrValue;
            else
                m_StyleName = attrValue;
        }
        break;
    default:
        break;
    }
}

void odt_documentHandler::handleTagBody()
{
    switch(m_state) {
    case odt_el_span:
    case odt_el_p:
        break;
    case odt_el_list:
        {
            css_list_style_type_t listType = css_lst_none;
            css_length_t levelStart(css_val_unspecified, 0);
            odt_ListStyle* listStyle = m_context->getListStyle(m_StyleName);
            if( !listStyle && !m_ListLevels.empty() ) {
                listStyle = m_ListLevels.get( m_ListLevels.length() -1);
            }
            m_ListLevels.add(listStyle);
            if(listStyle) {
                odt_ListLevelStyle* levelStyle = listStyle->getLevel(m_ListLevels.length());
                if(levelStyle) {
                    listType = levelStyle->getLevelType();
                    levelStart = levelStyle->getLevelStart();
                }
            }
            m_writer->OnAttribute(U"", U"style", m_context->getListStyleCss(listType).c_str());
            if(levelStart.type != css_val_unspecified)
                m_writer->OnAttribute(U"", U"start", lString32::itoa(levelStart.value).c_str());
            m_writer->OnTagBody();
        }
        break;
    case odt_el_h:
        if(m_inListItem) {
            m_listItemHadContent = true;
            m_writer->OnTagOpenNoAttr(U"", U"li");
        }
        m_titleHandler->onTitleStart(m_outlineLevel + 1, m_inTable || m_inListItem);
        m_writer->OnTagBody();
        break;
    default:
        if( odt_elements_mapping[m_state].name )
            m_writer->OnTagBody();
        break;
    }
}

void odt_documentHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    switch(m_state) {
    case odt_el_body:
        m_titleHandler->onBodyEnd();
        m_writer->OnTagClose(nsname, tagname);
        if(m_footNotes)
            finishNotes(m_footNotes, m_footNotesWriter);
        if(m_endNotes)
            finishNotes(m_endNotes, m_endNotesWriter);
        break;
    case odt_el_h:
        closeStyleTags(m_writer);
        m_titleHandler->onTitleEnd();
        break;
    case odt_el_p:
        if( !m_paragraphStarted ) {
            if( m_inListItem) {
                m_writer->OnTagOpen(U"", U"li");
                m_writer->OnAttribute(U"", U"style", U"display:none");
                m_writer->OnTagBody();
                m_writer->OnTagClose(U"", U"li");
            } else
                m_writer->OnTagOpenNoAttr(U"", U"p");
            m_paragraphStarted = true;
        } else
            closeStyleTags(m_writer);
        m_writer->OnTagClose(nsname, tagname);
        break;
    case odt_el_list:
        m_ListLevels.remove(m_ListLevels.length() - 1);
        m_writer->OnTagClose(U"", U"ol");
        break;
    case odt_el_listItem:
        if(m_listItemHadContent)
            m_writer->OnTagClose(U"", U"li");
        if(!m_listItems.empty()) {
            m_listItemHadContent = m_listItems[m_listItems.length() - 1];
            m_listItems.erase(m_listItems.length() - 1, 1);
        }
        m_inListItem = false;
        break;
    case odt_el_noteBody:
        m_writer->OnTagClose(U"", U"section");
        m_writer = m_saveWriter;
        break;
    case odt_el_noteCitation:
        m_writer->OnTagClose(U"", U"a");
        break;
    case odt_el_table:
        m_inTable = false;
    default:
        if( odt_elements_mapping[m_state].name )
            m_writer->OnTagClose(U"", odt_elements_mapping[m_state].name);
        break;
    }
    m_levels.erase(m_levels.length() - 1, 1);
    if( !m_levels.empty() )
        m_state = m_levels[m_levels.length() - 1];
    else
        m_state = odt_el_NULL;
}

void odt_documentHandler::handleText(const lChar32 *text, int len, lUInt32 flags)
{
    odx_Style* style;

    switch(m_state) {
    case odt_el_h:
    case odt_el_p:
    case odt_el_span:
        style = m_context->getStyle(m_state == odt_el_span ? m_spanStyleName : m_StyleName);
        if(style) {
            odx_rPr rPr;

            rPr.combineWith(style->get_rPr(m_context));
            rPr.combineWith(m_context->get_rPrDefault());
            if( rPr.isHidden() )
                break;
            checkParagraphStart();
            closeStyleTags(&rPr, m_writer);
            openStyleTags(&rPr, m_writer);
        } else
            checkParagraphStart();
        m_writer->OnText(text, len, flags);
        break;
    case odt_el_noteCitation:
        m_noteRefText = text;
        m_writer->OnTagBody();
    case odt_el_bookmarkRef:
    case odt_el_noteRef:
    case odt_el_referenceRef:
    case odt_el_a:
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
    m_paragraphStarted = false;
}

void odtImportContext::addListStyle(odt_ListStyleRef listStyle)
{
    if( !listStyle.isNull())
        m_ListStyles.set(listStyle->getId(), listStyle);
}

LVStreamRef odtImportContext::openFile(const lChar32 * const fileName)
{
    LVContainerRef container = m_doc->getContainer();
    if( !container.isNull() )
        return container->OpenStream(fileName, LVOM_READ);
    return LVStreamRef();
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
    case odt_el_listStyle:
        m_ListStyleRef = odt_ListStyleRef( new odt_ListStyle );
        m_ListStyle = m_ListStyleRef.get();
        break;
    case odt_el_listLevelStyleBullet:
    case odt_el_listLevelStyleNumber:
        m_ListLevelStyleRef = odt_ListLevelStyleRef( new odt_ListLevelStyle );
        m_ListLevelStyle = m_ListLevelStyleRef.get();
        break;
    default:
        break;
    }
    m_state = tagId;
    m_levels.add(tagId);
    return NULL;
}

void odt_stylesHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
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
    case odt_el_listStyle:
        if( !lStr_cmp(attrname, "name") )
            m_ListStyle->setId(attrvalue);
        break;
    case odt_el_listLevelStyleNumber:
        if( !lStr_cmp(attrname, "num-format") ) {
            lString32 format(attrvalue);

            if( format.length() == 1) {
                switch(attrvalue[0]) {
                case '1':
                    m_ListLevelStyle->setLevelType(css_lst_decimal);
                    break;
                case 'a':
                    m_ListLevelStyle->setLevelType(css_lst_lower_alpha);
                    break;
                case 'A':
                    m_ListLevelStyle->setLevelType(css_lst_upper_alpha);
                    break;
                case 'i':
                    m_ListLevelStyle->setLevelType(css_lst_lower_roman);
                    break;
                case 'I':
                    m_ListLevelStyle->setLevelType(css_lst_upper_roman);
                    break;
                default:
                    m_ListLevelStyle->setLevelType(css_lst_none);
                    break;
                }
            } else if( format.empty() )
                m_ListLevelStyle->setLevelType(css_lst_none);
            break;
        }
        if( !lStr_cmp(attrname, "start-value") ) {
            int startValue;

            if( lString32(attrvalue).atoi( startValue) )
                m_ListLevelStyle->setLevelStart( startValue );
            break;
        }
    case odt_el_listLevelStyleBullet:
        if( !lStr_cmp(attrname, "level") )
            m_ListLevelStyle->setLevel(attrvalue);
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
        if( NULL == m_style && !lStr_cmp(attrname, "language") ) {
            m_context->setLanguage(attrvalue);
        } else if( !lStr_cmp(attrname, "font-style") ) {
            m_rPr->setItalic( lStr_cmp(attrvalue, "normal") !=0 );
        } else if( !lStr_cmp(attrname, "font-weight") ) {
            int n = parse_name(odt_fontWeigth_attr_values, attrvalue);
            if( n != -1 )
                m_rPr->setBold( n >= 600 );
        } else if( !lStr_cmp(attrname, "text-underline-style") ) {
            m_rPr->setUnderline( lStr_cmp( attrvalue, "none") !=0 );
        } else if( !lStr_cmp(attrname, "text-line-through-type") ) {
            m_rPr->setStrikeThrough( lStr_cmp( attrvalue, "none") !=0 );
        } else if( !lStr_cmp(attrname, "text-position") ) {
            lString32 val = attrvalue;

            if( val.startsWith(U"super") ) {
                m_rPr->setVertAlign(css_va_super);
            } else if( val.startsWith(U"sub") ) {
                m_rPr->setVertAlign(css_va_sub);
            }
        }
        break;
    default:
        break;
    }
}

void odt_stylesHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case odt_el_listStyle:
        m_context->addListStyle(m_ListStyleRef);
        break;
    case odt_el_listLevelStyleBullet:
    case odt_el_listLevelStyleNumber:
        m_ListStyle->addLevel(m_ListLevelStyleRef);
        break;
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

void odt_ListStyle::addLevel(odt_ListLevelStyleRef listLevel)
{
    if( !listLevel.isNull() )
        m_levels.set( listLevel->getLevel(), listLevel );
}
