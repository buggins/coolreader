
// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#include "my_hyphpatternreader.h"

MyHyphPatternReader::MyHyphPatternReader(lString32Collection &result) : insidePatternTag(false), data(result)
{
    result.clear();
}

ldomNode *MyHyphPatternReader::OnTagOpen(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED(nsname);
    if (!lStr_cmp(tagname, "pattern")) {
        insidePatternTag = true;
    }
    return NULL;
}

void MyHyphPatternReader::OnTagClose(const lChar32 *nsname, const lChar32 *tagname, bool self_closing_tag)
{
    CR_UNUSED2(nsname, tagname);
    insidePatternTag = false;
}

void MyHyphPatternReader::OnText(const lChar32 *text, int len, lUInt32 flags)
{
    CR_UNUSED(flags);
    if ( insidePatternTag )
        data.add( lString32(text, len) );
}
