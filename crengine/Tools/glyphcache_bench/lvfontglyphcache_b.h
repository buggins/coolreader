/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2011,2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2019,2020 Aleksey Chernov <valexlin@gmail.com>          *
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

// This is a new glyph cache based on hash table, significally faster item lookup.

#ifndef __LV_FONTGLYPHCACHEB_H_INCLUDED__
#define __LV_FONTGLYPHCACHEB_H_INCLUDED__

#include "lvtypes.h"
#include "lvhashtable.h"

union GlyphCacheItemData {
	lChar32 ch;
#if USE_HARFBUZZ==1
	lUInt32 gindex;
#endif
};

struct LVFontGlyphCacheItemB;

class LVFontGlobalGlyphCacheB {
private:
	LVFontGlyphCacheItemB *head;
	LVFontGlyphCacheItemB *tail;
	int size;
	int max_size;

	void removeNoLock(LVFontGlyphCacheItemB *item);

	void putNoLock(LVFontGlyphCacheItemB *item);

public:
	LVFontGlobalGlyphCacheB(int maxSize)
			: head(NULL), tail(NULL), size(0), max_size(maxSize) {
	}

	~LVFontGlobalGlyphCacheB() {
		clear();
	}

	void put(LVFontGlyphCacheItemB *item);

	void remove(LVFontGlyphCacheItemB *item);

	void refresh(LVFontGlyphCacheItemB *item);

	void clear();

	int getSize() { return size; }
};

class LVFontLocalGlyphCacheB {
private:
	LVFontGlobalGlyphCacheB *global_cache;
	LVHashTable<GlyphCacheItemData, struct LVFontGlyphCacheItemB*> hashTable;
public:
	LVFontLocalGlyphCacheB(LVFontGlobalGlyphCacheB *globalCache, int hashTableSize)
			: global_cache(globalCache), hashTable(hashTableSize) {}

	~LVFontLocalGlyphCacheB() {
		clear();
	}

	void clear();

	LVFontGlyphCacheItemB *getByChar(lChar32 ch);
#if USE_HARFBUZZ==1
	LVFontGlyphCacheItemB *getByIndex(lUInt32 index);
#endif

	void put(LVFontGlyphCacheItemB *item);

	void remove(LVFontGlyphCacheItemB *item);
};

struct LVFontGlyphCacheItemB {
	LVFontGlyphCacheItemB *prev_global;
	LVFontGlyphCacheItemB *next_global;
	LVFontGlyphCacheItemB *prev_local;
	LVFontGlyphCacheItemB *next_local;
	LVFontLocalGlyphCacheB *local_cache;
	GlyphCacheItemData data;
	lUInt16 bmp_width;
	lUInt16 bmp_height;
	lInt16 origin_x;
	lInt16 origin_y;
	lUInt16 advance;
	lUInt8 bmp[1];

	//=======================================================================
	int getSize() {
		return sizeof(LVFontGlyphCacheItemB)
			   + (bmp_width * bmp_height - 1) * sizeof(lUInt8);
	}

	static LVFontGlyphCacheItemB *newItem(LVFontLocalGlyphCacheB *local_cache, lChar32 data, int w, int h);
#if USE_HARFBUZZ==1
	static LVFontGlyphCacheItemB *newItem(LVFontLocalGlyphCacheB *local_cache, lUInt32 glyph_index, int w, int h);
#endif

	static void freeItem(LVFontGlyphCacheItemB *item);
};

#endif //__LV_FONTGLYPHCACHEB_H_INCLUDED__
