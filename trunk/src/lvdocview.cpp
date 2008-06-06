/*******************************************************

   CoolReader Engine

   lvdocview.cpp:  XML DOM tree rendering tools

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/


#include "../include/lvdocview.h"
#include "../include/rtfimp.h"

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

#ifdef LBOOK
#define INFO_FONT_SIZE      20
#else
#define INFO_FONT_SIZE      20
#endif

#if defined(__SYMBIAN32__)
#include <e32std.h>
#define DEFAULT_PAGE_MARGIN 2
#else
#ifdef LBOOK
#define DEFAULT_PAGE_MARGIN      8
#else
#define DEFAULT_PAGE_MARGIN      18
#endif
#endif

/// minimum EM width of page (prevents show two pages for windows that not enougn wide)
#define MIN_EM_PER_PAGE     20

static int def_font_sizes[] = { 16, 18, 22, 26, 30, 36, 42 };

LVDocView::LVDocView()
: m_dx(100), m_dy(100), m_pos(50), m_battery_state(66)
#if (LBOOK==1)
, m_font_size(32)
#elif defined(__SYMBIAN32__)
, m_font_size(30)
#else
, m_font_size(26)
#endif
, m_def_interline_space(130)
, m_font_sizes( def_font_sizes, sizeof(def_font_sizes) / sizeof(int) )
, m_font_sizes_cyclic(false)
, m_is_rendered(false)
, m_view_mode( 1 ? DVM_PAGES : DVM_SCROLL ) // choose 0/1
/*
, m_drawbuf(100, 100
#if COLOR_BACKBUFFER==0
            , GRAY_BACKBUFFER_BITS
#endif
            )
*/
, m_stream(NULL), m_doc(NULL)
, m_stylesheet( def_stylesheet )
, m_pageMargins(DEFAULT_PAGE_MARGIN, DEFAULT_PAGE_MARGIN + INFO_FONT_SIZE + 4, DEFAULT_PAGE_MARGIN, DEFAULT_PAGE_MARGIN)
, m_pagesVisible(2)
, m_pageHeaderInfo (
      PGHDR_PAGE_NUMBER
#ifndef LBOOK
    | PGHDR_CLOCK
#endif
    | PGHDR_BATTERY
    | PGHDR_PAGE_COUNT
    | PGHDR_AUTHOR
    | PGHDR_TITLE)
, m_showCover(true)
, m_rotateAngle(CR_ROTATE_ANGLE_0)
, m_section_bounds_valid(false)
, m_posIsSet(false)
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
    m_defaultFontFace = lString8(DEFAULT_FONT_NAME);

    //m_drawbuf.Clear(m_backgroundColor);
    createDefaultDocument( lString16(L"No document"), lString16(L"Welcome to CoolReader! Please select file to open") );
}

LVDocView::~LVDocView()
{
    Clear();
}

/// returns true if document is opened
bool LVDocView::isDocumentOpened()
{
    return m_doc && m_doc->getRootNode();
}

/// rotate rectangle by current angle, winToDoc==false for doc->window translation, true==ccw
lvRect LVDocView::rotateRect( lvRect & rc, bool winToDoc )
{
    lvRect rc2;
    cr_rotate_angle_t angle = m_rotateAngle;
    if ( winToDoc )
        angle = (cr_rotate_angle_t)((4 - (int)angle) & 3);
    switch ( angle ) {
    case CR_ROTATE_ANGLE_0:
        rc2 = rc;
        break;
    case CR_ROTATE_ANGLE_90:
        /*
          . . . . . .      . . . . . . . .
          . . . . . .      . . . . . 1 . .
          . 1 . . . .      . . . . . . . .
          . . . . . .  ==> . . . . . . . .
          . . . . . .      . 2 . . . . . .
          . . . . . .      . . . . . . . .
          . . . . 2 .
          . . . . . .

        */
        rc2.left = m_dy - rc.bottom - 1;
        rc2.right = m_dy - rc.top - 1;
        rc2.top = rc.left;
        rc2.bottom = rc.right;
        break;
    case CR_ROTATE_ANGLE_180:
        rc2.left = m_dx - rc.left - 1;
        rc2.right = m_dx - rc.right - 1;
        rc2.top = m_dy - rc.top - 1;
        rc2.bottom = m_dy - rc.bottom - 1;
        break;
    case CR_ROTATE_ANGLE_270:
        /*
          . . . . . .      . . . . . . . .
          . . . . . .      . 1 . . . . . .
          . . . . 2 .      . . . . . . . .
          . . . . . .  <== . . . . . . . .
          . . . . . .      . . . . . 2 . .
          . . . . . .      . . . . . . . .
          . 1 . . . .
          . . . . . .

        */
        rc2.left = rc.top;
        rc2.right = rc.bottom;
        rc2.top = m_dx - rc.right - 1;
        rc2.bottom = m_dx - rc.left - 1;
        break;
    }
    return rc2;
}

/// rotate point by current angle, winToDoc==false for doc->window translation, true==ccw
lvPoint LVDocView::rotatePoint( lvPoint & pt, bool winToDoc )
{
    lvPoint pt2;
    cr_rotate_angle_t angle = m_rotateAngle;
    if ( winToDoc )
        angle = (cr_rotate_angle_t)((4 - (int)angle) & 3);
    switch ( angle ) {
    case CR_ROTATE_ANGLE_0:
        pt2 = pt;
        break;
    case CR_ROTATE_ANGLE_90:
        /*
          . . . . . .      . . . . . . . .
          . . . . . .      . . . . . 1 . .
          . 1 . . . .      . . . . . . . .
          . . . . . .  ==> . . . . . . . .
          . . . . . .      . 2 . . . . . .
          . . . . . .      . . . . . . . .
          . . . . 2 .
          . . . . . .

        */
        pt2.y = pt.x;
        pt2.x = m_dx - pt.y - 1;
        break;
    case CR_ROTATE_ANGLE_180:
        pt2.y = m_dy - pt.y - 1;
        pt2.x = m_dx - pt.x - 1;
        break;
    case CR_ROTATE_ANGLE_270:
        /*
          . . . . . .      . . . . . . . .
          . . . . . .      . 1 . . . . . .
          . . . . 2 .      . . . . . . . .
          . . . . . .  <== . . . . . . . .
          . . . . . .      . . . . . 2 . .
          . . . . . .      . . . . . . . .
          . 1 . . . .
          . . . . . .

        */
        pt2.y = m_dy - pt.x - 1;
        pt2.x = pt.y;
        break;
    }
    return pt2;
}

void LVDocView::setPageHeaderInfo( int hdrFlags )
{
    LVLock lock(getMutex());
    m_pageHeaderInfo = hdrFlags;
    int oldMargin = m_pageMargins.top;
    m_pageMargins.top = m_pageMargins.bottom + (hdrFlags ? INFO_FONT_SIZE : 0);
    if ( m_pageMargins.top != oldMargin ) {
        requestRender();
    } else {
        clearImageCache();
    }
}

/// set document stylesheet text
void LVDocView::setStyleSheet( lString8 css_text )
{
    LVLock lock(getMutex());
    requestRender();
    m_stylesheet = css_text;
}

void LVDocView::Clear()
{
    {
        LVLock lock(getMutex());
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
        m_filename.clear();
    }
    m_imageCache.clear();
}

/// invalidate image cache, request redraw
void LVDocView::clearImageCache()
{
    m_imageCache.clear();
}

/// invalidate formatted data, request render
void LVDocView::requestRender()
{
    m_is_rendered = false;
    m_imageCache.clear();
}

/// render document, if not rendered
void LVDocView::checkRender()
{
    if ( !m_is_rendered ) {
        LVLock lock(getMutex());
        Render();
        m_imageCache.clear();
        m_posIsSet = false;
    }
}

/// ensure current position is set to current bookmark value
void LVDocView::checkPos()
{
    checkRender();
    if ( m_posIsSet )
        return;
    LVLock lock(getMutex());
    if ( _posBookmark.isNull() ) {
        SetPos( 0, false );
    } else {
        lvPoint pt = _posBookmark.toPoint();
        SetPos( pt.y, false );
    }
    m_posIsSet = true;
}

/// get page image
LVDocImageRef LVDocView::getPageImage( int delta )
{
    checkPos();
    // find existing object in cache
    int offset = m_pos;
    if ( delta<0 )
        offset = getPrevPageOffset();
    else if ( delta>0 )
        offset = getNextPageOffset();
    CRLog::trace("getPageImage: checking cache for page [%d] (delta=%d)", offset, delta);
    LVDocImageRef ref = m_imageCache.get( offset );
    if ( !ref.isNull() ) {
        CRLog::trace("getPageImage: + page [%d] found in cache", offset);
        return ref;
    }
    while ( ref.isNull() ) {
        CRLog::trace("getPageImage: - page [%d] not found, force rendering", offset);
        cachePageImage( delta );
        ref = m_imageCache.get( offset );
    }
    CRLog::trace("getPageImage: page [%d] is ready", offset);
    return ref;
}

class LVDrawThread : public LVThread {
    LVDocView * _view;
    int _offset;
    LVRef<LVDrawBuf> _drawbuf;
public:
    LVDrawThread( LVDocView * view, int offset, LVRef<LVDrawBuf> drawbuf )
    : _view(view), _offset(offset), _drawbuf(drawbuf)
    {
        start();
    }
    virtual void run()
    {
        _view->Draw( *_drawbuf, _offset, true );
        //_drawbuf->Rotate( _view->GetRotateAngle() );
    }
};
/// cache page image (render in background if necessary)
void LVDocView::cachePageImage( int delta )
{
    int offset = m_pos;
    if ( delta<0 )
        offset = getPrevPageOffset();
    else if ( delta>0 )
        offset = getNextPageOffset();
    CRLog::trace("cachePageImage: request to cache page [%d] (delta=%d)", offset, delta);
    if ( m_imageCache.has(offset) ) {
        CRLog::trace("cachePageImage: Page [%d] is found in cache", offset);
        return;
    }
    CRLog::trace("cachePageImage: starting new render task for page [%d]", offset);
#if (COLOR_BACKBUFFER==1)
    LVRef<LVDrawBuf> drawbuf( new LVColorDrawBuf( m_dx, m_dy ) );
#else
    LVRef<LVDrawBuf> drawbuf( new LVGrayDrawBuf( m_dx, m_dy, GRAY_BACKBUFFER_BITS ) );
#endif
    LVRef<LVThread> thread( new LVDrawThread( this, offset, drawbuf ) );
    m_imageCache.set( offset, drawbuf, thread );
    CRLog::trace("cachePageImage: caching page [%d] is finished", offset);
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
    header = child->getText(L' ');
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
        page = pages.FindNearestPage( y, -1 );
        //dumpSection( section );
        //fprintf(log.f, "page %d: %d->%d..%d\n", page+1, y, pages[page].start, pages[page].start+pages[page].height );
    }
    return page;
}

static void addTocItems( ldomElement * basesection, LVTocItem * parent )
{
    if ( !basesection || !parent )
        return;
    lString16 name = getSectionHeader( basesection );
    if ( name.empty() )
        return; // section without header
    ldomXPointer ptr( basesection, 0 );
    LVTocItem * item = parent->addChild( name, ptr );
    for ( int i=0; ;i++ ) {
        ldomElement * section = basesection->findChildElement( LXML_NS_ANY, el_section, i );
        if ( !section )
            break;
        addTocItems( section, item );
    }
}

/// returns Y position
int LVTocItem::getY()
{
    return _position.toPoint().y;
}

/// returns page number
int LVTocItem::getPageNum( LVRendPageList & pages )
{
    return getSectionPage( (ldomElement*)_position.getNode(), pages );
}

void LVDocView::makeToc()
{
    m_toc.clear();
    ldomElement * body = ((ldomElement*)m_doc->getRootNode())
        ->findChildElement( LXML_NS_ANY, el_FictionBook, -1 )
        ->findChildElement( LXML_NS_ANY, el_body, 0 );
    if ( !body )
        return;
    for ( int i=0; ;i++ ) {
        ldomElement * section = body->findChildElement( LXML_NS_ANY, el_section, i );
        if ( !section )
            break;
        addTocItems( section, &m_toc );
    }
}

/// returns cover page image source, if any
LVImageSourceRef LVDocView::getCoverPageImage()
{
    ldomElement * cover_img_el = ((ldomElement*)m_doc->getRootNode())
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
    if ( rc.width()<130 || rc.height()<130)
        return;
    int base_font_size = 24;
    int w = rc.width();
    if ( w<300 )
        base_font_size = 22;
    else if ( w<500 )
        base_font_size = 24;
    else if ( w<700 )
        base_font_size = 26;
    else
        base_font_size = 32;
    LVFontRef author_fnt( fontMan->GetFont( base_font_size, 600, true, css_ff_serif, lString8("Times New Roman")) );
    LVFontRef title_fnt( fontMan->GetFont( base_font_size+4, 600, false, css_ff_serif, lString8("Times New Roman")) );
    LVFontRef series_fnt( fontMan->GetFont( base_font_size-3, 300, true, css_ff_serif, lString8("Times New Roman")) );
    lString16 authors = getAuthors();
    lString16 title = getTitle();
    lString16 series = getSeries();
    if ( title.empty() )
        title = L"no title";
    LFormattedText txform;
    if ( !authors.empty() )
        txform.AddSourceLine( authors.c_str(), authors.length(), 0xFFFFFFFF, 0xFFFFFFFF, author_fnt.get(), LTEXT_ALIGN_CENTER, 20 );
    txform.AddSourceLine( title.c_str(), title.length(), 0xFFFFFFFF, 0xFFFFFFFF, title_fnt.get(), LTEXT_ALIGN_CENTER, 20 );
    txform.AddSourceLine( series.c_str(), series.length(), 0xFFFFFFFF, 0xFFFFFFFF, series_fnt.get(), LTEXT_ALIGN_CENTER, 20 );
    int title_w = rc.width() - rc.width()/3;
    int h = txform.Format( title_w, rc.height() ) + 16;

    lvRect imgrc = rc;
    imgrc.bottom -= h + 16;

    LVImageSourceRef imgsrc = getCoverPageImage();
    if ( !imgsrc.isNull() && imgrc.height()>30 )
    {
        //fprintf( stderr, "Writing coverpage image...\n" );
        int src_dx = imgsrc->GetWidth();
        int src_dy = imgsrc->GetHeight();
        int scale_x = imgrc.width() * 0x10000 / src_dx;
        int scale_y = imgrc.height() * 0x10000 / src_dy;
        if ( scale_x < scale_y )
            scale_y = scale_x;
        else
            scale_x = scale_y;
        int dst_dx = (src_dx * scale_x) >> 16;
        int dst_dy = (src_dy * scale_y) >> 16;
        if (dst_dx>rc.width())
            dst_dx = imgrc.width();
        if (dst_dy>rc.height())
            dst_dy = imgrc.height();
        drawBuf->Draw( imgsrc, imgrc.left + (imgrc.width()-dst_dx)/2, imgrc.top + (imgrc.height()-dst_dy)/2, dst_dx, dst_dy );
        //fprintf( stderr, "Done.\n" );
    }
    else
    {
        imgrc.bottom = imgrc.top;
    }
    rc.top = imgrc.bottom;
    txform.Draw( drawBuf, (rc.right + rc.left - title_w) / 2, (rc.bottom + rc.top - h) / 2, NULL );
}

/// export to WOL format
bool LVDocView::exportWolFile( LVStream * stream, bool flgGray, int levels )
{
    checkRender();
    int old_flags = m_pageHeaderInfo;
    m_pageHeaderInfo &= ~(PGHDR_CLOCK | PGHDR_BATTERY);
    LVRendPageList pages;
    int dx = 600 - m_pageMargins.left - m_pageMargins.right;
    int dy = 800 - m_pageMargins.top - m_pageMargins.bottom;
    Render(dx, dy, &pages);

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

        for (int i=1; i<pages.length(); i++)
        {
			LVGrayDrawBuf drawbuf(600, 800, flgGray ? 2 : 1); //flgGray ? 2 : 1);
			drawPageTo( &drawbuf, *pages[i], NULL, pages.length(), 0 );
            if (!flgGray)
                drawbuf.ConvertToBitmap(false);
            else
                drawbuf.Invert();
            wol.addImage(drawbuf);
        }

        // add TOC
        ldomElement * body = (ldomElement *)m_doc->nodeFromXPath( lString16(L"/FictionBook/body[1]") );
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
    m_pageHeaderInfo = old_flags;
    Render();

    return true;
}

void LVDocView::SetPos( int pos, bool savePos )
{
    LVLock lock(getMutex());
    checkRender();
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
        int pc = getVisiblePageCount();
        int page = m_pages.FindNearestPage( pos, 0 );
        if ( pc==2 )
            page &= ~1;
        if (page<m_pages.length())
            m_pos = m_pages[page]->start;
        else
            m_pos = 0;
    }
    if ( savePos )
        _posBookmark = getBookmark();
    m_posIsSet = true;
    updateScroll();
    //Draw();
}

int LVDocView::GetFullHeight()
{
    LVLock lock(getMutex());
    checkRender();
    lvdomElementFormatRec * rd = m_doc ? m_doc->getMainNode()->getRenderData() : NULL;
    return ( rd ? rd->getHeight()+rd->getY() : m_dy );
}

/// calculate page header rectangle
void LVDocView::getPageHeaderRectangle( int pageIndex, lvRect & headerRc )
{
    lvRect pageRc;
    getPageRectangle( pageIndex, pageRc );
    headerRc = pageRc;
    if ( pageIndex==0 ) {
        headerRc.bottom = 0;
    } else {
        headerRc.bottom = headerRc.top + m_pageMargins.top;
        headerRc.top += 4;
        headerRc.left += 4;
        headerRc.right -= 4;
        headerRc.bottom -= 4;
    }
}

/// returns current time representation string
lString16 LVDocView::getTimeString()
{
    time_t t = (time_t)time(0);
    tm * bt = localtime(&t);
    char str[12];
    sprintf(str, "%02d:%02d", bt->tm_hour, bt->tm_min);
    return Utf8ToUnicode( lString8( str ) );
}

/// draw battery state to buffer
void LVDocView::drawBatteryState( LVDrawBuf * drawbuf, const lvRect & batteryRc, bool isVertical )
{
    if ( m_batteryIcons.length()>1 ) {
        int iconIndex = ((m_batteryIcons.length() - 1 ) * m_battery_state + (100/m_batteryIcons.length()/2) )/ 100;
        if ( iconIndex<0 )
            iconIndex = 0;
        if ( iconIndex>m_batteryIcons.length()-1 )
            iconIndex = m_batteryIcons.length()-1;
        LVImageSourceRef icon = m_batteryIcons[iconIndex];
        drawbuf->Draw( icon, (batteryRc.left + batteryRc.right - icon->GetWidth() ) / 2,
            (batteryRc.top + batteryRc.bottom - icon->GetHeight())/2,
            icon->GetWidth(),
            icon->GetHeight() );
    } else {
        lvRect rc =  batteryRc;
        if ( m_battery_state<0 )
            return;
        lUInt32 cl = 0xA0A0A0;
        lUInt32 cl2 = 0xD0D0D0;
        if ( drawbuf->GetBitsPerPixel()<=2 ) {
            cl = 1;
            cl2 = 2;
        }
    #if 1

        if ( isVertical ) {
            int h = rc.height();
            h = ( (h - 4) / 4 * 4 ) + 3;
            int dh = (rc.height() - h) / 2;
            rc.bottom -= dh;
            rc.top = rc.bottom - h;
            int w = rc.width();
            int h0 = 4; //h / 6;
            int w0 = w / 3;
            // contour
            drawbuf->FillRect( rc.left, rc.top+h0, rc.left+1, rc.bottom, cl );
            drawbuf->FillRect( rc.right-1, rc.top+h0, rc.right, rc.bottom, cl );

            drawbuf->FillRect( rc.left, rc.top+h0, rc.left+w0, rc.top+h0+1, cl );
            drawbuf->FillRect( rc.right-w0, rc.top+h0, rc.right, rc.top+h0+1, cl );

            drawbuf->FillRect( rc.left+w0-1, rc.top, rc.left+w0, rc.top+h0, cl );
            drawbuf->FillRect( rc.right-w0, rc.top, rc.right-w0+1, rc.top+h0, cl );

            drawbuf->FillRect( rc.left+w0, rc.top, rc.right-w0, rc.top+1, cl );
            drawbuf->FillRect( rc.left, rc.bottom-1, rc.right, rc.bottom, cl );
            // fill
            int miny = rc.bottom - 2 - (h - 4) * m_battery_state / 100;
            for ( int i=0; i<h-4 ; i++ ) {
                if ( (i&3) != 3 ) {
                    int y = rc.bottom - 2 - i;
                    int w = 2;
                    if ( y < rc.top + h0 + 2 )
                        w = w0 + 1;
                    lUInt32 c = cl2;
                    if ( y >= miny )
                        c = cl;
                    drawbuf->FillRect( rc.left+w, y-1, rc.right-w, y, c );
                }
            }
        } else {
            // horizontal
            int h = rc.width();
            h = ( (h - 4) / 4 * 4 ) + 3;
            int dh = (rc.height() - h) / 2;
            rc.right -= dh;
            rc.left = rc.right - h;
            h = rc.height();
            dh = h - (rc.width() * 4/8 + 1);
            if ( dh>0 ) {
                rc.bottom -= dh/2;
                rc.top += (dh/2);
                h = rc.height();
            }
            int w = rc.width();
            int h0 = h / 3; //h / 6;
            int w0 = 4;
            // contour
            drawbuf->FillRect( rc.left+w0, rc.top, rc.right, rc.top+1, cl );
            drawbuf->FillRect( rc.left+w0, rc.bottom-1, rc.right, rc.bottom, cl );

            drawbuf->FillRect( rc.left+w0, rc.top, rc.left+w0+1, rc.top+h0, cl );
            drawbuf->FillRect( rc.left+w0, rc.bottom-h0, rc.left+w0+1, rc.bottom, cl );

            drawbuf->FillRect( rc.left, rc.top+h0-1, rc.left+w0, rc.top+h0, cl );
            drawbuf->FillRect( rc.left, rc.bottom-h0, rc.left+w0, rc.bottom-h0+1, cl );

            drawbuf->FillRect( rc.left, rc.top+h0, rc.left+1, rc.bottom-h0, cl );
            drawbuf->FillRect( rc.right-1, rc.top, rc.right, rc.bottom, cl );
            // fill
            int minx = rc.right - 2 - (w - 4) * m_battery_state / 100;
            for ( int i=0; i<w-4 ; i++ ) {
                if ( (i&3) != 3 ) {
                    int x = rc.right - 2 - i;
                    int h = 2;
                    if ( x < rc.left + w0 + 2 )
                        h = h0 + 1;
                    lUInt32 c = cl2;
                    if ( x >= minx )
                        c = cl;
                    drawbuf->FillRect( x-1, rc.top+h, x, rc.bottom-h, c );
                }
            }
        }
    #else
        //lUInt32 cl = getTextColor();
        int h = rc.height() / 6;
        if ( h<5 )
            h = 5;
        int n = rc.height() / h;
        int dy = rc.height() % h / 2;
        if ( n<1 )
            n = 1;
        int k = m_battery_state * n / 100;
        for ( int i=0; i<n; i++ ) {
            lvRect rrc = rc;
            rrc.bottom -= h * i + dy;
            rrc.top = rrc.bottom - h + 1;
            int dx = (i<n-1) ? 0 : rc.width()/5;
            rrc.left += dx;
            rrc.right -= dx;
            if ( i<k ) {
                // full
                drawbuf->FillRect( rrc.left, rrc.top, rrc.right, rrc.bottom, cl );
            } else {
                // empty
                drawbuf->FillRect( rrc.left, rrc.top, rrc.right, rrc.top+1, cl );
                drawbuf->FillRect( rrc.left, rrc.bottom-1, rrc.right, rrc.bottom, cl );
                drawbuf->FillRect( rrc.left, rrc.top, rrc.left+1, rrc.bottom, cl );
                drawbuf->FillRect( rrc.right-1, rrc.top, rrc.right, rrc.bottom, cl );
            }
        }
    #endif
    }
}

/// returns section bounds, in 1/100 of percent
LVArray<int> & LVDocView::getSectionBounds( )
{
    if ( m_section_bounds_valid )
        return m_section_bounds;
    m_section_bounds.clear();
    m_section_bounds.add(0);
    ldomElement * body = (ldomElement *)m_doc->nodeFromXPath( lString16(L"/FictionBook/body[1]") );
    lUInt16 section_id = m_doc->getElementNameIndex( L"section" );
    int fh = GetFullHeight();
    if ( body && fh>0 ) {
        for ( int l1=0; l1<1000; l1++) {
            ldomElement * l1section = body->findChildElement(LXML_NS_ANY, section_id, l1);
            if ( !l1section )
                break;
            lvRect rc;
            l1section->getAbsRect( rc );
            int p = (int)(((lInt64)rc.top * 10000) / fh);
            m_section_bounds.add( p );
        }
    }
    m_section_bounds.add(10000);
    m_section_bounds_valid = true;
    return m_section_bounds;
}

int LVDocView::getPosPercent()
{
    LVLock lock(getMutex());
    checkPos();
    int fh = GetFullHeight();
    int p = GetPos();
    if ( fh>0 )
        return (int)(((lInt64)p * 10000) / fh);
    else
        return 0;
}

void LVDocView::getPageRectangle( int pageIndex, lvRect & pageRect )
{
    if ( (pageIndex & 1)==0 || (getVisiblePageCount()<2) )
        pageRect = m_pageRects[0];
    else
        pageRect = m_pageRects[1];
}

void LVDocView::getNavigationBarRectangle( lvRect & navRect )
{
    getNavigationBarRectangle( getVisiblePageCount()==2 ? 1 : 2, navRect );
}

void LVDocView::getNavigationBarRectangle( int pageIndex, lvRect & navRect )
{
    lvRect headerRect;
    getPageHeaderRectangle( pageIndex, headerRect );
    navRect = headerRect;
    if ( headerRect.bottom <= headerRect.top )
        return;
    navRect.top = navRect.bottom - 6;
}

void LVDocView::drawNavigationBar( LVDrawBuf * drawbuf, int pageIndex, int percent )
{
    //LVArray<int> & sbounds = getSectionBounds();
    lvRect navBar;
    getNavigationBarRectangle( pageIndex, navBar );
    //bool leftPage = (getVisiblePageCount()==2 && !(pageIndex&1) );

    //lUInt32 cl1 = 0xA0A0A0;
    //lUInt32 cl2 = getBackgroundColor();
}

/// set list of battery icons to display battery state
void LVDocView::setBatteryIcons( LVRefVec<LVImageSource> icons )
{
    m_batteryIcons = icons;
}

lString16 fitTextWidthWithEllipsis( lString16 text, LVFontRef font, int maxwidth )
{
    int w = font->getTextWidth( text.c_str(), text.length() );
    if ( w <= maxwidth )
        return text;
    int len;
    for ( len = text.length()-1; len>1; len-- ) {
        lString16 s = text.substr(0, len) + L"...";
        w = font->getTextWidth( s.c_str(), s.length() );
        if ( w <= maxwidth )
            return s;
    }
    return lString16();
}

/// draw page header to buffer
void LVDocView::drawPageHeader( LVDrawBuf * drawbuf, const lvRect & headerRc, int pageIndex, int phi, int pageCount )
{
    bool drawGauge = true;
    lvRect info = headerRc;
    lUInt32 cl1 = 0xA0A0A0;
    lUInt32 cl2 = getBackgroundColor();
    lUInt32 cl3 = 0xD0D0D0;
    lUInt32 cl4 = 0xC0C0C0;
    lUInt32 pal[4];
    int percent = getPosPercent();
    bool leftPage = (getVisiblePageCount()==2 && !(pageIndex&1) );
    if ( leftPage || !drawGauge )
        percent=10000;
    int percent_pos = percent * info.width() / 10000;
    int gh = drawGauge ? 3 : 1;
    LVArray<int> & sbounds = getSectionBounds();
    lvRect navBar;
    getNavigationBarRectangle( pageIndex, navBar );
    int gpos = info.bottom+2;
    if ( drawbuf->GetBitsPerPixel() <= 2 ) {
        // gray
        cl1 = getTextColor();
        cl3 = 1;
        cl4 = cl1;
        pal[0] = cl1;
        drawbuf->SetTextColor(cl1);
        drawbuf->FillRect(info.left, gpos-gh, info.left+percent_pos, gpos-gh+1, cl1 );
        drawbuf->FillRect(info.left, gpos-1, info.left+percent_pos, gpos, cl1 );
        drawbuf->FillRect(info.left+percent_pos, gpos-gh, info.right, gpos-gh+1, cl1 ); //cl3
        drawbuf->FillRect(info.left+percent_pos, gpos-1, info.right, gpos, cl1 ); // cl3

        if ( !leftPage ) {
            for ( int i=0; i<sbounds.length(); i++) {
                int x = info.left + sbounds[i]*(info.width()-1) / 10000;
                lUInt32 c = cl2; //x<info.left+percent_pos ? cl2 : cl1;
                drawbuf->FillRect(x, gpos-gh, x+2, gpos, c );
                drawbuf->FillRect(x, gpos-2, x+2, gpos-1, cl3 );
                //drawbuf->FillRect(x, info.bottom+2-1, x+2, info.bottom+2, c );
            }
        }

        if ( !leftPage ) {
/*
            drawbuf->FillRect(info.left+percent_pos-2, gpos+0-1, info.left+percent_pos-1, gpos+3-1, cl4 );
            drawbuf->FillRect(info.left+percent_pos-2, gpos+2-1, info.left+percent_pos+2, gpos+3-1, cl4 );
            drawbuf->FillRect(info.left+percent_pos+1, gpos+0-1, info.left+percent_pos+2, gpos+3-1, cl4 );
            drawbuf->FillRect(info.left+percent_pos-2, gpos-4-1, info.left+percent_pos-1, gpos-2-1, cl4 );
            drawbuf->FillRect(info.left+percent_pos-2, gpos-4-1, info.left+percent_pos+2, gpos-3-1, cl4 );

            drawbuf->FillRect(info.left+percent_pos+1, gpos-4-1, info.left+percent_pos+2, gpos-2-1, cl4 );
*/
            drawbuf->FillRect(info.left+percent_pos-2, gpos-4-1, info.left+percent_pos+2, gpos+2, cl4 );
            drawbuf->FillRect(info.left+percent_pos-2+1, gpos-4, info.left+percent_pos+1, gpos+1, cl3 );
/*
            drawbuf->FillRect(info.left+percent_pos, gpos+1, info.left+percent_pos+1, gpos+2, cl1 );
            drawbuf->FillRect(info.left+percent_pos-1, gpos+2, info.left+percent_pos+0, gpos+3, cl1 );
            drawbuf->FillRect(info.left+percent_pos+1, gpos+2, info.left+percent_pos+2, gpos+3, cl1 );
*/
        }
    } else {
        // color
        pal[0] = cl1;
        drawbuf->SetTextColor(cl1);
#if 0
        static lUInt8 pattern[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0};
        drawbuf->FillRectPattern(info.left, gpos-gh, info.left+percent_pos, gpos, cl1, cl2, pattern );
        drawbuf->FillRectPattern(info.left+percent_pos, gpos-gh, info.right, gpos, cl3, cl2, pattern );
#else
        static lUInt8 pattern[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
        drawbuf->FillRect(info.left, gpos-gh, info.left+percent_pos, gpos-gh+1, cl1 );
        drawbuf->FillRectPattern(info.left, gpos-2, info.left+percent_pos, gpos-1, cl3, cl2, pattern );
        drawbuf->FillRect(info.left, gpos-1, info.left+percent_pos, gpos, cl1 );
        drawbuf->FillRect(info.left+percent_pos, gpos-gh, info.right, gpos-gh+1, cl3 );
        drawbuf->FillRect(info.left+percent_pos, gpos-1, info.right, gpos, cl3 );
        if ( !leftPage ) {
            drawbuf->FillRect(info.left+percent_pos-1, gpos+0-1, info.left+percent_pos+0, gpos+3-1, cl1 );
            drawbuf->FillRect(info.left+percent_pos-1, gpos+2-1, info.left+percent_pos+3, gpos+3-1, cl1 );
            drawbuf->FillRect(info.left+percent_pos+2, gpos+0-1, info.left+percent_pos+3, gpos+3-1, cl1 );
            drawbuf->FillRect(info.left+percent_pos-1, gpos-4-1, info.left+percent_pos+0, gpos-2-1, cl1 );
            drawbuf->FillRect(info.left+percent_pos-1, gpos-4-1, info.left+percent_pos+3, gpos-3-1, cl1 );
            drawbuf->FillRect(info.left+percent_pos+2, gpos-4-1, info.left+percent_pos+3, gpos-2-1, cl1 );
/*
            drawbuf->FillRect(info.left+percent_pos, gpos+1, info.left+percent_pos+1, gpos+2, cl1 );
            drawbuf->FillRect(info.left+percent_pos-1, gpos+2, info.left+percent_pos+0, gpos+3, cl1 );
            drawbuf->FillRect(info.left+percent_pos+1, gpos+2, info.left+percent_pos+2, gpos+3, cl1 );
*/
        }
#endif
        if ( !leftPage ) {
            for ( int i=0; i<sbounds.length(); i++) {
                int x = info.left + sbounds[i]*(info.width()-1) / 10000;
                lUInt32 c = cl2; //x<info.left+percent_pos ? cl2 : cl1;
                drawbuf->FillRect(x, gpos-gh, x+2, gpos, c );
                //drawbuf->FillRect(x, info.bottom+2-1, x+2, info.bottom+2, c );
            }
        }
    }

    int iy = info.top + (info.height() - m_infoFont->getHeight()) * 2 / 3;
    if ( getVisiblePageCount()==1 || !(pageIndex&1) ) {
        int dwIcons = 0;
        int icony = iy + m_infoFont->getHeight() / 2;
        for ( int ni=0; ni<m_headerIcons.length(); ni++ ) {
            LVImageSourceRef icon = m_headerIcons[ni];
            int h = icon->GetHeight();
            int w = icon->GetWidth();
            drawbuf->Draw( icon, info.left + dwIcons, icony - h / 2, w, h );
            dwIcons += w + 4;
        }
        info.left += dwIcons;
    }

    if ( (phi & PGHDR_BATTERY) && m_battery_state>=0 ) {
        lvRect brc = info;
        brc.right -= 3;
        brc.top += 1;
        brc.bottom -= 2;
        int h = brc.height();
        bool isVertical = (h>30);
        if ( isVertical )
            brc.left = brc.right - brc.height()/2;
        else
            brc.left = brc.right - 30;
        drawBatteryState( drawbuf, brc, isVertical );
        info.right = brc.left - info.height()/2;
    }
    lString16 pageinfo;
    if ( pageCount>0 ) {
        if ( phi & PGHDR_PAGE_NUMBER )
            pageinfo += lString16::itoa( pageIndex+1 );
        if ( phi & PGHDR_PAGE_COUNT )
            pageinfo += L" / " + lString16::itoa( pageCount );
    }
    int piw = 0;
    if ( !pageinfo.empty() ) {
        piw = m_infoFont->getTextWidth( pageinfo.c_str(), pageinfo.length() );
        m_infoFont->DrawTextString( drawbuf, info.right-piw, iy,
            pageinfo.c_str(), pageinfo.length(), L' ', pal, false);
        info.right -= piw + info.height()/2;
    }
    if ( phi & PGHDR_CLOCK ) {
        lString16 clock = getTimeString();
        m_last_clock = clock;
        int w = m_infoFont->getTextWidth( clock.c_str(), clock.length() ) + 2;
        m_infoFont->DrawTextString( drawbuf, info.right-w, iy,
            clock.c_str(), clock.length(), L' ', pal, false);
        info.right -= w + info.height()/2;
    }
    int titlew = 0;
    lString16 title;
    if ( phi & PGHDR_TITLE ) {
        title = getTitle();
        if ( !title.empty() )
             titlew = m_infoFont->getTextWidth( title.c_str(), title.length() );
    }
    int authorsw = 0;
    lString16 authors;
    if ( phi & PGHDR_AUTHOR ) {
        authors = getAuthors();
        if ( !authors.empty() ) {
            if ( !title.empty() )
                authors += L'.';
            authorsw = m_infoFont->getTextWidth( authors.c_str(), authors.length() );
        }
    }
    int w = info.width() - 10;
    lString16 text;
    if ( authorsw + titlew + 10 > w ) {
        if ( (pageIndex & 1) )
            text = title;
        else {
            text = authors;
            if ( !text.empty() && text[text.length()-1]=='.')
                text = text.substr(0, text.length() - 1 );
        }
    } else {
        text = authors + L"  " + title;
    }
    lvRect oldcr;
    drawbuf->GetClipRect( &oldcr );
    lvRect newcr = oldcr;
    newcr.right = info.right - 10;
    drawbuf->SetClipRect(&newcr);
    text = fitTextWidthWithEllipsis( text, m_infoFont, newcr.width() );
    if ( !text.empty() ) {
        m_infoFont->DrawTextString( drawbuf, info.left, iy,
            text.c_str(), text.length(), L' ', pal, false);
    }
    drawbuf->SetClipRect(&oldcr);
    //--------------
    drawbuf->SetTextColor(getTextColor());
}

void LVDocView::drawPageTo(LVDrawBuf * drawbuf, LVRendPageInfo & page, lvRect * pageRect, int pageCount, int basePage )
{
    int start = page.start;
    int height = page.height;
    lvRect fullRect( 0, 0, drawbuf->GetWidth(), drawbuf->GetHeight() );
    if ( !pageRect )
        pageRect = &fullRect;
    int offset = (pageRect->height() - m_pageMargins.top - m_pageMargins.bottom - height) / 3;
    if (offset>16)
        offset = 16;
    if (offset<0)
        offset = 0;
    offset = 0;
    lvRect clip;
    clip.left = pageRect->left + m_pageMargins.left;
    clip.top = pageRect->top + offset + m_pageMargins.top;
    clip.bottom = pageRect->top + m_pageMargins.top + height + offset;
    clip.right = pageRect->left + pageRect->width() - m_pageMargins.right;
    if ( page.type==PAGE_TYPE_COVER )
        clip.top = pageRect->top + m_pageMargins.bottom;
    if ( m_pageHeaderInfo && page.type!=PAGE_TYPE_COVER) {
        int phi = m_pageHeaderInfo;
        if ( getVisiblePageCount()==2 ) {
            if ( page.index & 1 ) {
                // right
                phi &= ~PGHDR_AUTHOR;
            } else {
                // left
                phi &= ~PGHDR_TITLE;
                phi &= ~PGHDR_PAGE_NUMBER;
                phi &= ~PGHDR_PAGE_COUNT;
                phi &= ~PGHDR_BATTERY;
                phi &= ~PGHDR_CLOCK;
            }
        }
        lvRect info;
        getPageHeaderRectangle( page.index, info );
        drawPageHeader( drawbuf, info, page.index-1+basePage, phi, pageCount-1+basePage );
    }
    drawbuf->SetClipRect(&clip);
    if ( m_doc ) {
        if ( page.type == PAGE_TYPE_COVER ) {
            lvRect rc = *pageRect;
            rc.left += m_pageMargins.left;
            rc.top += m_pageMargins.bottom;
            rc.right -= m_pageMargins.right;
            rc.bottom -= m_pageMargins.bottom;
            drawCoverTo( drawbuf, rc );
        } else {
            // draw main page text
            DrawDocument( *drawbuf, m_doc->getMainNode(), pageRect->left + m_pageMargins.left, pageRect->top + m_pageMargins.top + offset, pageRect->width() - m_pageMargins.left - m_pageMargins.right, height, 0, -start+offset, m_dy, &m_markRanges );
            // draw footnotes
#define FOOTNOTE_MARGIN 8
            int fny = pageRect->top + m_pageMargins.top + offset + page.height + FOOTNOTE_MARGIN;
            int fy = fny;
            bool footnoteDrawed = false;
            for ( int fn=0; fn<page.footnotes.length(); fn++ ) {
                int fstart = page.footnotes[fn].start;
                int fheight = page.footnotes[fn].height;
                clip.top = fy + offset;
                clip.left = pageRect->left + m_pageMargins.left;
                clip.right = pageRect->right - m_pageMargins.right;
                clip.bottom = fy + offset + fheight;
                drawbuf->SetClipRect(&clip);
                DrawDocument( *drawbuf, m_doc->getMainNode(), pageRect->left + m_pageMargins.left, fy + offset, pageRect->width() - m_pageMargins.left - m_pageMargins.right, fheight, 0, -fstart+offset, m_dy, &m_markRanges );
                footnoteDrawed = true;
                fy += fheight;
            }
            if ( footnoteDrawed ) {
                fny -= FOOTNOTE_MARGIN / 2;
                drawbuf->SetClipRect(NULL);
                drawbuf->FillRect( pageRect->left + m_pageMargins.left, fny, pageRect->right - m_pageMargins.right, fny + 1, 0xAAAAAA );
            }
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
    LVLock lock(getMutex());
    checkPos();
    return m_pages.FindNearestPage(m_pos, 0);
}

/// returns true if time changed since clock has been last drawed
bool LVDocView::isTimeChanged()
{
    if ( m_pageHeaderInfo & PGHDR_CLOCK ) {
        bool res = (m_last_clock != getTimeString());
        if ( res )
            m_imageCache.clear();
        return res;
    }
    return false;
}

/// draw to specified buffer
void LVDocView::Draw( LVDrawBuf & drawbuf, int position, bool rotate  )
{
    LVLock lock(getMutex());
    checkPos();
    drawbuf.Resize( m_dx, m_dy );
    drawbuf.SetBackgroundColor( m_backgroundColor );
    drawbuf.SetTextColor( m_textColor );
    drawbuf.Clear(m_backgroundColor);
    if ( drawbuf.GetBitsPerPixel()==32 && getVisiblePageCount()==2 ) {
        int x = drawbuf.GetWidth() / 2;
        lUInt32 cl = m_backgroundColor;
        cl = ((cl & 0xFCFCFC) + 0x404040) >> 1;
        drawbuf.FillRect( x, 0, x+1, drawbuf.GetHeight(), cl);
    }

    if ( !m_is_rendered )
        return;
    if ( !m_doc )
        return;
    if (m_font.isNull())
        return;
    if (m_view_mode==DVM_SCROLL)
    {
        drawbuf.SetClipRect(NULL);
        int cover_height = 0;
        if ( m_pages.length()>0 && m_pages[0]->type == PAGE_TYPE_COVER )
            cover_height = m_pages[0]->height;
        if ( position < cover_height ) {
            lvRect rc;
            drawbuf.GetClipRect( &rc );
            rc.top -= position;
            rc.bottom -= position;
            rc.top += m_pageMargins.bottom;
            rc.bottom -= m_pageMargins.bottom;
            rc.left += m_pageMargins.left;
            rc.right -= m_pageMargins.right;
            drawCoverTo( &drawbuf, rc );
        }
        DrawDocument( drawbuf, m_doc->getMainNode(), m_pageMargins.left, 0, m_dx - m_pageMargins.left - m_pageMargins.right, m_dy, 0, -position, m_dy, &m_markRanges );
    }
    else
    {
        int pc = getVisiblePageCount();
        int page = m_pages.FindNearestPage(position, 0);
        if ( page>=0 && page<m_pages.length() )
            drawPageTo( &drawbuf, *m_pages[page], &m_pageRects[0], m_pages.length(), 1 );
        if ( pc==2 && page>=0 && page+1<m_pages.length() )
            drawPageTo( &drawbuf, *m_pages[page + 1], &m_pageRects[1], m_pages.length(), 1 );
    }
    if ( rotate ) {
        CRLog::trace("Rotate drawing buffer. Src size=(%d, %d), angle=%d, buf(%d, %d)", m_dx, m_dy, m_rotateAngle, drawbuf.GetWidth(), drawbuf.GetHeight() );
        drawbuf.Rotate( m_rotateAngle );
        CRLog::trace("Rotate done. buf(%d, %d)", drawbuf.GetWidth(), drawbuf.GetHeight() );
    }
}

//void LVDocView::Draw()
//{
    //Draw( m_drawbuf, m_pos, true );
//}

/// converts point from window to document coordinates, returns true if success
bool LVDocView::windowToDocPoint( lvPoint & pt )
{
    checkRender();
    pt = rotatePoint( pt, true );
    int page = m_pages.FindNearestPage(m_pos, 0);
    lvRect * rc = NULL;
    lvRect page1( m_pageRects[0] );
    page1.left += m_pageMargins.left;
    page1.top += m_pageMargins.top;
    page1.right -= m_pageMargins.right;
    page1.bottom -= m_pageMargins.bottom;
    if ( page1.isPointInside( pt ) ) {
        rc = &page1;
    } else if ( getVisiblePageCount()==2 ) {
        lvRect page2( m_pageRects[1] );
        page2.left += m_pageMargins.left;
        page2.top += m_pageMargins.top;
        page2.right -= m_pageMargins.right;
        page2.bottom -= m_pageMargins.bottom;
        if ( page2.isPointInside( pt ) ) {
            rc = &page2;
            page++;
        }
    }
    if ( rc && page>=0 && page<m_pages.length() ) {
        int page_y = m_pages[page]->start;
        pt.x -= rc->left;
        pt.y -= rc->top;
        //CRLog::debug(" point page offset( %d, %d )", pt.x, pt.y );
        pt.y += page_y;
        return true;
    }
    return false;
}

/// converts point from documsnt to window coordinates, returns true if success
bool LVDocView::docToWindowPoint( lvPoint & pt )
{
    LVLock lock(getMutex());
    checkRender();
    pt = rotatePoint( pt, false );
    return false;
}

/// returns xpointer for specified window point
ldomXPointer LVDocView::getNodeByPoint( lvPoint pt )
{
    LVLock lock(getMutex());
    checkRender();
    if ( windowToDocPoint( pt ) ) {
        ldomXPointer ptr = m_doc->createXPointer( pt );
        //CRLog::debug("  ptr (%d, %d) node=%08X offset=%d", pt.x, pt.y, (lUInt32)ptr.getNode(), ptr.getOffset() );
        return ptr;
    }
    return ldomXPointer();
}

void LVDocView::updateLayout()
{
    lvRect rc( 0, 0, m_dx, m_dy );
    m_pageRects[0] = rc;
    m_pageRects[1] = rc;
    if ( getVisiblePageCount()==2 ) {
        int middle = (rc.left + rc.right) >> 1;
        m_pageRects[0].right = middle - m_pageMargins.right/2;
        m_pageRects[1].left = middle + m_pageMargins.left/2;
    }
}

/// set list of icons to display at left side of header
void LVDocView::setHeaderIcons( LVRefVec<LVImageSource> icons )
{
    m_headerIcons = icons;
}

/// get page document range, -1 for current page
LVRef<ldomXRange> LVDocView::getPageDocumentRange( int pageIndex )
{
    LVLock lock(getMutex());
    checkRender();
    LVRef<ldomXRange> res(NULL);
    if ( pageIndex<0 || pageIndex>=m_pages.length() )
        pageIndex = getCurPage();
    LVRendPageInfo * page = m_pages[ pageIndex ];
    if ( page->type!=PAGE_TYPE_NORMAL)
        return res;
    ldomXPointer start = m_doc->createXPointer( lvPoint( 0, page->start ) );
    ldomXPointer end = m_doc->createXPointer( lvPoint( m_dx+m_dy, page->start + page->height-1 ) );
    if ( start.isNull() || end.isNull() )
        return res;
    res = LVRef<ldomXRange> ( new ldomXRange(start, end) );
    return res;
}

/// get page text, -1 for current page
lString16 LVDocView::getPageText( bool wrapWords, int pageIndex )
{
    LVLock lock(getMutex());
    checkRender();
    lString16 txt;
    LVRef<ldomXRange> range = getPageDocumentRange( pageIndex );
    txt = range->getRangeText();
    return txt;
}

void LVDocView::Render( int dx, int dy, LVRendPageList * pages )
{
    LVLock lock(getMutex());
    {
        if ( !m_doc || !isDocumentOpened() || m_doc->getMainNode()==NULL)
            return;
        if ( pages==NULL )
            pages = &m_pages;
        updateLayout();
        if ( dx==0 )
            dx = m_pageRects[0].width() - m_pageMargins.left - m_pageMargins.right;
        if ( dy==0 )
            dy = m_pageRects[0].height() - m_pageMargins.top - m_pageMargins.bottom;
        lString8 fontName = lString8(DEFAULT_FONT_NAME);
        m_font = fontMan->GetFont( m_font_size, 300, false, DEFAULT_FONT_FAMILY, m_defaultFontFace );
        m_infoFont = fontMan->GetFont( INFO_FONT_SIZE, 300, false, DEFAULT_FONT_FAMILY, fontName );
        if ( !m_font )
            return;

        pages->clear();
        if ( m_showCover )
            pages->add( new LVRendPageInfo( dy ) );
        LVRendPageContext context( pages, dy );
        CRLog::trace("calling render() for document %08X font=%08X", (unsigned int)m_doc, (unsigned int)m_font.get() );
        m_doc->render( context, dx, m_showCover ? dy + m_pageMargins.bottom*4 : 0, m_font, m_def_interline_space );
        CRLog::trace("returned from render()");

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

        makeToc();
        updateSelections();
    }
}

/// update selection ranges
void LVDocView::updateSelections()
{
    checkRender();
    m_imageCache.clear();
    LVLock lock(getMutex());
    ldomXRangeList ranges( m_doc->getSelections(), true );
    ranges.getRanges( m_markRanges );
}

/// set view mode (pages/scroll)
void LVDocView::setViewMode( LVDocViewMode view_mode, int visiblePageCount )
{
    if ( m_view_mode==view_mode && (visiblePageCount==m_pagesVisible || visiblePageCount<1) )
        return;
    m_imageCache.clear();
    LVLock lock(getMutex());
    m_view_mode = view_mode;
    if ( visiblePageCount==1 || visiblePageCount==2 )
        m_pagesVisible = visiblePageCount;
    requestRender();
    goToBookmark(_posBookmark);
}

/// get view mode (pages/scroll)
LVDocViewMode LVDocView::getViewMode()
{
    return m_view_mode;
}

int LVDocView::getVisiblePageCount()
{
    return (m_view_mode == DVM_SCROLL || m_dx < m_font_size * MIN_EM_PER_PAGE || m_dx*5 < m_dy*6 )
        ? 1
        : m_pagesVisible;
}

/// set window visible page count (1 or 2)
void LVDocView::setVisiblePageCount( int n )
{
    m_imageCache.clear();
    LVLock lock(getMutex());
    if ( n == 2 )
        m_pagesVisible = 2;
    else
        m_pagesVisible = 1;
    updateLayout();
    requestRender();
}

static int findBestFit( LVArray<int> & v, int n, bool rollCyclic=false )
{
    int bestsz = -1;
    int bestfit = -1;
    if ( rollCyclic ) {
        if ( n<v[0] )
            return v[v.length()-1];
        if ( n>v[v.length()-1] )
            return v[0];
    }
    for ( int i=0; i<v.length(); i++ ) {
        int delta = v[i] - n;
        if ( delta<0 )
            delta = -delta;
        if ( bestfit==-1 || bestfit>delta ) {
            bestfit = delta;
            bestsz = v[i];
        }
    }
    if ( bestsz<0 )
        bestsz = n;
    return bestsz;
}

void LVDocView::setDefaultInterlineSpace( int percent )
{
    LVLock lock(getMutex());
    requestRender();
    m_def_interline_space = percent;
    goToBookmark(_posBookmark);
}

void LVDocView::setFontSize( int newSize )
{
    LVLock lock(getMutex());
    m_font_size = findBestFit( m_font_sizes, newSize );
    requestRender();
    //goToBookmark(_posBookmark);
}

void LVDocView::setDefaultFontFace( const lString8 & newFace )
{
    m_defaultFontFace = newFace;
    requestRender();
}

/// sets posible base font sizes (for ZoomFont feature)
void LVDocView::setFontSizes( LVArray<int> & sizes, bool cyclic )
{
    m_font_sizes = sizes;
    m_font_sizes_cyclic = cyclic;
}

void LVDocView::ZoomFont( int delta )
{
    if ( m_font.isNull() )
        return;
#if 1
    int sz = m_font_size;
    for (int i=0; i<15; i++)
    {
        sz += delta;
        int nsz = findBestFit( m_font_sizes, sz, m_font_sizes_cyclic );
        if ( nsz != m_font_size ) {
            setFontSize( nsz );
            return;
        }
        if (sz<12)
            break;
    }
#else
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
#endif
}

/// sets current bookmark
void LVDocView::setBookmark( ldomXPointer bm )
{
    _posBookmark = bm;
}

/// get view height
int LVDocView::GetHeight()
{
    return (m_rotateAngle & 1) ? m_dx : m_dy;
}

/// get view width
int LVDocView::GetWidth()
{
    return (m_rotateAngle & 1) ? m_dy : m_dx;
}

/// sets rotate angle
void LVDocView::SetRotateAngle( cr_rotate_angle_t angle )
{
    if ( m_rotateAngle==angle )
        return;
    m_imageCache.clear();
    LVLock lock(getMutex());
    if ( (m_rotateAngle & 1) == (angle & 1) ) {
        m_rotateAngle = angle;
        return;
    }
    m_rotateAngle = angle;
    int ndx = (angle&1) ? m_dx : m_dy;
    int ndy = (angle&1) ? m_dy : m_dx;
    Resize( ndx, ndy );
}

void LVDocView::Resize( int dx, int dy )
{
    //LVCHECKPOINT("Resize");
    if (dx<80 || dx>3000)
        dx = 80;
    if (dy<80 || dy>3000)
        dy = 80;
    if ( m_rotateAngle==CR_ROTATE_ANGLE_90 || m_rotateAngle==CR_ROTATE_ANGLE_270 ) {
        int tmp = dx;
        dx = dy;
        dy = tmp;
    }
    m_imageCache.clear();
    //m_drawbuf.Resize(dx, dy);
    if (m_doc)
    {
        //ldomXPointer bm = getBookmark();
        if (dx!=m_dx || dy!=m_dy || m_view_mode!=DVM_SCROLL || !m_is_rendered)
        {
            m_dx = dx;
            m_dy = dy;
            updateLayout();
            requestRender();
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

/// save last file position
void LVDocView::savePosition()
{
    if ( m_filename.empty() )
        return;
    //lString16 titleText;
    //lString16 posText;
    //getBookmarkPosText( getBookmark(), titleText, posText );
    m_hist.savePosition( m_filename, m_filesize,
        getTitle(), getAuthors(), getSeries(), getBookmark() );
}

/// restore last file position
void LVDocView::restorePosition()
{
    if ( m_filename.empty() )
        return;
    LVLock lock(getMutex());
    //checkRender();
    ldomXPointer pos = m_hist.restorePosition( m_doc, m_filename, m_filesize );
    if ( !pos.isNull() ) {
        //goToBookmark( pos );
        _posBookmark = pos; //getBookmark();
        m_posIsSet = false;
    }
}

/// load document from file
bool LVDocView::LoadDocument( const lChar16 * fname )
{
    Clear();
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return false;
    if ( LoadDocument( stream ) ) {
        m_filename = lString16(fname);
        return true;
    }
    return false;
}

void LVDocView::createDefaultDocument( lString16 title, lString16 message )
{
    lUInt32 saveFlags = m_doc ? m_doc->getDocFlags() : DOC_FLAG_DEFAULTS;
    if ( m_doc )
        delete m_doc;
    m_is_rendered = false;
#if COMPACT_DOM==1
    m_doc = new ldomDocument( LVStreamRef(), 0 );
#else
    m_doc = new ldomDocument();
#endif
    m_doc->setDocFlags( saveFlags );

    ldomDocumentWriter writer(m_doc);
    m_doc->setNodeTypes( fb2_elem_table );
    m_doc->setAttributeTypes( fb2_attr_table );
    m_doc->setNameSpaceTypes( fb2_ns_table );

    m_pos = 0;

    // make fb2 document structure
    writer.OnTagOpen( NULL, L"?xml" );
    writer.OnAttribute( NULL, L"version", L"1.0" );
    writer.OnAttribute( NULL, L"encoding", L"utf-8" );
    writer.OnEncoding( L"utf-8", NULL );
    writer.OnTagClose( NULL, L"?xml" );
    writer.OnTagOpen( NULL, L"FictionBook" );
      // DESCRIPTION
      writer.OnTagOpen( NULL, L"description" );
        writer.OnTagOpen( NULL, L"title-info" );
          writer.OnTagOpen( NULL, L"book-title" );
            writer.OnText( title.c_str(), title.length(), 0, 0, 0 );
          writer.OnTagClose( NULL, L"book-title" );
        writer.OnTagOpen( NULL, L"title-info" );
      writer.OnTagClose( NULL, L"description" );
      // BODY
      writer.OnTagOpen( NULL, L"body" );
        //m_callback->OnTagOpen( NULL, L"section" );
          // process text
          writer.OnTagOpen( NULL, L"p" );
            writer.OnText( message.c_str(), message.length(), 0, 0, 0 );
          writer.OnTagClose( NULL, L"p" );
        //m_callback->OnTagClose( NULL, L"section" );
      writer.OnTagClose( NULL, L"body" );
    writer.OnTagClose( NULL, L"FictionBook" );

    // set stylesheet
    m_doc->getStyleSheet()->clear();
    m_doc->getStyleSheet()->parse(m_stylesheet.c_str());

    m_series.clear();
    m_authors.clear();
    m_title.clear();

    m_title = title;

    requestRender();
}

/// load document from stream
bool LVDocView::LoadDocument( LVStreamRef stream )
{
    LVLock lock(getMutex());
    {
        m_filesize = stream->GetSize();
        m_stream = stream;

    #if (USE_ZLIB==1)

        m_arc = LVOpenArchieve( m_stream );
        if (!m_arc.isNull())
        {
            // archieve
            bool found = false;
            for (int i=0; i<m_arc->GetObjectCount(); i++)
            {
                const LVContainerItemInfo * item = m_arc->GetObjectInfo(i);
                if (item)
                {
                    if ( !item->IsContainer() )
                    {
                        lString16 name( item->GetName() );
                        bool nameIsOk = false;
                        if ( name.length() >= 5 )
                        {
                            name.lowercase();
                            const lChar16 * pext = name.c_str() + name.length() - 4;
                            if ( pext[0]=='.' && pext[1]=='f' && pext[2]=='b' && pext[3]=='2')
                                nameIsOk = true;
                            else if ( pext[0]=='.' && pext[1]=='t' && pext[2]=='x' && pext[3]=='t')
                                nameIsOk = true;
                            else if ( pext[0]=='.' && pext[1]=='r' && pext[2]=='t' && pext[3]=='f')
                                nameIsOk = true;
                        }
                        if ( !nameIsOk )
                            continue;
                        m_stream = m_arc->OpenStream( item->GetName(), LVOM_READ );
                        if ( m_stream.isNull() )
                            continue;
                        found = true;
                        break;
                    }
                }
            }
            // opened archieve stream
            if ( !found )
            {
                Clear();
                return false;
            }

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

        lUInt32 saveFlags = m_doc ? m_doc->getDocFlags() : DOC_FLAG_DEFAULTS;
        if ( m_doc )
            delete m_doc;
        m_is_rendered = false;
    #if COMPACT_DOM==1
        int minRefLen = COMPACT_DOM_MIN_REF_TEXT_LENGTH;
        m_doc = new ldomDocument( m_stream, minRefLen );
    #else
        m_doc = new ldomDocument();
    #endif
        m_doc->setDocFlags( saveFlags );

#if COMPACT_DOM == 1
        if ( m_stream->GetSize() < COMPACT_DOM_SIZE_THRESHOLD )
            m_doc->setMinRefTextSize( 0 ); // disable compact mode
#endif
        ldomDocumentWriter writer(m_doc);
        m_doc->setNodeTypes( fb2_elem_table );
        m_doc->setAttributeTypes( fb2_attr_table );
        m_doc->setNameSpaceTypes( fb2_ns_table );

        /// FB2 format
        LVFileFormatParser * parser = new LVXMLParser(m_stream, &writer);
        if ( !parser->CheckFormat() ) {
            delete parser;
            parser = NULL;
        }

        /// RTF format
        if ( parser==NULL ) {
            parser = new LVRtfParser(m_stream, &writer);
            if ( !parser->CheckFormat() ) {
                delete parser;
                parser = NULL;
            } else {
#if COMPACT_DOM==1
                m_doc->setMinRefTextSize( 0 );
#endif
            }
        }

        /// plain text format
        if ( parser==NULL ) {
            parser = new LVTextParser(m_stream, &writer);
            if ( !parser->CheckFormat() ) {
                delete parser;
                parser = NULL;
            }
        }

        // unknown format
        if ( !parser ) {
            createDefaultDocument( lString16(L"ERROR: Unknown document format"), lString16(L"Cannot open document") );
            return false;
        }

        // set stylesheet
        m_doc->getStyleSheet()->clear();
        m_doc->getStyleSheet()->parse(m_stylesheet.c_str());

        // parse
        if ( !parser->Parse() ) {
            delete parser;
            createDefaultDocument( lString16(L"ERROR: Bad document format"), lString16(L"Cannot open document") );
            return false;
        }
        delete parser;
        m_pos = 0;


        lString16 docstyle = m_doc->createXPointer(L"/FictionBook/stylesheet").getText();
        if ( !docstyle.empty() && m_doc->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) ) {
            m_doc->getStyleSheet()->parse(UnicodeToUtf8(docstyle).c_str());
        }

    #if 0
        {
            LVStreamRef ostream = LVOpenFileStream( "test_save.fb2", LVOM_WRITE );
            m_doc->saveToStream( ostream, "utf-16" );
            m_doc->getRootNode()->recurseElements( SaveBase64Objects );
        }
    #endif


        m_series.clear();
        m_authors.clear();
        m_title.clear();


        m_authors = extractDocAuthors( m_doc );
        m_title = extractDocTitle( m_doc );
        m_series = extractDocSeries( m_doc );

    }
    requestRender();
    return true;
}

bool LVDocView::LoadDocument( const char * fname )
{
    return LoadDocument( LocalToUnicode(lString8(fname)).c_str() );
}

/// returns bookmark
ldomXPointer LVDocView::getBookmark()
{
    LVLock lock(getMutex());
    checkPos();
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

/// returns bookmark for specified page
ldomXPointer LVDocView::getPageBookmark( int page )
{
    LVLock lock(getMutex());
    checkRender();
    if ( page<0 || page>=m_pages.length() )
        return ldomXPointer();
    ldomXPointer ptr = m_doc->createXPointer( lvPoint( 0, m_pages[page]->start ) );
    return ptr;
}

void limitStringSize( lString16 & str, int maxSize )
{
    if ( (int)str.length()<maxSize )
        return;
    int lastSpace = -1;
    for ( int i=str.length()-1; i>0; i-- )
        if ( str[i]==' ') {
            while ( i>0 && str[i-1]==' ' )
                i--;
            lastSpace = i;
            break;
        }
    int split = lastSpace>0 ? lastSpace : maxSize;
    str = str.substr( 0, split );
    str += L"...";
}

/// get bookmark position text
bool LVDocView::getBookmarkPosText( ldomXPointer bm, lString16 & titleText, lString16 & posText )
{
    LVLock lock(getMutex());
    checkRender();
    titleText = posText = lString16();
    if ( bm.isNull() )
        return false;
    ldomElement * el = (ldomElement *) bm.getNode();
    if ( el->getNodeType()==LXML_TEXT_NODE ) {
        lString16 txt = bm.getNode()->getText();
        int startPos = bm.getOffset();
        int len = txt.length() - startPos;
        if ( len>0 )
            txt = txt.substr( startPos, len );
        if ( startPos>0 )
            posText = L"...";
        posText = txt;
        el = el->getParentNode();
    } else {
        posText = el->getText();
    }
    bool inTitle = false;
    do {
        while ( el && el->getNodeId()!=el_section && el->getNodeId()!=el_body )
        {
            if ( el->getNodeId()==el_title || el->getNodeId()==el_subtitle )
                inTitle = true;
            el = el->getParentNode();
        }
        if ( el ) {
            if ( inTitle ) {
                posText.clear();
                if ( el->getChildCount()>1 ) {
                    ldomNode * node = el->getChildNode(1);
                    posText = node->getText(' ');
                }
                inTitle = false;
            }
            if ( el->getNodeId()==el_body && !titleText.empty() )
                break;
            lString16 txt = getSectionHeader( el );
            lChar16 lastch = !txt.empty() ? txt[txt.length()-1] : 0;
            if ( !titleText.empty() ) {
                if ( lastch!='.' && lastch!='?' && lastch!='!' )
                    txt += L".";
                txt += L" ";
            }
            titleText = txt + titleText;
            el = el->getParentNode();
        }
        if ( titleText.length()>50 )
            break;
    } while ( el );
    limitStringSize( titleText, 70 );
    limitStringSize( posText, 120 );
    return true;
}


/// moves position to bookmark
void LVDocView::goToBookmark(ldomXPointer bm)
{
    LVLock lock(getMutex());
    checkRender();
    m_posIsSet = false;
    _posBookmark = bm;
}

/// get page number by bookmark
int LVDocView::getBookmarkPage(ldomXPointer bm)
{
    LVLock lock(getMutex());
    checkRender();
    if ( bm.isNull() ) {
        return 0;
    } else {
        lvPoint pt = bm.toPoint();
        if ( pt.y<0 )
            return 0;
        return m_pages.FindNearestPage(pt.y, 0);
    }
}

void LVDocView::updateScroll()
{
    checkPos();
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
        char str[32] = {0};
        if ( m_pages.length()>1 ) {
            sprintf(str, "%d / %d", page, m_pages.length()-1 );
        }
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

/// get list of links
void LVDocView::getCurrentPageLinks( ldomXRangeList & list )
{
    list.clear();
    /// get page document range, -1 for current page
    LVRef<ldomXRange> page = getPageDocumentRange();
    if ( !page.isNull() ) {
        // search for links
        class LinkKeeper : public ldomNodeCallback {
            ldomXRangeList &_list;
        public:
            LinkKeeper( ldomXRangeList & list )
                : _list(list) { }
            /// called for each found text fragment in range
            virtual void onText( ldomXRange * nodeRange ) { }
            /// called for each found node in range
            virtual bool onElement( ldomXPointerEx * ptr )
            {
                //
                ldomElement * elem = (ldomElement *)ptr->getNode();
                if ( elem->getNodeId()==el_a ) {
                    _list.add( new ldomXRange(elem) );
                }
                return true;
            }
        };
        LinkKeeper callback( list );
        page->forEach( &callback );
    }
}


void LVDocView::goToPage( int page )
{
    LVLock lock(getMutex());
    checkRender();
    if (!m_pages.length())
        return;
    if (page >= m_pages.length())
        page = m_pages.length()-1;
    if (page<0)
        page = 0;
    SetPos( m_pages[page]->start );
}

/// returns document offset for next page
int LVDocView::getNextPageOffset()
{
    LVLock lock(getMutex());
    checkPos();
    if (m_view_mode==DVM_SCROLL)
    {
        return GetPos() + m_dy;
    }
    else
    {
        int p = m_pages.FindNearestPage(m_pos, 0)  + getVisiblePageCount();
        if ( p<m_pages.length() )
            return m_pages[p]->start;
        return m_pages[m_pages.length()-1]->start;
    }
}

/// returns document offset for previous page
int LVDocView::getPrevPageOffset()
{
    LVLock lock(getMutex());
    checkPos();
    if (m_view_mode==DVM_SCROLL)
    {
        return GetPos() - m_dy;
    }
    else
    {
        int p = m_pages.FindNearestPage(m_pos, 0);
        p -= getVisiblePageCount();
        if ( p<0 )
            p = 0;
        return m_pages[p]->start;
    }
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
                int p = m_pages.FindNearestPage(m_pos, 0);
                goToPage( p - getVisiblePageCount() );
                //goToPage( m_pages.FindNearestPage(m_pos, -1));
            }
        }
        break;
    case DCMD_PAGEUP:
        {
            SetPos( getPrevPageOffset() );
        }
        break;
    case DCMD_PAGEDOWN:
        {
            SetPos( getNextPageOffset() );
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
                int p = m_pages.FindNearestPage(m_pos, 0);
                goToPage( p + getVisiblePageCount() );
                //goToPage( m_pages.FindNearestPage(m_pos, +1));
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


