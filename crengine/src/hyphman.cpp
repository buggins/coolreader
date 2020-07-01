/** \file hyphman.cpp
    \brief AlReader hyphenation manager

    (c) Alan, adapted TeX hyphenation dictionaries code: http://alreader.kms.ru/
    (c) Mark Lipsman -- hyphenation algorithm, modified my Mike & SeNS

    Adapted for CREngine by Vadim Lopatin

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

// set to 1 for debug dump
#if 0
#define DUMP_HYPHENATION_WORDS 1
#define DUMP_PATTERNS 1
#else
#define DUMP_HYPHENATION_WORDS 0
#define DUMP_PATTERNS 0
#endif

#include "../include/crsetup.h"

#include <stdlib.h>
#include <string.h>
#include "../include/lvxml.h"

#if !defined(__SYMBIAN32__)
#include <stdio.h>
#include <wchar.h>
#endif

#include "../include/lvtypes.h"
#include "../include/lvstream.h"
#include "../include/hyphman.h"
#include "../include/lvfnt.h"
#include "../include/lvstring.h"
#include "../include/lvstring16collection.h"
#include "../include/crlog.h"
#include "../include/textlang.h"


#ifdef ANDROID

#define _16(x) lString16(x)

#else

#include "../include/cri18n.h"

#endif

int HyphMan::_LeftHyphenMin = HYPH_DEFAULT_HYPHEN_MIN;
int HyphMan::_RightHyphenMin = HYPH_DEFAULT_HYPHEN_MIN;
int HyphMan::_TrustSoftHyphens = HYPH_DEFAULT_TRUST_SOFT_HYPHENS;
LVHashTable<lString16, HyphMethod*> HyphMan::_loaded_hyph_methods(16);

// Obsolete: now fetched from TextLangMan main lang TextLangCfg
// HyphDictionary * HyphMan::_selectedDictionary = NULL;

HyphDictionaryList * HyphMan::_dictList = NULL;

// MAX_PATTERN_SIZE is actually the max size of a word (pattern stripped
// from all the numbers that give the quality of a split after previous char)
// (35 is needed for German.pattern)
#define MAX_PATTERN_SIZE  35
#define PATTERN_HASH_SIZE 16384
class TexPattern;
class TexHyph : public HyphMethod
{
    TexPattern * table[PATTERN_HASH_SIZE];
    lUInt32 _hash;
    lUInt32 _pattern_count;
public:
    int largest_overflowed_word;
    bool match( const lChar16 * str, char * mask );
    virtual bool hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize );
    void addPattern( TexPattern * pattern );
    TexHyph( lString16 id=HYPH_DICT_ID_DICTIONARY, int leftHyphenMin=HYPHMETHOD_DEFAULT_HYPHEN_MIN, int rightHyphenMin=HYPHMETHOD_DEFAULT_HYPHEN_MIN );
    virtual ~TexHyph();
    bool load( LVStreamRef stream );
    bool load( lString16 fileName );
    virtual lUInt32 getHash() { return _hash; }
    virtual lUInt32 getCount() { return _pattern_count; }
    virtual lUInt32 getSize();
};

class AlgoHyph : public HyphMethod
{
public:
    AlgoHyph(): HyphMethod(HYPH_DICT_ID_ALGORITHM) {};
    virtual bool hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize );
    virtual ~AlgoHyph();
};

class SoftHyphensHyph : public HyphMethod
{
public:
    SoftHyphensHyph(): HyphMethod(HYPH_DICT_ID_SOFTHYPHENS) {};
    virtual bool hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize );
    virtual ~SoftHyphensHyph();
};

class NoHyph : public HyphMethod
{
public:
    NoHyph(): HyphMethod(HYPH_DICT_ID_NONE) {};
    virtual bool hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
    {
        CR_UNUSED6(str, len, widths, flags, hyphCharWidth, maxWidth);
        return false;
    }
    virtual ~NoHyph() { }
};

static NoHyph NO_HYPH;
static AlgoHyph ALGO_HYPH;
static SoftHyphensHyph SOFTHYPHENS_HYPH;

// Obsolete: provided by TextLangMan main lang
// HyphMethod * HyphMan::_method = &NO_HYPH;

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

void HyphMan::uninit()
{
    // Avoid existing frontend code to have to call it:
    TextLangMan::uninit();
    // Clean up _loaded_hyph_methods
    LVHashTable<lString16, HyphMethod*>::iterator it = _loaded_hyph_methods.forwardIterator();
    LVHashTable<lString16, HyphMethod*>::pair* pair;
    while ((pair = it.next())) {
        delete pair->value;
    }
    _loaded_hyph_methods.clear();
    if ( _dictList )
            delete _dictList;
    _dictList = NULL;
    /* Obsolete:
	_selectedDictionary = NULL;
    if ( HyphMan::_method != &ALGO_HYPH && HyphMan::_method != &NO_HYPH && HyphMan::_method != &SOFTHYPHENS_HYPH )
            delete HyphMan::_method;
    _method = &NO_HYPH;
    */
}

bool HyphMan::activateDictionaryFromStream( LVStreamRef stream )
{
    if ( stream.isNull() )
        return false;
    /* Obsolete:
    CRLog::trace("remove old hyphenation method");
    if ( HyphMan::_method != &NO_HYPH && HyphMan::_method != &ALGO_HYPH && HyphMan::_method != &SOFTHYPHENS_HYPH && HyphMan::_method ) {
        delete HyphMan::_method;
        HyphMan::_method = &NO_HYPH;
    }
    */
    CRLog::trace("creating new TexHyph method");
    TexHyph * method = new TexHyph(HYPH_DICT_ID_DICTIONARY);
    CRLog::trace("loading from file");
    if ( !method->load( stream ) ) {
		CRLog::error("HyphMan::activateDictionaryFromStream: Cannot open hyphenation dictionary from stream" );
        delete method;
        return false;
    }
    if (method->largest_overflowed_word)
        CRLog::warn("hyph dict from stream: some hyphenation patterns were too long and have been ignored: increase MAX_PATTERN_SIZE from %d to %d\n", MAX_PATTERN_SIZE, method->largest_overflowed_word);
    CRLog::debug("Dictionary is loaded successfully. Activating.");

    // Replace any previously dict loaded from stream
    HyphMethod * prev_method;
    if ( _loaded_hyph_methods.get(HYPH_DICT_ID_DICTIONARY, prev_method) ) {
        delete prev_method;
        _loaded_hyph_methods.remove(HYPH_DICT_ID_DICTIONARY);
    }
    _loaded_hyph_methods.set(HYPH_DICT_ID_DICTIONARY, method);

    if (!_dictList)
        _dictList = new HyphDictionaryList();
    /* Obsolete:
    HyphMan::_method = method;
    */
    if ( HyphMan::_dictList->find(lString16(HYPH_DICT_ID_DICTIONARY))==NULL ) {
        HyphDictionary * dict = new HyphDictionary( HDT_DICT_ALAN, cs16("Dictionary"), lString16(HYPH_DICT_ID_DICTIONARY), lString16::empty_str );
        HyphMan::_dictList->add(dict);
        /* Obsolete:
    	HyphMan::_selectedDictionary = dict;
        */
    }
    TextLangMan::setMainLangFromHyphDict( HYPH_DICT_ID_DICTIONARY );
    CRLog::trace("Activation is done");
    return true;
}

bool HyphMan::initDictionaries(lString16 dir, bool clear)
{
    if (clear && _dictList)
        delete _dictList;
    if (clear || !_dictList)
        _dictList = new HyphDictionaryList();
    if (_dictList->open(dir, clear)) {
		if ( !_dictList->activate( lString16(DEF_HYPHENATION_DICT) ) )
    			_dictList->activate( lString16(HYPH_DICT_ID_ALGORITHM) );
		return true;
	} else {
		_dictList->activate( lString16(HYPH_DICT_ID_ALGORITHM) );
		return false;
	}
}

bool HyphMan::setLeftHyphenMin( int left_hyphen_min ) {
    if (left_hyphen_min >= HYPH_MIN_HYPHEN_MIN && left_hyphen_min <= HYPH_MAX_HYPHEN_MIN) {
        HyphMan::_LeftHyphenMin = left_hyphen_min;
        return true;
    }
    return false;
}

bool HyphMan::setRightHyphenMin( int right_hyphen_min ) {
    if (right_hyphen_min >= HYPH_MIN_HYPHEN_MIN && right_hyphen_min <= HYPH_MAX_HYPHEN_MIN) {
        HyphMan::_RightHyphenMin = right_hyphen_min;
        return true;
    }
    return false;
}

bool HyphMan::setTrustSoftHyphens( int trust_soft_hyphens ) {
    HyphMan::_TrustSoftHyphens = trust_soft_hyphens;
    return true;
}

bool HyphMan::isEnabled() {
    return TextLangMan::getHyphenationEnabled();
    /* Obsolete:
    return _selectedDictionary != NULL && _selectedDictionary->getId() != HYPH_DICT_ID_NONE;
    */
}

bool HyphMan::hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
{
    return TextLangMan::getMainLangHyphMethod()->hyphenate( str, len, widths, flags, hyphCharWidth, maxWidth, flagSize );
    /* Obsolete:
    return _method->hyphenate( str, len, widths, flags, hyphCharWidth, maxWidth, flagSize );
    */
}

HyphDictionary * HyphMan::getSelectedDictionary() {
    lString16 id = TextLangMan::getTextLangCfg()->getHyphMethod()->getId();
    HyphDictionary * dict = _dictList->find( id );
    return dict;
}

HyphMethod * HyphMan::getHyphMethodForDictionary( lString16 id, int leftHyphenMin, int rightHyphenMin ) {
    if ( id.empty() )
        return &NO_HYPH;
    HyphDictionary * p = _dictList->find(id);
    if ( !p || p->getType() == HDT_NONE )
        return &NO_HYPH;
    if ( p->getType() == HDT_ALGORITHM )
        return &ALGO_HYPH;
    if ( p->getType() == HDT_SOFTHYPHENS )
        return &SOFTHYPHENS_HYPH;
    if ( p->getType() != HDT_DICT_ALAN && p->getType() != HDT_DICT_TEX )
        return &NO_HYPH;
    HyphMethod * method;
    if ( _loaded_hyph_methods.get(id, method) ) {
        // printf("getHyphMethodForDictionary reusing cached %s\n", UnicodeToUtf8(p->getFilename()).c_str());
        return method;
    }
    lString16 filename = p->getFilename();
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
        CRLog::error("Cannot open hyphenation dictionary %s", UnicodeToUtf8(filename).c_str() );
        return &NO_HYPH;
    }
    TexHyph * newmethod = new TexHyph(id, leftHyphenMin, rightHyphenMin);
    if ( !newmethod->load( stream ) ) {
        CRLog::error("Cannot open hyphenation dictionary %s", UnicodeToUtf8(filename).c_str() );
        delete newmethod;
        return &NO_HYPH;
    }
    // printf("CRE: loaded hyphenation dict %s\n", UnicodeToUtf8(id).c_str());
    if ( newmethod->largest_overflowed_word )
        CRLog::warn("%s: some hyphenation patterns were too long and have been ignored: increase MAX_PATTERN_SIZE from %d to %d\n", UnicodeToUtf8(filename).c_str(), MAX_PATTERN_SIZE, newmethod->largest_overflowed_word);
    _loaded_hyph_methods.set(id, newmethod);
    return newmethod;
}

bool HyphDictionary::activate()
{
    TextLangMan::setMainLangFromHyphDict( getId() );
    return true;
    /* Obsolete:
    if (HyphMan::_selectedDictionary == this)
        return true; // already active
	if ( getType() == HDT_ALGORITHM ) {
		CRLog::info("Turn on algorythmic hyphenation" );
        if ( HyphMan::_method != &ALGO_HYPH ) {
            if ( HyphMan::_method != &SOFTHYPHENS_HYPH && HyphMan::_method != &NO_HYPH )
                delete HyphMan::_method;
            HyphMan::_method = &ALGO_HYPH;
        }
	} else if ( getType() == HDT_SOFTHYPHENS ) {
		CRLog::info("Turn on soft-hyphens hyphenation" );
        if ( HyphMan::_method != &SOFTHYPHENS_HYPH ) {
            if ( HyphMan::_method != &ALGO_HYPH && HyphMan::_method != &NO_HYPH )
                delete HyphMan::_method;
            HyphMan::_method = &SOFTHYPHENS_HYPH;
        }
	} else if ( getType() == HDT_NONE ) {
		CRLog::info("Disabling hyphenation" );
        if ( HyphMan::_method != &NO_HYPH ) {
            if ( HyphMan::_method != &ALGO_HYPH && HyphMan::_method != &SOFTHYPHENS_HYPH )
                delete HyphMan::_method;
            HyphMan::_method = &NO_HYPH;
        }
	} else if ( getType() == HDT_DICT_ALAN || getType() == HDT_DICT_TEX ) {
        if ( HyphMan::_method != &NO_HYPH && HyphMan::_method != &ALGO_HYPH && HyphMan::_method != &SOFTHYPHENS_HYPH ) {
            delete HyphMan::_method;
            HyphMan::_method = &NO_HYPH;
        }
		CRLog::info("Selecting hyphenation dictionary %s", UnicodeToUtf8(_filename).c_str() );
		LVStreamRef stream = LVOpenFileStream( getFilename().c_str(), LVOM_READ );
		if ( stream.isNull() ) {
			CRLog::error("Cannot open hyphenation dictionary %s", UnicodeToUtf8(_filename).c_str() );
			return false;
		}
        TexHyph * method = new TexHyph();
        if ( !method->load( stream ) ) {
			CRLog::error("Cannot open hyphenation dictionary %s", UnicodeToUtf8(_filename).c_str() );
            delete method;
            return false;
        }
        if (method->largest_overflowed_word)
            printf("CRE WARNING: %s: some hyphenation patterns were too long and have been ignored: increase MAX_PATTERN_SIZE from %d to %d\n", UnicodeToUtf8(_filename).c_str(), MAX_PATTERN_SIZE, method->largest_overflowed_word);
        HyphMan::_method = method;
	}
	HyphMan::_selectedDictionary = this;
	return true;
    */
}

bool HyphDictionaryList::activate( lString16 id )
{
    CRLog::trace("HyphDictionaryList::activate(%s)", LCSTR(id));
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
		_list.add( new HyphDictionary( HDT_ALGORITHM, _16("[Algorithmic Hyphenation]"), lString16(HYPH_DICT_ID_ALGORITHM), lString16(HYPH_DICT_ID_ALGORITHM) ) );
	}
	if ( !find( lString16( HYPH_DICT_ID_SOFTHYPHENS ) ) ) {
		_list.add( new HyphDictionary( HDT_SOFTHYPHENS, _16("[Soft-hyphens Hyphenation]"), lString16(HYPH_DICT_ID_SOFTHYPHENS), lString16(HYPH_DICT_ID_SOFTHYPHENS) ) );
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

bool HyphDictionaryList::open(lString16 hyphDirectory, bool clear)
{
    CRLog::info("HyphDictionaryList::open(%s)", LCSTR(hyphDirectory) );
    if (clear) {
        _list.clear();
        addDefault();
    }
    if ( hyphDirectory.empty() )
        return true;
    //LVAppendPathDelimiter( hyphDirectory );
    LVContainerRef container;
    LVStreamRef stream;
    if ( (hyphDirectory.endsWith("/") || hyphDirectory.endsWith("\\")) && LVDirectoryExists(hyphDirectory) ) {
        container = LVOpenDirectory( hyphDirectory.c_str(), L"*.*" );
    } else if ( LVFileExists(hyphDirectory) ) {
        stream = LVOpenFileStream( hyphDirectory.c_str(), LVOM_READ );
        if ( !stream.isNull() )
            container = LVOpenArchieve( stream );
    }

	if ( !container.isNull() ) {
		int len = container->GetObjectCount();
        int count = 0;
        CRLog::info("%d items found in hyph directory", len);
		for ( int i=0; i<len; i++ ) {
			const LVContainerItemInfo * item = container->GetObjectInfo( i );
			lString16 name = item->GetName();
            lString16 suffix;
            HyphDictType t = HDT_NONE;
            if ( name.endsWith("_hyphen_(Alan).pdb") ) {
                suffix = "_hyphen_(Alan).pdb";
                t = HDT_DICT_ALAN;
            } else if ( name.endsWith(".pattern") ) {
                suffix = ".pattern";
                t = HDT_DICT_TEX;
            } else
                continue;



			lString16 filename = hyphDirectory + name;
			lString16 id = name;
			lString16 title = name;
			if ( title.endsWith( suffix ) )
				title.erase( title.length() - suffix.length(), suffix.length() );

            CRLog::error("adding: id=%s, title=%s", UnicodeToUtf8(id).c_str(), UnicodeToUtf8(title).c_str());

			_list.add( new HyphDictionary( t, title, id, filename ) );
            count++;
		}
		CRLog::info("%d dictionaries added to list", _list.length());
		return true;
	} else {
        CRLog::info("no hyphenation dictionary items found in hyph directory %s", LCSTR(hyphDirectory));
	}
	return false;
}

HyphMan::HyphMan()
{
}

HyphMan::~HyphMan()
{
}

// Used by SoftHyphensHyph::hyphenate(), but also possibly (when
// TrustSoftHyphens is true) as a first step by TexHyph::hyphenate()
// and AlgoHyph::hyphenate(): if soft hyphens are found in the
// provided word, trust and use them; don't do the regular patterns
// and algorithm matching.
static bool softhyphens_hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
{
    bool soft_hyphens_found = false;
    for ( int i = 0; i<len; i++ ) {
        if ( widths[i] + hyphCharWidth > maxWidth )
            break;
        if ( str[i] == UNICODE_SOFT_HYPHEN_CODE ) {
            if ( flagSize == 2 ) {
                lUInt16* flags16 = (lUInt16*) flags;
                flags16[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
            }
            else {
                flags[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
            }
            soft_hyphens_found = true;
        }
    }
    return soft_hyphens_found;
}

bool SoftHyphensHyph::hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
{
    return softhyphens_hyphenate(str, len, widths, flags, hyphCharWidth, maxWidth, flagSize);
}

SoftHyphensHyph::~SoftHyphensHyph()
{
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

class TexPattern {
public:
    lChar16 word[MAX_PATTERN_SIZE+1];
    char attr[MAX_PATTERN_SIZE+2];
    int overflowed; // 0, or size of complete word if larger than MAX_PATTERN_SIZE
    TexPattern * next;

    int cmp( TexPattern * v )
    {
        return lStr_cmp( word, v->word );
    }

    static int hash( const lChar16 * s )
    {
        return ((lUInt32)(((s[0] *31 + s[1])*31 + s[2]) * 31 + s[3])) % PATTERN_HASH_SIZE;
    }

    static int hash3( const lChar16 * s )
    {
        return ((lUInt32)(((s[0] *31 + s[1])*31 + s[2]) * 31 + 0)) % PATTERN_HASH_SIZE;
    }

    static int hash2( const lChar16 * s )
    {
        return ((lUInt32)(((s[0] *31 + s[1])*31 + 0) * 31 + 0)) % PATTERN_HASH_SIZE;
    }

    static int hash1( const lChar16 * s )
    {
        return ((lUInt32)(((s[0] *31 + 0)*31 + 0) * 31 + 0)) % PATTERN_HASH_SIZE;
    }

    int hash()
    {
        return ((lUInt32)(((word[0] *31 + word[1])*31 + word[2]) * 31 + word[3])) % PATTERN_HASH_SIZE;
    }

    bool match( const lChar16 * s, char * mask )
    {
        TexPattern * p = this;
        bool found = false;
        while ( p ) {
            bool res = true;
            for ( int i=2; p->word[i]; i++ )
                if ( p->word[i]!=s[i] ) {
                    res = false;
                    break;
                }
            if ( res ) {
                if ( p->word[0]==s[0] && (p->word[1]==0 || p->word[1]==s[1]) ) {
#if DUMP_PATTERNS==1
                    CRLog::debug("Pattern matched: %s %s on %s %s", LCSTR(lString16(p->word)), p->attr, LCSTR(lString16(s)), mask);
#endif
                    p->apply(mask);
                    found = true;
                }
            }
            p = p->next;
        }
        return found;
    }

    void apply( char * mask )
    {
        ;
        for ( char * p = attr; *p && *mask; p++, mask++ ) {
            if ( *mask < *p )
                *mask = *p;
        }
    }

    TexPattern( const lString16 &s ) : next( NULL )
    {
        overflowed = 0;
        memset( word, 0, sizeof(word) );
        memset( attr, '0', sizeof(attr) );
        attr[sizeof(attr)-1] = 0;
        int n = 0;
        for ( int i=0; i<(int)s.length(); i++ ) {
            lChar16 ch = s[i];
            if (n > MAX_PATTERN_SIZE) {
                if ( ch<'0' || ch>'9' ) {
                    overflowed = n++;
                }
                continue;
            }
            if ( ch>='0' && ch<='9' ) {
                attr[n] = (char)ch;
//                if (n>0)
//                    attr[n-1] = (char)ch;
            } else {
                if (n == MAX_PATTERN_SIZE) { // we previously reached max word size
                    // Let the last 0 (string termination) in
                    // word[MAX_PATTERN_SIZE] and mark it as overflowed
                    overflowed = n++;
                }
                else {
                    word[n++] = ch;
                }
            }
        }
        // if n==MAX_PATTERN_SIZE (or >), attr[MAX_PATTERN_SIZE] is either the
        // memset '0', or a 0-9 we got on next iteration, and
        // attr[MAX_PATTERN_SIZE+1] is the 0 set by attr[sizeof(attr)-1] = 0
        if (n < MAX_PATTERN_SIZE)
            attr[n+1] = 0;

        if (overflowed)
            overflowed = overflowed + 1; // convert counter to number of things counted
    }

    TexPattern( const unsigned char * s, int sz, const lChar16 * charMap )
    {
        overflowed = 0;
        if ( sz > MAX_PATTERN_SIZE ) {
            overflowed = sz;
            sz = MAX_PATTERN_SIZE;
        }
        memset( word, 0, sizeof(word) );
        memset( attr, 0, sizeof(attr) );
        for ( int i=0; i<sz; i++ )
            word[i] = charMap[ s[i] ];
        memcpy( attr, s+sz, sz+1 );
    }
};

class HyphPatternReader : public LVXMLParserCallback
{
protected:
    bool insidePatternTag;
    lString16Collection & data;
public:
    HyphPatternReader(lString16Collection & result) : insidePatternTag(false), data(result)
    {
        result.clear();
    }
    /// called on parsing end
    virtual void OnStop() { }
    /// called on opening tag end
    virtual void OnTagBody() {}
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname)
    {
        CR_UNUSED(nsname);
        if (!lStr_cmp(tagname, "pattern")) {
            insidePatternTag = true;
        }
        return NULL;
    }
    /// called on closing
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
    {
        CR_UNUSED2(nsname, tagname);
        insidePatternTag = false;
    }
    /// called on element attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
    {
        CR_UNUSED3(nsname, attrname, attrvalue);
    }
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 flags )
    {
        CR_UNUSED(flags);
        if ( insidePatternTag )
            data.add( lString16(text, len) );
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString16 name, const lUInt8 * data, int size) {
        CR_UNUSED3(name, data, size);
        return false;
    }

};

TexHyph::TexHyph(lString16 id, int leftHyphenMin, int rightHyphenMin) : HyphMethod(id, leftHyphenMin, rightHyphenMin)
{
    memset( table, 0, sizeof(table) );
    _hash = 123456;
    _pattern_count = 0;
    largest_overflowed_word = 0;
}

TexHyph::~TexHyph()
{
    for ( int i=0; i<PATTERN_HASH_SIZE; i++ ) {
        TexPattern * p = table[i];
        while (p) {
            TexPattern * tmp = p;
            p = p->next;
            delete tmp;
        }
    }
}

void TexHyph::addPattern( TexPattern * pattern )
{
    int h = pattern->hash();
    TexPattern * * p = &table[h];
    while ( *p && pattern->cmp(*p)<0 )
        p = &((*p)->next);
    pattern->next = *p;
    *p = pattern;
    _pattern_count++;
}

lUInt32 TexHyph::getSize() {
    return _pattern_count * sizeof(TexPattern);
}

bool TexHyph::load( LVStreamRef stream )
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
        lChar16 charMap[256] = { 0 };
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
//            lChar16 ch = hyph.wl;
//            CRLog::debug("wl=%s mask=%c%c", LCSTR(lString16(&ch, 1)), hyph.mask0[0], hyph.mask0[1]);
            if (hyph.mask0[0]!='0'||hyph.mask0[1]!='0') {
                unsigned char pat[4];
                pat[0] = hyph.al;
                pat[1] = hyph.mask0[0];
                pat[2] = hyph.mask0[1];
                pat[3] = 0;
                TexPattern * pattern = new TexPattern(pat, 1, charMap);
#if DUMP_PATTERNS==1
                CRLog::debug("Pattern: '%s' - %s", LCSTR(lString16(pattern->word)), pattern->attr );
#endif
                if (pattern->overflowed) {
                    // don't use truncated words
                    CRLog::warn("Pattern overflowed (%d > %d) and ignored: '%s'", pattern->overflowed, MAX_PATTERN_SIZE, LCSTR(lString16(pattern->word)));
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
                TexPattern * pattern = new TexPattern( p, sz, charMap );
#if DUMP_PATTERNS==1
                CRLog::debug("Pattern: '%s' - %s", LCSTR(lString16(pattern->word)), pattern->attr);
#endif
                if (pattern->overflowed) {
                    // don't use truncated words
                    CRLog::warn("Pattern overflowed (%d > %d) and ignored: '%s'", pattern->overflowed, MAX_PATTERN_SIZE, LCSTR(lString16(pattern->word)));
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

        return patternCount>0;
    } else {
        // tex xml format as for FBReader
        lString16Collection data;
        HyphPatternReader reader( data );
        LVXMLParser parser( stream, &reader );
        if ( !parser.CheckFormat() )
            return false;
        if ( !parser.Parse() )
            return false;
        if ( !data.length() )
            return false;
        for ( int i=0; i<(int)data.length(); i++ ) {
            data[i].lowercase();
            TexPattern * pattern = new TexPattern( data[i] );
#if DUMP_PATTERNS==1
            CRLog::debug("Pattern: (%s) '%s' - %s", LCSTR(data[i]), LCSTR(lString16(pattern->word)), pattern->attr);
#endif
            if (pattern->overflowed) {
                // don't use truncated words
                CRLog::warn("Pattern overflowed (%d > %d) and ignored: (%s) '%s'", pattern->overflowed, MAX_PATTERN_SIZE, LCSTR(data[i]), LCSTR(lString16(pattern->word)));
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

bool TexHyph::load( lString16 fileName )
{
    LVStreamRef stream = LVOpenFileStream( fileName.c_str(), LVOM_READ );
    if ( stream.isNull() )
        return false;
    return load( stream );
}


bool TexHyph::match( const lChar16 * str, char * mask )
{
    bool found = false;
    TexPattern * res = table[ TexPattern::hash( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    res = table[ TexPattern::hash3( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    res = table[ TexPattern::hash2( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    res = table[ TexPattern::hash1( str ) ];
    if ( res ) {
        found = res->match( str, mask ) || found;
    }
    return found;
}

//TODO: do we need it?
///// returns false if there is rule disabling hyphenation at specified point
//static bool checkHyphenRules( const lChar16 * str, int len, int pos )
//{
//    if ( pos<1 || pos>len-3 )
//        return false;
//    lUInt16 props[2] = { 0, 0 };
//    lStr_getCharProps( str+pos+1, 1, props);
//    if ( props[0]&CH_PROP_ALPHA_SIGN )
//        return false;
//    if ( pos==len-3 ) {
//        lStr_getCharProps( str+len-2, 2, props);
//        return (props[0]&CH_PROP_VOWEL) || (props[1]&CH_PROP_VOWEL);
//    }
//    if ( pos==1 ) {
//        lStr_getCharProps( str, 2, props);
//        return (props[0]&CH_PROP_VOWEL) || (props[1]&CH_PROP_VOWEL);
//    }
//    return true;
//}

bool TexHyph::hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
{
    if ( HyphMan::_TrustSoftHyphens ) {
        if ( softhyphens_hyphenate(str, len, widths, flags, hyphCharWidth, maxWidth, flagSize) )
            return true;
    }
    if ( len<=3 )
        return false;
    if ( len>=WORD_LENGTH )
        len = WORD_LENGTH - 2;
    lChar16 word[WORD_LENGTH+4] = { 0 };
    char mask[WORD_LENGTH+4] = { 0 };

    // Make word from str, with soft-hyphens stripped out.
    // Prepend and append a space so patterns can match word boundaries.
    int wlen;
    word[0] = ' ';
    int w = 1;
    for ( int i=0; i<len; i++ ) {
        if ( str[i] != UNICODE_SOFT_HYPHEN_CODE ) {
            word[w++] = str[i];
        }
    }
    wlen = w-1;
    word[w++] = ' ';
    if ( wlen<=3 )
        return false;
    lStr_lowercase(word+1, wlen);
    // printf("word:%s => #%s# (%d => %d)\n", LCSTR(lString16(str, len)), LCSTR(lString16(word)), len, wlen);

#if DUMP_HYPHENATION_WORDS==1
    CRLog::trace("word to hyphenate: '%s'", LCSTR(lString16(word)));
#endif

    // Find matches from dict patterns, at any position in word.
    // Places where hyphenation is allowed are put into 'mask'.
    memset( mask, '0', wlen+3 );	// 0x30!
    bool found = false;
    for ( int i=0; i<=wlen; i++ ) {
        found = match( word + i, mask + i ) || found;
    }
    if ( !found )
        return false;

#if DUMP_HYPHENATION_WORDS==1
    lString16 buf;
    lString16 buf2;
    bool boundFound = false;
    for ( int i=0; i<wlen; i++ ) {
        buf << word[i+1];
        buf2 << word[i+1];
        buf2 << (lChar16)mask[i+2];
        // This maxWidth check may be wrong here (in the dump only) because
        // of a +1 shift and possible more shifts due to soft-hyphens.
        int nw = widths[i]+hyphCharWidth;
        if ( (mask[i+2]&1) ) {
            buf << (lChar16)'-';
            buf2 << (lChar16)'-';
        }
        if ( nw>maxWidth && !boundFound ) {
            buf << (lChar16)'|';
            buf2 << (lChar16)'|';
            boundFound = true;
//            buf << (lChar16)'-';
//            buf2 << (lChar16)'-';
        }
    }
    CRLog::trace("Hyphenate: %s  %s", LCSTR(buf), LCSTR(buf2) );
#endif

    // Use HyphMan global left/right hyphen min, unless set to 0 (the default)
    // which means we should use the HyphMethod specific values.
    int left_hyphen_min = HyphMan::_LeftHyphenMin ? HyphMan::_LeftHyphenMin : _left_hyphen_min;
    int right_hyphen_min = HyphMan::_RightHyphenMin ? HyphMan::_RightHyphenMin : _right_hyphen_min;

    // Moves allowed hyphenation positions from 'mask' to the provided 'flags',
    // taking soft-hyphen shifts into account
    int soft_hyphens_skipped = 0;
    bool res = false;
    for ( int p=0 ; p<=len-2; p++ ) {
        // printf(" char %c\n", str[p]);
        if ( str[p] == UNICODE_SOFT_HYPHEN_CODE ) {
            soft_hyphens_skipped++;
            continue;
        }
        if (p-soft_hyphens_skipped < left_hyphen_min - 1)
            continue;
        if (p > len - right_hyphen_min - 1)
            continue;
        // hyphenate
        //00010030100
        int nw = widths[p]+hyphCharWidth;
        // printf(" word %c\n", word[p+1-soft_hyphens_skipped]);
        // p+2 because: +1 because word has a space prepended, and +1 because
        // mask[] holds the flag for char n on slot n+1
        if ( (mask[p+2-soft_hyphens_skipped]&1) && nw <= maxWidth ) {
            if ( flagSize == 2 ) {
                lUInt16* flags16 = (lUInt16*) flags;
                flags16[p] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
            }
            else {
                flags[p] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
            }
            // printf(" allowed after %c\n", str[p]);
            res = true;
        }
    }
    return res;
}

bool AlgoHyph::hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize )
{
    if ( HyphMan::_TrustSoftHyphens ) {
        if ( softhyphens_hyphenate(str, len, widths, flags, hyphCharWidth, maxWidth, flagSize) )
            return true;
    }

    // Use HyphMan global left/right hyphen min, unless set to 0 (the default)
    // which means we should use the HyphMethod specific values.
    int left_hyphen_min = HyphMan::_LeftHyphenMin ? HyphMan::_LeftHyphenMin : _left_hyphen_min;
    int right_hyphen_min = HyphMan::_RightHyphenMin ? HyphMan::_RightHyphenMin : _right_hyphen_min;

    lUInt16 chprops[WORD_LENGTH];
    if ( len > WORD_LENGTH-2 )
        len = WORD_LENGTH - 2;
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
                if (i-start < left_hyphen_min - 1)
                    continue;
                if (end-i < right_hyphen_min + 1)
                    continue;
                if ( widths[i] > maxWidth )
                    break;
                if ( chprops[i] & CH_PROP_VOWEL ) {
                    for ( j=i+1; j<end; ++j ) {
                        if ( chprops[j] & CH_PROP_VOWEL ) {
                            int next = i+1;
                            while ( (chprops[next] & CH_PROP_HYPHEN) && next<end-MIN_WORD_LEN_TO_HYPHEN) {
                                // printf("next++\n");
                                next++;
                            }
                            int next2 = next+1;
                            while ( (chprops[next2] & CH_PROP_HYPHEN) && next2<end-MIN_WORD_LEN_TO_HYPHEN) {
                                // printf("next2++\n");
                                next2++;
                            }
                            if ( (chprops[next] & CH_PROP_CONSONANT) && (chprops[next2] & CH_PROP_CONSONANT) )
                                i = next;
                            else if ( (chprops[next] & CH_PROP_CONSONANT) && ( chprops[next2] & CH_PROP_ALPHA_SIGN ) )
                                i = next2;
                            if ( i-start>=1 && end-i>2 ) {
                                // insert hyphenation mark
                                lUInt16 nw = widths[i] + hyphCharWidth;
                                if ( nw<maxWidth )
                                {
                                    bool disabled = false;
                                    const char * dblSequences[] = {
                                        "sh", "th", "ph", "ch", NULL
                                    };
                                    next = i+1;
                                    while ( (chprops[next] & CH_PROP_HYPHEN) && next<end-MIN_WORD_LEN_TO_HYPHEN) {
                                        // printf("next3++\n");
                                        next++;
                                    }
                                    for (int k=0; dblSequences[k]; k++)
                                        if (str[i]==dblSequences[k][0] && str[next]==dblSequences[k][1]) {
                                            disabled = true;
                                            break;
                                        }
                                    if (!disabled) {
                                        if ( flagSize == 2 ) {
                                            lUInt16* flags16 = (lUInt16*) flags;
                                            flags16[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
                                        }
                                        else {
                                            flags[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
                                        }
                                    }
                                    //widths[i] = nw; // don't add hyph width
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
}

AlgoHyph::~AlgoHyph()
{
}



