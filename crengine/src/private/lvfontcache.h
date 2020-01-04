/** \file lvfontcache.h
    \brief font cache

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_FONTCACHE_H_INCLUDED__
#define __LV_FONTCACHE_H_INCLUDED__

#include "../../include/crsetup.h"
#include "../../include/lvfont.h"
#include "../../include/lvptrvec.h"
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

    virtual void getFaceList(lString16Collection &list) {
        list.clear();
        for (int i = 0; i < _registered_list.length(); i++) {
            if (_registered_list[i]->getDef()->getDocumentId() != -1)
                continue;
            lString16 name = Utf8ToUnicode(_registered_list[i]->getDef()->getTypeFace());
            if (!list.contains(name))
                list.add(name);
        }
        list.sort();
    }

    virtual void getFontFileNameList(lString16Collection &list) {
        list.clear();
        for (int i = 0; i < _registered_list.length(); i++) {
            if (_registered_list[i]->getDef()->getDocumentId() == -1) {
                lString16 name = Utf8ToUnicode(_registered_list[i]->getDef()->getName());
                if (!list.contains(name))
                    list.add(name);
            }
        }
        list.sort();
    }

    virtual void clearFallbackFonts() {
        for (int i = 0; i < _registered_list.length(); i++) {
            _registered_list[i]->getFont()->setFallbackFont(LVFontRef());
        }
    }

    LVFontCache() {}

    virtual ~LVFontCache() {}
};

#endif  // __LV_FONTCACHE_H_INCLUDED__
