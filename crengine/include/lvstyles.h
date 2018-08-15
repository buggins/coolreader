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

/* bit position (in 'lUInt64 important' bitmap) of each css_style_rec_tag
 * properties to flag its '!important' status */
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
    imp_bit_text_indent           = 1ULL << 12,
    imp_bit_line_height           = 1ULL << 13,
    imp_bit_width                 = 1ULL << 14,
    imp_bit_height                = 1ULL << 15,
    imp_bit_margin_left           = 1ULL << 16,
    imp_bit_margin_right          = 1ULL << 17,
    imp_bit_margin_top            = 1ULL << 18,
    imp_bit_margin_bottom         = 1ULL << 19,
    imp_bit_padding_left          = 1ULL << 20,
    imp_bit_padding_right         = 1ULL << 21,
    imp_bit_padding_top           = 1ULL << 22,
    imp_bit_padding_bottom        = 1ULL << 23,
    imp_bit_color                 = 1ULL << 24,
    imp_bit_background_color      = 1ULL << 25,
    imp_bit_letter_spacing        = 1ULL << 26,
    imp_bit_page_break_before     = 1ULL << 27,
    imp_bit_page_break_after      = 1ULL << 28,
    imp_bit_page_break_inside     = 1ULL << 29,
    imp_bit_hyphenate             = 1ULL << 30,
    imp_bit_list_style_type       = 1ULL << 31,
    imp_bit_list_style_position   = 1ULL << 32,
    imp_bit_border_style_top      = 1ULL << 33,
    imp_bit_border_style_bottom   = 1ULL << 34,
    imp_bit_border_style_right    = 1ULL << 35,
    imp_bit_border_style_left     = 1ULL << 36,
    imp_bit_border_width_top      = 1ULL << 37,
    imp_bit_border_width_right    = 1ULL << 38,
    imp_bit_border_width_bottom   = 1ULL << 39,
    imp_bit_border_width_left     = 1ULL << 40,
    imp_bit_border_color_top      = 1ULL << 41,
    imp_bit_border_color_right    = 1ULL << 42,
    imp_bit_border_color_bottom   = 1ULL << 43,
    imp_bit_border_color_left     = 1ULL << 44,
    imp_bit_background_image      = 1ULL << 45,
    imp_bit_background_repeat     = 1ULL << 46,
    imp_bit_background_attachment = 1ULL << 47,
    imp_bit_background_position   = 1ULL << 48,
    imp_bit_border_collapse       = 1ULL << 49,
    imp_bit_border_spacing_h      = 1ULL << 50,
    imp_bit_border_spacing_v      = 1ULL << 51
};

/**
    \brief Element style record.

    Contains set of style properties.
*/
typedef struct css_style_rec_tag {
    int                  refCount; // for reference counting
    lUInt32              hash; // cache calculated hash value here
    lUInt64              important; // bitmap for !important (used only by LVCssDeclaration)
                                    // we have currently below 52 css properties
                                    // lvstsheet knows about 67, which are mapped to these 52
                                    // update bits above if you add new properties below
    css_display_t        display;
    css_white_space_t    white_space;
    css_text_align_t     text_align;
    css_text_align_t     text_align_last;
    css_text_decoration_t text_decoration;
    css_text_transform_t text_transform;
    css_vertical_align_t vertical_align;
    css_font_family_t    font_family;
    lString8             font_name;
    css_length_t         font_size;
    css_font_style_t     font_style;
    css_font_weight_t    font_weight;
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
    css_style_rec_tag()
    : refCount(0)
    , hash(0)
    , important(0)
    , display( css_d_inline )
    , white_space(css_ws_inherit)
    , text_align(css_ta_inherit)
    , text_align_last(css_ta_inherit)
    , text_decoration (css_td_inherit)
    , text_transform (css_tt_inherit)
    , vertical_align(css_va_inherit)
    , font_family(css_ff_inherit)
    , font_size(css_val_inherited, 0)
    , font_style(css_fs_inherit)
    , font_weight(css_fw_inherit)
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
    {
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
    template <typename T> inline void Apply( T value, T *field, css_style_rec_important_bit bit, bool is_important ) {
        if ( !(important & bit) || is_important ) {
            // important flag not previously set, or coming value has '!important' and
            // should override previous important
            *field = value; // apply
            if (is_important) important |= bit; // update important flag
        }
    };
} css_style_rec_t;

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
    erm_block,         ///< render as block element (render as containing other elements)
    erm_final,         ///< final element: render the whole it's content as single render block
    erm_inline,        ///< inline element
    erm_mixed,         ///< block and inline elements are mixed: autobox inline portions of nodes; TODO
    erm_list_item,     ///< render as block element as list item
    erm_table,         ///< table element: render as table
    erm_table_row_group, ///< table row group
    erm_table_header_group, ///< table header group
    erm_table_footer_group, ///< table footer group
    erm_table_row,  ///< table row
    erm_table_column_group, ///< table column group
    erm_table_column, ///< table column
    erm_table_cell, ///< table cell
    erm_table_caption, ///< table caption
    erm_runin          ///< run-in
};

/// node format record
class lvdomElementFormatRec {
protected:
    int  _x;
    int  _width;
    int  _y;
    int  _height;
public:
    lvdomElementFormatRec()
    : _x(0), _width(0), _y(0), _height(0)//, _formatter(NULL)
    {
    }
    ~lvdomElementFormatRec()
    {
    }
    void clear()
    {
        _x = _width = _y = _height = 0;
    }
    bool operator == ( lvdomElementFormatRec & v )
    {
        return (_height==v._height && _y==v._y && _width==v._width && _x==v._x );
    }
    bool operator != ( lvdomElementFormatRec & v )
    {
        return (_height!=v._height || _y!=v._y || _width!=v._width || _x!=v._x );
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
