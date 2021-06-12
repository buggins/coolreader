/** \file fb2def.h
    \brief FictionBook2 format defitions

    When included w/o XS_IMPLEMENT_SCHEME defined,
    declares enums for element, attribute and namespace names.

    When included with XS_IMPLEMENT_SCHEME defined,
    defines fb2_elem_table, fb2_attr_table and fb2_ns_table tables
    which can be passed to document to define schema.
    Please include it with XS_IMPLEMENT_SCHEME only into once in project.

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#if !defined(__FB2_DEF_H_INCLUDED__) || defined(XS_IMPLEMENT_SCHEME)
#define __FB2_DEF_H_INCLUDED__

#include "dtddef.h"

//=====================================================
// el_ definitions
//=====================================================
XS_BEGIN_TAGS

// Boxing elements (inserted in the DOM tree between original parent and children):
//
// Internal element for block wrapping inline elements (without a proper parent
// block container) among proper block siblings (would be better named "blockBox")
XS_TAG1T( autoBoxing )
// Internal element for tabular elements added to complete incomplete tables
XS_TAG1T( tabularBox )
// Internal element for ruby wrapping completion (so we can render them as inline-table with tweaks)
XS_TAG1I( rubyBox )
// Internal element used to wrap MathML elements for rendering them as inline-table/inline-block sub-elements
XS_TAG1I( mathBox )
// Internal element for float rendering
XS_TAG1T( floatBox )
// Internal element for inline-block and inline-table rendering
XS_TAG1I( inlineBox )

#define EL_BOXING_START el_autoBoxing
#define EL_BOXING_END   el_inlineBox

// Internal element created for CSS pseudo elements ::before and ::after :
//  - defaults to "display: none", but will be set to "inline" when style is applied
//  - it doesn't have a text node child, the content will be fetched from
//    its style->content when rendering and drawing text.
// It does not box anything and has no child, so it's not considered a boxing node.
XS_TAG1D( pseudoElem, false, css_d_none, css_ws_inherit )

// Internal element for EPUB, containing each individual HTML file
XS_TAG1( DocFragment )

XS_TAG2( xml, "?xml" )
XS_TAG2( xml_stylesheet, "?xml-stylesheet" )

// Classic HTML / EPUB elements
XS_TAG1( html )
XS_TAG1( head )
XS_TAG1D( title, true, css_d_block, css_ws_inherit )
XS_TAG1D( style, true, css_d_none, css_ws_inherit )
XS_TAG1D( script, true, css_d_none, css_ws_inherit )
XS_TAG1D( base, false, css_d_none, css_ws_inherit ) // among crengine autoclose elements
XS_TAG1D( basefont, false, css_d_none, css_ws_inherit )
XS_TAG1D( bgsound, false, css_d_none, css_ws_inherit )
XS_TAG1D( meta, false, css_d_none, css_ws_inherit )
XS_TAG1D( link, false, css_d_none, css_ws_inherit )
XS_TAG1T( body )

// Limits for head handling by our HTML Parser (ldomDocumentWriterFilter)
// (if met before any HEAD or BODY, HTML/HEAD might be auto-inserted)
#define EL_IN_HEAD_START el_head
#define EL_IN_HEAD_END   el_link
#define EL_IN_BODY_START el_body

                // HTML5: start of special tags, closing all
                // the way, and closing any <P>
// Block elements
XS_TAG1T( hr )
XS_TAG1T( svg )
XS_TAG1T( form )
XS_TAG1D( pre, true, css_d_block, css_ws_pre )
XS_TAG1T( blockquote )
XS_TAG1T( div )
XS_TAG1T( h1 )
XS_TAG1T( h2 )
XS_TAG1T( h3 )
XS_TAG1T( h4 )
XS_TAG1T( h5 )
XS_TAG1T( h6 )
XS_TAG1T( p )
XS_TAG1T( output )
XS_TAG1T( section )

// Lists
XS_TAG1T( ol )
XS_TAG1T( ul )
XS_TAG1D( li, true, css_d_list_item_block, css_ws_inherit )

// Definitions
XS_TAG1T( dl )
XS_TAG1T( dt )
XS_TAG1T( dd )

// Tables
XS_TAG1D( table, false, css_d_table, css_ws_inherit )
XS_TAG1D( caption, true, css_d_table_caption, css_ws_inherit )
XS_TAG1D( colgroup, false, css_d_table_column_group, css_ws_inherit )
XS_TAG1D( col, false, css_d_table_column, css_ws_inherit )
XS_TAG1D( thead, false, css_d_table_header_group, css_ws_inherit )
XS_TAG1D( tbody, false, css_d_table_row_group, css_ws_inherit )
XS_TAG1D( tfoot, false, css_d_table_footer_group, css_ws_inherit )
XS_TAG1D( tr, false, css_d_table_row, css_ws_inherit )
XS_TAG1D( th, true, css_d_table_cell, css_ws_inherit )
XS_TAG1D( td, true, css_d_table_cell, css_ws_inherit )

// Added 20180528
// Keep this block starting with "address" and ending with "xmp" as we
// are using: if (id >= el_address && id <= el_xmp) in lvrend.cpp
// Additional semantic block elements
XS_TAG1T( address )
XS_TAG1T( article )
XS_TAG1T( aside )
XS_TAG1T( canvas ) // no support for canvas, but keep it block
XS_TAG1T( fieldset )
XS_TAG1T( figcaption )
XS_TAG1T( figure )
XS_TAG1T( footer )
XS_TAG1T( header )
XS_TAG1T( hgroup )
XS_TAG1T( legend ) // child of fieldset, rendered as block by most browsers
XS_TAG1T( main )
XS_TAG1T( nav )
XS_TAG1T( noscript )
XS_TAG1T( video ) // no support for video, but keep it block
// Additional obsoleted block elements
XS_TAG1T( center )
XS_TAG1T( dir )    // similar to "ul"
XS_TAG1T( menu )   // similar to "ul"
// Other non-inline elements present in html5.css
XS_TAG1T( noframes )
XS_TAG1D( listing, true, css_d_block, css_ws_pre ) // similar to "pre"
XS_TAG1D( textarea, true, css_d_block, css_ws_pre ) // similar to "pre"
XS_TAG1D( plaintext, true, css_d_block, css_ws_pre ) // start of raw text (no end tag), not supported
XS_TAG1D( xmp, true, css_d_block, css_ws_pre ) // similar to "pre"

// Added 20200824
// Keep this block starting with "details" and ending with "wbr" as we
// are using: if (id >= el_details && id <= el_wbr) in lvrend.cpp
// Additional semantic block elements
XS_TAG1T( details )
XS_TAG1T( dialog )
XS_TAG1T( summary )
// Additional "special" elements mentioned in the HTML standard,
// not supposed to close any P, but let's consider them similarly,
// and be block elements, so their content is shown.
XS_TAG1T( frame )
XS_TAG1T( frameset )
XS_TAG1T( iframe )
XS_TAG1T( noembed )
XS_TAG1T( template )
XS_TAG1T( select )
// BUTTON should not close a P, so we could have P > BUTTON > P,
// and other elements close a P "in button scope" - but we want to
// avoid nested Ps - so for our HTML parser, a BUTTON closes a P)
XS_TAG1T( button )
// HTML5: these 3 are special tags, that should not close any <P>,
// but they start a new "scope" that should not be crossed when
// other special tags are closing a P. As they are rare, we make
// them close a P too, just so that we'll never have nested Ps.
XS_TAG1T( marquee )
XS_TAG1( applet )
XS_TAG1( object )
                // HTML5: end of special tags, closing all
                // the way, and closing any <P>
// Other HTML elements with usually no content or no usable content
XS_TAG1T( optgroup ) // shown as block
XS_TAG1I( option )   // shown as inline
XS_TAG1T( map )
XS_TAG1( area )
XS_TAG1( track )
XS_TAG1( embed )
XS_TAG1( input )
XS_TAG1( keygen )
XS_TAG1( param )
XS_TAG1( audio )
XS_TAG1( source )
XS_TAG1I( picture ) // may contain one <img>, and multiple <source>
XS_TAG1I( wbr )

// Limits for special handling by our HTML Parser (ldomDocumentWriterFilter)
#define EL_SPECIAL_START           el_html
#define EL_SPECIAL_END             el_wbr
#define EL_SPECIAL_CLOSING_P_START el_hr
#define EL_SPECIAL_CLOSING_P_END   el_object

// Inline elements
XS_TAG1OBJ( img ) /* inline and specific handling as 'object' */

                // HTML5: start of "active formatting elements"
XS_TAG1I( a )
XS_TAG1I( b )
XS_TAG1I( big )
XS_TAG1I( code ) // should not be css_ws_pre according to specs
XS_TAG1I( em )
XS_TAG1I( font )
XS_TAG1I( i )
XS_TAG1I( nobr )
XS_TAG1I( s )
XS_TAG1I( small )
XS_TAG1I( strike )
XS_TAG1I( strong )
XS_TAG1I( tt )
XS_TAG1I( u )
                // HTML5: end of "active formatting elements"
                // This is just for refence: we don't handle them specifically
                // (in HTML5, when mis-nested tags would close one of these,
                // they are re-opened when leaving the mis-nested tag container)

XS_TAG1I( acronym )
XS_TAG1I( bdi )
XS_TAG1I( bdo )
XS_TAG1I( br )
XS_TAG1I( cite ) // conflict between HTML (inline) and FB2 (block): default here to inline (fb2.css puts it back to block)
XS_TAG1I( del )
XS_TAG1I( dfn )
XS_TAG1I( emphasis )
XS_TAG1I( ins )
XS_TAG1I( kbd )
XS_TAG1I( q )
XS_TAG1I( samp )
XS_TAG1I( span )
XS_TAG1I( sub )
XS_TAG1I( sup )
XS_TAG1I( var )

// Ruby elements (defaults to inline)
XS_TAG1D( ruby, true, css_d_ruby, css_ws_inherit )
XS_TAG1I( rbc ) // no more in HTML5, but in 2001's https://www.w3.org/TR/ruby/
XS_TAG1I( rb )
XS_TAG1I( rtc )
XS_TAG1I( rt )
XS_TAG1I( rp )

// EPUB3 elements (in ns_epub - otherwise set to inline like any unknown element)
XS_TAG1I( switch )  // <epub:switch>
XS_TAG1I( case )    // <epub:case required-namespace="...">
XS_TAG1I( default ) // <epub:default>

// FB2 elements
XS_TAG1( FictionBook )
XS_TAG1( annotation )
XS_TAG1( author )
XS_TAG1( coverpage )
XS_TAG1( epigraph )
XS_TAG1( part )
XS_TAG1( poem )
XS_TAG1( stanza )
XS_TAG1D( binary, true, css_d_none, css_ws_inherit )
XS_TAG1D( description, false, css_d_none, css_ws_inherit )
XS_TAG1D( genre, true, css_d_none, css_ws_inherit )
XS_TAG1D( stylesheet, true, css_d_none, css_ws_inherit )
XS_TAG1I( spacing )
XS_TAG1I( strikethrough )
XS_TAG1I( underline )
XS_TAG1OBJ( image )
XS_TAG1T( city )
XS_TAG1T( date )
XS_TAG1T( email )
XS_TAG1T( history )
XS_TAG1T( id )
XS_TAG1T( isbn )
XS_TAG1T( keywords )
XS_TAG1T( lang )
XS_TAG1T( nickname )
XS_TAG1T( publisher )
XS_TAG1T( sequence )
XS_TAG1T( subtitle )
XS_TAG1T( v )
XS_TAG1T( version )
XS_TAG1T( year )
XS_TAG2( document_info, "document-info" )
XS_TAG2( empty_line, "empty-line" )
XS_TAG2( publish_info, "publish-info" )
XS_TAG2( src_title_info, "src-title-info" )
XS_TAG2( title_info, "title-info" )
XS_TAG2I( first_name, "first-name" )
XS_TAG2I( last_name, "last-name" )
XS_TAG2I( middle_name, "middle-name" )
XS_TAG2T( book_name, "book-name" )
XS_TAG2T( book_title, "book-title" )
XS_TAG2T( custom_info, "custom-info" )
XS_TAG2T( home_page, "home-page" )
XS_TAG2T( program_used, "program-used" )
XS_TAG2T( src_lang, "src-lang" )
XS_TAG2T( src_ocr, "src-ocr" )
XS_TAG2T( src_url, "src-url" )
XS_TAG2T( text_author, "text-author" )

#if MATHML_SUPPORT==1
// MathML presentation elements, all inline by default, but might be modified by mathml_css_h.css
XS_TAG1I( math )
// MathML tokens (containing text)
// https://www.w3.org/TR/MathML/chapter3.html#presm.tokel
XS_TAG1I( mglyph ) // empty element, similar to <img src=>
XS_TAG1I( mi )
XS_TAG1I( mn )
XS_TAG1I( mo )
XS_TAG1I( mtext )
XS_TAG1I( mspace )
XS_TAG1I( ms )
// MathML containers (containing tokens or other containers)
// https://www.w3.org/TR/MathML/chapter3.html#presm.genlayout
// MathML general layout schemata
XS_TAG1I( mrow )
XS_TAG1I( mstyle )
XS_TAG1I( merror )
XS_TAG1I( mphantom )
XS_TAG1I( mpadded )
XS_TAG1I( menclose )
XS_TAG1I( mfenced )
XS_TAG1I( mfrac )
XS_TAG1I( msqrt )
XS_TAG1I( mroot )
// MathML script and limit schemata
XS_TAG1I( munder )
XS_TAG1I( mover )
XS_TAG1I( munderover )
XS_TAG1I( msub )
XS_TAG1I( msup )
XS_TAG1I( msubsup )
XS_TAG1I( mmultiscripts )
XS_TAG1I( mprescripts ) // empty element
XS_TAG1I( none ) // empty element
// MathML script and limit schemata
XS_TAG1I( mtable )
XS_TAG1I( mtr )
XS_TAG1I( mlabeledtr )
XS_TAG1I( mtd )
XS_TAG1I( maligngroup ) // empty element
XS_TAG1I( malignmark ) // empty element
// MathML elementary maths (learning maths)
XS_TAG1I( mstack )
XS_TAG1I( mlongdiv )
XS_TAG1I( msgroup )
XS_TAG1I( msrow )
XS_TAG1I( mscarries )
XS_TAG1I( mscarry )
XS_TAG1I( msline )
// MathML other elements
XS_TAG1I( maction )
XS_TAG1I( semantics )
XS_TAG2I( annotation_xml, "annotation-xml" )
// <annotation> is already defined above for FB2
#define EL_MATHML_START         el_math
#define EL_MATHML_END           el_annotation_xml
#define EL_MATHML_TOKEN_START   el_mglyph
#define EL_MATHML_TOKEN_END     el_ms
#endif // MATHML_SUPPORT==1

XS_END_TAGS


//=====================================================
// attr_ definitions
//=====================================================
XS_BEGIN_ATTRS

XS_ATTR( id )
XS_ATTR( class )
XS_ATTR( value )
XS_ATTR( name )
XS_ATTR( number )
XS_ATTR( href )
XS_ATTR( type )
XS_ATTR( mode )
XS_ATTR( price )
XS_ATTR( style )
XS_ATTR( width )
XS_ATTR( height )
XS_ATTR( colspan )
XS_ATTR( rowspan )
XS_ATTR( rbspan )
XS_ATTR( align )
XS_ATTR( valign )
XS_ATTR( currency )
XS_ATTR( version )
XS_ATTR( encoding )
XS_ATTR( l )
XS_ATTR( xmlns )
XS_ATTR( genre )
XS_ATTR( xlink )
XS_ATTR( link )
XS_ATTR( xsi )
XS_ATTR( schemaLocation )
XS_ATTR( include )
XS_ATTR2( include_all, "include-all" )
XS_ATTR2( content_type, "content-type" )
XS_ATTR( StyleSheet )
XS_ATTR( title )
XS_ATTR( subtitle )
XS_ATTR( suptitle )
XS_ATTR( start )
XS_ATTR( reversed )
XS_ATTR( role )
XS_ATTR( dir )
XS_ATTR( lang )
XS_ATTR( recindex ) // used with mobi images
XS_ATTR2( required_namespace, "required-namespace" ) // <epub:case required-namespace="...">

// Note that attributes parsed in the HTML are lowercased, unlike the ones
// we explicitely set while building the DOM. So, for our internal elements
// needs, let's use some uppercase to avoid conflicts with HTML content
// and the risk to have them matched by publishers CSS selectors.
XS_ATTR( T )      // to flag subtype of boxing internal elements if needed
XS_ATTR( Before ) // for pseudoElem internal element
XS_ATTR( After )  // for pseudoElem internal element
XS_ATTR( ParserHint )   // HTML parser hints (used for Lib.ru support)
XS_ATTR( NonLinear )    // for non-linear items in EPUB

// Other classic attributes present in html5.css
XS_ATTR2( accept_charset, "accept-charset" )
XS_ATTR( alt )
XS_ATTR( background )
XS_ATTR( bgcolor )
XS_ATTR( border )
XS_ATTR( cellpadding )
XS_ATTR( cellspacing )
XS_ATTR( clear )
XS_ATTR( color )
XS_ATTR( cols )
XS_ATTR( disabled )
XS_ATTR( face )
XS_ATTR( hidden )
XS_ATTR( hspace )
XS_ATTR2( http_equiv, "http-equiv" )
XS_ATTR( nowrap )
XS_ATTR( readonly )
XS_ATTR( rel )
XS_ATTR( rows )
XS_ATTR( rules )
XS_ATTR( scheme )
XS_ATTR( selected )
XS_ATTR( src )
XS_ATTR( tabindex )
XS_ATTR( target )
XS_ATTR( vspace )
XS_ATTR( wrap )

#if MATHML_SUPPORT==1
// Various MathML attributes for internal rendering
XS_ATTR( MT )       // MathML element or mathBox: type
XS_ATTR( MD )       // MathML element has or inherits "math-style: normal" / "displaystyle=true": drawn larger
XS_ATTR( MS )       // MathML element has or inherits "math-shift: normal": superscripts can be higher
XS_ATTR( MN )       // MathML element has this "math-depth: <N>" nested level, used for reducing font-size
XS_ATTR( ML )       // MathML element (or mathBox) has this added left spacing
XS_ATTR( MR )       // MathML element (or mathBox) has this added right spacing
XS_ATTR( Memb )     // MathML element is embelished or an embelisher
XS_ATTR( Mform )    // MathML computed form (prefix/infix/postfix) (for debugging)
XS_ATTR( Mlargeop ) // MathML <mo> should be larger ("", "integral")
XS_ATTR( Mtransform ) // MathML <mo> or Memb should be transformed ("vstretch", "hstretch")
XS_ATTR( Msymmetric ) // MathML <mo> stretching should be symmetric
XS_ATTR2( Mhas_hstretch, "has-hstretch" ) // MathML <munder/mover/munderover> contains <mo> that should hstretch
XS_ATTR( Msubsup )  // MathML <munder/mover/munderover> should be rendered as <msub/msup/subsup>
XS_ATTR( Maccent )  // MathML <munder/mover><mathBox> is to be drawn as accent
XS_ATTR( Maccentunder )  // MathML <munder/mover><mathBox> is to be drawn as accentunder
// MathML regular attributes we might want to parse or handle via mathml.css (all attributes
// checked in CSS with ie. 'mtable[frame="solid"] {...}' *must* be declared here; if stuff
// works on first book after launch, but not on later re-open, this might be the reason).
XS_ATTR( display )       // <math>
XS_ATTR( mathcolor )     // most elements
XS_ATTR( mathbackground )
XS_ATTR( mathsize )
XS_ATTR( mathvariant )   // <mo> and most elements
XS_ATTR( form )          // <mo>
XS_ATTR( stretchy )
XS_ATTR( symmetric )
XS_ATTR( largeop )
XS_ATTR( movablelimits )
XS_ATTR( accent )
XS_ATTR( lspace )
XS_ATTR( rspace )
XS_ATTR( maxsize )
XS_ATTR( minsize )
XS_ATTR( linebreak )     // <mo>, <mspace>
XS_ATTR( depth )         // <mspace>
XS_ATTR( lquote )        // <ms>
XS_ATTR( rquote )        // <ms>
XS_ATTR( linethickness ) // <mfrac>
XS_ATTR( bevelled )
XS_ATTR( numalign )
XS_ATTR( denalign )
XS_ATTR( displaystyle )
XS_ATTR( open )          // <mfenced>
XS_ATTR( close )
XS_ATTR( separators )
XS_ATTR( notation )      // <menclose>
XS_ATTR( voffset )       // <mpadded>
XS_ATTR( accentunder )   // <munder>
XS_ATTR( columnalign )   // <mtable>
XS_ATTR( columnlines )
XS_ATTR( columnspan )
XS_ATTR( frame )
XS_ATTR( rowalign )
XS_ATTR( rowlines )
#endif // MATHML_SUPPORT==1

XS_END_ATTRS


//=====================================================
// ns_ definitions
//=====================================================
XS_BEGIN_NS

XS_NS( l )
XS_NS( xsi )
XS_NS( xmlns )
XS_NS( xlink )
XS_NS( xs )
XS_NS( epub )

XS_END_NS


#endif // __FB2_DEF_H_INCLUDED__
