/** \file lvfontdef.cpp
    @brief font definition implemetation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfontdef.h"


int LVFontDef::CalcDuplicateMatch(const LVFontDef &def) const {
    if (def._documentId != -1 && _documentId != def._documentId)
        return false;
    bool size_match = (_size == -1 || def._size == -1) ? true
                                                       : (def._size == _size);
    bool weight_match = (_weight == -1 || def._weight == -1) ? true
                                                             : (def._weight == _weight);
    bool italic_match = (_italic == def._italic || _italic == -1 || def._italic == -1);
    bool family_match = (_family == css_ff_inherit || def._family == css_ff_inherit ||
                         def._family == _family);
    bool typeface_match = (_typeface == def._typeface);
    return size_match && weight_match && italic_match && family_match && typeface_match;
}

int LVFontDef::CalcMatch(const LVFontDef &def) const {
    if (_documentId != -1 && _documentId != def._documentId)
        return 0;
    int size_match = (_size == -1 || def._size == -1) ? 256
                                                      : (def._size > _size ? _size * 256 / def._size
                                                                           : def._size * 256 /
                                                                             _size);
    int weight_diff = def._weight - _weight;
    if (weight_diff < 0)
        weight_diff = -weight_diff;
    if (weight_diff > 800)
        weight_diff = 800;
    int weight_match = (_weight == -1 || def._weight == -1) ? 256
                                                            : (256 - weight_diff * 256 / 800);
    int italic_match = (_italic == def._italic || _italic == -1 || def._italic == -1) ? 256 : 0;
    if ((_italic == 2 || def._italic == 2) && _italic > 0 && def._italic > 0)
        italic_match = 128;
    int family_match = (_family == css_ff_inherit || def._family == css_ff_inherit ||
                        def._family == _family)
                       ? 256
                       : ((_family == css_ff_monospace) == (def._family == css_ff_monospace) ? 64
                                                                                             : 0);
    int typeface_match = (_typeface == def._typeface) ? 256 : 0;
    return
            +(size_match * 100)
            + (weight_match * 5)
            + (italic_match * 5)
            + (family_match * 100)
            + (typeface_match * 1000);
}

int LVFontDef::CalcFallbackMatch(lString8 face, int size) const {
    if (_typeface != face) {
        //CRLog::trace("'%s'' != '%s'", face.c_str(), _typeface.c_str());
        return 0;
    }
    int size_match = (_size == -1 || size == -1 || _size == size) ? 256 : 0;
    int weight_match = (_weight == -1) ? 256 : (256 - _weight * 256 / 800);
    int italic_match = _italic == 0 ? 256 : 0;
    return
            +(size_match * 100)
            + (weight_match * 5)
            + (italic_match * 5);
}
