#include "../include/lvopc.h"
#include "../include/lvtinydom.h"

static const lChar16 * const OPC_PropertiesContentType = L"application/vnd.openxmlformats-package.core-properties+xml";

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

void OpcPackage::readCoreProperties(CRPropRef doc_props)
{
    LVStreamRef propStream = openContentPart(OPC_PropertiesContentType);

    if ( !propStream.isNull() ) {
        ldomDocument * propertiesDoc = LVParseXMLStream( propStream );
        if ( propertiesDoc ) {
            lString16 author = propertiesDoc->textFromXPath( cs16("coreProperties/creator") );
            lString16 title = propertiesDoc->textFromXPath( cs16("coreProperties/title") );
            doc_props->setString(DOC_PROP_TITLE, title);
            doc_props->setString(DOC_PROP_AUTHORS, author );
            delete propertiesDoc;
        } else {
            CRLog::error("Couldn't parse core properties");
        }
    } else {
        CRLog::error("Couldn't read core properties");
    }
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
