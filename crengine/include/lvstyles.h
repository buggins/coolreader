/** \file lvstyles.h

    \brief CSS Styles and node format

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#if !defined(__LV_STYLES_H_INCLUDED__)
#define __LV_STYLES_H_INCLUDED__

#include "cssdef.h"
#include "lvmemman.h"
#include "lvrefcache.h"
#include "lvtextfm.h"
#include "lvfntman.h"
#include "lvstring8collection.h"

class SerialBuf;

/* bit position (in 'lUInt64 important' and 'lUInt64 importance' bitmaps) of
 * each css_style_rec_tag properties to flag its '!important' status */
// enum css_style_rec_important_bit : lUInt64 {  <= disliked by clang
enum css_style_rec_important_bit {
    imp_bit_display               = 1ULL << 0,
    imp_bit_white_space           = 1ULL << 1,
    imp_bit_text_align            = 1ULL << 2,
    imp_bit_text_align_last       = 1ULL << 3,
    imp_bit_text_decoration       = 1ULL << 4,
    imp_bit_text_transform        = 1ULL << 5,
    imp_bit_vertical_align        = 1ULL << 6,
    imp_bit_font_family           = 1ULL << 7,
    imp_bit_font_name             = 1ULL << 8,
    imp_bit_font_size             = 1ULL << 9,
    imp_bit_font_style            = 1ULL << 10,
    imp_bit_font_weight           = 1ULL << 11,
    imp_bit_font_features         = 1ULL << 12,
    imp_bit_text_indent           = 1ULL << 13,
    imp_bit_line_height           = 1ULL << 14,
    imp_bit_width                 = 1ULL << 15,
    imp_bit_height                = 1ULL << 16,
    imp_bit_margin_left           = 1ULL << 17,
    imp_bit_margin_right          = 1ULL << 18,
    imp_bit_margin_top            = 1ULL << 19,
    imp_bit_margin_bottom         = 1ULL << 20,
    imp_bit_padding_left          = 1ULL << 21,
    imp_bit_padding_right         = 1ULL << 22,
    imp_bit_padding_top           = 1ULL << 23,
    imp_bit_padding_bottom        = 1ULL << 24,
    imp_bit_color                 = 1ULL << 25,
    imp_bit_background_color      = 1ULL << 26,
    imp_bit_letter_spacing        = 1ULL << 27,
    imp_bit_page_break_before     = 1ULL << 28,
    imp_bit_page_break_after      = 1ULL << 29,
    imp_bit_page_break_inside     = 1ULL << 30,
    imp_bit_hyphenate             = 1ULL << 31,
    imp_bit_list_style_type       = 1ULL << 32,
    imp_bit_list_style_position   = 1ULL << 33,
    imp_bit_border_style_top      = 1ULL << 34,
    imp_bit_border_style_bottom   = 1ULL << 35,
    imp_bit_border_style_right    = 1ULL << 36,
    imp_bit_border_style_left     = 1ULL << 37,
    imp_bit_border_width_top      = 1ULL << 38,
    imp_bit_border_width_right    = 1ULL << 39,
    imp_bit_border_width_bottom   = 1ULL << 40,
    imp_bit_border_width_left     = 1ULL << 41,
    imp_bit_border_color_top      = 1ULL << 42,
    imp_bit_border_color_right    = 1ULL << 43,
    imp_bit_border_color_bottom   = 1ULL << 44,
    imp_bit_border_color_left     = 1ULL << 45,
    imp_bit_background_image      = 1ULL << 46,
    imp_bit_background_repeat     = 1ULL << 47,
    imp_bit_background_attachment = 1ULL << 48,
    imp_bit_background_position   = 1ULL << 49,
    imp_bit_border_collapse       = 1ULL << 50,
    imp_bit_border_spacing_h      = 1ULL << 51,
    imp_bit_border_spacing_v      = 1ULL << 52,
    imp_bit_orphans               = 1ULL << 53,
    imp_bit_widows                = 1ULL << 54,
    imp_bit_float                 = 1ULL << 55,
    imp_bit_clear                 = 1ULL << 56,
    imp_bit_direction             = 1ULL << 57,
    imp_bit_content               = 1ULL << 58,
    imp_bit_cr_hint               = 1ULL << 59
};

// Style handling flags
#define STYLE_REC_FLAG_MATCHED  0x01 // This style has had some stylesheet declaration matched and applied.
                                     // Currently only used for a pseudo element style,
                                     // see LVCssSelector::apply() if more generic usage needed.

/**
    \brief Element style record.

    Contains set of style properties.
*/
typedef struct css_style_rec_tag css_style_rec_t;
struct css_style_rec_tag {
    int                  refCount; // for reference counting
    lUInt32              hash; // cache calculated hash value here
    lUInt64              important;  // bitmap for !important (used only by LVCssDeclaration)
                                     // we have currently below 60 css properties
                                     // lvstsheet knows about 82, which are mapped to these 60
                                     // update bits above if you add new properties below
    lUInt64              importance; // bitmap for important bit's importance/origin
                                     // (allows for 2 level of !important importance)
    css_display_t        display;
    css_white_space_t    white_space;
    css_text_align_t     text_align;
    css_text_align_t     text_align_last;
    css_text_decoration_t text_decoration;
    css_text_transform_t text_transform;
    css_length_t         vertical_align;
    css_font_family_t    font_family;
    lString8             font_name;
    css_length_t         font_size;
    css_font_style_t     font_style;
    css_font_weight_t    font_weight;
    css_length_t         font_features;
    css_length_t         text_indent;
    css_length_t         line_height;
    css_length_t         width;
    css_length_t         height;
    css_length_t         margin[4]; ///< margin-left, -right, -top, -bottom
    css_length_t         padding[4]; ///< padding-left, -right, -top, -bottom
    css_length_t         color;
    css_length_t         background_color;
    css_length_t         letter_spacing;
    css_page_break_t     page_break_before;
    css_page_break_t     page_break_after;
    css_page_break_t     page_break_inside;
    css_hyphenate_t        hyphenate;
    css_list_style_type_t list_style_type;
    css_list_style_position_t list_style_position;
    css_border_style_type_t border_style_top;
    css_border_style_type_t border_style_bottom;
    css_border_style_type_t border_style_right;
    css_border_style_type_t border_style_left;
    css_length_t border_width[4]; ///< border-top-width, -right-, -bottom-, -left-
    css_length_t border_color[4]; ///< border-top-color, -right-, -bottom-, -left-
    lString8 background_image;
    css_background_repeat_value_t background_repeat;
    css_background_attachment_value_t background_attachment;
    css_background_position_value_t background_position;
    css_border_collapse_value_t border_collapse;
    css_length_t border_spacing[2];//first horizontal and the second vertical spacing
    css_orphans_widows_value_t orphans;
    css_orphans_widows_value_t widows;
    css_float_t            float_; // "float" is a C++ keyword...
    css_clear_t            clear;
    css_direction_t        direction;
    lString16              content;
    css_cr_hint_t          cr_hint;
    // The following should only be used when applying stylesheets while in lvend.cpp setNodeStyle(),
    // and cleaned up there, before the style is cached and shared. They are not serialized.
    lInt8                flags; // bitmap of STYLE_REC_FLAG_*
    css_style_rec_t *    pseudo_elem_before_style;
    css_style_rec_t *    pseudo_elem_after_style;

    css_style_rec_tag()
    : refCount(0)
    , hash(0)
    , important(0)
    , importance(0)
    , display( css_d_inline )
    , white_space(css_ws_inherit)
    , text_align(css_ta_inherit)
    , text_align_last(css_ta_inherit)
    , text_decoration (css_td_inherit)
    , text_transform (css_tt_inherit)
    , vertical_align(css_val_unspecified, css_va_baseline)
    , font_family(css_ff_inherit)
    , font_size(css_val_inherited, 0)
    , font_style(css_fs_inherit)
    , font_weight(css_fw_inherit)
    , font_features(css_val_inherited, 0)
    , text_indent(css_val_inherited, 0)
    , line_height(css_val_inherited, 0)
    , width(css_val_unspecified, 0)
    , height(css_val_unspecified, 0)
    , color(css_val_inherited, 0)
    , background_color(css_val_unspecified, 0)
    , letter_spacing(css_val_inherited, 0)
    , page_break_before(css_pb_auto)
    , page_break_after(css_pb_auto)
    , page_break_inside(css_pb_auto)
    , hyphenate(css_hyph_inherit)
    , list_style_type(css_lst_inherit)
    , list_style_position(css_lsp_inherit)
    , border_style_top(css_border_none)
    , border_style_bottom(css_border_none)
    , border_style_right(css_border_none)
    , border_style_left(css_border_none)
    , background_repeat(css_background_r_none)
    , background_attachment(css_background_a_none)
    , background_position(css_background_p_none)
    , border_collapse(css_border_seperate)
    , orphans(css_orphans_widows_inherit)
    , widows(css_orphans_widows_inherit)
    , float_(css_f_none)
    , clear(css_c_none)
    , direction(css_dir_inherit)
    , cr_hint(css_cr_hint_none)
    , flags(0)
    , pseudo_elem_before_style(NULL)
    , pseudo_elem_after_style(NULL)
    {
        // css_length_t fields are initialized by css_length_tag()
        // to (css_val_screen_px, 0)
        // These should not: a not specified border width will
        // use DEFAULT_BORDER_WIDTH (=2)
        border_width[0] = css_length_t(css_val_unspecified, 0);
        border_width[1] = css_length_t(css_val_unspecified, 0);
        border_width[2] = css_length_t(css_val_unspecified, 0);
        border_width[3] = css_length_t(css_val_unspecified, 0);
    }
    void AddRef() { refCount++; }
    int Release() { return --refCount; }
    int getRefCount() { return refCount; }
    bool serialize( SerialBuf & buf );
    bool deserialize( SerialBuf & buf );
    //  important bitmap management
    bool isImportant( css_style_rec_important_bit bit ) { return important & bit; }
    void setImportant( css_style_rec_important_bit bit ) { important |= bit; }
    // apply value to field if important bit not yet set, then set it if is_important
    template <typename T> inline void Apply( T value, T *field, css_style_rec_important_bit bit, lUInt8 is_important ) {
        // is_important is 2 bits: (high_importance, is_important) (see lvstsheet.cpp)
        if (     !(important & bit)     // important flag not previously set
              || (is_important == 0x3)  // coming value has '!important' and comes from some CSS parsed with higher_importance=true
              || (is_important == 0x1 && !(importance & bit) ) // coming value has '!important' and comes from some CSS parsed
                                                               // with higher_importance=false, but previous value was not set from
                                                               // a !important that came from CSS parsed with higher_importance=true
           ) {
            *field = value; // apply
            if (is_important & 0x1) important |= bit;   // update important flag
            if (is_important == 0x3) importance |= bit; // update importance flag (!important comes from higher_importance CSS)
        }
    }
    // Similar to previous one, but logical-OR'ing values, for bitmaps (currently, only style->font_features)
    inline void ApplyAsBitmapOr( css_length_t value, css_length_t *field, css_style_rec_important_bit bit, lUInt8 is_important ) {
        if (     !(important & bit)
              || (is_important == 0x3)
              || (is_important == 0x1 && !(importance & bit) )
           ) {
            field->value |= value.value; // logical-or values
            field->type = value.type;    // use the one from value (always css_val_unspecified for font_features)
            if (is_important & 0x1) important |= bit;
            if (is_important == 0x3) importance |= bit;
        }
    }
};

/// style record reference type
typedef LVFastRef< css_style_rec_t > css_style_ref_t;
/// font reference type
typedef LVFontRef font_ref_t;

/// to compare two styles
bool operator == (const css_style_rec_t & r1, const css_style_rec_t & r2);

/// style hash table size
#define LV_STYLE_HASH_SIZE 0x100

/// style cache: allows to avoid duplicate style object allocation
class lvdomStyleCache : public LVRefCache< css_style_ref_t >
{
public:
    lvdomStyleCache( int size = LV_STYLE_HASH_SIZE ) : LVRefCache< css_style_ref_t >( size ) {}
};

/// element rendering methods
enum lvdom_element_render_method
{
    erm_invisible = 0, ///< invisible: don't render
    erm_killed,        ///< reset to this when there is no room to render element
    erm_block,         ///< render as block element (render as containing other elements)
    erm_final,         ///< final element: render the whole it's content as single render block
    erm_inline,        ///< inline element
    erm_runin,         ///< run-in (used as a solution to inline FB2 footnotes)
    erm_list_item,     ///< obsolete/legacy: render as block element as list item
    erm_table,         ///< table element: render as table
    erm_table_row_group,    ///< table row group
    erm_table_header_group, ///< table header group
    erm_table_footer_group, ///< table footer group
    erm_table_row,          ///< table row
    erm_table_column_group, ///< table column group
    erm_table_column,       ///< table column
    // Note that table cells always become either erm_block or erm_final depending on their content
    // and that table captions are set erm_final.
};

/// node format record
class lvdomElementFormatRec {
protected:
    // Values on the x-axis can be stored in a 16bit int, values
    // on the y-axis should better be stored in a 32bit int.
    // Some of these fields were added to support floats and
    // optimize some computations (we increased the size of
    // this object from 4 to 16 32bits-int - this make cache
    // files grow only by ~1%, probably because they compress
    // well when not filled and left to be 0).

    // Position and size of a (block or final) element, relative to
    // it's parent block container top left, not including margins,
    // but including borders and paddings.
    int    _y;
    int    _height;
    short  _x;
    short  _width;
    // For erm_final block, inner width and position (so, excluding
    // paddings and border) of the LFormattedText inside that element.
    short  _inner_width; // = _width - left and right borders and paddings
    short  _inner_x;     // = left border + padding
    short  _inner_y;     // = top border + padding
                         // (no need for any _inner_height currently)

    short  _baseline;    // reference baseline y (only set for inline-block box)
        // (Note: it's limited to 32767px here, but images and inline-box height
        // is also limited to that for being carried in lInt16 slots when
        // formatting text in lvtextfm.cpp.)

    // Children blocks should be fully contained in their parent block,
    // and sibling nodes blocks should not overlap with other siblings,
    // except when float are involved and we allow them to continue
    // over next outer elements.
    // Record these overflows, mostly to just be able to draw floats
    // correctly and not ignore them if their container block is out
    // of screen. Also help selecting text in overflowing floats.
    int _top_overflow;    // Overflow (positive value) below _y
    int _bottom_overflow; // Overflow (positive value) after _y+_height

    int _lang_node_idx;     // dataIndex of the upper node this erm_final block
                            // should get its lang= langage from

    // Flags & extras, to have additional info related to this rect cached.
    // - For erm_final nodes, these contain the footprint of outer floats
    //   that we need to know when accessing this final node (for re-
    //   formatting, when drawing, or selecting text or links...) to expect
    //   a reproducible layout (see BlockFloatFootprint for the different
    //   possible usages of these extra fields).
    unsigned short  _flags;
    unsigned short  _extra0;
    int  _extra1;
    int  _extra2;
    int  _extra3;
    int  _extra4;
    int  _extra5;

    int _listprop_node_idx; // dataIndex of the UL/OL node this erm_final block
                            // should get its marker from

    // Added for padding from 15 to 16 32-bits ints
    int _available1;

public:
    lvdomElementFormatRec()
    : _x(0), _width(0), _y(0), _height(0)
    , _inner_width(0), _inner_x(0), _inner_y(0), _baseline(0)
    , _top_overflow(0), _bottom_overflow(0)
    , _lang_node_idx(0) , _listprop_node_idx(0)
    , _flags(0), _extra0(0)
    , _extra1(0), _extra2(0), _extra3(0), _extra4(0), _extra5(0)
    , _available1(0)
    {
    }
    ~lvdomElementFormatRec()
    {
    }
    void clear()
    {
        _x = _width = _y = _height = 0;
        _inner_width = _inner_x = _inner_y = _baseline = 0;
        _top_overflow = _bottom_overflow = 0;
        _lang_node_idx = _listprop_node_idx = 0;
        _flags = _extra0 = 0;
        _extra1 = _extra2 = _extra3 = _extra4 = _extra5 = 0;
        _available1 = 0;
    }
    bool operator == ( lvdomElementFormatRec & v )
    {
        return (_height==v._height && _y==v._y && _width==v._width && _x==v._x &&
                _inner_width==v._inner_width && _inner_x==v._inner_x &&
                _inner_y==v._inner_y && _baseline==v._baseline &&
                _top_overflow==v._top_overflow && _bottom_overflow==v._bottom_overflow &&
                _lang_node_idx==v._lang_node_idx && _listprop_node_idx==v._listprop_node_idx &&
                _flags==v._flags && _extra0==v._extra0 &&
                _extra1==v._extra1 && _extra2==v._extra2 && _extra3==v._extra3 &&
                _extra4==v._extra4 && _extra5==v._extra5 &&
                _available1==v._available1
                );
    }
    bool operator != ( lvdomElementFormatRec & v )
    {
        return (_height!=v._height || _y!=v._y || _width!=v._width || _x!=v._x ||
                _inner_width!=v._inner_width || _inner_x!=v._inner_x ||
                _inner_y!=v._inner_y || _baseline!=v._baseline ||
                _top_overflow!=v._top_overflow || _bottom_overflow!=v._bottom_overflow ||
                _lang_node_idx!=v._lang_node_idx || _listprop_node_idx!=v._listprop_node_idx ||
                _flags!=v._flags || _extra0!=v._extra0 ||
                _extra1!=v._extra1 || _extra2!=v._extra2 || _extra3!=v._extra3 ||
                _extra4!=v._extra4 || _extra5!=v._extra5 ||
                _available1!=v._available1
                );
    }
    // Get/Set
    int getX() const { return _x; }
    int getY() const { return _y; }
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    void getRect( lvRect & rc ) const
    {
        rc.left = _x;
        rc.top = _y;
        rc.right = _x + _width;
        rc.bottom = _y + _height;
    }
    void setX( int x ) { _x = x; }
    void setY( int y ) { _y = y; }
    void setWidth( int w ) { _width = w; }
    void setHeight( int h ) { _height = h; }
    // No real need for setters/getters for the other fields, RenderRectAccessor
    // extends this class and can access them freely.
};

/// calculate cache record hash
lUInt32 calcHash(css_style_rec_t & rec);
/// calculate font instance record hash
lUInt32 calcHash(font_ref_t & rec);
/// calculate cache record hash
inline lUInt32 calcHash(css_style_ref_t & rec) { return rec.isNull() ? 0 : calcHash( *rec.get() ); }

/// splits string like "Arial", Times New Roman, Courier;  into list
// returns number of characters processed
int splitPropertyValueList( const char * fontNames, lString8Collection & list );

/// joins list into string of comma separated quoted values
lString8 joinPropertyValueList( const lString8Collection & list );


#endif // __LV_STYLES_H_INCLUDED__
