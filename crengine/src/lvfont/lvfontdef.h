/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2011,2012 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2017,2020,2021 poire-z <poire-z@users.noreply.github.com>
 *   Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>               *
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
 * \file lvfontdef.h
 * \brief font definition interface
 */

#ifndef __LV_FONTDEF_H_INCLUDED__
#define __LV_FONTDEF_H_INCLUDED__

#include "crsetup.h"
#include "lvfont.h"
#include "lvstring.h"
#include "cssdef.h"
#include "lvarray.h"

// LVFontDef carries a font definition, and can be used to identify:
// - registered fonts, from available font files (size=-1 if scalable)
// - instantiated fonts from one of the registered fonts, with some
//   updated properties:
//     - the specific size, > -1
//     - _italic=2 (if font has no real italic, and it is synthesized
//       thanks to Freetype from the regular font glyphs)
//     - _weight=600 (updated weight if synthesized weight made from
//       the regular font glyphs)
// It can be used as a key by caches to retrieve a registered font
// or an instantiated one, and as a query to find in the cache an
// exact or an approximate font.

/**
    @brief Font properties definition
*/
class LVFontDef {
private:
    int _size;
    int _weight;
    int _italic;
    int _features; // OpenType features requested
    css_font_family_t _family;
    lString8 _typeface;
    lString8 _name;
    int _index;
    // for document font: _documentId, _buf, _name
    int _documentId;
    LVByteArrayRef _buf;
    int _bias;
    bool _real_weight;
public:
    LVFontDef(const lString8 &name, int size, int weight, int italic, int features, css_font_family_t family,
              const lString8 &typeface, int index = -1, int documentId = -1,
              LVByteArrayRef buf = LVByteArrayRef())
            : _size(size), _weight(weight), _italic(italic), _features(features), _family(family), _typeface(typeface),
              _name(name), _index(index), _documentId(documentId), _buf(buf), _bias(0), _real_weight(true) {
    }

    LVFontDef(const LVFontDef &def)
            : _size(def._size), _weight(def._weight), _italic(def._italic), _features(def._features), _family(def._family),
              _typeface(def._typeface), _name(def._name), _index(def._index),
              _documentId(def._documentId), _buf(def._buf), _bias(def._bias), _real_weight(def._real_weight) {
    }

    /// returns true if definitions are equal
    bool operator==(const LVFontDef &def) const {
        return (_size == def._size || _size == -1 || def._size == -1)
               && (_weight == def._weight || _weight == -1 || def._weight == -1)
               && _real_weight == def._real_weight
               && (_italic == def._italic || _italic == -1 || def._italic == -1)
               && _features == def._features
               && _family == def._family
               && _typeface == def._typeface
               && _name == def._name
               && (_index == def._index || def._index == -1)
               && (_documentId == def._documentId || _documentId == -1);
    }

    lUInt32 getHash() {
        return (((((_size * 31) + _weight)*31  + _italic)*31 + _features)*31+ _family)*31 + _name.getHash();
    }

    /// returns font file name
    lString8 getName() const { return _name; }

    void setName(lString8 name) { _name = name; }

    int getIndex() const { return _index; }

    void setIndex(int index) { _index = index; }

    int getSize() const { return _size; }

    void setSize(int size) { _size = size; }

    int getWeight() const { return _weight; }

    bool isRealWeight() const { return _real_weight; }

    void setWeight(int weight, bool real = true) { _weight = weight; _real_weight = real; }

    bool getItalic() const { return _italic != 0; }

    bool isRealItalic() const { return _italic == 1; }

    void setItalic(int italic) { _italic = italic; }

    css_font_family_t getFamily() const { return _family; }

    void getFamily(css_font_family_t family) { _family = family; }

    lString8 getTypeFace() const { return _typeface; }

    void setTypeFace(lString8 tf) { _typeface = tf; }

    int getFeatures() const { return _features; }

    void setFeatures( int features ) { _features = features; }

    int getDocumentId() { return _documentId; }

    void setDocumentId(int id) { _documentId = id; }

    LVByteArrayRef getBuf() { return _buf; }

    void setBuf(LVByteArrayRef buf) { _buf = buf; }

    ~LVFontDef() {}

    /// calculates difference between two fonts
    int CalcMatch(const LVFontDef &def, bool useBias) const;

    /// difference between fonts for duplicates search
    int CalcDuplicateMatch(const LVFontDef &def) const;

    /// calc match for fallback font search
    int CalcFallbackMatch(lString8 face, int size) const;

    bool setBiasIfNameMatch( lString8 facename, int bias, bool clearIfNot=true ) {
        if (_typeface.compare(facename) == 0) {
            _bias = bias;
            return true;
        }
        if (clearIfNot) {
            _bias = 0; // reset bias for other fonts
        }
        return false;
    }
};

#endif  // __LV_FONTDEF_H_INCLUDED__
