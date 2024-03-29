/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2010,2014 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2018-2020 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

/**
 * \file lvfnt.h
 * \brief Grayscale Bitmap Font engine
 * \section Unicode greyscale bitmap font file structure
 *
 * (pointers are byte offsets from file beginning)
 *
 * - <font file>::
 *    - <file header>      -- lvfont_header_t
 *    - <decode table>     -- huffman decode table
 *    - <glyph chunk 1>    -- lvfont_glyph_t * [64]
 *    - ...
 *    - <glyph chunk N>    -- lvfont_glyph_t * [64]
 * - <file header>::
 *    - <signature>        -- 4 bytes
 *    - <version>          -- 4 bytes
 *    - <fontName>         -- 64 bytes
 *    - <copyright>        -- 64 bytes
 *    - <fileSize>         -- 4 bytes
 *    - <fontAttributes>   -- 8 bytes
 *    - <font ranges>      -- lvfont_glyph_t * * [1024], 0 if no chunk
 * - <glyph chunk>::
 *    - <glyph table>      -- lvfont_glyph_t * [64], 0 if no glyph
 *    - <glyph 1>          -- lvfont_glyph_t [arbitrary size]
 *    - ...
 *    - <glyph M>          -- lvfont_glyph_t [arbitrary size]
 */

#ifndef __LVFNT_H_INCLUDED__
#define __LVFNT_H_INCLUDED__

#include "lvtypes.h"

/// maximum font name length
#define FONT_NAME_LENGTH       64
/// maximum copyright notice length
#define FONT_COPYRIGHT_LENGTH  64

#ifdef __cplusplus

#include "hyphman.h"

extern "C" {
#endif

/**
    \brief Bitmap font header structure

    This structure starts from the beginning of file.

*/
#pragma pack(push,1)
typedef struct 
tag_lvfont_header 
{
    char magic[4];   ///< {'L', 'F', 'N', 'T'}
    char version[4]; ///< {'1', '.', '0', '0'  } */
    char fontName[FONT_NAME_LENGTH];  ///< font typeface name
    char fontCopyright[FONT_COPYRIGHT_LENGTH]; ///< copyright notice
    lUInt32 fileSize;         ///< full font file size, bytes
    lUInt8  fontHeight;       ///< font height, pixels
    lUInt8  fontAvgWidth;     ///< avg char width, pixels
    lUInt8  fontMaxWidth;     ///< max char width
    lUInt8  fontBaseline;     ///< font baseline offset, from top, pixels
    lUInt8  fontBitsPerPixel; ///< usually 1, 2 or 4
    lUInt8  flgBold;          ///< 1 for bold, 0 for normal
    lUInt8  flgItalic;        ///< 1 for italic, 0 for normal
    lUInt8  fontFamily;       ///< font family
    lUInt16 minCode;          ///< min font character code
    lUInt16 maxCode;          ///< max font character code
    lUInt32 decodeTableOffset;///< huffman decode table offset
    lUInt32 rangesOffset[1024]; /**< \brief byte offset from beginning of file to 64 chars unicode ranges

                                   - 0 if no chars in range 
                                   - rangesOffset[0] - 0..63
                                   - rangesOffset[1] - 64..127
                                   - rangesOffset[2] - 128..191
                                   - ...
                                   - rangesOffset[maxCode/64] -- ..maxCode
                                   The real length of whis array is
                                   (maxCode/64+1), the rest is overlapped
                                   with first range data.
                                */
    /* decode table follows */
} lvfont_header_t;
#pragma pack(pop)

/**
    \brief Glyph range offset table.

    Contains pointers to individual glyphs for 64-character glyph range.
*/
#pragma pack(push,1)
typedef struct 
tag_lvfont_range 
{
    lUInt16 glyphsOffset[64]; /** \brief offset table for 64 glyphs in range
                                   - 0 if no glyph for char 
                                   - byte offset from beginning of range to glyph if exists

                                   followed by lvfont_glyph_t for each non-zero offset
                              */
} lvfont_range_t;
#pragma pack(pop)

/**
    \brief Glyph data structure.

    Describes properties of single glyph followed by compressed glyph image.

*/
#pragma pack(push,1)
typedef struct 
tag_lvfont_glyph 
{
    lUInt32  glyphSize;   ///< 4: bytes in glyph array
    lUInt16  blackBoxX;   ///< 0: width of glyph
    lUInt16  blackBoxY;   ///< 1: height of glyph black box
    lInt16   originX;     ///< 2: X origin for glyph
    lInt16   originY;     ///< 3: Y origin for glyph
    lUInt16  width;       ///< 6: full width of glyph
    lUInt8   glyph[2];    ///< 7: glyph data, arbitrary size
} lvfont_glyph_t;
#pragma pack(pop)

/** \brief RLE/Huffman table entry used for glyph image encoding.

    Describes huffman code of single symbol.

*/
#pragma pack(push,1)
typedef struct {
    lUInt8  value;    ///< color value
    lUInt8  count;    ///< number of times value is repeated
    lUInt8  codelen;  ///< number of bits in huffman code
    lUInt8  code;     ///< huffman code (left aligned)
} hrle_decode_table_t;
#pragma pack(pop)

/**
    \brief RLE/Huffman table used for glyph image encoding.

    Contains table used to encode glyph images.

*/
#pragma pack(push,1)
typedef struct {
    lUInt8  itemcount; ///< number of items in table
    lUInt8  bitcount;  ///< bit count per color value
    lUInt8  rightmask; ///< left aligned mask
    lUInt8  leftmask;  ///< right aligned mask
    hrle_decode_table_t table[1]; ///< table items [itemcount]
} hrle_decode_info_t;
#pragma pack(pop)

/// Font handle typedef to refer the font
typedef void * lvfont_handle;

/**********************************************************************
    font object API
**********************************************************************/

/** 
   \brief loads font from file, allocates memory 
   \return 1 if successful, 0 for error
*/
int lvfontOpen( const char * fname, lvfont_handle * hfont );

/// frees memory allocated for font 
void lvfontClose( lvfont_handle pfont );

/** \brief retrieves font header pointer by handle 
    \param hfont is font handle
    \return pointer to font header structure
*/
const lvfont_header_t * lvfontGetHeader( const lvfont_handle hfont );

/** \brief retrieves pointer to huffman decode table by font handle
    \param hfont is font handle
    \return pointer to huffman decode table
*/
const hrle_decode_info_t * lvfontGetDecodeTable( const lvfont_handle hfont );

/** \brief retrieves pointer to glyph structure for specified char by font handle
    \param hfont is font handle
    \param code is unicode character
    \return pointer to glyph structure
*/
const lvfont_glyph_t * lvfontGetGlyph( const lvfont_handle hfont, lUInt16 code );

/** \brief measures test line width

    \param pfont is font handle
    \param text is pointer to text string
    \param len is number of characters in text to measure
    \param max_width is a width limit -- measuring stops when total width exceeds this value
    \param widths returns running total widths of string [0..i] including text[i]
    \return number of items written to widths[]
*/
lUInt16 lvfontMeasureText( const lvfont_handle pfont, 
                    const lChar32 * text, int len, 
                    lUInt16 * widths,
                    lUInt8 * flags,
                    int max_width,
                    lChar32 def_char
                 );

// These lower than 0x0100 (that fit in a lUint8) may be set by lvfntman's measureText()
// (to possibly get some informative flags back from harfbuzz) and hyphman's hyphenate().
// (These should be changed or dropped with care, as they may be used by some other parts of CoolReader)
#define LCHAR_IS_SPACE               0x0001 ///< flag: this char is one of the unicode space chars.
                                            //         It is set only on the normal space and the normal non-breakable
                                            //         space (spaces that can have their widths expanded or shrunk).
                                            //         It is not set on the unicode fixed width spaces.
#define LCHAR_ALLOW_WRAP_AFTER       0x0002 ///< flag: line break after this char is allowed.
                                            //         It is set on all spaces, except non-breakable ones.
                                            //         It is set on soft-hyphen.
                                            //         It is not set on CJK chars.
#define LCHAR_DEPRECATED_WRAP_AFTER  0x0004 ///< flag: line break after this char is possible but deprecated
                                            //         When not using libunibreak: it is set on '-' and other unicode hyphens.
                                            //         When using libunibreak: set on all text inside "white-space: nowrap"
#define LCHAR_ALLOW_HYPH_WRAP_AFTER  0x0008 ///< flag: line break after this char is allowed with addition of hyphen
                                            //         It is set by Hyphman when finding hyphenation points in a word.
#define LCHAR_MANDATORY_NEWLINE      0x0010 ///< flag: this char must start with new line
#define LCHAR_IS_CLUSTER_TAIL        0x0020 ///< flag: this char is a tail of a cluster (eg. ligature,
                                            //         whose glyph is carried by first char)
                                            //         It is set by harfbuzz when used.

// (This one is actually not set by lvfntman)
#define LCHAR_LOCKED_SPACING         0x0040 ///< flag: forbid any letter spacing tweak on this char
                                            //         (for cursive scripts like arabic, and special cases)
#define LCHAR__AVAILABLE_BIT_08__    0x0080

/// The next ones, not fitting in a lUInt8, should only be set and used by lvtextfm
#define LCHAR_IS_OBJECT              0x0100 ///< flag: this char is object (image, float)
#define LCHAR_IS_COLLAPSED_SPACE     0x0200 ///< flag: this char is a space that should not be rendered
#define LCHAR_IS_TO_IGNORE           0x0400 ///< flag: this char is to be ignored/skipped in text measurement and drawing
#define LCHAR_IS_RTL                 0x0800 ///< flag: this char is part of a RTL segment

#define LCHAR__AVAILABLE_BIT_13__    0x1000
#define LCHAR__AVAILABLE_BIT_14__    0x2000
#define LCHAR__AVAILABLE_BIT_15__    0x4000
#define LCHAR__AVAILABLE_BIT_16__    0x8000

// Some idea, if needed:
// #define LCHAR_IS_CJK_NOT_PUNCT       0x1000 ///< flag: this char is part a CJK char but not a punctuation
// #define LCHAR_IS_CJK_LEFT_PUNCT      0x2000 ///< flag: this char is part a CJK left punctuation
// #define LCHAR_IS_CJK_RIGHT_PUNCT     0x4000 ///< flag: this char is part a CJK right punctuation
// #define LCHAR_IS_CJK_PUNCT           0x6000 ///< flag: (for checking) this char is a CJK punctuation (neutral if set)
// #define LCHAR_IS_CJK                 0x7000 ///< flag: (for checking) this char is a CJK char

// LCHAR_IS_EOL was not used by any code, and has been replaced by LCHAR_IS_CLUSTER_TAIL
// #define LCHAR_IS_EOL              0x0010 ///< flag: this char is CR or LF


/** \brief returns true if character is unicode space 
    \param code is character
    \return 1 if character is space, 0 otherwise 
*/
inline int lvfontIsUnicodeSpace( lChar32 code ) { return code==0x0020; }

/** \brief returns unpacked glyph image 
    \param packed is RLE/Huffman encoded glyph data
    \param table is RLE/Huffman table
    \param unpacked is buffer to place unpacked image (1 byte per pixel)
    \param unp_size is size of \a unpacked
*/
void lvfontUnpackGlyph( const lUInt8 * packed, 
                       const hrle_decode_info_t * table, 
                       lUInt8 * unpacked, 
                       int unp_size );

#ifdef __cplusplus
}



#endif

#endif
