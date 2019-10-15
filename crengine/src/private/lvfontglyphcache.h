/** @file lvfont_glyphcache.h
    @brief font glyph cache interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_FONTGLYPHCACHE_H_INCLUDED__
#define __LV_FONTGLYPHCACHE_H_INCLUDED__

#include <stdlib.h>
#include "crsetup.h"
#include "lvtypes.h"

#if USE_GLYPHCACHE_HASHTABLE==1
#include "lvhashtable.h"
#define GLYPHCACHE_TABLE_SZ         256
#endif

struct LVFontGlyphCacheItem;

union GlyphCacheItemData {
	lChar16 ch;
#if USE_HARFBUZZ==1
	lUInt32 gindex;
#endif
};

class LVFontGlobalGlyphCache {
private:
    LVFontGlyphCacheItem *head;
    LVFontGlyphCacheItem *tail;
    int size;
    int max_size;

    void removeNoLock(LVFontGlyphCacheItem *item);

    void putNoLock(LVFontGlyphCacheItem *item);

public:
    LVFontGlobalGlyphCache(int maxSize)
            : head(NULL), tail(NULL), size(0), max_size(maxSize) {
    }

    ~LVFontGlobalGlyphCache() {
        clear();
    }

    void put(LVFontGlyphCacheItem *item);

    void remove(LVFontGlyphCacheItem *item);

#if USE_GLYPHCACHE_HASHTABLE != 1
    void refresh(LVFontGlyphCacheItem *item);
#endif

    void clear();
};

class LVFontLocalGlyphCache {
private:
    LVFontGlobalGlyphCache *global_cache;
#if USE_GLYPHCACHE_HASHTABLE == 1
    LVHashTable<GlyphCacheItemData, struct LVFontGlyphCacheItem*> hashTable;
#else
    LVFontGlyphCacheItem *head;
    LVFontGlyphCacheItem *tail;
#endif
    //int size;
public:
    LVFontLocalGlyphCache(LVFontGlobalGlyphCache *globalCache)
            : global_cache(globalCache),
#if USE_GLYPHCACHE_HASHTABLE == 1
            hashTable(GLYPHCACHE_TABLE_SZ)
#else
            head(NULL), tail(NULL)
#endif
    {}

    ~LVFontLocalGlyphCache() {
        clear();
    }

    void clear();

    LVFontGlyphCacheItem *get(lChar16 ch);
#if USE_HARFBUZZ==1
    LVFontGlyphCacheItem *getByIndex(lUInt32 index);
#endif

    void put(LVFontGlyphCacheItem *item);

    void remove(LVFontGlyphCacheItem *item);
};

struct LVFontGlyphCacheItem {
    LVFontGlyphCacheItem *prev_global;
    LVFontGlyphCacheItem *next_global;
    LVFontGlyphCacheItem *prev_local;
    LVFontGlyphCacheItem *next_local;
    LVFontLocalGlyphCache *local_cache;
    GlyphCacheItemData data;
    lUInt16 bmp_width;
    lUInt16 bmp_height;
    lInt16 origin_x;
    lInt16 origin_y;
    lUInt16 advance;
    lUInt8 bmp[1];

    //=======================================================================
    int getSize() {
        return sizeof(LVFontGlyphCacheItem)
               + (bmp_width * bmp_height - 1) * sizeof(lUInt8);
    }

    static LVFontGlyphCacheItem *newItem(LVFontLocalGlyphCache *local_cache, lChar16 data, int w, int h);
#if USE_HARFBUZZ==1
    static LVFontGlyphCacheItem *newItem(LVFontLocalGlyphCache *local_cache, lUInt32 glyph_index, int w, int h);
#endif

    static void freeItem(LVFontGlyphCacheItem *item);
};

#endif //__LV_FONTGLYPHCACHE_H_INCLUDED__
