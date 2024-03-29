/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 poire-z <poire-z@users.noreply.github.com>         *
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

#include "lvfntman.h"
#include "lvstyles.h"
#include "crlog.h"
#include "lvfreetypefontman.h"
#include "lvwin32fontman.h"
#include "lvbitmapfontman.h"

#define GAMMA_TABLES_IMPL
#include "gammatbl.h"

LVFontManager *fontMan = NULL;

static double gammaLevel = 1.0;
int gammaIndex = GAMMA_NO_CORRECTION_INDEX;

/// returns first found face from passed list, or return face for font found by family only
lString8 LVFontManager::findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily) {
    // faces we want
    lString8Collection list;
    splitPropertyValueList(commaSeparatedFaceList.c_str(), list);
    // faces we have
    lString32Collection faces;
    getFaceList(faces);
    // find first matched
    for (int i = 0; i < list.length(); i++) {
        lString8 wantFace = list[i];
        for (int j = 0; j < faces.length(); j++) {
            lString32 haveFace = faces[j];
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
