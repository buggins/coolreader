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
#include "lvhashtable.h"
#include "../../include/crlocks.h"
#define GLYPHCACHE_TABLE_SZ         256

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

class LVLocalGlyphCacheHashTableStorage
{
    LVHashTable<lUInt32, struct LVFontGlyphCacheItem*> hashTable;
    LVFontGlobalGlyphCache* m_global_cache;
    //non-cpyable
    LVLocalGlyphCacheHashTableStorage();
    LVLocalGlyphCacheHashTableStorage( const LVLocalGlyphCacheHashTableStorage& );
    LVLocalGlyphCacheHashTableStorage& operator=( const LVLocalGlyphCacheHashTableStorage& );
public:
    LVLocalGlyphCacheHashTableStorage(LVFontGlobalGlyphCache *global_cache) :
        m_global_cache(global_cache), hashTable(GLYPHCACHE_TABLE_SZ) {}
    ~LVLocalGlyphCacheHashTableStorage() {
        clear();
    }
    LVFontGlyphCacheItem* get(lUInt32 ch);
    void put(LVFontGlyphCacheItem *item);
    void remove(LVFontGlyphCacheItem *item);
    void clear();
};

class LVLocalGlyphCacheListStorage
{
    LVFontGlobalGlyphCache* m_global_cache;
    LVFontGlyphCacheItem* head;
    LVFontGlyphCacheItem* tail;
    //non-cpyable
    LVLocalGlyphCacheListStorage();
    LVLocalGlyphCacheListStorage( const LVLocalGlyphCacheListStorage& );
    LVLocalGlyphCacheListStorage& operator=( const LVLocalGlyphCacheListStorage& );
public:
    LVLocalGlyphCacheListStorage(LVFontGlobalGlyphCache *global_cache) :
         m_global_cache(global_cache), head(), tail() {}
    ~LVLocalGlyphCacheListStorage() {
        clear();
    }
    LVFontGlyphCacheItem* get(lUInt32 ch);
    void put(LVFontGlyphCacheItem *item);
    void remove(LVFontGlyphCacheItem *item);
    void clear();
};

template<class S>
class LVFontLocalGlyphCache_t {
public:
    LVFontLocalGlyphCache_t(LVFontGlobalGlyphCache *globalCache) : m_storage(globalCache) {

    }
    void clear() {
        FONT_LOCAL_GLYPH_CACHE_GUARD
        m_storage.clear();
    }
    LVFontGlyphCacheItem *get(lUInt32 index) {
        FONT_LOCAL_GLYPH_CACHE_GUARD
        return m_storage.get(index);
    }
    void put(LVFontGlyphCacheItem *item) {
        FONT_LOCAL_GLYPH_CACHE_GUARD
        m_storage.put(item);
    }
    void remove(LVFontGlyphCacheItem *item) {
        FONT_LOCAL_GLYPH_CACHE_GUARD
        m_storage.remove(item);
    }
private:
    S m_storage;
};

#if USE_GLYPHCACHE_HASHTABLE == 1
    typedef LVFontLocalGlyphCache_t<LVLocalGlyphCacheHashTableStorage> LVFontLocalGlyphCache;
#else
    typedef LVFontLocalGlyphCache_t<LVLocalGlyphCacheListStorage> LVFontLocalGlyphCache;
#endif

#if USE_HARFBUZZ == 1
    typedef lUInt32 LVFontGlyphCacheKeyType;
#else
    typedef lChar16 LVFontGlyphCacheKeyType;
#endif

struct LVFontGlyphCacheItem {
    LVFontGlyphCacheItem *prev_global;
    LVFontGlyphCacheItem *next_global;
    LVFontGlyphCacheItem *prev_local;
    LVFontGlyphCacheItem *next_local;
    LVFontLocalGlyphCache *local_cache;
    LVFontGlyphCacheKeyType data;
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
    static LVFontGlyphCacheItem *newItem(LVFontLocalGlyphCache *local_cache, LVFontGlyphCacheKeyType ch_or_index, int w, int h);
    static void freeItem(LVFontGlyphCacheItem *item);
};
#endif //__LV_FONTGLYPHCACHE_H_INCLUDED__
