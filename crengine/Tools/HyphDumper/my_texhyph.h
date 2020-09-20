
// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#ifndef MY_TEXHYPH_H
#define MY_TEXHYPH_H

#include "hyphman.h"
#include "my_texpattern.h"

class MyTexHyph : public HyphMethod
{
    MyTexPattern * table[PATTERN_HASH_SIZE];
    lUInt32 _hash;
    lUInt32 _pattern_count;
public:
    int largest_overflowed_word;
    bool match( const lChar16 * str, char * mask );
    virtual bool hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize );
    void addPattern( MyTexPattern * pattern );
    MyTexHyph( lString16 id=HYPH_DICT_ID_DICTIONARY, int leftHyphenMin=HYPHMETHOD_DEFAULT_HYPHEN_MIN, int rightHyphenMin=HYPHMETHOD_DEFAULT_HYPHEN_MIN );
    virtual ~MyTexHyph();
    bool load( LVStreamRef stream );
    bool load( lString16 fileName );
    virtual lUInt32 getHash() { return _hash; }
    virtual lUInt32 getCount() { return _pattern_count; }
    virtual lUInt32 getSize();

    // added to dump content to stream
    bool dump(LVStreamRef stream, const lString8 &title);
};

#endif // MY_TEXHYPH_H
