/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2013 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2019-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020,2021 Aleksey Chernov <valexlin@gmail.com>          *
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
 * \file lvbasefont.h
 * \brief base font interface
 */

#ifndef __LV_BASEFONT_H_INCLUDED__
#define __LV_BASEFONT_H_INCLUDED__

#include <stdlib.h>
#include "crsetup.h"
#include "lvfont.h"

class LVDrawBuf;

class LVBaseFont : public LVFont {
protected:
    lString8 _typeface;
    css_font_family_t _family;
public:
    /// returns font typeface name
    virtual lString8 getTypeFace() const { return _typeface; }

    /// returns font family id
    virtual css_font_family_t getFontFamily() const { return _family; }

    /// draws text string
    virtual int DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar32 * text, int len,
                       lChar32 def_char, lUInt32 * palette = NULL,
                       bool addHyphen = false, TextLangCfg * lang_cfg = NULL,
                       lUInt32 flags=0, int letter_spacing=0, int width=-1,
                       int text_decoration_back_gap=0,
                       int target_w=-1, int target_h=-1,
                       lUInt32 fallbackPassMask = 0);
};

#endif  // __LV_BASEFONT_H_INCLUDED__
