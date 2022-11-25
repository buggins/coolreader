/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2011,2013,2014 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2019 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2021 NiLuJe <ninuje@gmail.com>                          *
 *   Copyright (C) 2019,2021 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "lvfontglyphcache.h"

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

LVFontGlyphCacheItem *LVFontGlyphCacheItem::newItem(LVFontLocalGlyphCache* local_cache, LVFontGlyphCacheKeyType ch_or_index, int w, int h, unsigned int bmp_pitch, unsigned int bmp_sz)
{
    LVFontGlyphCacheItem *item = (LVFontGlyphCacheItem *) malloc(offsetof(LVFontGlyphCacheItem, bmp) + bmp_sz);
    if (item) {
        item->data = ch_or_index;
        item->bmp_width = (lUInt16) w;
        item->bmp_height = (lUInt16) h;
        item->bmp_pitch = (lInt16) bmp_pitch;
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

void LVFontGlyphCacheItem::freeItem(LVFontGlyphCacheItem *item) {
    if (item)
        ::free(item);
}

LVFontGlyphCacheItem *LVLocalGlyphCacheHashTableStorage::get(lUInt32 ch)
{
    LVFontGlyphCacheItem *ptr = 0;
    if (hashTable.get(ch, ptr))
        m_global_cache->refresh(ptr);
    return ptr;
}

void LVLocalGlyphCacheHashTableStorage::put(LVFontGlyphCacheItem *item)
{
    m_global_cache->put(item);
    hashTable.set(item->data, item);
}

void LVLocalGlyphCacheHashTableStorage::remove(LVFontGlyphCacheItem *item)
{
    hashTable.remove(item->data);
}

void LVLocalGlyphCacheHashTableStorage::clear()
{
    FONT_LOCAL_GLYPH_CACHE_GUARD

    LVHashTable<lUInt32, struct LVFontGlyphCacheItem*>::iterator it = hashTable.forwardIterator();
    LVHashTable<lUInt32, struct LVFontGlyphCacheItem*>::pair* pair;
    while( (pair = it.next()) ) {
        m_global_cache->remove(pair->value);
        LVFontGlyphCacheItem::freeItem(pair->value);
    }
    hashTable.clear();
}

LVFontGlyphCacheItem *LVLocalGlyphCacheListStorage::get(lUInt32 ch)
{
    LVFontGlyphCacheItem *ptr = head;
    for (; ptr; ptr = ptr->next_local) {
        if (ptr->data == ch) {
            m_global_cache->refresh(ptr);
            return ptr;
        }
    }
    return NULL;
}

void LVLocalGlyphCacheListStorage::put(LVFontGlyphCacheItem *item)
{
    m_global_cache->put(item);
    item->next_local = head;
    if (head)
        head->prev_local = item;
    if (!tail)
        tail = item;
    head = item;
}

void LVLocalGlyphCacheListStorage::remove(LVFontGlyphCacheItem *item)
{
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

void LVLocalGlyphCacheListStorage::clear()
{
    while (head) {
        LVFontGlyphCacheItem *ptr = head;
        remove(ptr);
        m_global_cache->remove(ptr);
        LVFontGlyphCacheItem::freeItem(ptr);
    }
}
