/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2017,2018 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2018,2020,2021 Aleksey Chernov <valexlin@gmail.com>     *
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
 * \file lvfontcache.h
 * \brief font cache
 */

#ifndef __LV_FONTCACHE_H_INCLUDED__
#define __LV_FONTCACHE_H_INCLUDED__

#include "crsetup.h"
#include "lvfont.h"
#include "lvptrvec.h"
#include "lvstring32collection.h"
#include "lvfontdef.h"

/// font cache item
class LVFontCacheItem {
    friend class LVFontCache;

    LVFontDef _def;
    LVFontRef _fnt;
public:
    LVFontDef *getDef() { return &_def; }

    LVFontRef &getFont() { return _fnt; }

    void setFont(LVFontRef &fnt) { _fnt = fnt; }

    LVFontCacheItem(const LVFontDef &def)
            : _def(def) {}
};

/// font cache
class LVFontCache {
    LVPtrVector<LVFontCacheItem> _registered_list;
    LVPtrVector<LVFontCacheItem> _instance_list;
public:
    void clear() {
        _registered_list.clear();
        _instance_list.clear();
    }

    void gc(); // garbage collector
    void update(const LVFontDef *def, LVFontRef ref);

    void removefont(const LVFontDef *def);

    void removeDocumentFonts(int documentId);

    int length() { return _registered_list.length(); }

    void addInstance(const LVFontDef *def, LVFontRef ref);

    bool setAsPreferredFontWithBias( lString8 face, int bias, bool clearOthersBias );

    LVPtrVector<LVFontCacheItem> *getInstances() { return &_instance_list; }

    LVFontCacheItem *find(const LVFontDef *def, bool useBias=false);

    LVFontCacheItem *findFallback(lString8 face, int size);

    LVFontCacheItem *findDuplicate(const LVFontDef *def);

    LVFontCacheItem *findDocumentFontDuplicate(int documentId, lString8 name);

    /// get hash of installed fonts and fallback font
    virtual lUInt32 GetFontListHash(int documentId) {
        lUInt32 hash = 0;
        for (int i = 0; i < _registered_list.length(); i++) {
            int doc = _registered_list[i]->getDef()->getDocumentId();
            if (doc == -1 || doc == documentId) // skip document fonts
                hash = hash + _registered_list[i]->getDef()->getHash();
        }
        return 0;
    }

    virtual void getFaceList(lString32Collection &list) {
        list.clear();
        for (int i = 0; i < _registered_list.length(); i++) {
            if (_registered_list[i]->getDef()->getDocumentId() != -1)
                continue;
            lString32 name = Utf8ToUnicode(_registered_list[i]->getDef()->getTypeFace());
            if (!list.contains(name))
                list.add(name);
        }
        list.sort();
    }

    virtual void getFontFileNameList(lString32Collection &list) {
        list.clear();
        for (int i = 0; i < _registered_list.length(); i++) {
            if (_registered_list[i]->getDef()->getDocumentId() == -1) {
                lString32 name = Utf8ToUnicode(_registered_list[i]->getDef()->getName());
                if (!list.contains(name))
                    list.add(name);
            }
        }
        list.sort();
    }

    virtual void getAvailableFontWeights(LVArray<int>& weights, lString8 faceName);

    virtual void clearFallbackFonts() {
        for (int i = 0; i < _registered_list.length(); i++) {
            LVFontRef fontRef = _registered_list[i]->getFont();
            if (!fontRef.isNull())
                fontRef->setFallbackFont(LVFontRef());
        }
    }

    LVFontCache() {}

    virtual ~LVFontCache() {}
};

#endif  // __LV_FONTCACHE_H_INCLUDED__
