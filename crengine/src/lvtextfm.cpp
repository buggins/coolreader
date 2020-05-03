/*******************************************************

   CoolReader Engine C-compatible API

   lvtextfm.cpp:  Text formatter

   (c) Vadim Lopatin, 2000-2011
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "../include/crsetup.h"
#include "../include/lvfnt.h"
#include "../include/lvtextfm.h"
#include "../include/lvdrawbuf.h"
#include "../include/fb2def.h"

#ifdef __cplusplus
#include "../include/lvimg.h"
#include "../include/lvtinydom.h"
#include "../include/lvrend.h"
#include "../include/textlang.h"
#endif

#if USE_HARFBUZZ==1
#include <hb.h>
#endif

#if (USE_FRIBIDI==1)
#if BUNDLED_FRIBIDI==1
#include "fribidi.h"
#else
#include <fribidi/fribidi.h>
#endif
#endif

#define SPACE_WIDTH_SCALE_PERCENT 100
#define MIN_SPACE_CONDENSING_PERCENT 50
#define UNUSED_SPACE_THRESHOLD_PERCENT 5
#define MAX_ADDED_LETTER_SPACING_PERCENT 0


// to debug formatter

#if defined(_DEBUG) && 0
#define TRACE_LINE_SPLITTING 1
#else
#define TRACE_LINE_SPLITTING 0
#endif

#if TRACE_LINE_SPLITTING==1
#ifdef _MSC_VER
#define TR(...) CRLog::trace(__VA_ARGS__)
#else
#define TR(x...) CRLog::trace(x)
#endif
#else
#ifdef _MSC_VER
#define TR(...)
#else
#define TR(x...)
#endif
#endif

#define FRM_ALLOC_SIZE 16
#define FLT_ALLOC_SIZE 4

formatted_line_t * lvtextAllocFormattedLine( )
{
    formatted_line_t * pline = (formatted_line_t *)calloc(1, sizeof(*pline));
    return pline;
}

formatted_line_t * lvtextAllocFormattedLineCopy( formatted_word_t * words, int word_count )
{
    formatted_line_t * pline = (formatted_line_t *)calloc(1, sizeof(*pline));
    lUInt32 size = (word_count + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    pline->words = (formatted_word_t*)malloc( sizeof(formatted_word_t)*(size) );
    memcpy( pline->words, words, word_count * sizeof(formatted_word_t) );
    return pline;
}

void lvtextFreeFormattedLine( formatted_line_t * pline )
{
    if (pline->words)
        free( pline->words );
    free(pline);
}

formatted_word_t * lvtextAddFormattedWord( formatted_line_t * pline )
{
    int size = (pline->word_count + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pline->word_count >= size)
    {
        size += FRM_ALLOC_SIZE;
        pline->words = cr_realloc( pline->words, size );
    }
    return &pline->words[ pline->word_count++ ];
}

formatted_line_t * lvtextAddFormattedLine( formatted_text_fragment_t * pbuffer )
{
    int size = (pbuffer->frmlinecount + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if (pbuffer->frmlinecount >= size)
    {
        size += FRM_ALLOC_SIZE;
        pbuffer->frmlines = cr_realloc( pbuffer->frmlines, size );
    }
    return (pbuffer->frmlines[ pbuffer->frmlinecount++ ] = lvtextAllocFormattedLine());
}

formatted_line_t * lvtextAddFormattedLineCopy( formatted_text_fragment_t * pbuffer, formatted_word_t * words, int words_count )
{
    int size = (pbuffer->frmlinecount + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->frmlinecount >= size)
    {
        size += FRM_ALLOC_SIZE;
        pbuffer->frmlines = cr_realloc( pbuffer->frmlines, size );
    }
    return (pbuffer->frmlines[ pbuffer->frmlinecount++ ] = lvtextAllocFormattedLineCopy(words, words_count));
}

embedded_float_t * lvtextAllocEmbeddedFloat( )
{
    embedded_float_t * flt = (embedded_float_t *)calloc(1, sizeof(*flt));
    return flt;
}

embedded_float_t * lvtextAddEmbeddedFloat( formatted_text_fragment_t * pbuffer )
{
    int size = (pbuffer->floatcount + FLT_ALLOC_SIZE-1) / FLT_ALLOC_SIZE * FLT_ALLOC_SIZE;
    if (pbuffer->floatcount >= size)
    {
        size += FLT_ALLOC_SIZE;
        pbuffer->floats = cr_realloc( pbuffer->floats, size );
    }
    return (pbuffer->floats[ pbuffer->floatcount++ ] = lvtextAllocEmbeddedFloat());
}


formatted_text_fragment_t * lvtextAllocFormatter( lUInt16 width )
{
    formatted_text_fragment_t * pbuffer = (formatted_text_fragment_t*)calloc(1, sizeof(*pbuffer));
    pbuffer->width = width;
    pbuffer->strut_height = 0;
    pbuffer->strut_baseline = 0;
    pbuffer->is_reusable = true;
    int defMode = MAX_IMAGE_SCALE_MUL > 1 ? (ARBITRARY_IMAGE_SCALE_ENABLED==1 ? 2 : 1) : 0;
    int defMult = MAX_IMAGE_SCALE_MUL;
    // Notes from thornyreader:
    // mode: 0=disabled, 1=integer scaling factors, 2=free scaling
    // scale: 0=auto based on font size, 1=no zoom, 2=scale up to *2, 3=scale up to *3
    pbuffer->img_zoom_in_mode_block = defMode; /**< can zoom in block images: 0=disabled, 1=integer scale, 2=free scale */
    pbuffer->img_zoom_in_scale_block = defMult; /**< max scale for block images zoom in: 1, 2, 3 */
    pbuffer->img_zoom_in_mode_inline = defMode; /**< can zoom in inline images: 0=disabled, 1=integer scale, 2=free scale */
    pbuffer->img_zoom_in_scale_inline = defMult; /**< max scale for inline images zoom in: 1, 2, 3 */
    pbuffer->img_zoom_out_mode_block = defMode; /**< can zoom out block images: 0=disabled, 1=integer scale, 2=free scale */
    pbuffer->img_zoom_out_scale_block = defMult; /**< max scale for block images zoom out: 1, 2, 3 */
    pbuffer->img_zoom_out_mode_inline = defMode; /**< can zoom out inline images: 0=disabled, 1=integer scale, 2=free scale */
    pbuffer->img_zoom_out_scale_inline = defMult; /**< max scale for inline images zoom out: 1, 2, 3 */
    pbuffer->space_width_scale_percent = SPACE_WIDTH_SCALE_PERCENT; // 100% (keep original width)
    pbuffer->min_space_condensing_percent = MIN_SPACE_CONDENSING_PERCENT; // 50%
    pbuffer->unused_space_threshold_percent = UNUSED_SPACE_THRESHOLD_PERCENT; // 5%
    pbuffer->max_added_letter_spacing_percent = MAX_ADDED_LETTER_SPACING_PERCENT; // 0%

    return pbuffer;
}

void lvtextFreeFormatter( formatted_text_fragment_t * pbuffer )
{
    if (pbuffer->srctext)
    {
        for (int i=0; i<pbuffer->srctextlen; i++)
        {
            if (pbuffer->srctext[i].flags & LTEXT_FLAG_OWNTEXT)
                free( (void*)pbuffer->srctext[i].t.text );
        }
        free( pbuffer->srctext );
    }
    if (pbuffer->frmlines)
    {
        for (int i=0; i<pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( pbuffer->frmlines[i] );
        }
        free( pbuffer->frmlines );
    }
    if (pbuffer->floats)
    {
        for (int i=0; i<pbuffer->floatcount; i++)
        {
            if (pbuffer->floats[i]->links) {
                delete pbuffer->floats[i]->links;
            }
            free(pbuffer->floats[i]);
        }
        free( pbuffer->floats );
    }
    free(pbuffer);
}


void lvtextAddSourceLine( formatted_text_fragment_t * pbuffer,
   lvfont_handle   font,     /* handle of font to draw string */
   TextLangCfg *   lang_cfg,
   const lChar16 * text,     /* pointer to unicode text string */
   lUInt32         len,      /* number of chars in text, 0 for auto(strlen) */
   lUInt32         color,    /* color */
   lUInt32         bgcolor,  /* bgcolor */
   lUInt32         flags,    /* flags */
   lInt16          interval, /* line height in screen pixels */
   lInt16          valign_dy, /* drift y from baseline */
   lInt16          indent,    /* first line indent (or all but first, when negative) */
   void *          object,    /* pointer to custom object */
   lUInt16         offset,
   lInt16          letter_spacing
                         )
{
    int srctextsize = (pbuffer->srctextlen + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->srctextlen >= srctextsize)
    {
        srctextsize += FRM_ALLOC_SIZE;
        pbuffer->srctext = cr_realloc( pbuffer->srctext, srctextsize );
    }
    src_text_fragment_t * pline = &pbuffer->srctext[ pbuffer->srctextlen++ ];
    pline->t.font = font;
//    if (font) {
//        // DEBUG: check for crash
//        CRLog::trace("c font = %08x  txt = %08x", (lUInt32)font, (lUInt32)text);
//        ((LVFont*)font)->getVisualAligmentWidth();
//    }
//    if (font == NULL && ((flags & LTEXT_WORD_IS_OBJECT) == 0)) {
//        CRLog::fatal("No font specified for text");
//    }
    if ( !lang_cfg )
        lang_cfg = TextLangMan::getTextLangCfg(); // use main_lang
    pline->lang_cfg = lang_cfg;
    if (!len) for (len=0; text[len]; len++) ;
    if (flags & LTEXT_FLAG_OWNTEXT)
    {
        /* make own copy of text */
        pline->t.text = (lChar16*)malloc( len * sizeof(lChar16) );
        memcpy((void*)pline->t.text, text, len * sizeof(lChar16));
    }
    else
    {
        pline->t.text = text;
    }
    pline->index = (lUInt16)(pbuffer->srctextlen-1);
    pline->object = object;
    pline->t.len = (lUInt16)len;
    pline->indent = indent;
    pline->flags = flags;
    pline->interval = interval;
    pline->valign_dy = valign_dy;
    pline->t.offset = offset;
    pline->color = color;
    pline->bgcolor = bgcolor;
    pline->letter_spacing = letter_spacing;
}

void lvtextAddSourceObject(
   formatted_text_fragment_t * pbuffer,
   lInt16         width,
   lInt16         height,
   lUInt32         flags,     /* flags */
   lInt16          interval,  /* line height in screen pixels */
   lInt16          valign_dy, /* drift y from baseline */
   lInt16          indent,    /* first line indent (or all but first, when negative) */
   void *          object,    /* pointer to custom object */
   TextLangCfg *   lang_cfg,
   lInt16          letter_spacing
                         )
{
    int srctextsize = (pbuffer->srctextlen + FRM_ALLOC_SIZE-1) / FRM_ALLOC_SIZE * FRM_ALLOC_SIZE;
    if ( pbuffer->srctextlen >= srctextsize)
    {
        srctextsize += FRM_ALLOC_SIZE;
        pbuffer->srctext = cr_realloc( pbuffer->srctext, srctextsize );
    }
    src_text_fragment_t * pline = &pbuffer->srctext[ pbuffer->srctextlen++ ];
    pline->index = (lUInt16)(pbuffer->srctextlen-1);
    pline->o.width = width;
    pline->o.height = height;
    pline->object = object;
    pline->indent = indent;
    pline->flags = flags | LTEXT_SRC_IS_OBJECT;
    pline->interval = interval;
    pline->valign_dy = valign_dy;
    pline->letter_spacing = letter_spacing;
    if ( !lang_cfg )
        lang_cfg = TextLangMan::getTextLangCfg(); // use main_lang
    pline->lang_cfg = lang_cfg;
}


#define DEPRECATED_LINE_BREAK_WORD_COUNT    3
#define DEPRECATED_LINE_BREAK_SPACE_LIMIT   64


#ifdef __cplusplus

#define DUMMY_IMAGE_SIZE 16

bool gFlgFloatingPunctuationEnabled = true;

void LFormattedText::AddSourceObject(
            lUInt32         flags,     /* flags */
            lInt16          interval,  /* line height in screen pixels */
            lInt16          valign_dy, /* drift y from baseline */
            lInt16          indent,    /* first line indent (or all but first, when negative) */
            void *          object,    /* pointer to custom object */
            TextLangCfg *   lang_cfg,
            lInt16          letter_spacing
     )
{
    ldomNode * node = (ldomNode*)object;
    if (!node || node->isNull()) {
        TR("LFormattedText::AddSourceObject(): node is NULL!");
        return;
    }

    if (flags & LTEXT_SRC_IS_FLOAT) { // not an image but a float:'ing node
        // Nothing much to do with it at this point
        lvtextAddSourceObject(m_pbuffer, 0, 0,
            flags, interval, valign_dy, indent, object, lang_cfg, letter_spacing );
            // lvtextAddSourceObject will itself add to flags: | LTEXT_SRC_IS_OBJECT
            // (only flags & object parameter will be used, the others are not,
            // but they matter if this float is the first node in a paragraph,
            // as the code may grab them from the first source)
        return;
    }
    if (flags & LTEXT_SRC_IS_INLINE_BOX) { // not an image but a inline-block wrapping node
        // Nothing much to do with it at this point: we can't yet render it to
        // get its width & neight, as they might be in % of our main width, that
        // we don't know yet (but only when ->Format() is called).
        lvtextAddSourceObject(m_pbuffer, 0, 0,
            flags, interval, valign_dy, indent, object, lang_cfg, letter_spacing );
            // lvtextAddSourceObject will itself add to flags: | LTEXT_SRC_IS_OBJECT
        return;
    }

    LVImageSourceRef img = node->getObjectImageSource();
    if ( img.isNull() )
        img = LVCreateDummyImageSource( node, DUMMY_IMAGE_SIZE, DUMMY_IMAGE_SIZE );
    lInt16 width = (lUInt16)img->GetWidth();
    lInt16 height = (lUInt16)img->GetHeight();

    // Scale image native size according to gRenderDPI
    width = scaleForRenderDPI(width);
    height = scaleForRenderDPI(height);

    css_style_ref_t style = node->getStyle();
    lInt16 w = 0, h = 0;
    int em = node->getFont()->getSize();
    lString16 nodename = node->getNodeName();
    if ((nodename.lowercase().compare("sub")==0
                || nodename.lowercase().compare("sup")==0)
            && (style->font_size.type==css_val_percent)) {
        em = em * 100 * 256 / style->font_size.value ; // value is %*256
    }
    w = lengthToPx(style->width, 100, em);
    h = lengthToPx(style->height, 100, em);
    if (style->width.type==css_val_percent) w = -w;
    if (style->height.type==css_val_percent) h = w*height/width;

    if ( w*h==0 ) {
        if ( w==0 ) {
            if ( h==0 ) { // use image native size
                h = height;
                w = width;
            } else { // use style height, keep aspect ratio
                w = width*h/height;
            }
        } else if ( h==0 ) { // use style width, keep aspect ratio
            h = w*height/width;
            if (h == 0) h = height;
        }
    }
    width = w;
    height = h;

    lvtextAddSourceObject(m_pbuffer, width, height,
        flags, interval, valign_dy, indent, object, lang_cfg, letter_spacing );
}

class LVFormatter {
public:
    //LVArray<lUInt16>  widths_buf;
    //LVArray<lUInt8>   flags_buf;
    formatted_text_fragment_t * m_pbuffer;
    int       m_length;
    int       m_size;
    bool      m_staticBufs;
    static bool      m_staticBufs_inUse;
    #if (USE_LIBUNIBREAK==1)
    static bool      m_libunibreak_init_done;
    #endif
    lChar16 * m_text;
    lUInt16 * m_flags;
    src_text_fragment_t * * m_srcs;
    lUInt16 * m_charindex;
    int  *     m_widths;
    int  m_y;
    int  m_max_img_height;
    bool m_has_images;
    bool m_has_float_to_position;
    bool m_has_ongoing_float;
    bool m_no_clear_own_floats;
    bool m_allow_strut_confinning;
    bool m_has_multiple_scripts;
    bool m_indent_first_line_done;
    int  m_indent_after_first_line;
    int  m_indent_current;
    int  m_specified_para_dir;
    #if (USE_FRIBIDI==1)
        // Bidi/RTL support
        FriBidiCharType *    m_bidi_ctypes;
        FriBidiBracketType * m_bidi_btypes;
        FriBidiLevel *       m_bidi_levels;
        FriBidiParType       m_para_bidi_type;
    #endif
    // These default to false and LTR when USE_FRIBIDI==0,
    // just to avoid too many "#if (USE_FRIBIDI==1)"
    bool m_has_bidi; // true when Bidi (or pure RTL) detected
    bool m_para_dir_is_rtl; // boolean shortcut of m_para_bidi_type

// These are not unicode codepoints: these values are put where we
// store text indexes in the source text node.
// So, when checking for these, also checks for m_flags[i] & LCHAR_IS_OBJECT.
// Note that m_charindex, being lUInt16, assume text nodes are not longer
// than 65535 chars. Things will get messy with longer text nodes...
#define OBJECT_CHAR_INDEX     ((lUInt16)0xFFFF)
#define FLOAT_CHAR_INDEX      ((lUInt16)0xFFFE)
#define INLINEBOX_CHAR_INDEX  ((lUInt16)0xFFFD)

    LVFormatter(formatted_text_fragment_t * pbuffer)
    : m_pbuffer(pbuffer), m_length(0), m_size(0), m_staticBufs(true), m_y(0)
    {
        #if (USE_LIBUNIBREAK==1)
        if (!m_libunibreak_init_done) {
            m_libunibreak_init_done = true;
            // Have libunibreak build up a few lookup tables for quicker computation
            init_linebreak();
        }
        #endif
        if (m_staticBufs_inUse)
            m_staticBufs = false;
        m_text = NULL;
        m_flags = NULL;
        m_srcs = NULL;
        m_charindex = NULL;
        m_widths = NULL;
        m_has_images = false,
        m_max_img_height = -1;
        m_has_float_to_position = false;
        m_has_ongoing_float = false;
        m_no_clear_own_floats = false;
        m_has_multiple_scripts = false;
        m_specified_para_dir = REND_DIRECTION_UNSET;
        #if (USE_FRIBIDI==1)
            m_bidi_ctypes = NULL;
            m_bidi_btypes = NULL;
            m_bidi_levels = NULL;
        #endif
    }

    ~LVFormatter()
    {
    }

    // Embedded floats positionning helpers.
    // Returns y of the bottom of the lowest float
    int getFloatsMaxBottomY() {
        int max_b_y = m_y;
        for (int i=0; i<m_pbuffer->floatcount; i++) {
            embedded_float_t * flt = m_pbuffer->floats[i];
            // Ignore fake floats (no src) made from outer floats footprint
            if ( flt->srctext != NULL ) {
                int b_y = flt->y + flt->height;
                if (b_y > max_b_y)
                    max_b_y = b_y;
            }
        }
        return max_b_y;
    }
    // Returns min y for next float
    int getNextFloatMinY(css_clear_t clear) {
        int y = m_y; // current line y
        for (int i=0; i<m_pbuffer->floatcount; i++) {
            embedded_float_t * flt = m_pbuffer->floats[i];
            if (flt->to_position) // ignore not yet positionned floats
                continue;
            // A later float should never be positionned above an earlier float
            if ( flt->y > y )
                y = flt->y;
            if ( clear > css_c_none) {
                if ( (clear == css_c_both) || (clear == css_c_left && !flt->is_right)
                                           || (clear == css_c_right && flt->is_right) ) {
                    int b_y = flt->y + flt->height;
                    if (b_y > y)
                        y = b_y;
                }
            }
        }
        return y;
    }
    // Returns available width (for text or a new float) available at y
    // and between y and y+h
    // Also set offset_x to the x where this width is available
    int getAvailableWidthAtY(int start_y, int h, int & offset_x) {
        if (m_pbuffer->floatcount == 0) { // common short path when no float
            offset_x = 0;
            return m_pbuffer->width;
        }
        int fl_left_max_x = 0;
        int fl_right_min_x = m_pbuffer->width;
        // We need to scan line by line from start_y to start_y+h to be sure
        int y = start_y;
        while (y <= start_y + h) {
            for (int i=0; i<m_pbuffer->floatcount; i++) {
                embedded_float_t * flt = m_pbuffer->floats[i];
                if (flt->to_position) // ignore not yet positionned floats
                    continue;
                if (flt->y <= y && flt->y + flt->height > y) { // this float is spanning this y
                    if (flt->is_right) {
                        if (flt->x < fl_right_min_x)
                            fl_right_min_x = flt->x;
                    }
                    else {
                        if (flt->x + flt->width > fl_left_max_x)
                            fl_left_max_x = flt->x + flt->width;
                    }
                }
            }
            y += 1;
        }
        offset_x = fl_left_max_x;
        return fl_right_min_x - fl_left_max_x;
    }
    // Returns next y after start_y where required_width is available
    // Also set offset_x to the x where that width is available
    int getYWithAvailableWidth(int start_y, int required_width, int required_height, int & offset_x, bool get_right_offset_x=false) {
        int y = start_y;
        int w;
        while (true) {
            w = getAvailableWidthAtY(y, required_height, offset_x);
            if (w >= required_width) // found it
                break;
            if (w == m_pbuffer->width) { // We're past all floats
                // returns this y even if required_width is larger than
                // m_pbuffer->width and it will overflow
                offset_x = 0;
                break;
            }
            y += 1;
        }
        if (get_right_offset_x) {
            int left_floats_w = offset_x;
            int right_floats_w = m_pbuffer->width - left_floats_w - w;
            offset_x = m_pbuffer->width - right_floats_w - required_width;
            if (offset_x < 0) // overflow
                offset_x = 0;
        }
        return y;
    }
    // The following positionning codes is not the most efficient, as we
    // call the previous functions that do many of the same kind of loops.
    // But it's the clearest to express the decision flow

    /// Embedded (among other inline elements) floats management
    void addFloat(src_text_fragment_t * src, int currentTextWidth) {
        embedded_float_t * flt =  lvtextAddEmbeddedFloat( m_pbuffer );
        flt->srctext = src;

        ldomNode * node = (ldomNode *) src->object;
        flt->is_right = node->getStyle()->float_ == css_f_right;
        // clear was not moved to the floatBox: get it from its single child
        flt->clear = node->getChildNode(0)->getStyle()->clear;

        // Thanks to the wrapping floatBox element, which has no
        // margin, we can set its RenderRectAccessor to be exactly
        // our embedded_float coordinates and sizes.
        //   If the wrapped element has margins, its renderRectAccessor
        //   will be positionned/sized at the level of borders or padding,
        //   as crengine does naturally with:
        //       fmt.setWidth(width - margin_left - margin_right);
        //       fmt.setHeight(height - margin_top - margin_bottom);
        //       fmt.setX(x + margin_left);
        //       fmt.setY(y + margin_top);
        // So, the RenderRectAccessor(floatBox) can act as a cache
        // of previously rendered and positionned floats!
        int width;
        int height;
        // This formatting code is called when rendering, but can also be called when
        // looking for links, highlighting... so it may happen that floats have
        // already been rendered and positionnned, and we already know their width
        // and height.
        bool already_rendered = false;
        { // in its own scope, so this RenderRectAccessor is forgotten when left
            RenderRectAccessor fmt( node );
            if ( RENDER_RECT_HAS_FLAG(fmt, BOX_IS_RENDERED) )
                already_rendered = true;
            // We could also directly use fmt.getX/Y() if it has already been
            // positionned, and avoid the positionning code below.
            // But let's be fully deterministic with that, and redo it.
        }
        if ( !already_rendered ) {
            LVRendPageContext alt_context( NULL, m_pbuffer->page_height, false );
            // We render the float with the specified direction (from upper dir=), even
            // if UNSET (and not with the direction determined by fribidi from the text).
            renderBlockElement( alt_context, node, 0, 0, m_pbuffer->width, m_specified_para_dir );
            // (renderBlockElement will ensure style->height if requested.)
            // Gather footnotes links accumulated by alt_context
            // (We only need to gather links in the rendering phase, for
            // page splitting, so no worry if we don't when already_rendered)
            lString16Collection * link_ids = alt_context.getLinkIds();
            if (link_ids->length() > 0) {
                flt->links = new lString16Collection();
                for ( int n=0; n<link_ids->length(); n++ ) {
                    flt->links->add( link_ids->at(n) );
                }
            }
        }
        // (renderBlockElement() above may update our RenderRectAccessor(),
        // so (re)get it only now)
        RenderRectAccessor fmt( node );
        width = fmt.getWidth();
        height = fmt.getHeight();

        flt->width = width;
        flt->height = height;
        flt->to_position = true;

        // If there are already floats to position, don't position any more for now
        if ( !m_has_float_to_position ) {
            if ( getNextFloatMinY(flt->clear) == m_y ) {
                // No previous float, nor any clear:'ing, prevents having this one
                // on current line,
                // See if it can still fit on this line, accounting for the current
                // width used by the text before this inline float (getCurrentLineWidth()
                // accounts for already positionned floats on this line)
                if ( currentTextWidth + flt->width <= getCurrentLineWidth() ) {
                    // Call getYWithAvailableWidth() just to get x
                    int x;
                    int y = getYWithAvailableWidth(m_y, flt->width + currentTextWidth, 0, x, flt->is_right);
                    if (y == m_y) { // should always be true, but just to be sure
                        if (flt->is_right) // correct x: add currentTextWidth we added
                            x = x + currentTextWidth;  // to the width for computation
                        flt->x = x;
                        flt->y = y;
                        flt->to_position = false;
                        fmt.setX(flt->x);
                        fmt.setY(flt->y);
                        if (flt->is_right)
                            RENDER_RECT_SET_FLAG(fmt, FLOATBOX_IS_RIGHT);
                        else
                            RENDER_RECT_UNSET_FLAG(fmt, FLOATBOX_IS_RIGHT);
                        RENDER_RECT_SET_FLAG(fmt, BOX_IS_RENDERED);
                        // Small trick for elements with negative margins (invert dropcaps)
                        // that would overflow above flt->x, to avoid a page split by
                        // sticking the line to the hopefully present margin-top that
                        // precedes this paragraph
                        // (we may want to deal with that more generically by storing these
                        // overflows so we can ensure no page split on the other following
                        // lines as long as they are not consumed)
                        RenderRectAccessor cfmt( node->getChildNode(0));
                        if (cfmt.getY() < 0)
                            m_has_ongoing_float = true;
                        return; // all done with this float
                    }
                }
            }
            m_has_float_to_position = true;
        }
    }
    void positionDelayedFloats() {
        // m_y has been updated, position delayed floats
        if (!m_has_float_to_position)
            return;
        for (int i=0; i<m_pbuffer->floatcount; i++) {
            embedded_float_t * flt = m_pbuffer->floats[i];
            if (!flt->to_position)
                continue;
            int x = 0;
            int y = getNextFloatMinY(flt->clear);
            y = getYWithAvailableWidth(y, flt->width, flt->height, x, flt->is_right);
            flt->x = x;
            flt->y = y;
            flt->to_position = false;
            ldomNode * node = (ldomNode *) flt->srctext->object;
            RenderRectAccessor fmt( node );
            fmt.setX(flt->x);
            fmt.setY(flt->y);
            if (flt->is_right)
                RENDER_RECT_SET_FLAG(fmt, FLOATBOX_IS_RIGHT);
            else
                RENDER_RECT_UNSET_FLAG(fmt, FLOATBOX_IS_RIGHT);
            RENDER_RECT_SET_FLAG(fmt, BOX_IS_RENDERED);
        }
        m_has_float_to_position = false;
    }
    void finalizeFloats() {
        // Adds blank lines to fill the vertical space still occupied by our own
        // inner floats (we don't fill the height of outer floats (float_footprint)
        // as they can still apply over our siblings.)
        fillAndMoveToY( getFloatsMaxBottomY() );
    }
    void fillAndMoveToY(int target_y) {
        // Adds blank lines to fill the vertical space from current m_y to target_y.
        // We need to use 1px lines to get a chance to allow a page wrap at
        // vertically stacked floats boundaries
        if ( target_y <= m_y ) // bogus: we won't rewind y
            return;
        bool has_ongoing_float;
        while ( m_y < target_y ) {
            formatted_line_t * frmline =  lvtextAddFormattedLine( m_pbuffer );
            frmline->y = m_y;
            frmline->x = 0;
            frmline->height = 1;
            frmline->baseline = 1; // no word to draw, does not matter
            // Check if there are floats spanning that y, so we
            // can avoid a page split
            has_ongoing_float = false;
            for (int i=0; i<m_pbuffer->floatcount; i++) {
                embedded_float_t * flt = m_pbuffer->floats[i];
                if (flt->to_position) // ignore not yet positionned floats (even if
                    continue;         // there shouldn't be any when this is called)
                if (flt->y < m_y && flt->y + flt->height > m_y) {
                    has_ongoing_float = true;
                    break;
                }
                // flt->y == m_y is fine: the float starts on this line,
                // we can split on it
            }
            if (has_ongoing_float) {
                frmline->flags |= LTEXT_LINE_SPLIT_AVOID_BEFORE;
            }
            m_y += 1;
            m_pbuffer->height = m_y;
        }
        checkOngoingFloat();
    }
    void floatClearText( int flags ) {
        // Handling of "clear: left/right/both" is different if the 'clear:'
        // is carried by a <BR> or by a float'ing box (for floating boxes, it
        // is done in addFloat()). Here, we deal with <BR style="clear:..">.
        // If a <BR/> has a "clear:", it moves the text below the floats, and the
        // text continues from there.
        // (Only a <BR> can carry a clear: among the non-floating inline elements.)
        if ( flags & LTEXT_SRC_IS_CLEAR_LEFT ) {
            int y = getNextFloatMinY( css_c_left );
            if (y > m_y)
                fillAndMoveToY( y );
        }
        if ( flags & LTEXT_SRC_IS_CLEAR_RIGHT ) {
            int y = getNextFloatMinY( css_c_right );
            if (y > m_y)
                fillAndMoveToY( y );
        }
    }
    int getCurrentLineWidth() {
        int x;
        // m_pbuffer->strut_height is all we can check for at this point,
        // but the text that will be put on this line may exceed it if
        // there's some vertical-align or font size change involved.
        // So, the line could be pushed down and conflict with a float below.
        // But this will do for now...
        return getAvailableWidthAtY(m_y, m_pbuffer->strut_height, x);
    }
    int getCurrentLineX() {
        int x;
        getAvailableWidthAtY(m_y, m_pbuffer->strut_height, x);
        return x;
    }
    bool isCurrentLineWithFloat() {
        int x;
        int w = getAvailableWidthAtY(m_y, m_pbuffer->strut_height, x);
        return w < m_pbuffer->width;
    }
    void checkOngoingFloat() {
        // Check if there is still some float spanning at current m_y
        // If there is, next added line will ensure no page split
        // between it and the previous line
        m_has_ongoing_float = false;
        for (int i=0; i<m_pbuffer->floatcount; i++) {
            embedded_float_t * flt = m_pbuffer->floats[i];
            if (flt->to_position) // ignore not yet positionned floats, as they
                continue;         // are not yet running past m_y
            if (flt->y < m_y && flt->y + flt->height > m_y) {
                m_has_ongoing_float = true;
                break;
            }
            // flt->y == m_y is fine: the float starts on this line,
            // no need to avoid page split by next line
        }
    }

    /// allocate buffers for paragraph
    void allocate( int start, int end )
    {
        int pos = 0;
        int i;
        // PASS 1: calculate total length (characters + objects)
        for ( i=start; i<end; i++ ) {
            src_text_fragment_t * src = &m_pbuffer->srctext[i];
            if ( src->flags & LTEXT_SRC_IS_FLOAT ) {
                pos++;
            }
            else if ( src->flags & LTEXT_SRC_IS_INLINE_BOX ) {
                pos++;
            }
            else if ( src->flags & LTEXT_SRC_IS_OBJECT ) {
                pos++;
                if (!m_has_images) {
                    // Compute images max height only when we meet an image,
                    // and only for the first one as it's the same for all
                    // images in this paragraph
                    ldomNode * node = (ldomNode *) src->object;
                    if ( node && !node->isNull() ) {
                        // We have to limit the image height so that the line
                        // that contains it does fit in the page without any
                        // uneeded page break
                        m_max_img_height = m_pbuffer->page_height;
                        // remove parent nodes' margin/border/padding
                        m_max_img_height -= node->getSurroundingAddedHeight();
                        // remove height taken by the strut baseline
                        m_max_img_height -= (m_pbuffer->strut_height - m_pbuffer->strut_baseline);
                        m_has_images = true;
                    }
                }
            }
            else {
                pos += src->t.len;
            }
        }

        // allocate buffers
        m_length = pos;

        TR("allocate(%d)", m_length);
        // We start with static buffers, but when m_length reaches STATIC_BUFS_SIZE,
        // we switch to dynamic buffers and we keep using them (realloc'ating when
        // needed).
        // The code in this file will fill these buffers with m_length items, so
        // from index [0] to [m_length-1], and read them back.
        // Willingly or not (bug?), this code may also access the buffer one slot
        // further at [m_length], and we need to set this slot to zero to avoid
        // a segfault. So, we need to reserve this additional slot when
        // allocating dynamic buffers, or checking if the static buffers can be
        // used.
        // (memset()'ing all buffers on their full allocated size to 0 would work
        // too, but there's a small performance hit when doing so. Just setting
        // to zero the additional slot seems enough, as all previous slots seems
        // to be correctly filled.)

#define STATIC_BUFS_SIZE 8192
#define ITEMS_RESERVED 16

        // "m_length+1" to keep room for the additional slot to be zero'ed
        if ( !m_staticBufs || m_length+1 > STATIC_BUFS_SIZE ) {
            // if (!m_staticBufs && m_text == NULL) printf("allocating dynamic buffers\n");
            if ( m_length+1 > m_size ) {
                // realloc
                m_size = m_length+ITEMS_RESERVED;
                m_text = cr_realloc(m_staticBufs ? NULL : m_text, m_size);
                m_flags = cr_realloc(m_staticBufs ? NULL : m_flags, m_size);
                m_charindex = cr_realloc(m_staticBufs ? NULL : m_charindex, m_size);
                m_srcs = cr_realloc(m_staticBufs ? NULL : m_srcs, m_size);
                m_widths = cr_realloc(m_staticBufs ? NULL : m_widths, m_size);
                #if (USE_FRIBIDI==1)
                    // Note: we could here check for RTL chars (and have a flag
                    // to then not do it in copyText()) so we don't need to allocate
                    // the following ones if we won't be using them.
                    m_bidi_ctypes = cr_realloc(m_staticBufs ? NULL : m_bidi_ctypes, m_size);
                    m_bidi_btypes = cr_realloc(m_staticBufs ? NULL : m_bidi_btypes, m_size);
                    m_bidi_levels = cr_realloc(m_staticBufs ? NULL : m_bidi_levels, m_size);
                #endif
            }
            m_staticBufs = false;
        } else {
            // static buffer space
            static lChar16 m_static_text[STATIC_BUFS_SIZE];
            static lUInt16 m_static_flags[STATIC_BUFS_SIZE];
            static src_text_fragment_t * m_static_srcs[STATIC_BUFS_SIZE];
            static lUInt16 m_static_charindex[STATIC_BUFS_SIZE];
            static int m_static_widths[STATIC_BUFS_SIZE];
            #if (USE_FRIBIDI==1)
                static FriBidiCharType m_static_bidi_ctypes[STATIC_BUFS_SIZE];
                static FriBidiBracketType m_static_bidi_btypes[STATIC_BUFS_SIZE];
                static FriBidiLevel m_static_bidi_levels[STATIC_BUFS_SIZE];
            #endif
            m_text = m_static_text;
            m_flags = m_static_flags;
            m_charindex = m_static_charindex;
            m_srcs = m_static_srcs;
            m_widths = m_static_widths;
            m_staticBufs = true;
            m_staticBufs_inUse = true;
            // printf("using static buffers\n");
            #if (USE_FRIBIDI==1)
                m_bidi_ctypes = m_static_bidi_ctypes;
                m_bidi_btypes = m_static_bidi_btypes;
                m_bidi_levels = m_static_bidi_levels;
            #endif
        }
        memset( m_flags, 0, sizeof(lUInt16)*m_length ); // start with all flags set to zero
        pos = 0;

        // We set to zero the additional slot that the code may peek at (with
        // the checks against m_length we did, we know this slot is allocated).
        // (This can be removed if we find this was a bug and can fix it)
        m_flags[m_length] = 0;
        m_text[m_length] = 0;
        m_charindex[m_length] = 0;
        m_srcs[m_length] = NULL;
        m_widths[m_length] = 0;
        #if (USE_FRIBIDI==1)
            m_bidi_ctypes[m_length] = 0;
            m_bidi_btypes[m_length] = 0;
            m_bidi_levels[m_length] = 0;
        #endif
    }

    /// copy text of current paragraph to buffers
    void copyText( int start, int end )
    {
        #if (USE_LIBUNIBREAK==1)
        struct LineBreakContext lbCtx;
        // Let's init it before the first char, by adding a leading Zero-Width Joiner
        // (Word Joiner, non-breakable) which should not change the behaviour with
        // the real first char coming up. We then can just use lb_process_next_char()
        // with the real text.
        // The lang lb_props will be plugged in from the TextLangCfg of the
        // coming up text node. We provide NULL in the meantime.
        lb_init_break_context(&lbCtx, 0x200D, NULL); // ZERO WIDTH JOINER
        #endif

        m_has_bidi = false; // will be set if fribidi detects it is bidirectionnal text
        m_para_dir_is_rtl = false;
        bool has_rtl = false; // if no RTL char, no need for expensive bidi processing
        // todo: according to https://www.w3.org/TR/css-text-3/#bidi-linebox
        // the bidi direction, if determined from the text itself (no dir= from
        // outer containers) must follow up to next paragraphs (separated by <BR/> or newlines).
        // Here in lvtextfm, each gets its own call to copyText(), so we might need some state.
        // This link also points out that line box direction and its text content direction
        // might be different... Could be we have that right (or not).
        // If this para final node or some upper block node specifies dir=rtl, assume fribidi
        // is needed, and avoid checking for rtl chars
        if ( m_specified_para_dir == REND_DIRECTION_RTL ) {
            has_rtl = true;
        }
        // Whether any "-cr-hint: strut-confined" should be applied: only when
        // we have non-space-only text in the paragraph - standalone images
        // possibly separated by spaces don't need to be reduced in size.
        m_allow_strut_confinning = false;

        int pos = 0;
        int i;
        bool prev_was_space = true; // start with true, to get rid of all leading spaces
        int last_non_space_pos = -1; // to get rid of all trailing spaces
        for ( i=start; i<end; i++ ) {
            src_text_fragment_t * src = &m_pbuffer->srctext[i];
            if ( src->flags & LTEXT_SRC_IS_FLOAT ) {
                m_text[pos] = 0;
                m_srcs[pos] = src;
                m_flags[pos] = LCHAR_IS_OBJECT;
                m_charindex[pos] = FLOAT_CHAR_INDEX; //0xFFFE;
                    // Note: m_flags was a lUInt8, and there were already 8 LCHAR_IS_* bits/flags
                    //   so we couldn't add our own. But using LCHAR_IS_OBJECT should not hurt,
                    //   as we do the FLOAT tests before it is used.
                    //   m_charindex[pos] is the one to use to detect FLOATs
                    // m_flags has since be updated to lUint16, but no real need
                    // to change what we did for floats to use a new flag.
                pos++;
                // No need to update prev_was_space or last_non_space_pos
                // No need for libunibreak object replacement character
            }
            else if ( src->flags & LTEXT_SRC_IS_INLINE_BOX ) {
                // Note: we shouldn't meet any EmbeddedBlock inlineBox here (and in
                // processParagraph(), addLine() and alignLine()) as they are dealt
                // with specifically in splitParagraphs() by processEmbeddedBlock().
                m_text[pos] = 0;
                m_srcs[pos] = src;
                m_flags[pos] = LCHAR_IS_OBJECT;
                #if (USE_LIBUNIBREAK==1)
                    // Let libunibreak know there was an object, for the followup text
                    // to set LCHAR_ALLOW_WRAP_AFTER on it.
                    // (it will allow wrap before and after an object, unless it's near
                    // some punctuation/quote/paren, whose rules will be ensured it seems).
                    int brk = lb_process_next_char(&lbCtx, (utf32_t)0xFFFC); // OBJECT REPLACEMENT CHARACTER
                    if (brk == LINEBREAK_ALLOWBREAK)
                        m_flags[pos-1] |= LCHAR_ALLOW_WRAP_AFTER;
                    else
                        m_flags[pos-1] &= ~LCHAR_ALLOW_WRAP_AFTER;
                #else
                    m_flags[pos] |= LCHAR_ALLOW_WRAP_AFTER;
                #endif
                m_charindex[pos] = INLINEBOX_CHAR_INDEX; //0xFFFD;
                last_non_space_pos = pos;
                prev_was_space = false;
                pos++;
            }
            else if ( src->flags & LTEXT_SRC_IS_OBJECT ) {
                m_text[pos] = 0;
                m_srcs[pos] = src;
                m_flags[pos] = LCHAR_IS_OBJECT;
                #if (USE_LIBUNIBREAK==1)
                    // Let libunibreak know there was an object
                    int brk = lb_process_next_char(&lbCtx, (utf32_t)0xFFFC); // OBJECT REPLACEMENT CHARACTER
                    if (brk == LINEBREAK_ALLOWBREAK)
                        m_flags[pos-1] |= LCHAR_ALLOW_WRAP_AFTER;
                    else
                        m_flags[pos-1] &= ~LCHAR_ALLOW_WRAP_AFTER;
                #else
                    m_flags[pos] |= LCHAR_ALLOW_WRAP_AFTER;
                #endif
                m_charindex[pos] = OBJECT_CHAR_INDEX; //0xFFFF;
                last_non_space_pos = pos;
                prev_was_space = false;
                pos++;
            }
            else {
                #if (USE_LIBUNIBREAK==1)
                // We hack into lbCtx private member and switch its lbpLang
                // on-the-fly to the props for a possibly new language.
                lbCtx.lbpLang = src->lang_cfg->getLBProps();
                #endif

                int len = src->t.len;
                lStr_ncpy( m_text+pos, src->t.text, len );
                if ( i==0 || (src->flags & LTEXT_FLAG_NEWLINE) )
                    m_flags[pos] = LCHAR_MANDATORY_NEWLINE;

                // On non PRE-formatted text, our XML parser have already removed
                // consecutive spaces, \t, \r and \n in each single text node
                // (inside and at boundaries), keeping only (if any) one leading
                // space and one trailing space.
                // These text nodes were simply appended (by lvrend) as is into
                // the src_text_fragment_t->t.text that we are processing here.
                // It may happen then that we, here, do get consecutive spaces, eg with:
                //   "<div> Some <span> text </span> and <span> </span> even more. </div>"
                // which would give us here:
                //   " Some  text  and   even more "
                //
                // https://www.w3.org/TR/css-text-3/#white-space-processing states, for
                // non-PRE paragraphs:
                // (a "segment break" is just a \n in the HTML source)
                //   (a) A sequence of segment breaks and other white space between two Chinese,
                //       Japanese, or Yi characters collapses into nothing.
                // (So it looks like CJY is CJK minus K - with Korean, if there is a
                // space between K chars, it should be kept.)
                //   (b) A zero width space before or after a white space sequence containing a
                //       segment break causes the entire sequence of white space to collapse
                //       into a zero width space.
                //   (c) Otherwise, consecutive white space collapses into a single space.
                //
                // For now, we only implement (c).
                // (b) can't really be implemented, as we don't know at this point
                // if there was a segment break or not, as any would have already been
                // converted to a space.
                // (a) is not implemented, but some notes and comments are below (may be
                // not too much bothering for CJK users if nothing was done to fix that?)
                //
                // It also states:
                //     Any space immediately following another collapsible space - even one
                //     outside the boundary of the inline containing that space, provided both
                //     spaces are within the same inline formatting context - is collapsed to
                //     have zero advance width. (It is invisible, but retains its soft wrap
                //     opportunity, if any.)
                // (lvtextfm actually deals with a single "inline formatting context", what
                // crengine calls a "final block".)
                //
                // It also states:
                //     - A sequence of collapsible spaces at the beginning of a line is removed.
                //     - A sequence of collapsible spaces at the end of a line is removed.
                //
                // The specs don't say which, among the consecutive collapsible spaces, to
                // keep, so let's keep the first one (they may have different width,
                // eg with: <big> some </big> <small> text </small> )
                //
                // Note: we can't "remove" any char: m_text, src_text_fragment_t->t.text
                // and the ldomNode text node own text need all to be in-sync: a shift
                // because of a removed char in any of them will cause wrong XPointers
                // and Rects (displaced highlights, etc...)
                // We can just "replace" a char (only in m_text, gone after this paragraph
                // processing) or flag (in m_flags for the time of paragraph processing,
                // in word->flags if needed later for drawing).

                bool preformatted = (src->flags & LTEXT_FLAG_PREFORMATTED);
                for ( int k=0; k<len; k++ ) {
                    lChar16 c = m_text[pos];

                    bool is_space = (c == ' ');
                    if ( is_space && prev_was_space && !preformatted && src->object ) {
                        // On non-pre paragraphs, flag spaces following a space
                        // so we can discard them later.
                        // (But only if the space is from a document text node (it then
                        // has a non-NULL ->object), to keep those we added for empty
                        // lines or identation with 'txform->AddSourceLine(L" "...)'.)
                        m_flags[pos] = LCHAR_IS_COLLAPSED_SPACE | LCHAR_ALLOW_WRAP_AFTER;
                        // m_text[pos] = '_'; // uncomment when debugging
                        // (We can replace the char to see it in printf() (m_text is not the
                        // text that is drawn, it's measured but we correct the measure
                        // by setting a zero width, it's just used here for analysis.
                        // But best to let it as-is except for debugging)
                    }
                    if ( !is_space || preformatted ) // don't strip traling spaces if pre
                        last_non_space_pos = pos;
                    if ( !is_space )
                        m_allow_strut_confinning = true;
                    prev_was_space = is_space;

                    /* non-optimized implementation of "(a) A sequence of segment breaks
                     * and other white space between two Chinese, Japanese, or Yi characters
                     * collapses into nothing", not excluding Korea chars
                     * (to be tested/optimized by a CJK dev)
                    if ( ch == ' ' && k>0 && k<len-1
                            && (isCJKIdeograph(m_text[pos-1]) || isCJKIdeograph(m_text[pos+1])) ) {
                        m_flags[pos] = LCHAR_IS_COLLAPSED_SPACE | LCHAR_ALLOW_WRAP_AFTER;
                        // m_text[pos] = '_';
                    }
                    */

                    // if ( ch == '-' || ch == 0x2010 || ch == '.' || ch == '+' || ch==UNICODE_NO_BREAK_SPACE )
                    //     m_flags[pos] |= LCHAR_DEPRECATED_WRAP_AFTER;

                    // Some of these (in the 2 commented lines just above) will be set
                    // in lvfntman measureText().
                    // We might want to have them all done here, for clarity.
                    // We may also want to flags CJK chars to distinguish
                    // left|right punctuations, and those that can have their
                    // ideograph width expanded/collapsed if needed.

                    // We flag some chars as we want them to be ignored: some font
                    // would render a glyph (like "[PDI]") for some control chars
                    // that shouldn't be rendered (Harfbuzz would skip them by itself,
                    // but we also want to skip them when using FreeType directly).
                    // We don't skip them when filling these buffer, as some of them
                    // can give valuable information to the bidi algorithm.
                    // Ignore the unicode direction hints (that we may have added ourselves
                    // in lvrend.cpp when processing <bdi>, <bdo> and the dir= attribute).
                    // Try to balance the searches:
                    if ( c >= 0x202A ) {
                        if ( c <= 0x2069 ) {
                            if ( c <= 0x202E ) m_flags[pos] = LCHAR_IS_TO_IGNORE;      // 202A>202E
                            else if ( c >= 0x2066 ) m_flags[pos] = LCHAR_IS_TO_IGNORE; // 2066>2069
                        }
                    }
                    // We might want to add some others when we happen to meet them.
                    // todo: see harfbuzz hb-unicode.hh is_default_ignorable() for how
                    // to do this kind of check fast

                    // Note: the overhead of using one of the following is quite minimal, so do if needed
                    /*
                    utf8proc_category_t uc = utf8proc_category(c);
                    if (uc == UTF8PROC_CATEGORY_CF)
                        printf("format char %x\n", c);
                    else if (uc == UTF8PROC_CATEGORY_CC)
                        printf("control char %x\n", c);
                    // Alternative, using HarfBuzz:
                    int uc = hb_unicode_general_category(hb_unicode_funcs_get_default(), c);
                    if (uc == HB_UNICODE_GENERAL_CATEGORY_FORMAT)
                        printf("format char %x\n", c);
                    else if (uc == HB_UNICODE_GENERAL_CATEGORY_CONTROL)
                        printf("control char %x\n", c);
                    */

                    #if (USE_LIBUNIBREAK==1)
                    lChar16 ch = m_text[pos];
                    if ( src->lang_cfg->hasLBCharSubFunc() ) {
                        // Lang specific function may want to substitute char (for
                        // libunibreak only) to tweak line breaking around it
                        ch = src->lang_cfg->getLBCharSubFunc()(m_text, pos, len-1 - k);
                    }
                    int brk = lb_process_next_char(&lbCtx, (utf32_t)ch);
                    if ( pos > 0 ) {
                        // printf("between <%c%c>: brk %d\n", m_text[pos-1], m_text[pos], brk);
                        // printf("between <%x.%x>: brk %d\n", m_text[pos-1], m_text[pos], brk);
                        if (brk != LINEBREAK_ALLOWBREAK) {
                            m_flags[pos-1] &= ~LCHAR_ALLOW_WRAP_AFTER;
                        }
                        else {
                            m_flags[pos-1] |= LCHAR_ALLOW_WRAP_AFTER;
                            // brk is set on the last space in a sequence of multiple spaces.
                            //   between <ne>: brk 2
                            //   between <ed>: brk 2
                            //   between <d.>: brk 2
                            //   between <. >: brk 2
                            //   between <  >: brk 2
                            //   between <  >: brk 2
                            //   between < T>: brk 1
                            //   between <Th>: brk 2
                            //   between <he>: brk 2
                            //   between <ey>: brk 2
                            //   between <y >: brk 2
                            //   between <  >: brk 2
                            //   between < h>: brk 1
                            //   between <ha>: brk 2
                            //   between <av>: brk 2
                            //   between <ve>: brk 2
                            //   between <e >: brk 2
                            //   between < a>: brk 1
                            //   between <as>: brk 2
                            // Given the algorithm described in addLine(), we want the break
                            // after the first space, so the following collapsed spaces can
                            // be at start of next line where they will be ignored.
                            // (Not certain this is really needed, but let's do it, as the
                            // code expecting that has been quite well tested and fixed over
                            // the months, so let's avoid adding uncertainty.)
                            if ( m_flags[pos-1] & LCHAR_IS_COLLAPSED_SPACE ) {
                                // We have spaces before, and if we are allowed to break,
                                // the break is allowed on all preceeding spaces.
                                int j = pos-2;
                                while ( j >= 0 && ( (m_flags[j] & LCHAR_IS_COLLAPSED_SPACE) || m_text[j] == ' ' ) ) {
                                    m_flags[j] |= LCHAR_ALLOW_WRAP_AFTER;
                                    j--;
                                }
                            }
                        }
                    }
                    #endif

                    #if (USE_FRIBIDI==1)
                        // Also try to detect if we have RTL chars, so that if we don't have any,
                        // we don't need to invoke expensive fribidi processing below (which
                        // may add a 50% duration increase to the text rendering phase).
                        // Looking at fribidi/lib/bidi-type.tab.i and its rules for tagging
                        // a char as RTL, only the following ranges will trigger it:
                        //   0590>08FF      Hebrew, Arabic, Syriac, Thaana, Nko, Samaritan...
                        //   200F 202B      Right-To-Left mark/embedding control chars
                        //   202E 2067      Right-To-Left override/isolate control chars
                        //   FB1D>FDFF      Hebrew and Arabic presentation forms
                        //   FE70>FEFF      Arabic presentation forms
                        //   10800>10FFF    Other rare scripts possibly RTL
                        //   1E800>1EEBB    Other rare scripts possibly RTL
                        // (There may be LTR chars in these ranges, but it's fine, we'll
                        // invoke fribidi, which will say there's no bidi.)
                        if ( !has_rtl ) {
                            // Try to balance the searches
                            if ( c >= 0x0590 ) {
                                if ( c <= 0x2067 ) {
                                    if ( c <= 0x08FF ) has_rtl = true;
                                    else if ( c >= 0x200F ) {
                                        if ( c == 0x200F || c == 0x202B || c == 0x202E || c == 0x2067 ) has_rtl = true;
                                    }
                                }
                                else if ( c >= 0xFB1D ) {
                                    if ( c <= 0xFDFF ) has_rtl = true;
                                    else if ( c <= 0xFEFF ) {
                                        if ( c >= 0xFE70) has_rtl = true;
                                    }
                                    else if ( c <= 0x1EEBB ) {
                                        if (c >= 0x1E800) has_rtl = true;
                                        else if ( c <= 0x10FFF && c >= 0x10800 ) has_rtl = true;
                                    }
                                }
                            }
                        }
                    #endif

                    m_charindex[pos] = k;
                    m_srcs[pos] = src;
                    pos++;
                }
            }
        }
        // Also flag as collapsed all spaces at the end of text
        pos = pos-1; // get back last pos++
        if (last_non_space_pos >= 0 && last_non_space_pos+1 <= pos) {
            for ( int k=last_non_space_pos+1; k<=pos; k++ ) {
                if (m_flags[k] == LCHAR_IS_OBJECT)
                    continue; // don't unflag floats
                m_flags[k] = LCHAR_IS_COLLAPSED_SPACE | LCHAR_ALLOW_WRAP_AFTER;
                // m_text[k] = '='; // uncomment when debugging
            }
        }
        TR("%s", LCSTR(lString16(m_text, m_length)));

        #if (USE_FRIBIDI==1)
        if ( has_rtl ) {
            // Trust the direction determined by renderBlockElementEnhanced() from the
            // upper nodes dir= attributes or CSS style->direction.
            if ( m_specified_para_dir == REND_DIRECTION_RTL ) {
                m_para_bidi_type = FRIBIDI_PAR_RTL; // Strong RTL
            }
            else if ( m_specified_para_dir == REND_DIRECTION_LTR ) {
                m_para_bidi_type = FRIBIDI_PAR_LTR; // Strong LTR
            }
            else { // REND_DIRECTION_UNSET
                m_para_bidi_type = FRIBIDI_PAR_WLTR; // Weak LTR (= auto with a bias toward LTR)
            }

            // Compute bidi levels
            fribidi_get_bidi_types( (const FriBidiChar*)m_text, m_length, m_bidi_ctypes);
            fribidi_get_bracket_types( (const FriBidiChar*)m_text, m_length, m_bidi_ctypes, m_bidi_btypes);
            int max_level = fribidi_get_par_embedding_levels_ex(m_bidi_ctypes, m_bidi_btypes,
                                m_length, (FriBidiParType*)&m_para_bidi_type, m_bidi_levels);
            // If computed max level == 1, we are in plain and only LTR, so no need for
            // more bidi work later.
            if ( max_level > 1 ) {
                m_has_bidi = true;
            }
            if ( m_para_bidi_type == FRIBIDI_PAR_RTL || m_para_bidi_type == FRIBIDI_PAR_WRTL )
                m_para_dir_is_rtl = true;

            // fribidi_shape(FRIBIDI_FLAG_SHAPE_MIRRORING, m_bidi_levels, m_length, NULL, (FriBidiChar*)m_text);
            // No use mirroring at this point I think, as it's not the text that will
            // be drawn. Hoping parens & al. have the same widths when mirrored.
            // We'll do that in addLine() when processing words when meeting
            // a rtl one, with fribidi_get_mirror_char().

            /* For debugging:
                printf("par_type %d , max_level %d\n", m_para_bidi_type, max_level);
                for (int i=0; i<m_length; i++)
                    printf("%d", m_bidi_levels[i]);
                printf("\n");
            // We get:
            //   pure LTR: par_type 272 , max_level 1  0000000000
            //   pure RTL: par_type 273 , max_level 2  1111111111
            //   LTR at start with later some RTL: par_type 272 , max_level 2  00000111111000000000000000
            //   RTL at start with later some LTR: par_type 273 , max_level 3  1111111111112222222222222221
            */
        }
        #endif
    }

    void resizeImage( int & width, int & height, int maxw, int maxh, bool isInline )
    {
        //CRLog::trace("Resize image (%dx%d) max %dx%d %s", width, height, maxw, maxh, isInline ? "inline" : "block");
        bool arbitraryImageScaling = false;
        int maxScale = 1;
        bool zoomIn = width<maxw && height<maxh;
        if ( isInline ) {
            if ( zoomIn ) {
                if ( m_pbuffer->img_zoom_in_mode_inline==0 )
                    return; // no zoom
                arbitraryImageScaling = m_pbuffer->img_zoom_in_mode_inline == 2;
                // maxScale = m_pbuffer->img_zoom_in_scale_inline;
            } else {
//                if ( m_pbuffer->img_zoom_out_mode_inline==0 )
//                    return; // no zoom
                arbitraryImageScaling = m_pbuffer->img_zoom_out_mode_inline == 2;
                // maxScale = m_pbuffer->img_zoom_out_scale_inline;
            }
        } else {
            if ( zoomIn ) {
                if ( m_pbuffer->img_zoom_in_mode_block==0 )
                    return; // no zoom
                arbitraryImageScaling = m_pbuffer->img_zoom_in_mode_block == 2;
                // maxScale = m_pbuffer->img_zoom_in_scale_block;
            } else {
//                if ( m_pbuffer->img_zoom_out_mode_block==0 )
//                    return; // no zoom
                arbitraryImageScaling = m_pbuffer->img_zoom_out_mode_block == 2;
                // maxScale = m_pbuffer->img_zoom_out_scale_block;
            }
        }
        resizeImage( width, height, maxw, maxh, arbitraryImageScaling, maxScale );
    }

    void resizeImage( int & width, int & height, int maxw, int maxh, bool arbitraryImageScaling, int maxScaleMult )
    {
        if (width == 0 || height == 0) {
            // Avoid a floating point exception (division by zero) crash.
            printf("CRE WARNING: resizeImage(width=0 or height=0)\n");
            return;
        }
        if (width < 0 || height < 0) {
            // Avoid invalid resizing if we are provided with negative values
            printf("CRE WARNING: resizeImage(width<0 or height<0)\n");
            return;
        }
        if (maxw < 0 || maxh < 0) {
            // Avoid invalid resizing if we are provided with negative max values
            printf("CRE WARNING: resizeImage(maxw<0 or maxh<0)\n");
            return;
        }
        //CRLog::trace("Resize image (%dx%d) max %dx%d %s  *%d", width, height, maxw, maxh, arbitraryImageScaling ? "arbitrary" : "integer", maxScaleMult);
        if ( maxScaleMult<1 ) maxScaleMult = 1;
        if ( arbitraryImageScaling ) {
            int pscale_x = 1000 * maxw / width;
            int pscale_y = 1000 * maxh / height;
            int pscale = pscale_x < pscale_y ? pscale_x : pscale_y;
            int maxscale = maxScaleMult * 1000;
            if ( pscale>maxscale )
                pscale = maxscale;
            height = height * pscale / 1000;
            width = width * pscale / 1000;
        } else {
            if (maxw == 0 || maxh == 0) {
                // Avoid a floating point exception (division by zero) crash.
                printf("CRE WARNING: resizeImage(maxw=0 or maxh=0)\n");
                return;
            }
            int scale_div = 1;
            int scale_mul = 1;
            int div_x = (width * 1000 / maxw);
            int div_y = (height * 1000 / maxh);
            if ( maxScaleMult>=3 && height*3 < maxh - 20
                    && width*3 < maxw - 20 ) {
                scale_mul = 3;
            } else if ( maxScaleMult>=2 && height * 2 < maxh - 20
                    && width * 2 < maxw - 20 ) {
                scale_mul = 2;
            } else if (div_x>1 || div_y>1) {
                if (div_x>div_y)
                    scale_div = div_x;
                else
                    scale_div = div_y;
            }
            height = height * 1000 * scale_mul / scale_div;
            width = width * 1000 * scale_mul / scale_div;
        }
    }

    /// checks whether to add more space after italic character
    /// (this could be used to shift some regular font glyphs
    /// like 'f' that often overflows the glyph - but we let
    /// such glyphs overflow in the padding/margin, as it is
    /// quite small and possibly intended by the font designer;
    /// italic overflows are often larger, and need to be
    /// corrected at end of line or end of italic node)
    int getAdditionalCharWidth( int pos, int maxpos ) {
        if (m_text[pos]==0) // object
            return 0; // no additional space
        LVFont * font = (LVFont*)m_srcs[pos]->t.font;
        if (!font)
            return 0; // no font
        if ( pos<maxpos-1 && m_srcs[pos+1]==m_srcs[pos] )
            return 0; // the same font, non-last char
        // Correct italic_only, only if overflow
        int glyph_overflow = - font->getRightSideBearing(m_text[pos], true, true);
        // if (glyph_overflow > 0) printf("right overflow: %c %d\n", m_text[pos], glyph_overflow);
        return glyph_overflow;
    }

    /// checks whether to add more space on left before italic character
    /// (this could be used to shift some regular font glyphs
    /// like 'J' whose foot often overflows the glyph - but we let
    /// such glyphs overflow in the padding/margin, as it is
    /// quite small and possibly intended by the font designer;
    /// italic underflows and overflows are often larger, and need
    /// to be corrected at start of line)
    int getAdditionalCharWidthOnLeft( int pos ) {
        if (m_text[pos]==0) // object
            return 0; // no additional space
        LVFont * font = (LVFont*)m_srcs[pos]->t.font;
        // Correct italic_only, including removal of positive leading space,
        int glyph_overflow = - font->getLeftSideBearing(m_text[pos], false, true);
        // if (glyph_overflow != 0) printf("left overflow %c: %d\n", m_text[pos], glyph_overflow);
        return glyph_overflow;
    }

    /// measure word
    bool measureWord(formatted_word_t * word, int & width)
    {
        src_text_fragment_t * srcline = &m_pbuffer->srctext[word->src_text_index];
        LVFont * srcfont= (LVFont *) srcline->t.font;
        const lChar16 * str = srcline->t.text + word->t.start;
        // Avoid malloc by using static buffers. Returns false if word too long.
        #define MAX_MEASURED_WORD_SIZE 127
        static lUInt16 widths[MAX_MEASURED_WORD_SIZE+1];
        static lUInt8 flags[MAX_MEASURED_WORD_SIZE+1];
        if (word->t.len > MAX_MEASURED_WORD_SIZE)
            return false;
        lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
        int chars_measured = srcfont->measureText(
                str,
                word->t.len,
                widths, flags,
                0x7FFF,
                '?',
                srcline->lang_cfg,
                srcline->letter_spacing,
                false,
                hints );
        width = widths[word->t.len-1];
        return true;
    }

    /// measure text of current paragraph
    void measureText()
    {
        int i;
        src_text_fragment_t * lastSrc = NULL;
        LVFont * lastFont = NULL;
        lInt16 lastLetterSpacing = 0;
        int start = 0;
        int lastWidth = 0;
        #define MAX_TEXT_CHUNK_SIZE 4096
        static lUInt16 widths[MAX_TEXT_CHUNK_SIZE+1];
        static lUInt8 flags[MAX_TEXT_CHUNK_SIZE+1];
        int tabIndex = -1;
        #if (USE_FRIBIDI==1)
            FriBidiLevel lastBidiLevel = 0;
            FriBidiLevel newBidiLevel;
        #endif
        #if (USE_HARFBUZZ==1)
            bool checkIfHarfbuzz = true;
            bool usingHarfbuzz = false;
            // Unicode script change (note: hb_script_t is uint32_t)
            lUInt32 prevScript = HB_SCRIPT_COMMON;
            hb_unicode_funcs_t* _hb_unicode_funcs = hb_unicode_funcs_get_default();
            bool prevSpecificScriptIsCursive = false;
        #endif
        int first_word_len = 0; // set to -1 when done with it (only used to check
                                // for single char first word, see below)
        for ( i=0; i<=m_length; i++ ) {
            LVFont * newFont = NULL;
            lInt16 newLetterSpacing = 0;
            src_text_fragment_t * newSrc = NULL;
            if ( tabIndex<0 && m_text[i]=='\t' ) {
                tabIndex = i;
            }
            bool isObject = false;
            bool prevCharIsObject = false;
            if ( i<m_length ) {
                newSrc = m_srcs[i];
                isObject = m_flags[i] & LCHAR_IS_OBJECT; // image, float or inline box
                newFont = isObject ? NULL : (LVFont *)newSrc->t.font;
                newLetterSpacing = newSrc->letter_spacing; // 0 for objects
                #if (USE_HARFBUZZ==1)
                    // Check if we are using Harfbuzz kerning with the first font met
                    if ( checkIfHarfbuzz && newFont ) {
                        if ( newFont->getShapingMode() == SHAPING_MODE_HARFBUZZ ) {
                            usingHarfbuzz = true;
                        }
                        checkIfHarfbuzz = false;
                    }
                #endif
            }
            if (i > 0)
                prevCharIsObject = m_flags[i-1] & LCHAR_IS_OBJECT; // image, float or inline box
            if ( !lastFont )
                lastFont = newFont;
            if (i == 0) {
                lastSrc = newSrc;
                lastLetterSpacing = newLetterSpacing;
            }
            bool srcChangedAndUsingHarfbuzz = false;
            #if (USE_HARFBUZZ==1)
                // When 2 contiguous text nodes have the same font, we measure the
                // whole combined segment. But when making words, we split on
                // text node change. When using full harfbuzz, we don't want it
                // to make ligatures at such text nodes boundaries: we need to
                // measure each text node individually.
                if ( usingHarfbuzz && newSrc != lastSrc && newFont && newFont == lastFont ) {
                    srcChangedAndUsingHarfbuzz = true;
                }
            #endif
            bool bidiLevelChanged = false;
            int lastDirection = 0; // unknown
            #if (USE_FRIBIDI==1)
                lastDirection = 1; // direction known: LTR if no bidi found
                if (m_has_bidi) {
                    newBidiLevel = m_bidi_levels[i];
                    if (i == 0)
                        lastBidiLevel = newBidiLevel;
                    else if ( newBidiLevel != lastBidiLevel )
                        bidiLevelChanged = true;
                    if ( FRIBIDI_LEVEL_IS_RTL(lastBidiLevel) )
                        lastDirection = -1; // RTL
                }
            #endif
            // When measuring with Harfbuzz, we should also split on Unicode script change,
            // even in a same bidi level (mixed hebrew and arabic in a single text node
            // should be handled as multiple segments, or Harfbuzz would shape the whole
            // text with the script of the first kind of text it meets).
            bool scriptChanged = false;
            #if (USE_HARFBUZZ==1)
                if ( usingHarfbuzz && !isObject ) {
                    // While we have the hb_script here, we'll update m_flags[i]
                    // with LCHAR_LOCKED_SPACING if the script is cursive
                    hb_script_t script = hb_unicode_script(_hb_unicode_funcs, m_text[i]);
                    if ( script != HB_SCRIPT_COMMON && script != HB_SCRIPT_INHERITED && script != HB_SCRIPT_UNKNOWN ) {
                        if ( script != prevScript ) {
                            if ( prevScript != HB_SCRIPT_COMMON ) {
                                // We previously met a real script, and we're meeting a new one
                                scriptChanged = true;
                                m_has_multiple_scripts = true;
                                // When only a single script found in a paragraph, we don't need
                                // to do that same kind of work in AddLine() to split on script
                                // change, as there's only one.
                            }
                            prevSpecificScriptIsCursive = isHBScriptCursive(script);
                        }
                        prevScript = script; // Real script met
                        if ( prevSpecificScriptIsCursive )
                            m_flags[i] |= LCHAR_LOCKED_SPACING;
                    }
                    // else: assume HB_SCRIPT_COMMON/INHERITED/UNKNOWN, even among cursive glyphs,
                    // can be letter_space'd for justification.
                }
            #endif
            // Note: some additional tweaks (like disabling letter-spacing when
            // a cursive script is detected) are done in measureText() and drawTextString().

            // Make a new segment to measure when any property changes from previous char
            if ( i>start && (   newFont != lastFont
                             || newLetterSpacing != lastLetterSpacing
                             || srcChangedAndUsingHarfbuzz
                             || bidiLevelChanged
                             || scriptChanged
                             || isObject
                             || prevCharIsObject
                             || i >= start+MAX_TEXT_CHUNK_SIZE
                             || (m_flags[i] & LCHAR_IS_TO_IGNORE)
                             || (m_flags[i] & LCHAR_MANDATORY_NEWLINE) ) ) {
                // measure start..i-1 chars
                if ( !(m_flags[i-1] & LCHAR_IS_OBJECT) ) { // neither image, float, nor inline box
                    // measure text
                    // Note: we provide text in the logical order, and measureText()
                    // will apply kerning in that order, which might be wrong if some
                    // text fragment happens to be RTL (except for Harfbuzz which will
                    // do the right thing).
                    int len = i - start;
                    // Provide direction and start/end of paragraph hints, for Harfbuzz
                    lUInt32 hints = 0;
                    if ( start == 0 ) hints |= LFNT_HINT_BEGINS_PARAGRAPH;
                    if ( i == m_length ) hints |= LFNT_HINT_ENDS_PARAGRAPH;
                    if ( lastDirection ) {
                        hints |= LFNT_HINT_DIRECTION_KNOWN;
                        if ( lastDirection < 0 )
                            hints |= LFNT_HINT_DIRECTION_IS_RTL;
                    }
                    int chars_measured = lastFont->measureText(
                            m_text + start,
                            len,
                            widths, flags,
                            0x7FFF, //pbuffer->width,
                            '?',
                            lastSrc->lang_cfg,
                            lastLetterSpacing,
                            false,
                            hints
                            );
                    if ( chars_measured<len ) {
                        // printf("######### chars_measured %d < %d\n", chars_measured, len);
                        // too long line
                        int newlen = chars_measured; // TODO: find best wrap position
                        i = start + newlen;
                        len = newlen;
                        // As we're going to continue measuring this text node,
                        // reset newFont (the font of the next text node), so
                        // it does not replace lastFont at the end of the loop.
                        newFont = NULL;
                        // If we didn't measure the full text, src, letter spacing and
                        // bidi level are to stay the same
                        newSrc = lastSrc;
                        newLetterSpacing = lastLetterSpacing;
                        #if (USE_FRIBIDI==1)
                            if (m_has_bidi)
                                newBidiLevel = lastBidiLevel;
                        #endif
                    }

                    // Deal with chars flagged as collapsed spaces:
                    // make each zero-width, so they are not accounted
                    // in the words width and position calculation.
                    // Note: widths[] (obtained from lastFont->measureText)
                    // and the m_widths[] we build have cumulative widths
                    // (width[k] is the length of the rendered text from
                    // chars 0 to k included).
                    // Also handle space width scaling if requested.
                    bool scale_space_width = m_pbuffer->space_width_scale_percent != 100;
                    if ( scale_space_width && lastSrc ) { // but not if <pre>
                        if ( lastSrc->flags & LTEXT_FLAG_PREFORMATTED )
                            scale_space_width = false;
                    }
                    int cumulative_width_removed = 0;
                    int prev_orig_measured_width = 0;
                    int char_width = 0; // current single char width
                    for ( int k=0; k<len; k++ ) {
                        // printf("%c %x f=%d w=%d\n", m_text[start+k], m_text[start+k], flags[k], widths[k]);
                        char_width = widths[k] - prev_orig_measured_width;
                        prev_orig_measured_width = widths[k];
                        if ( m_flags[start + k] & LCHAR_IS_COLLAPSED_SPACE) {
                            cumulative_width_removed += char_width;
                            // make it zero width: same cumulative width as previous char's
                            widths[k] = k>0 ? widths[k-1] : 0;
                            flags[k] = 0; // remove SPACE/WRAP/... flags
                        }
                        else if ( flags[k] & LCHAR_IS_SPACE ) {
                            // LCHAR_IS_SPACE has just been guessed, and is available in flags[], not yet in m_flags[]
                            if ( scale_space_width ) {
                                int scaled_width = char_width * m_pbuffer->space_width_scale_percent / 100;
                                // We can just account for the space reduction (or increase) in cumulative_width_removed
                                cumulative_width_removed += char_width - scaled_width;
                                widths[k] -= cumulative_width_removed;
                            }
                            if ( first_word_len >= 0 ) { // This is the space (or nbsp) after first word
                                if ( first_word_len == 1 ) { // Previous word is a single char
                                    if ( isLeftPunctuation(m_text[k-1]) ) {
                                        // This space follows one of the common opening quotation marks or
                                        // dashes used to introduce a quotation or a part of a dialog:
                                        // https://en.wikipedia.org/wiki/Quotation_mark
                                        // Don't allow this space to change width, so text justification
                                        // doesn't move away next word, so that other similar paragraphs
                                        // get their real first words vertically aligned.
                                        flags[k] |= LCHAR_LOCKED_SPACING;
                                        // Also prevent that quotation mark or dash from getting
                                        // additional letter spacing for justification
                                        flags[k-1] |= LCHAR_LOCKED_SPACING;
                                        //
                                        // Note: we do this check here, with the text still in logical
                                        // order, so we get that working with RTL text too (where, in
                                        // visual order, we'll have lost track of which word is the
                                        // first word - untested though).
                                    }
                                }
                                first_word_len = -1; // We don't need to deal with this anymore
                            }
                        }
                        else {
                            // remove, from the measured cumulative width, what we previously removed
                            widths[k] -= cumulative_width_removed;
                            if ( first_word_len >= 0 ) {
                                // Not a collapsed space and not a space: this will be part of first word
                                first_word_len++;
                            }
                        }
                        m_widths[start + k] = lastWidth + widths[k];
                        #if (USE_LIBUNIBREAK==1)
                        // Reset these flags if lastFont->measureText() has set them, as we trust
                        // only libunibreak (which is more clever with hyphens, that our code flag
                        // with LCHAR_DEPRECATED_WRAP_AFTER).
                        flags[k] &= ~(LCHAR_ALLOW_WRAP_AFTER|LCHAR_DEPRECATED_WRAP_AFTER);
                        #endif
                        m_flags[start + k] |= flags[k];
                        // printf("  => w=%d\n", m_widths[start + k]);
                    }

                    int dw = getAdditionalCharWidth(i-1, m_length);
                    if ( lastDirection < 0 ) // ignore it for RTL (as right side bearing is measured)
                        dw = 0;
                    if ( dw ) {
                        m_widths[i-1] += dw;
                        lastWidth += dw;
                    }

                    lastWidth += widths[len-1]; //len<m_length ? len : len-1];

                    // ?????? WTF
                    //m_flags[len] = 0;
                    // TODO: letter spacing letter_spacing
                }
                else { // measure object
                    // We have start=i-1 and m_flags[i-1] & LCHAR_IS_OBJECT
                    if (start != i-1) {
                        crFatalError(126, "LCHAR_IS_OBJECT with start!=i-1");
                    }
                    if ( m_charindex[start] == FLOAT_CHAR_INDEX ) {
                        // Embedded floats can have a zero width in this process of
                        // text measurement. They'll be measured when positionned.
                        m_widths[start] = lastWidth;
                    }
                    else if ( m_charindex[start] == INLINEBOX_CHAR_INDEX ) {
                        // Render this inlineBox to get its width, similarly to how we
                        // render floats in addFloat(). See there for more comments.
                        src_text_fragment_t * src = m_srcs[start];
                        ldomNode * node = (ldomNode *) src->object;
                        bool already_rendered = false;
                        { // in its own scope, so this RenderRectAccessor is forgotten when left
                            RenderRectAccessor fmt( node );
                            if ( RENDER_RECT_HAS_FLAG(fmt, BOX_IS_RENDERED) ) {
                                already_rendered = true;
                            }
                        }
                        if ( !already_rendered ) {
                            LVRendPageContext alt_context( NULL, m_pbuffer->page_height, false );
                            // inline-block and inline-table have a baseline, that renderBlockElement()
                            // will compute and give us back.
                            int baseline = REQ_BASELINE_FOR_INLINE_BLOCK;
                            if ( node->getChildNode(0)->getStyle()->display == css_d_inline_table )
                                baseline = REQ_BASELINE_FOR_TABLE;
                            // We render the inlineBox with the specified direction (from upper dir=), even
                            // if UNSET (and not with the direction determined by fribidi from the text).
                            renderBlockElement( alt_context, node, 0, 0, m_pbuffer->width, m_specified_para_dir, &baseline );
                            // (renderBlockElement will ensure style->height if requested.)

                            // Note: this inline box we just rendered can have some overflow
                            // (i.e. if it has some negative margins). As these overflows are
                            // usually small, we'll handle that in LFormattedText::Draw() by
                            // just dropping the page rect clip when drawing it, so that the
                            // overflowing content might be drawn in the page margins.
                            // (Otherwise, we'd need to upgrade our frmline to store a line
                            // top and bottom overflows, use LTEXT_LINE_SPLIT_AVOID_BEFORE/AFTER
                            // to stick that line to previous or next, with the risk of bringing
                            // a large top margin to top of page just to display that small
                            // overflow in it...)

                            RenderRectAccessor fmt( node );
                            fmt.setBaseline(baseline);
                            RENDER_RECT_SET_FLAG(fmt, BOX_IS_RENDERED);
                            // We'll have alignLine() do the fmt.setX/Y once it is fully positionned

                            // We'd like to gather footnote links accumulated by alt_context
                            // (we do that for floats), but it's quite more complicated:
                            // we have them here too early, and we would need to associate
                            // the links to this "char" index, so needing in LVFormatter
                            // something like:
                            //   LVHashTable<lUInt32, lString16Collection> m_inlinebox_links
                            // When adding this inlineBox to a frmline, we could then get back
                            // the links, and associate them to the frmline (so, needing a
                            // new field holding a lString16Collection, which would hold
                            // all the links in all the inlineBoxs part of that line).
                            // Finally, in renderBlockElementEnhanced, when adding
                            // links for words, we'd also need to add the one found
                            // in the frmline's lString16Collection.
                            // A bit complicated, for a probably very rare case, so
                            // let's just forget it and not have footnotes from inlineBox
                            // among our in-page footnotes...
                        }
                        // (renderBlockElement() above may update our RenderRectAccessor(),
                        // so (re)get it only now)
                        RenderRectAccessor fmt( node );
                        int width = fmt.getWidth();
                        int height = fmt.getHeight();
                        int baseline = fmt.getBaseline();
                        m_srcs[start]->o.width = width;
                        m_srcs[start]->o.height = height;
                        m_srcs[start]->o.baseline = baseline;
                        lastWidth += width;
                        m_widths[start] = lastWidth;
                    }
                    else {
                        // measure image
                        // assume i==start+1
                        int width = m_srcs[start]->o.width;
                        int height = m_srcs[start]->o.height;
                        width=width<0?-width*(m_pbuffer->width)/100:width;
                        height=height<0?-height*(m_pbuffer->width)/100:height;
                        /*
                        printf("measureText img: o.w=%d o.h=%d > w=%d h=%d (max %d %d is_inline=%d) %s\n",
                            m_srcs[start]->o.width, m_srcs[start]->o.height, width, height,
                            m_pbuffer->width, m_max_img_height, m_length>1,
                            UnicodeToLocal(ldomXPointer((ldomNode*)m_srcs[start]->object, 0).toString()).c_str());
                        */
                        resizeImage(width, height, m_pbuffer->width, m_max_img_height, m_length>1);
                        lastWidth += width;
                        m_widths[start] = lastWidth;
                    }
                }
                start = i;
                #if (USE_HARFBUZZ==1)
                    prevScript = HB_SCRIPT_COMMON; // Reset as next segment can start with any script
                #endif
            }
            // Skip measuring chars to ignore.
            if ( m_flags[i] & LCHAR_IS_TO_IGNORE) {
                m_widths[start] = lastWidth;
                start++;
                // This whole function here is very convoluted, it could really
                // be made simpler and be more readable.
                // This simple test here feels out of place, but it seems to
                // work in the various cases (ignorable char at start, standalone,
                // multiples, or at end).
            }
            //
            if (newFont)
                lastFont = newFont;
            lastSrc = newSrc;
            lastLetterSpacing = newLetterSpacing;
            #if (USE_FRIBIDI==1)
                if (m_has_bidi)
                    lastBidiLevel = newBidiLevel;
            #endif
        }
        if ( tabIndex >= 0 && m_srcs[0]->indent < 0) {
            // Used by obsolete rendering method erm_list_item when css_lsp_outside,
            // where the marker width is provided as negative/hanging indent.
            int tabPosition = -m_srcs[0]->indent; // has been set to marker_width
            if ( tabPosition>0 && tabPosition > m_widths[tabIndex] ) {
                int dx = tabPosition - m_widths[tabIndex];
                for ( i=tabIndex; i<m_length; i++ )
                    m_widths[i] += dx;
            }
        }
//        // debug dump
//        lString16 buf;
//        for ( int i=0; i<m_length; i++ ) {
//            buf << L" " << lChar16(m_text[i]) << L" " << lString16::itoa(m_widths[i]);
//        }
//        TR("%s", LCSTR(buf));
    }

#define MIN_WORD_LEN_TO_HYPHENATE 4
#define MAX_WORD_SIZE 64

    /// align line: add or reduce widths of spaces to achieve desired text alignment
    void alignLine( formatted_line_t * frmline, int alignment, int rightIndent=0, bool hasInlineBoxes=false ) {
        // Fetch current line x offset and max width
        int x_offset;
        int width = getAvailableWidthAtY(m_y, m_pbuffer->strut_height, x_offset);
        // printf("alignLine %d+%d < %d\n", frmline->x, frmline->width, width);

        // (frmline->x may be different from x_offset when non-zero text-indent)
        int usable_width = width - (frmline->x - x_offset) - rightIndent; // remove both sides indents
        int extra_width = usable_width - frmline->width;

        // We might want to prevent this when LangCfg == "de" (in german,
        // letter spacing is used for emphasis)
        if ( m_pbuffer->max_added_letter_spacing_percent > 0 // only if allowed
                        && alignment == LTEXT_ALIGN_WIDTH    // only when justifying
                        && frmline->word_count > 1           // not if single word (expanded, but not taking the full width is ugly)
                        && 100 * extra_width > m_pbuffer->unused_space_threshold_percent * usable_width ) {
            // extra_width is more than 5% of usable_width: we would be added too much spacing.
            // But we're allowed to add some letter spacing intoto words to reduce spacing
            // between words.
            // (We do that only when this line is justified - we could do it too when the
            // line is left- or right-aligned, but we do not know here if this is not the
            // last line of a paragraph, left aligned, that would not need to be expanded.)
            // We loop and increase letter spacing, and we stop as soon as we are
            // under the unused_space_threshold_percent (5%). If some iteration
            // brings us below min_extra_width (spaces shrunk too much), we go
            // back to the previous letter_spacing (which may put us back with
            // the unused extra space > 5%, but that is preferable).
            //
            // First, gather some info
            int min_extra_width = 0; // negative value (from the allowed spaces condensing)
            int max_font_size = 0;
            for ( int i=0; i<(int)frmline->word_count; i++ ) {
                formatted_word_t * word = &frmline->words[i];
                if ( word->distinct_glyphs <= 0 ) // image, inline box, cursive word
                    continue;
                min_extra_width += word->min_width - word->width;
                src_text_fragment_t * srcline = &m_pbuffer->srctext[word->src_text_index];
                LVFont * font = (LVFont *)srcline->t.font;
                int font_size = font->getSize();
                if ( font_size > max_font_size )
                    max_font_size = font_size;
                // Store this word font size in this temporary slot (that is not used anymore)
                word->_top_to_baseline = font_size;
            }
            int added_spacing = 0;
            int letter_spacing_ratio = 0;
            while ( true ) {
                letter_spacing_ratio++;
                added_spacing = 0;
                bool can_try_larger = false;
                for ( int i=0; i<(int)frmline->word_count; i++ ) {
                    formatted_word_t * word = &frmline->words[i];
                    if ( word->distinct_glyphs <= 0 ) // image, inline box, cursive word
                        continue;
                    // Store previous value in _baseline_to_bottom (also not used anymore) in case of
                    // excess and the need to use previous value (so we don't have to recompute it)
                    word->_baseline_to_bottom = word->added_letter_spacing;
                    // We apply letter_spacing proportionally to the font size (words
                    // in a smaller font size won't get any in the loop first steps)
                    int word_font_size = word->_top_to_baseline;
                    word->added_letter_spacing = letter_spacing_ratio * word_font_size / max_font_size;
                    int word_max_letter_spacing = word_font_size * m_pbuffer->max_added_letter_spacing_percent / 100;
                    if ( word->added_letter_spacing > word_max_letter_spacing  )
                        word->added_letter_spacing = word_max_letter_spacing;
                    else
                        can_try_larger = true;
                    added_spacing += word->distinct_glyphs * word->added_letter_spacing;
                }
                int new_extra_width = extra_width - added_spacing;
                if ( new_extra_width < min_extra_width ) { // too much added, not enough for spaces
                    // Get back values from previous step (which was fine)
                    added_spacing = 0;
                    for ( int i=0; i<(int)frmline->word_count; i++ ) {
                        formatted_word_t * word = &frmline->words[i];
                        if ( word->distinct_glyphs <= 0 ) // image, inline box, cursive word
                            continue;
                        word->added_letter_spacing = word->_baseline_to_bottom;
                        added_spacing += word->distinct_glyphs * word->added_letter_spacing;
                    }
                    break;
                }
                if ( !can_try_larger ) // all allowed max letter_spacing reached
                    break;
                if ( 100 * new_extra_width <= m_pbuffer->unused_space_threshold_percent * usable_width ) {
                    // < 5%, we're good
                    break;
                }
            }
            if ( added_spacing ) {
                // Fix up words positions and widths
                int shift_x = 0;
                for ( int i=0; i<(int)frmline->word_count; i++ ) {
                    formatted_word_t * word = &frmline->words[i];
                    if ( word->distinct_glyphs > 0 ) {
                        int added_width = word->distinct_glyphs * word->added_letter_spacing;
                        if ( i == frmline->word_count-1 ) {
                            // For the last word on a justified line, we want to not see
                            // any letter_spacing added after last glyph.
                            // The font will draw it, but we just want to position this
                            // word so it's drawn outside: just remove one letter_spacing.
                            // But not if this last word gets a hyphen, or the hyphen
                            // (not part of the word but added when drawing) would be
                            // shifted to the left.
                            if ( !(word->flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER) ) {
                                added_width -= word->added_letter_spacing;
                            }
                        }
                        word->width += added_width;
                        word->min_width += added_width;
                        word->x += shift_x;
                        shift_x += added_width;
                        frmline->width += added_width;
                        extra_width -= added_width;
                    }
                    else {
                        // Images, inline box, cursive words still need to be shifted
                        word->x += shift_x;
                    }
                }
            }
        }
        extra_width = usable_width - frmline->width;

        if ( extra_width < 0 ) {
            // line is too wide
            // reduce spaces to fit line
            int extraSpace = -extra_width;
            int totalSpace = 0;
            int i;
            for ( i=0; i<(int)frmline->word_count-1; i++ ) {
                if ( frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER ) {
                    int dw = frmline->words[i].width - frmline->words[i].min_width;
                    if (dw>0) {
                        totalSpace += dw;
                    }
                }
            }
            if ( totalSpace>0 ) {
                int delta = 0;
                for ( i=0; i<(int)frmline->word_count; i++ ) {
                    frmline->words[i].x -= delta;
                    if ( frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER ) {
                        int dw = frmline->words[i].width - frmline->words[i].min_width;
                        if (dw>0 && totalSpace>0) {
                            int n = dw * extraSpace / totalSpace;
                            totalSpace -= dw;
                            extraSpace -= n;
                            delta += n;
                            frmline->width -= n;
                        }
                    }
                }
            }
        }
        else if ( alignment==LTEXT_ALIGN_LEFT ) {
            // no additional alignment necessary
        }
        else if ( alignment==LTEXT_ALIGN_CENTER ) {
            frmline->x += extra_width / 2;
        }
        else if ( alignment==LTEXT_ALIGN_RIGHT ) {
            frmline->x += extra_width;
        }
        else {
            // LTEXT_ALIGN_WIDTH
            if ( extra_width > 0 ) {
                // distribute additional space
                int extraSpace = extra_width;
                int addSpacePoints = 0;
                int i;
                for ( i=0; i<(int)frmline->word_count-1; i++ ) {
                    if ( frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER )
                        addSpacePoints++;
                }
                if ( addSpacePoints>0 ) {
                    int addSpaceDiv = extraSpace / addSpacePoints;
                    int addSpaceMod = extraSpace % addSpacePoints;
                    int delta = 0;
                    for ( i=0; i<(int)frmline->word_count; i++ ) {
                        frmline->words[i].x += delta;
                        if ( frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER ) {
                            delta += addSpaceDiv;
                            if ( addSpaceMod>0 ) {
                                addSpaceMod--;
                                delta++;
                            }
                        }
                    }
                    frmline->width += extraSpace;
                }
            }
        }
        if ( hasInlineBoxes ) {
            // Now that we have the final x of each word, we can update
            // the RenderRectAccessor x/y of each word that is a inlineBox
            // (needed to correctly draw highlighted text in them).
            for ( int i=0; i<frmline->word_count; i++ ) {
                if ( frmline->words[i].flags & LTEXT_WORD_IS_INLINE_BOX ) {
                    formatted_word_t * word = &frmline->words[i];
                    src_text_fragment_t * srcline = &m_pbuffer->srctext[word->src_text_index];
                    ldomNode * node = (ldomNode *) srcline->object;
                    RenderRectAccessor fmt( node );
                    fmt.setX( frmline->x + word->x );
                    fmt.setY( frmline->y + frmline->baseline - word->o.baseline + word->y );
                }
            }
        }
    }

    /// split line into words, add space for width alignment
    void addLine( int start, int end, int x, src_text_fragment_t * para, int interval, bool first, bool last, bool preFormattedOnly, bool needReduceSpace, bool isLastPara )
    {
        // Note: provided 'interval' is no more used
        int maxWidth = getCurrentLineWidth();
        int rightIndent = 0;
        if ( m_para_dir_is_rtl ) {
            rightIndent = x;
            maxWidth -= x; // put x/first char indent on the right: reduce width
            x = getCurrentLineX(); // use shift induced by left floats
        }
        else {
            x += getCurrentLineX(); // add shift induced by left floats
        }
        //int w0 = start>0 ? m_widths[start-1] : 0;
        int align = para->flags & LTEXT_FLAG_NEWLINE;
        TR("addLine(%d, %d) y=%d  align=%d", start, end, m_y, align);
        // printf("addLine(%d, %d) y=%d  align=%d maxWidth=%d\n", start, end, m_y, align, maxWidth);

        // For some reason, text_align_last inheritance is not ensured in lvrend.cpp,
        // may be to be able to kill justification for the last (or a single) line as
        // easily as what follows below.
        // Here, text_align_last = 0 when it has not explicitely been set by the style
        // of the erm_final node.
        int text_align_last = (para->flags >> LTEXT_LAST_LINE_ALIGN_SHIFT) & LTEXT_FLAG_NEWLINE;
        if ( last && !first && align==LTEXT_ALIGN_WIDTH && text_align_last!=0 )
            align = text_align_last;
        else if ( align==LTEXT_ALIGN_WIDTH && last ) {
            // text-align-last: not specified, justification is in use, and this line
            // is the last (or a single line): align it to the left.
            align = LTEXT_ALIGN_LEFT;
            // Unless fribidi detected this paragraph is RTL: align it to the right
            if ( m_para_dir_is_rtl )
                align = LTEXT_ALIGN_RIGHT;
        }
        if ( preFormattedOnly || !align )
            align = LTEXT_ALIGN_LEFT;

        // Note: in the code and comments, all these mean the same thing:
        // visual alignment enabled, floating punctuation, hanging punctuation
        bool visualAlignmentEnabled = gFlgFloatingPunctuationEnabled!=0 && (align == LTEXT_ALIGN_WIDTH || align == LTEXT_ALIGN_RIGHT ||align==LTEXT_ALIGN_LEFT);

        bool splitBySpaces = (align == LTEXT_ALIGN_WIDTH) || needReduceSpace; // always true with current code

        if ( last && !first ) {
            int last_align = (para->flags>>16) & LTEXT_FLAG_NEWLINE;
            if ( last_align )
                align = last_align;
        }

        bool trustDirection = false;
        bool lineIsBidi = false;
        #if (USE_FRIBIDI==1)
        trustDirection = true;
        bool restore_last_width = false;
        int last_width_to_restore;
        if (m_has_bidi) {
            // We don't want to mess too much with the follow up code, so we
            // do the following, which might be expensive for full RTL documents:
            // we just reorder all chars, flags, width and references to
            // the original nodes, according to how fribidi decides the visual
            // order of chars should be.
            // We can mess with the m_* arrays (the range that spans the current
            // line) as they won't be used anymore after this function.
            // Except for the width of the last char (that we may modify
            // while zeroing the widths of collapsed spaces) that will be
            // used as the starting width of next line. We'll restore it
            // when done with this line.
            last_width_to_restore = m_widths[end-1];
            restore_last_width = true;

            // From fribidi documentation:
            // fribidi_reorder_line() reorders the characters in a line of text
            // from logical to final visual order. Note:
            // - the embedding levels may change a bit
            // - the bidi types and embedding levels are not reordered
            // - last parameter is a map of string indices which is reordered to
            //   reflect where each glyph ends up
            //
            // For re-ordering, we need some temporary buffers.
            // We use static buffers, and don't bother with dynamic buffers
            // in case we would overflow the static buffers.
            // (4096, if some glyphs spans 4 composing unicode codepoints, would
            // make 1000 glyphs, which with a small font of width 4px, would
            // allow them to be displayed on a 4000px screen.
            // Increase that if not enough.)
            #define MAX_LINE_SIZE 4096
            if ( end-start > MAX_LINE_SIZE ) {
                // Show a warning and truncate to avoid a segfault.
                printf("CRE WARNING: bidi processing line overflow (%d > %d)\n", end-start, MAX_LINE_SIZE);
                end = start + MAX_LINE_SIZE;
            }
            static lChar16 bidi_tmp_text[MAX_LINE_SIZE];
            static lUInt16 bidi_tmp_flags[MAX_LINE_SIZE];
            static src_text_fragment_t * bidi_tmp_srcs[MAX_LINE_SIZE];
            static lUInt16 bidi_tmp_charindex[MAX_LINE_SIZE];
            static int     bidi_tmp_widths[MAX_LINE_SIZE];
            // Map of string indices which is reordered to reflect where each
            // glyph ends up. Note that fribidi will access it starting
            // from 0 (and not from 'start'): this would need us to allocate
            // it the size of the full m_text (instead of MAX_LINE_SIZE)!
            // But we can trick that by providing a fake start address,
            // shifted by 'start' (which is ugly and could cause a segfault
            // if some other part than [start:end] would be accessed, but
            // we know fribid doesn't - by contract as it shouldn't reorder
            // any other part except between start:end).
            static FriBidiStrIndex bidi_indices_map[MAX_LINE_SIZE];
            for (int i=start; i<end; i++) {
                bidi_indices_map[i-start] = i;
            }
            FriBidiStrIndex * _virtual_bidi_indices_map = bidi_indices_map - start;

            FriBidiFlags bidi_flags = 0;
            // We're not using bidi_flags=FRIBIDI_FLAG_REORDER_NSM (which is mostly
            // needed for code drawing the resulting reordered result) as it would
            // mess with our indices map, and the final result would be messy.
            // (Looks like even Freetype drawing does not need the BIDI rule
            // L3 (combining-marks-must-come-after-base-char) as it draws finely
            // RTL when we draw the combining marks before base char.)
            int max_level = fribidi_reorder_line(bidi_flags, m_bidi_ctypes, end-start, start,
                                m_para_bidi_type, m_bidi_levels, NULL, _virtual_bidi_indices_map);
            if (max_level > 1) {
                lineIsBidi = true;
                // bidi_tmp_* will contain things in the visual order, from which
                // we will make words (exactly as if it had been LTR that way)
                for (int i=start; i<end; i++) {
                    int bidx = i - start;
                    int j = bidi_indices_map[bidx]; // original indice in m_text, m_flags, m_bidi_levels...
                    bidi_tmp_text[bidx] = m_text[j];
                    bidi_tmp_srcs[bidx] = m_srcs[j];
                    bidi_tmp_charindex[bidx] = m_charindex[j];
                    // Add a flag if this char is part of a RTL segment
                    if ( FRIBIDI_LEVEL_IS_RTL( m_bidi_levels[j] ) )
                        m_flags[j] |= LCHAR_IS_RTL;
                    else
                        m_flags[j] &= ~LCHAR_IS_RTL;
                    bidi_tmp_flags[bidx] = m_flags[j];
                    // bidi_tmp_widths will contains each individual char width, that we
                    // compute from the accumulated width. We'll make it a new
                    // accumulated width in next loop
                    bidi_tmp_widths[bidx] = m_widths[j] - (j > 0 ? m_widths[j-1] : 0);
                    // todo: we should probably also need to update/move the
                    // LCHAR_IS_CLUSTER_TAIL flag... haven't really checked
                    // (might be easier or harder due to the fact that we
                    // don't use FRIBIDI_FLAG_REORDER_NSM?)
                }

                // It looks like fribidi is quite good enough at taking
                // care of collapsed spaces! No real extra space seen
                // when testing, except at start and end.
                // Anyway, we handle collapsed spaces and their widths
                // as we would expect them to be with LTR text just out
                // of copyText().
                bool prev_was_space = true; // start as true to make leading spaces collapsed
                int prev_non_collapsed_space = -1;
                int w = start > 0 ? m_widths[start-1] : 0;
                for (int i=start; i<end; i++) {
                    int bidx = i - start;
                    m_text[i] = bidi_tmp_text[bidx];
                    m_flags[i] = bidi_tmp_flags[bidx];
                    m_srcs[i] = bidi_tmp_srcs[bidx];
                    m_charindex[i] = bidi_tmp_charindex[bidx];
                    // Handle consecutive spaces at start and in the text
                    if ( (m_srcs[i]->flags & LTEXT_FLAG_PREFORMATTED) ) {
                        prev_was_space = false;
                        prev_non_collapsed_space = -1;
                        m_flags[i] &= ~LCHAR_IS_COLLAPSED_SPACE;
                    }
                    else {
                        if ( m_text[i] == ' ' ) {
                            if (prev_was_space) {
                                m_flags[i] |= LCHAR_IS_COLLAPSED_SPACE;
                                // Put this (now collapsed, but possibly previously non-collapsed)
                                // space width on the preceeding now non-collapsed space
                                int w_orig = bidi_tmp_widths[bidx];
                                bidi_tmp_widths[bidx] = 0;
                                if ( prev_non_collapsed_space >= 0 ) {
                                    m_widths[prev_non_collapsed_space] += w_orig;
                                    w += w_orig;
                                }
                            }
                            else {
                                m_flags[i] &= ~LCHAR_IS_COLLAPSED_SPACE;
                                prev_was_space = true;
                                prev_non_collapsed_space = i;
                            }
                        }
                        else {
                            prev_was_space = false;
                            prev_non_collapsed_space = -1;
                            m_flags[i] &= ~LCHAR_IS_COLLAPSED_SPACE;
                        }
                    }
                    w += bidi_tmp_widths[bidx];
                    m_widths[i] = w;
                    // printf("%x:f%x,w%d ", m_text[i], m_flags[i], m_widths[i]);
                }
                // Also flag as collapsed the trailing spaces on the reordered line
                if (prev_non_collapsed_space >= 0) {
                    int prev_width = prev_non_collapsed_space > 0 ? m_widths[prev_non_collapsed_space-1] :0 ;
                    for (int i=prev_non_collapsed_space; i<end; i++) {
                        m_flags[i] |= LCHAR_IS_COLLAPSED_SPACE;
                        m_widths[i] = prev_width;
                    }
                }

            }
            // Note: we reordered m_text and others, which are used from now on only
            // to properly split words. When drawing the text, these are no more used,
            // and the string is taken directly from the copy of the text node string
            // stored as src_text_fragment_t->t.text, so FreeType and HarfBuzz will
            // get the text in logical order (as HarfBuzz expects it).
            // Also, when parens/brackets are involved in RTL text, only HarfBuzz
            // will correctly mirror them. When not using Harfbuzz, we'll mirror
            // mirrorable chars below when a word is RTL.
        }
        #endif

        // Note: not certain why or how useful this lastnonspace (used below) is.
        int lastnonspace = 0;
        if ( splitBySpaces || align==LTEXT_ALIGN_WIDTH ) { // always true with current code
            for ( int k=end-1; k>=start; k-- ) {
                // Also not certain if we should skip floats or LCHAR_IS_OBJECT
                if ( !(m_flags[k] & LCHAR_IS_SPACE) ) {
                    lastnonspace = k;
                    break;
                }
            }
        }

        formatted_line_t * frmline =  lvtextAddFormattedLine( m_pbuffer );
        frmline->y = m_y;
        frmline->x = x;
        // This new line starts with a minimal height and baseline, as set from the
        // paragraph parent node (by lvrend.cpp renderFinalBlock()). These may get
        // increased if some inline elements need more, but not decreased.
        frmline->height = m_pbuffer->strut_height;
        frmline->baseline = m_pbuffer->strut_baseline;
        if (m_has_ongoing_float)
            // Avoid page split when some float that started on a previous line
            // still spans this line
            frmline->flags |= LTEXT_LINE_SPLIT_AVOID_BEFORE;

        if ( preFormattedOnly && (start == end) ) {
            // Specific for preformatted text when consecutive \n\n:
            // start == end, and we have no source text to point to,
            // but we should draw en empty line (we can't just simply
            // increase m_y and m_pbuffer->height, we need to have
            // a frmline as Draw() loops thru these lines - a frmline
            // with no word will do).
            src_text_fragment_t * srcline = m_srcs[start];
            if (srcline->interval > 0) { // should always be the case
                if (srcline->interval > frmline->height) // keep strut_height if greater
                    frmline->height = srcline->interval;
            }
            else { // fall back to line-height: normal
                LVFont * font = (LVFont*)srcline->t.font;
                frmline->height = font->getHeight();
            }
            m_y += frmline->height;
            m_pbuffer->height = m_y;
            return;
        }

        src_text_fragment_t * lastSrc = m_srcs[start];

        // We can just skip FLOATs in addLine(), as they were taken
        // care of in processParagraph() to just reduce the available width
        // So skip floats at start:
        while (lastSrc && (lastSrc->flags & LTEXT_SRC_IS_FLOAT) ) {
            start++;
            lastSrc = m_srcs[start];
        }
        if (!lastSrc) { // nothing but floats
            // A line has already been added: just make it zero-height.
            frmline->height = 0;
            return;
        }

        if ( lineIsBidi ) {
            // Flag that line, so createXPointer() and getRect() know it's not
            // a regular one and can't assume words and text nodes are linear.
            frmline->flags |= LTEXT_LINE_IS_BIDI;
        }
        if ( m_para_dir_is_rtl ) {
            frmline->flags |= LTEXT_LINE_PARA_IS_RTL;
            // Not used yet, but might be useful (we may have a bidi line
            // in a LTR paragraph).
        }

        // Some words vertical-align positionning might need to be fixed
        // only once the whole line has been laid out
        bool delayed_valign_computation = false;
        // alignLine() will have more work to do if we have inlineBox elements
        bool has_inline_boxes = false;

        // Ignore space at start of line (this rarely happens, as line
        // splitting discards the space on which a split is made - but it
        // can happen in other rare wrap cases like lastDeprecatedWrap)
        if ( (m_flags[start] & LCHAR_IS_SPACE) && !(lastSrc->flags & LTEXT_FLAG_PREFORMATTED) ) {
            // But do it only if we're going to stay in same text node (if not
            // the space may have some reason - there's sometimes a no-break-space
            // before an image)
            if (start < end-1 && m_srcs[start+1] == m_srcs[start]) {
                start++;
                lastSrc = m_srcs[start];
            }
        }
        int wstart = start;
        bool lastIsSpace = false;
        bool lastWord = false;
        //bool isObject = false;
        bool isSpace = false;
        //bool nextIsSpace = false;
        bool space = false;
        // Bidi
        bool lastIsRTL = false;
        bool isRTL = false;
        bool bidiLogicalIndicesShift = false;
        // Unicode script change
        bool scriptChanged = false;
        #if (USE_HARFBUZZ==1)
            lUInt32 prevScript = HB_SCRIPT_COMMON;
            hb_unicode_funcs_t* _hb_unicode_funcs = hb_unicode_funcs_get_default();
        #endif
        // Ignorables
        bool isToIgnore = false;
        for ( int i=start; i<=end; i++ ) { // loop thru each char
            src_text_fragment_t * newSrc = i<end ? m_srcs[i] : NULL;
            if ( i<end ) {
                //isObject = (m_flags[i] & LCHAR_IS_OBJECT)!=0;
                isSpace = (m_flags[i] & LCHAR_IS_SPACE)!=0; // current char is a space
                //nextIsSpace = i<end-1 && (m_flags[i+1] & LCHAR_IS_SPACE);
                space = splitBySpaces && lastIsSpace && !isSpace && i<=lastnonspace;
                // /\ previous char was a space, current char is not a space
                //     Note: last check was initially "&& i<lastnonspace", but with
                //     this, a line containing "thing inside a " (ending with a
                //     1-char word) would be considered only 2 words ("thing" and
                //     "inside a") and, when justify'ing text, space would not be
                //     distributed between "inside" and "a"...
                //     Not really sure what's the purpose of this last test...
                #if (USE_HARFBUZZ==1)
                    // To be done only when we met multiple scripts in a same paragraph
                    // while measuring (which we checked only when using Harfbuzz kerning)
                    if ( m_has_multiple_scripts && !(m_flags[i] & LCHAR_IS_OBJECT) ) {
                        hb_script_t script = hb_unicode_script(_hb_unicode_funcs, m_text[i]);
                        if ( script != HB_SCRIPT_COMMON && script != HB_SCRIPT_INHERITED && script != HB_SCRIPT_UNKNOWN ) {
                            if ( prevScript != HB_SCRIPT_COMMON && script != prevScript ) {
                                scriptChanged = true;
                            }
                            prevScript = script;
                        }
                    }
                #endif
                isToIgnore = m_flags[i] & LCHAR_IS_TO_IGNORE;
                isRTL = m_flags[i] & LCHAR_IS_RTL;
                bidiLogicalIndicesShift = false;
                if ( lineIsBidi && isRTL == lastIsRTL && i > 0) {
                    // The bidi algo may have reordered logical chars, and
                    // put side by side same-direction chars that where
                    // not consecutive in the original logical text.
                    // We need to make a new word when we see these
                    // reordered indices shifting by more than +/- 1,
                    // as when drawing the words, we'll use the source
                    // text nodes' logical text.
                    if ( isRTL ) { // indices should be decreasing by 1
                        if ( m_charindex[i] != m_charindex[i-1] - 1 )
                            bidiLogicalIndicesShift = true;
                    }
                    else { // LTR: indices should be increasing by 1
                        if ( m_charindex[i] != m_charindex[i-1] + 1 )
                            bidiLogicalIndicesShift = true;
                    }
                    // (m_charindex[i-1] might be bad when i-1 is from
                    // another text node, or an object - but no need
                    // for checking that as it will have triggered
                    // another condition for making a word.)
                }
            }
            else {
                lastWord = true;
            }

            // This loop goes thru each char, and create a new word when it meets:
            // - a non-space char that follows a space (this non-space char will be
            //   the first char of next word).
            // - a char from a different text node (eg: "<span>first</span>next")
            // - a CJK char (whether or not preceded by a space): each becomes a word
            // - the end of text, which makes the last word
            //
            // It so grabs all spaces (0 or 1 with our XML parser) following
            // the current real word, and includes it in the word. So a word
            // includes its following space if any, but should not start with
            // a space. The trailing space is needed for the word processing
            // code below to properly set flags and guess the amount of spaces
            // that can be increased or reduced for proper alignment.
            // Also, these words being then stacked to each other to build the
            // line, the ending space should be kept to be drawn and seen
            // between each word (some words may not be separated by space when
            // from different text nodes or CJK).
            // Note: a "word" in our current context is just a unit of text that
            // should be rendered together, and can be moved on the x-axis for
            // alignment purpose (the 2 french words "qu'autrefois" make a
            // single "word" here, the single word "quelconque", if hyphentaded
            // as "quel-conque" will make one "word" on this line and another
            // "word" on the next line.
            //
            // In a sequence of collapsing spaces, only the first was kept as
            // a LCHAR_IS_SPACE. The following ones were flagged as
            // LCHAR_IS_COLLAPSED_SPACE, and thus are not LCHAR_IS_SPACE.
            // With the algorithm described just above, these collapsed spaces
            // can then only be at the start of a word.
            // Their calculated width has been made to 0, but the drawing code
            // (LFormattedText::Draw() below) will use the original srctext text
            // to draw the string: we can't override this original text (it is
            // made read-only with the use of 'const') to replace the space with
            // a zero-width char (which may not be zero-width in a monospace font).
            // So, we need to adjust each word start index to get rid of the
            // collapsed spaces.
            //
            // Note: if we were to make a space between 2 CJY chars a collapsed
            // space, we would have it at the end of each word, which may
            // be fine without additional work needed (not verified):
            // having a zero-width, it would not change the width of the
            // CJKchar/word, and would not affect the next CJKchar/word position.
            // It would be drawn as a space, but the next CJKchar would override
            // it when it is drawn next.

            if ( i>wstart && (   newSrc!=lastSrc
                              || space
                              || lastWord
                              || isCJKIdeograph(m_text[i])
                              || isRTL != lastIsRTL
                              || bidiLogicalIndicesShift
                              || scriptChanged
                              || isToIgnore
                             ) ) {
                // New HTML source node, space met just before, last word, or CJK char:
                // create and add new word with chars from wstart to i-1

                #if (USE_HARFBUZZ==1)
                    if ( m_has_multiple_scripts ) {
                        // Reset as next segment can start with any script
                        prevScript = HB_SCRIPT_COMMON;
                        scriptChanged = false;
                    }
                #endif

                // Remove any collapsed space at start of word: they
                // may have a zero width and not influence positionning,
                // but they will be drawn as a space by Draw(). We need
                // to increment the start index into the src_text_fragment_t
                // for Draw() to start rendering the text from this position.
                // Also skip floating nodes and chars flagged as to be ignored.
                while (wstart < i) {
                    if ( !(m_flags[wstart] & LCHAR_IS_COLLAPSED_SPACE) &&
                         !(m_flags[wstart] & LCHAR_IS_TO_IGNORE) &&
                            !(m_srcs[wstart]->flags & LTEXT_SRC_IS_FLOAT) )
                        break;
                    // printf("_"); // to see when we remove one, before the TR() below
                    wstart++;
                }
                if (wstart == i) { // word is only collapsed spaces
                    // No need to create it.
                    // Except if it is the last word, and we have not yet added any:
                    // we need a word for the line to have a height (frmline->height)
                    // so that the following line is one line below the empty line we
                    // made (eg, when <br/><br/>)
                    // However, we don't do that if it would be the last empty line in
                    // the last paragraph (paragraphs here are just sections of the final
                    // block cut by <BR>): most browsers don't display the line break
                    // implied by the BR when we have: "<div>some text<br/> </div>more text"
                    // or "<div>some text<br/> <span> </span> </div>more text".
                    if (lastWord && frmline->word_count == 0) {
                        if (!isLastPara) {
                            wstart--; // make a single word with a single collapsed space
                            if (m_flags[wstart] & LCHAR_IS_TO_IGNORE) {
                                // In this (edgy) case, we would be rendering this char we
                                // want to ignore.
                                // This is a bit hacky, but no other solution: just
                                // replace that ignorable char with a space in the
                                // src text
                                *((lChar16 *) (m_srcs[wstart]->t.text + m_charindex[wstart])) = L' ';
                            }
                        }
                        else { // Last or single para with no word
                            // A line has already been added: just make
                            // it zero height.
                            frmline->height = 0;
                            frmline->baseline = 0;
                            continue;
                            // We'll then just exit the loop as we are lastWord
                        }
                    }
                    else {
                        // no word made, get ready for next loop
                        lastSrc = newSrc;
                        lastIsSpace = isSpace;
                        lastIsRTL = isRTL;
                        continue;
                    }
                }

                formatted_word_t * word = lvtextAddFormattedWord(frmline);
                src_text_fragment_t * srcline = m_srcs[wstart];
                // This LTEXT_VALIGN_ flag is now only of use with objects (images)
                int vertical_align_flag = srcline->flags & LTEXT_VALIGN_MASK;
                // These will be used later to adjust the main line baseline and height:
                int top_to_baseline; // distance from this word top to its own baseline (formerly named 'b')
                int baseline_to_bottom; // descender below baseline for this word (formerly named 'h')
                word->src_text_index = m_srcs[wstart]->index;

                // For each word, we'll have to check and adjust line height and baseline,
                // except when LTEXT_VALIGN_TOP and LTEXT_VALIGN_BOTTOM where it has to
                // be delayed until the full line is laid out. Until that, we store some
                // info into word->_top_to_baseline and word->_baseline_to_bottom.
                bool adjust_line_box = true;
                // We will make sure elements with "-cr-hint: strut-confined"
                // do not change the strut baseline and height
                bool strut_confined = (lastSrc->flags & LTEXT_STRUT_CONFINED) && m_allow_strut_confinning;

                if ( lastSrc->flags & LTEXT_SRC_IS_OBJECT ) {
                    // object: image or inline-block box (floats have been skipped above)
                    word->distinct_glyphs = 0;
                    word->x = frmline->width;
                    word->width = lastSrc->o.width;
                    word->min_width = word->width;
                    word->o.height = lastSrc->o.height;
                    if ( lastSrc->flags & LTEXT_SRC_IS_INLINE_BOX ) { // inline-block
                        has_inline_boxes = true;
                        word->flags = LTEXT_WORD_IS_INLINE_BOX;
                        // For inline-block boxes, the baseline may not be the bottom; it has
                        // been computed in measureText().
                        word->o.baseline = lastSrc->o.baseline;
                        top_to_baseline = word->o.baseline;
                        baseline_to_bottom = word->o.height - word->o.baseline;
                        // We can't really ensure strut_confined with inline-block boxes,
                        // or we could miss content (it would be overwritten by next lines)
                    }
                    else { // image
                        word->flags = LTEXT_WORD_IS_OBJECT;
                        word->o.height = lastSrc->o.height;
                        // Resize image so it fits in our available width
                        int width = lastSrc->o.width;
                        int height = lastSrc->o.height;
                        // Negative width and height mean the value is a % (of our final block width)
                        width = width<0 ? (-width * (m_pbuffer->width - x) / 100) : width;
                        height = height<0 ? (-height * (m_pbuffer->width-x) / 100) : height;
                        if ( strut_confined && height > m_pbuffer->strut_height ) {
                            // Don't make image taller than initial strut height.
                            // (We could have checked height against frmline->height (which may
                            // be larger than m_pbuffer->strut_height if not all elements have
                            // "-cr-hint: strut-confined"), but in processParagraph() we checked
                            // against m_pbuffer->strut_height, so keep doing that to not
                            // have this line width different.
                            width = width * m_pbuffer->strut_height / height; // keep aspect ratio
                            height = frmline->height;
                        }
                        // todo: adjust m_max_img_height with this image valign_dy/vertical_align_flag
                        resizeImage(width, height, m_pbuffer->width - x, m_max_img_height, m_length>1);
                            // Note: it can happen with a standalone image in a small container
                            // where text-indent is greater than width, that 'm_pbuffer->width - x'
                            // can be negative. We could cap it to zero and resize the image to 0,
                            // but let it be shown un-resized, possibly overflowing or overriding
                            // other content.
                        word->width = width;
                        word->o.height = height;
                        // Per specs, the baseline is the bottom of the image
                        top_to_baseline = word->o.height;
                        baseline_to_bottom = 0;
                    }

                    // srcline->valign_dy sets the baseline, except in a few specific cases
                    // word->y has to be set to where the baseline should be
                    // For vertical-align: top or bottom, delay computation as we need to
                    // know the final frmline height and baseline, which might change
                    // with upcoming words.
                    if ( vertical_align_flag == LTEXT_VALIGN_TOP ) {
                        // was (before we delayed computation):
                        // word->y = top_to_baseline - frmline->baseline;
                        adjust_line_box = false;
                        delayed_valign_computation = true;
                        word->flags |= LTEXT_WORD_VALIGN_TOP;
                        if ( strut_confined )
                            word->flags |= LTEXT_WORD_STRUT_CONFINED;
                        word->_top_to_baseline = top_to_baseline;
                        word->_baseline_to_bottom = baseline_to_bottom;
                        word->y = top_to_baseline;
                    }
                    else if ( vertical_align_flag == LTEXT_VALIGN_BOTTOM ) {
                        // was (before we delayed computation):
                        // word->y = frmline->height - frmline->baseline;
                        adjust_line_box = false;
                        delayed_valign_computation = true;
                        word->flags |= LTEXT_WORD_VALIGN_BOTTOM;
                        if ( strut_confined )
                            word->flags |= LTEXT_WORD_STRUT_CONFINED;
                        word->_top_to_baseline = top_to_baseline;
                        word->_baseline_to_bottom = baseline_to_bottom;
                        word->y = - baseline_to_bottom;
                    }
                    else if ( vertical_align_flag == LTEXT_VALIGN_TEXT_TOP ) {
                        // srcline->valign_dy has been set to where top of image or box should be
                        word->y = srcline->valign_dy + top_to_baseline;
                    }
                    else if ( vertical_align_flag == LTEXT_VALIGN_TEXT_BOTTOM ) {
                        // srcline->valign_dy has been set to where bottom of image or box should be
                        word->y = srcline->valign_dy - baseline_to_bottom;
                    }
                    else if ( vertical_align_flag == LTEXT_VALIGN_MIDDLE ) {
                        // srcline->valign_dy has been set to where the middle of image or box should be
                        word->y = srcline->valign_dy - (top_to_baseline + baseline_to_bottom)/2 + top_to_baseline;
                    }
                    else { // otherwise, align baseline according to valign_dy (computed in lvrend.cpp)
                        word->y = srcline->valign_dy;
                    }

                    // Inline image or inline-block: ensure any "page-break-before/after: avoid"
                    // specified on them (the specs say those apply to "block-level elements
                    // in the normal flow of the root element. User agents may also apply it
                    // to other elements like table-row elements", so it's mostly assumed that
                    // they won't apply on inline elements and we'll never meet them - but as
                    // it doesn't say we should not, let's ensure them if provided - and
                    // only "avoid" as it may have some purpose to stick a full-width image
                    // or inline-block to the previous or next line).
                    ldomNode * node = (ldomNode *) lastSrc->object;
                    if ( node && lastSrc->flags & LTEXT_SRC_IS_INLINE_BOX ) {
                        // We have not propagated page_break styles from the original
                        // inline-block to its inlineBox wrapper
                        node = node->getChildNode(0);
                    }
                    if ( node ) {
                        css_style_ref_t style = node->getStyle();
                        if ( style->page_break_before == css_pb_avoid )
                            frmline->flags |= LTEXT_LINE_SPLIT_AVOID_BEFORE;
                        if ( style->page_break_after == css_pb_avoid )
                            frmline->flags |= LTEXT_LINE_SPLIT_AVOID_AFTER;
                    }
                }
                else {
                    // word
                    // wstart points to the previous first non-space char
                    // i points to a non-space char that will be in next word
                    // i-1 may be a space, or not (when different html tag/text nodes stuck to each other)
                    src_text_fragment_t * srcline = m_srcs[wstart];
                    LVFont * font = (LVFont*)srcline->t.font;
                    word->flags = 0;

                    int vertical_align_flag = srcline->flags & LTEXT_VALIGN_MASK;
                    int line_height = srcline->interval;
                    int fh = font->getHeight();
                    if ( strut_confined && line_height > m_pbuffer->strut_height ) {
                        // If we'll be confining text inside the strut, get rid of any
                        // excessive line-height for the following computations).
                        // But we should keep it at least fh so drawn text doesn't
                        // overflow the box we'll try to confine into the strut.
                        line_height = fh > m_pbuffer->strut_height ? fh : m_pbuffer->strut_height;
                    }
                    // As we do only +/- arithmetic, the following values being negative should be fine.
                    // Accounts for line-height (adds what most documentation calls half-leading to top
                    // and to bottom  - note that "leading" is a typography term referring to "lead" the
                    // metal, and not to lead/leader/head/header - so the half use for bottom should not
                    // be called half-tailing :):
                    int half_leading = (line_height - fh) / 2;
                    int half_leading_bottom = line_height - fh - half_leading;
                    top_to_baseline = font->getBaseline() + half_leading;
                    baseline_to_bottom = line_height - top_to_baseline;
                    // For vertical-align: top or bottom, delay computation as we need to
                    // know the final frmline height and baseline, which might change
                    // with upcoming words.
                    if ( vertical_align_flag == LTEXT_VALIGN_TOP ) {
                        // was (before we delayed computation):
                        // word->y = font->getBaseline() - frmline->baseline + half_leading;
                        adjust_line_box = false;
                        delayed_valign_computation = true;
                        word->flags |= LTEXT_WORD_VALIGN_TOP;
                        if ( strut_confined )
                            word->flags |= LTEXT_WORD_STRUT_CONFINED;
                        word->_top_to_baseline = top_to_baseline;
                        word->_baseline_to_bottom = baseline_to_bottom;
                        word->y = font->getBaseline() + half_leading;
                    }
                    else if ( vertical_align_flag == LTEXT_VALIGN_BOTTOM ) {
                        // was (before we delayed computation):
                        // word->y = frmline->height - fh + font->getBaseline() - frmline->baseline - half_leading;
                        adjust_line_box = false;
                        delayed_valign_computation = true;
                        word->flags |= LTEXT_WORD_VALIGN_BOTTOM;
                        if ( strut_confined )
                            word->flags |= LTEXT_WORD_STRUT_CONFINED;
                        word->_top_to_baseline = top_to_baseline;
                        word->_baseline_to_bottom = baseline_to_bottom;
                        word->y = - fh + font->getBaseline() - half_leading_bottom;
                    }
                    else {
                        // For others, vertical-align computation is done in lvrend.cpp renderFinalBlock()
                        word->y = srcline->valign_dy;
                    }
                    // printf("baseline_to_bottom=%d top_to_baseline=%d word->y=%d txt=|%s|\n", baseline_to_bottom,
                    //   top_to_baseline, word->y, UnicodeToLocal(lString16(srcline->t.text, srcline->t.len)).c_str());

                    // For Harfbuzz, which may shape differently words at start or end of paragraph
                    if (first && frmline->word_count == 1) // first line of paragraph + first word of line
                        word->flags |= LTEXT_WORD_BEGINS_PARAGRAPH;
                    if (last && lastWord) // last line of paragraph + last word of line
                        word->flags |= LTEXT_WORD_ENDS_PARAGRAPH;

                    if ( trustDirection)
                        word->flags |= LTEXT_WORD_DIRECTION_KNOWN;
                    if ( !m_has_bidi ) {
                        // No bidi, everything is linear
                        word->t.start = m_charindex[wstart];
                        word->t.len = i - wstart;
                    }
                    else if ( m_flags[wstart] & LCHAR_IS_RTL ) {
                        // Bidi and first char RTL.
                        // As we split on bidi level change, the full word is RTL.
                        // As we split on src text fragment, we are sure all chars
                        // are in the same text node.
                        // charindex may have been reordered, and may not be sync'ed with wstart/i-1,
                        // but it is linearly decreasing between i-1 and wstart
                        word->t.start = m_charindex[i-1];
                        word->t.len = m_charindex[wstart] - m_charindex[i-1] + 1;
                        word->flags |= LTEXT_WORD_DIRECTION_IS_RTL; // Draw glyphs in reverse order
                        #if (USE_FRIBIDI==1)
                        // If not using Harfbuzz, procede to mirror parens & al (don't
                        // do that if Harfbuzz is used, as it does that by itself, and
                        // would mirror back our mirrored chars!)
                        if ( font->getShapingMode() != SHAPING_MODE_HARFBUZZ) {
                            lChar16 * str = (lChar16*)(srcline->t.text + word->t.start);
                            FriBidiChar mirror;
                            for (int i=0; i < word->t.len; i++) {
                                if ( fribidi_get_mirror_char( (FriBidiChar)(str[i]), &mirror) )
                                    str[i] = (lChar16)mirror;
                            }
                        }
                        #endif
                    }
                    else {
                        // Bidi and first char LTR. Same comments as above, except for last one:
                        // it is linearly increasing between wstart and i-1
                        word->t.start = m_charindex[wstart];
                        word->t.len = m_charindex[i-1] + 1 - m_charindex[wstart];
                    }

                    // We need to compute how many glyphs can have letter_spacing added, that
                    // might be done in alignLine() (or not). We have to do it now even if
                    // not used, as we won't have that information anymore in alignLine().
                    word->added_letter_spacing = 0;
                    word->distinct_glyphs = word->t.len; // start with all chars are distinct glyphs
                    bool seen_non_space = false;
                    int tailing_spaces = 0;
                    for ( int j=i-1; j >= wstart; j-- ) {
                        if ( m_flags[j] & LCHAR_LOCKED_SPACING ) {
                            // A single char flagged with this makes the whole word non tweakable
                            word->distinct_glyphs = 0;
                            tailing_spaces = 0; // prevent tailing spaces correction
                            break;
                        }
                        if ( !seen_non_space && (m_flags[j] & LCHAR_IS_SPACE) ) {
                            // We'd rather not include the space that ends most words.
                            word->distinct_glyphs--;
                            // But some words can be made of a single space, that we'd rather
                            // not ignore when adjusting spacing.
                            tailing_spaces++;
                            continue;
                        }
                        seen_non_space = true;
                        if ( m_flags[j] & (LCHAR_IS_CLUSTER_TAIL|LCHAR_IS_COLLAPSED_SPACE|LCHAR_IS_TO_IGNORE) ) {
                            word->distinct_glyphs--;
                        }
                    }
                    if ( !seen_non_space && tailing_spaces ) {
                        word->distinct_glyphs += tailing_spaces;
                    }

                    word->x = frmline->width;
                    word->width = m_widths[i>0 ? i-1 : 0] - (wstart>0 ? m_widths[wstart-1] : 0);
                    word->min_width = word->width;
                    TR("addLine - word(%d, %d) x=%d (%d..%d)[%d] |%s|", wstart, i, frmline->width, wstart>0 ? m_widths[wstart-1] : 0, m_widths[i-1], word->width, LCSTR(lString16(m_text+wstart, i-wstart)));
//                    lChar16 lastch = m_text[i-1];
//                    if ( lastch==UNICODE_NO_BREAK_SPACE )
//                        CRLog::trace("last char is UNICODE_NO_BREAK_SPACE");
                    if ( m_flags[wstart] & LCHAR_IS_CLUSTER_TAIL ) {
                        // The start of this word is part of a ligature that started
                        // in a previous word: some hyphenation wrap happened on
                        // this ligature, which will not be rendered as such.
                        // We are the second part of the hyphenated word, and our first
                        // char(s) have a width of 0 (for being part of the ligature):
                        // we need to re-measure this half of the original word.
                        int new_width;
                        if ( measureWord(word, new_width) ) {
                            word->width = new_width;
                            word->min_width = word->width;
                        }
                    }
                    if ( m_flags[i-1] & LCHAR_ALLOW_HYPH_WRAP_AFTER ) {
                        if ( m_flags[i] & LCHAR_IS_CLUSTER_TAIL ) {
                            // The end of this word is part of a ligature that, because
                            // of hyphenation, has been splitted onto next word.
                            // We are the first part of the hyphenated word, and
                            // our last char(s) have been assigned the width of the
                            // ligature glyph, which will not be rendered as such:
                            // we need to re-measure this half of the original word.
                            int new_width;
                            if ( measureWord(word, new_width) ) {
                                word->width = new_width;
                            }
                        }
                        word->width += font->getHyphenWidth();
                        word->min_width = word->width;
                        word->flags |= LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER;
                    }
                    if ( m_flags[i-1] & LCHAR_IS_SPACE) { // Current word ends with a space
                        // Each word ending with a space (except in some conditions) can
                        // have its width reduced by a fraction of this space width or
                        // increased if needed (for text justification), so actually
                        // making that space larger or smaller.
                        bool can_adjust_width = true;
                        // Note: checking if the first word of first line is one of the
                        // common opening quotation marks or dashes is done in measureText(),
                        // to have it work also with BiDi/RTL text (checking that here
                        // would be too late, as reordering has been done).
                        if ( m_flags[i-1] & LCHAR_LOCKED_SPACING ) {
                            can_adjust_width = false;
                        }
                        else if ( word->t.len>=2 && i>=2 && m_text[i-1]==UNICODE_NO_BREAK_SPACE
                                                         && m_text[i-2]==UNICODE_NO_BREAK_SPACE ) {
                            // condition for double nbsp after run-in footnote title
                            can_adjust_width = false;
                            // (not sure what this one and the next are about)
                        }
                        else if ( i < m_length-1 && m_text[i]==UNICODE_NO_BREAK_SPACE
                                                 && m_text[i+1]==UNICODE_NO_BREAK_SPACE ) {
                            // condition for double nbsp after run-in footnote title
                            can_adjust_width = false;
                        }
                        if ( can_adjust_width ) {
                            word->flags |= LTEXT_WORD_CAN_ADD_SPACE_AFTER;
                            int dw = getMaxCondensedSpaceTruncation(i-1);
                            if (dw>0) {
                                word->min_width = word->width - dw;
                            }
                        }
                        if ( !visualAlignmentEnabled && lastWord ) {
                            // If last word of line, remove any trailing space from word's width
                            word->width = m_widths[i>1 ? i-2 : 0] - (wstart>0 ? m_widths[wstart-1] : 0);
                            word->min_width = word->width;
                        }
                    } else if ( frmline->word_count>1 && m_flags[wstart] & LCHAR_IS_SPACE ) {
                        // Current word starts with a space (looks like this should not happen):
                        // we can increase the space between previous word and this one if needed
                        //if ( word->t.len<2 || m_text[i-1]!=UNICODE_NO_BREAK_SPACE || m_text[i-2]!=UNICODE_NO_BREAK_SPACE)
//                        if ( m_text[wstart]==UNICODE_NO_BREAK_SPACE && m_text[wstart+1]==UNICODE_NO_BREAK_SPACE)
//                            CRLog::trace("Double nbsp text[-1]=%04x", m_text[wstart-1]);
//                        else
                        frmline->words[frmline->word_count-2].flags |= LTEXT_WORD_CAN_ADD_SPACE_AFTER;
                    } else if (frmline->word_count>1 && isCJKIdeograph(m_text[i])) {
                        // Current word is a CJK char: we can increase the space
                        // between previous word and this one if needed
                        frmline->words[frmline->word_count-2].flags |= LTEXT_WORD_CAN_ADD_SPACE_AFTER;
                    }
                    if ( m_flags[i-1] & LCHAR_ALLOW_WRAP_AFTER )
                        word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER; // not used anywhere
                    if ( word->t.start==0 && srcline->flags & LTEXT_IS_LINK )
                        word->flags |= LTEXT_WORD_IS_LINK_START; // for in-page footnotes

                    if ( visualAlignmentEnabled && lastWord ) { // if floating punctuation enabled
                        int endp = i-1;
                        int lastc = m_text[endp];
                        int wAlign = font->getVisualAligmentWidth();
                        word->width += wAlign/2;
                        while ( (m_flags[endp] & LCHAR_IS_SPACE) && endp>0 ) { // || lastc=='\r' || lastc=='\n'
                            word->width -= m_widths[endp] - m_widths[endp-1];
                            endp--;
                            lastc = m_text[endp];
                        }
                        // We reduce the word width from the hanging char width, so it's naturally pushed
                        // outside in the margin by the alignLine code
                        if ( word->flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER ) {
                            word->width -= font->getHyphenWidth(); // TODO: strange fix - need some other solution
                        }
                        else if ( lastc=='.' || lastc==',' || lastc=='!' || lastc==':' || lastc==';' || lastc=='?') {
                            FONT_GUARD
                            int w = font->getCharWidth(lastc);
                            TR("floating: %c w=%d", lastc, w);
                            if (frmline->width + w + wAlign + x >= maxWidth)
                                word->width -= w; //fix russian "?" at line end
                        }
                        else if ( lastc==0x2019 || lastc==0x201d ||   // ’ ” right quotation marks
                                  lastc==0x3001 || lastc==0x3002 ||   // 、 。 ideographic comma and full stop
                                  lastc==0x300d || lastc==0x300f ||   // 」 』 ideographic right bracket
                                  lastc==0xff01 || lastc==0xff0c ||   // ！ ， fullwidth ! and ,
                                  lastc==0xff1a || lastc==0xff1b ) {  // ： ； fullwidth : and ;
                            FONT_GUARD
                            int w = font->getCharWidth(lastc);
                            if (frmline->width + w + wAlign + x >= maxWidth)
                                word->width -= w;
                            else if (w!=0) {
                                // (This looks like some awkward way of detecting if the line
                                // is made out of solely same-fixed-width CJK ideographs,
                                // which will fail if there's enough variable-width western
                                // chars to fail the rounded division vs nb of char comparison.)
                                if (end - start == int((maxWidth - wAlign) / w))
                                    word->width -= w; // Chinese floating punctuation
                                else if (x/w >= 1 && (end-start==int(maxWidth-wAlign-x)/w)-1)
                                    word->width -= w; // first line with text-indent
                            }
                        }
                        if (frmline->width!=0 && last && align!=LTEXT_ALIGN_CENTER) {
                            // (Chinese) add spaces between words in last line or single line
                            // (so they get visually aligned on a grid with the char on the
                            // previous justified lines)
                            FONT_GUARD
                            int properwordcount = maxWidth/font->getSize() - 2;
                            int extraSpace = maxWidth - properwordcount*font->getSize() - wAlign;
                            int exccess = (frmline->width + x + word->width + extraSpace) - maxWidth;
                            if ( exccess>0 && exccess<maxWidth ) { // prevent the line exceeds screen boundary
                                extraSpace -= exccess;
                            }
                            if ( extraSpace>0 ) {
                                int addSpacePoints = 0;
                                int a;
                                int points=0;
                                for ( a=0; a<(int)frmline->word_count-1; a++ ) {
                                    if ( frmline->words[a].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER )
                                        points++;
                                }
                                addSpacePoints = properwordcount - (frmline->word_count - 1 - points);
                                if (addSpacePoints > 0) {
                                    int addSpaceDiv = extraSpace / addSpacePoints;
                                    int addSpaceMod = extraSpace % addSpacePoints;
                                    int delta = 0;
                                    for (a = 0; a < (int) frmline->word_count; a++) {
                                        frmline->words[a].x +=  delta;
                                        {
                                            delta += addSpaceDiv;
                                            if (addSpaceMod > 0) {
                                                addSpaceMod--;
                                                delta++;
                                            }
                                        }
                                    }
                                }
                            }
                            word->width+=extraSpace;
                        }
                        if ( first && font->getSize()!=0 && (maxWidth/font->getSize()-2)!=0 ) {
                            // proportionally enlarge text-indent when visualAlignment or
                            // floating punctuation is enabled
                            FONT_GUARD
                            int cnt = ((x-wAlign/2)%font->getSize()==0) ? (x-wAlign/2)/font->getSize() : 0;
                                // ugly way to caculate text-indent value, I can not get text-indent from here
                            int p = cnt*(cnt+1)/2;
                            int asd = (2*font->getSize()-font->getCharWidth(lastc)) / (maxWidth/font->getSize()-2);
                            int width = p*asd + cnt; //same math as delta above
                            if (width>0)
                                frmline->x+=width;
                        }
                        word->min_width = word->width;
                    } // done if floating punctuation enabled

                    // printf("addLine - word(%d, %d) x=%d (%d..%d)[%d>%d %x] |%s|\n", wstart, i,
                    //      frmline->width, wstart>0 ? m_widths[wstart-1] : 0, m_widths[i-1], word->width,
                    //      word->min_width, word->flags, LCSTR(lString16(m_text+wstart, i-wstart)));
                }

                if ( adjust_line_box ) {
                    // Adjust full line box height and baseline if needed:
                    // frmline->height is the current line height
                    // frmline->baseline is the distance from line top to the main baseline of the line
                    // top_to_baseline (normally positive number) is the distance from this word top to its own baseline.
                    // baseline_to_bottom (normally positive number) is the descender below baseline for this word
                    // word->y is the distance from this word baseline to the line main baseline
                    //   it is positive when word is subscript, negative when word is superscript
                    //
                    // negative word->y means it's superscript, so the line's baseline might need to go
                    // down (increase) to make room for the superscript
                    int needed_baseline = top_to_baseline - word->y;
                    if ( needed_baseline > frmline->baseline ) {
                        // shift the line baseline and height by the amount needed at top
                        int shift_down = needed_baseline - frmline->baseline;
                        // if (frmline->baseline) printf("pushed down +%d\n", shift_down);
                        // if (frmline->baseline && lastSrc->object)
                        //     printf("%s\n", UnicodeToLocal(ldomXPointer((ldomNode*)lastSrc->object, 0).toString()).c_str());
                        if ( !strut_confined ) {
                            // move line away from the strut baseline
                            frmline->baseline += shift_down;
                            frmline->height += shift_down;
                        }
                        else { // except if "-cr-hint: strut-confined":
                            // Keep the strut, move the word down
                            word->y += shift_down;
                        }
                    }
                    // positive word->y means it's subscript, so the line's baseline does not need to be
                    // changed, but more room below might be needed to display the subscript: increase
                    // line height so next line is pushed down and dont overwrite the subscript
                    int needed_height = frmline->baseline + baseline_to_bottom + word->y;
                    if ( needed_height > frmline->height ) {
                        // printf("extended down +%d\n", needed_height-frmline->height);
                        if ( !strut_confined ) {
                            frmline->height = needed_height;
                        }
                        else { // except if "-cr-hint: strut-confined":
                            // We'd rather move the word up, but it shouldn't go
                            // above the top of the line, so it's not drawn over
                            // previous line text. If it's taller than line height,
                            // it's ok to have it overflow bottom: some part of
                            // it might be overwritten by next line, which we'd
                            // rather have fully readable.
                            word->y -= needed_height - frmline->height;
                            int top_dy = top_to_baseline - word->y - frmline->baseline;
                            if ( top_dy > 0 )
                                word->y += top_dy;
                        }
                    }
                }

                frmline->width += word->width;

                lastSrc = newSrc;
                wstart = i;
            }
            lastIsSpace = isSpace;
            lastIsRTL = isRTL;
        }
        if ( delayed_valign_computation ) {
            // Delayed computation and line box adjustment when we have some words
            // (or images, or inline-boxes) with vertical-align: top or bottom.
            // First, see if we need to adjust frmline->baseline and frmline->height,
            // similarly as done above if adjust_line_box:
            for ( int i=0; i<frmline->word_count; i++ ) {
                if ( frmline->words[i].flags & (LTEXT_WORD_VALIGN_TOP|LTEXT_WORD_VALIGN_BOTTOM) ) {
                    formatted_word_t * word = &frmline->words[i];
                    if ( word->flags & LTEXT_WORD_STRUT_CONFINED )
                        continue; // don't have such words affect current line height & baseline
                    // Update incomplete word->y with current frmline baseline & height,
                    // just as it would have been done if not delayed
                    int cur_word_y;
                    if ( word->flags & LTEXT_WORD_VALIGN_TOP )
                        cur_word_y = word->y - frmline->baseline;
                    else if ( word->flags & LTEXT_WORD_VALIGN_BOTTOM )
                        cur_word_y = word->y + frmline->height - frmline->baseline;
                    else // should not happen
                        cur_word_y = word->y;
                    int needed_baseline = word->_top_to_baseline - cur_word_y;
                    if ( needed_baseline > frmline->baseline ) {
                        // shift the line baseline and height by the amount needed at top
                        int shift_down = needed_baseline - frmline->baseline;
                        frmline->baseline += shift_down;
                        frmline->height += shift_down;
                    }
                    int needed_height = frmline->baseline + word->_baseline_to_bottom + cur_word_y;
                    if ( needed_height > frmline->height ) {
                        frmline->height = needed_height;
                    }
                }
            }
            // Then, get the final word->y (baseline) that aligns the word to top or bottom of frmline
            for ( int i=0; i<frmline->word_count; i++ ) {
                if ( frmline->words[i].flags & (LTEXT_WORD_VALIGN_TOP|LTEXT_WORD_VALIGN_BOTTOM) ) {
                    formatted_word_t * word = &frmline->words[i];
                    if ( word->flags & LTEXT_WORD_VALIGN_TOP ) {
                        word->y = word->y - frmline->baseline;
                    }
                    else if ( word->flags & LTEXT_WORD_VALIGN_BOTTOM ) {
                        word->y = word->y + frmline->height - frmline->baseline;
                    }
                    if ( word->flags & LTEXT_WORD_STRUT_CONFINED ) {
                        // If this word is taller than final line height,
                        // we'd rather have it overflows bottom.
                        int top_dy = word->_top_to_baseline - word->y - frmline->baseline;
                        if ( top_dy > 0 )
                            word->y += top_dy; // move it down
                    }
                }
            }
        }
        alignLine( frmline, align, rightIndent, has_inline_boxes );
        m_y += frmline->height;
        m_pbuffer->height = m_y;
        checkOngoingFloat();
        positionDelayedFloats();
        #if (USE_FRIBIDI==1)
        if ( restore_last_width ) // bidi: restore last width to not mess with next line
            m_widths[end-1] = last_width_to_restore;
        #endif
    }

    int getMaxCondensedSpaceTruncation(int pos) {
        if (pos<0 || pos>=m_length || !(m_flags[pos] & LCHAR_IS_SPACE))
            return 0;
        if (m_pbuffer->min_space_condensing_percent==100)
            return 0;
        int w = (m_widths[pos] - (pos > 0 ? m_widths[pos-1] : 0));
        int dw = w * (100 - m_pbuffer->min_space_condensing_percent) / 100;
        if ( dw>0 ) {
            // typographic rule: don't use spaces narrower than 1/4 of font size
            /* 20191126: disabled, to allow experimenting with lower %
            LVFont * fnt = (LVFont *)m_srcs[pos]->t.font;
            int fntBasedSpaceWidthDiv2 = fnt->getSize() * 3 / 4;
            if ( dw>fntBasedSpaceWidthDiv2 )
                dw = fntBasedSpaceWidthDiv2;
            */
            return dw;
        }
        return 0;
    }

    bool isCJKIdeograph(lChar16 c) {
        return c >= UNICODE_CJK_IDEOGRAPHS_BEGIN &&
               c <= UNICODE_CJK_IDEOGRAPHS_END   &&
               ( c <= UNICODE_CJK_PUNCTUATION_HALF_AND_FULL_WIDTH_BEGIN ||
                 c >= UNICODE_CJK_PUNCTUATION_HALF_AND_FULL_WIDTH_END );
    }

    bool isCJKPunctuation(lChar16 c) {
        return ( c >= UNICODE_CJK_PUNCTUATION_BEGIN && c <= UNICODE_CJK_PUNCTUATION_END ) ||
               ( c >= UNICODE_GENERAL_PUNCTUATION_BEGIN && c <= UNICODE_GENERAL_PUNCTUATION_END &&
                    c!=0x2018 && c!=0x201a && c!=0x201b &&    // ‘ ‚ ‛  left quotation marks
                    c!=0x201c && c!=0x201e && c!=0x201f &&    // “ „ ‟  left double quotation marks
                    c!=0x2035 && c!=0x2036 && c!=0x2037 &&    // ‵ ‶ ‷ reversed single/double/triple primes
                    c!=0x2039 && c!=0x2045 && c!=0x204c  ) || // ‹ ⁅ ⁌ left angle quot mark, bracket, bullet
               ( c >= UNICODE_CJK_PUNCTUATION_HALF_AND_FULL_WIDTH_BEGIN &&
                 c <= UNICODE_CJK_PUNCTUATION_HALF_AND_FULL_WIDTH_END ) ||
               ( c == 0x00b7 ); // · middle dot
    }

    bool isCJKLeftPunctuation(lChar16 c) {
        return c==0x2018 || c==0x201c || // ‘ “ left single and double quotation marks
               c==0x3008 || c==0x300a || c==0x300c || c==0x300e || c==0x3010 || // 〈 《 「 『 【 CJK left brackets
               c==0xff08; // （ fullwidth left parenthesis
    }

    bool isLeftPunctuation(lChar16 c) {
        // Opening quotation marks and dashes that we don't want a followup space to
        // have its width changed
        return ( c >= 0x2010 && c <= 0x2027 ) || // Hyphens, dashes, quotation marks, bullets...
               ( c >= 0x2032 && c <= 0x205E ) || // Primes, bullets...
               ( c >= 0x002A && c <= 0x002F ) || // Ascii * + , - . /
                 c == 0x00AB || c == 0x00BB   || // Quotation marks (including right pointing, for german text)
                 c == 0x0022 || c == 0x0027 || c == 0x0023; // Ascii " ' #

    }

    /// Split paragraph into lines
    void processParagraph( int start, int end, bool isLastPara )
    {
        TR("processParagraph(%d, %d)", start, end);

        // ensure buffer size is ok for paragraph
        allocate( start, end );
        // copy paragraph text to buffer
        copyText( start, end );
        // measure paragraph text
        measureText();

        // run-in detection
        src_text_fragment_t * para = &m_pbuffer->srctext[start];
        int i;
        for ( i=start; i<end; i++ ) {
            if ( !(m_pbuffer->srctext[i].flags & LTEXT_RUNIN_FLAG) ) {
                para = &m_pbuffer->srctext[i];
                break;
            }
        }

        // detect case with inline preformatted text inside block with line feeds -- override align=left for this case
        bool preFormattedOnly = true;
        for ( i=start; i<end; i++ ) {
            if ( !(m_pbuffer->srctext[i].flags & LTEXT_FLAG_PREFORMATTED) ) {
                preFormattedOnly = false;
                break;
            }
        }
        bool lfFound = false;
        for ( i=0; i<m_length; i++ ) {
            if ( m_text[i]=='\n' ) {
                lfFound = true;
                break;
            }
        }
        preFormattedOnly = preFormattedOnly && lfFound;

        int interval = m_srcs[0]->interval; // Note: no more used inside AddLine()
        int maxWidth = getCurrentLineWidth();

        // reservation of space for floating punctuation
        bool visualAlignmentEnabled = gFlgFloatingPunctuationEnabled!=0;
        int visualAlignmentWidth = 0;
        if ( visualAlignmentEnabled ) {
            // We remove from the available width the max of the max width
            // of -/./,/!/? (and other CJK ones) in all fonts used in that
            // paragraph, to reserve room for it in case we get one hanging.
            // (This will lead to messy variable paragraph widths if some
            // paragraph use some bigger font for some inline parts, and
            // others don't.)
            LVFont * font = NULL;
            for ( int i=start; i<end; i++ ) {
                if ( !(m_pbuffer->srctext[i].flags & LTEXT_SRC_IS_OBJECT) ) {
                    font = (LVFont*)m_pbuffer->srctext[i].t.font;
                    if (font) {
                        int dx = font->getVisualAligmentWidth();
                        if ( dx>visualAlignmentWidth )
                            visualAlignmentWidth = dx;
                    }
                }
            }
            maxWidth -= visualAlignmentWidth;
        }

        // split paragraph into lines, export lines
        int pos = 0;
        #if (USE_LIBUNIBREAK!=1)
        int upSkipPos = -1;
        #endif

        // int minWidth = 0;
        // Not per-specs, but when floats reduce the available width, skip y until
        // we have the width to draw at least a few chars on a line.
        // We use N x strut_height because it's one easily acccessible font metric here.
        int minWidth = 3 * m_pbuffer->strut_height;

        for (;pos<m_length;) { // each loop makes a line
            // x is this line indent. We use it like a x coordinates below, but
            // we'll use it on the right in addLine() if para is RTL.
            int x = m_indent_current;
            if ( !m_indent_first_line_done ) {
                m_indent_first_line_done = true;
                m_indent_current = m_indent_after_first_line;
            }
            int w0 = pos>0 ? m_widths[pos-1] : 0;
            int i;
            int lastNormalWrap = -1;
            int lastDeprecatedWrap = -1;
            int lastHyphWrap = -1;
            int lastMandatoryWrap = -1;
            int spaceReduceWidth = 0; // max total line width which can be reduced by narrowing of spaces
            int firstCharMargin = getAdditionalCharWidthOnLeft(pos); // for first italic char with elements below baseline
            // We might not need to bother with negative left side bearing, as we now
            // can have them in the margin as we don't clip anymore. So we could just have:
            // int firstCharMargin = 0;
            // and italic "J" or "f" would be drawn a bit in the margin.
            // (But as I don't know about the wanted effect with visualAlignmentEnabled,
            // and given it's only about italic chars, and that we would need to remove
            // stuff in getRenderedWidths... letting it as it is.)

            if ( m_has_bidi ) {
                // If bidi, our first char may be no more the first char
                // inside AddLine, so reset firtCharMargin to 0.
                firstCharMargin = 0;
                // todo: probably some other things to avoid if bidi or
                // if m_para_dir_is_rtl, like hyphenation.
                // Also possible: scan chars as they fit on this line for
                // bidi level > 1: if none, this line is pure LTR
            }

            maxWidth = getCurrentLineWidth();
            if (maxWidth <= minWidth) {
                // Find y with available minWidth
                int unused_x;
                // We need to provide a height to find some width available over
                // this height, but we don't know yet the height of text (that
                // may have some vertical-align or use a bigger font) or images
                // that will end up on this line (line height is handled later,
                // by AddLine()), we can only ask for the only height we know
                // about: m_pbuffer->strut_height...
                // todo: find a way to be sure or react to that
                int new_y = getYWithAvailableWidth(m_y, minWidth, m_pbuffer->strut_height, unused_x);
                fillAndMoveToY( new_y );
                maxWidth = getCurrentLineWidth();
            }

            if ( visualAlignmentEnabled ) { // Floating punctuation
                maxWidth -= visualAlignmentWidth;
                spaceReduceWidth -= visualAlignmentWidth/2;
                firstCharMargin += visualAlignmentWidth/2;
                if (isCJKLeftPunctuation(m_text[pos])) {
                    // Make that left punctuation left-hanging by reducing firstCharMargin
                    LVFont * fnt = (LVFont *)m_srcs[pos]->t.font;
                    if (fnt)
                        firstCharMargin -= fnt->getCharWidth(m_text[pos]);
                    firstCharMargin = (x + firstCharMargin) > 0 ? firstCharMargin : 0;
                }
            }

            // Find candidates where end of line is possible
            bool seen_non_collapsed_space = false;
            bool seen_first_rendered_char = false;
            for ( i=pos; i<m_length; i++ ) {
                if ( (m_flags[i] & LCHAR_IS_OBJECT) && (m_charindex[i] == FLOAT_CHAR_INDEX) ) { // float
                    src_text_fragment_t * src = m_srcs[i];
                    // Not sure if we can be called again on the same LVFormatter
                    // object, but the whole code allows for re-formatting and
                    // they should give the same result.
                    // So, use a flag to not re-add already processed floats.
                    if ( !(src->flags & LTEXT_SRC_IS_FLOAT_DONE) ) {
                        int currentWidth = x + m_widths[i]-w0 - spaceReduceWidth + firstCharMargin;
                        addFloat( src, currentWidth );
                        src->flags |= LTEXT_SRC_IS_FLOAT_DONE;
                        maxWidth = getCurrentLineWidth();
                    }
                    // We don't set lastNormalWrap when collapsed spaces,
                    // so let's not for floats either.
                    // But we need to when the float is the last source (as
                    // done below, otherwise we would not update wrapPos and
                    // we'd get another ghost line, and this real last line
                    // might be wrongly justified).
                    if ( i==m_length-1 ) {
                        lastNormalWrap = i;
                    }
                    continue;
                }
                lUInt16 flags = m_flags[i];
                // We would not need to bother with LCHAR_IS_COLLAPSED_SPACE, as they have zero
                // width and so can be grabbed here. They carry LCHAR_ALLOW_WRAP_AFTER just like
                // a space, so they will set lastNormalWrap.
                // But we don't want any collapsed space at start to make a new line if the
                // following text is a long word that doesn't fit in the available width (which
                // can happen in a small table cell). So, ignore them at start of line:
                if (!seen_non_collapsed_space) {
                    if (flags & LCHAR_IS_COLLAPSED_SPACE)
                        continue;
                    else
                        seen_non_collapsed_space = true;
                }
                if ( m_text[i]=='\n' ) {
                    lastMandatoryWrap = i;
                    break;
                }
                // Text with "-cr-hint: strut-confined" might just be vertically shifted,
                // but won't change widths. But images who will change height must also
                // have their width reduced to keep their aspect ratio.
                if ( (m_srcs[i]->flags & LTEXT_STRUT_CONFINED) && m_allow_strut_confinning &&
                        (m_flags[i] & LCHAR_IS_OBJECT) && (m_charindex[i] == OBJECT_CHAR_INDEX) ) {
                    int width = m_srcs[i]->o.width;
                    int height = m_srcs[i]->o.height;
                    // Negative width and height mean the value is a % (of our final block width)
                    width = width<0 ? (-width * (m_pbuffer->width - x) / 100) : width;
                    height = height<0 ? (-height * (m_pbuffer->width - x) / 100) : height;
                    resizeImage(width, height, m_pbuffer->width - x, m_max_img_height, m_length>1);
                    if ( height > m_pbuffer->strut_height ) {
                        // Don't make image taller than initial strut height, so adjust width
                        // to keep aspect ratio.
                        width = width * m_pbuffer->strut_height / height;
                        int orig_width = i > 0 ? m_widths[i] - m_widths[i-1] : m_widths[i];
                        // Coding shortcut: instead of messing with m_widths, or having a
                        // strutConfinedReclaimedWidth variable to be used everywhere, add the
                        // reclaimed width to w0 (which holds the cumulative width at start of
                        // line) as it is already used everywhere to get the width of the line.
                        w0 += orig_width - width;
                    }
                }
                if ( !seen_first_rendered_char ) {
                    seen_first_rendered_char = true;
                    // First real non ignoreable char (collapsed spaces skipped):
                    // it might be a wide image or inlineBox. Check that we have
                    // enough current width to have it on this line, otherwise,
                    // move down until we find a y where it would fit (but only
                    // if we're sure we'll find some)
                    int needed_width = x + m_widths[i]-w0;
                    if ( needed_width > maxWidth && needed_width <= m_pbuffer->width ) {
                        // Find y with available needed_width
                        int unused_x;
                        // todo: provide the height of the image or inline-box
                        int new_y = getYWithAvailableWidth(m_y, needed_width, m_pbuffer->strut_height, unused_x);
                        fillAndMoveToY( new_y );
                        maxWidth = getCurrentLineWidth();
                    }
                }
                bool grabbedExceedingSpace = false;
                if ( x + m_widths[i]-w0 > maxWidth + spaceReduceWidth - firstCharMargin) {
                    // It's possible the char at i is a space whose width exceeds maxWidth,
                    // but it should be a candidate for lastNormalWrap (otherwise, the
                    // previous word will be hyphenated and we will get spaces widen for
                    // text justification)
                    if ( (flags & LCHAR_IS_SPACE) && (flags & LCHAR_ALLOW_WRAP_AFTER) ) // don't break yet
                        grabbedExceedingSpace = true;
                    else
                        break;
                }
                // Note: upstream has added in:
                //   https://github.com/buggins/coolreader/commit/e2a1cf3306b6b083467d77d99dad751dc3aa07d9
                // to the next if:
                //  || lGetCharProps(m_text[i]) == 0
                // but this does not look right, as any other unicode char would allow wrap.
                //
                #if (USE_LIBUNIBREAK==1)
                // Note: with libunibreak, we can't assume anymore that LCHAR_ALLOW_WRAP_AFTER is synonym to IS_SPACE.
                if (flags & LCHAR_ALLOW_WRAP_AFTER) {
                    lastNormalWrap = i;
                }
                #else
                // A space or a CJK ideograph make a normal allowed wrap
                if ((flags & LCHAR_ALLOW_WRAP_AFTER) || isCJKIdeograph(m_text[i])) {
                    // Need to check if previous and next non-space char request a wrap on
                    // this space (or CJK char) to be avoided
                    bool avoidWrap = false;
                    // Look first at following char(s)
                    for (int j = i+1; j < m_length; j++) {
                        if ( m_flags[j] & LCHAR_IS_OBJECT ) {
                            if (m_charindex[j] == FLOAT_CHAR_INDEX) // skip floats
                                continue;
                            else // allow wrap between space/CJK and image or inline-box
                                break;
                        }
                        if ( !(m_flags[j] & LCHAR_ALLOW_WRAP_AFTER) ) { // not another (collapsible) space
                            avoidWrap = lGetCharProps(m_text[j]) & CH_PROP_AVOID_WRAP_BEFORE;
                            break;
                        }
                    }
                    if (!avoidWrap && i < m_length-1) { // Look at preceding char(s)
                        // (but not if it is the last char, where a wrap is fine
                        // even if it ends after a CH_PROP_AVOID_WRAP_AFTER char)
                        for (int j = i-1; j >= 0; j--) {
                            if ( m_flags[j] & LCHAR_IS_OBJECT ) {
                                if (m_charindex[j] == FLOAT_CHAR_INDEX) // skip floats
                                    continue;
                                else // allow wrap after a space following an image or inline-box
                                    break;
                            }
                            if ( !(m_flags[j] & LCHAR_ALLOW_WRAP_AFTER) ) { // not another (collapsible) space
                                avoidWrap = lGetCharProps(m_text[j]) & CH_PROP_AVOID_WRAP_AFTER;
                                break;
                            }
                        }
                    }
                    if (!avoidWrap)
                        lastNormalWrap = i;
                    // We could use lastDeprecatedWrap, but it then get too much real chances to be used:
                    // else lastDeprecatedWrap = i;
                    // Note that a wrap can happen AFTER a '-' (that has CH_PROP_AVOID_WRAP_AFTER)
                    // when lastDeprecatedWrap is prefered below.
                }
                #endif // not USE_LIBUNIBREAK==1
                else if ( i==m_length-1 ) // Last char
                    lastNormalWrap = i;
                else if ( flags & LCHAR_DEPRECATED_WRAP_AFTER ) // Hyphens make a less priority wrap
                    lastDeprecatedWrap = i;
                else if ( flags & LCHAR_ALLOW_HYPH_WRAP_AFTER ) // can't happen at this point as we haven't
                    lastHyphWrap = i;                           // gone thru hyphenate()
                if ( !grabbedExceedingSpace &&
                        m_pbuffer->min_space_condensing_percent != 100 &&
                        i < m_length-1 &&
                        ( m_flags[i] & LCHAR_IS_SPACE ) &&
                        ( i==m_length-1 || !(m_flags[i + 1] & LCHAR_IS_SPACE) ) ) {
                    // Each space not followed by a space is candidate for space condensing
                    int dw = getMaxCondensedSpaceTruncation(i);
                    if ( dw>0 )
                        spaceReduceWidth += dw;
                }
                if (grabbedExceedingSpace)
                    break; // delayed break
            }
            // It feels there's no need to do anything if there's been one single float
            // that took all the width: we moved i and can wrap.
            if (i<=pos)
                i = pos + 1; // allow at least one character to be shown on line
            int wordpos = i-1;
            int normalWrapWidth = lastNormalWrap > 0 ? x + m_widths[lastNormalWrap]-w0 : 0;
            int deprecatedWrapWidth = lastDeprecatedWrap > 0 ? x + m_widths[lastDeprecatedWrap]-w0 : 0;
            int unusedSpace = maxWidth - normalWrapWidth - 2*visualAlignmentWidth;
            int unusedPercent = maxWidth > 0 ? unusedSpace * 100 / maxWidth : 0;
            if ( deprecatedWrapWidth>normalWrapWidth && unusedPercent>3 ) {
                lastNormalWrap = lastDeprecatedWrap;
            }
            // If, with normal wrapping, more than 5% of the line would not be used,
            // try to find a word (from where we stopped back to lastNormalWrap) to
            // hyphenate, if hyphenation is not forbidden by CSS.
            // todo: decide if we should hyphenate if bidi is happening up to now
            if ( lastMandatoryWrap<0 && lastNormalWrap<m_length-1 && unusedPercent > m_pbuffer->unused_space_threshold_percent ) {
                // There may be more than one word between wordpos and lastNormalWrap (or
                // pos, the start of this line): if hyphenation is not possible with
                // the right most one, we have to try the previous words.
                // #define DEBUG_HYPH_EXTRA_LOOPS // Uncomment for debugging loops
                #ifdef DEBUG_HYPH_EXTRA_LOOPS
                    int debug_loop_num = 0;
                #endif
                int wordpos_min = lastNormalWrap > pos ? lastNormalWrap : pos;
                while ( wordpos > wordpos_min ) {
                    if ( m_srcs[wordpos]->flags & LTEXT_SRC_IS_OBJECT ) {
                        wordpos--; // skip images & floats
                        continue;
                    }
                    #ifdef DEBUG_HYPH_EXTRA_LOOPS
                        debug_loop_num++;
                        if (debug_loop_num > 1)
                            printf("hyph loop #%d checking: %s\n", debug_loop_num,
                                LCSTR(lString16(m_text+wordpos_min, i-wordpos_min+1)));
                    #endif
                    if ( !(m_srcs[wordpos]->flags & LTEXT_HYPHENATE) ) {
                        // The word at worpos can't be hyphenated, but it might be
                        // allowed on some earlier word in another text node.
                        // As this is a rare situation (they are mostly all hyphenat'able,
                        // or none of them are), and to skip some loops, as the min size
                        // of a word to go look for hyphenation is 4, skip by 4 chars.
                        wordpos = wordpos - MIN_WORD_LEN_TO_HYPHENATE;
                        continue;
                    }
                    // lStr_findWordBounds() will find the word contained at wordpos
                    // (or the previous word if wordpos happens to be a space or some
                    // punctuation) by looking only for alpha chars in m_text.
                    // Note: it actually does that with the char at wordpos-1 - not sure
                    // if we shoud correct it, here or there - or if this is fine - but
                    // let's go with it as-is as it might be a safety and might help
                    // us not be stuck in some infinite loop here.
                    int wstart, wend;
                    lStr_findWordBounds( m_text, m_length, wordpos, wstart, wend );
                    if ( wend <= lastNormalWrap ) {
                        // We passed back lastNormalWrap: no need to look for more
                        break;
                    }
                    int len = wend - wstart;
                    if ( len < MIN_WORD_LEN_TO_HYPHENATE ) {
                        // Too short word found, skip it
                        wordpos = wstart - 1;
                        continue;
                    }
                    if ( wstart >= wordpos ) {
                        // Shouldn't happen, but let's be sure we don't get stuck
                        wordpos = wordpos - MIN_WORD_LEN_TO_HYPHENATE;
                        continue;
                    }
                    #ifdef DEBUG_HYPH_EXTRA_LOOPS
                        if (debug_loop_num > 1)
                            printf("  hyphenating: %s\n", LCSTR(lString16(m_text+wstart, len)));
                    #endif
                    #if TRACE_LINE_SPLITTING==1
                        TR("wordBounds(%s) unusedSpace=%d wordWidth=%d",
                                LCSTR(lString16(m_text+wstart, len)), unusedSpace, m_widths[wend]-m_widths[wstart]);
                    #endif
                    // We have a valid word to look for hyphenation
                    if ( len > MAX_WORD_SIZE ) // hyphenate() stops/truncates at 64 chars
                        len = MAX_WORD_SIZE;
                    // ->hyphenate(), which is used by some other parts of the code,
                    // expects a lUInt8 array. We added flagSize=1|2 so it can set the correct
                    // flags on our upgraded (from lUInt8 to lUInt16) m_flags.
                    lUInt8 * flags = (lUInt8*) (m_flags + wstart);
                    // Fill static array with cumulative widths relative to word start
                    static lUInt16 widths[MAX_WORD_SIZE];
                    int wordStart_w = wstart>0 ? m_widths[wstart-1] : 0;
                    for ( int i=0; i<len; i++ ) {
                        widths[i] = m_widths[wstart+i] - wordStart_w;
                    }
                    int max_width = maxWidth + spaceReduceWidth - x - (wordStart_w - w0) - firstCharMargin;
                    // In some rare cases, a word here can be made with parts from multiple text nodes.
                    // Use the font of the first text node to compute the hyphen width, which
                    // might then be wrong - but that will be smoothed by alignLine().
                    // (lStr_findWordBounds() might grab objects or inlineboxes as part of
                    // the word, so skip them when looking for a font)
                    int _hyphen_width = 0;
                    for ( int i=wstart; i<wend; i++ ) {
                        if ( !(m_srcs[i]->flags & LTEXT_SRC_IS_OBJECT) ) {
                            _hyphen_width = ((LVFont*)m_srcs[i]->t.font)->getHyphenWidth();
                            break;
                        }
                    }
                    // Use the hyph method of the source node that contains wordpos
                    if ( m_srcs[wordpos]->lang_cfg->getHyphMethod()->hyphenate(m_text+wstart, len, widths, flags, _hyphen_width, max_width, 2) ) {
                        // We need to reset the flag for the multiple hyphenation
                        // opportunities we will not be using (or they could cause
                        // spurious spaces, as a word here may be multiple words
                        // in AddLine() if parts from different text nodes).
                        for ( int i=0; i<len; i++ ) {
                            if ( m_flags[wstart+i] & LCHAR_ALLOW_HYPH_WRAP_AFTER ) {
                                if ( widths[i] + _hyphen_width > max_width ) {
                                    TR("hyphen found, but max width reached at char %d", i);
                                    m_flags[wstart+i] &= ~LCHAR_ALLOW_HYPH_WRAP_AFTER; // reset flag
                                }
                                else if ( wstart + i > pos+1 ) {
                                    if ( lastHyphWrap >= 0 ) { // reset flag on previous candidate
                                        m_flags[lastHyphWrap] &= ~LCHAR_ALLOW_HYPH_WRAP_AFTER;
                                    }
                                    lastHyphWrap = wstart + i;
                                    // Keep looking for some other candidates in that word
                                }
                                else if ( wstart + i >= pos ) {
                                    m_flags[wstart+i] &= ~LCHAR_ALLOW_HYPH_WRAP_AFTER; // reset flag
                                }
                                // Don't reset those < pos as they are part of previous line
                            }
                        }
                        if ( lastHyphWrap >= 0 ) {
                            // Found in this word, no need to look at previous words
                            break;
                        }
                    }
                    TR("no hyphen found - max_width=%d", max_width);
                    // Look at previous words if any
                    wordpos = wstart - 1;
                }
            }

            // Find best position to end this line
            int wrapPos = lastHyphWrap;
            if ( lastMandatoryWrap>=0 )
                wrapPos = lastMandatoryWrap;
            else {
                if ( wrapPos<lastNormalWrap )
                    wrapPos = lastNormalWrap;
                if ( wrapPos<0 )
                    wrapPos = i-1;
                #if (USE_LIBUNIBREAK!=1)
                if ( wrapPos<=upSkipPos ) {
                    // Ensure that what, when dealing with previous line, we pushed to
                    // next line (below) is actually on this new line.
                    //CRLog::trace("guard old wrapPos at %d", wrapPos);
                    wrapPos = upSkipPos+1;
                    //CRLog::trace("guard new wrapPos at %d", wrapPos);
                    upSkipPos = -1;
                }
                #endif
            }
            bool needReduceSpace = true; // todo: calculate whether space reducing required
            int endp = wrapPos+(lastMandatoryWrap<0 ? 1 : 0);

            // Specific handling of CJK punctuation that should not happen at start or
            // end of line. When using libunibreak, we trust it to handle them correctly.
            #if (USE_LIBUNIBREAK!=1)
            // The following looks left (up) and right (down) if there are any chars/punctuation
            // that should be prevented from being at the end of line or start of line, and if
            // yes adjust wrapPos so they are pushed to next line, or brought to this line.
            // It might be a bit of a duplication of what's done above (for latin punctuations)
            // in the avoidWrap section.
            int downSkipCount = 0;
            int upSkipCount = 0;
            if (endp > 1 && isCJKLeftPunctuation(*(m_text + endp))) {
                // Next char will be fine at the start of next line.
                //CRLog::trace("skip skip punctuation %s, at index %d", LCSTR(lString16(m_text+endp, 1)), endp);
            } else if (endp > 1 && endp < m_length - 1 && isCJKLeftPunctuation(*(m_text + endp - 1))) {
                // Most right char is left punctuation: go back 1 char so this one
                // goes onto next line.
                upSkipPos = endp;
                endp--; wrapPos--;
                //CRLog::trace("up skip left punctuation %s, at index %d", LCSTR(lString16(m_text+endp, 1)), endp);
            } else if (endp > 1 && isCJKPunctuation(*(m_text + endp))) {
                // Next char (start of next line) is some right punctuation that
                // is not allowed at start of line.
                // Look if it's better to wrap before (up) or after (down), and how
                // much up or down we find an adequate wrap position, and decide
                // which to use.
                for (int epos = endp; epos<m_length; epos++, downSkipCount++) {
                   if ( !isCJKPunctuation(*(m_text + epos)) ) break;
                   //CRLog::trace("down skip punctuation %s, at index %d", LCSTR(lString16(m_text + epos, 1)), epos);
                }
                for (int epos = endp; epos>=start; epos--, upSkipCount++) {
                   if ( !isCJKPunctuation(*(m_text + epos)) ) break;
                   //CRLog::trace("up skip punctuation %s, at index %d", LCSTR(lString16(m_text + epos, 1)), epos);
                }
                if (downSkipCount <= upSkipCount && downSkipCount <= 2 && visualAlignmentEnabled) {
                   // Less skips if we bring next char on this line, and hanging
                   // punctuation is enabled so this punctuation will naturally
                   // find it's place in the reserved right area.
                   endp += downSkipCount;
                   wrapPos += downSkipCount;
                   //CRLog::trace("finally down skip punctuations %d", downSkipCount);
                } else if (upSkipCount <= 2) {
                   // Otherwise put it on next line (spaces or inter-ideograph spaces
                   // will be expanded for justification).
                   upSkipPos = endp;
                   endp -= upSkipCount;
                   wrapPos -= upSkipCount;
                   //CRLog::trace("finally up skip punctuations %d", upSkipCount);
                }
            }
            #endif

            // Best position to end this line found.
            // We need to possibly extend the last char width to account for italic
            // right side bearing overflow (but not if we ended the line with some
            // hyphenation, as the last glyph will then be the hyphen).
            if ( endp > 0 && !(m_flags[endp-1] & LCHAR_ALLOW_HYPH_WRAP_AFTER) ) {
                // Find the real last displayed glyph, skipping spaces and floats
                int lastnonspace = endp-1;
                for ( int k=endp-1; k>=start; k-- ) {
                    if ( !(m_flags[k] & LCHAR_IS_SPACE) &&
                         !( (m_flags[k] & LCHAR_IS_OBJECT) && (m_charindex[k] == FLOAT_CHAR_INDEX) ) ) {
                        lastnonspace = k;
                        break;
                    }
                }
                // If the last non-space/non-float is an image or an inline-block box, we don't do it.
                // Note: it feels we should do that for the char before ANY image on the line (so the italic
                // glyph does not overlap with the image). It's unclear whether the former code did that
                // (or not) for the char before an image at end of line only...
                if ( !(m_flags[lastnonspace] & LCHAR_IS_OBJECT) ) {
                    // todo: probably need be avoided if bidi/rtl:
                    int dw = lastnonspace>=start ? getAdditionalCharWidth(lastnonspace, lastnonspace+1) : 0;
                    if (dw) {
                        TR("additional width = %d, after char %s", dw, LCSTR(lString16(m_text + lastnonspace, 1)));
                        m_widths[lastnonspace] += dw;
                    }
                }
            }
            if (endp > m_length)
                endp = m_length;
            addLine(pos, endp, x + firstCharMargin, para, interval, pos==0, wrapPos>=m_length-1, preFormattedOnly, needReduceSpace, isLastPara);
            pos = wrapPos + 1;
            #if (USE_LIBUNIBREAK==1)
            // (Only when using libunibreak, which we trust decisions to wrap on hyphens.)
            if ( m_srcs[wrapPos]->lang_cfg->duplicateRealHyphenOnNextLine() && pos > 0 && pos < m_length-1 ) {
                if ( m_text[wrapPos] == '-' || m_text[wrapPos] == UNICODE_HYPHEN ) {
                    pos--; // Have that last hyphen also at the start of next line
                           // (small caveat: the duplicated hyphen at start of next
                           // line won't be part of the highlighted text)
                }
            }
            #endif
        }
    }

    void processEmbeddedBlock( int idx )
    {
        ldomNode * node = (ldomNode *) m_pbuffer->srctext[idx].object;
        // Use current width available at current y position for the whole block
        // (Firefox would lay out this block content around the floats met along
        // the way, but it would be quite tedious to do the same... so, we don't).
        int width = getCurrentLineWidth();
        int block_x = getCurrentLineX();
        int cur_y = m_y;

        bool already_rendered = false;
        { // in its own scope, so this RenderRectAccessor is forgotten when left
            RenderRectAccessor fmt( node );
            if ( RENDER_RECT_HAS_FLAG(fmt, BOX_IS_RENDERED) ) {
                already_rendered = true;
            }
        }
        // On the first rendering (after type settings changes), we want to forward
        // this block individual lines to the main page splitting context.
        // But on later calls (once already_rendered), used for drawing or text
        // selection, we want to have a single line with the inlineBox.
        // We'll mark the first rendering with is_reusable=false, so that we go
        // reformatting this final node when we need to draw it.
        // (We could mix the individual lines with the main inlineBox line, but
        // that would need added code at various places to ignore one or the
        // others depending on what's needed there.)
        if ( !already_rendered ) {
            LVRendPageContext context( NULL, m_pbuffer->page_height );
            // We don't know if the upper LVRendPageContext wants lines or not,
            // so assume it does (the main flow does).
            int rend_flags = gRenderBlockRenderingFlags; // global flags
            // We want to avoid negative margins (if allowed in global flags) and
            // going back the flow y, as the transfered lines would not reflect
            // that, and we could get some small mismatches and glitches.
            rend_flags &= ~BLOCK_RENDERING_ALLOW_NEGATIVE_COLLAPSED_MARGINS;
            int baseline = REQ_BASELINE_FOR_TABLE; // baseline of block is baseline of its first line
            renderBlockElement( context, node, 0, 0, width, m_specified_para_dir, &baseline, rend_flags);
            RenderRectAccessor fmt( node );
            fmt.setX(block_x);
            fmt.setY(m_y);
            fmt.setBaseline(baseline);
            RENDER_RECT_SET_FLAG(fmt, BOX_IS_RENDERED);
            // Transfer individual lines from this sub-context into real frmlines (they
            // will be transferred to the upper context by renderBlockElementEnhanced())
            if ( context.getLines() ) {
                LVPtrVector<LVRendLineInfo> * lines = context.getLines();
                for ( int i=0; i < lines->length(); i++ ) {
                    LVRendLineInfo * line = lines->get(i);
                    formatted_line_t * frmline = lvtextAddFormattedLine( m_pbuffer );
                    frmline->x = block_x;
                    frmline->y = cur_y + line->getStart();
                    frmline->height = line->getHeight();
                    frmline->flags = line->getFlags();
                    if (m_has_ongoing_float)
                        frmline->flags |= LTEXT_LINE_SPLIT_AVOID_BEFORE;
                    // Unfortunaltely, we can't easily forward footnotes links
                    // gathered by this sub-context via frmlines.
                    // printf("emb line %d>%d\n", frmline->y, frmline->height);
                    m_y += frmline->height;
                    // We only check for already positionned floats to ensure
                    // no page break along them. We'll positionned yet-to-be
                    // positionned floats only when done with this embedded block.
                    checkOngoingFloat();
                }
            }
            // Next time we have to use this LFormattedText for drawing, have it
            // trashed: we'll re-format it by going into the following 'else'.
            m_pbuffer->is_reusable = false;
        }
        else {
            RenderRectAccessor fmt( node );
            int height = fmt.getHeight();
            formatted_line_t * frmline = lvtextAddFormattedLine( m_pbuffer );
            frmline->x = block_x;
            frmline->y = cur_y;
            frmline->height = height;
            frmline->flags = 0; // no flags needed once page split has been done
            // printf("final line %d>%d\n", frmline->y, frmline->height);
            // This line has a single word: the inlineBox.
            formatted_word_t * word = lvtextAddFormattedWord(frmline);
            word->src_text_index = idx;
            word->flags = LTEXT_WORD_IS_INLINE_BOX;
            word->x = 0;
            word->width = width;
            m_y = cur_y + height;
            m_pbuffer->height = m_y;
        }
        // Not tested how this would work with floats...
        checkOngoingFloat();
        positionDelayedFloats();
    }

    /// split source data into paragraphs
    void splitParagraphs()
    {
        int start = 0;
        int i;
//        TR("==== splitParagraphs() ====");
//        for ( i=0; i<m_pbuffer->srctextlen; i++ ) {
//            int flg = m_pbuffer->srctext[i].flags;
//            if ( (flg & LTEXT_RUNIN_FLAG) )
//                TR("run-in found");
//            TR("  %d: flg=%04x al=%d ri=%d '%s'", i, flg, (flg & LTEXT_FLAG_NEWLINE), (flg & LTEXT_RUNIN_FLAG)?1:0, (flg&LTEXT_SRC_IS_OBJECT ? "<image>" : LCSTR(lString16(m_pbuffer->srctext[i].t.text, m_pbuffer->srctext[i].t.len)) ) );
//        }
//        TR("============================");

        int srctextlen = m_pbuffer->srctextlen;
        int clear_after_last_flag = 0;
        if ( srctextlen>0 && (m_pbuffer->srctext[srctextlen-1].flags & LTEXT_SRC_IS_CLEAR_LAST) ) {
            // Ignorable source line added to carry a last <br clear=>.
            clear_after_last_flag = m_pbuffer->srctext[srctextlen-1].flags & LTEXT_SRC_IS_CLEAR_BOTH;
            srctextlen -= 1; // Don't process this last srctext
        }

        bool prevRunIn = srctextlen>0 && (m_pbuffer->srctext[0].flags & LTEXT_RUNIN_FLAG);
        for ( i=1; i<=srctextlen; i++ ) {
            // Split on LTEXT_FLAG_NEWLINE, mostly set when <BR/> met
            // (we check m_pbuffer->srctext[i], the next srctext that we are not
            // adding to the current paragraph, as <BR> and its clear= are carried
            // by the following text.)
            bool isLastPara = (i == srctextlen);
            if ( isLastPara || ((m_pbuffer->srctext[i].flags & LTEXT_FLAG_NEWLINE) && !prevRunIn) ) {
                if ( m_pbuffer->srctext[start].flags & LTEXT_SRC_IS_CLEAR_BOTH ) {
                    // (LTEXT_SRC_IS_CLEAR_BOTH is a mask, will match _LEFT and _RIGHT too)
                    floatClearText( m_pbuffer->srctext[start].flags & LTEXT_SRC_IS_CLEAR_BOTH );
                }
                // We do not need to go thru processParagraph() to handle an embedded block
                // (bogus block element children of an inline element): we have a dedicated
                // handler for it.
                bool isEmbeddedBlock = false;
                if ( i == start + 1 ) {
                    // Embedded block among inlines had been surrounded by LTEXT_FLAG_NEWLINE,
                    // so we'll get one standalone here.
                    if ( m_pbuffer->srctext[start].flags & LTEXT_SRC_IS_INLINE_BOX ) {
                        // We used LTEXT_SRC_IS_INLINE_BOX for embedded blocks too (to not
                        // waste a bit in the lUInt32 for LTEXT_SRC_IS_EMBEDDED_BLOCK that
                        // we would only be using here), so do this check to see if it
                        // really is an embedded block.
                        ldomNode * node = (ldomNode *) m_pbuffer->srctext[start].object;
                        if ( node->isEmbeddedBlockBoxingInlineBox() ) {
                            isEmbeddedBlock = true;
                        }
                    }
                }
                if ( isEmbeddedBlock )
                    processEmbeddedBlock( start );
                else
                    processParagraph( start, i, isLastPara );
                start = i;
            }
            prevRunIn = (i<srctextlen) && (m_pbuffer->srctext[i].flags & LTEXT_RUNIN_FLAG);
        }
        if ( !m_no_clear_own_floats ) {
            // Clear our own floats so they are fully contained in this final block.
            finalizeFloats();
        }
        if ( clear_after_last_flag ) {
            floatClearText( clear_after_last_flag );
        }
    }

    void dealloc()
    {
        if ( !m_staticBufs ) {
            free( m_text );
            free( m_flags );
            free( m_srcs );
            free( m_charindex );
            free( m_widths );
            m_text = NULL;
            m_flags = NULL;
            m_srcs = NULL;
            m_charindex = NULL;
            m_widths = NULL;
            #if (USE_FRIBIDI==1)
                free( m_bidi_ctypes );
                free( m_bidi_btypes );
                free( m_bidi_levels );
                m_bidi_ctypes = NULL;
                m_bidi_btypes = NULL;
                m_bidi_levels = NULL;
            #endif
            m_staticBufs = true;
            // printf("freeing dynamic buffers\n");
        }
        else {
            m_staticBufs_inUse = false;
            // printf("releasing static buffers\n");
        }
    }

    /// format source data
    int format()
    {
        // split and process all paragraphs
        splitParagraphs();
        // cleanup
        dealloc();
        TR("format() finished: h=%d  lines=%d", m_y, m_pbuffer->frmlinecount);
        return m_y;
    }
};

bool LVFormatter::m_staticBufs_inUse = false;
#if (USE_LIBUNIBREAK==1)
bool LVFormatter::m_libunibreak_init_done = false;
#endif

static void freeFrmLines( formatted_text_fragment_t * m_pbuffer )
{
    // clear existing formatted data, if any
    if (m_pbuffer->frmlines)
    {
        for (int i=0; i<m_pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( m_pbuffer->frmlines[i] );
        }
        free( m_pbuffer->frmlines );
    }
    m_pbuffer->frmlines = NULL;
    m_pbuffer->frmlinecount = 0;

    // Also clear floats
    if (m_pbuffer->floats)
    {
        for (int i=0; i<m_pbuffer->floatcount; i++)
        {
            if (m_pbuffer->floats[i]->links) {
                delete m_pbuffer->floats[i]->links;
            }
            free( m_pbuffer->floats[i] );
        }
        free( m_pbuffer->floats );
    }
    m_pbuffer->floats = NULL;
    m_pbuffer->floatcount = 0;
}

// experimental formatter
lUInt32 LFormattedText::Format(lUInt16 width, lUInt16 page_height, int para_direction, BlockFloatFootprint * float_footprint)
{
    // clear existing formatted data, if any
    freeFrmLines( m_pbuffer );
    // setup new page size
    m_pbuffer->width = width;
    m_pbuffer->height = 0;
    m_pbuffer->page_height = page_height;
    m_pbuffer->is_reusable = true;
    // format text
    LVFormatter formatter( m_pbuffer );

    // Set (as properties of the whole final block) the text-indent computed
    // values for the first line and for the next lines, by taking it
    // from the first src_text_fragment_t added (see comment in lvrend.cpp
    // renderFinalBlock() why we do it that way - while it might be better
    // if it were provided as a parameter to LFormattedText::Format()).
    int indent = m_pbuffer->srctextlen > 0 ? m_pbuffer->srctext[0].indent : 0;
    formatter.m_indent_first_line_done = false;
    if ( indent >= 0 ) { // positive indent affects only first line
        formatter.m_indent_current = indent;
        formatter.m_indent_after_first_line = 0;
    }
    else { // negative indent affects all but first lines
        formatter.m_indent_current = 0;
        formatter.m_indent_after_first_line = -indent;
    }

    // Set specified para direction (can be REND_DIRECTION_UNSET, in which case
    // it will be detected by fribidi)
    formatter.m_specified_para_dir = para_direction;

    if (float_footprint) {
        formatter.m_no_clear_own_floats = float_footprint->no_clear_own_floats;

        // BlockFloatFootprint provides a set of floats to represent
        // outer floats possibly having some footprint over the final
        // block that is to be formatted.
        // See FlowState->getFloatFootprint() for details.
        // So, for each of them, just add an embedded_float_t (without
        // a scrtext as they are not ours) to the buffer so our
        // positionning code can handle them.
        for (int i=0; i<float_footprint->floats_cnt; i++) {
            embedded_float_t * flt =  lvtextAddEmbeddedFloat( m_pbuffer );
            flt->srctext = NULL; // not our own float
            flt->x = float_footprint->floats[i][0];
            flt->y = float_footprint->floats[i][1];
            flt->width = float_footprint->floats[i][2];
            flt->height = float_footprint->floats[i][3];
            flt->is_right = (bool)(float_footprint->floats[i][4]);
        }
    }

    lUInt32 h = formatter.format();

    if ( float_footprint && float_footprint->no_clear_own_floats ) {
        // If we did not finalize/clear our embedded floats, forward
        // them to FlowState so it can ensure layout around them of
        // other block or final nodes.
        for (int i=0; i<m_pbuffer->floatcount; i++) {
            embedded_float_t * flt = m_pbuffer->floats[i];
            if (flt->srctext == NULL) // ignore outer floats given to us by flow
                continue;
            float_footprint->forwardOverflowingFloat(flt->x, flt->y, flt->width, flt->height,
                                        flt->is_right, (ldomNode *)flt->srctext->object);
        }
    }

    return h;
}

void LFormattedText::setImageScalingOptions( img_scaling_options_t * options )
{
    m_pbuffer->img_zoom_in_mode_block = options->zoom_in_block.mode;
    m_pbuffer->img_zoom_in_scale_block = options->zoom_in_block.max_scale;
    m_pbuffer->img_zoom_in_mode_inline = options->zoom_in_inline.mode;
    m_pbuffer->img_zoom_in_scale_inline = options->zoom_in_inline.max_scale;
    m_pbuffer->img_zoom_out_mode_block = options->zoom_out_block.mode;
    m_pbuffer->img_zoom_out_scale_block = options->zoom_out_block.max_scale;
    m_pbuffer->img_zoom_out_mode_inline = options->zoom_out_inline.mode;
    m_pbuffer->img_zoom_out_scale_inline = options->zoom_out_inline.max_scale;
}

void LFormattedText::setSpaceWidthScalePercent(int spaceWidthScalePercent)
{
    if (spaceWidthScalePercent>=10 && spaceWidthScalePercent<=500)
        m_pbuffer->space_width_scale_percent = spaceWidthScalePercent;
}

void LFormattedText::setMinSpaceCondensingPercent(int minSpaceCondensingPercent)
{
    if (minSpaceCondensingPercent>=25 && minSpaceCondensingPercent<=100)
        m_pbuffer->min_space_condensing_percent = minSpaceCondensingPercent;
}

void LFormattedText::setUnusedSpaceThresholdPercent(int unusedSpaceThresholdPercent)
{
    if (unusedSpaceThresholdPercent>=0 && unusedSpaceThresholdPercent<=20)
        m_pbuffer->unused_space_threshold_percent = unusedSpaceThresholdPercent;
}

void LFormattedText::setMaxAddedLetterSpacingPercent(int maxAddedLetterSpacingPercent)
{
    if (maxAddedLetterSpacingPercent>=0 && maxAddedLetterSpacingPercent<=20)
        m_pbuffer->max_added_letter_spacing_percent = maxAddedLetterSpacingPercent;
}

/// set colors for selection and bookmarks
void LFormattedText::setHighlightOptions(text_highlight_options_t * v)
{
    m_pbuffer->highlight_options.selectionColor = v->selectionColor;
    m_pbuffer->highlight_options.commentColor = v->commentColor;
    m_pbuffer->highlight_options.correctionColor = v->correctionColor;
    m_pbuffer->highlight_options.bookmarkHighlightMode = v->bookmarkHighlightMode;
}


void DrawBookmarkTextUnderline(LVDrawBuf & drawbuf, int x0, int y0, int x1, int y1, int y, int flags, text_highlight_options_t * options) {
    if (!(flags & (4 | 8)))
        return;
    if (options->bookmarkHighlightMode == highlight_mode_none)
        return;
    bool isGray = drawbuf.GetBitsPerPixel() <= 8;
    lUInt32 cl = 0x000000;
    if (isGray) {
        if (options->bookmarkHighlightMode == highlight_mode_solid)
            cl = (flags & 4) ? 0xCCCCCC : 0xAAAAAA;
    } else {
        cl = (flags & 4) ? options->commentColor : options->correctionColor;
    }

    if (options->bookmarkHighlightMode == highlight_mode_solid) {
        // solid fill
        lUInt32 cl2 = (cl & 0xFFFFFF) | 0xA0000000;
        drawbuf.FillRect(x0, y0, x1, y1, cl2);
    }

    if (options->bookmarkHighlightMode == highlight_mode_underline) {
        // underline
        cl = (cl & 0xFFFFFF);
        lUInt32 cl2 = cl | 0x80000000;
        int step = 4;
        int index = 0;
        for (int x = x0; x < x1; x += step ) {

            int x2 = x + step;
            if (x2 > x1)
                x2 = x1;
            if (flags & 8) {
                // correction
                int yy = (index & 1) ? y - 1 : y;
                drawbuf.FillRect(x, yy-1, x+1, yy, cl2);
                drawbuf.FillRect(x+1, yy-1, x2-1, yy, cl);
                drawbuf.FillRect(x2-1, yy-1, x2, yy, cl2);
            } else if (flags & 4) {
                if (index & 1)
                    drawbuf.FillRect(x, y-1, x2 + 1, y, cl);
            }
            index++;
        }
    }
}

static void getAbsMarksFromMarks(ldomMarkedRangeList * marks, ldomMarkedRangeList * absmarks, ldomNode * node) {
    // Provided ldomMarkedRangeList * marks are ranges made from the words
    // of a selection currently being made (native highlights by crengine).
    // Their coordinates have been translated from absolute to relative
    // to the final node, by the DrawDocument() that called
    // LFormattedText::Draw() for this final node.
    // In LFormattedText::Draw(), when we need to call DrawDocument() to
    // draw floats or inlineBoxes, we need to translate them back to
    // absolute coordinates (DrawDocument() will translate them again
    // to relative coordinates in the drawn float or inlineBox).
    // (They are matched in LFormattedText::Draw() against the lineRect,
    // which have coordinates in the context of where we are drawing.)
    // The 'node' provided to this function must be a floatBox or inlineBox:
    // its parent is either the final node that contains them, or some
    // inline node contained in it.

    // We need to know the current final node that contains the provided
    // node, and its absolute coordinates
    ldomNode * final_node = node->getParentNode();
    for ( ; final_node; final_node = final_node->getParentNode() ) {
        int rm = final_node->getRendMethod();
        if ( rm == erm_final || rm == erm_list_item || rm == erm_table_caption )
            break;
    }
    lvRect final_node_rect = lvRect();
    if ( final_node )
        final_node->getAbsRect( final_node_rect, true );

    // Fill the second provided ldomMarkedRangeList with marks in absolute
    // coordinates.
    for ( int i=0; i<marks->length(); i++ ) {
        ldomMarkedRange * mark = marks->get(i);
        ldomMarkedRange * newmark = new ldomMarkedRange( *mark );
        newmark->start.y += final_node_rect.top;
        newmark->end.y += final_node_rect.top;
        newmark->start.x += final_node_rect.left;
        newmark->end.x += final_node_rect.left;
            // (Note: early when developping this, NOT updating x gave the
            // expected results, althought logically it should be updated...
            // But now, it seems to work, and is needed to correctly shift
            // highlight marks in inlineBox by the containing final block's
            // left margin...)
        absmarks->add(newmark);
    }
}

void LFormattedText::Draw( LVDrawBuf * buf, int x, int y, ldomMarkedRangeList * marks, ldomMarkedRangeList *bookmarks )
{
    int i, j;
    formatted_line_t * frmline;
    src_text_fragment_t * srcline;
    formatted_word_t * word;
    LVFont * font;
    lvRect clip;
    buf->GetClipRect( &clip );
    const lChar16 * str;
    int line_y = y;

    // We might need to translate "marks" (native highlights) from relative
    // coordinates to absolute coordinates if we have to draw floats or
    // inlineBoxes: we'll do that when dealing with the first of these if any.
    ldomMarkedRangeList * absmarks = new ldomMarkedRangeList();
    bool absmarks_update_needed = marks!=NULL && marks->length()>0;

    // printf("x/y: %d/%d clip.top/bottom: %d %d\n", x, y, clip.top, clip.bottom);
    // When drawing a paragraph that spans 3 pages, we may get:
    //   x/y: 9/407 clip.top/bottom: 13 559
    //   x/y: 9/-139 clip.top/bottom: 13 583
    //   x/y: 9/-709 clip.top/bottom: 13 545

    for (i=0; i<m_pbuffer->frmlinecount; i++)
    {
        if (line_y >= clip.bottom)
            break;
        frmline = m_pbuffer->frmlines[i];
        if (line_y + frmline->height > clip.top)
        {
            // process background

            //lUInt32 bgcl = buf->GetBackgroundColor();
            //buf->FillRect( x+frmline->x, y + frmline->y, x+frmline->x + frmline->width, y + frmline->y + frmline->height, bgcl );

            // draw background for each word
            // (if multiple consecutive words share the same bgcolor, this will
            // actually fill a single rect encompassing these words)
            // todo: the way background color (not inherited in lvrend.cpp) is
            // handled here (only looking at the style of the inline node
            // that contains the word, and not at its other inline parents),
            // some words may not get their proper bgcolor
            lUInt32 lastWordColor = 0xFFFFFFFF;
            int lastWordStart = -1;
            int lastWordEnd = -1;
            for (j=0; j<frmline->word_count; j++)
            {
                word = &frmline->words[j];
                srcline = &m_pbuffer->srctext[word->src_text_index];
                if (word->flags & LTEXT_WORD_IS_OBJECT)
                {
                    // no background, TODO
                }
                else if (word->flags & LTEXT_WORD_IS_INLINE_BOX)
                {
                    // background if any will be drawn when drawing the box below
                }
                else
                {
                    lUInt32 bgcl = srcline->bgcolor;
                    if ( lastWordColor!=bgcl || lastWordStart==-1 ) {
                        if ( lastWordStart!=-1 )
                            if ( ((lastWordColor>>24) & 0xFF) < 128 )
                                buf->FillRect( lastWordStart, y + frmline->y, lastWordEnd, y + frmline->y + frmline->height, lastWordColor );
                        lastWordColor=bgcl;
                        lastWordStart = x+frmline->x+word->x;
                    }
                    lastWordEnd = x+frmline->x+word->x+word->width;
                }
            }
            if ( lastWordStart!=-1 )
                if ( ((lastWordColor>>24) & 0xFF) < 128 )
                    buf->FillRect( lastWordStart, y + frmline->y, lastWordEnd, y + frmline->y + frmline->height, lastWordColor );

            // process marks
#ifndef CR_USE_INVERT_FOR_SELECTION_MARKS
            if ( marks!=NULL && marks->length()>0 ) {
                // Here is drawn the "native highlighting" of a selection in progress
                lvRect lineRect( frmline->x, frmline->y, frmline->x + frmline->width, frmline->y + frmline->height );
                for ( int i=0; i<marks->length(); i++ ) {
                    lvRect mark;
                    ldomMarkedRange * range = marks->get(i);
                    // printf("marks #%d %d %d > %d %d\n", i, range->start.x, range->start.y, range->end.x, range->end.y);
                    if ( range->intersects( lineRect, mark ) ) {
                        //
                        buf->FillRect(mark.left + x, mark.top + y, mark.right + x, mark.bottom + y, m_pbuffer->highlight_options.selectionColor);
                    }
                }
            }
            if (bookmarks!=NULL && bookmarks->length()>0) {
                lvRect lineRect( frmline->x, frmline->y, frmline->x + frmline->width, frmline->y + frmline->height );
                for ( int i=0; i<bookmarks->length(); i++ ) {
                    lvRect mark;
                    ldomMarkedRange * range = bookmarks->get(i);
                    if ( range->intersects( lineRect, mark ) ) {
                        //
                        DrawBookmarkTextUnderline(*buf, mark.left + x, mark.top + y, mark.right + x, mark.bottom + y, mark.bottom + y - 2, range->flags,
                                                  &m_pbuffer->highlight_options);
                    }
                }
            }
#endif
#ifdef CR_USE_INVERT_FOR_SELECTION_MARKS
            // process bookmarks
            if ( bookmarks != NULL && bookmarks->length() > 0 ) {
                lvRect lineRect( frmline->x, frmline->y, frmline->x + frmline->width, frmline->y + frmline->height );
                for ( int i=0; i<bookmarks->length(); i++ ) {
                    lvRect bookmark_rc;
                    ldomMarkedRange * range = bookmarks->get(i);
                    if ( range->intersects( lineRect, bookmark_rc ) ) {
                        buf->FillRect( bookmark_rc.left + x, bookmark_rc.top + y, bookmark_rc.right + x, bookmark_rc.bottom + y, 0xAAAAAA );
                    }
                }
            }
#endif

            int text_decoration_back_gap;
            lUInt16 lastWordSrcIndex;
            for (j=0; j<frmline->word_count; j++)
            {
                word = &frmline->words[j];
                if (word->flags & LTEXT_WORD_IS_OBJECT)
                {
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    ldomNode * node = (ldomNode *) srcline->object;
                    if (node) {
                        LVImageSourceRef img = node->getObjectImageSource();
                        if ( img.isNull() )
                            img = LVCreateDummyImageSource( node, word->width, word->o.height );
                        int xx = x + frmline->x + word->x;
                        int yy = line_y + frmline->baseline - word->o.height + word->y;
                        buf->Draw( img, xx, yy, word->width, word->o.height );
                        //buf->FillRect( xx, yy, xx+word->width, yy+word->height, 1 );
                    }
                }
                else if (word->flags & LTEXT_WORD_IS_INLINE_BOX)
                {
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    ldomNode * node = (ldomNode *) srcline->object;
                    // Logically, the coordinates of the top left of the box are:
                    // int x0 = x + frmline->x + word->x;
                    // int y0 = line_y + frmline->baseline - word->o.baseline + word->y;
                    // But we have updated the node's RenderRectAccesor x/y in alignLine(),
                    // ahd DrawDocument() will by default fetch them to shift the block
                    // it has to draw. So, we can use the provided x/y as-is, with
                    // the offsets from the RenderRectAccesor.
                    RenderRectAccessor fmt( node );
                    int x0 = x + fmt.getX();
                    int y0 = y + fmt.getY();
                    int doc_x = 0 - fmt.getX();
                    int doc_y = 0 - fmt.getY();
                    int dx = m_pbuffer->width;
                    int dy = frmline->height; // can be > m_pbuffer->page_height
                            // A frmline can be bigger than page_height, if
                            // this inlineBox contains many long paragraphs
                    int page_height = m_pbuffer->page_height;
                    if ( absmarks_update_needed ) {
                        getAbsMarksFromMarks(marks, absmarks, node);
                        absmarks_update_needed = false;
                    }
                    if ( node->isEmbeddedBlockBoxingInlineBox() ) {
                        // With embedded blocks, we shouldn't drop the clip (as we do next
                        // for regular inline-block boxes)
                        DrawDocument( *buf, node, x0, y0, dx, dy, doc_x, doc_y, page_height, absmarks, bookmarks );
                    }
                    else {
                        // inline-block boxes with negative margins can overflow the
                        // line height, and so possibly the page when that line is
                        // at top or bottom of page.
                        // When witnessed, that overflow was very small, and probably
                        // aimed at vertically aligning the box vs the text, but enough
                        // to have their glyphs truncated when clipped to the page rect.
                        // So, to avoid that, we just drop that clip when drawing the
                        // box, and restore it when done.
                        lvRect curclip;
                        buf->GetClipRect( &curclip ); // backup clip
                        buf->SetClipRect(NULL); // no clipping
                        DrawDocument( *buf, node, x0, y0, dx, dy, doc_x, doc_y, page_height, absmarks, bookmarks );
                        buf->SetClipRect(&curclip); // restore original page clip
                    }
                }
                else
                {
                    bool flgHyphen = false;
                    if ( word->flags&LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER) {
                        if (j==frmline->word_count-1)
                            flgHyphen = true;
                        // Also do that even if it's not the last word in the line
                        // AND the line is bidi: the hyphen may be in the middle of
                        // the text, but it's fine for some people with bidi, see
                        // conversation "Bidi reordering of soft hyphen" at:
                        //   https://unicode.org/pipermail/unicode/2014-April/thread.html#348
                        // If that's not desirable, just disable hyphenation lookup
                        // in processParagraph() if m_has_bidi or if chars found in
                        // line span multilple bidi levels (so that we don't get
                        // a blank space for a hyphen not drawn after this word).
                        else if (frmline->flags & LTEXT_LINE_IS_BIDI)
                            flgHyphen = true;
                    }
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    font = (LVFont *) srcline->t.font;
                    str = srcline->t.text + word->t.start;
                    /*
                    lUInt32 srcFlags = srcline->flags;
                    if ( srcFlags & LTEXT_BACKGROUND_MARK_FLAGS ) {
                        lvRect rc;
                        rc.left = x + frmline->x + word->x;
                        rc.top = line_y + (frmline->baseline - font->getBaseline()) + word->y;
                        rc.right = rc.left + word->width;
                        rc.bottom = rc.top + font->getHeight();
                        buf->FillRect( rc.left, rc.top, rc.right, rc.bottom, 0xAAAAAA );
                    }
                    */
                    // Check if we need to continue the text decoration from previous word.
                    // For now, we only ensure it if this word and previous one are in the
                    // same text node. We wrongly won't when one of these is in a sub <SPAN>
                    // because we can't detect that rightly at this point anymore...
                    text_decoration_back_gap = 0;
                    if (j > 0 && word->src_text_index == lastWordSrcIndex) {
                        text_decoration_back_gap = word->x - lastWordEnd;
                    }
                    lUInt32 oldColor = buf->GetTextColor();
                    lUInt32 oldBgColor = buf->GetBackgroundColor();
                    lUInt32 cl = srcline->color;
                    lUInt32 bgcl = srcline->bgcolor;
                    if ( cl!=0xFFFFFFFF )
                        buf->SetTextColor( cl );
                    if ( bgcl!=0xFFFFFFFF )
                        buf->SetBackgroundColor( bgcl );
                    // Add drawing flags: text decoration (underline...)
                    lUInt32 drawFlags = srcline->flags & LTEXT_TD_MASK;
                    // and chars direction, and if word begins or ends paragraph (for Harfbuzz)
                    drawFlags |= WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                    font->DrawTextString(
                        buf,
                        x + frmline->x + word->x,
                        line_y + (frmline->baseline - font->getBaseline()) + word->y,
                        str,
                        word->t.len,
                        '?',
                        NULL,
                        flgHyphen,
                        srcline->lang_cfg,
                        drawFlags,
                        srcline->letter_spacing + word->added_letter_spacing,
                        word->width,
                        text_decoration_back_gap);
                    /* To display the added letter spacing % at end of line
                    if (j == frmline->word_count-1 && word->added_letter_spacing ) {
                        // lString16 val = lString16::itoa(word->added_letter_spacing);
                        lString16 val = lString16::itoa(100*word->added_letter_spacing / font->getSize());
                        font->DrawTextString( buf, x + frmline->x + word->x + word->width + 10,
                            line_y + (frmline->baseline - font->getBaseline()) + word->y,
                            val.c_str(), val.length(), '?', NULL, false);
                    }
                    */
                    if ( cl!=0xFFFFFFFF )
                        buf->SetTextColor( oldColor );
                    if ( bgcl!=0xFFFFFFFF )
                        buf->SetBackgroundColor( oldBgColor );
                }
                lastWordSrcIndex = word->src_text_index;
                lastWordEnd = word->x + word->width;
            }

#ifdef CR_USE_INVERT_FOR_SELECTION_MARKS
            // process marks
            if ( marks!=NULL && marks->length()>0 ) {
                lvRect lineRect( frmline->x, frmline->y, frmline->x + frmline->width, frmline->y + frmline->height );
                for ( int i=0; i<marks->length(); i++ ) {
                    lvRect mark;
                    ldomMarkedRange * range = marks->get(i);
                    if ( range->intersects( lineRect, mark ) ) {
                        buf->InvertRect( mark.left + x, mark.top + y, mark.right + x, mark.bottom + y);
                    }
                }
            }
#endif
        }
        line_y += frmline->height;
    }

    // Draw floats if any
    for (i=0; i<m_pbuffer->floatcount; i++) {
        embedded_float_t * flt = m_pbuffer->floats[i];
        if (flt->srctext == NULL) {
            // Ignore outer floats (they are either fake footprint floats,
            // or real outer floats not to be drawn by us)
            continue;
        }
        ldomNode * node = (ldomNode *) flt->srctext->object;

        // Only some part of this float needs to be in the clip area.
        // Also account for the overflows, so we can render fully
        // floats with negative margins.
        RenderRectAccessor fmt( node );
        int top_overflow = fmt.getTopOverflow();
        int bottom_overflow = fmt.getBottomOverflow();
        // Note: some dropcaps may still not being draw in spite of this
        // because of the checks with _hidePartialGlyphs in lvdrawbuf.cpp
        // (todo: get rid of these _hidePartialGlyphs checks ?)

        if (y + flt->y - top_overflow < clip.bottom && y + flt->y + flt->height + bottom_overflow > clip.top) {
            // DrawDocument() parameters (y0 + doc_y must be equal to our y,
            // doc_y just shift the viewport, so anything outside is not drawn).
            int x0 = x + flt->x;
            int y0 = y + flt->y;
            int doc_x = 0 - flt->x;
            int doc_y = 0 - flt->y;
            int dx = m_pbuffer->width;
            int dy = m_pbuffer->page_height;
            int page_height = m_pbuffer->page_height;
            if ( absmarks_update_needed ) {
                getAbsMarksFromMarks(marks, absmarks, node);
                absmarks_update_needed = false;
            }
            DrawDocument( *buf, node, x0, y0, dx, dy, doc_x, doc_y, page_height, absmarks, bookmarks );
        }
    }
    delete absmarks;
}

#endif
