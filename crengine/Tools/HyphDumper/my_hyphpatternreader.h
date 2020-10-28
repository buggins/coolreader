
// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#ifndef MY_HYPHPATTERNREADER_H
#define MY_HYPHPATTERNREADER_H

#include "lvxml.h"
#include "lvstring.h"
#include "lvstring32collection.h"

class MyHyphPatternReader : public LVXMLParserCallback
{
protected:
    bool insidePatternTag;
    lString32Collection & data;
public:
    MyHyphPatternReader(lString32Collection & result);
    /// called on parsing end
    virtual void OnStop() { }
    /// called on opening tag end
    virtual void OnTagBody() {}
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname);
    /// called on closing
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );
    /// called on element attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue )
    {
        CR_UNUSED3(nsname, attrname, attrvalue);
    }
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags );
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8 * data, int size) {
        CR_UNUSED3(name, data, size);
        return false;
    }
};

#endif // MY_HYPHPATTERNREADER_H
