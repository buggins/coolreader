/** @file lvfontglyphcache.h
	@brief font glyph cache interface

	CoolReader Engine

	(c) Vadim Lopatin, 2000-2006

	This source code is distributed under the terms of
	GNU General Public License.

	See LICENSE file for details.

*/

// This file contains copy of class LVFontLocalGlyphCache from cr3.2.31

#ifndef __LV_FONTGLYPHCACHEA_H_INCLUDED__
#define __LV_FONTGLYPHCACHEA_H_INCLUDED__

//#include <stdlib.h>
//#include "crsetup.h"
#include "lvtypes.h"


struct LVFontGlyphCacheItemA;

class LVFontGlobalGlyphCacheA {
private:
	LVFontGlyphCacheItemA *head;
	LVFontGlyphCacheItemA *tail;
	int size;
	int max_size;

	void removeNoLock(LVFontGlyphCacheItemA *item);

	void putNoLock(LVFontGlyphCacheItemA *item);

public:
	LVFontGlobalGlyphCacheA(int maxSize)
			: head(NULL), tail(NULL), size(0), max_size(maxSize) {
	}

	~LVFontGlobalGlyphCacheA() {
		clear();
	}

	void put(LVFontGlyphCacheItemA *item);

	void remove(LVFontGlyphCacheItemA *item);

	void refresh(LVFontGlyphCacheItemA *item);

	void clear();
    
    int getSize() { return size; }
};

class LVFontLocalGlyphCacheA {
private:
	LVFontGlyphCacheItemA *head;
	LVFontGlyphCacheItemA *tail;
	LVFontGlobalGlyphCacheA *global_cache;
	//int size;
public:
	LVFontLocalGlyphCacheA(LVFontGlobalGlyphCacheA *globalCache)
			: head(NULL), tail(NULL), global_cache(globalCache) {}

	~LVFontLocalGlyphCacheA() {
		clear();
	}

	void clear();

	LVFontGlyphCacheItemA *getByChar(lChar32 ch);
#if USE_HARFBUZZ==1
	LVFontGlyphCacheItemA *getByIndex(lUInt32 index);
#endif

	void put(LVFontGlyphCacheItemA *item);

	void remove(LVFontGlyphCacheItemA *item);
};

struct LVFontGlyphCacheItemA {
	LVFontGlyphCacheItemA *prev_global;
	LVFontGlyphCacheItemA *next_global;
	LVFontGlyphCacheItemA *prev_local;
	LVFontGlyphCacheItemA *next_local;
	LVFontLocalGlyphCacheA *local_cache;
	union {
		lChar32 ch;
#if USE_HARFBUZZ==1
		lUInt32 gindex;
#endif
	} data;
	lUInt16 bmp_width;
	lUInt16 bmp_height;
	lInt16 origin_x;
	lInt16 origin_y;
	lUInt16 advance;
	lUInt8 bmp[1];

	//=======================================================================
	int getSize() {
		return sizeof(LVFontGlyphCacheItemA)
			   + (bmp_width * bmp_height - 1) * sizeof(lUInt8);
	}

	static LVFontGlyphCacheItemA *newItem(LVFontLocalGlyphCacheA *local_cache, lChar32 data, int w, int h);
#if USE_HARFBUZZ==1
	static LVFontGlyphCacheItemA *newItem(LVFontLocalGlyphCacheA *local_cache, lUInt32 glyph_index, int w, int h);
#endif

	static void freeItem(LVFontGlyphCacheItemA *item);
};

#endif //__LV_FONTGLYPHCACHEA_H_INCLUDED__
