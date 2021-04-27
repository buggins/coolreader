/*
MathML specific DOM building and rendering code

Main references:
 https://www.w3.org/TR/MathML/ MathML Version 3.0 (2014)
 https://www.w3.org/TR/MathML2/ MathML Version 2.0 (2003)

 https://mathml-refresh.github.io/mathml-core/
 https://github.com/mathml-refresh/mathml
   "This specification defines a core subset of Mathematical Markup Language,
   or MathML, that is suitable for browser implementation [...] being as
   accurate as possible on the visual rendering of mathematical formulas."

Other related informations and discussions:
 https://www.w3.org/Math/testsuite/
 http://fred-wang.github.io/AcidTestsMathML/acid3/description.html
 http://www.unicode.org/reports/tr25/ Unicode support for mathematics
 https://en.wikipedia.org/wiki/Mathematical_Alphanumeric_Symbols

Additional CSS tweaks by some implementations
 https://github.com/WebKit/WebKit/blob/main/Source/WebCore/css/mathml.css
 https://searchfox.org/mozilla-central/source/layout/mathml/mathml.css#16

Some attemps at supporting MathML via CSS only, that we took some bits from:
 https://www.w3.org/TR/mathml-for-css/
 https://github.com/fred-wang/mathml.css

Harfbuzz OT math support:
 https://github.com/harfbuzz/harfbuzz/issues/235
 https://github.com/harfbuzz/harfbuzz/issues/2585
Harfbuzz code (not merged) to properly shape/draw stretchy operators:
 https://github.com/fred-wang/harfbuzz/tree/MATH-3
 https://frederic-wang.fr/opentype-math-in-harfbuzz.html

Math fonts:
 https://tex.stackexchange.com/questions/425098/which-opentype-math-fonts-are-available
 https://developer.mozilla.org/en-US/docs/Mozilla/MathML_Project/Fonts
 http://fred-wang.github.io/MathFonts/
 https://docs.microsoft.com/en-us/typography/opentype/otspec183/math

Other implementations:
In rust:
 https://github.com/manuel-rhdt/mlayout/blob/master/src/typesetting/shaper.rs
Firefox:
 https://github.com/mozilla/gecko-dev/tree/master/layout/mathml
 https://github.com/mozilla/gecko-dev/blob/master/layout/mathml/nsMathMLmoFrame.cpp
 https://github.com/mozilla/gecko-dev/blob/master/layout/mathml/nsMathMLChar.cpp
 https://github.com/mozilla/gecko-dev/blob/master/gfx/thebes/gfxFT2Utils.cpp font metrics
Chromium:
 https://mathml.igalia.com/project/
MathJax:
 https://github.com/mathjax/MathJax-src/blob/master/ts/core/MmlTree/OperatorDictionary.ts
 https://github.com/mathjax/MathJax-src/blob/master/ts/core/MmlTree/MmlNodes/mo.ts

How various implementations render <mover> with accent or not...:
 https://github.com/mathml-refresh/mathml/issues/210#issuecomment-700367434

What's currently supported by our implementation:
- Simple grouping <math> (with its display attribute), <mrow>, <mstyle>,
  <merror>, <mpadded>, <mphantom> (with visibility: hidden), <menclose>,
  but only a small set of their attributes (mostly handled via CSS).
- Simple text containers: <mi>, <mn>, <mo>, <mtext> and <ms>, with
  support for the mathvariant attribute (and italic'ing of <mi> with
  a single char), mathsize, mathcolor, mathbackground.
- Fraction <mfrac>, with linethickness zero and non-zero.
- Roots <msqrt> and <mroot>.
- Postscripts: <msub>, <msup>, <msubsup>.
- Multiple pre and post scripts: <mmultiscripts>.
- Under and over scripts: <munder>, <mover>, <munderover>.
- Basic HTML-like implementation of <mtable>.
- <mspace>, very limited support for <mpadded>
- <mo> and embelished operators, via operator dictionary or attributes:
  support for 'form', 'largeop', 'movablelimits', 'accent' (and the 'accent'
  and 'accentunder' attributes on <munderover>), 'stretchy', 'symmetric',
  'minsize' and 'maxsize'. 'lspace' and 'rspace' (forwarded to the sides
  of an upper embelished container).
- Scaling of largeop and stretchy operators (but not using the font OT math
  variants nor shaping with multiple glyphs).
- Support of length attributes with classic CSS units, and with
  namedspaces ("thinmathspace"...)

Current limitations:
- Some rare MathML elements are not handled in any specific way (at most, by some
  simple styling via CSS), mostly those not mentionned in the mathml-core spec:
  <menclosed>, <mstack> (and its inner elements), <mlongdiv> (and its inner elements).
- Many attributes mentionned in the MathML spec (very few of them are mentionned
  in the mathml-core spec) are not handled at all (none for <mstyle>, few for <mpadded>,
  only those for <mtable> that are named the same for HTML <table>, ...)
- Especially, some fine positionning in elements we otherwise support well are
  not (like <msuper superscriptshift=...>).
- No specific linebreaks handling: stuff might be wrapped inside inline-blocks
  that we abuse for rendering.
- Balanced strechy operators may have their heights unbalanced if they and
  their inner content happen to wrap onto 2 text lines.
- Because of the way vertical stretchy operators are handled (adjusting to
  the erm_final line containing them), 2 independant inline <math> elements
  on a same text line can have their stretchy operators get the same height,
  so not adjusting to the height of their inner content... (note that
  Firefox does not stretch a <mo>, while MathJax does, when it's a direct
  child of an inline <math> but it does when there is a <mrow> in between
  (may be to avoid having our behaviour?)
- Simple handling of stretchy operators by just scaling the glyph (using
  OpenType math features to build a composition from multiply glyphs is
  not implemented).
- The drawing of the <msqrt> and <mroot> symbols is not handled as per the
  mathml-core specs (fine positionning via the font OT math metric, using
  a the radical symbol glyph of the font), but is done via table borders
  and a background SVG image.

*/

#include "crsetup.h"

#if MATHML_SUPPORT==1

#include "../include/lvtypes.h"
#include "../include/lvstring.h"
#include "../include/lvtinydom.h"
#include "../include/lvrend.h"
#include "../include/lvstsheet.h"
#include "../include/fb2def.h"
#include "../include/mathml.h"

// ====================================================================
// Known Open Type Math fonts
// ====================================================================

// Our CSS stylesheet does not enforce a font-family, as we don't want to
// override a font set by the publisher or the user.
// But if there is none set, or the default inherited font does not have
// OT Math support, we'll pick a math font from this list.
//
// Hardcoded list of known fonts with good OpenType Math support
// See https://github.com/WebKit/WebKit/blob/main/Source/WebCore/css/mathml.css
// for some alternative list with good information about the pro/cons of many fonts.
// We limit the list to a few good ubiquitous Math fonts a user can drop
// among its fonts to have it picked first, with FreeSerif (shipped with
// KOReader) as the last fallback, as it happens to be a good math font
// that works quite well.
static lString8 MATHML_DEFAULT_FONTS = lString8(
    "\""    "Latin Modern Math"     "\", "
    "\""    "STIX Two Math"         "\", "
    "\""    "XITS Math"             "\", "
    "\""    "Libertinus Math"       "\", "
    "\""    "TeX Gyre Termes Math"  "\", "
    "\""    "TeX Gyre Bonum Math"   "\", "
    "\""    "TeX Gyre Schola Math"  "\", "
    "\""    "TeX Gyre Pagella Math" "\", "
    "\""    "DejaVu Math TeX Gyre"  "\", "
    "\""    "Asana Math"            "\", "
    "\""    "Cambria Math"          "\", "
    "\""    "Lucida Bright Math"    "\", "
    "\""    "Minion Math"           "\", "
    "\""    "FreeSerif"             "\""
);
// ("STIX Math" not included, as the one I tested with has really huge
// spacing and looks really bad.)

// ====================================================================
// Load internal CSS stylesheet for some part of elements styling
// ====================================================================

static LVStyleSheet _MathML_stylesheet;
static bool _MathML_stylesheet_loaded = false;

// Hard-include the MathML stylesheet, as it's mostly an help
// to the code here to properly set up the DOM, and is not
// aimed at being public and customizable.
static const char * MATHML_CSS =
#include "mathml_css_h.css"
;

// Allow parsing it at run-time when developping:
// Uncomment to parse mathml_css_h.css at run-time
// #define MATHML_CSS_DEVEL

static bool loadMathMLStylesheet( ldomNode * node ) {
    // We need a document to parse a stylesheet, to get elements and attributes IDs from.
    // We should use only those defined in fb2def.h, no matter gDomVersionRequested.
    // Note: we should avoid using pseudo classes >= csspc_last_child in the MathML
    // stylesheet, or we would be reparsing this first document.
    _MathML_stylesheet.setDocument( node->getDocument() );
    bool ok = _MathML_stylesheet.parse( MATHML_CSS );
    #ifdef MATHML_CSS_DEVEL
        // Reset hard-included one
        printf("MATHML_CSS_DEVEL checking mathml_css_h.css\n");
        // As accessible by crengine when run from KOReader emulator
        lString8 mathml_css = lString8("data/../../crengine/src/mathml_css_h.css");
        LVStreamRef file = LVOpenFileStream( mathml_css.c_str(), LVOM_READ );
        if ( !file.isNull() ) {
            printf("MATHML_CSS_DEVEL found mathml_css_h.css\n");
            ok = false;
            _MathML_stylesheet.clear();
            lString8 css = UnicodeToUtf8( LVReadTextFile( file ) );
            css = css.substr(css.pos('\n')); // Remove peculiar first line
            if ( !css.empty() ) {
                ok = _MathML_stylesheet.parse(css.c_str());
            }
            printf("MATHML_CSS_DEVEL parsed mathml_css_h.css: %s\n", ok?"ok":"FAILED");
        }
        else {
            printf("MATHML_CSS_DEVEL mathml_css_h.css not found, using hardcoded one\n");
        }
    #endif
    _MathML_stylesheet.setDocument( NULL );
    return ok;
}
/* For debugging, one can add bits of the following in mathml_css_h.css:
mo[Mform="prefix"] { background-color: #ff8888; }
mo[Mform="infix"] { background-color: #88ff88; }
mo[Mform="postfix"] { background-color: #8888ff; }
mo[Mform="prefix-f"] { background-color: #ffbbbb; }
mo[Mform="infix-f"] { background-color: #bbffbb; }
mo[Mform="postfix-f"] { background-color: #bbbbff; }
*[MD], mathBox[MD] { background-color: #ffff88; }
*[ML], mathBox[ML] { border-left: 1px dashed #ff4444; }
*[MR], mathBox[MR] { border-right: 1px dashed #4444ff; }
*[Mtransform=vstretch], mathBox[Mtransform=vstretch] { border: 1px solid red; }
*[Memb], mathBox[Memb], tabularBox[Memb], inlineBox[Memb] { border: 1px dashed blue; }
*[Memb=top], mathBox[Memb=top], tabularBox[Memb=top], inlineBox[Memb=top] { border: 1px solid blue; }
*/


// ====================================================================
// Text chars substitution for mathvariant= attribute
// ====================================================================

// MathML elements may have a mathvariant= attribute, specifying some
// kind of "font style" the inner text should be rendered with.
// The rendering of non-normal style is different from what happens
// with regular HTML/CSS (where we may use another italic or bold
// font instance).
// with MathML, we're supposed to use a math font, which always come in
// a single font style: regular. But math fonts are expected to have glyphs
// for some specific Unicode codepoints that are assigned to styled variants
// of the classic latin alphanumeric and greek uppercase and lowercase chars.
// So, when parsing the XML and meeting <mi mathvariant="italic">abc</mi>,
// we should insert in the DOM text node, not the regular ASCII "abc" chars,
// but their italic Unicode counterpart: U+1D44E U+1D44F U+1D450.
// It is OK from the MathML specs to not style at all chars that are not
// among these A-Z a-z 0_9 blocks.
// https://www.w3.org/TR/MathML/chapter7.html#chars.BMP-SMP
enum mathml_mathvariant_t {
    mathml_mathvariant_normal = 0,
    mathml_mathvariant_bold,
    mathml_mathvariant_italic,
    mathml_mathvariant_bold_italic,
    mathml_mathvariant_sans_serif,
    mathml_mathvariant_bold_sans_serif,
    mathml_mathvariant_sans_serif_italic,
    mathml_mathvariant_sans_serif_bold_italic,
    mathml_mathvariant_script,
    mathml_mathvariant_bold_script,
    mathml_mathvariant_fraktur,
    mathml_mathvariant_bold_fraktur,
    mathml_mathvariant_monospace,
    mathml_mathvariant_double_struck,
    mathml_mathvariant_initial, /* this and next ones only matter with arabic */
    mathml_mathvariant_tailed,
    mathml_mathvariant_looped,
    mathml_mathvariant_stretched
};
static const char * mathml_mathvariant_names[] = {
    "normal",
    "bold",
    "italic",
    "bold-italic",
    "sans-serif",
    "bold-sans-serif",
    "sans-serif-italic",
    "sans-serif-bold-italic",
    "script",
    "bold-script",
    "fraktur",
    "bold-fraktur",
    "monospace",
    "double-struck",
    "initial",
    "tailed",
    "looped",
    "stretched",
    NULL
};
static int parse_name( lString32 value, const char * * names, int def_value=-1 ) {
    for (int i=0; names[i]; i++) {
        if ( value == names[i] ) {
            return i; // found!
        }
    }
    return def_value;
}
static lChar32 substitute_codepoint(lChar32 c, mathml_mathvariant_t mathvariant) {
    // Returning 0 means no substitution needed.
    // We only handle the blocks that have Unicode codepoints styled equivalent.
    // Reference: https://en.wikipedia.org/wiki/Mathematical_Alphanumeric_Symbols
    // Other reference (found after making this, haven't checked if they differ):
    // https://mathml-refresh.github.io/mathml-core/#new-text-transform-mappings
    // todo: when "no substitution for..", we could use those from an other variant
    // which might look better.
    switch (mathvariant) {
    case mathml_mathvariant_normal:
        // Nothing to substitute when normal
        return 0;
        break;
    case mathml_mathvariant_bold:
        if ( c >= '0' && c <= '9' ) return c - '0' + 0x1D7CE;
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D400;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D41A;
        if ( c >= 0x0391 && c <= 0x03A9 ) return c - 0x0391 + 0x1D6A8; // Greek capital letters
        if ( c == 0x03F4 ) return 0x1D6B9; // Greek capital theta
        if ( c == 0x2207 ) return 0x1D6C1; // Nabla
        if ( c >= 0x03B1 && c <= 0x03C9 ) return c - 0x03B1 + 0x1D6C2; // Greek small letters
        if ( c == 0x2202 ) return 0x1D6DB; // Partial differential
        if ( c == 0x03F5 ) return 0x1D6DC; // Greek small epsilon
        if ( c == 0x03D1 ) return 0x1D6DD; // Greek small theta
        if ( c == 0x03F0 ) return 0x1D6DE; // Greek small kappa
        if ( c == 0x03D5 ) return 0x1D6DF; // Greek small phi
        if ( c == 0x03F1 ) return 0x1D6E0; // Greek small rho
        if ( c == 0x03D6 ) return 0x1D6E1; // Greek small pi
        if ( c == 0x03DC ) return 0x1D7CA; // Greek capital digamma (only available for bold)
        if ( c == 0x03DD ) return 0x1D7CB; // Greek small digamma (only available for bold)
        break;
    case mathml_mathvariant_italic:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D434;
        if ( c >= 'a' && c <= 'z' ) {
            if ( c == 'h' ) return 0x210E;
            return c - 'a' + 0x1D44E;
        }
        if ( c == 0x0131 ) return 0x1D6A4; // Latin small letter dotless i (only available for italic)
        if ( c == 0x0237 ) return 0x1D6A5; // Latin small letter dotless j (only available for italic)
        if ( c >= 0x0391 && c <= 0x03A9 ) return c - 0x0391 + 0x1D6E2; // Greek capital letters
        if ( c == 0x03F4 ) return 0x1D6F3; // Greek capital theta
        if ( c == 0x2207 ) return 0x1D6FB; // Nabla
        if ( c >= 0x03B1 && c <= 0x03C9 ) return c - 0x03B1 + 0x1D6FC; // Greek small letters
        if ( c == 0x2202 ) return 0x1D715; // Partial differential
        if ( c == 0x03F5 ) return 0x1D716; // Greek small epsilon
        if ( c == 0x03D1 ) return 0x1D717; // Greek small theta
        if ( c == 0x03F0 ) return 0x1D718; // Greek small kappa
        if ( c == 0x03D5 ) return 0x1D719; // Greek small phi
        if ( c == 0x03F1 ) return 0x1D71A; // Greek small rho
        if ( c == 0x03D6 ) return 0x1D71B; // Greek small pi
        break;
    case mathml_mathvariant_bold_italic:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D468;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D482;
        if ( c >= 0x0391 && c <= 0x03A9 ) return c - 0x0391 + 0x1D71C; // Greek capital letters
        if ( c == 0x03F4 ) return 0x1D72D; // Greek capital theta
        if ( c == 0x2207 ) return 0x1D735; // Nabla
        if ( c >= 0x03B1 && c <= 0x03C9 ) return c - 0x03B1 + 0x1D736; // Greek small letters
        if ( c == 0x2202 ) return 0x1D74F; // Partial differential
        if ( c == 0x03F5 ) return 0x1D750; // Greek small epsilon
        if ( c == 0x03D1 ) return 0x1D751; // Greek small theta
        if ( c == 0x03F0 ) return 0x1D752; // Greek small kappa
        if ( c == 0x03D5 ) return 0x1D753; // Greek small phi
        if ( c == 0x03F1 ) return 0x1D754; // Greek small rho
        if ( c == 0x03D6 ) return 0x1D755; // Greek small pi
        break;
    case mathml_mathvariant_sans_serif:
        if ( c >= '0' && c <= '9' ) return c - '0' + 0x1D7E2;
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D5A0;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D5BA;
        // No substitution for greek
        break;
    case mathml_mathvariant_bold_sans_serif:
        if ( c >= '0' && c <= '9' ) return c - '0' + 0x1D7EC;
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D5D4;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D5EE;
        if ( c >= 0x0391 && c <= 0x03A9 ) return c - 0x0391 + 0x1D756; // Greek capital letters
        if ( c == 0x03F4 ) return 0x1D767; // Greek capital theta
        if ( c == 0x2207 ) return 0x1D76F; // Nabla
        if ( c >= 0x03B1 && c <= 0x03C9 ) return c - 0x03B1 + 0x1D770; // Greek small letters
        if ( c == 0x2202 ) return 0x1D789; // Partial differential
        if ( c == 0x03F5 ) return 0x1D78A; // Greek small epsilon
        if ( c == 0x03D1 ) return 0x1D78B; // Greek small theta
        if ( c == 0x03F0 ) return 0x1D78C; // Greek small kappa
        if ( c == 0x03D5 ) return 0x1D78D; // Greek small phi
        if ( c == 0x03F1 ) return 0x1D78E; // Greek small rho
        if ( c == 0x03D6 ) return 0x1D78F; // Greek small pi
        break;
    case mathml_mathvariant_sans_serif_italic:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D608;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D622;
        // No substitution for greek
        break;
    case mathml_mathvariant_sans_serif_bold_italic:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D63C;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D656;
        if ( c >= 0x0391 && c <= 0x03A9 ) return c - 0x0391 + 0x1D790; // Greek capital letters
        if ( c == 0x03F4 ) return 0x1D7A1; // Greek capital theta
        if ( c == 0x2207 ) return 0x1D7A9; // Nabla
        if ( c >= 0x03B1 && c <= 0x03C9 ) return c - 0x03B1 + 0x1D7AA; // Greek small letters
        if ( c == 0x2202 ) return 0x1D7C3; // Partial differential
        if ( c == 0x03F5 ) return 0x1D7C4; // Greek small epsilon
        if ( c == 0x03D1 ) return 0x1D7C5; // Greek small theta
        if ( c == 0x03F0 ) return 0x1D7C6; // Greek small kappa
        if ( c == 0x03D5 ) return 0x1D7C7; // Greek small phi
        if ( c == 0x03F1 ) return 0x1D7C8; // Greek small rho
        if ( c == 0x03D6 ) return 0x1D7C9; // Greek small pi
        break;
    case mathml_mathvariant_script:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) {
            if ( c == 'B' ) return 0x212C;
            if ( c == 'E' ) return 0x2130;
            if ( c == 'F' ) return 0x2131;
            if ( c == 'H' ) return 0x210B;
            if ( c == 'I' ) return 0x2110;
            if ( c == 'L' ) return 0x2112;
            if ( c == 'M' ) return 0x2133;
            if ( c == 'R' ) return 0x211B;
            return c - 'A' + 0x1D49C;
        }
        if ( c >= 'a' && c <= 'z' ) {
            if ( c == 'e' ) return 0x212F;
            if ( c == 'g' ) return 0x210A;
            if ( c == 'o' ) return 0x2134;
            return c - 'a' + 0x1D4B6;
        }
        break;
    case mathml_mathvariant_bold_script:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D4D0;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D4EA;
        break;
    case mathml_mathvariant_fraktur:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) {
            if ( c == 'C' ) return 0x212D;
            if ( c == 'H' ) return 0x210C;
            if ( c == 'I' ) return 0x2111;
            if ( c == 'R' ) return 0x211C;
            if ( c == 'Z' ) return 0x2128;
            return c - 'A' + 0x1D504;
        }
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D51E;
        break;
    case mathml_mathvariant_bold_fraktur:
        // No substitution for digits
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D56C;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D586;
        break;
    case mathml_mathvariant_monospace:
        if ( c >= '0' && c <= '9' ) return c - '0' + 0x1D7F6;
        if ( c >= 'A' && c <= 'Z' ) return c - 'A' + 0x1D670;
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D68A;
        break;
    case mathml_mathvariant_double_struck:
        if ( c >= '0' && c <= '9' ) return c - '0' + 0x1D7D8;
        if ( c >= 'A' && c <= 'Z' ) {
            if ( c == 'C' ) return 0x2102;
            if ( c == 'H' ) return 0x210D;
            if ( c == 'N' ) return 0x2115;
            if ( c == 'P' ) return 0x2119;
            if ( c == 'Q' ) return 0x211A;
            if ( c == 'R' ) return 0x211D;
            if ( c == 'Z' ) return 0x2124;
            return c - 'A' + 0x1D538;
        }
        if ( c >= 'a' && c <= 'z' ) return c - 'a' + 0x1D552;
        break;
    case mathml_mathvariant_initial: // for arabic only
        if ( c == 0x0641 ) return 0x1EE30;
        if ( c == 0x0642 ) return 0x1EE32;
        if ( c == 0x0643 ) return 0x1EE2A;
        if ( c == 0x0644 ) return 0x1EE2B;
        if ( c == 0x0645 ) return 0x1EE2C;
        if ( c == 0x0646 ) return 0x1EE2D;
        if ( c == 0x0647 ) return 0x1EE24;
        if ( c == 0x0628 ) return 0x1EE21;
        if ( c == 0x064A ) return 0x1EE29;
        if ( c == 0x062B ) return 0x1EE36;
        if ( c == 0x062C ) return 0x1EE22;
        if ( c == 0x062D ) return 0x1EE27;
        if ( c == 0x062E ) return 0x1EE37;
        if ( c == 0x0633 ) return 0x1EE2E;
        if ( c == 0x0634 ) return 0x1EE34;
        if ( c == 0x0635 ) return 0x1EE31;
        if ( c == 0x0636 ) return 0x1EE39;
        if ( c == 0x0639 ) return 0x1EE2F;
        if ( c == 0x063A ) return 0x1EE3B;
        if ( c == 0x062A ) return 0x1EE35;
        break;
    case mathml_mathvariant_tailed: // for arabic only
        if ( c == 0x0642 ) return 0x1EE52;
        if ( c == 0x0644 ) return 0x1EE4B;
        if ( c == 0x0646 ) return 0x1EE4D;
        if ( c == 0x064A ) return 0x1EE49;
        if ( c == 0x062C ) return 0x1EE42;
        if ( c == 0x062D ) return 0x1EE47;
        if ( c == 0x062E ) return 0x1EE57;
        if ( c == 0x066F ) return 0x1EE5F;
        if ( c == 0x0633 ) return 0x1EE4E;
        if ( c == 0x0634 ) return 0x1EE54;
        if ( c == 0x0635 ) return 0x1EE51;
        if ( c == 0x0636 ) return 0x1EE59;
        if ( c == 0x0639 ) return 0x1EE4F;
        if ( c == 0x063A ) return 0x1EE5B;
        if ( c == 0x06BA ) return 0x1EE5D;
        break;
    case mathml_mathvariant_looped: // for arabic only
        if ( c == 0x0627 ) return 0x1EE80;
        if ( c == 0x0628 ) return 0x1EE81;
        if ( c == 0x062A ) return 0x1EE95;
        if ( c == 0x062B ) return 0x1EE96;
        if ( c == 0x062C ) return 0x1EE82;
        if ( c == 0x062D ) return 0x1EE87;
        if ( c == 0x062E ) return 0x1EE97;
        if ( c == 0x062F ) return 0x1EE83;
        if ( c == 0x0630 ) return 0x1EE98;
        if ( c == 0x0631 ) return 0x1EE93;
        if ( c == 0x0632 ) return 0x1EE86;
        if ( c == 0x0633 ) return 0x1EE8E;
        if ( c == 0x0634 ) return 0x1EE94;
        if ( c == 0x0635 ) return 0x1EE91;
        if ( c == 0x0636 ) return 0x1EE99;
        if ( c == 0x0637 ) return 0x1EE88;
        if ( c == 0x0638 ) return 0x1EE9A;
        if ( c == 0x0639 ) return 0x1EE8F;
        if ( c == 0x063A ) return 0x1EE9B;
        if ( c == 0x0641 ) return 0x1EE90;
        if ( c == 0x0642 ) return 0x1EE92;
        if ( c == 0x0644 ) return 0x1EE8B;
        if ( c == 0x0645 ) return 0x1EE8C;
        if ( c == 0x0646 ) return 0x1EE8D;
        if ( c == 0x0647 ) return 0x1EE84;
        if ( c == 0x0648 ) return 0x1EE85;
        if ( c == 0x064A ) return 0x1EE89;
        break;
    case mathml_mathvariant_stretched: // for arabic only
        if ( c == 0x06A1 ) return 0x1EE7E;
        if ( c == 0x0628 ) return 0x1EE61;
        if ( c == 0x062A ) return 0x1EE75;
        if ( c == 0x062B ) return 0x1EE76;
        if ( c == 0x062C ) return 0x1EE62;
        if ( c == 0x062D ) return 0x1EE67;
        if ( c == 0x062E ) return 0x1EE77;
        if ( c == 0x0633 ) return 0x1EE6E;
        if ( c == 0x0634 ) return 0x1EE74;
        if ( c == 0x0635 ) return 0x1EE71;
        if ( c == 0x0636 ) return 0x1EE79;
        if ( c == 0x0637 ) return 0x1EE68;
        if ( c == 0x0638 ) return 0x1EE7A;
        if ( c == 0x0639 ) return 0x1EE6F;
        if ( c == 0x063A ) return 0x1EE7B;
        if ( c == 0x0641 ) return 0x1EE70;
        if ( c == 0x0642 ) return 0x1EE72;
        if ( c == 0x0643 ) return 0x1EE6A;
        if ( c == 0x0645 ) return 0x1EE6C;
        if ( c == 0x0646 ) return 0x1EE6D;
        if ( c == 0x0647 ) return 0x1EE64;
        if ( c == 0x064A ) return 0x1EE69;
        if ( c == 0x066E ) return 0x1EE7C;
        break;
    }
    return 0;
}

// ====================================================================
// MathML text nodes specific handling
// ====================================================================

lString32 MathMLHelper::getMathMLAdjustedText(ldomNode * node, const lChar32 * text, int len) {
    // https://www.w3.org/TR/MathML/chapter2.html#fund.collapse
    // "MathML ignores whitespace occurring outside token elements.
    //  Non-whitespace characters are not allowed there. Whitespace
    //  occurring within the content of token elements, is normalized
    //  as follows. All whitespace at the beginning and end of the
    //  content is removed [...]"
    lUInt16 nodeId = node->getNodeId();
    bool text_allowed;
    if ( nodeId >= EL_MATHML_TOKEN_START && nodeId <= EL_MATHML_TOKEN_END ) {
        text_allowed = true;
    }
    else if ( nodeId == el_mathBox ) {
        text_allowed = true;
    }
    else if ( nodeId >= EL_MATHML_START && nodeId <= EL_MATHML_END ) {
        text_allowed = false;
    }
    else {
        // keep text if we find it in another tag than the MathML ones we know about
        text_allowed = true;
    }
    if ( !text_allowed ) {
        return lString32::empty_str;
    }
    lString32 mtext = lString32(text, len);
    mtext.trim();
    if ( mtext.empty() ) { // No need to create a text node
        return lString32::empty_str;
    }
    mathml_mathvariant_t mathvariant = mathml_mathvariant_normal;
    if ( nodeId == el_mi && mtext.length() == 1 ) {
        // For <mi>, the default is "normal" (non-slanted) unless the content
        // is a single character, in which case it is "italic".
        mathvariant = mathml_mathvariant_italic;
    }
    // If this text node or one of its ancestors has a mathvariant= attribute,
    // we should inherit it
    ldomNode * parent = node;
    while ( parent ) {
        if ( parent->hasAttribute(attr_mathvariant) ) {
            int mvar = parse_name(parent->getAttributeValueLC(attr_mathvariant), mathml_mathvariant_names);
            if ( mvar != -1 ) { // parsed, supported value
                mathvariant = (mathml_mathvariant_t) mvar;
                break;
            }
        }
        if ( parent->getNodeId() == el_math ) {
            break;
        }
        parent = parent->getParentNode();
    }
    // Substitute codepoints for the mathvariant if there is one
    // (no substitution for "normal")
    if ( mathvariant != mathml_mathvariant_normal ) {
        len = mtext.length();
        for ( int i=0; i<len; i++ ) {
            lChar32 c = substitute_codepoint(mtext[i], mathvariant);
            // If c==0, no available substitution : we avoid modifying
            // the lString32 when not needed (as it may malloc/free)
            if (c != 0) {
                mtext[i] = c;
            }
        }
    }
    // Note: mathml_mathvariant_italic glyphs may have some overflowing side bearings,
    // that may protude in the neighbour glyphs if it's not italic (noticable with '(' or '[').
    // (WebKit adds "mi { padding-end: 0.1em; }" possibly to counteract that)

    // Some chars look better if replaced by more mathematical equivalents
    // https://www.w3.org/TR/MathML/chapter7.html#chars.anomalous
    if ( nodeId == el_mo && mtext == U"-" ) {
        mtext = lString32(U"\x2212");
    }
    // Some other substitutions are encouraged (' > prime), but they need
    // a bit more context checking...

    // Some publishers do use unicode combining marks as the single char
    // of a text node - which feels invalid. Freetype would draw it
    // on the left (over a non-existant previous char), and Harfbuzz
    // would add a dotted circcle (that we can remove, but then, drawn
    // on the left).
    // Found no solution to handle them easily.
    // So, convert the ones that have a non-combining equivalement
    // as we meet them.
    // Some small list at https://trac.webkit.org/changeset/203714/webkit
    // More at https://lists.w3.org/Archives/Public/www-math/2018Nov/0005.html
    switch (mtext[0]) {
        case 0x0302:            // COMBINING CIRCUMFLEX ACCENT
            mtext[0] = 0x02C6;  // => MODIFIER LETTER CIRCUMFLEX ACCENT
            break;
        case 0x0303:            // COMBINING TILDE
            mtext[0] = 0x007E;  // => TILDE
            break;
        case 0x0304:            // COMBINING MACRON
            mtext[0] = 0x00AF;  // => MACRON
            break;
        case 0x0305:            // COMBINING OVERLINE
            mtext[0] = 0x00AF;  // => MACRON
            break;
        case 0x030C:            // COMBINING CARON
            mtext[0] = 0x02C7;  // => CARON
            break;
        case 0x0332:            // COMBINING LOW LINE
            mtext[0] = 0x005F;  // => LOW LINE
            break;
        default:
            break;
    }

    if ( nodeId == el_ms ) { // String litteral
        // Escape quote chars found in the text
        lString32 lquote = U"\"";
        lString32 rquote = U"\"";
        if ( node->hasAttribute(attr_lquote) )
            lquote = node->getAttributeValue(attr_lquote);
        if ( node->hasAttribute(attr_rquote) )
            rquote = node->getAttributeValue(attr_rquote);
        // Use a temporary char we're not likely to find in the DOM
        while ( mtext.replace( lquote, cs32(U"\xFFFF") ) ) ;
        lString32 repl_lquote = U"\\" + lquote;
        while ( mtext.replace( cs32(U"\xFFFF"), repl_lquote ) ) ;
        if ( lquote != rquote ) {
            while ( mtext.replace( rquote, cs32(U"\xFFFF") ) ) ;
            lString32 repl_rquote = U"\\" + lquote;
            while ( mtext.replace( cs32(U"\xFFFF"), repl_rquote ) ) ;
        }
    }

    return mtext;
}

// ====================================================================
// MathML operators <mo> properties via dictionary
// ====================================================================

// <mo> operators have specific properties depending on their text content (usually a single char)
#define MATHML_OP_UNDEF    0
#define MATHML_OP_INFIX    1
#define MATHML_OP_PREFIX   2
#define MATHML_OP_POSTFIX  3

#define MATHML_OP_STRETCHY      0x0001 // might stretch to the size of adjacent material
#define MATHML_OP_SYMMETRIC     0x0002 // should be kept symmetric around the math axis when stretchy
#define MATHML_OP_HORIZONTAL    0x0004 // stretches horizontally
#define MATHML_OP_VERTICAL      0x0008 // stretches vertically
#define MATHML_OP_MOVABLELIMITS 0x0010 // op in munder/mover might become msub/msuper when displaystyle=false
#define MATHML_OP_ACCENT        0x0020 // op in munder/mover is an accent and nearer to base
#define MATHML_OP_LARGEOP       0x0040 // should be drawn larger than normal when displaystyle=true"
#define MATHML_OP_INTEGRAL      0x0080 // (not in specs, but in mozilla data) like largeop but a bit larger (*2 vs *sqrt(2))
#define MATHML_OP_MIRRORABLE    0x0100 // (not in specs, but in mozilla data)
#define MATHML_OP_FENCE         0x0200 // no direct effect on the visual rendering
#define MATHML_OP_SEPARATOR     0x0400 // no direct effect on the visual rendering

typedef struct  {
    const lChar32 * op;
    char pos;
    char lspace;
    char rspace;
    lUInt32 flags;
} mathml_operator_dict_entry;

// This dictionary is taken from Firefox data (generated by tools/gen_mathml_operators_h.py):
// https://raw.githubusercontent.com/mozilla/gecko-dev/master/layout/mathml/mathfont.properties
static const mathml_operator_dict_entry mathml_operators[] = {
#include "mathml_operators.h"
{NULL, 0, 0, 0, 0},
};

static bool getOperatorProperties(const lChar32 * op, const mathml_operator_dict_entry * &infix,
                const mathml_operator_dict_entry * &prefix, const mathml_operator_dict_entry * &postfix) {
    int nb = sizeof(mathml_operators) / sizeof((mathml_operators)[0]) - 1; // ignore last NULL
    int left = 0;
    int right = nb;
    // start binary search near the start to skip a few iterations
    // as most common operators are ASCII +=()
    // (takes around 6 iterations for these, 11 otherwise vs.
    // mostly always 10 when starting from the middle)
    int middle = nb/20;
    int iters = 0;
    while ( left < right ) {
        iters++;
        int res = lStr_cmp( mathml_operators[middle].op, op );
        if ( res >= 0 ) {
            right = middle;
        }
        else {
            left = middle + 1;
        }
        middle = (left + right) / 2;
    }
    int idx = left;
    if ( idx < 0 )
        idx = 0;
    else if (idx >= nb)
        idx = nb - 1;
    // printf("nb ops %d , iters %d, idx=%d\n", nb, iters, idx);

    // Multiple entries for a same char are possible, possibly one
    // for each of infix/prefix/postfix
    bool found = false;
    while ( idx < nb && lStr_cmp( mathml_operators[idx].op, op ) == 0 ) {
        found = true;
        if (mathml_operators[idx].pos == MATHML_OP_INFIX)
            infix = &mathml_operators[idx];
        else if (mathml_operators[idx].pos == MATHML_OP_PREFIX)
            prefix = &mathml_operators[idx];
        else if (mathml_operators[idx].pos == MATHML_OP_POSTFIX)
            postfix = &mathml_operators[idx];
        idx++;
    }
    return found;
}

// ====================================================================
// MathML DOM building helper
// ====================================================================

// Called by lvtinydom.cpp ldomDocumentWriters' onTagOpen/Body/Close/onText.
// For some specific MathML elements, we may insert mathBox elements in the DOM
// wrapping one or all children, so we can handle the element like a HTML table.
// We may also add internal attributes to some nodes, so we can catch them again
// in a later step, and/or target them with CSS.
// This process is done at XML load/parsing time, so only once.
//
// After this XML load/parsing phase, and for the first rendering that
// follows, an additional step is done with fixupMathMLMathElement(),
// called at end of initNodeRendMethod().
// On each re-rendering (and before this fixupMathMLMathElement() on the
// first rendering), only setMathMLElementNodeStyle() will set the node
// style from the DOM nodes hierarchy and attributes we have build and
// set at XML load/parsing time.
//
bool MathMLHelper::handleMathMLtag( ldomDocumentWriter * writer, int step, lUInt16 tag_id,
                        const lChar32 * text, int len) {
    ldomElementWriter * _currNode = writer->_currNode;
    // Gather some node and ancestor informations
    ldomNode * curNode = _currNode->_element;
    lUInt16 curNodeId = curNode->getNodeId();
    int curNodeIndex = curNode->getNodeIndex(); // index in parent children collection
    ldomNode * parentNode = NULL;
    lUInt16 parentNodeId = el_NULL;
    if ( _currNode->_parent && _currNode->_parent->_element ) {
        parentNode = _currNode->_parent->_element;
        parentNodeId = parentNode->getNodeId();
    }

    if ( step == MATHML_STEP_NODE_SET ) {
        // This section implements the handling of math-style, math-shift
        // and math-depth, as specified in:
        //   https://mathml-refresh.github.io/mathml-core/#user-agent-stylesheet
        //   https://mathml-refresh.github.io/mathml-core/#css-extensions-for-math-layout
        // As all this depends only on the DOM structure, we don't need to handle
        // them via CSS: we can just set hardcoded attributes in the DOM to express
        // the resulting computed values of these properties.
        //
        // We set an empty MD attribute when the element has displaystyle=true
        // (when false or not set, largeop does not apply, and munder/over may
        // be rendered as msub/msub). This is inherited, and can be cancelled
        // by some elements, or reset by an explicite displaystyle= attribute.
        // On the initial <math> element, the attribute is named "display=block",
        // and defaults to inline.
        // Having MD is equivalent to the recent CSS property 'math-style: normal',
        // not having MD to 'math-style: compact'
        // We set an empty MS attribute when the element has the internal mathML
        // property math-shift:normal, and unset it for math-shift:compact. The
        // only effect of MS is using OpenType Math superscriptShiftUp when normal,
        // and superscriptShiftUpCramped when compact.
        bool is_MD; // math-style
        bool is_MS; // math-shift
        int add_MN = 0; // math-depth: number to add to nearest upper
        if ( curNodeId == el_math ) {
            lString32 at_display = curNode->getAttributeValueLC( attr_display );
            is_MD = at_display == U"block"; // defaults to false if absent
            is_MS = true; // <math> starts with math-shift: normal
        }
        else {
            lString32 at_displaystyle = curNode->getAttributeValueLC( attr_displaystyle );
            if ( at_displaystyle == U"true" )
                is_MD = true;
            else if ( at_displaystyle == U"false" )
                is_MD = false;
            else if ( parentNode->hasAttribute(attr_MD) ) { // Otherwise, just inherit MD
                is_MD = true;
                // Except for some elements, that resets some of their child to displaystyle=false
                if ( parentNodeId == el_mfrac )
                    is_MD = false; // both numerator and denominator
                else if ( curNodeId == el_mtable )
                    is_MD = false;
                else if ( parentNodeId >= el_munder && parentNodeId <= el_mmultiscripts && curNodeIndex > 0 )
                    is_MD = false; // only on scripts element (not on first child, the base)
                else if ( parentNodeId == el_mroot && curNodeIndex > 0 )
                    is_MD = false; // only on the 2nd child
            }
            // Inherit MS / math-shift
            is_MS = parentNode->hasAttribute(attr_MS);
            // Except for some elements, that resets it to math-shift: compact
            if ( curNodeId == el_msqrt || curNodeId == el_mroot )
                is_MS = false; // become compact
            else if ( parentNodeId == el_mfrac && curNodeIndex > 0 )
                is_MS = false; // denominator becomes compact
            else if ( parentNodeId == el_msub && curNodeIndex == 1 )
                is_MS = false; // subscripts become compact
            else if ( parentNodeId == el_msubsup && curNodeIndex == 1 )
                is_MS = false; // subscripts become compact
            else if ( parentNodeId == el_mmultiscripts && curNodeIndex >= 1 )
                is_MS = false; // should be only subscripts, but hard to check with mprescripts: make them all compact
            else if ( ( parentNodeId == el_mover || parentNodeId == el_munderover ) && curNodeIndex == 0 ) {
                // Base becomes compact if "accent=true"
                if ( parentNode->getAttributeValueLC( attr_accent ) == U"true" )
                    is_MS = false;
            }
            // MN: don't inherit, when set, it states the nested level
            if ( parentNodeId >= el_munder && parentNodeId <= el_mmultiscripts && curNodeIndex > 0 ) {
                add_MN = 1; // sub/superscripts get a smaller font-size
                // If accent/accentunder (on the mover/munder, nothing said about what if the <mo> itself
                // has the accent property), it should not be incremented (but the suggested User-Agent
                // stylesheet does it, just using 'font-size: inherit' to cancel the font decrease...)
                if ( (parentNodeId == el_munder || parentNodeId == el_munderover) && curNodeIndex == 1 ) { // underscript
                    if ( parentNode->getAttributeValueLC( attr_accentunder ) == U"true" )
                        add_MN = 0;
                }
                if ( (parentNodeId == el_mover && curNodeIndex == 1 ) || (parentNodeId == el_munderover && curNodeIndex == 2 ) ) { // overscript
                    if ( parentNode->getAttributeValueLC( attr_accent ) == U"true" )
                        add_MN = 0;
                }
            }
            else if ( parentNodeId == el_mfrac && !parentNode->hasAttribute(attr_MD) )
                add_MN = 1; // mfrac num/dem gets smaller only if the mfrac itself is math-style: compact
            else if ( parentNodeId == el_mroot && curNodeIndex > 0 )
                add_MN = 2; // mroot root numbers get a twice smaller font-size
        }
        if ( is_MD ) { // Add this attribute (with an empty value)
            curNode->setAttributeValue(LXML_NS_NONE, attr_MD, U"");
        }
        if ( is_MS ) { // Add this attribute (with an empty value)
            curNode->setAttributeValue(LXML_NS_NONE, attr_MS, U"");
        }
        if ( add_MN ) {
            // https://mathml-refresh.github.io/mathml-core/#the-math-script-level-property
            // These rules, for the limited usage we do (+1/+2, no set), come down to:
            //   level  inc       S      C^E      if no OT: S  C^E         tag
            //     0     +1      SPD     1                  1  0.71        "0+1"
            //     0     +2     SSPD     1                  1  0.71^2      "0+2"
            //     1     +1    SSPD/SPD  1                  1  0.71        "1+1"
            //     1     +2    SSPD/SPD  0.71               1  0.71^2      "1+2"
            //    >=2    +1       1      0.71                              "+1"
            //    >=2    +2       1      0.71^2                            "+2"
            // and the new font size should be: current font size * S * C^E
            // So, we store in MN a tag for the rule to apply.
            // Get current level
            int level = 0;
            ldomNode * tmp = parentNode;
            while ( tmp && tmp->getNodeId() != el_math ) {
                if ( tmp->hasAttribute(attr_MN) ) {
                    lString32 tag = tmp->getAttributeValue(attr_MN);
                    if ( tag == U"0+1" ) level = 1;
                    else if ( tag == U"0+2" ) level = 2;
                    else if ( tag == U"1+1" ) level = 2;
                    else if ( tag == U"1+2" ) level = 3;
                    else if ( tag == U"+1" ) level = 3;
                    else if ( tag == U"+2" ) level = 4;
                    break;
                }
                tmp = tmp->getParentNode();
            }
            lString32 tag;
            if ( level == 0 )
                tag = add_MN == 1 ? U"0+1" : U"0+2";
            else if ( level == 1 )
                tag = add_MN == 1 ? U"1+1" : U"0+2";
            else
                tag = add_MN == 1 ? U"+1" : U"+2";
            curNode->setAttributeValue(LXML_NS_NONE, attr_MN, tag.c_str());
        }

        if ( curNodeId == el_mtd ) {
            // Just to avoid having to check for this columnspan attribute, clone
            // it as the expected colspan attribute
            if ( curNode->hasAttribute(attr_columnspan) ) {
                curNode->setAttributeValue(LXML_NS_NONE, attr_colspan, curNode->getAttributeValue(attr_columnspan).c_str());
            }
        }
        return false;
    }

    // Only abort at this point: we want the above to still be done
    if ( skip_specific_handling ) { // (property of the MathMLHelper object)
        skip_specific_handling = false;
        return false;
    }

    // ------------------------------------------
    // <mfrac> numerator denominator </mfrac>:
    //
    // mathml_css_h.css sets it to be display:inline-table.
    // Wrap each of numerator and denominator in a <mathBox> (each to be
    // set display:table-row by mathml_css_h.css).
    if ( step == MATHML_STEP_BEFORE_NEW_CHILD && curNodeId == el_mfrac && tag_id != el_mathBox ) {
        // Opening child of <mfrac>: wrap it in a <mathBox>
        writer->OnTagOpen( NULL, U"mathBox" );
        writer->OnTagBody();
        return true;
    }
    if ( step == MATHML_STEP_NODE_CLOSED && curNodeId == el_mathBox && parentNodeId == el_mfrac ) {
        // Close a <mathBox> child of <mfrac> when closing its child (a mathBox is wrapping *each* child)
        writer->OnTagClose( NULL, U"mathBox" );
        return true;
    }

    // ------------------------------------------
    // <msub> base sub </msub>
    // <msup> base sup </msup>
    // <msubsup> base sub sup </msubsup>
    // <mmultiscripts> base sub sup ... mprescripts sub sup ... </mmultiscripts>
    // <munder> base under </munder>
    // <mover> base over </mover>
    // <munderover> base under over </munderover>
    //
    // mathml_css_h.css sets them to be display:inline-table.
    // Wrap each child in a <mathBox> (each to be set display:table-cell by mathml_css_h.css,
    // a <tabularBox> will be normally added to act as the missing display:table-row)
    // This will set a single row of many cells. The table rendering code, with the tweaks
    // from mathml_table_ext.h, will add additional rows and move the cells in them as
    // expected for a correct rendering as a table of these elements.
    if ( step == MATHML_STEP_BEFORE_NEW_CHILD && curNodeId >= el_munder && curNodeId <= el_mmultiscripts
                                              && tag_id != el_mathBox && tag_id != el_mprescripts ) {
        // Opening child of <msub...>: wrap it in a <mathBox>
        writer->OnTagOpen( NULL, U"mathBox" );
        writer->OnTagBody();
        return true;
    }
    if ( step == MATHML_STEP_NODE_CLOSED && curNodeId == el_mathBox && parentNodeId >= el_munder && parentNodeId <= el_mmultiscripts ) {
        // Close a <mathBox> child of <msub...> when closing its child (a mathBox is wrapping *each* child)
        writer->OnTagClose( NULL, U"mathBox" );
        return true;
    }

    // ------------------------------------------
    // <mroot> base index </mroot>
    //
    // mathml_css_h.css sets it to be display:inline-table.
    // Wrap each child in a <mathBox> (each to be set display:table-cell by mathml_css_h.css,
    // a <tabularBox> will be normally added to act as the missing display:table-row)
    if ( step == MATHML_STEP_BEFORE_NEW_CHILD && curNodeId == el_mroot && tag_id != el_mathBox ) {
        writer->OnTagOpen( NULL, U"mathBox" );
        writer->OnTagBody();
        return true;
    }
    if ( step == MATHML_STEP_NODE_CLOSED && curNodeId == el_mathBox && parentNodeId == el_mroot ) {
        writer->OnTagClose( NULL, U"mathBox" );
        return true;
    }

    // ------------------------------------------
    // <msqrt> anything </msqrt>
    //
    // mathml_css_h.css sets it to be display:inline-table.
    // Have it look exactly like a <mroot> by adding an empty 2nd child,
    // so the same code can handle both similarly
    if ( step == MATHML_STEP_NODE_ENTERED && curNodeId == el_msqrt ) {
        // Create a mathBox that will wrap all other children
        writer->OnTagOpen( NULL, U"mathBox" );
        writer->OnTagBody();
        return true;
    }
    if ( step == MATHML_STEP_NODE_CLOSING && tag_id == el_msqrt && curNodeId == el_mathBox && parentNodeId == el_msqrt ) {
        writer->OnTagClose( NULL, U"mathBox" );
        // Create an empty 2nd mathBox to hold the root symbol (drawn as a background image)
        // (the cells will be re-ordered, so the 2nd one is drawn on the left)
        writer->OnTagOpen( NULL, U"mathBox" );
        writer->OnTagBody();
        writer->OnTagClose( NULL, U"mathBox" );
        return true;
    }

    // ------------------------------------------
    // <mfenced open="A" close="B" separators="S1 S2..." > elem1 elem2 ... </mfenced>
    //
    // https://www.w3.org/TR/MathML/chapter3.html#presm.mfenced
    // This element is deprecated, but still often met.
    // Contrary to the other elements we handle above, for simplicity, we don't wrap or
    // add <mathBox> elements (so we don't have to check for this too specific case), but
    // we add a <mrow> instead
    // from the above, we will generate literally:
    //  <mfenced> <mo>A</mo> <mrow> elem1 <mo>S1</mo> elem2 <mo>S2</mo> ... </mrow> <mo>B</mo> </mfenced>
    if ( step == MATHML_STEP_NODE_ENTERED && curNodeId == el_mfenced ) {
        lString32 opening = U"(";
        if ( curNode->hasAttribute(attr_open) ) {
            opening = curNode->getAttributeValue( attr_open );
        }
        writer->OnTagOpen( NULL, U"mo" );
        writer->OnTagBody();
        writer->OnText(opening.c_str(), opening.length(), 0);
        writer->OnTagClose( NULL, U"mo" );
        writer->OnTagOpen( NULL, U"mrow" );
        writer->OnTagBody();
        // Additional completion done below when handling this <mrow> element
        return true;
    }
    if ( step == MATHML_STEP_NODE_CLOSING && tag_id == el_mfenced && curNodeId == el_mrow && parentNodeId == el_mfenced ) {
        writer->OnTagClose( NULL, U"mrow" );
        lString32 closing = U")";
        if ( parentNode->hasAttribute(attr_close) ) {
            closing = parentNode->getAttributeValue( attr_close );
        }
        writer->OnTagOpen( NULL, U"mo" );
        writer->OnTagBody();
        writer->OnText(closing.c_str(), closing.length(), 0);
        writer->OnTagClose( NULL, U"mo" );
        return true;
    }
    // ------------------------------------------
    // <mrow> anything </mrow>, children of <mfenced> added just above
    if ( step == MATHML_STEP_BEFORE_NEW_CHILD && parentNodeId == el_mfenced && curNodeId == el_mrow ) {
        int child_count = curNode->getChildCount();
        if ( child_count == 0 ) {
            // No separator to add before first child
            return false;
        }
        // New child of <mfenced><mrow>: add the separator as <mo>
        lString32 separator;
        if ( parentNode->hasAttribute(attr_separators) ) {
            lString32 separators = parentNode->getAttributeValue( attr_separators );
            // Trim all optional whitespace, as per specs
            while ( separators.replace( cs32(" "), lString32::empty_str ) ) ;
            if ( separators.empty() ) {
                // if separators="" or " ", don't add any <mo>, as per specs
                return false;
            }
            // At this point, we have an even number of children (for each previously
            // added child of mrow, we added the separator as <mo>)
            int sep_idx = (child_count - 1) / 2;
            if ( sep_idx >= separators.length() ) {
                sep_idx = separators.length() - 1; // re-use the last one
            }
            separator = lString32(separators, sep_idx, 1);
        }
        else {
            separator = U","; // default separator
        }
        // But as this separator will itself be a new child, and we don't
        // want to trigger adding a new one, just have it ignored above.
        skip_specific_handling = true;
        writer->OnTagOpen( NULL, U"mo" );
        // Note: Firefox does not use the default lspace/rspace of 5 for unknown operators
        // in mfenced, but 0 ! The specs don't mention that. Let's do as the specs say.
        // (Otherwise, we could add attributes here to avoid adding ML/MR.)
        writer->OnTagBody();
        writer->OnText(separator.c_str(), separator.length(), 0);
        writer->OnTagClose( NULL, U"mo" );
        skip_specific_handling = false;
        return true;
    }
    if ( step == MATHML_STEP_NODE_ENTERED && curNodeId == el_mglyph ) {
        // Not adding specific support for mglyph everywhere we handle <img>:
        // we just add a <img> child to it.
        lString32 src = curNode->getAttributeValue( attr_src );
        writer->OnTagOpen( NULL, U"img" );
        writer->OnAttribute(U"", U"src", src.c_str() );
        writer->OnTagBody();
        writer->OnTagClose( NULL, U"img" );
    }

    // No tag opened nor closed, no element added: return false so callers know curNodeId has not changed
    return false;
}

// Return a SVG image suitable to be used as a radical symbol for <msqrt>
// and <mroot>. Will return something like this:
//   <svg version='1.1' width='17' height='9' viewBox='0 0 17 9'>
//     <path fill='none' stroke-linecap='square' stroke-linejoin='miter'
//       stroke-width='2.25' stroke='#008000' d='M 19 11 L 8 0 L 5 3'/>
//   </svg>
lString8 getRadicalSymbolSVGImageString( int font_size, int border_thickness,
                                int stroke_color, int & width, int & height ) {
    width = font_size * 1/2;   // 0.5em
    height = font_size * 3/10; // 0.3em
    // Make the int color a proper 6 hex digits MathML/HTML color value
    lString8 hexcolor;
    hexcolor << fmt::hex(stroke_color);
    int hexcolor_len = hexcolor.length();
    if ( hexcolor_len < 6 ) // pad with 0 to get 6 hexdigits
        hexcolor.insert(0, 6-hexcolor_len, '0');
    int svg_w = width;
    int svg_h = height;
    // Our SVG rasterizer adds 1 px on each side of the SVG viewBox to
    // avoid blurry edges on standalone images.
    // As we're putting it near a vertical border, make it smaller if
    // we have enough room to draw something.
    if ( width >= 10 ) {
        svg_w -= 2;
        svg_h -= 2;
    }
    // Keep stroke width as border thickness, we'll add a .25 below to make it a bit bolder
    int stroke_width = border_thickness;
    lString8 img;
    img << "data:image/svg;-cr-plain,";
    img << "<svg version='1.1'";
    img << " width='" << fmt::decimal(svg_w) << "'";
    img << " height='" << fmt::decimal(svg_h) << "'";
    img << " viewBox='0 0 " << fmt::decimal(svg_w) << " " << fmt::decimal(svg_h) << "'>";
    img << "<path fill='none' stroke-linecap='square' stroke-linejoin='miter'";
    img << " stroke-width='" << fmt::decimal(stroke_width) << ".25'";
    img << " stroke='#" << hexcolor << "'";
    // Draw 2 line segments
    int x,y;
    // Start from bottom right edge, a bit off to have sharp edges
    x = svg_w + 2; y = svg_h + 2;
    img << " d='M " << fmt::decimal(x) << " " << fmt::decimal(y);
    // Move north west
    x -= svg_h + 2; y -= svg_h + 2;
    img << " L " << fmt::decimal(x) << " " << fmt::decimal(y);
    // Move south west
    x -= svg_h/3 ; y += svg_h/3;
    img << " L " << fmt::decimal(x) << " " << fmt::decimal(y);
    img << "'/></svg>";
    // printf("%s\n", img.c_str());
    return img;
}

bool getLengthFromMathMLAttributeValue( lString32 value, css_length_t & length,
                                            bool accept_percent,
                                            bool accept_negative,
                                            bool accept_namedspaces,
                                            bool accept_op_spacing) {
    if ( value.empty() )
        return false;
    if ( accept_op_spacing && value.length() == 1 ) {
        lChar32 v = value[0];
        if ( v >= U'1' && v <= U'5' ) {
            // 1: veryverythinmathspace 1/18 em
            // 2: verythinmathspace 2/18 em
            // 3: thinmathspace 3/18 em
            // 4: mediummathspace 4/18 em
            // 5: thickmathspace 5/18 em
            int ratio = v - U'0';
            length.type = css_val_em;
            length.value = 256 * ratio / 18;
            return true;
        }
    }
    if ( accept_namedspaces ) {
        int ratio = 0;
        if ( value[0] == U'v' ) {
            if ( value == U"veryverythinmathspace" ) ratio = 1;
            if ( value == U"verythinmathspace" ) ratio = 2;
            if ( value == U"verythickmathspace" ) ratio = 6;
            if ( value == U"veryverythickmathspace" ) ratio = 7;
        }
        else if ( value[0] == U't' ) {
            if ( value == U"thickmathspace" ) ratio = 5;
            if ( value == U"thinmathspace" ) ratio = 3;
        }
        else if ( value[0] == U'm' ) {
            if ( value == U"mediummathspace" ) ratio = 4;
        }
        else if ( accept_negative && value[0] == U'n' ) {
            if ( value == U"negativeveryverythinmathspace" ) ratio = -1;
            if ( value == U"negativeverythinmathspace" ) ratio = -2;
            if ( value == U"negativethinmathspace" ) ratio = -3;
            if ( value == U"negativemediummathspace" ) ratio = -4;
            if ( value == U"negativethickmathspace" ) ratio = -5;
            if ( value == U"negativeverythickmathspace" ) ratio = -6;
            if ( value == U"negativeveryverythickmathspace" ) ratio = -7;
        }
        if ( ratio ) { // 1/18th of em (thinmathspace = 3/18em)
            length.type = css_val_em;
            length.value = 256 * ratio / 18;
            return true;
        }
    }
    // Parse classic CSS length values
    lString8 value8 = UnicodeToUtf8(value);
    const char * cvalue8 = value8.c_str();
    if ( parse_number_value(cvalue8, length, accept_percent, accept_negative) ) {
        if ( length.type == css_val_unspecified && length.value != 0 )
            return false; // Don't allow non-zero with no unit
        return true;
    }
    return false;
}


// ====================================================================
// MathML node styling
// ====================================================================

// This is called at loading time (after above MathMLHelper::handleMathMLtag())
// and at re-rendering time, by lvrend.cpp's setNodeStyle().
// It applies the internal CSS stylesheet, and complement the styling with
// computed values from normal MathML MathML attributes but also from the
// one we have set when building the DOM in MathMLHelper::handleMathMLtag()).
void setMathMLElementNodeStyle( ldomNode * node, css_style_rec_t * style ) {
    // First, apply the MathML stylesheet, which will do a good part of the styling.
    if ( ! _MathML_stylesheet_loaded ) {
        loadMathMLStylesheet( node );
        _MathML_stylesheet_loaded = true;
    }
    _MathML_stylesheet.apply( node, style );
        // This could be done after what's below if needed, depending on which
        // from CSS or code may want to override stuff set by the other

    // Do more style computations for stuff we can't handle via CSS
    int curNodeIndex = node->getNodeIndex(); // index in parent children collection
    ldomNode * curNode = node;
    lUInt16    curNodeId = node->getNodeId();
    ldomNode * parentNode = node->getParentNode();
    lUInt16    parentNodeId = parentNode ? parentNode->getNodeId() : el_NULL;
    ldomNode * gParentNode = parentNode ? parentNode->getParentNode() : NULL;
    lUInt16    gParentNodeId = gParentNode ? gParentNode->getNodeId() : el_NULL;

    // Our CSS stylesheet does not enforce a font-family, as we don't want to
    // override a font set by the publisher or the user.
    // But if there is none set, or the default inherited font does not have
    // OT Math support, we enforce a few known OT math fonts.
    if ( curNodeId == el_math ) {
        // At this point, normal User-Agent and publisher stylesheets have been applied,
        // as well as our _MathML_stylesheet, but font inheritance has not yet been done
        // (it's handled by lvrend.cpp setNodeStyle() after we return).
        if ( style->font_family == css_ff_inherit ) {
            // No font set by any stylesheet
            // See if the font we would inherit has OT Math support
            if ( !parentNode->getFont()->hasOTMathSupport() ) {
                // It has not: use our default Math font list
                style->font_family = css_ff_sans_serif;
                style->font_name = MATHML_DEFAULT_FONTS;
            }
        }
    }

    // Scale down font-size if MN= attribute present.
    if ( curNode->hasAttribute(attr_MN) ) {
        bool scale_font_size = true;
        // Except in a few cases: we already handle the case for accent/accentunder=true
        // set on the munder/mover element by not setting MN; here, we handle the case
        // where the accentness comes from the <mo>
        // (check gParent, as there's been a tabularBox added above the mathBox that got the MN=)
        if ( (gParentNodeId == el_munder || gParentNodeId == el_munderover) && curNodeIndex == 1 ) {
            if ( gParentNode->hasAttribute( attr_Maccentunder ) )
                scale_font_size = false;
        }
        if ( (gParentNodeId == el_mover && curNodeIndex == 1 ) || (gParentNodeId == el_munderover && curNodeIndex == 2 ) ) {
            if ( gParentNode->hasAttribute( attr_Maccent ) )
                scale_font_size = false;
        }
        if ( scale_font_size ) {
            // This tag was set at DOM building time, according to:
            // https://mathml-refresh.github.io/mathml-core/#the-math-script-level-property
            // These rules, for the limited usage we do (+1/+2, no set), come down to:
            //   level  inc       S      C^E      if no OT: S  C^E         tag
            //     0     +1      SPD     1                  1  0.71        "0+1"
            //     0     +2     SSPD     1                  1  0.71^2      "0+2"
            //     1     +1    SSPD/SPD  1                  1  0.71        "1+1"
            //     1     +2    SSPD/SPD  0.71               1  0.71^2      "1+2"
            //    >=2    +1       1      0.71                              "+1"
            //    >=2    +2       1      0.71^2 (=0.5)                     "+2"
            // and the new font size should be: current font size * S * C^E
            lString32 tag = curNode->getAttributeValue(attr_MN);
            LVFontRef font = parentNode->getFont();
            // int orig_size = font->getSize();
            // Work on size<<6, so we can round better
            int size = font->getSize() << 6;
            if ( tag == U"+1" ) {
                size = size * 71 / 100;
            }
            else if ( tag == U"+2" ) {
                size = size / 2;
            }
            else {
                int spd = font->getExtraMetric(font_metric_math_script_percent_scale_down, false); // 71 if n/a
                int sspd = font->getExtraMetric(font_metric_math_script_script_percent_scale_down, false); // 50 if n/a
                // printf("spd: %d sspd: %d\n", spd, sspd);
                if ( tag == U"0+1" )
                    size = size * spd / 100;
                else if ( tag == U"0+2" )
                    size = size * sspd / 100;
                else if ( tag == U"1+1" )
                    size = size * sspd / spd;
                else if ( tag == U"1+2" )
                    size = size * sspd * 71 / spd / 100;
            }
            style->font_size.type = css_val_screen_px;
            style->font_size.value = (size + 32) >> 6; // round()
            // printf("attr_MN=%s\t%d => %d\n", LCSTR(tag), orig_size, style->font_size.value);
        }
    }

    // if ( curNode->hasAttribute(attr_Mlargeop) )
    // Currently, the font size for largeop operator is set to 1.41em (or 2em for integral) via mathml_css_h.css.
    // We could try to get something out of font_metric_math_display_operator_min_height (which is the height,
    // so we should get the height of a classic largeop glyph to get a ratio to apply to the font size).
    // A quick test shows differences with font, and often too large glyphs....

    // Attributes common to token elements
    // (mathvariant is handled at text node creation time by getMathMLAdjustedText())
    if ( curNode->hasAttribute(attr_mathsize) ) {
            // && curNodeId >= EL_MATHML_TOKEN_START && curNodeId <= EL_MATHML_TOKEN_END
            // MathML specs limit it to token elements, but mathml-core does not
            // (Acid3 tests set it on <math> and others)
        // "mathsize effectively overrides the effect of scriptlevel", so
        // we handle it after attr_MN
        css_length_t font_size;
        lString32 at_mathsize = curNode->getAttributeValueLC(attr_mathsize);
        if ( getLengthFromMathMLAttributeValue(at_mathsize, font_size) ) {
            style->font_size = font_size;
        }
        else {
            if ( at_mathsize == U"small" ) {
                style->font_size.type = css_val_em;
                style->font_size.value = 75 * 256 / 100; // 0.75em (as WebKit)
            }
            else if ( at_mathsize == U"big" ) {
                style->font_size.type = css_val_em;
                style->font_size.value = 150 * 256 / 100;  // 1.5em (as WebKit)
            }
            // No need to parse "normal"
        }
    }
    // todo: cap font size if too small ? crengine limits it to 8 screen px,
    // but this might be unreadeable on large DPI screens

    // Attributes common to presentation elements
    if ( curNode->hasAttribute(attr_mathcolor) ) {
        css_length_t color;
        lString8 value8 = UnicodeToUtf8(curNode->getAttributeValue(attr_mathcolor));
        const char * cvalue8 = value8.c_str();
        if ( parse_color_value( cvalue8, color ) ) {
            style->color = color;
        }
    }
    if ( curNode->hasAttribute(attr_mathbackground) ) {
        css_length_t bgcolor;
        lString8 value8 = UnicodeToUtf8(curNode->getAttributeValue(attr_mathbackground));
        const char * cvalue8 = value8.c_str();
        if ( parse_color_value( cvalue8, bgcolor ) ) {
            style->background_color = bgcolor;
        }
    }

    // Spacing around operators, possibly set on upper node that embellish the operator
    // Using margin allows it to be set on any inline-block/table element
    if ( curNode->hasAttribute(attr_ML) ) {
        css_length_t margin;
        lString32 at_ML = curNode->getAttributeValueLC(attr_ML);
        // Ignore percent and negative, accept named spaces and our 1,2,3,... operator
        // lspace values we got from operator dict (converted to a ratio of 1em)
        if ( getLengthFromMathMLAttributeValue(at_ML, margin, false, false, true, true) ) {
            style->margin[0] = margin; // margin-left
        }
    }
    if ( curNode->hasAttribute(attr_MR) ) {
        css_length_t margin;
        lString32 at_MR = curNode->getAttributeValueLC(attr_MR);
        if ( getLengthFromMathMLAttributeValue(at_MR, margin, false, false, true, true) ) {
            style->margin[1] = margin; // margin-right
        }
    }

    // mfrac: set up fraction line thickness
    if ( parentNodeId == el_mfrac && curNodeId == el_mathBox && curNodeIndex == 1) {
        // 2nd <MathBox> child of <mfrac>: denominator
        // This 2nd table_row will carry the fraction line thickness as its top border
        css_length_t bw;
        lString32 at_linethickness = parentNode->getAttributeValueLC(attr_linethickness);
        if ( !getLengthFromMathMLAttributeValue(at_linethickness, bw) || bw.type == css_val_percent ) {
            // Default to "medium", = OpenType Math FractionRuleThickness
            // We don't yet have the font defined for this node, but it should come
            // from the parentNode, which has had its font defined!
            bool allow_zero = false;
            // This is the default value for "medium"
            int thickness_px  = parentNode->getFont()->getExtraMetric(font_metric_math_fraction_rule_thickness);
            if ( bw.type == css_val_percent ) { // percentage of the default value
                thickness_px = lengthToPx( parentNode, bw, thickness_px );
                if ( bw.value == 0 )
                    allow_zero = true;
            }
            else if ( at_linethickness == U"thin" ) {
                thickness_px = thickness_px / 2;
            }
            else if ( at_linethickness == U"thick" ) {
                thickness_px = thickness_px * 2;
            }
            if ( thickness_px == 0 && !allow_zero)
                thickness_px = 1;
            bw.type = css_val_screen_px;
            bw.value = thickness_px;
        }
        style->border_width[0] = bw;
        if ( bw.value == 0 )
            style->border_style_top = css_border_none;
        else
            style->border_style_top = css_border_solid;
        // This border_style_top will be used to know if zero or nonzero line thickness
    }

    // msqrt/mroot: 1 row, 2 cells
    if ( curNodeId == el_msqrt || curNodeId == el_mroot ) {
        // margin top, space above rule
        style->margin[2].type = css_val_screen_px;
        style->margin[2].value = parentNode->getFont()->getExtraMetric(font_metric_math_radical_extra_ascender);
        // (Metric from the parent font, as current node as not yet its font defined)
    }
    else if ( curNodeId == el_mathBox && ( parentNodeId == el_msqrt || parentNodeId == el_mroot
             || (parentNodeId == el_tabularBox && (gParentNodeId == el_msqrt || gParentNodeId == el_mroot)) ) ) {
                        // (The mathBoxes we target are not yet wrapped in a tabularBox at load time,
                        // but will be at re-rendering time: so we check both cases)
        ldomNode * pnode = parentNodeId == el_tabularBox ? gParentNode : parentNode;
        lUInt16 pnodeId = parentNodeId == el_tabularBox ? gParentNodeId : parentNodeId;
        LVFontRef font = pnode->getFont();
        int thickness_px  = font->getExtraMetric(font_metric_math_radical_rule_thickness);
        if ( thickness_px == 0 ) // might happen at small font sizes
            thickness_px = 1;
        if ( curNodeIndex == 0) {
            // Right cell with the content and top/left borders to make the root frame
            style->border_width[0].type = css_val_screen_px; // border-top
            style->border_width[0].value = thickness_px;
            style->border_style_top = css_border_solid;
            style->border_width[3].type = css_val_screen_px; // border-left
            style->border_width[3].value = thickness_px;
            style->border_style_left = css_border_solid;
            // Gap between rule and content, as padding-top
            int gap;
            if ( pnode->hasAttribute(attr_MD) ) // displaystyle=true / math-style: normal
                gap = font->getExtraMetric(font_metric_math_radical_display_style_vertical_gap);
            else
                gap = font->getExtraMetric(font_metric_math_radical_vertical_gap);
            style->padding[2].type = css_val_screen_px; // padding-top
            style->padding[2].value = gap;
            // Use it as padding left too
            style->padding[0].type = css_val_screen_px; // padding-left
            style->padding[0].value = gap;
            // And as padding right, so there's no confusion with the follow up content.
            style->padding[1].type = css_val_screen_px; // padding-right
            style->padding[1].value = gap;
            // To see if we need any margin-bottom below the left border bottom
        }
        else {
            // Left cell with the root symbol and radical degree/index text of <mroot>
            int symbol_w, symbol_h;
            int fgcolor = parentNode->getStyle()->color.value;
            style->background_image = getRadicalSymbolSVGImageString( font->getSize(), thickness_px, fgcolor, symbol_w, symbol_h);
            style->background_position = css_background_right_bottom;
            style->background_repeat = css_background_no_repeat;
            style->min_width.type = css_val_screen_px;
            style->min_width.value = symbol_w;
            // min-witdh is used to estimate widths of table cells, but the table rendering
            // code won't use it when actually laying out the table. We need to ensure
            // some width
            if ( pnodeId == el_msqrt ) {
                // Nothing else in this cell: enforce the same width as the root symbol
                style->width.type = css_val_screen_px;
                style->width.value = symbol_w;
            }
            else {
                int gap; // will be used as padding below
                if ( pnode->hasAttribute(attr_MD) ) // displaystyle=true / math-style: normal
                    gap = font->getExtraMetric(font_metric_math_radical_display_style_vertical_gap);
                else
                    gap = font->getExtraMetric(font_metric_math_radical_vertical_gap);
                // el_mroot has some content, that should ensure the width
                // We ensure a bit of padding and positionning from font metrics
                style->padding[0].type = css_val_screen_px; // padding-left
                style->padding[0].value = font->getExtraMetric(font_metric_math_radical_kern_before_degree);
                // For right padding, we can't really use font_metric_math_radical_kern_after_degree, which
                // is negative and depends on the content, and could, by specs, have this overlap with the
                // right cell... Use vertical gap for consistent spacing, even if totally unrelated.
                style->padding[1].type = css_val_screen_px; // padding-right
                style->padding[1].value = gap;
                // Enforce some bottom padding to not overlap with the symbol
                style->padding[3].type = css_val_screen_px; // padding-bottom
                style->padding[3].value = symbol_h;
                // We can't use font_metric_math_radical_degree_bottom_raise_percent which depends on
                // the final content height (we could use it with added processing in the table code,
                // but we'd rather spare us that). So, align it on the top, and ensure the same padding
                style->vertical_align.type = css_val_unspecified;
                style->vertical_align.value = css_va_top;
                // And align it to the right, near the vertical left border of next cell
                style->text_align = css_ta_end;
            }
        }
    }

    // mtable: too many attributes with complex specifications we can't handle easily
    // We handle the easy one via CSS or here
    if ( curNodeId == el_mtable ) {
        lString32 at_width = curNode->getAttributeValueLC(attr_width);
        css_length_t w;
        if ( getLengthFromMathMLAttributeValue(at_width, w, false) ) {
            style->width = w;
        }
    }

    // mspace: if non-zero width, make it inline-block and put width as margin-left
    // (We could really not care about mspace depth/height, but it is often used
    // as a reference visible elements with background color in some test suites.)
    if ( curNodeId == el_mspace ) {
        // If linebreak="indentingnewline" or linebreak="indentingnewline", we handle them
        // specifically via mathml_css_h.css, and we shouldn't have it inline-block.
        bool check_size = true;
        bool allow_no_size = false;
        if ( curNode->hasAttribute(attr_linebreak) ) {
            lString32 at_linebreak = curNode->getAttributeValueLC(attr_linebreak);
            if ( at_linebreak == U"newline" || at_linebreak == U"indentingnewline") {
                // This is handled via CSS with :before/:after: don't ensure any width/height
                check_size = false;
            }
            if ( at_linebreak == U"goodbreak" )
                allow_no_size = true;
                // We'll make it inline-block even if no size specified,
                // as an inline-block allows a break
        }
        if ( check_size ) {
            css_length_t w;
            lString32 at_width = curNode->getAttributeValueLC(attr_width);
            bool has_width = getLengthFromMathMLAttributeValue(at_width, w, false) // no % (default value = 0)
                                        && w.type != css_val_unspecified && w.value != 0;
            css_length_t h;
            lString32 at_height = curNode->getAttributeValueLC(attr_height);
            bool has_height = getLengthFromMathMLAttributeValue(at_height, h, false) // no % (default value = 0)
                                        && h.type != css_val_unspecified && h.value != 0;
            css_length_t d;
            lString32 at_depth = curNode->getAttributeValueLC(attr_depth);
            bool has_depth = getLengthFromMathMLAttributeValue(at_depth, d, false) // no % (default value = 0)
                                        && d.type != css_val_unspecified && d.value != 0;

            if ( has_width || has_height || has_depth || allow_no_size ) {
                style->display = css_d_inline_block;
                if ( has_width ) {
                    // style->padding[0] = w;
                    style->width = w;
                }
                if ( has_height || has_depth ) {
                    // These MathML height and depth make the full CSS height.
                    // depth allows setting the baseline, that we can't handle here;
                    // we'll do it in MathML_updateBaseline().
                    // To make the CSS height, we need to resolve these lengths
                    // to screen_px, so we can add them.
                    // This node font size is not yet defined, so compute it.
                    int base_em = parentNode->getFont()->getSize();
                    if ( style->font_size.type != css_val_inherited ) {
                        base_em = lengthToPx(curNode, style->font_size, base_em, base_em);
                    }
                    int h_px = has_height ? lengthToPx(curNode, h, 0, base_em) : 0;
                    int d_px = has_depth ? lengthToPx(curNode, d, 0, base_em) : 0;
                    h.type = css_val_screen_px;
                    h.value = h_px + d_px;
                    style->height = h;
                }
            }
        }
    }

    // mpadded: translate some attributes to CSS properties
    if ( curNodeId == el_mpadded ) {
        // We don't handle <mpadded> right: it's supposed to be a container
        // that can get a fixed width, or get it from its containee. But
        // then, this containee is kinda position:absolue vs the container,
        // so possibly outside. It would need a few hacks in many place
        // to get it right.
        // For now, we only handle positive "lspace" as padding-left,
        // and "voffset" as vertical-align (which is not the right way
        // to handle any of them, but it may be the less risky, and can
        // help supporting a few use cases for <mpadded>).
        // Hopefully, it's not used much anywhere (except in test
        // suites like Acid3...)
        // todo: do better
        //
        // We ignore % as we don't know the inner content size. Other
        // "pseudo units" won't be parsed, and can be ignored for the
        // same reason.
        // A bit too risky to enforce width, as we could wrap the inner
        // content, which <mpadded> never does.
        /*
        lString32 at_width = curNode->getAttributeValueLC(attr_width);
        css_length_t w;
        if ( getLengthFromMathMLAttributeValue(at_width, w, false) ) {
            style->width = w;
        }
        */
        lString32 at_lspace = curNode->getAttributeValueLC(attr_lspace);
        css_length_t l;
        if ( getLengthFromMathMLAttributeValue(at_lspace, l, false) ) {
            style->padding[0] = l;
        }
        lString32 at_voffset = curNode->getAttributeValueLC(attr_voffset);
        css_length_t v;
        if ( getLengthFromMathMLAttributeValue(at_voffset, v, false, true) ) { // negative allowed
            style->vertical_align = v;
        }
    }

    // mglyph: we added a <img> child, update its style form the mglyph attributes
    if ( curNodeId == el_img && parentNodeId == el_mglyph ) {
        lString32 at_width = parentNode->getAttributeValueLC(attr_width);
        css_length_t w;
        if ( getLengthFromMathMLAttributeValue(at_width, w) ) {
            style->width = w;
        }
        lString32 at_height = parentNode->getAttributeValueLC(attr_height);
        css_length_t h;
        if ( getLengthFromMathMLAttributeValue(at_height, h) ) {
            style->height = h;
        }
        lString32 at_valign = parentNode->getAttributeValueLC(attr_valign);
        css_length_t va;
        if ( getLengthFromMathMLAttributeValue(at_valign, va, true, true) ) {
            style->vertical_align = va;
        }
    }
}


// ====================================================================
// MathML elements (mostly only <mo>) attributes tweaking
// ====================================================================

// Forward declarations (defined below)
static void fixupMathML( ldomNode * node, bool is_in_script );
static void fixupMathMLRecursive( ldomNode * node, bool is_in_script );

// Called by lvtinydom.cpp at end of initNodeRendMethod() on a <math>
// element, but should do its work only once after the initial XML
// load/parsing phase, and not on re-renderings.
//
// Once the MathML DOM subtree is set-up (with <mathBox> elements added
// to build table-like trees we can render), we can do some more
// specific MathML tweaks according to the specs.
// Currently, all tweaks are related to <mo> and the operator properties
// (spacing, embelishment, strechy...).
// As we recurse depth-first, this can't handle inherited properties
// (which should better be handled in MathMLHelper::handleMathMLtag(),
// like displaystyle= is).
// Note that when, in fixupMathML() for an inner node, we set some
// properties on some embellished parent, we may updateStyleDataRecursive()
// from it to re-apply CSS styles (which should only change non-critial
// CSS properties).
void fixupMathMLMathElement( ldomNode * node ) {
    // We don't want to go twice thru fixupMathMLRecursive, so set the
    // attribute <math MT="ok"> when we're about to do it the first time
    if ( !node->hasAttribute( attr_MT ) ) {
        node->setAttributeValue(LXML_NS_NONE, attr_MT, U"ok");
        fixupMathMLRecursive( node, false );
    }
}

static void fixupMathMLRecursive( ldomNode * node, bool is_in_script ) {
    // We might want to adjust a few things if the element appears
    // within subscripts or superscripts: check and pass it here
    // (rather than having to look up for each node)
    if ( !is_in_script ) {
        // All <munder>...<msuper> have a <tabularBox> child that contains
        // many <mathBox> wrapping each original children. We're a subscript
        // or superscript when we're in such a <mathBox> except the first
        // (which is the base)
        if ( node->getNodeId() == el_mathBox ) {
            lUInt16 unboxedParentId = node->getUnboxedParent()->getNodeId();
            if ( unboxedParentId >= el_munder && unboxedParentId <= el_mmultiscripts ) {
                if ( node->getNodeIndex() > 0 ) { // index in parent children collection
                    is_in_script = true; // we're not the first <mathBox>
                }
            }
        }
    }
    // Recursive depth-first
    for (int i=0; i<node->getChildCount(); i++) {
        ldomNode * child = node->getChildNode(i);
        if ( child && child->isElement() ) {
            fixupMathMLRecursive( child, is_in_script );
        }
    }
    fixupMathML( node, is_in_script );
    return;
        // If we would be inseting nodes in the upper tree, we might
        // want to use a non-recursive subtree walker
}

// Helpers for fixupMathML()
static int inspectGroupElement(ldomNode * parent, ldomNode * target,
            bool & target_is_first_child, bool & target_is_last_child,
            bool recurse_inner_group_elements, bool include_msqrt_as_group=false) {
    int nb_others_non_space_like = 0;
    ldomNode * child = parent->getUnboxedFirstChild(true);
    target_is_first_child = child == target;
    while ( child ) {
        lUInt16 childId = child->getNodeId();
        if ( child == target ) {
            // Ignore it
        }
        else if ( childId == el_mtext || childId == el_mspace ) {
            // space-like: ignore it
            // (even mtext with content is to be considered space-like)
        }
        else if ( recurse_inner_group_elements && (
                          (childId >= el_mrow && childId <= el_mpadded)
                        || childId == el_math
                        || childId == el_maction
                        || (include_msqrt_as_group && childId == el_msqrt)
                    ) ) {
            // Not space-like, but space-like if it contains only space-like
            bool unused_is_first, unused_is_last;
            nb_others_non_space_like += inspectGroupElement(child, target,
                unused_is_first, unused_is_last, true, include_msqrt_as_group);
        }
        else {
            nb_others_non_space_like++;
        }
        ldomNode * tmp = child->getUnboxedNextSibling(true);
        if ( !tmp )
            break;
        child = tmp;
    }
    target_is_last_child = child == target;
    return nb_others_non_space_like;
}

// (used in lvrend.cpp via mathml_table_ext.h)
ldomNode * getMathMLCoreEmbelishedOperator( ldomNode * node ) {
    while ( node ) {
        if ( node->getNodeId() == el_mo )
            return node;
        if ( node->hasAttribute(attr_Memb) ) {
            node = node->getUnboxedFirstChild(true);
        }
        else {
            // We may have space-like elements like a leading <mtext>
            node = node->getUnboxedNextSibling(true);
        }
    }
    return NULL;
}

// Simpler clone of the one in lvtinydom.cpp.
// (We don't want to use node->initNodeStyleRecursive() in
// fixupMathML as the element we do that on may be the <math>
// element - which initNodeStyleRecursive() may do more
// with when met)
static void updateStyleDataRecursive( ldomNode * node ) {
    if ( !node->isElement() )
        return;
    node->initNodeStyle();
    int n = node->getChildCount();
    for ( int i=0; i<n; i++ ) {
        ldomNode * child = node->getChildNode(i);
        if ( child->isElement() )
            updateStyleDataRecursive( child );
    }
}

static void fixupMathML( ldomNode * node, bool is_in_script ) {
    lUInt16 nodeElementId = node->getNodeId();

    // Currently, we only have to deal with <mo>
    if ( nodeElementId != el_mo )
        return;
    // A <mo> element should have a single text node as child
    if ( node->getChildCount() != 1 )
        return;
    ldomNode * child = node->getChildNode(0);
    if ( !child->isText() )
        return;

    // We need to find out if this operator is "embellished":
    // - the form will be the position of the top most embellishing element,
    // - lspace/rspace should be applied outside this element
    // - stretching should be done up to this element container box
    // (This is the most tedious part of the MathML specs, but should be
    // ensured for things to look not too bad...)
    //
    // https://mathml-refresh.github.io/mathml-core/#embellished-operators
    // "A MathML Core element is an embellished operator if it is:
    //  1. An <mo> element"
    ldomNode * topEmbelNode = node;
    ldomNode * tmpNode = node;
    while ( true ) {
        ldomNode * unboxedParent = tmpNode->getUnboxedParent();
        if ( !unboxedParent ) {
            break;
        }
        lUInt16 unboxedParentId = unboxedParent->getNodeId();
        if ( unboxedParentId < EL_MATHML_START || unboxedParentId > EL_MATHML_END ) {
            break; // out of <math>
        }
        // "2. A scripted element (<mmultiscripts>, <mover>, <msub>, <msubsup>,
        //  <msup>, <munder> and <munderover> ) or an <mfrac>, whose first
        //  in-flow child exists and is an embellished operator"
        if ( (unboxedParentId >= el_munder && unboxedParentId <= el_mmultiscripts) || unboxedParentId == el_mfrac ) {
            if ( unboxedParent->getUnboxedFirstChild(true) == tmpNode ) {
                topEmbelNode = unboxedParent;
                tmpNode = unboxedParent;
                continue;
            }
        }
        // "3. A grouping element (<maction>, <math>, <merror> <mphantom>,
        // <mprescripts>, <mrow>, <mstyle>, <none>, <semantics>) or <mpadded>,
        // whose in-flow children consist (in any order) of one embellished
        // operator and zero or more space-like elements (<mtext>, <mspace> - or
        // grouping elements or <mpadded> containing only space-like elements).
        if ( (unboxedParentId >= el_mrow && unboxedParentId <= el_mpadded)
                            || unboxedParentId == el_math
                            || unboxedParentId == el_maction
                        ) {
            // Note: el_math is kept pure inline (to at least allow breaking inside inline <math>
            // with simple math statements), so if it would embellishes a v-stretchy operator, we
            // wouldn't be able to stretch it (lvtext require inlineBoxes).
            // So, let <math> not be an embellisher (Acid3's "test 44: mo stretchy").
            // todo: see about <mphantom> and <mstyle> if they should be inline-block, although
            // we don't need them to be for any other reason than allowing embelishment to
            // pop up to where it could.
            bool unused_is_first, unused_is_last;
            int nb_others_non_space_like = inspectGroupElement(unboxedParent, tmpNode, unused_is_first, unused_is_last, true);
            // printf("emb grp %d (%d %d)\n", nb_others_non_space_like, unused_is_first, unused_is_last);
            if ( nb_others_non_space_like == 0 ) {
                if (    unboxedParentId == el_math
                     || unboxedParentId == el_mphantom
                     || unboxedParentId == el_mstyle
                     || unboxedParentId == el_maction
                        ) {
                    // These ones are set display: inline and not inline-block: it won't help
                    // much if they are the top embellisher node with Mtransform=vstretch as
                    // the code handling that expects inlineBoxes.
                    // So we keep the previous node be the top embellisher, but we keep
                    // checking embellisment from them: if they're an upper more friendly
                    // container, it will be the top embellisher.
                    tmpNode = unboxedParent;
                }
                else {
                    topEmbelNode = unboxedParent;
                    tmpNode = unboxedParent;
                }
                continue;
            }
        }
        // Not sure what to do with other MathML elements not mentionned
        // in mathml-core, like <mfenced> and some others.
        break;
    }
    bool is_embelished = topEmbelNode != node;
        // if ( is_embelished ) printf("%s embelished up to %s\n",
        //            UnicodeToLocal(ldomXPointer(node, 0).toString()).c_str(),
        //            UnicodeToLocal(ldomXPointer(topEmbelNode, 0).toString()).c_str());

    // Get this <mo> properties from the dictionary
    lString32 text = child->getText();
    const mathml_operator_dict_entry *   infix = NULL;
    const mathml_operator_dict_entry *  prefix = NULL;
    const mathml_operator_dict_entry * postfix = NULL;
    bool found = getOperatorProperties((const lChar32 *)text.c_str(), infix, prefix, postfix);
        // printf("%s %d %d %d %d\n", UnicodeToLocal(text).c_str(), found, infix, prefix, postfix);

    // The "form" (prefix/infix/postfix) to be used depends on various things:
    // https://mathml-refresh.github.io/mathml-core/#dictionary-based-attributes
    // "1. If the form attribute is present and valid on the core operator,
    //  then its ASCII lowercased value is used"
    int form = MATHML_OP_UNDEF;
    if ( node->hasAttribute(attr_form) ) {
        lString32 at_form = node->getAttributeValueLC( attr_form );
        if ( at_form == U"infix" && infix )
            form = MATHML_OP_INFIX;
        else if ( at_form == U"prefix" && prefix )
            form = MATHML_OP_PREFIX;
        else if ( at_form == U"postfix" && postfix )
            form = MATHML_OP_POSTFIX;
    }
    if ( form == MATHML_OP_UNDEF ) {
        // If not, we must look at the context if this operator (or its uppermost
        // container it embellishes) is a prefix or a postfix
        ldomNode * unboxedParent = topEmbelNode->getUnboxedParent();
        lUInt16 unboxedParentId = unboxedParent->getNodeId();
        // "2. If the embellished operator is the first in-flow child of a grouping
        //  element, <mpadded> or <msqrt> with more than one in-flow child (ignoring
        //  all space-like children) then it has form prefix;"
        // "3. Or, if the embellished operator is the last in-flow child of a grouping
        //  element, <mpadded> or <msqrt> with more than one in-flow child (ignoring
        //  all space-like children) then it has form postfix;"
        if ( (unboxedParentId >= el_mrow && unboxedParentId <= el_mpadded) ||
                    unboxedParentId == el_math || unboxedParentId == el_maction ||
                    unboxedParentId == el_msqrt ) {
            bool is_first, is_last;
            int nb_others_non_space_like = inspectGroupElement(unboxedParent, topEmbelNode, is_first, is_last, false, true);
            if ( nb_others_non_space_like > 0 ) {
                if ( is_first )
                    form = MATHML_OP_PREFIX;
                else if ( is_last )
                    form = MATHML_OP_POSTFIX;
            }
        }
        if ( form == MATHML_OP_UNDEF ) {
            // "4. Or, if the embellished operator is an in-flow child of a scripted
            //  element, other than the first in-flow child, then it has form postfix;"
            if ( (unboxedParentId >= el_munder && unboxedParentId <= el_mmultiscripts) ) {
                if ( unboxedParent->getUnboxedFirstChild(true) != topEmbelNode )
                    form = MATHML_OP_POSTFIX;
            }
        }
        // "5. Otherwise, the embellished operator has form infix."
        if ( form == MATHML_OP_UNDEF ) {
            form = MATHML_OP_INFIX;
        }
    }

    // We have everything to get the properties for this operator
    // from the dictionary (they may still be overriden by some
    // attributes, handled below).
    // Specs are a bit confusing
    // https://www.w3.org/TR/MathML/chapter3.html#presm.formdefval says:
    // "If the operator does not occur in the dictionary with the specified form,
    //  the renderer should use one of the forms that is available there, in the
    //  order of preference: infix, postfix, prefix; if no forms are available
    //  for the given mo element content, the renderer should use the defaults
    //  given in parentheses in the table of attributes for mo."
    // But https://mathml-refresh.github.io/mathml-core/ does not mention that.
    // Given that <mo>(</mo> appears only once in the operator dictionary
    // for the PREFIX form, and results in it being STRETCHY - by using that
    // form because of the lack of an INFIX form, we'd get all '(' stretched.
    // And that's what we get with Firefox. So, do as the specs say.
    int lspace = 5;
    int rspace = 5;
    int flags = 0;
    if ( prefix && (form == MATHML_OP_PREFIX || (!infix && !postfix)) ) {
        lspace = prefix->lspace;
        rspace = prefix->rspace;
        flags = prefix->flags;
        // Set mform="prefix" (right form) or "prefix-f" (fallback form),
        // mostly only for debugging (with View HTML, or coloring via CSS)
        if ( form == MATHML_OP_PREFIX )
            node->setAttributeValue(LXML_NS_NONE, attr_Mform, U"prefix");
        else
            node->setAttributeValue(LXML_NS_NONE, attr_Mform, U"prefix-f"); // "-f" = fallback
    }
    else if ( postfix && (form == MATHML_OP_POSTFIX || !infix) ) {
        lspace = postfix->lspace;
        rspace = postfix->rspace;
        flags = postfix->flags;
        if ( form == MATHML_OP_POSTFIX )
            node->setAttributeValue(LXML_NS_NONE, attr_Mform, U"postfix");
        else
            node->setAttributeValue(LXML_NS_NONE, attr_Mform, U"postfix-f");
    }
    else if ( infix ) {
        lspace = infix->lspace;
        rspace = infix->rspace;
        flags = infix->flags;
        if ( form == MATHML_OP_INFIX )
            node->setAttributeValue(LXML_NS_NONE, attr_Mform, U"infix");
        else
            node->setAttributeValue(LXML_NS_NONE, attr_Mform, U"infix-f");
    }
    else {
        // No dictionary info for the operator form: use the above defaults (5/5/0)
    }

    // From https://www.w3.org/TR/MathML/chapter3.html#id.3.2.5.7:
    //   "Some renderers may choose to use no space around most operators
    //   appearing within subscripts or superscripts, as is done in TEX."
    // (It happens Firefox does not reset these space to 0 but use half
    // of their original width, which might look nicer. But ensuring this
    // spacing on operators/accents larger than their base can cause some
    // uneven spacing issues around the base - the specs say nothing about
    // this case. So, safer to drop any spacing. The issue is noticable on
    // https://github.com/mathml-refresh/mathml/issues/210#issuecomment-700367434
    // on the Safari rendering screenshot, where the leading 'a=' as more spacing
    // vs the next '+' than any other thing near a '+' has.)
    // Embelishment can't pass up a sub/superscript, so we can trust from this
    // node's is_in_script that its embelisher is also is_in_script.
    if ( is_in_script ) {
        lspace = 0;
        rspace = 0;
    }

    // We'll be transfering them as ML/MR= attributes as strings
    lString32 lspace_s;
    lString32 rspace_s;
    if ( lspace )
        lspace_s.appendDecimal(lspace);
    if ( rspace )
        rspace_s.appendDecimal(rspace);

    // Parse attributes that might tweak the default flags (only those that we handle)
    if ( node->hasAttribute(attr_lspace) ) {
        // l/rspace= attributes can be lengths
        lString32 at_lspace = node->getAttributeValueLC( attr_lspace );
        css_length_t lspace_l;
        if ( getLengthFromMathMLAttributeValue(at_lspace, lspace_l, false) ) { // ignore percent
            // Valid attribute values, just re-use the original attribute string value
            lspace_s = at_lspace;
        }
    }
    if ( node->hasAttribute(attr_rspace) ) {
        lString32 at_rspace = node->getAttributeValueLC( attr_rspace );
        css_length_t rspace_l;
        if ( getLengthFromMathMLAttributeValue(at_rspace, rspace_l, false) ) {
            rspace_s = at_rspace;
        }
    }
    if ( node->hasAttribute(attr_stretchy) ) {
        lString32 at_stretchy = node->getAttributeValueLC( attr_stretchy );
        if ( at_stretchy == U"true" )
            flags |= MATHML_OP_STRETCHY;
        else if ( at_stretchy == U"false" )
            flags &= ~MATHML_OP_STRETCHY;
    }
    if ( node->hasAttribute(attr_symmetric) ) {
        lString32 at_symmetric = node->getAttributeValueLC( attr_symmetric );
        if ( at_symmetric == U"true" )
            flags |= MATHML_OP_SYMMETRIC;
        else if ( at_symmetric == U"false" )
            flags &= ~MATHML_OP_SYMMETRIC;
    }
    if ( node->hasAttribute(attr_largeop) ) {
        lString32 at_largeop = node->getAttributeValueLC( attr_largeop );
        if ( at_largeop == U"true" )
            flags |= MATHML_OP_LARGEOP;
        else if ( at_largeop == U"false" )
            flags &= ~(MATHML_OP_LARGEOP|MATHML_OP_INTEGRAL);
    }
    if ( node->hasAttribute(attr_movablelimits) ) {
        lString32 at_movablelimits = node->getAttributeValueLC( attr_movablelimits );
        if ( at_movablelimits == U"true" )
            flags |= MATHML_OP_MOVABLELIMITS;
        else if ( at_movablelimits == U"false" )
            flags &= ~MATHML_OP_MOVABLELIMITS;
    }
    if ( node->hasAttribute(attr_accent) ) {
        lString32 at_accent = node->getAttributeValueLC( attr_accent );
        if ( at_accent == U"true" )
            flags |= MATHML_OP_ACCENT;
        else if ( at_accent == U"false" )
            flags &= ~MATHML_OP_ACCENT;
    }
    if ( node->hasAttribute(attr_accentunder) ) {
        lString32 at_accentunder = node->getAttributeValueLC( attr_accentunder );
        if ( at_accentunder == U"true" )
            flags |= MATHML_OP_ACCENT;
        else if ( at_accentunder == U"false" )
            flags &= ~MATHML_OP_ACCENT;
    }
        // todo: accent/accentunder set on an upper munder/mover... should override
        // a value set on the <mo> (but the <mo> value is the default)

    bool vstretch = (flags & MATHML_OP_STRETCHY) && (flags & MATHML_OP_VERTICAL);
    bool hstretch = (flags & MATHML_OP_STRETCHY) && (flags & MATHML_OP_HORIZONTAL);
    bool symmetric = flags & MATHML_OP_SYMMETRIC;
    bool largeop = false;
    bool integral = false;
    if ( node->hasAttribute(attr_MD) ) {
        // These only apply if "displaystyle"/"math-style: normal"
        largeop = flags & MATHML_OP_LARGEOP;
        integral = flags & MATHML_OP_INTEGRAL;
    }
    bool movablelimits = flags & MATHML_OP_MOVABLELIMITS;
    bool accent = flags & MATHML_OP_ACCENT;

    if ( is_embelished ) {
        node->setAttributeValue(LXML_NS_NONE, attr_Memb, U"");
        // If embelished, lspace and rspace apply on the top embelisher,
        // which we handle below
    }
    else {
        // Only if not embelished, we apply the spacing around the <mo>
        if ( !lspace_s.empty() ) {
            node->setAttributeValue(LXML_NS_NONE, attr_ML, lspace_s.c_str());
        }
        if ( !rspace_s.empty() ) {
            node->setAttributeValue(LXML_NS_NONE, attr_MR, rspace_s.c_str());
        }
    }
    if ( hstretch ) {
        node->setAttributeValue(LXML_NS_NONE, attr_Mtransform, U"hstretch");
    }
    if ( vstretch ) {
        node->setAttributeValue(LXML_NS_NONE, attr_Mtransform, U"vstretch");
    }
    if ( symmetric ) {
        node->setAttributeValue(LXML_NS_NONE, attr_Msymmetric, U"");
    }
    if ( integral ) {
        // todo: should this be done by the font itself, with Mtransform ?
        node->setAttributeValue(LXML_NS_NONE, attr_Mlargeop, U"integral");
    }
    else if ( largeop ) {
        node->setAttributeValue(LXML_NS_NONE, attr_Mlargeop, U"");
    }

    if ( is_embelished ) {
        // We need to forward ML and MR up to the topmost embelisher
        ldomNode * tmp = node;
        while ( tmp ) {
            // if ( tmp->getNodeId() == el_math ) printf("reached <math> %d\n", tmp == topEmbelNode);

            // Set an empty Memb attribute on all nodes part of the chain, so we can follow them
            tmp->setAttributeValue(LXML_NS_NONE, attr_Memb, U"");

            if ( tmp != node ) {
                // For any munder/mover/munderover met (Firefox does it even if it's not
                // the top embellisher node), see if they should be swapped to msub/msup
                lInt16 tmpElementId = tmp->getNodeId();
                if ( tmpElementId >= el_munder && tmpElementId <= el_munderover ) {
                    if ( !tmp->hasAttribute(attr_MD) && movablelimits ) {
                        // This <mo> with movable limits is in a munder/mover/munderover
                        // that is not "displaystyle": it should be rendered as if
                        // it was msub/msup/msubsup
                        tmp->setAttributeValue(LXML_NS_NONE, attr_Msubsup, U"");
                    }
                }
                if ( tmpElementId == el_mfrac ) {
                    // Drop the vstretch if we meet a <mfrac> while walking up to the
                    // top embellisher: Firefox seems to not stretch a <mfrac><mo>,
                    // and we wouldn't really know how to stretch a <mo> as a numerator
                    // and what to do about the baselines.
                    vstretch = false;
                }
            }

            if ( tmp == topEmbelNode ) {
                tmp->setAttributeValue(LXML_NS_NONE, attr_Memb, U"top");
                // Also set this on the top node, so we know when sizing/positionning the
                // top node that we should go do the same for the inner <mo>
                if ( vstretch )
                    tmp->setAttributeValue(LXML_NS_NONE, attr_Mtransform, U"vstretch");
                /* but not:
                if ( hstretch )
                   tmp->setAttributeValue(LXML_NS_NONE, attr_Mtransform, U"hstretch");
                // We won't ensure horizontal stretching of embellished operators - as
                // it would mean stretching a <mmultiscripts> base and repositionning
                // all scripts - as it's a lot of work for a probably quite rare case.
                // But we will ensure below horitzontal stretching of operators
                // contained in nested munder/mover (which could be embellished, but
                // not necessarily) which is more common - as well as streching in
                // both directions of <mo> in <mtd>.
                */
                if ( !lspace_s.empty() ) {
                    tmp->setAttributeValue(LXML_NS_NONE, attr_ML, lspace_s.c_str());
                }
                if ( !rspace_s.empty() ) {
                    tmp->setAttributeValue(LXML_NS_NONE, attr_MR, rspace_s.c_str());
                }
                // (Re-applying styles at the initNodeRendMethod() time used to
                // cause many issues - but I haven't seen any! so let's do it.)
                updateStyleDataRecursive(tmp);
                break;
            }
            // No need to tmp->initNodeStyle() this intermediate node: it will be
            // done when meeting the top node with updateStyleDataRecursive()
            tmp = tmp->getParentNode();
        }
    }
    else {
        bool init_style = true;
        if ( accent ) {
            // This is an operator with accent: if we are a child of <munder/mover/munderover>,
            // we want to translate this accentness into Maccent or Maccentunder attributes,
            // that we'll use when rendering the table.
            ldomNode * unboxedParent = node->getUnboxedParent();
            lUInt16 unboxedParentId = unboxedParent->getNodeId();
            if ( unboxedParentId >= el_munder && unboxedParentId <= el_munderover ) {
                // (As this <mo> is not embelished, we know it is not the first child)
                bool is_second_child = false;
                bool is_third_child = false;
                ldomNode * child = unboxedParent->getUnboxedFirstChild();
                if ( child ) {
                    child = child->getUnboxedNextSibling(); // skip base
                }
                if ( child ) {
                    if ( child == node ) {
                        is_second_child = true;
                    }
                    else {
                        child = child->getUnboxedNextSibling(); // third child
                        if ( child && child == node ) {
                            is_third_child = true;
                        }
                    }
                }
                bool is_over = (unboxedParentId == el_mover && is_second_child) || (unboxedParentId == el_munderover && is_third_child);
                bool is_under = is_second_child && (unboxedParentId == el_munder || unboxedParentId == el_munderover);
                bool set_Maccent_over = is_over;
                bool set_Maccent_under = is_under;
                // This accentness can be overriden by attributes of the <munder/mover/munderover>
                if ( is_over ) {
                    if ( unboxedParent->hasAttribute(attr_accent) ) {
                        lString32 at_accent = unboxedParent->getAttributeValueLC( attr_accent );
                        if ( at_accent == U"true" ) {
                            flags |= MATHML_OP_ACCENT;
                        }
                        else if ( at_accent == U"false" ) {
                            flags &= ~MATHML_OP_ACCENT;
                            set_Maccent_over = false;
                        }
                    }
                }
                if ( is_under ) {
                    if ( unboxedParent->hasAttribute(attr_accentunder) ) {
                        lString32 at_accentunder = unboxedParent->getAttributeValueLC( attr_accentunder );
                        if ( at_accentunder == U"true" ) {
                            flags |= MATHML_OP_ACCENT;
                        }
                        else if ( at_accentunder == U"false" ) {
                            flags &= ~MATHML_OP_ACCENT;
                            set_Maccent_under = false;
                        }
                    }
                }
                // Set the computed accentness on our internal attributes
                if ( set_Maccent_over ) {
                    unboxedParent->setAttributeValue(LXML_NS_NONE, attr_Maccent, U"");
                }
                if ( set_Maccent_under ) {
                    unboxedParent->setAttributeValue(LXML_NS_NONE, attr_Maccentunder, U"");
                }
                if ( set_Maccent_over || set_Maccent_under ) {
                    // Re-apply our MathML CSS styling
                    updateStyleDataRecursive(unboxedParent);
                    init_style = false;
                }
            }
        }
        if ( init_style ) {
            // Even if no change, we might want to grab background-color
            // for Mform= for debugging
            node->initNodeStyle();
        }
    }

    if ( hstretch ) {
        ldomNode * top_munderover_container = NULL;
        ldomNode * tmp = node->getUnboxedParent();
        while ( tmp ) {
            lInt16 tmpElementId = tmp->getNodeId();
            if ( tmpElementId >= el_munder && tmpElementId <= el_munderover && !tmp->hasAttribute(attr_Msubsup) ) {
                top_munderover_container = tmp;
            }
            else {
                break;
            }
            tmp = tmp->getUnboxedParent();
        }
        if ( top_munderover_container ) {
            top_munderover_container->setAttributeValue(LXML_NS_NONE, attr_Mhas_hstretch, U"");
        }
    }
    return;
}

// ====================================================================
// Stretchy operators RenderRectAccessor updating
// ====================================================================

void ensureMathMLVerticalStretch( ldomNode * node, lUInt32 line_y, lUInt16 line_baseline, lUInt16 line_height,
                                                               lUInt16 & needed_baseline, lUInt16 & needed_height ) {
    // We use the current frmline height as the stretch target. It might be wrong if
    // some larger elements in the same <mrow> are to be part of the next frmline.
    // The stretching may also feels excessive when the font has a large line-height
    // as we'll target it (ie. (1+1/N) in a <mfrac> with the TeX fonts).
    // (May be we could walk 'node' siblings to guess their height, as
    // all inline-block/table have been rendered, even if not yet splitted
    // to frmlines, and get more accurate stretch target?)
    // (And/or we could use InkMeasurement to get more accurate stretch limits,
    // discarding blank space from line-height?)
    //
    // See if the provided inlineBox node contains a <mo> that should be stretched to
    // fill the current line height (which has already been adjusted to contain the
    // non-stretchy content, including the not-yet-stretched <mo>.
    // It may be a non-embellished <mo>, or any of then embellished node up to the top one.
    if ( !node->isBoxingInlineBox() || node->getChildCount() != 1 )
        return;
    ldomNode * child = node->getChildNode(0);
    ldomNode * mo = NULL;
    if ( child->getNodeId() == el_mo ) {
        // if ( child->hasAttribute(attr_Memb) ) return;
        // Initially, we thought we should not stretch a <mo> if it is embellished,
        // and leave that to the top embellisher. But we should stretch it at each
        // step, so it adjusts to the neighbours at this step. We thought it would
        // not have any intermediate neighbours if it is embellished, but it can
        // with <mtext> or <mspace> being considered "space-like".
        if ( child->hasAttribute(attr_Mtransform) && child->getAttributeValue(attr_Mtransform) == U"vstretch" ) {
            mo = child;
        }
    }
    else if ( child->hasAttribute(attr_Memb) && child->getAttributeValue(attr_Memb) == U"top" &&
              child->hasAttribute(attr_Mtransform) && child->getAttributeValue(attr_Mtransform) == U"vstretch" ) {
        child = getMathMLCoreEmbelishedOperator(child);
        if ( child && child->hasAttribute(attr_Mtransform) && child->getAttributeValue(attr_Mtransform) == U"vstretch" ) {
            mo = child;
        }
    }
    if ( !mo )
        return;

    // 3.2.5.8.2 Vertical Stretching Rules:
    //  The general rules governing stretchy operators are:
    //  - If a stretchy operator is a direct sub-expression of an mrow
    //    element, or is the sole direct sub-expression of an mtd element in
    //    some row of a table, then it should stretch to cover the height and
    //    depth (above and below the axis) of the non-stretchy direct
    //    sub-expressions in the mrow element or table row, unless stretching
    //    is constrained by minsize or maxsize attributes.
    //  - In the case of an embellished stretchy operator, the preceding rule
    //    applies to the stretchy operator at its core.
    //  - The preceding rules also apply in situations where the mrow element
    //    is inferred.
    //  - The rules for symmetric stretching only apply if symmetric="true"
    //    and if the stretching occurs in an mrow or in an mtr whose rowalign
    //    value is either "baseline" or "axis".
    // 3.1.3.1 Inferred <mrow>s
    //  The elements [...] (msqrt, mstyle, merror, mpadded, mphantom, menclose,
    //  mtd, mscarry, and math) conceptually accept a single argument, but
    //  actually accept any number of children. If the number of children is 0
    //  or is more than 1, they treat their contents as a single inferred mrow
    //  formed from all their children, and treat this mrow as the argument.

    // So, looks like we must check the parent is one of these elements
    // Note: Firefox seems to not do it for <math>, but MathJax does.
    ldomNode * parent = node->getUnboxedParent();
    lUInt16 parentElementId = parent->getNodeId();
    if ( (parentElementId < el_mrow || parentElementId > el_mfenced)
                  && parentElementId != el_math
                  && parentElementId != el_msqrt
                  && parentElementId != el_mtd
                  && parentElementId != el_mscarry ) {
        return;
    }

    int orig_mo_baseline;
    int orig_mo_height;
    { // (in its own scope, so these RenderRectAccessors are forgotten when left)
        RenderRectAccessor mofmt( mo );
        orig_mo_height = mofmt.getHeight();
        // Only the inlineBox containing the mo has had its baseline set
        RenderRectAccessor moparentfmt( mo->getParentNode() );
        orig_mo_baseline = moparentfmt.getBaseline();
    }
    int cur_stretch_height = orig_mo_baseline;
    int cur_stretch_depth = orig_mo_height - orig_mo_baseline;

    int stretch_height = line_baseline;
    int stretch_depth = line_height - line_baseline;
        // Note: currently, we can only draw a stretched glyph with a single scale factor
        // (we can't with different scale factors for above and below the baseline): so,
        // the baseline of the stretched glyph will be a bit random, and will no longer
        // align with its original baseline...

    if ( mo->hasAttribute(attr_Msymmetric) ) {
        // "If symmetric="true", then the computed height and depth of the stretchy operator are:
        //  - height = max(maxheight-axis, maxdepth+axis) + axis
        //  - depth  = max(maxheight-axis, maxdepth+axis) - axis"
        // We take the axis height from the inlineBox's parent's font
        int axis  = parent->getFont()->getExtraMetric(font_metric_math_axis_height);
        int ascend = stretch_height - axis;
        int descend = stretch_depth + axis;
        int max = ascend > descend ? ascend : descend;
        stretch_height = max + axis;
        stretch_depth = max - axis;
    }
    // Correct these for any minsize= and maxsize= attributes (absolute lengths,
    // or factor to the glyph native size)
    // "If the total size = height+depth is less than minsize or greater than
    // maxsize, increase or decrease both height and depth proportionately so
    // that the effective size meets the constraint."
    if ( mo->hasAttribute(attr_minsize) ) {
        lString32 at_minsize = mo->getAttributeValueLC(attr_minsize);
        css_length_t minsize;
        if ( getLengthFromMathMLAttributeValue(at_minsize, minsize) ) {
            // We may not have the native glyph size anymore for relative lengths,
            // so use the font size as an estimate of the glyph size)
            int base_em = mo->getFont()->getSize();
            int minsize_px = lengthToPx( mo, minsize, base_em, base_em );
            int cursize = stretch_height + stretch_depth;
            if ( cursize < minsize_px ) {
                stretch_height = stretch_height * minsize_px / cursize;
                stretch_depth = stretch_depth * minsize_px / cursize;
            }
        }
    }
    if ( mo->hasAttribute(attr_maxsize) ) {
        lString32 at_maxsize = mo->getAttributeValueLC(attr_maxsize);
        css_length_t maxsize;
        if ( getLengthFromMathMLAttributeValue(at_maxsize, maxsize) ) {
            // We may not have the native glyph size anymore for relative lengths,
            // so use the font size as an estimate of the glyph size)
            int base_em = mo->getFont()->getSize();
            int maxsize_px = lengthToPx( mo, maxsize, base_em, base_em );
            int cursize = stretch_height + stretch_depth;
            if ( cursize > maxsize_px ) {
                stretch_height = stretch_height * maxsize_px / cursize;
                stretch_depth = stretch_depth * maxsize_px / cursize;
            }
        }
    }

    int d_above = stretch_height > cur_stretch_height ? stretch_height - cur_stretch_height : 0;
    int d_below =  stretch_depth > cur_stretch_depth  ?  stretch_depth - cur_stretch_depth  : 0;
    int d_total = d_above + d_below;

    // We will walk from the mo up to node, and set all containers
    // to have Y=0 and the height of the stretched <mo>, while keeping
    // the baseline where appropriate.
    ldomNode * tmp = mo;
    while ( tmp ) {
        lUInt16 tmpElementId = tmp->getNodeId();
        RenderRectAccessor tfmt( tmp );
        if ( tmp == mo )
            RENDER_RECT_SET_FLAG(tfmt, DO_MATH_TRANSFORM);

        if ( tmp == node ) {
            // We reach the top inlineBox: its erm_final contains the frmline
            // that has had this called.
            // We do as below, except about the erm_final
            int y = tfmt.getY() - d_above;
            int baseline = tfmt.getBaseline() + d_above;
            int height = tfmt.getHeight() + d_total;
            tfmt.setY( y );
            tfmt.setBaseline( baseline );
            tfmt.setHeight( height );
            // This start inlineBox may have had its baseline and height increased
            // by this loop: let lvtextm.cpp alignLine() knows about it so it can
            // adjust reposition all inlineboxes in the current frmline
            if ( baseline > needed_baseline )
                needed_baseline = baseline;
            if ( height > needed_height )
                needed_height = height;
            return;
        }

        if ( tmp->isBoxingInlineBox() ) {
            ldomNode * finalNode = tmp;
            while (finalNode && finalNode->getRendMethod() != erm_final)
                finalNode = finalNode->getParentNode();
            RenderRectAccessor finalfmt( finalNode );

            // erm_final are tricky: content is laid out from getInnerY() (which
            // represents the padding-top of the final block). From there, there
            // is no y positionning: all is aligned on the baseline, and the
            // baseline is set from the largest text baseline, or the inlineBox
            // with the largest baseline.
            // This inlineBox may or may not be the largest one... but it tells
            // us where the baseline of the erm_final is (we assume in this that
            // the erm_final is a single line!)
            if ( tfmt.getY() >= d_above ) {
                // This inlineBox can extend above without reaching innerY, and
                // will let another inlineBox or the text set the baseline
                tfmt.setY( tfmt.getY() - d_above );
                tfmt.setBaseline( tfmt.getBaseline() + d_above );
                tfmt.setHeight( tfmt.getHeight() + d_total );
            }
            else {
                // This inlineBox will set the baseline - but we have to lie, as
                // all other inlineBoxes are positionned relative to innerY, and we
                // can't easily update all their getY
                int baseline_in_erm_final = tfmt.getY() + tfmt.getBaseline();
                int ascend = tfmt.getBaseline() + d_above;
                int shift_y = baseline_in_erm_final - ascend; // a negative value
                tfmt.setBaseline( baseline_in_erm_final ); // We keep it
                tfmt.setY( shift_y ); // a negative value
                tfmt.setHeight( tfmt.getHeight() + d_total );
                // And move the innerY (from where this tfmt.setY() and other
                // inlineBoxes' getY() will start drawing them) down
                finalfmt.setInnerY( finalfmt.getInnerY() - shift_y );
            }

            // Skip every parent (erm_inline, no x/y/w/h positionning
            // and size) up to the erm_final node's parent
            tmp = finalNode->getParentNode();
            continue;
        }

        // Here, we should only meet pure block elements (table upper node,
        // that contains mathBox as cell possibly erm_final): we can just
        // extend them to keep wrapping their inner content
        tfmt.setY( 0 );
        tfmt.setHeight( tfmt.getHeight() + d_total );
        // tfmt.setBaseline( tfmt.getBaseline() + d_above ); // never used with pure block elements

        // But a few MathML block elements need some tweaks
        if ( tmpElementId == el_munder || tmpElementId == el_munderover ||
                tmpElementId == el_msub || tmpElementId == el_msubsup ) {
            // These 4 elements have a lower 'subscript' element that should be moved down
            // (no need to do anything for 'superscripts' of mover or msup)
            ldomNode * sub = tmp->getUnboxedFirstChild(true, el_mathBox); // The "base" <mathBox>
            if (sub) {
                sub = sub->getUnboxedNextSibling(true, el_mathBox); // The "under/sub" <mathBox>
                if (sub) {
                    RenderRectAccessor subfmt( sub );
                    subfmt.setY( subfmt.getY() + d_total ); // push it down by the height added
                }
            }
        }
        else if ( tmpElementId == el_mmultiscripts ) {
            // This elements may also have, among others, 'subscript' elements
            ldomNode * sub = tmp->getUnboxedFirstChild(true, el_mathBox); // The "base" <mathBox>
            if (sub) {
                bool next_is_sub = true;
                sub = sub->getUnboxedNextSibling(true, el_mathBox);
                while ( sub ) {
                    lUInt16 subElementId = sub->getNodeId();
                    if ( subElementId == el_none ) {
                        next_is_sub = !next_is_sub;
                    }
                    else if ( subElementId == el_mprescripts ) {
                        next_is_sub = true;
                    }
                    else {
                        if ( next_is_sub ) {
                            RenderRectAccessor subfmt( sub );
                            subfmt.setY( subfmt.getY() + d_total ); // push it down by the height added
                        }
                        next_is_sub = !next_is_sub;
                    }
                    sub = sub->getUnboxedNextSibling(true, el_mathBox);
                }
            }
        }
        tmp = tmp->getParentNode();
    }

    // Shouldn't be reached: we should have returned above when tmp == node
    return;
}

static void ensureMathMLInnerMOsHorizontalStretchRecursive( ldomNode * node, ldomNode * topNode, int width=-1 ) {
    // todo: ensure minsize, maxsize, symmetric attributes
    for (int i=0; i<node->getChildCount(); i++) {
        ldomNode * child = node->getChildNode(i);
        if ( !child || !child->isElement() )
            continue;
        lUInt16 childElementId = child->getNodeId();
        if ( childElementId >= EL_BOXING_START && childElementId <= EL_BOXING_END ) {
            if ( width == -1 && childElementId == el_mathBox ) {
                // Grab the width of the first mathBox we meet: we know a mathBox
                // child of munder/mover has been set table-cell and has the width
                // of the table, so the width all <mo>s in these nested munder/mover
                // can expand to.
                RenderRectAccessor boxfmt( child );
                width = boxfmt.getWidth();
            }
            // Walk down this boxing node looking for real MathML elements
            ensureMathMLInnerMOsHorizontalStretchRecursive(child, topNode, width);
        }
        else if ( childElementId >= el_munder && childElementId <= el_munderover ) {
            ensureMathMLInnerMOsHorizontalStretchRecursive(child, topNode, width);
        }
        else if ( width >= 0 && childElementId == el_mo && child->hasAttribute(attr_Mtransform)
                        && child->getAttributeValue(attr_Mtransform) == U"hstretch" ) {
            { // (in its own scope, so this RenderRectAccessor is forgotten when left)
                RenderRectAccessor mofmt( child );
                // The first time we go thru this, we must have DO_MATH_TRANSFORM *not* set,
                // so getInkOffsets() below can get the non-stretched glyph ink height so
                // we can ensure this height don't change by storing it in the RenderRectAccessor.
                // Only then, we set this flag so the drawing code can stretch it.
                if ( RENDER_RECT_HAS_FLAG(mofmt, DO_MATH_TRANSFORM) ) {
                    // Already done
                    return;
                }
                if ( mofmt.getWidth() >= width) {
                    // Don't scale down
                    return;
                }
            }
            // Get coordinate of mo inside topNode
            int mo_x = 0;
            ldomNode * tmp = child;
            for (; tmp != topNode; tmp = tmp->getParentNode()) {
                RenderRectAccessor tfmt( tmp );
                mo_x += tfmt.getX();
                if ( RENDER_RECT_HAS_FLAG(tfmt, INNER_FIELDS_SET) ) {
                    mo_x += tfmt.getInnerX(); // add padding left
                }
            }

            // The erm_final <mo> has been sized normally accordingly to the font size
            // and line height, but it had been positionned finely by the munder/mover
            // table rendering code according to the math font metrics.
            // When stretching it, we should drop the normal erm_final height, and
            // adjust it to the non-scaled glyph ink height and position, so that when
            // scaled, it stays vertically positionned at the same place.
            // (Unfortunately, we had nowhere to store the virtual table-row elements' heights,
            // otherwise we could know how much unused space is available before we reach
            // neighbours, that we could use to scale the height a bit up. So, we have to
            // limit the scaled height to the current ink height of the original non-scaled
            // glyph: they will look quite thin and flat :/ )
            RenderRectAccessor mofmt( child );
            lvRect ink_offsets;
            bool has_ink = getInkOffsets( child, ink_offsets);
            if ( has_ink ) {
                mofmt.setInnerY( mofmt.getInnerY() + ink_offsets.top );
                mofmt.setBaseline( mofmt.getBaseline() - ink_offsets.top ); // not used
                mofmt.setHeight( mofmt.getHeight() - ink_offsets.top - ink_offsets.bottom);
            }
            mofmt.setX( mofmt.getX() - mo_x );
            mofmt.setWidth( width );
            RENDER_RECT_SET_FLAG(mofmt, DO_MATH_TRANSFORM);
        }
    }
}

void ensureMathMLInnerMOsHorizontalStretch( ldomNode * node ) {
    // We should be called on the top most munder/mover/munderover
    // that may contain as direct children other munder/mover/munderover
    // and <mo>: we will look for and process these <mo>.
    ensureMathMLInnerMOsHorizontalStretchRecursive(node, node);
}

void ensureMathMLMOInMTDStretch( ldomNode * node ) {
    if ( node->getNodeId() != el_mtd )
        return;
    if ( node->getChildCount() != 1 )
        return;
    ldomNode * ibox = node->getChildNode(0);
    if ( ibox->getNodeId() != el_inlineBox )
        return; // Not the expected inlineBox containing the <mo>
    if ( ibox->getChildCount() != 1 )
        return;
    ldomNode * mo = ibox->getChildNode(0);
    if ( mo->getNodeId() != el_mo )
        return; // Not a <mo>
    if ( !mo->hasAttribute(attr_Mtransform) )
        return; // Not stretchy
    if ( node->getUnboxedLastChild(true) != mo )
        return; // Not standalone

    // todo: ensure minsize, maxsize, symmetric attributes
    // todo: ensure it too for embellished operators

    lString32 stretch_type = mo->getAttributeValue(attr_Mtransform);
    RenderRectAccessor mtdfmt( node );
    RenderRectAccessor iboxfmt( ibox );
    RenderRectAccessor mofmt( mo );
    if ( stretch_type == U"vstretch" ) {
        // The <mo> might have already gone thru ensureMathMLVerticalStretch(), so, unlike
        // in previous functions,  we do not check for DO_MATH_TRANSFORM being unset.
        // Get the available inner height to stretch into
        css_style_ref_t mtd_style = node->getStyle();
        int padding_top = lengthToPx( node, mtd_style->padding[2], mtdfmt.getWidth() ) + measureBorder(node, 0);
        int padding_bottom = lengthToPx( node, mtd_style->padding[3], mtdfmt.getWidth() ) + measureBorder(node, 2);
        int height = mtdfmt.getHeight() - padding_top - padding_bottom;
        mtdfmt.setInnerY(padding_top); // reset pad set by any cell vertical-align
        // Have the <inlineBox> fill the cell's inner vertical space
        iboxfmt.setY(0);
        iboxfmt.setInnerY(0);
        iboxfmt.setHeight( height );
        // Have the <mo> fill the cell's inner vertical space
        mofmt.setY(0);
        mofmt.setInnerY(0);
        mofmt.setHeight( height );
        RENDER_RECT_SET_FLAG(mofmt, DO_MATH_TRANSFORM); // Have it stretched when drawing
        // Note: we could check the glyph ink width vs the available padding, to increase
        // it a bit if the height scaling is important, to avoid a too thin scaled glyph
    }
    else if ( stretch_type == U"hstretch" ) {
        // The <mo> shouldn't have gone thru ensureMathMLInnerMOsHorizontalStretch(),
        // so check for DO_MATH_TRANSFORM being unset as we need the original glyph
        // ink measurements.
        if ( RENDER_RECT_HAS_FLAG(mofmt, DO_MATH_TRANSFORM) ) {
            return;
        }
        lvRect ink_offsets;
        bool has_ink = getInkOffsets( mo, ink_offsets);
        if ( has_ink ) {
            mofmt.setInnerY( mofmt.getInnerY() + ink_offsets.top );
            mofmt.setBaseline( mofmt.getBaseline() - ink_offsets.top ); // not used
            mofmt.setHeight( mofmt.getHeight() - ink_offsets.top - ink_offsets.bottom);
        }
        css_style_ref_t mtd_style = node->getStyle();
        int padding_left = lengthToPx( node, mtd_style->padding[0], mtdfmt.getWidth() ) + measureBorder(node, 3);
        int padding_right = lengthToPx( node, mtd_style->padding[1], mtdfmt.getWidth() ) + measureBorder(node, 1);
        int width = mtdfmt.getWidth() - padding_left - padding_right;
        mtdfmt.setInnerX(padding_left);
        mtdfmt.setInnerWidth(width);
        iboxfmt.setX(0);
        iboxfmt.setInnerX(0);
        iboxfmt.setWidth( width );
        mofmt.setX(0);
        mofmt.setInnerX(0);
        mofmt.setWidth( width );
        mofmt.setInnerWidth( width );
        RENDER_RECT_SET_FLAG(mofmt, DO_MATH_TRANSFORM);
        // Note: we could check the glyph ink height vs the available padding, to increase
        // it a bit if the width scaling is important, to avoid a too thin scaled glyph
    }
}

#endif // MATHML_SUPPORT==1
