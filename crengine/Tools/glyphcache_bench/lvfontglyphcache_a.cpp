/** @file lvfontglyphcache.cpp
    @brief font glyph cache implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

// This file contains copy of class LVFontLocalGlyphCacheA from cr3.2.31

#include "lvfontglyphcache_a.h"
#include "../../include/crlocks.h"


void LVFontLocalGlyphCacheA::clear() {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    while (head) {
        LVFontGlyphCacheItemA *ptr = head;
        remove(ptr);
        global_cache->remove(ptr);
        LVFontGlyphCacheItemA::freeItem(ptr);
    }
}

LVFontGlyphCacheItemA *LVFontLocalGlyphCacheA::getByChar(lChar32 ch) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    LVFontGlyphCacheItemA *ptr = head;
    for (; ptr; ptr = ptr->next_local) {
        if (ptr->data.ch == ch) {
            global_cache->refresh(ptr);
            return ptr;
        }
    }
    return NULL;
}

#if USE_HARFBUZZ==1
LVFontGlyphCacheItemA* LVFontLocalGlyphCacheA::getByIndex(lUInt32 index)
{
    FONT_LOCAL_GLYPH_CACHE_GUARD
    LVFontGlyphCacheItemA *ptr = head;
    for (; ptr; ptr = ptr->next_local) {
        if (ptr->data.gindex == index) {
            global_cache->refresh(ptr);
            return ptr;
        }
    }
    return NULL;
}
#endif

void LVFontLocalGlyphCacheA::put(LVFontGlyphCacheItemA *item) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    global_cache->put(item);
    item->next_local = head;
    if (head)
        head->prev_local = item;
    if (!tail)
        tail = item;
    head = item;
}

/// remove from list, but don't delete
void LVFontLocalGlyphCacheA::remove(LVFontGlyphCacheItemA *item) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    if (item == head)
        head = item->next_local;
    if (item == tail)
        tail = item->prev_local;
    if (!head || !tail)
        return;
    if (item->prev_local)
        item->prev_local->next_local = item->next_local;
    if (item->next_local)
        item->next_local->prev_local = item->prev_local;
    item->next_local = NULL;
    item->prev_local = NULL;
}

void LVFontGlobalGlyphCacheA::refresh(LVFontGlyphCacheItemA *item) {
    FONT_GLYPH_CACHE_GUARD
    if (tail != item) {
        //move to head
        removeNoLock(item);
        putNoLock(item);
    }
}

void LVFontGlobalGlyphCacheA::put(LVFontGlyphCacheItemA *item) {
    FONT_GLYPH_CACHE_GUARD
    putNoLock(item);
}

void LVFontGlobalGlyphCacheA::putNoLock(LVFontGlyphCacheItemA *item) {
    int sz = item->getSize();
    // remove extra items from tail
    while (sz + size > max_size) {
        LVFontGlyphCacheItemA *removed_item = tail;
        if (!removed_item)
            break;
        removeNoLock(removed_item);
        removed_item->local_cache->remove(removed_item);
        LVFontGlyphCacheItemA::freeItem(removed_item);
    }
    // add new item to head
    item->next_global = head;
    if (head)
        head->prev_global = item;
    head = item;
    if (!tail)
        tail = item;
    size += sz;
}

void LVFontGlobalGlyphCacheA::remove(LVFontGlyphCacheItemA *item) {
    FONT_GLYPH_CACHE_GUARD
    removeNoLock(item);
}

void LVFontGlobalGlyphCacheA::removeNoLock(LVFontGlyphCacheItemA *item) {
    if (item == head)
        head = item->next_global;
    if (item == tail)
        tail = item->prev_global;
    if (!head || !tail)
        return;
    if (item->prev_global)
        item->prev_global->next_global = item->next_global;
    if (item->next_global)
        item->next_global->prev_global = item->prev_global;
    item->next_global = NULL;
    item->prev_global = NULL;
    size -= item->getSize();
}

void LVFontGlobalGlyphCacheA::clear() {
    FONT_GLYPH_CACHE_GUARD
    while (head) {
        LVFontGlyphCacheItemA *ptr = head;
        remove(ptr);
        ptr->local_cache->remove(ptr);
        LVFontGlyphCacheItemA::freeItem(ptr);
    }
}

LVFontGlyphCacheItemA *LVFontGlyphCacheItemA::newItem(LVFontLocalGlyphCacheA *local_cache, lChar32 ch, int w, int h) {
    LVFontGlyphCacheItemA *item = (LVFontGlyphCacheItemA *) malloc(sizeof(LVFontGlyphCacheItemA)
                                                                 + (w * h - 1) * sizeof(lUInt8));
    if (item) {
        item->data.ch = ch;
        item->bmp_width = (lUInt16) w;
        item->bmp_height = (lUInt16) h;
        item->origin_x = 0;
        item->origin_y = 0;
        item->advance = 0;
        item->prev_global = NULL;
        item->next_global = NULL;
        item->prev_local = NULL;
        item->next_local = NULL;
        item->local_cache = local_cache;
    }
    return item;
}

#if USE_HARFBUZZ==1
LVFontGlyphCacheItemA *LVFontGlyphCacheItemA::newItem(LVFontLocalGlyphCacheA* local_cache, lUInt32 glyph_index, int w, int h)
{
    LVFontGlyphCacheItemA *item = (LVFontGlyphCacheItemA *) malloc(sizeof(LVFontGlyphCacheItemA)
                                                                 + (w * h - 1) * sizeof(lUInt8));
    if (item) {
        item->data.gindex = glyph_index;
        item->bmp_width = (lUInt16) w;
        item->bmp_height = (lUInt16) h;
        item->origin_x = 0;
        item->origin_y = 0;
        item->advance = 0;
        item->prev_global = NULL;
        item->next_global = NULL;
        item->prev_local = NULL;
        item->next_local = NULL;
        item->local_cache = local_cache;
    }
    return item;
}
#endif // USE_HARFBUZZ==1

void LVFontGlyphCacheItemA::freeItem(LVFontGlyphCacheItemA *item) {
    if (item)
        ::free(item);
}
