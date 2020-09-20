
// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#include "my_hyphpatternreader.h"

MyHyphPatternReader::MyHyphPatternReader(lString16Collection &result) : insidePatternTag(false), data(result)
{
    result.clear();
}

ldomNode *MyHyphPatternReader::OnTagOpen(const lChar16 *nsname, const lChar16 *tagname)
{
    CR_UNUSED(nsname);
    if (!lStr_cmp(tagname, "pattern")) {
        insidePatternTag = true;
    }
    return NULL;
}

void MyHyphPatternReader::OnTagClose(const lChar16 *nsname, const lChar16 *tagname, bool self_closing_tag)
{
    CR_UNUSED2(nsname, tagname);
    insidePatternTag = false;
}

void MyHyphPatternReader::OnText(const lChar16 *text, int len, lUInt32 flags)
{
    CR_UNUSED(flags);
    if ( insidePatternTag )
        data.add( lString16(text, len) );
}
