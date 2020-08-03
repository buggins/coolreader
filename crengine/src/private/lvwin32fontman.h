/** \file lvwin32fontman.cpp
    \brief Win32 font manager interface

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_WIN32FONTMAN_H_INCLUDED__
#define __LV_WIN32FONTMAN_H_INCLUDED__

#include "../../include/crsetup.h"
#include "../../include/lvfntman.h"

#include "lvfontcache.h"


#if !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE != 1

class LVWin32FontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
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

    virtual bool RegisterFont( const LOGFONTA * lf );
    virtual bool RegisterFont( lString8 name )
    {
        return false;
    }
    virtual bool Init( lString8 path );

    virtual void getFaceList( lString16Collection & list )
    {
        _cache.getFaceList(list);
    }
    /// returns registered font files
    virtual void getFontFileNameList( lString16Collection & list );
};

#endif  // !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE!=1

#endif  // __LV_WIN32FONTMAN_H_INCLUDED__
