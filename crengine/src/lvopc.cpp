/**
    CoolReader Engine

    lvopc.cpp: ODT/DOCX support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2019-2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#include "../include/lvopc.h"
#include "../include/lvtinydom.h"
#include "../include/lvstreamutils.h"
#include "../include/crlog.h"

static const lChar32 * const OPC_PropertiesContentType = U"application/vnd.openxmlformats-package.core-properties+xml";

OpcPart::~OpcPart()
{
    m_relations.clear();
}

LVStreamRef OpcPart::open()
{
    return m_package->open(m_name);
}

lString32 OpcPart::getRelatedPartName(const lChar32 * const relationType, const lString32 id)
{
    if( !m_relationsValid ) {
        readRelations();
        m_relationsValid = true;
    }
    LVHashTable<lString32, lString32> *relationsTable = m_relations.get(relationType);
    if( relationsTable ) {
        if( id.empty() ) {
            LVHashTable<lString32, lString32>::iterator it = relationsTable->forwardIterator();
            LVHashTable<lString32, lString32>::pair *p = it.next();
            if( p ) {
                return p->value; // return first value
            }
        } else {
            return relationsTable->get(id);
        }
    }
    return lString32();
}

OpcPartRef OpcPart::getRelatedPart(const lChar32 * const relationType, const lString32 id)
{
    return m_package->getPart( getRelatedPartName(relationType, id) );
}

void OpcPart::readRelations()
{
    lString32 relsPath = LVExtractPath(m_name) + cs32("_rels/") + LVExtractFilename(m_name) + cs32(".rels");
    LVStreamRef container_stream = m_package->open(relsPath);

    if ( !container_stream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( container_stream );
        lString32 srcPath = LVExtractPath(m_name);

        if ( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs32("Relationships"));
            if( root ) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * relationshipNode = root->getChildNode((lUInt32)i);
                    const lString32 relType = relationshipNode->getAttributeValue(U"Type");
                    LVHashTable<lString32, lString32> *relationsTable = m_relations.get(relType);
                    if( !relationsTable ) {
                        relationsTable = new LVHashTable<lString32, lString32>(16);
                        m_relations.set(relType, relationsTable);
                    }
                    const lString32 id = relationshipNode->getAttributeValue(U"Id");
                    relationsTable->set( id, getTargetPath(srcPath, relationshipNode->getAttributeValue(U"TargetMode"),
                                                           relationshipNode->getAttributeValue(U"Target")) );
                }
            }
            delete doc;
        }
    }
}

lString32 OpcPart::getTargetPath(const lString32 srcPath, const lString32 targetMode, lString32 target)
{
    if( !target.empty() ) {
        if ( targetMode == U"External" || target.pos(U":") != -1 )
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

lString32 OpcPackage::getContentPartName(const lChar32 *contentType)
{
    if ( !m_contentTypesValid ) {
        readContentTypes();
        m_contentTypesValid = true;
    }
    return m_contentTypes.get(contentType);
}

OpcPartRef OpcPackage::getPart(const lString32 partName)
{
    return OpcPartRef(createPart(this, partName));
}

bool OpcPackage::partExist(const lString32 partName)
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
            lString32 author = propertiesDoc->textFromXPath( cs32("coreProperties/creator") );
            lString32 title = propertiesDoc->textFromXPath( cs32("coreProperties/title") );
            lString32 language = propertiesDoc->textFromXPath( cs32("coreProperties/language") );
            lString32 description = propertiesDoc->textFromXPath( cs32("coreProperties/description") );
            doc_props->setString(DOC_PROP_TITLE, title);
            doc_props->setString(DOC_PROP_AUTHORS, author );
            doc_props->setString(DOC_PROP_LANGUAGE, language );
            doc_props->setString(DOC_PROP_DESCRIPTION, description );
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
    LVStreamRef mtStream = m_container->OpenStream(U"[Content_Types].xml", LVOM_READ );
    if ( !mtStream.isNull() ) {
        ldomDocument * doc = LVParseXMLStream( mtStream );
        if( doc ) {
            ldomNode *root = doc->nodeFromXPath(cs32("Types"));
            if(root) {
                for(int i = 0; i < root->getChildCount(); i++) {
                    ldomNode * typeNode = root->getChildNode(i);

                    if(typeNode->getNodeName() == cs32("Override")) //Don't care about Extensions
                        m_contentTypes.set( typeNode->getAttributeValue(U"ContentType"),
                                            typeNode->getAttributeValue(U"PartName") );
                }
            }
            delete doc;
        }
    }
}
