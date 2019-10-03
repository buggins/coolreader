/** @file lvfntman.cpp
    @brief font glyph cache implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfontglyphcache.h"
#include "../../include/crlocks.h"


void LVFontLocalGlyphCache::clear() {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    while (head) {
        LVFontGlyphCacheItem *ptr = head;
        remove(ptr);
        global_cache->remove(ptr);
        LVFontGlyphCacheItem::freeItem(ptr);
    }
}

LVFontGlyphCacheItem *LVFontLocalGlyphCache::getByChar(lChar16 ch) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    LVFontGlyphCacheItem *ptr = head;
    for (; ptr; ptr = ptr->next_local) {
        if (ptr->data.ch == ch) {
            global_cache->refresh(ptr);
            return ptr;
        }
    }
    return NULL;
}

#if USE_HARFBUZZ==1
LVFontGlyphCacheItem*LVFontLocalGlyphCache::getByIndex(lUInt32 index)
{
    FONT_LOCAL_GLYPH_CACHE_GUARD
    LVFontGlyphCacheItem *ptr = head;
    for (; ptr; ptr = ptr->next_local) {
        if (ptr->data.gindex == index) {
            global_cache->refresh(ptr);
            return ptr;
        }
    }
    return NULL;
}
#endif

void LVFontLocalGlyphCache::put(LVFontGlyphCacheItem *item) {
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
void LVFontLocalGlyphCache::remove(LVFontGlyphCacheItem *item) {
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

void LVFontGlobalGlyphCache::refresh(LVFontGlyphCacheItem *item) {
    FONT_GLYPH_CACHE_GUARD
    if (tail != item) {
        //move to head
        removeNoLock(item);
        putNoLock(item);
    }
}

void LVFontGlobalGlyphCache::put(LVFontGlyphCacheItem *item) {
    FONT_GLYPH_CACHE_GUARD
    putNoLock(item);
}

void LVFontGlobalGlyphCache::putNoLock(LVFontGlyphCacheItem *item) {
    int sz = item->getSize();
    // remove extra items from tail
    while (sz + size > max_size) {
        LVFontGlyphCacheItem *removed_item = tail;
        if (!removed_item)
            break;
        removeNoLock(removed_item);
        removed_item->local_cache->remove(removed_item);
        LVFontGlyphCacheItem::freeItem(removed_item);
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

void LVFontGlobalGlyphCache::remove(LVFontGlyphCacheItem *item) {
    FONT_GLYPH_CACHE_GUARD
    removeNoLock(item);
}

void LVFontGlobalGlyphCache::removeNoLock(LVFontGlyphCacheItem *item) {
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

void LVFontGlobalGlyphCache::clear() {
    FONT_GLYPH_CACHE_GUARD
    while (head) {
        LVFontGlyphCacheItem *ptr = head;
        remove(ptr);
        ptr->local_cache->remove(ptr);
        LVFontGlyphCacheItem::freeItem(ptr);
    }
}

LVFontGlyphCacheItem *LVFontGlyphCacheItem::newItem(LVFontLocalGlyphCache *local_cache, lChar16 ch, int w, int h) {
    LVFontGlyphCacheItem *item = (LVFontGlyphCacheItem *) malloc(sizeof(LVFontGlyphCacheItem)
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
LVFontGlyphCacheItem *LVFontGlyphCacheItem::newItem(LVFontLocalGlyphCache* local_cache, lUInt32 glyph_index, int w, int h)
{
    LVFontGlyphCacheItem *item = (LVFontGlyphCacheItem *) malloc(sizeof(LVFontGlyphCacheItem)
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

void LVFontGlyphCacheItem::freeItem(LVFontGlyphCacheItem *item) {
    if (item)
        ::free(item);
}
