
// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#ifndef MY_HYPHPATTERNREADER_H
#define MY_HYPHPATTERNREADER_H

#include "lvxml.h"
#include "lvstring.h"
#include "lvstring16collection.h"

class MyHyphPatternReader : public LVXMLParserCallback
{
protected:
    bool insidePatternTag;
    lString16Collection & data;
public:
    MyHyphPatternReader(lString16Collection & result);
    /// called on parsing end
    virtual void OnStop() { }
    /// called on opening tag end
    virtual void OnTagBody() {}
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname);
    /// called on closing
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname, bool self_closing_tag=false );
    /// called on element attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
    {
        CR_UNUSED3(nsname, attrname, attrvalue);
    }
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 flags );
    /// add named BLOB data to document
    virtual bool OnBlob(lString16 name, const lUInt8 * data, int size) {
        CR_UNUSED3(name, data, size);
        return false;
    }
};

#endif // MY_HYPHPATTERNREADER_H
