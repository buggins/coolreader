/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2019 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2021 NiLuJe <ninuje@gmail.com>                          *
 *   Copyright (C) 2018-2021 Aleksey Chernov <valexlin@gmail.com>          *
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
 * \file lvfont_glyphcache.h
 * \brief font glyph cache interface
 */

#ifndef __LV_FONTGLYPHCACHE_H_INCLUDED__
#define __LV_FONTGLYPHCACHE_H_INCLUDED__

#include <stdlib.h>
#include <stddef.h>
#include "crsetup.h"
#include "lvtypes.h"
#include "lvhashtable.h"
#include "lvdrawbuf.h"
#include "crlocks.h"

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
    typedef lChar32 LVFontGlyphCacheKeyType;
#endif

struct LVFontGlyphCacheItem {
    LVFontGlyphCacheItem *prev_global;
    LVFontGlyphCacheItem *next_global;
    LVFontGlyphCacheItem *prev_local;
    LVFontGlyphCacheItem *next_local;
    LVFontLocalGlyphCache *local_cache;
    LVFontGlyphCacheKeyType data;
    FontBmpPixelFormat bmp_fmt;
    lUInt16 bmp_width;
    lUInt16 bmp_height;
    lInt16 bmp_pitch;
    lInt16 origin_x;
    lInt16 origin_y;
    lUInt16 advance;
    lUInt8 bmp[1];

    //=======================================================================
    int getSize() {
        return offsetof(LVFontGlyphCacheItem, bmp)
               + (bmp_pitch * bmp_height) * sizeof(lUInt8);
    }
    static LVFontGlyphCacheItem *newItem(LVFontLocalGlyphCache *local_cache, LVFontGlyphCacheKeyType ch_or_index, int w, int h, unsigned int bmp_pitch, unsigned int bmp_sz);
    static void freeItem(LVFontGlyphCacheItem *item);
};
#endif //__LV_FONTGLYPHCACHE_H_INCLUDED__
