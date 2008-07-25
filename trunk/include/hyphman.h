/** \file hyphman.h
    \brief AlReader hyphenation manager

    (c) Alan, http://alreader.kms.ru/

    Adapted for CREngine by Vadim Lopatin

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef _HYPHEN_
#define _HYPHEN_

#include "lvtypes.h"
#include "lvstream.h"

#define WORD_LENGTH   64
#define MAX_REAL_WORD 24

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

class HyphIndex {
    private:
        unsigned char mask0[2];    // mask for first 2 characters
        unsigned char * pattern;   // full patterns list for 2 characters
        unsigned char ** index;    // pointers to each pattern start
        int size;                  // count of patterns in index
        hyph_index_item_t pos[256]; // ranges of second char in index
    public:
        // checks pattern match and applies mask if found
        void apply( unsigned char * word, int word_len, unsigned char * result  );
        HyphIndex ( thyph * hyph, unsigned char * hyph_pattern );
        ~HyphIndex();
};

/// AlReader hyphenation manager
class HyphMan
{
    //int             _hyph;
    unsigned char * _wtoa_index[256];
    HyphIndex * _hyph_index[256];
    static HyphMan * _instance;
    bool  open(LVStream * stream);
    void  close();

    void  hyphenateNew( const lChar16 * word4hyph, int word_size, unsigned char * dest_mask );

    void  mapChar( lUInt16 wc, unsigned char c );
public:
    static int isCorrectHyphFile(LVStream * stream);
    static bool hyphenate( 
        const lChar16 * str, 
        int len, 
        lChar8 * flags );
    static HyphMan * GetInstance() { return _instance; }
    static bool Open(LVStream * stream);
    static void Close()
    {
        if (_instance)
            delete _instance;
        _instance= NULL;
    }
    HyphMan();
    ~HyphMan();
    static bool hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth );
};



#endif
