#include "../include/fb3fmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"

const lChar16 * const fb3_BodyContentType = L"application/fb3-body+xml";
const lChar16 * const fb3_PropertiesContentType = L"application/vnd.openxmlformats-package.core-properties+xml";
const lChar16 * const fb3_DescriptionContentType = L"application/fb3-description+xml";
const lChar16 * const fb3_CoverRelationship = L"http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail";
const lChar16 * const fb3_ImageRelationship = L"http://www.fictionbook.org/FictionBook3/relationships/image";

class OpcPackage;
class OpcPart;
typedef LVFastRef<OpcPart> OpcPartRef;

class OpcPart : public LVRefCounter
{
public:
    ~OpcPart();
    LVStreamRef open();
    lString16 getRelatedPartName(const lChar16 * const relationType, const lString16 id = lString16());
    OpcPartRef getRelatedPart(const lChar16 * const relationType, const lString16 id = lString16());
protected:
    OpcPart(OpcPackage* package, lString16 name):
       m_relations(16), m_package(package), m_name(name), m_relationsValid(false)
    {
    }
    void readRelations();
    lString16 getTargetPath(const lString16 srcPath, const lString16 targetMode, lString16 target);
    OpcPart* createPart(OpcPackage* package, lString16 name) {
        return new OpcPart(package, name);
    }
private:
    LVHashTable<lString16, LVHashTable<lString16, lString16> *> m_relations;
    OpcPackage* m_package;
    lString16 m_name;
    bool m_relationsValid;
private:
    // non copyable
    OpcPart();
    OpcPart( const OpcPart& );
    OpcPart& operator=( const OpcPart& );
};


class OpcPackage : public OpcPart
{
private:
    bool m_contentTypesValid;
    LVContainerRef m_container;
    LVHashTable<lString16, lString16> m_contentTypes;
private:
    // non copyable
    OpcPackage();
    OpcPackage( const OpcPackage& );
    OpcPackage& operator=( const OpcPart& );
public:
    OpcPackage(LVContainerRef container) : OpcPart(this, L"/"),
        m_contentTypesValid(false), m_container(container),
        m_contentTypes(16)
    {
    }
    LVStreamRef open(lString16 partName) {
        return m_container->OpenStream(partName.c_str(), LVOM_READ);
    }
    lString16 getContentPartName(const lChar16* contentType);
    OpcPartRef getContentPart(const lChar16* contentType) {
        return getPart(getContentPartName(contentType));
    }
    LVStreamRef openContentPart(const lChar16* contentType) {
        return open(getContentPartName(contentType));
    }
    OpcPartRef getPart(const lString16 partName);
    bool partExist(const lString16 partName);
private:
    void readContentTypes();
};

class fb3ImportContext
{
private:
    OpcPackage *m_package;
    OpcPartRef m_bookPart;
    ldomDocument *m_descDoc;
public:
    fb3ImportContext(OpcPackage *package);
    virtual ~fb3ImportContext();

    lString16 geImageTarget(const lString16 relationId) {
        return m_bookPart->getRelatedPartName(fb3_ImageRelationship, relationId);
    }
    LVStreamRef openBook() {
        m_bookPart = m_package->getContentPart(fb3_BodyContentType);
        m_coverImage = m_package->getRelatedPartName(fb3_CoverRelationship);
        return m_bookPart->open();
    }
    ldomDocument *getDescription();
public:
    lString16 m_coverImage;
};

bool DetectFb3Format( LVStreamRef stream )
{
    LVContainerRef m_arc = LVOpenArchieve( stream );
    if ( m_arc.isNull() )
        return false; // not a ZIP archive

    OpcPackage package(m_arc);

    return package.partExist(package.getContentPartName(fb3_BodyContentType));
}

class fb3DomWriter : public LVXMLParserCallback
{
private:
    fb3ImportContext *m_context;
    ldomDocumentWriter *m_parent;
    bool m_checkRole;
protected:
    void writeDescription();
public:
    /// constructor
    fb3DomWriter(ldomDocumentWriter * parent, fb3ImportContext *importContext ) :
        m_context(importContext), m_parent(parent), m_checkRole(false)
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

    OpcPackage package(arc);

    fb3ImportContext context(&package);

    doc->setContainer(arc);

    CRPropRef doc_props = doc->getProps();

    LVStreamRef propStream = package.openContentPart(fb3_PropertiesContentType);

    if ( !propStream.isNull() ) {
        ldomDocument * propertiesDoc = LVParseXMLStream( propStream );
        if ( propertiesDoc ) {
            lString16 author = propertiesDoc->textFromXPath( cs16("coreProperties/creator") );
            lString16 title = doc->textFromXPath( cs16("coreProperties/title") );
            doc_props->setString(DOC_PROP_TITLE, title);
            doc_props->setString(DOC_PROP_AUTHORS, author );
            CRLog::info("Author: %s Title: %s", author.c_str(), title.c_str());
            delete propertiesDoc;
        } else {
            CRLog::error("Couldn't parse core properties");
        }
    } else {
        CRLog::error("Couldn't read core properties");
    }

    ldomDocument * descDoc = context.getDescription();

    if ( descDoc ) {
        lString16 language = descDoc->textFromXPath( cs16("fb3-description/lang") );
        doc_props->setString(DOC_PROP_LANGUAGE, language);
    } else {
        CRLog::error("Couldn't parse description doc");
    }

#if BUILD_LITE!=1
    if ( doc->openFromCache(formatCallback) ) {
        if ( progressCallback ) {
            progressCallback->OnLoadFileEnd( );
        }
        return true;
    }
#endif

    LVStreamRef bookStream = context.openBook();
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
    }

    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        doc->compact();
        doc->dumpStatistics();
    }

    return ret;
}

fb3ImportContext::fb3ImportContext(OpcPackage *package) : m_package(package), m_descDoc(NULL)
{
}

fb3ImportContext::~fb3ImportContext()
{
    if(m_descDoc)
        delete  m_descDoc;
}

ldomDocument *fb3ImportContext::getDescription()
{
    if( !m_descDoc ) {
        LVStreamRef descStream = m_package->openContentPart(fb3_DescriptionContentType);

        if ( !descStream.isNull() ) {
            m_descDoc = LVParseXMLStream( descStream );
        }
    }
    return m_descDoc;
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
         m_parent->OnTagClose(NULL, L"body");
         ldomNode *footnotesBody = m_parent->OnTagOpen(NULL,L"body");
         m_parent->OnAttribute(NULL, L"name", L"notes");
         m_parent->OnTagBody();
         return footnotesBody;
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
    } else if ( !lStr_cmp(tagname, "notes" )) {
        tagname = L"body";
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

OpcPart::~OpcPart()
{
    m_relations.clear();
}

LVStreamRef OpcPart::open()
{
    return m_package->open(m_name);
}

lString16 OpcPart::getRelatedPartName(const lChar16 * const relationType, const lString16 id)
{
    if( !m_relationsValid ) {
        readRelations();
        m_relationsValid = true;
    }
    LVHashTable<lString16, lString16> *relationsTable = m_relations.get(relationType);
    if( relationsTable ) {
        if( id.empty() ) {
            LVHashTable<lString16, lString16>::iterator it = relationsTable->forwardIterator();
            LVHashTable<lString16, lString16>::pair *p = it.next();
            if( p ) {
                return p->value; // return first value
            }
        } else {
            return relationsTable->get(id);
        }
    }
    return lString16();
}

OpcPartRef OpcPart::getRelatedPart(const lChar16 * const relationType, const lString16 id)
{
    return m_package->getPart( getRelatedPartName(relationType, id) );
}

void OpcPart::readRelations()
{
    lString16 relsPath = LVExtractPath(m_name) + cs16("_rels/") + LVExtractFilename(m_name) + cs16(".rels");
    LVStreamRef container_stream = m_package->open(relsPath);

    if ( !container_stream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( container_stream );
        lString16 srcPath = LVExtractPath(m_name);

        if ( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs16("Relationships"));
            if( root ) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * relationshipNode = root->getChildNode((lUInt32)i);
                    const lString16 relType = relationshipNode->getAttributeValue(L"Type");
                    LVHashTable<lString16, lString16> *relationsTable = m_relations.get(relType);
                    if( !relationsTable ) {
                        relationsTable = new LVHashTable<lString16, lString16>(16);
                        m_relations.set(relType, relationsTable);
                    }
                    const lString16 id = relationshipNode->getAttributeValue(L"Id");
                    relationsTable->set( id, getTargetPath(srcPath, relationshipNode->getAttributeValue(L"TargetMode"),
                                                           relationshipNode->getAttributeValue(L"Target")) );
                }
            }
            delete doc;
        }
    }
}

lString16 OpcPart::getTargetPath(const lString16 srcPath, const lString16 targetMode, lString16 target)
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

lString16 OpcPackage::getContentPartName(const lChar16 *contentType)
{
    if ( !m_contentTypesValid ) {
        readContentTypes();
        m_contentTypesValid = true;
    }
    return m_contentTypes.get(contentType);
}

OpcPartRef OpcPackage::getPart(const lString16 partName)
{
    return OpcPartRef(createPart(this, partName));
}

bool OpcPackage::partExist(const lString16 partName)
{
    LVStreamRef partStream = open(partName);
    return !partStream.isNull();
}

void OpcPackage::readContentTypes()
{
    LVStreamRef mtStream = m_container->OpenStream(L"[Content_Types].xml", LVOM_READ );
    if ( !mtStream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( mtStream );
        if( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs16("Types"));
            if(root) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * typeNode = root->getChildNode(i);

                    if(typeNode->getNodeName() == cs16("Override")) //Don't care about Extensions
                        m_contentTypes.set( typeNode->getAttributeValue(L"ContentType"),
                                            typeNode->getAttributeValue(L"PartName") );
                }
            }
            delete doc;
        }
    }
}
