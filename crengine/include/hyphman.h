/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2012,2015 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) Alan <alan@alreader.com>                                *
 *   Copyright (C) 2018-2020 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020,2022 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

/**
 *  \file hyphman.h
 *  \brief AlReader hyphenation manager adapted for CREngine by Vadim Lopatin
 */

#ifndef _HYPHEN_
#define _HYPHEN_

#include "lvtypes.h"
#include "lvstring.h"
#include "lvstream.h"
#include "lvhashtable.h"
#include "lvptrvec.h"

#define WORD_LENGTH   2048
//#define MAX_REAL_WORD 24

// min value supported by algorithms is 1 (max is arbitrary 10)
// 0 means to use the defaults per HyphMethod
// if set to >= 1, the values apply to all HyphMethods
#define HYPH_MIN_HYPHEN_MIN 0
#define HYPH_MAX_HYPHEN_MIN 10
// Default for global HyphMan values is 0: use per-HyphMethod defaults
#define HYPH_DEFAULT_HYPHEN_MIN 0
// Default for per-HyphMethod values (value enforced by algorithms
// previously was 2, so let's keep that as the default)
#define HYPHMETHOD_DEFAULT_HYPHEN_MIN 2

// Don't trust soft-hyphens when using dict or algo methods
#define HYPH_DEFAULT_TRUST_SOFT_HYPHENS 0

class HyphMethod
{
protected:
    lString32 _id;
    int _left_hyphen_min;
    int _right_hyphen_min;
public:
    HyphMethod(lString32 id)
        : _id(id)
        , _left_hyphen_min(HYPHMETHOD_DEFAULT_HYPHEN_MIN)
        , _right_hyphen_min(HYPHMETHOD_DEFAULT_HYPHEN_MIN)
    {}
    lString32 getId() { return _id; }
    virtual bool hyphenate( const lChar32 *str, int len, lUInt16 *widths, lUInt8 *flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize = 1 ) = 0;
    virtual ~HyphMethod() {}
    virtual lUInt32 getPatternsCount() { return 0; }
    virtual int getLeftHyphenMin() { return _left_hyphen_min; }
    virtual int getRightHyphenMin() { return _right_hyphen_min; }
};

enum HyphDictType
{
	HDT_NONE,      // disable hyphenation
	HDT_ALGORITHM, // universal
	HDT_SOFTHYPHENS, // from soft hyphens in text
	HDT_DICT_ALAN, // tex/alreader
    HDT_DICT_TEX   // tex/fbreader
};

class HyphDictionary
{
	HyphDictType _type;
	lString32 _title;
	lString32 _id;
	lString32 _langTag;
	lString32 _filename;
public:
	HyphDictionary(HyphDictType type, lString32 title, lString32 id, lString32 langTag, lString32 filename)
		: _type(type), _title(title), _id(id), _langTag(langTag), _filename(filename) {}
	HyphDictType getType() const { return _type; }
	lString32 getTitle() const { return _title; }
	lString32 getId() const { return _id; }
	lString32 getLangTag() const { return _langTag; }
	lString32 getFilename() const { return _filename; }
	bool activate();
	virtual lUInt32 getHash() const { return getTitle().getHash(); }
    virtual ~HyphDictionary() { }
};

#define HYPH_DICT_ID_NONE U"@none"
#define HYPH_DICT_ID_ALGORITHM U"@algorithm"
#define HYPH_DICT_ID_SOFTHYPHENS U"@softhyphens"
#define HYPH_DICT_ID_DICTIONARY U"@dictionary"

class HyphDictionaryList
{
	LVPtrVector<HyphDictionary> _list;
	void addDefault();
public:
    void add(HyphDictionary * dict) { _list.add(dict); }
	int length() { return _list.length(); }
	HyphDictionary * get( int index ) { return (index>=0 && index<+_list.length()) ? _list[index] : NULL; }
	HyphDictionaryList() { addDefault(); }
    bool open(lString32 hyphDirectory, bool clear = true);
	HyphDictionary * find( const lString32& id );
	bool activate( lString32 id );
};

#define DEF_HYPHENATION_DICT "hyph-en-us.pattern"
// We'll be loading hyph-en-us.pattern even if non-english users
// may never use it, but it's a bit tedious not going with it.
// It might use around 1M of memory, but it will avoid re-rendering
// the document if the book does not contain any language tag, and
// we end up going with it anyway.

class TexHyph;
class AlgoHyph;
class SoftHyphensHyph;

class HyphDataLoader
{
public:
    HyphDataLoader() {}
    virtual ~HyphDataLoader() {}
	virtual LVStreamRef loadData(lString32 id) = 0;
};

/// hyphenation manager
class HyphMan
{
    friend class HyphDictionary;
    friend class TexHyph;
    friend class AlgoHyph;
    friend class SoftHyphensHyph;
    // Obsolete: now fetched from TextLangMan main lang TextLangCfg
    // static HyphMethod * _method;
    // static HyphDictionary * _selectedDictionary;
    static HyphDictionaryList * _dictList; // available hyph dict files (+ none/algo/softhyphens)
    static LVHashTable<lString32, HyphMethod*> _loaded_hyph_methods; // methods with loaded dictionaries
    static HyphDataLoader* _dataLoader;
    static int _OverriddenLeftHyphenMin;
    static int _OverriddenRightHyphenMin;
    static int _TrustSoftHyphens;
    static HyphMethod* getHyphMethodForLang_impl(lString32 lang_tag);
public:
    static void uninit();
    static bool initDictionaries(lString32 dir, bool clear = true);
    static HyphDictionaryList * getDictList() { return _dictList; }
    static bool addDictionaryItem(HyphDictionary* dict);
    static void setDataLoader(HyphDataLoader* loader);
    static bool activateDictionary(lString32 id) { return _dictList->activate(id); }
    static HyphDictionary* getSelectedDictionary(); // was: { return _selectedDictionary; }
    static int getOverriddenLeftHyphenMin() { return _OverriddenLeftHyphenMin; }
    static int getOverriddenRightHyphenMin() { return _OverriddenRightHyphenMin; }
    static bool overrideLeftHyphenMin(int left_hyphen_min);
    static bool overrideRightHyphenMin(int right_hyphen_min);
    static int getTrustSoftHyphens() { return _TrustSoftHyphens; }
    static bool setTrustSoftHyphens( int trust_soft_hyphen );
    static bool isEnabled();
    static HyphMethod* getHyphMethodForDictionary(lString32 id);
    static HyphMethod* getHyphMethodForLang(lString32 lang_tag);
    
    HyphMan();
    ~HyphMan();

    static bool hyphenate( const lChar32 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize=1 );
    /* Obsolete:
    inline static bool hyphenate( const lChar32 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth, size_t flagSize=1 )
    {
        return _method->hyphenate( str, len, widths, flags, hyphCharWidth, maxWidth, flagSize );
    }
    */
};

#endif
