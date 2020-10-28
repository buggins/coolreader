/** @file lvfntman.cpp
    @brief font glyph cache implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfontglyphcache_b.h"
#include "../../include/crlocks.h"
#include <stdio.h>


inline lUInt32 getHash(GlyphCacheItemData data)
{
    return getHash(*((lUInt32*)&data));
}

inline bool operator==(GlyphCacheItemData data1, GlyphCacheItemData data2)
{
    return (*((lUInt32*)&data1)) == (*((lUInt32*)&data2));
}

void LVFontLocalGlyphCacheB::clear() {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    int count = 0;
    LVHashTable<GlyphCacheItemData, struct LVFontGlyphCacheItemB*>::iterator it = hashTable.forwardIterator();
    LVHashTable<GlyphCacheItemData, struct LVFontGlyphCacheItemB*>::pair* pair;
    while ((pair = it.next()))
    {
        global_cache->remove(pair->value);
        LVFontGlyphCacheItemB::freeItem(pair->value);
        count++;
    }
    hashTable.clear();
    printf("LVFontLocalGlyphCacheB::clear(): removed %d items\n", count);
}

LVFontGlyphCacheItemB *LVFontLocalGlyphCacheB::getByChar(lChar32 ch) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    LVFontGlyphCacheItemB *ptr = 0;
    GlyphCacheItemData data;
    data.ch = ch;
    if (hashTable.get(data, ptr))
    {
        global_cache->refresh(ptr);
        return ptr;
    }
    return NULL;
}

#if USE_HARFBUZZ==1
LVFontGlyphCacheItemB* LVFontLocalGlyphCacheB::getByIndex(lUInt32 index)
{
    FONT_LOCAL_GLYPH_CACHE_GUARD
    LVFontGlyphCacheItemB *ptr = 0;
    GlyphCacheItemData data;
    data.gindex = index;
    if (hashTable.get(data, ptr))
    {
        global_cache->refresh(ptr);
        return ptr;
    }
    return NULL;
}
#endif

void LVFontLocalGlyphCacheB::put(LVFontGlyphCacheItemB *item) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    global_cache->put(item);
    hashTable.set(item->data, item);
}

/// remove hash table, but don't delete
void LVFontLocalGlyphCacheB::remove(LVFontGlyphCacheItemB *item) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    hashTable.remove(item->data);
}

void LVFontGlobalGlyphCacheB::refresh(LVFontGlyphCacheItemB *item) {
    FONT_GLYPH_CACHE_GUARD
    if (tail != item) {
        //move to head
        removeNoLock(item);
        putNoLock(item);
    }
}

void LVFontGlobalGlyphCacheB::put(LVFontGlyphCacheItemB *item) {
    FONT_GLYPH_CACHE_GUARD
    putNoLock(item);
}

void LVFontGlobalGlyphCacheB::putNoLock(LVFontGlyphCacheItemB *item) {
    int sz = item->getSize();
    // remove extra items from tail
    while (sz + size > max_size) {
        LVFontGlyphCacheItemB *removed_item = tail;
        if (!removed_item)
            break;
        removeNoLock(removed_item);
        removed_item->local_cache->remove(removed_item);
        LVFontGlyphCacheItemB::freeItem(removed_item);
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

void LVFontGlobalGlyphCacheB::remove(LVFontGlyphCacheItemB *item) {
    FONT_GLYPH_CACHE_GUARD
    removeNoLock(item);
}

void LVFontGlobalGlyphCacheB::removeNoLock(LVFontGlyphCacheItemB *item) {
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

void LVFontGlobalGlyphCacheB::clear() {
    FONT_GLYPH_CACHE_GUARD
    int count = 0;
    while (head) {
        LVFontGlyphCacheItemB *ptr = head;
        remove(ptr);
        ptr->local_cache->remove(ptr);
        LVFontGlyphCacheItemB::freeItem(ptr);
        count++;
    }
    printf("LVFontGlobalGlyphCacheB::clear(): removed %d items\n", count);
}

LVFontGlyphCacheItemB *LVFontGlyphCacheItemB::newItem(LVFontLocalGlyphCacheB *local_cache, lChar32 ch, int w, int h) {
    LVFontGlyphCacheItemB *item = (LVFontGlyphCacheItemB *) malloc(sizeof(LVFontGlyphCacheItemB)
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
LVFontGlyphCacheItemB *LVFontGlyphCacheItemB::newItem(LVFontLocalGlyphCacheB* local_cache, lUInt32 glyph_index, int w, int h)
{
    LVFontGlyphCacheItemB *item = (LVFontGlyphCacheItemB *) malloc(sizeof(LVFontGlyphCacheItemB)
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

void LVFontGlyphCacheItemB::freeItem(LVFontGlyphCacheItemB *item) {
    if (item)
        ::free(item);
}
