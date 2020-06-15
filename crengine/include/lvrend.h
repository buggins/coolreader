/** \file lvrend.h
    \brief DOM document rendering (formatting) functions

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_REND_H_INCLUDED__
#define __LV_REND_H_INCLUDED__

#include "lvtinydom.h"
#include "textlang.h"

// Current direction, from dir="ltr" or dir="rtl" element attribute
// Should map directly to the RENDER_RECT_FLAG_DIRECTION_* below
// (bit 0: 0=unset 1=set - bit 1: 0=normal 1=inverted - bit 2: 0=horizontal 1=vertical)
#define REND_DIRECTION_UNSET  0X00
#define REND_DIRECTION_LTR    0X01 // 0b001
#define REND_DIRECTION_RTL    0X03 // 0b011
// Not supported:
// #define REND_DIRECTION_TTB 0X05 // 0b101
// #define REND_DIRECTION_BTT 0X07 // 0b111

// Flags for RenderRectAccessor
#define RENDER_RECT_FLAG_DIRECTION_SET                      0x0001
#define RENDER_RECT_FLAG_DIRECTION_INVERTED                 0x0002
#define RENDER_RECT_FLAG_DIRECTION_VERTICAL                 0x0004 // not used (only horizontal currently supported)
#define RENDER_RECT_FLAG_INNER_FIELDS_SET                   0x0008
#define RENDER_RECT_FLAG_BOX_IS_RENDERED                    0x0010 // for floatBox and inlineBox
#define RENDER_RECT_FLAG_NO_CLEAR_OWN_FLOATS                0x0020
#define RENDER_RECT_FLAG_FINAL_FOOTPRINT_AS_SAVED_FLOAT_IDS 0x0040
#define RENDER_RECT_FLAG_FLOATBOX_IS_RIGHT                  0x0080
#define RENDER_RECT_FLAG_NO_INTERLINE_SCALE_UP              0x0100 // for ruby elements to not scale up

#define RENDER_RECT_SET_FLAG(r, f)     ( r.setFlags( r.getFlags() | RENDER_RECT_FLAG_##f ) )
#define RENDER_RECT_UNSET_FLAG(r, f)   ( r.setFlags( r.getFlags() & ~RENDER_RECT_FLAG_##f ) )
#define RENDER_RECT_HAS_FLAG(r, f)     ( (bool)(r.getFlags() & RENDER_RECT_FLAG_##f) )
#define RENDER_RECT_PTR_HAS_FLAG(r, f) ( (bool)(r->getFlags() & RENDER_RECT_FLAG_##f) )

#define RENDER_RECT_FLAG_DIRECTION_MASK                     0x0007
#define RENDER_RECT_SET_DIRECTION(r, d)   ( r.setFlags( r.getFlags() | d ) )
#define RENDER_RECT_GET_DIRECTION(r)      ( r.getFlags() & RENDER_RECT_FLAG_DIRECTION_MASK )
#define RENDER_RECT_PTR_GET_DIRECTION(r)  ( r->getFlags() & RENDER_RECT_FLAG_DIRECTION_MASK )
#define RENDER_RECT_HAS_DIRECTION(r)      ( (bool)(r.getFlags() & RENDER_RECT_FLAG_DIRECTION_SET) )
#define RENDER_RECT_HAS_DIRECTION_RTL(r)  ( (bool)(r.getFlags() & RENDER_RECT_FLAG_DIRECTION_MASK == REND_DIRECTION_RTL) )
#define RENDER_RECT_PTR_HAS_DIRECTION_RTL(r)  ( (bool)(r->getFlags() & RENDER_RECT_FLAG_DIRECTION_MASK == REND_DIRECTION_RTL) )

// To be provided via the initial value to renderBlockElement(... int *baseline ...) to
// have FlowState compute baseline (different rules whether inline-block or inline-table).
#define REQ_BASELINE_NOT_NEEDED       0
#define REQ_BASELINE_FOR_INLINE_BLOCK 1    // use last baseline fed
#define REQ_BASELINE_FOR_TABLE        2    // keep first baseline fed

class FlowState;

// Footprint of block floats (from FlowState) on a final block,
// to be passed to lvtextfm LFormattedText::Format().
// Also allows LFormattedText to pass back information about
// its own embedded floats if they overflow the final block
// own height.
class BlockFloatFootprint {
private:
    FlowState * flow;
    int d_left;
    int d_top;
    int used_min_y;
    int used_max_y;
public:
    // Forwarded from BLOCK_RENDERING_DO_NOT_CLEAR_OWN_FLOATS
    // (or not when table cell erm_final)
    bool no_clear_own_floats;
    bool use_floatIds;
    // Footprints, used when they are more than 5 floats involved,
    // or ALLOW_EXACT_FLOATS_FOOTPRINTS not enabled.
    int left_w;
    int left_h;
    int right_w;
    int right_h;
    int left_min_y;
    int right_min_y;
    // FloatIds, used when less than 5 floats involved
    // and ALLOW_EXACT_FLOATS_FOOTPRINTS enabled
    int nb_floatIds;
    lUInt32 floatIds[5];
    // Floats to transfer to final block for it to
    // start with these 5 "fake" embedded floats
    int floats_cnt;
    int floats[5][5]; // max 5 floats, with (x,y,w,h,is_right) each

    int getFinalMinY() { return used_min_y; };
    int getFinalMaxY() { return used_max_y; };
    void store( ldomNode * node );
    void restore( ldomNode * node, int final_width );
    void generateEmbeddedFloatsFromFootprints( int final_width );
    void generateEmbeddedFloatsFromFloatIds( ldomNode * node, int final_width );
    void forwardOverflowingFloat( int x, int y, int w, int h, bool r, ldomNode * node );
    int getTopShiftX(int final_width, bool get_right_shift=false);

    BlockFloatFootprint( FlowState * fl=NULL, int dleft=0, int dtop=0, bool noclearownfloats=false ) :
        flow(fl), d_left(dleft), d_top(dtop), no_clear_own_floats(noclearownfloats),
        used_min_y(0), used_max_y(0),
        left_w(0), left_h(0), right_w(0), right_h(0), left_min_y(0), right_min_y(0),
        use_floatIds(false), nb_floatIds(0), floats_cnt(0)
        { }
};

/// returns true if styles are identical
bool isSameFontStyle( css_style_rec_t * style1, css_style_rec_t * style2 );
/// removes format data from node
void freeFormatData( ldomNode * node );
/// returns best suitable font for style
LVFontRef getFont(css_style_rec_t * style, int documentId);
/// initializes format data for node
void initFormatData( ldomNode * node );
/// initializes rendering method for node
int initRendMethod( ldomNode * node, bool recurseChildren, bool allowAutoboxing );
/// converts style to text formatting API flags
lUInt32 styleToTextFmtFlags( bool is_block, const css_style_ref_t & style, lUInt32 oldflags, int direction=REND_DIRECTION_UNSET );
/// renders block as single text formatter object
void renderFinalBlock( ldomNode * node, LFormattedText * txform, RenderRectAccessor * fmt, lUInt32 & flags,
                       int indent, int line_h, TextLangCfg * lang_cfg=NULL, int valign_dy=0, bool * is_link_start=NULL );
/// renders block which contains subblocks (with gRenderBlockRenderingFlags as flags)
int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width, int direction=REND_DIRECTION_UNSET, int * baseline=NULL );
/// renders block which contains subblocks
int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width, int direction, int * baseline, int rend_flags );
/// renders table element
int renderTable( LVRendPageContext & context, ldomNode * element, int x, int y, int width,
                 bool shrink_to_fit, int & fitted_width, int direction=REND_DIRECTION_UNSET,
                 bool pb_inside_avoid=false, bool enhanced_rendering=false, bool is_ruby_table=false );
/// sets node style
void setNodeStyle( ldomNode * node, css_style_ref_t parent_style, LVFontRef parent_font );
/// copy style
void copystyle( css_style_ref_t sourcestyle, css_style_ref_t deststyle );

/// draws formatted document to drawing buffer
void DrawDocument( LVDrawBuf & drawbuf, ldomNode * node, int x0, int y0, int dx, int dy, int doc_x, int doc_y,
                   int page_height, ldomMarkedRangeList * marks, ldomMarkedRangeList * bookmarks = NULL,
                   bool draw_content=true, bool draw_background=true );

// Estimate width of node when rendered:
//   maxWidth: width if it would be rendered on an infinite width area
//   minWidth: width with a wrap on all spaces (no hyphenation), so width taken by the longest word
// full function for recursive use:
void getRenderedWidths(ldomNode * node, int &maxWidth, int &minWidth, int direction, bool ignorePadding, int rendFlags,
            int &curMaxWidth, int &curWordWidth, bool &collapseNextSpace, int &lastSpaceWidth,
            int indent, TextLangCfg * lang_cfg, bool processNodeAsText=false, bool isStartNode=false);
// simpler function for first call:
void getRenderedWidths(ldomNode * node, int &maxWidth, int &minWidth, int direction=REND_DIRECTION_UNSET, bool ignorePadding=false, int rendFlags=0);

#define STYLE_FONT_EMBOLD_MODE_NORMAL 0
#define STYLE_FONT_EMBOLD_MODE_EMBOLD 300

/// set global document font style embolden mode (0=off, 300=on)
void LVRendSetFontEmbolden( int addWidth=STYLE_FONT_EMBOLD_MODE_EMBOLD );
/// get global document font style embolden mode
int LVRendGetFontEmbolden();

int measureBorder(ldomNode *enode,int border);
int lengthToPx( css_length_t val, int base_px, int base_em, bool unspecified_as_em=false );
int scaleForRenderDPI( int value );

#define BASE_CSS_DPI 96 // at 96 dpi, 1 css px = 1 screen px
#define DEF_RENDER_DPI 96
#define DEF_RENDER_SCALE_FONT_WITH_DPI 0
extern int gRenderDPI;
extern bool gRenderScaleFontWithDPI;
extern int gRootFontSize;

#define INTERLINE_SCALE_FACTOR_NO_SCALE 1024
#define INTERLINE_SCALE_FACTOR_SHIFT 10
extern int gInterlineScaleFactor;

extern int gRenderBlockRenderingFlags;

// Enhanced rendering flags
#define BLOCK_RENDERING_ENHANCED                           0x00000001
#define BLOCK_RENDERING_ALLOW_PAGE_BREAK_WHEN_NO_CONTENT   0x00000002 // Allow consecutive page breaks when only separated
                                                                      // by margin/padding/border.
// Vertical margins
#define BLOCK_RENDERING_COLLAPSE_VERTICAL_MARGINS          0x00000010 // Collapse vertical margins
#define BLOCK_RENDERING_ALLOW_VERTICAL_NEGATIVE_MARGINS    0x00000020 // Allow individual negative margins in the calculation, the
                                                                      // final collapsed margin is ensure to be zero or positive.
#define BLOCK_RENDERING_ALLOW_NEGATIVE_COLLAPSED_MARGINS   0x00000040 // Allow the final vertical collapsed margin to be negative
                                                                      // (may mess with page splitting and text selection).
// Horizontal margins
#define BLOCK_RENDERING_ENSURE_MARGIN_AUTO_ALIGNMENT       0x00000100 // Ensure CSS "margin: auto", for aligning blocks.
#define BLOCK_RENDERING_ALLOW_HORIZONTAL_NEGATIVE_MARGINS  0x00000200 // Allow negative margins (otherwise, they are set to 0)
#define BLOCK_RENDERING_ALLOW_HORIZONTAL_BLOCK_OVERFLOW    0x00000400 // Allow block content to overflow its block container.
#define BLOCK_RENDERING_ALLOW_HORIZONTAL_PAGE_OVERFLOW     0x00000800 // Allow block content to overflow the page rect, showing
                                                                      // in the margin, and possibly clipped out.
// Widths and heights
#define BLOCK_RENDERING_USE_W3C_BOX_MODEL                  0x00001000 // Use W3C box model (CSS width and height do not include
                                                                      // paddings and borders)
#define BLOCK_RENDERING_ALLOW_STYLE_W_H_ABSOLUTE_UNITS     0x00002000 // Allow widths and heights in absolute units (when ensured)
#define BLOCK_RENDERING_ENSURE_STYLE_WIDTH                 0x00004000 // Ensure CSS widths and heights on all elements (otherwise
#define BLOCK_RENDERING_ENSURE_STYLE_HEIGHT                0x00008000 // only on <HR> and images, and when sizing floats).
// Floats
#define BLOCK_RENDERING_WRAP_FLOATS                        0x00010000 // Wrap floats in an internal floatBox element.
#define BLOCK_RENDERING_PREPARE_FLOATBOXES                 0x00020000 // Avoid style hash mismatch when toggling FLOAT_FLOATBOXES,
                                                                      // but make embedded floats inline when no more floating.
#define BLOCK_RENDERING_FLOAT_FLOATBOXES                   0x00040000 // Actually render floatBoxes floating.
// These 2, although allowing a more correct rendering of floats, can impact drawing performances and text/links selection:
#define BLOCK_RENDERING_DO_NOT_CLEAR_OWN_FLOATS            0x00100000 // Prevent blocks from clearing their own floats.
#define BLOCK_RENDERING_ALLOW_EXACT_FLOATS_FOOTPRINTS      0x00200000 // When 5 or less outer floats have impact on a final
                                                                      // block, store their ids instead of the 2 top left/right
                                                                      // rectangle, allowing text layout staircase-like.
// Inline block/table
#define BLOCK_RENDERING_BOX_INLINE_BLOCKS                  0x01000000 // Wrap inline-block in an internal inlineBox element.
#define BLOCK_RENDERING_COMPLETE_INCOMPLETE_TABLES         0x02000000 // Add anonymous missing elements to a table without proper
                                                                      // children and table-cells without proper parents

// Enable everything
#define BLOCK_RENDERING_FULL_FEATURED                      0x7FFFFFFF

#define BLOCK_RENDERING_G(f) ( gRenderBlockRenderingFlags & BLOCK_RENDERING_##f )
#define BLOCK_RENDERING(v, f) ( v & BLOCK_RENDERING_##f )

// rendering flags presets
#define BLOCK_RENDERING_FLAGS_LEGACY     0
#define BLOCK_RENDERING_FLAGS_FLAT       ( BLOCK_RENDERING_ENHANCED | \
                                           BLOCK_RENDERING_COLLAPSE_VERTICAL_MARGINS | \
                                           BLOCK_RENDERING_ALLOW_VERTICAL_NEGATIVE_MARGINS | \
                                           BLOCK_RENDERING_USE_W3C_BOX_MODEL | \
                                           BLOCK_RENDERING_WRAP_FLOATS | \
                                           BLOCK_RENDERING_PREPARE_FLOATBOXES | \
                                           BLOCK_RENDERING_BOX_INLINE_BLOCKS )
#define BLOCK_RENDERING_FLAGS_BOOK       ( BLOCK_RENDERING_ENHANCED | \
                                           BLOCK_RENDERING_COLLAPSE_VERTICAL_MARGINS | \
                                           BLOCK_RENDERING_ALLOW_VERTICAL_NEGATIVE_MARGINS | \
                                           BLOCK_RENDERING_ENSURE_MARGIN_AUTO_ALIGNMENT | \
                                           BLOCK_RENDERING_USE_W3C_BOX_MODEL | \
                                           BLOCK_RENDERING_ENSURE_STYLE_WIDTH | \
                                           BLOCK_RENDERING_WRAP_FLOATS | \
                                           BLOCK_RENDERING_PREPARE_FLOATBOXES | \
                                           BLOCK_RENDERING_FLOAT_FLOATBOXES | \
                                           BLOCK_RENDERING_DO_NOT_CLEAR_OWN_FLOATS | \
                                           BLOCK_RENDERING_ALLOW_EXACT_FLOATS_FOOTPRINTS | \
                                           BLOCK_RENDERING_BOX_INLINE_BLOCKS )
#define BLOCK_RENDERING_FLAGS_WEB        BLOCK_RENDERING_FULL_FEATURED
#define BLOCK_RENDERING_FLAGS_DEFAULT    BLOCK_RENDERING_FLAGS_WEB

int validateBlockRenderingFlags( int f );

#endif
