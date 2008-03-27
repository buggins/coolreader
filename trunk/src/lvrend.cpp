/*******************************************************

   CoolReader Engine

   lvrend.cpp:  XML DOM tree rendering tools

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <string.h>
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvrend.h"

//#define DEBUG_TREE_DRAW 3
#define DEBUG_TREE_DRAW 0

#ifdef _DEBUG
//#define DEBUG_DUMP_ENABLED
#endif

#ifdef DEBUG_DUMP_ENABLED

class simpleLogFile
{
public:
    FILE * f;
    simpleLogFile(const char * fname) { f = fopen( fname, "wt" ); }
    ~simpleLogFile() { if (f) fclose(f); }
    simpleLogFile & operator << ( const char * str ) { fprintf( f, "%s", str ); fflush( f ); return *this; }
    //simpleLogFile & operator << ( int d ) { fprintf( f, "%d(0x%X) ", d, d ); fflush( f ); return *this; }
    simpleLogFile & operator << ( int d ) { fprintf( f, "%d ", d ); fflush( f ); return *this; }
    simpleLogFile & operator << ( const wchar_t * str )
    { 
        if (str)
        {
            for (; *str; str++ ) 
            {
                fputc( *str >= 32 && *str<127 ? *str : '?', f );
            }
        }
        fflush( f );
        return *this; 
    }
    simpleLogFile & operator << ( const lString16 &str ) { return operator << (str.c_str()); }
};

simpleLogFile logfile("logfile.log");

#else

// stubs
class simpleLogFile
{
public:
    simpleLogFile & operator << ( const char * str ) { return *this; }
    simpleLogFile & operator << ( int d ) { return *this; }
    simpleLogFile & operator << ( const wchar_t * str ) { return *this; }
    simpleLogFile & operator << ( const lString16 &str ) { return *this; }
};

simpleLogFile logfile;

#endif


void freeFormatData( ldomNode * node )
{
    node->setRenderData( NULL );
}

bool isSameFontStyle( css_style_rec_t * style1, css_style_rec_t * style2 )
{
    return (style1->font_family == style2->font_family)
        && (style1->font_size == style2->font_size)
        && (style1->font_style == style2->font_style)
        && (style1->font_name == style2->font_name)
        && (style1->font_weight == style2->font_weight);
}

LVFontRef getFont( css_style_rec_t * style )
{
    int sz = style->font_size.value;
    int fw;
    if (style->font_weight>=css_fw_100 && style->font_weight<=css_fw_900)
        fw = ((style->font_weight - css_fw_100)+1) * 100;
    else
        fw = 400;
    LVFontRef fnt = fontMan->GetFont(
        sz, 
        fw,
        style->font_style==css_fs_italic,
        style->font_family,
        lString8(style->font_name.c_str()) );
    return fnt;
}

void initFormatData( ldomNode * node )
{
    //lvdomElementFormatRec * fmt = new lvdomElementFormatRec;
    //node->setRenderData( fmt );
    if ( node->isRoot() || node->getParentNode()->isRoot() )
    {
        setNodeStyle( node,
            node->getDocument()->getDefaultStyle(),
            node->getDocument()->getDefaultFont()
        );
    }
    else
    {
        ldomElement * parent = node->getParentNode();
        //lvdomElementFormatRec * parent_fmt = node->getParentNode()->getRenderData();
        setNodeStyle( node,
            parent->getStyle(),
            parent->getFont()
            );
    }
}

void initRendMethod( ldomNode * node )
{
    if ( node->getNodeType()==LXML_ELEMENT_NODE )
    {
        ldomElement * enode = (ldomElement *)node;
        if (enode->getStyle()->display == css_d_none)
        {
            enode->setRendMethod( erm_invisible );
            return;
        }
        int cnt = node->getChildCount();
        int textCount = 0;
        int inlineCount = 0;
        int blockCount = 0;
        int runinCount = 0;
        if (enode->getNodeId() == el_empty_line)
            cnt = cnt;
        int i;
        for (i=0; i<cnt; i++)
        {
            ldomElement * child = (ldomElement *)enode->getChildNode( i );
            if ( child->getNodeType()==LXML_ELEMENT_NODE )
            {
                initRendMethod( child );
                //lvdomElementFormatRec * childfmt = child->getRenderData();
                switch( child->getStyle()->display )
                {
                case css_d_inline:
                    if ( child->getRendMethod() != erm_invisible )
                        inlineCount++; // count visible inline elements only
                    break;
                case css_d_none:
                    break;
                case css_d_run_in:
                    if ( child->getRendMethod() != erm_invisible )
                        runinCount++;
                    break;
                default:
                    if ( child->getRendMethod() != erm_invisible )
                    {
                        blockCount++; // count visible blocks only
                    }
                    break;
                }
            }
            else if ( child->getNodeType()==LXML_TEXT_NODE )
            {
                textCount++;
            }
        }
#ifdef DEBUG_DUMP_ENABLED
      for (i=0; i<node->getNodeLevel(); i++)
        logfile << " . ";
#endif
#ifdef DEBUG_DUMP_ENABLED
        lvRect rect;
        node->getAbsRect( rect );
        logfile << "<" << node->getNodeName() << ">     text:" 
            << textCount << " inline: " << inlineCount
            << " block: " << blockCount << "   rendMethod: ";
#endif
        
        const elem_def_t * ntype = node->getElementTypePtr();
        if ( textCount || inlineCount || runinCount )
        {
            // if there are inline or text in block, make it final
            enode->setRendMethod( erm_final );
#ifdef DEBUG_DUMP_ENABLED
            logfile << "final";
#endif
        }
        else if ( blockCount )
        {
            // if there are blocks only inside element, treat it as block too
            enode->setRendMethod( erm_block );
#ifdef DEBUG_DUMP_ENABLED
            logfile << "block";
#endif
        }
        else if (ntype && ntype->props.is_object)
        {
            switch ( enode->getStyle()->display )
            {
            case css_d_block:
            case css_d_inline:
            case css_d_run_in:
                enode->setRendMethod( erm_final );
#ifdef DEBUG_DUMP_ENABLED
                logfile << "object final";
#endif
                break;
            default:
                enode->setRendMethod( erm_invisible );
                break;
            }
        }
        else if (enode->getStyle()->display != css_d_none )
        {
            // empty element: may be visible (i.e. <empty-line>)
            enode->setRendMethod( erm_block );
#ifdef DEBUG_DUMP_ENABLED
            logfile << "block";
#endif
        }
        else
        {
            // empty element: make it invisible
            enode->setRendMethod( erm_invisible );
#ifdef DEBUG_DUMP_ENABLED
            logfile << "invisible";
#endif
        }
#ifdef DEBUG_DUMP_ENABLED
        logfile << "\n";
#endif
    }
}

int styleToTextFmtFlags( const css_style_ref_t & style, int oldflags )
{
    int flg = oldflags;
    if ( style->display == css_d_run_in ) {
        flg |= LTEXT_RUNIN_FLAG;
    } else if (style->display != css_d_inline) {
        // text alignment flags
        flg = oldflags & ~LTEXT_FLAG_NEWLINE;
        if ( !(oldflags & LTEXT_RUNIN_FLAG) ) {
            switch (style->text_align)
            {
            case css_ta_left:
                flg |= LTEXT_ALIGN_LEFT;
                break;
            case css_ta_right:
                flg |= LTEXT_ALIGN_RIGHT;
                break;
            case css_ta_center:
                flg |= LTEXT_ALIGN_CENTER;
                break;
            case css_ta_justify:
                flg |= LTEXT_ALIGN_WIDTH;
                break;
            case css_ta_inherit:
                break;
            }
        }
    }
    //flg |= oldflags & ~LTEXT_FLAG_NEWLINE;
    return flg;
}

int lengthToPx( css_length_t val, int base_px, int base_em )
{
    switch( val.type )
    {
    case css_val_px:
        // nothing to do
        return val.value;
    case css_val_ex: // not implemented: treat as em 
    case css_val_em: // value = em*256
        return ( (base_em * val.value) >> 8 );
    case css_val_percent:
        return ( (base_px * val.value) / 100 );
    case css_val_unspecified:
    case css_val_in: // 2.54 cm
    case css_val_cm:
    case css_val_mm:
    case css_val_pt: // 1/72 in
    case css_val_pc: // 12 pt
    case css_val_inherited:
    default:
        // not supported: treat as 0
        return 0;
    }
}


//=======================================================================
// Render final block
//=======================================================================
void renderFinalBlock( ldomNode * node, LFormattedText * txform, lvdomElementFormatRec * fmt, int & baseflags, int ident, int line_h )
{
    if ( node->getNodeType()==LXML_ELEMENT_NODE )
    {
        ldomElement * enode = (ldomElement *) node; 
        fmt = node->getRenderData();
        if ( enode->getRendMethod() == erm_invisible )
            return; // don't draw invisible
        int flags = styleToTextFmtFlags( enode->getStyle(), baseflags );
        if (flags & LTEXT_FLAG_NEWLINE)
        {
            css_length_t len = enode->getStyle()->text_indent;
            switch( len.type )
            {
            case css_val_percent:
                ident = fmt->getWidth() * len.value / 100;
                break;
            case css_val_px:
                ident = len.value;
                break;
            case css_val_em:
                ident = len.value * enode->getFont()->getHeight() / 256;
                break;
            default:
                ident = 0;
                break;
            }
            len = enode->getStyle()->line_height;
            switch( len.type )
            {
            case css_val_percent:
                line_h = len.value * 16 / 100;
                break;
            case css_val_px:
                line_h = len.value * 16 / enode->getFont()->getHeight();
                break;
            case css_val_em:
                line_h = len.value * 16;
                break;
            default:
                break;
            }
        }
        // save flags
        int f = flags;
        // vertical alignment flags
        switch (enode->getStyle()->vertical_align)
        {
        case css_va_sub:
            flags |= LTEXT_VALIGN_SUB;
            break;
        case css_va_super:
            flags |= LTEXT_VALIGN_SUPER;
            break;
        case css_va_baseline:
        default:
            break;
        }
        switch ( enode->getStyle()->text_decoration ) {
        case css_td_underline:
            flags |= LTEXT_TD_UNDERLINE;
            break;
        case css_td_overline:
            flags |= LTEXT_TD_OVERLINE;
            break;
        case css_td_line_through:
            flags |= LTEXT_TD_LINE_THROUGH;
            break;
        case css_td_blink:
            flags |= LTEXT_TD_BLINK;
            break;
        default:
            break;
        }
        const elem_def_t * ntype = node->getElementTypePtr();
        if ( ntype && ntype->props.is_object )
        {
#ifdef DEBUG_DUMP_ENABLED
            logfile << "+OBJECT ";
#endif
            // object element, like <IMG>
            txform->AddSourceObject(baseflags, line_h, ident, node );
            baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
        }
        else
        {
            int cnt = node->getChildCount();
#ifdef DEBUG_DUMP_ENABLED
            logfile << "+BLOCK [" << cnt << "]";
#endif
            // usual elements
            int runin_count = 0;
            for (int i=0; i<cnt; i++)
            {
                ldomNode * child = node->getChildNode( i );
                renderFinalBlock( child, txform, fmt, flags, ident, line_h );
                //flags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
                if ( flags & LTEXT_RUNIN_FLAG ) {
                    runin_count++;
                    if ( runin_count>1 ) {
                        runin_count = 0;
                        flags &= ~LTEXT_RUNIN_FLAG;
                    } else if ( i<cnt-1 && child->getNodeType()==LXML_ELEMENT_NODE && ((ldomElement*)child)->getStyle()->display==css_d_run_in ) {
                        // append space to run-in object
                        LVFont * font = enode->getFont().get();
                        css_style_ref_t style = enode->getStyle();
                        lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                        lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                        lChar16 delimiter[] = {160, 160}; //160
                        txform->AddSourceLine( delimiter, sizeof(delimiter)/sizeof(lChar16), cl, bgcl, font, LTEXT_FLAG_OWNTEXT, line_h, 0, NULL );
                    }
                }
            }
        }


#ifdef DEBUG_DUMP_ENABLED
      for (int i=0; i<node->getNodeLevel(); i++)
        logfile << " . ";
#endif
#ifdef DEBUG_DUMP_ENABLED
        lvRect rect;
        node->getAbsRect( rect );
        logfile << "<" << node->getNodeName() << ">     flags( " 
            << baseflags << "-> " << flags << ")  rect( " 
            << rect.left << rect.top << rect.right << rect.bottom << ")\n";
#endif

        // restore flags
        //***********************************
        baseflags = f; // to allow blocks in one level with inlines
        baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
        //baseflags &= ~LTEXT_RUNIN_FLAG;
    }
    else if ( node->getNodeType()==LXML_TEXT_NODE )
    {
        // text nodes
        lString16 txt = node->getText();
        if ( !txt.empty() )
        {
            
#ifdef DEBUG_DUMP_ENABLED
      for (int i=0; i<node->getNodeLevel(); i++)
        logfile << " . ";
#endif
#ifdef DEBUG_DUMP_ENABLED
            logfile << "#text" << " flags( " 
                << baseflags << ")\n";
#endif

            ldomElement * parent = ((ldomElement*)node->getParentNode());
            int tflags = LTEXT_FLAG_OWNTEXT;
            if ( parent->getNodeId() == el_a )
                tflags |= LTEXT_IS_LINK;
            LVFont * font = parent->getFont().get();
            css_style_ref_t style = parent->getStyle();
            lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
            lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
            txform->AddSourceLine( txt.c_str(), txt.length(), cl, bgcl, font, baseflags | tflags, line_h, ident, node );
            baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
        }
    }
    else
    {
        crFatalError();
    }
}

int CssPageBreak2Flags( css_page_break_t prop )
{
    switch (prop)
    {
    case css_pb_always:
    case css_pb_left:
    case css_pb_right:
        return RN_SPLIT_ALWAYS;
    case css_pb_avoid:
        return RN_SPLIT_AVOID;
    case css_pb_auto:
        return RN_SPLIT_AUTO;
    default:
        return RN_SPLIT_AUTO;
    }
}

int renderBlockElement( LVRendPageContext & context, ldomNode * node, int x, int y, int width )
{
    if ( node->getNodeType()==LXML_ELEMENT_NODE )
    {
        bool isFootNoteBody = false;
        if ( node->getNodeId()==el_section && node->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) {
            ldomElement * body = node->getParentNode();
            while ( body != NULL && body->getNodeId()!=el_body )
                body = body->getParentNode();
            if ( body ) {
                if ( body->getAttributeValue(attr_name)==L"notes" )
                    if ( !node->getAttributeValue(attr_id).empty() )
                        isFootNoteBody = true;
            }
        }
        ldomElement * enode = (ldomElement *) node; 
        lvdomElementFormatRec * fmt = node->getRenderData();
        if (!fmt)
            crFatalError();
        if (node->getNodeId() == el_empty_line)
            x = x;
        int em = enode->getFont()->getHeight();
        int margin_left = lengthToPx( enode->getStyle()->margin[0], width, em ) + DEBUG_TREE_DRAW;
        int margin_right = lengthToPx( enode->getStyle()->margin[1], width, em ) + DEBUG_TREE_DRAW;
        int margin_top = lengthToPx( enode->getStyle()->margin[2], width, em ) + DEBUG_TREE_DRAW;
        int margin_bottom = lengthToPx( enode->getStyle()->margin[3], width, em ) + DEBUG_TREE_DRAW;
        
        //margin_left += 50;
        //margin_right += 50;
        
        if (margin_left>0)
            x += margin_left;
        y += margin_top;
        
        width -= margin_left + margin_right;
        fmt->setX( x );
        fmt->setY( y );
        fmt->setWidth( width );
        fmt->setHeight( 0 );


        switch( enode->getRendMethod() )
        {
        case erm_block:
            {
                if ( isFootNoteBody )
                    context.enterFootNote( node->getAttributeValue(attr_id) );
                // recurse all sub-blocks for blocks
                int y = 0;
                int cnt = node->getChildCount();
                for (int i=0; i<cnt; i++)
                {
                    ldomNode * child = node->getChildNode( i );
                    int h = renderBlockElement( context, child, 0, y, width );
                    y += h;
                }
                int st_y = lengthToPx( enode->getStyle()->height, em, em );
                if ( y < st_y )
                    y = st_y;
                fmt->setHeight( y ); //+ margin_top + margin_bottom ); //???
                if ( isFootNoteBody )
                    context.leaveFootNote();
                return y + margin_top + margin_bottom; // return block height
            }
            break;
        case erm_final:
            {
                if ( isFootNoteBody )
                    context.enterFootNote( node->getAttributeValue(attr_id) );
                // render whole node content as single formatted object
                LFormattedTextRef txform;
                int h = enode->renderFinalBlock( txform, width );
#ifdef DEBUG_DUMP_ENABLED
                logfile << "\n";
#endif
                //int flags = styleToTextFmtFlags( fmt->getStyle(), 0 );
                //renderFinalBlock( node, &txform, fmt, flags, 0, 16 );
                //int h = txform.Format( width, context.getPageHeight() );
                fmt->setHeight( h );
                lvRect rect;
                node->getAbsRect(rect);
                // split pages
                int break_before = CssPageBreak2Flags( enode->getStyle()->page_break_before );
                int break_after = CssPageBreak2Flags( enode->getStyle()->page_break_after );
                int break_inside = CssPageBreak2Flags( enode->getStyle()->page_break_inside );
                int count = txform->GetLineCount();
                for (int i=0; i<count; i++)
                {
                    const formatted_line_t * line = txform->GetLineInfo(i);
                    int line_flags = 0; //TODO
                    if (i==0)
                        line_flags |= break_before << RN_SPLIT_BEFORE;
                    else
                        line_flags |= break_inside << RN_SPLIT_BEFORE;
                    if (i==count-1)
                        line_flags |= break_after << RN_SPLIT_AFTER;
                    else
                        line_flags |= break_inside << RN_SPLIT_AFTER;

                    context.AddLine(rect.top+line->y, rect.top+line->y+line->height, line_flags);

                    // footnote links analysis
                    if ( !isFootNoteBody && node->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) { // disable footnotes for footnotes
                        for ( unsigned w=0; w<line->word_count; w++ ) {
                            // check link start flag for every word
                            if ( line->words[w].flags & LTEXT_WORD_IS_LINK_START ) {
                                const src_text_fragment_t * src = txform->GetSrcInfo( line->words[w].src_text_index );
                                if ( src && src->object ) {
                                    ldomNode * node = (ldomNode*)src->object;
                                    ldomElement * parent = node->getParentNode();
                                    if ( parent->getNodeId()==el_a && parent->hasAttribute(LXML_NS_ANY, attr_href )
                                            && parent->getAttributeValue(LXML_NS_ANY, attr_type )==L"note") {
                                        lString16 href = parent->getAttributeValue(LXML_NS_ANY, attr_href );
                                        if ( href.length()>0 && href.at(0)=='#' ) {
                                            href.erase(0,1);
                                            context.addLink( href );
                                        }

                                    }
                                }
                            }
                        }
                    }
                }
                if ( isFootNoteBody )
                    context.leaveFootNote();
                return h + margin_top + margin_bottom;
            }
            break;
        case erm_invisible:
            // don't render invisible blocks
            return 0;
        default:
            crFatalError(); // error
        }
    }
    else
    {
        crFatalError();
    }
    return 0;
}

void DrawDocument( LVDrawBuf & drawbuf, ldomNode * node, int x0, int y0, int dx, int dy, int doc_x, int doc_y, int page_height, ldomMarkedRangeList * marks )
{
    if ( node->getNodeType()==LXML_ELEMENT_NODE )
    {
        ldomElement * enode = (ldomElement *)node; 
        lvdomElementFormatRec * fmt = node->getRenderData();
        if (!fmt)
            crFatalError();
        doc_x += fmt->getX();
        doc_y += fmt->getY();
        if ( doc_y + fmt->getHeight() <= 0 || doc_y > 0 + dy ) //0~=y0
        {
            return; // out of range
        }
        css_length_t bg = enode->getStyle()->background_color;
        lUInt32 oldColor = 0;
        if ( bg.type==css_val_color ) {
            oldColor = drawbuf.GetBackgroundColor();
            drawbuf.SetBackgroundColor( bg.value );
            drawbuf.FillRect( x0 + doc_x, y0 + doc_y, x0 + doc_x+fmt->getWidth(), y0+doc_y+fmt->getHeight(), bg.value );
        }
#if (DEBUG_TREE_DRAW!=0)
        lUInt32 color;
        if (drawbuf.GetBitsPerPixel()>=16)
            color = (node->getNodeLevel() & 1) ? 0x808080 : 0xC0C0C0;
        else
            color = (node->getNodeLevel() & 1) ? 1 : 2;
#endif
        switch( enode->getRendMethod() )
        {
        case erm_block:
            {
#if (DEBUG_TREE_DRAW!=0)
                drawbuf.FillRect( doc_x, doc_y, doc_x+fmt->getWidth(), doc_y+1, color );
                drawbuf.FillRect( doc_x, doc_y, doc_x+1, doc_y+fmt->getHeight(), color );
                drawbuf.FillRect( doc_x+fmt->getWidth()-1, doc_y, doc_x+fmt->getWidth(), doc_y+fmt->getHeight(), color );
                drawbuf.FillRect( doc_x, doc_y+fmt->getHeight()-1, doc_x+fmt->getWidth(), doc_y+fmt->getHeight(), color );
#endif
                // recursive draw all sub-blocks for blocks
                int cnt = node->getChildCount();
                for (int i=0; i<cnt; i++)
                {
                    ldomNode * child = node->getChildNode( i );
                    DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks ); //+fmt->getX() +fmt->getY()
                }
            }
            break;
        case erm_final:
            {
#if (DEBUG_TREE_DRAW!=0)
                drawbuf.FillRect( doc_x, doc_y, doc_x+fmt->getWidth(), doc_y+1, color );
                drawbuf.FillRect( doc_x, doc_y, doc_x+1, doc_y+fmt->getHeight(), color );
                drawbuf.FillRect( doc_x+fmt->getWidth()-1, doc_y, doc_x+fmt->getWidth(), doc_y+fmt->getHeight(), color );
                drawbuf.FillRect( doc_x, doc_y+fmt->getHeight()-1, doc_x+fmt->getWidth(), doc_y+fmt->getHeight(), color );
#endif
                // draw whole node content as single formatted object
                LFormattedTextRef txform;
                enode->renderFinalBlock( txform, fmt->getWidth() );

                {
                    if ( marks && marks->length() ) {
                        lvRect rc;
                        enode->getAbsRect( rc );
                        //rc.left -= doc_x;
                        //rc.right -= doc_x;
                        //rc.top -= doc_y;
                        //rc.bottom -= doc_y;
                        ldomMarkedRangeList nmarks( marks, rc );
                        txform->Draw( &drawbuf, doc_x+x0, doc_y+y0, &nmarks );

                    } else {
                        txform->Draw( &drawbuf, doc_x+x0, doc_y+y0, marks );
                    }
                }
            }
            break;
        case erm_invisible:
            // don't draw invisible blocks
            break;
        default:
            crFatalError(); // error
        }
        if ( bg.type==css_val_color ) {
            drawbuf.SetBackgroundColor( oldColor );
        }
    }
}

void convertLengthToPx( css_length_t & val, int base_px, int base_em )
{
    switch( val.type )
    {
    case css_val_inherited:
        val = css_length_t ( base_px );
        break;
    case css_val_px:
        // nothing to do
        break;
    case css_val_ex: // not implemented: treat as em 
    case css_val_em: // value = em*256
        val = css_length_t ( (base_em * val.value) >> 8 );
        break;
    case css_val_percent:
        val = css_length_t ( (base_px * val.value) / 100 );
        break;
    case css_val_unspecified:
    case css_val_in: // 2.54 cm
    case css_val_cm:
    case css_val_mm:
    case css_val_pt: // 1/72 in
    case css_val_pc: // 12 pt
    case css_val_color:
        // not supported: use inherited value
        val = css_length_t ( val.value );
        break;
    }
}

inline void spreadParent( css_length_t & val, css_length_t & parent_val )
{
    if ( val.type == css_val_inherited )
        val = parent_val;
}

void setNodeStyle( ldomNode * node, css_style_ref_t parent_style, LVFontRef parent_font )
{
    ldomElement * enode = (ldomElement *) node;
    //lvdomElementFormatRec * fmt = node->getRenderData();
    css_style_ref_t style( new css_style_rec_t );
    css_style_rec_t * pstyle = style.get();

    // init default style attribute values
    const elem_def_t * type_ptr = node->getElementTypePtr();
    if (type_ptr)
    {
        pstyle->display = type_ptr->props.display;
        pstyle->white_space = type_ptr->props.white_space;
    }

    //////////////////////////////////////////////////////
    // apply style sheet
    //////////////////////////////////////////////////////
    node->getDocument()->getStyleSheet()->apply( node, pstyle );

    if ( enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) && enode->hasAttribute( LXML_NS_ANY, attr_style ) ) {
        lString16 nodeStyle = enode->getAttributeValue( LXML_NS_ANY, attr_style );
        if ( !nodeStyle.empty() ) {
            nodeStyle = lString16(L"{") + nodeStyle + L"}";
            LVCssDeclaration decl;
            lString8 s8 = UnicodeToUtf8(nodeStyle);
            const char * s = s8.c_str();
            if ( decl.parse( s ) ) {
                decl.apply( pstyle );
            }
        }
    }

    // update inherited style attributes
    #define UPDATE_STYLE_FIELD(fld,inherit_value) \
        if (pstyle->fld == inherit_value) \
            pstyle->fld = parent_style->fld
    #define UPDATE_LEN_FIELD(fld) \
        switch( pstyle->fld.type ) \
        { \
        case css_val_inherited: \
            pstyle->fld = parent_style->fld; \
            break; \
        case css_val_percent: \
            pstyle->fld.type = parent_style->fld.type; \
            pstyle->fld.value = parent_style->fld.value * pstyle->fld.value / 100; \
            break; \
        case css_val_em: \
            pstyle->fld.type = css_val_px; \
            pstyle->fld.value = parent_style->font_size.value * pstyle->fld.value; \
            break; \
        default: \
            pstyle->fld.type = css_val_px; \
            pstyle->fld.value = 0; \
            break; \
        }

    //if ( (pstyle->display == css_d_inline) && (pstyle->text_align==css_ta_inherit))
    //{
        //if (parent_style->text_align==css_ta_inherit)
        //parent_style->text_align = css_ta_center;
    //}

    UPDATE_STYLE_FIELD( display, css_d_inherit );
    UPDATE_STYLE_FIELD( white_space, css_ws_inherit );
    UPDATE_STYLE_FIELD( text_align, css_ta_inherit );
    UPDATE_STYLE_FIELD( text_decoration, css_td_inherit );

    UPDATE_STYLE_FIELD( page_break_before, css_pb_inherit );
    UPDATE_STYLE_FIELD( page_break_after, css_pb_inherit );
    UPDATE_STYLE_FIELD( page_break_inside, css_pb_inherit );
    UPDATE_STYLE_FIELD( vertical_align, css_va_inherit );
    UPDATE_STYLE_FIELD( font_style, css_fs_inherit );
    UPDATE_STYLE_FIELD( font_weight, css_fw_inherit );
    UPDATE_STYLE_FIELD( font_family, css_ff_inherit );
    UPDATE_STYLE_FIELD( font_name, "" );
    UPDATE_LEN_FIELD( font_size );
    //UPDATE_LEN_FIELD( text_indent );
    spreadParent( pstyle->text_indent, parent_style->text_indent );
    switch( pstyle->font_weight )
    {
    case css_fw_inherit:
        pstyle->font_weight = parent_style->font_weight;
        break;
    case css_fw_normal:
        pstyle->font_weight = css_fw_400;
        break;
    case css_fw_bold:
        pstyle->font_weight = css_fw_600;
        break;
    case css_fw_bolder:
        pstyle->font_weight = parent_style->font_weight;
        if (pstyle->font_weight < css_fw_800)
        {
            pstyle->font_weight = (css_font_weight_t)((int)pstyle->font_weight + 2);
        }
        break;
    case css_fw_lighter:
        pstyle->font_weight = parent_style->font_weight;
        if (pstyle->font_weight > css_fw_200)
        {
            pstyle->font_weight = (css_font_weight_t)((int)pstyle->font_weight - 2);
        }
        break;
    case css_fw_100:
    case css_fw_200:
    case css_fw_300:
    case css_fw_400:
    case css_fw_500:
    case css_fw_600:
    case css_fw_700:
    case css_fw_800:
    case css_fw_900:
        break;
    }
    switch( pstyle->font_size.type )
    {
    case css_val_inherited:
        pstyle->font_size = parent_style->font_size;
        break;
    case css_val_px:
        // nothing to do
        break;
    case css_val_ex: // not implemented: treat as em 
    case css_val_em: // value = em*256
        pstyle->font_size.type = css_val_px;
        pstyle->font_size.value = parent_style->font_size.value * pstyle->font_size.value / 256;
        break;
    case css_val_percent:
        pstyle->font_size.type = css_val_px;
        pstyle->font_size.value = parent_style->font_size.value * pstyle->font_size.value / 100;
        break;
    case css_val_unspecified:
    case css_val_in: // 2.54 cm
    case css_val_cm:
    case css_val_mm:
    case css_val_pt: // 1/72 in
    case css_val_pc: // 12 pt
    case css_val_color: // 12 pt
        // not supported: use inherited value
        pstyle->font_size = parent_style->font_size;
        break;
    }
    // line_height
    spreadParent( pstyle->line_height, parent_style->line_height );
    spreadParent( pstyle->color, parent_style->color );
    spreadParent( pstyle->background_color, parent_style->background_color );

    // set calculated style
    //node->getDocument()->cacheStyle( style );
    enode->setStyle( style );

    // set font
    if ( isSameFontStyle( parent_style.get(), style.get() ) )
    {
        enode->setFont( parent_font );
    }
    else
    {
        enode->setFont( getFont(style.get()) );
    }
}

