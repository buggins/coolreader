/** \file hyphman.cpp
    \brief AlReader hyphenation manager

    (c) Alan, adapted TeX hyphenation dictionaries code: http://alreader.kms.ru/
    (c) Mark Lipsman -- hyphenation algorithm, modified my Mike & SeNS

    Adapted for CREngine by Vadim Lopatin

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

// set to 0 for old hyphenation, 1 for new algorithm
#define NEW_HYPHENATION 1


#include "../include/crsetup.h"

#include <stdlib.h>
#include <string.h>

#if !defined(__SYMBIAN32__)
#include <stdio.h>
#include <wchar.h>
#endif

#include "../include/lvtypes.h"
#include "../include/lvstream.h"
#include "../include/hyphman.h"
#include "../include/lvfnt.h"
#include "../include/lvstring.h"
#include "../include/cri18n.h"

HyphMan * HyphMan::_instance = NULL;

HyphDictionary * HyphMan::_selectedDictionary = NULL;

HyphDictionaryList * HyphMan::_dictList = NULL;

bool HyphMan::_disabled = false;

void HyphMan::uninit()
{
	if ( _dictList )
		delete _dictList;
	_selectedDictionary = NULL;
	Close();
}

bool HyphMan::initDictionaries( lString16 dir )
{
	_dictList = new HyphDictionaryList();
	if ( _dictList->open( dir ) ) {
		if ( !_dictList->activate( lString16(DEF_HYPHENATION_DICT) ) )
			_dictList->activate( lString16(HYPH_DICT_ID_ALGORITHM) );
		return true;
	} else {
		_dictList->activate( lString16(HYPH_DICT_ID_ALGORITHM) );
		return false;
	}
}

bool HyphDictionary::activate()
{
	if ( getType() == HDT_ALGORITHM ) {
		CRLog::info("Turn on algorythmic hyphenation" );
		HyphMan::Close();
		HyphMan::_disabled = false;
	} else if ( getType() == HDT_NONE ) {
		CRLog::info("Disabling hyphenation" );
		HyphMan::Close();
		HyphMan::_disabled = true;
	} else if ( getType() == HDT_DICT_ALAN ) {
		CRLog::info("Selecting hyphenation dictionary %s", UnicodeToUtf8(_filename).c_str() );
		LVStreamRef stream = LVOpenFileStream( getFilename().c_str(), LVOM_READ );
		if ( stream.isNull() ) {
			CRLog::error("Cannot open hyphenation dictionary %s", UnicodeToUtf8(_filename).c_str() );
			return false;
		}
		HyphMan::_disabled = false;
		HyphMan::Open( stream.get() );
	}
	HyphMan::_selectedDictionary = this;
	return true;
}

bool HyphDictionaryList::activate( lString16 id )
{
	HyphDictionary * p = find(id); 
	if ( p ) 
		return p->activate(); 
	else 
		return false;
}

void HyphDictionaryList::addDefault()
{
	if ( !find( lString16( HYPH_DICT_ID_NONE ) ) ) {
		_list.add( new HyphDictionary( HDT_NONE, _16("[No Hyphenation]"), lString16(HYPH_DICT_ID_NONE), lString16(HYPH_DICT_ID_NONE) ) );
	}
	if ( !find( lString16( HYPH_DICT_ID_ALGORITHM ) ) ) {
		_list.add( new HyphDictionary( HDT_ALGORITHM, _16("[Algorythmic Hyphenation]"), lString16(HYPH_DICT_ID_ALGORITHM), lString16(HYPH_DICT_ID_ALGORITHM) ) );
	}
		
}

HyphDictionary * HyphDictionaryList::find( lString16 id )
{
	for ( int i=0; i<_list.length(); i++ ) {
		if ( _list[i]->getId() == id )
			return _list[i];
	}
	return NULL;
}

bool HyphDictionaryList::open( lString16 hyphDirectory )
{
	_list.clear();
	addDefault();
	LVAppendPathDelimiter( hyphDirectory );
    LVContainerRef container = LVOpenDirectory( hyphDirectory.c_str(), L"*.pdb" );
	if ( !container.isNull() ) {
		int len = container->GetObjectCount();
		for ( int i=0; i<len; i++ ) {
			const LVContainerItemInfo * item = container->GetObjectInfo( i );
			lString16 name = item->GetName();

			lString16 filename = hyphDirectory + name;
			lString16 id = name;
			lString16 title = name;
			lString16 suffix("_hyphen_(Alan).pdb");
			if ( title.endsWith( suffix ) )
				title.erase( title.length() - suffix.length(), suffix.length() );
			_list.add( new HyphDictionary( HDT_DICT_ALAN, title, id, filename ) );
		}
		return true;
	}
	return false;
}

bool HyphMan::Open(LVStream * stream)
{
    //printf("HyphMan::Open()\n");
    if (_instance)
        _instance->close();
    else
        _instance = new HyphMan();
    lvopen_mode_t om = stream->GetMode();
    if (om!=LVOM_ERROR && _instance->open(stream))
        return true;
    // error
    delete _instance;
    _instance = NULL;
    return false;
}
/*
bool HyphMan::hyphenate( const lChar16 * str, 
        int len, 
        lChar8 * flags )
{
    if (!_instance)
        return false;
    if (len<=3 || len>WORD_LENGTH)
        return false; // too short word
    lChar16 buf[WORD_LENGTH];
    int i;
    for (i=0; i<len; i++)
    {
        if (str[i]==UNICODE_SOFT_HYPHEN_CODE)
            return false; // word already hyphenated
        buf[i] = str[i];
    }
    buf[len] = 0;
    _instance->hyphenate(buf);

    memcpy( flags, _instance->_wresult, len );
    flags[len] = 0;
    return true;
}
*/
bool HyphMan::hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth )
{
    if (len<=3 || len>WORD_LENGTH)
        return false; // too short or too long word

	if ( _disabled )
		return false;

    if ( !_instance ) {
        //=====================================================================
        // hyphenation algorithm by Mark Lipsman, modified my Mike & SeNS & Buggins
/*
        if ( (lGetCharProps(0x42a) != (CH_PROP_UPPER | CH_PROP_ALPHA_SIGN) )
            || (lGetCharProps(0x44c) != (CH_PROP_LOWER | CH_PROP_ALPHA_SIGN) )  
        ) {
            printf( "props unit test failed! %d %d %d %d %d %d\n"
               , (lGetCharProps(0x0BF) == (CH_PROP_PUNCT) )  
               , (lGetCharProps(0x0DF) == (CH_PROP_LOWER | CH_PROP_CONSONANT) )  
               , (lGetCharProps(0x0F7) == (CH_PROP_SIGN) )  
               , (lGetCharProps(0x111) == (CH_PROP_LOWER | CH_PROP_CONSONANT) )  
               , (lGetCharProps(0x130) == (CH_PROP_UPPER | CH_PROP_VOWEL) )  
               , (lGetCharProps(0x14d) == (CH_PROP_LOWER | CH_PROP_VOWEL) )  
             );
        }
*/
        lUInt16 chprops[WORD_LENGTH];
        lStr_getCharProps( str, len, chprops );
        int start, end, i, j;
        #define MIN_WORD_LEN_TO_HYPHEN 2
        for ( start = 0; start<len; ) {
            // find start of word
            while (start<len && !(chprops[start] & CH_PROP_ALPHA) )
                ++start;
            // find end of word
            for ( end=start+1; end<len && (chprops[start] & CH_PROP_ALPHA); ++end )
                ;
            // now look over word, placing hyphens
            if ( end-start > MIN_WORD_LEN_TO_HYPHEN ) { // word must be long enough
                for (i=start;i<end-MIN_WORD_LEN_TO_HYPHEN;++i) {
                    if ( widths[i] > maxWidth )
                        break;
                    if ( chprops[i] & CH_PROP_VOWEL ) {
                        for ( j=i+1; j<end; ++j ) {
                            if ( chprops[j] & CH_PROP_VOWEL ) {
                                if ( (chprops[i+1] & CH_PROP_CONSONANT) && (chprops[i+2] & CH_PROP_CONSONANT) )
                                    ++i;
                                else if ( (chprops[i+1] & CH_PROP_CONSONANT) && ( chprops[i+2] & CH_PROP_ALPHA_SIGN ) )
                                    i += 2;
                                if ( i-start>=1 && end-i>2 ) {
                                    // insert hyphenation mark
                                    lUInt16 nw = widths[i] + hyphCharWidth;
                                    if ( nw<maxWidth )
                                    {
                                        flags[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
                                        widths[i] = nw;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            start=end;
        }
        return true;
        // end of hyphenation algorithm
        //=====================================================================
    }

#if NEW_HYPHENATION==1
    unsigned char res[WORD_LENGTH+32+2];
    int i;
    for ( i=0; i<len; i++ )
    {
        if (str[i]==UNICODE_SOFT_HYPHEN_CODE)
            return false; // word already hyphenated
    }
    _instance->hyphenateNew(str, len, res);
    for (i=0; i<len; i++)
    {
        if (widths[i]>maxWidth)
        {
            i--;
            break;
        }
    }
    for (; i>=1; i--)
    {
        if (i<len-2 && res[i]=='-' && !(flags[i]&LCHAR_ALLOW_WRAP_AFTER))
        {
            lUInt16 nw = widths[i] += hyphCharWidth;
            if (nw<maxWidth)
            {
                flags[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
                widths[i] = nw;
                break;
            }
        }
    }
#else
    lChar16 buf[WORD_LENGTH];
    int i;
    for (i=0; i<len; i++)
    {
        if (str[i]==UNICODE_SOFT_HYPHEN_CODE)
            return false; // word already hyphenated
        buf[i] = str[i];
    }
    buf[len] = 0;
    _instance->hyphenate(buf);
    unsigned char * res = _instance->_wresult;
    for (i=0; i<len; i++)
    {
        if (widths[i]>maxWidth)
        {
            i--;
            break;
        }
    }
    for (; i>=1; i--)
    {
        if (i<len-2 && res[i]=='-' && !(flags[i]&LCHAR_ALLOW_WRAP_AFTER))
        {
            lUInt16 nw = widths[i] += hyphCharWidth;
            if (nw<maxWidth)
            {
                flags[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
                widths[i] = nw;
                break;
            }
        }
    }
#endif
    return true;
}

void HyphMan::mapChar( lUInt16 wc, unsigned char c )
{
    int high = (wc >> 8) & 255;
    int low = (wc) & 255;
    if ( !_wtoa_index[high] ) {
        _wtoa_index[high] = new unsigned char[ 256 ];
        memset( _wtoa_index[high], 0, sizeof(unsigned char) * 256 );
    }
    _wtoa_index[high][low] = c;
}

HyphMan::HyphMan()
{
    memset( _wtoa_index, 0, sizeof(unsigned char *) * 256 );
    memset( _hyph_index, 0, sizeof(HyphIndex*) * 256);
}

HyphMan::~HyphMan()
{
    close();
}

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

/*
inline lUInt16 rword( lUInt16 n )
{
    return ((n&255)<<8)|((n>>8)&255);
}

static void rwords( lUInt16 * buf, int len )
{
    for ( int i=0; i<len; i++ ) {
        buf[i] = ((buf[i]&255)<<8)|((buf[i]>>8)&255);
    }
}
*/

int HyphMan::isCorrectHyphFile(LVStream * stream)
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

    //for (int i=0; i<8; i++)
    //    if (HDR.type[i]!="HypHAlR4"[i])
    //        w = 0;
    if (strncmp((const char*)&HDR.type, "HypHAlR4", 8)) 
        w = 0;
        
    return w;
}

//        unsigned char mask0[2];    // mask for first 2 characters
//        unsigned char * pattern;   // full patterns list for 2 characters
//        unsigned char ** index;    // pointers to each pattern start
//        int size;                  // count of patterns in index
//        hyph_index_item_t pos[256]; // ranges of second char in index
HyphIndex::HyphIndex ( thyph * hyph, unsigned char * hyph_pattern )
{
    index = NULL;
    pattern = NULL;
    mask0[0] = hyph->mask0[0];
    mask0[1] = hyph->mask0[1];
    memset( pos, 0, sizeof(pos) );
    int len = hyph->len;
    pattern = hyph_pattern;
    unsigned char * p = pattern;
    unsigned char * end_p = p + len;
    size = 0;
    while ( p < end_p ) {
        lUInt8 sz = *p++;
        p += sz + sz + 1;
        size++;
    }
    index = new unsigned char * [size];
    p=pattern;
    for ( int k = 0; k<size; k++ ) {
        index[k] = p;
        lUInt8 sz = *p++;
        unsigned char ch2 = p[1];
        if ( pos[ch2].len++ == 0 )
            pos[ch2].start = k;
        p += sz + sz + 1;
    }
}

HyphIndex::~HyphIndex()
{
    if ( index )
        delete[] index;
    if ( pattern )
        delete[] pattern;
}

inline int compare_words( unsigned char * p, unsigned char * word, int word_len )
{
    int sz = (int)(*p++);
    int res = 0;
    for ( int i=2; i<sz; i++ ) {
        if ( i >= word_len ) {
                // word is shorter than pattern
            return -1;
        }
        if ( word[i] > p[i] ) {
            return 1;
        } else if ( word[i] < p[i] ) {
            return -1;
        }
    }
    return res;
}

// binary search
void HyphIndex::apply( unsigned char * word, int word_len, unsigned char * result )
{
    unsigned char ch2 = word[1];
    if ( pos[ch2].len == 0 )
        return;
    int a = pos[ch2].start;
    int b = a + pos[ch2].len;
    if ( result[0] < mask0[0] )
        result[0] = mask0[0];
    if ( result[1] < mask0[1] )
        result[1] = mask0[1];
    while ( a < b ) {
        int c = (a + b) / 2;
        int res = compare_words( index[c], word, word_len );
        if ( res > 0 ) {
            // go down
            a  = c + 1;
        } else if ( res < 0 ) {
            // go up
            b = c;
        } else {
            // matched
            // check bounds
            int first = c;
            int last = c;
            int j;
            for ( j = first-1; j>=a; j-- ) {
                res = compare_words( index[j], word, word_len );
                if ( !res )
                    first = j;
                else
                    break;
            }
            for ( j = last+1; j<b; j++ ) {
                res = compare_words( index[j], word, word_len );
                if ( !res )
                    last = j;
                else
                    break;
            }
            // apply all patterns in equal range
            for ( j=first; j<=last; j++ ) {
                unsigned char * p = index[j];
                int sz = (int)(*p++);
                p += sz;
                // apply pattern
                for ( int i=-1; i<sz; i++, p++ ) {
                    if ( result[i] < *p )
                        result[i] = *p;
                }
            }
            return;
        }
    }
}

bool HyphMan::open( LVStream * stream )
{
    int        i;
    lvsize_t   dw;

    lvByteOrderConv cnv;

    //printf("HyphMan::open()\n");

    close();

    int w = isCorrectHyphFile(stream);
    if (!w)
        return false;

    int hyph_count = w;
    thyph hyph;

    lvpos_t p = 78 + (hyph_count * 8 + 2);
    stream->SetPos(p);
    if (stream->GetPos()!=p)
        goto no_valid;

    for (i=0; i<hyph_count; i++)
    {
        stream->Read( &hyph, 522, &dw );
        if (dw!=522) 
            goto no_valid;
        cnv.msf( &hyph.len ); //rword(_main_hyph[i].len);

        cnv.msf( hyph.aux, 256 ); //rwords(_main_hyph[i].aux, 256);

        unsigned char * pattern = new unsigned char [hyph.len + 1];
        stream->Read(pattern, hyph.len, &dw); 
        pattern[hyph.len] = 0x00;
        if (dw!=hyph.len)
            goto no_valid;

        unsigned char ch = hyph.al;
        mapChar( hyph.wl, ch );
        mapChar( hyph.wu, ch );
        _hyph_index[ ch ] = new HyphIndex( &hyph, pattern );
    }

    return true;
no_valid:
    HyphMan::close();
    return false;
}

void HyphMan::close()
{
    int i;
    for ( i=0; i<256; i++ )
        if ( _wtoa_index[i] )
            delete[] _wtoa_index[i];
    memset( _wtoa_index, 0, sizeof(unsigned char *) * 256 );
    for ( i=0; i<256; i++ )
        if ( _hyph_index[i] )
            delete _hyph_index[i];
    memset( _hyph_index, 0, sizeof(HyphIndex*) * 256);
}

void  HyphMan::hyphenateNew( const lChar16 * word4hyph, int word_size, unsigned char * dest_mask )
{
    unsigned char aword[ WORD_LENGTH+32+2 ];
    unsigned char result[ WORD_LENGTH+32+2 ];
    // convert wide chars to 8 bit
    if ( word_size > MAX_REAL_WORD )
        word_size = MAX_REAL_WORD;
    aword[0] = ' ';
    int i;
    for ( i=0; i<word_size; i++ ) {
        lChar16 ch = word4hyph[i];
        lUInt8 high = (ch>>8) & 255;
        lUInt8 low = (ch) & 255;
        unsigned char ach;
        if ( !_wtoa_index[high] ) {
            ach = ' ';
        } else {
            ach = _wtoa_index[high][low];
            if ( !ach )
                ach = ' ';
        }
        aword[i+1] = ach;
    }
    aword[ word_size+1 ] = ' ';
    aword[ word_size+2 ] = 0;
    word_size += 2;
    // initial result value
    memset( result, '0', word_size+1 );
    result[word_size] = 0;
    for ( int len = 2; len <= word_size; len++ ) {
        unsigned char * w = aword + word_size - len;
        HyphIndex * index = _hyph_index[w[0]];
        if ( !index )
            continue;
        index->apply( w, len - 1, result + word_size - len + 1); // - 1
    }
    // copy result
    word_size -= 2;
    memcpy( dest_mask, result+2, word_size * sizeof(unsigned char) );
#if 1
    for (i=0; i<word_size; i++) {
        //switch ( word4hyph[i] ) {
            //case 45:
        //case L'/':
            //case L'>':
            //case L':':
            //case 160:
            //case 61: //=
           // case 8211: //-
            //case 8212: //-
            //case 173: //-
            //    dest_mask[i]='!';
            //    break;
            //default:
                if (dest_mask[i] & 0x01) {
                    dest_mask[i]='-';
                    //break;
                } else 
                    dest_mask[i]='0';
        //}
    }

    dest_mask[0] = '0';
#endif
/*
    for (i=0; i < word_size; i++) {        
        if (word4hyph[i+1] == 0x20) {
            if (dest_mask[i] != '!') {
                dest_mask[i] = '0';
            }
            if (dest_mask[i + 1] != '!') {
                dest_mask[i + 1] = '0';
            }
            if (i - 2 >= 0 && dest_mask[i - 2] != '!') {
                dest_mask[i - 2] = '0';
            }
            if (dest_mask[i - 1] != '!') {
                dest_mask[i - 1] = '0';
            }
        }
    }
*/
    dest_mask[0]='0';
    dest_mask[word_size]=0x00;
    dest_mask[word_size - 1]='0';
    dest_mask[word_size - 2]='0';
}

