#ifndef LVOPC_H
#define LVOPC_H

#include "lvstream.h"
#include "lvhashtable.h"
#include "props.h"

/*
 * Open Packaging Conventions (OPC)
 * The OPC is specified in Part 2 of the Office Open XML standards ISO/IEC 29500:2008 and ECMA-376
*/

class OpcPart;
typedef LVFastRef<OpcPart> OpcPartRef;
class OpcPackage;

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
    void readCoreProperties(CRPropRef doc_props);
private:
    void readContentTypes();
};

#endif // LVOPC_H
