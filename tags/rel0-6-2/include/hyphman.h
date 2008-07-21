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
    char*           pattern;    
} thyph;
#pragma pack(pop)

/// AlReader hyphenation manager
class HyphMan
{
    int             _hyph_count;
    thyph*          _main_hyph;
    lUInt32         _w_len;
    unsigned char   _winput[WORD_LENGTH+32+2];
    wchar_t         _wword[WORD_LENGTH];
    unsigned char   _dict[256];
    unsigned char   _dict_w[65535];
    unsigned char   _wresult[WORD_LENGTH+32+2];

    void  prepareInput();
    void  prepareResult();
    //int             _hyph;
    static HyphMan * _instance;
    bool  open(LVStream * stream);
    void  close();
    void  hyphenate(wchar_t* word4hyph);
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
