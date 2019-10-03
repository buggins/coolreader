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


struct LVFontGlyphCacheItem;

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

    void refresh(LVFontGlyphCacheItem *item);

    void clear();
};

class LVFontLocalGlyphCache {
private:
    LVFontGlyphCacheItem *head;
    LVFontGlyphCacheItem *tail;
    LVFontGlobalGlyphCache *global_cache;
    //int size;
public:
    LVFontLocalGlyphCache(LVFontGlobalGlyphCache *globalCache)
            : head(NULL), tail(NULL), global_cache(globalCache) {}

    ~LVFontLocalGlyphCache() {
        clear();
    }

    void clear();

    LVFontGlyphCacheItem *getByChar(lChar16 ch);
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
    union {
        lChar16 ch;
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
