/*******************************************************

   CoolReader Engine

   lvdocview.cpp:  XML DOM tree rendering tools

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/


#include "../include/lvdocview.h"

#include "../include/lvstyles.h"
#include "../include/lvrend.h"
#include "../include/lvstsheet.h"

#include "../include/fb2def.h"
#include "../include/wolutil.h"
#include "../include/crtxtenc.h"

const char * def_stylesheet =
"image { text-align: center; text-indent: 0px } \n"
"empty-line { height: 1em; } \n"
"sub { vertical-align: sub; font-size: 70% }\n"
"sup { vertical-align: super; font-size: 70% }\n"
"body > image, section > image { text-align: center; margin-before: 1em; margin-after: 1em }\n"
"p > image { display: inline }\n"
"a { vertical-align: super; font-size: 80% }\n"
"p { margin-top:0em; margin-bottom: 0em }\n"
"text-author { font-weight: bold; font-style: italic; margin-left: 5%}\n"
"empty-line { height: 1em }\n"
"epigraph { margin-left: 30%; margin-right: 4%; text-align: left; text-indent: 1px; font-style: italic; margin-top: 15px; margin-bottom: 25px; font-family: Times New Roman, serif }\n"
"strong { font-weight: bold }\n"
"emphasis { font-style: italic }\n"
"title { text-align: center; text-indent: 0px; font-size: 130%; font-weight: bold; margin-top: 10px; margin-bottom: 10px; font-family: Times New Roman, serif }\n"
"subtitle { text-align: center; text-indent: 0px; font-size: 150%; margin-top: 10px; margin-bottom: 10px }\n"
"title { page-break-before: always; page-break-inside: avoid; page-break-after: avoid; }\n"
"body { text-align: justify; text-indent: 2em; line-height: 140% }\n"
"cite { margin-left: 30%; margin-right: 4%; text-align: justyfy; text-indent: 0px;  margin-top: 20px; margin-bottom: 20px; font-family: Times New Roman, serif }\n"
;

static const char * DEFAULT_FONT_NAME = "Arial"; //Times New Roman";
static css_font_family_t DEFAULT_FONT_FAMILY = css_ff_sans_serif;
//    css_ff_serif,
//    css_ff_sans_serif,
//    css_ff_cursive,
//    css_ff_fantasy,
//    css_ff_monospace

#define INFO_FONT_SIZE      20    
#define DEFAULT_PAGE_MARGIN 10
    
LVDocView::LVDocView() 
: m_dx(100), m_dy(100), m_pos(50)
#if (LBOOK==1)
, m_font_size(36)
#else
, m_font_size(24)
#endif
, m_view_mode( 1 ? DVM_PAGES : DVM_SCROLL ) // choose 0/1
, m_drawbuf(100, 100
#if COLOR_BACKBUFFER==0
            , GRAY_BACKBUFFER_BITS
#endif
            ), m_stream(NULL), m_doc(NULL)
, m_stylesheet( def_stylesheet )
, m_is_rendered(false)
, m_pageMargins(DEFAULT_PAGE_MARGIN, DEFAULT_PAGE_MARGIN + INFO_FONT_SIZE + 4, DEFAULT_PAGE_MARGIN, DEFAULT_PAGE_MARGIN)
, m_pageHeaderInfo ( 
      PGHDR_PAGE_NUMBER
    | PGHDR_PAGE_COUNT
    | PGHDR_AUTHOR
    | PGHDR_TITLE)
, m_showCover(true)

{ 
#if (COLOR_BACKBUFFER==1)
    m_backgroundColor = 0xFFFFE0;
    m_textColor = 0x000060;
#else
#if (GRAY_INVERSE==1)
    m_backgroundColor = 0;
    m_textColor = 3;
#else
    m_backgroundColor = 3;
    m_textColor = 0;
#endif
#endif
    m_drawbuf.Clear(m_backgroundColor);
}

LVDocView::~LVDocView()
{
    Clear();
}

void LVDocView::setPageHeaderInfo( int hdrFlags )
{ 
    m_pageHeaderInfo = hdrFlags;
    int oldMargin = m_pageMargins.top;
    m_pageMargins.top = m_pageMargins.bottom + (hdrFlags ? 16 : 0);
    if ( m_pageMargins.top != oldMargin ) {
        Render();
    }
}

/// set document stylesheet text
void LVDocView::setStyleSheet( lString8 css_text )
{
    m_stylesheet = css_text;
}

void LVDocView::Clear()
{
    if (m_doc)
        delete m_doc;
    m_doc = NULL;
    if (!m_stream.isNull())
        m_stream.Clear();
    if (!m_arc.isNull())
        m_arc.Clear();
    _posBookmark = ldomXPointer();
    m_is_rendered = false;
    m_pos = 0;
}

bool LVDocView::exportWolFile( const char * fname, bool flgGray, int levels )
{
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_WRITE);
    if (!stream)
        return false;
	return exportWolFile( stream.get(), flgGray, levels );
}

bool LVDocView::exportWolFile( const wchar_t * fname, bool flgGray, int levels )
{
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_WRITE);
    if (!stream)
        return false;
	return exportWolFile( stream.get(), flgGray, levels );
}

lString16 getSectionHeader( ldomElement * section )
{
    lString16 header;
    if ( !section || section->getChildCount() == 0 )
        return header;
    ldomElement * child = (ldomElement *)section->getChildNode(0);
    if ( !child->isElement() || child->getNodeName()!=L"title" )
        return header;
    header = child->getText();
    return header;
}


void dumpSection( ldomElement * elem )
{
    lvRect rc;
    elem->getAbsRect(rc);
    //fprintf( log.f, "rect(%d, %d, %d, %d)  ", rc.left, rc.top, rc.right, rc.bottom );
}


int getSectionPage( ldomElement * section, LVRendPageList & pages )
{
    if ( !section )
        return -1;
#if 1
    int y = ldomXPointer( section, 0 ).toPoint().y;
#else
    lvRect rc;
    section->getAbsRect(rc);
    int y = rc.top;
#endif
    int page = -1;
    if ( y>=0 ) {
        page = pages.FindNearestPage( y, 0 );
        //dumpSection( section );
        //fprintf(log.f, "page %d: %d->%d..%d\n", page+1, y, pages[page].start, pages[page].start+pages[page].height );
    }
    return page;
}

/// returns cover page image source, if any
LVImageSourceRef LVDocView::getCoverPageImage()
{
    ldomElement * cover_img_el = ((ldomElement*)m_doc->getMainNode())
        ->findChildElement( LXML_NS_ANY, el_FictionBook, -1 )
        ->findChildElement( LXML_NS_ANY, el_description, -1 )
        ->findChildElement( LXML_NS_ANY, el_title_info, -1 )
        ->findChildElement( LXML_NS_ANY, el_coverpage, -1 )
        ->findChildElement( LXML_NS_ANY, el_image, -1 );

    if ( cover_img_el )
    {
        LVImageSourceRef imgsrc = cover_img_el->getObjectImageSource();
        return imgsrc;
    }
    return LVImageSourceRef(); // not found: return NULL ref
}


/// draws coverpage to image buffer
void LVDocView::drawCoverTo( LVDrawBuf * drawBuf, lvRect & rc )
{
    LVImageSourceRef imgsrc = getCoverPageImage();
    if ( !imgsrc.isNull() )
    {
        //fprintf( stderr, "Writing coverpage image...\n" );
        int src_dx = imgsrc->GetWidth();
        int src_dy = imgsrc->GetHeight();
        int scale_x = rc.width() * 0x10000 / src_dx;
        int scale_y = rc.height() * 0x10000 / src_dy;
        if ( scale_x < scale_y )
            scale_y = scale_x;
        else
            scale_x = scale_y;
        int dst_dx = (src_dx * scale_x) >> 16;
        int dst_dy = (src_dy * scale_y) >> 16;
        if (dst_dx>rc.width())
            dst_dx = rc.width();
        if (dst_dy>rc.height())
            dst_dy = rc.height();
        drawBuf->Draw( imgsrc, rc.left + (rc.width()-dst_dx)/2, rc.top + (rc.height()-dst_dy)/2, dst_dx, dst_dy );
        //fprintf( stderr, "Done.\n" );
    }
    else
    {

        m_font->DrawTextString(drawBuf, 10, 10, L"Testing WOL export", 18, '?', NULL, false);
        m_font->DrawTextString(drawBuf, 30, 50, L"NO COVERPAGE", 9, '?', NULL, false);
        drawBuf->FillRect(20, 80, 580, 90, 1);
        drawBuf->FillRect(20, 90, 580, 100, 2);
        drawBuf->FillRect(20, 100, 580, 110, 3);

    }
}

/// export to WOL format
bool LVDocView::exportWolFile( LVStream * stream, bool flgGray, int levels )
{
    LVRendPageList pages;
    Render(600, 800, &pages);

    const lChar8 * * table = GetCharsetUnicode2ByteTable( L"windows-1251" );

    //ldomXPointer bm = getBookmark();
    {
        WOLWriter wol(stream);
        lString8 authors = UnicodeTo8Bit( getAuthors(), table );
        lString8 name = UnicodeTo8Bit( getTitle(), table );
        wol.addTitle(
                name,
                lString8("-"),
                authors,
                lString8("-"), //adapter
                lString8("-"), //translator
                lString8("-"), //publisher
                lString8("-"), //2006-11-01
                lString8("-"), //This is introduction.
                lString8("")   //ISBN
        );

        LVGrayDrawBuf cover(600, 800);
        lvRect coverRc( 0, 0, 600, 800 );
        drawCoverTo( &cover, coverRc );
        wol.addCoverImage(cover);
        
        for (int i=0; i<pages.length(); i++)
        {
			LVGrayDrawBuf drawbuf(600, 800, flgGray ? 2 : 1); //flgGray ? 2 : 1);
			drawPageTo( &drawbuf, *pages[i] );
            if (!flgGray)
                drawbuf.ConvertToBitmap(false);
            else
                drawbuf.Invert();
            wol.addImage(drawbuf);
        }

        // add TOC
        ldomElement * body = (ldomElement *)m_doc->nodeFromXPath( lString16(L"/FictionBook/body[0]") );
        lUInt16 section_id = m_doc->getElementNameIndex( L"section" );

        if ( body ) {
            int l1n = 0;
            for ( int l1=0; l1<1000; l1++ ) {
                ldomElement * l1section = body->findChildElement(LXML_NS_ANY, section_id, l1);
                if ( !l1section )
                    break;
                lString8 title = UnicodeTo8Bit(getSectionHeader( l1section ), table);
                int page = getSectionPage( l1section, pages );
                if ( !title.empty() && page>=0 ) {
                    wol.addTocItem( ++l1n, 0, 0, page, title );
                    int l2n = 0;
                    if ( levels<2 )
                        continue;
                    for ( int l2=0; l2<1000; l2++ ) {
                        ldomElement * l2section = l1section->findChildElement(LXML_NS_ANY, section_id, l2);
                        if ( !l2section )
                            break;
                        lString8 title = UnicodeTo8Bit(getSectionHeader( l2section ), table);
                        int page = getSectionPage( l2section, pages );
                        if ( !title.empty() && page>=0 ) {
                            wol.addTocItem( l1n, ++l2n, 0, page, title );
                            int l3n = 0;
                            if ( levels<3 )
                                continue;
                            for ( int l3=0; l3<1000; l3++ ) {
                                ldomElement * l3section = l2section->findChildElement(LXML_NS_ANY, section_id, l3);
                                if ( !l3section )
                                    break;
                                lString8 title = UnicodeTo8Bit(getSectionHeader( l3section ), table);
                                int page = getSectionPage( l3section, pages );
                                if ( !title.empty() && page>=0 ) {
                                    wol.addTocItem( l1n, l2n, ++l3n, page, title );
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Render();
    return true;
}

void LVDocView::SetPos( int pos, bool savePos )
{
    if (m_view_mode==DVM_SCROLL)
    {
        if (pos > GetFullHeight() - m_dy )
            pos = GetFullHeight() - m_dy;
        if (pos<0)
            pos = 0;
        m_pos = pos;
    }
    else
    {
        int page = m_pages.FindNearestPage( pos, 0 );
        if (page<m_pages.length())
            m_pos = m_pages[page]->start;
        else
            m_pos = 0;
    }
    if ( savePos )
        _posBookmark = getBookmark();
    updateScroll();
    Draw();
}

int LVDocView::GetFullHeight()
{ 
    lvdomElementFormatRec * rd = m_doc ? m_doc->getMainNode()->getRenderData() : NULL;
    return ( rd ? rd->getHeight()+rd->getY() : m_dy ); 
}

void LVDocView::drawPageTo(LVDrawBuf * drawbuf, LVRendPageInfo & page)
{
    int start = page.start;
    int height = page.height;
    int offset = (drawbuf->GetHeight() - m_pageMargins.top - m_pageMargins.bottom - height) / 3;
    if (offset>16)
        offset = 16;
    if (offset<0)
        offset = 0;
    offset = 0;
    lvRect clip;
    clip.left = m_pageMargins.left;
    clip.top = offset + m_pageMargins.top;
    clip.bottom = m_pageMargins.top + height + offset;
    clip.right = drawbuf->GetWidth() - m_pageMargins.right;
    if ( page.type==PAGE_TYPE_COVER )
        clip.top = m_pageMargins.bottom;
    if ( m_pageHeaderInfo && page.type!=PAGE_TYPE_COVER) {
        lvRect info( 4, 4, drawbuf->GetWidth()-4, clip.top-7 );
        lUInt32 cl1 = 0xA0A0A0;
        lUInt32 cl2 = getBackgroundColor();
        lUInt32 pal[4];
        if ( drawbuf->GetBitsPerPixel()<=2 ) {
            // gray
            cl1 = 1;
            pal[0] = 3;
        } else {
            // color
            pal[0] = cl1;
        }
        drawbuf->SetTextColor(cl1);
        static lUInt8 pattern[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
        drawbuf->FillRectPattern(4, info.bottom, drawbuf->GetWidth()-4, info.bottom+1, cl1, cl2, pattern );
        info.bottom -= 1;
        lString16 pageinfo;
        if ( m_pageHeaderInfo & PGHDR_PAGE_NUMBER )
            pageinfo += lString16::itoa( getCurPage()+1 );
        if ( m_pageHeaderInfo & PGHDR_PAGE_COUNT )
            pageinfo += L" / " + lString16::itoa( getPageCount() );
        int piw = 0;
        int iy = info.bottom - m_infoFont->getHeight();
        if ( !pageinfo.empty() ) {
            piw = m_infoFont->getTextWidth( pageinfo.c_str(), pageinfo.length() );
            m_infoFont->DrawTextString( drawbuf, info.right-piw, iy, 
                pageinfo.c_str(), pageinfo.length(), L' ', pal, false);
        }
        int titlew = 0;
        lString16 title;
        if ( m_pageHeaderInfo & PGHDR_TITLE ) {
            title = getTitle();
            if ( !title.empty() )
                 titlew = m_infoFont->getTextWidth( title.c_str(), title.length() );
        }
        int authorsw = 0;
        lString16 authors;
        if ( m_pageHeaderInfo & PGHDR_AUTHOR ) {
            authors = getAuthors();
            if ( !authors.empty() ) {
                authors += L'.';
                authorsw = m_infoFont->getTextWidth( authors.c_str(), authors.length() );
            }
        }
        int w = info.width() - piw - 10;
        lString16 text;
        if ( authorsw + titlew + 10 > w ) {
            if ( (page.index & 1) )
                text = title;
            else
                text = authors;
        } else {
            text = authors + L"  " + title;
        }
        lvRect oldcr;
        drawbuf->GetClipRect( &oldcr );
        lvRect newcr = oldcr;
        newcr.right = info.right - piw - 10;
        drawbuf->SetClipRect(&newcr);
        m_infoFont->DrawTextString( drawbuf, info.left, iy, 
            text.c_str(), text.length(), L' ', pal, false);
        drawbuf->SetClipRect(&oldcr);
        //==============
        drawbuf->SetTextColor(getTextColor());
    }
    drawbuf->SetClipRect(&clip);
    if ( m_doc ) {
        if ( page.type == PAGE_TYPE_COVER ) {
            lvRect rc = clip;
            rc.top = m_pageMargins.bottom;
            drawCoverTo( drawbuf, rc );
        } else {
            DrawDocument( *drawbuf, m_doc->getMainNode(), m_pageMargins.left, m_pageMargins.top + offset, drawbuf->GetWidth() - m_pageMargins.left - m_pageMargins.right, height, 0, -start+offset, m_dy );
        }
    }
    drawbuf->SetClipRect(NULL);
#if 0
    lString16 pagenum = lString16::itoa( page.index+1 );
    m_font->DrawTextString(drawbuf, 5, 0 , pagenum.c_str(), pagenum.length(), '?', NULL, false); //drawbuf->GetHeight()-m_font->getHeight()
#endif
}

int LVDocView::getCurPage()
{
    return m_pages.FindNearestPage(m_pos, 0);
}

void LVDocView::Draw()
{
    m_drawbuf.SetBackgroundColor( m_backgroundColor );
    m_drawbuf.SetTextColor( m_textColor );
    m_drawbuf.Clear(m_backgroundColor);

    if ( !m_is_rendered )
        return;
    if ( !m_doc )
        return;
    if (m_font.isNull())
        return;
    if (m_view_mode==DVM_SCROLL)
    {
        m_drawbuf.SetClipRect(NULL);
        int cover_height = 0;
        if ( m_pages.length()>0 && m_pages[0]->type == PAGE_TYPE_COVER )
            cover_height = m_pages[0]->height;
        if ( m_pos < cover_height ) {
            lvRect rc;
            m_drawbuf.GetClipRect( &rc );
            rc.top -= m_pos;
            rc.bottom -= m_pos;
            rc.top += m_pageMargins.bottom;
            rc.bottom -= m_pageMargins.bottom;
            rc.left += m_pageMargins.left;
            rc.right -= m_pageMargins.right;
            drawCoverTo( &m_drawbuf, rc );
        }
        DrawDocument( m_drawbuf, m_doc->getMainNode(), m_pageMargins.left, 0, m_dx - m_pageMargins.left - m_pageMargins.right, m_dy, 0, -m_pos, m_dy );
    }
    else
    {
        int page = m_pages.FindNearestPage(m_pos, 0);
		if ( page>=0 && page<m_pages.length() )
			drawPageTo( &m_drawbuf, *m_pages[page] );
    }
}

/// returns xpointer for specified window point
ldomXPointer LVDocView::getNodeByPoint( lvPoint pt )
{
    int page = m_pages.FindNearestPage(m_pos, 0);
    if ( page>=0 && page<m_pages.length() ) {
        int page_y = m_pages[page]->start;
        return m_doc->createXPointer( lvPoint( pt.x, pt.y + page_y ) );
    }
    return ldomXPointer();
}

void LVDocView::Render( int dx, int dy, LVRendPageList * pages )
{
    if ( !m_doc )
		return;
	if ( pages==NULL )
		pages = &m_pages;
	if ( dx==0 )
		dx = m_drawbuf.GetWidth() - m_pageMargins.left - m_pageMargins.right;
	if ( dy==0 )
		dy = m_drawbuf.GetHeight() - m_pageMargins.top - m_pageMargins.bottom;
    lString8 fontName = lString8(DEFAULT_FONT_NAME);
    m_font = fontMan->GetFont( m_font_size, 300, false, DEFAULT_FONT_FAMILY, fontName );
    m_infoFont = fontMan->GetFont( INFO_FONT_SIZE, 300, false, DEFAULT_FONT_FAMILY, fontName );
    if ( !m_font )
        return;

    pages->clear();
    if ( m_showCover )
        pages->add( new LVRendPageInfo( dy ) );
    LVRendPageContext context( pages, dy );
    m_doc->render( context, dx, m_showCover ? dy + m_pageMargins.bottom*4 : 0, m_font );

#if 0        
    FILE * f = fopen("pagelist.log", "wt");
    if (f) {
        for (int i=0; i<m_pages.length(); i++)
        {
            fprintf(f, "%4d:   %7d .. %-7d [%d]\n", i, m_pages[i].start, m_pages[i].start+m_pages[i].height, m_pages[i].height);
        }
        fclose(f);
    }
#endif
    fontMan->gc();
    m_is_rendered = true;
}

/// set view mode (pages/scroll)
void LVDocView::setViewMode( LVDocViewMode view_mode )
{
    if ( m_view_mode==view_mode )
        return;
    m_view_mode = view_mode;
    Render();
    goToBookmark(_posBookmark);
}

/// get view mode (pages/scroll)
LVDocViewMode LVDocView::getViewMode()
{
    return m_view_mode;
}

void LVDocView::ZoomFont( int delta )
{
    if ( m_font.isNull() )
        return;
    LVFontRef nfnt;
    int sz = m_font->getHeight();
    for (int i=0; i<15; i++)
    {
        sz += delta;
        nfnt = fontMan->GetFont( sz, 400, false, DEFAULT_FONT_FAMILY, lString8(DEFAULT_FONT_NAME) );
        if ( !nfnt.isNull() && nfnt->getHeight() != m_font->getHeight() )
        {
            // found!
            //ldomXPointer bm = getBookmark();
            m_font_size = nfnt->getHeight();
            Render();
            goToBookmark(_posBookmark);
            return;
        }
        if (sz<12)
            break;
    }
}

void LVDocView::Resize( int dx, int dy )
{
    //LVCHECKPOINT("Resize");
    if (dx<80 || dx>3000)
        dx = 80;
    if (dy<80 || dy>3000)
        dy = 80;
    m_drawbuf.Resize(dx, dy);
    if (m_doc)
    {
        //ldomXPointer bm = getBookmark();
        if (dx!=m_dx || m_view_mode!=DVM_SCROLL)
        {
            m_dx = dx;
            m_dy = dy;
            Render();
        }
        goToBookmark(_posBookmark);
    }
    m_dx = dx;
    m_dy = dy;
}

#define XS_IMPLEMENT_SCHEME 1
#include "../include/fb2def.h"

#if 0
void SaveBase64Objects( ldomNode * node )
{
    if ( !node->isElement() || node->getNodeId()!=el_binary )
        return;
    lString16 name = node->getAttributeValue(attr_id);
    if ( name.empty() )
        return;
    fprintf( stderr, "opening base64 stream...\n" );
    LVStreamRef in = ((ldomElement*)node)->createBase64Stream();
    if ( in.isNull() )
        return;
    fprintf( stderr, "base64 stream opened: %d bytes\n", (int)in->GetSize() );
    fprintf( stderr, "opening out stream...\n" );
    LVStreamRef outstream = LVOpenFileStream( name.c_str(), LVOM_WRITE );
    if (outstream.isNull())
        return;
    //outstream->Write( "test", 4, NULL );
    fprintf( stderr, "streams opened, copying...\n" );
/*    
    lUInt8 dbuf[128000];
    lvsize_t bytesRead = 0;
    if ( in->Read( dbuf, 128000, &bytesRead )==LVERR_OK )
    {
        fprintf(stderr, "Read %d bytes, writing...\n", (int) bytesRead );
        //outstream->Write( "test2", 5, NULL );
        //outstream->Write( "test3", 5, NULL );
        outstream->Write( dbuf, 100, NULL );
        outstream->Write( dbuf, bytesRead, NULL );
        //outstream->Write( "test4", 5, NULL );
    }
*/        
    LVPumpStream( outstream, in );
    fprintf(stderr, "...\n");
}
#endif

/// load document from file
bool LVDocView::LoadDocument( const lChar16 * fname )
{
    Clear();
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return false;
    return LoadDocument( stream );
}

/// load document from stream
bool LVDocView::LoadDocument( LVStreamRef stream )
{
    m_stream = stream;

#if (USE_ZLIB==1)

    m_arc = LVOpenArchieve( m_stream );
    if (!m_arc.isNull())
    {
        // archieve
        for (int i=0; i<m_arc->GetObjectCount(); i++)
        {
            const LVContainerItemInfo * item = m_arc->GetObjectInfo(i);
            if (item)
            {
                if ( !item->IsContainer() )
                {
                    lString16 name( item->GetName() );
                    bool nameIsOk = false;
                    if ( name.length() > 5 )
                    {
                        const lChar16 * pext = name.c_str() + name.length() - 4;
                        if ( pext[0]=='.' && pext[1]=='f' && pext[2]=='b' && pext[3]=='2')
                            nameIsOk = true;
                    }
                    if ( !nameIsOk )
                    {
                        Clear();
                        return false;
                    }
                    m_stream = m_arc->OpenStream( item->GetName(), LVOM_READ );
                    if ( m_stream.isNull() )
                        return false;
                }
            }
        }
        // opened archieve stream
    }
    else

#endif //USE_ZLIB

    {   
#if 1
        m_stream = LVCreateBufferedStream( m_stream, FILE_STREAM_BUFFER_SIZE );
#else
        LVStreamRef stream = LVCreateBufferedStream( m_stream, FILE_STREAM_BUFFER_SIZE );
        lvsize_t sz = stream->GetSize();
        const lvsize_t bufsz = 0x1000;
        lUInt8 buf[bufsz];
        lUInt8 * fullbuf = new lUInt8 [sz];
        stream->SetPos(0);
        stream->Read(fullbuf, sz, NULL);
        lvpos_t maxpos = sz - bufsz;
        for (int i=0; i<1000; i++)
        {
            lvpos_t pos = (lvpos_t)((((lUInt64)i) * 1873456178) % maxpos);
            stream->SetPos( pos );
            lvsize_t readsz = 0;
            stream->Read( buf, bufsz, &readsz );
            if (readsz != bufsz)
            {
                //
                fprintf(stderr, "data read error!\n");
            }
            for (int j=0; j<bufsz; j++)
            {
                if (fullbuf[pos+j] != buf[j])
                {
                    fprintf(stderr, "invalid data!\n");
                }
            }
        }
        delete fullbuf;
#endif
    }

    if ( m_doc )
        delete m_doc;
    m_is_rendered = false;
#if COMPACT_DOM==1
    m_doc = new ldomDocument( m_stream );
#else
    m_doc = new ldomDocument();
#endif
    ldomDocumentWriter writer(m_doc);
    m_doc->setNodeTypes( fb2_elem_table );
    m_doc->setAttributeTypes( fb2_attr_table );
    m_doc->setNameSpaceTypes( fb2_ns_table );
    LVXMLParser parser(m_stream.get(), &writer);

    // set stylesheet
    m_doc->getStyleSheet()->clear();
    m_doc->getStyleSheet()->parse(m_stylesheet.c_str());
    
    // parse
    parser.Parse();
    m_pos = 0;

#if 0
    {
        LVStreamRef ostream = LVOpenFileStream( "test_save.fb2", LVOM_WRITE );
        m_doc->saveToStream( ostream, "utf-16" );
        m_doc->getRootNode()->recurseElements( SaveBase64Objects );
    }
#endif


    lString16 authors;
    for ( int i=0; i<16; i++) {
        lString16 path = lString16(L"/FictionBook/description/title-info/author[") + lString16::itoa(i) + L"]";
        ldomXPointer pauthor = m_doc->createXPointer(path);
        if ( !pauthor )
            break;
        lString16 firstName = pauthor.relative( L"/first-name" ).getText();
        lString16 lastName = pauthor.relative( L"/last-name" ).getText();
        lString16 middleName = pauthor.relative( L"/middle-name" ).getText();
        lString16 author = firstName;
        if ( !author.empty() )
            author += L" ";
        if ( !middleName.empty() )
            author += lString16(middleName, 0, 1) + L". ";
        author += lastName;
        if ( !authors.empty() )
            authors += L", ";
        authors += author;
    }
    m_authors = authors;
    m_title = m_doc->createXPointer(L"/FictionBook/description/title-info/book-title").getText();
    return true;
}

bool LVDocView::LoadDocument( const char * fname )
{
    Clear();
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return false;
    return LoadDocument( stream );
}

/// returns bookmark
ldomXPointer LVDocView::getBookmark()
{
    ldomXPointer ptr = m_doc->createXPointer( lvPoint( 0, m_pos ) );
    return ptr;
/*
    lUInt64 pos = m_pos;
    if (m_view_mode==DVM_PAGES)
        m_pos += m_dy/3;
    lUInt64 h = GetFullHeight();
    if (h<1)
        h = 1;
    return pos*1000000/h;
*/
}

/// moves position to bookmark
void LVDocView::goToBookmark(ldomXPointer bm)
{
    lvPoint pt = bm.toPoint();
    SetPos( pt.y, false );
    /*
    lUInt64 h = GetFullHeight();
    int pos = (int) (bm*h/1000000);
    SetPos( pos );
    */
}

void LVDocView::updateScroll()
{
    if (m_view_mode==DVM_SCROLL)
    {
        int npos = m_pos;
        int fh = GetFullHeight();
        int shift = 1;
        int npage = m_dy;
        while (fh > 16384)
        {
            fh >>= 1;
            npos >>= 1;
            npage >>= 1;
            shift++;
        }
        if (npage<1)
            npage = 1;
        m_scrollinfo.pos = npos;
        m_scrollinfo.maxpos = fh - npage;
        m_scrollinfo.pagesize = npage;
        m_scrollinfo.scale = shift;
        char str[32];
        sprintf(str, "%d%%", (int)(fh>0?(100*npos/fh):0));
        m_scrollinfo.posText = lString16( str );
    }
    else
    {
        int page = m_pages.FindNearestPage( m_pos, 0 );
        m_scrollinfo.pos = page;
        m_scrollinfo.maxpos = m_pages.length()-1;
        m_scrollinfo.pagesize = 1;
        m_scrollinfo.scale = 0;
        char str[32];
        sprintf(str, "%d / %d", page+1, m_pages.length() );
        m_scrollinfo.posText = lString16( str );
    }
}

/// converts scrollbar pos to doc pos
int LVDocView::scrollPosToDocPos( int scrollpos )
{
    if (m_view_mode==DVM_SCROLL)
    {
        int n = scrollpos<<m_scrollinfo.scale;
        if (n<0)
            n = 0;
        int fh = GetFullHeight();
        if (n>fh)
            n = fh;
        return n;        
    }
    else
    {
        int n = scrollpos;
        if (!m_pages.length())
            return 0;
        if (n>=m_pages.length())
            n = m_pages.length()-1;
        if (n<0)
            n = 0;
        return m_pages[n]->start;
    }
}

void LVDocView::goToPage( int page )
{
    if (!m_pages.length())
        return;
    if (page >= m_pages.length())
        page = m_pages.length()-1;
    if (page<0)
        page = 0;
    SetPos( m_pages[page]->start );
}

// execute command
void LVDocView::doCommand( LVDocCmd cmd, int param )
{
    switch (cmd)
    {
    case DCMD_BEGIN:
        {
            SetPos(0);
        }
        break;
    case DCMD_LINEUP:
        {
            if (m_view_mode==DVM_SCROLL)
            {
                SetPos( GetPos() - param*(m_font_size*3/2));
            }
            else
            {
                goToPage( m_pages.FindNearestPage(m_pos, -1));
            }
        }
        break;
    case DCMD_PAGEUP:
        {
            if (m_view_mode==DVM_SCROLL)
            {
                SetPos( GetPos() - param*m_dy );
            }
            else
            {
                goToPage( m_pages.FindNearestPage(m_pos, -1));
            }
        }
        break;
    case DCMD_PAGEDOWN:
        {
            if (m_view_mode==DVM_SCROLL)
            {
                SetPos( GetPos() + param*m_dy );
            }
            else
            {
                goToPage( m_pages.FindNearestPage(m_pos, +1));
            }
        }
        break;
    case DCMD_LINEDOWN:
        {
            if (m_view_mode==DVM_SCROLL)
            {
                SetPos( GetPos() + param*(m_font_size*3/2));
            }
            else
            {
                goToPage( m_pages.FindNearestPage(m_pos, +1));
            }
        }
        break;
    case DCMD_END:
        {
            SetPos(GetFullHeight());
        }
        break;
    case DCMD_GO_POS:
        {
            if (m_view_mode==DVM_SCROLL)
            {
                SetPos( param );
            }
            else
            {
                goToPage( m_pages.FindNearestPage(param, 0) );
            }
        }
        break;
    case DCMD_GO_PAGE:
        {
            goToPage( param );
        }
        break;
    case DCMD_ZOOM_IN:
        {
            ZoomFont( +1 );
        }
        break;
    case DCMD_ZOOM_OUT:
        {
            ZoomFont( -1 );
        }
        break;
    }
}


