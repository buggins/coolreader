#include "../include/fb3fmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"

const lChar16 * const fb3_BodyContentType = L"application/fb3-body+xml";
const lChar16 * const fb3_PropertiesContentType = L"application/vnd.openxmlformats-package.core-properties+xml";
const lChar16 * const fb3_CoverRelationship = L"http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail";
const lChar16 * const fb3_ImageRelationship = L"http://www.fictionbook.org/FictionBook3/relationships/image";

class fb3ImportContext
{
private:
    LVHashTable<lString16, lString16> m_relationships;
    LVHashTable<lString16, lString16> m_contentTypes;
    LVContainerRef m_arc;
    lString16 getTargetPath(const lString16 src, const lString16 targetMode, lString16 target);
public:
    fb3ImportContext(LVContainerRef arc);
    virtual ~fb3ImportContext();

    bool readRelations(const lString16 path);
    lString16 readRelationByType(const lString16 path, const lString16 type);
    lString16 geImageTarget(const lString16 relationId) {
        return m_relationships.get( relationId );
    }
    lString16 getContentPath(const lString16 typeId) {
        return m_contentTypes.get( typeId );
    }
    void clearRelations() {
        m_relationships.clear();
    }
    bool readContentTypes();
public:
    lString16 m_coverImage;
};

bool DetectFb3Format( LVStreamRef stream )
{
    LVContainerRef m_arc = LVOpenArchieve( stream );
    bool ret = false;
    if ( m_arc.isNull() )
        return false; // not a ZIP archive

    LVStreamRef mtStream = m_arc->OpenStream(L"[Content_Types].xml", LVOM_READ );
    if ( !mtStream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( mtStream );
        if( doc ) {
            lUInt16 id = doc->findAttrValueIndex(fb3_BodyContentType);
            if ( id != (lUInt16)-1 )
                ret = true;
            delete doc;
        }
    }
    return ret;
}

class fb3DomWriter : public LVXMLParserCallback
{
private:
    fb3ImportContext *m_context;
    ldomDocumentWriter *m_parent;
    ldomNode *m_footnotesBody;
    bool m_checkRole;
protected:
    void writeDescription();
public:
    /// constructor
    fb3DomWriter(ldomDocumentWriter * parent, fb3ImportContext *importContext ) :
        m_parent(parent), m_context(importContext), m_footnotesBody(NULL), m_checkRole(false)
    {
    }
    // LVXMLParserCallback interface
public:
    ldomNode *OnTagOpen(const lChar16 *nsname, const lChar16 *tagname);
    /// called on closing tag
    void OnTagClose( const lChar16 * nsname, const lChar16 * tagname );
    void OnTagBody();
    void OnAttribute(const lChar16 *nsname, const lChar16 *attrname, const lChar16 *attrvalue);

    lUInt32 getFlags() { return m_parent->getFlags(); }
    void setFlags(lUInt32 flags) { m_parent->setFlags(flags); }
    void OnEncoding(const lChar16 *name, const lChar16 *table) { m_parent->OnEncoding(name, table); }
    void OnStart(LVFileFormatParser *parser) { m_parent->OnStart(parser); }
    void OnStop() { m_parent->OnStop(); }
    void OnText(const lChar16 *text, int len, lUInt32 flags) {  m_parent->OnText(text, len, flags); }
    bool OnBlob(lString16 name, const lUInt8 *data, int size) { return m_parent->OnBlob(name, data, size); }
    void OnDocProperty(const char *name, lString8 value)  { m_parent->OnDocProperty(name, value);  }
};

bool ImportFb3Document( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    LVContainerRef arc = LVOpenArchieve( stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive

    fb3ImportContext context(arc);

    if( !context.readContentTypes( )) {
        CRLog::error("Couldn't read content types");
        return false;
    }

    if ( !context.readRelations(context.getContentPath(fb3_BodyContentType)) ) {
        CRLog::error("Couldn't read relations");
        return false;
    }
    doc->setContainer(arc);

#if BUILD_LITE!=1
    if ( doc->openFromCache(formatCallback) ) {
        if ( progressCallback ) {
            progressCallback->OnLoadFileEnd( );
        }
        return true;
    }
#endif

    CRPropRef doc_props = doc->getProps();
    LVStreamRef propStream = arc->OpenStream(context.getContentPath(fb3_PropertiesContentType).c_str(), LVOM_READ );
    if ( propStream.isNull() ) {
        CRLog::error("Couldn't read properties");
        return false;
    }

    ldomDocument * propertiesDoc = LVParseXMLStream( propStream );
    if ( !propertiesDoc ) {
        CRLog::error("Couldn't parse properties doc");
        return false;
    }

    lString16 author = propertiesDoc->textFromXPath( cs16("coreProperties/creator") );
    lString16 title = doc->textFromXPath( cs16("coreProperties/title") );
    doc_props->setString(DOC_PROP_TITLE, title);
    doc_props->setString(DOC_PROP_AUTHORS, author );
    CRLog::info("Author: %s Title: %s", author.c_str(), title.c_str());
    delete propertiesDoc;

    LVStreamRef bookStream = arc->OpenStream(context.getContentPath(fb3_BodyContentType).c_str(), LVOM_READ );
    if ( bookStream.isNull() ) {
        CRLog::error("Couldn't read a book");
        return false;
    }

    ldomDocumentWriter writer(doc);
    fb3DomWriter fb3Writer(&writer, &context);
    LVFileFormatParser * parser = new LVXMLParser(bookStream, &fb3Writer);

    bool ret = parser->Parse();
    delete parser;

    if ( !ret ) {
        CRLog::error("Couldn't parse a book");
    } else {
#if 1
    // save compound XML document, for testing:
    doc->saveToStream(LVOpenFileStream("C:/Temp/fb3_dump.xml", LVOM_WRITE), NULL, true);
#endif

    }

    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        doc->compact();
        doc->dumpStatistics();
    }

    return ret;
}

lString16 fb3ImportContext::getTargetPath(const lString16 srcPath, const lString16 targetMode, lString16 target)
{
    if( !target.empty() ) {
        if ( targetMode == L"External" || target.pos(L":") != -1 )
            return target;

        if( !LVIsAbsolutePath(target) ) {
            target = LVCombinePaths(srcPath, target);
        }
        if( LVIsAbsolutePath(target) ) {
            return target.substr(1);
        }
    }
    return target;
}

fb3ImportContext::fb3ImportContext(LVContainerRef arc) :
    m_relationships(64), m_contentTypes(16), m_arc(arc)
{
    m_contentTypes.set(cs16(fb3_BodyContentType), lString16(L"/fb3/body.xml"));
    m_contentTypes.set(cs16(fb3_PropertiesContentType), lString16(L"/meta/core.xml"));
}

fb3ImportContext::~fb3ImportContext()
{
}

bool fb3ImportContext::readRelations(const lString16 path)
{
    lString16 srcPath = LVExtractPath(path);
    lString16 relsPath = srcPath + "_rels/" + LVExtractFilename(path) + ".rels";

    CRLog::info("rels path = %s", LCSTR(relsPath));

    LVStreamRef container_stream = m_arc->OpenStream(relsPath.c_str(), LVOM_READ);

    if ( !container_stream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( container_stream );
        if ( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs16("Relationships"));
            if( root ) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * relationshipNode = root->getChildNode(i);
                    const lString16 relType = relationshipNode->getAttributeValue(L"Type");
                    if( relType != fb3_ImageRelationship)
                        continue;
                    const lString16 id = relationshipNode->getAttributeValue(L"Id");
                    if( !id.empty() ) {
                        m_relationships.set( id,
                                             getTargetPath(srcPath, relationshipNode->getAttributeValue(L"TargetMode"),
                                                           relationshipNode->getAttributeValue(L"Target")) );
                    }
                }
            }
            delete doc;
            m_coverImage = readRelationByType(L"/", fb3_CoverRelationship);
            return true;
        }
    }
    return false;
}

lString16 fb3ImportContext::readRelationByType(const lString16 path, const lString16 type)
{
    lString16 srcPath = LVExtractPath(path);
    lString16 relsPath = srcPath + cs16("_rels/") + LVExtractFilename(path) + cs16(".rels");
    lString16 targetPath;

    LVStreamRef container_stream = m_arc->OpenStream(relsPath.c_str(), LVOM_READ);
    if ( !container_stream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( container_stream );
        if ( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs16("Relationships"));
            if( root ) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * relationshipNode = root->getChildNode(i);
                    const lString16 relType = relationshipNode->getAttributeValue(L"Type");
                    if( relType == type ) {
                        targetPath = getTargetPath(srcPath, relationshipNode->getAttributeValue(L"TargetMode"),
                                                   relationshipNode->getAttributeValue(L"Target"));
                        // return first found
                        break;
                    }
                }
            }
            delete doc;
        }
    }
    return targetPath;
}

bool fb3ImportContext::readContentTypes()
{
    bool ret = false;

    LVStreamRef mtStream = m_arc->OpenStream(L"[Content_Types].xml", LVOM_READ );
    if ( !mtStream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( mtStream );
        if( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs16("Types"));
            if(root) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * typeNode = root->getChildNode(i);
                    if(typeNode->getNodeName() == cs16("Override")) {
                        const lString16 contentType = typeNode->getAttributeValue(L"ContentType");
                        const lString16 partName = typeNode->getAttributeValue(L"PartName");
                        if( !contentType.empty() && !partName.empty() ) {
                            m_contentTypes.set( contentType, partName );
                        }
                    }
                }
            }
            delete doc;
            ret = true;
        }
    }
    return ret;
}

void fb3DomWriter::writeDescription()
{
    //TODO extended FB3 description
    m_parent->OnTagOpenNoAttr( NULL, L"description" );
    m_parent->OnTagOpenNoAttr( NULL, L"title-info" );
    m_parent->OnTagOpenNoAttr( NULL, L"book-title" );
    m_parent->OnTagClose( NULL, L"book-title" );
    if ( !m_context->m_coverImage.empty() ) {
        m_parent->OnTagOpenNoAttr( NULL, L"coverpage" );
        m_parent->OnTagOpen(NULL, L"image");
        m_parent->OnAttribute(L"l", L"href", m_context->m_coverImage.c_str());
        m_parent->OnTagClose( NULL, L"image" );
        m_parent->OnTagClose( NULL, L"coverpage" );
    }
    m_parent->OnTagClose( NULL, L"title-info" );
    m_parent->OnTagClose( NULL, L"description" );
}

ldomNode *fb3DomWriter::OnTagOpen(const lChar16 *nsname, const lChar16 *tagname)
{
    if( !lStr_cmp(tagname, "fb3-body") ) {
        m_parent->OnTagOpenNoAttr(NULL, L"FictionBook");
        writeDescription();
        tagname = L"body";
    } else if ( !lStr_cmp(tagname, "notes" )) {
        if ( m_footnotesBody == NULL ) {
            m_parent->OnTagClose(NULL, L"body");
            m_footnotesBody = m_parent->OnTagOpen(NULL,L"body");
            m_parent->OnAttribute(NULL, L"name", L"notes");
            m_parent->OnTagBody();
            return m_footnotesBody;
        }
    } else if( !lStr_cmp(tagname, "notebody") ) {
        tagname = L"section";
    } else if( !lStr_cmp(tagname, "note") ) {
        m_checkRole = true;
        return m_parent->OnTagOpen(nsname, L"a");
    }
    return m_parent->OnTagOpen(nsname, tagname);
}

void fb3DomWriter::OnTagClose(const lChar16 *nsname, const lChar16 *tagname)
{
    if ( !lStr_cmp(tagname, "fb3-body") ) {
        m_parent->OnTagClose(NULL, L"body");
        tagname = L"FictionBook";
    } else if ( !lStr_cmp(tagname, "notebody") ) {
        tagname = L"section";
    } else if( !lStr_cmp(tagname, "note") ) {
        tagname = L"a";
    }
    m_parent->OnTagClose(nsname, tagname);
}

void fb3DomWriter::OnTagBody()
{
    m_checkRole = false;
    m_parent->OnTagBody();
}

void fb3DomWriter::OnAttribute(const lChar16 *nsname, const lChar16 *attrname, const lChar16 *attrvalue)
{
    bool pass = true;

    if( !lStr_cmp(attrname, "href") ) {
        lString16 href(attrvalue);

        if ( href.pos(":") == -1 && href[0] != '#') {
            href = cs16("#") + href;
            m_parent->OnAttribute(nsname, attrname, href.c_str());
            pass = false;
        }
    } else if ( m_checkRole && !lStr_cmp(attrname, "role") ) {
        if( !lStr_cmp(attrvalue, "footnote") )
            m_parent->OnAttribute(NULL, L"type", L"note");
        else
            m_parent->OnAttribute(NULL, L"type", L"comment");
    } else if ( !lStr_cmp(attrname, "src") ) {
        lString16 target = m_context->geImageTarget(attrvalue);
        if( !target.empty() ) {
            m_parent->OnAttribute(nsname, attrname, target.c_str());
            pass = false;
        }
    }
    if ( pass) {
        m_parent->OnAttribute(nsname, attrname, attrvalue);
    }
}
