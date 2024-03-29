/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2010-2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2017,2019-2021 poire-z <poire-z@users.noreply.github.com>
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

#include "lvfontdef.h"


int LVFontDef::CalcDuplicateMatch(const LVFontDef &def) const {
    if (def._documentId != -1 && _documentId != def._documentId)
        return false;
    bool size_match = (_size == -1 || def._size == -1) ? true
                                                       : (def._size == _size);
    bool weight_match = (_weight == -1 || def._weight == -1) ? true
                                                             : (def._weight == _weight);
    bool italic_match = (_italic == def._italic || _italic == -1 || def._italic == -1);

    bool features_match = (_features == def._features || _features==-1 || def._features==-1);

    bool family_match = (_family == css_ff_inherit || def._family == css_ff_inherit ||
                         def._family == _family);
    bool typeface_match = (_typeface == def._typeface);
    return size_match && weight_match && italic_match && features_match && family_match && typeface_match;
}

int LVFontDef::CalcMatch(const LVFontDef &def, bool useBias) const {
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
    // It might happen that 2 fonts with different weights can get the same
    // score, e.g. with def._weight=550, a font with _weight=400 and an other
    // with _weight=700. Any could then be picked depending on their random
    // ordering in the cache, which may mess a book on re-openings.
    // To avoid this inconsistency, we give arbitrarily a small increase to
    // the score of the smaller weight font (mostly so that with the above
    // case, we keep synthesizing the 550 from the 400)
    if ( _weight < def._weight )
        weight_match += 1;
    int italic_match = (_italic == def._italic || _italic == -1 || def._italic == -1) ? 256 : 0;
    if ((_italic == 2 || def._italic == 2) && _italic > 0 && def._italic > 0)
        italic_match = 128;
    // OpenType features
    int features_match = (_features == def._features || _features==-1 || def._features==-1) ?
                256
                :   0;
    int family_match = (_family == css_ff_inherit || def._family == css_ff_inherit ||
                        def._family == _family)
                       ? 256
                       : ((_family == css_ff_monospace) == (def._family == css_ff_monospace) ? 64
                                                                                             : 0);
    int typeface_match = (_typeface == def._typeface) ? 256 : 0;

    // bias
    int bias = useBias ? _bias : 0;

    // Special handling for synthesized fonts:
    // The way this function is called:
    // 'this' (or '', properties not prefixed) is either an instance of a
    //     registered font, or a registered font definition,
    // 'def' is the requested definition.
    // 'def' can never be italic=2 (fake italic) or !_real_weight (fake weight), but
    //    either 0 or 1, or a 400, 500, 600, 700, ... for standard weight
    //    or any multiple of 25 for synthetic weights
    // 'this' registered can be only 400 when the font has no bold sibling,
    //           or any other real weight: 500, 600, 700 ...
    // 'this' instantiated can be 400 (for the regular original)
    //           or 700 when 'this' is the bold sibling instantiated
    //           or any other value with disabled flags _real_weight
    //           when it has been synthesized from the other font with real weight.
    // We want to avoid an instantiated fake bold (resp. fake bold italic) to
    // have a higher score than the registered original when a fake bold italic
    // (resp. fake bold) is requested, so the italic/non italic requested can
    // be re-synthesized. Otherwise, we'll get some italic when not wanting
    // italic (or vice versa), depending on which has been instantiated first...
    //
    if ( !_real_weight ) {        // 'this' is an instantiated fake weight font
        if ( def._italic > 0 ) {  // italic requested
            if ( _italic == 0 ) { // 'this' is fake bold but non-italic
                weight_match = 0; // => drop score
                italic_match = 0;
                // The regular (italic or fake italic) is available
                // and will get a better score than 'this'
            }
            // otherwise, 'this' is a fake bold italic, and it can match
        }
        else {                    // non-italic requested
            if ( _italic > 0 ) {  // 'this' is fake bold of (real or fake) italic
                weight_match = 0; // => drop score
                italic_match = 0;
                // The regular is available and will get a better score
                // than 'this'
            }
        }
        // Also, never use a synthetic weight font to synthesize another one
        if ( weight_diff >= 25 ) {
            weight_match = 0;
            italic_match = 0;
        }
    }

    // final score
    int score = bias
        + (size_match     * 100)
        + (weight_match   * 5)
        + (italic_match   * 5)
        + (features_match * 1000)
        + (family_match   * 100)
        + (typeface_match * 1000);

    return score;
}

int LVFontDef::CalcFallbackMatch(lString8 face, int size) const {
    if (_typeface != face) {
        //CRLog::trace("'%s'' != '%s'", face.c_str(), _typeface.c_str());
        return 0;
    }
    int size_match = (_size == -1 || size == -1 || _size == size) ? 256 : 0;
    int weight_match = (_weight == -1) ? 256 : (256 - _weight * 256 / 800);
    int italic_match = _italic == 0 ? 256 : 0;
    // Don't let instantiated font with non-zero features be usable as a fallback font
    int features_match = (_features == -1 || _features == 0 ) ?
                256
                :   0;
    return
            +(size_match * 100)
            + (weight_match * 5)
            + (italic_match * 5)
            + (features_match * 1000);
}
