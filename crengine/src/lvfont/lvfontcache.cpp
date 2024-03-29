/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2010-2012 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2017,2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2019-2021 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "lvfontcache.h"
#include "lvstyles.h"
#include "crlog.h"


LVFontCacheItem *LVFontCache::findDuplicate(const LVFontDef *def) {
    for (int i = 0; i < _registered_list.length(); i++) {
        if (_registered_list[i]->_def.CalcDuplicateMatch(*def))
            return _registered_list[i];
    }
    return NULL;
}

LVFontCacheItem *LVFontCache::findDocumentFontDuplicate(int documentId, lString8 name) {
    for (int i = 0; i < _registered_list.length(); i++) {
        if (_registered_list[i]->_def.getDocumentId() == documentId &&
            _registered_list[i]->_def.getName() == name)
            return _registered_list[i];
    }
    return NULL;
}

LVFontCacheItem *LVFontCache::findFallback(lString8 face, int size) {
    int best_index = -1;
    int best_match = -1;
    int best_instance_index = -1;
    int best_instance_match = -1;
    int i;
    for (i = 0; i < _instance_list.length(); i++) {
        int match = _instance_list[i]->_def.CalcFallbackMatch(face, size);
        if (match > best_instance_match) {
            best_instance_match = match;
            best_instance_index = i;
        }
    }
    for (i = 0; i < _registered_list.length(); i++) {
        int match = _registered_list[i]->_def.CalcFallbackMatch(face, size);
        if (match > best_match) {
            best_match = match;
            best_index = i;
        }
    }
    if (best_index <= 0)
        return NULL;
    if (best_instance_match >= best_match)
        return _instance_list[best_instance_index];
    return _registered_list[best_index];
}

LVFontCacheItem *LVFontCache::find(const LVFontDef *fntdef, bool useBias) {
    int best_index = -1;
    int best_match = -1;
    int best_instance_index = -1;
    int best_instance_match = -1;
    int i;
    LVFontDef def(*fntdef);
    lString8Collection list;
    splitPropertyValueList(fntdef->getTypeFace().c_str(), list);
    int nlen = list.length();
    for (int nindex=0; nindex==0 || nindex<nlen; nindex++) {
        // Give more weight to first fonts, so we don't risk (with the test at end)
        // picking an already instantiated second font over a not yet instantiated
        // first font with the same match.
        int ordering_weight = nlen - nindex;
        if ( nindex < nlen )
            def.setTypeFace(list[nindex]);
        else
            def.setTypeFace(lString8::empty_str);
        for (i = 0; i < _instance_list.length(); i++) {
            int match = _instance_list[i]->_def.CalcMatch(def, useBias);
            match = match * 256 + ordering_weight;
            if (match > best_instance_match) {
                best_instance_match = match;
                best_instance_index = i;
            }
        }
        for (i = 0; i < _registered_list.length(); i++) {
            int match = _registered_list[i]->_def.CalcMatch(def, useBias);
            match = match * 256 + ordering_weight;
            if (match > best_match) {
                best_match = match;
                best_index = i;
            }
        }
    }
    if (best_index < 0)
        return NULL;
    if (best_instance_match >= best_match)
        return _instance_list[best_instance_index];
    return _registered_list[best_index];
}

bool LVFontCache::setAsPreferredFontWithBias( lString8 face, int bias, bool clearOthersBias )
{
    bool found = false;
    int i;
    for (i=0; i<_instance_list.length(); i++) {
        if (_instance_list[i]->_def.setBiasIfNameMatch( face, bias, clearOthersBias ))
            found = true;
    }
    for (i=0; i<_registered_list.length(); i++) {
        if (_registered_list[i]->_def.setBiasIfNameMatch( face, bias, clearOthersBias ))
            found = true;
    }
    return found;
}

void LVFontCache::addInstance(const LVFontDef *def, LVFontRef ref) {
    if (ref.isNull())
        CRLog::error("Adding null font instance!");
    LVFontCacheItem *item = new LVFontCacheItem(*def);
    item->_fnt = ref;
    _instance_list.add(item);
}

void LVFontCache::removefont(const LVFontDef *def) {
    int i;
    for (i = 0; i < _instance_list.length(); i++) {
        if (_instance_list[i]->_def.getTypeFace() == def->getTypeFace()) {
            _instance_list.remove(i);
        }

    }
    for (i = 0; i < _registered_list.length(); i++) {
        if (_registered_list[i]->_def.getTypeFace() == def->getTypeFace()) {
            _registered_list.remove(i);
        }
    }
}

void LVFontCache::update(const LVFontDef *def, LVFontRef ref) {
    int i;
    if (!ref.isNull()) {
        for (i = 0; i < _instance_list.length(); i++) {
            if (_instance_list[i]->_def == *def) {
                if (ref.isNull()) {
                    _instance_list.erase(i, 1);
                } else {
                    _instance_list[i]->_fnt = ref;
                }
                return;
            }
        }
        // add new
        //LVFontCacheItem * item;
        //item = new LVFontCacheItem(*def);
        addInstance(def, ref);
    } else {
        for (i = 0; i < _registered_list.length(); i++) {
            if (_registered_list[i]->_def == *def) {
                return;
            }
        }
        // add new
        LVFontCacheItem *item;
        item = new LVFontCacheItem(*def);
        _registered_list.add(item);
    }
}

void LVFontCache::removeDocumentFonts(int documentId) {
    if (-1 == documentId)
        return;
    int i;
    for (i = _instance_list.length() - 1; i >= 0; i--) {
        if (_instance_list[i]->_def.getDocumentId() == documentId)
            delete _instance_list.remove(i);
    }
    for (i = _registered_list.length() - 1; i >= 0; i--) {
        if (_registered_list[i]->_def.getDocumentId() == documentId)
            delete _registered_list.remove(i);
    }
}

static int s_int_comparator(const void * n1, const void * n2)
{
    int* i1 = (int*)n1;
    int* i2 = (int*)n2;
    return *i1 == *i2 ? 0 : (*i1 < *i2 ? -1 : 1);
}

void LVFontCache::getAvailableFontWeights(LVArray<int>& weights, lString8 faceName) {
    weights.clear();
    for (int i = 0; i < _registered_list.length(); i++) {
        const LVFontCacheItem* item = _registered_list[i];
        if (item->_def.getTypeFace() == faceName) {
            if (item->_def.isRealWeight()) {       // ignore fonts with fake weight
                int weight = item->_def.getWeight();
                if (weights.indexOf(weight) < 0) {
                    weights.add(weight);
                }
            }
        }
    }
    int* ptr = weights.get();
    qsort(ptr, (size_t)weights.length(), sizeof(int), s_int_comparator);
}

// garbage collector
void LVFontCache::gc() {
    int droppedCount = 0;
    int usedCount = 0;
    for (int i = _instance_list.length() - 1; i >= 0; i--) {
        if (_instance_list[i]->_fnt.getRefCount() <= 1) {
            if (CRLog::isTraceEnabled())
                CRLog::trace("dropping font instance %s[%d] by gc()",
                             _instance_list[i]->getDef()->getTypeFace().c_str(),
                             _instance_list[i]->getDef()->getSize());
            _instance_list.erase(i, 1);
            droppedCount++;
        } else {
            usedCount++;
        }
    }
    if (CRLog::isDebugEnabled())
        CRLog::debug("LVFontCache::gc() : %d fonts still used, %d fonts dropped", usedCount,
                     droppedCount);
}
