/** @file lvopc.h

    CoolReader Engine

    ODT/DOCX support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2019-2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef LVOPC_H
#define LVOPC_H

#include "lvstream.h"
#include "lvcontainer.h"
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
    lString32 getRelatedPartName(const lChar32 * const relationType, const lString32 id = lString32());
    OpcPartRef getRelatedPart(const lChar32 * const relationType, const lString32 id = lString32());
protected:
    OpcPart(OpcPackage* package, lString32 name):
       m_relations(16), m_package(package), m_name(name), m_relationsValid(false)
    {
    }
    void readRelations();
    lString32 getTargetPath(const lString32 srcPath, const lString32 targetMode, lString32 target);
    OpcPart* createPart(OpcPackage* package, lString32 name) {
        return new OpcPart(package, name);
    }
private:
    LVHashTable<lString32, LVHashTable<lString32, lString32> *> m_relations;
    OpcPackage* m_package;
    lString32 m_name;
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
    LVHashTable<lString32, lString32> m_contentTypes;
private:
    // non copyable
    OpcPackage();
    OpcPackage( const OpcPackage& );
    OpcPackage& operator=( const OpcPart& );
public:
    OpcPackage(LVContainerRef container) : OpcPart(this, U"/"),
        m_contentTypesValid(false), m_container(container),
        m_contentTypes(16)
    {
    }
    LVStreamRef open(lString32 partName) {
        return m_container->OpenStream(partName.c_str(), LVOM_READ);
    }
    lString32 getContentPartName(const lChar32* contentType);
    OpcPartRef getContentPart(const lChar32* contentType) {
        return getPart(getContentPartName(contentType));
    }
    LVStreamRef openContentPart(const lChar32* contentType) {
        return open(getContentPartName(contentType));
    }
    OpcPartRef getPart(const lString32 partName);
    bool partExist(const lString32 partName);
    void readCoreProperties(CRPropRef doc_props);
private:
    void readContentTypes();
};

#endif // LVOPC_H
