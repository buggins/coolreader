/** @file lvbitmapfontman.h
    @brief bitmap font manager interface

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_BITMAPFONTMAN_H_INCLUDED__
#define __LV_BITMAPFONTMAN_H_INCLUDED__

#include "crsetup.h"
#include "lvfntman.h"
#include "lvfontcache.h"


#if (USE_BITMAP_FONTS == 1)

class LVBitmapFontManager : public LVFontManager {
private:
    lString8 _path;
    LVFontCache _cache;
    //FILE * _log;
public:
    virtual int GetFontCount() {
        return _cache.length();
    }

    virtual ~LVBitmapFontManager();

    LVBitmapFontManager();

    virtual void gc();

    lString8 makeFontFileName(lString8 name);

    virtual LVFontRef
    GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface,
            int features=0, int documentId = -1, bool useBias=false);

    virtual void GetAvailableFontWeights(LVArray<int>& weights, lString8 typeface) {}

    virtual bool RegisterFont(lString8 name);

    /// returns registered font files
    virtual void getFontFileNameList(lString32Collection &list);

    virtual bool Init(lString8 path);
};

#endif  // (USE_BITMAP_FONTS==1)

#endif  // __LV_BITMAPFONTMAN_H_INCLUDED__
