#ifndef __TEXTLANG_H_INCLUDED__
#define __TEXTLANG_H_INCLUDED__

#include "crsetup.h"
#include "lvptrvec.h"
#include "lvstring.h"

#if USE_HARFBUZZ==1
#include <hb.h>
#include <hb-ft.h>
#endif

#if USE_LIBUNIBREAK==1
    #ifdef __cplusplus
    extern "C" {
    #endif
#include <linebreak.h>
#include <linebreakdef.h>
    #ifdef __cplusplus
    }
    #endif
#endif

// Be similar to HyphMan default state with "English_US.pattern"
#define TEXTLANG_DEFAULT_MAIN_LANG              "en"   // for LVDocView
#define TEXTLANG_DEFAULT_MAIN_LANG_16           L"en"  // for textlang.cpp
#define TEXTLANG_DEFAULT_EMBEDDED_LANGS_ENABLED false
#define TEXTLANG_DEFAULT_HYPHENATION_ENABLED    true
#define TEXTLANG_DEFAULT_HYPH_SOFT_HYPHENS_ONLY false
#define TEXTLANG_DEFAULT_HYPH_FORCE_ALGORITHMIC false
#define TEXTLANG_FALLBACK_HYPH_DICT_ID  L"English_US.pattern" // For languages without specific hyph dicts

class TextLangCfg;
class HyphMethod;
struct ldomNode;

class TextLangMan
{
    friend class TextLangCfg;
    static lString16 _main_lang;
    static bool _embedded_langs_enabled;
    static LVPtrVector<TextLangCfg> _lang_cfg_list;

    static bool _overridden_hyph_method; // (to avoid checking the 3 following bool)
    static bool _hyphenation_enabled;
    static bool _hyphenation_soft_hyphens_only;
    static bool _hyphenation_force_algorithmic;
    static HyphMethod * _no_hyph_method;       // instance of hyphman NoHyph
    static HyphMethod * _soft_hyphens_method;  // instance of hyphman SoftHyphensHyph
    static HyphMethod * _algo_hyph_method;     // instance of hyphman AlgoHyph

    static HyphMethod * getHyphMethodForLang( lString16 lang_tag ); // Used by TextLangCfg
public:
    static void uninit();
    static lUInt32 getHash();

    static void setMainLang( lString16 lang_tag ) { _main_lang = lang_tag; }
    static void setMainLangFromHyphDict( lString16 id ); // For HyphMan legacy methods
    static lString16 getMainLang() { return _main_lang; }

    static void setEmbeddedLangsEnabled( bool enabled ) { _embedded_langs_enabled = enabled; }
    static bool getEmbeddedLangsEnabled() { return _embedded_langs_enabled; }

    static bool getHyphenationEnabled() { return _hyphenation_enabled; }
    static void setHyphenationEnabled( bool enabled ) {
        _hyphenation_enabled = enabled;
        _overridden_hyph_method = !_hyphenation_enabled || _hyphenation_soft_hyphens_only || _hyphenation_force_algorithmic;
    }

    static bool getHyphenationSoftHyphensOnly() { return _hyphenation_soft_hyphens_only; }
    static void setHyphenationSoftHyphensOnly( bool enabled ) {
        _hyphenation_soft_hyphens_only = enabled;
        _overridden_hyph_method = !_hyphenation_enabled || _hyphenation_soft_hyphens_only || _hyphenation_force_algorithmic;
    }

    static bool getHyphenationForceAlgorithmic() { return _hyphenation_force_algorithmic; }
    static void setHyphenationForceAlgorithmic( bool enabled ) {
        _hyphenation_force_algorithmic = enabled;
        _overridden_hyph_method = !_hyphenation_enabled || _hyphenation_soft_hyphens_only || _hyphenation_force_algorithmic;
    }

    static TextLangCfg * getTextLangCfg(); // get LangCfg for _main_lang
    static TextLangCfg * getTextLangCfg( lString16 lang_tag );
    static TextLangCfg * getTextLangCfg( ldomNode * node );
    static int getLangNodeIndex( ldomNode * node );

    static HyphMethod * getMainLangHyphMethod(); // For HyphMan::hyphenate()

    // For frontend info about TextLangMan status and seen langs
    static LVPtrVector<TextLangCfg> * getLangCfgList() {
        return &_lang_cfg_list;
    }

    static lString16 getLangTag(const lString16& title);

    TextLangMan();
    ~TextLangMan();
};

#define MAX_NB_LB_PROPS_ITEMS 10 // for our statically sized array (increase if needed)

typedef lChar16 (*lb_char_sub_func_t)(const lChar16 * text, int pos, int next_usable);

class TextLangCfg
{
    friend class TextLangMan;
    lString16 _lang_tag;
    HyphMethod * _hyph_method;

    #if USE_HARFBUZZ==1
    hb_language_t _hb_language;
    #endif

    #if USE_LIBUNIBREAK==1
    lb_char_sub_func_t _lb_char_sub_func;
    struct LineBreakProperties _lb_props[MAX_NB_LB_PROPS_ITEMS];
    #endif

    bool _duplicate_real_hyphen_on_next_line;

public:
    lString16 getLangTag() const { return _lang_tag; }

    HyphMethod * getHyphMethod() const {
        if ( !TextLangMan::_overridden_hyph_method )
            return _hyph_method;
        if ( !TextLangMan::_hyphenation_enabled )
            return TextLangMan::_no_hyph_method;
        if ( TextLangMan::_hyphenation_soft_hyphens_only )
            return TextLangMan::_soft_hyphens_method;
        if ( TextLangMan::_hyphenation_force_algorithmic )
            return TextLangMan::_algo_hyph_method;
        // Should not be reached
        return _hyph_method;
    }
    HyphMethod * getDefaultHyphMethod() const {
        return _hyph_method;
    }

    #if USE_HARFBUZZ==1
    hb_language_t getHBLanguage() const { return _hb_language; }
    #endif

    #if USE_LIBUNIBREAK==1
    bool hasLBCharSubFunc() const { return _lb_char_sub_func != NULL; }
    lb_char_sub_func_t getLBCharSubFunc() const { return _lb_char_sub_func; }
    struct LineBreakProperties * getLBProps() const { return (struct LineBreakProperties *)_lb_props; }
    #endif

    bool duplicateRealHyphenOnNextLine() const { return _duplicate_real_hyphen_on_next_line; }

    TextLangCfg( lString16 lang_tag );
    ~TextLangCfg();
};


#endif
