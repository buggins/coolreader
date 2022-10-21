/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>           *
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
 * \file lvembeddedfont.h
 * \brief embedded font definition interface
 */

#ifndef __LV_EMBEDDEDFONT_H_INCLUDED__
#define __LV_EMBEDDEDFONT_H_INCLUDED__

#include "crsetup.h"
#include "lvstring.h"
#include "lvptrvec.h"

class SerialBuf;

class LVEmbeddedFontDef {
    lString32 _url;
    lString8 _face;
    bool _bold;
    bool _italic;
public:
    LVEmbeddedFontDef(lString32 url, lString8 face, bool bold, bool italic) :
            _url(url), _face(face), _bold(bold), _italic(italic) {
    }

    LVEmbeddedFontDef() : _bold(false), _italic(false) {
    }

    const lString32 &getUrl() { return _url; }

    const lString8 &getFace() { return _face; }

    bool getBold() { return _bold; }

    bool getItalic() { return _italic; }

    void setFace(const lString8 &face) { _face = face; }

    void setBold(bool bold) { _bold = bold; }

    void setItalic(bool italic) { _italic = italic; }

    bool serialize(SerialBuf &buf);

    bool deserialize(SerialBuf &buf);
};

class LVEmbeddedFontList : public LVPtrVector<LVEmbeddedFontDef> {
public:
    LVEmbeddedFontDef *findByUrl(lString32 url);

    void add(LVEmbeddedFontDef *def) { LVPtrVector<LVEmbeddedFontDef>::add(def); }

    bool add(lString32 url, lString8 face, bool bold, bool italic);

    bool add(lString32 url) { return add(url, lString8::empty_str, false, false); }

    bool addAll(LVEmbeddedFontList &list);

    void set(LVEmbeddedFontList &list) {
        clear();
        addAll(list);
    }

    bool serialize(SerialBuf &buf);

    bool deserialize(SerialBuf &buf);
};

#endif // __LV_EMBEDDEDFONT_H_INCLUDED__
