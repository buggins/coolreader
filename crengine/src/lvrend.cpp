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
#include <lvtextfm.h>
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvrend.h"


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

int measureBorder(ldomNode *enode,int border);
// prototypes
int lengthToPx( css_length_t val, int base_px, int base_em );
void copystyle( css_style_ref_t sourcestyle, css_style_ref_t deststyle );
css_page_break_t getPageBreakBefore( ldomNode * el );
int CssPageBreak2Flags( css_page_break_t prop );
///////////////////////////////////////////////////////////////////////////////
//
// TABLE RENDERING CLASSES
//
///////////////////////////////////////////////////////////////////////////////

#define TABLE_BORDER_WIDTH 1

class CCRTableCol;
class CCRTableRow;

class CCRTableCell {
public:
    CCRTableCol * col;
    CCRTableRow * row;
    int width;
    int height;
    int percent;
    int txtlen;
    short colspan;
    short rowspan;
    short padding_left;
    short padding_right;
    short padding_top;
    short padding_bottom;
    char halign;
    char valign;
    ldomNode * elem;
    CCRTableCell() : col(NULL), row(NULL)
    , width(0)
    , height(0)
    , percent(0)
    , txtlen(0)
    , colspan(1)
    , rowspan(1)
    , padding_left(0)
    , padding_right(0)
    , padding_top(0)
    , padding_bottom(0)
    , halign(0)
    , valign(0)
    , elem(NULL)
    { }
};

class CCRTableRowGroup;

class CCRTableRow {
public:
    int index;
    int height;
    int y;
    int numcols; // sum of colspan
    int linkindex;
    ldomNode * elem;
    LVPtrVector<CCRTableCell> cells;
    CCRTableRowGroup * rowgroup;
    CCRTableRow() : index(0)
    , height(0)
    , y(0)
    , numcols(0) // sum of colspan
    , linkindex(-1)
    , elem(NULL)
    , rowgroup(NULL)
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
    int txtlen;
    int nrows;
    int x;      // sum of previous col widths
    LVPtrVector<CCRTableCell, false> cells;
    ldomNode * elem;
    CCRTableCol() :
    index(0)
    , width(0)
    , percent(0)
    , txtlen(0)
    , nrows(0)
    , x(0) // sum of previous col widths
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

class CCRTable {
public:
    int width;
    int digitwidth;
    ldomNode * elem;
    ldomNode * caption;
    int caption_h;
    LVPtrVector<CCRTableRow> rows;
    LVPtrVector<CCRTableCol> cols;
    LVPtrVector<CCRTableRowGroup> rowgroups;
    LVMatrix<CCRTableCell*> cells;
    CCRTableRowGroup * currentRowGroup;

    void ExtendCols( int ncols ) {
        while (cols.length()<ncols) {
            CCRTableCol * col = new CCRTableCol;
            col->index = cols.length();
            cols.add(col);
        }
    }

    int LookupElem( ldomNode * el, int state ) {
        if (!el->getChildCount())
            return 0;
        int colindex = 0;
        int tdindex = 0;
        for (int i=0; i<el->getChildCount(); i++) {
            ldomNode * item = el->getChildElementNode(i);
            if ( item ) {
                // for each child element
                lvdom_element_render_method rendMethod = item->getRendMethod();
                //CRLog::trace("LookupElem[%d] (%s, %d) %d", i, LCSTR(item->getNodeName()), state, (int)item->getRendMethod() );
                switch ( rendMethod ) {
                case erm_invisible:  // invisible: don't render
                    // do nothing: invisible
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
                        LookupElem( item, 0 );
                        currentRowGroup = NULL;
                    } else {
                    }
                    break;
                case erm_table_column_group: // table column group
                    // just fall into groups
                    LookupElem( item, 0 );
                    break;
                case erm_table_row: // table row
                    {
                        // rows of table
                        CCRTableRow * row = new CCRTableRow;
                        row->elem = item;
//						if ( item==NULL )
//							item = item;
                        if ( currentRowGroup ) {
                            // add row to group
                            row->rowgroup = currentRowGroup;
                            currentRowGroup->rows.add( row );
                        }
                        rows.add( row );
                        if (row->elem->hasAttribute(LXML_NS_ANY, attr_link)) {
                            lString16 lnk=row->elem->getAttributeValue(attr_link);
                            row->linkindex = lnk.atoi();
                        }
                        // recursion: search for inner elements
                        //int res =
                        LookupElem( item, 1 ); // lookup row
                    }
                    break;
                case erm_table_column: // table column
                    {
                        // cols width definitions
                        ExtendCols(colindex+1);
                        CCRTableCol * col = cols[colindex];
                        col->elem = item;
                        lString16 w = item->getAttributeValue(attr_width);
                        if (!w.empty()) {
                            // TODO: px, em, and other length types support
                            int wn = StrToIntPercent(w.c_str(), digitwidth);
                            if (wn<0)
                                col->percent = -wn;
                            else if (wn>0)
                                col->width = wn;
                        }
                        colindex++;
                    }
                    break;
                case erm_list_item:
                case erm_block:         // render as block element (render as containing other elements)
                case erm_final:         // final element: render the whole it's content as single render block
                case erm_mixed:         // block and inline elements are mixed: autobox inline portions of nodes; TODO
                case erm_table_cell:    // table cell
                    {
                        // <th> or <td> inside <tr>

                        if ( rows.length()==0 ) {
                            CCRTableRow * row = new CCRTableRow;
                            row->elem = item;
//                            if ( item==NULL )
//                                item = item;
                            if ( currentRowGroup ) {
                                // add row to group
                                row->rowgroup = currentRowGroup;
                                currentRowGroup->rows.add( row );
                            }
                            rows.add( row );
                        }


                        CCRTableCell * cell = new CCRTableCell;
                        cell->elem = item;
                        lString16 w = item->getAttributeValue(attr_width);
                        if (!w.empty()) {
                            int wn = StrToIntPercent(w.c_str(), digitwidth);
                            if (wn<0)
                                cell->percent = -wn;
                            else if (wn>0)
                                cell->width = wn;
                        }
                        int cs=StrToIntPercent(item->getAttributeValue(attr_colspan).c_str());
                        if (cs>0 && cs<100) {
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
                        // "align"
                        lString16 halign = item->getAttributeValue(attr_align);
                        if (halign == "center")
                            cell->halign = 1; // center
                        else if (halign == "right")
                            cell->halign = 2; // right
                        // "valign"
                        lString16 valign = item->getAttributeValue(attr_valign);
                        if (valign == "center")
                            cell->valign = 1; // center
                        else if (valign == "bottom")
                            cell->valign = 2; // bottom

                        cell->row = rows[rows.length()-1];
                        cell->row->cells.add( cell );
                        cell->row->numcols += cell->colspan;
                        ExtendCols( cell->row->numcols ); // update col count
                        tdindex++;
                    }
                    break;
                case erm_table_caption: // table caption
                    {
                        //TODO
                        caption = item;
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
                // update col width
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
        // init CELLS matrix
        cells.SetSize( rows.length(), cols.length(), NULL );
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                // init cell range in matrix  (x0,y0)[colspanXrowspan]
                CCRTableCell * cell = (rows[i]->cells[j]);
                int x0 = cell->col->index;
                int y0 = cell->row->index;
                for (int y=0; y<cell->rowspan; y++) {
                    for (int x=0; x<cell->colspan; x++) {
                        cells[y0+y][x0+x] = cell;
                    }
                }

                // calc cell text size
                lString16 txt = (cell->elem)->getText();
                int txtlen = txt.length();
                txtlen=cell->elem->getFont()->getTextWidth(txt.c_str(),txtlen);//use actuall string width to calculate
                //txtlen = (txtlen+(cell->colspan-1))/(cell->colspan + 1);
                for (int x=0; x<cell->colspan; x++) {
                    if ( txtlen > cols[x0+x]->txtlen )
                        cols[x0+x]->txtlen = txtlen;
                }
            }
        }
        int npercent=0;
        int sumpercent=0;
        int nwidth = 0;
        int sumwidth = 0;
        for (int x=0; x<cols.length(); x++) {
            if (cols[x]->percent>0) {
                sumpercent += cols[x]->percent;
                cols[x]->width = 0;
                npercent++;
            } else if (cols[x]->width>0) {
                sumwidth += cols[x]->width;
                nwidth++;
            }
        }
        int nrest = cols.length()-nwidth-npercent; // not specified
        int sumwidthpercent = 0; // percent of sum-width
        int fullWidth = width - measureBorder(elem,1)-measureBorder(elem,3);//TABLE_BORDER_WIDTH * 2;
        if (sumwidth) {
            sumwidthpercent = 100*sumwidth/fullWidth;
            if (sumpercent+sumwidthpercent+5*nrest>100) {
                // too wide: convert widths to percents
                for (int i=0; i<cols.length(); i++) {
                    if (cols[i]->width>0) {
                        cols[i]->percent = cols[i]->width*100/fullWidth;
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
        int maxpercent = 100-3*nrest;
        if (sumpercent>maxpercent) {
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
        int sumtext = 1;
        nwidth = 0;
        for (i=0; i<cols.length(); i++) {
            if (cols[i]->percent>0) {
                cols[i]->width = width * cols[i]->percent / 100;
                cols[i]->percent = 0;
            }
            if (cols[i]->width>0) {
                // calc width stats
                sumwidth += cols[i]->width;
                nwidth++;
            } else if (cols[i]->txtlen>0) {
                // calc text len sum of rest cols
                sumtext += cols[i]->txtlen;
            }
        }
        nrest = cols.length() - nwidth;
        int restwidth = width - sumwidth;
        // new pass: convert text len percent into width
        for (i=0; i<cols.length(); i++) {
            if (cols[i]->width==0) {
                cols[i]->width = cols[i]->txtlen * restwidth / sumtext;
                sumwidth += cols[i]->width;
                nwidth++;
            }
            if (cols[i]->width<8) { // extend too small cols!
                int delta = 8 - cols[i]->width;
                cols[i]->width+=delta;
                sumwidth += delta;
            }
        }
        if (sumwidth>fullWidth) {
            // too wide! rescale down
            int newsumwidth = 0;
            for (i=0; i<cols.length(); i++) {
                cols[i]->width = cols[i]->width * fullWidth / sumwidth;
                newsumwidth += cols[i]->width;
            }
            sumwidth = newsumwidth;
        }
        // distribute rest of width between all cols
        int rw=0;
        for (int x=0; x<cols.length(); x++) {
            if (cols[x]->width>cols[x]->txtlen)
            {
                rw+=(cols[x]->width-cols[x]->txtlen);
                cols[x]->width=cols[x]->txtlen;
            }
        }
        int restw = fullWidth - sumwidth+rw;
        if (restw>0 && cols.length()>0) {
            int a = restw / cols.length();
            int b = restw % cols.length();
            for (i=0; i<cols.length(); i++) {
                cols[i]->width += a;
                if (b>0) {
                    cols[i]->width ++;
                    b--;
                }
            }
        }
        // widths calculated ok!
        // update width of each cell
        for (i=0; i<rows.length(); i++) {
            for (j=0; j<rows[i]->cells.length(); j++) {
                // calculate width of cell
                CCRTableCell * cell = (rows[i]->cells[j]);
                cell->width = 0;
                int x0 = cell->col->index;
                for (int x=0; x<cell->colspan; x++) {
                    cell->width += cols[x0+x]->width;
                }
                // padding
                RenderRectAccessor fmt( cell->elem );
                int em = cell->elem->getFont()->getSize();
                int width = fmt.getWidth();
                cell->padding_left = (short)lengthToPx( cell->elem->getStyle()->padding[0], width, em );
                cell->padding_right = (short)lengthToPx( cell->elem->getStyle()->padding[1], width, em );
                cell->padding_top = (short)lengthToPx( cell->elem->getStyle()->padding[2], width, em );
                cell->padding_bottom = (short)lengthToPx( cell->elem->getStyle()->padding[3], width, em );
                if(elem->getStyle()->border_collapse==css_border_collapse)
                {//simple collapse by disable some borders and eliminate paddings
                    css_style_ref_t style = cell->elem->getStyle();
                    style->border_style_left = css_border_none;
                    style->border_style_bottom = css_border_none;
                    if (i==0) {
                        css_style_ref_t firstrowstyle(new css_style_rec_t);
                        copystyle(style,firstrowstyle);
                        firstrowstyle->border_style_top=css_border_none;
                        cell->elem->setStyle(firstrowstyle);
                    }
                    else cell->elem->setStyle(style);
                }
                else {
                    int n=rows[i]->cells.length();
                    int bsp_h=lengthToPx(elem->getStyle()->border_spacing[0],width,em);
                    int delta=(lengthToPx(elem->getStyle()->padding[0],width,em)+lengthToPx(elem->getStyle()->padding[1],width,em)+bsp_h)/n;
                    cell->width=cell->width-delta-bsp_h*(100+100/n)/100;
                }
            }
        }
        // update col x
        for (i=1; i<cols.length(); i++) {
            cols[i]->x = cols[i-1]->x + cols[i-1]->width;
        }
    }

    int renderCells( LVRendPageContext & context )
    {
        int posx=measureBorder(elem,3);
        int posy=measureBorder(elem,0);
        int em=elem->getFont()->getSize();
        css_style_ref_t table_style=elem->getStyle();
        int bsp_v=lengthToPx(table_style->border_spacing[1],width,em);
        int table_padding_top=lengthToPx(table_style->padding[2],width,em);
        int table_padding_bottom=lengthToPx(table_style->padding[3],width,em);
        int table_padding_left=lengthToPx(table_style->padding[0],width,em);
        int table_padding_right=lengthToPx(table_style->padding[1],width,em);
        int bsp_h=lengthToPx(table_style->border_spacing[0],width,em);
        bool border_collapse=(table_style->border_collapse==css_border_collapse);
        if (border_collapse) {
            table_padding_top=0;
            table_padding_bottom=0;
            table_padding_left=0;
            table_padding_right=0;
            bsp_v=0;
            bsp_h=0;
        }
        // render caption
        if ( caption ) {
            RenderRectAccessor fmt( caption );
            int em = caption->getFont()->getSize();
            int w = width-measureBorder(elem,3)-measureBorder(elem,1);
            int padding_left = lengthToPx( caption->getStyle()->padding[0], width, em );
            int padding_right = lengthToPx( caption->getStyle()->padding[1], width, em );
            int padding_top = lengthToPx( caption->getStyle()->padding[2], width, em );
            int padding_bottom = lengthToPx( caption->getStyle()->padding[3], width, em );
            LFormattedTextRef txform;
            caption_h = caption->renderFinalBlock( txform, &fmt, w - padding_left - padding_right ) + padding_top + padding_bottom+measureBorder(caption,0)+measureBorder(caption,2);
            fmt.setY( posy+table_padding_top ); //cell->padding_top ); //cell->row->y - cell->row->y );
            fmt.setX( posx ); // + cell->padding_left
            fmt.setWidth( w ); //  - cell->padding_left - cell->padding_right
            fmt.setHeight( caption_h ); // - cell->padding_top - cell->padding_bottom
            fmt.push();
        }
        int i, j;
        // calc individual cells dimensions
        for (i=0; i<rows.length(); i++) {
            CCRTableRow * row = rows[i];
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                //int x = cell->col->index;
                int y = cell->row->index;
                int n=rows[i]->cells.length();
                int delta=(table_padding_left+table_padding_right+bsp_h)/n;
                if ( i==y ) {
                    //upper left corner of cell

                    RenderRectAccessor fmt( cell->elem );
                    if ( cell->elem->getRendMethod()==erm_final ) {
                        LFormattedTextRef txform;
                        if (border_collapse&&j==rows[i]->cells.length()-1) cell->width+=measureBorder(cell->elem,1);
                        int h = cell->elem->renderFinalBlock( txform, &fmt, cell->width - cell->padding_left - cell->padding_right-delta-bsp_h*(100+100/n)/100);
                        cell->height = h + cell->padding_top + cell->padding_bottom+measureBorder(cell->elem,0)+measureBorder(cell->elem,2);
                        fmt.setY( posy +table_padding_top+bsp_v); //cell->padding_top ); //cell->row->y - cell->row->y );
                        fmt.setX( cell->col->x+posx+table_padding_left-delta*j+bsp_h); // + cell->padding_left
                        fmt.setWidth( cell->width); //  - cell->padding_left - cell->padding_right
                        fmt.setHeight( cell->height); // - cell->padding_top - cell->padding_bottom
                    } else if ( cell->elem->getRendMethod()!=erm_invisible ) {
                        LVRendPageContext emptycontext( NULL, context.getPageHeight() );
                        int h = renderBlockElement( context, cell->elem, posx, posy, cell->width-cell->padding_left-cell->padding_right );
                        cell->height = h;
                        fmt.setY( posy ); //cell->row->y - cell->row->y );
                        fmt.setX( cell->col->x+posx );
                        fmt.setWidth( cell->width );
                        fmt.setHeight( cell->height );
                    }
                    if ( cell->rowspan==1 ) {
                        if ( row->height < cell->height )
                            row->height = cell->height;
                    }
                }
            }
        }
        // update rows by multyrow cell height
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
        // update rows y and total height
        int h = caption_h;
        for (i=0; i<rows.length(); i++) {
            CCRTableRow * row = rows[i];
            row->y = h;
            h+=row->height+bsp_v;
            if (i==rows.length()-1&&!border_collapse)
                h+=bsp_v+table_padding_bottom+table_padding_top;
			if ( row->elem ) {
                RenderRectAccessor fmt( row->elem );
                fmt.setX(measureBorder(row->elem,3));
                fmt.setY(row->y + measureBorder(row->elem,0));
                fmt.setWidth( width - measureBorder(row->elem,1) -measureBorder(row->elem,3));
                fmt.setHeight( row->height );
            }
        }
        // update cell Y relative to row element
        // calc individual cells dimensions
        for (i=0; i<rows.length(); i++) {
            //CCRTableRow * row = rows[i];
            for (j=0; j<rows[i]->cells.length(); j++) {
                CCRTableCell * cell = rows[i]->cells[j];
                //int x = cell->col->index;
                int y = cell->row->index;
                if ( i==y ) {
                    RenderRectAccessor fmt( cell->elem );
                    //CCRTableCol * lastcol = cols[ cell->col->index + cell->colspan - 1 ];
                    //fmt->setWidth( lastcol->width + lastcol->x - cell->col->x - cell->padding_left - cell->padding_right );
                    CCRTableRow * lastrow = rows[ cell->row->index + cell->rowspan - 1 ];
                    fmt.setHeight( lastrow->height + lastrow->y - cell->row->y ); // - cell->padding_top - cell->padding_bottom
                }
            }
        }

        lvRect rect;
        elem->getAbsRect(rect);
        // split pages
        if ( context.getPageList() != NULL ) {
            //int break_before = CssPageBreak2Flags( node->getStyle()->page_break_before );
            //int break_after = CssPageBreak2Flags( node->getStyle()->page_break_after );
            //int break_inside = CssPageBreak2Flags( node->getStyle()->page_break_inside );
            if ( caption && caption_h ) {
                int line_flags = 0;  //TODO
                int y0 = rect.top+table_padding_top+(border_collapse?0:posy); // start of row
                int y1 = rect.top + caption_h + posy+table_padding_top; // end of row
                line_flags |= RN_SPLIT_AUTO << RN_SPLIT_BEFORE;
                line_flags |= RN_SPLIT_AVOID << RN_SPLIT_AFTER;
                context.AddLine(y0,
                    y1, line_flags);
            }
            int count = rows.length();
            for (int i=0; i<count; i++)
            {
                CCRTableRow * row = rows[ i ];
                int line_flags = 0;  //TODO
                int y0 = rect.top + row->y +table_padding_top+bsp_v+posy;// start of row
                int y1 = rect.top + row->y + row->height +bsp_v+table_padding_top+posy; // end of row
                if ( i==count-1) {
                    line_flags |= RN_SPLIT_AVOID << RN_SPLIT_BEFORE;
                   if (border_collapse)
                       y1 += measureBorder(elem,2);
                } else
                    line_flags |= RN_SPLIT_AUTO << RN_SPLIT_BEFORE;
                if ( i==0 ) {
                    line_flags |= RN_SPLIT_AVOID << RN_SPLIT_AFTER;
                    line_flags |= CssPageBreak2Flags(getPageBreakBefore(elem))<<RN_SPLIT_BEFORE;
                    if(border_collapse&&!caption) y0 -= posy;

                } else
                    line_flags |= RN_SPLIT_AUTO << RN_SPLIT_AFTER;
                //if (i==0)
                //    line_flags |= break_before << RN_SPLIT_BEFORE;
                //else
                //    line_flags |= break_inside << RN_SPLIT_BEFORE;
                //if (i==count-1)
                //    line_flags |= break_after << RN_SPLIT_AFTER;
                //else
                //    line_flags |= break_inside << RN_SPLIT_AFTER;

                context.AddLine(y0,
                    y1, line_flags);
            }
        }

        // update row groups placement
        for ( int i=0; i<rowgroups.length(); i++ ) {
            CCRTableRowGroup * grp = rowgroups[i];
            if ( grp->rows.length() > 0 ) {
                int y0 = grp->rows.first()->y;
                int y1 = grp->rows.last()->y + grp->rows.first()->height;
                RenderRectAccessor fmt( grp->elem );
                fmt.setY( y0 );
                fmt.setHeight( y1 - y0 );
                fmt.setX( 0 );
                fmt.setWidth( width );
                for ( int j=0; j<grp->rows.length(); j++ ) {
                    // make row Y position relative to group
                    RenderRectAccessor rowfmt( grp->rows[j]->elem );
                    rowfmt.setY( rowfmt.getY() - y0 );
                }
            }
        }


        return h + measureBorder(elem,0) +measureBorder(elem,2);
    }

    CCRTable(ldomNode * tbl_elem, int tbl_width, int dwidth) : digitwidth(dwidth) {
        currentRowGroup = NULL;
        caption = NULL;
        caption_h = 0;
        elem = tbl_elem;
        width = tbl_width;
        LookupElem( tbl_elem, 0 );
        PlaceCells();
    }
};



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
    int sz = style->font_size.value;
    if ( style->font_size.type != css_val_px && style->font_size.type != css_val_percent )
        sz >>= 8;
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
    LVFontRef fnt = fontMan->GetFont(
        sz,
        fw,
        style->font_style==css_fs_italic,
        style->font_family,
        lString8(style->font_name.c_str()),
        documentId);
    //fnt = LVCreateFontTransform( fnt, LVFONT_TRANSFORM_EMBOLDEN );
    return fnt;
}

int styleToTextFmtFlags( const css_style_ref_t & style, int oldflags )
{
    int flg = oldflags;
    if ( style->display == css_d_run_in ) {
        flg |= LTEXT_RUNIN_FLAG;
    } //else
    if (style->display != css_d_inline) {
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
                flg |= LTEXT_LAST_LINE_ALIGN_LEFT;
                break;
            case css_ta_inherit:
                break;
            }
        }
    }
    if ( style->white_space == css_ws_pre )
        flg |= LTEXT_FLAG_PREFORMATTED;
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

//=======================================================================
// Render final block
//=======================================================================
void renderFinalBlock( ldomNode * enode, LFormattedText * txform, RenderRectAccessor * fmt, int & baseflags, int ident, int line_h )
{
    if ( enode->isElement() )
    {
        lvdom_element_render_method rm = enode->getRendMethod();
        if ( rm == erm_invisible )
            return; // don't draw invisible
        //RenderRectAccessor fmt2( enode );
        //fmt = &fmt2;
        int flags = styleToTextFmtFlags( enode->getStyle(), baseflags );
        int width = fmt->getWidth();
        css_style_rec_t * style = enode->getStyle().get();
        if ((flags & LTEXT_FLAG_NEWLINE) && rm != erm_inline)
        {
            css_length_t len = style->text_indent;
            switch( len.type )
            {
            case css_val_percent:
                ident = width * len.value / 100;
                break;
            case css_val_px:
                ident = len.value;
                break;
            case css_val_em:
                ident = len.value * enode->getFont()->getSize() / 256;
                break;
            default:
                ident = 0;
                break;
            }
            len = style->line_height;
            switch( len.type )
            {
            case css_val_percent:
                line_h = len.value * 16 / 100;
                break;
            case css_val_px:
                line_h = len.value * 16 / enode->getFont()->getHeight();
                break;
            case css_val_em:
                line_h = len.value * 16 / 256;
                break;
            default:
                break;
            }
        }
        // save flags
        int f = flags;
        // vertical alignment flags
        switch (style->vertical_align)
        {
        case css_va_sub:
            flags |= LTEXT_VALIGN_SUB;
            break;
        case css_va_super:
            flags |= LTEXT_VALIGN_SUPER;
            break;
        case css_va_middle:
            flags |= LTEXT_VALIGN_MIDDLE;
            break;
        case css_va_baseline:
        default:
            break;
        }
        switch ( style->text_decoration ) {
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
        switch ( style->hyphenate ) {
            case css_hyph_auto:
                flags |= LTEXT_HYPHENATE;
                break;
            default:
                break;
        }

        if ( rm==erm_list_item ) {
            // put item number/marker to list
            lString16 marker;
            int marker_width = 0;

            ListNumberingPropsRef listProps =  enode->getDocument()->getNodeNumberingProps( enode->getParentNode()->getDataIndex() );
            if ( listProps.isNull() ) {
                int counterValue = 0;
                ldomNode * parent = enode->getParentNode();
                int maxWidth = 0;
                for ( int i=0; i<parent->getChildCount(); i++ ) {
                    lString16 marker;
                    int markerWidth = 0;
                    ldomNode * child = parent->getChildElementNode(i);
                    if ( child && child->getNodeListMarker( counterValue, marker, markerWidth ) ) {
                        if ( markerWidth>maxWidth )
                            maxWidth = markerWidth;
                    }
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
                    margin = -marker_width;
                marker += "\t";
                txform->AddSourceLine( marker.c_str(), marker.length(), cl, bgcl, font, flags|LTEXT_FLAG_OWNTEXT, line_h,
                                        margin, NULL );
                flags &= ~LTEXT_FLAG_NEWLINE;
            }
        }

        const css_elem_def_props_t * ntype = enode->getElementTypePtr();
//        if ( ntype ) {
//            CRLog::trace("Node %s is Object ?  %d", LCSTR(enode->getNodeName()), ntype->is_object );
//        } else {
//            CRLog::trace("Node %s (%d) has no css_elem_def_props_t", LCSTR(enode->getNodeName()), enode->getNodeId() );
//        }
        if ( ntype && ntype->is_object )
        {
#ifdef DEBUG_DUMP_ENABLED
            logfile << "+OBJECT ";
#endif
            // object element, like <IMG>
            bool isBlock = style->display == css_d_block;
            if ( isBlock ) {
                int flags = styleToTextFmtFlags( enode->getStyle(), baseflags );
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
                        txform->AddSourceLine( lines[i].c_str(), lines[i].length(), cl, bgcl, font, flags|LTEXT_FLAG_OWNTEXT, line_h, 0, NULL );
                }
                txform->AddSourceObject(flags, line_h, ident, enode );
                title = enode->getAttributeValue(attr_subtitle);
                if ( !title.empty() ) {
                    lString16Collection lines;
                    lines.parse(title, cs16("\\n"), true);
                    for ( int i=0; i<lines.length(); i++ )
                        txform->AddSourceLine( lines[i].c_str(), lines[i].length(), cl, bgcl, font, flags|LTEXT_FLAG_OWNTEXT, line_h, 0, NULL );
                }
                title = enode->getAttributeValue(attr_title);
                if ( !title.empty() ) {
                    lString16Collection lines;
                    lines.parse(title, cs16("\\n"), true);
                    for ( int i=0; i<lines.length(); i++ )
                        txform->AddSourceLine( lines[i].c_str(), lines[i].length(), cl, bgcl, font, flags|LTEXT_FLAG_OWNTEXT, line_h, 0, NULL );
                }
            } else {
                txform->AddSourceObject(baseflags, line_h, ident, enode );
                baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
            }
        }
        else
        {
            int cnt = enode->getChildCount();
#ifdef DEBUG_DUMP_ENABLED
            logfile << "+BLOCK [" << cnt << "]";
#endif
            // usual elements
            bool thisIsRunIn = enode->getStyle()->display==css_d_run_in;
            if ( thisIsRunIn )
                flags |= LTEXT_RUNIN_FLAG;
            for (int i=0; i<cnt; i++)
            {
                ldomNode * child = enode->getChildNode( i );
                renderFinalBlock( child, txform, fmt, flags, ident, line_h );
            }
            if ( thisIsRunIn ) {
                // append space to run-in object
                LVFont * font = enode->getFont().get();
                css_style_ref_t style = enode->getStyle();
                lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
                lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
                lChar16 delimiter[] = {UNICODE_NO_BREAK_SPACE, UNICODE_NO_BREAK_SPACE}; //160
                txform->AddSourceLine( delimiter, sizeof(delimiter)/sizeof(lChar16), cl, bgcl, font, LTEXT_FLAG_OWNTEXT | LTEXT_RUNIN_FLAG, line_h, 0, NULL );
                flags &= ~LTEXT_RUNIN_FLAG;
            }
        }


#ifdef DEBUG_DUMP_ENABLED
      for (int i=0; i<enode->getNodeLevel(); i++)
        logfile << " . ";
#endif
#ifdef DEBUG_DUMP_ENABLED
        lvRect rect;
        enode->getAbsRect( rect );
        logfile << "<" << enode->getNodeName() << ">     flags( "
            << baseflags << "-> " << flags << ")  rect( "
            << rect.left << rect.top << rect.right << rect.bottom << ")\n";
#endif

        // restore flags
        //***********************************
        baseflags = f; // to allow blocks in one level with inlines
        if ( enode->getNodeId()==el_br ) {
            // use the same alignment
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
                ident = 0;
                break;
            case css_ta_inherit:
                break;
            }
//            baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
//            LVFont * font = enode->getFont().get();
//            txform->AddSourceLine( L"\n", 1, 0, 0, font, baseflags | LTEXT_FLAG_OWNTEXT,
//                line_h, 0, enode, 0, 0 );
        } else {
            if (!(enode->getStyle()->display == css_d_inline && enode->getText8(0, 0).trim().empty() &&
                  enode->getParentNode()->getFirstChild() == enode))
            baseflags &= ~LTEXT_FLAG_NEWLINE; // clear newline flag
        }
        //baseflags &= ~LTEXT_RUNIN_FLAG;
    }
    else if ( enode->isText() )
    {
        // text nodes
        lString16 txt = enode->getText();
        if ( !txt.empty() )
        {

#ifdef DEBUG_DUMP_ENABLED
      for (int i=0; i<enode->getNodeLevel(); i++)
        logfile << " . ";
#endif
#ifdef DEBUG_DUMP_ENABLED
            logfile << "#text" << " flags( "
                << baseflags << ")\n";
#endif

            ldomNode * parent = enode->getParentNode();
            int tflags = LTEXT_FLAG_OWNTEXT;
            if ( parent->getNodeId() == el_a )
                tflags |= LTEXT_IS_LINK;
            LVFont * font = parent->getFont().get();
            css_style_ref_t style = parent->getStyle();
            lUInt32 cl = style->color.type!=css_val_color ? 0xFFFFFFFF : style->color.value;
            lUInt32 bgcl = style->background_color.type!=css_val_color ? 0xFFFFFFFF : style->background_color.value;
            if(!enode->getParentNode()->getParentNode()->isNull())
                if((enode->getParentNode()->getParentNode()->getStyle()->background_color.value)==
                        lInt32(bgcl))
                    bgcl=0xFFFFFFFF;

            lInt8 letter_spacing;
            css_length_t len = style->letter_spacing;
            switch( len.type )
            {
            case css_val_percent:
                letter_spacing = (lInt8)(font->getSize() * len.value / 100);
                break;
            case css_val_px:
                letter_spacing = (lInt8)(len.value);
                break;
            case css_val_em:
                letter_spacing = (lInt8)(len.value * font->getSize() / 256);
                break;
            default:
                letter_spacing = 0;
                break;
            }
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
            //int offs = 0;
            if ( txform->GetSrcCount()==0 && style->white_space!=css_ws_pre ) {
                // clear leading spaces for first text of paragraph
                int i=0;
                for ( ;txt.length()>i && (txt[i]==' ' || txt[i]=='\t'); i++ )
                    ;
                if ( i>0 ) {
                    txt.erase(0, i);
                    //offs = i;
                }
            }
            if ( txt.length()>0 )
                txform->AddSourceLine( txt.c_str(), txt.length(), cl, bgcl, font, baseflags | tflags,
                    line_h, ident, enode, 0, letter_spacing );
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
    dest->list_style_type = source->list_style_type ;
    dest->list_style_position = source->list_style_position ;
    dest->hyphenate = source->hyphenate ;
    dest->vertical_align = source->vertical_align ;
    dest->line_height = source->line_height ;
    dest->width = source->width ;
    dest->height = source->height ;
    dest->color = source->color ;
    dest->background_color = source->background_color ;
    dest->text_indent = source->text_indent ;
    dest->margin[0] = source->margin[0] ;
    dest->margin[1] = source->margin[1] ;
    dest->margin[2] = source->margin[2] ;
    dest->margin[3] = source->margin[3] ;
    dest->padding[0] = source->padding[0] ;
    dest->padding[1] = source->padding[1] ;
    dest->padding[2] = source->padding[2] ;
    dest->padding[3] = source->padding[3] ;
    dest->font_size.type = source->font_size.type ;
    dest->font_size.value = source->font_size.value ;
    dest->font_style = source->font_style ;
    dest->font_weight = source->font_weight ;
    dest->font_name = source->font_name ;
    dest->font_family = source->font_family;
    dest->border_style_top=source->border_style_top;
    dest->border_style_right=source->border_style_right;
    dest->border_style_bottom=source->border_style_bottom;
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
}

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
            css_style_ref_t newstyle( new css_style_rec_t );
            el->setStyle(newstyle);//can't modify styles directly, as the change in style cache will affect other node with same style
            if(!style.isNull())
            {
                copystyle(style,el->getStyle());
                el->getStyle()->page_break_before=css_pb_auto;
                el->getStyle()->page_break_inside=style->page_break_inside;
                el->getStyle()->page_break_after=style->page_break_after;
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

//measure border width, 0 for top,1 for right,2 for bottom,3 for left
int measureBorder(ldomNode *enode,int border) {
        int em = enode->getFont()->getSize();
        int width = 0;
        if (border==0){
                bool hastopBorder = (enode->getStyle()->border_style_top >= css_border_solid &&
                                     enode->getStyle()->border_style_top <= css_border_outset);
                int topBorderwidth = lengthToPx(enode->getStyle()->border_width[0], width, em);
                topBorderwidth = topBorderwidth != 0 ? topBorderwidth : 2;
                topBorderwidth = hastopBorder?topBorderwidth:0;
                return topBorderwidth;}
            else if (border==1){
                bool hasrightBorder = (enode->getStyle()->border_style_right >= css_border_solid &&
                                       enode->getStyle()->border_style_right <= css_border_outset);
                int rightBorderwidth = lengthToPx(enode->getStyle()->border_width[1], width, em);
                rightBorderwidth = rightBorderwidth != 0 ? rightBorderwidth : 2;
                rightBorderwidth = hasrightBorder?rightBorderwidth:0;
                return rightBorderwidth;}
            else if (border ==2){
                bool hasbottomBorder = (enode->getStyle()->border_style_bottom >= css_border_solid &&
                                        enode->getStyle()->border_style_bottom <= css_border_outset);
                int bottomBorderwidth = lengthToPx(enode->getStyle()->border_width[2], width, em);
                bottomBorderwidth = bottomBorderwidth != 0 ? bottomBorderwidth : 2;
                bottomBorderwidth = hasbottomBorder?bottomBorderwidth:0;
                return bottomBorderwidth;}
            else if (border==3){
                bool hasleftBorder = (enode->getStyle()->border_style_left >= css_border_solid &&
                                      enode->getStyle()->border_style_left <= css_border_outset);
                int leftBorderwidth = lengthToPx(enode->getStyle()->border_width[3], width, em);
                leftBorderwidth = leftBorderwidth != 0 ? leftBorderwidth : 2;
                leftBorderwidth = hasleftBorder?leftBorderwidth:0;
                return leftBorderwidth;}
           else return 0;
        }

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

int renderBlockElement( LVRendPageContext & context, ldomNode * enode, int x, int y, int width )
{
    if ( enode->isElement() )
    {
        bool isFootNoteBody = false;
        if ( enode->getNodeId()==el_section && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) {
            ldomNode * body = enode->getParentNode();
            while ( body != NULL && body->getNodeId()!=el_body )
                body = body->getParentNode();
            if ( body ) {
                if (body->getAttributeValue(attr_name) == "notes" || body->getAttributeValue(attr_name) == "comments")
                    if ( !enode->getAttributeValue(attr_id).empty() )
                        isFootNoteBody = true;
            }
        }
//        if ( isFootNoteBody )
//            CRLog::trace("renderBlockElement() : Footnote body detected! %s", LCSTR(ldomXPointer(enode,0).toString()) );
        //if (!fmt)
        //    crFatalError();
//        if ( enode->getNodeId() == el_empty_line )
//            x = x;
        int em = enode->getFont()->getSize();
        int margin_left = lengthToPx( enode->getStyle()->margin[0], width, em ) + DEBUG_TREE_DRAW;
        int margin_right = lengthToPx( enode->getStyle()->margin[1], width, em ) + DEBUG_TREE_DRAW;
        int margin_top = lengthToPx( enode->getStyle()->margin[2], width, em ) + DEBUG_TREE_DRAW;
        int margin_bottom = lengthToPx( enode->getStyle()->margin[3], width, em ) + DEBUG_TREE_DRAW;
        int padding_left = lengthToPx( enode->getStyle()->padding[0], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,3);
        int padding_right = lengthToPx( enode->getStyle()->padding[1], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,1);
        int padding_top = lengthToPx( enode->getStyle()->padding[2], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,0);
        int padding_bottom = lengthToPx( enode->getStyle()->padding[3], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,2);

        //margin_left += 50;
        //margin_right += 50;

        if (margin_left>0)
            x += margin_left;
        y += margin_top;

        bool flgSplit = false;
        width -= margin_left + margin_right;
        int h = 0;
        LFormattedTextRef txform;
        {
            //CRLog::trace("renderBlockElement - creating render accessor");
            RenderRectAccessor fmt( enode );
            fmt.setX( x );
            fmt.setY( y );
            fmt.setWidth( width );
            fmt.setHeight( 0 );
            fmt.push();

            int m = enode->getRendMethod();
            switch( m )
            {
            case erm_mixed:
                {
                    // TODO: autoboxing not supported yet
                }
                break;
            case erm_table:
                {
                    // ??? not sure
                    if ( isFootNoteBody )
                        context.enterFootNote( enode->getAttributeValue(attr_id) );
                    // recurse all sub-blocks for blocks
                    int y = 0;
                    lvRect r;
                    enode->getAbsRect(r);
                    lString16 name=enode->getNodeName();
                    bool TableCollapse=false;
                    if(name.lowercase().compare("table")==0&&enode->getStyle()->border_collapse==css_border_collapse){
                        TableCollapse=true;
                    }
                    if (margin_top>0) context.AddLine(r.top-margin_top, r.top-1, pagebreakhelper(enode,width));
                    if (padding_top>0&&!TableCollapse) context.AddLine(r.top,r.top+padding_top-1,pagebreakhelper(enode,width));
                    int h = renderTable( context, enode, 0, y, width );
                    y += h;
                    int st_y = lengthToPx( enode->getStyle()->height, em, em );
                    if ( y < st_y )
                        y = st_y;
                    fmt.setHeight( y ); //+ margin_top + margin_bottom ); //???
                    // ??? not sure
                    lvRect rect;
                    enode->getAbsRect(rect);
                    if(padding_bottom>0&&!TableCollapse)
                        context.AddLine(y+rect.top-padding_bottom,y+rect.top,RN_SPLIT_AFTER_AUTO);
                    if(margin_bottom>0)
                        context.AddLine(y+rect.top+1,y+rect.top+margin_bottom,RN_SPLIT_AFTER_AUTO);;
                    if ( isFootNoteBody )
                        context.leaveFootNote();
                    return y + margin_top + margin_bottom; // return block height
                }
                break;
            case erm_block:
                {
                    if ( isFootNoteBody )
                        context.enterFootNote( enode->getAttributeValue(attr_id) );


                    // recurse all sub-blocks for blocks
                    int y = padding_top;
                    int cnt = enode->getChildCount();
                    lvRect r;
                    enode->getAbsRect(r);
                    if (margin_top>0)
                        context.AddLine(r.top - margin_top, r.top, pagebreakhelper(enode,width));
                    if (padding_top>0)
                        context.AddLine(r.top,r.top+padding_top,pagebreakhelper(enode,width));
                    for (int i=0; i<cnt; i++)
                    {
                        ldomNode * child = enode->getChildNode( i );
                        //fmt.push();
                        int h = renderBlockElement( context, child, padding_left, y,
                            width - padding_left - padding_right );
                        y += h;
                    }
                    int st_y = lengthToPx( enode->getStyle()->height, em, em );
                    if ( y < st_y )
                        y = st_y;
                    fmt.setHeight( y + padding_bottom ); //+ margin_top + margin_bottom ); //???
                    lvRect rect;
                    enode->getAbsRect(rect);
                    if(padding_bottom>0)
                        context.AddLine(y+rect.top,y+rect.top+padding_bottom,margin_bottom>0?RN_SPLIT_AFTER_AUTO:CssPageBreak2Flags(getPageBreakAfter(enode))<<RN_SPLIT_AFTER);
                    if(margin_bottom>0)
                        context.AddLine(y+rect.top+padding_bottom,y+rect.top+padding_bottom+margin_bottom,CssPageBreak2Flags(getPageBreakAfter(enode))<<RN_SPLIT_AFTER);
                    if ( isFootNoteBody )
                        context.leaveFootNote();
                    return y + margin_top + margin_bottom + padding_bottom; // return block height
                }
                break;
            case erm_list_item:
            case erm_final:
            case erm_table_cell:
                {
                    if ( isFootNoteBody )
                        context.enterFootNote( enode->getAttributeValue(attr_id) );
                    // render whole node content as single formatted object
                    fmt.setWidth( width );
                    fmt.setX( fmt.getX() );
                    fmt.setY( fmt.getY() );
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
                crFatalError(); // error
                break;
            }
        }
        if ( flgSplit ) {
            lvRect rect;
            enode->getAbsRect(rect);
            // split pages
            if ( context.getPageList() != NULL ) {
                if (margin_top>0)
                        context.AddLine(rect.top-margin_top,rect.top,pagebreakhelper(enode,width));
                if (padding_top>0)
                        context.AddLine(rect.top,rect.top+padding_top,pagebreakhelper(enode,width));
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
                for (int i=0; i<count; i++)
                {
                    const formatted_line_t * line = txform->GetLineInfo(i);
                    int line_flags = 0; //TODO
                    if (i==0)
                        line_flags |= break_before << RN_SPLIT_BEFORE;
                    else
                        line_flags |= break_inside << RN_SPLIT_BEFORE;
                    if (i==count-1&&(padding_bottom+margin_bottom==0))
                        line_flags |= break_after << RN_SPLIT_AFTER;
                    else
                        line_flags |= break_inside << RN_SPLIT_AFTER;
                    context.AddLine(rect.top+line->y+padding_top, rect.top+line->y+line->height+padding_top, line_flags);
                    if(padding_bottom>0&&i==count-1)
                        context.AddLine(rect.bottom-padding_bottom,rect.bottom,margin_bottom>0?RN_SPLIT_AFTER_AUTO:CssPageBreak2Flags(getPageBreakAfter(enode))<<RN_SPLIT_AFTER);
                    if(margin_bottom>0&&i==count-1)
                        context.AddLine(rect.bottom,rect.bottom+margin_bottom,CssPageBreak2Flags(getPageBreakAfter(enode))<<RN_SPLIT_AFTER);
                    // footnote links analysis
                    if ( !isFootNoteBody && enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_FOOTNOTES) ) { // disable footnotes for footnotes
                        for ( int w=0; w<line->word_count; w++ ) {
                            // check link start flag for every word
                            if ( line->words[w].flags & LTEXT_WORD_IS_LINK_START ) {
                                const src_text_fragment_t * src = txform->GetSrcInfo( line->words[w].src_text_index );
                                if ( src && src->object ) {
                                    ldomNode * node = (ldomNode*)src->object;
                                    ldomNode * parent = node->getParentNode();
                                    if ( parent->getNodeId()==el_a && parent->hasAttribute(LXML_NS_ANY, attr_href )
                                            && parent->getAttributeValue(LXML_NS_ANY, attr_type ) == "note") {
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
            } // has page list
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

//draw border lines,support color,width,all styles, not support border-collapse
void DrawBorder(ldomNode *enode,LVDrawBuf & drawbuf,int x0,int y0,int doc_x,int doc_y,RenderRectAccessor fmt)
{
    bool hastopBorder = (enode->getStyle()->border_style_top >=css_border_solid&&enode->getStyle()->border_style_top<=css_border_outset);
    bool hasrightBorder = (enode->getStyle()->border_style_right >=css_border_solid&&enode->getStyle()->border_style_right<=css_border_outset);
    bool hasbottomBorder = (enode->getStyle()->border_style_bottom >=css_border_solid&&enode->getStyle()->border_style_bottom<=css_border_outset);
    bool hasleftBorder = (enode->getStyle()->border_style_left >=css_border_solid&&enode->getStyle()->border_style_left<=css_border_outset);

    if (hasbottomBorder or hasleftBorder or hasrightBorder or hastopBorder) {
        lUInt32 shadecolor=0x555555;
        lUInt32 lightcolor=0xAAAAAA;
        int em = enode->getFont()->getSize();
        int width = fmt.getWidth();
        int topBorderwidth = lengthToPx(enode->getStyle()->border_width[0],width,em);
        topBorderwidth=topBorderwidth!=0?topBorderwidth:2;
        int rightBorderwidth = lengthToPx(enode->getStyle()->border_width[1],width,em);
        rightBorderwidth=rightBorderwidth!=0?rightBorderwidth:2;
        int bottomBorderwidth = lengthToPx(enode->getStyle()->border_width[2],width,em);
        bottomBorderwidth=bottomBorderwidth!=0?bottomBorderwidth:2;
        int leftBorderwidth = lengthToPx(enode->getStyle()->border_width[3],width,em);
        leftBorderwidth=leftBorderwidth!=0?leftBorderwidth:2;
        int tbw=topBorderwidth,rbw=rightBorderwidth,bbw=bottomBorderwidth,lbw=leftBorderwidth;
        if (hastopBorder) {
            int dot=1,interval=0;//default style
            lUInt32 topBordercolor = enode->getStyle()->border_color[0].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (enode->getStyle()->border_color[0].type==css_val_color)
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
            left=(enode->getStyle()->border_style_left==css_border_dotted||enode->getStyle()->border_style_left==css_border_dashed)?0:left;
            right=(enode->getStyle()->border_style_right==css_border_dotted||enode->getStyle()->border_style_right==css_border_dashed)?0:right;
            lvPoint leftpoint1=lvPoint(x0+doc_x,y0+doc_y),
                    leftpoint2=lvPoint(x0+doc_x,y0+doc_y+0.5*topBorderwidth),
                    leftpoint3=lvPoint(x0+doc_x,doc_y+y0+topBorderwidth),
                    rightpoint1=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0),
                    rightpoint2=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0+0.5*topBorderwidth),
                    rightpoint3=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0+topBorderwidth);
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
                rightpoint1.x=x0+doc_x+fmt.getWidth();
                rightpoint1.y=doc_y+y0;
                rightpoint2.x=x0+doc_x+fmt.getWidth()-0.5*rightBorderwidth;
                rightpoint2.y=doc_y+y0+0.5*topBorderwidth;
                rightpoint3.x=x0+doc_x+fmt.getWidth()-rightBorderwidth;
                rightpoint3.y=doc_y+y0+topBorderwidth;
            } else rightBorderwidth=0;
            rightrate=(double)rightBorderwidth/(double)topBorderwidth;
            switch (enode->getStyle()->border_style_top){
                case css_border_dotted:
                    dot=interval=topBorderwidth;
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, topBordercolor,dot,interval,0);}
                    break;
                case css_border_dashed:
                    dot=3*topBorderwidth;
                    interval=3*topBorderwidth;
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, topBordercolor,dot,interval,0);}
                    break;
                case css_border_solid:
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, topBordercolor,dot,interval,0);}
                    break;
                case css_border_double:
                    for(int i=0;i<=(leftpoint2.y-leftpoint1.y)/(leftpoint2.y-leftpoint1.y>2?3:2);i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, topBordercolor);}
                    for(int i=0;i<=(leftpoint3.y-leftpoint2.y)/(leftpoint3.y-leftpoint2.y>2?3:2);i++)
                    {drawbuf.FillRect(leftpoint3.x-i*leftrate, leftpoint3.y-i, rightpoint3.x+i*rightrate,
                                      rightpoint3.y-i+1, topBordercolor);}
                    break;
                case css_border_groove:
                    for(int i=0;i<=leftpoint2.y-leftpoint1.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, shadecolor);}
                    for(int i=0;i<leftpoint3.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y+i, rightpoint2.x-i*rightrate,
                                      rightpoint2.y+i+1, lightcolor);}
                    break;
                case css_border_inset:
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, shadecolor,dot,interval,0);}
                    break;
                case css_border_outset:
                    for(int i=0;i<leftpoint3.y-leftpoint1.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y+i+1, lightcolor,dot,interval,0);}
                    break;
                case css_border_ridge:
                    for(int i=0;i<=leftpoint2.y-leftpoint1.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y+i, rightpoint1.x-i*rightrate,
                                     rightpoint1.y+i+1, lightcolor);}
                    for(int i=0;i<leftpoint3.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y+i, rightpoint2.x-i*rightrate,
                                      rightpoint2.y+i+1, shadecolor);}
                    break;
                default:
                    break;
            }
        }
        //right
        if (hasrightBorder) {
            int dot=1,interval=0;//default style
            lUInt32 rightBordercolor = enode->getStyle()->border_color[1].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (enode->getStyle()->border_color[1].type==css_val_color)
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
            up=(enode->getStyle()->border_style_top==css_border_dotted||enode->getStyle()->border_style_top==css_border_dashed)?1:up;
            down=(enode->getStyle()->border_style_bottom==css_border_dotted||enode->getStyle()->border_style_bottom==css_border_dashed)?1:down;
            lvPoint toppoint1=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0),
                    toppoint2=lvPoint(x0+doc_x+fmt.getWidth()-0.5*rightBorderwidth,doc_y+y0),
                    toppoint3=lvPoint(x0+doc_x+fmt.getWidth()-rightBorderwidth,doc_y+y0),
                    bottompoint1=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0+fmt.getHeight()),
                    bottompoint2=lvPoint(x0+doc_x+fmt.getWidth()-0.5*rightBorderwidth,doc_y+y0+fmt.getHeight()),
                    bottompoint3=lvPoint(x0+doc_x+fmt.getWidth()-rightBorderwidth,doc_y+y0+fmt.getHeight());
            double toprate=1,bottomrate=1;
            if (up==0) {
                toppoint3.y=doc_y+y0+topBorderwidth;
                toppoint2.y=doc_y+y0+0.5*topBorderwidth;
            } else topBorderwidth=0;
            toprate=(double)topBorderwidth/(double)rightBorderwidth;
            if (down==0) {
                bottompoint3.y=y0+doc_y+fmt.getHeight()-bottomBorderwidth;
                bottompoint2.y=y0+doc_y+fmt.getHeight()-0.5*bottomBorderwidth;
            } else bottomBorderwidth=0;
            bottomrate=(double)bottomBorderwidth/(double)rightBorderwidth;
            switch (enode->getStyle()->border_style_right){
                case css_border_dotted:
                    dot=interval=rightBorderwidth;
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i-1,toppoint1.y+i*toprate,bottompoint1.x-i,
                                         bottompoint1.y-i*bottomrate, rightBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_dashed:
                    dot=3*rightBorderwidth;
                    interval=3*rightBorderwidth;
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i-1,toppoint1.y+i*toprate,bottompoint1.x-i,
                                         bottompoint1.y-i*bottomrate, rightBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_solid:
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i-1,toppoint1.y+i*toprate,bottompoint1.x-i,
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
                        drawbuf.DrawLine(toppoint1.x-i-1,toppoint1.y+i*toprate,bottompoint1.x-i,
                                         bottompoint1.y-i*bottomrate+1, lightcolor,dot,interval,1);
                    }
                    break;
                case css_border_outset:
                    for (int i=0;i<toppoint1.x-toppoint3.x;i++){
                        drawbuf.DrawLine(toppoint1.x-i-1,toppoint1.y+i*toprate,bottompoint1.x-i,
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
            lUInt32 bottomBordercolor = enode->getStyle()->border_color[2].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (enode->getStyle()->border_color[2].type==css_val_color)
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
            left=(enode->getStyle()->border_style_left==css_border_dotted||enode->getStyle()->border_style_left==css_border_dashed)?1:left;
            right=(enode->getStyle()->border_style_right==css_border_dotted||enode->getStyle()->border_style_right==css_border_dashed)?1:right;
            lvPoint leftpoint1=lvPoint(x0+doc_x,y0+doc_y+fmt.getHeight()),
                    leftpoint2=lvPoint(x0+doc_x,y0+doc_y-0.5*bottomBorderwidth+fmt.getHeight()),
                    leftpoint3=lvPoint(x0+doc_x,doc_y+y0+fmt.getHeight()-bottomBorderwidth),
                    rightpoint1=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0+fmt.getHeight()),
                    rightpoint2=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0+fmt.getHeight()-0.5*bottomBorderwidth),
                    rightpoint3=lvPoint(x0+doc_x+fmt.getWidth(),doc_y+y0+fmt.getHeight()-bottomBorderwidth);
            double leftrate=1,rightrate=1;
            if (left==0) {
                leftpoint3.x=x0+doc_x+leftBorderwidth;
                leftpoint2.x=x0+doc_x+0.5*leftBorderwidth;
            }else leftBorderwidth=0;
            leftrate=(double)leftBorderwidth/(double)bottomBorderwidth;
            if (right==0) {
                rightpoint3.x=x0+doc_x+fmt.getWidth()-rightBorderwidth;
                rightpoint2.x=x0+doc_x+fmt.getWidth()-0.5*rightBorderwidth;
            } else rightBorderwidth=0;
            rightrate=(double)rightBorderwidth/(double)bottomBorderwidth;
            switch (enode->getStyle()->border_style_bottom){
                case css_border_dotted:
                    dot=interval=bottomBorderwidth;
                    for(int i=0;i<leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y-i+1, bottomBordercolor,dot,interval,0);}
                    break;
                case css_border_dashed:
                    dot=3*bottomBorderwidth;
                    interval=3*bottomBorderwidth;
                    for(int i=0;i<leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y-i+1, bottomBordercolor,dot,interval,0);}
                    break;
                case css_border_solid:
                    for(int i=0;i<leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate,
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
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y-i+1, lightcolor,dot,interval,0);}
                    break;
                case css_border_outset:
                    for(int i=0;i<=leftpoint1.y-leftpoint3.y;i++)
                    {drawbuf.DrawLine(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y-i+1, shadecolor,dot,interval,0);}
                    break;
                case css_border_ridge:
                    for(int i=0;i<=leftpoint1.y-leftpoint2.y;i++)
                    {drawbuf.FillRect(leftpoint1.x+i*leftrate, leftpoint1.y-i, rightpoint1.x-i*rightrate,
                                      rightpoint1.y-i+1, shadecolor);}
                    for(int i=0;i<leftpoint2.y-leftpoint3.y;i++)
                    {drawbuf.FillRect(leftpoint2.x+i*leftrate, leftpoint2.y-i, rightpoint2.x-i*rightrate,
                                      rightpoint2.y-i+1, lightcolor);}
                    break;
                default:break;
            }
        }
        //left
        if (hasleftBorder) {
            int dot=1,interval=0;//default style
            lUInt32 leftBordercolor = enode->getStyle()->border_color[3].value;
            topBorderwidth=tbw;
            rightBorderwidth=rbw;
            bottomBorderwidth=bbw;
            leftBorderwidth=lbw;
            if (enode->getStyle()->border_color[3].type==css_val_color)
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
            up=(enode->getStyle()->border_style_top==css_border_dotted||enode->getStyle()->border_style_top==css_border_dashed)?1:up;
            down=(enode->getStyle()->border_style_bottom==css_border_dotted||enode->getStyle()->border_style_bottom==css_border_dashed)?1:down;
            lvPoint toppoint1=lvPoint(x0+doc_x,doc_y+y0),
                    toppoint2=lvPoint(x0+doc_x+0.5*leftBorderwidth,doc_y+y0),
                    toppoint3=lvPoint(x0+doc_x+leftBorderwidth,doc_y+y0),
                    bottompoint1=lvPoint(x0+doc_x,doc_y+y0+fmt.getHeight()),
                    bottompoint2=lvPoint(x0+doc_x+0.5*leftBorderwidth,doc_y+y0+fmt.getHeight()),
                    bottompoint3=lvPoint(x0+doc_x+leftBorderwidth,doc_y+y0+fmt.getHeight());
            double toprate=1,bottomrate=1;
            if (up==0) {
                toppoint3.y=doc_y+y0+topBorderwidth;
                toppoint2.y=doc_y+y0+0.5*topBorderwidth;
            } else topBorderwidth=0;
            toprate=(double)topBorderwidth/(double)leftBorderwidth;
            if (down==0) {
                bottompoint3.y=y0+doc_y+fmt.getHeight()-bottomBorderwidth;
                bottompoint2.y=y0+doc_y+fmt.getHeight()-0.5*bottomBorderwidth;
            } else bottomBorderwidth=0;
            bottomrate=(double)bottomBorderwidth/(double)leftBorderwidth;
            switch (enode->getStyle()->border_style_left){
                case css_border_dotted:
                    dot=interval=leftBorderwidth;
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,leftBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_dashed:
                    dot=3*leftBorderwidth;
                    interval=3*leftBorderwidth;
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,leftBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_solid:
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,leftBordercolor,dot,interval,1);
                    }
                    break;
                case css_border_double:
                    for (int i=0;i<=(toppoint2.x-toppoint1.x)/(toppoint2.x-toppoint1.x>2?3:2);i++){
                        drawbuf.FillRect(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,leftBordercolor);
                    }
                    for (int i=0;i<=(toppoint3.x-toppoint2.x)/(toppoint3.x-toppoint2.x>2?3:2);i++){
                        drawbuf.FillRect(toppoint3.x-i,toppoint3.y-i*toprate,bottompoint3.x-i+1,
                                         bottompoint3.y+i*bottomrate,leftBordercolor);
                    }
                    break;
                case css_border_groove:
                    for (int i=0;i<=toppoint2.x-toppoint1.x;i++){
                        drawbuf.FillRect(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,shadecolor);
                    }
                    for (int i=0;i<toppoint3.x-toppoint2.x;i++){
                        drawbuf.FillRect(toppoint2.x+i,toppoint2.y+i*toprate,bottompoint2.x+i+1,
                                         bottompoint2.y-i*bottomrate,lightcolor);
                    }
                    break;
                case css_border_inset:
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,shadecolor,dot,interval,1);
                    }
                    break;
                case css_border_outset:
                    for (int i=0;i<toppoint3.x-toppoint1.x;i++){
                        drawbuf.DrawLine(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,lightcolor,dot,interval,1);
                    }
                    break;
                case css_border_ridge:
                    for (int i=0;i<=toppoint2.x-toppoint1.x;i++){
                        drawbuf.FillRect(toppoint1.x+i,toppoint1.y+i*toprate,bottompoint1.x+i+1,
                                         bottompoint1.y-i*bottomrate,lightcolor);
                    }
                    for (int i=0;i<toppoint3.x-toppoint2.x;i++){
                        drawbuf.FillRect(toppoint2.x+i,toppoint2.y+i*toprate,bottompoint2.x+i+1,
                                         bottompoint2.y-i*bottomrate,shadecolor);
                    }
                    break;
                default:break;
            }
        }
    }
}
void DrawBackgroundImage(ldomNode *enode,LVDrawBuf & drawbuf,int x0,int y0,int doc_x,int doc_y,RenderRectAccessor fmt)
{
    css_style_ref_t style=enode->getStyle();
    if (!style->background_image.empty()) {
        lString16 filename = lString16(style->background_image.c_str());
        {//url("path") to path
            if (filename.lowercase().startsWith("url")) filename = filename.substr(3);
            filename.trim();
            if (filename.lowercase().startsWith("(")) filename = filename.substr(1);
            if (filename.lowercase().endsWith(")")) filename = filename.substr(0, filename.length() - 1);
            filename.trim();
            if (filename.lowercase().startsWith("\"")) filename = filename.substr(1);
            if (filename.lowercase().endsWith("\"")) filename = filename.substr(0, filename.length() - 1);
            filename.trim();
            if (filename.lowercase().startsWith("../")) filename = filename.substr(3);
        }
        LVImageSourceRef img = enode->getParentNode()->getDocument()->getObjectImageSource(filename);
        if (!img.isNull()) {
            int repeat_times_x = 0, repeat_times_y = 0;
            int position_x = 0, position_y = 0, direction_x = 1, direction_y = 1, center_x = 0, center_y = 0;
            switch (style->background_repeat) {
                case css_background_no_repeat:
                    repeat_times_x = 0;
                    repeat_times_y = 0;
                    break;
                case css_background_repeat_x:
                    repeat_times_x = fmt.getWidth() / img->GetWidth() ? fmt.getWidth() / img->GetWidth() :
                                     fmt.getWidth() / img->GetWidth() + 1;
                    repeat_times_y = 0;
                    break;
                case css_background_repeat_y:
                    repeat_times_x = 0;
                    repeat_times_y = fmt.getHeight() / img->GetHeight() ? fmt.getHeight() / img->GetHeight() :
                                     fmt.getHeight() / img->GetHeight() + 1;
                    break;
                case css_background_repeat:
                    repeat_times_x = fmt.getWidth() / img->GetWidth() ? fmt.getWidth() / img->GetWidth() :
                                     fmt.getWidth() / img->GetWidth() + 1;
                    repeat_times_y = fmt.getHeight() / img->GetHeight() ? fmt.getHeight() / img->GetHeight() :
                                     fmt.getHeight() / img->GetHeight() + 1;
                    break;
                default:
                    repeat_times_x = fmt.getWidth() / img->GetWidth() ? fmt.getWidth() / img->GetWidth() :
                                     fmt.getWidth() / img->GetWidth() + 1;
                    repeat_times_y = fmt.getHeight() / img->GetHeight() ? fmt.getHeight() / img->GetHeight() :
                                     fmt.getHeight() / img->GetHeight() + 1;
                    break;
            }
            switch (style->background_position) {
                case css_background_center_bottom:
                    position_x = fmt.getWidth() / 2 - img->GetWidth() / 2;
                    position_y = fmt.getHeight() - img->GetHeight();
                    center_x = 1;
                    direction_y = -1;
                    break;
                case css_background_center_center:
                    position_x = fmt.getWidth() / 2 - img->GetWidth() / 2;;
                    position_y = fmt.getHeight() / 2 - img->GetHeight() / 2;
                    center_x = 1;
                    center_y = 1;
                    break;
                case css_background_center_top:
                    position_x = fmt.getWidth() / 2 - img->GetWidth() / 2;;
                    position_y = 0;
                    center_x = 1;
                    break;
                case css_background_left_bottom:
                    position_x = x0;
                    position_y = fmt.getHeight() - img->GetHeight();
                    direction_y = -1;
                    break;
                case css_background_left_center:
                    position_x = 0;
                    position_y = fmt.getHeight() / 2 - img->GetHeight() / 2;
                    center_y = 1;
                    break;
                case css_background_left_top:
                    position_x = 0;
                    position_y = 0;
                    break;
                case css_background_right_bottom:
                    position_x = fmt.getWidth() - img->GetWidth();
                    position_y = fmt.getHeight() - img->GetHeight();
                    direction_x = -1;
                    direction_y = -1;
                    break;
                case css_background_right_center:
                    position_x = fmt.getWidth() - img->GetWidth();
                    position_y = fmt.getHeight() / 2 - img->GetHeight() / 2;
                    direction_x = -1;
                    center_y = 1;
                    break;
                case css_background_right_top:
                    position_x = fmt.getWidth() - img->GetWidth();
                    position_y = 0;
                    direction_x = -1;
                    break;
                default:
                    position_x = 0;
                    position_y = 0;
            }
            LVDrawBuf *tmp = NULL;
            tmp = new LVColorDrawBuf(fmt.getWidth(), fmt.getHeight(), 32);
            for (int i = 0; i < repeat_times_x + 1; i++) {
                for (int j = 0; j < repeat_times_y + 1; j++) {
                    tmp->Draw(img, position_x + i * img->GetWidth() * direction_x,
                              position_y + j * img->GetHeight() * direction_y,
                              img->GetWidth(), img->GetHeight());
                    if (center_x == 1)
                        tmp->Draw(img, position_x - i * img->GetWidth() * direction_x,
                                  position_y + j * img->GetHeight() * direction_y,
                                  img->GetWidth(), img->GetHeight());
                    if (center_y == 1)
                        tmp->Draw(img, position_x + i * img->GetWidth() * direction_x,
                                  position_y - j * img->GetHeight() * direction_y,
                                  img->GetWidth(), img->GetHeight());
                }
            }
            tmp->DrawOnTop(&drawbuf, x0 + doc_x, y0 + doc_y);
            delete tmp;
        }
    }
}

void DrawDocument( LVDrawBuf & drawbuf, ldomNode * enode, int x0, int y0, int dx, int dy, int doc_x, int doc_y, int page_height, ldomMarkedRangeList * marks,
                   ldomMarkedRangeList *bookmarks)
{
    if ( enode->isElement() )
    {
        RenderRectAccessor fmt( enode );
        doc_x += fmt.getX();
        doc_y += fmt.getY();
        int em = enode->getFont()->getSize();
        int width = fmt.getWidth();
        int height = fmt.getHeight();
        bool draw_padding_bg = true; //( enode->getRendMethod()==erm_final );
        int padding_left = !draw_padding_bg ? 0 : lengthToPx( enode->getStyle()->padding[0], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,3);
        int padding_right = !draw_padding_bg ? 0 : lengthToPx( enode->getStyle()->padding[1], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,1);
        int padding_top = !draw_padding_bg ? 0 : lengthToPx( enode->getStyle()->padding[2], width, em ) + DEBUG_TREE_DRAW+measureBorder(enode,0);
        //int padding_bottom = !draw_padding_bg ? 0 : lengthToPx( enode->getStyle()->padding[3], width, em ) + DEBUG_TREE_DRAW;
        if ( (doc_y + height <= 0 || doc_y > 0 + dy)
            && (
               enode->getRendMethod()!=erm_table_row
               && enode->getRendMethod()!=erm_table_row_group
            ) ) //0~=y0
        {
            return; // out of range
        }
        css_length_t bg = enode->getStyle()->background_color;
        lUInt32 oldColor = 0;
        if ( bg.type==css_val_color ) {
            oldColor = drawbuf.GetBackgroundColor();
            drawbuf.SetBackgroundColor( bg.value );
            drawbuf.FillRect( x0 + doc_x, y0 + doc_y, x0 + doc_x+fmt.getWidth(), y0+doc_y+fmt.getHeight(), bg.value );
        }
        lString16 nodename=enode->getNodeName();    // CSS specific: <body> background does not obey margin rules
        if (nodename.lowercase().compare("body")==0&&enode->getStyle()->background_image!=lString8(""))
        {
            int width=fmt.getWidth();
            fmt.setWidth(drawbuf.GetWidth());
            DrawBackgroundImage(enode,drawbuf,0,y0,0,doc_y,fmt);
            fmt.setWidth(width);
        }
        else  DrawBackgroundImage(enode,drawbuf,x0,y0,doc_x,doc_y,fmt);
#if (DEBUG_TREE_DRAW!=0)
        lUInt32 color;
        static lUInt32 const colors2[] = { 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA, 0x555555, 0xAAAAAA };
        static lUInt32 const colors4[] = { 0x555555, 0xFF4040, 0x40FF40, 0x4040FF, 0xAAAAAA, 0xFF8000, 0xC0C0C0, 0x808080 };
        if (drawbuf.GetBitsPerPixel()>=16)
            color = colors4[enode->getNodeLevel() & 7];
        else
            color = colors2[enode->getNodeLevel() & 7];
#endif
        switch( enode->getRendMethod() )
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
                for (int i=0; i<cnt; i++)
                {
                    ldomNode * child = enode->getChildNode( i );
                    DrawDocument( drawbuf, child, x0, y0, dx, dy, doc_x, doc_y, page_height, marks, bookmarks ); //+fmt->getX() +fmt->getY()
                }
#if (DEBUG_TREE_DRAW!=0)
                drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+fmt.getWidth(), doc_y+y0+1, color );
                drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+1, doc_y+y0+fmt.getHeight(), color );
                drawbuf.FillRect( doc_x+x0+fmt.getWidth()-1, doc_y+y0, doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), color );
                drawbuf.FillRect( doc_x+x0, doc_y+y0+fmt.getHeight()-1, doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), color );
#endif
                /*lUInt32 tableBorderColor = 0xAAAAAA;
                lUInt32 tableBorderColorDark = 0x555555;
                bool needBorder = enode->getRendMethod()==erm_table || enode->getStyle()->display==css_d_table_cell;
                if ( needBorder ) {
                   drawbuf.FillRect( doc_x+x0, doc_y+y0,
                                      doc_x+x0+fmt.getWidth(), doc_y+y0+1, tableBorderColor );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0,
                                      doc_x+x0+1, doc_y+y0+fmt.getHeight(), tableBorderColor );
                    drawbuf.FillRect( doc_x+x0+fmt.getWidth()-1, doc_y+y0,
                                      doc_x+x0+fmt.getWidth(),   doc_y+y0+fmt.getHeight(), tableBorderColorDark );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0+fmt.getHeight()-1,
                                      doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), tableBorderColorDark );
                }*/
                 DrawBorder(enode,drawbuf,x0,y0,doc_x,doc_y,fmt);
            	}
            break;
        case erm_list_item:
        case erm_final:
        case erm_table_caption:
            {
                // draw whole node content as single formatted object
                LFormattedTextRef txform;
                enode->renderFinalBlock( txform, &fmt, fmt.getWidth() - padding_left - padding_right );
                fmt.push();
                {
                    lvRect rc;
                    enode->getAbsRect( rc );
                    ldomMarkedRangeList *nbookmarks = NULL;
                    if ( bookmarks && bookmarks->length()) {
                        nbookmarks = new ldomMarkedRangeList( bookmarks, rc );
                    }
                    if ( marks && marks->length() ) {
                        //rc.left -= doc_x;
                        //rc.right -= doc_x;
                        //rc.top -= doc_y;
                        //rc.bottom -= doc_y;
                        ldomMarkedRangeList nmarks( marks, rc );
                        DrawBackgroundImage(enode,drawbuf,x0,y0,doc_x,doc_y,fmt);
                        txform->Draw( &drawbuf, doc_x+x0 + padding_left, doc_y+y0 + padding_top, &nmarks, nbookmarks );
                    } else {
                        DrawBackgroundImage(enode,drawbuf,x0,y0,doc_x,doc_y,fmt);
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
                DrawBorder(enode, drawbuf, x0, y0, doc_x, doc_y, fmt);
                /*lUInt32 tableBorderColor = 0x555555;
                lUInt32 tableBorderColorDark = 0xAAAAAA;
                bool needBorder = enode->getStyle()->display==css_d_table_cell;
                if ( needBorder ) {
                    drawbuf.FillRect( doc_x+x0, doc_y+y0,
                                      doc_x+x0+fmt.getWidth(), doc_y+y0+1, tableBorderColor );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0,
                                      doc_x+x0+1, doc_y+y0+fmt.getHeight(), tableBorderColor );
                    drawbuf.FillRect( doc_x+x0+fmt.getWidth()-1, doc_y+y0,
                                      doc_x+x0+fmt.getWidth(),   doc_y+y0+fmt.getHeight(), tableBorderColorDark );
                    drawbuf.FillRect( doc_x+x0, doc_y+y0+fmt.getHeight()-1,
                                      doc_x+x0+fmt.getWidth(), doc_y+y0+fmt.getHeight(), tableBorderColorDark );
                    //drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+fmt->getWidth(), doc_y+y0+1, tableBorderColorDark );
                    //drawbuf.FillRect( doc_x+x0, doc_y+y0, doc_x+x0+1, doc_y+y0+fmt->getHeight(), tableBorderColorDark );
                    //drawbuf.FillRect( doc_x+x0+fmt->getWidth()-1, doc_y+y0, doc_x+x0+fmt->getWidth(), doc_y+y0+fmt->getHeight(), tableBorderColor );
                    //drawbuf.FillRect( doc_x+x0, doc_y+y0+fmt->getHeight()-1, doc_x+x0+fmt->getWidth(), doc_y+y0+fmt->getHeight(), tableBorderColor );
                }*/
            }
            break;
        case erm_invisible:
            // don't draw invisible blocks
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

inline void spreadParent( css_length_t & val, css_length_t & parent_val, bool inherited=true )
{
    if ( val.type == css_val_inherited || (val.type == css_val_unspecified && inherited))
        val = parent_val;
}

void setNodeStyle( ldomNode * enode, css_style_ref_t parent_style, LVFontRef parent_font )
{
    CR_UNUSED(parent_font);
    //lvdomElementFormatRec * fmt = node->getRenderData();
    css_style_ref_t style( new css_style_rec_t );
    css_style_rec_t * pstyle = style.get();

//    if ( parent_style.isNull() ) {
//        CRLog::error("parent style is null!!!");
//    }

    // init default style attribute values
    const css_elem_def_props_t * type_ptr = enode->getElementTypePtr();
    if (type_ptr)
    {
        pstyle->display = type_ptr->display;
        pstyle->white_space = type_ptr->white_space;
    }

    int baseFontSize = enode->getDocument()->getDefaultFont()->getSize();

    //////////////////////////////////////////////////////
    // apply style sheet
    //////////////////////////////////////////////////////
    enode->getDocument()->applyStyle( enode, pstyle );

    if ( enode->getDocument()->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) && enode->hasAttribute( LXML_NS_ANY, attr_style ) ) {
        lString16 nodeStyle = enode->getAttributeValue( LXML_NS_ANY, attr_style );
        if ( !nodeStyle.empty() ) {
            nodeStyle = cs16("{") + nodeStyle + "}";
            LVCssDeclaration decl;
            lString8 s8 = UnicodeToUtf8(nodeStyle);
            const char * s = s8.c_str();
            if ( decl.parse( s ) ) {
                decl.apply( pstyle );
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
        case css_val_percent: \
            pstyle->fld.type = parent_style->fld.type; \
            pstyle->fld.value = parent_style->fld.value * pstyle->fld.value / 100; \
            break; \
        case css_val_px: \
            /*no need to change*/\
            break; \
        case css_val_pt: \
            /*treat as px*/\
            pstyle->fld.type = css_val_px; \
            pstyle->fld.value = pstyle->fld.value/256; \
            break; \
        case css_val_em: \
            pstyle->fld.type = css_val_px; \
            pstyle->fld.value = parent_style->font_size.value * pstyle->fld.value / 256; \
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
    UPDATE_STYLE_FIELD( hyphenate, css_hyph_inherit );
    UPDATE_STYLE_FIELD( list_style_type, css_lst_inherit );
    UPDATE_STYLE_FIELD( list_style_position, css_lsp_inherit );
    UPDATE_STYLE_FIELD( page_break_before, css_pb_inherit );
    UPDATE_STYLE_FIELD( page_break_after, css_pb_inherit );
    UPDATE_STYLE_FIELD( page_break_inside, css_pb_inherit );
    UPDATE_STYLE_FIELD( vertical_align, css_va_inherit );
    UPDATE_STYLE_FIELD( font_style, css_fs_inherit );
    UPDATE_STYLE_FIELD( font_weight, css_fw_inherit );
    if ( pstyle->font_family == css_ff_inherit ) {
        //UPDATE_STYLE_FIELD( font_name, "" );
        pstyle->font_name = parent_font.get()->getTypeFace();
    }
    UPDATE_STYLE_FIELD( font_family, css_ff_inherit );
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
    spreadParent( pstyle->letter_spacing, parent_style->letter_spacing );
    spreadParent( pstyle->line_height, parent_style->line_height );
    spreadParent( pstyle->color, parent_style->color );
    spreadParent( pstyle->background_color, parent_style->background_color, false );

    // set calculated style
    //enode->getDocument()->cacheStyle( style );
    enode->setStyle( style );
    if ( enode->getStyle().isNull() ) {
        CRLog::error("NULL style set!!!");
        enode->setStyle( style );
    }

    // set font
    enode->initNodeFont();
}

int renderTable( LVRendPageContext & context, ldomNode * node, int x, int y, int width )
{
    CR_UNUSED2(x, y);
    CCRTable table( node, width, 10 );
    int h = table.renderCells( context );

    return h;
}
