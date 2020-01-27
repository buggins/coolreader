/** \file lvfreetypefontman.cpp
    \brief FreeType font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvfreetypefontman.h"
#include "lvfreetypeface.h"
#include "lvfontboldtransform.h"
#include "../../include/crlog.h"

#if (USE_FONTCONFIG == 1)
#include <fontconfig/fontconfig.h>
#endif

#if (USE_FREETYPE == 1)

lString8 familyName(FT_Face face) {
    lString8 faceName(face->family_name);
    if (faceName == "Arial" && face->style_name && !strcmp(face->style_name, "Narrow"))
        faceName << " " << face->style_name;
    else if ( /*faceName == "Arial" &&*/ face->style_name && strstr(face->style_name, "Condensed"))
        faceName << " " << "Condensed";
    return faceName;
}


lUInt32 LVFreeTypeFontManager::GetFontListHash(int documentId) {
    FONT_MAN_GUARD
    return _cache.GetFontListHash(documentId) * 75 + _fallbackFontFace.getHash();
}

bool LVFreeTypeFontManager::SetFallbackFontFace(lString8 face) {
    FONT_MAN_GUARD
    if (face != _fallbackFontFace) {
        CRLog::trace("Looking for fallback font %s", face.c_str());
        LVFontCacheItem *item = _cache.findFallback(face, -1);
        if (!item) {
            face.clear();
            // Don't reset previous fallback if this one is not found/valid
            return false;
        }
        _cache.clearFallbackFonts();
        _fallbackFontFace = face;
        // Somehow, with Fedra Serif (only!), changing the fallback font does
        // not prevent glyphs from previous fallback font to be re-used...
        // So let's clear glyphs caches too.
        gc();
        clearGlyphCache();
    }
    return !_fallbackFontFace.empty();
}

LVFontRef LVFreeTypeFontManager::GetFallbackFont(int size) {
    FONT_MAN_GUARD
    if ( _fallbackFontFace.empty() )
        return LVFontRef();
    // reduce number of possible distinct sizes for fallback font
    if (size > 40)
        size &= 0xFFF8;
    else if (size > 28)
        size &= 0xFFFC;
    else if (size > 16)
        size &= 0xFFFE;
    LVFontCacheItem *item = _cache.findFallback(_fallbackFontFace, size);
    if (!item->getFont().isNull())
        return item->getFont();
    return GetFont(size, 400, false, css_ff_sans_serif, _fallbackFontFace, -1);
}

LVFontRef LVFreeTypeFontManager::GetFallbackFont(int size, int weight, bool italic )
{
    FONT_MAN_GUARD
    if ( _fallbackFontFace.empty() )
        return LVFontRef();
    // reduce number of possible distinct sizes for fallback font
    if ( size>40 )
        size &= 0xFFF8;
    else if ( size>28 )
        size &= 0xFFFC;
    else if ( size>16 )
        size &= 0xFFFE;
    // We don't use/extend findFallback(), which was made to work
    // assuming the fallback font is a standalone regular font
    // without any bold/italic sibling.
    // GetFont() works just as fine when we need specified weigh and italic.
    return GetFont(size, weight, italic, css_ff_sans_serif, _fallbackFontFace, -1);
}

bool LVFreeTypeFontManager::isBitmapModeForSize(int size) {
    bool isBitmap = false;
    switch (_antialiasMode) {
        case font_aa_none:
            isBitmap = true;
            break;
        case font_aa_big:
            isBitmap = size < 20 ? true : false;
            break;
        case font_aa_all:
        default:
            isBitmap = false;
            break;
    }
    return isBitmap;
}

void LVFreeTypeFontManager::SetAntialiasMode(int mode) {
    _antialiasMode = mode;
    gc();
    clearGlyphCache();
    FONT_MAN_GUARD
    LVPtrVector<LVFontCacheItem> *fonts = _cache.getInstances();
    for (int i = 0; i < fonts->length(); i++) {
        fonts->get(i)->getFont()->setBitmapMode(
                isBitmapModeForSize(fonts->get(i)->getFont()->getHeight()));
    }
}

void LVFreeTypeFontManager::SetHintingMode(hinting_mode_t mode) {
    FONT_MAN_GUARD
    CRLog::debug("Hinting mode is changed: %d", (int) mode);
    _hintingMode = mode;
    gc();
    clearGlyphCache();
    LVPtrVector<LVFontCacheItem> *fonts = _cache.getInstances();
    for (int i = 0; i < fonts->length(); i++) {
        fonts->get(i)->getFont()->setHintingMode(mode);
    }
}

void LVFreeTypeFontManager::SetKerning(bool kerningEnabled)
{
    FONT_MAN_GUARD
    CRLog::debug("Kerning mode is changed: %d", (int) kerningEnabled);
    _allowKerning = kerningEnabled;
    gc();
    clearGlyphCache();
    LVPtrVector<LVFontCacheItem> *fonts = _cache.getInstances();
    for (int i = 0; i < fonts->length(); i++) {
        fonts->get(i)->getFont()->setKerning(kerningEnabled);
    }
}

void LVFreeTypeFontManager::SetShapingMode( shaping_mode_t mode )
{
    FONT_MAN_GUARD
    CRLog::debug("Shaping mode is changed: %d", (int) mode);
    _shapingMode = mode;
    gc();
    clearGlyphCache();
    LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
    for ( int i=0; i<fonts->length(); i++ ) {
        fonts->get(i)->getFont()->setShapingMode( mode );
    }
}

void LVFreeTypeFontManager::clearGlyphCache() {
    FONT_MAN_GUARD
    _globalCache.clear();
    #if USE_HARFBUZZ==1
    // needs to clear each font _glyph_cache2 (for Gamma change, which
    // does not call any individual font method)
    LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
    for ( int i=0; i<fonts->length(); i++ ) {
        fonts->get(i)->getFont()->clearCache();
    }
    #endif
}

bool LVFreeTypeFontManager::initSystemFonts() {
#if (DEBUG_FONT_SYNTHESIS == 1)
    fontMan->RegisterFont(lString8("/usr/share/fonts/liberation/LiberationSans-Regular.ttf"));
    CRLog::debug("fonts:");
    LVFontRef fnt4 = dumpFontRef( fontMan->GetFont(24, 200, true, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
    LVFontRef fnt1 = dumpFontRef( fontMan->GetFont(18, 200, false, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
    LVFontRef fnt2 = dumpFontRef( fontMan->GetFont(20, 400, false, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
    LVFontRef fnt3 = dumpFontRef( fontMan->GetFont(22, 600, false, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
    LVFontRef fnt5 = dumpFontRef( fontMan->GetFont(26, 400, true, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
    LVFontRef fnt6 = dumpFontRef( fontMan->GetFont(28, 600, true, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
    CRLog::debug("end of font testing");
#elif (USE_FONTCONFIG == 1)
    {
        CRLog::info("Reading list of system fonts using FONTCONFIG");
        lString16Collection fonts;
        
        int facesFound = 0;
        
        FcFontSet *fontset;
        
        FcObjectSet *os = FcObjectSetBuild(FC_FILE, FC_WEIGHT, FC_FAMILY,
                                           FC_SLANT, FC_SPACING, FC_INDEX,
                                           FC_STYLE, NULL);
        FcPattern *pat = FcPatternCreate();
        //FcBool b = 1;
        FcPatternAddBool(pat, FC_SCALABLE, 1);
        
        fontset = FcFontList(NULL, pat, os);
        
        FcPatternDestroy(pat);
        FcObjectSetDestroy(os);
        
        // load fonts from file
        CRLog::debug("FONTCONFIG: %d font files found", fontset->nfont);
        for(int i = 0; i < fontset->nfont; i++) {
            FcChar8 *s=(FcChar8*)"";
            FcChar8 *family=(FcChar8*)"";
            FcChar8 *style=(FcChar8*)"";
            //FcBool b;
            FcResult res;
            //FC_SCALABLE
            //res = FcPatternGetBool( fontset->fonts[i], FC_OUTLINE, 0, (FcBool*)&b);
            //if(res != FcResultMatch)
            //    continue;
            //if ( !b )
            //    continue; // skip non-scalable fonts
            res = FcPatternGetString(fontset->fonts[i], FC_FILE, 0, (FcChar8 **)&s);
            if(res != FcResultMatch) {
                continue;
            }
            lString8 fn( (const char *)s );
            lString16 fn16( fn.c_str() );
            fn16.lowercase();
            if (!fn16.endsWith(".ttf") && !fn16.endsWith(".odf") && !fn16.endsWith(".otf") && !fn16.endsWith(".pfb") && !fn16.endsWith(".pfa")  ) {
                continue;
            }
            int weight = FC_WEIGHT_MEDIUM;
            res = FcPatternGetInteger(fontset->fonts[i], FC_WEIGHT, 0, &weight);
            if(res != FcResultMatch) {
                CRLog::debug("no FC_WEIGHT for %s", s);
                //continue;
            }
            switch ( weight ) {
            case FC_WEIGHT_THIN:          //    0
                weight = 100;
                break;
            case FC_WEIGHT_EXTRALIGHT:    //    40
                //case FC_WEIGHT_ULTRALIGHT        FC_WEIGHT_EXTRALIGHT
                weight = 200;
                break;
            case FC_WEIGHT_LIGHT:         //    50
            case FC_WEIGHT_BOOK:          //    75
            case FC_WEIGHT_REGULAR:       //    80
                //case FC_WEIGHT_NORMAL:            FC_WEIGHT_REGULAR
                weight = 400;
                break;
            case FC_WEIGHT_MEDIUM:        //    100
                weight = 500;
                break;
            case FC_WEIGHT_DEMIBOLD:      //    180
                //case FC_WEIGHT_SEMIBOLD:          FC_WEIGHT_DEMIBOLD
                weight = 600;
                break;
            case FC_WEIGHT_BOLD:          //    200
                weight = 700;
                break;
            case FC_WEIGHT_EXTRABOLD:     //    205
                //case FC_WEIGHT_ULTRABOLD:         FC_WEIGHT_EXTRABOLD
                weight = 800;
                break;
            case FC_WEIGHT_BLACK:         //    210
                //case FC_WEIGHT_HEAVY:             FC_WEIGHT_BLACK
                weight = 900;
                break;
#ifdef FC_WEIGHT_EXTRABLACK
            case FC_WEIGHT_EXTRABLACK:    //    215
                //case FC_WEIGHT_ULTRABLACK:        FC_WEIGHT_EXTRABLACK
                weight = 900;
                break;
#endif
            default:
                weight = 400;
                break;
            }
            FcBool scalable = 0;
            res = FcPatternGetBool(fontset->fonts[i], FC_SCALABLE, 0, &scalable);
            int index = 0;
            res = FcPatternGetInteger(fontset->fonts[i], FC_INDEX, 0, &index);
            if(res != FcResultMatch) {
                CRLog::debug("no FC_INDEX for %s", s);
                //continue;
            }
            res = FcPatternGetString(fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **)&family);
            if(res != FcResultMatch) {
                CRLog::debug("no FC_FAMILY for %s", s);
                continue;
            }
            res = FcPatternGetString(fontset->fonts[i], FC_STYLE, 0, (FcChar8 **)&style);
            if(res != FcResultMatch) {
                CRLog::debug("no FC_STYLE for %s", s);
                style = (FcChar8*)"";
                //continue;
            }
            int slant = FC_SLANT_ROMAN;
            res = FcPatternGetInteger(fontset->fonts[i], FC_SLANT, 0, &slant);
            if(res != FcResultMatch) {
                CRLog::debug("no FC_SLANT for %s", s);
                //continue;
            }
            int spacing = 0;
            res = FcPatternGetInteger(fontset->fonts[i], FC_SPACING, 0, &spacing);
            if(res != FcResultMatch) {
                //CRLog::debug("no FC_SPACING for %s", s);
                //continue;
            }
            //                int cr_weight;
            //                switch(weight) {
            //                    case FC_WEIGHT_LIGHT: cr_weight = 200; break;
            //                    case FC_WEIGHT_MEDIUM: cr_weight = 300; break;
            //                    case FC_WEIGHT_DEMIBOLD: cr_weight = 500; break;
            //                    case FC_WEIGHT_BOLD: cr_weight = 700; break;
            //                    case FC_WEIGHT_BLACK: cr_weight = 800; break;
            //                    default: cr_weight=300; break;
            //                }
            css_font_family_t fontFamily = css_ff_sans_serif;
            lString16 face16((const char *)family);
            face16.lowercase();
            if ( spacing==FC_MONO )
                fontFamily = css_ff_monospace;
            else if (face16.pos("sans") >= 0)
                fontFamily = css_ff_sans_serif;
            else if (face16.pos("serif") >= 0)
                fontFamily = css_ff_serif;
            
            //css_ff_inherit,
            //css_ff_serif,
            //css_ff_sans_serif,
            //css_ff_cursive,
            //css_ff_fantasy,
            //css_ff_monospace,
            bool italic = (slant!=FC_SLANT_ROMAN);
            
            lString8 face((const char*)family);
            lString16 style16((const char*)style);
            style16.lowercase();
            if (style16.pos("condensed") >= 0)
                face << " Condensed";
            else if (style16.pos("extralight") >= 0)
                face << " Extra Light";
            
            LVFontDef def(
                        lString8((const char*)s),
                        -1, // height==-1 for scalable fonts
                        weight,
                        italic,
                        fontFamily,
                        face,
                        index
                        );
            
            CRLog::debug("FONTCONFIG: Font family:%s style:%s weight:%d slant:%d spacing:%d file:%s", family, style, weight, slant, spacing, s);
            if ( _cache.findDuplicate( &def ) ) {
                CRLog::debug("is duplicate, skipping");
                continue;
            }
            _cache.update( &def, LVFontRef(NULL) );
            
            if ( scalable && !def.getItalic() ) {
                LVFontDef newDef( def );
                newDef.setItalic(2); // can italicize
                if ( !_cache.findDuplicate( &newDef ) )
                    _cache.update( &newDef, LVFontRef(NULL) );
            }
            
            facesFound++;
            
            
        }
        
        FcFontSetDestroy(fontset);
        CRLog::info("FONTCONFIG: %d fonts registered", facesFound);
        
        const char * fallback_faces [] = {
            "Arial Unicode MS",
            "AR PL ShanHeiSun Uni",
            "Liberation Sans",
            "Roboto",
            "DejaVu Sans",
            "Noto Sans",
            "Droid Sans",
            NULL
        };
        
        for ( int i=0; fallback_faces[i]; i++ )
            if ( SetFallbackFontFace(lString8(fallback_faces[i])) ) {
                CRLog::info("Fallback font %s is found", fallback_faces[i]);
                break;
            } else {
                CRLog::trace("Fallback font %s is not found", fallback_faces[i]);
            }
        
        return facesFound > 0;
    }
#elif (CR3_OSX == 1)

    int facesFound = 0;
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Bold.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Italic.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Bold Italic.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Unicode.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Narrow.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Narrow Bold.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Arial Narrow Italic.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Courier New.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Courier New Bold.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Courier New Italic.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Courier New Bold Italic.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Georgia.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Georgia Bold.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Georgia Italic.ttf"));
    facesFound += fontMan->RegisterFont(lString8("/Library/Fonts/Georgia Bold Italic.ttf"));
    
    return facesFound > 0;

#else
    return false;
#endif
}

LVFreeTypeFontManager::~LVFreeTypeFontManager() {
    FONT_MAN_GUARD
    _globalCache.clear();
    _cache.clear();
    if (_library)
        FT_Done_FreeType(_library);
#if (DEBUG_FONT_MAN == 1)
    if ( _log ) {
        fclose(_log);
    }
#endif
}

LVFreeTypeFontManager::LVFreeTypeFontManager()
        : _library(NULL), _globalCache(GLYPH_CACHE_SIZE) {
    FONT_MAN_GUARD
    int error = FT_Init_FreeType(&_library);
    if (error) {
        // error
        CRLog::error("Error while initializing freetype library");
    }
#if (DEBUG_FONT_MAN == 1)
    _log = fopen(DEBUG_FONT_MAN_LOG_FILE, "at");
    if ( _log ) {
        fprintf(_log, "=========================== LOGGING STARTED ===================\n");
    }
#endif
    // _requiredChars = L"azAZ09";//\x0410\x042F\x0430\x044F";
    // Some fonts come without any of these (ie. NotoSansMyanmar.ttf), there's
    // no reason to prevent them from being used.
    // So, check only for the presence of the space char, hoping it's there in any font.
    _requiredChars = L" ";
}

void LVFreeTypeFontManager::gc() // garbage collector
{
    FONT_MAN_GUARD
    _cache.gc();
}

lString8 LVFreeTypeFontManager::makeFontFileName(lString8 name) {
    lString8 filename = _path;
    if (!filename.empty() && _path[_path.length() - 1] != PATH_SEPARATOR_CHAR)
        filename << PATH_SEPARATOR_CHAR;
    filename << name;
    return filename;
}

void LVFreeTypeFontManager::getFaceList(lString16Collection &list) {
    FONT_MAN_GUARD
    _cache.getFaceList(list);
}

void LVFreeTypeFontManager::getFontFileNameList(lString16Collection &list) {
    FONT_MAN_GUARD
    _cache.getFontFileNameList(list);
}

bool LVFreeTypeFontManager::SetAlias(lString8 alias, lString8 facename, int id, bool bold, bool italic) {
    FONT_MAN_GUARD
    lString8 fontname=lString8("\0");
    LVFontDef def(
        fontname,
        -1,
        bold?700:400,
        italic,
        css_ff_inherit,
        facename,
        -1,
        id
    );
    LVFontCacheItem * item = _cache.find( &def);
    LVFontDef def1(
        fontname,
        -1,
        bold?700:400,
        italic,
        css_ff_inherit,
        alias,
        -1,
        id
    );

    int index = 0;

    FT_Face face = NULL;

    // for all faces in file
    for ( ;; index++ ) {
        int error = FT_New_Face( _library, item->getDef()->getName().c_str(), index, &face ); /* create face object */
        if ( error ) {
            if (index == 0) {
                CRLog::error("FT_New_Face returned error %d", error);
            }
            break;
        }
        int num_faces = face->num_faces;

        css_font_family_t fontFamily = css_ff_sans_serif;
        if ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
            fontFamily = css_ff_monospace;
        //lString8 familyName(!facename.empty() ? facename : ::familyName(face));
        // We don't need this here and in other places below: all fonts (except
        // monospaces) will be marked as sans-serif, and elements with
        // style {font-family:serif;} will use the default font too.
        // (we don't ship any Times, and someone having unluckily such
        // a font among his would see it used for {font-family:serif;}
        // elements instead of his default font)
        /*
        if ( familyName=="Times" || familyName=="Times New Roman" )
            fontFamily = css_ff_serif;
        */

        bool boldFlag = !facename.empty() ? bold : (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
        bool italicFlag = !facename.empty() ? italic : (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

        LVFontDef def2(
                item->getDef()->getName(),
                -1, // height==-1 for scalable fonts
                boldFlag ? 700 : 400,
                italicFlag,
                fontFamily,
                alias,
                index,
                id
        );

        if ( face ) {
            FT_Done_Face( face );
            face = NULL;
        }

        if ( _cache.findDuplicate( &def2 ) ) {
            CRLog::trace("font definition is duplicate");
            return false;
        }
        _cache.update( &def2, LVFontRef(NULL) );
        if (!def.getItalic()) {
            LVFontDef newDef( def2 );
            newDef.setItalic(2); // can italicize
            if ( !_cache.findDuplicate( &newDef ) )
                _cache.update( &newDef, LVFontRef(NULL) );
        }
        if ( index>=num_faces-1 )
            break;
    }
    item = _cache.find( &def1);
    if (item->getDef()->getTypeFace()==alias ) {
        return true;
    }
    else {
        return false;
    }
}

LVFontRef LVFreeTypeFontManager::GetFont(int size, int weight, bool italic, css_font_family_t family,
                                         lString8 typeface, int documentId, bool useBias) {
    FONT_MAN_GUARD
#if (DEBUG_FONT_MAN == 1)
    if ( _log ) {
fprintf(_log, "GetFont(size=%d, weight=%d, italic=%d, family=%d, typeface='%s')\n",
        size, weight, italic?1:0, (int)family, typeface.c_str() );
}
#endif
    lString8 fontname;
    LVFontDef def(
            fontname,
            size,
            weight,
            italic,
            family,
            typeface,
            -1,
            documentId
    );
#if (DEBUG_FONT_MAN == 1)
    if ( _log )
        fprintf( _log, "GetFont: %s %d %s %s\n",
                 typeface.c_str(),
                 size,
                 weight>400?"bold":"",
                 italic?"italic":"" );
#endif
    LVFontCacheItem *item = _cache.find( &def, useBias );
#if (DEBUG_FONT_MAN == 1)
    if ( item && _log ) { //_log &&
        fprintf(_log, "   found item: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s, weightDelta=%d) FontRef=%d\n",
                item->getDef()->getName().c_str(), item->getDef()->getIndex(), item->getDef()->getSize(), item->getDef()->getWeight(), item->getDef()->getItalic()?1:0,
                (int)item->getDef()->getFamily(), item->getDef()->getTypeFace().c_str(),
                weight - item->getDef()->getWeight(), item->getFont().isNull()?0:item->getFont()->getHeight()
                                                                               );
    }
#endif
    bool italicize = false;

    LVFontDef newDef(*item->getDef());

    if (!item->getFont().isNull()) {
        int deltaWeight = weight - item->getDef()->getWeight();
        if (deltaWeight >= 200) {
            // This instantiated cached font has a too low weight
            #ifndef USE_FT_EMBOLDEN
                // embolden using LVFontBoldTransform
                CRLog::debug("font: apply Embolding to increase weight from %d to %d",
                                    newDef.getWeight(), newDef.getWeight() + 200 );
                newDef.setWeight( newDef.getWeight() + 200 );
                LVFontRef ref = LVFontRef( new LVFontBoldTransform( item->getFont(), &_globalCache ) );
                _cache.update( &newDef, ref );
                return ref;
            #endif
            // when USE_FT_EMBOLDEN, ignore this low-weight cached font instance
            // and go loading from the font file again to apply embolden.
        } else {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
    }
    lString8 fname = item->getDef()->getName();
#if (DEBUG_FONT_MAN == 1)
    if ( _log ) {
        int index = item->getDef()->getIndex();
        fprintf(_log, "   no instance: adding new one for filename=%s, index = %d\n", fname.c_str(), index );
    }
#endif
    LVFreeTypeFace *font = new LVFreeTypeFace(_lock, _library, &_globalCache);
    lString8 pathname = makeFontFileName(fname);
    //def.setName( fname );
    //def.setIndex( index );

    //if ( fname.empty() || pathname.empty() ) {
    //    pathname = lString8("arial.ttf");
    //}

    if (!item->getDef()->isRealItalic() && italic) {
        //CRLog::debug("font: fake italic");
        newDef.setItalic(2);
        italicize = true;
    }

    // Use the family of the font we found in the cache (it may be different
    // from the requested family).
    // Assigning the requested familly to this new font could be wrong, and
    // may cause a style or font mismatch when loading from cache, forcing a
    // full re-rendering).
    family = item->getDef()->getFamily();

    //printf("going to load font file %s\n", fname.c_str());
    bool loaded = false;
    if (item->getDef()->getBuf().isNull())
        loaded = font->loadFromFile(pathname.c_str(), item->getDef()->getIndex(), size, family,
                                    isBitmapModeForSize(size), italicize);
    else
        loaded = font->loadFromBuffer(item->getDef()->getBuf(), item->getDef()->getIndex(), size,
                                      family, isBitmapModeForSize(size), italicize);
    if (loaded) {
        //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
        //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
        LVFontRef ref(font);
        font->setKerning( GetKerning() );
        font->setShapingMode( GetShapingMode() );
        font->setFaceName(item->getDef()->getTypeFace());
        newDef.setSize(size);
        //item->setFont( ref );
        //_cache.update( def, ref );
        int deltaWeight = weight - newDef.getWeight();
        if (deltaWeight >= 200) {
            // embolden
            #ifndef USE_FT_EMBOLDEN
                CRLog::debug("font: apply Embolding to increase weight from %d to %d",
                                    newDef.getWeight(), newDef.getWeight() + 200 );
                // Create a wrapper with LVFontBoldTransform which will bolden the glyphs
                newDef.setWeight( newDef.getWeight() + 200 );
                ref = LVFontRef( new LVFontBoldTransform( ref, &_globalCache ) );
            #else
                // Will make some of this font's methods do embolden the glyphs and widths
                font->setEmbolden();
                newDef.setWeight( font->getWeight() );
            #endif
        }
        _cache.update( &newDef, ref );
        //            int rsz = ref->getSize();
        //            if ( rsz!=size ) {
        //                size++;
        //            }
        //delete def;
        return ref;
    } else {
        //printf("    not found!\n");
    }
    //delete def;
    delete font;
    return LVFontRef(NULL);
}

bool LVFreeTypeFontManager::checkCharSet(FT_Face face) {
    // TODO: check existance of required characters (e.g. cyrillic)
    if (face == NULL)
        return false; // invalid face
    for (int i = 0; i < _requiredChars.length(); i++) {
        lChar16 ch = _requiredChars[i];
        FT_UInt ch_glyph_index = FT_Get_Char_Index(face, ch);
        if ( ch_glyph_index == 0 ) {
            CRLog::debug("Required char not found in font: %04x", ch);
            return false; // no required char!!!
        }
    }
    return true;
}

bool LVFreeTypeFontManager::checkFontLangCompat(const lString8 &typeface, const lString8 &langCode) {
    LVFontRef fntRef = GetFont(10, 400, false, css_ff_inherit, typeface, -1);
    if (!fntRef.isNull())
        return fntRef->checkFontLangCompat(langCode);
    else
        CRLog::debug("checkFontLangCompat(): typeface not found: %s", typeface.c_str());
    return true;
}

/*
bool LVFreeTypeFontManager::isMonoSpaced( FT_Face face )
{
    // TODO: check existance of required characters (e.g. cyrillic)
    if (face==NULL)
        return false; // invalid face
    lChar16 ch1 = 'i';
    FT_UInt ch_glyph_index1 = FT_Get_Char_Index( face, ch1 );
    if ( ch_glyph_index1==0 )
        return false; // no required char!!!
    int w1, w2;
    int error1 = FT_Load_Glyph( face,  //    handle to face object
            ch_glyph_index1,           //    glyph index
            FT_LOAD_DEFAULT );         //   load flags, see below
    if ( error1 )
        w1 = 0;
    else
        w1 = (face->glyph->metrics.horiAdvance >> 6);
    int error2 = FT_Load_Glyph( face,  //     handle to face object
            ch_glyph_index2,           //     glyph index
            FT_LOAD_DEFAULT );         //     load flags, see below
    if ( error2 )
        w2 = 0;
    else
        w2 = (face->glyph->metrics.horiAdvance >> 6);

    lChar16 ch2 = 'W';
    FT_UInt ch_glyph_index2 = FT_Get_Char_Index( face, ch2 );
    if ( ch_glyph_index2==0 )
        return false; // no required char!!!
    return w1==w2;
}
*/

bool LVFreeTypeFontManager::RegisterDocumentFont(int documentId, LVContainerRef container,
                                                 lString16 name, lString8 faceName, bool bold,
                                                 bool italic) {
    FONT_MAN_GUARD
    lString8 name8 = UnicodeToUtf8(name);
    CRLog::debug("RegisterDocumentFont(documentId=%d, path=%s)", documentId, name8.c_str());
    if (_cache.findDocumentFontDuplicate(documentId, name8)) {
        return false;
    }
    LVStreamRef stream = container->OpenStream(name.c_str(), LVOM_READ);
    if (stream.isNull())
        return false;
    lUInt32 size = (lUInt32) stream->GetSize();
    if (size < 100 || size > 5000000)
        return false;
    LVByteArrayRef buf(new LVByteArray(size, 0));
    lvsize_t bytesRead = 0;
    if (stream->Read(buf->get(), size, &bytesRead) != LVERR_OK || bytesRead != size)
        return false;
    bool res = false;

    int index = 0;

    FT_Face face = NULL;

    // for all faces in file
    for (;; index++) {
        int error = FT_New_Memory_Face(_library, buf->get(), buf->length(), index,
                                       &face); /* create face object */
        if (error) {
            if (index == 0) {
                CRLog::error("FT_New_Memory_Face returned error %d", error);
            }
            break;
        }
        //            bool scal = FT_IS_SCALABLE( face );
        //            bool charset = checkCharSet( face );
        //            //bool monospaced = isMonoSpaced( face );
        //            if ( !scal || !charset ) {
        //    //#if (DEBUG_FONT_MAN==1)
        //     //           if ( _log ) {
        //                CRLog::debug("    won't register font %s: %s",
        //                    name.c_str(), !charset?"no mandatory characters in charset" : "font is not scalable"
        //                    );
        //    //            }
        //    //#endif
        //                if ( face ) {
        //                    FT_Done_Face( face );
        //                    face = NULL;
        //                }
        //                break;
        //            }
        int num_faces = face->num_faces;

        css_font_family_t fontFamily = css_ff_sans_serif;
        if (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)
            fontFamily = css_ff_monospace;
        lString8 familyName(!faceName.empty() ? faceName : ::familyName(face));
        if (familyName == "Times" || familyName == "Times New Roman")
            fontFamily = css_ff_serif;

        bool boldFlag = !faceName.empty() ? bold : (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
        bool italicFlag = !faceName.empty() ? italic : (face->style_flags & FT_STYLE_FLAG_ITALIC) !=
                                                       0;

        LVFontDef def(
                name8,
                -1, // height==-1 for scalable fonts
                boldFlag ? 700 : 400,
                italicFlag,
                fontFamily,
                familyName,
                index,
                documentId,
                buf
        );
#if (DEBUG_FONT_MAN == 1)
        if ( _log ) {
            fprintf(_log, "registering font: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                    def.getName().c_str(), def.getIndex(), def.getSize(), def.getWeight(), def.getItalic()?1:0, (int)def.getFamily(), def.getTypeFace().c_str()
                    );
        }
#endif
        if (face) {
            FT_Done_Face(face);
            face = NULL;
        }

        if (_cache.findDuplicate(&def)) {
            CRLog::trace("font definition is duplicate");
            return false;
        }
        _cache.update(&def, LVFontRef(NULL));
        if (!def.getItalic()) {
            LVFontDef newDef(def);
            newDef.setItalic(2); // can italicize
            if (!_cache.findDuplicate(&newDef))
                _cache.update(&newDef, LVFontRef(NULL));
        }
        res = true;

        if (index >= num_faces - 1)
            break;
    }

    return res;
}

void LVFreeTypeFontManager::UnregisterDocumentFonts(int documentId) {
    _cache.removeDocumentFonts(documentId);
}

bool LVFreeTypeFontManager::RegisterExternalFont(lString16 name, lString8 family_name, bool bold,
                                                 bool italic) {
    if (name.startsWithNoCase(lString16("res://")))
        name = name.substr(6);
    else if (name.startsWithNoCase(lString16("file://")))
        name = name.substr(7);
    lString8 fname = UnicodeToUtf8(name);

    bool res = false;

    int index = 0;

    FT_Face face = NULL;

    // for all faces in file
    for (;; index++) {
        int error = FT_New_Face(_library, fname.c_str(), index, &face); /* create face object */
        if (error) {
            if (index == 0) {
                CRLog::error("FT_New_Face returned error %d", error);
            }
            break;
        }
        bool scal = FT_IS_SCALABLE(face);
        bool charset = checkCharSet(face);
        if (!charset) {
            if (FT_Select_Charmap(face, FT_ENCODING_UNICODE)) // returns 0 on success
                // If no unicode charmap found, try symbol charmap
                if (!FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL))
                    // It has a symbol charmap: consider it valid
                    charset = true;
        }
        //bool monospaced = isMonoSpaced( face );
        if (!scal || !charset) {
            CRLog::debug("    won't register font %s: %s",
                         name.c_str(),
                         !charset ? "no mandatory characters in charset" : "font is not scalable"
            );
            if (face) {
                FT_Done_Face(face);
                face = NULL;
            }
            break;
        }
        int num_faces = face->num_faces;

        css_font_family_t fontFamily = css_ff_sans_serif;
        if (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)
            fontFamily = css_ff_monospace;
        //lString8 familyName(::familyName(face));
        /*
        if (familyName == "Times" || familyName == "Times New Roman")
            fontFamily = css_ff_serif;
         */

        LVFontDef def(
                fname,
                -1, // height==-1 for scalable fonts
                bold ? 700 : 400,
                italic ? true : false,
                fontFamily,
                family_name,
                index
        );
#if (DEBUG_FONT_MAN == 1)
        if ( _log ) {
            fprintf(_log, "registering font: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                    def.getName().c_str(), def.getIndex(), def.getSize(), def.getWeight(), def.getItalic()?1:0, (int)def.getFamily(), def.getTypeFace().c_str()
                    );
        }
#endif
        if ( _cache.findDuplicate(&def) ) {
            CRLog::trace("font definition is duplicate");
            return false;
        }
        _cache.update(&def, LVFontRef(NULL));
        if ( scal && !def.getItalic() ) {
            LVFontDef newDef(def);
            newDef.setItalic(2); // can italicize
            if ( !_cache.findDuplicate(&newDef) )
                _cache.update(&newDef, LVFontRef(NULL));
        }
        res = true;

        if ( face ) {
            FT_Done_Face(face);
            face = NULL;
        }

        if ( index >= num_faces - 1 )
            break;
    }

    return res;
}

bool LVFreeTypeFontManager::RegisterFont(lString8 name) {
    FONT_MAN_GUARD
#ifdef LOAD_TTF_FONTS_ONLY
    if ( name.pos( cs8(".ttf") ) < 0 && name.pos( cs8(".TTF") ) < 0 )
    return false; // load ttf fonts only
#endif
    //CRLog::trace("RegisterFont(%s)", name.c_str());
    lString8 fname = makeFontFileName(name);
    //CRLog::trace("font file name : %s", fname.c_str());
#if (DEBUG_FONT_MAN == 1)
    if ( _log ) {
        fprintf(_log, "RegisterFont( %s ) path=%s\n",
                name.c_str(), fname.c_str()
                );
    }
#endif
    bool res = false;

    int index = 0;

    FT_Face face = NULL;

    // for all faces in file
    for (;; index++) {
        int error = FT_New_Face(_library, fname.c_str(), index, &face); /* create face object */
        if (error) {
            if (index == 0) {
                CRLog::error("FT_New_Face returned error %d", error);
            }
            break;
        }
        bool scal = FT_IS_SCALABLE(face) != 0;
        bool charset = checkCharSet(face);
        if (!charset) {
            if (FT_Select_Charmap(face, FT_ENCODING_UNICODE)) // returns 0 on success
                // If no unicode charmap found, try symbol charmap
                if (!FT_Select_Charmap(face, FT_ENCODING_MS_SYMBOL))
                    // It has a symbol charmap: consider it valid
                    charset = true;
        }
        //bool monospaced = isMonoSpaced( face );
        if (!scal || !charset) {
            CRLog::debug("    won't register font %s: %s",
                         name.c_str(),
                         !charset ? "no mandatory characters in charset" : "font is not scalable"
            );
            if (face) {
                FT_Done_Face(face);
                face = NULL;
            }
            break;
        }
        int num_faces = face->num_faces;

        css_font_family_t fontFamily = css_ff_sans_serif;
        if (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)
            fontFamily = css_ff_monospace;
        lString8 familyName(::familyName(face));
        /*
        if (familyName == "Times" || familyName == "Times New Roman")
            fontFamily = css_ff_serif;
         */

        LVFontDef def(
                name,
                -1, // height==-1 for scalable fonts
                (face->style_flags & FT_STYLE_FLAG_BOLD) ? 700 : 400,
                (face->style_flags & FT_STYLE_FLAG_ITALIC) ? true : false,
                fontFamily,
                familyName,
                index
        );
#if (DEBUG_FONT_MAN == 1)
        if ( _log ) {
            fprintf(_log, "registering font: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                    def.getName().c_str(), def.getIndex(), def.getSize(), def.getWeight(), def.getItalic()?1:0, (int)def.getFamily(), def.getTypeFace().c_str()
                    );
        }
#endif

        if (face) {
            FT_Done_Face(face);
            face = NULL;
        }

        if (_cache.findDuplicate(&def)) {
            CRLog::trace("font definition is duplicate");
            return false;
        }
        _cache.update(&def, LVFontRef(NULL));
        if (scal && !def.getItalic()) {
            // If this font is not italic, create another definition
            // with italic=2 (=fake italic) as we can italicize it.
            // A real italic font (italic=1) will be found first
            // when italic is requested.
            // (Strange that italic and embolden are managed differently...
            // maybe it makes the 2x2 combinations easier to manage?)
            LVFontDef newDef( def );
            newDef.setItalic(2); // can italicize
            if ( !_cache.findDuplicate( &newDef ) )
                _cache.update( &newDef, LVFontRef(NULL) );
        }
        res = true;

        if ( index >= num_faces - 1 )
            break;
    }

    return res;
}

bool LVFreeTypeFontManager::Init(lString8 path) {
    _path = path;
    initSystemFonts();
    return (_library != NULL);
}

bool LVFreeTypeFontManager::SetAsPreferredFontWithBias(lString8 face, int bias, bool clearOthersBias)
{
    FONT_MAN_GUARD
    return _cache.setAsPreferredFontWithBias(face, bias, clearOthersBias);
}

#endif  // (USE_FREETYPE==1)
