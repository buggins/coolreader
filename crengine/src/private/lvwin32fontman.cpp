/** \file lvwin32fontman.h
    \brief Win32 font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvwin32fontman.h"
#include "lvwin32font.h"

#if !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE != 1

// prototype
int CALLBACK LVWin32FontEnumFontFamExProc(
  const LOGFONTA *lpelfe,    // logical-font data
  const TEXTMETRICA *lpntme,  // physical-font data
  //ENUMLOGFONTEX *lpelfe,    // logical-font data
  //NEWTEXTMETRICEX *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
);

LVWin32FontManager::~LVWin32FontManager()
{
    //if (_log)
    //    fclose(_log);
}

LVWin32FontManager::LVWin32FontManager()
{
    //_log = fopen( "fonts.log", "wt" );
}

LVFontRef LVWin32FontManager::GetFont(int size, int weight, bool bitalic, css_font_family_t family, lString8 typeface,
                                      int features, int documentId, bool useBias)
{
    int italic = bitalic?1:0;
    if (size < 8)
        size = 8;
    if (size > 255)
        size = 255;

    LVFontDef def(
                lString8::empty_str,
                size,
                weight,
                italic,
                0,
                family,
                typeface
                );

    //fprintf( _log, "GetFont: %s %d %s %s\n",
    //    typeface.c_str(),
    //    size,
    //    weight>400?"bold":"",
    //    italic?"italic":"" );
    LVFontCacheItem * item = _cache.find( &def );
    if (!item->getFont().isNull())
    {
        //fprintf(_log, "    : fount existing\n");
        return item->getFont();
    }

#if COLOR_BACKBUFFER==0
    LVWin32Font * font = new LVWin32Font;
#else
    LVWin32DrawFont * font = new LVWin32DrawFont;
#endif

    LVFontDef * fdef = item->getDef();
    LVFontDef def2( fdef->getName(), size, weight, italic,
                    fdef->getFamily(), fdef->getTypeFace() );

    if ( font->Create(size, weight, italic?true:false, fdef->getFamily(), fdef->getTypeFace()) )
    {
        //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
        //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
        LVFontRef ref(font);
        _cache.addInstance( &def2, ref );
        return ref;
    }
    delete font;
    return LVFontRef(NULL);
}

bool LVWin32FontManager::RegisterFont(const LOGFONTA *lf)
{
    lString8 face(lf->lfFaceName);
    css_font_family_t ff;
    switch (lf->lfPitchAndFamily & 0x70)
    {
    case FF_ROMAN:
        ff = css_ff_serif;
        break;
    case FF_SWISS:
        ff = css_ff_sans_serif;
        break;
    case FF_SCRIPT:
        ff = css_ff_cursive;
        break;
    case FF_DECORATIVE:
        ff = css_ff_fantasy;
        break;
    case FF_MODERN:
        ff = css_ff_monospace;
        break;
    default:
        ff = css_ff_sans_serif;
        break;
    }
    LVFontDef def(
                face,
                -1, //lf->lfHeight>0 ? lf->lfHeight : -lf->lfHeight,
                -1, //lf->lfWeight,
                -1, //lf->lfItalic!=0,
                -1,
                ff,
                face
                );
    _cache.update( &def, LVFontRef(NULL) );
    return true;
}

bool LVWin32FontManager::Init(lString8 path)
{
    LVColorDrawBuf drawbuf(1,1);
    LOGFONTA lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    int res =
            EnumFontFamiliesExA(
                drawbuf.GetDC(),                  // handle to DC
                &lf,                              // font information
                LVWin32FontEnumFontFamExProc, // callback function (FONTENUMPROC)
                (LPARAM)this,                    // additional data
                0                     // not used; must be 0
                );

    return res!=0;
}

void LVWin32FontManager::getFontFileNameList(lString32Collection &list)
{
    FONT_MAN_GUARD
            _cache.getFontFileNameList(list);
}


// definition
int CALLBACK LVWin32FontEnumFontFamExProc(
  const LOGFONTA *lf,    // logical-font data
  const TEXTMETRICA *lpntme,  // physical-font data
  //ENUMLOGFONTEX *lpelfe,    // logical-font data
  //NEWTEXTMETRICEX *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
)
{
    //
    if (FontType == TRUETYPE_FONTTYPE)
    {
        LVWin32FontManager * fontman = (LVWin32FontManager *)lParam;
        LVWin32Font fnt;
        //if (strcmp(lf->lfFaceName, "Courier New"))
        //    return 1;
        if ( fnt.Create( *lf ) )
        {
            //
            static lChar32 chars[] = {0, 0xBF, 0xE9, 0x106, 0x410, 0x44F, 0 };
            for (int i=0; chars[i]; i++)
            {
                LVFont::glyph_info_t glyph;
                if (!fnt.getGlyphInfo( chars[i], &glyph, L' ' )) //def_char
                    return 1;
            }
            fontman->RegisterFont( lf ); //&lpelfe->elfLogFont
        }
    }
    return 1;
}

#endif  // !defined(__SYMBIAN32__) && defined(_WIN32) && USE_FREETYPE != 1
