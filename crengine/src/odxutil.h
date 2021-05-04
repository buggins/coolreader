#include <lvtinydom.h>

// build FB2 DOM, comment out to build HTML DOM
#define DOCX_FB2_DOM_STRUCTURE 1
//If true <title class="hx"><p>...</p></title> else <title><hx>..</hx></title>
#define DOCX_USE_CLASS_FOR_HEADING true
// comment this out to disable in-page footnotes
#define ODX_CRENGINE_IN_PAGE_FOOTNOTES 1

enum odx_style_type {
    odx_invalid_style,
    odx_paragraph_style,
    odx_character_style,
    odx_table_style,
    odx_numbering_style
};

enum odx_lineRule_type {
    odx_lineRule_atLeast,
    odx_lineRule_auto,
    odx_lineRule_exact
};

class odx_StylePropertiesGetter
{
public:
    virtual css_length_t get(int index) const = 0;
};

class odx_Style;
typedef LVFastRef< odx_Style > odx_StyleRef;

class odx_ImportContext;

template <int N>
class odx_StylePropertiesContainer : public odx_StylePropertiesGetter
{
    odx_style_type m_styleType;
    lString32 m_styleId;
public:
    static const int PROP_COUNT = N;

    virtual void reset() {
        init();
        m_styleId.clear();
    }

    virtual ~odx_StylePropertiesContainer() {}

    odx_StylePropertiesContainer(odx_style_type styleType) : m_styleType(styleType) {
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

    void combineWith(const odx_StylePropertiesGetter* other)
    {
        for(int i = 0; other && i < PROP_COUNT; i++) {
            css_length_t baseValue = other->get(i);
            if( get(i).type == css_val_unspecified &&
                baseValue.type != css_val_unspecified)
                set(i, baseValue);
        }
    }
    void setStyleId(odx_ImportContext* context, const lChar32* styleId);
    odx_Style* getStyle(odx_ImportContext* context);
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


enum odx_run_properties
{
    odx_run_italic_prop,
    odx_run_bold_prop,
    odx_run_underline_prop,
    odx_run_strikethrough_prop,
    odx_run_hidden_prop,
    odx_run_halign_prop,
    odx_run_valign_prop,
    odx_run_font_size_prop,
    odx_run_max_prop
};

class odx_rPr : public odx_StylePropertiesContainer<odx_run_max_prop>
{
public:
    odx_rPr();
    ///properties
    inline bool isBold() const { return getValue(odx_run_bold_prop, false); }
    inline void setBold(bool value) { set(odx_run_bold_prop, value); }
    inline bool isItalic() const { return getValue(odx_run_italic_prop, false); }
    inline void setItalic(bool value) { set(odx_run_italic_prop, value); }
    inline bool isUnderline() const { return getValue(odx_run_underline_prop, false); }
    inline void setUnderline(bool value) { set(odx_run_underline_prop, value); }
    inline bool isStrikeThrough() const { return getValue(odx_run_strikethrough_prop, false); }
    inline void setStrikeThrough(bool value) { set(odx_run_strikethrough_prop, value); }
    inline bool isSubScript() const { return (getVertAlign() == css_va_sub);  }
    inline bool isSuperScript() const { return (getVertAlign() == css_va_super); }
    inline bool isHidden() const { return getValue(odx_run_hidden_prop, false); }
    inline void setHidden(bool value) { set(odx_run_hidden_prop, value); }
    inline css_text_align_t getTextAlign() const {
        return getValue(odx_run_halign_prop, css_ta_inherit);
    }
    inline void setTextAlign( css_text_align_t value ) { set(odx_run_halign_prop, value); }
    inline css_vertical_align_t getVertAlign() const {
        return getValue(odx_run_valign_prop, css_va_inherit);
    }
    inline void setVertAlign(css_vertical_align_t value) { set(odx_run_valign_prop,value); }
    lString32 getCss();
};

enum odx_p_properties {
    odx_p_page_break_before_prop,
    odx_p_keep_next_prop,
    odx_p_mirror_indents_prop,
    odx_p_halign_prop,
    odx_p_valign_prop,
    odx_p_line_rule_prop,
    odx_p_hyphenate_prop,
    odx_p_before_spacing_prop,
    odx_p_after_spacing_prop,
    odx_p_before_auto_spacing_prop,
    odx_p_after_auto_spacing_prop,
    odx_p_line_spacing_prop,
    odx_p_line_height_prop,
    odx_p_left_margin_prop,
    odx_p_right_margin_prop,
    odx_p_indent_prop,
    odx_p_hanging_prop,
    odx_p_outline_level_prop,
    odx_p_num_id_prop,
    odx_p_ilvl_prop,
    odx_p_max_prop
};

class odx_pPr : public odx_StylePropertiesContainer<odx_p_max_prop>
{
public:
    odx_pPr();

    ///properties
    inline css_text_align_t getTextAlign() const {
        return getValue(odx_p_halign_prop, css_ta_inherit);
    }
    inline void setTextAlign( css_text_align_t value ) { set(odx_p_halign_prop, value); }
    inline css_vertical_align_t getVertAlign() const {
        return getValue(odx_p_valign_prop, css_va_inherit);
    }
    inline void setVertAlign(css_vertical_align_t value) { set(odx_p_valign_prop, value); }
    inline css_hyphenate_t getHyphenate() const {
        return getValue(odx_p_hyphenate_prop, css_hyph_inherit);
    }
    inline void setHyphenate( css_hyphenate_t value ) { set(odx_p_hyphenate_prop, value); }
    // page-break-before:always
    inline bool isPageBreakBefore() const { return getValue(odx_p_page_break_before_prop, false); }
    inline void setPageBreakBefore(bool value) { set(odx_p_page_break_before_prop, value); }
    // page-break-after:avoid
    inline bool isKeepNext() const { return getValue(odx_p_keep_next_prop, false); }
    inline void setKeepNext(bool value) { set(odx_p_keep_next_prop, value); }
    inline bool isMirrorIndents() const { return getValue(odx_p_mirror_indents_prop, false); }
    inline void setMirrorIndents(bool value) { set(odx_p_mirror_indents_prop, value); }
    inline odx_lineRule_type getLineRule() const { return getValue(odx_p_line_rule_prop, odx_lineRule_auto); }
    inline void setLineRule(odx_lineRule_type value) { set(odx_p_line_rule_prop, value); }
    inline int getNumberingId() { return getValue(odx_p_num_id_prop, 0); }
    css_length_t getOutlineLvl() { return get(odx_p_outline_level_prop); }
    inline int getNumberingLevel() { return get(odx_p_ilvl_prop).value; }
    lString32 getCss();
};

class odx_ImportContext
{
    LVHashTable<lString32, odx_StyleRef> m_styles;
    odx_rPr m_rPrDefault;
    odx_pPr m_pPrDefault;
protected:
    ldomDocument* m_doc;
public:
    odx_ImportContext(ldomDocument* doc) : m_styles(64), m_doc(doc) { }
    virtual ~odx_ImportContext() {}
    void addStyle( odx_StyleRef style );
    odx_Style * getStyle( lString32 id ) {
        return m_styles.get(id).get();
    }
    inline odx_rPr * get_rPrDefault() { return &m_rPrDefault; }
    inline odx_pPr * get_pPrDefault() { return &m_pPrDefault; }
    void setLanguage(const lChar32 *lang);
    lString32 getListStyleCss(css_list_style_type_t listType);
    void startDocument(ldomDocumentWriter& writer);
    void endDocument(ldomDocumentWriter& writer);
};

class odx_Style : public LVRefCounter
{
    lString32 m_Name;
    lString32 m_Id;
    lString32 m_basedOn;
    odx_style_type m_type;
    odx_pPr m_pPr;
    odx_rPr m_rPr;
    bool m_pPrMerged;
    bool m_rPrMerged;
public:
    odx_Style();

    inline lString32 getName() const { return m_Name; }
    inline void setName(const lChar32 * value) { m_Name = value; }

    inline lString32 getId() const { return m_Id; }
    inline void setId(const lChar32 * value) { m_Id = value; }

    inline lString32 getBasedOn() const { return m_basedOn; }
    inline void setBasedOn(const lChar32 * value) { m_basedOn = value; }
    bool isValid() const;

    inline odx_style_type getStyleType() const { return m_type; }
    inline void setStyleType(odx_style_type value) { m_type = value; }
    odx_Style* getBaseStyle(odx_ImportContext* context);
    odx_pPr * get_pPr(odx_ImportContext* context);
    odx_rPr * get_rPr(odx_ImportContext* context);
    inline odx_pPr * get_pPrPointer() { return &m_pPr; }
    inline odx_rPr * get_rPrPointer() { return &m_rPr; }
    odx_StylePropertiesGetter* getStyleProperties(odx_ImportContext* context,
                                                  odx_style_type styleType);
};

template<int N>
void odx_StylePropertiesContainer<N>::setStyleId(odx_ImportContext *context, const lChar32 *styleId) {
    m_styleId = styleId;
    if ( !m_styleId.empty() ) {
        odx_Style *style = context->getStyle(m_styleId);
        if( style && (m_styleType == style->getStyleType()) ) {
            combineWith(style->getStyleProperties(context, m_styleType));
        }
    }
}

template<int N>
odx_Style *odx_StylePropertiesContainer<N>::getStyle(odx_ImportContext *context) {
    odx_Style* ret = NULL;

    if (!m_styleId.empty() ) {
        ret = context->getStyle(m_styleId);
    }
    return ret;
}

/// known docx items name and identifier
struct item_def_t {
    int      id;
    const lChar32 * name;
};

class xml_ElementHandler;

class docXMLreader : public LVXMLParserCallback
{
private:
    enum xml_doc_reader_state {
        xml_doc_in_start,
        xml_doc_in_xml_declaration,
        xml_doc_in_body,
        xml_doc_in_document
    };
    int m_skipTag;
    xml_doc_reader_state m_state;
protected:
    xml_ElementHandler *m_handler;
    ldomDocumentWriter *m_writer;

    inline bool isSkipping()
    {
        return (m_skipTag != 0);
    }

    inline void skipped()
    {
        m_skipTag--;
    }

public:
    /// constructor
    docXMLreader(ldomDocumentWriter *writer) : m_skipTag(0), m_state(xml_doc_in_start),
        m_handler(NULL), m_writer(writer)
    {
    }

    /// destructor
    virtual ~docXMLreader() { }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser *);
    /// called on parsing end
    virtual void OnStop() {  }

    inline void skip()
    {
        m_skipTag++;
    }

    /// called on opening tag <
    ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname);

    /// called after > of opening tag (when entering tag body)
    void OnTagBody();

    /// called on tag close
    void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );

    /// called on element attribute
    void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue );

    /// called on text
    void OnText( const lChar32 * text, int len, lUInt32 flags );

    /// add named BLOB data to document
    bool OnBlob(lString32 name, const lUInt8 * data, int size);

    xml_ElementHandler * getHandler()
    {
        return m_handler;
    }

    void setHandler(xml_ElementHandler *a_handler)
    {
        m_handler = a_handler;
    }

    void setWriter(ldomDocumentWriter *writer)
    {
        m_writer = writer;
    }
};

class xml_ElementHandler
{
protected:
    docXMLreader * m_reader;
    ldomDocumentWriter *m_writer;
    xml_ElementHandler *m_savedHandler;
    const item_def_t *m_children;
    int m_element;
    int m_state;
protected:
    xml_ElementHandler(docXMLreader * reader, ldomDocumentWriter *writer,
                       int element, const struct item_def_t *children) :
        m_reader(reader), m_writer(writer), m_children(children), m_element(element),
        m_state(element)
    {
    }
    virtual ~xml_ElementHandler() {}
    virtual int parseTagName(const lChar32 *tagname) {
        if(m_children)
            return parse_name(m_children, tagname);
        return -1;
    }
public:
    static int parse_name(const struct item_def_t *tags, const lChar32 * nameValue);
    static void parse_int(const lChar32 * attrValue, css_length_t & result);
    void setChildrenInfo(const struct item_def_t *tags);
    ldomNode * handleTagOpen(const lChar32 * nsname, const lChar32 * tagname);
    virtual ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue)
    {
        CR_UNUSED(nsname);

        handleAttribute(attrname, attrvalue);
    }
    virtual void handleAttribute(const lChar32 * attrname, const lChar32 * attrvalue) {
        CR_UNUSED2(attrname, attrvalue);
    }
    virtual void handleTagBody() {}
    virtual void handleText( const lChar32 * text, int len, lUInt32 flags ) {
        CR_UNUSED3(text,len,flags);
    }
    virtual void handleTagClose( const lChar32 * nsname, const lChar32 * tagname )
    {
        CR_UNUSED2(nsname, tagname);

        if(m_state == m_element)
            stop();
        else
            m_state = m_element;
    }
    virtual void start();
    virtual void stop();
    virtual void reset();
};

class xml_SkipElementHandler : public xml_ElementHandler
{
public:
    xml_SkipElementHandler(docXMLreader * reader, ldomDocumentWriter *writer,
                           int element) : xml_ElementHandler(reader, writer, element, NULL) {}
    void skipElement(int element) {
        m_state = element;
        start();
    }
};

class odx_styleTagsHandler
{
    lString32 m_styleTags;
    int styleTagPos(lChar32 ch);
protected:
    const lChar32 * getStyleTagName( lChar32 ch );
    void closeStyleTag( lChar32 ch, ldomDocumentWriter *writer);
    void openStyleTag(lChar32 ch, ldomDocumentWriter *writer);
public:
    odx_styleTagsHandler() {}
    void openStyleTags(odx_rPr* runProps, ldomDocumentWriter *writer);
    void closeStyleTags(odx_rPr* runProps, ldomDocumentWriter *writer);
    void closeStyleTags(ldomDocumentWriter *writer);
};

class odx_titleHandler
{
public:
    odx_titleHandler(ldomDocumentWriter *writer, bool useClassName=false) :
        m_writer(writer), m_titleLevel(), m_useClassName(useClassName) {}
    virtual ~odx_titleHandler() {}
    virtual ldomNode* onBodyStart();
    virtual void onTitleStart(int level, bool noSection = false);
    virtual void onTitleEnd();
    virtual void onBodyEnd() {}
    bool useClassForTitle() { return m_useClassName; }
protected:
    ldomDocumentWriter *m_writer;
    int m_titleLevel;
    bool m_useClassName;
};

class odx_fb2TitleHandler : public odx_titleHandler
{
public:
    odx_fb2TitleHandler(ldomDocumentWriter *writer, bool useClassName) :
        odx_titleHandler(writer, useClassName), m_hasTitle(false)
    {}
    ldomNode* onBodyStart();
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
