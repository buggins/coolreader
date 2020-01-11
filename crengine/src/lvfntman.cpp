/** @file lvfntman.cpp
    @brief font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "../include/lvfntman.h"
#include "../include/lvstyles.h"
#include "../include/crlog.h"
#include "private/lvfreetypefontman.h"
#include "private/lvwin32fontman.h"
#include "private/lvbitmapfontman.h"

#define GAMMA_TABLES_IMPL
#include "../include/gammatbl.h"

LVFontManager *fontMan = NULL;

static double gammaLevel = 1.0;
int gammaIndex = GAMMA_NO_CORRECTION_INDEX;

/// returns first found face from passed list, or return face for font found by family only
lString8 LVFontManager::findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily) {
    // faces we want
    lString8Collection list;
    splitPropertyValueList(commaSeparatedFaceList.c_str(), list);
    // faces we have
    lString16Collection faces;
    getFaceList(faces);
    // find first matched
    for (int i = 0; i < list.length(); i++) {
        lString8 wantFace = list[i];
        for (int j = 0; j < faces.length(); j++) {
            lString16 haveFace = faces[j];
            if (wantFace == haveFace)
                return wantFace;
        }
    }
    // not matched - get by family name
    LVFontRef fnt = GetFont(10, 400, false, fallbackByFamily, lString8("Arial"));
    if (fnt.isNull())
        return lString8::empty_str; // not found
    // get face from found font
    return fnt->getTypeFace();
}

/// fills array with list of available gamma levels
void LVFontManager::GetGammaLevels(LVArray<double> dst) {
    dst.clear();
    for (int i = 0; i < GAMMA_LEVELS; i++)
        dst.add(cr_gamma_levels[i]);
}

/// returns current gamma level index
int LVFontManager::GetGammaIndex() {
    return gammaIndex;
}

/// sets current gamma level index
void LVFontManager::SetGammaIndex(int index) {
    if (index < 0)
        index = 0;
    if (index >= GAMMA_LEVELS)
        index = GAMMA_LEVELS - 1;
    if (gammaIndex != index) {
        CRLog::trace("FontManager gamma index changed from %d to %d", gammaIndex, index);
        gammaIndex = index;
        gammaLevel = cr_gamma_levels[index];
        gc();
        clearGlyphCache();
    }
}

/// returns current gamma level
double LVFontManager::GetGamma() {
    return gammaLevel;
}

/// sets current gamma level
void LVFontManager::SetGamma( double gamma ) {
    // gammaLevel = cr_ft_gamma_levels[GAMMA_LEVELS/2];
    // gammaIndex = GAMMA_LEVELS/2;
    int oldGammaIndex = gammaIndex;
    for (int i = 0; i < GAMMA_LEVELS; i++) {
        double diff1 = cr_gamma_levels[i] - gamma;
        if (diff1 < 0) diff1 = -diff1;
        double diff2 = gammaLevel - gamma;
        if (diff2 < 0) diff2 = -diff2;
        if (diff1 < diff2) {
            gammaLevel = cr_gamma_levels[i];
            gammaIndex = i;
        }
    }
    if (gammaIndex != oldGammaIndex) {
        CRLog::trace("FontManager gamma index changed from %d to %d", oldGammaIndex, gammaIndex);
        gc();
        clearGlyphCache();
    }
}

bool InitFontManager(lString8 path) {
    if (fontMan) {
        return true;
        //delete fontMan;
    }
#if (USE_WIN32_FONTS == 1)
    fontMan = new LVWin32FontManager;
#elif (USE_FREETYPE == 1)
    fontMan = new LVFreeTypeFontManager;
#else
    fontMan = new LVBitmapFontManager;
#endif
    return fontMan->Init(path);
}

bool ShutdownFontManager() {
    if (fontMan) {
        delete fontMan;
        fontMan = NULL;
        return true;
    }
    return false;
}
