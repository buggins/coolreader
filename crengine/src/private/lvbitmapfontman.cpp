/** @file lvbitmapfontman.cpp
    @brief bitmap font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvbitmapfontman.h"
#include "lvbitmapfont.h"

#if (USE_BITMAP_FONTS == 1)

LVBitmapFontManager::~LVBitmapFontManager() {
    //if (_log)
    //    fclose(_log);
}

LVBitmapFontManager::LVBitmapFontManager() {
    //_log = fopen( "fonts.log", "wt" );
}

void LVBitmapFontManager::gc() // garbage collector
{
    _cache.gc();
}

lString8 LVBitmapFontManager::makeFontFileName(lString8 name) {
    lString8 filename = _path;
    if (!filename.empty() && _path[filename.length() - 1] != PATH_SEPARATOR_CHAR)
        filename << PATH_SEPARATOR_CHAR;
    filename << name;
    return filename;
}

LVFontRef LVBitmapFontManager::GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface,
                                       int features, int documentId, bool useBias) {
    LVFontDef *def = new LVFontDef(
            lString8::empty_str,
            size,
            weight,
            italic,
            0,
            family,
            typeface,
            documentId
    );
    //fprintf( _log, "GetFont: %s %d %s %s\n",
    //    typeface.c_str(),
    //    size,
    //    weight>400?"bold":"",
    //    italic?"italic":"" );
    LVFontCacheItem *item = _cache.find(def);
    delete def;
    if (!item->getFont().isNull()) {
        //fprintf(_log, "    : fount existing\n");
        return item->getFont();
    }
    LBitmapFont *font = new LBitmapFont;
    lString8 fname = makeFontFileName(item->getDef()->getName());
    //printf("going to load font file %s\n", fname.c_str());
    if (font->LoadFromFile(fname.c_str())) {
        //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
        //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
        LVFontRef ref(font);
        item->setFont(ref);
        return ref;
    } else {
        //printf("    not found!\n");
    }
    delete font;
    return LVFontRef(NULL);
}

bool LVBitmapFontManager::RegisterFont(lString8 name) {
    lString8 fname = makeFontFileName(name);
    //printf("going to load font file %s\n", fname.c_str());
    LVStreamRef stream = LVOpenFileStream(fname.c_str(), LVOM_READ);
    if (!stream) {
        //printf("    not found!\n");
        return false;
    }
    tag_lvfont_header hdr;
    bool res = false;
    lvsize_t bytes_read = 0;
    if (stream->Read(&hdr, sizeof(hdr), &bytes_read) == LVERR_OK && bytes_read == sizeof(hdr)) {
        LVFontDef def(
                name,
                hdr.fontHeight,
                hdr.flgBold ? 700 : 400,
                hdr.flgItalic ? true : false,
                -1,
                (css_font_family_t) hdr.fontFamily,
                lString8(hdr.fontName)
        );
        //fprintf( _log, "Register: %s %s %d %s %s\n",
        //    name.c_str(), hdr.fontName,
        //    hdr.fontHeight,
        //    hdr.flgBold?"bold":"",
        //    hdr.flgItalic?"italic":"" );
        _cache.update(&def, LVFontRef(NULL));
        res = true;
    }
    return res;
}

void LVBitmapFontManager::getFontFileNameList(lString16Collection &list) {
    FONT_MAN_GUARD
    _cache.getFontFileNameList(list);
}

bool LVBitmapFontManager::Init(lString8 path) {
    _path = path;
    return true;
}

#endif  // (USE_BITMAP_FONTS==1)
