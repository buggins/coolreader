// IMPORTANT : when making changes in language detection logic and per-language
// rules here, be sure to also bump FORMATTING_VERSION_ID in src/lvtinydom.cpp

#include "../include/textlang.h"
#include "../include/hyphman.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/crlog.h"

// Uncomment to see which lang_tags are seen and lang_cfg created
// #define DEBUG_LANG_USAGE

// Some macros to expand: LANG_STARTS_WITH(("fr") ("es"))   (no comma!)
// to: lang_tag.startsWith("fr") || lang_tag.startsWith("es") || false
// (from https://stackoverflow.com/questions/19680962/translate-sequence-in-macro-parameters-to-separate-macros )
#define PRIMITIVE_SEQ_ITERATE(...) __VA_ARGS__ ## _END
#define SEQ_ITERATE(...) PRIMITIVE_SEQ_ITERATE(__VA_ARGS__)
#define LANG_STARTS_WITH(seq) SEQ_ITERATE(LANG_STARTS_WITH_EACH_1 seq)
#define LANG_STARTS_WITH_EACH_1(...) lang_tag.startsWith(__VA_ARGS__) || LANG_STARTS_WITH_EACH_2
#define LANG_STARTS_WITH_EACH_2(...) lang_tag.startsWith(__VA_ARGS__) || LANG_STARTS_WITH_EACH_1
#define LANG_STARTS_WITH_EACH_1_END false
#define LANG_STARTS_WITH_EACH_2_END false

// (hyph_filename_prefix added because CoolReader may still have both
// current "Italian.pattern" and old "Italian_hyphen_(Alan).pdb".)
// (Romanian and Ukrainian have the prefix truncated because previous
// pattern files, still in CoolReader, had these truncated names.)
static struct {
    const char * lang_tag;
    const char * hyph_filename_prefix;
    const char * hyph_filename;
    int left_hyphen_min;
    int right_hyphen_min;
} _hyph_dict_table[] = {
    { "eu",    "Basque",        "Basque.pattern",        2, 2 },
    { "bg",    "Bulgarian",     "Bulgarian.pattern",     2, 2 },
    { "ca",    "Catalan",       "Catalan.pattern",       2, 2 },
    { "cs",    "Czech",         "Czech.pattern",         2, 3 },
    { "da",    "Danish",        "Danish.pattern",        2, 2 },
    { "nl",    "Dutch",         "Dutch.pattern",         2, 2 },
    { "en-GB", "English_GB",    "English_GB.pattern",    2, 3 },
    { "en",    "English_US",    "English_US.pattern",    2, 3 },
    { "eo",    "Esperanto",     "Esperanto.pattern",     2, 2 },
    { "et",    "Estonian",      "Estonian.pattern",      2, 3 },
    { "fi",    "Finnish",       "Finnish.pattern",       2, 2 },
    { "fr",    "French",        "French.pattern",        2, 1 }, // see French.pattern file for why right_hyphen_min=1
    { "gl",    "Galician",      "Galician.pattern",      2, 2 },
    { "ka",    "Georgian",      "Georgian.pattern",      1, 2 },
    { "de",    "German",        "German.pattern",        2, 2 },
    { "el",    "Greek",         "Greek.pattern",         1, 1 },
    { "hr",    "Croatian",      "Croatian.pattern",      2, 2 },
    { "hu",    "Hungarian",     "Hungarian.pattern",     2, 2 },
    { "is",    "Icelandic",     "Icelandic.pattern",     2, 2 },
    { "ga",    "Irish",         "Irish.pattern",         2, 3 },
    { "it",    "Italian",       "Italian.pattern",       2, 2 },
    { "lv",    "Latvian",       "Latvian.pattern",       2, 2 },
    { "lt",    "Lithuanian",    "Lithuanian.pattern",    2, 2 },
    { "mk",    "Macedonian",    "Macedonian.pattern",    2, 2 },
    { "no",    "Norwegian",     "Norwegian.pattern",     2, 2 },
    { "oc",    "Occitan",       "Occitan.pattern",       2, 2 },
    { "pl",    "Polish",        "Polish.pattern",        2, 2 },
    { "pt",    "Portuguese",    "Portuguese.pattern",    2, 3 },
    { "ro",    "Roman",         "Romanian.pattern",      2, 2 }, // truncated prefix (see above)
    { "ru-GB", "Russian_EnGB",  "Russian_EnGB.pattern",  2, 2 },
    { "ru-US", "Russian_EnUS",  "Russian_EnUS.pattern",  2, 2 },
    { "ru",    "Russian",       "Russian.pattern",       2, 2 },
    { "sr",    "Serbian",       "Serbian.pattern",       2, 2 },
    { "sk",    "Slovak",        "Slovak.pattern",        2, 3 },
    { "sl",    "Slovenian",     "Slovenian.pattern",     2, 2 },
    { "es",    "Spanish",       "Spanish.pattern",       2, 2 },
    { "sv",    "Swedish",       "Swedish.pattern",       2, 2 },
    { "tr",    "Turkish",       "Turkish.pattern",       2, 2 },
    { "uk",    "Ukrain",        "Ukrainian.pattern",     2, 2 }, // truncated prefix (see above)
    { "cy",    "Welsh",         "Welsh.pattern",         2, 3 },
    // No-lang hyph methods, for legacy HyphMan methods: other lang properties will be from English
    { "en#@none",        "@none",        "@none",        2, 2 },
    { "en#@softhyphens", "@softhyphens", "@softhyphens", 2, 2 },
    { "en#@algorithm",   "@algorithm",   "@algorithm",   2, 2 },
    { NULL, NULL, NULL, 0, 0 }
};

// Init global TextLangMan members
lString32 TextLangMan::_main_lang = TEXTLANG_DEFAULT_MAIN_LANG_32;
bool TextLangMan::_embedded_langs_enabled = TEXTLANG_DEFAULT_EMBEDDED_LANGS_ENABLED;
LVPtrVector<TextLangCfg> TextLangMan::_lang_cfg_list;

bool TextLangMan::_hyphenation_enabled = TEXTLANG_DEFAULT_HYPHENATION_ENABLED;
bool TextLangMan::_hyphenation_soft_hyphens_only = TEXTLANG_DEFAULT_HYPH_SOFT_HYPHENS_ONLY;
bool TextLangMan::_hyphenation_force_algorithmic = TEXTLANG_DEFAULT_HYPH_FORCE_ALGORITHMIC;
bool TextLangMan::_overridden_hyph_method =   !TEXTLANG_DEFAULT_HYPHENATION_ENABLED
                                            || TEXTLANG_DEFAULT_HYPH_SOFT_HYPHENS_ONLY
                                            || TEXTLANG_DEFAULT_HYPH_FORCE_ALGORITHMIC ;
// These will be set when we can
HyphMethod * TextLangMan::_no_hyph_method = NULL;
HyphMethod * TextLangMan::_algo_hyph_method = NULL;
HyphMethod * TextLangMan::_soft_hyphens_method = NULL;

TextLangMan::TextLangMan() {
}

TextLangMan::~TextLangMan() {
}

lUInt32 TextLangMan::getHash() {
    lUInt32 hash = _main_lang.getHash();
    hash = hash << 4;
    hash = hash + (_embedded_langs_enabled << 3);
    hash = hash + (_hyphenation_soft_hyphens_only << 2);
    hash = hash + (_hyphenation_force_algorithmic << 1);
    hash = hash + _hyphenation_enabled;
    // printf("TextLangMan::getHash %x\n", hash);
    return hash;
}

// No need to explicitely call this in frontend code.
// Calling HyphMan::uninit() will have this one called.
void TextLangMan::uninit() {
    _lang_cfg_list.clear();
}

// For HyphMan legacy methods
void TextLangMan::setMainLangFromHyphDict( lString32 id ) {
    // When setting up TextlangMan thru HyphMan legacy methods,
    // disable embedded langs, for a consistent hyphenation.
    TextLangMan::setEmbeddedLangsEnabled( false );
    // Update flags if asked for @none, @softhyphens or @algorithm
    TextLangMan::setHyphenationEnabled( id != HYPH_DICT_ID_NONE );
    TextLangMan::setHyphenationSoftHyphensOnly( id == HYPH_DICT_ID_SOFTHYPHENS );
    TextLangMan::setHyphenationForceAlgorithmic( id == HYPH_DICT_ID_ALGORITHM );

    for (int i=0; _hyph_dict_table[i].lang_tag!=NULL; i++) {
        if ( id.startsWith( _hyph_dict_table[i].hyph_filename_prefix ) ) {
            TextLangMan::setMainLang( lString32(_hyph_dict_table[i].lang_tag) );
            #ifdef DEBUG_LANG_USAGE
            printf("TextLangMan::setMainLangFromHyphDict %s => %s\n",
                UnicodeToLocal(id).c_str(), UnicodeToLocal(TextLangMan::getMainLang()).c_str());
            #endif
            return;
        }
    }
    CRLog::warn("lang not found for hyphenation dict: %s\n", UnicodeToLocal(id).c_str());
}

// Used only by TextLangCfg
HyphMethod * TextLangMan::getHyphMethodForLang( lString32 lang_tag ) {
    // Look for full lang_tag
#if 1
    // CoolReader use dynamically loaded hyphenation dictionaries (at startup)
    HyphDictionaryList* dictList = HyphMan::getDictList();
    HyphDictionary* dict;
    lString32 dict_lang_tag;
    lang_tag.lowercase();
    for (int i = 0; i < dictList->length(); i++) {
        dict = dictList->get(i);
        if (dict) {
            if (dict->getType() == HDT_DICT_ALAN || dict->getType() == HDT_DICT_TEX)
                dict_lang_tag = TextLangMan::getLangTag(dict->getTitle());      // for dictionary's files
            else
                dict_lang_tag = TextLangMan::getLangTag(dict->getId());         // for default dictionaries
            dict_lang_tag.lowercase();
            if (lang_tag == dict_lang_tag)
                return HyphMan::getHyphMethodForDictionary( dict->getId(),
                            _hyph_dict_table[i].left_hyphen_min, _hyph_dict_table[i].right_hyphen_min);
        }
    }
    // Look for lang_tag initial subpart
    int m_pos = lang_tag.pos("-");
    if ( m_pos > 0 ) {
        lString32 lang_tag2 = lang_tag.substr(0, m_pos);
        lang_tag2.lowercase();
        for (int i = 0; i < dictList->length(); i++) {
            dict = dictList->get(i);
            if (dict) {
                if (dict->getType() == HDT_DICT_ALAN || dict->getType() == HDT_DICT_TEX)
                    dict_lang_tag = TextLangMan::getLangTag(dict->getTitle());
                else
                    dict_lang_tag = TextLangMan::getLangTag(dict->getId());     // for default dictionaries
                dict_lang_tag.lowercase();
                if (lang_tag2 == dict_lang_tag)
                    return HyphMan::getHyphMethodForDictionary( dict->getId(),
                                _hyph_dict_table[i].left_hyphen_min, _hyph_dict_table[i].right_hyphen_min);
            }
        }
    }
#else
    // koreader use hardcoded hyphenation dictionary table
    for (int i=0; _hyph_dict_table[i].lang_tag!=NULL; i++) {
        if ( lang_tag == lString32(_hyph_dict_table[i].lang_tag).lowercase() ) {
            return HyphMan::getHyphMethodForDictionary( lString32(_hyph_dict_table[i].hyph_filename),
                        _hyph_dict_table[i].left_hyphen_min, _hyph_dict_table[i].right_hyphen_min);
        }
    }
    // Look for lang_tag initial subpart
    int m_pos = lang_tag.pos("-");
    if ( m_pos > 0 ) {
        lString32 lang_tag2 = lang_tag.substr(0, m_pos);
        for (int i=0; _hyph_dict_table[i].lang_tag!=NULL; i++) {
            if ( lang_tag2 == lString32(_hyph_dict_table[i].lang_tag).lowercase() ) {
                return HyphMan::getHyphMethodForDictionary( lString32(_hyph_dict_table[i].hyph_filename),
                            _hyph_dict_table[i].left_hyphen_min, _hyph_dict_table[i].right_hyphen_min);
            }
        }
    }
#endif
    // Fallback to English_US, as other languages are more likely to get mixed
    // with english text (it feels better than using @algorithm)
    return HyphMan::getHyphMethodForDictionary(TEXTLANG_FALLBACK_HYPH_DICT_ID);
}

// Return the (single and cached) TextLangCfg for the provided lang_tag
TextLangCfg * TextLangMan::getTextLangCfg( lString32 lang_tag ) {
    if ( !_embedded_langs_enabled ) {
        // Drop provided lang_tag: always return main lang TextLangCfg
        lang_tag = _main_lang;
    }
    // Not sure if we can lowercase lang_tag and avoid duplicate (Harfbuzz might
    // need the proper lang tag with some parts starting with some uppercase letter)
    for ( int i=0; i<_lang_cfg_list.length(); i++ ) {
        if ( _lang_cfg_list[i]->_lang_tag == lang_tag ) {
            // printf("TextLangCfg %s reused\n", UnicodeToLocal(lang_tag).c_str());
            // There should rarely be more than 3 lang in a document, so move
            // any requested far down in the list at top to shorten next loops.
            if ( i > 2 ) {
                _lang_cfg_list.move(0, i);
                return _lang_cfg_list[0];
            }
            return _lang_cfg_list[i];
        }
    }
    // Not found in cache: create it
    TextLangCfg * lang_cfg = new TextLangCfg( lang_tag );
    _lang_cfg_list.add( lang_cfg ); // and cache it
    return lang_cfg;
}

TextLangCfg * TextLangMan::getTextLangCfg() {
    // No lang_tag specified: return main lang one
    return TextLangMan::getTextLangCfg( _main_lang );
}

TextLangCfg * TextLangMan::getTextLangCfg( ldomNode * node ) {
    if ( !_embedded_langs_enabled || !node ) {
        // No need to look at nodes: return main lang one
        return TextLangMan::getTextLangCfg( _main_lang );
    }
    if ( node->isText() )
        node = node->getParentNode();
    // We are usually called from renderFinalBlock() with a node that
    // we know has a lang= attribute.
    // But we may be called in other contexts (e.g. writeNodeEx) with
    // any node: so, look at this node parents for that lang= attribute.
    for ( ; !node->isRoot(); node = node->getParentNode() ) {
        if ( node->hasAttribute( attr_lang ) ) {
            lString32 lang_tag = node->getAttributeValue( attr_lang );
            if ( !lang_tag.empty() )
                return TextLangMan::getTextLangCfg( lang_tag );
        }
    }
    // No parent with lang= attribute: return main lang one
    return TextLangMan::getTextLangCfg( _main_lang );
}

int TextLangMan::getLangNodeIndex( ldomNode * node ) {
    if ( !_embedded_langs_enabled || !node ) {
        // No need to look up if !_embedded_langs_enabled
        return 0;
    }
    if ( node->isText() )
        node = node->getParentNode();
    for ( ; !node->isRoot(); node = node->getParentNode() ) {
        if ( node->hasAttribute( attr_lang ) ) {
            if ( !node->getAttributeValue( attr_lang ).empty() ) {
                return node->getDataIndex();
            }
        }
    }
    return 0;
}

// For HyphMan::hyphenate()
HyphMethod * TextLangMan::getMainLangHyphMethod() {
    return getTextLangCfg()->getHyphMethod();
}

lString32 TextLangMan::getLangTag(const lString32& title)
{
    for (int i = 0; _hyph_dict_table[i].lang_tag!=NULL; i++) {
        if (title == lString32(_hyph_dict_table[i].hyph_filename_prefix)) {
            return lString32(_hyph_dict_table[i].lang_tag);
        }
    }
    return lString32();
}

void TextLangMan::resetCounters() {
    for ( int i=0; i<_lang_cfg_list.length(); i++ ) {
        _lang_cfg_list[i]->resetCounters();
    }
}

// TextLangCfg object: per language holder of language specificities

// For CSS "content: open-quote / close-quote"
typedef struct quotes_spec {
    const char * lang_tag;
    const lChar32 *  open_quote_level_1;
    const lChar32 * close_quote_level_1;
    const lChar32 *  open_quote_level_2;
    const lChar32 * close_quote_level_2;
} quotes_spec;

// List built 20200601 from https://html.spec.whatwg.org/multipage/rendering.html#quotes
// 2nd part of lang_tag lowercased for easier comparison, and if multiple
// lang_tag with the same starting chars, put the longest first.
// Small issue: 3-letters lang tag not specified here might match
// a 2-letter lang tag specified here ("ito" will get those from "it").
static quotes_spec _quotes_spec_table[] = {
    { "af",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "agq",      U"\x201e", U"\x201d", U"\x201a", U"\x2019" }, /* „ ” ‚ ’ */
    { "ak",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "am",       U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "ar",       U"\x201d", U"\x201c", U"\x2019", U"\x2018" }, /* ” “ ’ ‘ */
    { "asa",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ast",      U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "az-cyrl",  U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "az",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "bas",      U"\x00ab", U"\x00bb", U"\x201e", U"\x201c" }, /* « » „ “ */
    { "bem",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "bez",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "be",       U"\x00ab", U"\x00bb", U"\x201e", U"\x201c" }, /* « » „ “ */
    { "bg",       U"\x201e", U"\x201c", U"\x2018", U"\x2019" }, /* „ “ ‘ ’ */
    { "bm",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "bn",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "brx",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "br",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "bs-cyrl",  U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "bs",       U"\x201e", U"\x201d", U"\x2018", U"\x2019" }, /* „ ” ‘ ’ */
    { "ca",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "cgg",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "chr",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "cs",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "cy",       U"\x2018", U"\x2019", U"\x201c", U"\x201d" }, /* ‘ ’ “ ” */
    { "dav",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "da",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "de",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "dje",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "dsb",      U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "dua",      U"\x00ab", U"\x00bb", U"\x2018", U"\x2019" }, /* « » ‘ ’ */
    { "dyo",      U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "dz",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ebu",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ee",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "el",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "en",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "eo",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "es",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "et",       U"\x201e", U"\x201c", U"\x00ab", U"\x00bb" }, /* „ “ « » */
    { "eu",       U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "ewo",      U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "fa",       U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "ff",       U"\x201e", U"\x201d", U"\x201a", U"\x2019" }, /* „ ” ‚ ’ */
    { "fil",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "fi",       U"\x201d", U"\x201d", U"\x2019", U"\x2019" }, /* ” ” ’ ’ */
    { "fo",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "fr-ch",    U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    // { "fr",    U"\x00ab", U"\x00bb", U"\x00ab", U"\x00bb" }, /* « » « » */  /* Same pair for both level, bit sad... */
    { "fr",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */  /* Better to have "fr" just as "it" */
    { "ga",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "gd",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "gl",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "gsw",      U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "guz",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "gu",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ha",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "he",       U"\x201d", U"\x201d", U"\x2019", U"\x2019" }, /* ” ” ’ ’ */
    { "hi",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "hr",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "hsb",      U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "hu",       U"\x201e", U"\x201d", U"\x00bb", U"\x00ab" }, /* „ ” » « */
    { "hy",       U"\x00ab", U"\x00bb", U"\x00ab", U"\x00bb" }, /* « » « » */
    { "id",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ig",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "is",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "it",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "ja",       U"\x300c", U"\x300d", U"\x300e", U"\x300f" }, /* 「 」 『 』 */
    { "jgo",      U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "jmc",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "kab",      U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "kam",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ka",       U"\x201e", U"\x201c", U"\x2018", U"\x2019" }, /* „ “ “ ” */
    { "kde",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "kea",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "khq",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ki",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "kkj",      U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "kk",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "kln",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "km",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "kn",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ko",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ksb",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ksf",      U"\x00ab", U"\x00bb", U"\x2018", U"\x2019" }, /* « » ‘ ’ */
    { "ky",       U"\x00ab", U"\x00bb", U"\x201e", U"\x201c" }, /* « » „ “ */
    { "lag",      U"\x201d", U"\x201d", U"\x2019", U"\x2019" }, /* ” ” ’ ’ */
    { "lb",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "lg",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ln",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "lo",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "lrc",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "lt",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "luo",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "luy",      U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "lu",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "lv",       U"\x201c", U"\x201d", U"\x201e", U"\x201d" }, /* “ ” „ ” */
    { "mas",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mer",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mfe",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mgo",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mg",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "mk",       U"\x201e", U"\x201c", U"\x2019", U"\x2018" }, /* „ “ ’ ‘ */
    { "ml",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mn",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mr",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ms",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mt",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mua",      U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "my",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "mzn",      U"\x00ab", U"\x00bb", U"\x2039", U"\x203a" }, /* « » ‹ › */
    { "naq",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "nb",       U"\x00ab", U"\x00bb", U"\x2018", U"\x2019" }, /* « » ‘ ’ */
    { "nd",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ne",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "nl",       U"\x2018", U"\x2019", U"\x201c", U"\x201d" }, /* ‘ ’ “ ” */
    { "nmg",      U"\x201e", U"\x201d", U"\x00ab", U"\x00bb" }, /* „ ” « » */
    { "nnh",      U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "nn",       U"\x00ab", U"\x00bb", U"\x2018", U"\x2019" }, /* « » ‘ ’ */
    { "nus",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "nyn",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "oc",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "pa",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "pl",       U"\x201e", U"\x201d", U"\x00ab", U"\x00bb" }, /* „ ” « » */
    { "pt-pt",    U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "pt",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "rn",       U"\x201d", U"\x201d", U"\x2019", U"\x2019" }, /* ” ” ’ ’ */
    { "rof",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ro",       U"\x201e", U"\x201d", U"\x00ab", U"\x00bb" }, /* „ ” « » */
    { "ru",       U"\x00ab", U"\x00bb", U"\x201e", U"\x201c" }, /* « » „ “ */
    { "rwk",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "rw",       U"\x00ab", U"\x00bb", U"\x2018", U"\x2019" }, /* « » ‘ ’ */
    { "sah",      U"\x00ab", U"\x00bb", U"\x201e", U"\x201c" }, /* « » „ “ */
    { "saq",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "sbp",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "seh",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ses",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "sg",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "shi-latn", U"\x00ab", U"\x00bb", U"\x201e", U"\x201d" }, /* « » „ ” */
    { "shi",      U"\x00ab", U"\x00bb", U"\x201e", U"\x201d" }, /* « » „ ” */
    { "si",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "sk",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "sl",       U"\x201e", U"\x201c", U"\x201a", U"\x2018" }, /* „ “ ‚ ‘ */
    { "sn",       U"\x201d", U"\x201d", U"\x2019", U"\x2019" }, /* ” ” ’ ’ */
    { "so",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "sq",       U"\x00ab", U"\x00bb", U"\x201c", U"\x201d" }, /* « » “ ” */
    { "sr-latn",  U"\x201e", U"\x201c", U"\x2018", U"\x2018" }, /* „ “ ‘ ‘ */
    { "sr",       U"\x201e", U"\x201d", U"\x2019", U"\x2019" }, /* „ ” ’ ’ */
    { "sv",       U"\x201d", U"\x201d", U"\x2019", U"\x2019" }, /* ” ” ’ ’ */
    { "sw",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ta",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "teo",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "te",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "th",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "ti-er",    U"\x2018", U"\x2019", U"\x201c", U"\x201d" }, /* ‘ ’ “ ” */
    { "tk",       U"\x201c", U"\x201d", U"\x201c", U"\x201d" }, /* “ ” “ ” */
    { "to",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "tr",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "twq",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "tzm",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "uk",       U"\x00ab", U"\x00bb", U"\x201e", U"\x201c" }, /* « » „ “ */
    { "ur",       U"\x201d", U"\x201c", U"\x2019", U"\x2018" }, /* ” “ ’ ‘ */
    { "uz-cyrl",  U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "uz",       U"\x201c", U"\x201d", U"\x2019", U"\x2018" }, /* “ ” ’ ‘ */
    { "vai-latn", U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "vai",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "vi",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "vun",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "xog",      U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "yav",      U"\x00ab", U"\x00bb", U"\x00ab", U"\x00bb" }, /* « » « » */
    { "yo",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "yue-hans", U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "yue",      U"\x300c", U"\x300d", U"\x300e", U"\x300f" }, /* 「 」 『 』 */
    { "zgh",      U"\x00ab", U"\x00bb", U"\x201e", U"\x201d" }, /* « » „ ” */
    { "zh-hant",  U"\x300c", U"\x300d", U"\x300e", U"\x300f" }, /* 「 」 『 』 */
    { "zh",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { "zu",       U"\x201c", U"\x201d", U"\x2018", U"\x2019" }, /* “ ” ‘ ’ */
    { NULL, NULL, NULL, NULL, NULL }
};
// Default to quotes for English
static quotes_spec _quotes_spec_default = { "", U"\x201c", U"\x201d", U"\x2018", U"\x2019" };

#if USE_LIBUNIBREAK==1
#if KO_LIBUNIBREAK_PATCH==1
lChar32 lb_char_sub_func_english(struct LineBreakContext *lbpCtx, const lChar32 * text, int pos, int next_usable) {
    // https://github.com/koreader/crengine/issues/364
    // Normally, line breaks are allowed at both sides of an em-dash.
    // When an em-dash is at the "end of a word" (or beginning), we want to avoid separating it from its word,
    // this is detected by looking for letters/numbers at both sides of the dash, if on any side a space
    // is closer than any letter/number, treat it as a non-breakable dash.
    // The current implementation does not allow examining the following characters beyond the current node,
    // so the detection is not perfect and we replace the dash with "opening" or "closing" characters
    // (or "ambiguous), to play safer (note that "}" allows a break after, while ")" doesn't).
    //
    // The intent is the following:
    //   blah—blah                     ->  —  (break before or after)
    //   blah “—blah , <p>—blah        ->  {  (do not break after)
    //   blah—” Blah , blah—”</p>      ->  }  (do not break before)
    //   blah — blah , blah —<em>blah  ->  "  (break only at spaces)
    switch ( text[pos] ) {
        case 0x2014:  // em dash
        case 0x2E3A:  // two-em dash
        case 0x2E3B:  // three-em dash
            {
                // The variable "replacement" will be the output char,
                // we start by setting it to the actual input char.
                // It will be '{' if no-break on right,
                //            '}' if no-break on left,
                //            '"' if no-break on both.
                lChar32 replacement = text[pos];
                int new_pos;
                enum LineBreakClass new_lbc;
                // 1. Detect no-break on right (scan left of dash)
                //
                // already at the beginning of text
                if ( pos == 0 ) {
                    replacement = '{';
                }
                else {
                    // inspect preceding characters
                    new_pos = pos;
                    while ( new_pos > 0) {
                        new_pos--;
                        new_lbc = lb_get_char_class(lbpCtx, text[new_pos]);
                        if ( new_lbc == LBP_AL || new_lbc == LBP_NU ) {
                            // found word / number
                            break;
                        }
                        else if ( new_lbc == LBP_SP || new_pos == 0 ) {
                            // found space or beginning
                            replacement = '{';
                            break;
                        }
                    }
                }
                // 2. Detect no-break on left (scan right of dash)
                //    If already no-break on right, replacement will be '"'
                //
                // already at the end of text
                if ( next_usable == 0 ) {
                    replacement = ( replacement == '{' ) ? '"' : '}';
                }
                else {
                    // inspect following characters
                    new_pos = pos;
                    while ( new_pos < pos+next_usable ) {
                        new_pos++;
                        new_lbc = lb_get_char_class(lbpCtx, text[new_pos]);
                        if ( new_lbc == LBP_AL || new_lbc == LBP_NU ) {
                            // found word / number
                            break;
                        }
                        else if ( new_lbc == LBP_SP || new_pos == pos+next_usable ) {
                            // found space or end (of the current text node, there could be letters beyond)
                            replacement = ( replacement == '{' ) ? '"' : '}';
                            break;
                        }
                    }
                }
                return replacement;
            }
            break;
        default:
            break;
    }
    return text[pos];
}
#endif      // KO_LIBUNIBREAK_PATCH==1

lChar32 lb_char_sub_func_polish(struct LineBreakContext *lbpCtx, const lChar32 * text, int pos, int next_usable) {
    // https://github.com/koreader/koreader/issues/5645#issuecomment-559193057
    // Letters aiouwzAIOUWS are prepositions that should not be left at the
    // end of a line.
    // Make them behave (for libunibreak) just like a opening paren (which
    // being LBC_OP, will prevent a line break after it, even if followed
    // by a space).
    if ( pos >= 1 && text[pos-1] == ' ' ) {
        switch ( text[pos] ) {
            case 'A':
            case 'I':
            case 'O':
            case 'U':
            case 'W':
            case 'Z': // Meaning in english:
            case 'a': // and
            case 'i': // and
            case 'o': // about
            case 'u': // at
            case 'w': // in
            case 'z': // with
                return '(';
                break;
            default:
                break;
        }
    }
    return text[pos];
}

lChar32 lb_char_sub_func_czech_slovak(struct LineBreakContext *lbpCtx, const lChar32 * text, int pos, int next_usable) {
    // Same for Czech and Slovak : AIiVvOoUuSsZzKk
    // https://tex.stackexchange.com/questions/27780/one-letter-word-at-the-end-of-line
    // https://github.com/michal-h21/luavlna
    if ( pos >= 1 && text[pos-1] == ' ' ) {
        switch ( text[pos] ) {
            case 'A':
            case 'I':
            case 'K':
            case 'O':
            case 'S':
            case 'U':
            case 'V':
            case 'Z':
            case 'i':
            case 'k':
            case 'o':
            case 's':
            case 'u':
            case 'v':
            case 'z':
                return '(';
                break;
            default:
                break;
        }
    }
    return text[pos];
}
#endif

// Instantiate a new TextLangCfg with properties adequate to the provided lang_tag
TextLangCfg::TextLangCfg( lString32 lang_tag ) {
    if ( TextLangMan::_no_hyph_method == NULL ) {
        // We need to init static TextLangMan::_no_hyph_method and friends after
        // HyphMan is set up. Do that here, even if unrelated, as TextLangCfg
        // creation is called less often that every other methods around here.
        TextLangMan::_no_hyph_method = HyphMan::getHyphMethodForDictionary(HYPH_DICT_ID_NONE);
        TextLangMan::_soft_hyphens_method = HyphMan::getHyphMethodForDictionary(HYPH_DICT_ID_SOFTHYPHENS);
        TextLangMan::_algo_hyph_method = HyphMan::getHyphMethodForDictionary(HYPH_DICT_ID_ALGORITHM);
    }

    // Keep as our id the provided and non-lowercase'd lang_tag (with possibly bogus #@algorithm)
    _lang_tag = lang_tag;
    // Harfbuzz may know more than us about exotic/complex lang tags,
    // so let it deal the the provided one as-is.
    lString32 hb_lang_tag = lang_tag;
    // Lowercase it for our tests
    lang_tag.lowercase(); // (used by LANG_STARTS_WITH() macros)

    // Get hyph method/dictionary from _hyph_dict_table
    _hyph_method = TextLangMan::getHyphMethodForLang(lang_tag);

    // Cleanup if we got "en#@something" from legacy HyphMan methods
    int h_pos = lang_tag.pos("#");
    if ( h_pos > 0 ) {
        lang_tag = lang_tag.substr(0, h_pos);
        hb_lang_tag = hb_lang_tag.substr(0, h_pos); // Also clean the one for HB
    }
    #ifdef DEBUG_LANG_USAGE
    printf("TextLangCfg %s created (%s %s)\n", UnicodeToLocal(_lang_tag).c_str(),
                    UnicodeToLocal(lang_tag).c_str(), UnicodeToLocal(_hyph_method->getId()).c_str());
    #endif

    // https://drafts.csswg.org/css-text-3/#script-tagging
    // We might need to check for the script subpart (optional 2nd
    // subpart) Lant, Hant, Hrkt... and make some non latin language
    // with a Lant script behave more like latin languages...

    // Note that Harfbuzz seems to do the right same thing with
    // either "zh-TW" and "zh-Hant".

    // See for more clever/complex handling of lang tags:
    // https://android.googlesource.com/platform/frameworks/minikin/+/refs/heads/master/libs/minikin/Locale.cpp

    // We thought about adding a 2nd fallback font per-language, but it feels
    // a bit wrong to limit this feature to documents with lang tags.
    // Better to implement a generic font fallback chain independant of language.

    // https://unicode.org/reports/tr14/#Hyphen : in Polish and Portuguese,
    // a real hyphen at end of line must be duplicated at start of next line.
    _duplicate_real_hyphen_on_next_line = false;

#if USE_HARFBUZZ==1
    _hb_language = hb_language_from_string(UnicodeToLocal(hb_lang_tag).c_str(), -1);
#endif

#if USE_LIBUNIBREAK==1
    // libunibreak per-language LineBreakProperties extensions
    //
    // Rules extracted from libunibreak/src/linebreakdef.c, so we can adapt
    // them and build LineBreakProperties adequately for more languages.
    // See https://en.wikipedia.org/wiki/Quotation_mark
    // These are mostly need only for languages that may add a space between
    // the quote and its content - otherwise, the quote will be part of the
    // word it sticks to, and break will be allowed on the other side which
    // probably is a space.
    // When a language allows the use of unpaired quotes (same quote on both
    // sides), it seems best to not specify anything.
    bool has_left_single_quotation_mark_opening = false;   // U+2018 ‘
    bool has_left_single_quotation_mark_closing = false;
    bool has_right_single_quotation_mark_opening = false;  // U+2019 ’
    bool has_right_single_quotation_mark_closing = false;
    bool has_right_single_quotation_mark_glue = false;
    bool has_left_double_quotation_mark_opening = false;   // U+201C “
    bool has_left_double_quotation_mark_closing = false;
    bool has_right_double_quotation_mark_opening = false;  // U+201D ”
    bool has_right_double_quotation_mark_closing = false;
    bool has_left_single_angle_quotation_mark_opening = false;   // U+2039 ‹
    bool has_left_single_angle_quotation_mark_closing = false;
    bool has_right_single_angle_quotation_mark_opening = false;  // U+203A ›
    bool has_right_single_angle_quotation_mark_closing = false;
    bool has_left_double_angle_quotation_mark_opening = false;   // U+00AB «
    bool has_left_double_angle_quotation_mark_closing = false;
    bool has_right_double_angle_quotation_mark_opening = false;  // U+00BB »
    bool has_right_double_angle_quotation_mark_closing = false;
    // Additional rule for treating em-dashes as e.g. "horizontal bar"
    // This is appropriate for languages that typically have a space at a
    // breakable side of the dash
    bool has_em_dash_alphabetic = false; // U+2014 —, U+2E3A ⸺, U+2E3B ⸻

    // Note: these macros use 'lang_tag'.
    if ( LANG_STARTS_WITH(("en")) ) { // English
        has_left_single_quotation_mark_opening = true; // no right..closing in linebreakdef.c
        has_left_double_quotation_mark_opening = true;
        has_right_double_quotation_mark_closing = true;
    }
    else if ( LANG_STARTS_WITH(("fr") ("es")) ) { // French, Spanish
        has_left_single_quotation_mark_opening = true; // no right..closing in linebreakdef.c
        has_left_double_quotation_mark_opening = true;
        has_right_double_quotation_mark_closing = true;
        has_left_single_angle_quotation_mark_opening = true;
        has_right_single_angle_quotation_mark_closing = true;
        has_left_double_angle_quotation_mark_opening = true;
        has_right_double_angle_quotation_mark_closing = true;
        has_em_dash_alphabetic = true;
    }
    else if ( LANG_STARTS_WITH(("de")) ) { // German
        has_left_single_quotation_mark_closing = true;
        has_right_single_quotation_mark_glue = true;
        has_left_double_quotation_mark_closing = true;
        has_left_single_angle_quotation_mark_closing = true;
        has_right_single_angle_quotation_mark_opening = true;
        has_left_double_angle_quotation_mark_closing = true;
        has_right_double_angle_quotation_mark_opening = true;
    }
    else if ( LANG_STARTS_WITH(("ru")) ) { // Russian
        has_left_double_quotation_mark_closing = true;
        has_left_double_angle_quotation_mark_opening = true;
        has_right_double_angle_quotation_mark_closing = true;
    }
    else if ( LANG_STARTS_WITH(("zh")) ) { // Chinese
        has_left_single_quotation_mark_opening = true;
        has_right_single_quotation_mark_closing = true;
        has_left_double_quotation_mark_opening = true;
        has_right_double_quotation_mark_closing = true;
    }
    // Add languages rules here, or reuse previous one with other languages if needed.

    // Set up _lb_props.
    // Important: the unicode indices must be in strict ascending order (or libunibreak
    // might abort checking them all)
    int n = 0;
    if ( has_left_double_angle_quotation_mark_opening )  _lb_props[n++] = { 0x00AB, 0x00AB, LBP_OP };
    if ( has_left_double_angle_quotation_mark_closing )  _lb_props[n++] = { 0x00AB, 0x00AB, LBP_CL };
    // Soft-Hyphens are handled by Hyphman hyphenate(), have them handled as Zero-Width-Joiner by
    // libunibreak so they don't allow any break and don't prevent hyphenate() to handle them correctly.
    _lb_props[n++] = { 0x00AD, 0x00AD, LBP_ZWJ };
    if ( has_right_double_angle_quotation_mark_opening ) _lb_props[n++] = { 0x00BB, 0x00BB, LBP_OP };
    if ( has_right_double_angle_quotation_mark_closing ) _lb_props[n++] = { 0x00BB, 0x00BB, LBP_CL };
    if ( has_em_dash_alphabetic )                        _lb_props[n++] = { 0x2014, 0x2014, LBP_AL };
    if ( has_left_single_quotation_mark_opening )        _lb_props[n++] = { 0x2018, 0x2018, LBP_OP };
    if ( has_left_single_quotation_mark_closing )        _lb_props[n++] = { 0x2018, 0x2018, LBP_CL };
    if ( has_right_single_quotation_mark_opening )       _lb_props[n++] = { 0x2019, 0x2019, LBP_OP };
    if ( has_right_single_quotation_mark_closing )       _lb_props[n++] = { 0x2019, 0x2019, LBP_CL };
    if ( has_right_single_quotation_mark_glue )          _lb_props[n++] = { 0x2019, 0x2019, LBP_GL };
    if ( has_left_double_quotation_mark_opening )        _lb_props[n++] = { 0x201C, 0x201C, LBP_OP };
    if ( has_left_double_quotation_mark_closing )        _lb_props[n++] = { 0x201C, 0x201C, LBP_CL };
    if ( has_right_double_quotation_mark_opening )       _lb_props[n++] = { 0x201D, 0x201D, LBP_OP };
    if ( has_right_double_quotation_mark_closing )       _lb_props[n++] = { 0x201D, 0x201D, LBP_CL };
    if ( has_left_single_angle_quotation_mark_opening )  _lb_props[n++] = { 0x2039, 0x2039, LBP_OP };
    if ( has_left_single_angle_quotation_mark_closing )  _lb_props[n++] = { 0x2039, 0x2039, LBP_CL };
    if ( has_right_single_angle_quotation_mark_opening ) _lb_props[n++] = { 0x203A, 0x203A, LBP_OP };
    if ( has_right_single_angle_quotation_mark_closing ) _lb_props[n++] = { 0x203A, 0x203A, LBP_CL };
    if ( has_em_dash_alphabetic )                        _lb_props[n++] = { 0x2E3A, 0x2E3B, LBP_AL };
    // End of list
    _lb_props[n++] = { 0, 0, LBP_Undefined };
        // When adding properties, be sure combinations for all languages
        // do fit in _lb_props[MAX_NB_LB_PROPS_ITEMS] (MAX_NB_LB_PROPS_ITEMS
        // is defined in textlang.h, currently at 20).
    // Done with libunibreak per-language LineBreakProperties extensions

    // Other line breaking and text layout tweaks
    _lb_char_sub_func = NULL;
#if KO_LIBUNIBREAK_PATCH==1
    if ( LANG_STARTS_WITH(("en")) ) { // English
        _lb_char_sub_func = &lb_char_sub_func_english;
    } else
#endif
    if ( LANG_STARTS_WITH(("pl")) ) { // Polish
        _lb_char_sub_func = &lb_char_sub_func_polish;
        _duplicate_real_hyphen_on_next_line = true;
    }
    else if ( LANG_STARTS_WITH(("cs") ("sk")) ) { // Czech, Slovak
        _lb_char_sub_func = &lb_char_sub_func_czech_slovak;
    }
    else if ( LANG_STARTS_WITH(("pt") ("sr")) ) { // Portuguese, Serbian
        _duplicate_real_hyphen_on_next_line = true;
    }
#endif

    // Language default opening and closing quotes, for CSS
    //   "q::before { content: open-quote }" and
    //   "q::after  { content: close-quote }"
    quotes_spec * quotes = &_quotes_spec_default;
    for (int i=0; _quotes_spec_table[i].lang_tag!=NULL; i++) {
        if ( lang_tag.startsWith( _quotes_spec_table[i].lang_tag ) ) {
            quotes = &_quotes_spec_table[i];
            break;
        }
    }
    // Avoid a wrap after/before an opening/close quote.
    const lChar32 * quote_joiner = U"\x2060";
        // (Zero width, equivalent to deprecated ZERO WIDTH NO-BREAK SPACE)
        // We might want with some languages to use a non-breaking thin space instead.

    _open_quote1  << quotes->open_quote_level_1    << quote_joiner;
    _close_quote1 << quote_joiner   << quotes->close_quote_level_1;
    _open_quote2  << quotes->open_quote_level_2    << quote_joiner;
    _close_quote2 << quote_joiner   << quotes->close_quote_level_2;

    resetCounters();
}

TextLangCfg::~TextLangCfg() {
}

void TextLangCfg::resetCounters() {
    _quote_nesting_level = 0;
}

lString32 & TextLangCfg::getOpeningQuote( bool update_level ) {
    if ( !update_level )
        return _open_quote1;
    _quote_nesting_level++;
    return (_quote_nesting_level % 2) ? _open_quote1 : _open_quote2;
}

lString32 & TextLangCfg::getClosingQuote( bool update_level ) {
    if ( !update_level )
        return _close_quote1;
    _quote_nesting_level--;
    return ((_quote_nesting_level+1) % 2) ? _close_quote1 : _close_quote2;
}

int TextLangCfg::getHyphenHangingPercent() {
    return 70; // 70%
}

int TextLangCfg::getHangingPercent( bool right_hanging, bool & check_font, const lChar32 * text, int pos, int next_usable ) {
    // We get provided with the BiDi re-ordered m_text (so, visually
    // ordered) and the index of char: if needed, we can look at
    // previous or next chars for context to decide how much to hang
    // (i.e. consecutive punctuations).

    // If we ever need to tweak this per language, try to avoid checks
    // for the lang_tag in here:
    // - either set bool members to enable or disable some checks and tweaks
    // - or make this hanging_percent_func_generic, and add dedicated
    //   functions per language, hanging_percent_func_french, that
    //   could fallback to calling hanging_percent_func_generic after
    //   some checks - and have TextLangCfg::getHangingPercent() call
    //   the dedicated function pointer stored as a member.

    // We might want to prevent any hanging with Chinese and Japanese
    // as the text might be mostly full-width glyphs, and this might
    // break the grid. This is less risky if the main font is a CJK
    // font, but if it is not, punctuation might be picked from the
    // main non-CJK font and won't be full-width.
    // Or we could round any value to 0 or 100%  (and/or tweak any
    // glyph in lvtextfm.cpp so it looks like it is full-width).

    lChar32 ch = text[pos];
    int ratio = 0;

    // In French, there's usually a space before and after guillemets,
    // or before a quotation mark. Having them hanging, and then a
    // space, looks like there's a hole in the margin.
    // So, for some chars, we'll avoid hanging or reduce the hanging
    // ratio if the next/prev char is a space char.
    // This might not happen in other languages, so let's do that
    // prevention generically. If needed, make that dependant on
    // a boolean member, set to true if LANG_STARTS_WITH(("fr")).
    bool space_alongside = false;
    if ( right_hanging ) {
        if ( pos > 0 ) {
            lChar32 prev_ch = text[pos-1];
            if ( prev_ch == 0x0020 || prev_ch == 0x00A0 || (prev_ch >= 0x2000 && prev_ch <= 0x200A ) ) {
                // Normal space, no-break space, and other unicode spaces (except zero-width ones)
                space_alongside = true;
            }
        }
    }
    else {
        if ( next_usable > 0 ) {
            lChar32 next_ch = text[pos+1];
            if ( next_ch == 0x0020 || next_ch == 0x00A0 || (next_ch >= 0x2000 && next_ch <= 0x200A ) ) {
                // Normal space, no-break space, and other unicode spaces (except zero-width ones)
                space_alongside = true;
            }
        }
    }

    // For the common punctuations, parens and quotes, we check and
    // return the same value whether asked for left or right hanging.
    // Normally, libunibreak has prevented them from happening on
    // one of the sides - but with RTL text, they may happen on
    // the other side. Also, some BiDi mirrorable chars "([])" might
    // be mirrored in the provided *text when not-using HarfBuzz, but
    // won't be mirrored when using HarfBuzz - so let's handle
    // all of them no matter the hanging side asked for.
    // Also, because in some languages, quotation marks and guillemets
    // are used reverted, we include left and right ones in both sets.

    // Most values taken from the "protusion" section in:
    // https://source.contextgarden.net/tex/context/base/mkiv/font-imp-quality.lua
    // https://www.w3.org/Mail/flatten/index?subject=Amending+hanging-punctuation+for+Western+typography&list=www-style
    // and the microtypography thesis: http://www.pragma-ade.nl/pdftex/thesis.pdf
    // (screenshot at https://github.com/koreader/koreader/issues/6235#issuecomment-639307634)

    switch (ch) {
        case 0x0027: // ' single quote
        case 0x002C: // , comma
        case 0x002D: // - minus
        case 0x002E: // . period
        case 0x0060: // ` back quote
        // case 0x00AD: // soft hyphen (we don't draw them, so don't handle them)
        case 0x060C: // ، arabic comma
        case 0x06D4: // ۔ arabic full stop
        case 0x2010: // ‐ hyphen
        case 0x2018: // ‘ left single quotation mark
        case 0x2019: // ’ right single quotation mark
        case 0x201A: // ‚ single low-9 quotation mark
        case 0x201B: // ‛ single high-reversed-9 quotation mark
            ratio = 70;
            break;
        case 0x2039: // ‹ left single guillemet
        case 0x203A: // › right single guillemet
            // These are wider than the previous ones, and hanging by 70% with a space
            // alongside can give a feeling of bad justification. So, hang less.
            ratio = space_alongside ? 20 : 70;
            break;
        case 0x0022: // " double quote
        case 0x003A: // : colon
        case 0x003B: // ; semicolon
        case 0x061B: // ؛ arabic semicolon
        case 0x201C: // “ left double quotation mark
        case 0x201D: // ” right double quotation mark
        case 0x201E: // „ double low-9 quotation mark
        case 0x201F: // ‟ double high-reversed-9 quotation mark
            ratio = 50;
            break;
        case 0x00AB: // « left guillemet
        case 0x00BB: // » right guillemet
            // These are wider than the previous ones, and hanging by 50% with a space
            // alongside can give a feeling of bad justification. So, hang less.
            ratio = space_alongside ? 20 : 50;
            break;
        case 0x2013: // – endash
            // Should have enough body inside (with only 30% hanging)
            ratio = 30;
            break;
        case 0x0021: // !
        case 0x003F: // ?
        case 0x00A1: // ¡
        case 0x00BF: // ¿
        case 0x061F: // ؟
        case 0x2014: // — emdash
        case 0x2026: // … ellipsis
            // These will have enough body inside (with only 20% hanging),
            // so they shouldn't hurt when space_alongside.
            ratio = 20;
            break;
        case 0x0028: // (
        case 0x0029: // )
        case 0x005B: // [
        case 0x005D: // ]
        case 0x007B: // {
        case 0x007D: // }
            ratio  = 5;
            break;
        default:
            break;
    }
    if ( ratio ) {
        check_font = false;
        return ratio;
    }
    // Other are non punctuation but slight adjustment for some letters,
    // that might be ignored if the font already include some negative
    // left side bearing.
    // The hanging ratio is small, so no need to correct if space_alongside.
    check_font = true;
    if ( right_hanging ) {
        switch (ch) {
            case 'A':
            case 'F':
            case 'K':
            case 'L':
            case 'T':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'k':
            case 'r':
            case 't':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
                ratio  = 5;
                break;
            default:
                break;
        }
    }
    else { // left hanging
        switch (ch) {
            case 'A':
            case 'J':
            case 'T':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
                ratio  = 5;
                break;
            default:
                break;
        }
    }
    return ratio;
}
