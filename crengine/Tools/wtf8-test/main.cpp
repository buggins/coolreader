#include "lvstring.h"

#include <stdio.h>

lUInt32 uni_chars[] = {
    0x10000,        // LINEAR B SYLLABLE B008 A
    0x2026,
    0x10123,        // AEGEAN NUMBER TWO THOUSAND
    0x10081,        // LINEAR B IDEOGRAM B102 WOMAN
    0x1F600,        // GRINNING FACE
    0x1F601,        // GRINNING FACE WITH SMILING EYES
    0x1F602,        // FACE WITH TEARS OF JOY
    0x1F603,        // SMILING FACE WITH OPEN MOUTH
    0x1F604,        // SMILING FACE WITH OPEN MOUTH AND SMILING EYES
    0x1F605,        // SMILING FACE WITH OPEN MOUTH AND COLD SWEAT
    0x1F606,        // SMILING FACE WITH OPEN MOUTH AND TIGHTLY-CLOSED EYES
    0x1F607,        // SMILING FACE WITH HALO
    0x1F608,        // SMILING FACE WITH HORNS
    0x1F609,        // WINKING FACE
    0x1F60A,        // SMILING FACE WITH SMILING EYES
    0x1F60B         // FACE SAVOURING DELICIOUS FOOD
};

int main(int argc, char* argv[])
{
    lString32 src;
    for (size_t i = 0; i < sizeof(uni_chars)/sizeof(lUInt32); i++)
    {
        src.append(1, uni_chars[i]);
    }
    lString8 dst = UnicodeToUtf8(src);
    printf("UTF8: %s\n", dst.c_str());
    lString8 dstw = UnicodeToWtf8(src);
    printf("WTF8: %s\n", dstw.c_str());
    // Back to unicode
    lString32 str2 = Wtf8ToUnicode(dstw);
    //   and compare...
    if (str2.compare(src) == 0)
        printf("OK, strings is equal.\n");
    else
        printf("Sorry, strings is NOT equal!\n");
    return 0;
}
