
// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#include "my_texhyph.h"
#include "my_hyphpatternreader.h"

#include <string.h>
#include <stdlib.h>

#include "lvfnt.h"
#include "lvstring.h"
#include "lvstring32collection.h"
#include "crlog.h"

//#define DUMP_PATTERNS 1

struct tPDBHdr
{
    char filename[36];
    lUInt32 dw1;
    lUInt32 dw2;
    lUInt32 dw4[4];
    char type[8];
    lUInt32 dw44;
    lUInt32 dw48;
    lUInt16 numrec;
};

#pragma pack(push, 1)
typedef struct {
    lUInt16         wl;
    lUInt16         wu;
    char            al;
    char            au;

    unsigned char   mask0[2];
    lUInt16         aux[256];

    lUInt16         len;
} thyph;

typedef struct {
    lUInt16 start;
    lUInt16 len;
} hyph_index_item_t;
#pragma pack(pop)


static int isCorrectHyphFile(LVStream * stream)
{
    if (!stream)
        return false;
    lvsize_t   dw;
    int    w = 0;
    tPDBHdr    HDR;
    stream->SetPos(0);
    stream->Read( &HDR, 78, &dw);
    stream->SetPos(0);
    lvByteOrderConv cnv;
    w=cnv.msf(HDR.numrec);
    if (dw!=78 || w>0xff)
        w = 0;

    if (strncmp((const char*)&HDR.type, "HypHAlR4", 8) != 0)
        w = 0;

    return w;
}



MyTexHyph::MyTexHyph( lString32 id, int leftHyphenMin, int rightHyphenMin )
 : HyphMethod(id, leftHyphenMin, rightHyphenMin)
{
    memset( table, 0, sizeof(table) );
    _hash = 123456;
    _pattern_count = 0;
    largest_overflowed_word = 0;
}

MyTexHyph::~MyTexHyph()
{
    for ( int i=0; i<PATTERN_HASH_SIZE; i++ ) {
        MyTexPattern * p = table[i];
        while (p) {
            MyTexPattern * tmp = p;
            p = p->next;
            delete tmp;
        }
    }
}

bool MyTexHyph::match( const lChar32 * str, char * mask )
{
    bool found = false;
    MyTexPattern * res = table[ MyTexPattern::hash( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    res = table[ MyTexPattern::hash3( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    res = table[ MyTexPattern::hash2( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    res = table[ MyTexPattern::hash1( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    return found;
}

bool MyTexHyph::hyphenate( const lChar32 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
{
    // stub
    return false;
}

void MyTexHyph::addPattern( MyTexPattern * pattern )
{
    int h = pattern->hash();
    MyTexPattern * * p = &table[h];
    while ( *p && pattern->cmp(*p)<0 )
        p = &((*p)->next);
    pattern->next = *p;
    *p = pattern;
    _pattern_count++;
}

bool MyTexHyph::load( LVStreamRef stream )
{
    int w = isCorrectHyphFile(stream.get());
    int patternCount = 0;
    if (w) {
        _hash = stream->getcrc32();
        int        i;
        lvsize_t   dw;

        lvByteOrderConv cnv;

        int hyph_count = w;
        thyph hyph;

        lvpos_t p = 78 + (hyph_count * 8 + 2);
        stream->SetPos(p);
        if ( stream->SetPos(p)!=p )
            return false;
        lChar32 charMap[256] = { 0 };
        unsigned char buf[0x10000];
        // make char map table
        for (i=0; i<hyph_count; i++)
        {
            if ( stream->Read( &hyph, 522, &dw )!=LVERR_OK || dw!=522 )
                return false;
            cnv.msf( &hyph.len ); //rword(_main_hyph[i].len);
            lvpos_t newPos;
            if ( stream->Seek( hyph.len, LVSEEK_CUR, &newPos )!=LVERR_OK )
                return false;

            cnv.msf( hyph.wl );
            cnv.msf( hyph.wu );
            charMap[ (unsigned char)hyph.al ] = hyph.wl;
            charMap[ (unsigned char)hyph.au ] = hyph.wu;
//            lChar32 ch = hyph.wl;
//            CRLog::debug("wl=%s mask=%c%c", LCSTR(lString32(&ch, 1)), hyph.mask0[0], hyph.mask0[1]);
            if (hyph.mask0[0]!='0'||hyph.mask0[1]!='0') {
                unsigned char pat[4];
                pat[0] = hyph.al;
                pat[1] = hyph.mask0[0];
                pat[2] = hyph.mask0[1];
                pat[3] = 0;
                MyTexPattern * pattern = new MyTexPattern(pat, 1, charMap);
#if DUMP_PATTERNS==1
                CRLog::debug("Pattern: '%s' - %s", LCSTR(lString32(pattern->word)), pattern->attr );
#endif
                if (pattern->overflowed) {
                    // don't use truncated words
                    CRLog::warn("Pattern overflowed (%d > %d) and ignored: '%s'", pattern->overflowed, MAX_PATTERN_SIZE, LCSTR(lString32(pattern->word)));
                    if (pattern->overflowed > largest_overflowed_word)
                        largest_overflowed_word = pattern->overflowed;
                    delete pattern;
                }
                else {
                    addPattern( pattern );
                    patternCount++;
                }
            }
        }

        if ( stream->SetPos(p)!=p )
            return false;

        for (i=0; i<hyph_count; i++)
        {
            stream->Read( &hyph, 522, &dw );
            if (dw!=522)
                return false;
            cnv.msf( &hyph.len );

            stream->Read(buf, hyph.len, &dw);
            if (dw!=hyph.len)
                return false;

            unsigned char * p = buf;
            unsigned char * end_p = p + hyph.len;
            while ( p < end_p ) {
                lUInt8 sz = *p++;
                if ( p + sz > end_p )
                    break;
                MyTexPattern * pattern = new MyTexPattern( p, sz, charMap );
#if DUMP_PATTERNS==1
                CRLog::debug("Pattern: '%s' - %s", LCSTR(lString32(pattern->word)), pattern->attr);
#endif
                if (pattern->overflowed) {
                    // don't use truncated words
                    CRLog::warn("Pattern overflowed (%d > %d) and ignored: '%s'", pattern->overflowed, MAX_PATTERN_SIZE, LCSTR(lString32(pattern->word)));
                    if (pattern->overflowed > largest_overflowed_word)
                        largest_overflowed_word = pattern->overflowed;
                    delete pattern;
                }
                else {
                    addPattern( pattern );
                    patternCount++;
                }
                p += sz + sz + 1;
            }
        }

#if DUMP_PATTERNS==1
        CRLog::debug("Patterns count = %d", patternCount);
#endif
        return patternCount>0;
    } else {
        // tex xml format as for FBReader
        lString32Collection data;
        MyHyphPatternReader reader( data );
        LVXMLParser parser( stream, &reader );
        if ( !parser.CheckFormat() )
            return false;
        if ( !parser.Parse() )
            return false;
        if ( !data.length() )
            return false;
        for ( int i=0; i<(int)data.length(); i++ ) {
            data[i].lowercase();
            MyTexPattern * pattern = new MyTexPattern( data[i] );
#if DUMP_PATTERNS==1
            CRLog::debug("Pattern: (%s) '%s' - %s", LCSTR(data[i]), LCSTR(lString32(pattern->word)), pattern->attr);
#endif
            if (pattern->overflowed) {
                // don't use truncated words
                CRLog::warn("Pattern overflowed (%d > %d) and ignored: (%s) '%s'", pattern->overflowed, MAX_PATTERN_SIZE, LCSTR(data[i]), LCSTR(lString32(pattern->word)));
                if (pattern->overflowed > largest_overflowed_word)
                    largest_overflowed_word = pattern->overflowed;
                delete pattern;
            }
            else {
                addPattern( pattern );
                patternCount++;
            }
        }
        return patternCount>0;
    }
}

bool MyTexHyph::load( lString32 fileName )
{
    LVStreamRef stream = LVOpenFileStream( fileName.c_str(), LVOM_READ );
    if ( stream.isNull() )
        return false;
    return load( stream );
}

lUInt32 MyTexHyph::getSize()
{
    return _pattern_count * sizeof(MyTexPattern);
}


bool MyTexHyph::dump(LVStreamRef stream, const lString8& title)
{
    if (stream.isNull()) {
        CRLog::error("Output stream is null!");
        return false;
    }
    CRLog::info("Dictionary contains %d patterns.", _pattern_count);

    lString32Collection strPatterns;
    MyTexPattern* pattern;
    for (lUInt32 i = 0; i < PATTERN_HASH_SIZE; i++) {
        pattern = table[i];
        while (pattern != 0) {
            //CRLog::debug("Pattern: '%s' - %s", LCSTR(lString32(pattern->word)), pattern->attr);
            lString32 str;
            int k = 0;
            for (int j = 0; pattern->attr[j] && j < MAX_PATTERN_SIZE + 2 && k < MAX_PATTERN_SIZE + 1; j++) {
                if (pattern->attr[j] > '0' && pattern->attr[j] <= '9') {
                    str << pattern->attr[j];
                    if (pattern->word[k])
                        str << pattern->word[k++];
                } else {
                    if (pattern->word[k])
                        str << pattern->word[k++];
                }
            }
            strPatterns.add(str);
            pattern = pattern->next;
        }
    }
    if (strPatterns.length() != _pattern_count) {
        CRLog::error("Not all patterns processed!");
        return false;
    }
    // sort
    strPatterns.sort();
    // Write to stream
    lString8 decStr;
    *stream << "<?xml version=\"1.0\" encoding=\"utf8\"?>\n";
    *stream << "<!-- dump of hyphenation dictionary \"";
    *stream << title << "\"\n";
    *stream << "  pattern's count ";
    decStr.appendDecimal(_pattern_count);
    *stream << decStr;
    *stream << " -->\n";
    *stream << "<HyphenationDescription>\n";
    for (int i = 0; i < strPatterns.length(); i++) {
        *stream << "<pattern>";
        *stream << UnicodeToUtf8(strPatterns[i]);
        *stream << "</pattern>\n";
    }
    *stream << "</HyphenationDescription>\n";
    return true;
}
