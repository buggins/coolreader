#include "../include/docxfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/crlog.h"
#include "odxutil.h"

#define DOCX_TAG_NAME(itm) docx_el_##itm##_name
#define DOCX_TAG_ID(itm) docx_el_##itm
#define DOCX_TAG_CHILD(itm) { DOCX_TAG_ID(itm), DOCX_TAG_NAME(itm) }
#define DOCX_LAST_ITEM { -1, NULL }

static const lChar32* const docx_DocumentContentType   = U"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml";
static const lChar32* const docx_NumberingContentType  = U"application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml";
static const lChar32* const docx_StylesContentType     = U"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml";
static const lChar32* const docx_ImageRelationship     = U"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image";
static const lChar32* const docx_HyperlinkRelationship = U"http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink";
static const lChar32* const docx_FootNotesRelationShip = U"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes";
static const lChar32* const docx_EndNotesRelationShip  = U"http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes";

enum {
#define DOCX_NUM_FMT(itm)
#define DOCX_TAG(itm) 	DOCX_TAG_ID(itm),
    docx_el_NULL = 0,
    #include "docxdtd.inc"
    docx_el_MAX_ID
};

#define DOCX_NUM_FMT(itm)
#define DOCX_TAG(itm) static const lChar32 * const DOCX_TAG_NAME(itm) = U ## #itm;
    #include "docxdtd.inc"

const struct item_def_t styles_elements[] = {
    DOCX_TAG_CHILD(styles),
    DOCX_TAG_CHILD(style),
    DOCX_TAG_CHILD(docDefaults),
    DOCX_LAST_ITEM
};

enum docx_multilevel_type {
    docx_hybrid_multilevel,
    docx_multilevel,
    docx_singlelevel
};

#define DOCX_NUM_FMT(itm) docx_numFormat_##itm ,
#define DOCX_TAG(itm)
enum docx_numFormat_type {
    #include "docxdtd.inc"
    docx_numFormat_MAX_ID
};

enum docx_LevelSuffix_type {
    docx_level_suffix_tab,
    docx_level_suffix_space,
    docx_level_suffix_nothing
};

const struct item_def_t style_elements[] = {
    DOCX_TAG_CHILD(name),
    DOCX_TAG_CHILD(basedOn),
    DOCX_TAG_CHILD(pPr),
    DOCX_TAG_CHILD(rPr),
    DOCX_TAG_CHILD(tblPr),
    DOCX_TAG_CHILD(trPr),
    DOCX_TAG_CHILD(tcPr),
    DOCX_LAST_ITEM
};

const struct item_def_t docDefaults_elements[] = {
    DOCX_TAG_CHILD(pPr),
    DOCX_TAG_CHILD(pPrDefault),
    DOCX_TAG_CHILD(rPrDefault),
    DOCX_TAG_CHILD(rPr),
    DOCX_LAST_ITEM
};

const struct item_def_t rPr_elements[] = {
    DOCX_TAG_CHILD(b),
    DOCX_TAG_CHILD(i),
    DOCX_TAG_CHILD(color),
    DOCX_TAG_CHILD(jc),
    DOCX_TAG_CHILD(lang),
    DOCX_TAG_CHILD(rFonts),
    DOCX_TAG_CHILD(rStyle),
    DOCX_TAG_CHILD(u),
    DOCX_TAG_CHILD(vertAlign),
    DOCX_TAG_CHILD(sz),
    DOCX_TAG_CHILD(vanish),
    DOCX_TAG_CHILD(strike),
    DOCX_LAST_ITEM
};

const struct item_def_t numPr_elements[] = {
    DOCX_TAG_CHILD(ilvl),
    DOCX_TAG_CHILD(numId),
    DOCX_LAST_ITEM
};

const struct item_def_t pPr_elements[] = {
    DOCX_TAG_CHILD(pageBreakBefore),
    DOCX_TAG_CHILD(keepNext),
    DOCX_TAG_CHILD(pStyle),
    DOCX_TAG_CHILD(jc),
    DOCX_TAG_CHILD(spacing),
    DOCX_TAG_CHILD(numPr),
    DOCX_TAG_CHILD(textAlignment),
    DOCX_TAG_CHILD(ind),
    DOCX_TAG_CHILD(suppressAutoHyphens),
//    DOCX_TAG_CHILD(rPr), don't care about Paragraph merker formatting
    DOCX_TAG_CHILD(outlineLvl),
    DOCX_LAST_ITEM
};

const struct item_def_t p_elements[] = {
    DOCX_TAG_CHILD(r),
    DOCX_TAG_CHILD(pPr),
    DOCX_TAG_CHILD(hyperlink),
    DOCX_TAG_CHILD(bookmarkStart),
    DOCX_LAST_ITEM
};

const struct item_def_t r_elements[] = {
    DOCX_TAG_CHILD(br),
    DOCX_TAG_CHILD(t),
    DOCX_TAG_CHILD(tab),
    DOCX_TAG_CHILD(drawing),
    DOCX_TAG_CHILD(rPr),
    DOCX_TAG_CHILD(footnoteReference),
    DOCX_TAG_CHILD(footnoteRef),
    DOCX_TAG_CHILD(endnoteReference),
    DOCX_TAG_CHILD(endnoteRef),
    DOCX_TAG_CHILD(fldChar),
    DOCX_TAG_CHILD(instrText),
    DOCX_LAST_ITEM
};

const struct item_def_t hyperlink_elements[] = {
    DOCX_TAG_CHILD(r),
    DOCX_LAST_ITEM
};

const struct item_def_t drawing_elements[] = {
    DOCX_TAG_CHILD(blipFill),
    DOCX_TAG_CHILD(blip),
    DOCX_TAG_CHILD(graphic),
    DOCX_TAG_CHILD(graphicData),
    DOCX_TAG_CHILD(inline),
    DOCX_TAG_CHILD(anchor),
    DOCX_TAG_CHILD(pic),
    DOCX_LAST_ITEM
};

const struct item_def_t tbl_elements[] = {
    DOCX_TAG_CHILD(bookmarkStart),
    DOCX_TAG_CHILD(tblPr),
    DOCX_TAG_CHILD(tblGrid),
    DOCX_TAG_CHILD(tcPr),
    DOCX_TAG_CHILD(gridCol),
    DOCX_TAG_CHILD(gridSpan),
    DOCX_TAG_CHILD(tr),
    DOCX_TAG_CHILD(tc),
    DOCX_TAG_CHILD(p),
    DOCX_TAG_CHILD(vMerge),
    DOCX_LAST_ITEM
};

const struct item_def_t lvl_elements[] = {
    DOCX_TAG_CHILD(isLgl),
    DOCX_TAG_CHILD(lvlJc),
    DOCX_TAG_CHILD(lvlRestart),
    DOCX_TAG_CHILD(lvlText),
    DOCX_TAG_CHILD(numFmt),
    DOCX_TAG_CHILD(pPr),
    DOCX_TAG_CHILD(pStyle),
    DOCX_TAG_CHILD(rPr),
    DOCX_TAG_CHILD(start),
    DOCX_TAG_CHILD(suff),
    DOCX_LAST_ITEM
};

const struct item_def_t numbering_elements[] = {
    DOCX_TAG_CHILD(numbering),
    DOCX_TAG_CHILD(abstractNum),
    DOCX_TAG_CHILD(num),
    DOCX_LAST_ITEM
};

const struct item_def_t abstractNum_elements[] = {
    DOCX_TAG_CHILD(lvl),
    DOCX_LAST_ITEM
};

const struct item_def_t num_elements[] = {
    DOCX_TAG_CHILD(abstractNumId),
    DOCX_TAG_CHILD(lvlOverride),
    DOCX_LAST_ITEM
};


const struct item_def_t document_elements[] = {
    DOCX_TAG_CHILD(document),
    DOCX_TAG_CHILD(body),
    DOCX_TAG_CHILD(p),
    DOCX_TAG_CHILD(tbl),
    DOCX_LAST_ITEM
};

const struct item_def_t footnotes_elements[] = {
    DOCX_TAG_CHILD(footnotes),
    DOCX_TAG_CHILD(footnote),
    DOCX_TAG_CHILD(endnotes),
    DOCX_TAG_CHILD(endnote),
    DOCX_TAG_CHILD(p),
    DOCX_LAST_ITEM
};

const struct item_def_t no_elements[] = {
    DOCX_LAST_ITEM
};

const struct item_def_t jc_attr_values[] = {
    { css_ta_left, U"left"},
    { css_ta_right, U"right" },
    { css_ta_center, U"center" },
    { css_ta_justify, U"both" },
    DOCX_LAST_ITEM
};

const struct item_def_t vertAlign_attr_values[] = {
    { css_va_baseline, U"baseline"},
    { css_va_super, U"superscript" },
    { css_va_sub, U"subscript" },
    DOCX_LAST_ITEM
};

const struct item_def_t textAlignment_attr_values[] = {
    { css_va_inherit, U"auto" },
    { css_va_baseline, U"baseline"},
    { css_va_bottom, U"bottom"},
    { css_va_middle, U"center" },
    { css_va_top, U"top" },
    DOCX_LAST_ITEM
};

const struct item_def_t lineRule_attr_values[] = {
    { odx_lineRule_atLeast, U"atLeast" },
    { odx_lineRule_auto, U"auto"},
    { odx_lineRule_exact, U"exact"},
    DOCX_LAST_ITEM
};

const struct item_def_t styleType_attr_values[] = {
    { odx_paragraph_style, U"paragraph" },
    { odx_character_style, U"character"},
    { odx_numbering_style, U"numbering"},
    { odx_table_style, U"table"},
    DOCX_LAST_ITEM
};

const struct item_def_t lvlSuff_attr_values[] = {
    { docx_level_suffix_tab, U"tab" },
    { docx_level_suffix_space, U"space" },
    { docx_level_suffix_nothing, U"nothing" },
    DOCX_LAST_ITEM
};

#define DOCX_TAG(itm)
#define DOCX_NUM_FMT(itm) { docx_numFormat_##itm , U ## #itm },
const struct item_def_t numFmt_attr_values[] = {
    #include "docxdtd.inc"
    DOCX_LAST_ITEM
};

bool DetectDocXFormat( LVStreamRef stream )
{
    LVContainerRef m_arc = LVOpenArchieve( stream );
    if ( m_arc.isNull() )
        return false; // not a ZIP archive

    OpcPackage package(m_arc);

    return package.partExist(package.getContentPartName(docx_DocumentContentType));
}

class docxImportContext;

class docxNumLevel : public LVRefCounter
{
private:
    bool m_isLgl;
    css_text_align_t m_lvlJc;
    css_length_t m_ilvl;
    css_length_t m_lvlRestart;
    lString32 m_lvlText;
    bool m_lvlTextNull;
    docx_numFormat_type m_lvlNumFormat;
    odx_pPr m_pPr;
    odx_rPr m_rPr;
    lString32 m_pStyle;
    css_length_t m_lvlStart;
    docx_LevelSuffix_type m_suffix;
public:
    docxNumLevel();
    virtual ~docxNumLevel() {}
    void reset();
    ///properties
    inline bool isLgl() const { return m_isLgl; }
    inline void setLgl(bool value) { m_isLgl = value; }

    inline css_text_align_t getLevelAlign() const { return m_lvlJc; }
    inline void setLevelAlign( css_text_align_t value ) { m_lvlJc = value; }
    inline css_length_t getLevel() const { return m_ilvl; }
    inline void setLevel(const css_length_t &value) { m_ilvl = value; }
    inline css_length_t getLevelRestart() const { return m_lvlRestart; }
    inline void setLevelRestart(const css_length_t &value) { m_lvlRestart = value; }
    inline lString32 getLevelText() const { return m_lvlText; }
    inline void setLevelText(const lString32 value) { m_lvlText = value; }
    inline bool getLevelTextNull() const { return m_lvlTextNull; }
    inline void setLevelTextNull(const bool value) { m_lvlTextNull = value; }
    inline docx_numFormat_type getNumberFormat() const { return m_lvlNumFormat; }
    inline void setNumberFormat(const docx_numFormat_type value) { m_lvlNumFormat = value; }
    inline lString32 getReferencedStyleId() const { return m_pStyle; }
    inline void setReferencedStyleId(const lString32 value) { m_pStyle = value; }
    inline css_length_t getLevelStart() const { return m_lvlStart; }
    inline void setLevelStart(const css_length_t &value) { m_lvlStart = value; }
    inline docx_LevelSuffix_type getLevelSuffix() const { return m_suffix; }
    inline void setLevelSuffix(const docx_LevelSuffix_type value) { m_suffix = value; }
    inline odx_rPr * get_rPr() { return &m_rPr; }
    inline odx_pPr * get_pPr() { return &m_pPr; }
    css_list_style_type_t getListType() const;
};

typedef LVFastRef< docxNumLevel > docxNumLevelRef;

class docxAbstractNum : public LVRefCounter
{
private:
    docx_multilevel_type m_multilevel;
    css_length_t m_abstractNumId;
    LVHashTable<lUInt32, docxNumLevelRef> m_levels;
public:
    docxAbstractNum();
    docxNumLevel* getLevel(int level);
    void addLevel(docxNumLevelRef docxLevel);
    void setId(int id) { m_abstractNumId.value = id; m_abstractNumId.type = css_val_in; }
    int getId() { return m_abstractNumId.value; }
    virtual ~docxAbstractNum() {}
    void reset();
};

typedef LVFastRef< docxAbstractNum > docxAbstractNumRef;

class docxNum : public LVRefCounter
{
private:
    css_length_t m_id;
    css_length_t m_abstractNumId;
    LVHashTable<lUInt32, docxNumLevelRef> m_overrides;
public:
    docxNum() : m_id(css_val_unspecified, 0), m_abstractNumId(css_val_unspecified, 0),
        m_overrides(10) {
    }
    const docxAbstractNumRef getBase(docxImportContext &context) const;
    void setId(int id) { m_id.value = id; m_id.type = css_val_in; }
    int getId() const { return m_id.value; }
    void setBaseId(int id) { m_abstractNumId.value = id; m_abstractNumId.type = css_val_in; }
    int getBaseId() const { return m_abstractNumId.value; }
    void overrideLevel(docxNumLevelRef docxLevel);
    docxNumLevel* getDocxLevel(docxImportContext &context, int level);
    bool isValid() const;
    void reset();
};

typedef LVFastRef< docxNum > docxNumRef;

class docxImportContext : public odx_ImportContext
{
private:
    LVHashTable<lUInt32, docxAbstractNumRef> m_abstractNumbers;
    LVHashTable<lUInt32, docxNumRef> m_Numbers;
    LVArray<css_list_style_type_t> m_ListLevels;
    OpcPartRef m_docPart;
    OpcPartRef m_relatedPart;
    OpcPackage* m_package;
public:
    docxImportContext(OpcPackage *package, ldomDocument * doc);
    virtual ~docxImportContext();
    void addNum( docxNumRef num );
    void addAbstractNum(docxAbstractNumRef abstractNum );
    docxNumRef getNum(lUInt32 id) { return m_Numbers.get(id); }
    docxAbstractNumRef getAbstractNum(lUInt32 id) { return m_abstractNumbers.get(id); }
    lString32 getImageTarget(lString32 id) {
        return getRelationTarget(docx_ImageRelationship, id);
    }
    lString32 getLinkTarget(lString32 id) {
        return getRelationTarget(docx_HyperlinkRelationship, id);
    }
    lString32 getRelationTarget(const lChar32 * const relationType, lString32 id) {
        if ( !m_relatedPart.isNull() )
            return m_relatedPart->getRelatedPartName(relationType, id);
        return m_docPart->getRelatedPartName(relationType, id);
    }
    LVStreamRef openContentPart(const lChar32 * const contentType);
    LVStreamRef openRelatedPart(const lChar32 * const relationshipType);
    void closeRelatedPart();
    void openList(int level, int numid, ldomDocumentWriter *writer);
    void closeList(int level, ldomDocumentWriter *writer);
    inline int getListLevel() { return m_ListLevels.length(); }
    inline bool isInList() { return m_ListLevels.length() != 0; }
    lString32 m_footNoteId;
    int m_footNoteCount;
    int m_endNoteCount;
    bool m_inField;
    ldomNode *m_linkNode;
    odx_Style* m_pStyle;
private:
    lString32 getListStyle(css_list_style_type_t listType);
};

class docx_ElementHandler : public xml_ElementHandler
{
protected:
    docxImportContext *m_importContext;
protected:
    static bool parse_OnOff_attribute(const lChar32 * attrValue);
    void generateLink(const lChar32 * target, const lChar32 * type, const lChar32 *text);
    docx_ElementHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context,
                        int element, const struct item_def_t *children) : xml_ElementHandler(reader, writer, element, children),
        m_importContext(context)
    {
    }
    virtual ~docx_ElementHandler() {}
};

class docx_rPrHandler : public docx_ElementHandler
{
private:
    odx_rPr *m_rPr;
public:
    docx_rPrHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_rPr, rPr_elements), m_rPr(NULL)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void start(odx_rPr *rPr);
    void reset();
};

class docx_drawingHandler  : public docx_ElementHandler
{
private:
    int m_level;
public:
    docx_drawingHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_drawing, drawing_elements), m_level(0)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void reset() { m_level = 1; }
};

class docx_pHandler;
class docx_rHandler : public docx_ElementHandler
{
private:
    odx_rPr m_rPr;
    docx_pHandler* m_pHandler;
    docx_rPrHandler m_rPrHandler;
    lString32 m_footnoteId;
    lString32 m_instruction;
    docx_drawingHandler m_drawingHandler;
    bool m_content;
private:
    void handleInstruction(lString32& instruction, lString32 parameters);
public:
    docx_rHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_pHandler* pHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_r, r_elements), m_pHandler(pHandler),
        m_rPrHandler(reader, writer, context),
        m_drawingHandler(reader, writer, context),
        m_content(false)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleText( const lChar32 * text, int len, lUInt32 flags );
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void reset();
};

class docx_pPrHandler : public docx_ElementHandler
{
private:
    odx_pPr *m_pPr;
public:
    docx_pPrHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_pPr, pPr_elements), m_pPr(NULL)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void start(odx_pPr *pPr);
    void reset();
};

class docx_hyperlinkHandler  : public docx_ElementHandler
{
    docx_rHandler m_rHandler;
    lString32 m_target;
    int m_runCount;
public:
    docx_hyperlinkHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_pHandler* pHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_hyperlink, hyperlink_elements),
        m_rHandler(reader, writer, context, pHandler), m_runCount(0)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void reset() { m_target.clear(); m_rHandler.reset(); m_runCount = 0; }
};

class docx_documentHandler;

class docx_pHandler : public docx_ElementHandler, public odx_styleTagsHandler
{
private:
    docx_pPrHandler m_pPrHandler;
    odx_pPr m_pPr;
    docx_rHandler m_rHandler;
    odx_titleHandler* m_titleHandler;
    docx_hyperlinkHandler m_hyperlinkHandler;
    int m_runCount;
    bool m_inTitle;
public:
    docx_pHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, odx_titleHandler* p_documentHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_p, p_elements),
        m_pPrHandler(reader, writer, context),
        m_rHandler(reader, writer, context, this),
        m_titleHandler(p_documentHandler),
        m_hyperlinkHandler(reader, writer, context, this), m_runCount(0), m_inTitle(false)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void reset();
};

struct docx_row_span_info {
    ldomNode *column;
    int rows;
    docx_row_span_info() : column(NULL), rows(1) {}
    docx_row_span_info(ldomNode *column) : column(column), rows(1) {}
};

class docx_tblHandler : public docx_ElementHandler
{
private:
    LVArray<int> m_levels;
    LVArray<docx_row_span_info> m_rowSpaninfo;
    int m_rowCount;
    odx_titleHandler m_titleHandler;
    docx_pHandler m_pHandler;
    xml_SkipElementHandler m_skipHandler;
    xml_ElementHandler* m_pHandler_;
    int m_colSpan;
    int m_column;
    int m_columnCount;
    enum vMergeState_tyep {
        VMERGE_NONE,
        VMERGE_RESET,
        VMERGE_CONTINUE
    };
    int m_vMergeState;
    void endRowSpan(int column);
public:
    docx_tblHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, odx_titleHandler* titleHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_tbl, tbl_elements),
        m_rowCount(0), m_titleHandler(writer, titleHandler->useClassForTitle()),
        m_pHandler(reader, writer, context, &m_titleHandler),
        m_skipHandler(reader, writer, docx_el_p), m_pHandler_(NULL), m_colSpan(1),
        m_column(0), m_columnCount(0), m_vMergeState(VMERGE_NONE)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void reset();
};

class docx_footnotesHandler : public docx_ElementHandler
{
private:
    bool m_normal;
    int m_pCount;
    docx_pHandler paragraphHandler;
private:
    bool isEndNote() { return m_element == docx_el_endnotes; }
public:
    docx_footnotesHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, int element) :
        docx_ElementHandler(reader, writer, context, element, footnotes_elements), m_normal(), m_pCount(),
        paragraphHandler(reader, writer, context, NULL)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
};

class docx_documentHandler : public docx_ElementHandler
{
private:
    docx_pHandler paragraphHandler;
    docx_tblHandler m_tableHandler;
protected:
    odx_titleHandler* m_titleHandler;
public:
    docx_documentHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, odx_titleHandler* titleHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_document, document_elements),
        paragraphHandler(reader, writer, context, titleHandler),
        m_tableHandler(reader, writer, context, titleHandler), m_titleHandler(titleHandler)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
};

class docx_styleHandler : public docx_ElementHandler
{
private:
    docx_pPrHandler m_pPrHandler;
    docx_rPrHandler m_rPrHandler;
    odx_StyleRef m_styleRef;
    odx_Style *m_style;
public:
    /// constructor
    docx_styleHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_style, style_elements),
        m_pPrHandler(reader, writer, context),
        m_rPrHandler(reader, writer, context)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void start();
};

class docx_stylesHandler : public docx_ElementHandler
{
private:
    docx_styleHandler m_styleHandler;
    docx_pPrHandler m_pPrHandler;
    docx_rPrHandler m_rPrHandler;
public:
    /// constructor
    docx_stylesHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_styles, styles_elements),
        m_styleHandler(reader, writer, context),
        m_pPrHandler(reader, writer, context),
        m_rPrHandler(reader, writer, context)
    {
    }
    /// destructor
    ldomNode * handleTagOpen(int tagId);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void reset();
};

class docx_lvlHandler : public docx_ElementHandler
{
private:
    docxNumLevel *m_lvl;
    docx_pPrHandler m_pPrHandler;
    docx_rPrHandler m_rPrHandler;
public:
    /// constructor
    docx_lvlHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_lvl, lvl_elements),
        m_pPrHandler(reader, writer, context),
        m_rPrHandler(reader, writer, context)
    {
    }
    void start(docxNumLevel* level) {
        m_lvl = level;
        docx_ElementHandler::start();
    }
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    ldomNode * handleTagOpen(int tagId);
    void reset();
};

class docx_numHandler : public docx_ElementHandler
{
    docx_lvlHandler m_lvlHandler;
    docxNumRef m_numRef;
    docxNumLevelRef m_levelRef;
public:
    docx_numHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_num, num_elements),
        m_lvlHandler(reader, writer, context)
    {
    }
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    ldomNode * handleTagOpen(int tagId);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void start();
};

class docx_abstractNumHandler : public docx_ElementHandler
{
    docx_lvlHandler m_lvlHandler;
    docxNumLevelRef m_levelRef;
    docxAbstractNumRef m_abstractNumRef;
public:
    docx_abstractNumHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_abstractNum, abstractNum_elements),
        m_lvlHandler(reader, writer, context)
    {
    }
    void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue);
    ldomNode * handleTagOpen(int tagId);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
    void start();
};

class docx_numberingHandler : public docx_ElementHandler
{
private:
    docx_numHandler m_numHandler;
    docx_abstractNumHandler m_abstractNumHandler;
public:
    /// constructor
    docx_numberingHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_numbering, numbering_elements),
        m_numHandler(reader, writer, context),
        m_abstractNumHandler(reader, writer, context)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleTagClose( const lChar32 * nsname, const lChar32 * tagname );
};


docxNumLevel::docxNumLevel() :
    m_isLgl(false), m_lvlJc(css_ta_inherit), m_ilvl(css_val_unspecified, 0),
    m_lvlRestart(css_val_unspecified, 0), m_lvlTextNull(false), m_lvlNumFormat(docx_numFormat_ordinal),
    m_lvlStart(css_val_unspecified, 0), m_suffix(docx_level_suffix_space)
{
}

void docxNumLevel::reset()
{
    m_isLgl = false;
    m_lvlJc = css_ta_inherit;
    m_ilvl.type = css_val_unspecified;
    m_lvlRestart.type = css_val_unspecified;
    m_lvlText.clear();
    m_lvlTextNull = false;
    m_lvlNumFormat = docx_numFormat_ordinal;
    m_pPr.reset();
    m_rPr.reset();
    m_pStyle.clear();
    m_lvlStart.type = css_val_unspecified;
    m_suffix = docx_level_suffix_space;
}

css_list_style_type_t docxNumLevel::getListType() const
{
    if(m_isLgl)
        return css_lst_decimal;
    switch(m_lvlNumFormat) {
    case docx_numFormat_lowerLetter:
        return css_lst_lower_alpha;
    case docx_numFormat_lowerRoman:
        return css_lst_lower_roman;
    case docx_numFormat_upperLetter:
        return css_lst_upper_alpha;
    case docx_numFormat_upperRoman:
        return css_lst_upper_roman;
    case docx_numFormat_bullet:
        if ( getLevelText() == U"\xf0a7" )
            return css_lst_square;
        return css_lst_disc;
    case docx_numFormat_decimal:
        return css_lst_decimal;
    default:
        return css_lst_none;
    }
}

bool docx_ElementHandler::parse_OnOff_attribute(const lChar32 * attrValue)
{
    if ( !lStr_cmp(attrValue, "1") || !lStr_cmp(attrValue, "on") || !lStr_cmp(attrValue, "true") )
        return true;
    return false;
}

void docx_ElementHandler::generateLink(const lChar32 *target, const lChar32 *type, const lChar32 *text)
{
    m_writer->OnTagOpen(U"", U"a");
    m_writer->OnAttribute(U"", U"href", target );
    if(type)
        m_writer->OnAttribute(U"", U"type", type);
    // Add classic role=doc-noteref attribute to allow popup/in-page footnotes
    m_writer->OnAttribute(U"", U"role", U"doc-noteref");
    m_writer->OnTagBody();
#ifndef ODX_CRENGINE_IN_PAGE_FOOTNOTES
    if( !lStr_cmp(type, "note") ) {
        // For footnotes (but not endnotes), wrap in <sup> (to get the
        // same effect as the following in docx.css:
        //   a[type="note"] { vertical-align: super; font-size: 70%; }
        m_writer->OnTagOpen(U"", U"sup");
        m_writer->OnTagBody();
    }
#endif
    lString32 t(text);
    m_writer->OnText(t.c_str(), t.length(), 0);
#ifndef ODX_CRENGINE_IN_PAGE_FOOTNOTES
    if( !lStr_cmp(type, "note") ) {
        m_writer->OnTagClose(U"", U"sup");
    }
#endif
    m_writer->OnTagClose(U"", U"a");
}

ldomNode * docx_rPrHandler::handleTagOpen(int tagId)
{
    m_state = tagId;
    switch(tagId) {
    case docx_el_b:
        m_rPr->setBold(true);
        break;
    case docx_el_i:
        m_rPr->setItalic(true);
        break;
    case docx_el_u:
        m_rPr->setUnderline(true);
        break;
    case docx_el_vanish:
        m_rPr->setHidden(true);
        break;
    case docx_el_strike:
        m_rPr->setStrikeThrough(true);
        break;
    default:
        break;
    }
    return NULL;
}

void docx_rPrHandler::handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue)
{
    int attr_value;
    switch(m_state) {
    case docx_el_lang:
        if( !lStr_cmp(attrname, "val") ) {
            if( m_rPr == m_importContext->get_rPrDefault() ) {
                m_importContext->setLanguage(attrvalue);
            }
        }
        break;
    case docx_el_color:
        // todo
        break;
    case docx_el_b:
        if( !lStr_cmp(attrname, "val") )
            m_rPr->setBold(parse_OnOff_attribute( attrvalue ));
        break;
    case docx_el_i:
        if( !lStr_cmp(attrname, "val") )
            m_rPr->setItalic(parse_OnOff_attribute( attrvalue ));
        break;
    case docx_el_u:
        if( !lStr_cmp(attrname, "val") )
            m_rPr->setUnderline( lStr_cmp(attrvalue, "none") != 0);
        break;
    case docx_el_jc:
        if( !lStr_cmp(attrname, "val") ) {
            attr_value = parse_name(jc_attr_values, attrvalue);
            if(attr_value != -1)
                m_rPr->setTextAlign((css_text_align_t)attr_value);
        }
        break;
    case docx_el_rFonts:
        //todo
        break;
    case docx_el_rStyle:
        m_rPr->setStyleId(m_importContext, attrvalue);
        break;
    case docx_el_strike:
        if( !lStr_cmp(attrname, "val") )
            m_rPr->setStrikeThrough(parse_OnOff_attribute(attrvalue));
        break;
    case docx_el_vertAlign:
        if( !lStr_cmp(attrname, "val") ) {
            attr_value = parse_name(vertAlign_attr_values, attrvalue);
            if(attr_value != -1)
                m_rPr->setVertAlign((css_vertical_align_t)attr_value);
        }
        break;
    case docx_el_sz:
        //todo
        break;
    case docx_el_vanish:
        if ( !lStr_cmp(attrname, "val") )
            m_rPr->setHidden(parse_OnOff_attribute(attrvalue));
        break;
    default:
        break;
    }
}

void docx_rPrHandler::reset()
{
    m_state = m_element;
    if (m_rPr)
        m_rPr->reset();
}

void docx_rPrHandler::start(odx_rPr * const rPr)
{
    m_rPr = rPr;
    docx_ElementHandler::start();
}

void docx_rHandler::handleInstruction(lString32 &instruction, lString32 parameters)
{
    if( instruction == cs32("REF") || instruction == cs32("NOTEREF") || instruction == cs32("PAGEREF") ) {
        lString32 argument, switches;
        if( parameters.split2( cs32(" "), argument, switches) && !argument.empty() )
        {
            m_importContext->m_linkNode = m_writer->OnTagOpen(U"", U"a");
            lString32 target = U"#";
            target  << argument;
            m_writer->OnAttribute(U"", U"href", target.c_str());
            m_writer->OnTagBody();
        }
    }
}

ldomNode *docx_rHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_br:
    case docx_el_t:
    case docx_el_tab:
        if( !m_content ) {
            if( m_importContext->m_pStyle )
                m_rPr.combineWith(m_importContext->m_pStyle->get_rPr(m_importContext));
            m_rPr.combineWith(m_importContext->get_rPrDefault());
            m_pHandler->closeStyleTags(&m_rPr, m_writer);
            m_pHandler->openStyleTags(&m_rPr, m_writer);
            m_content = true;
        }
        m_state = tagId;
        break;
    case docx_el_rPr:
        m_rPrHandler.start(&m_rPr);
        break;
    case docx_el_footnoteRef:
    case docx_el_endnoteRef:
        m_state = tagId;
        break;
    case docx_el_drawing:
        m_drawingHandler.start();
        break;
    case docx_el_footnoteReference:
    case docx_el_endnoteReference:
        m_footnoteId.clear();
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_rHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    if( (docx_el_footnoteReference == m_state || docx_el_endnoteReference == m_state) &&
       !lStr_cmp(attrname, "id") ) {
        m_footnoteId = attrvalue;
    }
    if( docx_el_fldChar == m_state && !lStr_cmp(attrname, "fldCharType") ) {
        if( !lStr_cmp(attrvalue, "begin") ) {
            m_importContext->m_inField = true;
        } else if( !lStr_cmp(attrvalue, "end") ) {
            if( m_importContext->m_linkNode ) {
                m_writer->OnTagClose(U"", U"a");
                m_importContext->m_linkNode = NULL;
            }
            m_importContext->m_inField = false;
        }
    }
}

void docx_rHandler::handleText(const lChar32 *text, int len, lUInt32 flags)
{
    switch(m_state) {
    case docx_el_t:
        m_writer->OnText(text, len, flags);
        break;
    case docx_el_instrText:
        m_instruction = text;
        break;
    default:
        break;
    }
}

void docx_rHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    lChar32 nobsp = 0x00A0;
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_br:
        m_writer->OnTagOpenAndClose(U"", U"br");
        m_state = docx_el_r;
        break;
    case docx_el_r:
        stop();
        break;
    case docx_el_tab:
        m_writer->OnText(&nobsp, 1, 0);
        m_state = docx_el_r;
        break;
    case docx_el_footnoteReference:
        if( !m_footnoteId.empty() ) {
            m_importContext->m_footNoteCount++;
            lString32 target = U"#n_";
            target  << m_footnoteId;
            generateLink(target.c_str(), U"note", m_footnoteId.c_str());
        }
        m_state = docx_el_r;
        break;
    case docx_el_instrText:
        if( m_importContext->m_inField ) {
            m_instruction.trim();
            if ( !m_instruction.empty() ) {
                lString32 instruction, parameters;
                if ( m_instruction.split2(cs32(" "), instruction, parameters) )
                    handleInstruction(instruction, parameters);
            }
        }
        m_state = docx_el_r;
        break;
    case docx_el_endnoteReference:
        if( !m_footnoteId.empty() ) {
            m_importContext->m_endNoteCount++;
            lString32 target = U"#c_";
            target  << m_footnoteId;
            generateLink(target.c_str(), U"comment", m_footnoteId.c_str());
        }
        m_state = docx_el_r;
        break;
    case docx_el_footnoteRef:
    case docx_el_endnoteRef:
        if(!m_importContext->m_footNoteId.empty()) {
            m_writer->OnTagOpen(U"", U"sup");
            m_writer->OnTagBody();
            m_writer->OnText(m_importContext->m_footNoteId.c_str(), m_importContext->m_footNoteId.length(), 0);
            m_writer->OnTagClose(U"", U"sup");
        }
    default:
        m_state = docx_el_r;
        break;
    }
}

void docx_rHandler::reset()
{
    m_rPrHandler.reset();
    m_state = docx_el_r;
    m_content = false;
}

ldomNode * docx_pPrHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_rPr:
        break;
    case docx_el_numPr:
        m_state = tagId;
        setChildrenInfo(numPr_elements);
        break;
    case docx_el_pageBreakBefore:
        m_state = tagId;
        m_pPr->setPageBreakBefore(true);
        break;
    case docx_el_keepNext:
        m_state = tagId;
        m_pPr->setKeepNext(true);
        break;
    case docx_el_mirrorIndents:
        m_state = tagId;
        m_pPr->setMirrorIndents(true);
        break;
    case docx_el_suppressAutoHyphens:
        m_pPr->setHyphenate(css_hyph_none);
        //fallthrough
    default:
        m_state = tagId;
    }
    return NULL;
}

void docx_pPrHandler::handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue)
{
    switch(m_state) {
    case docx_el_pStyle:
        if( !lStr_cmp(attrname, "val") ) {
             m_pPr->setStyleId(m_importContext, attrvalue);
        }
        break;
    case docx_el_jc:
        if( !lStr_cmp(attrname, "val") ) {
            int attr_value = parse_name(jc_attr_values, attrvalue);
            if(attr_value != -1)
                m_pPr->setTextAlign((css_text_align_t)attr_value);
        }
        break;
    case docx_el_spacing:
        if( !lStr_cmp(attrname, "line") ) {
            css_length_t val;
            parse_int(attrvalue, val);
            m_pPr->set(odx_p_line_spacing_prop, val);
        } else if( !lStr_cmp(attrname, "lineRule") ) {
            int attr_value = parse_name(lineRule_attr_values, attrvalue);
            if( -1 != attr_value )
                m_pPr->setLineRule((odx_lineRule_type)attr_value);
        } else if ( !lStr_cmp(attrname, "afterAutospacing") ) {
            m_pPr->set(odx_p_after_auto_spacing_prop, parse_OnOff_attribute(attrvalue));
        } else if ( !lStr_cmp(attrname, "beforeAutospacing") ) {
            m_pPr->set(odx_p_before_auto_spacing_prop, parse_OnOff_attribute(attrvalue));
        } else {
            //todo
        }
        break;
    case docx_el_textAlignment:
        if( !lStr_cmp(attrname, "val") ) {
            int attr_value = parse_name(textAlignment_attr_values, attrvalue);
            if(attr_value != -1)
                m_pPr->setVertAlign((css_vertical_align_t)attr_value);
        }
        break;
    case docx_el_ind:
        //todo
        break;
    case docx_el_ilvl:
        if( !lStr_cmp(attrname, "val") ) {
            css_length_t val;
            parse_int(attrvalue, val);
            m_pPr->set(odx_p_ilvl_prop, val.value);
        }
        break;
    case docx_el_numId:
        if( !lStr_cmp(attrname, "val") ) {
            css_length_t val;
            parse_int(attrvalue, val);
            m_pPr->set(odx_p_num_id_prop, val);
        }
        break;
    case docx_el_outlineLvl:
        if( !lStr_cmp(attrname, "val") ) {
            css_length_t val;
            parse_int(attrvalue, val);
            m_pPr->set(odx_p_outline_level_prop, val);
        }
        break;
    case docx_el_pageBreakBefore:
        if( !lStr_cmp(attrname, "val") )
            m_pPr->setPageBreakBefore(parse_OnOff_attribute(attrvalue));
        break;
    case docx_el_keepNext:
        if( !lStr_cmp(attrname, "val") )
            m_pPr->setKeepNext(parse_OnOff_attribute(attrvalue));
        break;
    case docx_el_mirrorIndents:
        if( !lStr_cmp(attrname, "val") )
            m_pPr->setMirrorIndents(parse_OnOff_attribute(attrvalue));
        break;
    case docx_el_suppressAutoHyphens:
        if( !lStr_cmp(attrname, "val") && !parse_OnOff_attribute(attrvalue) )
            m_pPr->setHyphenate(css_hyph_auto);
        break;
    default:
        break;
    }
}

void docx_pPrHandler::handleTagClose( const lChar32 * nsname, const lChar32 * tagname )
{
    switch(m_state) {
    case docx_el_ilvl:
    case docx_el_numId:
        m_state = docx_el_numPr;
        break;
    case docx_el_numPr:
        setChildrenInfo(pPr_elements);
        //falltrrough
    default:
        docx_ElementHandler::handleTagClose(nsname, tagname);
        break;
    }
}

void docx_pPrHandler::reset()
{
    if(m_pPr)
        m_pPr->reset();
}

void docx_pPrHandler::start(odx_pPr *pPr)
{
    m_pPr = pPr;
    docx_ElementHandler::start();
}

ldomNode * docx_pHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_r:
    case docx_el_hyperlink:
        if ( 0 == m_runCount ) {
            m_pPr.combineWith(m_importContext->get_pPrDefault());
            css_length_t outlineLevel = m_pPr.getOutlineLvl();
            m_importContext->m_pStyle = m_pPr.getStyle(m_importContext);
            if ( outlineLevel.type != css_val_unspecified ) {
                m_inTitle = true;
            }
            int numId = m_pPr.getNumberingId();
            if( numId != 0 && !m_inTitle ) {
                int level = m_pPr.getNumberingLevel() + 1;
                if( level > m_importContext->getListLevel() )
                    m_importContext->openList(level, numId, m_writer);
                else if( level < m_importContext->getListLevel() )
                    m_importContext->closeList(level, m_writer);
                else
                    m_writer->OnTagClose(U"", U"li");
                m_writer->OnTagOpen(U"", U"li");
            } else {
                if( m_importContext->isInList() )
                    m_importContext->closeList(0, m_writer);
                if( m_inTitle )
                    m_titleHandler->onTitleStart(outlineLevel.value + 1);
                else
                    m_writer->OnTagOpen(U"", U"p");
            }
            lString32 style = m_pPr.getCss();
            if( !style.empty() )
                m_writer->OnAttribute(U"", U"style", style.c_str());
            m_writer->OnTagBody();
        }
        if(docx_el_r == tagId)
            m_rHandler.start();
        else
            m_hyperlinkHandler.start();
        m_runCount++;
        break;
    case docx_el_bookmarkStart:
        m_state = tagId;
        break;
        break;
    case docx_el_pPr:
        m_pPrHandler.start(&m_pPr);
        break;
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_pHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    if( docx_el_bookmarkStart == m_state && !lStr_cmp(attrname, "name") ) {
        m_writer->OnTagOpen(U"", U"a");
        m_writer->OnAttribute(U"", U"id", attrvalue);
        m_writer->OnTagBody();
        m_writer->OnTagClose(U"", U"a");
    }
}

void docx_pHandler::handleTagClose( const lChar32 * nsname, const lChar32 * tagname )
{
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_p:
        closeStyleTags(m_writer);
        if( m_pPr.getNumberingId() == 0 ) {
            if( !m_inTitle ) {
                m_writer->OnTagClose(U"", U"p");
            }
        }
        stop();
        if( m_inTitle ) {
            m_inTitle = false;
            m_titleHandler->onTitleEnd();
        }
        break;
    default:
        m_state = docx_el_p;
        break;
    }
}

void docx_pHandler::reset()
{
    m_pPrHandler.reset();
    m_rHandler.reset();
    m_state = docx_el_p;
    m_runCount = 0;
}

ldomNode * docx_documentHandler::handleTagOpen(int tagId)
{
    if( tagId != docx_el_p && m_importContext->isInList() )
        m_importContext->closeList(0, m_writer);
    switch(tagId) {
    case docx_el_p:
        paragraphHandler.start();
        break;
    case docx_el_tbl:
        m_tableHandler.start();
        break;
    case docx_el_body:
        m_titleHandler->onBodyStart();
        m_writer->OnTagBody();
        //fallthrough
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_documentHandler::handleAttribute(const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue)
{
    if (m_state == docx_el_document && !lStr_cmp(nsname, "xmlns") )
        CRLog::debug("namespace declaration %s:%s",  LCSTR(attrname), LCSTR(attrvalue));
}

void docx_documentHandler::handleTagClose( const lChar32 * nsname, const lChar32 * tagname )
{
    switch(m_state) {
    case docx_el_body:
        m_titleHandler->onBodyEnd();
        m_writer->OnTagClose(nsname, tagname);
        break;
    default:
        break;
    }
}

ldomNode * docx_styleHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_pPr:
        m_pPrHandler.start(m_style->get_pPrPointer());
        break;
    case docx_el_rPr:
        m_rPrHandler.start(m_style->get_rPrPointer());
        break;
    case docx_el_tblPr:
    case docx_el_trPr:
    case docx_el_tcPr:
        m_state = tagId;
        break;
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_styleHandler::handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue)
{
    switch(m_state) {
    case docx_el_style:
        if ( !lStr_cmp(attrname, "type") ) {
            int attr_value = parse_name(styleType_attr_values, attrvalue);
            if( -1 != attr_value )
                m_style->setStyleType((odx_style_type)attr_value);
        } else if ( !lStr_cmp(attrname, "styleId") ) {
            m_style->setId(attrvalue);
        }
        break;
    case docx_el_name:
        if ( !lStr_cmp(attrname, "val") )
            m_style->setName(attrvalue);
        break;
    case docx_el_basedOn:
        if ( !lStr_cmp(attrname, "val") )
            m_style->setBasedOn(attrvalue);
        break;
    case docx_el_pPr:
        break;
    case docx_el_rPr:
        break;
    }
}

void docx_styleHandler::handleTagClose( const lChar32 * nsname, const lChar32 * tagname )
{
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_style:
        if ( m_style->isValid() )
            m_importContext->addStyle(m_styleRef);
        stop();
        break;
    default:
        m_state = docx_el_style;
        break;
    }
}

void docx_styleHandler::start()
{
    docx_ElementHandler::start();
    m_styleRef = odx_StyleRef( new odx_Style );
    m_style = m_styleRef.get();
    m_state = docx_el_style;
}

ldomNode * docx_stylesHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_pPr:
        m_pPrHandler.start(m_importContext->get_pPrDefault());
        break;
    case docx_el_rPr:
        m_rPrHandler.start(m_importContext->get_rPrDefault());
        break;
    case docx_el_style:
        m_styleHandler.start();
        break;
    case docx_el_docDefaults:
        setChildrenInfo(docDefaults_elements);
        //falltrough
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_stylesHandler::handleTagClose( const lChar32 * nsname, const lChar32 * tagname )
{
    switch(m_state) {
    case docx_el_rPrDefault:
    case docx_el_pPrDefault:
        m_state = docx_el_docDefaults;
        break;
    case docx_el_docDefaults:
        setChildrenInfo(styles_elements);
        //fallthrough
    case docx_el_style:
        m_state = docx_el_styles;
        break;
    case docx_el_styles:
        stop();
        break;
    default:
        CRLog::error("Unexpected tag(%s:%)", nsname, tagname);
        break;
    }
}

void docx_stylesHandler::reset()
{
    m_styleHandler.reset();
    m_state = docx_el_NULL;
}

bool parseStyles(docxImportContext *importContext)
{
    LVStreamRef m_stream = importContext->openContentPart(docx_StylesContentType);
    if ( m_stream.isNull() )
        return false;

    docXMLreader docReader(NULL);
    docx_stylesHandler stylesHandler(&docReader, NULL, importContext);
    docReader.setHandler(&stylesHandler);

    LVXMLParser parser(m_stream, &docReader);

    if ( !parser.Parse() )
        return false;
    return true;
}

bool parseNumbering(docxImportContext *importContext)
{
    LVStreamRef m_stream = importContext->openContentPart(docx_NumberingContentType);
    if ( m_stream.isNull() )
        return false;

    docXMLreader docReader(NULL);
    docx_numberingHandler numberingHandler(&docReader, NULL, importContext);
    docReader.setHandler(&numberingHandler);

    LVXMLParser parser(m_stream, &docReader);

    if ( !parser.Parse() )
        return false;
    return true;
}

void parseFootnotes(ldomDocumentWriter& writer, docxImportContext& context, int element)
{
    LVStreamRef m_stream;

    if( element == docx_el_footnotes )
        m_stream = context.openRelatedPart(docx_FootNotesRelationShip);
    else
        m_stream = context.openRelatedPart(docx_EndNotesRelationShip);

    if ( !m_stream.isNull() ) {
        docXMLreader docReader(&writer);
        docx_footnotesHandler footnotesHandler(&docReader, &writer, &context, element);
        docReader.setHandler(&footnotesHandler);

        LVXMLParser parser(m_stream, &docReader);

        if(parser.Parse())
#ifdef ODX_CRENGINE_IN_PAGE_FOOTNOTES
            writer.OnTagClose(U"", docx_el_body_name);
#else
            // We didn't add <body name=notes> to not trigger crengine auto-in-page-foonotes
            // mechanism, so we can tweak them with style tweaks. We used a simple <div> instead.
            writer.OnTagClose(U"", U"div");
#endif
    }
    context.closeRelatedPart();
}

bool ImportDocXDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    LVContainerRef arc = LVOpenArchieve( stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive

    doc->setContainer(arc);
    OpcPackage package(arc);

    docxImportContext importContext(&package, doc);

    package.readCoreProperties(doc->getProps());

#if BUILD_LITE!=1
    if ( doc->openFromCache(formatCallback) ) {
        if ( progressCallback ) {
            progressCallback->OnLoadFileEnd( );
        }
        return true;
    }
#endif

    parseNumbering(&importContext);

    if ( !parseStyles(&importContext) )
        return false;

    LVStreamRef m_stream = importContext.openContentPart(docx_DocumentContentType);
    if ( m_stream.isNull() )
        return false;

    ldomDocumentWriter writer(doc);
    docXMLreader docReader(&writer);

    importContext.startDocument(writer);

#ifdef DOCX_FB2_DOM_STRUCTURE
    //Two options when dealing with titles: (FB2|HTML)
    odx_fb2TitleHandler titleHandler(&writer, DOCX_USE_CLASS_FOR_HEADING); //<section><title>..</title></section>
#else
    odx_titleHandler titleHandler(&writer);  //<hx>..</hx>
#endif
    docx_documentHandler documentHandler(&docReader, &writer, &importContext, &titleHandler);
    docReader.setHandler(&documentHandler);

    LVXMLParser parser(m_stream, &docReader);

    if ( !parser.Parse() )
        return false;

    if(importContext.m_footNoteCount > 0) {
        parseFootnotes(writer, importContext, docx_el_footnotes);
    }
    if(importContext.m_endNoteCount > 0) {
        parseFootnotes(writer, importContext, docx_el_endnotes);
    }

    importContext.endDocument(writer);
    writer.OnStop();

    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        doc->compact();
        doc->dumpStatistics();
    }
    return true;
}

docxImportContext::docxImportContext(OpcPackage *package, ldomDocument *doc) :
    odx_ImportContext(doc), m_abstractNumbers(16),
    m_Numbers(16), m_footNoteCount(0), m_endNoteCount(0),
    m_inField(false), m_linkNode(NULL), m_pStyle(NULL),
    m_package(package)
{
}

docxImportContext::~docxImportContext()
{
}

void docxImportContext::addNum(docxNumRef num)
{
    if ( !num.isNull() ) {
        m_Numbers.set(num->getId(), num);
    }
}

void docxImportContext::addAbstractNum(docxAbstractNumRef abstractNum)
{
    if ( !abstractNum.isNull() ) {
        m_abstractNumbers.set(abstractNum->getId(), abstractNum);
    }
}

LVStreamRef docxImportContext::openContentPart(const lChar32 * const contentType)
{
    m_docPart = m_package->getContentPart(contentType);
    if( !m_docPart.isNull() ) {
        return m_docPart->open();
    }
    return LVStreamRef();
}

LVStreamRef docxImportContext::openRelatedPart(const lChar32 * const relationshipType)
{
    if ( !m_docPart.isNull() ) {
        m_relatedPart = m_docPart->getRelatedPart(relationshipType);
        if ( !m_relatedPart.isNull())
            return m_relatedPart->open();
    }
    return LVStreamRef();
}

void docxImportContext::closeRelatedPart()
{
    if ( !m_relatedPart.isNull() ) {
        m_relatedPart.Clear();
    }
}

void docxImportContext::openList(int level, int numid, ldomDocumentWriter *writer)
{
    const docxNumRef num = getNum(numid);

    for(int i = getListLevel(); i < level; i++) {
        const docxNumLevel* listLevel = NULL;
        css_list_style_type_t listType = css_lst_disc;
        if ( !num.isNull() )
            listLevel = num->getDocxLevel(const_cast<docxImportContext&>(*this), level - 1);
        if (listLevel)
            listType = listLevel->getListType();
        writer->OnTagOpen(U"", U"ol");
        m_ListLevels.add(listType);
        writer->OnAttribute(U"", U"style", getListStyleCss(listType).c_str());
        writer->OnTagBody();
        if ( i != level - 1 )
            writer->OnTagOpenNoAttr(U"", U"li");
    }
}

void docxImportContext::closeList(int level, ldomDocumentWriter *writer)
{
    for(int i = getListLevel(); i > level; i--) {
        writer->OnTagClose(U"", U"li");
        writer->OnTagClose(U"", U"ol");
        m_ListLevels.remove(getListLevel() - 1);
    }
}

ldomNode * docx_lvlHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_pPr:
        m_pPrHandler.start(m_lvl->get_pPr());
        break;
    case docx_el_rPr:
        m_rPrHandler.start(m_lvl->get_rPr());
        break;
    case docx_el_isLgl:
        m_lvl->setLgl(true);
        //fallthrough
    case docx_el_lvlJc:
    case docx_el_lvlRestart:
    case docx_el_lvlText:
    case docx_el_numFmt:
    case docx_el_pStyle:
    case docx_el_start:
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_lvlHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    css_length_t result;

    if( !lStr_cmp(attrname, "val") ) {
        int attr_value;

        switch(m_state) {
        case docx_el_pStyle:
            m_lvl->setReferencedStyleId(attrvalue);
            break;
        case docx_el_lvlJc:
            attr_value = parse_name(jc_attr_values, attrvalue);
            if(attr_value != -1)
                m_lvl->setLevelAlign((css_text_align_t)attr_value);
            break;
        case docx_el_isLgl:
            m_lvl->setLgl(parse_OnOff_attribute( attrvalue ));
            break;
        case docx_el_lvlRestart:
            parse_int(attrvalue, result);
            m_lvl->setLevelRestart(result);
            break;
        case docx_el_lvlText:
            m_lvl->setLevelText(attrvalue);
            break;
        case docx_el_numFmt:
            attr_value = parse_name(numFmt_attr_values, attrvalue);
            if( -1 != attr_value )
                m_lvl->setNumberFormat((docx_numFormat_type)attr_value);
            break;
        case docx_el_start:
            parse_int(attrvalue, result);
            m_lvl->setLevelStart(result);
            break;
        case docx_el_suff:
            attr_value = parse_name(lvlSuff_attr_values, attrvalue);
            if( -1 != attr_value )
                m_lvl->setLevelSuffix((docx_LevelSuffix_type)attr_value);
            break;
        }
    } else if( !lStr_cmp(attrname, "ilvl") ) {
        // m_state should be docx_el_lvl
        parse_int(attrvalue, result);
        m_lvl->setLevel(result);
    } else if( !lStr_cmp(attrname, "null") ) {
        // m_state should be docx_el_lvl
        m_lvl->setLevelTextNull(parse_OnOff_attribute( attrvalue ));
    }
}

void docx_lvlHandler::reset()
{
    m_rPrHandler.reset();
    m_pPrHandler.reset();
    if(m_lvl)
        m_lvl->reset();
}

ldomNode *docx_footnotesHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_p:
        if( m_normal && !m_importContext->m_footNoteId.empty() ) {
            if( m_pCount == 0 ) {
                m_writer->OnTagOpen(U"", U"section");
                lString32 id = isEndNote() ? U"c_" : U"n_";
                id << m_importContext->m_footNoteId.c_str();
                m_writer->OnAttribute(U"", U"id", id.c_str());
                m_writer->OnAttribute(U"", U"role", isEndNote() ? U"doc-rearnote" : U"doc-footnote");
                m_writer->OnTagBody();
            }
            paragraphHandler.start();
        } else {
            m_state = tagId;
        }
        m_pCount++;
        break;
    case docx_el_footnote:
    case docx_el_endnote:
        m_normal = true;
        m_importContext->m_footNoteId.clear();
        m_pCount = 0;
        m_state = tagId;
        break;
    case docx_el_footnotes:
    case docx_el_endnotes:
#ifdef ODX_CRENGINE_IN_PAGE_FOOTNOTES
        m_writer->OnTagOpen(U"", docx_el_body_name);
        if(isEndNote()) {
            m_writer->OnAttribute(U"", U"name", U"comments");
            m_writer->OnTagBody();
            m_writer->OnTagOpen(U"", U"subtitle");
            m_writer->OnTagBody();
            m_writer->OnText(U"* * *", 5, 0);
            m_writer->OnTagClose(U"", U"subtitle");
        } else {
            m_writer->OnAttribute(U"", U"name", U"notes");
            m_writer->OnTagBody();
        }
#else
        // We don't add <body name=notes> to not trigger crengine auto-in-page-foonotes
        // mechanism, so we can tweak them with style tweaks. We use a simple <div> instead.
        m_writer->OnTagOpen(U"", U"div");
        m_writer->OnAttribute(U"", U"style", U"page-break-before: always");
        m_writer->OnTagBody();
#endif
        //fallthrough
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_footnotesHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    switch(m_state) {
    case docx_el_footnote:
    case docx_el_endnote:
        if( !lStr_cmp(attrname, "type") ) {
            if( lStr_cmp(attrvalue, "normal") )
                m_normal = false;
        } else if( !lStr_cmp(attrname, "id") )
            m_importContext->m_footNoteId.append(attrvalue);
        break;
    default:
        break;
    }
}

void docx_footnotesHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    switch (m_state) {
    case docx_el_p:
        m_state = isEndNote() ? docx_el_endnote : docx_el_footnote;
        break;
    case docx_el_endnote:
    case docx_el_footnote:
        m_writer->OnTagClose(U"", U"section");
    default:
        docx_ElementHandler::handleTagClose(nsname, tagname);
        break;
    }
}

ldomNode *docx_hyperlinkHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_r:
        if ( !m_target.empty() && 0 == m_runCount ) {
            m_writer->OnTagOpen(U"", U"a");
            m_writer->OnAttribute(U"", U"href", m_target.c_str());
            m_writer->OnTagBody();
        }
        m_runCount++;
        m_rHandler.start();
        break;
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_hyperlinkHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    if( docx_el_hyperlink == m_state) {
        if ( !lStr_cmp(attrname, "id") ) {
            m_target = m_importContext->getLinkTarget(lString32(attrvalue));
        } else if (!lStr_cmp(attrname, "anchor") && m_target.empty()) {
            m_target = cs32("#") + lString32(attrvalue);
        }
    }
}

void docx_hyperlinkHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    switch (m_state) {
    case docx_el_hyperlink:
        if ( !m_target.empty() ) {
            m_writer->OnTagClose(U"", U"a");
        }
    default:
        docx_ElementHandler::handleTagClose(nsname, tagname);
        break;
    }
}

ldomNode *docx_drawingHandler::handleTagOpen(int tagId)
{
    m_level++;
    m_state = tagId;
    return NULL;
}

void docx_drawingHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    if( m_state == docx_el_blip && !lStr_cmp(attrname, "embed") ) {
        lString32 imgPath = m_importContext->getImageTarget(lString32(attrvalue));
        if( !imgPath.empty() ) {
            m_writer->OnTagOpen(U"", U"img");
            m_writer->OnAttribute(U"", U"src",  imgPath.c_str());
            m_writer->OnTagBody();
            m_writer->OnTagClose(U"", U"img", true);
        }
    }
}

void docx_drawingHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED2(nsname, tagname);

    if(m_level <= 1)
        stop();
    m_level--;
}

void docx_tblHandler::endRowSpan(int column)
{
    docx_row_span_info rowSpan = m_rowSpaninfo[column];
    if( rowSpan.rows > 1 ) {
        CRLog::warn("Row span on column: %d, end: %d", column, rowSpan.rows);
        if( rowSpan.column ) {
            rowSpan.column->setAttributeValue(LXML_NS_NONE,
                                              rowSpan.column->getDocument()->getAttrNameIndex(U"rowspan"),
                                              lString32::itoa(rowSpan.rows).c_str());
        } else {
            CRLog::error("No column node");
        }
    }
}

ldomNode *docx_tblHandler::handleTagOpen(int tagId)
{
    bool elementHandled = false;
    switch(tagId) {
    case docx_el_p:
        m_pHandler_->start();
        elementHandled = true;
        break;
    case docx_el_tc:
        m_colSpan = 1;
        CRLog::warn("Column: %d", m_column);
        m_vMergeState = VMERGE_NONE;
        break;
    case docx_el_vMerge:
        m_vMergeState = VMERGE_CONTINUE;
        break;
    case docx_el_tr:
        m_column = 0;
        m_writer->OnTagOpenNoAttr(U"", U"tr");
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

void docx_tblHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    if( m_state == docx_el_gridSpan && !lStr_cmp( attrname, "val" ) ) {
        m_colSpan = lString32(attrvalue).atoi();
    } else if( m_state == docx_el_vMerge && !lStr_cmp( attrname, "val" ) ) {
        if( !lStr_cmp( attrvalue, "restart" ) )
            m_vMergeState = VMERGE_RESET;
    }
}

void docx_tblHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED2(nsname, tagname);

    if( !m_levels.empty() ) {
        switch(m_state) {
        case docx_el_tblPr:
            m_writer->OnTagOpenNoAttr(U"", U"table");
            break;
        case docx_el_tr:
            m_writer->OnTagClose(U"", U"tr");
            m_rowCount++;
            break;
        case docx_el_tc:
            m_column++;
            if( m_pHandler_ == &m_pHandler )
                m_writer->OnTagClose(U"", U"td");
            break;
        case docx_el_gridCol:
            m_columnCount++;
            break;
        case docx_el_tblGrid:
            if( m_columnCount )
                m_rowSpaninfo.reserve(m_columnCount);
            break;
        case docx_el_tcPr:
            if( VMERGE_NONE == m_vMergeState || VMERGE_RESET == m_vMergeState) {
                m_pHandler_ = &m_pHandler;
                ldomNode *columnNode = m_writer->OnTagOpen(U"", U"td");
                for(int i = 0; i < m_colSpan; i++) {
                    if( m_column + i >= m_columnCount )
                        break; // shouldn't happen
                    endRowSpan(m_column + i);
                }
                m_rowSpaninfo[m_column] = docx_row_span_info(columnNode);
                if( m_colSpan > 1)
                    m_writer->OnAttribute(U"", U"colspan", lString32::itoa(m_colSpan).c_str() );
                m_writer->OnTagBody();
            } else if ( VMERGE_CONTINUE == m_vMergeState ) {
                m_pHandler_ = &m_skipHandler;
                m_rowSpaninfo[m_column].rows++;
            }
            m_column += m_colSpan - 1;
            break;
        default:
            break;
        }
        m_levels.erase(m_levels.length() - 1, 1);
        if( !m_levels.empty() ) {
            m_state = m_levels[m_levels.length() - 1];
        } else {
            m_state = docx_el_tbl;
        }
    } else {
        for(int i = 0; i < m_columnCount; i++) {
            endRowSpan(i);
        }
        m_writer->OnTagClose(U"", U"table");
        stop();
    }

}

void docx_tblHandler::reset()
{
    m_levels.clear();
    m_rowSpaninfo.clear();
    m_rowCount = 0;
    m_columnCount = 0;
}

ldomNode *docx_numberingHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_abstractNum:
        m_abstractNumHandler.start();
        break;
    case docx_el_num:
        m_numHandler.start();
        break;
    default:
        m_state = tagId;
    }
    return NULL;
}

void docx_numberingHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    switch(m_state) {
    case docx_el_num:
    case docx_el_abstractNum:
        m_state = docx_el_numbering;
        break;
    case docx_el_numbering:
        stop();
        break;
    default:
        CRLog::error("Unexpected tag(%s:%)", nsname, tagname);
        break;
    }
}

ldomNode *docx_abstractNumHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_lvl:
        if ( !m_levelRef.isNull() )
            m_abstractNumRef->addLevel( m_levelRef );
        m_levelRef = docxNumLevelRef( new docxNumLevel );
        m_lvlHandler.start( m_levelRef.get() );
        break;
    default:
        m_state = tagId;
    }
    return NULL;
}

void docx_abstractNumHandler::handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue)
{
    switch(m_state) {
    case docx_el_abstractNum:
        if ( !lStr_cmp(attrname, "abstractNumId") )
            m_abstractNumRef->setId(lString32(attrvalue).atoi());
        break;
    default:
        break;
    }
}

void docx_abstractNumHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_abstractNum:
        if ( !m_levelRef.isNull() )
            m_abstractNumRef->addLevel( m_levelRef );
        if ( !m_abstractNumRef.isNull() )
            m_importContext->addAbstractNum( m_abstractNumRef );
        stop();
        break;
    default:
        m_state = docx_el_abstractNum;
        break;
    }
}

void docx_abstractNumHandler::start()
{
    m_abstractNumRef = docxAbstractNumRef( new docxAbstractNum );
    docx_ElementHandler::start();
}

void docx_numHandler::handleAttribute(const lChar32 *attrname, const lChar32 *attrvalue)
{
    switch(m_state) {
    case docx_el_num:
        if ( !lStr_cmp(attrname, "numId") )
            m_numRef->setId( lString32(attrvalue).atoi() );
        break;
    case docx_el_abstractNumId:
        if ( !lStr_cmp(attrname, "val") )
            m_numRef->setBaseId( lString32(attrvalue).atoi() );
        break;
    default:
        break;
    }
}

ldomNode *docx_numHandler::handleTagOpen(int tagId)
{
    switch(tagId) {
    case docx_el_lvl:
        if ( !m_levelRef.isNull() )
            m_numRef->overrideLevel( m_levelRef );
        m_levelRef = docxNumLevelRef( new docxNumLevel );
        m_lvlHandler.start( m_levelRef.get() );
        break;
    default:
        m_state = tagId;
    }
    return NULL;
}

void docx_numHandler::handleTagClose(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_num:
        if ( !m_levelRef.isNull() )
            m_numRef->overrideLevel( m_levelRef );
        if ( m_numRef->isValid() )
            m_importContext->addNum( m_numRef );
        stop();
        break;
    default:
        m_state = docx_el_num;
        break;
    }
}

void docx_numHandler::start()
{
    m_numRef = docxNumRef( new docxNum );
    docx_ElementHandler::start();
}

const docxAbstractNumRef docxNum::getBase(docxImportContext &context) const
{
    return context.getAbstractNum(getBaseId());
}

void docxNum::overrideLevel(docxNumLevelRef docxLevel)
{
    if( !docxLevel.isNull() )
        m_overrides.set(docxLevel->getLevel().value, docxLevel);
}

docxNumLevel *docxNum::getDocxLevel(docxImportContext &context, int level)
{
    docxNumLevelRef levelRef = m_overrides.get(level);
    if( !levelRef.isNull() )
        return levelRef.get();
    docxAbstractNumRef baseRef = getBase(context);
    if( !baseRef.isNull() )
        return baseRef->getLevel(level);
    return NULL;
}

bool docxNum::isValid() const
{
    return (m_id.type != css_val_unspecified
            && m_abstractNumId.type != css_val_unspecified);
}

void docxNum::reset()
{
    m_id.type = css_val_unspecified;
    m_abstractNumId.type = css_val_unspecified;
    m_overrides.clear();
}

void docxAbstractNum::addLevel(docxNumLevelRef docxLevel)
{
    m_levels.set(docxLevel->getLevel().value, docxLevel);
}

docxAbstractNum::docxAbstractNum() : m_multilevel(docx_singlelevel),
    m_abstractNumId(css_val_unspecified, 0), m_levels(10)
{
}

docxNumLevel *docxAbstractNum::getLevel(int level)
{
    return m_levels.get(level).get();
}

void docxAbstractNum::reset()
{
    m_levels.clear();
}
