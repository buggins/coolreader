/*******************************************************

   CoolReader Engine

   lvstsheet.cpp:  style sheet implementation

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*******************************************************/

#include "../include/lvstsheet.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvstream.h"

// define to dump all tokens
//#define DUMP_CSS_PARSING

enum css_decl_code {
    cssd_unknown,
    cssd_display,
    cssd_white_space,
    cssd_text_align,
    cssd_text_align_last,
    cssd_text_decoration,
    cssd_hyphenate, // hyphenate
    cssd_hyphenate2, // -webkit-hyphens
    cssd_hyphenate3, // adobe-hyphenate
    cssd_hyphenate4, // adobe-text-layout
    cssd_color,
    cssd_border_top_color,
    cssd_border_right_color,
    cssd_border_bottom_color,
    cssd_border_left_color,
    cssd_background_color,
    cssd_vertical_align,
    cssd_font_family, // id families like serif, sans-serif
    cssd_font_names,   // string font name like Arial, Courier
    cssd_font_size,
    cssd_font_style,
    cssd_font_weight,
    cssd_text_indent,
    cssd_line_height,
    cssd_letter_spacing,
    cssd_width,
    cssd_height,
    cssd_margin_left,
    cssd_margin_right,
    cssd_margin_top,
    cssd_margin_bottom,
    cssd_margin,
    cssd_padding_left,
    cssd_padding_right,
    cssd_padding_top,
    cssd_padding_bottom,
    cssd_padding,
    cssd_page_break_before,
    cssd_page_break_after,
    cssd_page_break_inside,
    cssd_list_style,
    cssd_list_style_type,
    cssd_list_style_position,
    cssd_list_style_image,
    cssd_border_top_style,
    cssd_border_top_width,
    cssd_border_right_style,
    cssd_border_right_width,
    cssd_border_bottom_style,
    cssd_border_bottom_width,
    cssd_border_left_style,
    cssd_border_left_width,
    cssd_border_style,
    cssd_border_width,
    cssd_border_color,
    cssd_border,
    cssd_border_top,
    cssd_border_right,
    cssd_border_bottom,
    cssd_border_left,
    cssd_background,
    cssd_background_image,
    cssd_background_repeat,
    cssd_background_attachment,
    cssd_background_position,
    cssd_border_collapse,
    cssd_border_spacing,
    cssd_stop
};

static const char * css_decl_name[] = {
    "",
    "display",
    "white-space",
    "text-align",
    "text-align-last",
    "text-decoration",
    "hyphenate",
    "-webkit-hyphens",
    "adobe-hyphenate",
    "adobe-text-layout",
    "color",
    "border-top-color",
    "border-right-color",
    "border-bottom-color",
    "border-left-color",
    "background-color",
    "vertical-align",
    "font-family",
    "$dummy-for-font-names$",
    "font-size",
    "font-style",
    "font-weight",
    "text-indent",
    "line-height",
    "letter-spacing",
    "width",
    "height",
    "margin-left",
    "margin-right",
    "margin-top",
    "margin-bottom",
    "margin",
    "padding-left",
    "padding-right",
    "padding-top",
    "padding-bottom",
    "padding",
    "page-break-before",
    "page-break-after",
    "page-break-inside",
    "list-style",
    "list-style-type",
    "list-style-position",
    "list-style-image",
    "border-top-style",
    "border-top-width",
    "border-right-style",
    "border-right-width",
    "border-bottom-style",
    "border-bottom-width",
    "border-left-style",
    "border-left-width",
    "border-style",
    "border-width",
    "border-color",
    "border",
    "border-top",
    "border-right",
    "border-bottom",
    "border-left",
    "background",
    "background-image",
    "background-repeat",
    "background-attachment",
    "background-position",
    "border-collapse",
    "border-spacing",
    NULL
};

inline bool css_is_alpha( char ch )
{
    return ( (ch>='A' && ch<='Z') || ( ch>='a' && ch<='z' ) || (ch=='-') || (ch=='_') );
}

inline bool css_is_alnum( char ch )
{
    return ( css_is_alpha(ch) || ( ch>='0' && ch<='9' ) );
}

static int substr_compare( const char * sub, const char * & str )
{
    int j;
    for ( j=0; sub[j] == str[j] && sub[j] && str[j]; j++)
        ;
    if (!sub[j])
    {
        //bool last_alpha = css_is_alpha( sub[j-1] );
        //bool next_alnum = css_is_alnum( str[j] );
        if ( !css_is_alpha( sub[j-1] ) || !css_is_alnum( str[j] ) )
        {
            str+=j;
            return j;
        }
    }
    return 0;
}

inline char toLower( char c )
{
    if ( c>='A' && c<='Z' )
        return c - 'A' + 'a';
    return c;
}

static int substr_icompare( const char * sub, const char * & str )
{
    int j;
    for ( j=0; toLower(sub[j]) == toLower(str[j]) && sub[j] && str[j]; j++)
        ;
    if (!sub[j])
    {
        //bool last_alpha = css_is_alpha( sub[j-1] );
        //bool next_alnum = css_is_alnum( str[j] );
        if ( !css_is_alpha( sub[j-1] ) || !css_is_alnum( str[j] ) )
        {
            str+=j;
            return j;
        }
    }
    return 0;
}

static bool skip_spaces( const char * & str )
{
    const char * oldpos = str;
    for (;;) {
        while (*str==' ' || *str=='\t' || *str=='\n' || *str == '\r')
            str++;
        if ( *str=='/' && str[1]=='*' ) {
            // comment found
            while ( *str && str[1] && (str[0]!='*' || str[1]!='/') )
                str++;
            if ( *str=='*' && str[1]=='/' )
                str +=2;
        }
        while (*str==' ' || *str=='\t' || *str=='\n' || *str == '\r')
            str++;
        if (oldpos == str)
            break;
        if (*str == 0)
            return false;
        oldpos = str;
    }
    return *str != 0;
}

static css_decl_code parse_property_name( const char * & res )
{
    const char * str = res;
    for (int i=1; css_decl_name[i]; i++)
    {
        if (substr_compare( css_decl_name[i], str ))
        {
            // found!
            skip_spaces(str);
            if ( substr_compare( ":", str )) {
#ifdef DUMP_CSS_PARSING
                CRLog::trace("property name: %s", lString8(res, str-res).c_str() );
#endif
                skip_spaces(str);
                res = str;
                return (css_decl_code)i;
            }
        }
    }
    return cssd_unknown;
}

static int parse_name( const char * & str, const char * * names, int def_value )
{
    for (int i=0; names[i]; i++)
    {
        if (substr_compare( names[i], str ))
        {
            // found!
            return i;
        }
    }
    return def_value;
}

static bool next_property( const char * & str )
{
    while (*str && *str !=';' && *str!='}')
        str++;
    if (*str == ';')
        str++;
    return skip_spaces( str );
}

static bool parse_number_value( const char * & str, css_length_t & value, bool is_font_size=false )
{
    value.type = css_val_unspecified;
    skip_spaces( str );
    if ( substr_compare( "inherited", str ) )
    {
        value.type = css_val_inherited;
        value.value = 0;
        return true;
    }
    if ( is_font_size ) {
        // Approximate the (usually uneven) gaps between named sizes.
        if ( substr_compare( "smaller", str ) ) {
            value.type = css_val_percent;
            value.value = 80;
            return true;
        }
        else if ( substr_compare( "larger", str ) ) {
            value.type = css_val_percent;
            value.value = 125;
            return true;
        }
    }
    int n = 0;
    if (*str != '.') {
        if (*str<'0' || *str>'9') {
            return false; // not a number
        }
        while (*str>='0' && *str<='9') {
            n = n*10 + (*str - '0');
            str++;
        }
    }
    int frac = 0;
    int frac_div = 1;
    if (*str == '.') {
        str++;
        while (*str>='0' && *str<='9')
        {
            frac = frac*10 + (*str - '0');
            frac_div *= 10;
            str++;
        }
    }
    if ( substr_compare( "em", str ) )
        value.type = css_val_em;
    else if ( substr_compare( "pt", str ) )
        value.type = css_val_pt;
    else if ( substr_compare( "ex", str ) )
        value.type = css_val_ex;
    else if ( substr_compare( "px", str ) )
        value.type = css_val_px;
    else if ( substr_compare( "in", str ) )
        value.type = css_val_in;
    else if ( substr_compare( "cm", str ) )
        value.type = css_val_cm;
    else if ( substr_compare( "mm", str ) )
        value.type = css_val_mm;
    else if ( substr_compare( "pc", str ) )
        value.type = css_val_pc;
    else if ( substr_compare( "%", str ) )
        value.type = css_val_percent;
    else if (n == 0 && frac == 0)
        value.type = css_val_px;
    else
        return false;
    if ( value.type == css_val_px || value.type == css_val_percent )
        value.value = n;                               // normal
    else
        value.value = n * 256 + 256 * frac / frac_div; // *256
    return true;
}

struct standard_color_t
{
    const char * name;
    lUInt32 color;
};

standard_color_t standard_color_table[] = {
    {"aliceblue",0xf0f8ff},
    {"antiquewhite",0xfaebd7},
    {"aqua",0x00ffff},
    {"aquamarine",0x7fffd4},
    {"azure",0xf0ffff},
    {"beige",0xf5f5dc},
    {"bisque",0xffe4c4},
    {"black",0x000000},
    {"blanchedalmond",0xffebcd},
    {"blue",0x0000ff},
    {"blueviolet",0x8a2be2},
    {"brown",0xa52a2a},
    {"burlywood",0xdeb887},
    {"cadetblue",0x5f9ea0},
    {"chartreuse",0x7fff00},
    {"chocolate",0xd2691e},
    {"coral",0xff7f50},
    {"cornflowerblue",0x6495ed},
    {"cornsilk",0xfff8dc},
    {"crimson",0xdc143c},
    {"cyan",0x00ffff},
    {"darkblue",0x00008b},
    {"darkcyan",0x008b8b},
    {"darkgoldenrod",0xb8860b},
    {"darkgray",0xa9a9a9},
    {"darkgreen",0x006400},
    {"darkkhaki",0xbdb76b},
    {"darkmagenta",0x8b008b},
    {"darkolivegreen",0x556b2f},
    {"darkorange",0xff8c00},
    {"darkorchid",0x9932cc},
    {"darkred",0x8b0000},
    {"darksalmon",0xe9967a},
    {"darkseagreen",0x8fbc8f},
    {"darkslateblue",0x483d8b},
    {"darkslategray",0x2f4f4f},
    {"darkturquoise",0x00ced1},
    {"darkviolet",0x9400d3},
    {"deeppink",0xff1493},
    {"deepskyblue",0x00bfff},
    {"dimgray",0x696969},
    {"dodgerblue",0x1e90ff},
    {"firebrick",0xb22222},
    {"floralwhite",0xfffaf0},
    {"forestgreen",0x228b22},
    {"fuchsia",0xff00ff},
    {"gainsboro",0xdcdcdc},
    {"ghostwhite",0xf8f8ff},
    {"gold",0xffd700},
    {"goldenrod",0xdaa520},
    {"gray",0x808080},
    {"green",0x008000},
    {"greenyellow",0xadff2f},
    {"honeydew",0xf0fff0},
    {"hotpink",0xff69b4},
    {"indianred",0xcd5c5c},
    {"indigo",0x4b0082},
    {"ivory",0xfffff0},
    {"khaki",0xf0e68c},
    {"lavender",0xe6e6fa},
    {"lavenderblush",0xfff0f5},
    {"lawngreen",0x7cfc00},
    {"lemonchiffon",0xfffacd},
    {"lightblue",0xadd8e6},
    {"lightcoral",0xf08080},
    {"lightcyan",0xe0ffff},
    {"lightgoldenrodyellow",0xfafad2},
    {"lightgray",0xd3d3d3},
    {"lightgreen",0x90ee90},
    {"lightpink",0xffb6c1},
    {"lightsalmon",0xffa07a},
    {"lightseagreen",0x20b2aa},
    {"lightskyblue",0x87cefa},
    {"lightslategray",0x778899},
    {"lightsteelblue",0xb0c4de},
    {"lightyellow",0xffffe0},
    {"lime",0x00ff00},
    {"limegreen",0x32cd32},
    {"linen",0xfaf0e6},
    {"magenta",0xff00ff},
    {"maroon",0x800000},
    {"mediumaquamarine",0x66cdaa},
    {"mediumblue",0x0000cd},
    {"mediumorchid",0xba55d3},
    {"mediumpurple",0x9370db},
    {"mediumseagreen",0x3cb371},
    {"mediumslateblue",0x7b68ee},
    {"mediumspringgreen",0x00fa9a},
    {"mediumturquoise",0x48d1cc},
    {"mediumvioletred",0xc71585},
    {"midnightblue",0x191970},
    {"mintcream",0xf5fffa},
    {"mistyrose",0xffe4e1},
    {"moccasin",0xffe4b5},
    {"navajowhite",0xffdead},
    {"navy",0x000080},
    {"oldlace",0xfdf5e6},
    {"olive",0x808000},
    {"olivedrab",0x6b8e23},
    {"orange",0xffa500},
    {"orangered",0xff4500},
    {"orchid",0xda70d6},
    {"palegoldenrod",0xeee8aa},
    {"palegreen",0x98fb98},
    {"paleturquoise",0xafeeee},
    {"palevioletred",0xdb7093},
    {"papayawhip",0xffefd5},
    {"peachpuff",0xffdab9},
    {"peru",0xcd853f},
    {"pink",0xffc0cb},
    {"plum",0xdda0dd},
    {"powderblue",0xb0e0e6},
    {"purple",0x800080},
    {"rebeccapurple",0x663399},
    {"red",0xff0000},
    {"rosybrown",0xbc8f8f},
    {"royalblue",0x4169e1},
    {"saddlebrown",0x8b4513},
    {"salmon",0xfa8072},
    {"sandybrown",0xf4a460},
    {"seagreen",0x2e8b57},
    {"seashell",0xfff5ee},
    {"sienna",0xa0522d},
    {"silver",0xc0c0c0},
    {"skyblue",0x87ceeb},
    {"slateblue",0x6a5acd},
    {"slategray",0x708090},
    {"snow",0xfffafa},
    {"springgreen",0x00ff7f},
    {"steelblue",0x4682b4},
    {"tan",0xd2b48c},
    {"teal",0x008080},
    {"thistle",0xd8bfd8},
    {"tomato",0xff6347},
    {"turquoise",0x40e0d0},
    {"violet",0xee82ee},
    {"wheat",0xf5deb3},
    {"white",0xffffff},
    {"whitesmoke",0xf5f5f5},
    {"yellow",0xffff00},
    {"yellowgreen",0x9acd32},
    {NULL, 0}
};

static int hexDigit( char c )
{
    if ( c >= '0' && c <= '9' )
        return c-'0';
    if ( c >= 'A' && c <= 'F' )
        return c - 'A' + 10;
    if ( c >= 'a' && c <= 'f' )
        return c - 'a' + 10;
    return -1;
}

bool parse_color_value( const char * & str, css_length_t & value )
{
    value.type = css_val_unspecified;
    skip_spaces( str );
    if ( substr_compare( "inherited", str ) )
    {
        value.type = css_val_inherited;
        value.value = 0;
        return true;
    }
    if ( substr_compare( "none", str ) )
    {
        value.type = css_val_unspecified;
        value.value = 0;
        return true;
    }
    if (*str=='#') {
        // #rgb or #rrggbb colors
        str++;
        int nDigits = 0;
        for ( ; hexDigit(str[nDigits])>=0; nDigits++ )
            ;
        if ( nDigits==3 ) {
            int r = hexDigit( *str++ );
            int g = hexDigit( *str++ );
            int b = hexDigit( *str++ );
            value.type = css_val_color;
            value.value = (((r + r*16) * 256) | (g + g*16)) * 256 | (b + b*16);
            return true;
        } else if ( nDigits==6 ) {
            int r = hexDigit( *str++ ) * 16;
            r += hexDigit( *str++ );
            int g = hexDigit( *str++ ) * 16;
            g += hexDigit( *str++ );
            int b = hexDigit( *str++ ) * 16;
            b += hexDigit( *str++ );
            value.type = css_val_color;
            value.value = ((r * 256) | g) * 256 | b;
            return true;
        } else
            return false;
    }
    for ( int i=0; standard_color_table[i].name != NULL; i++ ) {
        if ( substr_icompare( standard_color_table[i].name, str ) ) {
            value.type = css_val_color;
            value.value = standard_color_table[i].color;
            return true;
        }
    }
    return false;
}

static const char * css_d_names[] = 
{
    "inherit",
    "inline",
    "block",
    "list-item", 
    "run-in", 
    "compact", 
    "marker", 
    "table", 
    "inline-table", 
    "table-row-group", 
    "table-header-group", 
    "table-footer-group", 
    "table-row", 
    "table-column-group", 
    "table-column", 
    "table-cell", 
    "table-caption", 
    "none", 
    NULL
};

static const char * css_ws_names[] = 
{
    "inherit",
    "normal",
    "pre",
    "nowrap",
    NULL
};

static const char * css_ta_names[] = 
{
    "inherit",
    "left",
    "right",
    "center",
    "justify",
    NULL
};

static const char * css_td_names[] = 
{
    "inherit",
    "none",
    "underline",
    "overline",
    "line-through",
    "blink",
    NULL
};

static const char * css_hyph_names[] = 
{
    "inherit",
    "none",
    "auto",
    NULL
};

static const char * css_hyph_names2[] =
{
    "inherit",
    "optimizeSpeed",
    "optimizeQuality",
    NULL
};

static const char * css_hyph_names3[] =
{
    "inherit",
    "none",
    "explicit",
    NULL
};

static const char * css_pb_names[] =
{
    "inherit",
    "auto",
    "always",
    "avoid",
    "left",
    "right",
    NULL
};

static const char * css_fs_names[] = 
{
    "inherit",
    "normal",
    "italic",
    "oblique",
    NULL
};

static const char * css_fw_names[] = 
{
    "inherit",
    "normal",
    "bold",
    "bolder",
    "lighter",
    "100",
    "200",
    "300",
    "400",
    "500",
    "600",
    "700",
    "800",
    "900",
    NULL
};
static const char * css_va_names[] = 
{
    "inherit",
    "baseline", 
    "sub",
    "super",
    "top",
    "text-top",
    "middle",
    "bottom",
    "text-bottom",
    NULL
};

static const char * css_ti_attribute_names[] =
{
    "hanging",
    NULL
};

static const char * css_ff_names[] =
{
    "inherit",
    "serif",
    "sans-serif",
    "cursive",
    "fantasy",
    "monospace",
    NULL
};

static const char * css_lst_names[] =
{
    "inherit",
    "disc",
    "circle",
    "square",
    "decimal",
    "lower-roman",
    "upper-roman",
    "lower-alpha",
    "upper-alpha",
    "none",
    NULL
};

static const char * css_lsp_names[] =
{
    "inherit",
    "inside",
    "outside",
    NULL
};
///border style names
static const char * css_bst_names[]={
  "solid",
  "dotted",
  "dashed",
  "double",
  "groove",
  "ridge",
  "inset",
  "outset",
  "none",
  NULL
};
///border width value names
static const char * css_bw_names[]={
        "thin",
        "medium",
        "thick",
        "initial",
        "inherit",
        NULL
};

//background repeat names
static const char * css_bg_repeat_names[]={
        "repeat",
        "repeat-x",
        "repeat-y",
        "no-repeat",
        "initial",
        "inherit",
        NULL
};
//background attachment names
static const char * css_bg_attachment_names[]={
        "scroll",
        "fixed",
        "local",
        "initial",
        "inherit",
        NULL
};
//background position names
static const char * css_bg_position_names[]={
        "left top",
        "left center",
        "left bottom",
        "right top",
        "right center",
        "right bottom",
        "center top",
        "center center",
        "center bottom",
        "top left",
        "center left",
        "bottom left",
        "top right",
        "center right",
        "bottom right",
        "top center",
        "center center",
        "bottom center",
        "center",
        "left",
        "right",
        "top",
        "bottom",
        "initial",
        "inherit",
        NULL
};

//border-collpase names
static const char * css_bc_names[]={
        "seperate",
        "collapse",
        "initial",
        "inherit",
        NULL
};

bool LVCssDeclaration::parse( const char * &decl )
{
    #define MAX_DECL_SIZE 512
    int buf[MAX_DECL_SIZE];
    int buf_pos = 0;

    if ( !decl )
        return false;

    skip_spaces( decl );
    if (*decl!='{')
        return false;
    decl++;
    while (*decl && *decl!='}') {
        skip_spaces( decl );
        css_decl_code prop_code = parse_property_name( decl );
        skip_spaces( decl );
        lString8 strValue;
        if (prop_code != cssd_unknown)
        {
            // parsed ok
            int n = -1;
            switch ( prop_code )
            {
            case cssd_display:
                n = parse_name( decl, css_d_names, -1 );
                break;
            case cssd_white_space:
                n = parse_name( decl, css_ws_names, -1 );
                break;
            case cssd_text_align:
                n = parse_name( decl, css_ta_names, -1 );
                break;
            case cssd_text_align_last:
                n = parse_name( decl, css_ta_names, -1 );
                break;
            case cssd_text_decoration:
                n = parse_name( decl, css_td_names, -1 );
                break;
            case cssd_hyphenate:
            case cssd_hyphenate2:
            case cssd_hyphenate3:
            case cssd_hyphenate4:
            	prop_code = cssd_hyphenate;
                n = parse_name( decl, css_hyph_names, -1 );
                if ( n==-1 )
                    n = parse_name( decl, css_hyph_names2, -1 );
                if ( n==-1 )
                    n = parse_name( decl, css_hyph_names3, -1 );
                break;
            case cssd_page_break_before:
                n = parse_name( decl, css_pb_names, -1 );
                break;
            case cssd_page_break_inside:
                n = parse_name( decl, css_pb_names, -1 );
                break;
            case cssd_page_break_after:
                n = parse_name( decl, css_pb_names, -1 );
                break;
            case cssd_list_style_type:
                n = parse_name( decl, css_lst_names, -1 );
                break;
            case cssd_list_style_position:
                n = parse_name( decl, css_lsp_names, -1 );
                break;
            case cssd_vertical_align:
                n = parse_name( decl, css_va_names, -1 );
                break;
            case cssd_font_family:
                {
                    lString8Collection list;
                    int processed = splitPropertyValueList( decl, list );
                    decl += processed;
                    n = -1;
                    if (list.length())
                    {
                        for (int i=list.length()-1; i>=0; i--)
                        {
                            const char * name = list[i].c_str();
                            int nn = parse_name( name, css_ff_names, -1 );
                            if (n==-1 && nn!=-1)
                            {
                                n = nn;
                            }
                            if (nn!=-1)
                            {
                                // remove family name from font list
                                list.erase( i, 1 );
                            }
                        }
                        strValue = joinPropertyValueList( list );
                    }
                    // default to serif generic font-family
                    if (n == -1) n = 1;
                }
                break;
            case cssd_font_style:
                n = parse_name( decl, css_fs_names, -1 );
                break;
            case cssd_font_weight:
                n = parse_name( decl, css_fw_names, -1 );
                break;
            case cssd_text_indent:
                {
                    // read length
                    css_length_t len;
                    bool negative = false;
                    if ( *decl == '-' ) {
                        decl++;
                        negative = true;
                    }
                    if ( parse_number_value( decl, len ) )
                    {
                        // read optional "hanging" flag
                        skip_spaces( decl );
                        int attr = parse_name( decl, css_ti_attribute_names, -1 );
                        if ( attr==0 || negative ) {
                            len.value = -len.value;
                        }
                        // save result
                        buf[ buf_pos++ ] = prop_code;
                        buf[ buf_pos++ ] = len.type;
                        buf[ buf_pos++ ] = len.value;
                    }
                }
                break;
            case cssd_line_height:
            case cssd_letter_spacing:
            case cssd_font_size:
            case cssd_width:
            case cssd_height:
            case cssd_margin_left:
            case cssd_margin_right:
            case cssd_margin_top:
            case cssd_margin_bottom:
            case cssd_padding_left:
            case cssd_padding_right:
            case cssd_padding_top:
            case cssd_border_bottom_width:
            case cssd_border_top_width:
            case cssd_border_left_width:
            case cssd_border_right_width:
            {
                if (prop_code==cssd_border_bottom_width||prop_code==cssd_border_top_width||
                        prop_code==cssd_border_left_width||prop_code==cssd_border_right_width){
                const char*str=decl;
                int n1=parse_name(str,css_bw_names,-1);
                if (n1!=-1) {
                    buf[buf_pos++] = prop_code;
                    switch (n1) {
                        case 0:
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 1;
                            break;
                        case 1:
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 3;
                            break;
                        case 2:
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 5;
                            break;
                        case 3:
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 3;
                            break;
                        case 4:
                            buf[buf_pos++] = css_val_inherited;
                            buf[buf_pos++] = 0;
                            break;
                        default:break;
                    }
                    break;
                }
            }
            }//continue if value is number
            case cssd_padding_bottom:
                {
                    css_length_t len;
                    if ( parse_number_value( decl, len, prop_code==cssd_font_size ) )
                    {
                        buf[ buf_pos++ ] = prop_code;
                        buf[ buf_pos++ ] = len.type;
                        buf[ buf_pos++ ] = len.value;
                    }
                }
                break;
            case cssd_margin:
            case cssd_border_width:
            {
                if (prop_code==cssd_border_width){
                const char*str=decl;
                int n1=parse_name(str,css_bw_names,-1);
                if (n1!=-1) {
                    buf[buf_pos++] = prop_code;
                    switch (n1) {
                        case 0:
                            for (int i = 0; i < 4; ++i)
                            {
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 1;
                            }
                            break;
                        case 1:
                            for (int i = 0; i < 4; ++i)
                            {
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 3;
                            }
                            break;
                        case 2:
                            for (int i = 0; i < 4; ++i)
                            {
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 5;
                            }
                            break;
                        case 3:
                            for (int i = 0; i < 4; ++i)
                            {
                            buf[buf_pos++] = css_val_px;
                            buf[buf_pos++] = 3;
                            }
                            break;
                        case 4:
                            for (int i = 0; i < 4; ++i)
                            {
                            buf[buf_pos++] = css_val_inherited;
                            buf[buf_pos++] = 0;
                            break;
                            }
                        default:break;
                    }
                    break;
                }
                }
            }
            case cssd_padding:
		{
		    css_length_t len[4];
		    int i;
		    for (i = 0; i < 4; ++i)
			if (!parse_number_value( decl, len[i]))
			    break;
		    if (i)
		    {
			switch (i)
			{
			    case 1: len[1] = len[0]; /* fall through */
			    case 2: len[2] = len[0]; /* fall through */
			    case 3: len[3] = len[1];
			}
			buf[ buf_pos++ ] = prop_code;
			for (i = 0; i < 4; ++i)
			{
			    buf[ buf_pos++ ] = len[i].type;
			    buf[ buf_pos++ ] = len[i].value;
			}
		    }
		}
		break;
            case cssd_color:
            case cssd_background_color:
            case cssd_border_top_color:
            case cssd_border_right_color:
            case cssd_border_bottom_color:
            case cssd_border_left_color:
            {
                css_length_t len;
                if ( parse_color_value( decl, len ) )
                {
                    buf[ buf_pos++ ] = prop_code;
                    buf[ buf_pos++ ] = len.type;
                    buf[ buf_pos++ ] = len.value;
                }
            }
                break;
            case cssd_border_color:
            {
                css_length_t len[4];
                int i;
                for (i = 0; i < 4; ++i)
                    if (!parse_color_value( decl, len[i]))
                        break;
                if (i)
                {
                    switch (i)
                    {
                        case 1: len[1] = len[0]; /* fall through */
                        case 2: len[2] = len[0]; /* fall through */
                        case 3: len[3] = len[1];
                    }
                    buf[ buf_pos++ ] = prop_code;
                    for (i = 0; i < 4; ++i)
                    {
                        buf[ buf_pos++ ] = len[i].type;
                        buf[ buf_pos++ ] = len[i].value;
                    }
                }
            }
                break;
            case cssd_border_top_style:
            case cssd_border_right_style:
            case cssd_border_bottom_style:
            case cssd_border_left_style:
            {
                n = parse_name( decl, css_bst_names, -1 );
                break;
            }
            case cssd_border_style: {
                int n1=-1,n2=-1,n3=-1,n4=-1,sum=0;
                n1 = parse_name(decl, css_bst_names, -1);
                skip_spaces(decl);
                if (n1!=-1) {
                    sum=1;
                    n2 = parse_name(decl, css_bst_names, -1);
                    skip_spaces(decl);
                    if (n2!=-1) {
                        sum=2;
                        n3 = parse_name(decl, css_bst_names, -1);
                        skip_spaces(decl);
                        if (n3!=-1) {
                            sum=3;
                            n4 = parse_name(decl, css_bst_names, -1);
                            skip_spaces(decl);
                            if (n4!=-1) sum=4;
                        }
                        }
                    }
                switch (sum) {
                    case 1:
                    {
                        buf[buf_pos++] = prop_code;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n1;
                    }
                        break;
                    case 2:
                    {
                        buf[buf_pos++] = prop_code;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n2;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n2;
                    }
                    break;
                    case 3:
                    {
                        buf[buf_pos++] = prop_code;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n2;
                        buf[buf_pos++] = n3;
                        buf[buf_pos++] = n2;
                    }
                    break;
                    case 4:
                    {
                        buf[buf_pos++] = prop_code;
                        buf[buf_pos++] = n1;
                        buf[buf_pos++] = n2;
                        buf[buf_pos++] = n3;
                        buf[buf_pos++] = n4;
                    }
                    break;
                    default:break;
                }
            }
                break;
            case cssd_border:
            case cssd_border_top:
            case cssd_border_right:
            case cssd_border_bottom:
            case cssd_border_left:
                {
                    css_length_t width,color;
                    int n1=-1,n2=-1,n3=-1;
                    lString8 tmp = lString8(decl);
                    lString16 tmp1=lString16(tmp.c_str());
                    tmp1.trimDoubleSpaces(false,false,true);//remove double spaces
                    tmp=UnicodeToLocal(tmp1.c_str());
                    const char *str1=tmp.c_str();
                    if(!parse_color_value(str1,color))
                    {   str1=tmp.c_str();
                        if(!parse_number_value(str1,width)&&parse_name(str1,css_bw_names,-1)==-1) {
                            str1=tmp.c_str();
                            n1 = parse_name(str1, css_bst_names, -1);
                            skip_spaces(str1);
                            if (n1!=-1){
                                const char * str2=str1;
                                const char * str3=str1;
                                if(!parse_color_value(str2,color)){
                                    str2=str3;
                                    if(parse_number_value(str2,width)) n3=1;
                                    else{
                                        int num=parse_name(str2,css_bw_names,-1);
                                        if (num!=-1){
                                            n3=1;
                                            width.type=css_val_px;
                                            switch (num){
                                                case 0:
                                                    width.value=1;
                                                    break;
                                                case 1:
                                                    width.value=3;
                                                    break;
                                                case 2:
                                                    width.value=5;
                                                    break;
                                                case 3:
                                                    width.value=3;
                                                    break;
                                                case 4:
                                                    width.type=css_val_inherited;
                                                    width.value=0;
                                                    break;
                                                default:break;
                                            }
                                        }
                                    }
                                    skip_spaces(str2);
                                    if(parse_color_value(str2,color)) n2=1;
                                }
                                else {
                                    n2=1;
                                    skip_spaces(str2);
                                    if(parse_number_value(str2,width)) n3=1;
                                    else{
                                        int num=parse_name(str2,css_bw_names,-1);
                                        if (num!=-1){
                                            n3=1;
                                            width.type=css_val_px;
                                            switch (num){
                                                case 0:
                                                    width.value=1;
                                                    break;
                                                case 1:
                                                    width.value=3;
                                                    break;
                                                case 2:
                                                    width.value=5;
                                                    break;
                                                case 3:
                                                    width.value=3;
                                                    break;
                                                case 4:
                                                    width.type=css_val_inherited;
                                                    width.value=0;
                                                    break;
                                                default:break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else{
                            if (width.type==css_val_unspecified){
                                str1=tmp.c_str();
                                int num=parse_name(str1,css_bw_names,-1);
                                if (num!=-1){
                                    n3=1;
                                    width.type=css_val_px;
                                    switch (num){
                                        case 0:
                                            width.value=1;
                                            break;
                                        case 1:
                                            width.value=3;
                                            break;
                                        case 2:
                                            width.value=5;
                                            break;
                                        case 3:
                                            width.value=3;
                                            break;
                                        case 4:
                                            width.type=css_val_inherited;
                                            width.value=0;
                                            break;
                                        default:break;
                                    }
                                }
                            }else n3=1;
                            skip_spaces(str1);
                            const char * str2=str1;
                            if(!parse_color_value(str2,color)){
                                str2=str1;
                                skip_spaces(str2);
                                n1 = parse_name(str1, css_bst_names, -1);
                                if(parse_color_value(str1,color)) n2=1;
                            }
                            else{
                                n2=1;
                                skip_spaces(str2);
                                n1 = parse_name(str2, css_bst_names, -1);
                            }
                        }
                    }
                    else
                    {
                        n2=1;
                        skip_spaces(str1);
                        const char * str2=str1;
                        if(!parse_number_value(str1,width)&&parse_name(str1,css_bw_names,-1)==-1) {
                            str1=str2;
                            n1 = parse_name(str1, css_bst_names, -1);
                            skip_spaces(str1);
                            if(parse_number_value(str1,width)) n3=1;
                            else {
                                int num=parse_name(str1,css_bw_names,-1);
                                if (num!=-1){
                                    n3=1;
                                    width.type=css_val_px;
                                    switch (num){
                                        case 0:
                                            width.value=1;
                                            break;
                                        case 1:
                                            width.value=3;
                                            break;
                                        case 2:
                                            width.value=5;
                                            break;
                                        case 3:
                                            width.value=3;
                                            break;
                                        case 4:
                                            width.type=css_val_inherited;
                                            width.value=0;
                                            break;
                                        default:break;
                                    }
                                }
                            }
                        }
                        else{
                            if (width.type==css_val_unspecified){
                                str1=str2;
                                int num=parse_name(str1,css_bw_names,-1);
                                if (num!=-1){
                                    n3=1;
                                    width.type=css_val_px;
                                    switch (num){
                                        case 0:
                                            width.value=1;
                                            break;
                                        case 1:
                                            width.value=3;
                                            break;
                                        case 2:
                                            width.value=5;
                                            break;
                                        case 3:
                                            width.value=3;
                                            break;
                                        case 4:
                                            width.type=css_val_inherited;
                                            width.value=0;
                                            break;
                                        default:break;
                                    }
                                }
                            }else n3=1;
                            skip_spaces(str1);
                            n1 = parse_name(str1, css_bst_names, -1);
                        }
                    }

                    if (prop_code==cssd_border)
                    {
                        if (n1 != -1)
                        {
                            buf[buf_pos++] = cssd_border_top_style;
                            buf[buf_pos++] = n1;
                            buf[buf_pos++] = cssd_border_right_style;
                            buf[buf_pos++] = n1;
                            buf[buf_pos++] = cssd_border_bottom_style;
                            buf[buf_pos++] = n1;
                            buf[buf_pos++] = cssd_border_left_style;
                            buf[buf_pos++] = n1;
                            if (n2 != -1) {
                                buf[buf_pos++] = cssd_border_color;
                                for (int i = 0; i < 4; i++) {
                                    buf[buf_pos++] = color.type;
                                    buf[buf_pos++] = color.value;
                                }
                            }
                            if (n3 != -1) {
                                buf[buf_pos++] = cssd_border_width;
                                for (int i = 0; i < 4; i++) {
                                    buf[buf_pos++] = width.type;
                                    buf[buf_pos++] = width.value;
                                }
                            }
                        }
                    }
                    else {
                    if (n1 != -1) {
                        switch (prop_code){
                            case cssd_border_top:
                                buf[buf_pos++] = cssd_border_top_style;
                                buf[buf_pos++] = n1;
                                break;
                            case cssd_border_right:
                                buf[buf_pos++] = cssd_border_right_style;
                                buf[buf_pos++] = n1;
                                break;
                            case cssd_border_bottom:
                                buf[buf_pos++] = cssd_border_bottom_style;
                                buf[buf_pos++] = n1;
                                break;
                            case cssd_border_left:
                                buf[buf_pos++] = cssd_border_left_style;
                                buf[buf_pos++] = n1;
                                break;
                            default:break;
                        }
                        if (n2 != -1) {
                            switch (prop_code){
                                case cssd_border_top:
                                    buf[buf_pos++] = cssd_border_top_color;
                                    buf[buf_pos++] = color.type;
                                    buf[buf_pos++] = color.value;
                                    break;
                                case cssd_border_right:
                                    buf[buf_pos++] = cssd_border_right_color;
                                    buf[buf_pos++] = color.type;
                                    buf[buf_pos++] = color.value;
                                    break;
                                case cssd_border_bottom:
                                    buf[buf_pos++] = cssd_border_bottom_color;
                                    buf[buf_pos++] = color.type;
                                    buf[buf_pos++] = color.value;
                                    break;
                                case cssd_border_left:
                                    buf[buf_pos++] = cssd_border_left_color;
                                    buf[buf_pos++] = color.type;
                                    buf[buf_pos++] = color.value;
                                    break;
                                default:break;
                            }
                            }
                        }
                        if (n3 != -1) {
                            switch (prop_code){
                                case cssd_border_top:
                                    buf[buf_pos++] = cssd_border_top_width;
                                    buf[buf_pos++] = width.type;
                                    buf[buf_pos++] = width.value;
                                    break;
                                case cssd_border_right:
                                    buf[buf_pos++] = cssd_border_right_width;
                                    buf[buf_pos++] = width.type;
                                    buf[buf_pos++] = width.value;
                                    break;
                                case cssd_border_bottom:
                                    buf[buf_pos++] = cssd_border_bottom_width;
                                    buf[buf_pos++] = width.type;
                                    buf[buf_pos++] = width.value;
                                    break;
                                case cssd_border_left:
                                    buf[buf_pos++] = cssd_border_left_width;
                                    buf[buf_pos++] = width.type;
                                    buf[buf_pos++] = width.value;
                                    break;
                                default:break;
                            }
                            }
                        }
                    }
                    break;
            case cssd_background_image:
            {
                buf[buf_pos++] = prop_code;
                lString8 str;
                const char *tmp=decl;
                int len=0;
                while (*tmp && *tmp !=';' && *tmp!='}')
                {tmp++;len++;}
                str.append(decl,len);
                str.trim();
                len=str.length();
                buf[buf_pos++]=str.length();
                for(int i=0;i<len;i++)
                    buf[buf_pos++]=str[i];
            }
                break;
            case cssd_background_repeat:
               n= parse_name(decl,css_bg_repeat_names,-1);
               break;
            case cssd_background_position:
               n= parse_name(decl,css_bg_position_names,-1);
                    if (n>8&&n<18) n=n-9;
                    if (n==18) n=7;
                    if (n==19) n=1;
                    if (n==20) n=4;
                    if (n==21) n=6;
                    if (n==22) n=8;
                    if (n==23) n=9;
                    if (n==24) n=10;
                    if (n==25) n=11;
               break;
            case cssd_background_attachment:
               n= parse_name(decl,css_bg_attachment_names,-1);
               break;
            case cssd_background:
            {
                css_length_t color;
                if (parse_color_value(decl, color))
                {
                    buf[buf_pos++]=cssd_background_color;
                    buf[buf_pos++]=color.type;
                    buf[buf_pos++]=color.value;
                }
                lString8 str;
                const char *tmp=decl;
                int len=0;
                while (*tmp && *tmp !=';' && *tmp!='}')
                {tmp++;len++;}
                str.append(decl,len);
                tmp=str.c_str();
                str.trim();
                skip_spaces(tmp);
                int offset=len-str.length();//offset for removed spaces
                if (Utf8ToUnicode(str).lowercase().startsWith("url")) {
                    buf[buf_pos++]=cssd_background_image;
                    len=0;
                    while (*tmp && *tmp !=';' && *tmp!='}'&&*tmp!=')')
                    {tmp++;len++;}
                    len=len+1+offset;
                    str.clear();
                    str.append(decl,len);
                    str.trim();
                    buf[buf_pos++] = str.length();
                    for (int i = 0; i < str.length(); i++)
                        buf[buf_pos++] = str[i];
                    decl+=len;
                    skip_spaces(decl);
                    int repeat=parse_name(decl,css_bg_repeat_names,-1);
                    if(repeat!=-1)
                    {
                        buf[buf_pos++]=cssd_background_repeat;
                        buf[buf_pos++]=repeat;
                        skip_spaces(decl);
                    }
                    int position=parse_name(decl,css_bg_position_names,-1);
                    if (position!=-1)
                    {
                        if (position>8&&position<18) position=position-9;
                        if (position==18) position=7;
                        if (position==19) position=1;
                        if (position==20) position=4;
                        if (position==21) position=6;
                        if (position==22) position=8;
                        if (position==23) position=9;
                        if (position==24) position=10;
                        if (position==25) position=11;
                        buf[buf_pos++]=cssd_background_position;
                        buf[buf_pos++]=position;
                    }
                }

            }
               break;
            case cssd_border_spacing:
            {
                css_length_t len[2];
                int i;
                for (i = 0; i < 2; ++i)
                    if (!parse_number_value( decl, len[i]))
                        break;
                if (i)
                {
                    if (i==1) len[1] = len[0];

                    buf[ buf_pos++ ] = prop_code;
                    for (i = 0; i < 2; ++i)
                    {
                        buf[ buf_pos++ ] = len[i].type;
                        buf[ buf_pos++ ] = len[i].value;
                    }
                }
            }
                break;
            case cssd_border_collapse:
                n=parse_name(decl,css_bc_names,-1);
                break;
            case cssd_stop:
            case cssd_unknown:
            default:
                break;
            }
            if ( n!= -1)
            {
                // add enum property
                buf[buf_pos++] = prop_code;
                buf[buf_pos++] = n;
            }
            if (!strValue.empty())
            {
                // add string property
                if (prop_code==cssd_font_family)
                {
                    // font names
                    buf[buf_pos++] = cssd_font_names;
                    buf[buf_pos++] = strValue.length();
                    for (int i=0; i < strValue.length(); i++)
                        buf[buf_pos++] = strValue[i];
                }
            }
        }
        else
        {
            // error: unknown property?
        }
        next_property( decl );
    }

    // store parsed result
    if (buf_pos)
    {
        buf[buf_pos++] = cssd_stop; // add end marker
        _data = new int[buf_pos];
        for (int i=0; i<buf_pos; i++)
            _data[i] = buf[i];
    }

    // skip }
    skip_spaces( decl );
    if (*decl == '}')
    {
        decl++;
        return true;
    }
    return false;
}

static css_length_t read_length( int * &data )
{
    css_length_t len;
    len.type = (css_value_type_t) (*data++);
    len.value = (*data++);
    return len;
}

void LVCssDeclaration::apply( css_style_rec_t * style )
{
    if (!_data)
        return;
    int * p = _data;
    for (;;)
    {
        switch (*p++)
        {
        case cssd_display:
            style->display = (css_display_t) *p++;
            break;
        case cssd_white_space:
            style->white_space = (css_white_space_t) *p++;
            break;
        case cssd_text_align:
            style->text_align = (css_text_align_t) *p++;
            break;
        case cssd_text_align_last:
            style->text_align_last = (css_text_align_t) *p++;
            break;
        case cssd_text_decoration:
            style->text_decoration = (css_text_decoration_t) *p++;
            break;
        case cssd_hyphenate:
            style->hyphenate = (css_hyphenate_t) *p++;
            break;
        case cssd_list_style_type:
            style->list_style_type = (css_list_style_type_t) *p++;
            break;
        case cssd_list_style_position:
            style->list_style_position = (css_list_style_position_t) *p++;
            break;
        case cssd_page_break_before:
            style->page_break_before = (css_page_break_t) *p++;
            break;
        case cssd_page_break_after:
            style->page_break_after = (css_page_break_t) *p++;
            break;
        case cssd_page_break_inside:
            style->page_break_inside = (css_page_break_t) *p++;
            break;
        case cssd_vertical_align:
            style->vertical_align = (css_vertical_align_t) *p++;
            break;
        case cssd_font_family:
            style->font_family = (css_font_family_t) *p++;
            break;
        case cssd_font_names:
            {
                lString8 names;
                names.reserve(64);
                int len = *p++;
                for (int i=0; i<len; i++)
                    names << (lChar8)(*p++);
                names.pack();
                style->font_name = names;
            }
            break;
        case cssd_font_style:
            style->font_style = (css_font_style_t) *p++;
            break;
        case cssd_font_weight:
            style->font_weight = (css_font_weight_t) *p++;
            break;
        case cssd_font_size:
            style->font_size = read_length( p );
            break;
        case cssd_text_indent:
            style->text_indent = read_length( p );
            break;
        case cssd_line_height:
            style->line_height = read_length( p );
            break;
        case cssd_letter_spacing:
            style->letter_spacing = read_length( p );
            break;
        case cssd_color:
            style->color = read_length( p );
            break;
        case cssd_background_color:
            style->background_color = read_length( p );
            break;
        case cssd_width:
            style->width = read_length( p );
            break;
        case cssd_height:
            style->height = read_length( p );
            break;
        case cssd_margin_left:
            style->margin[0] = read_length( p );
            break;
        case cssd_margin_right:
            style->margin[1] = read_length( p );
            break;
        case cssd_margin_top:
            style->margin[2] = read_length( p );
            break;
        case cssd_margin_bottom:
            style->margin[3] = read_length( p );
            break;
        case cssd_margin:
            style->margin[2] = read_length( p );
            style->margin[1] = read_length( p );
            style->margin[3] = read_length( p );
            style->margin[0] = read_length( p );
            break;
        case cssd_padding_left:
            style->padding[0] = read_length( p );
            break;
        case cssd_padding_right:
            style->padding[1] = read_length( p );
            break;
        case cssd_padding_top:
            style->padding[2] = read_length( p );
            break;
        case cssd_padding_bottom:
            style->padding[3] = read_length( p );
            break;
        case cssd_padding:
            style->padding[2] = read_length( p );
            style->padding[1] = read_length( p );
            style->padding[3] = read_length( p );
            style->padding[0] = read_length( p );
            break;
        case cssd_border_top_color:
            style->border_color[0]=read_length(p);
            break;
        case cssd_border_right_color:
            style->border_color[1]=read_length(p);
            break;
        case cssd_border_bottom_color:
            style->border_color[2]=read_length(p);
            break;
        case cssd_border_left_color:
            style->border_color[3]=read_length(p);
            break;
        case cssd_border_top_width:
            style->border_width[0]=read_length(p);
            break;
        case cssd_border_right_width:
            style->border_width[1]=read_length(p);
            break;
        case cssd_border_bottom_width:
            style->border_width[2]=read_length(p);
            break;
        case cssd_border_left_width:
            style->border_width[3]=read_length(p);
            break;
        case cssd_border_top_style:
            style->border_style_top=(css_border_style_type_t) *p++;
            break;
        case cssd_border_right_style:
            style->border_style_right=(css_border_style_type_t) *p++;
            break;
        case cssd_border_bottom_style:
            style->border_style_bottom=(css_border_style_type_t) *p++;
            break;
        case cssd_border_left_style:
            style->border_style_left=(css_border_style_type_t) *p++;
            break;
        case cssd_border_color:
            style->border_color[0]=read_length(p);
            style->border_color[1]=read_length(p);
            style->border_color[2]=read_length(p);
            style->border_color[3]=read_length(p);
            break;
        case cssd_border_width:
            style->border_width[0]=read_length(p);
            style->border_width[1]=read_length(p);
            style->border_width[2]=read_length(p);
            style->border_width[3]=read_length(p);
            break;
        case cssd_border_style:
            style->border_style_top=(css_border_style_type_t) *p++;
            style->border_style_right=(css_border_style_type_t) *p++;
            style->border_style_bottom=(css_border_style_type_t) *p++;
            style->border_style_left=(css_border_style_type_t) *p++;
            break;
        case cssd_background_image:
        {
            lString8 imagefile;
                imagefile.reserve(64);
                int l = *p++;
                for (int i=0; i<l; i++)
                    imagefile << (lChar8)(*p++);
                imagefile.pack();
                style->background_image = imagefile;
        }
            break;
        case cssd_background_repeat:
            style->background_repeat=(css_background_repeat_value_t) *p++;
            break;
        case cssd_background_attachment:
            style->background_attachment=(css_background_attachment_value_t) *p++;
            break;
        case cssd_background_position:
            style->background_position=(css_background_position_value_t) *p++;
            break;
        case cssd_border_spacing:
            style->border_spacing[0]=read_length(p);
            style->border_spacing[1]=read_length(p);
            break;
        case cssd_border_collapse:
            style->border_collapse=(css_border_collapse_value_t) *p++;
            break;
        case cssd_stop:
            return;
        }
    }
}

lUInt32 LVCssDeclaration::getHash() {
    if (!_data)
        return 0;
    int * p = _data;
    lUInt32 hash = 0;
    for (;*p != cssd_stop;p++)
        hash = hash * 31 + *p;
    return hash;
}

static bool parse_ident( const char * &str, char * ident )
{
    *ident = 0;
    skip_spaces( str );
    if ( !css_is_alpha( *str ) )
        return false;
    int i;
    for (i=0; css_is_alnum(str[i]); i++)
        ident[i] = str[i];
    ident[i] = 0;
    str += i;
    return true;
}

bool LVCssSelectorRule::check( const ldomNode * & node )
{
    if (node->isNull() || node->isRoot())
        return false;
    switch (_type)
    {
    case cssrt_parent:        // E > F
        //
        {
            node = node->getParentNode();
            if (node->isNull())
                return false;
            return node->getNodeId() == _id;
        }
        break;
    case cssrt_ancessor:      // E F
        //
        {
            for (;;)
            {
                node = node->getParentNode();
                if (node->isNull())
                    return false;
                if (node->getNodeId() == _id)
                    return true;
            }
        }
        break;
    case cssrt_predecessor:   // E + F
        //
        {
            int index = node->getNodeIndex();
            // while
            if (index>0) {
                ldomNode * elem = node->getParentNode()->getChildElementNode(index-1, _id);
                if ( elem ) {
                    node = elem;
                    //CRLog::trace("+ selector: found pred element");
                    return true;
                }
                //index--;
            }
            return false;
        }
        break;
    case cssrt_attrset:       // E[foo]
        {
            if ( !node->hasAttributes() )
                return false;
            return node->hasAttribute(_attrid);
        }
        break;
    case cssrt_attreq:        // E[foo="value"]
        {
            lString16 val = node->getAttributeValue(_attrid);
            bool res = (val == _value);
            //if ( res )
            //    return true;
            //CRLog::trace("attreq: %s %s", LCSTR(val), LCSTR(_value) );
            return res;
        }
        break;
    case cssrt_attrhas:       // E[foo~="value"]
        // one of space separated values
        {
            lString16 val = node->getAttributeValue(_attrid);
            int p = val.pos( lString16(_value.c_str()) );            
            if (p<0)
                return false;
            if ( (p>0 && val[p-1]!=' ') 
                    || (p+_value.length()<val.length() && val[p+_value.length()]!=' ') )
                return false;
            return true;
        }
        break;
    case cssrt_attrstarts:    // E[foo|="value"]
        // todo
        {
            lString16 val = node->getAttributeValue(_attrid);
            if (_value.length()>val.length())
                return false;
            val = val.substr(0, _value.length());
            return val == _value;
        }
        break;
    case cssrt_id:            // E#id
        // todo
        {
            lString16 val = node->getAttributeValue(attr_id);
            /*lString16 ldomDocumentFragmentWriter::convertId( lString16 id ) adds codeBasePrefix to
             *original id name, I can not get codeBasePrefix from here so I add a space to identify the
             *real id name.*/
            int pos = val.pos(" ");
            if (pos != -1) {
                val = val.substr(pos + 1, val.length() - pos - 1);
            }
            if (_value.length()>val.length())
                return false;
            return val == _value;
        }
        break;
    case cssrt_class:         // E.class
        // todo
        {
            lString16 val = node->getAttributeValue(attr_class);
            val.lowercase();
//            if ( val.length() != _value.length() )
//                return false;
            //CRLog::trace("attr_class: %s %s", LCSTR(val), LCSTR(_value) );
        /*As I have eliminated leading and ending spaces in the attribute value, any space in
         *val means there are more than one classes */
        int pos = val.pos(_value);
        if (val.pos(" ") != -1 && pos != -1) {
            int len = _value.length();
            if (pos + len == val.length() || //in the end
                val.at(pos + len) == L' ')      //in the beginning or in the middle
                return true;
            else
                return false;
        }
            return val == _value;
        }
        break;
    case cssrt_universal:     // *
        return true;
    }
    return true;
}

bool LVCssSelector::check( const ldomNode * node ) const
{
    // check main Id
    if (_id!=0 && node->getNodeId() != _id)
        return false;
    if (!_rules)
        return true;
    // check additional rules
    const ldomNode * n = node;
    LVCssSelectorRule * rule = _rules;
    do
    {
        if ( !rule->check(n) )
            return false;
        rule = rule->getNext();
    } while (rule!=NULL);
    return true;
}

bool parse_attr_value( const char * &str, char * buf )
{
    int pos = 0;
    skip_spaces( str );
    if (*str=='\"')
    {
        str++;
        for ( ; str[pos] && str[pos]!='\"'; pos++)
        {
            if (pos>=64)
                return false;
        }
        if (str[pos]!='\"')
            return false;
        for (int i=0; i<pos; i++)
            buf[i] = str[i];
        buf[pos] = 0;
        str += pos+1;
        skip_spaces( str );
        if (*str != ']')
            return false;
        str++;
        return true;
    }
    else
    {
        for ( ; str[pos] && str[pos]!=' ' && str[pos]!='\t' && str[pos]!=']'; pos++)
        {
            if (pos>=64)
                return false;
        }
        if (str[pos]!=']')
            return false;
        for (int i=0; i<pos; i++)
            buf[i] = str[i];
        buf[pos] = 0;
        str+=pos;
        str++;
        return true;
    }
}

LVCssSelectorRule * parse_attr( const char * &str, lxmlDocBase * doc )
{
    char attrname[512];
    char attrvalue[512];
    LVCssSelectorRuleType st = cssrt_universal;
    if (*str=='.') {
        // E.class
        str++;
        skip_spaces( str );
        if (!parse_ident( str, attrvalue ))
            return NULL;
        skip_spaces( str );
        LVCssSelectorRule * rule = new LVCssSelectorRule(cssrt_class);
        lString16 s( attrvalue );
        s.lowercase();
        rule->setAttr(attr_class, s);
        return rule;
    } else if ( *str=='#' ) {
        // E#id
        str++;
        skip_spaces( str );
        if (!parse_ident( str, attrvalue ))
            return NULL;
        skip_spaces( str );
        LVCssSelectorRule * rule = new LVCssSelectorRule(cssrt_id);
        lString16 s( attrvalue );
        rule->setAttr(attr_id, s);
        return rule;
    } else if (*str != '[')
        return NULL;
    str++;
    skip_spaces( str );
    if (!parse_ident( str, attrname ))
        return NULL;
    skip_spaces( str );
    attrvalue[0] = 0;
    if (*str==']')
    {
        st = cssrt_attrset;
        str++;
    }
    else if (*str=='=')
    {
        str++;
        if (!parse_attr_value( str, attrvalue))
            return NULL;
        st = cssrt_attreq;
    }
    else if (*str=='~' && str[1]=='=')
    {
        str+=2;
        if (!parse_attr_value( str, attrvalue))
            return NULL;
        st = cssrt_attrhas;
    }
    else if (*str=='|' && str[1]=='=')
    {
        str+=2;
        if (!parse_attr_value( str, attrvalue))
            return NULL;
        st = cssrt_attrstarts;
    }
    else
    {
        return NULL;
    }
    LVCssSelectorRule * rule = new LVCssSelectorRule(st);
    lString16 s( attrvalue );
    lUInt16 id = doc->getAttrNameIndex( lString16(attrname).c_str() );
    rule->setAttr(id, s);
    return rule;
}

void LVCssSelector::insertRuleStart( LVCssSelectorRule * rule )
{
    rule->setNext( _rules );
    _rules = rule;
}

void LVCssSelector::insertRuleAfterStart( LVCssSelectorRule * rule )
{
    if ( !_rules ) {
        _rules = rule;
        return;
    }
    rule->setNext( _rules->getNext() );
    _rules->setNext( rule );
}

bool LVCssSelector::parse( const char * &str, lxmlDocBase * doc )
{
    if (!str || !*str)
        return false;
    for (;;)
    {
        skip_spaces( str );
        if ( *str == '*' ) // universal selector
        {
            str++;
            skip_spaces( str );
            _id = 0;
        } 
        else if ( *str == '.' ) // classname follows
        {
            _id = 0;
        }
        else if ( css_is_alpha( *str ) )
        {
            // ident
            char ident[64];
            if (!parse_ident( str, ident ))
                return false;
            _id = doc->getElementNameIndex( lString16(ident).c_str() );
            skip_spaces( str );
        }
        else
        {
            return false;
        }
        if ( *str == ',' || *str == '{' )
            return true;
        // one or more attribute rules
        bool attr_rule = false;
        while ( *str == '[' || *str=='.' || *str=='#' )
        {
            LVCssSelectorRule * rule = parse_attr( str, doc );
            if (!rule)
                return false;
            insertRuleStart( rule ); //insertRuleAfterStart
            //insertRuleAfterStart( rule ); //insertRuleAfterStart

            /*
            if ( _id!=0 ) {
                LVCssSelectorRule * rule = new LVCssSelectorRule(cssrt_parent);
                rule->setId(_id);
                insertRuleStart( rule );
                _id=0;
            }
            */

            skip_spaces( str );
            attr_rule = true;
            //continue;
        }
        // element relation
        if (*str == '>')
        {
            str++;
            LVCssSelectorRule * rule = new LVCssSelectorRule(cssrt_parent);
            rule->setId(_id);
            insertRuleStart( rule );
            _id=0;
            continue;
        }
        else if (*str == '+')
        {
            str++;
            LVCssSelectorRule * rule = new LVCssSelectorRule(cssrt_predecessor);
            rule->setId(_id);
            insertRuleStart( rule );
            _id=0;
            continue;
        }
        else if (css_is_alpha( *str ))
        {
            LVCssSelectorRule * rule = new LVCssSelectorRule(cssrt_ancessor);
            rule->setId(_id);
            insertRuleStart( rule );
            _id=0;
            continue;
        }
        if ( !attr_rule )
            return false;
        else if ( *str == ',' || *str == '{' )
            return true;
    }
}

static bool skip_until_end_of_rule( const char * &str )
{
    while ( *str && *str!='}' )
        str++;
    if ( *str == '}' )
        str++;
    return *str != 0;
}

LVCssSelectorRule::LVCssSelectorRule( LVCssSelectorRule & v )
: _type(v._type), _id(v._id), _attrid(v._attrid)
, _next(NULL)
, _value( v._value )
{
    if ( v._next )
        _next = new LVCssSelectorRule( *v._next );
}

LVCssSelector::LVCssSelector( LVCssSelector & v )
: _id(v._id), _decl(v._decl), _specificity(v._specificity), _next(NULL), _rules(NULL)
{
    if ( v._next )
        _next = new LVCssSelector( *v._next );
    if ( v._rules )
        _rules = new LVCssSelectorRule( *v._rules );
}

void LVStyleSheet::set(LVPtrVector<LVCssSelector> & v  )
{
    _selectors.clear();
    if ( !v.size() )
        return;
    _selectors.reserve( v.size() );
    for ( int i=0; i<v.size(); i++ ) {
        LVCssSelector * selector = v[i];
        if ( selector )
            _selectors.add( new LVCssSelector( *selector ) );
        else
            _selectors.add( NULL );
    }
}

LVStyleSheet::LVStyleSheet( LVStyleSheet & sheet )
:   _doc( sheet._doc )
{
    set( sheet._selectors );
}

void LVStyleSheet::apply( const ldomNode * node, css_style_rec_t * style )
{
    if (!_selectors.length())
        return; // no rules!
        
    lUInt16 id = node->getNodeId();
    
    LVCssSelector * selector_0 = _selectors[0];
    LVCssSelector * selector_id = id>0 && id<_selectors.length() ? _selectors[id] : NULL;

    for (;;)
    {
        if (selector_0!=NULL)
        {
            if (selector_id==NULL || selector_0->getSpecificity() < selector_id->getSpecificity() )
            {
                // step by sel_0
                selector_0->apply( node, style );
                selector_0 = selector_0->getNext();
            }
            else
            {
                // step by sel_id
                selector_id->apply( node, style );
                selector_id = selector_id->getNext();
            }
        }
        else if (selector_id!=NULL)
        {
            // step by sel_id
            selector_id->apply( node, style );
            selector_id = selector_id->getNext();
        }
        else
        {
            break; // end of chains
        }
    }
}

lUInt32 LVCssSelectorRule::getHash()
{
    lUInt32 hash = 0;
    hash = ( ( ( (lUInt32)_type * 31
        + (lUInt32)_id ) *31 )
        + (lUInt32)_attrid * 31 )
        + ::getHash(_value);
    return hash;
}

lUInt32 LVCssSelector::getHash()
{
    lUInt32 hash = 0;
    lUInt32 nextHash = 0;

    if (_next)
        nextHash = _next->getHash();
    for (LVCssSelectorRule * p = _rules; p; p = p->getNext()) {
        lUInt32 ruleHash = p->getHash();
        hash = hash * 31 + ruleHash;
    }
    hash = hash * 31 + nextHash;
    if (!_decl.isNull())
        hash = hash * 31 + _decl->getHash();
    return hash;
}

/// calculate hash
lUInt32 LVStyleSheet::getHash()
{
    lUInt32 hash = 0;
    for ( int i=0; i<_selectors.length(); i++ ) {
        if ( _selectors[i] )
            hash = hash * 31 + _selectors[i]->getHash() + i*15324;
    }
    return hash;
}

bool LVStyleSheet::parse( const char * str )
{
    LVCssSelector * selector = NULL;
    LVCssSelector * prev_selector;
    int err_count = 0;
    int rule_count = 0;
    for (;*str;)
    {
        // new rule
        prev_selector = NULL;
        bool err = false;
        for (;*str;)
        {
            // parse selector(s)
            selector = new LVCssSelector;
            selector->setNext( prev_selector );
            if ( !selector->parse(str, _doc) )
            {
                err = true;
                break;
            }
            else
            {
                if ( *str == ',' )
                {
                    str++;
                    prev_selector = selector;
                    continue; // next selector
                }
            }
            // parse declaration
            LVCssDeclRef decl( new LVCssDeclaration );
            if ( !decl->parse( str ) )
            {
                err = true;
                err_count++;
            }
            else
            {
                // set decl to selectors
                for (LVCssSelector * p = selector; p; p=p->getNext())
                    p->setDeclaration( decl );
                rule_count++;
            }
            break;
        }
        if (err)
        {
            // error:
            // delete chain of selectors
            delete selector;
            // ignore current rule
            skip_until_end_of_rule( str );
        }
        else
        {
            // Ok:
            // place rules to sheet
            for (LVCssSelector * p = selector; p;  )
            {
                LVCssSelector * item = p;
                p=p->getNext();
                lUInt16 id = item->getElementNameId();
                if (_selectors.length()<=id)
                    _selectors.set(id, NULL);
                // insert with specificity sorting
                if ( _selectors[id] == NULL 
                    || _selectors[id]->getSpecificity() > item->getSpecificity() )
                {
                    // insert as first item
                    item->setNext( _selectors[id] );
                    _selectors[id] = item;
                }
                else
                {
                    // insert as internal item
                    for (LVCssSelector * p = _selectors[id]; p; p = p->getNext() )
                    {
                        if ( p->getNext() == NULL
                            || p->getNext()->getSpecificity() > item->getSpecificity() )
                        {
                            item->setNext( p->getNext() );
                            p->setNext( item );
                            break;
                        }
                    }
                }
            }
        }
    }
    return _selectors.length() > 0;
}

/// extract @import filename from beginning of CSS
bool LVProcessStyleSheetImport( const char * &str, lString8 & import_file )
{
    const char * p = str;
    import_file.clear();
    skip_spaces( p );
    if ( *p !='@' )
        return false;
    p++;
    if (strncmp(p, "import", 6) != 0)
        return false;
    p+=6;
    skip_spaces( p );
    bool in_url = false;
    char quote_ch = 0;
    if ( !strncmp(p, "url", 3) ) {
        p+=3;
        skip_spaces( p );
        if ( *p != '(' )
            return false;
        p++;
        skip_spaces( p );
        in_url = true;
    }
    if ( *p == '\'' || *p=='\"' )
        quote_ch = *p++;
    while (*p) {
        if ( quote_ch && *p==quote_ch ) {
            p++;
            break;
        }
        if ( !quote_ch ) {
            if ( in_url && *p==')' ) {
                break;
            }
            if ( *p==' ' || *p=='\t' || *p=='\r' || *p=='\n' )
                break;
        }
        import_file << *p++;
    }
    skip_spaces( p );
    if ( in_url ) {
        if ( *p!=')' )
            return false;
        p++;
    }
    if ( import_file.empty() )
        return false;
    str = p;
    return true;
}

/// load stylesheet from file, with processing of import
bool LVLoadStylesheetFile( lString16 pathName, lString8 & css )
{
    LVStreamRef file = LVOpenFileStream( pathName.c_str(), LVOM_READ );
    if ( file.isNull() )
        return false;
    lString8 txt = UnicodeToUtf8( LVReadTextFile( file ) );
    lString8 txt2;
    const char * s = txt.c_str();
    lString8 import_file;
    if ( LVProcessStyleSheetImport( s, import_file ) ) {
        lString16 importFilename = LVMakeRelativeFilename( pathName, Utf8ToUnicode(import_file) );
        //lString8 ifn = UnicodeToLocal(importFilename);
        //const char * ifns = ifn.c_str();
        if ( !importFilename.empty() ) {
            LVStreamRef file2 = LVOpenFileStream( importFilename.c_str(), LVOM_READ );
            if ( !file2.isNull() )
                txt2 = UnicodeToUtf8( LVReadTextFile( file2 ) );
        }
    }
    if ( !txt2.empty() )
        txt2 << "\r\n";
    css = txt2 + s;
    return !css.empty();
}
