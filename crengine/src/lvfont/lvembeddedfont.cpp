/** @file lvembeddedfont.cpp
    @brief embedded font definition implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvembeddedfont.h"
#include "serialbuf.h"

#include <stdlib.h>

static const char *EMBEDDED_FONT_LIST_MAGIC = "FNTL";
static const char *EMBEDDED_FONT_DEF_MAGIC = "FNTD";

////////////////////////////////////////////////////////////////////
// LVEmbeddedFontDef
////////////////////////////////////////////////////////////////////
bool LVEmbeddedFontDef::serialize(SerialBuf &buf) {
    buf.putMagic(EMBEDDED_FONT_DEF_MAGIC);
    buf << _url << _face << _bold << _italic;
    return !buf.error();
}

bool LVEmbeddedFontDef::deserialize(SerialBuf &buf) {
    if (!buf.checkMagic(EMBEDDED_FONT_DEF_MAGIC))
        return false;
    buf >> _url >> _face >> _bold >> _italic;
    return !buf.error();
}

////////////////////////////////////////////////////////////////////
// LVEmbeddedFontList
////////////////////////////////////////////////////////////////////
LVEmbeddedFontDef *LVEmbeddedFontList::findByUrl(lString32 url) {
    for (int i = 0; i < length(); i++) {
        if (get(i)->getUrl() == url)
            return get(i);
    }
    return NULL;
}

bool LVEmbeddedFontList::addAll(LVEmbeddedFontList &list) {
    bool changed = false;
    for (int i = 0; i < list.length(); i++) {
        LVEmbeddedFontDef *def = list.get(i);
        changed = add(def->getUrl(), def->getFace(), def->getBold(), def->getItalic()) || changed;
    }
    return changed;
}

bool LVEmbeddedFontList::add(lString32 url, lString8 face, bool bold, bool italic) {
    LVEmbeddedFontDef *def = findByUrl(url);
    if (def) {
        bool changed = false;
        if (def->getFace() != face) {
            def->setFace(face);
            changed = true;
        }
        if (def->getBold() != bold) {
            def->setBold(bold);
            changed = true;
        }
        if (def->getItalic() != italic) {
            def->setItalic(italic);
            changed = true;
        }
        return changed;
    }
    def = new LVEmbeddedFontDef(url, face, bold, italic);
    add(def);
    return false;
}

bool LVEmbeddedFontList::serialize(SerialBuf &buf) {
    buf.putMagic(EMBEDDED_FONT_LIST_MAGIC);
    lUInt32 count = length();
    buf << count;
    for (lUInt32 i = 0; i < count; i++) {
        get(i)->serialize(buf);
        if (buf.error())
            return false;
    }
    return !buf.error();
}

bool LVEmbeddedFontList::deserialize(SerialBuf &buf) {
    if (!buf.checkMagic(EMBEDDED_FONT_LIST_MAGIC))
        return false;
    lUInt32 count = 0;
    buf >> count;
    if (buf.error())
        return false;
    for (lUInt32 i = 0; i < count; i++) {
        LVEmbeddedFontDef *item = new LVEmbeddedFontDef();
        if (!item->deserialize(buf)) {
            delete item;
            return false;
        }
        add(item);
    }
    return !buf.error();
}
