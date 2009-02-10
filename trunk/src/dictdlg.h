#ifndef __MOD_DICT_H
#define __MOD_DICT_H 1

#include <cstdlib>
#include <tinydict.h>
#include "lvdocview.h"
#include "crgui.h"
#include "t9encoding.h"

/// dictionary interface
class CRDictionary
{
public:
	virtual lString8 translate(const lString8 & w) = 0;
	virtual ~CRDictionary() { }
};


//TODO: place TinyDictionary to separate file
class CRTinyDict : public CRDictionary
{
	TinyDictionaryList dicts;
public:
	CRTinyDict( const lString16& config );
	virtual ~CRTinyDict() { }
    virtual lString8 translate(const lString8 & w);
};


class V3DocViewWin;

extern void
activate_dict(CRGUIWindowManager *wm, V3DocViewWin * mainwin, const TEncoding& encoding, CRDictionary & dict );



#endif
