/** \file lvstsheet.h
    \brief style sheet

    Implements CSS compiler for CoolReader Engine.

    Supports only subset of CSS.

    Selectors supported:

    - * { } - universal selector
    - element-name { } - selector by element name
    - element1, element2 { } - several selectors delimited by comma

    Properties supported:

    - display
    - white-space
    - text-align
    - vertical-align
    - font-family
    - font-size
    - font-style
    - font-weight
    - text-indent
    - line-height
    - width
    - height
    - margin-left
    - margin-right
    - margin-top
    - margin-bottom
    - margin
    

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/


#ifndef __LVSTSHEET_H_INCLUDED__
#define __LVSTSHEET_H_INCLUDED__

#include "cssdef.h"
#include "lvstyles.h"
#include "textlang.h"

class lxmlDocBase;
struct ldomNode;

/** \brief CSS property declaration
    
    Currently supports only subset of properties.

    Properties supported:

    - display
    - white-space
    - text-align
    - vertical-align
    - font-family
    - font-size
    - font-style
    - font-weight
    - text-indent
    - line-height
    - width
    - height
    - margin-left
    - margin-right
    - margin-top
    - margin-bottom
    - margin
*/
class LVCssDeclaration {
private:
    int * _data;
public:
    void apply( css_style_rec_t * style );
    bool empty() { return _data==NULL; }
    bool parse( const char * & decl, bool higher_importance=false, lxmlDocBase * doc=NULL, lString16 codeBase=lString16::empty_str );
    lUInt32 getHash();
    LVCssDeclaration() : _data(NULL) { }
    ~LVCssDeclaration() { if (_data) delete[] _data; }
};

typedef LVRef<LVCssDeclaration> LVCssDeclRef;

// See https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-classes
enum LVCssSelectorPseudoClass
{
    csspc_root,             // :root
    csspc_dir,              // :dir(rtl), :dir(ltr)
    csspc_first_child,      // :first-child
    csspc_first_of_type,    // :first-of-type
    csspc_nth_child,        // :nth-child(even), :nth-child(3n+4)
    csspc_nth_of_type,      // :nth-of-type()
    // Those after this won't be valid when checked in the initial
    // document loading phase when the XML is being parsed, as at
    // this point, the checked node is always the last node as we
    // haven't yet parsed its following siblings. When meeting one,
    // we'll need to re-render and re-check styles after load with
    // a fully built DOM.
    csspc_last_child,       // :last-child
    csspc_last_of_type,     // :last-of-type
    csspc_nth_last_child,   // :nth-last-child()
    csspc_nth_last_of_type, // :nth-last-of-type()
    csspc_only_child,       // :only-child
    csspc_only_of_type,     // :only-of-type
    csspc_empty,            // :empty
};

static const char * css_pseudo_classes[] =
{
    "root",
    "dir",
    "first-child",
    "first-of-type",
    "nth-child",
    "nth-of-type",
    "last-child",
    "last-of-type",
    "nth-last-child",
    "nth-last-of-type",
    "only-child",
    "only-of-type",
    "empty",
    NULL
};

// https://developer.mozilla.org/en-US/docs/Web/CSS/Pseudo-elements
enum LVCssSelectorPseudoElement
{
    csspe_before = 1,   // ::before
    csspe_after  = 2,   // ::after
};

static const char * css_pseudo_elements[] =
{
    "before",
    "after",
    NULL
};

enum LVCssSelectorRuleType
{
    cssrt_universal,         // *
    cssrt_parent,            // E > F
    cssrt_ancessor,          // E F
    cssrt_predecessor,       // E + F
    cssrt_predsibling,       // E ~ F
    cssrt_attrset,           // E[foo]
    cssrt_attreq,            // E[foo="value"]
    cssrt_attreq_i,          // E[foo="value i"] (case insensitive)
    cssrt_attrhas,           // E[foo~="value"]
    cssrt_attrhas_i,         // E[foo~="value i"]
    cssrt_attrstarts_word,   // E[foo|="value"]
    cssrt_attrstarts_word_i, // E[foo|="value i"]
    cssrt_attrstarts,        // E[foo^="value"]
    cssrt_attrstarts_i,      // E[foo^="value i"]
    cssrt_attrends,          // E[foo$="value"]
    cssrt_attrends_i,        // E[foo$="value i"]
    cssrt_attrcontains,      // E[foo*="value"]
    cssrt_attrcontains_i,    // E[foo*="value i"]
    cssrt_id,                // E#id
    cssrt_class,             // E.class
    cssrt_pseudoclass        // E:pseudo-class, E:pseudo-class(value)
};

class LVCssSelectorRule
{
    //
    LVCssSelectorRuleType _type;
    lUInt16 _id;
    lUInt16 _attrid;
    LVCssSelectorRule * _next;
    lString16 _value;
public:
    LVCssSelectorRule(LVCssSelectorRuleType type)
    : _type(type), _id(0), _attrid(0), _next(NULL)
    { }
    LVCssSelectorRule( LVCssSelectorRule & v );
    void setId( lUInt16 id ) { _id = id; }
    void setAttr( lUInt16 id, lString16 value ) { _attrid = id; _value = value; }
    LVCssSelectorRule * getNext() { return _next; }
    void setNext(LVCssSelectorRule * next) { _next = next; }
    ~LVCssSelectorRule() { if (_next) delete _next; }
    /// check condition for node
    bool check( const ldomNode * & node );
    /// check next rules for node
    bool checkNextRules( const ldomNode * node );
    /// Some selector rule types do the full rules chain check themselves
    bool isFullChecking() { return _type == cssrt_ancessor || _type == cssrt_predsibling; }
    lUInt32 getHash();
    lUInt32 getWeight();
};

/** \brief simple CSS selector
    
    Currently supports only element name and universal selector.

    - * { } - universal selector
    - element-name { } - selector by element name
    - element1, element2 { } - several selectors delimited by comma
*/
class LVCssSelector {
private:


    lUInt16 _id;
    LVCssDeclRef _decl;
    int _specificity;
    int _pseudo_elem; // from enum LVCssSelectorPseudoElement, or 0
    LVCssSelector * _next;
    LVCssSelectorRule * _rules;
    void insertRuleStart( LVCssSelectorRule * rule );
    void insertRuleAfterStart( LVCssSelectorRule * rule );
public:
    LVCssSelector( LVCssSelector & v );
    LVCssSelector() : _id(0), _specificity(0), _pseudo_elem(0),  _next(NULL), _rules(NULL) { }
    LVCssSelector(int specificity) : _id(0), _specificity(specificity), _pseudo_elem(0), _next(NULL), _rules(NULL) { }
    ~LVCssSelector() { if (_next) delete _next; if (_rules) delete _rules; }
    bool parse( const char * &str, lxmlDocBase * doc );
    lUInt16 getElementNameId() { return _id; }
    bool check( const ldomNode * node ) const;
    void applyToPseudoElement( const ldomNode * node, css_style_rec_t * style ) const;
    void apply( const ldomNode * node, css_style_rec_t * style ) const
    {
        if (check( node )) {
            if ( _pseudo_elem > 0 ) {
                applyToPseudoElement(node, style);
            }
            else {
                _decl->apply(style);
            }
            // style->flags |= STYLE_REC_FLAG_MATCHED;
            // Done in applyToPseudoElement() as currently only needed there.
            // Uncomment if more generic usage needed.
        }
    }
    void setDeclaration( LVCssDeclRef decl ) { _decl = decl; }
    int getSpecificity() { return _specificity; }
    LVCssSelector * getNext() { return _next; }
    void setNext(LVCssSelector * next) { _next = next; }
    lUInt32 getHash();
};


/** \brief stylesheet
    
    Can parse stylesheet and apply compiled rules.

    Currently supports only subset of CSS features.

    \sa LVCssSelector
    \sa LVCssDeclaration
*/
class LVStyleSheet {
    lxmlDocBase * _doc;

    int _selector_count;
    LVArray <int> _selector_count_stack;

    LVPtrVector <LVCssSelector> _selectors;
    LVPtrVector <LVPtrVector <LVCssSelector> > _stack;
    LVPtrVector <LVCssSelector> * dup()
    {
        LVPtrVector <LVCssSelector> * res = new LVPtrVector <LVCssSelector>();
        res->reserve( _selectors.length() );
        for ( int i=0; i<_selectors.length(); i++ ) {
            LVCssSelector * selector = _selectors[i];
            if ( selector )
                res->add( new LVCssSelector(*selector) );
            else
                res->add(NULL);
        }
        return res;
    }

    void set(LVPtrVector<LVCssSelector> & v );
public:


    // save current state of stylesheet
    void push()
    {
        _selector_count_stack.add( _selector_count );
        _stack.add( dup() );
    }
    // restore previously saved state
    bool pop()
    {
        // Restore original counter (so we don't overflow the 19 bits
        // of _specificity reserved for storing selector order, so up
        // to 524288, when we meet a book with 600 DocFragments each
        // including a 1000 selectors stylesheet).
        if ( !_selector_count_stack.empty() )
            _selector_count = _selector_count_stack.remove( _selector_count_stack.length()-1 );
        LVPtrVector <LVCssSelector> * v = _stack.pop();
        if ( !v )
            return false;
        set( *v );
        delete v;
        return true;
    }

    /// remove all rules from stylesheet
    void clear() {
        _selector_count = 0;
        _selector_count_stack.clear();
        _selectors.clear();
        _stack.clear();
    }
    /// set document to retrieve ID values from
    void setDocument( lxmlDocBase * doc ) { _doc = doc; }
    /// constructor
    LVStyleSheet( lxmlDocBase * doc = NULL ) : _doc(doc), _selector_count(0) { }
    /// copy constructor
    LVStyleSheet( LVStyleSheet & sheet );
    /// parse stylesheet, compile and add found rules to sheet
    bool parse( const char * str, bool higher_importance=false, lString16 codeBase=lString16::empty_str );
    /// apply stylesheet to node style
    void apply( const ldomNode * node, css_style_rec_t * style );
    /// calculate hash
    lUInt32 getHash();
};

/// parse color value like #334455, #345 or red
bool parse_color_value( const char * & str, css_length_t & value );

/// update (if needed) a style->content (parsed from the CSS declaration) before
//  applying to a node's style
void update_style_content_property( css_style_rec_t * style, ldomNode * node );
/// get the computed final text value for a node from its style->content
lString16 get_applied_content_property( ldomNode * node );

/// extract @import filename from beginning of CSS
bool LVProcessStyleSheetImport( const char * &str, lString8 & import_file );
/// load stylesheet from file, with processing of import
bool LVLoadStylesheetFile( lString16 pathName, lString8 & css );

#endif // __LVSTSHEET_H_INCLUDED__
