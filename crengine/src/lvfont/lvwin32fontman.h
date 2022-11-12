/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020,2022 Aleksey Chernov <valexlin@gmail.com>          *
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
 * \file lvwin32fontman.h
 * \brief Win32 font manager interface
 */

#ifndef __LV_WIN32FONTMAN_H_INCLUDED__
#define __LV_WIN32FONTMAN_H_INCLUDED__

#include "crsetup.h"
#include "lvfntman.h"

#include "lvfontcache.h"
#include "lvfontglyphcache.h"


#if !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE != 1

class LVWin32FontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
    LVFontGlobalGlyphCache _globalCache;
    //FILE * _log;
public:
    virtual int GetFontCount()
    {
        return _cache.length();
    }
    virtual ~LVWin32FontManager();
    LVWin32FontManager();
    virtual void gc() // garbage collector
    {
        _cache.gc();
    }
    virtual LVFontRef GetFont(int size, int weight, bool bitalic, css_font_family_t family, lString8 typeface,
                              int features=0, int documentId = -1, bool useBias=false);
    virtual void GetAvailableFontWeights(LVArray<int>& weights, lString8 typeface) {}
    virtual bool RegisterFont( const LOGFONTA * lf );
    virtual bool RegisterFont( lString8 name )
    {
        return false;
    }
    virtual bool Init( lString8 path );
    /// clear glyph cache
    virtual void clearGlyphCache();
    virtual void getFaceList( lString32Collection & list )
    {
        _cache.getFaceList(list);
    }
    /// returns registered font files
    virtual void getFontFileNameList( lString32Collection & list );
};

#endif  // !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE!=1

#endif  // __LV_WIN32FONTMAN_H_INCLUDED__
