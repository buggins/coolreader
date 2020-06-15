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
#include "../include/lvtextfm.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvrend.h"
#include "../include/crlog.h"

// Note about box model/sizing in crengine:
// https://quirksmode.org/css/user-interface/boxsizing.html says:
//   - In the W3C box model, the width of an element gives the width of
//     the content of the box, excluding padding and border.
//   - In the traditional box model, the width of an element gives the
//     width between the borders of the box, including padding and border.
//   By default, all browsers use the W3C box model, with the exception
//   of IE in "Quirks Mode" (IE5.5 Mode), which uses the traditional one.
//
// These models are toggable with CSS in current browsers:
//   - the first one is used when "box-sizing: content-box" (default in browsers)
//   - the second one is used when "box-sizing: border-box".
//
// crengine legacy rendering uses the traditional one (box-sizing: border-box).
// In enhanced rendering mode, the W3C box model can be enabled by
// setting BLOCK_RENDERING_USE_W3C_BOX_MODEL in flags.
//
// Note: internally in the code, RenderRectAccessor stores the position
// and width of the border box (and when in enhanced rendering mode, in
// its _inner* fields, those of the content box).
//
// See: https://www.456bereastreet.com/archive/201112/the_difference_between_widthauto_and_width100/
// for some example differences in rendering.
//
// As a side note, TABLE {width: auto} has a different behaviour that
// what is described there: with width:auto, a table adjusts its width to
// the table content, and does not take its full container width when
// the content does not need it.
// width: auto is the default for TABLEs in current browsers.
// crengine default used to be "width: 100%", but now that we
// can shrink to fit, it is "width: auto".

int gInterlineScaleFactor = INTERLINE_SCALE_FACTOR_NO_SCALE;

int gRenderDPI = DEF_RENDER_DPI; // if 0: old crengine behaviour: 1px/pt=1px, 1in/cm/pc...=0px
bool gRenderScaleFontWithDPI = DEF_RENDER_SCALE_FONT_WITH_DPI;
int gRootFontSize = 24; // will be reset as soon as font size is set

int scaleForRenderDPI( int value ) {
    // if gRenderDPI == 0 or 96, use value as is (1px = 1px)
    if (gRenderDPI && gRenderDPI != BASE_CSS_DPI) {
        value = value * gRenderDPI / BASE_CSS_DPI;
    }
    return value;
}

int gRenderBlockRenderingFlags = BLOCK_RENDERING_FLAGS_DEFAULT;

int validateBlockRenderingFlags(int f) {
    // Check coherency and ensure dependancies of flags
    if (f & ~BLOCK_RENDERING_ENHANCED) // If any other flag is set,
        f |= BLOCK_RENDERING_ENHANCED; // set ENHANGED
    if (f & BLOCK_RENDERING_FLOAT_FLOATBOXES)
        f |= BLOCK_RENDERING_PREPARE_FLOATBOXES;
    if (f & BLOCK_RENDERING_PREPARE_FLOATBOXES)
        f |= BLOCK_RENDERING_WRAP_FLOATS;
    return f;
}

// Uncomment for debugging enhanced block rendering
// #define DEBUG_BLOCK_RENDERING

//#define DEBUG_TREE_DRAW 3
// define to non-zero (1..5) to see block bounds
#define DEBUG_TREE_DRAW 0

//#ifdef _DEBUG
//#define DEBUG_DUMP_ENABLED
//#endif

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

simpleLogFile logfile("/tmp/logfile.log");

#else

// stubs
class simpleLogFile
{
public:
    simpleLogFile & operator << ( const char * ) { return *this; }
    simpleLogFile & operator << ( int ) { return *this; }
    simpleLogFile & operator << ( const wchar_t * ) { return *this; }
    simpleLogFile & operator << ( const lString16 & ) { return *this; }
};

simpleLogFile logfile;

#endif

// prototypes
void copystyle( css_style_ref_t sourcestyle, css_style_ref_t deststyle );
css_page_break_t getPageBreakBefore( ldomNode * el );
int CssPageBreak2Flags( css_page_break_t prop );

///////////////////////////////////////////////////////////////////////////////
//
// TABLE RENDERING CLASSES
//
///////////////////////////////////////////////////////////////////////////////

// Uncomment for debugging table rendering:
// #define DEBUG_TABLE_RENDERING

#define TABLE_BORDER_WIDTH 1

class CCRTableCol;
class CCRTableRow;

class CCRTableCell {
public:
    CCRTableCol * col;
    CCRTableRow * row;
    int col_index; // copy of col->index, only filled and used if RTL table for re-ordering cells
    int direction;
    int width;
    int height;
    int baseline;
    int percent;
    int max_content_width;
    int min_content_width;
    short colspan;
    short rowspan;
    char halign;
    char valign;
    ldomNode * elem;
    CCRTableCell() : col(NULL), row(NULL)
    , direction(REND_DIRECTION_UNSET)
    , width(0)
    , height(0)
    , baseline(0)
    , percent(0)
    , max_content_width(0)
    , min_content_width(0)
    , colspan(1)
    , rowspan(1)
    , halign(0) // default to text-align: left
    , valign(0) // default to vertical-align: baseline
    , elem(NULL)
    { }
};

class CCRTableRowGroup;

class CCRTableRow {
public:
    int index;
    int height;
    int baseline;
    int bottom_overflow; // extra height from row with rowspan>1
    int y;
    int numcols; // sum of colspan
    int linkindex;
    lString16Collection links;
    ldomNode * elem;
    LVPtrVector<CCRTableCell> cells;
    CCRTableRowGroup * rowgroup;
    LVRendPageContext * single_col_context; // we can add cells' lines instead of full rows
    CCRTableRow() : index(0)
    , height(0)
    , baseline(0)
    , bottom_overflow(0)
    , y(0)
    , numcols(0) // sum of colspan
    , linkindex(-1)
    , elem(NULL)
    , rowgroup(NULL)
    , single_col_context(NULL)
    { }
};

class CCRTableRowGroup {
public:
    int index;
    int height;
    int y;
    ldomNode * elem;
    LVPtrVector<CCRTableRow, false> rows;
    CCRTableRowGroup() : index(0)
    , height(0)
    , y(0)
    , elem(NULL)
    { }
};

class CCRTableCol {
public:
    int index;
    int width;
    int percent;
    int max_width;
    int sum_max_content_width;
    int nb_sum_max_content_width;
    int min_width;
    int nrows;
    int x;      // sum of previous col widths
    bool width_auto; // true when no width or percent is specified
    // LVPtrVector<CCRTableCell, false> cells; // not used
    ldomNode * elem;
    CCRTableCol() :
    index(0)
    , width(0)
    , percent(0)
    , max_width(0)
    , sum_max_content_width(0)
    , nb_sum_max_content_width(0)
    , min_width(0)
    , nrows(0)
    , x(0) // sum of previous col widths
    , width_auto(true)
    , elem( NULL )
    { }
    ~CCRTableCol() { }
};

/*
    in: string      25   35%
    out:            25   -35
*/
int StrToIntPercent( const wchar_t * s, int digitwidth=0 );
int StrToIntPercent( const wchar_t * s, int digitwidth )
{
    int n=0;
    if (!s || !s[0]) return 0;
    for (int i=0; s[i]; i++) {
        if (s[i]>='0' && s[i]<='9') {
            //=================
            n=n*10+(s[i]-'0');
        } else if (s[i]=='d') {
            //=================
            n=n*digitwidth;
            break;
        } else if (s[i]=='%') {
            //=================
            n=-n;
            break;
        }
    }
    return n;
}

// Utility function used in CCRTable::PlaceCells() when border_collapse.
// border_id is the border index: 0=top, 1=right, 2=bottom, 3=left.
// Update provided target_style and current_target_size.
// We don't implement fully the rules describe in:
//   https://www.w3.org/TR/CSS21/tables.html#border-conflict-resolution
//
// With use_neighbour_if_equal=true, neighbour border wins if its width
// is equal to target width.
// We would have intuitively set it to true, for a TABLE or TR border
// color to be used even if border width is the same as the TD one. But
// the above url rules state: "If border styles differ only in color,
// then a style set on a cell wins over one on a row, which wins over
// a row group, column, column group and, lastly, table."
void collapse_border(css_style_ref_t & target_style, int & current_target_size,
    int border_id, ldomNode * neighbour_node, bool use_neighbour_if_equal=false) {
    if (neighbour_node) {
        int neighbour_size = measureBorder(neighbour_node, border_id);
        if ( neighbour_size > current_target_size ||
                (use_neighbour_if_equal && neighbour_size == current_target_size) ) {
            css_style_ref_t neighbour_style = neighbour_node->getStyle();
            switch (border_id) {
                case 0: target_style->border_style_top = neighbour_style->border_style_top; break;
                case 1: target_style->border_style_right = neighbour_style->border_style_right; break;
                case 2: target_style->border_style_bottom = neighbour_style->border_style_bottom; break;
                case 3: target_style->border_style_left = neighbour_style->border_style_left; break;
            }
            target_style->border_width[border_id] = neighbour_style->border_width[border_id];
            target_style->border_color[border_id] = neighbour_style->border_color[border_id];
            current_target_size = neighbour_size;
        }
    }
}

class CCRTable {
public:
    int table_width;
    int digitwidth;
    int direction;
    bool is_rtl;
    bool shrink_to_fit;
    bool avoid_pb_inside;
    bool enhanced_rendering;
    ldomNode * elem;
    ldomNode * caption;
    int caption_h;
    int caption_direction;
    LVPtrVector<CCRTableRow> rows;
    LVPtrVector<CCRTableCol> cols;
    LVPtrVector<CCRTableRowGroup> rowgroups;
    // LVMatrix<CCRTableCell*> cells; // not used (it was filled, but never read)
    CCRTableRowGroup * currentRowGroup;

    void ExtendCols( int ncols ) {
        while (cols.length()<ncols) {
            CCRTableCol * col = new CCRTableCol;
            col->index = cols.length();
            cols.add(col);
        }
    }

    int LookupElem( ldomNode * el, int elem_direction, int state ) {
        if (!el->getChildCount())
            return 0;
        int colindex = 0;
        for (int i=0; i<el->getChildCount(); i++) {
            ldomNode * item = el->getChildElementNode(i);
            if ( item ) {
                // for each child element
                css_style_rec_t * style = item->getStyle().get();

                int item_direction = elem_direction;
                if ( item->hasAttribute( attr_dir ) ) {
                    lString16 dir = item->getAttributeValue( attr_dir );
                    dir = dir.lowercase();
                    if ( dir.compare("rtl") == 0 ) {
                        item_direction = REND_DIRECTION_RTL;
                    }
                    else if ( dir.compare("ltr") == 0 ) {
                        item_direction = REND_DIRECTION_LTR;
                    }
                    else if ( dir.compare("auto") == 0 ) {
                        item_direction = REND_DIRECTION_UNSET;
                    }
                }
                if ( style->direction != css_dir_inherit ) {
                    if ( style->direction == css_dir_rtl )
                        item_direction = REND_DIRECTION_RTL;
                    else if ( style->direction == css_dir_ltr )
                        item_direction = REND_DIRECTION_LTR;
                    else if ( style->direction == css_dir_unset )
                        item_direction = REND_DIRECTION_UNSET;
                }

                lvdom_element_render_method rendMethod = item->getRendMethod();
                //CRLog::trace("LookupElem[%d] (%s, %d) %d", i, LCSTR(item->getNodeName()), state, (int)item->getRendMethod() );
                switch ( rendMethod ) {
                case erm_invisible:  // invisible: don't render
                    // do nothing: invisible
                    break;
                case erm_killed:     // no room to render element, or unproper table element
                    {
                        // We won't visit this element in PlaceCells() and renderCells(),
                        // but we'll visit it in DrawDocument() as we walk the DOM tree.
                        // Give it some width and height so we can draw a symbol so users
                        // know there is some content missing.
                        RenderRectAccessor fmt = RenderRectAccessor( item );
                        fmt.setHeight( 15 ); // not squared, so it does not look
                        fmt.setWidth( 10 );  // like a list square bullet
                        fmt.setX( 0 );       // positionned at top left of its container
                        fmt.setY( 0 );       // (which ought to be a proper table element)
                    }
                    break;
                case erm_table:      // table element: render as table
                    // do nothing: impossible
                    break;
                case erm_table_row_group: // table row group
                case erm_table_header_group: // table header group
                case erm_table_footer_group: // table footer group
                    if ( state==0 && currentRowGroup==NULL ) {
                        currentRowGroup = new CCRTableRowGroup();
                        currentRowGroup->elem = item;
                        currentRowGroup->index = rowgroups.length();
                        rowgroups.add( currentRowGroup );
                        LookupElem( item, item_direction, 0 );
                        currentRowGroup = NULL;
                    } else {
                    }
                    break;
                case erm_table_column_group: // table column group
                    // just fall into groups
                    LookupElem( item, item_direction, 0 );
                    break;
                case erm_table_row: // table row
                    {
                        // rows of table
                        CCRTableRow * row = new CCRTableRow;
                        row->elem = item;
                        if ( currentRowGroup ) {
                            // add row to group
                            row->rowgroup = currentRowGroup;
                            currentRowGroup->rows.add( row );
                        }
                        rows.add( row );
                        // What could <tr link="number"> have been in the past ?
                        // It's not mentionned in any HTML or FB2 spec,
                        // and row->linkindex is never used.
                        if (row->elem->hasAttribute(LXML_NS_ANY, attr_link)) {
                            lString16 lnk=row->elem->getAttributeValue(attr_link);
                            row->linkindex = lnk.atoi();
                        }
                        // recursion: search for inner elements
                        //int res =
                        LookupElem( item, item_direction, 1 ); // lookup row
                    }
                    break;
                case erm_table_column: // table column
                    {
                        // cols width definitions
                        ExtendCols(colindex+1);
                        CCRTableCol * col = cols[colindex];
                        col->elem = item;
                        /*
                        lString16 w = item->getAttributeValue(attr_width);
                        if (!w.empty()) {
                            // TODO: px, em, and other length types support
                            int wn = StrToIntPercent(w.c_str(), digitwidth);
                            if (wn<0)
                                col->percent = -wn;
                            else if (wn>0)
                                col->width = wn;
                        }
                        */
                        css_length_t w = style->width;
                        if ( w.type == css_val_percent ) { // %
                            col->percent = w.value / 256;
                        }
                        else if ( w.type != css_val_unspecified ) { // px, em...
                            int em = item->getFont()->getSize();
                            col->width = lengthToPx( w, 0, em );
                            // (0 as the base width for %, as % was dealt with just above)
                        }
                        // otherwise cell->percent and cell->width stay at 0
                        colindex++;
                    }
                    break;
                case erm_block:         // render as block element (as containing other elements)
                case erm_final:         // final element: render the whole of its content as single text block
                    if ( style->display == css_d_table_caption ) {
                        caption = item;
                        caption_direction = item_direction;
                    }
                    else { // <th> or <td> inside <tr>
                        // Table cells became either erm_block or erm_final depending on their content

                        if ( rows.length()==0 ) {
                            CCRTableRow * row = new CCRTableRow;
                            row->elem = item;
                            if ( currentRowGroup ) {
                                // add row to group
                                row->rowgroup = currentRowGroup;
                                currentRowGroup->rows.add( row );
                            }
                            rows.add( row );
                        }


                        CCRTableCell * cell = new CCRTableCell;
                        cell->elem = item;
                        int cs=StrToIntPercent(item->getAttributeValue(attr_colspan).c_str());
                        if (cs>0 && cs<100) { // colspan=0 (span all remaining columns) not supported
                            cell->colspan=cs;
                        } else {
                            cs=1;
                        }
                        int rs=StrToIntPercent(item->getAttributeValue(attr_rowspan).c_str());
                        if (rs>0 && rs<100) {
                            cell->rowspan=rs;
                        } else {
                            rs=1;
                        }
                        /*
                        // "width"
                        lString16 w = item->getAttributeValue(attr_width);
                        if (!w.empty()) {
                            int wn = StrToIntPercent(w.c_str(), digitwidth);
                            if (wn<0)
                                cell->percent = -wn;
                            else if (wn>0)
                                cell->width = wn;
                        }
                        // "align"
                        lString16 halign = item->getAttributeValue(attr_align);
                        if (halign == "center")
                            cell->halign = 1; // center
                        else if (halign == "right")
                            cell->halign = 2; // right
                        // "valign"
                        lString16 valign = item->getAttributeValue(attr_valign);
                        if (valign == "center")
                            cell->valign = 2; // middle
                        else if (valign == "bottom")
                            cell->valign = 3; // bottom
                        */
                        // These commented above attributes have been translated to
                        // CSS properties by ldomDocumentWriterFilter::OnAttribute():
                        //   width= has been translated to elem style->width
                        //   align= has been translated to elem style->text_align
                        //   valign= has been translated to elem style->vertical_align
                        // (This allows overriding them with Style tweaks to remove
                        // publisher alignments and specified widths)
                        // Note: this is only done with plain .html files, but
                        // not with EPUBs.
                        // todo: see if needed with EPUBs

                        css_length_t w = style->width;
                        if ( w.type == css_val_percent ) { // %
                            cell->percent = w.value / 256;
                        }
                        else if ( w.type != css_val_unspecified ) { // px, em...
                            int em = item->getFont()->getSize();
                            cell->width = lengthToPx( w, 0, em );
                        }
                        // else: cell->percent and cell->width stay at 0

                        // This is not used here, but style->text_align will
                        // be naturally handled when cells are rendered
                        css_text_align_t ta = style->text_align;
                        if ( ta == css_ta_center )
                            cell->halign = 1; // center
                        else if ( ta == css_ta_right )
                            cell->halign = 2; // right

                        // https://developer.mozilla.org/en-US/docs/Web/CSS/vertical-align
                        // The default for vertical_align is baseline (cell->valign=0,
                        // set at CCRTableCell init time), and all other named values
                        // than top/middle/bottom act as baseline.
                        css_length_t va = style->vertical_align;
                        if ( va.type == css_val_unspecified ) {
                            if ( va.value == css_va_top )
                                cell->valign = 1; // top
                            else if ( va.value == css_va_middle )
                                cell->valign = 2; // middle
                            else if ( va.value == css_va_bottom )
                                cell->valign = 3; // bottom
                        }

                        cell->direction = item_direction;
                        cell->row = rows[rows.length()-1];
                        cell->row->cells.add( cell );
                        cell->row->numcols += cell->colspan;
                        ExtendCols( cell->row->numcols ); // update col count
                    }
                    break;
                case erm_inline:
                case erm_runin:
                    // do nothing
                    break;
                }
            }
        }
        return 0;
    }

    // More or less complex algorithms to calculate column widths are described at:
    //   https://www.w3.org/TR/css-tables-3/#computing-cell-measures
    //   https://www.w3.org/TR/REC-html40/appendix/notes.html#h-B.5.2
    //   https://www.w3.org/TR/CSS2/tables.html#auto-table-layout
    //   https://drafts.csswg.org/css3-tables-algorithms/Overview.src.htm
    //   https://developer.mozilla.org/en-US/docs/Archive/Mozilla/Table_Layout_Strategy
    //   http://www.tads.org/t3doc/doc/htmltads/tables.htm
    //   https://github.com/Kozea/WeasyPrint (html to pdf in python)

    // Beware of risk of division by zero!
    // (In vim, find divisions with: /\v(\/)@<!(\*)@<!\/(\/)@!(\*)@!.*   )

    void PlaceCells() {
        int i, j;
        // search for max column number
        int maxcols = 0;
        for (i=0; i<rows.length(); i++) {
            if (maxcols<rows[i]->numcols)
                maxcols=rows[i]->numcols;
        }
        // add column objects
        ExtendCols(maxcols);
        // place row cells horizontally
        for (i=0; i<rows.length(); i++) {
            int x=0;
            int miny=-1;
            CCRTableRow * row = rows[i];
            row->index = i;
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                int cs = cell->colspan;
                //int rs = cell->rowspan;
                while (x<cols.length() && cols[x]->nrows>i) { // find free cell position
                    x++;
                    ExtendCols(x); // update col count
                }
                ExtendCols( x + cs ); // update col count
                cell->col = cols[x];
                for (int xx=0; xx<cs; xx++) {
                    // place cell
                    ExtendCols(x+xx+1); // update col count
                    if ( cols[x+xx]->nrows < i+cell->rowspan )
                        cols[x+xx]->nrows = i+cell->rowspan;
                    if (cell->rowspan>1) {
                        //int flg =1;
                    }
                }
                // update col width (for regular cells with colspan=1 only)
                if (cell->colspan==1) {
                    if (cell->width>0 && cell->col->width<cell->width && cell->col->percent==0) {
                        cell->col->width = cell->width;
                    } else if (cell->percent>0 && cell->col->width==0 && cell->col->percent<cell->percent) {
                        cell->col->percent = cell->percent;
                    }
                }
                x += cs;
            }
            // update min row count
            for (j=0; j<x; j++) {
                if (miny==-1 || miny>cols[j]->nrows)
                    miny=cols[j]->nrows;
            }
            // skip fully filled rows!
            while (miny>i+1) {
                i++;
                // add new row (already filled)
                CCRTableRow * nrow = new CCRTableRow;
                nrow->index = i;
                rows.insert(i, nrow);
            }
        }
        int maxy = 0; // check highest column
        for (j=0; j<cols.length(); j++)
            if (maxy<cols[j]->nrows)
                maxy=cols[j]->nrows;
        // padding table with empty lines up to max col height
        while (maxy>i) {
            i++;
            // add new row (already filled)
            CCRTableRow * nrow = new CCRTableRow;
            nrow->index = i;
            rows.insert(i, nrow);
        }
        // The above code has possibly added with ExtendCols() virtual columns
        // (that do not contain any cell) to 'cols' to account for colspan
        // values (which may be bogus in some documents).
        // Remove any extraneous cols and rows, and adjust the colspan and
        // rowspan values of involved cells.
        int max_used_x = -1;
        int max_used_y = -1;
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = (rows[i]->cells[j]);
                int x0 = cell->col->index;
                if (x0 > max_used_x)
                    max_used_x = x0;
                int y0 = cell->row->index;
                if (y0 > max_used_y)
                    max_used_y = y0;
            }
        }
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = (rows[i]->cells[j]);
                if (cell->col->index + cell->colspan - 1 > max_used_x)
                    cell->colspan = max_used_x - cell->col->index + 1;
                if (cell->row->index + cell->rowspan - 1 > max_used_y)
                    cell->rowspan = max_used_y - cell->row->index + 1;
                if (is_rtl) // set up col_index, used for RTL re-ordering of cells
                    cell->col_index = cell->col->index;
            }
        }
        #ifdef DEBUG_TABLE_RENDERING
            printf("TABLE: grid %dx%d reduced to %dx%d\n",
                cols.length(), rows.length(), max_used_x+1, max_used_y+1);
        #endif
        for (i=rows.length()-1; i>max_used_y; i--) {
            if (rows[i]->rowgroup) {
                rows[i]->rowgroup->rows.remove(rows[i]);
            }
            delete rows.remove(i);
        }
        for (i=cols.length()-1; i>max_used_x; i--) {
            delete cols.remove(i);
            // No need to adjust cols[x]->nrows, we don't use it from now
        }

        // If RTL table, we need to draw cells from right to left.
        // So, just swap their order and update pointers at every place relevant
        if ( is_rtl ) {
            // Reverse cols (CCRTableCol):
            cols.reverse();
            int nbcols = cols.length();
            for (i=0; i<nbcols; i++)
                cols[i]->index = i;
            // Reverse cells in each row
            for (i=0; i<rows.length(); i++) {
                rows[i]->cells.reverse();
                // and update each cell pointer to the right CCRTableCol
                for (j=0; j<rows[i]->cells.length(); j++) {
                    CCRTableCell * cell = rows[i]->cells[j];
                    cell->col = cols[nbcols-1 - cell->col_index - (cell->colspan-1)];
                    // cell->col_index has been set up above, and reflects
                    // the original index before we reversed cols.
                }
            }
        }

        css_style_ref_t table_style = elem->getStyle();
        int table_em = elem->getFont()->getSize();
        // border-spacing does not accept values in % unit
        int borderspacing_h = lengthToPx(table_style->border_spacing[0], 0, table_em);
        bool border_collapse = (table_style->border_collapse == css_border_collapse);

        if (border_collapse) {
            borderspacing_h = 0; // no border spacing when table collapse
            // Each cell is responsible for drawing its borders.
            for (i=0; i<rows.length(); i++) {
                for (j=0; j<rows[i]->cells.length(); j++) {
                    CCRTableCell * cell = (rows[i]->cells[j]);
                    css_style_ref_t style = cell->elem->getStyle();
                    // (Note: we should not modify styles directly, as the change
                    // in style cache will affect other nodes with the same style,
                    // and corrupt style cache Hash, invalidating cache reuse.)
                    css_style_ref_t newstyle(new css_style_rec_t);
                    copystyle(style, newstyle);

                    // We don't do adjacent-cells "border size comparisons and
                    // take the larger one", as it's not obvious to get here
                    // this cell's adjactent ones (possibly multiple on a side
                    // when rowspan/colspan).
                    // But we should at least, for cells with no border, get the
                    // top and bottom, and the left and right for cells at
                    // table edges, from the TR, THEAD/TBODY or TABLE.
                    bool is_at_top = cell->row->index == 0;
                    bool is_at_bottom = (cell->row->index + cell->rowspan) == rows.length();
                    bool is_at_left = cell->col->index == 0;
                    bool is_at_right = (cell->col->index + cell->colspan) == cols.length();
                    // We'll avoid calling measureBorder() many times for this same cell,
                    // by passing these by reference to collapse_border():
                    int cell_border_top = measureBorder(cell->elem, 0);
                    int cell_border_right = measureBorder(cell->elem, 1);
                    int cell_border_bottom = measureBorder(cell->elem, 2);
                    int cell_border_left = measureBorder(cell->elem, 3);
                    //
                    // With border-collapse, a cell may get its top and bottom
                    // borders from its TR.
                    ldomNode * rtop = rows[cell->row->index]->elem;
                        // (may be NULL, but collapse_border() checks for that)
                    // For a cell with rowspan>1, not sure if its bottom border should come
                    // from its own starting row, or the other row it happens to end on.
                    // Should look cleaner if we use the later.
                    ldomNode * rbottom = rows[cell->row->index + cell->rowspan - 1]->elem;
                    collapse_border(newstyle, cell_border_top, 0, rtop);
                    collapse_border(newstyle, cell_border_bottom, 2, rbottom);
                    // We also get the left and right borders, for the first or
                    // the last cell in a row, from the row (top row if multi rows span)
                    if (is_at_left)
                        collapse_border(newstyle, cell_border_left, 3, rtop);
                    if (is_at_right)
                        collapse_border(newstyle, cell_border_right, 1, rtop);
                        // If a row is missing some cells, there is none that stick
                        // to the right of the table (is_at_right is false): the outer
                        // table border will have a hole for this row...
                    // We may also get them from the rowgroup this TR is part of, if any, for
                    // the cells in the first or the last row of this rowgroup
                    if (rows[cell->row->index]->rowgroup) {
                        CCRTableRowGroup * grp = rows[cell->row->index]->rowgroup;
                        if (rows[cell->row->index] == grp->rows.first())
                            collapse_border(newstyle, cell_border_top, 0, grp->elem);
                        if (rows[cell->row->index] == grp->rows.last())
                            collapse_border(newstyle, cell_border_bottom, 2, grp->elem);
                        if (is_at_left)
                            collapse_border(newstyle, cell_border_left, 3, grp->elem);
                        if (is_at_right)
                            collapse_border(newstyle, cell_border_right, 1, grp->elem);
                    }
                    // And we may finally get borders from the table itself ("elem")
                    if (is_at_top)
                        collapse_border(newstyle, cell_border_top, 0, elem);
                    if (is_at_bottom)
                        collapse_border(newstyle, cell_border_bottom, 2, elem);
                    if (is_at_left)
                        collapse_border(newstyle, cell_border_left, 3, elem);
                    if (is_at_right)
                        collapse_border(newstyle, cell_border_right, 1, elem);

                    // Now, we should disable some borders for this cell,
                    // at inter-cell boundaries.
                    // We could either keep the right and bottom borders
                    // (which would be better to catch coordinates bugs).
                    // Or keep the top and left borders, which is better:
                    // if a cell carries its top border, when splitting
                    // rows to pages, a row at top of page will have its
                    // top border (unlike previous alternative), which is
                    // clearer for the reader.
                    // So, we disable the bottom and right borders, except
                    // for cells that are on the right or at the bottom of
                    // the table, as these will draw the outer table border
                    // on these sides.
                    if ( !is_at_right )
                        newstyle->border_style_right = css_border_none;
                    if ( !is_at_bottom )
                        newstyle->border_style_bottom = css_border_none;

                    cell->elem->setStyle(newstyle);
                    // (Note: we should no more modify a style after it has been
                    // applied to a node with setStyle().)
                }
                // (Some optimisation could be made in these loops, as
                // collapse_border() currently calls measureBorder() many
                // times for the same elements: TR, TBODY, TABLE...)
            }
            // The TR and TFOOT borders will explicitely NOT be drawn by DrawDocument()
            // (Firefox never draws them, even when no border-collapse).
            // But the TABLE ones will be (needed when no border-collapse).
            // So, as we possibly collapsed the table border to the cells, we just
            // set them to none on the TABLE.
            css_style_ref_t style = elem->getStyle();
            css_style_ref_t newstyle(new css_style_rec_t);
            copystyle(style, newstyle);
            newstyle->border_style_top = css_border_none;
            newstyle->border_style_right = css_border_none;
            newstyle->border_style_bottom = css_border_none;
            newstyle->border_style_left = css_border_none;
            elem->setStyle(newstyle);
        }
        // We should no longer modify cells' padding and border beyond this point,
        // as these are used below to compute max_content_width and min_content_width,
        // which account for them.

        // Compute for each cell the max (prefered if possible) and min (width of the
        // longest word, so no word is cut) rendered widths.
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                // rows[i]->cells contains only real cells made from node elements
                CCRTableCell * cell = (rows[i]->cells[j]);
                getRenderedWidths(cell->elem, cell->max_content_width, cell->min_content_width, cell->direction);
                #ifdef DEBUG_TABLE_RENDERING
                    printf("TABLE: cell[%d,%d] getRenderedWidths: %d (min %d)\n",
                        j, i, cell->max_content_width, cell->min_content_width);
                #endif
                int x0 = cell->col->index;
                if (cell->colspan == 1) {
                    cols[x0]->sum_max_content_width += cell->max_content_width;
                    cols[x0]->nb_sum_max_content_width += 1;
                    // Update cols max/min widths only for colspan=1 cells
                    if ( cell->max_content_width > cols[x0]->max_width )
                        cols[x0]->max_width = cell->max_content_width;
                    if ( cell->min_content_width > cols[x0]->min_width )
                        cols[x0]->min_width = cell->min_content_width;
                }
            }
        }
        // Second pass for cells with colspan > 1
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = (rows[i]->cells[j]);
                if (cell->colspan > 1) {
                    // Check if we need to update the max_width and min_width
                    // of each cols we span
                    int nbspans = cell->colspan;
                    int x0 = cell->col->index;
                    // Get the existing aggregated min/max width of the cols we span
                    int cols_max_width = 0;
                    int cols_min_width = 0;
                    for (int x=0; x<nbspans; x++) {
                        cols_min_width += cols[x0+x]->min_width;
                        cols_max_width += cols[x0+x]->max_width;
                    }
                    cols_min_width += borderspacing_h * (nbspans-1);
                    cols_max_width += borderspacing_h * (nbspans-1);
                    #ifdef DEBUG_TABLE_RENDERING
                        printf("TABLE: COLS SPANNED[%d>%d] min_width=%d max_width=%d\n",
                            x0, x0+nbspans-1, cols_min_width, cols_max_width);
                    #endif
                    if ( cell->min_content_width > cols_min_width ) {
                        // Our min width is larger than the spanned cols min width
                        int to_distribute = cell->min_content_width - cols_min_width;
                        int distributed = 0;
                        for (int x=0; x<nbspans; x++) {
                            // Distribute more to bigger min_width to keep original
                            // cell proportions
                            int this_dist;
                            if (cols_min_width > 0)
                                this_dist = to_distribute * cols[x0+x]->min_width / cols_min_width;
                            else
                                this_dist = to_distribute / nbspans;
                            cols[x0+x]->min_width += this_dist;
                            distributed += this_dist;
                            #ifdef DEBUG_TABLE_RENDERING
                                printf("TABLE:   COL[%d] (todist:%d) min_wdith += %d => %d\n",
                                    x0+x, to_distribute, this_dist, cols[x0+x]->min_width);
                            #endif
                        }
                        // Distribute left over to last col
                        cols[x0+nbspans-1]->min_width += to_distribute - distributed;
                        #ifdef DEBUG_TABLE_RENDERING
                            printf("TABLE: COL[%d] (leftover:%d-%d) min_wdith += %d => %d\n",
                                x0+nbspans-1, to_distribute, distributed, to_distribute - distributed,
                                cols[x0+nbspans-1]->min_width);
                        #endif
                    }
                    // Let's do the same for max_width, although it may lead to
                    // messier layouts in complex colspan setups...
                    // (And we should probably not let all cols->max_width at 0 if they are 0!)
                    if ( cell->max_content_width > cols_max_width ) {
                        // Our max width is larger than the spanned cols max width
                        int to_distribute = cell->max_content_width - cols_max_width;
                        int distributed = 0;
                        for (int x=0; x<nbspans; x++) {
                            // Distribute more to bigger max_width to keep original
                            // cell proportions
                            int this_dist;
                            if (cols_max_width > 0)
                                this_dist = to_distribute * cols[x0+x]->max_width / cols_max_width;
                            else
                                this_dist = to_distribute / nbspans;
                            cols[x0+x]->max_width += this_dist;
                            distributed += this_dist;
                            #ifdef DEBUG_TABLE_RENDERING
                                printf("TABLE:   COL[%d] (todist:%d) max_wdith += %d => %d\n",
                                    x0+x, to_distribute, this_dist, cols[x0+x]->max_width);
                            #endif
                        }
                        // Distribute left over to last col
                        cols[x0+nbspans-1]->max_width += to_distribute - distributed;
                        #ifdef DEBUG_TABLE_RENDERING
                            printf("TABLE: COL[%d] (leftover:%d-%d) max_wdith += %d => %d\n",
                                x0+nbspans-1, to_distribute, distributed, to_distribute - distributed,
                                cols[x0+nbspans-1]->max_width);
                        #endif
                    }
                    // Also distribute to sum_max_content_width
                    for (int x=0; x<nbspans; x++) {
                        int this_dist;
                        if (cols_max_width > 0)
                            this_dist = cell->max_content_width * cols[x0+x]->max_width / cols_max_width;
                        else
                            this_dist = cell->max_content_width / nbspans;
                        cols[x0+x]->sum_max_content_width += this_dist;
                        cols[x0+x]->nb_sum_max_content_width += 1;
                    }
                }
            }
        }

        /////////////////////////// From here until further noticed, we just use and update the cols objects
        // Find width available for cells content (including their borders and paddings)
        // Start with table full width
        int assignable_width = table_width;
        // Remove table outer borders
        assignable_width -= measureBorder(elem,1) + measureBorder(elem,3); // (border indexes are TRBL)
        if ( border_collapse ) {
            // Table own outer paddings and any border-spacing are
            // ignored with border-collapse
        }
        else { // no collapse
            // Remove table outer paddings (margin and padding indexes are LRTB)
            assignable_width -= lengthToPx(table_style->padding[0], table_width, table_em);
            assignable_width -= lengthToPx(table_style->padding[1], table_width, table_em);
            // Remove (nb cols + 1) border-spacing
            assignable_width -= (cols.length() + 1) * borderspacing_h;
        }
        #ifdef DEBUG_TABLE_RENDERING
            printf("TABLE: table_width=%d assignable_width=%d\n", table_width, assignable_width);
        #endif
        if (assignable_width <= 0) { // safety check
            // In case we get a zero or negative value (too much padding or
            // borderspacing and many columns), just re-set it to table_width
            // for our calculation purpose, which expect a positive value to
            // work with: the rendering will be bad, but some stuff will show.
            assignable_width = table_width;
            borderspacing_h = 0;
        }

        // Find best width for each column
        int npercent=0;
        int sumpercent=0;
        int nwidth = 0;
        int sumwidth = 0;
        for (int x=0; x<cols.length(); x++) {
            #ifdef DEBUG_TABLE_RENDERING
                printf("TABLE WIDTHS step1: cols[%d]: %d%% %dpx\n",
                    x, cols[x]->percent, cols[x]->width);
            #endif
            if (cols[x]->percent>0) {
                cols[x]->width_auto = false;
                sumpercent += cols[x]->percent;
                cols[x]->width = 0;
                npercent++;
            } else if (cols[x]->width>0) {
                cols[x]->width_auto = false;
                sumwidth += cols[x]->width;
                nwidth++;
            }
        }
        int nrest = cols.length() - nwidth - npercent; // nb of cols with auto width
        int sumwidthpercent = 0; // percent of sum-width
        // Percents are to be used as a ratio of assignable_width (if we would
        // use the original full table width, 100% would overflow with borders,
        // paddings and border spacings)
        if (sumwidth) {
            sumwidthpercent = 100*sumwidth/assignable_width;
            if (sumpercent+sumwidthpercent+5*nrest>100) { // 5% (?) for each unsized column
                // too wide: convert widths to percents
                for (int i=0; i<cols.length(); i++) {
                    if (cols[i]->width>0) {
                        cols[i]->percent = cols[i]->width*100/assignable_width;
                        cols[i]->width = 0;
                        sumpercent += cols[i]->percent;
                        npercent++;
                    }
                }
                nwidth = 0;
                sumwidth = 0;
            }
        }
        // scale percents
        int maxpercent = 100-3*nrest; // 3% (?) for each unsized column
        if (sumpercent>maxpercent && sumpercent>0) {
            // scale percents
            int newsumpercent = 0;
            for (int i=0; i<cols.length(); i++) {
                if (cols[i]->percent>0) {
                    cols[i]->percent = cols[i]->percent*maxpercent/sumpercent;
                    newsumpercent += cols[i]->percent;
                    cols[i]->width = 0;
                }
            }
            sumpercent = newsumpercent;
        }
        // calc width by percents
        sumwidth = 0;
        int sum_auto_max_width = 0;
        int sum_auto_mean_max_content_width = 0;
        nwidth = 0;
        for (i=0; i<cols.length(); i++) {
            if (cols[i]->percent>0) {
                cols[i]->width = assignable_width * cols[i]->percent / 100;
                cols[i]->percent = 0;
            }
            if (cols[i]->width>0) {
                // calc width stats
                sumwidth += cols[i]->width;
                nwidth++;
            } else if (cols[i]->max_width>0) {
                sum_auto_max_width += cols[i]->max_width;
                sum_auto_mean_max_content_width += cols[i]->sum_max_content_width / cols[i]->nb_sum_max_content_width;
            }
        }
        #ifdef DEBUG_TABLE_RENDERING
            for (int x=0; x<cols.length(); x++)
                printf("TABLE WIDTHS step2: cols[%d]: %d%% %dpx\n",
                    x, cols[x]->percent, cols[x]->width);
        #endif
        // At this point, all columns with specified width or percent has been
        // set accordingly, or reduced to fit table width
        // We need to compute a width for columns with unspecified width.
        nrest = cols.length() - nwidth;
        int restwidth = assignable_width - sumwidth;
        int sumMinWidths = 0;
        // new pass: convert text len percent into width
        for (i=0; i<cols.length(); i++) {
            if (cols[i]->width==0) { // unspecified (or width scaled down and rounded to 0)
                // We have multiple options:
                // Either distribute remaining width according to max_width ratio
                // (so larger content gets more width to use less height)
                //     if (sum_auto_max_width > 0)
                //         cols[i]->width = cols[i]->max_width * restwidth / sum_auto_max_width;
                // Or better: distribute remaining width according to the mean
                // of cells' max_content_width, so a single large cell among
                // many smaller cells doesn't request all the width for this
                // column (which would give less to others, which could make
                // them use more height). This should help getting more
                // reasonable heights across all rows.
                if (sum_auto_mean_max_content_width > 0)
                    cols[i]->width = cols[i]->sum_max_content_width / cols[i]->nb_sum_max_content_width * restwidth / sum_auto_mean_max_content_width;
                // else stays at 0
                sumwidth += cols[i]->width;
                nwidth++;
            }
            if (cols[i]->width < cols[i]->min_width) {
                // extend too small cols to their min_width
                int delta = cols[i]->min_width - cols[i]->width;
                cols[i]->width += delta;
                sumwidth += delta;
            }
            sumMinWidths += cols[i]->min_width; // will be handy later
        }
        if (sumwidth>assignable_width && sumwidth>0) {
            // too wide! rescale down
            int newsumwidth = 0;
            for (i=0; i<cols.length(); i++) {
                cols[i]->width = cols[i]->width * assignable_width / sumwidth;
                newsumwidth += cols[i]->width;
            }
            sumwidth = newsumwidth;
        }
        #ifdef DEBUG_TABLE_RENDERING
            for (int x=0; x<cols.length(); x++)
                printf("TABLE WIDTHS step3: cols[%d]: %d%% %dpx (min:%d / max:%d)\n",
                    x, cols[x]->percent, cols[x]->width, cols[x]->min_width, cols[x]->max_width);
        #endif
        bool canFitMinWidths = (sumMinWidths > 0 && sumMinWidths < assignable_width);
        // new pass: resize columns with originally unspecified widths
        int rw=0;
        int dist_nb_cols = 0;
        for (int x=0; x<cols.length(); x++) {
            // Adjust it further down to the measured max_width
            int prefered_width = cols[x]->min_width;
            if (cols[x]->max_width > prefered_width) {
                prefered_width = cols[x]->max_width;
            }
            if (cols[x]->width_auto && cols[x]->width > prefered_width) {
                // Column can nicely fit in a smaller width
                rw += (cols[x]->width - prefered_width);
                cols[x]->width = prefered_width;
                // min_width is no longer needed: use it as a flag so we don't
                // redistribute width to this column
                cols[x]->min_width = -1;
            }
            else if (canFitMinWidths && cols[x]->width < cols[x]->min_width) {
                // Even for columns with originally specified width:
                // If all min_width can fit into available width, ensure
                // cell's min_width by taking back from to-be redistributed width
                rw -= (cols[x]->min_width - cols[x]->width);
                cols[x]->width = cols[x]->min_width;
                // min_width is no longer needed: use it as a flag so we don't
                // redistribute width to this column
                cols[x]->min_width = -1;
            }
            else if (cols[x]->width_auto && cols[x]->min_width > 0) {
                dist_nb_cols += 1;      // candidate to get more width
            }
        }
        #ifdef DEBUG_TABLE_RENDERING
            for (int x=0; x<cols.length(); x++)
                printf("TABLE WIDTHS step4: cols[%d]: %d%% %dpx (min %dpx)\n",
                    x, cols[x]->percent, cols[x]->width, cols[x]->min_width);
        #endif
        int restw = assignable_width - sumwidth + rw; // may be negative if we needed to
                                                      // increase to fulfill min_width
        if (shrink_to_fit && restw > 0) {
            // If we're asked to shrink width to fit cells content, don't
            // distribute restw to columns, but shrink table width
            // Table padding may be in %, and need to be corrected
            int correction = 0;
            correction += lengthToPx(table_style->padding[0], table_width, table_em);
            correction += lengthToPx(table_style->padding[0], table_width, table_em);
            table_width -= restw;
            correction -= lengthToPx(table_style->padding[0], table_width, table_em);
            correction -= lengthToPx(table_style->padding[0], table_width, table_em);
            table_width -= correction;
            assignable_width -= restw + correction; // (for debug printf() below)
            #ifdef DEBUG_TABLE_RENDERING
                printf("TABLE WIDTHS step5 (fit): reducing table_width %d -%d -%d > %d\n",
                    table_width+restw+correction, restw, correction, table_width);
            #endif
        }
        else {
            #ifdef DEBUG_TABLE_RENDERING
                printf("TABLE WIDTHS step5 (dist): %d to distribute to %d cols\n",
                    restw, dist_nb_cols);
            #endif
            // distribute rest of width between all cols that can benefit from more
            bool dist_all_non_empty_cols = false;
            if (dist_nb_cols == 0) {
                dist_all_non_empty_cols = true;
                // distribute to all non empty cols
                for (int x=0; x<cols.length(); x++) {
                    if (cols[x]->min_width != 0) {
                        dist_nb_cols += 1;
                    }
                }
                #ifdef DEBUG_TABLE_RENDERING
                    printf("TABLE WIDTHS step5: %d to distribute to all %d non empty cols\n",
                        restw, dist_nb_cols);
                #endif
            }
            if (restw != 0 && dist_nb_cols>0) {
                int a = restw / dist_nb_cols;
                int b = restw % dist_nb_cols;
                for (i=0; i<cols.length(); i++) {
                    if ( (!dist_all_non_empty_cols && cols[i]->min_width > 0)
                      || (dist_all_non_empty_cols && cols[i]->min_width != 0) ) {
                        cols[i]->width += a;
                        if (b>0) {
                            cols[i]->width ++;
                            b--;
                        }
                        else if (b<0) {
                            cols[i]->width --;
                            b++;
                        }
                    }
                }
                // (it would be better to distribute restw according
                // to each column max_width / min_width, so larger ones
                // get more of it)
            }
            #ifdef DEBUG_TABLE_RENDERING
                for (int x=0; x<cols.length(); x++)
                    printf("TABLE WIDTHS step5: cols[%d]: %d%% %dpx\n",
                        x, cols[x]->percent, cols[x]->width);
            #endif
        }
        #ifdef DEBUG_TABLE_RENDERING
            printf("TABLE WIDTHS SUM:");
            int colswidthsum = 0;
            for (int x=0; x<cols.length(); x++) {
                printf(" +%d", cols[x]->width);
                colswidthsum += cols[x]->width;
            }
            printf(" = %d", colswidthsum);
            if (assignable_width == colswidthsum)
                printf(" == assignable_width, GOOD\n");
            else
                printf(" != assignable_width %d, BAAAAAAADDD\n", assignable_width);
        #endif

        // update col x
        for (i=0; i<cols.length(); i++) {
            if (i == 0)
                cols[i]->x = borderspacing_h;
            else
                cols[i]->x = cols[i-1]->x + cols[i-1]->width + borderspacing_h;
            #ifdef DEBUG_TABLE_RENDERING
                printf("TABLE WIDTHS step6: cols[%d]->x = %d\n", i, cols[i]->x);
            #endif
        }
        /////////////////////////// Done with just using and updating the cols objects

        // Columns widths calculated ok!
        // Update width of each cell
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = (rows[i]->cells[j]);
                cell->width = 0;
                int x0 = cell->col->index;
                for (int x=0; x<cell->colspan; x++) {
                    cell->width += cols[x0+x]->width;
                }
                if ( cell->colspan > 1 ) {
                    // include skipped borderspacings in cell width
                    cell->width += borderspacing_h * (cell->colspan-1);
                }
                #ifdef DEBUG_TABLE_RENDERING
                    printf("TABLE: placeCell[%d,%d] width: %d\n", j, i, cell->width);
                #endif
            }
        }
    }

    int renderCells( LVRendPageContext & context )
    {
        // We should set, for each of the table children and sub-children,
        // its RenderRectAccessor fmt(node) x/y/w/h.
        // x/y of a cell are relative to its own parent node top left corner
        int em = elem->getFont()->getSize();
        css_style_ref_t table_style = elem->getStyle();
        int table_border_top = measureBorder(elem, 0);
        int table_border_right = measureBorder(elem, 1);
        int table_border_bottom = measureBorder(elem, 2);
        int table_border_left = measureBorder(elem, 3);
        int table_padding_left = lengthToPx(table_style->padding[0], table_width, em);
        int table_padding_right = lengthToPx(table_style->padding[1], table_width, em);
        int table_padding_top = lengthToPx(table_style->padding[2], table_width, em);
        int table_padding_bottom = lengthToPx(table_style->padding[3], table_width, em);
        int borderspacing_h = lengthToPx(table_style->border_spacing[0], 0, em); // does not accept %
        int borderspacing_v = lengthToPx(table_style->border_spacing[1], 0, em);
        bool border_collapse = (table_style->border_collapse==css_border_collapse);
        if (border_collapse) {
            table_padding_top = 0;
            table_padding_bottom = 0;
            table_padding_left = 0;
            table_padding_right = 0;
            borderspacing_v = 0;
            borderspacing_h = 0;
        }
        // We want to distribute border spacing on top and bottom of each row,
        // mainly for page splitting to carry half of it on each page.
        int borderspacing_v_top = borderspacing_v / 2;
        int borderspacing_v_bottom = borderspacing_v - borderspacing_v_top;
        // (Both will be 0 if border_collapse)

        int nb_rows = rows.length();

        // We will context.AddLine() for page splitting the elements
        // (caption, rows) as soon as we meet them and their y-positionnings
        // inside the tables are known and won't change.
        // (This would need that rowgroups be dealt with in this flow (and
        // not at the end) if we change the fact that we ignore their
        // border/padding/margin - see below why we do.)
        lvRect rect;
        elem->getAbsRect(rect);
        const int table_y0 = rect.top; // absolute y in document for top of table
        int last_y = table_y0; // used as y0 to AddLine(y0, table_y0+table_h)
        int line_flags = 0;

        // Final table height will be added to as we meet table content
        int table_h = 0;
        table_h += table_border_top;

        // render caption
        if ( caption ) {
            int em = caption->getFont()->getSize();
            int w = table_width - table_border_left - table_border_right;
            // (When border-collapse, these table_border_* will be 0)
            // Note: table padding does not apply to caption, and table padding-top
            // should be applied between the caption and the first row
            // Also, Firefox does not include the caption inside the table outer border.
            // We'll display as Firefox when border-collapse, as the table borders were
            // reset to 0 after we collapsed them to the cells.
            // But not when not border-collapse: we can't do that in crengine because
            // of parent>children boxes containment, so the caption will be at top
            // inside the table border.
            // A caption can have borders, that we must include in its padding:
            // we may then get a double border with the table one... (We could hack
            // caption->style to remove its border if the table has some, if needed.)
            LFormattedTextRef txform;
            RenderRectAccessor fmt( caption );
            fmt.setX( table_border_left );
            fmt.setY( table_h );
            fmt.setWidth( w ); // fmt.width must be set before 'caption->renderFinalBlock'
                               // to have text-indent in % not mess up at render time
            css_style_ref_t caption_style = caption->getStyle();
            int padding_left = lengthToPx( caption_style->padding[0], w, em ) + measureBorder(caption, 3);
            int padding_right = lengthToPx( caption_style->padding[1], w, em ) + measureBorder(caption,1);
            int padding_top = lengthToPx( caption_style->padding[2], w, em ) + measureBorder(caption,0);
            int padding_bottom = lengthToPx( caption_style->padding[3], w, em ) + measureBorder(caption,2);
            if ( enhanced_rendering ) {
                // As done in renderBlockElementEnhanced when erm_final
                fmt.setInnerX( padding_left );
                fmt.setInnerY( padding_top );
                fmt.setInnerWidth( w - padding_left - padding_right );
                RENDER_RECT_SET_FLAG(fmt, INNER_FIELDS_SET);
                RENDER_RECT_SET_DIRECTION(fmt, caption_direction);
                fmt.setLangNodeIndex( TextLangMan::getLangNodeIndex(caption) );
            }
            fmt.push();
            caption_h = caption->renderFinalBlock( txform, &fmt, w - padding_left - padding_right );
            caption_h += padding_top + padding_bottom;
            // Reload fmt, as enode->renderFinalBlock() may have updated it.
            fmt = RenderRectAccessor( caption );
            fmt.setHeight( caption_h );
            fmt.push();
            table_h += caption_h;
        }
        table_h += table_padding_top; // padding top applies after caption
        if (nb_rows > 0) {
            // There must be the full borderspacing_v above first row.
            // Includes half of it here, and the other half when adding the row
            table_h += borderspacing_v_bottom;
        }
        if ( context.wantsLines() ) {
            // Includes table border top + full caption if any + table padding
            // top + half of borderspacing_v.
            // We ask for a split between these and the first row to be avoided,
            // but if it can't, padding-top will be on previous page, leaving
            // more room for the big first row on next page.
            // Any table->style->page-break-before AVOID or ALWAYS has been
            // taken care of by renderBlockElement(), so we can use AVOID here.
            if ( !enhanced_rendering )
                line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
            else
                // but not if called from RenderBlockElementEnhanced, where
                // margins handle page splitting a bit differently
                line_flags = RN_SPLIT_BEFORE_AUTO | RN_SPLIT_AFTER_AVOID;
            if (is_rtl)
                line_flags |= RN_LINE_IS_RTL;
            context.AddLine(last_y, table_y0 + table_h, line_flags);
            last_y = table_y0 + table_h;
        }

        // If table is a single column, we can add to main context
        // the lines of each cell, instead of full table rows, which
        // will avoid cell lines to possibly be cut in the middle.
        // (When multi-columns, because same-row cells' lines may
        // not align, and because it's easier for the reader to
        // not have to go back one page to read next cell (from
        // same row) content, each row is considered a single
        // line in the matter of page splitting.)
        bool is_single_column = false;
        int min_row_height_for_split_by_line;
        if ( context.wantsLines() && cols.length() == 1 && enhanced_rendering ) {
            is_single_column = true;
            // We actually don't know if splitting by line is better
            // than splitting by row. By-row might be better if it's
            // a real table where rows really list items, while by-lines
            // is preferable where the table is just used as a content
            // wrapper. So, let's follow this simple rule from:
            // https://www.w3.org/TR/css-tables-3/#fragmentation
            //   "must attempt to preserve the table rows unfragmented if
            //   the cells spanning the row do not span any subsequent row,
            //   and their height is at least twice smaller than both the
            //   fragmentainer height and width. Other rows are said
            //   freely fragmentable."
            // which looks like it applies to multi-columns tables too
            // (but our lvpagesplitter will manage this as fine) - but
            // let's just use that rule for single-columns tables.
            int page_height = elem->getDocument()->getPageHeight();
            int page_width = elem->getDocument()->getPageWidth();
            if ( page_width < page_height )
                min_row_height_for_split_by_line = page_width / 2;
            else
                min_row_height_for_split_by_line = page_height / 2;
            // Looks like cells having rowspan > 1 (which makes no sense
            // if only own column) shouldn't need any specific tweaks.
            // Also, as we don't apply cells and row style height, and
            // vertical-align shouldn't change anything if there are no
            // other cell that would increase the row height, this too
            // shouldn't need any check.
            // (Note that a TD style height will still be applied if rendered
            // as erm_block (but not if erm_final), but vertical-align won't
            // be ensured. todo: make that consistent.)
            // todo: we could also try to do that even if multi columns,
            // by merging multiple cells' LVRendPageContext.
        }

        int i, j;
        // Calc individual cells dimensions
        for (i=0; i<rows.length(); i++) {
            CCRTableRow * row = rows[i];
            bool row_has_baseline_aligned_cells = false;
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                //int x = cell->col->index;
                int y = cell->row->index;
                int n = rows[i]->cells.length();
                if ( i==y ) { // upper left corner of cell
                    // We need to render the cell to get its height
                    if ( cell->elem->getRendMethod() == erm_final ) {
                        LFormattedTextRef txform;
                        int em = cell->elem->getFont()->getSize();
                        css_style_ref_t elem_style = cell->elem->getStyle();
                        int padding_left = lengthToPx( elem_style->padding[0], cell->width, em ) + measureBorder(cell->elem,3);
                        int padding_right = lengthToPx( elem_style->padding[1], cell->width, em ) + measureBorder(cell->elem,1);
                        int padding_top = lengthToPx( elem_style->padding[2], cell->width, em ) + measureBorder(cell->elem,0);
                        int padding_bottom = lengthToPx( elem_style->padding[3], cell->width, em ) + measureBorder(cell->elem,2);
                        RenderRectAccessor fmt( cell->elem );
                        fmt.setWidth( cell->width ); // needed before calling elem->renderFinalBlock
                        if ( enhanced_rendering ) {
                            // As done in renderBlockElementEnhanced when erm_final
                            fmt.setInnerX( padding_left );
                            fmt.setInnerY( padding_top );
                            fmt.setInnerWidth( cell->width - padding_left - padding_right );
                            RENDER_RECT_SET_FLAG(fmt, INNER_FIELDS_SET);
                            RENDER_RECT_SET_DIRECTION(fmt, cell->direction);
                            fmt.setLangNodeIndex( TextLangMan::getLangNodeIndex(cell->elem) );
                        }
                        fmt.push();
                        int h = cell->elem->renderFinalBlock( txform, &fmt, cell->width - padding_left - padding_right);
                        cell->height = padding_top + h + padding_bottom;

                        // A cell baseline is the baseline of its first line of text (or
                        // the bottom of content edge of the cell if no line)
                        if ( cell->valign == 0 ) { // vertical-align: baseline
                            if ( txform->GetLineCount() > 0 ) // we have a line
                                cell->baseline = padding_top + txform->GetLineInfo(0)->baseline;
                            else // no line, no image: bottom of content edge is at padding_top
                                cell->baseline = padding_top;
                        }
                        else { // all other vertical-align: values
                            // "If a row has no cell box aligned to its baseline,
                            // the baseline of that row is the bottom content edge
                            // of the lowest cell in the row."
                            // So, store that bottom content edge in cell->baseline
                            cell->baseline += h;
                        }

                        // Gather footnotes links, as done in renderBlockElement() when erm_final/flgSplit
                        // and cell lines when is_single_column:
                        if ( elem->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) || is_single_column ) {
                            int orphans;
                            int widows;
                            if ( is_single_column ) {
                                orphans = (int)(elem_style->orphans) - (int)(css_orphans_widows_1) + 1;
                                widows = (int)(elem_style->widows) - (int)(css_orphans_widows_1) + 1;
                                // We use a LVRendPageContext that gathers links by line,
                                // so we can transfer them line by line to the upper/main context
                                row->single_col_context = new LVRendPageContext(NULL, context.getPageHeight());
                                row->single_col_context->AddLine(0, padding_top, RN_SPLIT_AFTER_AVOID);
                            }
                            int count = txform->GetLineCount();
                            for (int i=0; i<count; i++) {
                                const formatted_line_t * line = txform->GetLineInfo(i);
                                int link_insert_pos; // used if is_single_column
                                if ( is_single_column ) {
                                    int line_flags = 0;
                                    // Honor widows and orphans
                                    if (orphans > 1 && i > 0 && i < orphans)
                                        line_flags |= RN_SPLIT_BEFORE_AVOID;
                                    if (widows > 1 && i < count-1 && count-1 - i < widows)
                                        line_flags |= RN_SPLIT_AFTER_AVOID;
                                    // Honor line's own flags
                                    if (line->flags & LTEXT_LINE_SPLIT_AVOID_BEFORE)
                                        line_flags |= RN_SPLIT_BEFORE_AVOID;
                                    if (line->flags & LTEXT_LINE_SPLIT_AVOID_AFTER)
                                        line_flags |= RN_SPLIT_AFTER_AVOID;
                                    row->single_col_context->AddLine(padding_top + line->y,
                                                        padding_top + line->y + line->height, line_flags);
                                    if (i == count-1) // add bottom padding
                                        row->single_col_context->AddLine(padding_top + line->y + line->height,
                                            padding_top + line->y + line->height + padding_bottom, RN_SPLIT_BEFORE_AVOID);
                                    if ( !elem->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) )
                                        continue;
                                    if ( line->flags & LTEXT_LINE_PARA_IS_RTL )
                                        link_insert_pos = row->single_col_context->getCurrentLinksCount();
                                    else
                                        link_insert_pos = -1; // append
                                }
                                for ( int w=0; w<line->word_count; w++ ) { // check link start flag for every word
                                    if ( line->words[w].flags & LTEXT_WORD_IS_LINK_START ) {
                                        const src_text_fragment_t * src = txform->GetSrcInfo( line->words[w].src_text_index );
                                        if ( src && src->object ) {
                                            ldomNode * node = (ldomNode*)src->object;
                                            ldomNode * parent = node->getParentNode();
                                            while (parent && parent->getNodeId() != el_a)
                                                parent = parent->getParentNode();
                                            if ( parent && parent->hasAttribute(LXML_NS_ANY, attr_href ) ) {
                                                lString16 href = parent->getAttributeValue(LXML_NS_ANY, attr_href );
                                                if ( href.length()>0 && href.at(0)=='#' ) {
                                                    href.erase(0,1);
                                                    if ( is_single_column )
                                                        row->single_col_context->addLink( href, link_insert_pos );
                                                    else
                                                        row->links.add( href );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                    }
                    else if ( cell->elem->getRendMethod()!=erm_invisible ) {
                        // We must use a different context (used by rendering
                        // functions to record, with context.AddLine(), each
                        // rendered block's height, to be used for splitting
                        // blocks among pages, for page-mode display), so that
                        // sub-renderings (of cells' content) do not add to our
                        // main context. Their heights will already be accounted
                        // in their row's height (added to main context below).
                        // Except when table is a single column, and we can just
                        // transfer lines to the upper context.
                        LVRendPageContext * cell_context;
                        int rend_flags = gRenderBlockRenderingFlags; // global flags
                        if ( is_single_column ) {
                            row->single_col_context = new LVRendPageContext(NULL, context.getPageHeight());
                            cell_context = row->single_col_context;
                            // We want to avoid negative margins (if allowed in global flags) and
                            // going back the flow y, as the transfered lines would not reflect
                            // that, and we could get some small mismatches and glitches.
                            rend_flags &= ~BLOCK_RENDERING_ALLOW_NEGATIVE_COLLAPSED_MARGINS;
                        }
                        else {
                            cell_context = new LVRendPageContext( NULL, context.getPageHeight(), false );
                        }
                        // See above about what we'll store in cell->baseline
                        int baseline = REQ_BASELINE_NOT_NEEDED;
                        if ( cell->valign == 0 ) { // vertical-align: baseline
                            baseline = REQ_BASELINE_FOR_TABLE;
                        }
                        int h = renderBlockElement( *cell_context, cell->elem, 0, 0, cell->width,
                                                    cell->direction, &baseline, rend_flags);
                        cell->height = h;
                        if ( cell->valign == 0 ) { // vertical-align: baseline
                            cell->baseline = baseline;
                        }
                        else {
                            // We'd need the bottom content edge of what's been rendered.
                            // We just need to remove this cell bottom padding (we should
                            // not remove the inner content bottom margins or paddings).
                            int em = cell->elem->getFont()->getSize();
                            css_style_ref_t elem_style = cell->elem->getStyle();
                            int padding_bottom = lengthToPx( elem_style->padding[3], cell->width, em ) + measureBorder(cell->elem,2);
                            cell->baseline = h - padding_bottom;
                        }
                        if ( !is_single_column ) {
                            // Gather footnotes links accumulated by cell_context
                            lString16Collection * link_ids = cell_context->getLinkIds();
                            if (link_ids->length() > 0) {
                                for ( int n=0; n<link_ids->length(); n++ ) {
                                    row->links.add( link_ids->at(n) );
                                }
                            }
                            delete cell_context;
                        }
                    }
                    // RenderRectAccessor needs to be updated after the call
                    // to renderBlockElement() which will have setX/setY to (0,0).
                    // But we're updating them to be in the coordinates of the TR.
                    RenderRectAccessor fmt( cell->elem );
                    // TRs padding and border don't apply (see below), so they
                    // don't add any x/y shift to the cells' positions in the TR
                    fmt.setX(cell->col->x); // relative to its TR (border_spacing_h is
                                            // already accounted in col->x)
                    fmt.setY(0); // relative to its TR
                    fmt.setWidth( cell->width );
                    fmt.setHeight( cell->height );
                    fmt.push();
                    // Some fmt.set* may be updated below
                    #ifdef DEBUG_TABLE_RENDERING
                        printf("TABLE: renderCell[%d,%d] w/h: %d/%d\n", j, i, cell->width, cell->height);
                    #endif
                    if ( cell->rowspan == 1 ) {
                        // Only set row height from this cell height if it is rowspan=1
                        // We'll update rows height from cells with rowspan > 1 just below
                        if ( row->height < cell->height )
                            row->height = cell->height;
                    }
                    // Set the row baseline from baseline-aligned cells' baselines.
                    // https://www.w3.org/TR/CSS22/tables.html#height-layout
                    //   "First the cells that are aligned on their baseline are positioned.
                    //   This will establish the baseline of the row"
                    if ( cell->valign == 0 ) { // only cells with vertical-align: baseline
                        row_has_baseline_aligned_cells = true;
                        if ( row->baseline < cell->baseline )
                            row->baseline = cell->baseline;
                    }
                }
            }
            // Fixup row height and baseline
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                int y = cell->row->index;
                if ( i==y ) { // upper left corner of cell
                    if ( !row_has_baseline_aligned_cells ) {
                        // "If a row has no cell box aligned to its baseline,
                        // the baseline of that row is the bottom content edge
                        // of the lowest cell in the row."
                        // We have computed cell->baseline that way
                        // when cell->valign != 0, so just use it:
                        if ( row->baseline < cell->baseline )
                            row->baseline = cell->baseline;
                        // cell->baseline = 0; // (not needed, as not used)
                    }
                    else if ( cell->valign == 0 ) {
                        // Cells with vertical-align: baseline must align with
                        // the row baseline: this can increase the height of
                        // a cell, and so the height of the row.
                        // As we don't need the real cell->baseline after this,
                        // we store in it the shift-down frop top for this cell,
                        // that we'll use below when handling cell->valign==0.
                        int shift_down = row->baseline - cell->baseline;
                        if ( row->height < cell->height + shift_down )
                            row->height = cell->height + shift_down;
                        cell->baseline = shift_down;
                    }
                    // else cell->baseline = 0; // (not needed, as not used)
                }
            }
        }

        // Update rows heights from multi-row (rowspan > 1) cells height
        for (i=0; i<rows.length(); i++) {
            //CCRTableRow * row = rows[i];
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                //int x = cell->col->index;
                int y = cell->row->index;
                if ( i==y && cell->rowspan>1 ) {
                    int k;
                    int total_h = 0;
                    for ( k=i; k<=i+cell->rowspan-1; k++ ) {
                        CCRTableRow * row2 = rows[k];
                        total_h += row2->height;
                    }
                    int extra_h = cell->height - total_h;
                    if ( extra_h>0 ) {
                        int delta = extra_h / cell->rowspan;
                        int delta_h = extra_h - delta * cell->rowspan;
                        for ( k=i; k<=i+cell->rowspan-1; k++ ) {
                            CCRTableRow * row2 = rows[k];
                            row2->height += delta;
                            if ( delta_h > 0 ) {
                                row2->height++;
                                delta_h--;
                            }
                        }
                    }
                }
            }
        }
        if ( enhanced_rendering ) {
            // Update rows' bottom overflow to include the height of
            // the next rows spanned over by cells with rowspan>1.
            // (This must be done in another loop from the one above)
            for (i=0; i<rows.length(); i++) {
                CCRTableRow * row = rows[i];
                int max_h = 0;
                for (j=0; j<rows[i]->cells.length(); j++) {
                    CCRTableCell * cell = rows[i]->cells[j];
                    int y = cell->row->index;
                    if ( i==y && cell->rowspan>1 ) {
                        int total_h = 0;
                        for ( int k=i; k<=i+cell->rowspan-1; k++ ) {
                            CCRTableRow * row2 = rows[k];
                            total_h += row2->height;
                        }
                        if ( total_h > max_h ) {
                            max_h = total_h;
                        }
                    }
                }
                if ( max_h > row->height ) {
                    row->bottom_overflow = max_h - row->height;
                }
            }
        }

        // update rows y and total height
        //
        // Notes:
        // TR are not supposed to have margin or padding according
        // to CSS 2.1 https://www.w3.org/TR/CSS21/box.html (they could
        // in CSS 2, and may be in CSS 3, not clear), and Firefox ignores
        // them too (no effet whatever value, with border-collapse or not).
        // (If we have to support them, we could account for their top
        // and bottom values here, but the left and right values would
        // need to be accounted above while computing assignable_width for
        // columns content. Given that each row can be styled differently
        // with classNames, we may have different values for each row,
        // which would make computing the assignable_width very tedious.)
        //
        // TR can have borders, but, tested on Firefox with various
        // table styles:
        //   - they are never displayed when NOT border-collapse
        //   - with border-collapse, they are only displayed when
        //     the border is greater than the TD ones, so when
        //     collapsing is applied.
        // So, we don't need to account for TR borders here either:
        // we collapsed them to the cell if border-collapse,
        // and we can just ignore them here, and not draw them in
        // DrawDocument() (Former crengine code did draw the border,
        // but it drew it OVER the cell content, for lack of accounting
        // it in cells placement and content width.)
        for (i=0; i<nb_rows; i++) {
            table_h += borderspacing_v_top;
            CCRTableRow * row = rows[i];
            row->y = table_h;
            // It can happen there is a row that does not map to
            // a node (some are added at start of PlaceCells()),
            // so check for row->elem to avoid a segfault
            if ( row->elem ) {
                RenderRectAccessor fmt( row->elem );
                // TR position relative to the TABLE. If it is contained in a table group
                // (thead, tbody...), these will be adjusted below to be relative to it.
                // (Here were previously added row->elem borders)
                fmt.setX(table_border_left + table_padding_left);
                fmt.setY(row->y);
                fmt.setWidth( table_width - table_border_left - table_padding_left - table_padding_right - table_border_right );
                fmt.setHeight( row->height );
                // This baseline will only be useful if we're part of
                // some flow rendered with REQ_BASELINE_FOR_TABLE
                fmt.setBaseline( row->baseline );
                if ( enhanced_rendering ) {
                    fmt.setBottomOverflow( row->bottom_overflow );
                }
            }
            if ( context.wantsLines() && is_single_column ) {
                // Transfer lines from each row->single_col_context to main context
                // (This has to be done before we update table_h)
                int cur_y = table_y0 + table_h;
                int line_flags = 0;
                if (avoid_pb_inside) {
                    line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
                }
                if (is_rtl)
                    line_flags |= RN_LINE_IS_RTL;
                int content_line_flags = line_flags;
                if ( row->height < min_row_height_for_split_by_line ) {
                    // Too small row height: stick all line together to prevent split
                    // inside this row
                    content_line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
                }
                // Add border spacing top
                int top_line_flags = line_flags | RN_SPLIT_AFTER_AVOID;
                if (i == 0) // first row (or single row): stick to table top padding/border
                    top_line_flags |= RN_SPLIT_BEFORE_AVOID;
                context.AddLine(last_y, cur_y, top_line_flags);
                // Add cell lines
                if ( row->single_col_context && row->single_col_context->getLines() ) {
                    // (It could happen no context was created or no line were added, if
                    // cell was erm_invisible)
                    LVPtrVector<LVRendLineInfo> * lines = row->single_col_context->getLines();
                    for ( int i=0; i < lines->length(); i++ ) {
                        LVRendLineInfo * line = lines->get(i);
                        context.AddLine(cur_y, cur_y+line->getHeight(), line->getFlags()|content_line_flags);
                        LVFootNoteList * links = line->getLinks();
                        if ( links ) {
                            for ( int j=0; j < links->length(); j++ ) {
                                context.addLink( links->get(j)->getId() );
                            }
                        }
                        cur_y += line->getHeight();
                    }
                }
                // Add border spacing bottom
                int bottom_line_flags = line_flags | RN_SPLIT_BEFORE_AVOID;
                if (i == nb_rows-1) // last row (or single row): stick to table bottom padding/border
                    bottom_line_flags |= RN_SPLIT_AFTER_AVOID;
                context.AddLine(cur_y, cur_y+borderspacing_v_bottom, bottom_line_flags|RN_SPLIT_BEFORE_AVOID);
                last_y = cur_y + borderspacing_v_bottom;
                if (last_y != table_y0 + table_h + row->height + borderspacing_v_bottom) {
                    printf("CRE WARNING: single column table row height error %d =! %d\n",
                                last_y, table_y0 + table_h + row->height + borderspacing_v_bottom);
                }
            }
            table_h += row->height;
            table_h += borderspacing_v_bottom;
            if ( context.wantsLines() && !is_single_column ) {
                // Includes the row and half of its border_spacing above and half below.
                if (avoid_pb_inside) {
                    // Avoid any split between rows
                    line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
                }
                else if (i == 0) { // first row (or single row)
                    // Avoid a split between table top border/padding/caption and first row.
                    // Also, the first row could be column headers: avoid a split between it
                    // and the 2nd row. (Any other reason to do that?)
                    line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
                    // Former code had:
                    // line_flags |= CssPageBreak2Flags(getPageBreakBefore(elem))<<RN_SPLIT_BEFORE;
                }
                else if ( i==nb_rows-1 ) { // last row
                    // Avoid a split between last row and previous to last (really?)
                    // Avoid a split between last row and table bottom padding/border
                    //   line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
                    // Let's not avoid a split between last and previous last, as
                    // the last row is most often not a bottom TH, and it would just
                    // drag them onto next page, leaving a hole on previous page.
                    line_flags = RN_SPLIT_BEFORE_AUTO | RN_SPLIT_AFTER_AVOID;
                }
                else {
                    // Otherwise, allow any split between rows, except if
                    // the rows has some bottom overflow, which means it has
                    // some cells with rowspan>1 that we'd rather not have cut.
                    if ( row->bottom_overflow > 0 )
                        line_flags = RN_SPLIT_BEFORE_AUTO | RN_SPLIT_AFTER_AVOID;
                    else
                        line_flags = RN_SPLIT_BEFORE_AUTO | RN_SPLIT_AFTER_AUTO;
                }
                if (is_rtl)
                    line_flags |= RN_LINE_IS_RTL;
                context.AddLine(last_y, table_y0 + table_h, line_flags);
                last_y = table_y0 + table_h;
            }
            // Add links gathered from this row's cells (even if ! context.wantsLines())
            // in case of imbricated tables)
            if (row->links.length() > 0) {
                for ( int n=0; n<row->links.length(); n++ ) {
                    context.addLink( row->links[n] );
                }
            }
        }
        if (nb_rows > 0) {
            // There must be the full borderspacing_v below last row.
            // Includes the last half of it here, as the other half was added
            // above with the row.
            table_h += borderspacing_v_top;
        }
        table_h += table_padding_bottom + table_border_bottom;
        if ( context.wantsLines() ) {
            // Any table->style->page-break-after AVOID or ALWAYS will be taken
            // care of by renderBlockElement(), so we can use AVOID here.
            if ( !enhanced_rendering )
                line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
            else
                // but not if called from RenderBlockElementEnhanced, where
                // margins handle page splitting a bit differently
                line_flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AUTO;
            if (is_rtl)
                line_flags |= RN_LINE_IS_RTL;
            context.AddLine(last_y, table_y0 + table_h, line_flags);
            last_y = table_y0 + table_h;
        }

        // Update each cell height to be its row height, so it can draw its
        // bottom border where it should be: as the row border.
        // We also apply table cells' vertical-align property.
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                //int x = cell->col->index;
                int y = cell->row->index;
                if ( i==y ) {
                    RenderRectAccessor fmt( cell->elem );
                    CCRTableRow * lastrow = rows[ cell->row->index + cell->rowspan - 1 ];
                    int row_h = lastrow->y + lastrow->height - cell->row->y;
                    // Implement CSS property vertical-align for table cells
                    // We have to format the cell with the row height for the borders
                    // to be drawn at the correct positions: we can't just use
                    // fmt.setY(fmt.getY() + pad) below to implement vertical-align.
                    // We have to shift down the cell content itself
                    int cell_h = fmt.getHeight(); // original height that fit cell content
                    fmt.setHeight( row_h );
                    if ( cell_h < row_h ) {
                        int pad = 0; // default when cell->valign=1 / top
                        if (cell->valign == 0) // baseline
                            pad = cell->baseline; // contains the shift-down to align cell and row baselines
                        else if (cell->valign == 2) // center
                            pad = (row_h - cell_h)/2;
                        else if (cell->valign == 3) // bottom
                            pad = (row_h - cell_h);
                        if ( pad == 0 ) // No need to update this cell
                            continue;
                        if ( cell->elem->getRendMethod() == erm_final ) {
                            if ( enhanced_rendering ) {
                                // Just shift down the content box
                                fmt.setInnerY( fmt.getInnerY() + pad );
                            }
                            else {
                                // We need to update the cell element padding-top to include this pad
                                css_style_ref_t style = cell->elem->getStyle();
                                css_style_ref_t newstyle(new css_style_rec_t);
                                copystyle(style, newstyle);
                                // If padding-top is a percentage, it is relative to
                                // the *width* of the containing block
                                int em = cell->elem->getFont()->getSize();
                                int orig_padding_top = lengthToPx( style->padding[2], cell->width, em );
                                newstyle->padding[2].type = css_val_screen_px;
                                newstyle->padding[2].value = orig_padding_top + pad;
                                cell->elem->setStyle(newstyle);
                            }
                        } else if ( cell->elem->getRendMethod() != erm_invisible ) { // erm_block
                            // We need to update each child fmt.y to include this pad
                            for (int i=0; i<cell->elem->getChildCount(); i++) {
                                ldomNode * item = cell->elem->getChildElementNode(i);
                                if ( item ) {
                                    RenderRectAccessor f( item );
                                    f.setY( f.getY() + pad );
                                }
                            }
                        }
                    }
                }
            }
        }

        // Update row groups (thead, tbody...) placement (we need to do that as
        // these rowgroup elements are just block containers of the row elements,
        // and they will be navigated by DrawDocument() to draw each child
        // relative to its container: RenderRectAccessor X & Y are relative to
        // the parent container top left corner)
        //
        // As mentionned above, rowgroups' margins and paddings should be
        // ignored, and their borders are only used with border-collapse,
        // and we collapsed them to the cells when we had to.
        // So, we ignore them here, and DrawDocument() will NOT draw their
        // border.
        for ( int i=0; i<rowgroups.length(); i++ ) {
            CCRTableRowGroup * grp = rowgroups[i];
            if ( grp->rows.length() > 0 ) {
                int y0 = grp->rows.first()->y;
                int y1 = grp->rows.last()->y + grp->rows.last()->height;
                RenderRectAccessor fmt( grp->elem );
                fmt.setY( y0 );
                fmt.setHeight( y1 - y0 );
                fmt.setX( 0 );
                fmt.setWidth( table_width );
                int max_row_bottom_overflow_y = 0;
                for ( int j=0; j<grp->rows.length(); j++ ) {
                    // make row Y position relative to group
                    RenderRectAccessor rowfmt( grp->rows[j]->elem );
                    rowfmt.setY( rowfmt.getY() - y0 );
                    if ( enhanced_rendering ) {
                        // max y (relative to y0) from rows with bottom overflow
                        int row_bottom_overflow_y = rowfmt.getY() + rowfmt.getHeight() + rowfmt.getBottomOverflow();
                        if (row_bottom_overflow_y > max_row_bottom_overflow_y)
                            max_row_bottom_overflow_y = row_bottom_overflow_y;
                    }
                }
                if ( enhanced_rendering ) {
                    // Update row group bottom overflow from the rows max
                    if ( max_row_bottom_overflow_y > fmt.getHeight() ) {
                        fmt.setBottomOverflow( max_row_bottom_overflow_y - fmt.getHeight() );
                    }
                }
            }
        }

        if ( is_single_column ) {
            // Cleanup rows' LVRendPageContext
            for ( i=0; i<rows.length(); i++ ) {
                if ( rows[i]->single_col_context ) {
                    delete rows[i]->single_col_context;
                }
            }
        }

        return table_h;
    }

    CCRTable(ldomNode * tbl_elem, int tbl_width, bool tbl_shrink_to_fit, int tbl_direction, bool tbl_avoid_pb_inside,
                                bool tbl_enhanced_rendering, int dwidth) : digitwidth(dwidth) {
        currentRowGroup = NULL;
        caption = NULL;
        caption_h = 0;
        caption_direction = REND_DIRECTION_UNSET;
        elem = tbl_elem;
        table_width = tbl_width;
        shrink_to_fit = tbl_shrink_to_fit;
        direction = tbl_direction;
        is_rtl = direction == REND_DIRECTION_RTL;
        avoid_pb_inside = tbl_avoid_pb_inside;
        enhanced_rendering = tbl_enhanced_rendering;
        #ifdef DEBUG_TABLE_RENDERING
            printf("TABLE: ============ parsing new table %s\n",
                UnicodeToLocal(ldomXPointer(elem, 0).toString()).c_str());
        #endif
        LookupElem( tbl_elem, direction, 0 );
        PlaceCells();
    }
};

int renderTable( LVRendPageContext & context, ldomNode * node, int x, int y, int width, bool shrink_to_fit,
                 int & fitted_width, int direction, bool avoid_pb_inside, bool enhanced_rendering )
{
    CR_UNUSED2(x, y);
    CCRTable table( node, width, shrink_to_fit, direction, avoid_pb_inside, enhanced_rendering, 10 );
    int h = table.renderCells( context );
    if (shrink_to_fit)
        fitted_width = table.table_width;
    return h;
}

void freeFormatData( ldomNode * node )
{
    node->clearRenderData();
}

bool isSameFontStyle( css_style_rec_t * style1, css_style_rec_t * style2 )
{
    return (style1->font_family == style2->font_family)
        && (style1->font_size == style2->font_size)
        && (style1->font_style == style2->font_style)
        && (style1->font_name == style2->font_name)
        && (style1->font_weight == style2->font_weight);
}

//int rend_font_embolden = STYLE_FONT_EMBOLD_MODE_EMBOLD;
int rend_font_embolden = STYLE_FONT_EMBOLD_MODE_NORMAL;

void LVRendSetFontEmbolden( int addWidth )
{
    if ( addWidth < 0 )
        addWidth = 0;
    else if ( addWidth>STYLE_FONT_EMBOLD_MODE_EMBOLD )
        addWidth = STYLE_FONT_EMBOLD_MODE_EMBOLD;

    rend_font_embolden = addWidth;
}

int LVRendGetFontEmbolden()
{
    return rend_font_embolden;
}

LVFontRef getFont(css_style_rec_t * style, int documentId)
{
    int sz;
    if ( style->font_size.type == css_val_em || style->font_size.type == css_val_ex ||
            style->font_size.type == css_val_percent ) {
        // font_size.type can't be em/ex/%, it should have been converted to px
        // or screen_px while in setNodeStyle().
        printf("CRE WARNING: getFont: %d of unit %d\n", style->font_size.value>>8, style->font_size.type);
        sz = style->font_size.value >> 8; // set some value anyway
    }
    else {
        // We still need to convert other absolute units to px.
        // (we pass 0 as base_em and base_px, as these would not be used).
        sz = lengthToPx(style->font_size, 0, 0);
    }
    if ( sz < 8 )
        sz = 8;
    if ( sz > 340 )
        sz = 340;
    int fw;
    if (style->font_weight>=css_fw_100 && style->font_weight<=css_fw_900)
        fw = ((style->font_weight - css_fw_100)+1) * 100;
    else
        fw = 400;
    fw += rend_font_embolden;
    if ( fw>900 )
        fw = 900;
    // printf("cssd_font_family: %d %s", style->font_family, style->font_name.c_str());
    LVFontRef fnt = fontMan->GetFont(
        sz,
        fw,
        style->font_style==css_fs_italic,
        style->font_family,
        lString8(style->font_name.c_str()),
        style->font_features.value, // (.type is always css_val_unspecified after setNodeStyle())
        documentId, true); // useBias=true, so that our preferred font gets used
    //fnt = LVCreateFontTransform( fnt, LVFONT_TRANSFORM_EMBOLDEN );
    return fnt;
}

lUInt32 styleToTextFmtFlags( const css_style_ref_t & style, lUInt32 oldflags, int direction )
{
    lUInt32 flg = oldflags;
    if (style->display > css_d_inline) {
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
            case css_ta_start:
                flg |= (direction == REND_DIRECTION_RTL ? LTEXT_ALIGN_RIGHT : LTEXT_ALIGN_LEFT);
                break;
            case css_ta_end:
                flg |= (direction == REND_DIRECTION_RTL ? LTEXT_ALIGN_LEFT : LTEXT_ALIGN_RIGHT);
                break;
            case css_ta_auto: // shouldn't happen (only accepted with text-align-last)
            case css_ta_inherit:
                break;
            }
            switch (style->text_align_last)
            {
            case css_ta_left:
                flg |= LTEXT_LAST_LINE_ALIGN_LEFT;
                break;
            case css_ta_right:
                flg |= LTEXT_LAST_LINE_ALIGN_RIGHT;
                break;
            case css_ta_center:
                flg |= LTEXT_LAST_LINE_ALIGN_CENTER;
                break;
            case css_ta_justify:
                flg |= LTEXT_LAST_LINE_ALIGN_WIDTH;
                break;
            case css_ta_start:
                flg |= (direction == REND_DIRECTION_RTL ? LTEXT_LAST_LINE_ALIGN_RIGHT : LTEXT_LAST_LINE_ALIGN_LEFT);
                break;
            case css_ta_end:
                flg |= (direction == REND_DIRECTION_RTL ? LTEXT_LAST_LINE_ALIGN_LEFT : LTEXT_LAST_LINE_ALIGN_RIGHT);
                break;
            case css_ta_auto: // let flg have none of the above set, which will mean "auto"
            case css_ta_inherit:
                break;
            }
        }
    }
    else if ( style->display == css_d_run_in ) {
        flg |= LTEXT_RUNIN_FLAG;
    }
    // We should clean these flags that we got from the parent node via baseFlags:
    // CSS white-space inheritance is correctly handled via styles (so, no need
    // for this alternative way to ensure inheritance with flags), but might have
    // been cancelled and set to some other value (e.g.: normal inside pre)
    flg &= ~(LTEXT_FLAG_PREFORMATTED|LTEXT_FLAG_NOWRAP);
    if ( style->white_space >= css_ws_pre )    // white-space: pre, pre-wrap, break-spaces
        flg |= LTEXT_FLAG_PREFORMATTED;
    if ( style->white_space == css_ws_nowrap ) // white-space: nowrap
        flg |= LTEXT_FLAG_NOWRAP;
    //flg |= oldflags & ~LTEXT_FLAG_NEWLINE;
    return flg;
}

// Convert CSS value (type + number value) to screen px
int lengthToPx( css_length_t val, int base_px, int base_em, bool unspecified_as_em )
{
    if (val.type == css_val_screen_px) { // use value as is
        return val.value;
    }

    // base_px is usually the width of the container element
    // base_em is usually the font size of the parent element
    int px = 0; // returned screen px
    bool ensure_non_zero = true; // return at least 1px if val.value is non-zero
    // Previously, we didn't, so don't ensure non-zero if gRenderDPI=0
    if (!gRenderDPI) ensure_non_zero = false;

    // Scale style value according to gRenderDPI (no scale if 96 or 0)
    // we do that early to not lose precision
    int value = scaleForRenderDPI(val.value);

    css_value_type_t type = val.type;
    if (unspecified_as_em && type == css_val_unspecified)
        type = css_val_em;

    // value for all units is stored *256 to not lose fractional part
    switch( type )
    {
    /* absolute value, most often seen */
    case css_val_px:
        // round it to closest int
        px = (value + 0x7F) >> 8;
        break;

    /* relative values */
    /* We should use val.value (not scaled by gRenderDPI) here */
    case css_val_em: // value = em*256 (font size of the current element)
        px = (base_em * val.value) >> 8;
        break;
    case css_val_percent:
        px = ( base_px * val.value / 100 ) >> 8;
        break;
    case css_val_ex: // value = ex*512 (approximated with base_em, 1ex =~ 0.5em in many fonts)
        px = (base_em * val.value) >> 9;
        break;

    case css_val_rem: // value = rem*256 (font size of the root element)
        px = (gRootFontSize * val.value) >> 8;
        break;

    /* absolute value, less often used - value = unit*256 */
    /* (previously treated by crengine as 0, which we still do if gRenderDPI=0) */
    case css_val_in: // 2.54 cm   1in = 96px
        if (gRenderDPI)
            px = (96 * value ) >> 8;
        break;
    case css_val_cm: //        2.54cm = 96px
        if (gRenderDPI)
            px = (int)(96 * value / 2.54) >> 8;
        break;
    case css_val_mm: //        25.4mm = 96px
        if (gRenderDPI)
            px = (int)(96 * value / 25.4) >> 8;
        break;
    case css_val_pt: // 1/72 in  72pt = 96px
        if (gRenderDPI)
            px = 96 * value / 72 >> 8;
        break;
    case css_val_pc: // 12 pt     6pc = 96px
        if (gRenderDPI)
            px = 96 * value / 6 >> 8;
        break;

    case css_val_unspecified: // may be used with named values like "auto", but should
    case css_val_inherited:            // return 0 when lengthToPx() is called on them
    default:
        px = 0;
        ensure_non_zero = false;
    }
    if (!px && val.value && ensure_non_zero)
        px = 1;
    return px;
}

void SplitLines( const lString16 & str, lString16Collection & lines )
{
    const lChar16 * s = str.c_str();
    const lChar16 * start = s;
    for ( ; *s; s++ ) {
        if ( *s=='\r' || *s=='\n' ) {
            //if ( s > start )
            //    lines.add( cs16("*") + lString16( start, s-start ) + cs16("<") );
            //else
            //    lines.add( cs16("#") );
            if ( (s[1] =='\r' || s[1]=='\n') && (s[1]!=s[0]) )
                s++;
            start = s+1;
        }
    }
    while ( *start=='\r' || *start=='\n' )
        start++;
    if ( s > start )
        lines.add( lString16( start, (lvsize_t)(s-start) ) );
}

// Returns the marker for a list item node. If txform is supplied render the marker, too.
// marker_width is updated and can be used to add indent or padding necessary to make
// room for the marker (what and how to do it depending of list-style_position (inside/outside)
// is left to the caller)
lString16 renderListItemMarker( ldomNode * enode, int & marker_width, LFormattedText * txform, int line_h, lUInt32 flags ) {
    lString16 marker;
    marker_width = 0;
    // The UL > LI parent-child chain may have had some of our boxing elements inserted
    ldomNode * parent = enode->getUnboxedParent();
    ListNumberingPropsRef listProps =  enode->getDocument()->getNodeNumberingProps( parent->getDataIndex() );
    if ( listProps.isNull() ) { // no previously cached info: compute and cache it
        // Scan all our siblings to know the widest marker width
        int counterValue = 0;
        int maxWidth = 0;
        ldomNode * sibling = parent->getUnboxedFirstChild(true);
        while ( sibling ) {
            lString16 marker;
            int markerWidth = 0;
            if ( sibling->getNodeListMarker( counterValue, marker, markerWidth ) ) {
                if ( markerWidth > maxWidth )
                    maxWidth = markerWidth;
            }
            sibling = sibling->getUnboxedNextSibling(true); // skip text nodes
        }
        listProps = ListNumberingPropsRef( new ListNumberingProps(counterValue, maxWidth) );
        enode->getDocument()->setNodeNumberingProps( parent->getDataIndex(), listProps );
    }
    // Note: node->getNodeListMarker() uses font->getTextWidth() without any hint about
    // text direction, so the marker is measured LTR.. We should probably upgrade them
    // to measureText() with the right direction, to get a correct marker_width...
    // For now, as node->getNodeListMarker() adds some whitespace and padding, we should
    // be fine with any small error due to different measuring with LTR vs RTL.
    int counterValue = 0;
    if ( enode->getNodeListMarker( counterValue, marker, marker_width ) ) {
        if ( !listProps.isNull() )
            marker_width = listProps->maxWidth;
        css_style_rec_t * style = enode->getStyle().get();
        LVFont * font = enode->getFont().get();
        lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
        lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
        if (line_h < 0) { // -1, not specified by caller: find it out from the node
            if ( style->line_height.type == css_val_unspecified &&
                        style->line_height.value == css_generic_normal ) {
                line_h = font->getHeight(); // line-height: normal
            }
            else {
                int em = font->getSize();
                line_h = lengthToPx(style->line_height, em, em, true);
            }
            // Scale it according to gInterlineScaleFactor
            if (style->line_height.type != css_val_screen_px && gInterlineScaleFactor != INTERLINE_SCALE_FACTOR_NO_SCALE)
                line_h = (line_h * gInterlineScaleFactor) >> INTERLINE_SCALE_FACTOR_SHIFT;
            if ( style->cr_hint == css_cr_hint_strut_confined )
                flags |= LTEXT_STRUT_CONFINED;
        }
        marker += "\t";
        // That "\t" had some purpose in css_d_list_item_legacy rendering to mark the end
        // of the marker, and by providing the marker_width as negative indent, so that
        // the following text can have some constant indent by rendering it just like
        // negative/hanging text-indent. It has no real use if we provide a 0-indent
        // like we do below.
        // But coincidentally, this "\t" acts for fribidi as a text segment separator (SS)
        // which will bidi-isolate the marker from the followup text, and will ensure,
        // for example, that:
        //   <li style="list-style-type: lower-roman; list-style-type: inside">Some text</li>
        // in a RTL direction context, will be rightly rendered as:
        //   "Some text   xviii"
        // and not wrongly (like it would if we were to use a space instead of \t):
        //   "xviii Some text"
        // (the "xviii" marker will be in its own LTR segment, and the followup text
        // in another LTR segment)
        if ( txform ) {
            TextLangCfg * lang_cfg = TextLangMan::getTextLangCfg( enode );
            txform->AddSourceLine( marker.c_str(), marker.length(), cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, 0, 0);
        }
    }
    return marker;
}

// (Common condition used at multiple occasions, made as as function for clarity)
bool renderAsListStylePositionInside( const css_style_rec_t * style, bool is_rtl=false ) {
    bool render_as_lsp_inside = false;
    if ( style->list_style_position == css_lsp_inside ) {
        return true;
    }
    else if ( style->list_style_position == css_lsp_outside ) {
        // Rendering hack: we do that too when list-style-position = outside AND
        // (with LTR) text-align "right" or "center", as this will draw the marker
        // at the near left of the text (otherwise, the marker would be drawn on
        // the far left of the whole available width, which is ugly)
        css_text_align_t ta = style->text_align;
        if ( ta == css_ta_end ) {
            return true;
        }
        else if ( is_rtl ) {
            if (ta == css_ta_center || ta == css_ta_left )
                return true;
        }
        else {
            if (ta == css_ta_center || ta == css_ta_right )
                return true;
        }
    }
    return false;
}

//=======================================================================
// Render final block
//=======================================================================
// This renderFinalBlock() is NOT the equivalent of renderBlockElement()
// for nodes with erm_final ! But ldomNode::renderFinalBlock() IS.
//
// Here, we just walk all the inline nodes to AddSourceLine() text nodes and
// AddSourceObject() image nodes to the provided LFormattedText * txform.
// It is the similarly named (called by renderBlockElement() when erm_final):
//   ldomNode::renderFinalBlock(LFormattedTextRef & frmtext, RenderRectAccessor * fmt, int width)
// that is provided with a width (with padding removed), and after calling
// this 'void renderFinalBlock()' here, calls:
//   int h = LFormattedTextRef->Format((lUInt16)width, (lUInt16)page_h)
// to do the actual width-constrained rendering of the AddSource*'ed objects.
// Note: fmt is the RenderRectAccessor of the final block itself, and is passed
// as is to the inline children elements: it is only used to get the width of
// the container, which is only needed to compute indent (text-indent) values in %,
// and to get paragraph direction (LTR/RTL/UNSET).
void renderFinalBlock( ldomNode * enode, LFormattedText * txform, RenderRectAccessor * fmt, lUInt32 & baseflags, int indent, int line_h, TextLangCfg * lang_cfg, int valign_dy, bool * is_link_start )
{
    if ( enode->isElement() ) {
        lvdom_element_render_method rm = enode->getRendMethod();
        if ( rm == erm_invisible )
            return; // don't draw invisible

        if ( enode->hasAttribute( attr_lang ) ) {
            lString16 lang_tag = enode->getAttributeValue( attr_lang );
            if ( !lang_tag.empty() )
                lang_cfg = TextLangMan::getTextLangCfg( lang_tag );
        }

        if ( enode->isFloatingBox() && rm != erm_final ) {
            // (A floating floatBox can't be erm_final: it is always erm_block,
            // but let's just be sure of that.)
            // If we meet a floatBox here, it's an embedded float (a float
            // among other inlines elements). We just add a reference to it
            // with AddSourceObject; nothing to do with it until a call
            // to LFormattedTextRef->Format(width) where its width will
            // be guessed and renderBlockElement() called to render it
            // and get is height, so LFormattedText knows how to render
            // this erm_final text around it.
            txform->AddSourceObject(baseflags|LTEXT_SRC_IS_FLOAT, line_h, valign_dy, indent, enode, lang_cfg );
            baseflags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
            return;
        }

        bool is_object = false;
        const css_elem_def_props_t * ntype = enode->getElementTypePtr();
        if ( ntype && ntype->is_object )
            is_object = true;
        // inline-block boxes are handled below quite just like inline images/is_object
        bool is_inline_box = enode->isBoxingInlineBox();

        int direction = RENDER_RECT_PTR_GET_DIRECTION(fmt);
        bool is_rtl = direction == REND_DIRECTION_RTL;

        // About styleToTextFmtFlags:
        // - with inline nodes, it only updates LTEXT_FLAG_PREFORMATTED flag
        //   when css_ws_pre and LTEXT_FLAG_NOWRAP when css_ws_nowrap.
        // - with block nodes (so, only with the first "final" node, and not
        //   when recursing its children which are inline), it will also set
        //   horitontal alignment flags.
        lUInt32 flags = styleToTextFmtFlags( enode->getStyle(), baseflags, direction );
        // Note:
        // - baseflags (passed by reference) is shared and re-used by this node's siblings
        //   (all inline); it should carry newline/horizontal aligment flag, which should
        //   be cleared when used.
        // - flags is provided to this node's children (all inline) (becoming baseflags
        //   for them), and should carry inherited text decoration, vertical alignment
        //   and whitespace-pre state.

        int width = fmt->getWidth();
        int em = enode->getFont()->getSize();
        css_style_rec_t * style = enode->getStyle().get();
        ldomNode * parent = enode->getParentNode(); // Needed for various checks below
        if (parent && parent->isNull())
            parent = NULL;

        // As seen with Firefox, an inline node line-height: do apply, so we need
        // to compute it for all inline nodes, and not only in the "the top and
        // single 'final' node" 'if' below. The computed line-height: of that final
        // node does ensure the strut height, which is the minimal line-height for
        // all inline nodes. But an individual inline node is able to increase that
        // strut height, for the line it happens on only.
        if (gRenderDPI) {
            // line_h is named 'interval' in lvtextfm.cpp.
            //     Note: it was formerly described as: "*16 (16=normal, 32=double)"
            //     and the scaling to the font size (font height actually) was done
            //     in lvtextfm.cpp.
            //     This has been modified so that lvtextfm only accepts interval as
            //     being already the final line height in screen pixels.
            //     So, we only do any conversion from CSS to screen pixels here (and in
            //     setNodeStyle() when the style->line_height is inherited).
            // All related values (%, em, ex, unitless) apply their factor to
            // enode->getFont()->getSize().
            // Only "normal" uses enode->getFont()->getHeight()
            if ( style->line_height.type == css_val_unspecified &&
                        style->line_height.value == css_generic_normal ) {
                line_h = enode->getFont()->getHeight(); // line-height: normal
            }
            else {
                // In all other cases (%, em, unitless/unspecified), we can just scale 'em',
                // and use the computed value for absolute sized values (these will
                // be affected by gRenderDPI) and 'rem' (related to root element font size).
                line_h = lengthToPx(style->line_height, em, em, true);
            }
        }
        else {
            // Let's fallback to the previous (wrong) behaviour when gRenderDPI=0
            // Only do it for the top and single final node
            if ((flags & LTEXT_FLAG_NEWLINE) && rm == erm_final) {
                int fh = enode->getFont()->getHeight(); // former code used font height for everything
                switch( style->line_height.type ) {
                    case css_val_percent:
                    case css_val_em:
                        line_h = lengthToPx(style->line_height, fh, fh);
                        break;
                    default: // Use font height (as line_h=16 in former code)
                        line_h = fh;
                        break;
                }
            }
        }
        if (line_h < 0) {
            // Shouldn't happen, but in case we're called with line_h=-1 and we
            // didn't compute a valid line_h:
            printf("CRE WARNING: line_h still < 0: using 'normal'\n");
            line_h = enode->getFont()->getHeight(); // line-height: normal
        }
        // having line_h=0 is ugly, but it's allowed and it works

        // Scale line_h according to gInterlineScaleFactor, but not if
        // it was already in screen_px, which means it has already been
        // scaled (in setNodeStyle() when inherited).
        if (style->line_height.type != css_val_screen_px && gInterlineScaleFactor != INTERLINE_SCALE_FACTOR_NO_SCALE)
            line_h = (line_h * gInterlineScaleFactor) >> INTERLINE_SCALE_FACTOR_SHIFT;

        if ((flags & LTEXT_FLAG_NEWLINE) && rm == erm_final) {
            // Top and single 'final' node (unless in the degenarate case
            // of obsolete css_d_list_item_legacy):
            // Get text-indent and line-height that will apply to the full final block

            // text-indent should really not have to be handled here: it would be
            // better handled in ldomNode::renderFinalBlock(), grabbing it from the
            // final node, and only passed as an arg to LFormattedText->Format(),
            // like we pass to it the text block width.
            // Current code passes indent to all txform->AddSource*(.., indent,..), so
            // it is stored in each src_text_fragment_t->indent, while it's really
            // a property of the whole paragraph, as it is fetched from the top node,
            // like we do here. (It is never updated, and as it is not passed by reference,
            // updates/reset would not apply to sibling or parent nodes.)
            // There is just one case that sets it to a different value: in the
            // obsolete css_d_list_item_legacy rendering with lsp_outside, where
            // it is set to a negative value (the width of the marker), so to handle text
            // indentation from the outside marker just like regular negative text-indent.
            // So, sadly, let's keep it that way to not break legacy rendering.
            indent = lengthToPx(style->text_indent, width, em);
            // lvstsheet sets the lowest bit to 1 when text-indent has the "hanging" keyword:
            if ( style->text_indent.value & 0x00000001 ) {
                // lvtextfm handles negative indent as "indent by the negated (so, then
                // positive) value all lines but the first"
                indent = -indent;
                // We keep real negative values as negative here. They are also handled
                // in renderBlockElementEnhanced() to possibly have the text block shifted
                // to the left to properly apply the negative effect ("hanging" text-indent
                // does not need that).
            }

            // We set the LFormattedText strut_height and strut_baseline
            // with the values from this "final" node. All lines made out from
            // children will have a minimal height and baseline set to these.
            // See https://www.w3.org/TR/CSS2/visudet.html#line-height
            //   The minimum height consists of a minimum height above
            //   the baseline and a minimum depth below it, exactly as if
            //   each line box starts with a zero-width inline box with the
            //   element's font and line height properties. We call that
            //   imaginary box a "strut."
            // and https://iamvdo.me/en/blog/css-font-metrics-line-height-and-vertical-align
            int fh = enode->getFont()->getHeight();
            int fb = enode->getFont()->getBaseline();
            int f_half_leading = (line_h - fh) / 2;
            txform->setStrut(line_h, fb + f_half_leading);
        }
        else if ( style->cr_hint == css_cr_hint_strut_confined ) {
            // Previous branch for the top final node has set the strut.
            // Inline nodes having "-cr-hint: strut-confined" will be confined
            // inside that strut.
            flags |= LTEXT_STRUT_CONFINED;
        }

        // Now, process styles that may differ between inline nodes, and
        // are needed to display any children text node.

        // Vertical alignment flags & y-drift from main baseline.
        // valign_dy is all that is needed for text nodes, but we need
        // a LTEXT_VALIGN_* flag for objects (images), as their height
        // is not known here, and only computed in lvtextfm.cpp.
        //
        // Texts in quotes from https://www.w3.org/TR/CSS2/visudet.html#line-height
        //
        // We update valign_dy, so it is passed to all children and used
        // as a base for their own vertical align computations.
        // There are a few vertical-align named values that need a special
        // processing for images (their current font is the parent font, and
        // of no use for vertical-alignement).
        css_length_t vertical_align = style->vertical_align;
        if ( (vertical_align.type == css_val_unspecified && vertical_align.value == css_va_baseline) ||
              vertical_align.value == 0 ) {
            // "Align the baseline of the box with the baseline of the parent box.
            //  If the box does not have a baseline, align the bottom margin edge with
            //  the parent's baseline."
            // This is the default behaviour in lvtextfm.cpp: no valign_dy or flags
            // change needed: keep the existing ones (parent's y drift related to
            // the line box main baseline)
        }
        else {
            // We need current and parent nodes font metrics for most computations.
            // A few misc notes:
            //   - Freetype includes any "line gap" from the font metrics
            //   in the ascender (the part above the baseline).
            //   - getBaseline() gives the distance from the top to the baseline (so,
            //   ascender + line gap)
            //   - the descender font value is added to baseline to make height
            //   - getHeight() is usually larger than getSize(), and
            //   getSize() is often nearer to getBaseline().
            //   See: https://iamvdo.me/en/blog/css-font-metrics-line-height-and-vertical-align
            //   Some examples with various fonts at various sizes:
            //     size=9  height=11 baseline=8
            //     size=11 height=15 baseline=11
            //     size=13 height=16 baseline=13
            //     size=19 height=23 baseline=18
            //     size=23 height=31 baseline=24
            //     size=26 height=31 baseline=25
            //   - Freetype has no function to give us font subscript, superscript, x-height
            //   and related values, so we have to approximate them from height and baseline.
            int fh = enode->getFont()->getHeight();
            int fb = enode->getFont()->getBaseline();
            int f_line_h = line_h; // computed above
            int f_half_leading = (f_line_h - fh) /2;
            // Use the current font values if no parent (should not happen thus) to
            // avoid the need for if-checks below
            int pem = em;
            int pfh = fh;
            int pfb = fb;
            if (parent) {
                pem = parent->getFont()->getSize();
                pfh = parent->getFont()->getHeight();
                pfb = parent->getFont()->getBaseline();
            }
            if (vertical_align.type == css_val_unspecified) { // named values
                switch (style->vertical_align.value) {
                    case css_va_sub:
                        // "Lower the baseline of the box to the proper position for subscripts
                        //  of the parent's box."
                        // Use a fraction of the height below the baseline only
                        // 3/5 looks perfect with some fonts, 5/5 looks perfect with
                        // some others, so use 4/5 (which is not the finest with some
                        // fonts, but a sane middle ground)
                        valign_dy += (pfh - pfb)*4/5;
                        flags |= LTEXT_VALIGN_SUB;
                        break;
                    case css_va_super:
                        // "Raise the baseline of the box to the proper position for superscripts
                        //  of the parent's box."
                        // 1/4 of the font height looks alright with most fonts (we could also
                        // use a fraction of 'baseline' only, the height above the baseline)
                        valign_dy -= pfh / 4;
                        flags |= LTEXT_VALIGN_SUPER;
                        break;
                    case css_va_middle:
                        // "Align the vertical midpoint of the box with the baseline of the parent box
                        //  plus half the x-height of the parent."
                        // For CSS lengths, we approximate 'ex' with 1/2 'em'. Let's do the same here.
                        // (Firefox falls back to 0.56 x ascender for x-height:
                        //   valign_dy -= 0.56 * pfb / 2;  but this looks a little too low)
                        if (is_object || is_inline_box)
                            valign_dy -= pem/4; // y for middle of image (lvtextfm.cpp will know from flags)
                        else {
                            valign_dy += fb - fh/2; // move down current middle point to baseline
                            valign_dy -= pem/4; // move up by half of parent ex
                            // This looks different from Firefox rendering, but actually a bit a
                            // better "middle" to me:
                            // valign_dy -= pem/2 - em/2;
                        }
                        flags |= LTEXT_VALIGN_MIDDLE;
                        break;
                    case css_va_text_bottom:
                        // "Align the bottom of the box with the bottom of the parent's content area"
                        // With valign_dy=0, they are centered on the baseline. We want
                        // them centered on their bottom line
                        if (is_object || is_inline_box)
                            valign_dy += (pfh - pfb); // y for bottom of image (lvtextfm.cpp will know from flags)
                        else
                            valign_dy += (pfh - pfb) - (fh - fb) - f_half_leading;
                        flags |= LTEXT_VALIGN_TEXT_BOTTOM;
                        break;
                    case css_va_text_top:
                        // "Align the top of the box with the top of the parent's content area"
                        // With valign_dy=0, they are centered on the baseline. We want
                        // them centered on their top line
                        if (is_object || is_inline_box)
                            valign_dy -= pfb; // y for top of image (lvtextfm.cpp will know from flags)
                        else
                            valign_dy -= pfb - fb - f_half_leading;
                        flags |= LTEXT_VALIGN_TEXT_TOP;
                        break;
                    case css_va_bottom:
                        // "Align the bottom of the aligned subtree with the bottom of the line box"
                        // This will be computed in lvtextfm.cpp when the full line has been laid out.
                        valign_dy = 0; // dummy value
                        flags |= LTEXT_VALIGN_BOTTOM;
                        break;
                    case css_va_top:
                        // "Align the top of the aligned subtree with the top of the line box."
                        // This will be computed in lvtextfm.cpp when the full line has been laid out.
                        valign_dy = 0; // dummy value
                        flags |= LTEXT_VALIGN_TOP;
                        break;
                    case css_va_baseline:
                    default:
                        break;
                }
            }
            else {
                // "<percentage> Raise (positive value) or lower (negative value) the box by this
                //  distance (a percentage of the 'line-height' value).
                //  <length> Raise (positive value) or lower (negative value) the box by this distance"
                // No mention if the base for 'em' should be the current font, or
                // the parent font, and if we should use ->getHeight() or ->getSize().
                // But using the current font size looks correct and similar when
                // comparing to Firefox rendering.
                int base_em = em; // use current font ->getSize()
                int base_pct = line_h;
                // positive values push text up, so reduce dy
                valign_dy -= lengthToPx(vertical_align, base_pct, base_em);
            }
        }
        switch ( style->text_decoration ) {
            case css_td_underline:
            case css_td_blink: // (render it underlined)
                flags |= LTEXT_TD_UNDERLINE;
                break;
            case css_td_overline:
                flags |= LTEXT_TD_OVERLINE;
                break;
            case css_td_line_through:
                flags |= LTEXT_TD_LINE_THROUGH;
                break;
            default:
                break;
        }
        switch ( style->hyphenate ) {
            case css_hyph_auto:
                flags |= LTEXT_HYPHENATE;
                break;
            case css_hyph_none:
                flags &= ~LTEXT_HYPHENATE;
                break;
            default:
                break;
        }

        // Firefox has some specific behaviour with floats, which
        // is not obvious from the specs. Let's do as it does.
        // It looks like we should do the same for inline-block boxes
        if ( parent && (parent->isFloatingBox() || parent->isBoxingInlineBox()) ) {
            if ( rm == erm_final && is_object ) {
                // When an image is the single top final node in a float (which is
                // the case for individual floating images (<IMG style="float: left">),
                // Firefox does not enforce the strut, line-height, vertical-align and
                // text-indent (but it does when in <SPAN style="float: left"><IMG/></SPAN>).
                txform->setStrut(0, 0);
                line_h = 0;
                indent = 0;
                valign_dy = 0;
                // Also, when such a floating image has a width in %, this width
                // has been used to set the width of the floating box. We need to
                // update this % width to be 100%, otherwise the image would be
                // again set to this % of the floating box width...
                // This feels a bit hacky, there might be a better place to deal with that...
                if (style->width.type == css_val_percent && style->width.value != 100*256) {
                    css_style_ref_t oldstyle = enode->getStyle();
                    css_style_ref_t newstyle(new css_style_rec_t);
                    copystyle(oldstyle, newstyle);
                    newstyle->width.value = 100*256; // 100%
                    enode->setStyle(newstyle);
                    style = enode->getStyle().get(); // update to the new style
                }
            }
            // Also, the floating element or inline-block inner element vertical-align drift is dropped
            valign_dy = 0;
            flags &= ~LTEXT_VALIGN_MASK; // also remove any such flag we've set
            flags &= ~LTEXT_STRUT_CONFINED; // remove this if it's been set above
            // (Looks like nothing special to do with indent or line_h)
        }

        if ( style->display == css_d_list_item_legacy ) { // obsolete (used only when gDOMVersionRequested < 20180524)
            // put item number/marker to list
            lString16 marker;
            int marker_width = 0;

            ListNumberingPropsRef listProps =  enode->getDocument()->getNodeNumberingProps( enode->getParentNode()->getDataIndex() );
            if ( listProps.isNull() ) { // no previously cached info: compute and cache it
                // Scan all our siblings to know the widest marker width
                int counterValue = 0;
                int maxWidth = 0;
                ldomNode * sibling = enode->getUnboxedParent()->getUnboxedFirstChild(true);
                while ( sibling ) {
                    lString16 marker;
                    int markerWidth = 0;
                    if ( sibling->getNodeListMarker( counterValue, marker, markerWidth ) ) {
                        if ( markerWidth > maxWidth )
                            maxWidth = markerWidth;
                    }
                    sibling = sibling->getUnboxedNextSibling(true); // skip text nodes
                }
                listProps = ListNumberingPropsRef( new ListNumberingProps(counterValue, maxWidth) );
                enode->getDocument()->setNodeNumberingProps( enode->getParentNode()->getDataIndex(), listProps );
            }
            int counterValue = 0;
            if ( enode->getNodeListMarker( counterValue, marker, marker_width ) ) {
                if ( !listProps.isNull() )
                    marker_width = listProps->maxWidth;
                css_list_style_position_t sp = style->list_style_position;
                LVFont * font = enode->getFont().get();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                int margin = 0;
                if ( sp==css_lsp_outside )
                    margin = -marker_width; // will ensure negative/hanging indent-like rendering
                marker += "\t";
                txform->AddSourceLine( marker.c_str(), marker.length(), cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy,
                                        margin, NULL );
                flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH;
            }
        }

        // List item marker rendering when css_d_list_item_block and list-style-position = inside:
        // render the marker if any, and continue rendering text on same line
        if ( style->display == css_d_list_item_block ) {
            // list_item_block rendered as final (containing only text and inline elements)
            // (we don't draw anything when list-style-type=none)
            if ( renderAsListStylePositionInside(style, is_rtl) && style->list_style_type != css_lst_none ) {
                int marker_width;
                lString16 marker = renderListItemMarker( enode, marker_width, txform, line_h, flags );
                if ( marker.length() ) {
                    flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH;
                }
            }
        }
        if ( rm == erm_final ) {
            // when list_item_block has been rendered as block (containing text and block elements)
            // and list-style-position=inside (or outside when text-align center or right), the
            // list item marker is to be propagated to the first erm_final child.
            // In renderBlockElement(), we saved the list item node index into the
            // RenderRectAccessor of the first child rendered as final.
            // So if we find one, we know we have to add the marker here.
            // (Nothing specific to do if RTL: we just add the marker to the txform content,
            // which is still done in logical order.)
            int listPropNodeIndex = fmt->getListPropNodeIndex();
            if ( listPropNodeIndex ) {
                ldomNode * list_item_block_parent = enode->getDocument()->getTinyNode( listPropNodeIndex );
                int marker_width;
                lString16 marker = renderListItemMarker( list_item_block_parent, marker_width, txform, line_h, flags );
                if ( marker.length() ) {
                    flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH;
                }
            }
        }

        if ( is_object ) { // object element, like <IMG>
            #ifdef DEBUG_DUMP_ENABLED
                logfile << "+OBJECT ";
            #endif
            bool isBlock = style->display == css_d_block;
            if ( isBlock ) {
                // If block image, forget any current flags and start from baseflags (?)
                lUInt32 flags = styleToTextFmtFlags( enode->getStyle(), baseflags, direction );
                //txform->AddSourceLine(L"title", 5, 0x000000, 0xffffff, font, baseflags, interval, margin, NULL, 0, 0);
                LVFont * font = enode->getFont().get();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                lString16 title;
                //txform->AddSourceLine( title.c_str(), title.length(), cl, bgcl, font, LTEXT_FLAG_OWNTEXT|LTEXT_FLAG_NEWLINE, line_h, 0, NULL );
                //baseflags
                title = enode->getAttributeValue(attr_suptitle);
                if ( !title.empty() ) {
                    lString16Collection lines;
                    lines.parse(title, cs16("\\n"), true);
                    for ( int i=0; i<lines.length(); i++ )
                        txform->AddSourceLine( lines[i].c_str(), lines[i].length(), cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, 0, NULL );
                }
                txform->AddSourceObject(flags, line_h, valign_dy, indent, enode, lang_cfg );
                title = enode->getAttributeValue(attr_subtitle);
                if ( !title.empty() ) {
                    lString16Collection lines;
                    lines.parse(title, cs16("\\n"), true);
                    for ( int i=0; i<lines.length(); i++ )
                        txform->AddSourceLine( lines[i].c_str(), lines[i].length(), cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, 0, NULL );
                }
                title = enode->getAttributeValue(attr_title);
                if ( !title.empty() ) {
                    lString16Collection lines;
                    lines.parse(title, cs16("\\n"), true);
                    for ( int i=0; i<lines.length(); i++ )
                        txform->AddSourceLine( lines[i].c_str(), lines[i].length(), cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, 0, NULL );
                }
            } else { // inline image
                // We use the flags computed previously (and not baseflags) as they
                // carry vertical alignment
                txform->AddSourceObject(flags, line_h, valign_dy, indent, enode, lang_cfg );
                flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
            }
        }
        else if ( is_inline_box ) { // inline-block wrapper
            #ifdef DEBUG_DUMP_ENABLED
                logfile << "+INLINEBOX ";
            #endif
            bool is_embedded_block = enode->isEmbeddedBlockBoxingInlineBox();
            if ( is_embedded_block ) {
                // If embedded-block wrapper: it should not be part of the lines
                // made by the surrounding text/elements: we should ensure a new
                // line before and after it.
                if ( !(flags & LTEXT_FLAG_NEWLINE) ) { // (Keep existing one if not yet consumed)
                    // The text-align of the paragraph has been inherited by
                    // all its children, including this inlineBox wrapper.
                    switch (style->text_align) {
                    case css_ta_left:
                        flags |= LTEXT_ALIGN_LEFT;
                        break;
                    case css_ta_right:
                        flags |= LTEXT_ALIGN_RIGHT;
                        break;
                    case css_ta_center:
                        flags |= LTEXT_ALIGN_CENTER;
                        break;
                    case css_ta_justify:
                        flags |= LTEXT_ALIGN_WIDTH;
                        break;
                    case css_ta_start:
                        flags |= (is_rtl ? LTEXT_ALIGN_RIGHT : LTEXT_ALIGN_LEFT);
                        break;
                    case css_ta_end:
                        flags |= (is_rtl ? LTEXT_ALIGN_LEFT : LTEXT_ALIGN_RIGHT);
                        break;
                    case css_ta_inherit:
                    case css_ta_auto:
                        break;
                    }
                }
                // These might have no effect, but let's explicitely drop them.
                valign_dy = 0;
                indent = 0;
                // Note: a space just before or just after (because of a newline in
                // the HTML source) should have been removed or included in the
                // boxing element - so we shouldn't have any spurious empty line
                // in this final block (except it that space is included in some
                // other inline element (<span> </span>) in which case, it is
                // explicitely expected to generate an empty line.
                // We also use LTEXT_SRC_IS_INLINE_BOX (no need to waste a bit in
                // the lUInt32 for LTEXT_SRC_IS_EMBEDDED_BLOCK).
            }
            // We use the flags computed previously (and not baseflags) as they
            // carry vertical alignment
            txform->AddSourceObject(flags|LTEXT_SRC_IS_INLINE_BOX, line_h, valign_dy, indent, enode, lang_cfg );
            if ( is_embedded_block ) {
                // Let flags unchanged, with their newline/alignment flag as if it
                // hadn't been consumed, so it is reported back into baseflags below
                // so that the next sibling (or upper followup inline node) starts
                // on a new line.
            }
            else {
                flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
            }
        }
        else { // non-IMG element: render children (elements or text nodes)
            int cnt = enode->getChildCount();
            #ifdef DEBUG_DUMP_ENABLED
                logfile << "+BLOCK [" << cnt << "]";
            #endif
            // Usual elements
            bool thisIsRunIn = style->display==css_d_run_in;
            if ( thisIsRunIn )
                flags |= LTEXT_RUNIN_FLAG;

            // Some elements add some generated content
            lUInt16 nodeElementId = enode->getNodeId();
            // Don't handle dir= for the erm_final (<p dir="auto"), as it would "isolate"
            // the whole content from the bidi algorithm and we would get a default paragraph
            // direction of LTR. It is handled directly in lvtextfm.cpp.
            bool hasDirAttribute = rm != erm_final && enode->hasAttribute( attr_dir );
            bool addGeneratedContent = hasDirAttribute ||
                                       nodeElementId == el_bdi ||
                                       nodeElementId == el_bdo ||
                                       nodeElementId == el_pseudoElem;
            bool closeWithPDI = false;
            bool closeWithPDF = false;
            bool closeWithPDFPDI = false;
            if ( addGeneratedContent ) {
                // Note: we need to explicitely clear newline flag after
                // any txform->AddSourceLine(). If we delay that and add another
                // char before, this other char would generate a new line.
                LVFont * font = enode->getFont().get();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;

                // The following is needed for fribidi to do the right thing when the content creator
                // has provided hints to explicite ambiguous cases.
                // <bdi> and <bdo> are HTML5 tags allowing to inform or override the bidi algorithm.
                // When meeting them, we add the equivalent unicode opening and closing chars so
                // that fribidi (working on text only) can ensure what's specified with HTML tags.
                // See http://unicode.org/reports/tr9/#Markup_And_Formatting
                lString16 dir = enode->getAttributeValue( attr_dir );
                dir = dir.lowercase(); // (no need for trim(), it's done by the XMLParser)
                if ( nodeElementId == el_bdo ) {
                    // <bdo> (bidirectional override): prevents the bidirectional algorithm from
                    //       rearranging the sequence of characters it encloses
                    //  dir=ltr  => LRO     U+202D  LEFT-TO-RIGHT OVERRIDE
                    //  dir=rtl  => RLO     U+202E  RIGHT-TO-LEFT OVERRIDE
                    //  leaving  => PDF     U+202C  POP DIRECTIONAL FORMATTING
                    // The link above suggest using these combinations:
                    //  dir=ltr  => FSI LRO
                    //  dir=rtl  => FSI RLO
                    //  leaving  => PDF PDI
                    // but it then doesn't have the intended effect (fribidi bug or limitation?)
                    if ( dir.compare("rtl") == 0 ) {
                        // txform->AddSourceLine( L"\x2068\x202E", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        // closeWithPDFPDI = true;
                        txform->AddSourceLine( L"\x202E", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        closeWithPDF = true;
                        flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                    }
                    else if ( dir.compare("ltr") == 0 ) {
                        // txform->AddSourceLine( L"\x2068\x202D", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        // closeWithPDFPDI = true;
                        txform->AddSourceLine( L"\x202D", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        closeWithPDF = true;
                        flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                    }
                }
                else if ( hasDirAttribute || nodeElementId == el_bdi ) {
                    // <bdi> (bidirectional isolate): isolates its content from the surrounding text,
                    //   and to be used also for any inline elements with "dir=":
                    //  dir=ltr  => LRI     U+2066  LEFT-TO-RIGHT ISOLATE
                    //  dir=rtl  => RLI     U+2067  RIGHT-TO-LEFT ISOLATE
                    //  dir=auto => FSI     U+2068  FIRST STRONG ISOLATE
                    //  leaving  => PDI     U+2069  POP DIRECTIONAL ISOLATE
                    if ( dir.compare("rtl") == 0 ) {
                        txform->AddSourceLine( L"\x2067", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        closeWithPDI = true;
                        flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                    }
                    else if ( dir.compare("ltr") == 0 ) {
                        txform->AddSourceLine( L"\x2066", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        closeWithPDI = true;
                        flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                    }
                    else if ( nodeElementId == el_bdi || dir.compare("auto") == 0 ) {
                        txform->AddSourceLine( L"\x2068", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                        closeWithPDI = true;
                        flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                    }
                    // Pre HTML5, we would have used for any inline tag with a dir= attribute:
                    //  dir=ltr  => LRE     U+202A  LEFT-TO-RIGHT EMBEDDING
                    //  dir=rtl  => RLE     U+202B  RIGHT-TO-LEFT EMBEDDING
                    //  leaving  => PDF     U+202C  POP DIRECTIONAL FORMATTING
                }
                // Note: in lvtextfm, we have to explicitely ignore these (added by us,
                // or already present in the HTML), in measurement and drawing, as
                // FreeType could draw some real glyphes for these, when the font
                // provide a glyph (ie: "[FSI]"). No issue when HarfBuzz is used.
                //
                // Note: if we wanted to support <ruby> tags, we could use the same kind
                // of trick. Unicode provides U+FFF9 to U+FFFA to wrap ruby content.
                // HarfBuzz does not support these (because multiple font sizes would
                // be involved for drawing ruby), but lvtextfm could deal with these
                // itself (by ignoring them in measurement, going back the previous
                // advance, increasing the line height, drawing above...)

                // BiDi stuff had to be outputed first, before any pseudo element
                // (if <q dir="rtl">...</q>, the added quote (first child pseudo element)
                // should be inside the RTL bidi isolation.
                if ( nodeElementId == el_pseudoElem ) {
                    lString16 content = get_applied_content_property(enode);
                    if ( !content.empty() ) {
                        int em = font->getSize();
                        int letter_spacing = lengthToPx(style->letter_spacing, em, em);
                        txform->AddSourceLine( content.c_str(), content.length(), cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent, NULL, 0, letter_spacing);
                        flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                    }
                }
            }

            // is_link_start is given to inner elements (to flag the first
            // text node part of a link), and will be reset to false by
            // the first non-space-only text node
            bool * is_link_start_p = is_link_start; // copy of orignal (possibly NULL) pointer
            bool tmp_is_link_start = true; // new bool, for new pointer if we're a <A>
            if ( nodeElementId == el_a ) {
                is_link_start_p = &tmp_is_link_start; // use new pointer
            }
            for (int i=0; i<cnt; i++)
            {
                ldomNode * child = enode->getChildNode( i );
                renderFinalBlock( child, txform, fmt, flags, indent, line_h, lang_cfg, valign_dy, is_link_start_p );
            }

            if ( addGeneratedContent ) {
                LVFont * font = enode->getFont().get();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                // See comment above: these are the closing counterpart
                if ( closeWithPDI ) {
                    txform->AddSourceLine( L"\x2069", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                    flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                }
                else if ( closeWithPDFPDI ) {
                    txform->AddSourceLine( L"\x202C\x2069", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                    flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                }
                else if ( closeWithPDF ) {
                    txform->AddSourceLine( L"\x202C", 1, cl, bgcl, font, lang_cfg, flags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, indent);
                    flags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                }
            }

            // Note: CSS "display: run-in" is no longer used with our epub.css (it is
            // used with older css files for "body[name="notes"] section title", either
            // for crengine internal footnotes displaying, or some FB2 features)
            if ( thisIsRunIn ) {
                // append space to run-in object
                LVFont * font = enode->getFont().get();
                css_style_ref_t style = enode->getStyle();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                lChar16 delimiter[] = {UNICODE_NO_BREAK_SPACE, UNICODE_NO_BREAK_SPACE}; //160
                txform->AddSourceLine( delimiter, sizeof(delimiter)/sizeof(lChar16), cl, bgcl, font, lang_cfg,
                                            LTEXT_RUNIN_FLAG | LTEXT_FLAG_PREFORMATTED | LTEXT_FLAG_OWNTEXT,
                                            line_h, valign_dy, 0, NULL );
                flags &= ~LTEXT_RUNIN_FLAG;
            }
        }

        #ifdef DEBUG_DUMP_ENABLED
            for (int i=0; i<enode->getNodeLevel(); i++)
                logfile << " . ";
            lvRect rect;
            enode->getAbsRect( rect );
            logfile << "<" << enode->getNodeName() << ">     flags( "
                << baseflags << "-> " << flags << ")  rect( "
                << rect.left << rect.top << rect.right << rect.bottom << ")\n";
        #endif

        // Children may have consumed the newline flag, or may have added one
        // (if the last one of them is a <BR>, it will not have been consumed
        // eg. with <P><SMALL>Some small text<BR/></SMALL> and normal text</P>)
        // So, forward the newline state from flags to baseflags:
        if ( flags & LTEXT_FLAG_NEWLINE ) {
            baseflags |= flags & LTEXT_FLAG_NEWLINE;
            // Also forward any CLEAR flag not consumed
            baseflags |= flags & LTEXT_SRC_IS_CLEAR_BOTH;
        }
        else { // newline consumed
            baseflags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
        }
        if ( enode->getNodeId()==el_br ) {
            if (baseflags & LTEXT_FLAG_NEWLINE) {
                // We meet a <BR/>, but no text node were met before (or it
                // would have cleared the newline flag).
                // Output a single space so that a blank line can be made,
                // as wanted by a <BR/>.
                // (This makes consecutive and stuck <br><br><br> work)
                LVFont * font = enode->getFont().get();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                txform->AddSourceLine( L" ", 1, cl, bgcl, font, lang_cfg,
                                        baseflags | LTEXT_FLAG_PREFORMATTED | LTEXT_FLAG_OWNTEXT,
                                        line_h, valign_dy);
                // baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
                // No need to clear the flag, as we set it just below
                // (any LTEXT_ALIGN_* set implies LTEXT_FLAG_NEWLINE)
            }
            // Re-set the newline and aligment flag for what's coming
            // after this <BR/>
            //baseflags |= LTEXT_ALIGN_LEFT;
            switch (style->text_align) {
            case css_ta_left:
                baseflags |= LTEXT_ALIGN_LEFT;
                break;
            case css_ta_right:
                baseflags |= LTEXT_ALIGN_RIGHT;
                break;
            case css_ta_center:
                baseflags |= LTEXT_ALIGN_CENTER;
                break;
            case css_ta_justify:
                baseflags |= LTEXT_ALIGN_WIDTH;
                break;
            case css_ta_start:
                baseflags |= (is_rtl ? LTEXT_ALIGN_RIGHT : LTEXT_ALIGN_LEFT);
                break;
            case css_ta_end:
                baseflags |= (is_rtl ? LTEXT_ALIGN_LEFT : LTEXT_ALIGN_RIGHT);
                break;
            case css_ta_inherit:
            case css_ta_auto:
                break;
            }
            // Among inline nodes, only <BR> can carry a "clear: left/right/both".
            // (No need to check for BLOCK_RENDERING_G(FLOAT_FLOATBOXES), this
            // should have no effect when there is not a single float in the way)
            baseflags &= ~LTEXT_SRC_IS_CLEAR_BOTH; // clear previous one
            switch (style->clear) {
            case css_c_left:
                baseflags |= LTEXT_SRC_IS_CLEAR_LEFT;
                break;
            case css_c_right:
                baseflags |= LTEXT_SRC_IS_CLEAR_RIGHT;
                break;
            case css_c_both:
                baseflags |= LTEXT_SRC_IS_CLEAR_BOTH;
                break;
            default:
                break;
            }
        }
        //baseflags &= ~LTEXT_RUNIN_FLAG;
        if ( rm == erm_final && (baseflags & LTEXT_SRC_IS_CLEAR_BOTH) ) {
            // We're leaving the top final node with a clear: not consumed
            // (set by a last or single <br clear=>), with no follow-up
            // txform->AddSourceLine() that would have carried it.
            // Add an empty source: this should be managed specifically
            // by lvtextfm.cpp splitParagraphs() to not add this empty
            // string to text, and just call floatClearText().
            LVFont * font = enode->getFont().get();
            lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
            lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
            txform->AddSourceLine( L" ", 1, cl, bgcl, font, lang_cfg,
                            baseflags | LTEXT_SRC_IS_CLEAR_LAST | LTEXT_FLAG_PREFORMATTED | LTEXT_FLAG_OWNTEXT,
                            line_h, valign_dy);
        }
    }
    else if ( enode->isText() ) {
        // text nodes
        lString16 txt = enode->getText();
        if ( !txt.empty() ) {
            #ifdef DEBUG_DUMP_ENABLED
                for (int i=0; i<enode->getNodeLevel(); i++)
                    logfile << " . ";
                logfile << "#text" << " flags( "
                    << baseflags << ")\n";
            #endif

            ldomNode * parent = enode->getParentNode();
            lUInt32 tflags = LTEXT_FLAG_OWNTEXT;
            // if ( parent->getNodeId() == el_a ) // "123" in <a href=><sup>123</sup></a> would not be flagged
            if (is_link_start && *is_link_start) { // was propagated from some outer <A>
                tflags |= LTEXT_IS_LINK; // used to gather in-page footnotes
                lString16 tmp = lString16(txt);
                if (!tmp.trim().empty()) // non empty text, will make out a word
                    *is_link_start = false;
                    // reset to false, so next text nodes in that link are not
                    // flagged, and don't make out duplicate in-page footnotes
            }
            LVFont * font = parent->getFont().get();
            css_style_ref_t style = parent->getStyle();
            lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
            lUInt32 bgcl = 0xFFFFFFFF;
            if ( style->background_color.type == css_val_color && (lUInt32)style->background_color.value != 0xFFFFFFFF ) {
                bgcl = style->background_color.value;
                // Avoid painting same background color as upper node, as it may cover any background image
                ldomNode * gparent = parent->getParentNode();
                if( gparent && !gparent->isNull() ) {
                    css_length_t gparent_bgcolor = gparent->getStyle()->background_color;
                    if ( gparent_bgcolor.type == css_val_color && (lUInt32)gparent_bgcolor.value == bgcl ) {
                        bgcl=0xFFFFFFFF;
                    }
                }
            }

            switch (style->text_transform) {
            case css_tt_uppercase:
                txt.uppercase();
                break;
            case css_tt_lowercase:
                txt.lowercase();
                break;
            case css_tt_capitalize:
                txt.capitalize();
                break;
            case css_tt_full_width:
                // txt.fullWidthChars(); // disabled for now (may change CJK rendering)
                break;
            case css_tt_none:
            case css_tt_inherit:
                break;
            }

            int letter_spacing;
            // % is not supported for letter_spacing by Firefox, but crengine
            // did support it, by relating it to font size, so let's use em
            // in place of width
            // lengthToPx() will correctly return 0 with css_generic_normal
            int em = font->getSize();
            letter_spacing = lengthToPx(style->letter_spacing, em, em);
            /*
            if ( baseflags & LTEXT_FLAG_PREFORMATTED ) {
                int flags = baseflags | tflags;
                lString16Collection lines;
                SplitLines( txt, lines );
                for ( int k=0; k<lines.length(); k++ ) {
                    lString16 str = lines[k];
                    txform->AddSourceLine( str.c_str(), str.length(), cl, bgcl,
                        font, flags, line_h, 0, node, 0, letter_spacing );
                    flags &= ~LTEXT_FLAG_NEWLINE;
                    flags |= LTEXT_ALIGN_LEFT;
                }
            } else {
            }
            */
            /* removal of leading spaces is now managed directly by lvtextfm
            //int offs = 0;
            if ( (txform->GetSrcCount()==0 || (tflags & LTEXT_IS_LINK)) && style->white_space!=css_ws_pre ) {
                // clear leading spaces for first text of paragraph
                int i=0;
                for ( ;txt.length()>i && (txt[i]==' ' || txt[i]=='\t'); i++ )
                    ;
                if ( i>0 ) {
                    txt.erase(0, i);
                    //offs = i;
                }
            }
            */
            if ( txt.length()>0 ) {
                txform->AddSourceLine( txt.c_str(), txt.length(), cl, bgcl, font, lang_cfg, baseflags | tflags,
                    line_h, valign_dy, indent, enode, 0, letter_spacing );
                baseflags &= ~LTEXT_FLAG_NEWLINE & ~LTEXT_SRC_IS_CLEAR_BOTH; // clear newline flag
                // To show the lang tag for the lang used for this text node AFTER it:
                // lString16 lang_tag_txt = L"[" + (lang_cfg ? lang_cfg->getLangTag() : lString16("??")) + L"]";
                // txform->AddSourceLine( lang_tag_txt.c_str(), lang_tag_txt.length(), cl, bgcl, font,
                //          lang_cfg, baseflags|tflags|LTEXT_FLAG_OWNTEXT, line_h, valign_dy, 0, NULL );
            }
        }
    }
    else {
        crFatalError(142, "Unexpected node type");
    }
}

int CssPageBreak2Flags( css_page_break_t prop )
{
    switch (prop)
    {
    case css_pb_auto:
        return RN_SPLIT_AUTO;
    case css_pb_avoid:
        return RN_SPLIT_AVOID;
    case css_pb_always:
    case css_pb_left:
    case css_pb_right:
    case css_pb_page:
    case css_pb_recto:
    case css_pb_verso:
        return RN_SPLIT_ALWAYS;
    default:
        return RN_SPLIT_AUTO;
    }
}

// Only used by renderBlockElementLegacy()
bool isFirstBlockChild( ldomNode * parent, ldomNode * child ) {
    int count = parent->getChildCount();
    for ( int i=0; i<count; i++ ) {
        ldomNode * el = parent->getChildNode(i);
        if ( el==child )
            return true;
        if ( el->isElement() ) {
            lvdom_element_render_method rm = el->getRendMethod();
            if ( rm==erm_final || rm==erm_block ) {
                RenderRectAccessor acc(el);
                if ( acc.getHeight()>5 )
                    return false;
            }
        }
    }
    return true;
}

void copystyle( css_style_ref_t source, css_style_ref_t dest )
{
    dest->display = source->display ;
    dest->white_space = source->white_space ;
    dest->text_align = source->text_align ;
    dest->text_align_last = source->text_align_last ;
    dest->text_decoration = source->text_decoration ;
    dest->text_transform = source->text_transform ;
    dest->vertical_align = source->vertical_align ;
    dest->font_family = source->font_family;
    dest->font_name = source->font_name ;
    dest->font_size.type = source->font_size.type ;
    dest->font_size.value = source->font_size.value ;
    dest->font_style = source->font_style ;
    dest->font_weight = source->font_weight ;
    dest->font_features.type = source->font_features.type ;
    dest->font_features.value = source->font_features.value ;
    dest->text_indent = source->text_indent ;
    dest->line_height = source->line_height ;
    dest->width = source->width ;
    dest->height = source->height ;
    dest->margin[0] = source->margin[0] ;
    dest->margin[1] = source->margin[1] ;
    dest->margin[2] = source->margin[2] ;
    dest->margin[3] = source->margin[3] ;
    dest->padding[0] = source->padding[0] ;
    dest->padding[1] = source->padding[1] ;
    dest->padding[2] = source->padding[2] ;
    dest->padding[3] = source->padding[3] ;
    dest->color = source->color ;
    dest->background_color = source->background_color ;
    dest->letter_spacing = source->letter_spacing ;
    dest->page_break_before = source->page_break_before ;
    dest->page_break_after = source->page_break_after ;
    dest->page_break_inside = source->page_break_inside ;
    dest->hyphenate = source->hyphenate ;
    dest->list_style_type = source->list_style_type ;
    dest->list_style_position = source->list_style_position ;
    dest->border_style_top=source->border_style_top;
    dest->border_style_bottom=source->border_style_bottom;
    dest->border_style_right=source->border_style_right;
    dest->border_style_left=source->border_style_left;
    dest->border_width[0]=source->border_width[0];
    dest->border_width[1]=source->border_width[1];
    dest->border_width[2]=source->border_width[2];
    dest->border_width[3]=source->border_width[3];
    dest->border_color[0]=source->border_color[0];
    dest->border_color[1]=source->border_color[1];
    dest->border_color[2]=source->border_color[2];
    dest->border_color[3]=source->border_color[3];
    dest->background_image=source->background_image;
    dest->background_repeat=source->background_repeat;
    dest->background_attachment=source->background_attachment;
    dest->background_position=source->background_position;
    dest->border_collapse=source->border_collapse;
    dest->border_spacing[0]=source->border_spacing[0];
    dest->border_spacing[1]=source->border_spacing[1];
    dest->orphans = source->orphans;
    dest->widows = source->widows;
    dest->float_ = source->float_;
    dest->clear = source->clear;
    dest->direction = source->direction;
    dest->content = source->content ;
    dest->cr_hint = source->cr_hint;
}

// Only used by renderBlockElementLegacy()
css_page_break_t getPageBreakBefore( ldomNode * el ) {
    if ( el->isText() )
        el = el->getParentNode();
    css_page_break_t before = css_pb_auto;
    while (el) {
        css_style_ref_t style = el->getStyle();
        if ( style.isNull() )
            return before;
        before = style->page_break_before;
        if ( before!=css_pb_auto )
        {
            if(!style.isNull())
            {
                // we should not modify styles directly, as the change in style cache will affect other
                // node with same style, and corrupt style cache Hash, invalidating cache reuse
                css_style_ref_t newstyle( new css_style_rec_t );
                copystyle(style, newstyle);
                newstyle->page_break_before=css_pb_auto;
                newstyle->page_break_inside=style->page_break_inside;
                newstyle->page_break_after=style->page_break_after;
                // we should no more modify a style after it has been applied to a node with setStyle()
                el->setStyle(newstyle);
            }
            return before;
        }
        ldomNode * parent = el->getParentNode();
        if ( !parent )
            return before;
        if ( !isFirstBlockChild(parent, el) )
            return before;
        el = parent;
    }
    return before;
}

// Only used by renderBlockElementLegacy()
css_page_break_t getPageBreakAfter( ldomNode * el ) {
    if ( el->isText() )
        el = el->getParentNode();
    css_page_break_t after = css_pb_auto;
    bool lastChild = true;
    while (el) {
        css_style_ref_t style = el->getStyle();
        if ( style.isNull() )
            return after;
        if ( lastChild && after==css_pb_auto )
            after = style->page_break_after;
        if ( !lastChild || after!=css_pb_auto )
            return after;
        ldomNode * parent = el->getParentNode();
        if ( !parent )
            return after;
        lastChild = ( lastChild && parent->getLastChild()==el );
        el = parent;
    }
    return after;
}

// Only used by renderBlockElementLegacy()
css_page_break_t getPageBreakInside( ldomNode * el ) {
    if ( el->isText() )
        el = el->getParentNode();
    css_page_break_t inside = css_pb_auto;
    while (el) {
        css_style_ref_t style = el->getStyle();
        if ( style.isNull() )
            return inside;
        if ( inside==css_pb_auto )
            inside = style->page_break_inside;
        if ( inside!=css_pb_auto )
            return inside;
        ldomNode * parent = el->getParentNode();
        if ( !parent )
            return inside;
        el = parent;
    }
    return inside;
}

// Only used by renderBlockElementLegacy()
void getPageBreakStyle( ldomNode * el, css_page_break_t &before, css_page_break_t &inside, css_page_break_t &after ) {
    bool firstChild = true;
    bool lastChild = true;
    before = inside = after = css_pb_auto;
    while (el) {
        css_style_ref_t style = el->getStyle();
        if ( style.isNull() )
            return;
        if ( firstChild && before==css_pb_auto ) {
            before = style->page_break_before;
        }
        if ( lastChild && after==css_pb_auto ) {
            after = style->page_break_after;
        }
        if ( inside==css_pb_auto ) {
            inside = style->page_break_inside;
        }
        if ( (!firstChild || before!=css_pb_auto) && (!lastChild || after!=css_pb_auto)
            && inside!=css_pb_auto)
            return;
        ldomNode * parent = el->getParentNode();
        if ( !parent )
            return;
        firstChild = ( firstChild && parent->getFirstChild()==el );
        lastChild = ( lastChild && parent->getLastChild()==el );
        el = parent;
    }
}

// Default border width in screen px when border requested but no width specified
#define DEFAULT_BORDER_WIDTH 2

//measure border width, 0 for top,1 for right,2 for bottom,3 for left
int measureBorder(ldomNode *enode,int border) {
        int em = enode->getFont()->getSize();
        // No need for a width, as border does not support units in % according
        // to CSS specs.
        int width = 0;
        // (Note: another reason for disabling borders in % (that we did support)
        // is that, at the various places where measureBorder() is called,
        // fmt.setWidth() has not yet been called and fmt.getWidth() would
        // return 0. Later, at drawing time, fmt.getWidth() will return the real
        // width, which could cause rendering of borders over child elements,
        // as these were positionned with a border=0.)
        css_style_rec_t * style = enode->getStyle().get();
        if (border==0){
                bool hastopBorder = (style->border_style_top >= css_border_solid &&
                                     style->border_style_top <= css_border_outset);
                if (!hastopBorder) return 0;
                css_length_t bw = style->border_width[0];
                if (bw.value == 0 && bw.type > css_val_unspecified) return 0; // explicit value of 0: no border
                int topBorderwidth = lengthToPx(bw, width, em);
                topBorderwidth = topBorderwidth != 0 ? topBorderwidth : DEFAULT_BORDER_WIDTH;
                return topBorderwidth;}
        else if (border==1){
                bool hasrightBorder = (style->border_style_right >= css_border_solid &&
                                       style->border_style_right <= css_border_outset);
                if (!hasrightBorder) return 0;
                css_length_t bw = style->border_width[1];
                if (bw.value == 0 && bw.type > css_val_unspecified) return 0;
                int rightBorderwidth = lengthToPx(bw, width, em);
                rightBorderwidth = rightBorderwidth != 0 ? rightBorderwidth : DEFAULT_BORDER_WIDTH;
                return rightBorderwidth;}
        else if (border ==2){
                bool hasbottomBorder = (style->border_style_bottom >= css_border_solid &&
                                        style->border_style_bottom <= css_border_outset);
                if (!hasbottomBorder) return 0;
                css_length_t bw = style->border_width[2];
                if (bw.value == 0 && bw.type > css_val_unspecified) return 0;
                int bottomBorderwidth = lengthToPx(bw, width, em);
                bottomBorderwidth = bottomBorderwidth != 0 ? bottomBorderwidth : DEFAULT_BORDER_WIDTH;
                return bottomBorderwidth;}
        else if (border==3){
                bool hasleftBorder = (style->border_style_left >= css_border_solid &&
                                      style->border_style_left <= css_border_outset);
                if (!hasleftBorder) return 0;
                css_length_t bw = style->border_width[3];
                if (bw.value == 0 && bw.type > css_val_unspecified) return 0;
                int leftBorderwidth = lengthToPx(bw, width, em);
                leftBorderwidth = leftBorderwidth != 0 ? leftBorderwidth : DEFAULT_BORDER_WIDTH;
                return leftBorderwidth;}
        else
            return 0;
}

// Only used by renderBlockElementLegacy()
//calculate total margin+padding before node,if >0 don't do campulsory page split
int pagebreakhelper(ldomNode *enode,int width)
{
    int flag=css_pb_auto;
    int em = enode->getFont()->getSize();
    int margin_top = lengthToPx( enode->getStyle()->margin[2], width, em ) + DEBUG_TREE_DRAW;
    int padding_top = lengthToPx( enode->getStyle()->padding[2], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,0);
    flag=CssPageBreak2Flags(getPageBreakBefore(enode))<<RN_SPLIT_BEFORE;
    if (flag==RN_SPLIT_BEFORE_ALWAYS){
        ldomNode *node=enode;
        int top=0;
        while (!node->isNull()) {
            top+=lengthToPx( node->getStyle()->margin[2], width, em ) +
                 lengthToPx( node->getStyle()->padding[2], width, em ) +
                 measureBorder(node,0);
            ldomNode * parent = node->getParentNode();
            if ( !parent ) break;
            if ( !isFirstBlockChild(parent, node) ) break;
            node = parent;
        }
        top-=margin_top+padding_top;
        if (top>0) flag=RN_SPLIT_AUTO;
        if ((getPageBreakBefore(enode)==css_pb_always)) flag=RN_SPLIT_ALWAYS;
    }
    return flag;
}

//=======================================================================
// Render block element
//=======================================================================
// renderBlockElement() aimed at positionning the node provided: setting
// its width and height, and its (x,y) in the coordinates of its parent
// element's border box (so, including parent's paddings and borders
// top/left, but not its margins).
// The provided x and y must include the parent's padding and border, and
// the relative y this new node happen to be in this parent container.
// renderBlockElement() will then add the node's own margins top/left to
// them, to set the (x,y) of this node's border box as its position in
// its parent coordinates.
// So, it just shift coordinates and adjust widths of imbricated block nodes,
// until it meets a "final" node (a node with text or image content), which
// will then have been sized and positionned.
// After the initial rendering, we mostly only care about these positionned
// final nodes (for text rendering, text selection, text search, links...).
// We still walk the block nodes when we need absolute coordinates, computed
// from all the relative shifts of the containing block nodes boxes up to the
// root node. Also when drawing, we draw background and borders of these
// block nodes, nothing much else with them, until we reach a final node
// and we can draw its text and images.

// Prototype of the entry point functions for rendering the root node, a table cell or a float, as defined in lvrend.h:
// int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width, int direction=REND_DIRECTION_UNSET );
// int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width, int direction, int rend_flags );

// Prototypes of the 2 alternative block rendering recursive functions
int  renderBlockElementLegacy( LVRendPageContext & context, ldomNode * enode, int x, int y, int width );
void renderBlockElementEnhanced( FlowState * flow, ldomNode * enode, int x, int width, int flags );

// Legacy/original CRE block rendering
int renderBlockElementLegacy( LVRendPageContext & context, ldomNode * enode, int x, int y, int width )
{
    if (!enode)
        return 0;
    if ( enode->isElement() )
    {
        css_style_rec_t * style = enode->getStyle().get();
        bool isFootNoteBody = false;
        lString16 footnoteId;
        // Allow displaying footnote content at the bottom of all pages that contain a link
        // to it, when -cr-hint: footnote-inpage is set on the footnote block container.
        if ( style->cr_hint == css_cr_hint_footnote_inpage &&
                    enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES)) {
            footnoteId = enode->getFirstInnerAttributeValue(attr_id);
            if ( !footnoteId.empty() )
                isFootNoteBody = true;
            // Notes:
            // It fails when that block element has itself an id, but links
            // do target an other inline sub element id (getFirstInnerAttributeValue()
            // would get the block element id, and there would be no existing footnote
            // for the link target id).
            // Not tested how it would behave with nested "-cr-hint: footnote-inpage"
        }
        // For fb2 documents. Description of the <body> element from FictionBook2.2.xsd:
        //   Main content of the book, multiple bodies are used for additional
        //   information, like footnotes, that do not appear in the main book
        //   flow. The first body is presented to the reader by default, and
        //   content in the other bodies should be accessible by hyperlinks. Name
        //   attribute should describe the meaning of this body, this is optional
        //   for the main body.
        /* Don't do that anymore in this hardcoded / not disable'able way: one can
         * enable in-page footnotes in fb2.css or a style tweak by just using:
         *     body[name="notes"] section    { -cr-hint: footnote-inpage; }
         *     body[name="comments"] section { -cr-hint: footnote-inpage; }
         * which will be hanbled by previous check.
         *
        if ( enode->getNodeId()==el_section && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) {
            ldomNode * body = enode->getParentNode();
            while ( body != NULL && body->getNodeId()!=el_body )
                body = body->getParentNode();
            if ( body ) {
                if (body->getAttributeValue(attr_name) == "notes" || body->getAttributeValue(attr_name) == "comments")
                    footnoteId = enode->getAttributeValue(attr_id);
                    if ( !footnoteId.empty() )
                        isFootNoteBody = true;
            }
        }
        */
//        if ( isFootNoteBody )
//            CRLog::trace("renderBlockElement() : Footnote body detected! %s", LCSTR(ldomXPointer(enode,0).toString()) );
        //if (!fmt)
        //    crFatalError();
//        if ( enode->getNodeId() == el_empty_line )
//            x = x;

        int em = enode->getFont()->getSize();
        int margin_left = lengthToPx( style->margin[0], width, em ) + DEBUG_TREE_DRAW;
        int margin_right = lengthToPx( style->margin[1], width, em ) + DEBUG_TREE_DRAW;
        int margin_top = lengthToPx( style->margin[2], width, em ) + DEBUG_TREE_DRAW;
        int margin_bottom = lengthToPx( style->margin[3], width, em ) + DEBUG_TREE_DRAW;
        int border_top = measureBorder(enode,0);
        int border_bottom = measureBorder(enode,2);
        int padding_left = lengthToPx( style->padding[0], width, em ) + DEBUG_TREE_DRAW + measureBorder(enode,3);
        int padding_right = lengthToPx( style->padding[1], width, em ) + DEBUG_TREE_DRAW + measureBorder(enode,1);
        int padding_top = lengthToPx( style->padding[2], width, em ) + DEBUG_TREE_DRAW + border_top;
        int padding_bottom = lengthToPx( style->padding[3], width, em ) + DEBUG_TREE_DRAW + border_bottom;
        // If there is a border at top/bottom, the AddLine(padding), which adds the room
        // for the border too, should avoid a page break between the node and its border
        int padding_top_split_flag = border_top ? RN_SPLIT_AFTER_AVOID : 0;
        int padding_bottom_split_flag = border_bottom ? RN_SPLIT_BEFORE_AVOID : 0;

        //margin_left += 50;
        //margin_right += 50;
        // Legacy rendering did/does not support negative margins
        if (margin_left < 0) margin_left = 0;
        if (margin_right < 0) margin_right = 0;
        if (margin_top < 0) margin_top = 0;
        if (margin_bottom < 0) margin_bottom = 0;

        if (margin_left>0)
            x += margin_left;
        y += margin_top;

        // Support style 'width:' attribute, for specific elements only: solely <HR> for now.
        // As crengine does not support many fancy display: styles, and each HTML block
        // elements is rendered as a crengine blockElement (an independant full width slice,
        // with possibly some margin/padding/indentation/border, of the document height),
        // we don't want to waste reading width with blank areas (as we are not sure
        // the content producer intended them because of crengine limitations).
        css_length_t style_width = style->width;
        if (style_width.type != css_val_unspecified) {
            // printf("style_width.type: %d (%d)\n", style_width.value, style_width.type);

            bool apply_style_width = false; // Don't apply width by default
            bool style_width_pct_em_only = true; // only apply if width is in '%' or in 'em'
            int  style_width_alignment = 0; // 0: left aligned / 1: centered / 2: right aligned
            // Uncomment for testing alternate defaults:
            // apply_style_width = true;        // apply width to all elements (except table elements)
            // style_width_pct_em_only = false; // accept any kind of unit

            if (enode->getNodeId() == el_hr) { // <HR>
                apply_style_width = true;
                style_width_alignment = 1; // <hr> are auto-centered
                style_width_pct_em_only = false; // width for <hr> is safe, whether px or %
            }

            // Note: we should not handle css_d_inline_table like css_d_table,
            // as it may be used with non-table elements.
            // But we might want to handle (css_d_inline_table & el_table) like we
            // handle css_d_table here and in the 3 other places below - but after
            // some quick thinking (but no check in a browser) it feels we should not,
            // and we'd better ensure and apply style width.
            if (apply_style_width && style->display >= css_d_table ) {
                // table elements are managed elsewhere: we'd rather not mess with the table
                // layout algorithm by applying styles width here (even if this algorithm
                // is not perfect, it looks like applying width here does not make it better).
                apply_style_width = false;
            }
            if (apply_style_width && style_width_pct_em_only) {
                if (style_width.type != css_val_percent && style_width.type != css_val_em) {
                    apply_style_width = false;
                }
            }
            if (apply_style_width) {
                int style_width_px = lengthToPx( style_width, width, em );
                if (style_width_px && style_width_px < width) { // ignore if greater than our given width
                    // printf("style_width: %dps at ~y=%d\n", style_width_px, y);
                    if (style_width_alignment == 1) { // centered
                        x += (width - style_width_px)/2;
                    }
                    else if (style_width_alignment == 2) { // right aligned
                        x += (width - style_width_px);
                    }
                    width = style_width_px;
                }
            }
        }

        bool flgSplit = false;
        width -= margin_left + margin_right;
        int h = 0;
        int pb_flag;
        LFormattedTextRef txform;
        {
            //CRLog::trace("renderBlockElement - creating render accessor");
            RenderRectAccessor fmt( enode );
            fmt.setX( x );
            fmt.setY( y );
            fmt.setWidth( width );
            fmt.setHeight( 0 );
            fmt.push();

            if (width <= 0) {
                // In case we get a negative width (no room to render and draw anything),
                // which may happen in hyper constrained layouts like heavy nested tables,
                // don't go further in the rendering code.
                // It seems erm_block and erm_final do "survive" such negative width,
                // by just keeping substracting margin and padding to this negative
                // number until, in ldomNode::renderFinalBlock(), when it's going to
                // be serious, it is (luckily?) casted to an unsigned int in:
                //   int h = f->Format((lUInt16)width, (lUInt16)page_h);
                // So, a width=-138 becomes width=65398 and the drawing is then done
                // without nearly any width constraint: some text may be drawn, some
                // parts clipped, but the user will see something is wrong.
                // So, we only do the following for tables, where the rendering code
                // is more easily messed up by negative widths. As we won't show
                // any table, and we want the user to notice something is missing,
                // we set this element rendering method to erm_killed, and
                // DrawDocument will then render a small figure...
                if (enode->getRendMethod() >= erm_table) {
                    printf("CRE WARNING: no width to draw %s\n", UnicodeToLocal(ldomXPointer(enode, 0).toString()).c_str());
                    enode->setRendMethod( erm_killed );
                    fmt.setHeight( 15 ); // not squared, so it does not look
                    fmt.setWidth( 10 );  // like a list square bullet
                    fmt.setX( fmt.getX() - 5 );
                        // We shift it half to the left, so a bit of it can be
                        // seen if some element on the right covers it with some
                        // background color.
                    return fmt.getHeight();
                }
            }

            int m = enode->getRendMethod();
            switch( m )
            {
            case erm_killed:
                {
                    // DrawDocument will render a small figure in this rect area
                    fmt.setHeight( 15 ); // not squared, so it does not look
                    fmt.setWidth( 10 );  // like a list square bullet
                    return fmt.getHeight();
                }
                break;
            case erm_table:
                {
                    // ??? not sure
                    if ( isFootNoteBody )
                        context.enterFootNote( footnoteId );
                    lvRect r;
                    enode->getAbsRect(r); // this will get as r.top the absolute Y from
                                          // the relative fmt.setY( y ) we did above.
                    // Was: if (margin_top>0)
                    // but add it even if 0-margin to carry node's page-break-before ALWAYS
                    // or AVOID, so renderTable doesn't have to care about it and can
                    // use AVOID on its first AddLine()
                    pb_flag = pagebreakhelper(enode, width);
                    context.AddLine(r.top - margin_top, r.top, pb_flag);
                    // (margin_top has already been added to make r.top, so we substracted it here)
                    // renderTable() deals itself with the table borders and paddings

                    // We allow a table to shrink width (cells to shrink to their content),
                    // unless they have a width specified.
                    // This can be tweaked with:
                    //   table {width: 100% !important} to have tables take the full available width
                    //   table {width: auto !important} to have tables shrink to their content
                    bool shrink_to_fit = false;
                    int fitted_width = -1;
                    int table_width = width;
                    int specified_width = lengthToPx( style->width, width, em );
                    if (specified_width <= 0) {
                        // We get 0 when width unspecified (not set or when "width: auto"):
                        // use container width, but allow table to shrink
                        // (Should this only be done when explicit (css_val_unspecified, css_generic_auto)?)
                        shrink_to_fit = true;
                    }
                    else {
                        if (specified_width > width)
                            specified_width = width;
                        table_width = specified_width;
                    }
                    int h = renderTable( context, enode, 0, y, table_width, shrink_to_fit, fitted_width );
                    // Should we really apply a specified height ?!
                    int st_h = lengthToPx( style->height, em, em );
                    if ( h < st_h )
                        h = st_h;
                    fmt.setHeight( h );
                    // Update table width if it was fitted/shrunk
                    if (shrink_to_fit && fitted_width > 0)
                        table_width = fitted_width;
                    fmt.setWidth( table_width );
                    if (table_width < width) {
                        // See for margin: auto, to center or align right the table
                        int shift_x = 0;
                        css_length_t m_left = style->margin[0];
                        css_length_t m_right = style->margin[1];
                        bool left_auto = m_left.type == css_val_unspecified && m_left.value == css_generic_auto;
                        bool right_auto = m_right.type == css_val_unspecified && m_right.value == css_generic_auto;
                        if (left_auto) {
                            if (right_auto) { // center align
                                shift_x = (width - table_width)/2;
                            }
                            else { // right align
                                shift_x = (width - table_width);
                            }
                        }
                        if (shift_x) {
                            fmt.setX( fmt.getX() + shift_x );
                        }
                    }

                    fmt.push();
                    enode->getAbsRect(r); // this will get as r.bottom the absolute Y after fmt.setHeight( y )

                    // Was: if(margin_bottom>0)
                    //   context.AddLine(r.bottom, r.bottom + margin_bottom, RN_SPLIT_AFTER_AUTO);;
                    // but add it even if 0-margin to carry node's page-break-after ALWAYS
                    // or AVOID, so renderTable doesn't have to care about it and can
                    // use AVOID on its last AddLine()
                    pb_flag = RN_SPLIT_BEFORE_AVOID;
                    pb_flag |= CssPageBreak2Flags(getPageBreakAfter(enode))<<RN_SPLIT_AFTER;
                    context.AddLine(r.bottom, r.bottom + margin_bottom, pb_flag);
                    if ( isFootNoteBody )
                        context.leaveFootNote();
                    return h + margin_top + margin_bottom; // return block height
                }
                break;
            case erm_block:
                {
                    if ( isFootNoteBody )
                        context.enterFootNote( footnoteId );


                    // recurse all sub-blocks for blocks
                    int y = padding_top;
                    int cnt = enode->getChildCount();
                    lvRect r;
                    enode->getAbsRect(r);
                    if (margin_top>0) {
                        pb_flag = pagebreakhelper(enode, width);
                        context.AddLine(r.top - margin_top, r.top, pb_flag);
                    }
                    if (padding_top>0) {
                        pb_flag = pagebreakhelper(enode,width) | padding_top_split_flag;
                        context.AddLine(r.top, r.top + padding_top, pb_flag);
                    }

                    // List item marker rendering when css_d_list_item_block
                    int list_marker_padding = 0; // set to non-zero when list-style-position = outside
                    int list_marker_height = 0;
                    if ( style->display == css_d_list_item_block ) {
                        // list_item_block rendered as block (containing text and block elements)
                        // Get marker width and height
                        LFormattedTextRef txform( enode->getDocument()->createFormattedText() );
                        int list_marker_width;
                        lString16 marker = renderListItemMarker( enode, list_marker_width, txform.get(), -1, 0);
                        list_marker_height = txform->Format( (lUInt16)(width - list_marker_width), (lUInt16)enode->getDocument()->getPageHeight() );
                        if ( style->list_style_position == css_lsp_outside &&
                            style->text_align != css_ta_center && style->text_align != css_ta_right) {
                            // When list_style_position = outside, we have to shift the whole block
                            // to the right and reduce the available width, which is done
                            // below when calling renderBlockElement() for each child
                            // Rendering hack: we treat it just as "inside" when text-align "right" or "center"
                            list_marker_padding = list_marker_width;
                        }
                        else if ( style->list_style_type != css_lst_none ) {
                            // When list_style_position = inside, we need to let renderFinalBlock()
                            // know there is a marker to prepend when rendering the first of our
                            // children (or grand-children, depth first) that is erm_final
                            // (caveat: the marker will not be shown if any of the first children
                            // is erm_invisible)
                            // (No need to do anything when  list-style-type none.)
                            ldomNode * tmpnode = enode;
                            while ( tmpnode && tmpnode->hasChildren() ) {
                                tmpnode = tmpnode->getChildNode( 0 );
                                if (tmpnode && tmpnode->getRendMethod() == erm_final) {
                                    // We need renderFinalBlock() to be able to reach the current
                                    // enode when it will render/draw this tmpnode, so it can call
                                    // renderListItemMarker() on it and get a marker formatted
                                    // according to current node style.
                                    // We store enode's data index into the RenderRectAccessor of
                                    // this erm_final tmpnode so it's saved in the cache.
                                    // (We used to use NodeNumberingProps to store it, but it
                                    // is not saved in the cache.)
                                    RenderRectAccessor tmpfmt( tmpnode );
                                    tmpfmt.setListPropNodeIndex( enode->getDataIndex() );
                                    break;
                                }
                            }
                        }
                    }

                    int block_height = 0;
                    for (int i=0; i<cnt; i++)
                    {
                        ldomNode * child = enode->getChildNode( i );
                        if (child) {
                            if (child->isText()) {
                                // We may occasionally let empty text nodes among block elements,
                                // just skip them
                                lString16 s = child->getText();
                                if (IsEmptySpace(s.c_str(), s.length()))
                                    continue;
                                crFatalError(144, "Attempting to render non-empty Text node");
                            }
                            //fmt.push();
                            int h = renderBlockElementLegacy(context, child,
                                                             padding_left + list_marker_padding, y,
                                                             width - padding_left - padding_right -
                                                             list_marker_padding);
                            y += h;
                            block_height += h;
                        }
                    }
                    // ensure there's enough height to fully display the list marker
                    if (list_marker_height && list_marker_height > block_height) {
                        y += list_marker_height - block_height;
                    }

                    int st_y = lengthToPx( style->height, em, em );
                    if ( y < st_y )
                        y = st_y;
                    fmt.setHeight( y + padding_bottom ); //+ margin_top + margin_bottom ); //???

                    if (margin_top==0 && padding_top==0) {
                        // If no margin or padding that would have carried the page break above, and
                        // if this page break was not consumed (it is reset to css_pb_auto when used)
                        // by any child node and is still there, add an empty line to carry it
                        pb_flag = pagebreakhelper(enode,width);
                        if (pb_flag)
                            context.AddLine(r.top, r.top, pb_flag);
                    }

                    lvRect rect;
                    enode->getAbsRect(rect);
                    pb_flag = CssPageBreak2Flags(getPageBreakAfter(enode))<<RN_SPLIT_AFTER;
                    if(padding_bottom>0) {
                        int p_pb_flag = margin_bottom>0 ? RN_SPLIT_AFTER_AUTO : pb_flag;
                        p_pb_flag |= padding_bottom_split_flag;
                        context.AddLine(y + rect.top, y + rect.top + padding_bottom, p_pb_flag);
                    }
                    if(margin_bottom>0) {
                        context.AddLine(y + rect.top + padding_bottom,
                                y + rect.top + padding_bottom + margin_bottom, pb_flag);
                    }
                    if (margin_bottom==0 && padding_bottom==0 && pb_flag) {
                        // If no margin or padding to carry pb_flag, add an empty line
                        context.AddLine(y + rect.top, y + rect.top, pb_flag);
                    }
                    if ( isFootNoteBody )
                        context.leaveFootNote();
                    return y + margin_top + margin_bottom + padding_bottom; // return block height
                }
                break;
            case erm_final:
                {

                    if ( style->display == css_d_list_item_block ) {
                        // list_item_block rendered as final (containing only text and inline elements)
                        // Rendering hack: not when text-align "right" or "center", as we treat it just as "inside"
                        if ( style->list_style_position == css_lsp_outside &&
                            style->text_align != css_ta_center && style->text_align != css_ta_right) {
                            // When list_style_position = outside, we have to shift the final block
                            // to the right and reduce its width
                            int list_marker_width;
                            lString16 marker = renderListItemMarker( enode, list_marker_width, NULL, -1, 0 );
                            fmt.setX( fmt.getX() + list_marker_width );
                            width -= list_marker_width;
                        }
                    }

                    if ( isFootNoteBody )
                        context.enterFootNote( footnoteId );
                    // render whole node content as single formatted object
                    fmt.setWidth( width );
                    fmt.setX( fmt.getX() );
                    fmt.setY( fmt.getY() );
                    fmt.setLangNodeIndex( 0 ); // No support for lang in legacy rendering
                    fmt.push();
                    //if ( CRLog::isTraceEnabled() )
                    //    CRLog::trace("rendering final node: %s %d %s", LCSTR(enode->getNodeName()), enode->getDataIndex(), LCSTR(ldomXPointer(enode,0).toString()) );
                    h = enode->renderFinalBlock( txform, &fmt, width - padding_left - padding_right );
                    context.updateRenderProgress(1);
                    // if ( context.updateRenderProgress(1) )
                    //    CRLog::trace("last rendered node: %s %d", LCSTR(enode->getNodeName()), enode->getDataIndex());
    #ifdef DEBUG_DUMP_ENABLED
                    logfile << "\n";
    #endif
                    //int flags = styleToTextFmtFlags( fmt->getStyle(), 0 );
                    //renderFinalBlock( node, &txform, fmt, flags, 0, 16 );
                    //int h = txform.Format( width, context.getPageHeight() );
                    fmt.push();
                    fmt.setHeight( h + padding_top + padding_bottom );
                    flgSplit = true;
                }
                break;
            case erm_invisible:
                // don't render invisible blocks
                return 0;
            default:
                CRLog::error("Unsupported render method %d", m);
                crFatalError(141, "Unsupported render method"); // error
                break;
            }
        }
        if ( flgSplit ) {
            lvRect rect;
            enode->getAbsRect(rect);
            // split pages
            if ( context.wantsLines() ) {
                if (margin_top>0) {
                    pb_flag = pagebreakhelper(enode,width);
                    context.AddLine(rect.top - margin_top, rect.top, pb_flag);
                }
                if (padding_top>0) {
                    pb_flag = pagebreakhelper(enode,width);
                    pb_flag |= padding_top_split_flag;
                    context.AddLine(rect.top, rect.top + padding_top, pb_flag);
                }
                css_page_break_t before, inside, after;
                //before = inside = after = css_pb_auto;
                before = getPageBreakBefore( enode );
                after = getPageBreakAfter( enode );
                inside = getPageBreakInside( enode );

//                if (before!=css_pb_auto) {
//                    CRLog::trace("page break before node %s class=%s text=%s", LCSTR(enode->getNodeName()), LCSTR(enode->getAttributeValue(L"class")), LCSTR(enode->getText(' ', 120) ));
//                }

                //getPageBreakStyle( enode, before, inside, after );
                int break_before = CssPageBreak2Flags( before );
                int break_after = CssPageBreak2Flags( after );
                int break_inside = CssPageBreak2Flags( inside );
                int count = txform->GetLineCount();
                int orphans = (int)(style->orphans) - (int)(css_orphans_widows_1) + 1;
                int widows = (int)(style->widows) - (int)(css_orphans_widows_1) + 1;
                for (int i=0; i<count; i++) {
                    const formatted_line_t * line = txform->GetLineInfo(i);
                    int line_flags = 0; //TODO
                    if (i == 0)
                        line_flags |= break_before << RN_SPLIT_BEFORE;
                    else
                        line_flags |= break_inside << RN_SPLIT_BEFORE;
                    if (i == count-1 && (padding_bottom + margin_bottom == 0))
                        line_flags |= break_after << RN_SPLIT_AFTER;
                    else
                        line_flags |= break_inside << RN_SPLIT_AFTER;
                    if (orphans > 1 && i > 0 && i < orphans)
                        // with orphans:2, and we're the 2nd line (i=1), avoid split before
                        // so we stick to first line
                        line_flags |= RN_SPLIT_AVOID << RN_SPLIT_BEFORE;
                    if (widows > 1 && i < count-1 && count-1 - i < widows)
                        // with widows:2, and we're the last before last line (i=count-2),
                        // avoid split after so we stick to last line
                        line_flags |= RN_SPLIT_AVOID << RN_SPLIT_AFTER;

                    context.AddLine(rect.top + line->y + padding_top,
                        rect.top + line->y + line->height + padding_top, line_flags);

                    if( padding_bottom>0 && i==count-1 ) {
                        pb_flag = margin_bottom>0 ? RN_SPLIT_AFTER_AUTO :
                                (CssPageBreak2Flags(getPageBreakAfter(enode)) << RN_SPLIT_AFTER);
                        pb_flag |= padding_bottom_split_flag;
                        context.AddLine(rect.bottom - padding_bottom, rect.bottom, pb_flag);
                    }
                    if( margin_bottom>0 && i==count-1 ) {
                        pb_flag = CssPageBreak2Flags(getPageBreakAfter(enode)) << RN_SPLIT_AFTER;
                        context.AddLine(rect.bottom, rect.bottom + margin_bottom, pb_flag);
                    }
                    // footnote links analysis
                    if ( !isFootNoteBody && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) { // disable footnotes for footnotes
                        // If paragraph is RTL, we are meeting words in the reverse of the reading order:
                        // so, insert each link for this line at the same position, instead of at the end.
                        int link_insert_pos = -1; // append
                        if ( line->flags & LTEXT_LINE_PARA_IS_RTL ) {
                            link_insert_pos = context.getCurrentLinksCount();
                        }
                        for ( int w=0; w<line->word_count; w++ ) {
                            // check link start flag for every word
                            if ( line->words[w].flags & LTEXT_WORD_IS_LINK_START ) {
                                const src_text_fragment_t * src = txform->GetSrcInfo( line->words[w].src_text_index );
                                if ( src && src->object ) {
                                    ldomNode * node = (ldomNode*)src->object;
                                    ldomNode * parent = node->getParentNode();
                                    while (parent && parent->getNodeId() != el_a)
                                        parent = parent->getParentNode();
                                    if ( parent && parent->hasAttribute(LXML_NS_ANY, attr_href ) ) {
                                            // was: && parent->getAttributeValue(LXML_NS_ANY, attr_type ) == "note")
                                            // but we want to be able to gather in-page footnotes by only
                                            // specifying a -cr-hint: to the footnote target, with no need
                                            // to set one to the link itself
                                        lString16 href = parent->getAttributeValue(LXML_NS_ANY, attr_href );
                                        if ( href.length()>0 && href.at(0)=='#' ) {
                                            href.erase(0,1);
                                            context.addLink( href, link_insert_pos );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } // wantsLines()
            else {
                // we still need to gather links when an alternative context is used
                // (duplicated part of the code above, as we don't want to consume any page-break)
                int count = txform->GetLineCount();
                for (int i=0; i<count; i++) {
                    const formatted_line_t * line = txform->GetLineInfo(i);
                    if ( !isFootNoteBody && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) {
                        // If paragraph is RTL, we are meeting words in the reverse of the reading order:
                        // so, insert each link for this line at the same position, instead of at the end.
                        int link_insert_pos = -1; // append
                        if ( line->flags & LTEXT_LINE_PARA_IS_RTL ) {
                            link_insert_pos = context.getCurrentLinksCount();
                        }
                        for ( int w=0; w<line->word_count; w++ ) {
                            // check link start flag for every word
                            if ( line->words[w].flags & LTEXT_WORD_IS_LINK_START ) {
                                const src_text_fragment_t * src = txform->GetSrcInfo( line->words[w].src_text_index );
                                if ( src && src->object ) {
                                    ldomNode * node = (ldomNode*)src->object;
                                    ldomNode * parent = node->getParentNode();
                                    while (parent && parent->getNodeId() != el_a)
                                        parent = parent->getParentNode();
                                    if ( parent && parent->hasAttribute(LXML_NS_ANY, attr_href ) ) {
                                        lString16 href = parent->getAttributeValue(LXML_NS_ANY, attr_href );
                                        if ( href.length()>0 && href.at(0)=='#' ) {
                                            href.erase(0,1);
                                            context.addLink( href, link_insert_pos );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ( isFootNoteBody )
                context.leaveFootNote();
            return h + margin_top + margin_bottom + padding_top + padding_bottom;
        }
    }
    else
    {
        crFatalError(111, "Attempting to render Text node");
    }
    return 0;
}


//=======================================================================
// FlowState: block formatting context manager
// used by renderBlockElementEnhanced()
//=======================================================================
// Created at the start of a "block formatting context" (one when rendering
// the root node, one when rendering each float or table cell).
// We move down in it as we add elements along the height, and shift
// (and unshift) x/width on nested block elements according to
// elements horizontal margins, borders, paddings and specified or
// computed widths.
// It ensures proper vertical margins collapsing by accumulating added
// vertical margins, and only pushing the resulting margin when some
// real non-margin content is added.
// It also forwards lines/spaces to lvpagesplitter's "context", ensuring
// proper page split flags (and avoiding unwelcome ones).
// Floats are added to it for positionning along this height
// depending on other floats.
// It then can provide a "float footprint" to final nodes, so they can
// lay out their text (and own embedded floats) alongside block floats.
//
// Some initial limitations with floats had later found some solutions,
// which can be enabled by setting some flags. Mentionning them because
// the code might be a bit rooted around these early limitations:
// - Floats added by a block element are "clear"'ed when leaving
//   this block: some blank height is added if necessary, so they are
//   fully contained in this block (they do not overflow the block
//   and are not shown alongside next elements).
//     Can be overcome by using BLOCK_RENDERING_DO_NOT_CLEAR_OWN_FLOATS,
//     which keeps active floats (and have lvtextfm embedded floats
//     forwarded to the main flow): this may cause some issues with
//     text selection in such floats, and may need a double drawing
//     (background first, content next) for later text blocks to not
//     draw their background over already drawn past floats.
// - The footprint provided to final nodes is just a single rectangle
//   anchored at the top left and another one anchored at the top right.
//   These footprints are stored in the cached RenderRectAccessor (that
//   we extended with new slots) of each final node (so we can get it
//   back to re-lay out the text when needed, when looking for links,
//   searching or selecting text).
//     Can be overcome by using BLOCK_RENDERING_ALLOW_EXACT_FLOATS_FOOTPRINTS,
//     which, when there are no more than 5 floats involved on a final
//     node, will store these floats' node Ids in the slots that would
//     otherwise be used for storing the footprint rectangles.
//     Allowing that for more than 5 would need new slots, or another
//     kind of decicated crengine cache for storing a variable number
//     of things related to a node.

#define NO_BASELINE_UPDATE 0x7FFFFFFF

class FlowState {
private:
    // BlockShift: backup of some FlowState fields when entering
    // an inner block (so, making a sub-level).
    class BlockShift { public:
        int direction;
        lInt32 lang_node_idx;
        int x_min;
        int x_max;
        int l_y;
        int in_y_min;
        int in_y_max;
        bool avoid_pb_inside;
        void reset(int dir, lInt32 langNodeIdx, int xmin, int xmax, int ly, int iymin, int iymax, bool avoidpbinside) {
            direction = dir;
            lang_node_idx = langNodeIdx;
            x_min = xmin;
            x_max = xmax;
            l_y = ly;
            in_y_min = iymin;
            in_y_max = iymax;
            avoid_pb_inside = avoidpbinside;
        }
        BlockShift(int dir, lInt32 langNodeIdx, int xmin, int xmax, int ly, int iymin, int iymax, bool avoidpbinside) :
                direction(dir),
                lang_node_idx(langNodeIdx),
                x_min(xmin),
                x_max(xmax),
                l_y(ly),
                in_y_min(iymin),
                in_y_max(iymax),
                avoid_pb_inside(avoidpbinside)
                { }
    };
    class BlockFloat : public lvRect {
        public:
        ldomNode * node;
        int level; // level that owns this float
        bool is_right;
        bool final_pos; // true if y0/y1 are the final absolute position and this
                        // float should not be moved when pushing vertical margins.
        BlockFloat( int x0, int y0, int x1, int y1, bool r, int l, bool f, ldomNode * n=NULL) :
                lvRect(x0,y0,x1,y1),
                level(l),
                is_right(r),
                final_pos(f),
                node(n)
                { }
    };
    int direction; // flow inline direction (LTR/RTL)
    lInt32 lang_node_idx; // dataIndex of nearest upper node with a lang="" attribute (0 if none)
                    // We don't need to know its value in here, the idx of this node
                    // will be saved in the final block RenderRectAccessor so it can
                    // be fetched from the node when needed, when laying out text).
    LVRendPageContext & context;
    LVPtrVector<BlockShift>  _shifts;
    LVPtrVector<BlockFloat>  _floats;
    int  rend_flags;
    int  page_height; // just needed to avoid excessive bogus margins and heights
    int  level;       // current level
    int  o_width;     // initial original container width
    int  c_y;         // current y relative to formatting context top (our absolute y for us here)
    int  l_y;         // absolute y at which current level started
    int  in_y_min;    // min/max children content abs y (for floats in current or inner levels
    int  in_y_max;    //   that overflow this level height)
    int  x_min;       // current left min x
    int  x_max;       // current right max x
    int  baseline_req; // baseline type requested (REQ_BASELINE_FOR_INLINE_BLOCK or REQ_BASELINE_FOR_TABLE)
    int  baseline_y;   // baseline y relative to formatting context top (computed when rendering inline-block/table)
    bool baseline_set; // (set to true on first baseline met)
    bool is_main_flow;
    int  top_clear_level; // level to attach floats for final clearance when leaving the flow
    bool avoid_pb_inside; // To carry this fact from upper elements to inner children
    bool avoid_pb_inside_just_toggled_on;  // for specific processing of boundaries
    bool avoid_pb_inside_just_toggled_off;
    bool seen_content_since_page_split; // to avoid consecutive page split when only empty or padding in between
    int  last_split_after_flag; // in case we need to adjust upcoming line's flag vs previous line's

    // vm_* : state of our handling of collapsable vertical margins
    bool vm_has_some;              // true when some vertical margin added, reset to false when pushed
    bool vm_disabled;              // for disabling vertical margin handling when in edge case situations
    bool vm_target_avoid_pb_inside;
    ldomNode * vm_target_node;     // target element that will be shifted by the collapsed margin
    int  vm_target_level;          // level of this target node
    int  vm_active_pb_flag;        // page-break flag for the whole margin
    int  vm_max_positive_margin;
    int  vm_max_negative_margin;
    int  vm_back_usable_as_margin; // previously moved vertical space where next margin could be accounted in

public:
    FlowState( LVRendPageContext & ctx, int width, int rendflags, int dir=REND_DIRECTION_UNSET, lInt32 langNodeIdx=0 ):
        direction(dir),
        lang_node_idx(langNodeIdx),
        context(ctx),
        rend_flags(rendflags),
        level(0),
        o_width(width),
        c_y(0),
        l_y(0),
        in_y_min(0),
        in_y_max(0),
        x_min(0),
        x_max(width),
        baseline_req(REQ_BASELINE_NOT_NEEDED),
        baseline_y(0),
        baseline_set(false),
        avoid_pb_inside(false),
        avoid_pb_inside_just_toggled_on(false),
        avoid_pb_inside_just_toggled_off(false),
        seen_content_since_page_split(false),
        last_split_after_flag(RN_SPLIT_AUTO),
        vm_has_some(false),
        vm_disabled(false),
        vm_target_node(NULL),
        vm_target_level(0),
        vm_max_positive_margin(0),
        vm_max_negative_margin(0),
        vm_back_usable_as_margin(0),
        vm_target_avoid_pb_inside(false),
        vm_active_pb_flag(RN_SPLIT_AUTO)
        {
            is_main_flow = context.getPageList() != NULL;
            if ( context.wantsLines() ) {
                // Also behave as is_main_flow when context wants lines (which,
                // if it is not the main flow, it should want lines only for
                // transfering them to the real main flow; the only use case
                // for now is when rendering cells in single-column tables).
                is_main_flow = true;
            }
            top_clear_level = is_main_flow ? 1 : 2; // see resetFloatsLevelToTopLevel()
            page_height = context.getPageHeight();
        }
    ~FlowState() {
        // Shouldn't be needed as these must have been cleared
        // by leaveBlockLevel(). But let's ensure we clean up well.
        for (int i=_floats.length()-1; i>=0; i--) {
            BlockFloat * flt = _floats[i];
            _floats.remove(i);
            delete flt;
        }
        for (int i=_shifts.length()-1; i>=0; i--) {
            BlockShift * sht = _shifts[i];
            _shifts.remove(i);
            delete sht;
        }
    }

    bool isMainFlow() {
        return is_main_flow;
    }
    int getDirection() {
        return direction;
    }
    lInt32 getLangNodeIndex() {
        return lang_node_idx;
    }
    int getOriginalContainerWidth() {
        return o_width;
    }
    int getCurrentAbsoluteX() {
        return x_min;
    }
    int getCurrentAbsoluteY() {
        return c_y;
    }
    int getCurrentRelativeY() {
        return c_y - l_y;
    }
    int getCurrentLevel() {
        return level;
    }
    int getCurrentLevelAbsoluteY() {
        return l_y;
    }
    int getPageHeight() {
        return page_height;
    }
    LVRendPageContext * getPageContext() {
        return &context;
    }
    bool getAvoidPbInside() {
        return avoid_pb_inside;
    }

    void setRequestedBaselineType(int baseline_req_type) {
        baseline_req = baseline_req_type;
    }
    int getBaselineAbsoluteY(ldomNode * node=NULL) {
        // Quotes from https://www.w3.org/TR/CSS21/visudet.html#propdef-vertical-align
        // Note that our table rendering code has not been updated to use FlowState,
        // so, if top element is a table, we haven't got any baseline.
        if ( baseline_req == REQ_BASELINE_FOR_TABLE ) {
            // "The baseline of an 'inline-table' is the baseline of the first
            //  row of the table.
            // Tests show that this is true even if the element with
            // display: inline-table is not iself a table, but has a table
            // as a child. But if there's any text non-table before the table,
            // the baseline for that text is used.
            // So, we should check for a table even when we have baseline_set
            // and a baseline_y. The returned baseline will be the smallest
            // of the two.
            //
            // Try to find the first table row, looking at descendants of the
            // provided node (which must be the top node of this FlowState).
            // Walk the tree up and down (avoid the need for recursion):
            ldomNode * n = node;
            ldomNode * rowNode = NULL;
            if ( n && n->getChildCount() > 0 ) {
                int nextChildIndex = 0;
                n = n->getChildNode(nextChildIndex);
                while ( true ) {
                    // Check the node only the first time we meet it
                    // (nextChildIndex == 0) and not when we get back
                    // to it from a child to process next sibling
                    if ( nextChildIndex == 0 ) {
                        if ( n->getRendMethod() == erm_table_row ) {
                            rowNode = n;
                            break; // found the first row
                        }
                    }
                    // Process next child
                    if ( nextChildIndex < n->getChildCount() ) {
                        n = n->getChildNode(nextChildIndex);
                        nextChildIndex = 0;
                        continue;
                    }
                    // No more child, get back to parent and have it process our sibling
                    nextChildIndex = n->getNodeIndex() + 1;
                    n = n->getParentNode();
                    if ( n == node && nextChildIndex >= n->getChildCount() )
                        break; // back to top node and all its children visited
                }
            }
            if ( rowNode ) {
                // Get this row baseline y related to the top node y
                RenderRectAccessor fmt( rowNode );
                int row_baseline = fmt.getY() + fmt.getBaseline();
                ldomNode * n = rowNode->getParentNode();
                for (; n && n!=node; n=n->getParentNode()) {
                    RenderRectAccessor fmt(n);
                    row_baseline += fmt.getY();
                }
                if ( !baseline_set || (row_baseline < baseline_y) ) {
                    baseline_y = row_baseline;
                }
                baseline_set = true;
            }
        }
        if ( !baseline_set ) {
            // "The baseline of an 'inline-block' is the baseline of its last line
            //  box in the normal flow, unless it has either no in-flow line boxes
            //  [or ...], in which case the baseline is the bottom margin edge."
            // So, return what will be returned as height just after this
            // function is called in renderBlockElement().
            return getCurrentAbsoluteY();
        }
        return baseline_y;
    }

    bool hasActiveFloats() {
        return _floats.length() > 0;
    }
    bool hasFloatRunningAtY( int y, int h=0 ) {
        for (int i=0; i<_floats.length(); i++) {
            BlockFloat * flt = _floats[i];
            if (flt->top < y+h && flt->bottom > y) {
                return true;
            }
        }
        return false;
    }

    void addSpaceToContext( int starty, int endy, int line_h,
            bool split_avoid_before, bool split_avoid_inside, bool split_avoid_after ) {
        // Add vertical space by adding multiple lines of height line_h
        // (as an alternative to adding a single huge line), ensuring
        // given split_avoid flags, and avoiding split on the floats met
        // along the way
        if (endy - starty <= 0)
            return;
        int line_dir_flag = direction == REND_DIRECTION_RTL ? RN_LINE_IS_RTL : 0;
        // Ensure avoid_pb_inside
        if ( avoid_pb_inside_just_toggled_off ) {
            avoid_pb_inside_just_toggled_off = false;
            if ( !split_avoid_before && !hasFloatRunningAtY(starty) ) {
                // Previous added line may have RN_SPLIT_AFTER_AVOID, but
                // we want to allow a split between it and this new line:
                // just add an empty line to cancel the split avoid
                context.AddLine( starty, starty, RN_SPLIT_BOTH_AUTO|line_dir_flag );
                last_split_after_flag = RN_SPLIT_AUTO;
            }
        }
        if ( avoid_pb_inside ) {
            // Update provided flags to split avoid
            if ( avoid_pb_inside_just_toggled_on ) {
                avoid_pb_inside_just_toggled_on = false;
                // previous added line may allow a break after,
                // let split_avoid_before unchanged
            }
            else {
                split_avoid_before = true;
            }
            split_avoid_inside = true;
            split_avoid_after = true;
        }

        if (line_h <= 0) // sanity check
            line_h = 1;
        bool is_first = true;
        bool is_last = false;
        int flags = 0;;
        int y0 = starty;
        while (y0 < endy) {
            int y1 = y0 + line_h;
            if (y1 >= endy) {
                y1 = endy;
                is_last = true;
            }
            flags = 0;
            if ( split_avoid_before && is_first )
                flags |= RN_SPLIT_BEFORE_AVOID;
            if ( split_avoid_after && is_last )
                flags |= RN_SPLIT_AFTER_AVOID;
            if ( split_avoid_inside && !is_first & !is_last )
                flags |= RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
            if ( hasFloatRunningAtY(y0) )
                flags |= RN_SPLIT_BEFORE_AVOID;
            flags |= line_dir_flag;
            context.AddLine(y0, y1, flags);
            y0 = y1;
            is_first = false;
        }
        last_split_after_flag = RN_GET_SPLIT_AFTER(flags);
    }

    int addContentLine( int height, int flags, int baseline=NO_BASELINE_UPDATE, bool is_padding=false ) {
        // As we may push vertical margins, we return the total height moved
        // (needed when adding bottom padding that may push inner vertical
        // margins, which should be accounted in the element height).
        int start_c_y = c_y;
        int line_dir_flag = direction == REND_DIRECTION_RTL ? RN_LINE_IS_RTL : 0;
        // Ensure avoid_pb_inside
        if ( avoid_pb_inside_just_toggled_off ) {
            avoid_pb_inside_just_toggled_off = false;
            // Previous added line may have RN_SPLIT_AFTER_AVOID,
            // but we want to allow a split between it and this new
            // line or the coming pushed vertical margin:
            // just add an empty line to cancel the split avoid
            if ( !(flags & RN_SPLIT_BEFORE_AVOID) && !hasFloatRunningAtY(c_y) ) {
                context.AddLine( c_y, c_y, RN_SPLIT_BOTH_AUTO|line_dir_flag );
                last_split_after_flag = RN_SPLIT_AUTO;
            }
        }
        if ( avoid_pb_inside ) {
            if ( avoid_pb_inside_just_toggled_on ) {
                avoid_pb_inside_just_toggled_on = false;
                // Previous added line may allow a break after, so dont prevent it
                flags = RN_GET_SPLIT_BEFORE(flags) | RN_SPLIT_AFTER_AVOID;
            }
            else {
                flags = RN_SPLIT_BOTH_AVOID;
            }
        }
        // Push vertical margin with active pb
        if ( vm_has_some ) { // avoid expensive call if not needed
            pushVerticalMargin(RN_GET_SPLIT_BEFORE(flags));
        }
        else if ( BLOCK_RENDERING(rend_flags, DO_NOT_CLEAR_OWN_FLOATS) && _floats.length()>0 ) {
            // but this has to be done if not done by pushVerticalMargin()
            resetFloatsLevelToTopLevel();
        }
        // Most often for content lines, lvtextfm.cpp's LVFormatter will
        // have already checked for float (via BlockFloatFootprint), so
        // avoid calling hasFloatRunningAtY() when not needed
        if ( !(flags & RN_SPLIT_BEFORE_AVOID) && hasFloatRunningAtY(c_y) )
            flags |= RN_SPLIT_BEFORE_AVOID;
        flags |= line_dir_flag;
        context.AddLine( c_y, c_y + height, flags );
        last_split_after_flag = RN_GET_SPLIT_AFTER(flags);
        if ( !is_padding )
            seen_content_since_page_split = true;
        moveDown( height );
        if ( vm_disabled ) // Re-enable it now that some real content has been seen
            enableVerticalMargin();
        if ( baseline_req != REQ_BASELINE_NOT_NEEDED ) {
            // We don't update last baseline when adding padding or when none is provided
            if ( !is_padding && baseline != NO_BASELINE_UPDATE ) {
                // https://www.w3.org/TR/CSS21/visudet.html#propdef-vertical-align
                //   "The baseline of an 'inline-table' is the baseline of the first row
                //    of the table.
                //    The baseline of an 'inline-block' is the baseline of its last line
                //    box in the normal flow, unless it has either no in-flow line boxes
                //    [or ...], in which case the baseline is the bottom margin edge."
                // Also see for some interesting sample snippet:
                // https://stackoverflow.com/questions/19352072/what-is-the-difference-between-inline-block-and-inline-table/56305302#56305302
                // A tricky thing with inline-table is that the baseline is different if
                // there is a table of if there's not
                // - if the first content is a table row, it should be the baseline of
                //   the row (which might not be the baseline of the first or last line
                //   of any table cell of that row).
                // - if the first content is not a table row (we can have inline-table
                //   containing no table-like elements), it is the real baseline of the
                //   first line in that content.
                // As the rendering table code does not use FlowState, we manage the
                // first case in getBaselineAbsoluteY() when we're done.
                if ( baseline_req == REQ_BASELINE_FOR_TABLE && baseline_set ) {
                    // inline-table: first baseline already met, it will stay the final baseline
                }
                else { // inline-block
                    // We update it from c_y which accounts for any vertical margins
                    // pushed // when adding this line:
                    baseline_y = c_y - height + baseline;
                    if ( !baseline_set)
                        baseline_set = true;
                }
            }
        }
        return c_y - start_c_y;
    }

    void addContentSpace( int height, int line_h, bool split_avoid_before,
                bool split_avoid_inside, bool split_avoid_after ) {
        // Push vertical margin with active pb
        if ( vm_has_some ) { // avoid expensive call if not needed
            pushVerticalMargin(split_avoid_before ? RN_SPLIT_AVOID : RN_SPLIT_AUTO);
        }
        else if ( BLOCK_RENDERING(rend_flags, DO_NOT_CLEAR_OWN_FLOATS) && _floats.length()>0 ) {
            // but this has to be done if not done by pushVerticalMargin()
            resetFloatsLevelToTopLevel();
        }
        addSpaceToContext( c_y, c_y + height, line_h, split_avoid_before, split_avoid_inside, split_avoid_after);
        last_split_after_flag = split_avoid_after ? RN_SPLIT_AVOID : RN_SPLIT_AUTO;
        if ( !seen_content_since_page_split ) {
            // Assume that if split_avoid_inside is not set, this space
            // is just padding/style_h. We want to prevent double
            // page splits inside such spaces.
            if ( split_avoid_inside ) {
                seen_content_since_page_split = true;
            }
        }
        moveDown( height );
        if ( vm_disabled ) // Re-enable it now that some real content has been seen
            enableVerticalMargin();
    }

    void disableVerticalMargin() {
        vm_disabled = true;
        // Reset everything so it's ready when re-enabled, and our various
        // tests act as if there's no vertical margin to handle.
        vm_has_some = false;
        vm_target_node = NULL;
        vm_target_avoid_pb_inside = false;
        vm_target_level = 0;
        vm_max_positive_margin = 0;
        vm_max_negative_margin = 0;
        vm_active_pb_flag = RN_SPLIT_AUTO;
        vm_back_usable_as_margin = 0;
    }
    void enableVerticalMargin() {
        vm_disabled = false;
        // Also drop any back margin added since disabling
        vm_back_usable_as_margin = 0;
    }
    void addVerticalMargin( ldomNode * node, int height, int pb_flag, bool is_top_margin=false ) {
        if ( vm_disabled ) { // ignore that margin, unless it asks for a page split
            if ( pb_flag != RN_SPLIT_ALWAYS ) {
                return;
            }
            enableVerticalMargin();
        }
        // printf("  adding vertical margin %d (%x top=%d)\n", height, pb_flag, is_top_margin);

        // When collapsing margins, the resulting margin is to be
        // applied outside the first element involved. So, remember
        // the first node we are given, that will later be shifted down
        // when some real content is added (padding/border or text).
        if ( is_top_margin ) { // new node
            if ( is_main_flow && !vm_target_node && vm_active_pb_flag == RN_SPLIT_ALWAYS ) {
                // We have only met bottom margins before, and one had "break-after: always":
                // push what we had till now
                pushVerticalMargin();
            }
            // If we get a new node with a same or lower level, we left our previous
            // node and it didn't have any content nor height: we can just let it
            // be where it was and continue with the new node, which will get the
            // accumulated margin.
            // (This is a wrong, but simpler to do. The right way would be to
            // grab down the previous empty nodes with the new final target node.)
            if ( !vm_target_node || level <= vm_target_level ) {
                vm_target_node = node;
                vm_target_level = level;
                // Remember avoid_pb_inside for the target node
                vm_target_avoid_pb_inside = avoid_pb_inside;
                if ( BLOCK_RENDERING(rend_flags, DO_NOT_CLEAR_OWN_FLOATS) && _floats.length()>0 ) {
                    // Reset level of all previous floats as they are
                    // correctly positionned and should not be moved
                    // if this (new) vm_target_node get some margin
                    // and is moved down.
                    resetFloatsLevelToTopLevel();
                }
            }
        }

        // We may combine multiple margins, each with its own flag, and we should
        // compute a global flag for the future single collapsed margin resulting
        // from them.
        // Quotes from https://www.w3.org/TR/CSS2/page.html#allowed-page-breaks:
        //   "Page breaks can occur at the following places: [...] 1. In the
        //   vertical margin between block-level boxes
        //   Rule A: Breaking at (1) is allowed only if the 'page-break-after' and
        //   'page-break-before' properties of all the elements generating boxes
        //   that meet at this margin allow it, which is when at least one of
        //   them has the value 'always', 'left', or 'right', or when all of them
        //   are 'auto'."
        // So, a page break "always" is stronger than "avoid" ("avoid" seems to be
        // just a way to not set "auto", so breaking the rule "when all of them
        // are 'auto'", but it is allowed when "at least one of them has the
        // value 'always'")
        // Note: if we wanted to avoid a page break between siblings H2 and H3,
        // we should use in our epub.css: H2 + H3 { page-break-before: avoid; }

        if ( pb_flag == RN_SPLIT_ALWAYS ) {
            // Avoid consecutive page split when no real content in between
            if ( BLOCK_RENDERING(rend_flags, ALLOW_PAGE_BREAK_WHEN_NO_CONTENT) || seen_content_since_page_split ) {
                if ( vm_active_pb_flag != RN_SPLIT_ALWAYS ) {
                    // First break-before or break-after:always seen.
                    // Forget any previously seen margin, as it would
                    // collapse at end of previous page, but we won't
                    // add a line for it.
                    vm_max_positive_margin = 0;
                    vm_max_negative_margin = 0;
                    // Also forget height taken while clearing floats
                    vm_back_usable_as_margin = 0;
                    // Also don't have the one provided added, if it's some
                    // bottom margin that comes with break-after:always.
                    if ( !is_top_margin )
                        height = 0;
                }
                vm_active_pb_flag = RN_SPLIT_ALWAYS;
            }
        }
        else if ( pb_flag == RN_SPLIT_AVOID ) {
            if ( vm_active_pb_flag != RN_SPLIT_ALWAYS )
                vm_active_pb_flag = RN_SPLIT_AVOID;
        }
        else { // auto
            // Keep current vm_active_pb_flag (RN_SPLIT_AUTO if nothing else seen).
            // But:
            // Rule B: "However, if all of them are 'auto' and a common ancestor of
            // all the elements has a 'page-break-inside' value of 'avoid',
            // then breaking here is not allowed"
            // As it is the target node that will get the margin, if avoid_pb_inside
            // was set for that node, we should avoid a split in that margin.
            if ( vm_target_avoid_pb_inside )
                vm_active_pb_flag = RN_SPLIT_AVOID;
        }

        if ( vm_active_pb_flag == RN_SPLIT_ALWAYS ) {
            // Also from https://www.w3.org/TR/CSS2/page.html#allowed-page-breaks:
            //   "When a forced page break occurs here, the used value of the relevant
            //    'margin-bottom' property is set to '0'; the relevant 'margin-top'
            //    [...] applies (i.e., is not set to '0') after a forced page break."
            // Not certain what should be the "relevant" margin-top when collapsing
            // multiple margins, but as we are going to push the collapsed margin
            // with RN_SPLIT_BEFORE_ALWAYS, it feels any bottom margin should be
            // ignored (so a large previous bottom margin does not make a larger
            // top margin on this new page).
            if ( !is_top_margin )
                height = 0; // Make that bottom margin be empty
        }

        // The collapsed margin will be the max of the positive ones + the min
        // of the negative ones (the most negative one)
        if (height > 0 && height > vm_max_positive_margin)
            vm_max_positive_margin = height;
        if (height < 0 && height < vm_max_negative_margin)
            vm_max_negative_margin = height;
        vm_has_some = true;

        if ( !is_top_margin && vm_target_node && node == vm_target_node && vm_active_pb_flag == RN_SPLIT_ALWAYS ) {
            // We leave our target node, and some pb always was set by it
            // or its children, but with no content nor height.
            // It looks like we should ensure the pb always, possibly making
            // an empty page if upcoming node has a pb always too (that's
            // what Prince seems to do).
            pushVerticalMargin();
        }
        if ( !BLOCK_RENDERING(rend_flags, COLLAPSE_VERTICAL_MARGINS) ) {
            // If we don't want collapsing margins, just push this added
            // unit of margin (not really checked if we get exactly what
            // we would get when in legacy rendering mode).
            pushVerticalMargin();
        }
    }
    int getCurrentVerticalMargin() {
        // The collapsed margin is the max of the positive ones + the min
        // of the negative ones (the most negative one)
        int margin = vm_max_positive_margin + vm_max_negative_margin;
        if ( is_main_flow && margin < 0 ) {
            // Unless requested, we don't allow negative margins in the
            // main flow, as it could have us moving backward and cause
            // issues with page splitting and text selection.
            if ( !BLOCK_RENDERING(rend_flags, ALLOW_NEGATIVE_COLLAPSED_MARGINS) ) {
                margin = 0;
            }
        }
        if ( margin > 0 && vm_back_usable_as_margin > 0 ) {
            if ( margin > vm_back_usable_as_margin )
                margin -= vm_back_usable_as_margin;
            else
                margin = 0;
        }
        // Even if below, we can have some margin lines discarded in page mode,
        // super huge bogus margins could still be shown in scroll mode (and footer
        // in scroll mode could still show "page 1 of 2" while we scroll this margin
        // height that spans many many screen heights).
        // To avoid any confusion, limit any margin to be the page height
        if ( margin > page_height )
            margin = page_height;
        return margin;
    }
    void pushVerticalMargin( int next_split_before_flag=RN_SPLIT_AUTO ) {
        if ( vm_disabled )
            return;
        int line_dir_flag = direction == REND_DIRECTION_RTL ? RN_LINE_IS_RTL : 0;

        // If this is a node at level 0 (root node, floatBox, inlineBox) or
        // level 1 (body, floatBox or inlineBox single child), drop any back
        // margin as we should get its full bottom margin (we should be called
        // twice: for the top margin, and there is not back margin yet - and
        // for the bottom margin, where there may be).
        if (level <= (is_main_flow ? 0 : 1))
            vm_back_usable_as_margin = 0;
        // Compute the single margin to add along our flow y and to pages context.
        int margin = getCurrentVerticalMargin();
        vm_back_usable_as_margin = 0;
        // printf("pushing vertical margin %d (%x %d)\n", margin, vm_target_node, vm_target_level);

        // Note: below, we allow some margin (previous page margin) to be discarded
        // if it can not fit on the previous page and is pushed on next page. This is
        // usually fine when we have white backgrounds, but may show some white holes
        // at bottom of page before a split (where a non-fully discarded margin would
        // otherwise allow some now-white background to be painted on the whole page).

        if (is_main_flow && margin < 0) { // can only happen if ALLOW_NEGATIVE_COLLAPSED_MARGINS
            // We're moving backward and don't know what was before and what's
            // coming next. Add an empty line to avoid a split there.
            context.AddLine( c_y, c_y, RN_SPLIT_BOTH_AVOID|line_dir_flag );
        }
        else if (is_main_flow) {
            // When this is called, whether we have some positive resulting vertical
            // margin or not (zero), we need to context.AddLine() a possibly empty
            // line with the right pb flags according to what we gathered while
            // vertical margins were added.
            int flags;
            bool emit_empty = true;
            if ( vm_active_pb_flag == RN_SPLIT_ALWAYS ) {
                // Forced page break
                // Quotes from https://www.w3.org/TR/CSS2/page.html#allowed-page-breaks:
                //   "In the normal flow, page breaks can occur at the following places:
                //     1) In the vertical margin between block-level boxes. [...]
                //     When a forced page break occurs here, the used value of the relevant
                //     'margin-bottom' property is set to '0'; the relevant 'margin-top'
                //     [...] applies (i.e., is not set to '0') after a forced page break."
                if ( vm_target_node ) {
                    // As top margin should be on the next page, split before the margin
                    // (Note that Prince does not do this: the top margin of an element
                    // with "page-break-before: always" is discarded.)
                    flags = RN_SPLIT_BEFORE_ALWAYS | RN_SPLIT_AFTER_AVOID;
                }
                else { // no target node: only bottom margin with some break-after
                    // If it does not fit on previous page, and is pushed on next page,
                    // discard it.
                    // (This fixes weasyprint/margin-collapse-160.htm "Margins should not
                    // propagate into the next page", which has a margin-bottom: 1000em
                    // that would make 37 pages - but these 1000em would still be ensured
                    // in scroll mode if we were not limiting a margin to max page_height.)
                    flags = RN_SPLIT_BEFORE_AUTO | RN_SPLIT_AFTER_ALWAYS | RN_SPLIT_DISCARD_AT_START;
                }
                // Note: floats must have been cleared before calling us if a page-break
                // has to happen.
                // (We could have cleared all floats here, so they are not truncated and
                // split on the new page, but it might be too late: uncleared floats
                // would be accounted in a BlockFloatFootprint, which would have some
                // impact on the content lines; these lines would be added, and could
                // call pushVerticalMargin(): we would here clear floats, but the
                // lines would have already been sized with the footprint, which would
                // not be there if we clear the floats here, and we would get holes in
                // the text.)
            }
            else if (vm_active_pb_flag == RN_SPLIT_AVOID ) { // obvious
                flags = RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID;
            }
            else {
                // Allow an unforced page break, keeping the margin on previous page
                //
                // Quotes from https://www.w3.org/TR/CSS2/page.html#allowed-page-breaks:
                //   "In the normal flow, page breaks can occur at the following places:
                //     3) Between the content edge of a block container box and the
                //     outer edges of its child content (margin edges of block-level
                //     children or line box edges for inline-level children) if there
                //     is a (non-zero) gap between them"
                //  Which means it is never allowed between a parent's top and its
                //  first child's top - and between a last child's bottom and its
                //  parent's bottom (except when some CSS "height:" is into play
                //  (which we deal with elsewhere), or when CSS "position: relative"
                //  is used, that we don't support).
                //
                //    "1) In the vertical margin between block-level boxes. When an
                //     unforced page break occurs here, the used values of the relevant
                //     'margin-top' and 'margin-bottom' properties are set to '0'.
                //     When a forced page break occurs here, the used value of the relevant
                //     'margin-bottom' property is set to '0'; the relevant 'margin-top'
                //     used value may either be set to '0' or retained."
                //  Which means it is only allowed between a block's bottom and its
                //  following sibling's top.
                //
                // We can ensure both of the above cases by just adding a line for
                // the margin (even if empty) forwarding the flags of previous and
                // next lines.
                // As everywhere, we push top paddings (and the first line of text) with
                // RN_SPLIT_BEFORE_AUTO and RN_SPLIT_AFTER_AVOID - and bottom paddings
                // (and the last line of text) with RN_SPLIT_BEFORE_AVOID and
                // RN_SPLIT_AFTER_AUTO:
                // - if we are pushing the margin between an upper block and a
                //   child we'll be between outer padding_top with RN_SPLIT_AFTER_AVOID
                //   and inner padding_top with RN_SPLIT_BEFORE_AUTO: by adding our
                //   margin with RN_SPLIT_BEFORE_AUTO and RN_SPLIT_AFTER_AVOID, we
                //   forbid a page split at this place
                // - if we are pushing the margin between same-level sibling blocks: the
                //   first block padding_bottom has RN_SPLIT_AFTER_AUTO and the next
                //   block padding_top has RN_SPLIT_BEFORE_AUTO: by using _AUTO on both
                //   sides of our margin, a split is then allowed.
                //
                if ( last_split_after_flag == RN_SPLIT_AUTO && next_split_before_flag == RN_SPLIT_AUTO )  {
                    // Allow a split on both sides.
                    // Note: if out of break-inside avoid, addContentLine() will have added
                    // an empty line to get rid of any previous SPLIT_AFTER_AVOID, so this margin
                    // line can be either at end of previous page, or at start of next page, and
                    // it will not drag the previous real line with it on next page.
                    // Allow it to be discarded if it happens to be put at start of next page.
                    flags = RN_SPLIT_BOTH_AUTO | RN_SPLIT_DISCARD_AT_START;
                }
                else {
                    // Just forward the flags (but not ALWAYS, which we should not do again)
                    int after_flag = last_split_after_flag & ~RN_SPLIT_ALWAYS;
                    flags = next_split_before_flag | (after_flag << RN_SPLIT_AFTER);
                }
                // No need to emit these empty lines when split auto and there is no margin
                emit_empty = false;
            }
            if ( margin > 0 || emit_empty ) {
                if ( c_y == 0 ) {
                    // First margin with no content yet. Just avoid a split
                    // with futur content, so that if next content is taller
                    // than page height, we don't get an empty first page.
                    flags &= ~RN_SPLIT_AFTER_ALWAYS;
                    flags |= RN_SPLIT_AFTER_AVOID;
                }
                if ( hasFloatRunningAtY(c_y, margin) ) {
                    // Don't discard margin, or we would discard some part of a float
                    flags &= ~RN_SPLIT_DISCARD_AT_START;
                }
                if ( hasFloatRunningAtY(c_y) ) {
                    // Avoid a split
                    flags &= ~RN_SPLIT_BEFORE_ALWAYS;
                    flags |= RN_SPLIT_BEFORE_AVOID;
                }
                else if ( RN_GET_SPLIT_BEFORE(flags) == RN_SPLIT_ALWAYS && last_split_after_flag == RN_SPLIT_AVOID ) {
                    // If last line ended with RN_SPLIT_AVOID, it would
                    // prevent our RN_SPLIT_ALWAYS to have effect.
                    // It seems that per-specs, the SPLIT_ALWAYS should win.
                    // So, kill the SPLIT_AVOID with an empty line.
                    context.AddLine( c_y, c_y, RN_SPLIT_BOTH_AUTO|line_dir_flag );
                    // Note: keeping the RN_SPLIT_AVOID could help avoiding
                    // consecutive page splits in some normal cases (we send
                    // SPLIT_BEFORE_ALWAYS with SPLIT_AFTER_AVOID, and top
                    // paddings come with SPLIT_AFTER_AVOID). We could
                    // use !seen_content_since_page_split here to not
                    // send this empty line, but that's somehow handled
                    // above where we just not use RN_SPLIT_ALWAYS if it
                    // hasn't been reset.
                }
                flags |= line_dir_flag;
                context.AddLine( c_y, c_y + margin, flags );
                // Note: we don't use AddSpace, a margin does not have to be arbitrarily
                // splitted, RN_SPLIT_DISCARD_AT_START ensures it does not continue
                // on next page.
                // But, if there's a float at start and  another at end, we could think
                // about trying to find a split point inside the margin (if 2 floats
                // are meeting there).
                last_split_after_flag = RN_GET_SPLIT_AFTER(flags);
                if ( (RN_GET_SPLIT_BEFORE(flags) == RN_SPLIT_ALWAYS) || (RN_GET_SPLIT_AFTER(flags) == RN_SPLIT_ALWAYS) )
                    // Reset this, so we can ignore some upcoming duplicate SPLIT_ALWAYS
                    seen_content_since_page_split = false;
            }
        }
        // No need to do any more stuff if margin == 0
        if ( margin != 0 ) {
            if (vm_target_node) {
                // As we are adding the margin above vm_target_node, so actually pushing
                // this node down, all its child blocks will also be moved down.
                // We need to correct all the l_y of all the sublevels since vm_target_node
                // (not including vm_target_level, which contains/is outside vm_target_node):
                if (level > vm_target_level) { // current one (not yet in _shifts)
                    l_y += margin;
                    in_y_min += margin;
                    in_y_max += margin;
                }
                for (int i=level-1; i>vm_target_level; i--) {
                    _shifts[i]->l_y += margin;
                    _shifts[i]->in_y_min += margin;
                    _shifts[i]->in_y_max += margin;
                }
                // We also need to update sub-levels' floats' absolute or
                // relative coordinates
                for (int i=0; i<_floats.length(); i++) {
                    BlockFloat * flt = _floats[i];
                    if ( flt->level > vm_target_level ) {
                        if ( flt->final_pos ) {
                            // Float already absolutely positionned (block float):
                            // adjust its relative y to its container's top to
                            // counteract the margin shift
                            RenderRectAccessor fmt( flt->node );
                            fmt.setY( fmt.getY() - margin );
                        }
                        else {
                            // Float not absolutely positionned (forwarded embedded float):
                            // update its absolute coordinates as its container will be
                            // moved by this margin, so will its floats.
                            flt->top += margin;
                            flt->bottom += margin;
                        }
                    }
                }
                // Update the vm_target_node y (its relative y in its container)
                RenderRectAccessor fmt( vm_target_node );
                    #ifdef DEBUG_BLOCK_RENDERING
                    if (isMainFlow()) {
                        printf("pushedVM %d => c_y=%d>%d target=%s y=%d>%d\n", margin, c_y, c_y+margin,
                            UnicodeToLocal(ldomXPointer(vm_target_node, 0).toString()).c_str(),
                            fmt.getY(), fmt.getY()+margin);
                    }
                    #endif
                fmt.setY( fmt.getY() + margin );
                fmt.push();
                // It feels we don't need to update margin_top in the ->style of this
                // node to its used value (which would be complicated), as its only
                // other use (apart from here) is in:
                // - ldomNode::getSurroundingAddedHeight() : where we'll then get a
                //   little smaller that screen height for full screen images
                // - ldomNode::elementFromPoint() : we tweaked it so it does not
                //   look at margins in enhanced rendering mode.
            }
            #ifdef DEBUG_BLOCK_RENDERING
                else { if (isMainFlow()) printf("pushedVM %d => c_y=%d>%d no target node\n", margin, c_y, c_y+margin); }
            #endif
            // If no target node (bottom margin), moveDown(margin) is all there
            // is to do.
            moveDown( margin );
            // (We have to do this moveDown() here, after the above floats'
            // coordinates updates, otherwise these floats might be removed
            // by moveDown() as it passes them.)
        }
        #ifdef DEBUG_BLOCK_RENDERING
            else { if (isMainFlow()) printf("pushedVM 0 => c_y=%d\n", c_y); }
        #endif

        // Reset everything, as we used it all, and nothing should have
        // any impact on the next vertical margin.
        // (No need to update vm_target_level which makes no
        // sense without a vm_target_node, and we wouldn't
        // know which value to reset to.)
        vm_target_node = NULL;
        vm_target_avoid_pb_inside = false;
        vm_max_positive_margin = 0;
        vm_max_negative_margin = 0;
        vm_active_pb_flag = RN_SPLIT_AUTO;
        vm_has_some = false;
        if ( BLOCK_RENDERING(rend_flags, DO_NOT_CLEAR_OWN_FLOATS) && _floats.length()>0 ) {
            // We did not clear floats, so we may meet them again when
            // pushing some upcoming vertical margin. But we don't want
            // to have them shifted down, as their positions are now fixed.
            // Just reset their level to be the top flow clear level, so
            // we ignore them in next pushVerticalMargin() calls.
            resetFloatsLevelToTopLevel();
        }
    }
    void resetFloatsLevelToTopLevel() {
        // To be called only if DO_NOT_CLEAR_OWN_FLOATS (moved
        // as a function because we should call it too when we
        // can skip pushVerticalMargin()).
        // As blocks are not clearing their own floats, and
        // we may meet previous siblings' floats, we don't
        // want to have them shifted again by some later
        // vertical margin.
        // So, assign them to a lower level, the one that
        // should have them cleared when exiting the flow
        // (to account them in the flow height):
        //   is_main_flow ? 0 : 1, plus 1
        for (int i=0; i<_floats.length(); i++) {
            BlockFloat * flt = _floats[i];
            flt->level = top_clear_level;
        }
    }

    // Enter/leave a block level: backup/restore some of this FlowState
    // fields, and do some housekeeping.
    void newBlockLevel( int width, int d_left, bool avoid_pb, int dir, lInt32 langNodeIdx ) {
        // Don't new/delete to avoid too many malloc/free, keep and re-use/reset
        // the ones already created
        if ( _shifts.length() <= level ) {
            _shifts.push( new BlockShift( direction, lang_node_idx, x_min, x_max, l_y, in_y_min, in_y_max, avoid_pb_inside ) );
        }
        else {
            _shifts[level]->reset( direction, lang_node_idx, x_min, x_max, l_y, in_y_min, in_y_max, avoid_pb_inside );
        }
        direction = dir;
        if (langNodeIdx != -1)
            lang_node_idx = langNodeIdx;
        x_min += d_left;
        x_max = x_min + width;
        l_y = c_y;
        in_y_min = c_y;
        in_y_max = c_y;
        level++;
        // Don't disable any upper avoid_pb_inside
        if ( avoid_pb ) {
            if ( !avoid_pb_inside)
                avoid_pb_inside_just_toggled_on = true;
            avoid_pb_inside = true;
        }
    }
    int leaveBlockLevel( int & top_overflow, int & bottom_overflow ) {
        int start_c_y = l_y;
        int last_c_y = c_y;
        top_overflow = in_y_min < start_c_y ? start_c_y - in_y_min : 0;  // positive value
        bottom_overflow = in_y_max > last_c_y ? in_y_max - last_c_y : 0; // positive value
        BlockShift * prev = _shifts[level-1];
        direction = prev->direction;
        lang_node_idx = prev->lang_node_idx;
        x_min = prev->x_min;
        x_max = prev->x_max;
        l_y = prev->l_y;
        in_y_min = in_y_min < prev->in_y_min ? in_y_min : prev->in_y_min; // keep sublevel's one if smaller
        in_y_max = in_y_max > prev->in_y_max ? in_y_max : prev->in_y_max; // keep sublevel's one if larger
        if ( prev->avoid_pb_inside != avoid_pb_inside )
            avoid_pb_inside_just_toggled_off = true;
        avoid_pb_inside = prev->avoid_pb_inside;
        level--;
        int height; // height of the block level we are leaving, that we should return
        if ( BLOCK_RENDERING(rend_flags, DO_NOT_CLEAR_OWN_FLOATS) && level > (is_main_flow ? 0 : 1) ) {
            // If requested, don't clear own floats.
            // But we need to for level 0 (root node, floatBox) and
            // level 1 (body, floating child) to get them accounted
            // in the document or float height.
            height = last_c_y - start_c_y;
        }
        else {
            // Otherwise, clear the floats that were added in this shift level
            int new_c_y = c_y;
            for (int i=_floats.length()-1; i>=0; i--) {
                BlockFloat * flt = _floats[i];
                if (flt->level > level) {
                    if (flt->bottom > new_c_y) {
                        new_c_y = flt->bottom; // move to the bottom of this float
                    }
                    // We can't remove/delete them yet, we need them
                    // to be here for addSpaceToContext() to ensure
                    // no page split over them
                }
            }
            // addSpaceToContext() will take care of avoiding page split
            // where some (non-cleared) floats are still running.
            addSpaceToContext(last_c_y, new_c_y, 1, false, false, false);
            int dy = new_c_y - last_c_y;
            moveDown( dy );
            if ( bottom_overflow > dy )
                bottom_overflow = bottom_overflow - dy;
            else
                bottom_overflow = 0;
            if (dy > 0)
                seen_content_since_page_split = true;
            // The vertical space moved to clear floats can be
            // deduced from upcoming pushed/collapsed vertical margin.
            vm_back_usable_as_margin += dy;
            // moveDown() should have already cleared out past floats,
            // but just ensure there's none left past the level we left
            for (int i=_floats.length()-1; i>=0; i--) {
                BlockFloat * flt = _floats[i];
                if (flt->level > level) {
                    _floats.remove(i);
                    delete flt;
                }
            }
            height = new_c_y - start_c_y;
        }
        return height;
    }

    bool moveDown( int dy ) {
        // Will return true if we had running floats *before* moving
        int prev_c_y = c_y;
        if (dy > 0) { // moving forward
            c_y += dy;
            if ( c_y > in_y_max ) {
                // update current level max seen y (not really needed as it is
                // checked on leaveBlockLeve, but for symetry)
                in_y_max = c_y;
            }
        }
        else if (dy < 0) { // moving backward
            // Only allowed if we are not the main flow (eg: a float), or if
            // explicitely requested (as it may cause issues with page splitting
            // and text/links selection)
            if ( !is_main_flow || BLOCK_RENDERING(rend_flags, ALLOW_NEGATIVE_COLLAPSED_MARGINS) ) {
                c_y += dy;
                if ( c_y < in_y_min ) {
                    // update current level min seen y (in case negative margins
                    // moved past level origin y)
                    in_y_min = c_y;
                }
            }
            // Nothing else to do, no float cleaning as we did not move forward
            return true; // pretend we have floats running
        }
        // Clear past floats, and see if we have some running
        bool had_floats_running = false;
        for (int i=_floats.length()-1; i>=0; i--) {
            BlockFloat * flt = _floats[i];
            if ( flt->top < prev_c_y && flt->bottom > prev_c_y ) {
                had_floats_running = true;
                // If a float has started strictly before prev_c_y, it was
                // running (if it started on prev_c_y, it was a new one
                // and should not prevent a page split at prev_c_y)
            }
            if (flt->bottom <= c_y) {
                // This float is past, we shouldn't have to worry
                // about it anymore
                _floats.remove(i);
                delete flt;
            }
        }
        return had_floats_running;
    }

    void clearFloats( css_clear_t clear ) {
        if (clear <= css_c_none)
            return;
        int cleared_y = c_y;
        for (int i=0; i<_floats.length(); i++) {
            BlockFloat * flt = _floats[i];
            if ( ( clear == css_c_both ) || ( clear == css_c_left && !flt->is_right ) ||
                                            ( clear == css_c_right && flt->is_right ) ) {
                if (flt->bottom > cleared_y)
                    cleared_y = flt->bottom;
            }
        }
        int dy = cleared_y - c_y;
        // Add the vertical space skipped to the page splitting context.
        // addSpaceToContext() will take care of avoiding page split
        // where some (non-cleared) floats are still running.
        addSpaceToContext(c_y, cleared_y, 1, false, false, false);
        if (dy > 0) {
            moveDown( dy ); // will delete past floats
            // The vertical space moved to clear floats can be
            // deduced from upcoming pushed/collapsed vertical margin.
            vm_back_usable_as_margin += dy;
            seen_content_since_page_split = true;
        }
        if ( vm_disabled ) {
            // Re-enable vertical margin (any clear, even if not "both",
            // mean we could have moved and we are not tied to stay
            // aligned with 0-height floats).
            // Not sure about this, but it allows working around the issue
            // with 0-height float containers that we disable margin with, by
            // just (with styletweaks) setting "clear: both" on their followup.
            enableVerticalMargin();
        }
    }

    // Floats positionning helpers. These work in absolute coordinates (relative
    // to flow state initial top)
    // Returns min y for next float
    int getNextFloatMinY(css_clear_t clear) {
        int y = c_y; // current line y
        for (int i=0; i<_floats.length(); i++) {
            BlockFloat * flt = _floats[i];
            // A later float should never be positionned above an earlier float
            if ( flt->top > y )
                y = flt->top;
            if ( clear > css_c_none) {
                if ( (clear == css_c_both) || (clear == css_c_left && !flt->is_right)
                                           || (clear == css_c_right && flt->is_right) ) {
                    if (flt->bottom > y)
                        y = flt->bottom;
                }
            }
        }
        return y;
    }
    // Returns next y after start_y where required_width is available over required_height
    // Also set offset_x to the x where that width is available
    int getYWithAvailableWidth(int start_y, int required_width, int required_height,
                int min_x, int max_x, int & offset_x, bool get_right_offset_x=false) {
        if (_floats.length() == 0) { // fast path
            // If no floats, width is available at start_y
            if (get_right_offset_x) {
                offset_x = max_x - required_width;
                if (offset_x < min_x) // overflow
                    offset_x = min_x;
            }
            else {
                offset_x = min_x;
            }
            return start_y;
        }
        bool fit = false;
        int fit_left_x = min_x;
        int fit_right_x = max_x;
        int fit_top_y = start_y;
        int fit_bottom_y = start_y;
        int y = start_y;
        int floats_max_bottom = start_y;
        while (true) {
            int left_x = min_x;
            int right_x = max_x;
            for (int i=0; i<_floats.length(); i++) {
                BlockFloat * flt = _floats[i];
                if (flt->bottom > floats_max_bottom)
                    floats_max_bottom = flt->bottom;
                if (flt->top <= y && flt->bottom > y) {
                    if (flt->is_right) {
                        if (flt->left < right_x)
                            right_x = flt->left;
                    }
                    else {
                        if (flt->right > left_x)
                            left_x = flt->right;
                    }
                }
            }
            if (right_x - left_x < required_width) { // doesn't fit
                fit = false;
            }
            else { // required_width fits at this y
                if (!fit) { // first y that fit
                    fit = true;
                    fit_top_y = y;
                    fit_bottom_y = y;
                    fit_left_x = left_x;
                    fit_right_x = right_x;
                }
                else { // we already fitted on previous y
                    // Adjust to previous boundaries
                    if (left_x > fit_left_x)
                        fit_left_x = left_x;
                    if (right_x < fit_right_x)
                        fit_right_x = right_x;
                    if (fit_right_x - fit_left_x < required_width) { // we don't fit anymore
                        fit = false;
                    }
                    else { // we continue fitting
                        fit_bottom_y = y;
                    }
                }
            }
            if ( fit && (fit_bottom_y - fit_top_y >= required_height) )
                break; // found & done
            y += 1;
            if ( y >= floats_max_bottom )
                break; // no more floats
        }
        if (!fit) {
            // If we left the loop non fitting, because of no or no more floats,
            // adjust to provided boundaries
            fit_left_x = min_x;
            fit_right_x = max_x;
            fit_top_y = y;
        }
        if (get_right_offset_x) {
            offset_x = fit_right_x - required_width;
            if (offset_x < fit_left_x) // overflow
                offset_x = fit_left_x;
        }
        else {
            offset_x = fit_left_x; // We don't mind it it overflows max-min
        }
        return fit_top_y;
    }
    void addFloat( ldomNode * node, css_clear_t clear, bool is_right, int top_margin ) {
        RenderRectAccessor fmt( node );
        int width = fmt.getWidth();
        int height = fmt.getHeight();
        int x = fmt.getX();   // a floatBox has no margin and no padding, but these
        int y = fmt.getY();   // x/y carries the container's padding left/top
        // printf("  block addFloat w=%d h=%d x=%d y=%d\n", width, height, x, y);
        int shift_x = 0;
        int shift_y = 0;

        // Get this float position in our flow state absolute coordinates
        int pos_y = c_y + top_margin;     // y with collapsed vertical margin included
        int fy = getNextFloatMinY(clear); // min_y depending on other floats
        if (pos_y > fy)
            fy = pos_y;
        int fx = 0;
        fy = getYWithAvailableWidth(fy, width, height, x_min, x_max, fx, is_right);
        _floats.push( new BlockFloat( fx, fy, fx + width, fy + height, is_right, level, true, node) );

        // Get relative coordinates to current container top
        shift_x = fx - x_min;
        shift_y = fy - l_y;
        // Set the float relative coordinate in its container
        fmt.setX(x + shift_x);
        fmt.setY(y + shift_y);
        if (is_right)
            RENDER_RECT_SET_FLAG(fmt, FLOATBOX_IS_RIGHT);
        else
            RENDER_RECT_UNSET_FLAG(fmt, FLOATBOX_IS_RIGHT);
        // printf("  block addFloat => %d %d > x=%d y=%d\n", shift_x, shift_y, fmt.getX(), fmt.getY());

        // If this float overflows the current in_y_min/max, update them
        if ( fy - fmt.getTopOverflow() < in_y_min )
            in_y_min = fy - fmt.getTopOverflow();
        if ( fy + height + fmt.getBottomOverflow() > in_y_max )
            in_y_max = fy + height + fmt.getBottomOverflow();
    }

    void addPositionnedFloat( int rel_x, int rel_y, int width, int height, int is_right, ldomNode * node ) {
        int fx = x_min + rel_x;
        // Where addPositionnedFloat is used (rendering erm_final), c_y has not
        // yet been updated, so it is still the base for rel_y.
        int fy = c_y + rel_y;
        // These embedded floats are kind of in a sublevel of current level
        // (even if we didn't create a sublevel), let's add them with level+1
        // so they get correctly shifted when vertical margins collapse
        // in an outer level from the final node they come from.
        _floats.push( new BlockFloat( fx, fy, fx + width, fy + height, is_right, level+1, false, node ) );

        // No need to update this level in_y_min/max with this float,
        // as it belongs to some erm_final block that will itself
        // carry its floats' own overflows, and forward them to
        // the current level with next methods.
    }
    // For erm_final nodes to forward their overflow to current level
    void updateCurrentLevelTopOverflow(int top_overflow) {
        if ( top_overflow <= 0 )
            return;
        int y = c_y - top_overflow;
        if ( y < in_y_min )
            in_y_min = y;
    }
    void updateCurrentLevelBottomOverflow(int bottom_overflow) {
        if ( bottom_overflow <= 0 )
            return;
        int y = c_y + bottom_overflow;
        if ( y > in_y_max )
            in_y_max = y;
    }

    BlockFloatFootprint getFloatFootprint(ldomNode * node, int d_left, int d_right, int d_top ) {
        // Returns the footprint of current floats over a final block
        // to be laid out at current c_y (+d_top).
        // This footprint will be a set of floats to represent outer
        // floats possibly having some impact over the final block
        // about to be formatted.
        // These floats can be either:
        // - real floats rectangles, when they are no more than 5
        //   and ALLOW_EXACT_FLOATS_FOOTPRINTS is enabled
        // - or "fake" floats ("footprints") embodying all floats
        //   in 2 rectangles (one at top left, one at top right),
        //   and 2 empty floats to represent lower outer floats not
        //   intersecting with this final block, but whose y sets
        //   the minimal y for the possible upcoming embedded floats.
        //
        // Why at most "5" real floats?
        // Because I initially went with the "fake" floats solution,
        // because otherwise, storing references (per final block) to
        // a variable number of other floatBox nodes would need another
        // kind of crengine cache, and that looked complicated...
        // This "fake" floats way is quite limited, making holes in
        // the text, preventing the "staircase" effect of differently
        // sized floats.
        // Very later, I realized that in the fields I added to
        // RenderRectAccessor (extra1...extra5) to store these fake
        // floats rectangles, I could store in them the dataIndex of
        // the real floatBoxes node (which I can if they are no more
        // than 5), so we can fetch their real positions and dimensions
        // each time a final block is to be (re-)formatted, to allow
        // for a nicer layout of text around these (at most 5) floats.

        // We need erm_final at level 1 (body, floatBox or inlineBox child)
        // to clear their own floats, to get them accounted in the document,
        // float, or inlineBox height, so they are fully contained in it and
        // don't overflow. (Level 0 can't be erm_final).
        bool no_clear_own_floats = (level > 1) && BLOCK_RENDERING(rend_flags, DO_NOT_CLEAR_OWN_FLOATS);
        BlockFloatFootprint footprint = BlockFloatFootprint( this, d_left, d_top, no_clear_own_floats);
        if (_floats.length() == 0) // zero footprint if no float
            return footprint;
        int top_y = c_y + d_top;
        int left_x = x_min + d_left;
        int right_x = x_max - d_right;
        int final_width = right_x - left_x;
        // Absolute coordinates of this box top left and top right
        int flprint_left_x = left_x;
        int flprint_left_y = top_y;
        int flprint_right_x = right_x;
        int flprint_right_y = top_y;
        // Bottomest top of all our current floats (needed to know the absolute
        // minimal y at which next left or right float can be positionned)
        int flprint_left_min_y = top_y;
        int flprint_right_min_y = top_y;
        // Extend them to include any part of floats that overlap it.
        // We can store at max 5 ldomNode IDs in a RenderRectAccessor
        // extra1..extra5 fields. If we meet more than that, we
        // will fall back to footprints.
        int floats_involved = 0;
        // printf("left_x = x_min %d + d_left %d = %d  top_y=%d\n", x_min, d_left, left_x, top_y);
        for (int i=0; i<_floats.length(); i++) {
            BlockFloat * flt = _floats[i];
            if ( BLOCK_RENDERING(rend_flags, ALLOW_EXACT_FLOATS_FOOTPRINTS) ) {
                // Ignore floats already passed by and possibly not yet removed
                if (flt->bottom > top_y) {
                    if (floats_involved < 5) { // at most 5 slots
                        // Do the following even if we end up seeing more
                        // than 5 floats and not using all that.
                        // Store their dataIndex directly in footprint
                        footprint.floatIds[floats_involved] = flt->node->getDataIndex();
                        // printf("  flt #%d x %d y %d\n", i, flt->left, flt->top);

                        // Compute the transferable floats as it is less expensive
                        // to do now than using generateEmbeddedFloatsFromFloatIds().
                        // Have them clip'ed to our top and width (seems to work
                        // without clipping, but just to be sure as the lvtextfm.cpp
                        // code was made with assuming rect are fully contained
                        // in its own working area).
                        int x0 = flt->left - left_x;
                        if ( x0 < 0 )
                            x0 = 0;
                        else if ( x0 > final_width )
                            x0 = final_width;
                        int x1 = flt->right - left_x;
                        if ( x1 < 0 )
                            x1 = 0;
                        else if ( x1 > final_width )
                            x1 = final_width;
                        int y0 = flt->top > top_y ? flt->top - top_y : 0;
                        int y1 = flt->bottom - top_y;
                        footprint.floats[floats_involved][0] = x0;      // x
                        footprint.floats[floats_involved][1] = y0;      // y
                        footprint.floats[floats_involved][2] = x1 - x0; // width
                        footprint.floats[floats_involved][3] = y1 - y0; // height
                        footprint.floats[floats_involved][4] = flt->is_right;
                    }
                    floats_involved++;
                }
            }
            // Compute the other fields even if we end up not using them
            if (flt->is_right) {
                if ( flt->left < flprint_right_x ) {
                    flprint_right_x = flt->left;
                }
                if ( flt->bottom > flprint_right_y ) {
                    flprint_right_y = flt->bottom;
                }
                if ( flt->top > flprint_right_min_y ) {
                    flprint_right_min_y = flt->top;
                }
            }
            else {
                if ( flt->right > flprint_left_x ) {
                    flprint_left_x = flt->right;
                }
                if ( flt->bottom > flprint_left_y ) {
                    flprint_left_y = flt->bottom;
                }
                if ( flt->top > flprint_left_min_y ) {
                    flprint_left_min_y = flt->top;
                }
            }
        }
        if ( floats_involved > 0 && floats_involved <= 5) {
            // We can use floatIds
            footprint.use_floatIds = true;
            footprint.nb_floatIds = floats_involved;
            // No need to call generateEmbeddedFloatsFromFloatIds() as we
            // already computed them above.
            /* Uncomment for checking reproducible results (here and below)
               footprint.generateEmbeddedFloatsFromFloatIds( node, final_width );
            */
            footprint.floats_cnt = floats_involved;
        }
        else {
            // In case we met only past floats not yet removed, that made
            // no impact on flprint_*, all this will result in zero-values
            // rects that will make no embedded floats.
            footprint.use_floatIds = false;
            // Get widths and heights of floats overlapping this final block
            footprint.left_h = flprint_left_y - top_y;
            footprint.right_h = flprint_right_y - top_y;
            footprint.left_w = flprint_left_x - (x_min + d_left);
            footprint.right_w = x_max - d_right - flprint_right_x;
            if (footprint.left_h < 0 )
                footprint.left_h = 0;
            if (footprint.right_h < 0 )
                footprint.right_h = 0;
            if (footprint.left_w < 0 )
                footprint.left_w = 0;
            if (footprint.right_w < 0 )
                footprint.right_w = 0;
            footprint.left_min_y = flprint_left_min_y - top_y;
            footprint.right_min_y = flprint_right_min_y - top_y;
            if (footprint.left_min_y < 0 )
                footprint.left_min_y = 0;
            if (footprint.right_min_y < 0 )
                footprint.right_min_y = 0;
            // Generate the float to transfer
            footprint.generateEmbeddedFloatsFromFootprints( final_width );
        }
        return footprint;
    }

}; // Done with FlowState

// Register overflowing embedded floats into the main flow
void BlockFloatFootprint::forwardOverflowingFloat( int x, int y, int w, int h, bool r, ldomNode * node )
{
    if ( flow == NULL )
        return;
    flow->addPositionnedFloat( d_left + x, d_top + y, w, h, r, node );
    // Also update used_min_y and used_max_y, so they can be fetched
    // to update erm_final block's top_overflow and bottom_overflow
    // if some floats did overflow
    RenderRectAccessor fmt( node );
    if (y - fmt.getTopOverflow() < used_min_y)
        used_min_y = y - fmt.getTopOverflow();
    if (y + h + fmt.getBottomOverflow() > used_max_y)
        used_max_y = y + h + fmt.getBottomOverflow();
}

void BlockFloatFootprint::generateEmbeddedFloatsFromFloatIds( ldomNode * node,  int final_width )
{
    // We need to compute the footprints from the already computed
    // RenderRectAccessor of the current node, and all the floats
    // that were associated to the node because of their involvement
    // in text layout.
    lvRect rc;
    node->getAbsRect( rc, true ); // get formatted text abs coordinates
    int node_x = rc.left;
    int node_y = rc.top;
    floats_cnt = 0;
    for (int i=0; i<nb_floatIds; i++) {
        ldomNode * fbox = node->getDocument()->getTinyNode(floatIds[i]); // get node from its dataIndex
        // The floatBox rect values should be exactly the same as what was
        // used in the flow's _floats when rendering. We can check if this
        // is not the case (so a bug) by uncommenting a few things below.
        RenderRectAccessor fmt(fbox);
        fbox->getAbsRect( rc );
        /* Uncomment for checking reproducible results:
            int bf0, bf1, bf2, bf3, bf4;
            bf0=floats[floats_cnt][0]; bf1=floats[floats_cnt][1]; bf2=floats[floats_cnt][2];
            bf3=floats[floats_cnt][3]; bf4=floats[floats_cnt][4];
        */
        // clip them
        int x0 = rc.left - node_x;
        if ( x0 < 0 )
            x0 = 0;
        else if ( x0 > final_width )
            x0 = final_width;
        int x1 = rc.right - node_x;
        if ( x1 < 0 )
            x1 = 0;
        else if ( x1 > final_width )
            x1 = final_width;
        int y0 = rc.top > node_y ? rc.top - node_y : 0;
        int y1 = rc.bottom - node_y;
        // Sanity check to avoid negative width or height:
        if ( y1 < y0 ) { int ytmp = y0; y0 = y1 ; y1 = ytmp; }
        if ( x1 < x0 ) { int xtmp = x0; x0 = x1 ; x1 = xtmp; }
        floats[floats_cnt][0] = x0;      // x
        floats[floats_cnt][1] = y0;      // y
        floats[floats_cnt][2] = x1 - x0; // width
        floats[floats_cnt][3] = y1 - y0; // height
        floats[floats_cnt][4] = RENDER_RECT_HAS_FLAG(fmt, FLOATBOX_IS_RIGHT); // is_right
        /* Uncomment for checking reproducible results:
            if (x1 < x0) printf("!!!! %d %d %d %d\n", rc.left, rc.right, rc.top, rc.bottom);
            if ( bf0!=floats[floats_cnt][0] || bf1!=floats[floats_cnt][1] || bf2!=floats[floats_cnt][2] ||
                 bf3!=floats[floats_cnt][3] || bf4!=floats[floats_cnt][4] ) {
                    printf("node_x=%d node_y=%d\n", node_x, node_y);
                    printf("  fbox #%d x=%d y=%d\n", i+1, rc.left, rc.top);
                    printf("floatIds flt|abs mismatch: %d|%d %d|%d %d|%d %d|%d %d|%d txt:%s flt:%s \n",
                    bf0, floats[floats_cnt][0], bf1, floats[floats_cnt][1], bf2, floats[floats_cnt][2],
                    bf3, floats[floats_cnt][3], bf4, floats[floats_cnt][4],
                    UnicodeToLocal(ldomXPointer(node, 0).toString()).c_str(),
                    UnicodeToLocal(ldomXPointer(fbox, 0).toString()).c_str());
            }
        */
        floats_cnt++;
    }
}

void BlockFloatFootprint::generateEmbeddedFloatsFromFootprints( int final_width )
{
    floats_cnt = 0;
    // Add fake floats (to represent real outer floats) so that
    // their rectangles are considered when laying out lines
    // and other floats.
    // We need to keep them even if left_w or right_w <=0 (in
    // which case they'll have no visual impact on the text),
    // just so we can clear them when a <BR style="clear:">
    // is met.
    // Top left rectangle
    if ( left_h > 0 ) {
        floats[floats_cnt][0] = 0;      // x
        floats[floats_cnt][1] = 0;      // y
        floats[floats_cnt][2] = left_w; // width
        floats[floats_cnt][3] = left_h; // height
        floats[floats_cnt][4] = 0;      // is_right
        floats_cnt++;
    }
    // Top right rectangle
    if ( right_h > 0 ) {
        floats[floats_cnt][0] = final_width - right_w; // x
        floats[floats_cnt][1] = 0;                     // y
        floats[floats_cnt][2] = right_w;               // width
        floats[floats_cnt][3] = right_h;               // height
        floats[floats_cnt][4] = 1;                     // is_right
        floats_cnt++;
    }
    // Dummy 0x0 float for minimal y for next left float
    if ( left_min_y > 0 ) {
        floats[floats_cnt][0] = 0;          // x
        floats[floats_cnt][1] = left_min_y; // y
        floats[floats_cnt][2] = 0;          // width
        floats[floats_cnt][3] = 0;          // height
        floats[floats_cnt][4] = 0;          // is_right
        floats_cnt++;
    }
    // Dummy 0x0 float for minimal y for next right float
    if ( right_min_y > 0 ) {
        floats[floats_cnt][0] = final_width; // x
        floats[floats_cnt][1] = right_min_y; // y
        floats[floats_cnt][2] = 0;           // width
        floats[floats_cnt][3] = 0;           // height
        floats[floats_cnt][4] = 1;           // is_right
        floats_cnt++;
    }
}

void BlockFloatFootprint::store(ldomNode * node)
{
    RenderRectAccessor fmt( node );
    if ( use_floatIds ) {
        RENDER_RECT_SET_FLAG(fmt, FINAL_FOOTPRINT_AS_SAVED_FLOAT_IDS);
        fmt.setInvolvedFloatIds( nb_floatIds, floatIds );
    }
    else {
        RENDER_RECT_UNSET_FLAG(fmt, FINAL_FOOTPRINT_AS_SAVED_FLOAT_IDS);
        fmt.setTopRectsExcluded( left_w, left_h, right_w, right_h );
        fmt.setNextFloatMinYs( left_min_y, right_min_y );
    }
    if ( no_clear_own_floats ) {
        RENDER_RECT_SET_FLAG(fmt, NO_CLEAR_OWN_FLOATS);
    }
    else {
        RENDER_RECT_UNSET_FLAG(fmt, NO_CLEAR_OWN_FLOATS);
    }
    fmt.push();
}

void BlockFloatFootprint::restore(ldomNode * node, int final_width)
{
    RenderRectAccessor fmt( node );
    if ( RENDER_RECT_HAS_FLAG(fmt, FINAL_FOOTPRINT_AS_SAVED_FLOAT_IDS) ) {
        use_floatIds = true;
        fmt.getInvolvedFloatIds( nb_floatIds, floatIds );
        generateEmbeddedFloatsFromFloatIds( node, final_width );
    }
    else {
        fmt.getTopRectsExcluded( left_w, left_h, right_w, right_h );
        fmt.getNextFloatMinYs( left_min_y, right_min_y );
        generateEmbeddedFloatsFromFootprints( final_width );
    }
    no_clear_own_floats = RENDER_RECT_HAS_FLAG(fmt, NO_CLEAR_OWN_FLOATS);
}

int BlockFloatFootprint::getTopShiftX(int final_width, bool get_right_shift)
{
    int shift_x = 0;
    for (int i=0; i<floats_cnt; i++) {
        int * flt = floats[i];
        if ( flt[1] <= 0 && flt[3] > 0 ) { // Float running at y=0 with some height
            if ( !get_right_shift && !flt[4] ) { // Left float and left shift requested
                int flt_right = flt[0] + flt[2]; // x + width
                if ( flt_right > shift_x ) {
                    shift_x = flt_right;
                }
            }
            else if ( get_right_shift && flt[4] ) { // Right float and right shift requested
                int flt_left = flt[0] - final_width; // x - final_width (negative value relative to right border)
                if ( flt_left < shift_x ) {
                    shift_x = flt_left;
                }
            }
        }
    }
    return shift_x;
}

// Enhanced block rendering
void renderBlockElementEnhanced( FlowState * flow, ldomNode * enode, int x, int container_width, int flags )
{
    if (!enode)
        return;
    if ( ! enode->isElement() ) {
        crFatalError(111, "Attempting to render Text node");
    }

    int m = enode->getRendMethod();
    if (m == erm_invisible) // don't render invisible blocks
        return;

    css_style_rec_t * style = enode->getStyle().get();
    lUInt16 nodeElementId = enode->getNodeId();

    // See if dir= attribute or CSS specified direction
    int direction = flow->getDirection();
    if ( enode->hasAttribute( attr_dir ) ) {
        lString16 dir = enode->getAttributeValue( attr_dir );
        dir = dir.lowercase(); // (no need for trim(), it's done by the XMLParser)
        if ( dir.compare("rtl") == 0 ) {
            direction = REND_DIRECTION_RTL;
        }
        else if ( dir.compare("ltr") == 0 ) {
            direction = REND_DIRECTION_LTR;
        }
        else if ( dir.compare("auto") == 0 ) {
            direction = REND_DIRECTION_UNSET; // let fribidi detect direction
        }
    }
    // Allow CSS direction to override the attribute one (content creators are
    // advised to use the dir= attribute and not CSS direction - as CSS should
    // be less common, we allow tweaking direction via styles).
    if ( style->direction != css_dir_inherit ) {
        if ( style->direction == css_dir_rtl )
            direction = REND_DIRECTION_RTL;
        else if ( style->direction == css_dir_ltr )
            direction = REND_DIRECTION_LTR;
        else if ( style->direction == css_dir_unset )
            direction = REND_DIRECTION_UNSET;
    }
    bool is_rtl = direction == REND_DIRECTION_RTL; // shortcut for followup tests

    // See if lang= attribute
    bool has_lang_attribute = false;
    if ( enode->hasAttribute( attr_lang ) && !enode->getAttributeValue( attr_lang ).empty() ) {
        // We'll probably have to check it is a valid lang specification
        // before overriding the upper one.
        //   lString16 lang = enode->getAttributeValue( attr_lang );
        //   LangManager->check(lang)...
        // In here, we don't care about the language, we just need to
        // know if this node specifies one, so children final blocks
        // can fetch if from it.
        has_lang_attribute = true;
    }

    // See if this block is a footnote container, so we can deal with it accordingly
    bool isFootNoteBody = false;
    lString16 footnoteId;
    // Allow displaying footnote content at the bottom of all pages that contain a link
    // to it, when -cr-hint: footnote-inpage is set on the footnote block container.
    if ( style->cr_hint == css_cr_hint_footnote_inpage &&
                enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES)) {
        footnoteId = enode->getFirstInnerAttributeValue(attr_id);
        if ( !footnoteId.empty() )
            isFootNoteBody = true;
        // Notes:
        // It fails when that block element has itself an id, but links
        // do target an other inline sub element id (getFirstInnerAttributeValue()
        // would get the block element id, and there would be no existing footnote
        // for the link target id).
        // Not tested how it would behave with nested "-cr-hint: footnote-inpage"
    }
    // For fb2 documents. Description of the <body> element from FictionBook2.2.xsd:
    //   Main content of the book, multiple bodies are used for additional
    //   information, like footnotes, that do not appear in the main book
    //   flow. The first body is presented to the reader by default, and
    //   content in the other bodies should be accessible by hyperlinks. Name
    //   attribute should describe the meaning of this body, this is optional
    //   for the main body.
    /* Don't do that anymore in this hardcoded / not disable'able way: one can
     * enable in-page footnotes in fb2.css or a style tweak by just using:
     *     body[name="notes"] section    { -cr-hint: footnote-inpage; }
     *     body[name="comments"] section { -cr-hint: footnote-inpage; }
     * which will be hanbled by previous check.
     *
    if ( enode->getNodeId()==el_section && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) {
        ldomNode * body = enode->getParentNode();
        while ( body != NULL && body->getNodeId()!=el_body )
            body = body->getParentNode();
        if ( body ) {
            if (body->getAttributeValue(attr_name) == "notes" || body->getAttributeValue(attr_name) == "comments")
                footnoteId = enode->getAttributeValue(attr_id);
                if ( !footnoteId.empty() )
                    isFootNoteBody = true;
        }
    }
    */

    // is this a floating float container (floatBox)?
    bool is_floating = BLOCK_RENDERING(flags, FLOAT_FLOATBOXES) && enode->isFloatingBox();
    bool is_floatbox_child = BLOCK_RENDERING(flags, FLOAT_FLOATBOXES)
            && enode->getParentNode() && enode->getParentNode()->isFloatingBox();
    // is this a inline block container (inlineBox)?
    bool is_inline_box = enode->isBoxingInlineBox();
    bool is_inline_box_child = enode->getParentNode() && enode->getParentNode()->isBoxingInlineBox();

    // In the business of computing width and height, we should handle a bogus
    // embedded block (<inlineBox T="EmbeddedBlock">) (and its child) just
    // like any normal block element (taking the full width of its container
    // if no specified width, without the need to get its rendered width).
    if ( is_inline_box && enode->isEmbeddedBlockBoxingInlineBox(true) ) {
        is_inline_box = false;
    }
    if ( is_inline_box_child && enode->getParentNode()->isEmbeddedBlockBoxingInlineBox(true) ) {
        is_inline_box_child = false;
    }

    int em = enode->getFont()->getSize();

    int padding_left   = lengthToPx( style->padding[0], container_width, em )
                            + measureBorder(enode, 3) + DEBUG_TREE_DRAW;
    int padding_right  = lengthToPx( style->padding[1], container_width, em )
                            + measureBorder(enode, 1) + DEBUG_TREE_DRAW;
    int padding_top    = lengthToPx( style->padding[2], container_width, em )
                            + measureBorder(enode, 0) + DEBUG_TREE_DRAW;
    int padding_bottom = lengthToPx( style->padding[3], container_width, em )
                            + measureBorder(enode, 2) + DEBUG_TREE_DRAW;

    css_length_t css_margin_left  = style->margin[0];
    css_length_t css_margin_right = style->margin[1];

    int margin_left   = lengthToPx( css_margin_left, container_width, em ) + DEBUG_TREE_DRAW;
    int margin_right  = lengthToPx( css_margin_right, container_width, em ) + DEBUG_TREE_DRAW;
    int margin_top    = lengthToPx( style->margin[2], container_width, em ) + DEBUG_TREE_DRAW;
    int margin_bottom = lengthToPx( style->margin[3], container_width, em ) + DEBUG_TREE_DRAW;

    if ( ! BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_NEGATIVE_MARGINS) ) {
        if (margin_left < 0) margin_left = 0;
        if (margin_right < 0) margin_right = 0;
    }
    if ( ! BLOCK_RENDERING(flags, ALLOW_VERTICAL_NEGATIVE_MARGINS) ) {
        if (margin_top < 0) margin_top = 0;
        if (margin_bottom < 0) margin_bottom = 0;
    }
    bool margin_left_auto = css_margin_left.type == css_val_unspecified && css_margin_left.value == css_generic_auto;
    bool margin_right_auto = css_margin_right.type == css_val_unspecified && css_margin_right.value == css_generic_auto;

    // Adjust box size and position

    // <HR> gets its style width, height and margin:auto no matter flags
    bool is_hr = nodeElementId == el_hr;
    // <EMPTY-LINE> block element with height added for empty lines in txt document
    bool is_empty_line_elem = nodeElementId == el_empty_line;
        // Note: for a short time, we handled <BR> set with "display:block" here
        // just like EMPTY-LINE. Before that, block BRs did not end up being part
        // of a final node, and were just a block with no height, so not ensuring
        // the vertical blank space they aimed at.
        // This caused other issues, and comparing with Firefox/Calibre, it looks
        // like it's just best to always force BR to be css_d_inline, which we do
        // in setNodeStyle(). So, we'll never meet any <BR> here.

    // Get any style height to be ensured below (just before we add bottom
    // padding when erm_block or erm_final)
    // Otherwise, this block height will just be its rendered content height.
    int style_h = -1;
    bool apply_style_height = false;
    css_length_t style_height;
    int style_height_base_em;
    if ( is_floating || is_inline_box ) {
        // Nothing special to do: the child style height will be
        // enforced by subcall to renderBlockElement(child)
    }
    else if ( is_hr || is_empty_line_elem || BLOCK_RENDERING(flags, ENSURE_STYLE_HEIGHT) ) {
        // We always use the style height for <HR>, to actually have
        // a height to fill with its color
        style_height = style->height;
        style_height_base_em = em;
        apply_style_height = true;
        if ( is_empty_line_elem && style_height.type == css_val_unspecified ) {
            // No height specified: default to line-height, just like
            // if it were rendered final.
            int line_h;
            if ( style->line_height.type == css_val_unspecified &&
                        style->line_height.value == css_generic_normal ) {
                line_h = enode->getFont()->getHeight(); // line-height: normal
            }
            else {
                // In all other cases (%, em, unitless/unspecified), we can just
                // scale 'em', and use the computed value for absolute sized
                // values and 'rem' (related to root element font size).
                line_h = lengthToPx(style->line_height, em, em, true);
            }
            // Scale line_h according to gInterlineScaleFactor, but not if
            // it was already in screen_px, which means it has already been
            // scaled (in setNodeStyle() when inherited).
            if (style->line_height.type != css_val_screen_px && gInterlineScaleFactor != INTERLINE_SCALE_FACTOR_NO_SCALE)
                line_h = (line_h * gInterlineScaleFactor) >> INTERLINE_SCALE_FACTOR_SHIFT;
            style_height.value = line_h;
            style_height.type = css_val_screen_px;
        }
    }
    if ( apply_style_height && style_height.type != css_val_unspecified ) {
        if ( !BLOCK_RENDERING(flags, ALLOW_STYLE_W_H_ABSOLUTE_UNITS) &&
                style_height.type != css_val_percent && style_height.type != css_val_em &&
                style_height.type != css_val_ex && style_height.type != css_val_rem ) {
            apply_style_height = false;
        }
        if ( is_hr || is_empty_line_elem || apply_style_height ) {
            style_h = lengthToPx( style_height, container_width, style_height_base_em );
            if ( BLOCK_RENDERING(flags, USE_W3C_BOX_MODEL) ) {
                // If W3C box model requested, CSS height specifies the height
                // of the content box, so we just add paddings and borders
                // to the height we got from styles (paddings will be removed
                // when enforcing it below, but we keep the computation
                // common to both models doing it that way).
                style_h += padding_top + padding_bottom;
            }
        }
    }

    // Compute this block width
    int width;
    bool auto_width = false;
    bool table_shrink_to_fit = false;

    if ( is_floating || is_inline_box ) {
        // Floats width computation - which should also work as-is for inline block box
        // We need to have a width for floats, so we don't ignore anything no
        // matter the flags.
        // As the el_floatBox itself does not have any style->width or margins,
        // we should compute our width from the child style, and possibly
        // from its rendered content width.
        ldomNode * child = enode->getChildNode(0);
        int child_em = child->getFont()->getSize();
        css_style_ref_t child_style = child->getStyle();

        // We may tweak child styles
        bool style_changed = false;
        // If the child paddings are in %, they are related to this container width!
        // As we won't have access to it anymore when rendering the float,
        // convert them to screen_px now.
        css_style_ref_t newstyle(new css_style_rec_t);
        for (int i=0; i<4; i++) {
            if ( child_style->padding[i].type == css_val_percent ) {
                if (!style_changed) {
                    copystyle(child_style, newstyle);
                    style_changed = true;
                }
                newstyle->padding[i].type = css_val_screen_px;
                newstyle->padding[i].value = lengthToPx( child_style->padding[i], container_width, child_em );
            }
        }
        // Same for width, as getRenderedWidths() won't ensure width in %
        if ( child_style->width.type == css_val_percent ) {
            if (!style_changed) {
                copystyle(child_style, newstyle);
                style_changed = true;
            }
            newstyle->width.type = css_val_screen_px;
            newstyle->width.value = lengthToPx( child_style->width, container_width, child_em );
        }
        // (We could do the same fot height if in %, but it looks like Firefox
        // just ignore floats height in %, so let's ignore them too.)
        if ( ! BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_BLOCK_OVERFLOW) ) {
            // Simplest way to avoid a float overflowing its floatBox is to
            // ensure no negative margins
            int child_margin_left   = lengthToPx( child_style->margin[0], container_width, child_em );
            int child_margin_right  = lengthToPx( child_style->margin[1], container_width, child_em );
            if ( child_margin_left < 0 ) {
                if (!style_changed) {
                    copystyle(child_style, newstyle);
                    style_changed = true;
                }
                newstyle->margin[0].type = css_val_screen_px;
                newstyle->margin[0].value = 0;
            }
            if ( child_margin_right < 0 ) {
                if (!style_changed) {
                    copystyle(child_style, newstyle);
                    style_changed = true;
                }
                newstyle->margin[1].type = css_val_screen_px;
                newstyle->margin[1].value = 0;
            }
        }
        // Save child styles if we updated them
        if ( style_changed ) {
            child->setStyle(newstyle);
            child_style = newstyle;
        }

        // The margins of the element with float: are related to our container_width,
        // so account for them here, and don't let getRenderedWidths() add them (or they
        // would be reverse-computed from the inner content)
        int child_margin_left   = lengthToPx( child_style->margin[0], container_width, child_em );
        int child_margin_right  = lengthToPx( child_style->margin[1], container_width, child_em );
        int child_margins = child_margin_left + child_margin_right;
        // A floatBox does not have any margin/padding/border itself, but we
        // may add some border with CSS for debugging, so account for any
        int floatBox_paddings = padding_left + padding_right;
        // We let getRenderedWidths() give us the width of the float content:
        // if the float element itself has a style->width, we'll get it, with
        // or without paddings depending on USE_W3C_BOX_MODEL).
        int max_content_width = 0;
        int min_content_width = 0;
        // If the floating child does not have a width, inner elements may have one
        // Even if main flow is not ensuring style width, we need to ensure it
        // for floats to avoid getting page wide floats which won't look like
        // they are floating.
        int rend_flags = flags | BLOCK_RENDERING_ENSURE_STYLE_WIDTH | BLOCK_RENDERING_ALLOW_STYLE_W_H_ABSOLUTE_UNITS;
        // (ignoreMargin=true to ignore the child node margins as we already have them)
        getRenderedWidths(child, max_content_width, min_content_width, direction, true, rend_flags);
        // We should not exceed our container_width
        if (max_content_width + child_margins + floatBox_paddings < container_width) {
            width = max_content_width + child_margins + floatBox_paddings;
        }
        else {
            // It looks like Firefox never use min_content_width.
            // If max_content_width does not fit, we don't go as small as
            // the longest word length: we take the full container_width.
            width = container_width;
        }
        auto_width = true; // no more width tweaks (nor any x adjustment if is_rtl)
        // printf("floatBox width: max_w=%d min_w=%d => %d", max_content_width, min_content_width, width);
    }
    else if ( is_floatbox_child || is_inline_box_child ) {
        // The float style or rendered width has been applied to the wrapping
        // floatBox, so just remove node's margins of the container (the
        // floatBox) to get the child width.
        width = container_width - margin_left - margin_right;
        auto_width = true; // no more width tweaks
        // For tables, keep table_shrink_to_fit=false, so this width is not reduced
    }
    else { // regular element (non-float)
        bool apply_style_width = false;
        css_length_t style_width = style->width;
        if ( style->display > css_d_table ) {
            // table sub-elements widths are managed by the table layout algorithm
            apply_style_width = false;
        }
        else {
            // Only if ENSURE_STYLE_WIDTH as we may prefer having
            // full width text blocks to not waste reading width with blank areas.
            if ( style_width.type != css_val_unspecified ) {
                apply_style_width = BLOCK_RENDERING(flags, ENSURE_STYLE_WIDTH);
                if ( apply_style_width && !BLOCK_RENDERING(flags, ALLOW_STYLE_W_H_ABSOLUTE_UNITS) &&
                        style_width.type != css_val_percent && style_width.type != css_val_em &&
                        style_width.type != css_val_ex && style_width.type != css_val_rem ) {
                    apply_style_width = false;
                }
                if ( is_hr ) {
                    // We always use style width for <HR> for aesthetic reasons
                    apply_style_width = true;
                }
            }
            else if ( style->display == css_d_table || m == erm_table ) {
                // Table with no style width can shrink (and so can inline-table
                // and anonymous incomplete-but-completed tables).
                // If we are not ensuring style widths above, tables with
                // a width will not shrink and will fit container width.
                // (This should allow our table style tweaks to work
                // when !ENSURE_STYLE_WIDTH.)
                table_shrink_to_fit = true;
            }
        }
        if ( apply_style_width ) {
            width = lengthToPx( style_width, container_width, em );
            // In all crengine computation, width/fmt.getWidth() is the width
            // of the border box (content box + paddings + borders).
            // If we use what we got directly, we are in the traditional
            // box model (Netscape / IE5 / crengine legacy/default).
            if ( style->display == css_d_table ) {
                // TABLE style width always specifies its border box.
                // It's an exception to the W3C box model, as witnessed
                // with Firefox, and discussed at:
                //  https://stackoverflow.com/questions/19068909/why-is-box-sizing-acting-different-on-table-vs-div
            }
            else if ( BLOCK_RENDERING(flags, USE_W3C_BOX_MODEL) ) {
                // If W3C box model requested, CSS width specifies the width
                // of the content box.
                // In crengine, the width we deal with is the border box, so we
                // just add paddings and borders to the width we got from styles.
                width += padding_left + padding_right;
            }
            // printf("  apply_style_width => %d\n", width);
        }
        else {
            width = container_width - margin_left - margin_right;
            auto_width = true; // no more width tweaks
        }
    }

    // What about a width with a negative value?
    // It seems we are fine with a negative width in our recursive
    // x and width computations, as children may make it positive
    // again with negative margins, and we seem to always end up
    // with a correct x position to draw our block at, quite
    // very similarly as Firefox.
    // So, our x/dx computations below seem like they may need
    // us to keep a negative width value to be done right.
    // (We'll revisit that when we meet counterexamples.)
    //
    // There are nevertheless a few things to take care of later
    // when we get a negative width:
    // - We want to avoid negative RenderRectAcessor.getWidth(),
    //   so we'll set it to zero. It's mostly only used to draw
    //   borders and backgrounds, and it seems alright to have
    //   zero of these on such blocks.
    //   Caveat: elementFromPoint()/createXPointer may get
    //   confused with such rect, and not travel thru them, or
    //   skip them.
    // - A table with a zero or negative width (which can happen
    //   with very busy imbricated tables) won't be drawn, and
    //   it's rendering method will be switched to erm_killed
    //   to display some small visual indicator.
    // - Legacy rendering code keeps a negative width in
    //   RenderRectAccessor, and, with erm_final nodes, provides
    //   it as is to node->renderFinalBlock(), which casts it to
    //   a signed integer when calling txform->Format(inner_width),
    //   resulting in text formatted over a huge overflowing width.
    //   This is somehow a quite good end solution to deal with
    //   text formatting over a bogus negative width (instead of
    //   just not displaying anything) as it gives a hint to the
    //   user that something is wrong, with this text overflowing
    //   the screen.
    //   So, we won't be using erm_killed for these erm_final nodes,
    //   but we will set the inner_width to be 1x screen width. Some
    //   text may still overflow, text selection may not work, but
    //   a bit  more of it will be seen on multiple lines.
    //   Note: Firefox in this case uses the min content width as
    //   a fallback width (we could do the same, but it is costly
    //   and may result in adding many many pages with a narrow
    //   column of one or two words on each line.

    // Reference: https://www.w3.org/TR/CSS2/visudet.html#blockwidth
    // As crengine uses internally the traditional box model, the width
    // we are computing here is the width between the borders of the box,
    // including padding and border.
    // So, these rules are simplified to:
    // - margin_left + width + margin_right = container_width
    // - If width is not 'auto', and 'margin_left + width + margin_right' is larger
    //   than container_width, then any 'auto' values for 'margin-left' or 'margin-right'
    //   are, for the following rules, treated as zero.
    // - If all of the above have a computed value other than 'auto', the values are
    //   said to be "over-constrained" and one of the used values will have to be
    //   different from its computed value. If the 'direction' property of the
    //   containing block has the value 'ltr', the specified value of 'margin-right'
    //   is ignored and the value is calculated so as to make the equality true.
    // - If there is exactly one value specified as 'auto', its used value follows
    //   from the equality
    // - If 'width' is set to 'auto', any other 'auto' values become '0' and 'width'
    //   follows from the resulting equality
    // - If both 'margin-left' and 'margin-right' are 'auto', their used values are
    //   equal. This horizontally centers the element with respect to the edges
    //   of the containing block

    // We now have the prefered width, and we need to adjust x to the position
    // where this width is to start.
    // ('x' might not be 0, as it includes the parent padding & borders)
    // (margin_left and margin_right have a value of 0 if they are "auto")

    // In most cases, our shift from x (our margin left inside container_width)
    // is... margin_left
    int dx = margin_left;

    if ( !auto_width ) { // We have a width that may not fill all available space
        // printf("fixed width: %d\n", width);
        if ( width + margin_left + margin_right > container_width ) {
            if ( is_rtl ) {
                margin_left = 0; // drop margin_left if RTL
                dx = 0;
            }
            else {
                margin_right = 0; // drop margin_right otherwise
            }
        }
        if ( width + margin_left + margin_right > container_width ) {
            // We can't ensure any auto (or should we? ensure centering
            // by even overflow on each side?)
        }
        else { // We fit into container_width
            bool margin_auto_ensured = false;
            if ( BLOCK_RENDERING(flags, ENSURE_MARGIN_AUTO_ALIGNMENT) || is_hr ) {
                // https://www.hongkiat.com/blog/css-margin-auto/
                //  "what do you think will happen when the value auto is given
                //   to only one of those? A left or right margin with auto will
                //   take up all of the "available" space making the element
                //   look like it has been flushed right or left."
                // The CSS specs do not seem to mention that.
                // But Firefox ensures it. So let's do the same.
                //
                // (if margin_left_auto, we have until now: dx = margin_left = 0)
                if ( margin_left_auto && margin_right_auto ) {
                    dx = (container_width - width) / 2;
                    margin_auto_ensured = true;
                }
                else if ( margin_left_auto ) {
                    dx = container_width - width;
                    margin_auto_ensured = true;
                }
                else if ( margin_right_auto ) {
                    // No dx tweak needed
                    margin_auto_ensured = true;
                }
            }
            if ( !margin_auto_ensured ) {
                // Nothing else needed for LTR: stay stuck to the left
                // For RTL: stick it to the right
                if (is_rtl) {
                    dx = container_width - width;
                }
            }
        }
    }

    // Prevent overflows if not allowed
    if ( ! BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_BLOCK_OVERFLOW) ) {
        if ( width > container_width ) { // width alone is bigger than container
            dx = 0; // drop any left shift
            width = container_width; // adjust to contained width
        }
        else if ( dx + width > container_width ) {
            // width is smaller that container's, but dx makes it overflow
            dx = container_width - width;
        }
    }
    if ( ! BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_PAGE_OVERFLOW) ) {
        // Ensure it does not go past the left or right of the original
        // container width (= page, for main flow)
        // Note: not (yet) implemented for floats (but it is naturally
        // ensured if !ALLOW_HORIZONTAL_BLOCK_OVERFLOW)
        int o_width = flow->getOriginalContainerWidth();
        int abs_x = flow->getCurrentAbsoluteX();
        // abs_x already accounts for x (=padding_left of parent container,
        // which is given to flow->newBlockLevel() before being also given
        // to renderBlockElementEnhanced() as x).
        if ( abs_x + dx < 0 ) {
            dx = - abs_x; // clip to page left margin
        }
        if ( abs_x + dx + width > o_width ) {
            width = o_width - abs_x - dx; // clip width to page right margin
        }
    }

    // Apply dx, and we're done with width and x
    x += dx;
    // printf("width: %d   dx: %d > x: %d\n", width, dx, x);

    bool no_margin_collapse = false;
    if ( flow->getCurrentLevel() == 0 ) {
        // "Margins of the root element's box do not collapse"
        // We'll push it immediately below
        no_margin_collapse = true;
    }
    else if ( flow->getCurrentLevel() == 1 && (enode->getParentNode()->isFloatingBox() ||
                                               enode->getParentNode()->isBoxingInlineBox()) ) {
        // The inner margin of the real float element (the single child of a floatBox)
        // have to be pushed and not collapse with outer margins so they can
        // get accounted in the float height.
        // (This must be true also with inline-block boxes, but not tested/verified.)
        no_margin_collapse = true;
    }

    // Ensure page breaks following the rules from:
    //   https://www.w3.org/TR/CSS2/page.html#allowed-page-breaks
    // Also ensure vertical margin collapsing, with rules from:
    //   https://www.w3.org/TR/CSS21/box.html#collapsing-margins
    //   https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Box_Model/Mastering_margin_collapsing
    // (Test suite for margin collapsing: http://test.weasyprint.org/suite-css21/chapter8/section3/)
    // All of this is mostly ensured in flow->pushVerticalMargin()
    int break_before = CssPageBreak2Flags( style->page_break_before );
    int break_after = CssPageBreak2Flags( style->page_break_after );
    int break_inside = CssPageBreak2Flags( style->page_break_inside );
    // Note: some test suites seem to indicate that an inner "break-inside: auto"
    // can override an outer "break-inside: avoid". We don't ensure that.

    if ( no_margin_collapse ) {
        // Push any earlier margin so it does not get collapsed with this one
        flow->pushVerticalMargin();
    }
    // We don't shift y yet. We accumulate margin, and, after ensuring
    // collapsing, emit it on the first non-margin real content added.
    flow->addVerticalMargin( enode, margin_top, break_before, true ); // is_top_margin=true

    LFormattedTextRef txform;

    // Set what we know till now about this block
    RenderRectAccessor fmt( enode );
    // Set direction for all blocks (needed for text in erm_final, but also for list item
    // markers in erm_block, so that DrawDocument can draw it on the right if rtl).
    RENDER_RECT_SET_DIRECTION(fmt, direction);
    // Store lang node index if it's an erm_final node (it's only needed for these,
    // as the starting lang for renderFinalBlock())
    if ( m == erm_final ) {
        if ( has_lang_attribute )
            fmt.setLangNodeIndex( enode->getDataIndex() );
        else
            fmt.setLangNodeIndex( flow->getLangNodeIndex() );
    }
    fmt.setX( x );
    fmt.setY( flow->getCurrentRelativeY() );
    fmt.setWidth( width );
    if ( width < 0) {
        // We might be fine with a negative width when recursively rendering
        // children nodes (which may make it positive again with negative
        // margins). But we don't want our RenderRect to have a negative width.
        fmt.setWidth( 0 );
    }
    fmt.setHeight( 0 ); // will be updated below
    fmt.push();
    // This has to be delayed till now: it may adjust the Y we just setY() above
    if ( no_margin_collapse ) {
        flow->pushVerticalMargin();
    }

    #ifdef DEBUG_BLOCK_RENDERING
        if (!flow->isMainFlow()) printf("\t\t|");
        for (int i=1; i<=flow->getCurrentLevel(); i++) { printf("%c", flow->isMainFlow() ? ' ':'-'); }
        printf("%c", m==erm_block?'B':m==erm_table?'T':m==erm_final?'F':'?');
        printf("\t%s", UnicodeToLocal(ldomXPointer(enode, 0).toString()).c_str());
        printf("\tc_y=%d (rely=%d)\n", flow->getCurrentAbsoluteY(), flow->getCurrentRelativeY());
    #endif

    if (width <= 0) {
        // In case we get a negative width (no room to render and draw anything),
        // which may happen in hyper constrained layouts like heavy nested tables,
        // don't go further in the rendering code.
        // It seems erm_block nodes do "survive" such negative width,
        // by just keeping substracting margin and padding to this negative
        // number until we reach an erm_final. For these, below, we possibly
        // do our best below to ensure a final positive inner_width.
        // So, we only do the following for tables, where the rendering code
        // is more easily messed up by negative widths. As we won't show
        // any table, and we want the user to notice something is missing,
        // we set this element rendering method to erm_killed, and
        // DrawDocument will then render a small figure...
        if ( m >= erm_table && !(is_floatbox_child || is_inline_box_child) ) {
            // (Avoid this with float or inline tables, that have been measured
            // and can have a 0-width when they have no content.)
            printf("CRE WARNING: no width to draw %s\n", UnicodeToLocal(ldomXPointer(enode, 0).toString()).c_str());
            enode->setRendMethod( erm_killed );
            fmt.setHeight( 15 ); // not squared, so it does not look
            fmt.setWidth( 10 );  // like a list square bullet
            fmt.setX( fmt.getX() - 5 );
                // We shift it half to the left, so a bit of it can be
                // seen if some element on the right covers it with some
                // background color.
            flow->addContentLine( fmt.getHeight(), RN_SPLIT_BOTH_AVOID, fmt.getHeight() );
            return;
        }
    }

    switch( m ) {
        case erm_table:
            {
                // As we don't support laying tables aside floats, just clear
                // all floats and push all margins
                flow->clearFloats( css_c_both );
                flow->pushVerticalMargin();

                // We need to update the RenderRectAccessor() as renderTable will
                // use it to get the absolute table start y for context.AddLine()'ing
                fmt.setY( flow->getCurrentRelativeY() );
                fmt.push();

                if ( isFootNoteBody )
                    flow->getPageContext()->enterFootNote( footnoteId );

                // Ensure page-break-inside avoid, from the table's style or
                // from outer containers
                bool avoid_pb_inside = break_inside==RN_SPLIT_AVOID;
                if (!avoid_pb_inside)
                    avoid_pb_inside = flow->getAvoidPbInside();

                // We allow a table to shrink width (cells to shrink to their content),
                // unless they have a width specified.
                // This can be tweaked with:
                //   table {width: 100% !important} to have tables take the full available width
                //   table {width: auto !important} to have tables shrink to their content
                int table_width = width;
                int fitted_width = -1;
                // renderTable has not been updated to use 'flow', and it looks
                // like it does not really need to.
                int h = renderTable( *(flow->getPageContext()), enode, 0, flow->getCurrentRelativeY(),
                            table_width, table_shrink_to_fit, fitted_width, direction, avoid_pb_inside, true );
                // (It feels like we don't need to ensure a table specified height.)
                fmt.setHeight( h );
                // Update table width if it was fitted/shrunk
                if (table_shrink_to_fit && fitted_width > 0)
                    table_width = fitted_width;
                fmt.setWidth( table_width );
                if (table_width < width) {
                    // This was already done above, but it needs to be adjusted
                    // again if the table width was shrunk.
                    // See for margin: auto, to center or align right the table
                    int shift_x = 0;
                    if (is_rtl) { // right align
                        shift_x = (width - table_width);
                    }
                    if (margin_left_auto) {
                        if (margin_right_auto) { // center align
                            shift_x = (width - table_width)/2;
                        }
                        else { // right align
                            shift_x = (width - table_width);
                        }
                    }
                    if (shift_x) {
                        fmt.setX( fmt.getX() + shift_x );
                    }
                }
                fmt.push();

                flow->moveDown(h);

                if ( isFootNoteBody )
                    flow->getPageContext()->leaveFootNote();

                flow->addVerticalMargin( enode, margin_bottom, break_after );
                return;
            }
            break;
        case erm_block:
        case erm_inline: // For inlineBox elements only
            {
                if (m == erm_inline && nodeElementId != el_inlineBox) {
                    printf("CRE WARNING: node discarded (unexpected erm_inline for elem %s)\n",
                                UnicodeToLocal(ldomXPointer(enode, 0).toString()).c_str());
                                // (add %s and enode->getText8().c_str() to see text content)
                    // Might be too early after introducing inline-block support to crash:
                    // let's just output this warning and ignore the node content.
                    // crFatalError(143, "erm_inline for element not inlineBox");
                    return;
                }
                // Deal with list item marker
                // List item marker rendering when css_d_list_item_block
                int list_marker_padding = 0; // set to non-zero when list-style-position = outside
                int list_marker_height = 0;
                if ( style->display == css_d_list_item_block ) {
                    // list_item_block rendered as block (containing text and block elements)
                    // Get marker width and height
                    LFormattedTextRef txform( enode->getDocument()->createFormattedText() );
                    int list_marker_width;
                    lString16 marker = renderListItemMarker( enode, list_marker_width, txform.get(), -1, 0);
                    list_marker_height = txform->Format( (lUInt16)(width - list_marker_width), (lUInt16)enode->getDocument()->getPageHeight(), direction );
                    if ( ! renderAsListStylePositionInside(style, is_rtl) ) {
                        // When list_style_position = outside, we have to shift the whole block
                        // to the right and reduce the available width, which is done
                        // below when calling renderBlockElement() for each child
                        list_marker_padding = list_marker_width;
                    }
                    else if ( style->list_style_type != css_lst_none ) {
                        // When list_style_position = inside, we need to let renderFinalBlock()
                        // know there is a marker to prepend when rendering the first of our
                        // children (or grand-children, depth first) that is erm_final
                        // (caveat: the marker will not be shown if any of the first children
                        // is erm_invisible)
                        // (No need to do anything when list-style-type none.)
                        ldomNode * tmpnode = enode;
                        while ( tmpnode && tmpnode->hasChildren() ) {
                            tmpnode = tmpnode->getChildNode( 0 );
                            if (tmpnode && tmpnode->getRendMethod() == erm_final) {
                                // We need renderFinalBlock() to be able to reach the current
                                // enode when it will render/draw this tmpnode, so it can call
                                // renderListItemMarker() on it and get a marker formatted
                                // according to current node style.
                                // We store enode's data index into the RenderRectAccessor of
                                // this erm_final tmpnode so it's saved in the cache.
                                // (We used to use NodeNumberingProps to store it, but it
                                // is not saved in the cache.)
                                RenderRectAccessor tmpfmt( tmpnode );
                                tmpfmt.setListPropNodeIndex( enode->getDataIndex() );
                                break;
                            }
                        }
                    }
                }

                // Note: there's something which can be a bit confusing here:
                // we shift the flow state by padding_top (above, while dealing
                // with it for page split context) and padding_left (just below).
                // But each child x and y (set by renderBlockElement() below) must
                // include padding_top and padding_left. So we keep providing these
                // to renderBlockElement() even if it feels a bit out of place,
                // notably in the float positionning code. But it works...

                // Shrink flow state area: children that are float will be
                // constrained into this area
                // ('width' already had margin_left/_right substracted)
                flow->newBlockLevel(width - list_marker_padding - padding_left - padding_right, // width
                       margin_left + (is_rtl ? 0 : list_marker_padding) + padding_left, // d_left
                       break_inside==RN_SPLIT_AVOID,
                       direction,
                       has_lang_attribute ? enode->getDataIndex() : -1);

                if (padding_top>0) {
                    // This may push accumulated vertical margin
                    flow->addContentLine(padding_top, RN_SPLIT_AFTER_AVOID, 0, true);
                }

                // Enter footnote body only after padding, to get rid of it
                // and have lean in-page footnotes
                if ( isFootNoteBody )
                    flow->getPageContext()->enterFootNote( footnoteId );

                // recurse all sub-blocks for blocks
                int cnt = enode->getChildCount();
                for (int i=0; i<cnt; i++) {
                    ldomNode * child = enode->getChildNode( i );
                    if (!child)
                        continue;
                    if ( child->isText() ) {
                        // We may occasionally let empty text nodes among block elements,
                        // just skip them
                        lString16 s = child->getText();
                        if ( IsEmptySpace(s.c_str(), s.length() ) )
                            continue;
                        crFatalError(144, "Attempting to render non-empty Text node");
                    }
                    css_style_ref_t child_style = child->getStyle();

                    // We must deal differently with children that are floating nodes.
                    // Different behaviors with "clear:"
                    // - If a non-floating block has a "clear:", it is moved below the last
                    //   float on that side
                    // - If a floating block has a "clear:", it is moved below the last
                    //   float on that side BUT the following non-floating blocks should
                    //   not move and continue being rendered at the current y

                    // todo: if needed, implement float: and clear: inline-start / inline-end

                    if ( child->isFloatingBox() ) {
                        // Block floats are positionned respecting the current collapsed
                        // margin, without actually globally pushing it, and without
                        // collapsing with it.
                        int flt_vertical_margin = flow->getCurrentVerticalMargin();
                        bool is_right = child_style->float_ == css_f_right;
                        // (style->clear has not been copied to the floatBox: we must
                        // get it from the floatBox single child)
                        css_clear_t child_clear = child->getChildNode(0)->getStyle()->clear;
                        // Provide an empty context so float content does not add lines
                        // to the page splitting context. The non-floating nodes will,
                        // and if !DO_NOT_CLEAR_OWN_FLOATS, we'll fill the remaining
                        // height taken by floats if any.
                        LVRendPageContext alt_context( NULL, flow->getPageHeight(), false );
                        // For floats too, the provided x/y must be the padding-left/top of the
                        // parent container of the float (and width must exclude the parent's
                        // padding-left/right) for the flow to correctly position inner floats:
                        // flow->addFloat() will additionally shift its positionning by the
                        // child x/y set by this renderBlockElement().
                        renderBlockElement( alt_context, child, (is_rtl ? 0 : list_marker_padding) + padding_left,
                                    padding_top, width - list_marker_padding - padding_left - padding_right, direction );
                        flow->addFloat(child, child_clear, is_right, flt_vertical_margin);
                        // Gather footnotes links accumulated by alt_context
                        lString16Collection * link_ids = alt_context.getLinkIds();
                        if (link_ids->length() > 0) {
                            for ( int n=0; n<link_ids->length(); n++ ) {
                                flow->getPageContext()->addLink( link_ids->at(n) );
                            }
                        }
                    }
                    else {
                        css_clear_t child_clear = child_style->clear;
                        // If this child is going to split page, clear all floats before
                        if ( CssPageBreak2Flags( child_style->page_break_before ) == RN_SPLIT_ALWAYS )
                            child_clear = css_c_both;
                        flow->clearFloats( child_clear );
                        renderBlockElementEnhanced( flow, child, (is_rtl ? 0 : list_marker_padding )+ padding_left,
                            width - list_marker_padding - padding_left - padding_right, flags );
                        // Vertical margins collapsing is mostly ensured in flow->pushVerticalMargin()
                        //
                        // Various notes about it:
                        // https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Box_Model/Mastering_margin_collapsing
                        //   "The margins of adjacent siblings are collapsed (except
                        //   when the latter sibling needs to be cleared past floats)."
                        // https://www.w3.org/TR/CSS21/box.html#collapsing-margins
                        //  "The bottom margin of an in-flow block-level element always
                        //  collapses with the top margin of its next in-flow block-level
                        //  sibling, unless that sibling has clearance. "
                        // https://www.w3.org/TR/CSS21/visuren.html#clearance
                        //  clearance is not just having clear: it's when clear: has to
                        //  do some moving
                        // So we should not do this:
                        //   if ( child_clear > css_c_none ) flow->pushVerticalMargin();
                        // It looks like it just means that the upcoming pushed/collapsed
                        // vertical margin could be part of the clear'ed vertical area.
                        // We attempt at doing that in pushVerticalMargin(), and we manage
                        // to look quite much like how Firefox renders - although we're
                        // not doing all the computations the specs suggest.
                        //
                        // https://www.w3.org/TR/CSS21/box.html#collapsing-margins
                        //  "Adjoining vertical margins collapse, except:
                        //  2) If the top and bottom margins of an element with clearance
                        //     are adjoining, its margins collapse with the adjoining
                        //     margins of following siblings but that resulting margin does
                        //     not collapse with the bottom margin of the parent block."
                        // Not sure about this one. Is this about empty elements ("top and
                        // bottom margins are adjoining" with each other)? Or something else?
                    }
                }

                // Ensure there's enough height to fully display the list marker
                int current_h = flow->getCurrentRelativeY();
                if (list_marker_height && list_marker_height > current_h) {
                    flow->addContentSpace(list_marker_height - current_h, 1, current_h > 0, true, false);
                }

                // Leave footnote body before style height and padding, to get
                // rid of them and have lean in-page footnotes
                if ( isFootNoteBody )
                    flow->getPageContext()->leaveFootNote();

                if (style_h >= 0) {
                    current_h = flow->getCurrentRelativeY() + padding_bottom;
                    int pad_h = style_h - current_h;
                    if (pad_h > 0) {
                        if (pad_h > flow->getPageHeight()) // don't pad more than one page height
                            pad_h = flow->getPageHeight();
                        // Add this space to the page splitting context
                        // Allow page splitting inside this useless excessive style height
                        // (Unless it's a <EMPTY-LINE> that we're rather keep it all on a
                        // page, to avoid text line shifts and ghosting in interline.)
                        bool split_avoid_inside = is_empty_line_elem;
                        flow->addContentSpace(pad_h, 1, false, split_avoid_inside, false);
                    }
                }

                if ( no_margin_collapse ) {
                    // Push any earlier margin so it does not get collapsed with this one,
                    // and we get the resulting margin in the height given by leaveBlockLevel().
                    flow->pushVerticalMargin();
                }
                else if ( current_h == 0 && BLOCK_RENDERING(flags, DO_NOT_CLEAR_OWN_FLOATS) && flow->hasActiveFloats() ) {
                    // There is no problem with some empty height/content blocks
                    // with vertical margins, unless there are floats around
                    // (whether they are outer floats, or standalone embedded
                    // float(s) inside some erm_final block with nothing else,
                    // which gave that current_h=0).
                    // If floats are involved, the vertical margin should
                    // apply above this empty block: it's like the following
                    // non-empty block should grab these previous empty blocks
                    // with it, and so the inner floats should be moved down.
                    // This is complicated to get right, so, to avoid more
                    // visible glitches and mismatches in floats position and
                    // footprints, it's safer and less noticable to just push
                    // vertical margin now and disable any further vertical
                    // margin until some real content lines have been sent.
                    // Sample test case:
                    //     <div style="margin: 1em 0">aaa</div>
                    //     <div style="margin: 1em 0"><span style="float: left">bbb</span></div>
                    //     <div style="margin: 1em 0">ccc</div>
                    //   bbb and ccc should be aligned
                    // So, this is wrong, but the simplest to solve this case:
                    flow->pushVerticalMargin();
                    flow->disableVerticalMargin();
                    // But this drop the H2 top margin in this test case:
                    //     <div>some dummy text</div>
                    //     <div> <!-- just because the float is inner to this div, which will get a 0-height -->
                    //       <div style="float: left">some floating div</div>
                    //     </div>
                    //     <H2>This is a H2</H2>
                }

                int top_overflow = 0;
                int bottom_overflow = 0;
                int h = flow->leaveBlockLevel(top_overflow, bottom_overflow);

                // padding bottom should be applied after leaveBlockLevel
                // (Firefox, with a float taller than text, both in another
                // float, applies bottom padding after the inner float)
                if (padding_bottom>0) {
                    int padding_bottom_with_inner_pushed_vm = flow->addContentLine(padding_bottom, RN_SPLIT_BEFORE_AVOID, 0, true);
                    h += padding_bottom_with_inner_pushed_vm;
                    bottom_overflow -= padding_bottom_with_inner_pushed_vm;
                }

                if (h <=0) {
                    // printf("negative h=%d %d %s %s\n", h, is_floating,
                    //     UnicodeToLocal(ldomXPointer(enode, 0).toString()).c_str(), enode->getText().c_str());
                    // Not sure if we can get negative heights in the main flow
                    // (but when we did, because of bugs?, it resulted in hundreds
                    // of blank pages).
                    // Getting a zero height may prevent the block and its children
                    // from being drawn, which is fine for node with no real content.
                    // But we can rightfully get a negative height for floatBoxes
                    // whose content has negative margins and is fully outside
                    // the floatBox.
                    // So, fix height as needed
                    if ( is_floating ) {
                        // Allow it to be candidate for drawing, and have a minimal
                        // height so it's not just ignored.
                        h = 1;
                        bottom_overflow -= 1;
                    }
                    else { // Assume no content and nothing to draw
                        h = 0;
                    }
                }

                // Finally, get the height used by our padding and children.
                // Original fmt.setY() might have been updated by collapsing margins,
                // but we got the real final height.
                fmt.setHeight( h );
                fmt.setTopOverflow( top_overflow );
                fmt.setBottomOverflow( bottom_overflow );
                fmt.push();
                // if (top_overflow > 0) printf("block top_overflow=%d\n", top_overflow);
                // if (bottom_overflow > 0) printf("block bottom_overflow=%d\n", bottom_overflow);

                flow->addVerticalMargin( enode, margin_bottom, break_after );
                if ( no_margin_collapse ) {
                    // Push our margin so it does not get collapsed with some later one
                    flow->pushVerticalMargin();
                }
                return;
            }
            break;
        case erm_final:
            {
                // Deal with list item marker
                int list_marker_padding = 0;;
                if ( style->display == css_d_list_item_block ) {
                    // list_item_block rendered as final (containing only text and inline elements)
                    if ( ! renderAsListStylePositionInside(style, is_rtl) ) {
                        // When list_style_position = outside, we have to shift the final block
                        // to the right (or to the left if RTL) and reduce its width
                        lString16 marker = renderListItemMarker( enode, list_marker_padding, NULL, -1, 0 );
                        // With css_lsp_outside, the marker is outside: it shifts x left and reduces width
                        width -= list_marker_padding;
                        fmt.setWidth( width );
                        // when is_rtl, just reducing width is enough
                        if (!is_rtl)
                            fmt.setX( fmt.getX() + list_marker_padding );
                        fmt.push();
                    }
                }
                // Deal with negative text-indent
                if ( style->text_indent.value < 0 ) {
                    int indent = - lengthToPx(style->text_indent, container_width, em);
                    // We'll need to have text written this positive distance outside
                    // the nominal text inner_width.
                    // We can remove it from left padding if indent is smaller than padding.
                    // If it is larger, we can't remove the excess from left margin, as
                    // these margin should stay fixed for proper background drawing in their
                    // limits (the text with negative text-indent should overflow the
                    // margin and background color).
                    // But, even if CSS forbids negative padding, the followup code might
                    // be just fine with negative values for padding_left/_right !
                    // (Not super sure of that, but it looks like it works, so let's
                    // go with it - if issues, one can switch to a rendering mode
                    // without the ALLOW_HORIZONTAL_BLOCK_OVERFLOW flag).
                    // (Text selection on the overflowing text may not work, but it's
                    // the same for negative margins.)
                    if ( !is_rtl ) {
                        padding_left -= indent;
                        if ( padding_left < 0 ) {
                            if ( !BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_BLOCK_OVERFLOW) ) {
                                padding_left = 0; // be safe, drop excessive part of indent
                            }
                            else if ( !BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_PAGE_OVERFLOW) ) {
                                // Limit to top node (page, float) left margin
                                int abs_x = flow->getCurrentAbsoluteX();
                                if ( abs_x + padding_left < 0 )
                                    padding_left = -abs_x;
                            }
                        }
                    }
                    else {
                        padding_right -= indent;
                        if ( padding_right < 0 ) {
                            if ( !BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_BLOCK_OVERFLOW) ) {
                                padding_right = 0;
                            }
                            else if ( !BLOCK_RENDERING(flags, ALLOW_HORIZONTAL_PAGE_OVERFLOW) ) {
                                int o_width = flow->getOriginalContainerWidth();
                                int abs_x = flow->getCurrentAbsoluteX();
                                if ( abs_x + width + padding_right < o_width )
                                    padding_right = o_width - width - abs_x;
                            }
                        }
                    }
                }

                // To get an accurate BlockFloatFootprint, we need to push vertical
                // margin now (and not delay it to the first addContentLine()).
                // This can mess with proper margins collapsing if we were to
                // output no content (we don't know that yet).
                // So, do it only if we have floats.
                if ( flow->hasActiveFloats() )
                    flow->pushVerticalMargin();

                // Store these in RenderRectAccessor fields, to avoid having to
                // compute them again when in XPointer, elementFromPoint...
                int inner_width = width - padding_left - padding_right;
                if (inner_width <= 0) {
                    // inner_width is the width given to LFormattedText->Format()
                    // to lay out inlines and text along this width.
                    // Legacy code allows it to be negative, but it ends
                    // up being cast'ed to a signed int, resulting in a
                    // huge width, with possibly text laid out on a single
                    // long overflowing line.
                    // We prefer here to make it positive with a limited
                    // width, still possibly overflowing screen, but allowing
                    // more text to be seen.
                    // (This will mess with BlockFloatFootprint and proper
                    // layout of floats, but we're in some edgy situation.)
                    if ( width - padding_left > 0 ) {
                        // Just kill padding_right
                        inner_width = width - padding_left;
                        padding_right = 0;
                    }
                    else if ( width > 0 ) {
                        // Just kill both paddings
                        inner_width = width - padding_left;
                        padding_left = padding_right = 0;
                    }
                    else {
                        // Kill padding and switch to the top container
                        // width (the page width)
                        inner_width = flow->getOriginalContainerWidth();
                        padding_left = padding_right = 0;
                    }
                    // We could also do like Firefox and use the minimal content width:
                    //   int max_content_width = 0;
                    //   int min_content_width = 0;
                    //   getRenderedWidths(enode, max_content_width, min_content_width, true, flags);
                    //   inner_width = min_content_width;
                    // but it is costly and may result in adding many many pages
                    // with a narrow column of words.
                }
                fmt.setInnerX( padding_left );
                fmt.setInnerY( padding_top );
                fmt.setInnerWidth( inner_width );
                RENDER_RECT_SET_FLAG(fmt, INNER_FIELDS_SET);
                fmt.push();
                // (These setInner* needs to be set before creating float_footprint if
                // we want to debug/valide floatIds coordinates)

                // Outer block floats may be drawn over the erm_final node rect.
                // Only its text (and embedded floats) must be laid out outside
                // these outer floats areas (left and right paddings should be
                // left under the floats, and should not be ensured after the
                // outer floats areas).
                // We will provide the text formatter with a small BlockFloatFootprint
                // object, that will provide at most 5 rectangles representing outer
                // floats (either real floats, or fake floats).
                BlockFloatFootprint float_footprint = flow->getFloatFootprint( enode,
                    margin_left + (is_rtl ? 0: list_marker_padding) + padding_left,
                    margin_right + (is_rtl ? list_marker_padding : 0) + padding_right,
                    padding_top );
                    // (No need to account for margin-top, as we pushed vertical margin
                    // just above if there were floats.)

                int final_h = enode->renderFinalBlock( txform, &fmt, inner_width, &float_footprint );
                int final_min_y = float_footprint.getFinalMinY();
                int final_max_y = float_footprint.getFinalMaxY();

                flow->getPageContext()->updateRenderProgress(1);
                #ifdef DEBUG_DUMP_ENABLED
                    logfile << "\n";
                #endif

                int pad_style_h = 0;
                if (style_h >= 0) { // computed above
                    int pad_h = style_h - (final_h + padding_top + padding_bottom);
                    if (pad_h > 0) {
                        // don't pad more than one page height
                        if (pad_h > flow->getPageHeight())
                            pad_h = flow->getPageHeight();
                        pad_style_h = pad_h; // to be context.AddLine() below
                    }
                }

                int h = padding_top + final_h + pad_style_h + padding_bottom;
                final_min_y += padding_top;
                final_max_y += padding_top;
                int top_overflow = final_min_y < 0 ? -final_min_y : 0;
                int bottom_overflow = final_max_y > h ? final_max_y - h : 0;
                // if (top_overflow > 0) printf("final top_overflow=%d\n", top_overflow);
                // if (bottom_overflow > 0) printf("final bottom_overflow=%d\n", bottom_overflow);

                // Reload fmt, as enode->renderFinalBlock() will have
                // updated it to store the float_footprint rects in.
                //   Note: beware RenderRectAccessor's own state and refresh
                //   management (push/_modified/_dirty) which may not be
                //   super safe (push() sets dirty only if modified,
                //   a getX() resets _dirty, and you can't set _dirty
                //   explicitely when you know a 2nd instance will be
                //   created and will modify the state).
                //   So, safer to create a new instance to be sure
                //   to get fresh data.
                fmt = RenderRectAccessor( enode );
                fmt.setHeight( h );
                fmt.setTopOverflow( top_overflow );
                fmt.setBottomOverflow( bottom_overflow );
                fmt.push();
                // (We set the height now because we know it, but it should be
                // equal to what we will addContentLine/addContentSpace below.)

                // We need to forward our overflow for it to be carried
                // by our block containers if we overflow them.
                flow->updateCurrentLevelTopOverflow(top_overflow);

                if (padding_top>0) {
                    // This may add accumulated margin
                    flow->addContentLine(padding_top, RN_SPLIT_AFTER_AVOID, 0, true);
                }

                // Enter footnote body after padding, to get rid of it
                // and have lean in-page footnotes
                if ( isFootNoteBody )
                    flow->getPageContext()->enterFootNote( footnoteId );

                // We have lines of text in 'txform', that we should register
                // into flow/context for later page splitting.
                int count = txform->GetLineCount();
                int orphans = (int)(style->orphans) - (int)(css_orphans_widows_1) + 1;
                int widows = (int)(style->widows) - (int)(css_orphans_widows_1) + 1;
                for (int i=0; i<count; i++) {
                    const formatted_line_t * line = txform->GetLineInfo(i);
                    int line_flags = 0;

                    // We let the first line with allow split before,
                    // and the last line with allow split after (padding
                    // top and bottom will too, but will themselves stick
                    // to the first and last lines).
                    // flow->addContentLine() may change that depending on
                    // surroundings.

                    // Honor widows and orphans
                    if (orphans > 1 && i > 0 && i < orphans)
                        // with orphans:2, and we're the 2nd line (i=1), avoid split before
                        // so we stick to first line
                        line_flags |= RN_SPLIT_BEFORE_AVOID;
                    if (widows > 1 && i < count-1 && count-1 - i < widows)
                        // with widows:2, and we're the last before last line (i=count-2),
                        // avoid split after so we stick to last line
                        line_flags |= RN_SPLIT_AFTER_AVOID;

                    // Honor line's own flags (used when filling space when
                    // clearing floats)
                    if (line->flags & LTEXT_LINE_SPLIT_AVOID_BEFORE)
                        line_flags |= RN_SPLIT_BEFORE_AVOID;
                    if (line->flags & LTEXT_LINE_SPLIT_AVOID_AFTER)
                        line_flags |= RN_SPLIT_AFTER_AVOID;

                    // Honor our own "page-break-inside: avoid" that hasn't been
                    // passed to "flow" (any upper "break-inside: avoid" will be
                    // enforced by flow->addContentLine())
                    if ( break_inside == RN_SPLIT_AVOID ) {
                        if (i > 0)
                            line_flags |= RN_SPLIT_BEFORE_AVOID;
                        if (i < count-1)
                            line_flags |= RN_SPLIT_AFTER_AVOID;
                    }

                    flow->addContentLine(line->height, line_flags, line->baseline);

                    // See if there are links to footnotes in that line, and add
                    // a reference to it so page splitting can bring the footnotes
                    // text on this page, and then decide about page split.
                    if ( !isFootNoteBody && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) { // disable footnotes for footnotes
                        // If paragraph is RTL, we are meeting words in the reverse of the reading order:
                        // so, insert each link for this line at the same position, instead of at the end.
                        int link_insert_pos = -1; // append
                        if ( line->flags & LTEXT_LINE_PARA_IS_RTL ) {
                            link_insert_pos = flow->getPageContext()->getCurrentLinksCount();
                        }
                        for ( int w=0; w<line->word_count; w++ ) {
                            // check link start flag for every word
                            if ( line->words[w].flags & LTEXT_WORD_IS_LINK_START ) {
                                const src_text_fragment_t * src = txform->GetSrcInfo( line->words[w].src_text_index );
                                if ( src && src->object ) {
                                    ldomNode * node = (ldomNode*)src->object;
                                    ldomNode * parent = node->getParentNode();
                                    while (parent && parent->getNodeId() != el_a)
                                        parent = parent->getParentNode();
                                    if ( parent && parent->hasAttribute(LXML_NS_ANY, attr_href ) ) {
                                            // was: && parent->getAttributeValue(LXML_NS_ANY, attr_type ) == "note")
                                            // but we want to be able to gather in-page footnotes by only
                                            // specifying a -cr-hint: to the footnote target, with no need
                                            // to set one to the link itself
                                        lString16 href = parent->getAttributeValue(LXML_NS_ANY, attr_href );
                                        if ( href.length()>0 && href.at(0)=='#' ) {
                                            href.erase(0,1);
                                            flow->getPageContext()->addLink( href, link_insert_pos );
                                        }
                                    }
                                }
                            }
                        }
                        // Also add links gathered by floats in their content
                        int fcount = txform->GetFloatCount();
                        for (int f=0; f<fcount; f++) {
                            const embedded_float_t * flt = txform->GetFloatInfo(f);
                            if ( i < count-1 ) { // Unless we're the last line:
                                // ignore this float if it ends after currently processed line
                                if ( flt->y + flt->height > line->y + line->height ) {
                                    continue;
                                }
                            }
                            if ( flt->links && flt->links->length() > 0 ) {
                                for ( int n=0; n<flt->links->length(); n++ ) {
                                    flow->getPageContext()->addLink( flt->links->at(n) );
                                }
                                flt->links->clear(); // don't reprocess them if float met again with next lines
                            }
                        }
                    }
                }

                // Leave footnote body before style height and padding, to get
                // rid of them and have lean in-page footnotes
                if ( isFootNoteBody )
                    flow->getPageContext()->leaveFootNote();

                if( pad_style_h > 0) {
                    // Add filling space to the page splitting context
                    // Allow page splitting inside that useless excessive style height
                    flow->addContentSpace(pad_style_h, 1, false, false, false);
                }

                if (padding_bottom>0) {
                    flow->addContentLine(padding_bottom, RN_SPLIT_BEFORE_AVOID, 0, true);
                }

                // We need to forward our overflow for it to be carried
                // by our block containers if we overflow them.
                flow->updateCurrentLevelBottomOverflow(bottom_overflow);

                flow->addVerticalMargin( enode, margin_bottom, break_after );
                if ( no_margin_collapse ) {
                    // Push our margin so it does not get collapsed with some later one
                    flow->pushVerticalMargin();
                }
                return;
            }
            break;
        case erm_killed:
            {
                // DrawDocument will render a small figure in this rect area
                fmt.setHeight( 15 ); // not squared, so it does not look
                fmt.setWidth( 10 );  // like a list square bullet
                // Let it be at the x/y decided above
                flow->addContentLine( fmt.getHeight(), 0, fmt.getHeight() );
                return;
            }
            break;
        default:
            CRLog::error("Unsupported render method %d", m);
            crFatalError(141, "Unsupported render method"); // error
            break;
    }
    return;
}

// Entry points for rendering the root node, a table cell or a float
int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width, int direction, int * baseline, int rend_flags )
{
    if ( BLOCK_RENDERING(rend_flags, ENHANCED) ) {
        // Create a flow state (aka "block formatting context") for the rendering
        // of this block and all its children.
        // (We are called when rendering the root node, and when rendering each float
        // met along walking the root node hierarchy - and when meeting a new float
        // in a float, etc...)
        FlowState flow( context, width, rend_flags, direction, TextLangMan::getLangNodeIndex(enode) );
        if (baseline != NULL) {
            flow.setRequestedBaselineType(*baseline);
        }
        renderBlockElementEnhanced( &flow, enode, x, width, rend_flags );
        if (baseline != NULL) {
            // (We pass the top node, so it can find the first table row
            // if needed with inline-table.)
            *baseline = flow.getBaselineAbsoluteY(enode);
        }
        // The block height is c_y when we are done
        return flow.getCurrentAbsoluteY();
    } else {
        // (Legacy rendering does not support direction)
        return renderBlockElementLegacy( context, enode, x, y, width);
    }
}
int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width, int direction, int * baseline )
{
    // Use global rendering flags
    // Note: we're not currently using it with other flags that the global ones.
    return renderBlockElement( context, enode, x, y, width, direction, baseline, gRenderBlockRenderingFlags );
}

//draw border lines,support color,width,all styles, not support border-collapse
void DrawBorder(ldomNode *enode,LVDrawBuf & drawbuf,int x0,int y0,int doc_x,int doc_y,RenderRectAccessor fmt)
{
    css_style_rec_t * style = enode->getStyle().get();
    bool hastopBorder = (style->border_style_top >=css_border_solid&&style->border_style_top<=css_border_outset);
    bool hasrightBorder = (style->border_style_right >=css_border_solid&&style->border_style_right<=css_border_outset);
    bool hasbottomBorder = (style->border_style_bottom >=css_border_solid&&style->border_style_bottom<=css_border_outset);
    bool hasleftBorder = (style->border_style_left >=css_border_solid&&style->border_style_left<=css_border_outset);

    // Check for explicit 'border-width: 0' which means no border.
    css_length_t bw;
    bw = style->border_width[0];
    hastopBorder = hastopBorder & !(bw.value == 0 && bw.type > css_val_unspecified);
    bw = style->border_width[1];
    hasrightBorder = hasrightBorder & !(bw.value == 0 && bw.type > css_val_unspecified);
    bw = style->border_width[2];
    hasbottomBorder = hasbottomBorder & !(bw.value == 0 && bw.type > css_val_unspecified);
    bw = style->border_width[3];
    hasleftBorder = hasleftBorder & !(bw.value == 0 && bw.type > css_val_unspecified);

    // Check for explicit 'border-color: transparent' which means no border to draw
    css_length_t bc;
    bc = style->border_color[0];
    hastopBorder = hastopBorder & !(bc.type == css_val_unspecified && bc.value == css_generic_transparent);
    bc = style->border_color[1];
    hasrightBorder = hasrightBorder & !(bc.type == css_val_unspecified && bc.value == css_generic_transparent);
    bc = style->border_color[2];
    hasbottomBorder = hasbottomBorder & !(bc.type == css_val_unspecified && bc.value == css_generic_transparent);
    bc = style->border_color[3];
    hasleftBorder = hasleftBorder & !(bc.type == css_val_unspecified && bc.value == css_generic_transparent);

    if (hasbottomBorder || hasleftBorder || hasrightBorder || hastopBorder) {
        lUInt32 shadecolor=0x555555;
        lUInt32 lightcolor=0xAAAAAA;
        int em = enode->getFont()->getSize();
        int width = 0; // values in % are invalid for borders, so we shouldn't get any
        int topBorderwidth = lengthToPx(style->border_width[0],width,em);
        topBorderwidth = topBorderwidth!=0 ? topBorderwidth : DEFAULT_BORDER_WIDTH;
        int rightBorderwidth = lengthToPx(style->border_width[1],width,em);
        rightBorderwidth = rightBorderwidth!=0 ? rightBorderwidth : DEFAULT_BORDER_WIDTH;
        int bottomBorderwidth = lengthToPx(style->border_width[2],width,em);
        bottomBorderwidth = bottomBorderwidth!=0 ? bottomBorderwidth : DEFAULT_BORDER_WIDTH;
        int leftBorderwidth = lengthToPx(style->border_width[3],width,em);
        leftBorderwidth = leftBorderwidth!=0 ? leftBorderwidth : DEFAULT_BORDER_WIDTH;
        int tbw=topBorderwidth,rbw=rightBorderwidth,bbw=bottomBorderwidth,lbw=leftBorderwidth;
        if (hastopBorder) {
            int dot=1,interval=0;//default style
            lUInt32 topBordercolor = style->border_color[0].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (style->border_color[0].type==css_val_color)
            {
                lUInt32 r,g,b;
                r=g=b=topBordercolor;
                r=r>>16;
                g=g>>8&0xff;
                b=b&0xff;
                shadecolor=(r*160/255)<<16|(g*160/255)<<8|b*160/255;
                lightcolor=topBordercolor;
            }
            int left=1,right=1;
            left=(hasleftBorder)?0:1;
            right=(hasrightBorder)?0:1;
            left=(style->border_style_left==css_border_dotted||style->border_style_left==css_border_dashed)?0:left;
            right=(style->border_style_right==css_border_dotted||style->border_style_right==css_border_dashed)?0:right;
            lvPoint leftpoint1=lvPoint(x0+doc_x,y0+doc_y),
                    leftpoint2=lvPoint(x0+doc_x,y0+doc_y+0.5*topBorderwidth),
                    leftpoint3=lvPoint(x0+doc_x,doc_y+y0+topBorderwidth),
                    rightpoint1=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0),
                    rightpoint2=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0+0.5*topBorderwidth),
                    rightpoint3=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0+topBorderwidth);
            double leftrate=1,rightrate=1;
            if (left==0) {
                leftpoint1.x=x0+doc_x;
                leftpoint1.y=doc_y+y0;
                leftpoint2.x=x0+doc_x+0.5*leftBorderwidth;
                leftpoint2.y=doc_y+y0+0.5*topBorderwidth;
                leftpoint3.x=x0+doc_x+leftBorderwidth;
                leftpoint3.y=doc_y+y0+topBorderwidth;
            }else leftBorderwidth=0;
            leftrate=(double)leftBorderwidth/(double)topBorderwidth;
            if (right==0) {
                rightpoint1.x=x0+doc_x+fmt.getWidth()-1;
                rightpoint1.y=doc_y+y0;
                rightpoint2.x=x0+doc_x+fmt.getWidth()-1-0.5*rightBorderwidth;
                rightpoint2.y=doc_y+y0+0.5*topBorderwidth;
                rightpoint3.x=x0+doc_x+fmt.getWidth()-1-rightBorderwidth;
                rightpoint3.y=doc_y+y0+topBorderwidth;
            } else rightBorderwidth=0;
            rightrate=(double)rightBorderwidth/(double)topBorderwidth;
            switch (style->border_style_top){
                case css_border_dotted:
                    dot=interval=topBorderwidth;
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, topBordercolor,dot,interval,0);}
                    break;
                case css_border_dashed:
                    dot=3*topBorderwidth;
                    interval=3*topBorderwidth;
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, topBordercolor,dot,interval,0);}
                    break;
                case css_border_solid:
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, topBordercolor,dot,interval,0);}
                    break;
                case css_border_double:
                    for(int i=0;i<=(leftpoint2.y-leftpoint1.y)/(leftpoint2.y-leftpoint1.y>2?3:2);i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, topBordercolor);}
                    for(int i=0;i<=(leftpoint3.y-leftpoint2.y)/(leftpoint3.y-leftpoint2.y>2?3:2);i++)
                    {drawbuf.FillRect(leftpoint3.x-i*leftrate, leftpoint3.y-i, rightpoint3.x+i*rightrate+1,
                                      rightpoint3.y-i+1, topBordercolor);}
                    break;
                case css_border_groove:
                    for(int i=0;i<=leftpoint2.y-leftpoint1.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, shadecolor);}
                    for(int i=0;i<leftpoint3.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y+i, rightpoint2.x-i*rightrate+1,
                                      rightpoint2.y+i+1, lightcolor);}
                    break;
                case css_border_inset:
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, shadecolor,dot,interval,0);}
                    break;
                case css_border_outset:
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y+i+1, lightcolor,dot,interval,0);}
                    break;
                case css_border_ridge:
                    for(int i=0;i<=leftpoint2.y-leftpoint1.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate+1,
                                     rightpoint1.y+i+1, lightcolor);}
                    for(int i=0;i<leftpoint3.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y+i, rightpoint2.x-i*rightrate+1,
                                      rightpoint2.y+i+1, shadecolor);}
                    break;
                default:
                    break;
            }
        }
        //right
        if (hasrightBorder) {
            int dot=1,interval=0;//default style
            lUInt32 rightBordercolor = style->border_color[1].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (style->border_color[1].type==css_val_color)
            {
                lUInt32 r,g,b;
                r=g=b=rightBordercolor;
                r=r>>16;
                g=g>>8&0xff;
                b=b&0xff;
                shadecolor=(r*160/255)<<16|(g*160/255)<<8|b*160/255;
                lightcolor=rightBordercolor;
            }
            int up=1,down=1;
            up=(hastopBorder)?0:1;
            down=(hasbottomBorder)?0:1;
            up=(style->border_style_top==css_border_dotted||style->border_style_top==css_border_dashed)?1:up;
            down=(style->border_style_bottom==css_border_dotted||style->border_style_bottom==css_border_dashed)?1:down;
            lvPoint toppoint1=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0),
                    toppoint2=lvPoint(x0+doc_x+fmt.getWidth()-1-0.5*rightBorderwidth,doc_y+y0),
                    toppoint3=lvPoint(x0+doc_x+fmt.getWidth()-1-rightBorderwidth,doc_y+y0),
                    bottompoint1=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0+fmt.getHeight()-1),
                    bottompoint2=lvPoint(x0+doc_x+fmt.getWidth()-1-0.5*rightBorderwidth,doc_y+y0+fmt.getHeight()-1),
                    bottompoint3=lvPoint(x0+doc_x+fmt.getWidth()-1-rightBorderwidth,doc_y+y0+fmt.getHeight()-1);
            double toprate=1,bottomrate=1;
            if (up==0) {
                toppoint3.y=doc_y+y0+topBorderwidth;
                toppoint2.y=doc_y+y0+0.5*topBorderwidth;
            } else topBorderwidth=0;
            toprate=(double)topBorderwidth/(double)rightBorderwidth;
            if (down==0) {
                bottompoint3.y=y0+doc_y+fmt.getHeight()-1-bottomBorderwidth;
                bottompoint2.y=y0+doc_y+fmt.getHeight()-1-0.5*bottomBorderwidth;
            } else bottomBorderwidth=0;
            bottomrate=(double)bottomBorderwidth/(double)rightBorderwidth;
            switch (style->border_style_right){
                case css_border_dotted:
                    dot=interval=rightBorderwidth;
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, rightBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_dashed:
                    dot=3*rightBorderwidth;
                    interval=3*rightBorderwidth;
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, rightBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_solid:
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, rightBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_double:
                    for (int i=0;i<=(toppoint1.x-toppoint2.x)/(toppoint1.x-toppoint2.x>2?3:2);i++){
                        drawbuf.FillRect(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, rightBordercolor);
                    }
                    for (int i=0;i<=(toppoint2.x-toppoint3.x)/(toppoint2.x-toppoint3.x>2?3:2);i++){
                        drawbuf.FillRect(toppoint3.x+i,toppoint3.y-i*toprate,bottompoint3.x+i+1,
                                         bottompoint3.y+i*bottomrate+1, rightBordercolor);
                    }
                    break;
                case css_border_groove:
                    for (int i=0;i<toppoint1.x-toppoint2.x;i++){
                        drawbuf.FillRect(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, lightcolor);
                    }
                    for (int i=0;i<=toppoint2.x-toppoint3.x;i++){
                        drawbuf.FillRect(toppoint2.x-i,toppoint2.y+i*toprate,bottompoint2.x-i+1,
                                         bottompoint2.y-i*bottomrate+1, shadecolor);
                    }
                    break;
                case css_border_inset:
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, lightcolor,dot,interval,1);
                    }
                    break;
                case css_border_outset:
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, shadecolor,dot,interval,1);
                    }
                    break;
                case css_border_ridge:
                    for (int i=0;i<toppoint1.x-toppoint2.x;i++){
                        drawbuf.FillRect(toppoint1.x-i,toppoint1.y+i*toprate,bottompoint1.x-i+1,
                                         bottompoint1.y-i*bottomrate+1, shadecolor);
                    }
                    for (int i=0;i<=toppoint2.x-toppoint3.x;i++){
                        drawbuf.FillRect(toppoint2.x-i,toppoint2.y+i*toprate,bottompoint2.x-i+1,
                                         bottompoint2.y-i*bottomrate+1,lightcolor);
                    }
                    break;
                default:break;
            }
        }
        //bottom
        if (hasbottomBorder) {
            int dot=1,interval=0;//default style
            lUInt32 bottomBordercolor = style->border_color[2].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (style->border_color[2].type==css_val_color)
            {
                lUInt32 r,g,b;
                r=g=b=bottomBordercolor;
                r=r>>16;
                g=g>>8&0xff;
                b=b&0xff;
                shadecolor=(r*160/255)<<16|(g*160/255)<<8|b*160/255;
                lightcolor=bottomBordercolor;
            }
            int left=1,right=1;
            left=(hasleftBorder)?0:1;
            right=(hasrightBorder)?0:1;
            left=(style->border_style_left==css_border_dotted||style->border_style_left==css_border_dashed)?1:left;
            right=(style->border_style_right==css_border_dotted||style->border_style_right==css_border_dashed)?1:right;
            lvPoint leftpoint1=lvPoint(x0+doc_x,y0+doc_y+fmt.getHeight()-1),
                    leftpoint2=lvPoint(x0+doc_x,y0+doc_y-0.5*bottomBorderwidth+fmt.getHeight()-1),
                    leftpoint3=lvPoint(x0+doc_x,doc_y+y0+fmt.getHeight()-1-bottomBorderwidth),
                    rightpoint1=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0+fmt.getHeight()-1),
                    rightpoint2=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0+fmt.getHeight()-1-0.5*bottomBorderwidth),
                    rightpoint3=lvPoint(x0+doc_x+fmt.getWidth()-1,doc_y+y0+fmt.getHeight()-1-bottomBorderwidth);
            double leftrate=1,rightrate=1;
            if (left==0) {
                leftpoint3.x=x0+doc_x+leftBorderwidth;
                leftpoint2.x=x0+doc_x+0.5*leftBorderwidth;
            }else leftBorderwidth=0;
            leftrate=(double)leftBorderwidth/(double)bottomBorderwidth;
            if (right==0) {
                rightpoint3.x=x0+doc_x+fmt.getWidth()-1-rightBorderwidth;
                rightpoint2.x=x0+doc_x+fmt.getWidth()-1-0.5*rightBorderwidth;
            } else rightBorderwidth=0;
            rightrate=(double)rightBorderwidth/(double)bottomBorderwidth;
            switch (style->border_style_bottom){
                case css_border_dotted:
                    dot=interval=bottomBorderwidth;
                    for(int i=0;i<leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, bottomBordercolor,dot,interval,0);}
                    break;
                case css_border_dashed:
                    dot=3*bottomBorderwidth;
                    interval=3*bottomBorderwidth;
                    for(int i=0;i<leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, bottomBordercolor,dot,interval,0);}
                    break;
                case css_border_solid:
                    for(int i=0;i<leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, bottomBordercolor,dot,interval,0);}
                    break;
                case css_border_double:
                    for(int i=0;i<=(leftpoint1.y-leftpoint2.y)/(leftpoint1.y-leftpoint2.y>2?3:2);i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, bottomBordercolor);}
                    for(int i=0;i<=(leftpoint2.y-leftpoint3.y)/(leftpoint2.y-leftpoint3.y>2?3:2);i++)
                    {drawbuf.FillRect(leftpoint3.x-i*leftrate, leftpoint3.y+i, rightpoint3.x+i*rightrate+1,
                                      rightpoint3.y+i+1, bottomBordercolor);}
                    break;
                case css_border_groove:
                    for(int i=0;i<=leftpoint1.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, lightcolor);}
                    for(int i=0;i<leftpoint2.y-leftpoint3.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y-i, rightpoint2.x-i*rightrate+1,
                                      rightpoint2.y-i+1, shadecolor);}
                    break;
                case css_border_inset:
                    for(int i=0;i<=leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, lightcolor,dot,interval,0);}
                    break;
                case css_border_outset:
                    for(int i=0;i<=leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, shadecolor,dot,interval,0);}
                    break;
                case css_border_ridge:
                    for(int i=0;i<=leftpoint1.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate+1,
                                      rightpoint1.y-i+1, shadecolor);}
                    for(int i=0;i<leftpoint2.y-leftpoint3.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y-i, rightpoint2.x-i*rightrate+1,
                                      rightpoint2.y-i+1, lightcolor);}
                    break;
                default:break;
            }
        }
        //left
        if (hasleftBorder) {
            int dot=1,interval=0;//default style
            lUInt32 leftBordercolor = style->border_color[3].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (style->border_color[3].type==css_val_color)
            {
                lUInt32 r,g,b;
                r=g=b=leftBordercolor;
                r=r>>16;
                g=g>>8&0xff;
                b=b&0xff;
                shadecolor=(r*160/255)<<16|(g*160/255)<<8|b*160/255;
                lightcolor=leftBordercolor;
            }
            int up=1,down=1;
            up=(hastopBorder)?0:1;
            down=(hasbottomBorder)?0:1;
            up=(style->border_style_top==css_border_dotted||style->border_style_top==css_border_dashed)?1:up;
            down=(style->border_style_bottom==css_border_dotted||style->border_style_bottom==css_border_dashed)?1:down;
            lvPoint toppoint1=lvPoint(x0+doc_x,doc_y+y0),
                    toppoint2=lvPoint(x0+doc_x+0.5*leftBorderwidth,doc_y+y0),
                    toppoint3=lvPoint(x0+doc_x+leftBorderwidth,doc_y+y0),
                    bottompoint1=lvPoint(x0+doc_x,doc_y+y0+fmt.getHeight()-1),
                    bottompoint2=lvPoint(x0+doc_x+0.5*leftBorderwidth,doc_y+y0+fmt.getHeight()-1),
                    bottompoint3=lvPoint(x0+doc_x+leftBorderwidth,doc_y+y0+fmt.getHeight()-1);
            double toprate=1,bottomrate=1;
            if (up==0) {
                toppoint3.y=doc_y+y0+topBorderwidth;
                toppoint2.y=doc_y+y0+0.5*topBorderwidth;
            } else topBorderwidth=0;
            toprate=(double)topBorderwidth/(double)leftBorderwidth;
            if (down==0) {
                bottompoint3.y=y0+doc_y+fmt.getHeight()-1-bottomBorderwidth;
                bottompoint2.y=y0+doc_y+fmt.getHeight()-1-0.5*bottomBorderwidth;
            } else bottomBorderwidth=0;
            bottomrate=(double)bottomBorderwidth/(double)leftBorderwidth;
            switch (style->border_style_left){
                case css_border_dotted:
                    dot=interval=leftBorderwidth;
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,leftBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_dashed:
                    dot=3*leftBorderwidth;
                    interval=3*leftBorderwidth;
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,leftBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_solid:
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,leftBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_double:
                    for (int i=0;i<=(toppoint2.x-toppoint1.x)/(toppoint2.x-toppoint1.x>2?3:2);i++){
                        drawbuf.FillRect(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,leftBordercolor);
                    }
                    for (int i=0;i<=(toppoint3.x-toppoint2.x)/(toppoint3.x-toppoint2.x>2?3:2);i++){
                        drawbuf.FillRect(toppoint3.x-i,toppoint3.y-i*toprate,bottompoint3.x-i+1,
                                         bottompoint3.y+i*bottomrate+1,leftBordercolor);
                    }
                    break;
                case css_border_groove:
                    for (int i=0;i<=toppoint2.x-toppoint1.x;i++){
                        drawbuf.FillRect(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,shadecolor);
                    }
                    for (int i=0;i<toppoint3.x-toppoint2.x;i++){
                        drawbuf.FillRect(toppoint2.x+i,toppoint2.y+i*toprate,bottompoint2.x+i+1,
                                         bottompoint2.y-i*bottomrate+1,lightcolor);
                    }
                    break;
                case css_border_inset:
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,shadecolor,dot,interval,1);
                    }
                    break;
                case css_border_outset:
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,lightcolor,dot,interval,1);
                    }
                    break;
                case css_border_ridge:
                    for (int i=0;i<=toppoint2.x-toppoint1.x;i++){
                        drawbuf.FillRect(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate+1,lightcolor);
                    }
                    for (int i=0;i<toppoint3.x-toppoint2.x;i++){
                        drawbuf.FillRect(toppoint2.x+i,toppoint2.y+i*toprate,bottompoint2.x+i+1,
                                         bottompoint2.y-i*bottomrate+1,shadecolor);
                    }
                    break;
                default:break;
            }
        }
    }
}
void DrawBackgroundImage(ldomNode *enode,LVDrawBuf & drawbuf,int x0,int y0,int doc_x,int doc_y, int width, int height)
{
    // (The provided width and height gives the area we have to draw the background image on)
    css_style_ref_t style=enode->getStyle();
    if (!style->background_image.empty()) {
        lString16 filepath = lString16(style->background_image.c_str());
        LVImageSourceRef img = enode->getParentNode()->getDocument()->getObjectImageSource(filepath);
        if (!img.isNull()) {
            int img_w =img->GetWidth();
            int img_h =img->GetHeight();
            // We can use some crengine facilities for background repetition and position,
            // which has the advantage that img will be decoded once even if tiling it many
            // times and if the target is many screen-heights long (like <BODY> could be).
            // Unfortunaly, it does not everything well when not using IMG_TRANSFORM_TILE,
            // as it would fill the not-drawn part of the target buffer with garbage,
            // instead of letting it as is.
            ImageTransform hori_transform = IMG_TRANSFORM_NONE;
            ImageTransform vert_transform = IMG_TRANSFORM_NONE;
            int transform_w = img_w;
            int transform_h = img_h;
            switch (style->background_repeat) {
                case css_background_no_repeat:
                case css_background_repeat_y:
                    break;
                case css_background_repeat_x:
                case css_background_repeat:
                default:
                    // No need to tile if image is larger than target
                    if ( width > img_w ) {
                        hori_transform = IMG_TRANSFORM_TILE;
                        transform_w = width;
                    }
                    break;
            }
            switch (style->background_repeat) {
                case css_background_no_repeat:
                case css_background_repeat_x:
                    break;
                case css_background_repeat_y:
                case css_background_repeat:
                default:
                    // No need to tile if image is larger than target
                    if ( height > img_h ) {
                        vert_transform = IMG_TRANSFORM_TILE;
                        transform_h = height;
                    }
                    break;
            }
            // Compute the position where to draw top left of image, as if
            // it was a single image when no-repeat
            int draw_x = 0;
            int draw_y = 0;
            switch (style->background_position) {
                case css_background_left_top:
                case css_background_left_center:
                case css_background_left_bottom:
                    break;
                case css_background_center_top:
                case css_background_center_center:
                case css_background_center_bottom:
                    draw_x = (width - img_w)/2;
                    break;
                case css_background_right_top:
                case css_background_right_center:
                case css_background_right_bottom:
                    draw_x = width - img_w;
                    break;
                default:
                    break;
            }
            switch (style->background_position) {
                case css_background_left_top:
                case css_background_center_top:
                case css_background_right_top:
                    break;
                case css_background_left_center:
                case css_background_center_center:
                case css_background_right_center:
                    draw_y = (height - img_h)/2;
                    break;
                case css_background_left_bottom:
                case css_background_center_bottom:
                case css_background_right_bottom:
                    draw_y = height - img_h;
                    break;
                default:
                    break;
            }
            // If tiling, we need to adjust the transform x/y (the offset
            // in img, so, a value between 0 and img_w/h) to the point
            // inside image that should be at top left of target area
            int transform_x = 0;
            int transform_y = 0;
            if ( hori_transform == IMG_TRANSFORM_TILE && draw_x ) {
                transform_x = (draw_x % img_w);
                draw_x = 0;
            }
            if ( vert_transform == IMG_TRANSFORM_TILE && draw_y ) {
                // Strangely, using the following instead of what we did for x/w
                // gives the expected result (not investigated, might be
                // a bug in LVStretchImgSource::OnLineDecoded() )
                transform_y = img_h - (draw_y % img_h);
                draw_y = 0;
            }
            // Ready to have crengine do all the work.
            // (Inspired from LVDocView::drawPageBackground(), we have to do it that complex
            // way to avoid memory leaks; and we have to use a 16bpp LVColorDrawBuf,
            // 32bpp would mess colors up).
            LVRef<LVColorDrawBuf> buf = LVRef<LVColorDrawBuf>( new LVColorDrawBuf(img_w, img_h, 16) );
            buf->Draw(img, 0, 0, img_w, img_h, false); // (dither=false doesn't matter with a color buffer)
            LVImageSourceRef src = LVCreateDrawBufImageSource(buf.get(), false);
            LVImageSourceRef transformed = LVCreateStretchFilledTransform(src, transform_w, transform_h,
                                               hori_transform, vert_transform, transform_x, transform_y);
            // We use the DrawBuf clip facility to ensure we don't draw outside this node fmt
            lvRect orig_clip;
            drawbuf.GetClipRect( &orig_clip ); // Backup the original one
            // Set a new one to the target area
            lvRect target_clip = lvRect(x0+doc_x, y0+doc_y, x0+doc_x+width, y0+doc_y+height);;
            // But don't overflow page top and bottom, in case target spans multiple pages
            if ( target_clip.top < orig_clip.top )
                target_clip.top = orig_clip.top;
            if ( target_clip.bottom > orig_clip.bottom )
                target_clip.bottom = orig_clip.bottom;
            drawbuf.SetClipRect( &target_clip );
            // Draw
            drawbuf.Draw(transformed, x0+doc_x+draw_x, y0+doc_y+draw_y, transform_w, transform_h);
            drawbuf.SetClipRect( &orig_clip ); // Restore the original one
        }
    }
}

//=======================================================================
// Draw document
//=======================================================================
// Recursively called as children nodes are walked.
// x0, y0 are coordinates of top left point to draw to in buffer
// dx, dy are width and height to draw to in buffer
// doc_x, doc_y are offset coordinates in document:
//   doc_x is initially 0, and doc_y is set to a negative
//   value (- page.start) from the y of the top of the page
//   (in the whole book height) we want to start showing
void DrawDocument( LVDrawBuf & drawbuf, ldomNode * enode, int x0, int y0, int dx, int dy,
                    int doc_x, int doc_y, int page_height, ldomMarkedRangeList * marks,
                    ldomMarkedRangeList *bookmarks, bool draw_content, bool draw_background )
{
    // Because of possible floats overflowing their block container box, that could
    // be drawn over the area of a next block, we may need to switch to two-steps drawing:
    // - first draw only the background of all block nodes and their
    //   children (excluding floats)
    // - then draw the content (border, text, images) without the background, and
    //   floats (with their background)
    // There may still be some issue when main content, some block floats, and some
    // embedded floats are mixed and some/all of them have backgrounds...
    if ( enode->isElement() )
    {
        RenderRectAccessor fmt( enode );
        doc_x += fmt.getX();
        doc_y += fmt.getY();
        lvdom_element_render_method rm = enode->getRendMethod();
        // A few things differ when done for TR, THEAD, TBODY and TFOOT
        // (erm_table_row_group, erm_table_header_group, erm_table_footer_group, erm_table_row)
        bool isTableRowLike = rm >= erm_table_row_group && rm <= erm_table_row;

        // Check if this node has content to be shown on viewport
        int height = fmt.getHeight();
        int top_overflow = fmt.getTopOverflow();
        int bottom_overflow = fmt.getBottomOverflow();
        if ( (doc_y + height + bottom_overflow <= 0 || doc_y - top_overflow >= 0 + dy) ) {
            // We don't have to draw this node.
            // Except TR which may have cells with rowspan>1, and even though
            // this TR is out of range, it must draw a rowspan>1 cell, so it
            // is not empty when a next TR (not out of range) is drawn (this
            // makes drawing multi-pages table slow).
            if ( !isTableRowLike ) {
                return; // out of range
            }
            if ( BLOCK_RENDERING_G(ENHANCED) ) {
                // But in enhanced mode, we have set bottom overflow on
                // TR and table row groups, so we can trust them.
                return; // out of range
            }
        }

        int direction = RENDER_RECT_GET_DIRECTION(fmt);
        bool is_rtl = direction == REND_DIRECTION_RTL; // shortcut for followup tests

        css_style_rec_t * style = enode->getStyle().get();

        // Check and draw background
        css_length_t bg = style->background_color;
        lUInt32 oldColor = 0;
        // Don't draw background color for TR and THEAD/TFOOT/TBODY as it could
        // override bgcolor of cells with rowspan > 1. We spread, in setNodeStyle(),
        // the TR bgcolor to its TDs that must have it, as it should be done (the
        // border spacing between cells does not have the bg color of the TR: only
        // cells have it).
        if ( bg.type==css_val_color && !isTableRowLike ) {
            // Even if we don't draw/fill background, we may need to
            // drawbuf.SetBackgroundColor() for the text to be correctly
            // drawn over this background color
            oldColor = drawbuf.GetBackgroundColor();
            drawbuf.SetBackgroundColor( bg.value );
            if ( draw_background )
                drawbuf.FillRect( x0 + doc_x, y0 + doc_y, x0 + doc_x+fmt.getWidth(), y0+doc_y+fmt.getHeight(), bg.value );
        }
        if ( draw_background && !style->background_image.empty() ) {
            if ( enode->getNodeId() == el_body ) {
                // CSS specific: <body> background does not obey margin rules
                // We don't draw on the fmt width, but on the drawbuf width.
                // Also, when in page mode, we'd rather have a fully fixed background,
                // (so, not respecting background-repeat and background-position)
                // to avoid ghosting and refreshes issues on eInk.
                // (This is not easy to ensure when there are multiple <BODY>, with
                // possibly different background images, in the viewed page. Drawing
                // a second background on the whole drawbuf would override the text
                // of the previous body.)
                lvRect curclip;
                drawbuf.GetClipRect( &curclip );
                // Cheap and possibly wrong (if 0-bottom margin and page just fits screen)
                // detection of page mode:
                bool is_page_mode = curclip.bottom != drawbuf.GetHeight();
                if ( is_page_mode && doc_y <= 0 ) {
                    // If doc_y > 0, we're a BODY starting on this page, and there
                    // may be a previous one that had already set the background
                    // image and draw text, that we'd rather not override with
                    // a new full screen background image.
                    // Remove the clip, so the background is drawn on the full page
                    // even on pages with shorter text.
                    drawbuf.SetClipRect(NULL);
                    DrawBackgroundImage(enode, drawbuf, 0, 0, 0, 0, drawbuf.GetWidth(), drawbuf.GetHeight());
                    drawbuf.SetClipRect(&curclip); // restore clip
                }
                else {
                    // No fixed background.
                    // This seems to work fine both in page and scroll modes, with proper
                    // drawing if multiple body with different backgrounds, with some
                    // occasional blank white space at DocFragment boundaries.
                    DrawBackgroundImage(enode, drawbuf, 0, y0, 0, doc_y, drawbuf.GetWidth(), fmt.getHeight());
                }
            }
            else { // regular drawing
                DrawBackgroundImage(enode, drawbuf, x0, y0, doc_x, doc_y, fmt.getWidth(), fmt.getHeight());
            }
            // (Commented identical calls below as they seem redundant with what was just done here)
        }
        #if (DEBUG_TREE_DRAW!=0)
            lUInt32 color;
            static lUInt32 const colors2[] = { 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA };
            static lUInt32 const colors4[] = { 0x555555, 0xFF4040, 0x40FF40, 0x4040FF, 0xAAAAAA, 0xFF8000, 0xC0C0C0, 0x808080 };
            if (drawbuf.GetBitsPerPixel()>=16)
                color = colors4[enode->getNodeLevel() & 7];
            else
                color = colors2[enode->getNodeLevel() & 7];
        #endif

        int m = enode->getRendMethod();
        // We should not get erm_inline, except for inlineBox elements, that
        // we must draw as erm_block
        if ( m == erm_inline && enode->isBoxingInlineBox() ) {
            m = erm_block;
        }
        switch( m )
        {
        case erm_table:
        case erm_table_row:
        case erm_table_row_group:
        case erm_table_header_group:
        case erm_table_footer_group:
        case erm_block:
            {
                // recursive draw all sub-blocks for blocks
                int cnt = enode->getChildCount();

                bool in_two_steps_drawing = true;
                if ( draw_content && draw_background )
                    in_two_steps_drawing = false;

                if ( in_two_steps_drawing && draw_background ) { // draw_content==false
                    // Recursively draw background only
                    for (int i=0; i<cnt; i++) {
                        ldomNode * child = enode->getChildNode( i );
                        // No need to draw early the background of floatboxes:
                        // it will be drawn with the content after non-floating
                        // content has been drawn below
                        if ( child->isFloatingBox() )
                            continue;
                        DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks, false, true );
                    }
                    // Cleanup and return
                    if ( bg.type==css_val_color ) {
                        drawbuf.SetBackgroundColor( oldColor );
                    }
                    return;
                }

                // When here, we are either drawing both content and background, or only content.

                // Draw borders before content, so inner content can bleed if necessary on
                // the border (some glyphs like 'J' at start or 'f' at end may be drawn
                // outside the text content box).
                // Don't draw border for TR TBODY... as their borders are never directly
                // rendered by Firefox (they are rendered only when border-collapse, when
                // they did collapse to the cell, and made out the cell border)
                if ( !isTableRowLike )
                    DrawBorder(enode,drawbuf,x0,y0,doc_x,doc_y,fmt);

                // List item marker drawing when css_d_list_item_block and list-style-position = outside
                // and list_item_block rendered as block (containing text and block elements)
                // Rendering hack (in renderAsListStylePositionInside(): not when text-align "right"
                // or "center", we treat it just as "inside", and drawing is managed by renderFinalBlock())
                if ( style->display == css_d_list_item_block && !renderAsListStylePositionInside(style, is_rtl) ) {
                    int width = fmt.getWidth();
                    int base_width = 0; // for padding_top in %
                    ldomNode * parent = enode->getParentNode();
                    if ( parent && !(parent->isNull()) ) {
                        RenderRectAccessor pfmt( parent );
                        base_width = pfmt.getWidth();
                    }
                    int em = enode->getFont()->getSize();
                    int padding_top = lengthToPx( style->padding[2], base_width, em ) + measureBorder(enode,0) + DEBUG_TREE_DRAW;
                    // We already adjusted all children blocks' left-padding and width in renderBlockElement(),
                    // we just need to draw the marker in the space we made
                    // But adjust the x to draw our marker if the first line of our
                    // first final children would start being drawn further because
                    // some outer floats are involved (as Calibre and Firefox do).
                    int shift_x = 0;
                    if ( BLOCK_RENDERING_G(ENHANCED) ) {
                        ldomNode * tmpnode = enode;
                        // Just look at each first descendant for a final child (we may find
                        // none and would have to look at next children, but well...)
                        while ( tmpnode && tmpnode->hasChildren() ) {
                            tmpnode = tmpnode->getChildNode( 0 );
                            if (tmpnode && tmpnode->getRendMethod() == erm_final) {
                                RenderRectAccessor tmpfmt( tmpnode );
                                if ( RENDER_RECT_HAS_FLAG(tmpfmt, INNER_FIELDS_SET) ) {
                                    int inner_width = tmpfmt.getInnerWidth();
                                    BlockFloatFootprint float_footprint;
                                    float_footprint.restore( tmpnode, inner_width );
                                    shift_x = float_footprint.getTopShiftX(inner_width, is_rtl);
                                }
                                break;
                            }
                        }
                    }

                    LFormattedTextRef txform( enode->getDocument()->createFormattedText() );
                    // If RTL, have the marker aligned to the right inside list_marker_width
                    lUInt32 txt_flags = is_rtl ? LTEXT_ALIGN_RIGHT : 0;
                    int list_marker_width;
                    lString16 marker = renderListItemMarker( enode, list_marker_width, txform.get(), -1, txt_flags);
                    lUInt32 h = txform->Format( (lUInt16)list_marker_width, (lUInt16)page_height, direction );
                    lvRect clip;
                    drawbuf.GetClipRect( &clip );
                    if (doc_y + y0 + h <= clip.bottom) { // draw only if marker fully fits on page
                        // In both LTR and RTL, for erm_block, we draw the marker inside 'width',
                        // (only the child elements got their width shrinked by list_marker_width).
                        if ( is_rtl ) {
                            txform->Draw( &drawbuf, doc_x+x0 + width + shift_x - list_marker_width, doc_y+y0 + padding_top );
                        }
                        else {
                            // (Don't shift by padding left, the list marker is outside padding left)
                            txform->Draw( &drawbuf, doc_x+x0 + shift_x, doc_y+y0 + padding_top );
                        }
                    }
                }

                // Draw first the non-floating nodes (as their background color would
                // otherwise be drawn over any floating node content drawn previously
                // (But if floats or their children have themselves some background,
                // and negative margins are involved, their background could be drawn
                // over non-floating text... but that's not easy to check...)
                bool has_floats = false;
                for (int i=0; i<cnt; i++) {
                    ldomNode * child = enode->getChildNode( i );
                    if ( child->isFloatingBox() ) {
                        has_floats = true;
                        // Floats can be drawn after non-floats no matter
                        // if we went two-steps or not.
                        continue;
                    }

                    if ( in_two_steps_drawing ) {
                        // If we are already in 2-steps drawing, drawing background
                        // first and then content is already taken care of by some
                        // upper node. So, no need to check if we need to switch,
                        // just draw the content.
                        DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks, true, false );
                        continue;
                    }

                    // If not yet in 2-steps drawing, we need to check if we
                    // have to do that 2-steps drawing ourselves.
                    RenderRectAccessor cfmt( child );
                    if ( cfmt.getBottomOverflow() == 0 ) {
                        // No bottom overflow: just draw both content and background
                        DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks, true, true );
                        continue;
                    }

                    // This child has content that overflows: we need to 2-steps draw
                    // it and its siblings up until all overflow is passed.
                    // printf("Starting 2-steps drawing at %d %s\n", cfmt.getY(),
                    //      UnicodeToLocal(ldomXPointer(child, 0).toString()).c_str());
                    int overflow_y = cfmt.getY() + cfmt.getHeight() + cfmt.getBottomOverflow();
                    int last_two_steps_drawn_node;
                    for (int j=i; j<cnt; j++) {
                        last_two_steps_drawn_node = j;
                        child = enode->getChildNode( j );
                        if ( child->isFloatingBox() ) {
                            has_floats = true;
                            continue;
                        }
                        // Draw backgrounds (recusively)
                        DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks, false, true );
                        cfmt = RenderRectAccessor( child );
                        int current_y = cfmt.getY() + cfmt.getHeight();
                        int this_overflow = cfmt.getBottomOverflow();
                        if ( current_y >= overflow_y && this_overflow == 0 ) {
                            // Overflow y passed by, and no more new overflow, we
                            // can switch back to 1-step drawing
                            // printf("Done with 2-steps drawing after %d %s\n", current_y,
                            //      UnicodeToLocal(ldomXPointer(child, 0).toString()).c_str());
                            break;
                        }
                        overflow_y = current_y + this_overflow;
                    }
                    // Now, draw the content of all these nodes we've just drawn the background of
                    for (int k=i; k<=last_two_steps_drawn_node; k++) {
                        child = enode->getChildNode( k );
                        if ( child->isFloatingBox() ) {
                            has_floats = true;
                            continue;
                        }
                        // Draw contents (recursively)
                        DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks, true, false );
                    }
                    // Go on with 1-step drawing
                    i = last_two_steps_drawn_node;
                }

                // Then draw over the floating nodes ignored in previous loop
                if (has_floats) {
                    for (int i=0; i<cnt; i++) {
                        ldomNode * child = enode->getChildNode( i );
                        if ( !child->isFloatingBox() )
                            continue;
                        // Draw both content and background for floats
                        DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks, true, true );
                    }
                }

                #if (DEBUG_TREE_DRAW!=0)
                    drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+fmt.getWidth(), doc_y+y0+1, color );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+1, doc_y+y0+fmt.getHeight(), color );
                    drawbuf.FillRect( doc_x+x0+fmt.getWidth()-1, doc_y+y0, doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), color );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0+fmt.getHeight()-1, doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), color );
                #endif
                // Border was previously drawn here, but has been moved above for earlier drawing.
            	}
            break;
        case erm_final:
            {
                // No sub-background drawing for erm_final (its background was
                // drawn above, before the switch())
                if ( !draw_content ) {
                    // Cleanup and return
                    if ( bg.type==css_val_color ) {
                        drawbuf.SetBackgroundColor( oldColor );
                    }
                    return;
                }

                // Draw borders before content, so inner content can bleed if necessary on
                // the border (some glyphs like 'J' at start or 'f' at end may be drawn
                // outside the text content box).
                DrawBorder(enode, drawbuf, x0, y0, doc_x, doc_y, fmt);

                // Get ready to create a LFormattedText with the correct content width
                // and position: we'll have it draw itself at the right coordinates.
                int width = fmt.getWidth();
                int inner_width;
                int padding_left;
                int padding_top;
                if ( RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET) ) { // enhanced rendering for erm_final nodes
                    // This flag is set only when in enhanced rendering mode, and only on erm_final nodes.
                    padding_left = fmt.getInnerX();
                    padding_top = fmt.getInnerY();
                    inner_width = fmt.getInnerWidth();
                }
                else { // legacy rendering
                    // Note: this computation is wrong for paddings in %, as they should
                    // apply against the parent container width, not this block width.
                    int em = enode->getFont()->getSize();
                    bool draw_padding_bg = true; //( enode->getRendMethod()==erm_final );
                    padding_left = !draw_padding_bg ? 0 : lengthToPx( style->padding[0], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,3);
                    int padding_right = !draw_padding_bg ? 0 : lengthToPx( style->padding[1], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,1);
                    padding_top = !draw_padding_bg ? 0 : lengthToPx( style->padding[2], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,0);
                    inner_width = width - padding_left - padding_right;
                }

                // List item marker drawing when css_d_list_item_block and list-style-position = outside
                // and list_item_block rendered as final (containing only text and inline elements)
                // Rendering hack (in renderAsListStylePositionInside(): not when text-align "right"
                // or "center", we treat it just as "inside", and drawing is managed by renderFinalBlock())
                if ( style->display == css_d_list_item_block && !renderAsListStylePositionInside(style, is_rtl) ) {
                    // We already adjusted our block X and width in renderBlockElement(),
                    // we just need to draw the marker in the space we made on the left of
                    // this node.
                    // But adjust the x to draw our marker if the first line of this
                    // final block would start being drawn further because some outer
                    // floats are involved (as Calibre and Firefox do).
                    BlockFloatFootprint float_footprint;
                    float_footprint.restore( enode, inner_width );
                    int shift_x = float_footprint.getTopShiftX(inner_width, is_rtl);

                    LFormattedTextRef txform( enode->getDocument()->createFormattedText() );

                    // If RTL, have the marker aligned to the right inside list_marker_width
                    lUInt32 txt_flags = is_rtl ? LTEXT_ALIGN_RIGHT : 0;
                    int list_marker_width;
                    lString16 marker = renderListItemMarker( enode, list_marker_width, txform.get(), -1, txt_flags);
                    lUInt32 h = txform->Format( (lUInt16)list_marker_width, (lUInt16)page_height, direction );
                    lvRect clip;
                    drawbuf.GetClipRect( &clip );
                    if (doc_y + y0 + h <= clip.bottom) { // draw only if marker fully fits on page
                        // In both LTR and RTL, for erm_final, we draw the marker outside 'width',
                        // as 'width' has already been shrinked by list_marker_width.
                        if ( is_rtl ) {
                            txform->Draw( &drawbuf, doc_x+x0 + width + shift_x, doc_y+y0 + padding_top, NULL, NULL );
                        }
                        else {
                            txform->Draw( &drawbuf, doc_x+x0 + shift_x - list_marker_width, doc_y+y0 + padding_top, NULL, NULL );
                        }
                    }
                    // Note: if there's a float on the left of the list item, we let
                    // the marker where it would be if there were no float, while Firefox
                    // would shift it by the float width. But for both, the marker may
                    // be hidden by/shown inside the float...
                    // Not obvious to do as Firefox, which draws it like if it has some
                    // negative padding from the text.
                }

                // draw whole node content as single formatted object
                LFormattedTextRef txform;
                enode->renderFinalBlock( txform, &fmt, inner_width );
                fmt.push();
                {
                    lvRect rc;
                    enode->getAbsRect( rc, true );
                    if ( !RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET) ) {
                        // In legacy mode, getAbsRect( ..., inner=true) did not have
                        // the inner geometry stored in fmt and computed. We need
                        // to correct it with paddings:
                        int em = enode->getFont()->getSize();
                        int padding_left = measureBorder(enode,3)+lengthToPx(enode->getStyle()->padding[0],rc.width(),em);
                        int padding_right = measureBorder(enode,1)+lengthToPx(enode->getStyle()->padding[1],rc.width(),em);
                        int padding_top = measureBorder(enode,0)+lengthToPx(enode->getStyle()->padding[2],rc.height(),em);
                        int padding_bottom = measureBorder(enode,2)+lengthToPx(enode->getStyle()->padding[3],rc.height(),em);
                        rc.top += padding_top;
                        rc.left += padding_left;
                        rc.right -= padding_right;
                        rc.bottom -= padding_bottom;
                    }
                    ldomMarkedRangeList *nbookmarks = NULL;
                    if ( bookmarks && bookmarks->length()) { // internal crengine bookmarked text highlights
                        nbookmarks = new ldomMarkedRangeList( bookmarks, rc );
                    }
                    if ( marks && marks->length() ) { // "native highlighting" of a selection in progress
                        // Keep marks that are part of the top and bottom overflows
                        lvRect crop_rc = lvRect(rc);
                        crop_rc.top -= fmt.getTopOverflow();
                        crop_rc.bottom += fmt.getBottomOverflow();
                        ldomMarkedRangeList nmarks( marks, rc, &crop_rc );
                        // DrawBackgroundImage(enode, drawbuf, x0, y0, doc_x, doc_y, fmt.getWidth(), fmt.getHeight());
                        // Draw regular text with currently marked highlights
                        txform->Draw( &drawbuf, doc_x+x0 + padding_left, doc_y+y0 + padding_top, &nmarks, nbookmarks );
                    } else {
                        // DrawBackgroundImage(enode, drawbuf, x0, y0, doc_x, doc_y, fmt.getWidth(), fmt.getHeight());
                        // Draw regular text, no marks
                        txform->Draw( &drawbuf, doc_x+x0 + padding_left, doc_y+y0 + padding_top, marks, nbookmarks );
                    }
                    if (nbookmarks)
                        delete nbookmarks;
                }
                #if (DEBUG_TREE_DRAW!=0)
                    drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+fmt.getWidth(), doc_y+y0+1, color );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+1, doc_y+y0+fmt.getHeight(), color );
                    drawbuf.FillRect( doc_x+x0+fmt.getWidth()-1, doc_y+y0, doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), color );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0+fmt.getHeight()-1, doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), color );
                #endif
                // Border was previously drawn here, but has been moved above for earlier drawing.
            }
            break;
        case erm_invisible:
            // don't draw invisible blocks
            break;
        case erm_killed:
            if ( !draw_content ) {
                if ( bg.type==css_val_color ) {
                    drawbuf.SetBackgroundColor( oldColor );
                }
                return;
            }
            //drawbuf.FillRect( x0 + doc_x, y0 + doc_y, x0 + doc_x+fmt.getWidth(), y0+doc_y+fmt.getHeight(), 0xFF0000 );
            // Draw something that does not look like a bullet
            // This should render in red something like: [\]
            drawbuf.RoundRect( x0 + doc_x, y0 + doc_y, x0 + doc_x+fmt.getWidth(), y0+doc_y+fmt.getHeight(),
                fmt.getWidth()/4, fmt.getWidth()/4, 0xFF0000, 0x9 );
            drawbuf.FillRect( x0 + doc_x + fmt.getWidth()/6, y0 + doc_y + fmt.getHeight()*3/8,
                x0 + doc_x + fmt.getWidth()*5/6, y0+doc_y+fmt.getHeight()*5/8, 0xFF0000 );
            break;
        default:
            break;
            //crFatalError(); // error
        }
        if ( bg.type==css_val_color ) {
            drawbuf.SetBackgroundColor( oldColor );
        }
    }
}

/* Not used anywhere: not updated for absolute length units and *256
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
*/

inline void spreadParent( css_length_t & val, css_length_t & parent_val, bool unspecified_is_inherited=true )
{
    if ( val.type == css_val_inherited || (val.type == css_val_unspecified && unspecified_is_inherited) )
        val = parent_val;
}

void setNodeStyle( ldomNode * enode, css_style_ref_t parent_style, LVFontRef parent_font )
{
    CR_UNUSED(parent_font);
    //lvdomElementFormatRec * fmt = node->getRenderData();
    css_style_ref_t style( new css_style_rec_t );
    css_style_rec_t * pstyle = style.get();

    lUInt16 nodeElementId = enode->getNodeId();

    if (gDOMVersionRequested < 20180524) {
        // The display property initial value has been changed from css_d_inherit
        // to css_d_inline (as per spec, and so that an unknown element does not
        // become block when contained in a P, and inline when contained in a SPAN)
        pstyle->display = css_d_inherit;
    }

//    if ( parent_style.isNull() ) {
//        CRLog::error("parent style is null!!!");
//    }

    // init default style attribute values
    const css_elem_def_props_t * type_ptr = enode->getElementTypePtr();
    bool is_object = false;
    if (type_ptr) {
        is_object = type_ptr->is_object;
        pstyle->display = type_ptr->display;
        pstyle->white_space = type_ptr->white_space;

        // Account for backward incompatible changes in fb2def.h
        if (gDOMVersionRequested < 20180528) { // revert what was changed 20180528
            if (nodeElementId == el_form) {
                pstyle->display = css_d_none; // otherwise shown as block, as it may have textual content
            }
            if (nodeElementId == el_code) {
                pstyle->white_space = css_ws_pre; // otherwise white-space: normal, as browsers do
            }
            if (nodeElementId >= el_address && nodeElementId <= el_xmp) { // newly added block elements
                pstyle->display = css_d_inline; // previously unknown and shown as inline
                if (gDOMVersionRequested < 20180524) {
                    pstyle->display = css_d_inherit; // previously unknown and display: inherit
                }
            }
            if (gDOMVersionRequested < 20180524) { // revert what was fixed 20180524
                if (nodeElementId == el_cite) {
                    pstyle->display = css_d_block; // otherwise correctly set to css_d_inline
                }
                if (nodeElementId == el_li) {
                    pstyle->display = css_d_list_item_legacy; // otherwise correctly set to css_d_list_item_block
                }
                if (nodeElementId == el_style) {
                    pstyle->display = css_d_inline; // otherwise correctly set to css_d_none (hidden)
                }
            }
        }
    }

    // Firefox resets text-align: to 'left' for table (eg: <center><table>
    // doesn't have its cells' content centered, not even justified if body
    // has "text-align: justify"), while crengine would make them centered.
    // So, we dont wan't table to starts with css_ta_inherit. We could use
    // css_ta_left (as Firefox), but it's best in our context to use the
    // value set to the (or current DocFragment's) BODY node, which starts
    // with css_ta_left but may be set to css_ta_justify by our epub.css.
    if (nodeElementId == el_table) {
        // To do as Firefox:
        // pstyle->text_align = css_ta_left;
        // But we'd rather use the BODY value:
        ldomNode * body = enode->getParentNode();
        while ( body != NULL && body->getNodeId()!=el_body )
            body = body->getParentNode();
        if ( body ) {
            pstyle->text_align = body->getStyle()->text_align;
        }
    }

    if (enode->getNodeNsId() == ns_epub) {
        if (nodeElementId == el_case) { // <epub:case required-namespace="...">
            // As we don't support any specific namespace (like MathML, SVG...), just
            // hide <epub:case> content - it must be followed by a <epub:default>
            // section with usually regular content like some image.
            ldomNode * parent = enode->getParentNode();
            if (parent && parent->getNodeNsId() == ns_epub && parent->getNodeId() == el_switch) {
                // (We can't here check parent's other children for the presence of one
                // el_default, as we can be called while XML is being parsed and the DOM
                // built and siblings not yet there, so just trust there is an el_default.)
                pstyle->display = css_d_none;
            }
        }
    }

    // display before stylesheet is applied (for fallback below if legacy mode)
    css_display_t orig_elem_display = pstyle->display;

    // not used (could be used for 'rem', but we have it in gRootFontSize)
    // int baseFontSize = enode->getDocument()->getDefaultFont()->getSize();

    //////////////////////////////////////////////////////
    // apply style sheet
    //////////////////////////////////////////////////////
    enode->getDocument()->applyStyle( enode, pstyle );

    //////////////////////////////////////////////////////
    // apply node style= attribute
    //////////////////////////////////////////////////////
    if ( enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) && enode->hasAttribute( LXML_NS_ANY, attr_style ) ) {
        lString16 nodeStyle = enode->getAttributeValue( LXML_NS_ANY, attr_style );
        if ( !nodeStyle.empty() ) {
            nodeStyle = cs16("{") + nodeStyle + "}";
            LVCssDeclaration decl;
            lString8 s8 = UnicodeToUtf8(nodeStyle);
            const char * s = s8.c_str();
            // We can't get the codeBase of this node anymore at this point, which
            // would be needed to resolve "background-image: url(...)" relative
            // file path... So these won't work when defined in a style= attribute.
            if ( decl.parse( s ) ) {
                decl.apply( pstyle );
            }
        }
    }

    // As per-specs (and to avoid checking edge cases in initNodeRendMethod()):
    // https://www.w3.org/TR/css-tables-3/#table-structure
    //  "Authors should not assign a display type from the previous
    //  list [inline-table & table*] to replaced elements (eg: input
    //  fields or images). When the display property of a replaced
    //  element computes to one of these values, it is handled
    //  instead as though the author had declared either block
    //  (for table display) or inline (for all other values).
    // Also:
    //  "This is a breaking change from css 2.1 but matches implementations."
    // The fallback values was different per-browser, as seen in:
    //  https://github.com/w3c/csswg-drafts/issues/508
    // but the discussion resolved it to:
    // - All internal 'table-*' displays on replaced elements behave as 'inline'.
    // - 'table' falls back to 'block', 'inline-table' falls back to 'inline'
    //
    // Note that with this bogus HTML snippet:
    //   <table style="border: solid 1px black">
    //     <img src="some.png" style="display: table-cell"/>
    //     <tr><img src="some.png" style="display: table-cell"/><td>text</td></tr>
    //     <tr><td>text in table cell</td><td>text</td></tr>
    //   </table
    // Firefox would draw both images before/outside of the table border
    // (so, making them inline and moving them outside the table),
    // while we will keep them inline inside the table, and wrapped
    // into tabularBoxes acting as the missing table elements.
    if ( is_object ) {
        switch ( pstyle->display ) {
            case css_d_table:
                pstyle->display = css_d_block;
                break;
            case css_d_table_row_group:
            case css_d_table_header_group:
            case css_d_table_footer_group:
            case css_d_table_row:
            case css_d_table_column_group:
            case css_d_table_column:
            case css_d_table_cell:
            case css_d_table_caption:
            case css_d_inline_table:
                pstyle->display = css_d_inline;
                break;
            default:
                break;
        }
    }

    // <br/> can be set to "display: block" by publishers, but
    // Firefox and Calibre do not handle them like other block
    // elements: they won't ensure a "height:" set on them.
    // It's not clear how such BR should be handled, but comparing
    // with how Firefox/Calibre/Chrome render them, it looks
    // like we'll render quite as they do when forcing BR to
    // always be css_d_inline:
    // When met alongside block elements, they'll be autoboxed and
    // will ensure just their (possibly inherited) line-height.
    if (nodeElementId == el_br && pstyle->display != css_d_none) {
        pstyle->display = css_d_inline;
    }

    // Ensure any <stylesheet> element (that crengine "added BODY>stylesheet child
    // element with HEAD>STYLE&LINKS content") stays invisible (it could end up being
    // made visible when some book stylesheet contains "body > * {display: block;}")
    if (nodeElementId == el_stylesheet) {
        pstyle->display = css_d_none;
    }

    if ( BLOCK_RENDERING_G(PREPARE_FLOATBOXES) ) {
        // https://developer.mozilla.org/en-US/docs/Web/CSS/float
        //  As float implies the use of the block layout, it modifies the computed value
        //  of the display values, in some cases: [...]
        // Mostly everything becomes display: block
        // As we use tests like node->getStyle()->display == css_d_block in a few places,
        // it's easier to change it here than add tests for getStyle()->float_ in these
        // places.
        // (This may not have much effect, as it may get ignored in initNodeRendMethod()
        // when !FLOAT_FLOATBOXES)
        // At the time setNodeStyle() is called, this can only happen on an original
        // element with float:, and not on a wrapping floatBox element which is either
        // not there yet (or just added, which will be handled by next 'if'), or has
        // not yet got its float_ from its child. So the ->display of the floatBox
        // element will have to be updated too elsewhere.
        if ( pstyle->float_ == css_f_left || pstyle->float_ == css_f_right ) {
            if ( pstyle->display <= css_d_inline ) {
                pstyle->display = css_d_block;
            }
        }
    }
    if ( BLOCK_RENDERING_G(WRAP_FLOATS) ) {
        if ( nodeElementId == el_floatBox ) {
            // floatBox added, by initNodeRendMethod(), as a wrapper around
            // element with float:.
            // We want to set the floatBox->style->float_ to the same value
            // as the wrapped original node.
            // We are either called explicitely (by initNodeRendMethod) while
            // the XML is being loaded, where the el_floatBox has just been
            // created - or on re-rendering when the el_floatBox is already there.
            // In the XML loading case, the child styles have already been applied,
            // so we can trust the child properties.
            // In the re-rendering case, the child styles have been reset and have
            // not yet been computed, so we can't apply it. This will be fixed
            // by initNodeRendMethod() when processing the nodes deep-first and
            // up the DOM tree, after the styles have been applied.
            if (enode->getChildCount() == 1) {
                ldomNode * child = enode->getChildNode(0);
                css_style_ref_t child_style = child->getStyle();
                if ( ! child_style.isNull() ) { // Initial XML loading phase
                    // This child_style is only non-null on the initial XML loading.
                    // We do as in ldomNode::initNodeRendMethod() when the floatBox
                    // is already there (on re-renderings):
                    pstyle->float_ = child_style->float_;
                    if (child_style->display <= css_d_inline) { // when !PREPARE_FLOATBOXES
                        pstyle->display = css_d_inline; // become an inline wrapper
                    }
                    else if (child_style->display == css_d_none) {
                        pstyle->display = css_d_none; // stay invisible
                    }
                    else { // everything else (including tables) must be wrapped by a block
                        pstyle->display = css_d_block;
                    }
                }
                // Else (child_style is null), it's a re-rendering: nothing special
                // to do, this will be dealt with later by initNodeRendMethod().
            }
        }
    }
    else { // legacy rendering or enhanced with no float support
        // Cancel any float value set from stylesheets:
        // this should be enough to trigger a displayhash mismatch
        // and a popup inviting the user to reload, to get rid of
        // floatBox elements.
        pstyle->float_ = css_f_none;
    }

    if ( BLOCK_RENDERING_G(BOX_INLINE_BLOCKS) ) {
        // See above, same reasoning
        if ( nodeElementId == el_inlineBox ) {
            // el_inlineBox are "display: inline" by default (defined in fb2def.h)
            if (enode->getChildCount() == 1) {
                ldomNode * child = enode->getChildNode(0);
                css_style_ref_t child_style = child->getStyle();
                if ( ! child_style.isNull() ) { // Initial XML loading phase
                    // This child_style is only non-null on the initial XML loading.
                    // We do as in ldomNode::initNodeRendMethod() when the inlineBox
                    // is already there (on re-renderings):
                    // (If this is an inlineBox in the initial XML loading phase,
                    // child is necessarily css_d_inline_block or css_d_inline_table,
                    // or this node is <inlineBox T=EmbeddedBlock>.
                    // The following 'else's should never trigger.
                    if (child_style->display == css_d_inline_block || child_style->display == css_d_inline_table) {
                        pstyle->display = css_d_inline; // become an inline wrapper
                        pstyle->vertical_align = child_style->vertical_align;
                    }
                    else if ( enode->hasAttribute( attr_T ) ) { // T="EmbeddedBlock"
                                            // (no other possible value yet, no need to compare strings)
                        pstyle->display = css_d_inline; // wrap bogus "block among inlines" in inline
                    }
                    else if (child_style->display <= css_d_inline) {
                        pstyle->display = css_d_inline; // wrap inline in inline
                    }
                    else if (child_style->display == css_d_none) {
                        pstyle->display = css_d_none; // stay invisible
                    }
                    else { // everything else must be wrapped by a block
                        pstyle->display = css_d_block;
                    }
                }
                // Else (child_style is null), it's a re-rendering: nothing special
                // to do, this will be dealt with later by initNodeRendMethod().
            }
        }
    }
    else {
        // Legacy rendering or enhanced with no inline-block support
        // Fallback to the default style for the element
        // (before enhanced rendering, css_d_inline_block did not exist, so the
        // node probably stayed with the default display: of the element when
        // no other lower specificity CSS set another).
        if ( pstyle->display == css_d_inline_block || pstyle->display == css_d_inline_table ) {
            if ( !BLOCK_RENDERING_G(ENHANCED) && pstyle->display == css_d_inline_table ) {
                // In legacy mode, inline-table was handled like css_d_block (as all
                // not specifically handled css_d_* are, so probably unwillingly).
                pstyle->display = css_d_block;
            }
            else {
                pstyle->display = orig_elem_display;
            }
        }
    }

    // update inherited style attributes
/*
  #define UPDATE_STYLE_FIELD(fld,inherit_value) \
  if (pstyle->fld == inherit_value) \
      pstyle->fld = parent_style->fld
*/
    #define UPDATE_STYLE_FIELD(fld,inherit_value) \
        if (pstyle->fld == inherit_value) \
            pstyle->fld = parent_style->fld
    #define UPDATE_LEN_FIELD(fld) \
        switch( pstyle->fld.type ) \
        { \
        case css_val_inherited: \
            pstyle->fld = parent_style->fld; \
            break; \
        /* relative values to parent style */\
        case css_val_percent: \
            pstyle->fld.type = parent_style->fld.type; \
            pstyle->fld.value = parent_style->fld.value * pstyle->fld.value / 100 / 256; \
            break; \
        case css_val_em: \
            pstyle->fld.type = parent_style->fld.type; \
            pstyle->fld.value = parent_style->font_size.value * pstyle->fld.value / 256; \
            break; \
        case css_val_ex: \
            pstyle->fld.type = parent_style->fld.type; \
            pstyle->fld.value = parent_style->font_size.value * pstyle->fld.value / 512; \
            break; \
        default: \
            /* absolute values, no need to relate to parent style */\
            break; \
        }

    //if ( (pstyle->display == css_d_inline) && (pstyle->text_align==css_ta_inherit))
    //{
        //if (parent_style->text_align==css_ta_inherit)
        //parent_style->text_align = css_ta_center;
    //}

    if (gDOMVersionRequested < 20180524) { // display should not be inherited
        UPDATE_STYLE_FIELD( display, css_d_inherit );
    }
    UPDATE_STYLE_FIELD( white_space, css_ws_inherit );
    UPDATE_STYLE_FIELD( text_align, css_ta_inherit );
    UPDATE_STYLE_FIELD( text_align_last, css_ta_inherit );
    UPDATE_STYLE_FIELD( text_decoration, css_td_inherit );
    UPDATE_STYLE_FIELD( text_transform, css_tt_inherit );
    UPDATE_STYLE_FIELD( hyphenate, css_hyph_inherit );
    UPDATE_STYLE_FIELD( orphans, css_orphans_widows_inherit );
    UPDATE_STYLE_FIELD( widows, css_orphans_widows_inherit );
    UPDATE_STYLE_FIELD( list_style_type, css_lst_inherit );
    UPDATE_STYLE_FIELD( list_style_position, css_lsp_inherit );

    // Note: we don't inherit "direction" (which should be inherited per specs);
    // We'll handle inheritance of direction in renderBlockEnhanced, because
    // it is also specified, with higher priority, by dir= attributes.

    // page_break_* should not be inherited per CSS specs, and as we set
    // each node to start with css_pb_auto, they shouldn't get a chance
    // to be inherited, except if a publisher really uses 'inherit'.
    // We usually don't ensure inheritance of styles that are not inherited
    // by default, but as these were originally in the code, let's keep them.
    UPDATE_STYLE_FIELD( page_break_before, css_pb_inherit );
    UPDATE_STYLE_FIELD( page_break_after, css_pb_inherit );
    UPDATE_STYLE_FIELD( page_break_inside, css_pb_inherit );

    // vertical_align
    // Should not be inherited per CSS specs: we fixed its propagation
    // to children with the use of 'valign_dy'.
    // But our default value for each node is not "inherit" but "baseline",
    // so we should be fine allowing an explicite "inherit" to get its parent
    // value. This is actually required with html5.css where TR,TD,TH are
    // explicitely set to "vertical-align: inherit", so they can inherit
    // from "thead, tbody, tfoot, table > tr { vertical-align: middle}"
    if ( pstyle->vertical_align.type == css_val_unspecified && pstyle->vertical_align.value == css_va_inherit)
        pstyle->vertical_align = parent_style->vertical_align;

    UPDATE_STYLE_FIELD( font_style, css_fs_inherit );
    UPDATE_STYLE_FIELD( font_weight, css_fw_inherit );
    if ( pstyle->font_family == css_ff_inherit ) {
        //UPDATE_STYLE_FIELD( font_name, "" );
        pstyle->font_name = parent_font.get()->getTypeFace();
    }
    UPDATE_STYLE_FIELD( font_family, css_ff_inherit );
    //UPDATE_LEN_FIELD( font_size ); // this is done below

    // font_features (font-variant/font-feature-settings)
    // The specs say a font-variant resets the ones that would be
    // inherited (as inheritance always does).
    // But, as we store in a single bitmap the values from multiple
    // properties (font-variant, font-variant-caps...), we drift from
    // the specs by OR'ing the ones sets by style on this node with
    // the ones inherited from parents (so we can use style-tweaks
    // like body { font-variant: oldstyle-nums; } without that being
    // reset by a lower H1 { font-variant: small-caps; }.
    // Note that we don't handle the !important bit whether it's set
    // for this node or the parent (if it's set on the parent, we
    // could decide to = instead of |=), as it's not clear whether
    // it's better or not: we'll see.
    // (Note that we can use * { font-variant: normal !important; } to
    // stop any font-variant without !important from being applied.)
    pstyle->font_features.value |= parent_style->font_features.value;
    pstyle->font_features.type = css_val_unspecified;

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
    { // ordered here as most likely to be met
    case css_val_inherited:
        pstyle->font_size = parent_style->font_size;
        break;
    case css_val_screen_px:
    case css_val_px:
        // absolute size, nothing to do
        break;
    case css_val_em: // value = em*256 ; 256 = 1em = x1
        pstyle->font_size.type = parent_style->font_size.type;
        pstyle->font_size.value = parent_style->font_size.value * pstyle->font_size.value / 256;
        break;
    case css_val_ex: // value = ex*256 ; 512 = 2ex = 1em = x1
        pstyle->font_size.type = parent_style->font_size.type;
        pstyle->font_size.value = parent_style->font_size.value * pstyle->font_size.value / 512;
        break;
    case css_val_percent: // value = percent number ; 100 = 100% => x1
        pstyle->font_size.type = parent_style->font_size.type;
        pstyle->font_size.value = parent_style->font_size.value * pstyle->font_size.value / 100 / 256;
        break;
    case css_val_rem:
        // not relative to parent, nothing to do
        break;
    case css_val_in:
    case css_val_cm:
    case css_val_mm:
    case css_val_pt:
    case css_val_pc:
        // absolute size, nothing to do
        break;
    case css_val_unspecified:
    case css_val_color:
        // not supported: use inherited value
        pstyle->font_size = parent_style->font_size;
        // printf("CRE WARNING: font-size css_val_unspecified or color, fallback to inherited\n");
        break;
    }

    // line_height
    if (pstyle->line_height.type == css_val_inherited) {
        // We didn't have yet the parent font when dealing with the parent style
        // (or we could have computed an absolute size line-height that we could
        // have just inherited as-is here).
        // But we have it now, so compute its absolute size, so it can be
        // inherited as-is by our children.
        switch( parent_style->line_height.type ) {
            case css_val_percent:
            case css_val_em:
            case css_val_ex:
                {
                int pem = parent_font->getSize(); // value in screen px
                int line_h = lengthToPx(parent_style->line_height, pem, pem);
                // Scale it according to gInterlineScaleFactor
                if (gInterlineScaleFactor != INTERLINE_SCALE_FACTOR_NO_SCALE)
                    line_h = (line_h * gInterlineScaleFactor) >> INTERLINE_SCALE_FACTOR_SHIFT;
                pstyle->line_height.value = line_h;
                pstyle->line_height.type = css_val_screen_px;
                }
                break;
            // For all others, we can inherit as-is:
            // case css_val_rem:         // related to font size of the root element
            // case css_val_screen_px:   // absolute sizes
            // case css_val_px:
            // case css_val_in:
            // case css_val_cm:
            // case css_val_mm:
            // case css_val_pt:
            // case css_val_pc:
            // case css_val_unspecified: // unitless number: factor to element's own font size: no relation to parent font
            default:
                pstyle->line_height = parent_style->line_height; // inherit as-is
                break;
        }
    }

    // We can't use spreadParent() with letter_spacing, as there is
    // (css_val_unspecified, css_generic_normal) which should not be overriden
    // by inheritance, while all other bogus (css_val_unspecified, number) should
    if ( pstyle->letter_spacing.type == css_val_inherited ||
            (pstyle->letter_spacing.type == css_val_unspecified &&
             pstyle->letter_spacing.value != css_generic_normal) )
        pstyle->letter_spacing = parent_style->letter_spacing;

    spreadParent( pstyle->color, parent_style->color );

    // background_color
    // Should not be inherited: elements start with unspecified.
    // The code will fill the rect of a parent element, and will
    // simply draw its children over the already filled rect.
    bool spread_background_color = false;
    // But for some elements, it should be propagated to children,
    // and we explicitely don't have the parent fill the rect with
    // its background-color:
    // - a TD or TH with unspecified background-color does inherit
    //   it from its parent TR. We don't draw bgcolor for TR.
    if ( pstyle->display == css_d_table_cell )
        spread_background_color = true;
    // - a TR with unspecified background-color may inherit it from
    //   its THEAD/TBODY/TFOOT parent (but not from its parent TABLE).
    //   It may then again be propagated to the TD by the previous rule.
    //   We don't draw bgcolor for THEAD/TBODY/TFOOT.
    if ( pstyle->display == css_d_table_row &&
            ( parent_style->display == css_d_table_row_group ||
              parent_style->display == css_d_table_header_group ||
              parent_style->display == css_d_table_footer_group ) )
        spread_background_color = true;
    if ( spread_background_color )
        spreadParent( pstyle->background_color, parent_style->background_color, true );

    // See if applying styles requires pseudo element before/after
    bool requires_pseudo_element_before = false;
    bool requires_pseudo_element_after = false;
    if ( pstyle->pseudo_elem_before_style ) {
        if ( pstyle->pseudo_elem_before_style->display != css_d_none
                && pstyle->pseudo_elem_before_style->content.length() > 0
                && pstyle->pseudo_elem_before_style->content[0] != L'X' ) {
            // Not "display: none" and with "content:" different than "none":
            // this pseudo element can be generated
            requires_pseudo_element_before = true;
        }
        delete pstyle->pseudo_elem_before_style;
        pstyle->pseudo_elem_before_style = NULL;
    }
    if ( pstyle->pseudo_elem_after_style ) {
        if ( pstyle->pseudo_elem_after_style->display != css_d_none
                && pstyle->pseudo_elem_after_style->content.length() > 0
                && pstyle->pseudo_elem_after_style->content[0] != L'X' ) {
            // Not "display: none" and with "content:" different than "none":
            // this pseudo element can be generated
            requires_pseudo_element_after = true;
        }
        delete pstyle->pseudo_elem_after_style;
        pstyle->pseudo_elem_after_style = NULL;
    }

    if ( nodeElementId == el_pseudoElem ) {
        // Pseudo element ->content may need some update if it contains
        // any of the open-quote-like tokens, to account for the
        // quoting nested levels. setNodeStyle() is actually the good
        // place to do that, as we're visiting all the nodes recursively.
        update_style_content_property(pstyle, enode);
    }

    pstyle->flags = 0; // cleanup, before setStyle() adds it to cache

    // set calculated style
    //enode->getDocument()->cacheStyle( style );
    enode->setStyle( style );
    if ( enode->getStyle().isNull() ) {
        CRLog::error("NULL style set!!!");
        enode->setStyle( style );
    }

    // set font
    enode->initNodeFont();

    // Now that this node is fully styled, ensure these pseudo elements
    // are there as children, creating them if needed and possible
    if ( requires_pseudo_element_before )
        enode->ensurePseudoElement(true);
    if ( requires_pseudo_element_after )
        enode->ensurePseudoElement(false);
}

// Uncomment for debugging getRenderedWidths():
// #define DEBUG_GETRENDEREDWIDTHS

// Estimate width of node when rendered:
//   maxWidth: width if it would be rendered on an infinite width area
//   minWidth: width with a wrap on all spaces (no hyphenation), so width taken by the longest word
void getRenderedWidths(ldomNode * node, int &maxWidth, int &minWidth, int direction, bool ignoreMargin, int rendFlags) {
    // Setup passed-by-reference parameters for recursive calls
    int curMaxWidth = 0;    // reset on <BR/> or on new block nodes
    int curWordWidth = 0;   // may not be reset to correctly estimate multi-nodes single-word ("I<sup>er</sup>")
    bool collapseNextSpace = true; // collapse leading spaces
    int lastSpaceWidth = 0; // trailing spaces width to remove
    // These do not need to be passed by reference, as they are only valid for inner nodes/calls
    int indent = 0;         // text-indent: used on first text, and set again on <BR/>
    bool isStartNode = true; // we are starting measurement on that node
    // Start measurements and recursions:
    getRenderedWidths(node, maxWidth, minWidth, direction, ignoreMargin, rendFlags,
        curMaxWidth, curWordWidth, collapseNextSpace, lastSpaceWidth, indent, NULL, false, isStartNode);
}

void getRenderedWidths(ldomNode * node, int &maxWidth, int &minWidth, int direction, bool ignoreMargin, int rendFlags,
    int &curMaxWidth, int &curWordWidth, bool &collapseNextSpace, int &lastSpaceWidth,
    int indent, TextLangCfg * lang_cfg, bool processNodeAsText, bool isStartNode)
{
    // This does mostly what renderBlockElement, renderFinalBlock and lvtextfm.cpp
    // do, but only with widths and horizontal margin/border/padding and indent
    // (with no width constraint, so no line splitting and hyphenation - and we
    // don't care about vertical spacing and alignment).
    // Limitations: no support for css_d_run_in (hardly ever used, we don't care)
    // todo : probably more tweaking to do when direction=RTL, and we should
    // also handle direction change when walking inner elements... (For now,
    // we only handle list-style-position/text-align combinations vs direction,
    // which have different rendering methods.)

    if ( node->isElement() && !processNodeAsText ) {
        int m = node->getRendMethod();
        if (m == erm_invisible)
            return;

        if ( isStartNode ) {
            lang_cfg = TextLangMan::getTextLangCfg( node ); // Fetch it from node or its parents
        }
        else if ( node->hasAttribute( attr_lang ) ) {
            lString16 lang_tag = node->getAttributeValue( attr_lang );
            if ( !lang_tag.empty() )
                lang_cfg = TextLangMan::getTextLangCfg( lang_tag );
        }

        if ( isStartNode && node->isBoxingInlineBox() ) {
            // The inlineBox is erm_inline, and we'll be measuring it below
            // as part of measuring other erm_inline in some erm_final.
            // If isStartNode, we want to measure its content, so switch
            // to handle it like erm_block.
            m = erm_block;
        }

        css_style_rec_t * style = node->getStyle().get();

        // Get image size early
        bool is_img = false;
        lInt16 img_width = 0;
        if ( node->getNodeId()==el_img ) {
            is_img = true;
            // (as in lvtextfm.cpp LFormattedText::AddSourceObject)
            #define DUMMY_IMAGE_SIZE 16
            LVImageSourceRef img = node->getObjectImageSource();
            if ( img.isNull() )
                img = LVCreateDummyImageSource( node, DUMMY_IMAGE_SIZE, DUMMY_IMAGE_SIZE );
            lInt16 width = (lUInt16)img->GetWidth();
            lInt16 height = (lUInt16)img->GetHeight();
            // Scale image native size according to gRenderDPI
            width = scaleForRenderDPI(width);
            height = scaleForRenderDPI(height);
            // Adjust if size defined by CSS
            int w = 0, h = 0;
            int em = node->getFont()->getSize();
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
            if (w > 0)
                img_width = w;
            else { // 0 or styles were in %
                // This is a bit tricky...
                // When w < 0, the style width was in %, which means % of the
                // container width.
                // So it does not influence the width we're trying to guess, and will
                // adjust to the final width. So, we could let it to be 0.
                // But, if this image is the single element of this block, we would
                // end up with a minWidth of 0, with no room for the image, and the
                // image would be scaled as a % of 0, so to 0.
                // So, consider we want the image to be shown as a % of its original
                // size: so, our width should be the original image width.
                img_width = width;
                // With that, it looks like we behave exactly as Firefox, whether
                // the image is single in a cell, or surrounded, in this cell
                // and/or sibling cells, by small or long text!
                // We ensure a minimal size of 1em (so it shows as least the size
                // of a letter).
                int em = node->getFont()->getSize();
                if (img_width < em)
                    img_width = em;
            }
        }

        if (m == erm_inline) {
            if ( is_img ) {
                // Get done with previous word
                if (curWordWidth > minWidth)
                    minWidth = curWordWidth;
                curWordWidth = 0;
                collapseNextSpace = false;
                lastSpaceWidth = 0;
                if (img_width > 0) { // inline img with a fixed width
                    maxWidth += img_width;
                    if (img_width > minWidth)
                        minWidth = img_width;
                }
                return;
            }
            if ( node->getNodeId()==el_br ) {
                #ifdef DEBUG_GETRENDEREDWIDTHS
                    printf("GRW: BR\n");
                #endif
                // Get done with current word
                if (lastSpaceWidth)
                    curMaxWidth -= lastSpaceWidth;
                if (curMaxWidth > maxWidth)
                    maxWidth = curMaxWidth;
                if (curWordWidth > minWidth)
                    minWidth = curWordWidth;
                // First word after a <BR> should not have text-indent in its width,
                // but we did reset 'indent' to 0 after the first word of the final block.
                // If we get some non-zero indent here, it is "hanging" indent, that
                // should be applied to all words, including the one after a <BR/>, and
                // so it should contribute to the new line full width (curMaxWidth).
                curMaxWidth = indent;
                curWordWidth = indent;
                collapseNextSpace = true; // skip leading spaces
                lastSpaceWidth = 0;
                return;
            }
            if ( node->isBoxingInlineBox() ) {
                // Get done with previous word
                if (curWordWidth > minWidth)
                    minWidth = curWordWidth;
                curWordWidth = 0;
                collapseNextSpace = false;
                lastSpaceWidth = 0;
                // Get the rendered width of the inlineBox
                int _maxw = 0;
                int _minw = 0;
                getRenderedWidths(node, _maxw, _minw, direction, false, rendFlags);
                maxWidth += _maxw;
                if (_minw > minWidth)
                    minWidth = _minw;
                return;
            }
            if ( node->getNodeId()==el_pseudoElem ) {
                // pseudoElem has no children: reprocess this same node
                // with processNodeAsText=true, to process its text content.
                getRenderedWidths(node, maxWidth, minWidth, direction, false, rendFlags,
                    curMaxWidth, curWordWidth, collapseNextSpace, lastSpaceWidth, indent, lang_cfg, true);
                return;
            }
            // Contains only other inline or text nodes:
            // add to our passed by ref *Width
            for (int i = 0; i < node->getChildCount(); i++) {
                ldomNode * child = node->getChildNode(i);
                // Nothing more to do with inline elements: they just carry some
                // styles that will be grabbed by children text nodes
                getRenderedWidths(child, maxWidth, minWidth, direction, false, rendFlags,
                    curMaxWidth, curWordWidth, collapseNextSpace, lastSpaceWidth, indent, lang_cfg);
            }
            return;
        }

        // For erm_block and erm_final:
        // We may have padding/margin/border, that we can simply add to
        // the widths that will be computed by our children.
        // Also, if these have % as their CSS unit, we need a width to
        // apply the % to, so we can only do that when we get a maxWidth
        // and minWidth from children.

        // For list-items, we need to compute the bullet width to use either
        // as indent, or as left padding
        int list_marker_width = 0;
        bool list_marker_width_as_padding = false;
        if ( style->display == css_d_list_item_block ) {
            LFormattedTextRef txform( node->getDocument()->createFormattedText() );
            lString16 marker = renderListItemMarker( node, list_marker_width, txform.get(), -1, 0);
            #ifdef DEBUG_GETRENDEREDWIDTHS
                printf("GRW: list_marker_width: %d\n", list_marker_width);
            #endif
            bool is_rtl = direction == REND_DIRECTION_RTL;
            if ( !renderAsListStylePositionInside(style, is_rtl) ) {
                // (same hack as in rendering code: we render 'outside' just
                // like 'inside' when center or right aligned)
                list_marker_width_as_padding = true;
            }
            else if ( style->list_style_type == css_lst_none ) {
                // When css_lsp_inside, or with that hack when outside & center/right,
                // no space should be used if list-style-type: none.
                list_marker_width = 0;
            }
        }

        // We use temporary parameters, that we'll add our padding/margin/border to
        int _maxWidth = 0;
        int _minWidth = 0;

        bool use_style_width = false;
        css_length_t style_width = style->width;
        if ( BLOCK_RENDERING(rendFlags, ENSURE_STYLE_WIDTH) ) {
            if ( style->display > css_d_table ) {
                // ignore width for table sub-elements
            }
            else {
                // Ignore widths in %, as we can't do much with them
                if ( style_width.type != css_val_unspecified && style_width.type != css_val_percent ) {
                    use_style_width = true;
                    if ( !BLOCK_RENDERING(rendFlags, ALLOW_STYLE_W_H_ABSOLUTE_UNITS) &&
                            style_width.type != css_val_percent && style_width.type != css_val_em &&
                            style_width.type != css_val_ex && style_width.type != css_val_rem ) {
                        use_style_width = false;
                    }
                    if ( node->getNodeId() == el_hr ) {
                        // We always use style width for <HR> for cosmetic reasons
                        use_style_width = true;
                    }
                }
            }
        }

        if ( use_style_width ) {
            int em = node->getFont()->getSize();
            _maxWidth = lengthToPx( style_width, 0, em );
            _minWidth = _maxWidth;
        }
        else if (m == erm_final) {
            // Block node that contains only inline or text nodes
            if ( is_img ) { // img with display: block always become erm_final (never erm_block)
                if (img_width > 0) { // block img with a fixed width
                    _maxWidth = img_width;
                    _minWidth = img_width;
                }
            }
            else {
                // curMaxWidth and curWordWidth are not used in our parents (which
                // are block-like elements), we can just reset them.
                curMaxWidth = 0;
                curWordWidth = 0;
                // We don't have any width yet to use for text-indent in % units,
                // but this is very rare - use em as we must use something
                int em = node->getFont()->getSize();
                indent = lengthToPx(style->text_indent, em, em);
                // First word will have text-indent as part of its width
                if ( style->text_indent.value & 0x00000001 ) {
                    // lvstsheet sets the lowest bit to 1 when text-indent has the "hanging" keyword.
                    // "hanging" means it should apply on all line except the first.
                    // Hanging indent does not apply on the first word, but may apply on each
                    // followup word if a wrap happens before them so don't reset it.
                    // To keep things simple and readable here, we only apply it to the first
                    // word after a <BR> - but it should really apply on each word, everytime
                    // we reset curWordWidth, which would make the below code quite ugly and
                    // hard to understand. Hopefully, negative or hanging indents should be
                    // rare in floats, inline boxes and table cells.
                    // (We don't handle the shift/overlap with padding that a real negative
                    // indent can cause - so, we may return excessive widths.)
                }
                else {
                    // Not-"hanging" positive or negative indent applies only on the first line,
                    // so account for it only on the first word.
                    curMaxWidth += indent;
                    curWordWidth += indent;
                    indent = 0; // but no more on following words in this final node, even after <BR>
                }
                if (list_marker_width > 0 && !list_marker_width_as_padding) {
                    // with additional list marker if list-style-position: inside
                    curMaxWidth += list_marker_width;
                    curWordWidth += list_marker_width;
                }
                collapseNextSpace = true; // skip leading spaces
                lastSpaceWidth = 0;
                // Process children, which are either erm_inline or text nodes
                for (int i = 0; i < node->getChildCount(); i++) {
                    ldomNode * child = node->getChildNode(i);
                    getRenderedWidths(child, _maxWidth, _minWidth, direction, false, rendFlags,
                        curMaxWidth, curWordWidth, collapseNextSpace, lastSpaceWidth, indent, lang_cfg);
                    // A <BR/> can happen deep among our children, so we deal with that when erm_inline above
                }
                if ( node->getNodeId() == el_pseudoElem ) {
                    // erm_final pseudoElem (which has no children): reprocess this same
                    // node with processNodeAsText=true, to process its text content.
                    getRenderedWidths(node, _maxWidth, _minWidth, direction, false, rendFlags,
                        curMaxWidth, curWordWidth, collapseNextSpace, lastSpaceWidth, indent, lang_cfg, true);
                }
                if (lastSpaceWidth)
                    curMaxWidth -= lastSpaceWidth;
                // Add current word as we're leaving a block node, so it can't be followed by some other text
                if (curMaxWidth > _maxWidth)
                    _maxWidth = curMaxWidth;
                if (curWordWidth > _minWidth)
                    _minWidth = curWordWidth;
            }
        }
        // if (m == erm_block) // Block node that contains other stacked block or final nodes
        // Not dealing with other rendering methods may return widths of 0 (and
        // on a nested inner table with erm_table, would ask to render this inner
        // table in a width of 0).
        // So, we treat all other erm_* as erm_block (which will obviously be
        // wrong for erm_table* with more than 1 column, but it should give a
        // positive enough width to draw something).
        //   Update: we use a trick when erm_table_row below.
        else {
            // Process children, which are all block-like nodes:
            // our *Width are the max of our children *Width
            for (int i = 0; i < node->getChildCount(); i++) {
                // New temporary parameters
                int _maxw = 0;
                int _minw = 0;
                ldomNode * child = node->getChildNode(i);
                getRenderedWidths(child, _maxw, _minw, direction, false, rendFlags,
                    curMaxWidth, curWordWidth, collapseNextSpace, lastSpaceWidth, indent, lang_cfg);
                if (m == erm_table_row) {
                    // For table rows, adding the min/max widths of each children
                    // (the table cells), instead of taking the largest, gives
                    // a better estimate of what the table width should be.
                    _maxWidth += _maxw;
                    _minWidth += _minw;
                }
                else {
                    if (_maxw > _maxWidth)
                        _maxWidth = _maxw;
                    if (_minw > _minWidth)
                        _minWidth = _minw;
                }
            }
        }

        // For both erm_block or erm_final, adds padding/margin/border
        // to _maxWidth and _minWidth (see comment above)

        // Style width includes paddings and border in the traditional box model,
        // but not in the W3C box model
        bool ignorePadding = use_style_width && !BLOCK_RENDERING(rendFlags, USE_W3C_BOX_MODEL);

        int padLeft = 0; // these will include padding, border and margin
        int padRight = 0;
        if (list_marker_width > 0 && list_marker_width_as_padding) {
            // with additional left padding for marker if list-style-position: outside
            padLeft += list_marker_width;
        }
        // For % values, we need to reverse-apply them as a whole.
        // We can'd do that individually for each, so we aggregate
        // the % values.
        // (And as we can't ceil() each individually, we'll add 1px
        // below for each one to counterbalance rounding errors.)
        int padPct = 0; // cumulative percent
        int padPctNb = 0; // nb of styles in % (to add 1px)
        int em = node->getFont()->getSize();
        // margin
        if (!ignoreMargin) {
            if (style->margin[0].type == css_val_percent) {
                padPct += style->margin[0].value;
                padPctNb += 1;
            }
            else {
                int margin = lengthToPx( style->margin[0], 0, em );
                if ( margin > 0 || BLOCK_RENDERING(rendFlags, ALLOW_HORIZONTAL_NEGATIVE_MARGINS) ) {
                    padLeft += margin;
                }
            }
            if (style->margin[1].type == css_val_percent) {
                padPct += style->margin[1].value;
                padPctNb += 1;
            }
            else {
                int margin = lengthToPx( style->margin[1], 0, em );
                if ( margin > 0 || BLOCK_RENDERING(rendFlags, ALLOW_HORIZONTAL_NEGATIVE_MARGINS) ) {
                    padRight += margin;
                }
            }
        }
        if (!ignorePadding) {
            // padding
            if (style->padding[0].type == css_val_percent) {
                padPct += style->padding[0].value;
                padPctNb += 1;
            }
            else
                padLeft += lengthToPx( style->padding[0], 0, em );
            if (style->padding[1].type == css_val_percent) {
                padPct += style->padding[1].value;
                padPctNb += 1;
            }
            else
                padRight += lengthToPx( style->padding[1], 0, em );
            // border (which does not accept units in %)
            padLeft += measureBorder(node,3);
            padRight += measureBorder(node,1);
        }
        // Add the non-pct values to make our base to invert-apply padPct
        _minWidth += padLeft + padRight;
        _maxWidth += padLeft + padRight;
        // For length in %, the % (P, padPct) should be from the outer width (L),
        // but we have only the inner width (w). We have w and P, we want L-w (m).
        //   m = L*P  and  w = L - m
        //   w = L - L*P  =  L*(1-P)
        //   L = w/(1-P)
        //   m = L - w  =  w/(1-P) - w  =  w*(1 - (1-P))/(1-P) = w*P/(1-P)
        // css_val_percent value are *256 (100% = 100*256)
        // We ignore a total of 100% (no space for content, and division by zero here
        int minPadPctLen = 0;
        int maxPadPctLen = 0;
        if (padPctNb > 0 && padPct != 100*256) {
            // add padPctNb: 1px for each value in %
            minPadPctLen = _minWidth * padPct / (100*256-padPct) + padPctNb;
            maxPadPctLen = _maxWidth * padPct / (100*256-padPct) + padPctNb;
            _minWidth += minPadPctLen;
            _maxWidth += maxPadPctLen;
        }
        #ifdef DEBUG_GETRENDEREDWIDTHS
            printf("GRW blk:  pad min+ %d %d +%d%%=%d\n", padLeft, padRight, padPct, minPadPctLen);
            printf("GRW blk:  pad max+ %d %d +%d%%=%d\n", padLeft, padRight, padPct, maxPadPctLen);
        #endif
        // We must have been provided with maxWidth=0 and minWidth=0 (temporary
        // parameters set by outer calls), but do these regular checks anyway.
        if (_maxWidth > maxWidth)
            maxWidth = _maxWidth;
        if (_minWidth > minWidth)
            minWidth = _minWidth;
    }
    else { // text or pseudoElem
        lString16 text;
        int start = 0;
        int len = 0;
        ldomNode * parent;
        if ( node->isText() ) {
            text = node->getText();
            parent = node->getParentNode();
        }
        else if ( node->getNodeId() == el_pseudoElem ) {
            text = get_applied_content_property(node);
            parent = node; // this pseudoElem node carries the font and style of the text
            if ( isStartNode ) {
                lang_cfg = TextLangMan::getTextLangCfg( node ); // Fetch it from node or its parents
            }
        }
        len = text.length();
        if ( len == 0 )
            return;
        // letter-spacing
        LVFont * font = parent->getFont().get();
        int em = font->getSize();
        css_style_rec_t * parent_style = parent->getStyle().get();
        int letter_spacing = lengthToPx(parent_style->letter_spacing, em, em);
        // text-transform
        switch (parent_style->text_transform) {
            case css_tt_uppercase:
                text.uppercase();
                break;
            case css_tt_lowercase:
                text.lowercase();
                break;
            case css_tt_capitalize:
                text.capitalize();
                break;
            case css_tt_full_width:
                // text.fullWidthChars(); // disabled for now (may change CJK rendering)
                break;
            case css_tt_none:
            case css_tt_inherit:
                break;
        }
        // white-space
        // When getting min width, ensure non free wrap for "white-space: pre" (even if we
        // don't when rendering). Others like "pre-wrap" and "pre-line" are allowed to wrap.
        bool nowrap = (parent_style->white_space == css_ws_nowrap) || (parent_style->white_space == css_ws_pre);
        bool pre = parent_style->white_space >= css_ws_pre;
        int space_width_scale_percent = pre ? 100 : parent->getDocument()->getSpaceWidthScalePercent();
        // measure text
        const lChar16 * txt = text.c_str();
        #ifdef DEBUG_GETRENDEREDWIDTHS
            printf("GRW text: |%s|\n", UnicodeToLocal(text).c_str());
            printf("GRW text:  (dumb text size=%d)\n", font->getTextWidth(txt, len));
        #endif
        #define MAX_TEXT_CHUNK_SIZE 4096
        static lUInt16 widths[MAX_TEXT_CHUNK_SIZE+1];
        static lUInt8 flags[MAX_TEXT_CHUNK_SIZE+1];
        // We adjust below each word width with calls to getLeftSideBearing()
        // and getRightSideBearing(). These should be called with the exact same
        // parameters as used in lvtextfm.cpp getAdditionalCharWidth() and
        // getAdditionalCharWidthOnLeft().
        // todo: use fribidi and split measurement at fribidi level change,
        // and beware left/right side bearing adjustments...
        #if (USE_LIBUNIBREAK==1)
        // If using libunibreak, we do similarly as in lvtextfm.cpp copyText(),
        // except that we don't update previous char, but look ahead at next
        // char to know about current break.
        // Also, as we do all that only text node by text node, we may lose
        // line breaking rules between contiguous text nodes (but it's a bit
        // complicated to pass this lbCtx across calls...)
        struct LineBreakContext lbCtx;
        lb_init_break_context(&lbCtx, 0x200D, NULL); // ZERO WIDTH JOINER
        lbCtx.lbpLang = lang_cfg->getLBProps();
        lb_process_next_char(&lbCtx, (utf32_t)(*txt));
        #endif
        while (true) {
            int chars_measured = font->measureText(
                    txt + start,
                    len,
                    widths, flags,
                    0x7FFF, // very wide width
                    '?',    // replacement char
                    lang_cfg,
                    letter_spacing,
                    false); // no hyphenation
                    // todo: provide direction and hints
            #if (USE_LIBUNIBREAK==1)
            for (int i=0; i<chars_measured; i++) {
                if (pre) {
                    collapseNextSpace = false; // Reset it if set previously
                }
                int w = widths[i] - (i>0 ? widths[i-1] : 0);
                if ( (flags[i] & LCHAR_IS_SPACE) && (space_width_scale_percent != 100) ) {
                    w = w * space_width_scale_percent / 100;
                }
                lChar16 c = *(txt + start + i);
                lChar16 next_c = *(txt + start + i + 1); // might be 0 at end of string
                if ( lang_cfg->hasLBCharSubFunc() ) {
                    next_c = lang_cfg->getLBCharSubFunc()(txt+start, i+1, len-1 - (i+1));
                }
                int brk = lb_process_next_char(&lbCtx, (utf32_t)next_c);
                    // We don't really need to bother with consecutive spaces (that
                    // should collapse when not 'pre', but libunibreak only allows
                    // break on the last one, so we would get the leading spaces
                    // width as part of current word), as we're dealing with a single
                    // text node, and the HTML parser has removed multiple consecutive
                    // spaces (except with 'pre', where it looks fine as they don't
                    // collapse; this might still not be right with pre-wrap though).
                // printf("between <%c%c>: brk %d\n", c, next_c, brk);
                if (brk == LINEBREAK_ALLOWBREAK && !nowrap) {
                    if (flags[i] & LCHAR_ALLOW_WRAP_AFTER) { // a breakable/collapsible space (flag set by measureText()
                        if (collapseNextSpace) // ignore this space
                            continue;
                        collapseNextSpace = true; // ignore next spaces, even if in another node
                        lastSpaceWidth = pre ? 0 : w; // Don't remove last space width if 'pre'
                        curMaxWidth += w; // add this space to non-wrap width
                        if (curWordWidth > 0) { // there was a word before this space
                            if (start+i > 0) {
                                // adjust for last word's last char overflow (italic, letter f...)
                                lChar16 prevc = *(txt + start + i - 1);
                                int right_overflow = - font->getRightSideBearing(prevc, true, true);
                                curWordWidth += right_overflow;
                            }
                        }
                        if (curWordWidth > minWidth) // done with previous word
                            minWidth = curWordWidth; // longest word found
                        curWordWidth = 0;
                    }
                    else { // break after a non space: might be a CJK char (or other stuff)
                        collapseNextSpace = false; // next space should not be ignored
                        lastSpaceWidth = 0; // no width to take off if we stop with this char
                        curMaxWidth += w;
                        if (curWordWidth > 0) { // there was a word or CJK char before this CJK char
                            if (start+i > 0) {
                                // adjust for last word's last char or previous CJK char right overflow
                                lChar16 prevc = *(txt + start + i - 1);
                                int right_overflow = - font->getRightSideBearing(prevc, true, true);
                                curWordWidth += right_overflow;
                            }
                        }
                        if (curWordWidth > minWidth) // done with previous word
                            minWidth = curWordWidth; // longest word found
                        curWordWidth = w;
                        // adjust for leading overflow
                        int left_overflow = - font->getLeftSideBearing(c, false, true);
                        curWordWidth += left_overflow;
                        if (start + i == 0) // at start of text only? (not sure)
                            curMaxWidth += left_overflow; // also add it to max width
                    }
                }
                else if (brk == LINEBREAK_MUSTBREAK) { // \n if pre
                    // Get done with current word
                    if (curWordWidth > 0) { // we end with a word
                        if (start+i > 0) {
                            // adjust for last word's last char or previous CJK char right overflow
                            lChar16 prevc = *(txt + start + i - 1);
                            int right_overflow = - font->getRightSideBearing(prevc, true, true);
                            curWordWidth += right_overflow;
                            curMaxWidth += right_overflow;
                        }
                    }
                    // Similar to what's done above on <BR> or at end of final node
                    if (lastSpaceWidth)
                        curMaxWidth -= lastSpaceWidth;
                    if (curMaxWidth > maxWidth)
                        maxWidth = curMaxWidth;
                    if (curWordWidth > minWidth)
                        minWidth = curWordWidth;
                    // Get ready for next text
                    curMaxWidth = indent;
                    curWordWidth = indent;
                    collapseNextSpace = true; // skip leading spaces
                    lastSpaceWidth = 0;
                }
                else { // break not allowed: this char is part of a word
                    // But it can be a space followed by another space (with libunibreak,
                    // only the last space will get LINEBREAK_ALLOWBREAK).
                    if (flags[i] & LCHAR_ALLOW_WRAP_AFTER) { // a breakable/collapsible space (flag set by measureText()
                        if (collapseNextSpace) { // space before (and space after)
                            continue; // ignore it
                        }
                        collapseNextSpace = true; // ignore next ones
                        lastSpaceWidth = pre ? 0 : w; // Don't remove last space width if 'pre'
                    }
                    else { // Not a space
                        collapseNextSpace = false; // next space should not be ignored
                        lastSpaceWidth = 0; // no width to take off if we stop with this char
                    }
                    if (curWordWidth == 0) { // first char of a word
                        // adjust for leading overflow on first char of a word
                        int left_overflow = - font->getLeftSideBearing(c, false, true);
                        curWordWidth += left_overflow;
                        if (start + i == 0) // at start of text only? (not sure)
                            curMaxWidth += left_overflow; // also add it to max width
                    }
                    curMaxWidth += w;
                    curWordWidth += w;
                    // libunibreak should handle properly '/' in urls (except may be
                    // if the url parts are made of numbers...)
                }
            }
            #else // not USE_LIBUNIBREAK==1
            // (This has not been updated to handle nowrap & pre)
            for (int i=0; i<chars_measured; i++) {
                int w = widths[i] - (i>0 ? widths[i-1] : 0);
                lChar16 c = *(txt + start + i);
                if ( (flags[i] & LCHAR_IS_SPACE) && (space_width_scale_percent != 100) ) {
                    w = w * space_width_scale_percent / 100;
                }
                bool is_cjk = (c >= UNICODE_CJK_IDEOGRAPHS_BEGIN && c <= UNICODE_CJK_IDEOGRAPHS_END
                            && ( c<=UNICODE_CJK_PUNCTUATION_HALF_AND_FULL_WIDTH_BEGIN
                                || c>=UNICODE_CJK_PUNCTUATION_HALF_AND_FULL_WIDTH_END) );
                            // Do we need to do something about CJK punctuation?
                    // Having CJK columns min_width the width of a single CJK char
                    // may, on some pages, make some table cells have a single
                    // CJK char per line, which can look uglier than when not
                    // dealing with them specifically (see with: bool is_cjk=false).
                    // But Firefox does that too, may be a bit less radically than
                    // us, so our table algorithm may need some tweaking...
                if (flags[i] & LCHAR_ALLOW_WRAP_AFTER) { // A space
                    if (collapseNextSpace) // ignore this space
                        continue;
                    collapseNextSpace = true; // ignore next spaces, even if in another node
                    lastSpaceWidth = w;
                    curMaxWidth += w; // add this space to non-wrap width
                    if (curWordWidth > 0) { // there was a word before this space
                        if (start+i > 0) {
                            // adjust for last word's last char overflow (italic, letter f...)
                            lChar16 prevc = *(txt + start + i - 1);
                            int right_overflow = - font->getRightSideBearing(prevc, true, true);
                            curWordWidth += right_overflow;
                        }
                    }
                    if (curWordWidth > minWidth) // done with previous word
                        minWidth = curWordWidth; // longest word found
                    curWordWidth = 0;
                }
                else if (is_cjk) { // CJK chars are themselves a word
                    collapseNextSpace = false; // next space should not be ignored
                    lastSpaceWidth = 0; // no width to take off if we stop with this char
                    curMaxWidth += w;
                    if (curWordWidth > 0) { // there was a word or CJK char before this CJK char
                        if (start+i > 0) {
                            // adjust for last word's last char or previous CJK char right overflow
                            lChar16 prevc = *(txt + start + i - 1);
                            int right_overflow = - font->getRightSideBearing(prevc, true, true);
                            curWordWidth += right_overflow;
                        }
                    }
                    if (curWordWidth > minWidth) // done with previous word
                        minWidth = curWordWidth; // longest word found
                    curWordWidth = w;
                    // adjust for leading overflow
                    int left_overflow = - font->getLeftSideBearing(c, false, true);
                    curWordWidth += left_overflow;
                    if (start + i == 0) // at start of text only? (not sure)
                        curMaxWidth += left_overflow; // also add it to max width
                }
                else { // A char part of a word
                    collapseNextSpace = false; // next space should not be ignored
                    lastSpaceWidth = 0; // no width to take off if we stop with this char
                    if (curWordWidth == 0) { // first char of a word
                        // adjust for leading overflow on first char of a word
                        int left_overflow = - font->getLeftSideBearing(c, false, true);
                        curWordWidth += left_overflow;
                        if (start + i == 0) // at start of text only? (not sure)
                            curMaxWidth += left_overflow; // also add it to max width
                    }
                    curMaxWidth += w;
                    curWordWidth += w;
                    // Try to guess long urls or hostnames, and split a word on
                    // each / or dot not followed by a space, so they are not
                    // a super long word and don't over extend minWidth.
                    if ( (c == '/' || c == '.') &&
                            i < start+len-1 && *(txt + start + i + 1) != ' ') {
                        if (curWordWidth > minWidth)
                            minWidth = curWordWidth;
                        curWordWidth = 0;
                    }
                }
            }
            #endif // not USE_LIBUNIBREAK==1
            if ( chars_measured == len ) { // done with this text node
                if (curWordWidth > 0) { // we end with a word
                    if (start+len > 0) {
                        // adjust for word last char right overflow
                        lChar16 prevc = *(txt + start + len - 1);
                        int right_overflow = - font->getRightSideBearing(prevc, true, true);
                        curWordWidth += right_overflow;
                        curMaxWidth += right_overflow; // also add it to max width
                    }
                }
                break;
            }
            // continue measuring
            len -= chars_measured;
            start += chars_measured;
        }
    }
    #ifdef DEBUG_GETRENDEREDWIDTHS
        printf("GRW current: max=%d word=%d (max=%d, min=%d)\n", curMaxWidth, curWordWidth, maxWidth, minWidth);
    #endif
}
