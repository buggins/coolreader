/** \file lvtextfm.h
    
    \brief Text formatter API

   CoolReader Engine C-compatible text formatter API

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVTEXTFM_H_INCLUDED__
#define __LVTEXTFM_H_INCLUDED__

#include "lvfont.h"
#include "lvstring16collection.h"
#include "lvbmpbuf.h"
#include "textlang.h"

// comment out following line to use old formatter
#define USE_NEW_FORMATTER 1

#ifdef __cplusplus
extern "C" {
#endif

// src_text_fragment_t flags
#define LTEXT_ALIGN_LEFT       0x0001  /**< \brief new left-aligned paragraph */
#define LTEXT_ALIGN_RIGHT      0x0002  /**< \brief new right-aligned paragraph */
#define LTEXT_ALIGN_CENTER     0x0003  /**< \brief new centered paragraph */
#define LTEXT_ALIGN_WIDTH      0x0004  /**< \brief new justified paragraph */

#define LTEXT_LAST_LINE_ALIGN_SHIFT 16

#define LTEXT_LAST_LINE_ALIGN_LEFT       0x00010000  /**< \brief last line of justified paragraph should be left-aligned */
#define LTEXT_LAST_LINE_ALIGN_RIGHT      0x00020000  /**< \brief last line of justified paragraph should be right-aligned */
#define LTEXT_LAST_LINE_ALIGN_CENTER     0x00030000  /**< \brief last line of justified paragraph should be centered */
#define LTEXT_LAST_LINE_ALIGN_WIDTH      0x00040000  /**< \brief last line of justified paragraph should be justified */


#define LTEXT_FLAG_NEWLINE     0x0007  /**< \brief new line flags mask */
#define LTEXT_FLAG_OWNTEXT     0x0008  /**< \brief store local copy of text instead of pointer */

#define LTEXT_VALIGN_MASK           0x0070  /**< \brief vertical align flags mask */
#define LTEXT_VALIGN_BASELINE       0x0000  /**< \brief baseline vertical align */
#define LTEXT_VALIGN_SUB            0x0010  /**< \brief subscript */
#define LTEXT_VALIGN_SUPER          0x0020  /**< \brief superscript */
#define LTEXT_VALIGN_MIDDLE         0x0030  /**< \brief middle */
#define LTEXT_VALIGN_BOTTOM         0x0040  /**< \brief bottom */
#define LTEXT_VALIGN_TEXT_BOTTOM    0x0050  /**< \brief text-bottom */
#define LTEXT_VALIGN_TOP            0x0060  /**< \brief top */
#define LTEXT_VALIGN_TEXT_TOP       0x0070  /**< \brief text-top */

#define LTEXT_TD_UNDERLINE     0x0100  /**< \brief underlined text */
#define LTEXT_TD_OVERLINE      0x0200  /**< \brief overlined text */
#define LTEXT_TD_LINE_THROUGH  0x0400  /**< \brief striked through text */
#define LTEXT_TD_BLINK         0x0800  /**< \brief blinking text */
#define LTEXT_TD_MASK          0x0F00  /**< \brief text decoration mask */
    // These 4 above translate to LFNT_DRAW_* equivalents (see lvfntman.h). Keep them in sync.

#define LTEXT_SRC_IS_OBJECT    0x8000  /**< \brief object (image) */
#define LTEXT_IS_LINK          0x4000  /**< \brief link */
#define LTEXT_HYPHENATE        0x1000  /**< \brief allow hyphenation */
#define LTEXT_RUNIN_FLAG       0x2000  /**< \brief element display mode is runin */

#define LTEXT_FLAG_PREFORMATTED 0x0080 /**< \brief element space mode is preformatted */

#define LTEXT_SRC_IS_CLEAR_RIGHT     0x00100000  /**< \brief text follows <BR style="clear: right"> */
#define LTEXT_SRC_IS_CLEAR_LEFT      0x00200000  /**< \brief text follows <BR style="clear: left"> */
#define LTEXT_SRC_IS_CLEAR_BOTH      0x00300000  /**< \brief text follows <BR style="clear: both"> */
#define LTEXT_SRC_IS_CLEAR_LAST      0x00400000  /**< \brief ignorable text, added when nothing follows <BR style="clear: both"> */

#define LTEXT_SRC_IS_FLOAT           0x01000000  /**< \brief float:'ing node */
#define LTEXT_SRC_IS_FLOAT_DONE      0x02000000  /**< \brief float:'ing node (already dealt with) */
#define LTEXT_SRC_IS_INLINE_BOX      0x04000000  /**< \brief inlineBox wrapping node */

#define LTEXT_STRUT_CONFINED         0x08000000  /**< \brief text should not overflow/modify its paragraph strut baseline and height */

/** \brief Source text line
*/
typedef struct
{
    void *          object;   /**< \brief pointer to object which represents source */
    TextLangCfg *   lang_cfg;
    lInt16          indent;   /**< \brief first line indent (or all but first, when negative) */
    lInt16          valign_dy; /* drift y from baseline */
    lInt16          interval; /**< \brief line height in screen pixels */
    lInt16          letter_spacing; /**< \brief additional letter spacing, pixels */
    lUInt32         color;    /**< \brief color */
    lUInt32         bgcolor;  /**< \brief background color */
    lUInt32         flags;    /**< \brief flags */
    lUInt16         index;
    // move unions bottom to simplify debugging
    union {
        struct {
            lvfont_handle   font;     /**< \brief handle of font to draw string */
            const lChar16 * text;     /**< \brief pointer to unicode text string */
            lUInt16         len;      /**< \brief number of chars in text */
            lUInt16         offset;   /**< \brief offset from node start to beginning of line */
        } t;
        struct {
            // (Note: width & height will be stored negative when they are in % unit)
            lInt16         width;    /**< \brief width of image or inline-block-box */
            lInt16         height;   /**< \brief height of image or inline-block box */
            lUInt16        baseline; /**< \brief baseline of inline-block box */
        } o;
    };
} src_text_fragment_t;


/** \brief Formatted word
*/
typedef struct
{
   lUInt16  src_text_index;  /**< \brief index of source text line */
   lUInt16  width;           /**< \brief word width, pixels, when at line end */
   lUInt16  min_width;       /**< \brief index of source text line */
   lInt16   x;               /**< \brief word x position in line */
   lInt16   y;               /**< \brief baseline y position */
   lUInt16  flags;           /**< \brief flags */
   union {
          /// for text word
       struct {
           lUInt16  start;           /**< \brief position of word in source text */
           lUInt16  len;             /**< \brief number of chars in word */
       } t;
       /// for object
       struct {
           lUInt16  height;          /**< \brief height of image or inline-block box */
           lUInt16  baseline;        /**< \brief baseline of inline-block box */
       } o;
   };
   lInt16   _top_to_baseline;        /* temporary storage slots when delaying y computation, */
   lInt16   _baseline_to_bottom;     /* when valign top or bottom */
   // lUInt16  padding;         /**< \brief not used */
} formatted_word_t;

// formatted_word_t flags
#define LTEXT_WORD_CAN_ADD_SPACE_AFTER       0x0001 /// can add space after this word
#define LTEXT_WORD_CAN_BREAK_LINE_AFTER      0x0002 /// can break line after this word (not used anywhere)
#define LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER 0x0004 /// can break with hyphenation after this word
#define LTEXT_WORD_MUST_BREAK_LINE_AFTER     0x0008 /// must break line after this word (not used anywhere)

#define LTEXT_WORD_IS_LINK_START             0x0010 /// first word of link flag
#define LTEXT_WORD_IS_OBJECT                 0x0020 /// word is an image
#define LTEXT_WORD_IS_INLINE_BOX             0x0040 /// word is a inline-block or inline-table wrapping box

#define LTEXT_WORD_DIRECTION_KNOWN           0x0100 /// word has been thru bidi: if next flag is unset, it is LTR.
#define LTEXT_WORD_DIRECTION_IS_RTL          0x0200 /// word is RTL
#define LTEXT_WORD_BEGINS_PARAGRAPH          0x0400 /// word is the first word of a paragraph
#define LTEXT_WORD_ENDS_PARAGRAPH            0x0800 /// word is the last word of a paragraph
    // These 4 translate (after mask & shift) to LFNT_HINT_* equivalents
    // (see lvfntman.h). Keep them in sync.
#define LTEXT_WORD_DIRECTION_PARA_MASK       0x0F00
#define LTEXT_WORD_DIRECTION_PARA_TO_LFNT_SHIFT   8
#define WORD_FLAGS_TO_FNT_FLAGS(f) ( (f & LTEXT_WORD_DIRECTION_PARA_MASK)>>LTEXT_WORD_DIRECTION_PARA_TO_LFNT_SHIFT)


#define LTEXT_WORD_VALIGN_TOP                0x1000 /// word is to be vertical-align: top
#define LTEXT_WORD_VALIGN_BOTTOM             0x2000 /// word is to be vertical-align: bottom
#define LTEXT_WORD_STRUT_CONFINED            0x4000 /// word is to be fully contained into strut bounds
                                                    /// (used only when one of the 2 previous is set)

//#define LTEXT_BACKGROUND_MARK_FLAGS 0xFFFF0000l

// formatted_line_t flags
#define LTEXT_LINE_SPLIT_AVOID_BEFORE        0x01
#define LTEXT_LINE_SPLIT_AVOID_AFTER         0x02
#define LTEXT_LINE_IS_BIDI                   0x04
#define LTEXT_LINE_PARA_IS_RTL               0x08

/** \brief Text formatter formatted line
*/
typedef struct
{
   formatted_word_t * words;       /**< array of words */
   lInt32             word_count;  /**< number of words */
   lUInt32            y;           /**< start y position of line */
   lInt16             x;           /**< start x position */
   lUInt16            width;       /**< width */
   lUInt16            height;      /**< height */
   lUInt16            baseline;    /**< baseline y offset */
   lUInt8             flags;       /**< flags */
   lUInt8             align;       /**< alignment */
} formatted_line_t;

/** \brief Text formatter embedded float
*/
typedef struct
{
   src_text_fragment_t * srctext;     /**< source node */
   lInt32                y;           /**< start y position of float */
   lInt16                x;           /**< start x position */
   lUInt16               width;       /**< width */
   lUInt16               height;      /**< height */
   css_clear_t           clear;       /**< clear: css property value */
   bool                  is_right;    /**< is float: right */
   bool                  to_position; /**< not yet positionned */
   lString16Collection * links;       /** footnote links found in this float text */
} embedded_float_t;

/** \brief Bookmark highlight modes.
*/
enum {
    highlight_mode_none = 0,
    highlight_mode_solid = 1,
    highlight_mode_underline = 2
};

/** \brief Text highlight options for selection, bookmarks, etc.
*/
struct text_highlight_options_t {
    lUInt32 selectionColor;    /**< selection color */
    lUInt32 commentColor;      /**< comment bookmark color */
    lUInt32 correctionColor;   /**< correction bookmark color */
    int bookmarkHighlightMode; /**< bookmark highlight mode: 0=no highlight, 1=solid fill, 2=underline */
    text_highlight_options_t() {
        selectionColor = 0x80AAAAAA;
        commentColor = 0xC0FFFF00;
        correctionColor = 0xC0FF8000;
        bookmarkHighlightMode = highlight_mode_solid;
    }
};

/** \brief Text formatter container
*/
typedef struct
{
   src_text_fragment_t * srctext;       /**< source text lines */
   lInt32                srctextlen;    /**< number of source text lines */
   formatted_line_t   ** frmlines;      /**< formatted lines */
   lInt32                frmlinecount;  /**< formatted lines count*/
   embedded_float_t   ** floats;        /**< embedded floats */
   lInt32                floatcount;    /**< embedded floats count*/
   lUInt32               height;        /**< height of text fragment */
   lUInt16               width;         /**< width of text fragment */
   lUInt16               page_height;   /**< max page height */

    // Each line box starts with a zero-width inline box (called "strut") with
    // the element's font and line height properties:
   lUInt16               strut_height;   /**< strut height */
   lUInt16               strut_baseline; /**< strut baseline */

   // Image scaling options
   lInt32                img_zoom_in_mode_block; /**< can zoom in block images: 0=disabled, 1=integer scale, 2=free scale */
   lInt32                img_zoom_in_scale_block; /**< max scale for block images zoom in: 1, 2, 3 */
   lInt32                img_zoom_in_mode_inline; /**< can zoom in inline images: 0=disabled, 1=integer scale, 2=free scale */
   lInt32                img_zoom_in_scale_inline; /**< max scale for inline images zoom in: 1, 2, 3 */
   lInt32                img_zoom_out_mode_block; /**< can zoom out block images: 0=disabled, 1=integer scale, 2=free scale */
   lInt32                img_zoom_out_scale_block; /**< max scale for block images zoom out: 1, 2, 3 */
   lInt32                img_zoom_out_mode_inline; /**< can zoom out inline images: 0=disabled, 1=integer scale, 2=free scale */
   lInt32                img_zoom_out_scale_inline; /**< max scale for inline images zoom out: 1, 2, 3 */

   // Space width
   lInt32                space_width_scale_percent; /**< scale the normal width of all spaces in all fonts by this percent */
   lInt32                min_space_condensing_percent; /**< min size of space (relative to scaled size) to allow fitting line by reducing of spaces */

   // Highlighting
   text_highlight_options_t highlight_options; /**< options for selection/bookmark highlighting */

   // Reusable after it is cached by ldomNode::renderFinalBlock()
   // (Usually true, except in a single case: first rendering of final block
   // containing embedded blocks, where we want the first rendering to forward
   // these embedded blocks individual lines, for page splitting - a second
   // rendering, with proper inlineBox lines will be needed for drawing.)
   bool                  is_reusable;
} formatted_text_fragment_t;

/**  Alloc & init formatted text buffer

    \param width is width of formatted text fragment
*/
formatted_text_fragment_t * lvtextAllocFormatter( lUInt16 width );

/** Free formatted text buffer

    dont't forget to call it when object is no longer used

    \param pbuffer is pointer to formatted text buffer
*/
void lvtextFreeFormatter( formatted_text_fragment_t * pbuffer );

/** Add source text line

    Call this function after lvtextInitFormatter for each source fragment
*/
void lvtextAddSourceLine( 
   formatted_text_fragment_t * pbuffer,
   lvfont_handle   font,     /* handle of font to draw string */
   TextLangCfg *   lang_cfg,
   const lChar16 * text,     /* pointer to unicode text string */
   lUInt32         len,      /* number of chars in text, 0 for auto(strlen) */
   lUInt32         color,    /* text color */
   lUInt32         bgcolor,  /* background color */
   lUInt32         flags,    /* flags */
   lInt16          interval, /* line height in screen pixels */
   lInt16          valign_dy,/* drift y from baseline */
   lInt16          indent,   /* first line indent (or all but first, when negative) */
   void *          object,   /* pointer to custom object */
   lUInt16         offset,    /* offset from node/object start to start of line */
   lInt16          letter_spacing
                         );

/** Add source object

    Call this function after lvtextInitFormatter for each source fragment
*/
void lvtextAddSourceObject( 
   formatted_text_fragment_t * pbuffer,
   TextLangCfg *   lang_cfg,
   lInt16         width,
   lInt16         height,
   lUInt32         flags,     /* flags */
   lInt16          interval,  /* line height in screen pixels */
   lInt16          valign_dy, /* drift y from baseline */
   lInt16          indent,    /* first line indent (or all but first, when negative) */
   void *          object,    /* pointer to custom object */
   lInt16          letter_spacing
                         );


#ifdef __cplusplus
}

class LVDrawBuf;
class ldomMarkedRangeList;
struct img_scaling_options_t;
class BlockFloatFootprint;

/* C++ wrapper class */
class LFormattedText
{
    friend class LGrayDrawBuf;
private:
    formatted_text_fragment_t * m_pbuffer;
public:
    formatted_text_fragment_t * GetBuffer() { return m_pbuffer; }

    /// set strut height and baseline (line boxes starting minimal values)
    void setStrut(lUInt16 height, lUInt16 baseline) {
        m_pbuffer->strut_height = height;
        m_pbuffer->strut_baseline = baseline;
    }

    /// set image scaling options
    void setImageScalingOptions( img_scaling_options_t * options );

    /// set space glyph width scaling percent option (10..500%)
    // (scale the normal width of all spaces in all fonts by this percent)
    void setSpaceWidthScalePercent(int spaceWidthScalePercent);

    /// set space condensing line fitting option (25..100%)
    // (applies after spaceWidthScalePercent has been applied)
    void setMinSpaceCondensingPercent(int minSpaceWidthPercent);

    /// set colors for selection and bookmarks
    void setHighlightOptions(text_highlight_options_t * options);

    void Clear()
    { 
        lUInt16 width = m_pbuffer->width;
        lvtextFreeFormatter( m_pbuffer );
        m_pbuffer = lvtextAllocFormatter( width );
    }

    void AddSourceObject(
                lUInt32         flags,     /* flags */
                lInt16          interval,  /* line height in screen pixels */
                lInt16          valign_dy, /* drift y from baseline */
                lInt16          indent,    /* first line indent (or all but first, when negative) */
                void *          object,    /* pointer to custom object */
                TextLangCfg *   lang_cfg,
                lInt16          letter_spacing=0
         );

    void AddSourceLine(
           const lChar16 * text,        /* pointer to unicode text string */
           lUInt32         len,         /* number of chars in text, 0 for auto(strlen) */
           lUInt32         color,       /* text color */
           lUInt32         bgcolor,     /* background color */
           LVFont *        font,        /* font to draw string */
           TextLangCfg *   lang_cfg,
           lUInt32         flags,       /* (had default =LTEXT_ALIGN_LEFT|LTEXT_FLAG_OWNTEXT) */
           lInt16          interval,    /* line height in screen pixels */
           lInt16          valign_dy=0, /* drift y from baseline */
           lInt16          indent=0,    /* first line indent (or all but first, when negative) */
           void *          object=NULL,
           lUInt32         offset=0,
           lInt16          letter_spacing=0
        )
    {
        lvtextAddSourceLine(m_pbuffer, 
            font,  //font->GetHandle()
            lang_cfg,
            text, len, color, bgcolor, 
            flags, interval, valign_dy, indent, object, (lUInt16)offset, letter_spacing );
    }

    lUInt32 Format(lUInt16 width, lUInt16 page_height,
                        int para_direction=0, // = REND_DIRECTION_UNSET in lvrend.h
                        BlockFloatFootprint * float_footprint = NULL );

    int GetSrcCount()
    {
        return m_pbuffer->srctextlen;
    }

    int GetWidth()
    {
        return m_pbuffer->width;
    }

    const src_text_fragment_t * GetSrcInfo(int index)
    {
        return &m_pbuffer->srctext[index];
    }

    int GetLineCount()
    {
        return m_pbuffer->frmlinecount;
    }

    const formatted_line_t * GetLineInfo(int index)
    {
        return m_pbuffer->frmlines[index];
    }

    int GetFloatCount()
    {
        return m_pbuffer->floatcount;
    }

    const embedded_float_t * GetFloatInfo(int index)
    {
        return m_pbuffer->floats[index];
    }

    void Draw( LVDrawBuf * buf, int x, int y, ldomMarkedRangeList * marks = NULL,  ldomMarkedRangeList *bookmarks = NULL );

    bool isReusable() { return m_pbuffer->is_reusable; }

    LFormattedText() { m_pbuffer = lvtextAllocFormatter( 0 ); }

    ~LFormattedText() { lvtextFreeFormatter( m_pbuffer ); }
};

#endif

extern bool gFlgFloatingPunctuationEnabled;

#endif
