/** @file lvfntman.h
    @brief font manager interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_FNT_MAN_H_INCLUDED__
#define __LV_FNT_MAN_H_INCLUDED__

#include <stdlib.h>
#include "crsetup.h"
#include "lvstring.h"
#include "lvstring32collection.h"
#include "lvfont.h"
#include "lvarray.h"
#include "lvcontainer.h"

/// font manager interface class
class LVFontManager {
protected:
    bool _allowKerning;
    int _antialiasMode;
    shaping_mode_t _shapingMode;
    hinting_mode_t _hintingMode;
public:
    /// garbage collector frees unused fonts
    virtual void gc() = 0;

    /// returns most similar font
    virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface,
                                int features=0, int documentId = -1, bool useBias=false) = 0;

    /// return available font weight values
    virtual void GetAvailableFontWeights(LVArray<int>& weights, lString8 typeface) = 0;

    /// set fallback font face list semicolon separated (returns true if any font is found)
    virtual bool SetFallbackFontFaces( lString8 faces ) {
        CR_UNUSED(faces);
        return false;
    }

    /// get fallback font face (returns empty string if no fallback font is set)
    virtual lString8 GetFallbackFontFace(int index = 0) {
        CR_UNUSED(index);
        return lString8::empty_str;
    }

    /// get fallback font faces list (returns empty string list collection if no fallback fonts is set)
    virtual lString8 GetFallbackFontFaces() { return lString8::empty_str; }

    /// get count of fallback fonts
    virtual int GetFallbackFontCount() { return 0; }

    /// returns fallback font for specified size
    virtual LVFontRef GetFallbackFont(int size, int index=0) {
        CR_UNUSED2(size, index);
        return LVFontRef();
    }
    /// returns fallback font for specified size, weight and italic
    virtual LVFontRef GetFallbackFont(int size, int weight=400, bool italic=false, int index=0 ) {
        CR_UNUSED4(size, weight, italic, index);
        return LVFontRef();
    }
    /// registers font by name
    virtual bool RegisterFont( lString8 name ) = 0;
    /// registers font by name and face
    virtual bool RegisterExternalFont(int /*documentId*/, lString32 /*name*/, lString8 /*face*/, bool /*bold*/, bool /*italic*/) { return false; }
    /// registers document font
    virtual bool
    RegisterDocumentFont(int /*documentId*/, LVContainerRef /*container*/, lString32 /*name*/,
                         lString8 /*face*/, bool /*bold*/, bool /*italic*/) { return false; }

    /// unregisters all document fonts
    virtual void UnregisterDocumentFonts(int /*documentId*/) {}

    /// initializes font manager
    virtual bool Init(lString8 path) = 0;

    /// get count of registered fonts
    virtual int GetFontCount() = 0;

    /// get hash of installed fonts and fallback font
    virtual lUInt32 GetFontListHash(int /*documentId*/) { return 0; }

    /// clear glyph cache
    virtual void clearGlyphCache() {}

    /// get antialiasing mode
    virtual int GetAntialiasMode() { return _antialiasMode; }

    /// set antialiasing mode
    virtual void SetAntialiasMode(int mode) {
        _antialiasMode = mode;
        gc();
        clearGlyphCache();
    }

    /// get kerning mode: true==ON, false=OFF
    virtual bool GetKerning() { return _allowKerning; }

    /// get kerning mode: true==ON, false=OFF
    virtual void SetKerning(bool kerningEnabled) {
        _allowKerning = kerningEnabled;
        gc();
        clearGlyphCache();
    }

    /// get shaping mode
    virtual shaping_mode_t GetShapingMode() { return _shapingMode; }
    /// set shaping mode
    virtual void SetShapingMode( shaping_mode_t mode ) { _shapingMode = mode; gc(); clearGlyphCache(); }
    /// constructor
    LVFontManager() : _allowKerning(false), _antialiasMode(font_aa_all), _shapingMode(SHAPING_MODE_FREETYPE), _hintingMode(HINTING_MODE_AUTOHINT) { }
    /// destructor
    virtual ~LVFontManager() { }
    /// returns available typefaces
    virtual void getFaceList( lString32Collection & ) { }
    /// returns available font files
    virtual void getFontFileNameList( lString32Collection & ) { }
    /// check font language compatibility
    virtual bool checkFontLangCompat(const lString8 &typeface, const lString8 &langCode) { return true; }
    /// returns first found face from passed list, or return face for font found by family only
    virtual lString8 findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily);
    /// fills array with list of available gamma levels
    virtual void GetGammaLevels(LVArray<double> dst);
    /// returns current gamma level index
    virtual int  GetGammaIndex();
    /// sets current gamma level index
    virtual void SetGammaIndex( int gammaIndex );
    /// returns current gamma level
    virtual double GetGamma();
    /// sets current gamma level
    virtual void SetGamma( double gamma );
    /// sets current hinting mode
    virtual void SetHintingMode(hinting_mode_t /*mode*/) { }
    /// returns current hinting mode
    virtual hinting_mode_t  GetHintingMode() { return HINTING_MODE_AUTOHINT; }
    ///
    virtual bool SetAlias(lString8 alias, lString8 facename, int id, bool italic, bool bold) {
        CR_UNUSED5(alias, facename, id, italic, bold);
        return false;
    }
    /// set as preferred font with the given bias to add in CalcMatch algorithm
    virtual bool SetAsPreferredFontWithBias( lString8 face, int bias, bool clearOthersBias=true ) {
        CR_UNUSED(face);
        return false;
    }
};

#define LVFONT_TRANSFORM_EMBOLDEN 1

/// create transform for font
LVFontRef LVCreateFontTransform(LVFontRef baseFont, int transformFlags);

/// current font manager pointer
extern LVFontManager *fontMan;

/// initializes font manager
bool InitFontManager(lString8 path);

/// deletes font manager
bool ShutdownFontManager();

LVFontRef LoadFontFromFile(const char *fname);

#endif //__LV_FNT_MAN_H_INCLUDED__
