/** \file lvdocview.h
    \brief XML/CSS document view

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.
*/

#ifndef __LV_TEXT_VIEW_H_INCLUDED__
#define __LV_TEXT_VIEW_H_INCLUDED__

#include "lvtinydom.h"
#include "lvpagesplitter.h"
#include "lvdrawbuf.h"


enum LVDocCmd
{
    DCMD_BEGIN,
    DCMD_LINEUP,
    DCMD_PAGEUP,
    DCMD_PAGEDOWN,
    DCMD_LINEDOWN,
    DCMD_END,
    DCMD_GO_POS,
    DCMD_GO_PAGE,
    DCMD_ZOOM_IN,
    DCMD_ZOOM_OUT,
};

enum LVDocViewMode
{
    DVM_SCROLL,
    DVM_PAGES,
};

class LVScrollInfo
{
public:
    int pos;
    int maxpos;
    int pagesize;
    int scale;
    lString16 posText;
};

enum {
    PGHDR_NONE=0,
    PGHDR_PAGE_NUMBER=1,
    PGHDR_PAGE_COUNT=2,
    PGHDR_AUTHOR=4,
    PGHDR_TITLE=8
};

class LVTocItem;

class LVTocItem
{
private:
    LVTocItem *     _parent;
    int             _level;
    int             _index;
	lString16       _name;
    ldomXPointer    _position;
    LVPtrVector<LVTocItem> _children;
    //====================================================
    LVTocItem( ldomXPointer pos, const lString16 & name ) : _parent(NULL), _level(0), _index(0), _name(name), _position(pos) { }
    void addChild( LVTocItem * item ) { item->_level=_level+1; item->_parent=this; item->_index=_children.length(), _children.add(item); }
    //====================================================
public:
    /// returns parent node pointer
    LVTocItem * getParent() const { return _parent; }
    /// returns node level (0==root node, 1==top level)
    int getLevel() const { return _level; }
    /// returns node index
    int getIndex() const { return _index; }
    /// returns section title
    lString16 getName() const { return _name; }
    /// returns position
    ldomXPointer getXPointer() const { return _position; }
    /// returns Y position
    int getY();
    /// returns page number
    int getPageNum( LVRendPageList & pages );
    /// returns child node count
    int getChildCount() const { return _children.length(); }
    /// returns child node by index
    LVTocItem * getChild( int index ) const { return _children[index]; }
    /// add child TOC node
    LVTocItem * addChild( const lString16 & name, ldomXPointer ptr )
    {
        LVTocItem * item = new LVTocItem( ptr, name );
        addChild( item );
        return item;
    }
    void clear() { _children.clear(); }    
    // root node constructor
    LVTocItem() : _parent(NULL), _level(0), _index(0) { }
    ~LVTocItem() { clear(); }
};

//typedef lUInt64 LVPosBookmark;

/**
    \brief XML document view

    Platform independant document view implementation.

    Supports scroll view of document.
*/
class LVDocView
{
private:
    int m_dx;
    int m_dy;
    int m_pos;
    int m_font_size;
    LVArray<int> m_font_sizes;
    bool m_font_sizes_cyclic;
    bool m_is_rendered;
    LVDocViewMode m_view_mode;
    LVTocItem m_toc;
#if (COLOR_BACKBUFFER==1)
    LVColorDrawBuf m_drawbuf;
#else
    LVGrayDrawBuf  m_drawbuf;
#endif
    lUInt32 m_backgroundColor;
    lUInt32 m_textColor;
    font_ref_t     m_font;
    font_ref_t     m_infoFont;
    LVStreamRef    m_stream;
    LVContainerRef m_arc;
    ldomDocument * m_doc;
    lString8 m_stylesheet;
    LVRendPageList m_pages;
    LVScrollInfo m_scrollinfo;

    lString16 m_title;
    lString16 m_authors;
    lString16 m_series;
    
    ldomXPointer _posBookmark;

    lvRect m_pageMargins;
    lvRect m_pageRects[2];
    int    m_pagesVisible;
    int m_pageHeaderInfo;
    bool m_showCover;


    // private functions
    void updateScroll();
    /// makes table of contents for current document
    void makeToc();
    /// updates page layout
    void updateLayout();
    
public:
    LVRendPageList * getPageList() { return &m_pages; }
    /// returns pointer to TOC root node
    LVTocItem * getToc() { return &m_toc; }
    /// set view mode (pages/scroll)
    void setViewMode( LVDocViewMode view_mode );
    /// get view mode (pages/scroll)
    LVDocViewMode getViewMode();
    /// get window visible page count (1 or 2)
    int getVisiblePageCount();
    /// set window visible page count (1 or 2)
    void setVisiblePageCount( int n );

    /// get page header info mask
    int getPageHeaderInfo() { return m_pageHeaderInfo; }
    /// set page header info mask
    void setPageHeaderInfo( int hdrFlags );

    /// returns background color
    lUInt32 getBackgroundColor()
    {
        return m_backgroundColor;
    }
    /// sets background color
    void setBackgroundColor( lUInt32 cl )
    {
        m_backgroundColor = cl;
        Draw();
    }
    /// returns text color
    lUInt32 getTextColor()
    {
        return m_textColor;
    }
    /// sets text color
    void setTextColor( lUInt32 cl )
    {
        m_textColor = cl;
        Draw();
    }

    /// returns xpointer for specified window point
    ldomXPointer getNodeByPoint( lvPoint pt );

    /// returns document
    ldomDocument * getDocument() { return m_doc; }

    /// returns book title
    lString16 getTitle() { return m_title; }
    /// returns book author(s)
    lString16 getAuthors() { return m_authors; }
    /// returns book series name and number (series name #1)
    lString16 getSeries() { return m_series; }

    /// export to WOL format
    bool exportWolFile( const char * fname, bool flgGray, int levels );
    /// export to WOL format
    bool exportWolFile( const wchar_t * fname, bool flgGray, int levels );
    /// export to WOL format
    bool exportWolFile( LVStream * stream, bool flgGray, int levels );

    /// draws page to image buffer
    void drawPageTo( LVDrawBuf * drawBuf, LVRendPageInfo & page, lvRect * pageRect=NULL);
    /// draws coverpage to image buffer
    void drawCoverTo( LVDrawBuf * drawBuf, lvRect & rc );
    /// returns cover page image source, if any
    LVImageSourceRef getCoverPageImage();

    /// returns bookmark
    ldomXPointer getBookmark();
    /// moves position to bookmark
    void goToBookmark(ldomXPointer bm);

    /// returns scrollbar control info
    const LVScrollInfo * getScrollInfo() { return &m_scrollinfo; }
    /// converts scrollbar pos to doc pos
    int scrollPosToDocPos( int scrollpos );

    /// execute command
    void doCommand( LVDocCmd cmd, int param=0 );

    /// set document stylesheet text
    void setStyleSheet( lString8 css_text );

    /// change font size, if rollCyclic is true, largest font is followed by smallest and vice versa
    void ZoomFont( int delta );
    /// retrieves current base font size
    int  getFontSize() { return m_font_size; }
    /// sets new base font size
    void setFontSize( int newSize );
    /// sets posible base font sizes (for ZoomFont)
    void setFontSizes( LVArray<int> & sizes, bool cyclic );

    /// get drawing buffer
    LVDrawBuf * GetDrawBuf() { return &m_drawbuf; }
    /// draw document into buffer
    void Draw();

    /// resize view
    void Resize( int dx, int dy );
    /// get view height
    int GetHeight() { return m_dy; }
    /// get view width
    int GetWidth() { return m_dx; }

    /// get full document height
    int GetFullHeight();

    /// get vertical position of view inside document
    int GetPos() { return m_pos; }
    /// set vertical position of view inside document
    void SetPos( int pos, bool savePos=true );

    /// get number of current page
    int getCurPage();
    /// move to specified page
    void goToPage( int page );
    /// returns page count
    int getPageCount() { return m_pages.length(); }

    /// clear view
    void Clear();
    /// load document from file
    bool LoadDocument( const char * fname );
    /// load document from file
    bool LoadDocument( const lChar16 * fname );
    /// load document from stream
    bool LoadDocument( LVStreamRef stream );

    /// render (format) document
    void Render( int dx=0, int dy=0, LVRendPageList * pages=NULL );

    /// Constructor
    LVDocView();
    /// Destructor
    virtual ~LVDocView();
};


#endif
