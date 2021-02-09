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

/* bit position (in 'lUInt32[] important' and 'lUInt32[] importance' bitmaps)
 * of each css_style_rec_tag properties to flag its '!important' status */
enum css_style_rec_important_bit {
    imp_bit_display = 0,
    imp_bit_white_space,
    imp_bit_text_align,
    imp_bit_text_align_last,
    imp_bit_text_decoration,
    imp_bit_text_transform,
    imp_bit_vertical_align,
    imp_bit_font_family,
    imp_bit_font_name,
    imp_bit_font_size,
    imp_bit_font_style,
    imp_bit_font_weight,
    imp_bit_font_features,
    imp_bit_text_indent,
    imp_bit_line_height,
    imp_bit_width,
    imp_bit_height,
    imp_bit_min_width,
    imp_bit_min_height,
    imp_bit_max_width,
    imp_bit_max_height,
    imp_bit_margin_left,
    imp_bit_margin_right,
    imp_bit_margin_top,
    imp_bit_margin_bottom,
    imp_bit_padding_left,
    imp_bit_padding_right,
    imp_bit_padding_top,
    imp_bit_padding_bottom,
    imp_bit_color,
    imp_bit_background_color,
    imp_bit_letter_spacing,
    imp_bit_page_break_before,
    imp_bit_page_break_after,
    imp_bit_page_break_inside,
    imp_bit_hyphenate,
    imp_bit_list_style_type,
    imp_bit_list_style_position,
    imp_bit_border_style_top,
    imp_bit_border_style_bottom,
    imp_bit_border_style_right,
    imp_bit_border_style_left,
    imp_bit_border_width_top,
    imp_bit_border_width_right,
    imp_bit_border_width_bottom,
    imp_bit_border_width_left,
    imp_bit_border_color_top,
    imp_bit_border_color_right,
    imp_bit_border_color_bottom,
    imp_bit_border_color_left,
    imp_bit_background_image,
    imp_bit_background_repeat,
    imp_bit_background_position,
    imp_bit_background_size_h,
    imp_bit_background_size_v,
    imp_bit_border_collapse,
    imp_bit_border_spacing_h,
    imp_bit_border_spacing_v,
    imp_bit_orphans,
    imp_bit_widows,
    imp_bit_float,
    imp_bit_clear,
    imp_bit_direction,
    imp_bit_visibility,
    imp_bit_content,
    imp_bit_cr_hint
};
#define NB_IMP_BITS 66 // The number of lines in the enum above: KEEP IT UPDATED.

#define NB_IMP_SLOTS    ((NB_IMP_BITS-1)>>5)+1
// In lvstyles.cpp, we have hardcoded important[0] ... importance[1]
// So once NB_IMP_SLOTS becomes 4 when IMP_BIT_MAX > 96, add in lvstyles.cpp
// the needed important[3] and importance[3]. Let us know if we forget that:
#if (NB_IMP_SLOTS != 3)
    #error "NB_IMP_SLOTS != 3, some updates in lvstyles.cpp (and then here) are needed"
#endif

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
    lUInt32              important[NB_IMP_SLOTS];  // bitmap for !important (used only by LVCssDeclaration)
    lUInt32              importance[NB_IMP_SLOTS]; // bitmap for important bit's importance/origin
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
    css_length_t         min_width;
    css_length_t         min_height;
    css_length_t         max_width;
    css_length_t         max_height;
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
    css_background_position_value_t background_position;
    css_length_t background_size[2];//first width and second height
    css_border_collapse_value_t border_collapse;
    css_length_t border_spacing[2];//first horizontal and the second vertical spacing
    css_orphans_widows_value_t orphans;
    css_orphans_widows_value_t widows;
    css_float_t            float_; // "float" is a C++ keyword...
    css_clear_t            clear;
    css_direction_t        direction;
    css_visibility_t       visibility;
    lString32              content;
    css_length_t           cr_hint;
    // The following should only be used when applying stylesheets while in lvend.cpp setNodeStyle(),
    // and cleaned up there, before the style is cached and shared. They are not serialized.
    lInt8                flags; // bitmap of STYLE_REC_FLAG_*
    css_style_rec_t *    pseudo_elem_before_style;
    css_style_rec_t *    pseudo_elem_after_style;

    css_style_rec_tag()
    : refCount(0)
    , hash(0)
    , important{} // zero-initialization of all array slots
    , importance{}
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
    , min_width(css_val_unspecified, 0)
    , min_height(css_val_unspecified, 0)
    , max_width(css_val_unspecified, 0)
    , max_height(css_val_unspecified, 0)
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
    , background_position(css_background_p_none)
    , border_collapse(css_border_seperate)
    , orphans(css_orphans_widows_inherit)
    , widows(css_orphans_widows_inherit)
    , float_(css_f_none)
    , clear(css_c_none)
    , direction(css_dir_inherit)
    , visibility(css_v_inherit)
    , cr_hint(css_val_inherited, 0)
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
        background_size[0] = css_length_t(css_val_unspecified, 0);
        background_size[1] = css_length_t(css_val_unspecified, 0);
    }
    void AddRef() { refCount++; }
    int Release() { return --refCount; }
    int getRefCount() { return refCount; }
    bool serialize( SerialBuf & buf );
    bool deserialize( SerialBuf & buf );
    //  important bitmap management
    bool isImportant( css_style_rec_important_bit bit ) { return important[bit>>5] & (1<<(bit&0x1f)); }
    void setImportant( css_style_rec_important_bit bit ) { important[bit>>5] |= (1<<(bit&0x1f)); }
    // apply value to field if important bit not yet set, then set it if is_important
    template <typename T> inline void Apply( T value, T *field, css_style_rec_important_bit bit, lUInt8 is_important ) {
        int slot = bit>>5;        // 32 bits per LUint32 slot
        int sbit = 1<<(bit&0x1f); // bitmask for this single bit in its slot
        // is_important is 2 bits: (high_importance, is_important) (see lvstsheet.cpp)
        if (     !(important[slot] & sbit)     // important flag not previously set
              || (is_important == 0x3)  // coming value has '!important' and comes from some CSS parsed with higher_importance=true
              || (is_important == 0x1 && !(importance[slot] & sbit) ) // coming value has '!important' and comes from some CSS parsed
                                                               // with higher_importance=false, but previous value was not set from
                                                               // a !important that came from CSS parsed with higher_importance=true
           ) {
            *field = value; // apply
            if (is_important & 0x1) important[slot] |= sbit;   // update important flag
            if (is_important == 0x3) importance[slot] |= sbit; // update importance flag (!important comes from higher_importance CSS)
        }
    }
    // Similar to previous one, but logical-OR'ing values, for bitmaps (currently, only style->font_features and style->cr_hint)
    inline void ApplyAsBitmapOr( css_length_t value, css_length_t *field, css_style_rec_important_bit bit, lUInt8 is_important ) {
        int slot = bit>>5;        // 32 bits per LUint32 slot
        int sbit = 1<<(bit&0x1f); // bitmask for this bit in its slot
        if (     !(important[slot] & sbit)
              || (is_important == 0x3)
              || (is_important == 0x1 && !(importance[slot] & sbit) )
           ) {
            field->value |= value.value; // logical-or values
            field->type = value.type;    // use the one from value (always css_val_unspecified for font_features)
            if (is_important & 0x1) important[slot] |= sbit;
            if (is_important == 0x3) importance[slot] |= sbit;
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

    short _usable_left_overflow;   // Usable overflow for hanging punctuation
    short _usable_right_overflow;  // and glyphs negative side bearings

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

    // We're now at 16 32-bits ints. If needing new fields, add some padding:
    // Added for padding from 17 to 20 32-bits ints
    // int _available1; int _available2; int _available3;

public:
    lvdomElementFormatRec()
    : _x(0), _width(0), _y(0), _height(0)
    , _inner_width(0), _inner_x(0), _inner_y(0), _baseline(0)
    , _usable_left_overflow(0), _usable_right_overflow(0)
    , _top_overflow(0), _bottom_overflow(0)
    , _lang_node_idx(0) , _listprop_node_idx(0)
    , _flags(0), _extra0(0)
    , _extra1(0), _extra2(0), _extra3(0), _extra4(0), _extra5(0)
    {
    }
    ~lvdomElementFormatRec()
    {
    }
    void clear()
    {
        _x = _width = _y = _height = 0;
        _inner_width = _inner_x = _inner_y = _baseline = 0;
        _usable_left_overflow = _usable_right_overflow = 0;
        _top_overflow = _bottom_overflow = 0;
        _lang_node_idx = _listprop_node_idx = 0;
        _flags = _extra0 = 0;
        _extra1 = _extra2 = _extra3 = _extra4 = _extra5 = 0;
    }
    bool operator == ( lvdomElementFormatRec & v )
    {
        return (_height==v._height && _y==v._y && _width==v._width && _x==v._x &&
                _inner_width==v._inner_width && _inner_x==v._inner_x &&
                _inner_y==v._inner_y && _baseline==v._baseline &&
                _usable_left_overflow==v._usable_left_overflow && _usable_right_overflow==v._usable_right_overflow &&
                _top_overflow==v._top_overflow && _bottom_overflow==v._bottom_overflow &&
                _lang_node_idx==v._lang_node_idx && _listprop_node_idx==v._listprop_node_idx &&
                _flags==v._flags && _extra0==v._extra0 &&
                _extra1==v._extra1 && _extra2==v._extra2 && _extra3==v._extra3 &&
                _extra4==v._extra4 && _extra5==v._extra5
                );
    }
    bool operator != ( lvdomElementFormatRec & v )
    {
        return (_height!=v._height || _y!=v._y || _width!=v._width || _x!=v._x ||
                _inner_width!=v._inner_width || _inner_x!=v._inner_x ||
                _inner_y!=v._inner_y || _baseline!=v._baseline ||
                _usable_left_overflow!=v._usable_left_overflow || _usable_right_overflow!=v._usable_right_overflow ||
                _top_overflow!=v._top_overflow || _bottom_overflow!=v._bottom_overflow ||
                _lang_node_idx!=v._lang_node_idx || _listprop_node_idx!=v._listprop_node_idx ||
                _flags!=v._flags || _extra0!=v._extra0 ||
                _extra1!=v._extra1 || _extra2!=v._extra2 || _extra3!=v._extra3 ||
                _extra4!=v._extra4 || _extra5!=v._extra5
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
