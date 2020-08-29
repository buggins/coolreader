#include <lvxml.h>
#include <lvtinydom.h>

// build FB2 DOM, comment out to build HTML DOM
#define DOCX_FB2_DOM_STRUCTURE 1
//If true <title class="hx"><p>...</p></title> else <title><hx>..</hx></title>
#define DOCX_USE_CLASS_FOR_HEADING true

/// known docx items name and identifier
struct item_def_t {
    int      id;
    const lChar16 * name;
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
    ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname);

    /// called after > of opening tag (when entering tag body)
    void OnTagBody();

    /// called on tag close
    void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );

    /// called on element attribute
    void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue );

    /// called on text
    void OnText( const lChar16 * text, int len, lUInt32 flags );

    /// add named BLOB data to document
    bool OnBlob(lString16 name, const lUInt8 * data, int size);

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
    int m_element;
    int m_state;
protected:
    xml_ElementHandler(docXMLreader * reader, ldomDocumentWriter *writer,
                       int element) :
        m_reader(reader), m_writer(writer), m_element(element), m_state(element)
    {
    }
    virtual ~xml_ElementHandler() {}
    virtual int parseTagName(const lChar16 *tagname) {
        CR_UNUSED(tagname);
        return -1;
    }
public:
    ldomNode * handleTagOpen(const lChar16 * nsname, const lChar16 * tagname);
    virtual ldomNode * handleTagOpen(int tagId);
    void handleAttribute(const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue)
    {
        CR_UNUSED(nsname);

        handleAttribute(attrname, attrvalue);
    }
    virtual void handleAttribute(const lChar16 * attrname, const lChar16 * attrvalue) {
        CR_UNUSED2(attrname, attrvalue);
    }
    virtual void handleTagBody() {}
    virtual void handleText( const lChar16 * text, int len, lUInt32 flags ) {
        CR_UNUSED3(text,len,flags);
    }
    virtual void handleTagClose( const lChar16 * nsname, const lChar16 * tagname )
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
                            int element) : xml_ElementHandler(reader, writer, element) {}
    void skipElement(int element) {
        m_state = element;
        start();
    }
};

class docx_titleHandler
{
public:
    docx_titleHandler(ldomDocumentWriter *writer, bool useClassName=false) :
        m_writer(writer), m_titleLevel(), m_useClassName(useClassName) {}
    virtual ~docx_titleHandler() {}
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

class docx_fb2TitleHandler : public docx_titleHandler
{
public:
    docx_fb2TitleHandler(ldomDocumentWriter *writer, bool useClassName) :
        docx_titleHandler(writer, useClassName)
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
