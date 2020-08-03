/** \file lvfontcache.cpp
    \brief font cache implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfontcache.h"
#include "../../include/lvstyles.h"
#include "../../include/crlog.h"


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
    for (int nindex = 0; nindex == 0 || nindex < list.length(); nindex++) {
        if (nindex < list.length())
            def.setTypeFace(list[nindex]);
        else
            def.setTypeFace(lString8::empty_str);
        for (i = 0; i < _instance_list.length(); i++) {
            int match = _instance_list[i]->_def.CalcMatch(def, useBias);
            if (match > best_instance_match) {
                best_instance_match = match;
                best_instance_index = i;
            }
        }
        for (i = 0; i < _registered_list.length(); i++) {
            int match = _registered_list[i]->_def.CalcMatch(def, useBias);
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
