#include "../include/docxfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/crlog.h"
#include "xmlutil.h"

#define DOCX_TAG_NAME(itm) docx_el_##itm##_name
#define DOCX_TAG_ID(itm) docx_el_##itm
#define DOCX_TAG_CHILD(itm) { DOCX_TAG_ID(itm), DOCX_TAG_NAME(itm) }
#define DOCX_LAST_ITEM { -1, NULL }

// comment this out to disable in-page footnotes
#define DOCX_CRENGINE_IN_PAGE_FOOTNOTES 1
// build FB2 DOM, comment out to build HTML DOM
#define DOCX_FB2_DOM_STRUCTURE 1
//If true <title class="hx"><p>...</p></title> else <title><hx>..</hx></title>
#define DOCX_USE_CLASS_FOR_HEADING true

static const lChar16* const docx_DocumentContentType   = L"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml";
static const lChar16* const docx_NumberingContentType  = L"application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml";
static const lChar16* const docx_StylesContentType     = L"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml";
static const lChar16* const docx_ImageRelationship     = L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image";
static const lChar16* const docx_HyperlinkRelationship = L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink";
static const lChar16* const docx_FootNotesRelationShip = L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footnotes";
static const lChar16* const docx_EndNotesRelationShip  = L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/endnotes";

enum {
#define DOCX_NUM_FMT(itm)
#define DOCX_TAG(itm) 	DOCX_TAG_ID(itm),
    docx_el_NULL = 0,
    #include "docxdtd.inc"
    docx_el_MAX_ID
};

#define DOCX_NUM_FMT(itm)
#define DOCX_TAG(itm) static const lChar16 * const DOCX_TAG_NAME(itm) = L ## #itm;
    #include "docxdtd.inc"

const struct item_def_t styles_elements[] = {
    DOCX_TAG_CHILD(styles),
    DOCX_TAG_CHILD(style),
    DOCX_TAG_CHILD(docDefaults),
    DOCX_LAST_ITEM
};


enum docx_lineRule_type {
    docx_lineRule_atLeast,
    docx_lineRule_auto,
    docx_lineRule_exact
};

enum docx_style_type {
    docx_paragraph_style,
    docx_character_style,
    docx_table_style,
    docx_numbering_style
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
    { css_ta_left, L"left"},
    { css_ta_right, L"right" },
    { css_ta_center, L"center" },
    { css_ta_justify, L"both" },
    DOCX_LAST_ITEM
};

const struct item_def_t vertAlign_attr_values[] = {
    { css_va_baseline, L"baseline"},
    { css_va_super, L"superscript" },
    { css_va_sub, L"subscript" },
    DOCX_LAST_ITEM
};

const struct item_def_t textAlignment_attr_values[] = {
    { css_va_inherit, L"auto" },
    { css_va_baseline, L"baseline"},
    { css_va_bottom, L"bottom"},
    { css_va_middle, L"center" },
    { css_va_top, L"top" },
    DOCX_LAST_ITEM
};

const struct item_def_t lineRule_attr_values[] = {
    { docx_lineRule_atLeast, L"atLeast" },
    { docx_lineRule_auto, L"auto"},
    { docx_lineRule_exact, L"exact"},
    DOCX_LAST_ITEM
};

const struct item_def_t styleType_attr_values[] = {
    { docx_paragraph_style, L"paragraph" },
    { docx_character_style, L"character"},
    { docx_numbering_style, L"numbering"},
    { docx_table_style, L"table"},
    DOCX_LAST_ITEM
};

const struct item_def_t lvlSuff_attr_values[] = {
    { docx_level_suffix_tab, L"tab" },
    { docx_level_suffix_space, L"space" },
    { docx_level_suffix_nothing, L"nothing" },
    DOCX_LAST_ITEM
};

#define DOCX_TAG(itm)
#define DOCX_NUM_FMT(itm) { docx_numFormat_##itm , L ## #itm },
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
class docxStyle;

template <int N>
class docx_PropertiesContainer
{
public:
    static const int PROP_COUNT = N;

    virtual void reset() {
        init();
    }

    virtual ~docx_PropertiesContainer() {}

    docx_PropertiesContainer() {
        init();
    }

    css_length_t get(int index) const {
        if( index < N ) {
            return m_properties[index];
        }
        return css_length_t(css_val_unspecified, 0);
    }

    void set(int index, int value) {
        if ( index < N ) {
            m_properties[index].type = css_val_pt;
            m_properties[index].value = value;
        }
    }

    void set(int index, css_length_t& value) {
        if ( index < N ) {
            m_properties[index] = value;
        }
    }

    template<class T, typename U = void>
    T getValue(int index, T defaultValue) const {
        css_length_t property = get(index);
        if(property.type != css_val_unspecified)
            return (T)property.value;
        return defaultValue;
    }

    template<typename U>
    bool getValue(int index, bool defaultValue) const {
        css_length_t property = get(index);
        if(property.type != css_val_unspecified)
            return (property.value != 0);
        return defaultValue;
    }

    void combineWith(const docx_PropertiesContainer* other)
    {
        for(int i = 0; i < PROP_COUNT; i++) {
            css_length_t baseValue = other->get(i);
            if( get(i).type == css_val_unspecified &&
                baseValue.type != css_val_unspecified)
                set(i, baseValue);
        }
    }

protected:
    css_length_t m_properties[N];
private:
    void init() {
        for(int i = 0; i < N; i++) {
            m_properties[i].type = css_val_unspecified;
            m_properties[i].value = 0;
        }
    }
};

enum docx_run_properties
{
    docx_run_italic_prop,
    docx_run_bold_prop,
    docx_run_underline_prop,
    docx_run_strikethrough_prop,
    docx_run_hidden_prop,
    docx_run_halign_prop,
    docx_run_valign_prop,
    docx_run_font_size_prop,
    docx_run_max_prop
};

class docx_rPr : public docx_PropertiesContainer<docx_run_max_prop>
{
    friend class docx_rPrHandler;
private:
    lString16 m_rStyle;
public:
    docx_rPr();
    void reset() { m_rStyle.clear(); docx_PropertiesContainer::reset(); }
    ///properties
    inline bool isBold() const { return getValue(docx_run_bold_prop, false); }
    inline void setBold(bool value) { set(docx_run_bold_prop, value); }
    inline bool isItalic() const { return getValue(docx_run_italic_prop, false); }
    inline void setItalic(bool value) { set(docx_run_italic_prop, value); }
    inline bool isUnderline() const { return getValue(docx_run_underline_prop, false); }
    inline void setUnderline(bool value) { set(docx_run_underline_prop, value); }
    inline bool isStrikeThrough() const { return getValue(docx_run_strikethrough_prop, false); }
    inline void setStrikeThrough(bool value) { set(docx_run_strikethrough_prop, value); }
    inline bool isSubScript() const { return (getVertAlign() == css_va_sub);  }
    inline bool isSuperScript() const { return (getVertAlign() == css_va_super); }
    inline bool isHidden() const { return getValue(docx_run_hidden_prop, false); }
    inline void setHidden(bool value) { set(docx_run_hidden_prop, value); }
    inline css_text_align_t getTextAlign() const {
        return getValue(docx_run_halign_prop, css_ta_inherit);
    }
    inline void setTextAlign( css_text_align_t value ) { set(docx_run_halign_prop, value); }
    inline css_vertical_align_t getVertAlign() const {
        return getValue(docx_run_valign_prop, css_va_inherit);
    }
    inline void setVertAlign(css_vertical_align_t value) { set(docx_run_valign_prop,value); }
    lString16 getCss();
};

enum docx_p_properties {
    docx_p_page_break_before_prop,
    docx_p_keep_next_prop,
    docx_p_mirror_indents_prop,
    docx_p_halign_prop,
    docx_p_valign_prop,
    docx_p_line_rule_prop,
    docx_p_hyphenate_prop,
    docx_p_before_spacing_prop,
    docx_p_after_spacing_prop,
    docx_p_before_auto_spacing_prop,
    docx_p_after_auto_spacing_prop,
    docx_p_line_spacing_prop,
    docx_p_line_height_prop,
    docx_p_left_margin_prop,
    docx_p_right_margin_prop,
    docx_p_indent_prop,
    docx_p_hanging_prop,
    docx_p_outline_level_prop,
    docx_p_num_id_prop,
    docx_p_ilvl_prop,
    docx_p_max_prop
};

class docx_pPr : public docx_PropertiesContainer<docx_p_max_prop>
{
    friend class docx_pPrHandler;
private:
    lString16 m_pStyleId;
public:
    docx_pPr();

    void reset() {
        m_pStyleId.clear();
        docx_PropertiesContainer::reset();
    }
    ///properties
    inline css_text_align_t getTextAlign() const {
        return getValue(docx_p_halign_prop, css_ta_inherit);
    }
    inline void setTextAlign( css_text_align_t value ) { set(docx_p_halign_prop, value); }
    inline css_vertical_align_t getVertAlign() const {
        return getValue(docx_p_valign_prop, css_va_inherit);
    }
    inline void setVertAlign(css_vertical_align_t value) { set(docx_p_valign_prop, value); }
    inline css_hyphenate_t getHyphenate() const {
        return getValue(docx_p_hyphenate_prop, css_hyph_inherit);
    }
    inline void setHyphenate( css_hyphenate_t value ) { set(docx_p_hyphenate_prop, value); }
    // page-break-before:always
    inline bool isPageBreakBefore() const { return getValue(docx_p_page_break_before_prop, false); }
    inline void setPageBreakBefore(bool value) { set(docx_p_page_break_before_prop, value); }
    // page-break-after:avoid
    inline bool isKeepNext() const { return getValue(docx_p_keep_next_prop, false); }
    inline void setKeepNext(bool value) { set(docx_p_keep_next_prop, value); }
    inline bool isMirrorIndents() const { return getValue(docx_p_mirror_indents_prop, false); }
    inline void setMirrorIndents(bool value) { set(docx_p_mirror_indents_prop, value); }
    inline docx_lineRule_type getLineRule() const { return getValue(docx_p_line_rule_prop, docx_lineRule_auto); }
    inline void setLineRule(docx_lineRule_type value) { set(docx_p_line_rule_prop, value); }
    inline int getNumberingId() { return getValue(docx_p_num_id_prop, 0); }
    css_length_t getOutlineLvl() { return get(docx_p_outline_level_prop); }
    inline int getNumberingLevel() { return get(docx_p_ilvl_prop).value; }
    docxStyle* getStyle(docxImportContext* context);
    lString16 getCss();
};

class docxNumLevel : public LVRefCounter
{
private:
    bool m_isLgl;
    css_text_align_t m_lvlJc;
    css_length_t m_ilvl;
    css_length_t m_lvlRestart;
    lString16 m_lvlText;
    bool m_lvlTextNull;
    docx_numFormat_type m_lvlNumFormat;
    docx_pPr m_pPr;
    docx_rPr m_rPr;
    lString16 m_pStyle;
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
    inline lString16 getLevelText() const { return m_lvlText; }
    inline void setLevelText(const lString16 value) { m_lvlText = value; }
    inline bool getLevelTextNull() const { return m_lvlTextNull; }
    inline void setLevelTextNull(const bool value) { m_lvlTextNull = value; }
    inline docx_numFormat_type getNumberFormat() const { return m_lvlNumFormat; }
    inline void setNumberFormat(const docx_numFormat_type value) { m_lvlNumFormat = value; }
    inline lString16 getReferencedStyleId() const { return m_pStyle; }
    inline void setReferencedStyleId(const lString16 value) { m_pStyle = value; }
    inline css_length_t getLevelStart() const { return m_lvlStart; }
    inline void setLevelStart(const css_length_t &value) { m_lvlStart = value; }
    inline docx_LevelSuffix_type getLevelSuffix() const { return m_suffix; }
    inline void setLevelSuffix(const docx_LevelSuffix_type value) { m_suffix = value; }
    inline docx_rPr * get_rPr() { return &m_rPr; }
    inline docx_pPr * get_pPr() { return &m_pPr; }
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

class docxStyle : public LVRefCounter
{
    friend class docx_styleHandler;
private:
    lString16 m_Name;
    lString16 m_Id;
    lString16 m_basedOn;
    docx_style_type m_type;
    docx_pPr m_pPr;
    docx_rPr m_rPr;
    bool m_pPrMerged;
    bool m_rPrMerged;
public:
    docxStyle();

    inline lString16 getName() const { return m_Name; }
    inline void setName(const lChar16 * value) { m_Name = value; }

    inline lString16 getId() const { return m_Id; }
    inline void setId(const lChar16 * value) { m_Id = value; }

    inline lString16 getBasedOn() const { return m_basedOn; }
    inline void setBasedOn(const lChar16 * value) { m_basedOn = value; }
    bool isValid() const;

    inline docx_style_type getStyleType() const { return m_type; }
    inline void setStyleType(docx_style_type value) { m_type = value; }
    docxStyle* getBaseStyle(docxImportContext* context);
    inline docx_pPr * get_pPr(docxImportContext* context);
    inline docx_rPr * get_rPr(docxImportContext* context);
};

typedef LVFastRef< docxStyle > docxStyleRef;

class docxImportContext
{
private:
    LVHashTable<lString16, docxStyleRef> m_styles;
    LVHashTable<lUInt32, docxAbstractNumRef> m_abstractNumbers;
    LVHashTable<lUInt32, docxNumRef> m_Numbers;
    LVArray<css_list_style_type_t> m_ListLevels;
    docx_rPr m_rPrDefault;
    docx_pPr m_pPrDefault;
    OpcPartRef m_docPart;
    OpcPartRef m_relatedPart;
    OpcPackage* m_package;
    ldomDocument* m_doc;
public:
    docxImportContext(OpcPackage *package, ldomDocument * doc);
    virtual ~docxImportContext();
    docxStyle * getStyle( lString16 id );
    void addStyle( docxStyleRef style );
    void addNum( docxNumRef num );
    void addAbstractNum(docxAbstractNumRef abstractNum );
    docxNumRef getNum(lUInt32 id) { return m_Numbers.get(id); }
    docxAbstractNumRef getAbstractNum(lUInt32 id) { return m_abstractNumbers.get(id); }
    lString16 getImageTarget(lString16 id) {
        return getRelationTarget(docx_ImageRelationship, id);
    }
    lString16 getLinkTarget(lString16 id) {
        return getRelationTarget(docx_HyperlinkRelationship, id);
    }
    lString16 getRelationTarget(const lChar16 * const relationType, lString16 id) {
        if ( !m_relatedPart.isNull() )
            return m_relatedPart->getRelatedPartName(relationType, id);
        return m_docPart->getRelatedPartName(relationType, id);
    }
    LVStreamRef openContentPart(const lChar16 * const contentType);
    LVStreamRef openRelatedPart(const lChar16 * const relationshipType);
    void closeRelatedPart();
    void openList(int level, int numid, ldomDocumentWriter *writer);
    void closeList(int level, ldomDocumentWriter *writer);
    inline docx_rPr * get_rPrDefault() { return &m_rPrDefault; }
    inline docx_pPr * get_pPrDefault() { return &m_pPrDefault; }
    inline int getListLevel() { return m_ListLevels.length(); }
    inline bool isInList() { return m_ListLevels.length() != 0; }
    void setLanguage(const lChar16 *lang);
    lString16 m_footNoteId;
    int m_footNoteCount;
    int m_endNoteCount;
    bool m_inField;
    ldomNode *m_linkNode;
    docxStyle* m_pStyle;
private:
    lString16 getListStyle(css_list_style_type_t listType);
};

class docx_ElementHandler : public xml_ElementHandler
{
protected:
    docxImportContext *m_importContext;
    const item_def_t *m_children;
protected:
    static bool parse_OnOff_attribute(const lChar16 * attrValue);
    static int parse_name(const struct item_def_t *tags, const lChar16 * nameValue);
    static void parse_int(const lChar16 * attrValue, css_length_t & result);
    void generateLink(const lChar16 * target, const lChar16 * type, const lChar16 *text);
    void setChildrenInfo(const struct item_def_t *tags);
    docx_ElementHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context,
                        int element, const struct item_def_t *children) : xml_ElementHandler(reader, writer, element),
        m_importContext(context), m_children(children)
    {
    }
    virtual ~docx_ElementHandler() {}
    int parseTagName(const lChar16 *tagname) override
    {
        return parse_name(m_children, tagname);
    }
};

class docx_rPrHandler : public docx_ElementHandler
{
private:
    docx_rPr *m_rPr;
public:
    docx_rPrHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_rPr, rPr_elements), m_rPr(NULL)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void start(docx_rPr *rPr);
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
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
    void reset() { m_level = 1; }
};

class docx_pHandler;
class docx_rHandler : public docx_ElementHandler
{
private:
    docx_rPr m_rPr;
    docx_pHandler* m_pHandler;
    docx_rPrHandler m_rPrHandler;
    lString16 m_footnoteId;
    lString16 m_instruction;
    docx_drawingHandler m_drawingHandler;
    bool m_content;
private:
    void handleInstruction(lString16& instruction, lString16 parameters);
public:
    docx_rHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_pHandler* pHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_r, r_elements), m_pHandler(pHandler),
        m_rPrHandler(reader, writer, context),
        m_drawingHandler(reader, writer, context),
        m_content(false)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleText( const lChar16 * text, int len, lUInt32 flags );
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
    void reset();
};

class docx_pPrHandler : public docx_ElementHandler
{
private:
    docx_pPr *m_pPr;
public:
    docx_pPrHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_pPr, pPr_elements), m_pPr(NULL)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
    void start(docx_pPr *pPr);
    void reset();
};

class docx_titleHandler
{
public:
    docx_titleHandler(ldomDocumentWriter *writer, docxImportContext *context, bool useClassName=false) :
        m_writer(writer), m_importContext(context), m_titleLevel(), m_useClassName(useClassName) {}
    virtual ~docx_titleHandler() {}
    virtual void onBodyStart();
    virtual void onTitleStart(int level, bool noSection = false);
    virtual void onTitleEnd();
    virtual void onBodyEnd() {}
    bool useClassForTitle() { return m_useClassName; }
protected:
    ldomDocumentWriter *m_writer;
    docxImportContext *m_importContext;
    int m_titleLevel;
    bool m_useClassName;
};

class docx_fb2TitleHandler : public docx_titleHandler
{
public:
    docx_fb2TitleHandler(ldomDocumentWriter *writer, docxImportContext *context, bool useClassName) :
        docx_titleHandler(writer, context, useClassName)
    {}
    void onBodyStart();
    void onTitleStart(int level, bool noSection = false);
    void onTitleEnd();
private:
    void makeSection(int startIndex);
    void openSection(int level);
    void closeSection(int level);
private:
    ldomNode *m_section;
    bool m_hasTitle;
};

class docx_hyperlinkHandler  : public docx_ElementHandler
{
    docx_rHandler m_rHandler;
    lString16 m_target;
    int m_runCount;
public:
    docx_hyperlinkHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_pHandler* pHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_hyperlink, hyperlink_elements),
        m_rHandler(reader, writer, context, pHandler), m_runCount(0)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
    void reset() { m_target.clear(); m_rHandler.reset(); m_runCount = 0; }
};

class docx_documentHandler;

class docx_pHandler : public docx_ElementHandler
{
private:
    docx_pPrHandler m_pPrHandler;
    docx_pPr m_pPr;
    docx_rHandler m_rHandler;
    docx_titleHandler* m_titleHandler;
    docx_hyperlinkHandler m_hyperlinkHandler;
    int m_runCount;
    lString16 m_styleTags;
    bool m_inTitle;
private:
    int styleTagPos(lChar16 ch)
    {
        for (int i=0; i < m_styleTags.length(); i++)
            if (m_styleTags[i] == ch)
                return i;
        return -1;
    }
    const lChar16 * getStyleTagName( lChar16 ch );
    void closeStyleTag( lChar16 ch);
    void openStyleTag( lChar16 ch);
public:
    docx_pHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_titleHandler* p_documentHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_p, p_elements),
        m_pPrHandler(reader, writer, context),
        m_rHandler(reader, writer, context, this),
        m_titleHandler(p_documentHandler),
        m_hyperlinkHandler(reader, writer, context, this), m_inTitle(false)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
    void reset();
    void openStyleTags(docx_rPr* runProps);
    void closeStyleTags(docx_rPr* runProps);
    void closeStyleTags();
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
    docx_titleHandler m_titleHandler;
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
    docx_tblHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_titleHandler* titleHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_tbl, tbl_elements),
        m_rowCount(0), m_titleHandler(writer, context, titleHandler->useClassForTitle()),
        m_pHandler(reader, writer, context, &m_titleHandler),
        m_skipHandler(reader, writer, docx_el_p), m_colSpan(1),
        m_column(0), m_columnCount(0), m_vMergeState(VMERGE_NONE)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
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
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
};

class docx_documentHandler : public docx_ElementHandler
{
private:
    docx_pHandler paragraphHandler;
    docx_tblHandler m_tableHandler;
protected:
    docx_titleHandler* m_titleHandler;
public:
    docx_documentHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context, docx_titleHandler* titleHandler) :
        docx_ElementHandler(reader, writer, context, docx_el_document, document_elements),
        paragraphHandler(reader, writer, context, titleHandler),
        m_tableHandler(reader, writer, context, titleHandler), m_titleHandler(titleHandler)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
};

class docx_styleHandler : public docx_ElementHandler
{
private:
    docx_pPrHandler m_pPrHandler;
    docx_rPrHandler m_rPrHandler;
    docxStyleRef m_styleRef;
    docxStyle *m_style;
public:
    /// constructor
    docx_styleHandler(docXMLreader * reader, ldomDocumentWriter *writer, docxImportContext *context) :
        docx_ElementHandler(reader, writer, context, docx_el_style, style_elements),
        m_pPrHandler(reader, writer, context),
        m_rPrHandler(reader, writer, context)
    {
    }
    ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
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
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
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
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
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
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    ldomNode * handleTagOpen(int tagId);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
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
    void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue);
    ldomNode * handleTagOpen(int tagId);
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
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
    void handleTagClose( const lChar16 * nsname, const lChar16 * tagname );
};

docx_rPr::docx_rPr()
{
}

lString16 docx_rPr::getCss()
{
    lString16 style;

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

docx_pPr::docx_pPr()
{
}

docxStyle *docx_pPr::getStyle(docxImportContext *context)
{
    docxStyle* ret = NULL;

    if (!m_pStyleId.empty() ) {
        ret = context->getStyle(m_pStyleId);
    }
    return ret;
}

lString16 docx_pPr::getCss()
{
    lString16 style;

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
            style << "right";
            break;
        case css_ta_center:
            style << "center;";
            break;
        case css_ta_justify:
        default:
            style << "justify";
            break;
        }
    }
    if( isPageBreakBefore() )
        style << "page-break-before: always;";
    else if ( isKeepNext() )
        style << "page-break-before: avoid;";
    return style;
}

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
        if ( getLevelText() == L"\xf0a7" )
            return css_lst_square;
        return css_lst_disc;
    case docx_numFormat_decimal:
        return css_lst_decimal;
    default:
        return css_lst_none;
    }
}

bool docx_ElementHandler::parse_OnOff_attribute(const lChar16 * attrValue)
{
    if ( !lStr_cmp(attrValue, "1") || !lStr_cmp(attrValue, "on") || !lStr_cmp(attrValue, "true") )
        return true;
    return false;
}

int docx_ElementHandler::parse_name(const struct item_def_t *tags, const lChar16 * nameValue)
{
    for (int i=0; tags[i].name; i++) {
        if ( !lStr_cmp( tags[i].name, nameValue )) {
            // found!
            return tags[i].id;
        }
    }
    return -1;
}

void docx_ElementHandler::parse_int(const lChar16 * attrValue, css_length_t & result)
{
    lString16 value = attrValue;

    result.type = css_val_unspecified;
    if ( value.atoi(result.value) )
        result.type = css_val_pt; //just to distinguish with unspecified value
}

void docx_ElementHandler::generateLink(const lChar16 *target, const lChar16 *type, const lChar16 *text)
{
    m_writer->OnTagOpen(L"", L"a");
    m_writer->OnAttribute(L"", L"href", target );
    if(type)
        m_writer->OnAttribute(L"", L"type", type);
    // Add classic role=doc-noteref attribute to allow popup/in-page footnotes
    m_writer->OnAttribute(L"", L"role", L"doc-noteref");
    m_writer->OnTagBody();
#ifndef DOCX_CRENGINE_IN_PAGE_FOOTNOTES
    if( !lStr_cmp(type, "note") ) {
        // For footnotes (but not endnotes), wrap in <sup> (to get the
        // same effect as the following in docx.css:
        //   a[type="note"] { vertical-align: super; font-size: 70%; }
        m_writer->OnTagOpen(L"", L"sup");
        m_writer->OnTagBody();
    }
#endif
    lString16 t(text);
    m_writer->OnText(t.c_str(), t.length(), 0);
#ifndef DOCX_CRENGINE_IN_PAGE_FOOTNOTES
    if( !lStr_cmp(type, "note") ) {
        m_writer->OnTagClose(L"", L"sup");
    }
#endif
    m_writer->OnTagClose(L"", L"a");
}

void docx_ElementHandler::setChildrenInfo(const struct item_def_t *tags)
{
    m_children = tags;
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

void docx_rPrHandler::handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue)
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
        m_rPr->m_rStyle = attrvalue;
        if ( !m_rPr->m_rStyle.empty() ) {
            docxStyle *style = m_importContext->getStyle(m_rPr->m_rStyle);
            if( style && (docx_character_style == style->getStyleType()) ) {
                m_rPr->combineWith(style->get_rPr(m_importContext));
            }
        }
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

void docx_rPrHandler::start(docx_rPr * const rPr)
{
    m_rPr = rPr;
    docx_ElementHandler::start();
}

void docx_rHandler::handleInstruction(lString16 &instruction, lString16 parameters)
{
    if( instruction == cs16("REF") || instruction == cs16("NOTEREF") || instruction == cs16("PAGEREF") ) {
        lString16 argument, switches;
        if( parameters.split2( cs16(" "), argument, switches) && !argument.empty() )
        {
            m_importContext->m_linkNode = m_writer->OnTagOpen(L"", L"a");
            lString16 target = L"#";
            target  << argument;
            m_writer->OnAttribute(L"", L"href", target.c_str());
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
            m_pHandler->closeStyleTags(&m_rPr);
            m_pHandler->openStyleTags(&m_rPr);
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

void docx_rHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
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
                m_writer->OnTagClose(L"", L"a");
                m_importContext->m_linkNode = NULL;
            }
            m_importContext->m_inField = false;
        }
    }
}

void docx_rHandler::handleText(const lChar16 *text, int len, lUInt32 flags)
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

void docx_rHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    lChar16 nobsp = 0x00A0;
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_br:
        m_writer->OnTagOpenAndClose(L"", L"br");
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
            lString16 target = L"#n_";
            target  << m_footnoteId;
            generateLink(target.c_str(), L"note", m_footnoteId.c_str());
        }
        m_state = docx_el_r;
        break;
    case docx_el_instrText:
        if( m_importContext->m_inField ) {
            m_instruction.trim();
            if ( !m_instruction.empty() ) {
                lString16 instruction, parameters;
                if ( m_instruction.split2(cs16(" "), instruction, parameters) )
                    handleInstruction(instruction, parameters);
            }
        }
        m_state = docx_el_r;
        break;
    case docx_el_endnoteReference:
        if( !m_footnoteId.empty() ) {
            m_importContext->m_endNoteCount++;
            lString16 target = L"#c_";
            target  << m_footnoteId;
            generateLink(target.c_str(), L"comment", m_footnoteId.c_str());
        }
        m_state = docx_el_r;
        break;
    case docx_el_footnoteRef:
    case docx_el_endnoteRef:
        if(!m_importContext->m_footNoteId.empty()) {
            m_writer->OnTagOpen(L"", L"sup");
            m_writer->OnTagBody();
            m_writer->OnText(m_importContext->m_footNoteId.c_str(), m_importContext->m_footNoteId.length(), 0);
            m_writer->OnTagClose(L"", L"sup");
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

void docx_pPrHandler::handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue)
{
    switch(m_state) {
    case docx_el_pStyle:
        if( !lStr_cmp(attrname, "val") ) {
             m_pPr->m_pStyleId = attrvalue;
             if ( !m_pPr->m_pStyleId.empty() ) {
                 docxStyle* style = m_importContext->getStyle(m_pPr->m_pStyleId);
                 if( style && (docx_paragraph_style == style->getStyleType()) ) {
                    m_pPr->combineWith(style->get_pPr(m_importContext));
                 }
             }
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
            m_pPr->set(docx_p_line_spacing_prop, val);
        } else if( !lStr_cmp(attrname, "lineRule") ) {
            int attr_value = parse_name(lineRule_attr_values, attrvalue);
            if( -1 != attr_value )
                m_pPr->setLineRule((docx_lineRule_type)attr_value);
        } else if ( !lStr_cmp(attrname, "afterAutospacing") ) {
            m_pPr->set(docx_p_after_auto_spacing_prop, parse_OnOff_attribute(attrvalue));
        } else if ( !lStr_cmp(attrname, "beforeAutospacing") ) {
            m_pPr->set(docx_p_before_auto_spacing_prop, parse_OnOff_attribute(attrvalue));
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
            m_pPr->set(docx_p_ilvl_prop, val.value);
        }
        break;
    case docx_el_numId:
        if( !lStr_cmp(attrname, "val") ) {
            css_length_t val;
            parse_int(attrvalue, val);
            m_pPr->set(docx_p_num_id_prop, val);
        }
        break;
    case docx_el_outlineLvl:
        if( !lStr_cmp(attrname, "val") ) {
            css_length_t val;
            parse_int(attrvalue, val);
            m_pPr->set(docx_p_outline_level_prop, val);
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

void docx_pPrHandler::handleTagClose( const lChar16 * nsname, const lChar16 * tagname )
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

void docx_pPrHandler::start(docx_pPr *pPr)
{
    m_pPr = pPr;
    docx_ElementHandler::start();
}

const lChar16 *docx_pHandler::getStyleTagName(lChar16 ch)
{
    switch ( ch ) {
    case 'b':
        return L"strong";
    case 'i':
        return L"em";
    case 'u':
        return L"u";
    case 's':
        return L"s";
    case 't':
        return L"sup";
    case 'd':
        return L"sub";
    default:
        return NULL;
    }
}

void docx_pHandler::closeStyleTag(lChar16 ch)
{
    int pos = styleTagPos( ch );
    if (pos >= 0) {
        for (int i = m_styleTags.length() - 1; i >= pos; i--) {
            const lChar16 * tag = getStyleTagName(m_styleTags[i]);
            m_styleTags.erase(m_styleTags.length() - 1, 1);
            if ( tag ) {
                m_writer->OnTagClose(L"", tag);
            }
        }
    }
}

void docx_pHandler::openStyleTag(lChar16 ch)
{
    int pos = styleTagPos( ch );
    if (pos < 0) {
        const lChar16 * tag = getStyleTagName(ch);
        if ( tag ) {
            m_writer->OnTagOpenNoAttr(L"", tag);
            m_styleTags.append( 1,  ch );
        }
    }
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
                    m_writer->OnTagClose(L"", L"li");
                m_writer->OnTagOpen(L"", L"li");
            } else {
                if( m_importContext->isInList() )
                    m_importContext->closeList(0, m_writer);
                if( m_inTitle )
                    m_titleHandler->onTitleStart(outlineLevel.value + 1);
                else
                    m_writer->OnTagOpen(L"", L"p");
            }
            lString16 style = m_pPr.getCss();
            if( !style.empty() )
                m_writer->OnAttribute(L"", L"style", style.c_str());
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

void docx_pHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
{
    if( docx_el_bookmarkStart == m_state && !lStr_cmp(attrname, "name") ) {
        m_writer->OnTagOpen(L"", L"a");
        m_writer->OnAttribute(L"", L"id", attrvalue);
        m_writer->OnTagBody();
        m_writer->OnTagClose(L"", L"a");
    }
}

void docx_pHandler::handleTagClose( const lChar16 * nsname, const lChar16 * tagname )
{
    CR_UNUSED2(nsname, tagname);

    switch(m_state) {
    case docx_el_p:
        closeStyleTags();
        if( m_pPr.getNumberingId() == 0 ) {
            if( !m_inTitle ) {
                m_writer->OnTagClose(L"", L"p");
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

void docx_pHandler::openStyleTags(docx_rPr *runProps)
{
    if(runProps->isBold())
        openStyleTag('b');
    if(runProps->isItalic())
        openStyleTag('i');
    if(runProps->isUnderline())
        openStyleTag('u');
    if(runProps->isStrikeThrough())
        openStyleTag('s');
    if(runProps->isSubScript())
        openStyleTag('d');
    if(runProps->isSuperScript())
        openStyleTag('t');
}

void docx_pHandler::closeStyleTags(docx_rPr *runProps)
{
    if(!runProps->isBold())
        closeStyleTag('b');
    if(!runProps->isItalic())
        closeStyleTag('i');
    if(!runProps->isUnderline())
        closeStyleTag('u');
    if(!runProps->isStrikeThrough())
        closeStyleTag('s');
    if(!runProps->isSubScript())
        closeStyleTag('d');
    if(!runProps->isSuperScript())
        closeStyleTag('t');

}

void docx_pHandler::closeStyleTags()
{
    for(int i = m_styleTags.length() - 1; i >= 0; i--)
        closeStyleTag(m_styleTags[i]);
    m_styleTags.clear();
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

void docx_documentHandler::handleAttribute(const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue)
{
    if (m_state == docx_el_document && !lStr_cmp(nsname, "xmlns") )
        CRLog::debug("namespace declaration %s:%s",  LCSTR(attrname), LCSTR(attrvalue));
}

void docx_documentHandler::handleTagClose( const lChar16 * nsname, const lChar16 * tagname )
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
        m_pPrHandler.start(&m_style->m_pPr);
        break;
    case docx_el_rPr:
        m_rPrHandler.start(&m_style->m_rPr);
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

void docx_styleHandler::handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue)
{
    switch(m_state) {
    case docx_el_style:
        if ( !lStr_cmp(attrname, "type") ) {
            int attr_value = parse_name(styleType_attr_values, attrvalue);
            if( -1 != attr_value )
                m_style->setStyleType((docx_style_type)attr_value);
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

void docx_styleHandler::handleTagClose( const lChar16 * nsname, const lChar16 * tagname )
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
    m_styleRef = docxStyleRef( new docxStyle );
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

void docx_stylesHandler::handleTagClose( const lChar16 * nsname, const lChar16 * tagname )
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
#ifdef DOCX_CRENGINE_IN_PAGE_FOOTNOTES
            writer.OnTagClose(L"", docx_el_body_name);
#else
            // We didn't add <body name=notes> to not trigger crengine auto-in-page-foonotes
            // mechanism, so we can tweak them with style tweaks. We used a simple <div> instead.
            writer.OnTagClose(L"", L"div");
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
    docx_fb2TitleHandler titleHandler(&writer, &importContext, DOCX_USE_CLASS_FOR_HEADING); //<section><title>..</title></section>
#else
    docx_titleHandler titleHandler(&writer, &importContext);  //<hx>..</hx>
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
#ifdef DOCX_FB2_DOM_STRUCTURE
    writer.OnTagClose(NULL, L"FictionBook");
#else
    writer.OnTagClose(NULL, L"html");
#endif
    writer.OnStop();

    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        doc->compact();
        doc->dumpStatistics();
    }
    return true;
}

docxStyle::docxStyle() : m_type(docx_paragraph_style),
    m_pPrMerged(false), m_rPrMerged(false)
{
}

bool docxStyle::isValid() const
{
    return ( !(m_Name.empty() || m_Id.empty()) );
}

docxStyle *docxStyle::getBaseStyle(docxImportContext *context)
{
    lString16 basedOn = getBasedOn();
    if ( !basedOn.empty() ) {
        docxStyle *pStyle = context->getStyle(basedOn);
        if( pStyle && pStyle->getStyleType() == getStyleType() )
            return pStyle;
    }
    return NULL;
}

docx_pPr *docxStyle::get_pPr(docxImportContext *context)
{
    if( !m_pPrMerged ) {
        docxStyle* pStyle = getBaseStyle(context);
        if (pStyle ) {
            m_pPr.combineWith(pStyle->get_pPr(context));
        }
        m_pPrMerged = true;
    }
    return &m_pPr;
}

docx_rPr *docxStyle::get_rPr(docxImportContext *context)
{
    if( !m_rPrMerged ) {
        docxStyle* pStyle = getBaseStyle(context);
        if (pStyle ) {
            m_rPr.combineWith(pStyle->get_rPr(context));
        }
        m_rPrMerged = true;
    }
    return &m_rPr;
}

docxImportContext::docxImportContext(OpcPackage *package, ldomDocument *doc) : m_styles(64), m_abstractNumbers(16),
    m_Numbers(16), m_footNoteCount(0), m_endNoteCount(0),
    m_inField(false), m_linkNode(NULL), m_pStyle(NULL),
    m_package(package), m_doc(doc)
{
}

docxImportContext::~docxImportContext()
{
}

docxStyle * docxImportContext::getStyle( lString16 id )
{
    return m_styles.get(id).get();
}

void docxImportContext::addStyle( docxStyleRef style )
{
    docxStyle *referenced = style.get();
    if ( NULL != referenced)
    {
        m_styles.set(referenced->getId(), style);
    }
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

LVStreamRef docxImportContext::openContentPart(const lChar16 * const contentType)
{
    m_docPart = m_package->getContentPart(contentType);
    if( !m_docPart.isNull() ) {
        return m_docPart->open();
    }
    return LVStreamRef();
}

LVStreamRef docxImportContext::openRelatedPart(const lChar16 * const relationshipType)
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
        writer->OnTagOpen(L"", L"ol");
        lString16 listStyle = getListStyle(listType);
         m_ListLevels.add(listType);
        if ( !listStyle.empty() )
            writer->OnAttribute(L"", L"style", listStyle.c_str());
        writer->OnTagBody();
        if ( i != level - 1 )
            writer->OnTagOpenNoAttr(L"", L"li");
    }
}

void docxImportContext::closeList(int level, ldomDocumentWriter *writer)
{
    for(int i = getListLevel(); i > level; i--) {
        writer->OnTagClose(L"", L"li");
        writer->OnTagClose(L"", L"ol");
        m_ListLevels.remove(getListLevel() - 1);
    }
}

void docxImportContext::setLanguage(const lChar16 *lang)
{
    lString16 language(lang);

    int p = language.pos(cs16("-"));
    if ( p > 0  ) {
        language = language.substr(0, p);
    }
    m_doc->getProps()->setString(DOC_PROP_LANGUAGE, language);
}

lString16 docxImportContext::getListStyle(css_list_style_type_t listType)
{
    switch(listType) {
    case css_lst_disc:
        return lString16("list-style-type: disc;");
    case css_lst_circle:
        return lString16("list-style-type: circle;");
    case css_lst_square:
        return lString16("list-style-type: square;");
    case css_lst_decimal:
        return lString16("list-style-type: decimal;");
    case css_lst_lower_roman:
        return lString16("list-style-type: lower-roman;");
    case css_lst_upper_roman:
        return lString16("list-style-type: upper-roman;");
    case css_lst_lower_alpha:
        return lString16("list-style-type: lower-alpha;");
    case css_lst_upper_alpha:
        return lString16("list-style-type: upper-alpha;");
    default:
        return lString16();
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

void docx_lvlHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
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
                m_writer->OnTagOpen(L"", L"section");
                lString16 id = isEndNote() ? L"c_" : L"n_";
                id << m_importContext->m_footNoteId.c_str();
                m_writer->OnAttribute(L"", L"id", id.c_str());
                m_writer->OnAttribute(L"", L"role", isEndNote() ? L"doc-rearnote" : L"doc-footnote");
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
#ifdef DOCX_CRENGINE_IN_PAGE_FOOTNOTES
        m_writer->OnTagOpen(L"", docx_el_body_name);
        if(isEndNote()) {
            m_writer->OnAttribute(L"", L"name", L"comments");
            m_writer->OnTagBody();
            m_writer->OnTagOpen(L"", L"subtitle");
            m_writer->OnTagBody();
            m_writer->OnText(L"* * *", 5, 0);
            m_writer->OnTagClose(L"", L"subtitle");
        } else {
            m_writer->OnAttribute(L"", L"name", L"notes");
            m_writer->OnTagBody();
        }
#else
        // We don't add <body name=notes> to not trigger crengine auto-in-page-foonotes
        // mechanism, so we can tweak them with style tweaks. We use a simple <div> instead.
        m_writer->OnTagOpen(L"", L"div");
        m_writer->OnAttribute(L"", L"style", L"page-break-before: always");
        m_writer->OnTagBody();
#endif
        //fallthrough
    default:
        m_state = tagId;
        break;
    }
    return NULL;
}

void docx_footnotesHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
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

void docx_footnotesHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    switch (m_state) {
    case docx_el_p:
        m_state = isEndNote() ? docx_el_endnote : docx_el_footnote;
        break;
    case docx_el_endnote:
    case docx_el_footnote:
        m_writer->OnTagClose(L"", L"section");
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
            m_writer->OnTagOpen(L"", L"a");
            m_writer->OnAttribute(L"", L"href", m_target.c_str());
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

void docx_hyperlinkHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
{
    if( docx_el_hyperlink == m_state) {
        if ( !lStr_cmp(attrname, "id") ) {
            m_target = m_importContext->getLinkTarget(lString16(attrvalue));
        } else if (!lStr_cmp(attrname, "anchor") && m_target.empty()) {
            m_target = cs16("#") + lString16(attrvalue);
        }
    }
}

void docx_hyperlinkHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    switch (m_state) {
    case docx_el_hyperlink:
        if ( !m_target.empty() ) {
            m_writer->OnTagClose(L"", L"a");
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

void docx_drawingHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
{
    if( m_state == docx_el_blip && !lStr_cmp(attrname, "embed") ) {
        lString16 imgPath = m_importContext->getImageTarget(lString16(attrvalue));
        if( !imgPath.empty() ) {
            m_writer->OnTagOpen(L"", L"img");
            m_writer->OnAttribute(L"", L"src",  imgPath.c_str());
            m_writer->OnTagBody();
            m_writer->OnTagClose(L"", L"img");
        }
    }
}

void docx_drawingHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
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
                                              rowSpan.column->getDocument()->getAttrNameIndex(L"rowspan"),
                                              lString16::itoa(rowSpan.rows).c_str());
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
        m_writer->OnTagOpenNoAttr(L"", L"tr");
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

void docx_tblHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
{
    if( m_state == docx_el_gridSpan && !lStr_cmp( attrname, "val" ) ) {
        m_colSpan = lString16(attrvalue).atoi();
    } else if( m_state == docx_el_vMerge && !lStr_cmp( attrname, "val" ) ) {
        if( !lStr_cmp( attrvalue, "restart" ) )
            m_vMergeState = VMERGE_RESET;
    }
}

void docx_tblHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    CR_UNUSED2(nsname, tagname);

    if( !m_levels.empty() ) {
        switch(m_state) {
        case docx_el_tblPr:
            m_writer->OnTagOpenNoAttr(L"", L"table");
            break;
        case docx_el_tr:
            m_writer->OnTagClose(L"", L"tr");
            m_rowCount++;
            break;
        case docx_el_tc:
            m_column++;
            if( m_pHandler_ == &m_pHandler )
                m_writer->OnTagClose(L"", L"td");
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
                ldomNode *columnNode = m_writer->OnTagOpen(L"", L"td");
                for(int i = 0; i < m_colSpan; i++) {
                    if( m_column + i >= m_columnCount )
                        break; // shouldn't happen
                    endRowSpan(m_column + i);
                }
                m_rowSpaninfo[m_column] = docx_row_span_info(columnNode);
                if( m_colSpan > 1)
                    m_writer->OnAttribute(L"", L"colspan", lString16::itoa(m_colSpan).c_str() );
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
        m_writer->OnTagClose(L"", L"table");
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

void docx_numberingHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
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

void docx_abstractNumHandler::handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue)
{
    switch(m_state) {
    case docx_el_abstractNum:
        if ( !lStr_cmp(attrname, "abstractNumId") )
            m_abstractNumRef->setId(lString16(attrvalue).atoi());
        break;
    default:
        break;
    }
}

void docx_abstractNumHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
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

void docx_numHandler::handleAttribute(const lChar16 *attrname, const lChar16 *attrvalue)
{
    switch(m_state) {
    case docx_el_num:
        if ( !lStr_cmp(attrname, "numId") )
            m_numRef->setId( lString16(attrvalue).atoi() );
        break;
    case docx_el_abstractNumId:
        if ( !lStr_cmp(attrname, "val") )
            m_numRef->setBaseId( lString16(attrvalue).atoi() );
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

void docx_numHandler::handleTagClose(const lChar16 *nsname, const lChar16 *tagname)
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

void docx_titleHandler::onBodyStart()
{
    m_writer->OnTagOpen(L"", docx_el_body_name);
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
    m_section = m_writer->OnTagOpen(L"", docx_el_body_name);
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
