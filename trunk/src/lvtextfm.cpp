/*******************************************************

   CoolReader Engine C-compatible API

   lvtextfm.cpp:  Text formatter

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/lvfnt.h"
#include "../include/lvtextfm.h"
#include "../include/lvdrawbuf.h"

#ifdef __cplusplus
#include "../include/lvimg.h"
#include "../include/lvtinydom.h"
#endif

formatted_line_t * lvtextAllocFormattedLine( )
{
    formatted_line_t * pline = (formatted_line_t *)malloc(sizeof(formatted_line_t));
    memset( pline, 0, sizeof(formatted_line_t) );
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
    lUInt32 size = (pline->word_count + 7) / 8 * 8;
    if ( pline->word_count >= size)
    {
        size += 8;
        pline->words = (formatted_word_t*)realloc( pline->words, sizeof(formatted_word_t)*(size) );
    }
    return &pline->words[ pline->word_count++ ];
}

formatted_line_t * lvtextAddFormattedLine( formatted_text_fragment_t * pbuffer )
{
    lUInt32 size = (pbuffer->frmlinecount + 7) / 8 * 8;
    if ( pbuffer->frmlinecount >= size)
    {
        size += 8;
        pbuffer->frmlines = (formatted_line_t**)realloc( pbuffer->frmlines, sizeof(formatted_line_t*)*(size) );
    }
    return (pbuffer->frmlines[ pbuffer->frmlinecount++ ] = lvtextAllocFormattedLine());
}

formatted_text_fragment_t * lvtextAllocFormatter( lUInt16 width )
{
    formatted_text_fragment_t * pbuffer = (formatted_text_fragment_t*)malloc( sizeof(formatted_text_fragment_t) );
    memset( pbuffer, 0, sizeof(formatted_text_fragment_t));
    pbuffer->width = width;
    return pbuffer;
}

void lvtextFreeFormatter( formatted_text_fragment_t * pbuffer )
{
    if (pbuffer->srctext)
    {
        for (lUInt32 i=0; i<pbuffer->srctextlen; i++)
        {
            if (pbuffer->srctext[i].flags & LTEXT_FLAG_OWNTEXT)
                free( (void*)pbuffer->srctext[i].t.text );
        }
        free( pbuffer->srctext );
    }
    if (pbuffer->frmlines)
    {
        for (lUInt32 i=0; i<pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( pbuffer->frmlines[i] );
        }
        free( pbuffer->frmlines );
    }
    free(pbuffer);
}

/*
   Reformats source lines stored in buffer into formatted lines
   Returns height (in pixels) of formatted text
*/
lUInt32 lvtextResize( formatted_text_fragment_t * pbuffer, int width, int page_height )
{
    if (pbuffer->frmlines)
    {
        for (lUInt32 i=0; i<pbuffer->frmlinecount; i++)
        {
            lvtextFreeFormattedLine( pbuffer->frmlines[i] );
        }
        free( pbuffer->frmlines );
    }
    pbuffer->frmlines = NULL;
    pbuffer->frmlinecount = 0;
    pbuffer->width = width;
    pbuffer->height = 0;
    pbuffer->page_height = page_height;
    return lvtextFormat( pbuffer );
}

void lvtextAddSourceLine( formatted_text_fragment_t * pbuffer,
   lvfont_handle   font,     /* handle of font to draw string */
   const lChar16 * text,     /* pointer to unicode text string */
   lUInt32         len,      /* number of chars in text, 0 for auto(strlen) */
   lUInt16         flags,    /* flags */
   lUInt8          interval, /* interline space, *16 (16=single, 32=double) */
   lUInt16         margin,   /* first line margin */
   void *          object    /* pointer to custom object */
                         )
{
    lUInt32 srctextsize = (pbuffer->srctextlen + 3) / 4 * 4;
    if ( pbuffer->srctextlen >= srctextsize)
    {
        srctextsize += 4;
        pbuffer->srctext = (src_text_fragment_t*)realloc( pbuffer->srctext, sizeof(src_text_fragment_t)*(srctextsize) );
    }
    src_text_fragment_t * pline = &pbuffer->srctext[ pbuffer->srctextlen++ ];
    pline->t.font = font;
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
    pline->object = object;
    pline->t.len = (lUInt16)len;
    pline->margin = margin;
    pline->flags = flags;
    pline->interval = interval;
}

void lvtextAddSourceObject( 
   formatted_text_fragment_t * pbuffer,
   lUInt16         width,
   lUInt16         height,
   lUInt16         flags,    /* flags */
   lUInt8          interval, /* interline space, *16 (16=single, 32=double) */
   lUInt16         margin,   /* first line margin */
   void *          object    /* pointer to custom object */
                         )
{
    lUInt32 srctextsize = (pbuffer->srctextlen + 3) / 4 * 4;
    if ( pbuffer->srctextlen >= srctextsize)
    {
        srctextsize += 4;
        pbuffer->srctext = (src_text_fragment_t*)realloc( pbuffer->srctext, sizeof(src_text_fragment_t)*(srctextsize) );
    }
    src_text_fragment_t * pline = &pbuffer->srctext[ pbuffer->srctextlen++ ];
    pline->o.width = width;
    pline->o.height = height;
    pline->object = object;
    pline->margin = margin;
    pline->flags = flags | LTEXT_SRC_IS_OBJECT;
    pline->interval = interval;
}

int lvtextFinalizeLine( formatted_line_t * frmline, int width, int align,
        lUInt16 * pSrcIndex, lUInt16 * pSrcOffset )
{
    int margin = frmline->x;
    int delta = 0;
    unsigned int i;
    unsigned short w;
    int expand_count = 0;
    int expand_dx, expand_dd;
    int flgRollback = 0;

    if (pSrcIndex!=NULL)
    {
        /* check whether words rollback is necessary */
        for (i=frmline->word_count-1; i>0; i--)
        {
            if ( (frmline->words[i].flags & LTEXT_WORD_CAN_BREAK_LINE_AFTER) )
                break;
            if ( (frmline->words[i].flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER) )
                break;
        }
        if (/*i > 0 && */i < frmline->word_count-1)
        {
            /* rollback */
            *pSrcIndex = frmline->words[i+1].src_text_index;
            *pSrcOffset = frmline->words[i+1].t.start;
            frmline->word_count = i+1;
            flgRollback = 1;
        }
    }

    frmline->width = 0;
    for (i=0; i<frmline->word_count; i++)
    {
        if (i == frmline->word_count-1)
            w = frmline->words[i].x;
        else
            w = frmline->words[i].width;
        frmline->words[i].x = frmline->width;
        frmline->width += w;
    }

    if (align == LTEXT_ALIGN_LEFT)
        return flgRollback;

    delta = width - frmline->width - margin;

    if (align == LTEXT_ALIGN_CENTER)
        delta /= 2;
    if ( align == LTEXT_ALIGN_CENTER || align == LTEXT_ALIGN_RIGHT )
    {
        frmline->x += delta;
    }
    else
    {
        /* LTEXT_ALIGN_WIDTH */
        for (i=0; i<frmline->word_count-1; i++)
        {
            if (frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER)
                expand_count++;
        }
        if (expand_count)
        {
            expand_dx = delta / expand_count;
            expand_dd = delta % expand_count;
            delta = 0;
            for (i=0; i<frmline->word_count-1; i++)
            {
                if (frmline->words[i].flags & LTEXT_WORD_CAN_ADD_SPACE_AFTER)
                {
                    delta += expand_dx;
                    if (expand_dd>0)
                    {
                        delta++;
                        expand_dd--;
                    }
                }
                frmline->words[i+1].x += delta;
            }
        }
    }
    return flgRollback;
}

#define DEPRECATED_LINE_BREAK_WORD_COUNT    3
#define DEPRECATED_LINE_BREAK_SPACE_LIMIT   64

lUInt32 lvtextFormat( formatted_text_fragment_t * pbuffer )
{
    lUInt16   line_flags;
    lUInt16 * widths_buf = NULL;
    lUInt8  * flags_buf = NULL;
    lUInt32   widths_buf_size = 0;
    lUInt16   chars_measured = 0;
    lUInt16   curr_x = 0;
    lUInt16   space_left = 0;
    lUInt16   text_offset = 0;
    lUInt8    align = LTEXT_ALIGN_LEFT;
    int flgRollback = 0;
    int h, b;
    src_text_fragment_t * srcline = NULL;
    src_text_fragment_t * first_para_line = NULL;
    LVFont * font = NULL;
    formatted_line_t * frmline = NULL;
    formatted_word_t * word = NULL;
    lUInt16 i, j, last_fit, chars_left;
    lUInt16 wstart = 0;
    lUInt16 wpos = 0;
    int phase;
    int isParaStart;
    int flgObject;

    pbuffer->height = 0;
    /* For each source line: */
    for (i = 0; i<pbuffer->srctextlen; i++)
    {
        srcline = &pbuffer->srctext[i];
        line_flags = srcline->flags;
        flgObject = (line_flags & LTEXT_SRC_IS_OBJECT) ? 1 : 0;
        if (!flgObject)
        {
            font = (LVFont*)srcline->t.font;
        }
        text_offset = 0;
        if ( i==0 && !(line_flags & LTEXT_FLAG_NEWLINE) )
            line_flags |= LTEXT_ALIGN_LEFT; /* first paragraph -- left by default */
        if (line_flags & LTEXT_FLAG_NEWLINE)
            first_para_line = srcline;
        if (!flgObject && (int)widths_buf_size < (int)srcline->t.len + 64) //
        {
            widths_buf_size = srcline->t.len + 64;
            widths_buf = (lUInt16 *) realloc( widths_buf, widths_buf_size * sizeof(lUInt16) );
            flags_buf  = (lUInt8 *) realloc( flags_buf, widths_buf_size * sizeof(lUInt8) );
        }

        while (flgObject || srcline->t.len > text_offset)
        {
            do {
                flgRollback = 0;
                space_left = pbuffer->width - curr_x -  (frmline?frmline->x:0);
                if (flgObject)
                {
                    chars_left = 1;
                    isParaStart = (line_flags & LTEXT_FLAG_NEWLINE);
                }
                else
                {
                    chars_left = srcline->t.len - text_offset;
                    isParaStart = (line_flags & LTEXT_FLAG_NEWLINE) && text_offset==0;
                    /* measure line */
                    chars_measured = font->measureText( 
                        text_offset + srcline->t.text, 
                        chars_left, 
                        widths_buf, flags_buf,
                        pbuffer->width,
                        '?');
                }
                phase = (!frmline || isParaStart) ? 1 : 0; //
                for ( ; phase<2; ++phase)
                {
                    /* add new formatted line in phase 1 */
                    if (phase == 1)
                    {
                        if (frmline)
                        {
                            flgRollback = lvtextFinalizeLine( frmline, pbuffer->width, 
                                   (isParaStart && align==LTEXT_ALIGN_WIDTH)?LTEXT_ALIGN_LEFT:align, 
                                   &i, &text_offset);
                        }
                        frmline = lvtextAddFormattedLine( pbuffer );
                        curr_x = 0;
                        frmline->x = isParaStart ? srcline->margin : 0;
                        space_left = pbuffer->width - curr_x - frmline->x;
                        if (flgRollback)
                        {
                            srcline = &pbuffer->srctext[i];
                            line_flags = srcline->flags;
                            flgObject = (line_flags & LTEXT_SRC_IS_OBJECT) ? 1 : 0;
                            font = (LVFont*)srcline->t.font;
                            for (j=i; j>0; j--)
                            {
                                if (pbuffer->srctext[j].flags & LTEXT_FLAG_NEWLINE)
                                    break;
                            }
                            first_para_line = &pbuffer->srctext[j];
                        }
                        align = first_para_line->flags & LTEXT_FLAG_NEWLINE;
                        if (!align)
                            align = LTEXT_ALIGN_LEFT;
                        if (flgRollback)
                            break;
                    }
                    last_fit = 0xFFFF;
                    if (flgObject)
                    {
                        if ( srcline->o.width <= space_left)
                        {
                            last_fit = 0;
                            break;
                        }
                        if (phase==0)
                            continue;
                    }
                    /* try to find good place for line break */
                    for (j = 0; j<chars_left && j<chars_measured; j++)
                    {
                        if (widths_buf[j] > space_left)
                            break;
                        if (flags_buf[j] & LCHAR_ALLOW_WRAP_AFTER)
                            last_fit = j;
                        if (flags_buf[j] & LCHAR_ALLOW_HYPH_WRAP_AFTER)
                            last_fit = j;
                    }
                    if (j==chars_left && widths_buf[j-1] <= space_left)
                        last_fit = j-1;
                    if (last_fit!=0xFFFF)
                        break;
                    /* avoid using deprecated line breaks if line is filled enough */
                    if (phase==0 && space_left<pbuffer->width*DEPRECATED_LINE_BREAK_SPACE_LIMIT/256 
                        && frmline->word_count>=DEPRECATED_LINE_BREAK_WORD_COUNT )
                        continue;
                    /* try to find deprecated place for line break if good is not found */
                    for (j = 0; j<chars_left && j<chars_measured; j++)
                    {
                        if (widths_buf[j] > space_left)
                            break;
                        if (flags_buf[j] & LCHAR_DEPRECATED_WRAP_AFTER)
                            last_fit = j;
                    }
                    if (last_fit!=0xFFFF)
                        break;
                    if (phase==0)
                        continue;
                    /* try to wrap in the middle of word */
                    for (j = 0; j<chars_left && j<chars_measured; j++)
                    {
                        if (widths_buf[j] > space_left)
                            break;
                    }
                    if (j)
                        last_fit = j - 1;
                    else
                        last_fit = 0;
                }
            } while (flgRollback);
            /* calc vertical align */
            //int vertical_align = pbuffer->srctext[i].flags & LTEXT_VALIGN_MASK;
            int vertical_align = srcline->flags & LTEXT_VALIGN_MASK;
            int wy = 0;
            if (!flgObject && vertical_align)
            {
                int fh = font->getHeight();
                if ( vertical_align == LTEXT_VALIGN_SUB )
                {
                    wy += fh / 2;
                }
                else if ( vertical_align == LTEXT_VALIGN_SUPER )
                {
                    wy -= fh / 2;
                }
            }
            /* add new words to frmline */
            if (flgObject)
            {
                // object
                word = lvtextAddFormattedWord( frmline );
                word->src_text_index = i;
                int scale_div = 1;
                int scale_mul = 1;
                int div_x = (srcline->o.width / pbuffer->width) + 1;
                int div_y = (srcline->o.height / pbuffer->page_height) + 1;
#if (MAX_IMAGE_SCALE_MUL==3)
                if ( srcline->o.height*3 < pbuffer->page_height-20 
                        && srcline->o.width*3 < pbuffer->width - 20 )
                    scale_mul = 3;
                else 
#endif
#if (MAX_IMAGE_SCALE_MUL==2) || (MAX_IMAGE_SCALE_MUL==3)
                    if ( srcline->o.height*2 < pbuffer->page_height-20 
                        && srcline->o.width*2 < pbuffer->width - 20 )
                    scale_mul = 2;
                else 
#endif
                if (div_x>1 || div_y>1) {
                    if (div_x>div_y)
                        scale_div = div_x;
                    else
                        scale_div = div_y;
                }
                word->o.height = srcline->o.height * scale_mul / scale_div;
                word->width = srcline->o.width * scale_mul / scale_div;
                word->flags = LTEXT_WORD_IS_OBJECT;
                word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
                word->y = 0;
                word->x = word->width;
                frmline->width += word->width;
            }
            else
            {
                // text string
                word = NULL;
                for (j=0, wstart=0, wpos=0; j<=last_fit; j++)
                {
                    if (flags_buf[j] & LCHAR_IS_SPACE || j==last_fit) /* LCHAR_ALLOW_WRAP_AFTER */
                    {
                        word = lvtextAddFormattedWord( frmline );
                        word->src_text_index = i;
                        word->t.len = j - wstart + 1;
                        word->width = widths_buf[j] - wpos;
                        word->t.start = text_offset + wstart;
                        word->flags = 0;
                        word->y = wy;
                        word->x = widths_buf[j] - wpos;
                        if (flags_buf[j] & LCHAR_IS_SPACE)
                            word->flags |= LTEXT_WORD_CAN_ADD_SPACE_AFTER;
                        if (flags_buf[j] & LCHAR_ALLOW_WRAP_AFTER)
                            word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
                        if (flags_buf[j] & LCHAR_ALLOW_HYPH_WRAP_AFTER)
                            word->flags |= LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER;
                        if ( text_offset+j == srcline->t.len-1 )
                        {
                            /* last char of src fragment */
                            if (i==pbuffer->srctextlen-1 || pbuffer->srctext[i+1].flags & LTEXT_FLAG_NEWLINE)
                                word->flags |= LTEXT_WORD_CAN_BREAK_LINE_AFTER;
                        }
                        for (int jj=j; jj>0 && (flags_buf[jj] & LCHAR_IS_SPACE); jj--)
                            word->x = widths_buf[jj-1] - wpos;
                        wpos = widths_buf[j];
                        wstart = j+1;
                        frmline->width += word->width;
                    }
                }
            }
            /* update Y positions of line */
            if (flgObject)
            {
                b = word->o.height;
                h = 0;
            }
            else
            {
                if (wy!=0)
                {
                    // subscript or superscript
                    b = font->getBaseline();
                    h = font->getHeight() - b;
                }
                else
                {
                    b = (( font->getBaseline() * first_para_line->interval) >> 4);
                    h = ( ( font->getHeight() * first_para_line->interval) >> 4) - b;
                }
            }
            if ( frmline->baseline < b - wy )
                frmline->baseline = (lUInt16) (b - wy);
            if ( frmline->height < frmline->baseline + h )
                frmline->height = (lUInt16) ( frmline->baseline + h );
            
            if (flgObject)
            {
                curr_x += word->width;
                break;
            }
            /* skip formatted text in source line */
            text_offset += wstart;
            curr_x += wpos;
        }
    }
    /* finish line formatting */
    if (frmline)
        lvtextFinalizeLine( frmline, pbuffer->width, 
        (align==LTEXT_ALIGN_WIDTH)?LTEXT_ALIGN_LEFT:align, NULL, NULL );
    /* cleanup */
    if (widths_buf)
        free(widths_buf);
    if (flags_buf)
        free(flags_buf);
    if (pbuffer->frmlinecount)
    {
        int y = 0;
        for (lUInt16 i=0; i<pbuffer->frmlinecount; i++)
        {
            pbuffer->frmlines[i]->y = y;
            y += pbuffer->frmlines[i]->height;
        }
        pbuffer->height = y;
    }
    return pbuffer->height;
}


/*
   Draws formatted text to draw buffer
*/
void lvtextDraw( formatted_text_fragment_t * text, draw_buf_t * buf, int x, int y )
{
    lUInt32 i, j;
    formatted_line_t * frmline;
    src_text_fragment_t * srcline;
    formatted_word_t * word;
    lvfont_header_t * font;
    const lChar16 * str;
    int line_y = y;
    for (i=0; i<text->frmlinecount; i++)
    {
        frmline = text->frmlines[i];
        for (j=0; j<frmline->word_count; j++)
        {
            word = &frmline->words[j];
            //int flg = 0;
            srcline = &text->srctext[word->src_text_index];
            font = (lvfont_header_t *) ( ((LVFont*)srcline->t.font)->GetHandle() );
            str = srcline->t.text + word->t.start;
            lvdrawbufDrawText( buf, 
                x + frmline->x + word->x,
                line_y + (frmline->baseline - font->fontBaseline) + word->y, 
                (lvfont_handle)font,
                str, 
                word->t.len,
                '?' );
        }
        line_y += frmline->height;
    }
}

#ifdef __cplusplus

void LFormattedText::AddSourceObject(
            lUInt16         flags,    /* flags */
            lUInt8          interval, /* interline space, *16 (16=single, 32=double) */
            lUInt16         margin,   /* first line margin */
            void *          object    /* pointer to custom object */
     )
{
    ldomElement * node = (ldomElement*)object;
    LVImageSourceRef img = node->getObjectImageSource();
    if ( img.isNull() )
        img = LVCreateDummyImageSource( node, 50, 50 );
    lUInt16 width = (lUInt16)img->GetWidth();
    lUInt16 height = (lUInt16)img->GetHeight();
    lvtextAddSourceObject(m_pbuffer, 
        width, height,
        flags, interval, margin, object );
}

void LFormattedText::Draw( LVDrawBuf * buf, int x, int y )
{
    lUInt32 i, j;
    formatted_line_t * frmline;
    src_text_fragment_t * srcline;
    formatted_word_t * word;
    LVFont * font;
    lvRect clip;
    buf->GetClipRect( &clip );
    const lChar16 * str;
    int line_y = y;
    for (i=0; i<m_pbuffer->frmlinecount; i++)
    {
        if (line_y>=clip.bottom)
            break;
        frmline = m_pbuffer->frmlines[i];
        if (line_y + frmline->height>=clip.top)
        {
            for (j=0; j<frmline->word_count; j++)
            {
                word = &frmline->words[j];
                if (word->flags & LTEXT_WORD_IS_OBJECT)
                {
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    ldomElement * node = (ldomElement *) srcline->object;
                    LVImageSourceRef img = node->getObjectImageSource();
                    if ( img.isNull() )
                        img = LVCreateDummyImageSource( node, word->width, word->o.height );
                    int xx = x + frmline->x + word->x;
                    int yy = line_y + frmline->baseline - word->o.height + word->y;
                    buf->Draw( img, xx, yy, word->width, word->o.height );
                    //buf->FillRect( xx, yy, xx+word->width, yy+word->height, 1 );
                }
                else
                {
                    bool flgHyphen = false;
                    if ( (j==frmline->word_count-1) &&
                        (word->flags&LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER))
                        flgHyphen = true;
                    srcline = &m_pbuffer->srctext[word->src_text_index];
                    font = (LVFont *) srcline->t.font;
                    str = srcline->t.text + word->t.start;
                    font->DrawTextString(
                        buf,
                        x + frmline->x + word->x,
                        line_y + (frmline->baseline - font->getBaseline()) + word->y, 
                        str, 
                        word->t.len,
                        '?',
                        NULL,
                        flgHyphen);
                }
            }
        }
        line_y += frmline->height;
    }
}

#endif
