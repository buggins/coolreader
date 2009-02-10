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

#include "crsetup.h"
#include "lvtinydom.h"
#include "lvpagesplitter.h"
#include "lvdrawbuf.h"
#include "hist.h"
#include "lvthread.h"

// standard properties supported by LVDocView
#define PROP_FONT_ANTIALIASING       "font.antialiasing.mode"
#define PROP_FONT_COLOR              "font.color.default"
#define PROP_FONT_FACE               "font.face.default"
#define PROP_BACKGROUND_COLOR        "background.color.default"
#define PROP_TXT_OPTION_PREFORMATTED "crengine.file.txt.preformatted"
#define PROP_LOG_FILENAME            "crengine.log.filename"
#define PROP_LOG_LEVEL               "crengine.log.level"
#define PROP_FONT_SIZE               "crengine.font.size"
#define PROP_PAGE_MARGIN_TOP         "crengine.page.margin.top"
#define PROP_PAGE_MARGIN_BOTTOM      "crengine.page.margin.bottom"
#define PROP_PAGE_MARGIN_LEFT        "crengine.page.margin.left"
#define PROP_PAGE_MARGIN_RIGHT       "crengine.page.margin.right"
#define PROP_INTERLINE_SPACE         "crengine.interline.space"
#define PROP_ROTATE_ANGLE            "window.rotate.angle"
#define PROP_EMBEDDED_STYLES         "crengine.doc.embedded.styles.enabled"
#define PROP_DISPLAY_INVERSE         "crengine.display.inverse"
#define PROP_STATUS_LINE             "window.status.line"
#define PROP_BOOKMARK_ICONS          "crengine.bookmarks.icons"
#define PROP_FOOTNOTES               "crengine.footnotes"
#define PROP_SHOW_TIME               "window.status.clock"
#define PROP_FONT_KERNING_ENABLED    "font.kerning.enabled"
#define PROP_LANDSCAPE_PAGES         "window.landscape.pages"

/// source document formats
typedef enum {
    doc_format_none,
    doc_format_fb2,
    doc_format_txt,
    doc_format_rtf,
    doc_format_epub,
    doc_format_html,
} doc_format_t;

/// text format import options
typedef enum {
    txt_format_pre,  // no formatting, leave lines as is
    txt_format_auto  // autodetect format
} txt_format_t;


/// Page imege holder which allows to unlock mutex after destruction
class LVDocImageHolder
{
private:
    LVRef<LVDrawBuf> _drawbuf;
    LVMutex & _mutex;
public:
    LVDrawBuf * getDrawBuf() { return _drawbuf.get(); }
    LVRef<LVDrawBuf> getDrawBufRef() { return _drawbuf; }
    LVDocImageHolder( LVRef<LVDrawBuf> drawbuf, LVMutex & mutex )
    : _drawbuf(drawbuf), _mutex(mutex)
    {
    }
    ~LVDocImageHolder()
    {
        _drawbuf = NULL;
        _mutex.unlock();
    }
};

typedef LVRef<LVDocImageHolder> LVDocImageRef;

class LVDocViewImageCache
{
    private:
        LVMutex _mutex;
        class Item {
            public:
                LVRef<LVDrawBuf> _drawbuf;
                LVRef<LVThread> _thread;
                int _offset;
                bool _ready;
                bool _valid;
        };
        Item _items[2];
        int _last;
    public:
        /// return mutex
        LVMutex & getMutex() { return _mutex; }
        /// set page to cache
        void set( int offset, LVRef<LVDrawBuf> drawbuf, LVRef<LVThread> thread )
        {
            LVLock lock( _mutex );
            _last = (_last + 1) & 1;
            _items[_last]._ready = false;
            _items[_last]._thread = thread;
            _items[_last]._drawbuf = drawbuf;
            _items[_last]._offset = offset;
            _items[_last]._valid = true;
        }
        /// return page image, wait until ready
        LVRef<LVDrawBuf> getWithoutLock( int offset )
        {
            for ( int i=0; i<2; i++ ) {
                if ( _items[i]._valid && _items[i]._offset == offset ) {
                    if ( !_items[i]._ready ) {
                        _items[i]._thread->join();
                        _items[i]._thread = NULL;
                        _items[i]._ready = true;
                    }
                    _last = i;
                    return _items[i]._drawbuf;
                }
            }
            return LVRef<LVDrawBuf>();
        }
        /// return page image, wait until ready
        LVDocImageRef get( int offset )
        {
            _mutex.lock();
            LVRef<LVDrawBuf> buf = getWithoutLock( offset );
            if ( !buf.isNull() )
                return LVDocImageRef( new LVDocImageHolder(getWithoutLock( offset ), _mutex) );
            return LVDocImageRef( NULL );
        }
        bool has( int offset )
        {
            _mutex.lock();
            for ( int i=0; i<2; i++ ) {
                if ( _items[i]._valid && _items[i]._offset == offset ) {
                    return true;
                }
            }
            return false;
        }
        void clear()
        {
            LVLock lock( _mutex );
            for ( int i=0; i<2; i++ ) {
                if ( _items[i]._valid && !_items[i]._ready ) {
                    _items[i]._thread->join();
                }
                _items[i]._thread = NULL;
                _items[i]._valid = false;
                _items[i]._drawbuf = NULL;
                _items[i]._offset = 0;
            }
        }
        LVDocViewImageCache()
        : _last(0)
        {
            for ( int i=0; i<2; i++ )
                _items[i]._valid = false;
        }
        ~LVDocViewImageCache()
        {
            clear();
        }
};

/// LVDocView commands
#define LVDOCVIEW_COMMANDS_START 100
enum LVDocCmd
{
    DCMD_BEGIN = LVDOCVIEW_COMMANDS_START,
    DCMD_LINEUP,
    DCMD_PAGEUP,
    DCMD_PAGEDOWN,
    DCMD_LINEDOWN,
    DCMD_LINK_FORWARD,
    DCMD_LINK_BACK,
    DCMD_LINK_NEXT,
    DCMD_LINK_PREV,
    DCMD_LINK_GO,
    DCMD_END,
    DCMD_GO_POS,
    DCMD_GO_PAGE,
    DCMD_ZOOM_IN,
    DCMD_ZOOM_OUT,
    DCMD_TOGGLE_TEXT_FORMAT,
    DCMD_BOOKMARK_SAVE_N, // save current page bookmark under spicified number
    DCMD_BOOKMARK_GO_N,  // go to bookmark with specified number
	DCMD_MOVE_BY_CHAPTER, // param=-1 - previous chapter, 1 = next chapter
};
#define LVDOCVIEW_COMMANDS_END DCMD_MOVE_BY_CHAPTER

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
    LVScrollInfo()
    : pos(0), maxpos(0), pagesize(0), scale(1)
    {
    }
};

/// page header flags
enum {
    PGHDR_NONE=0,
    PGHDR_PAGE_NUMBER=1,
    PGHDR_PAGE_COUNT=2,
    PGHDR_AUTHOR=4,
    PGHDR_TITLE=8,
    PGHDR_CLOCK=16,
    PGHDR_BATTERY=32,
};

class LVTocItem;
class LVDocView;

class LVTocItem
{
private:
    LVTocItem *     _parent;
    int             _level;
    int             _index;
    int             _page;
    int             _percent;
    lString16       _name;
    ldomXPointer    _position;
    LVPtrVector<LVTocItem> _children;
    //====================================================
    LVTocItem( ldomXPointer pos, const lString16 & name ) : _parent(NULL), _level(0), _index(0), _page(0), _percent(0), _name(name), _position(pos) { }
    void addChild( LVTocItem * item ) { item->_level=_level+1; item->_parent=this; item->_index=_children.length(), _children.add(item); }
    //====================================================
    void setPage( int n ) { _page = n; }
    void setPercent( int n ) { _percent = n; }
public:
    /// update page numbers for items
    void updatePageNumbers( LVDocView * docview );
    /// get page number
    int getPage() { return _page; }
    /// get position percent * 100
    int getPercent() { return _percent; }
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

/// Callback interface 
class LVDocViewCallback {
public:
    /// Override to handle external links
    virtual void OnExternalLink( lString16 url, ldomElement * node ) { }
    virtual ~LVDocViewCallback() { }
};

/**
    \brief XML document view

    Platform independant document view implementation.

    Supports scroll view of document.
*/
class LVDocView
{
    friend class LVDrawThread;
private:
    int m_dx;
    int m_dy;
    int m_pos;
    int m_battery_state;
    int m_font_size;
    int m_def_interline_space;
    LVArray<int> m_font_sizes;
    bool m_font_sizes_cyclic;
    bool m_is_rendered;
    LVDocViewMode m_view_mode;
    LVTocItem m_toc;
    /*
#if (COLOR_BACKBUFFER==1)
    LVColorDrawBuf m_drawbuf;
#else
    LVGrayDrawBuf  m_drawbuf;
#endif
    */
    lUInt32 m_backgroundColor;
    lUInt32 m_textColor;
    font_ref_t     m_font;
    font_ref_t     m_infoFont;
    LVContainerRef m_container;
    LVStreamRef    m_stream;
    LVContainerRef m_arc;
    ldomDocument * m_doc;
    lString8 m_stylesheet;
    LVRendPageList m_pages;
    LVScrollInfo m_scrollinfo;
    LVImageSourceRef m_defaultCover;

protected:
    lString16 m_last_clock;

    ldomMarkedRangeList m_markRanges;

private:
    lString16 m_filename;
    lvsize_t  m_filesize;

    ldomXPointer _posBookmark;

    lvRect m_pageMargins;
    lvRect m_pageRects[2];
    int    m_pagesVisible;
    int m_pageHeaderInfo;
    bool m_showCover;
    LVRefVec<LVImageSource> m_headerIcons;
    LVRefVec<LVImageSource> m_batteryIcons;

    cr_rotate_angle_t m_rotateAngle;

    CRFileHist m_hist;

    LVArray<int> m_section_bounds;
    bool m_section_bounds_valid;

    LVMutex _mutex;
    LVDocViewImageCache m_imageCache;

    bool m_posIsSet;

    lString8 m_defaultFontFace;
    ldomNavigationHistory _navigationHistory;

    doc_format_t m_doc_format;
    txt_format_t m_text_format;

    LVDocViewCallback * m_callback;

    CRPropRef m_props;

    /// sets current document format
    void setDocFormat( doc_format_t fmt ) { m_doc_format = fmt; }


    // private functions
    void updateScroll();
    /// makes table of contents for current document
    void makeToc();
    /// updates page layout
    void updateLayout();
    /// parse document from m_stream
    bool ParseDocument( );


protected:
    /// draw to specified buffer
    void Draw( LVDrawBuf & drawbuf, int pageTopPosition, bool rotate );

    virtual void drawNavigationBar( LVDrawBuf * drawbuf, int pageIndex, int percent );

    virtual void getNavigationBarRectangle( lvRect & rc );

    virtual void getNavigationBarRectangle( int pageIndex, lvRect & rc );

    virtual void getPageRectangle( int pageIndex, lvRect & pageRect );
    /// returns document offset for next page
    int getNextPageOffset();
    /// returns document offset for previous page
    int getPrevPageOffset();
    /// render document, if not rendered
    void checkRender();
    /// ensure current position is set to current bookmark value
    void checkPos();
    /// selects link on page, if any (delta==0 - current, 1-next, -1-previous). returns selected link range, null if no links.
    virtual ldomXRange * selectPageLink( int delta, bool wrapAround);
	/// returns pointer to bookmark/last position containter of currently opened file
	CRFileHistRecord * getCurrentFileHistRecord();
public:
	/// -1 moveto previous chapter, 0 to current chaoter first pae, 1 to next chapter
	bool moveByChapter( int delta );
	/// saves current page bookmark under numbered shortcut
	void saveCurrentPageShortcutBookmark( int number );
	/// restores page using bookmark by numbered shortcut
	bool goToPageShortcutBookmark( int number );
    /// returns true if page image is available (0=current, -1=prev, 1=next)
    bool getShowCover() { return  m_showCover; }
    /// returns true if page image is available (0=current, -1=prev, 1=next)
    void setShowCover( bool show ) { m_showCover = show; }
    /// returns true if page image is available (0=current, -1=prev, 1=next)
    bool isPageImageReady( int delta );

    // property support methods
    /// sets default property values if properties not found, checks ranges
    void propsUpdateDefaults( CRPropRef props );
    /// applies properties, returns list of not recognized properties
    CRPropRef propsApply( CRPropRef props );
    /// returns current values of supported properties
    CRPropRef propsGetCurrent();

    /// get current default cover image
    LVImageSourceRef getDefaultCover() const { return m_defaultCover; }
    /// set default cover image (for books w/o cover)
    void setDefaultCover(LVImageSourceRef cover) { m_defaultCover = cover; clearImageCache(); }

    // callback functions
    /// set callback
    void setCallback( LVDocViewCallback * callback ) { m_callback = callback; }
    /// get callback
    LVDocViewCallback * getCallback( ) { return m_callback; }

    // doc format functions
    /// set text format options
    void setTextFormatOptions( txt_format_t fmt );
    /// get text format options
    txt_format_t getTextFormatOptions() { return m_text_format; }
    /// get current document format
    doc_format_t getDocFormat() { return m_doc_format; }

    // Links and selections functions
    /// sets selection for whole element, clears previous selection
    virtual void selectElement( ldomElement * elem );
    /// sets selection for range, clears previous selection
    virtual void selectRange( const ldomXRange & range );
    /// sets selection for list of words, clears previous selection
    virtual void selectWords( const LVArray<ldomWord> & words );
    /// clears selection
    virtual void clearSelection();
    /// navigation history
    ldomNavigationHistory & getNavigationHistory() { return _navigationHistory; }
    /// get list of links
    virtual void getCurrentPageLinks( ldomXRangeList & list );
    /// selects first link on page, if any. returns selected link range, null if no links.
    virtual ldomXRange * selectFirstPageLink();
    /// selects next link on page, if any. returns selected link range, null if no links.
    virtual ldomXRange * selectNextPageLink( bool wrapAround);
    /// selects previous link on page, if any. returns selected link range, null if no links.
    virtual ldomXRange * selectPrevPageLink( bool wrapAround );
    /// returns selected link on page, if any. null if no links.
    virtual ldomXRange * getCurrentPageSelectedLink();
    /// follow link, returns true if navigation was successful
    virtual bool goLink( lString16 href );
    /// follow selected link, returns true if navigation was successful
    virtual bool goSelectedLink();
    /// go back. returns true if navigation was successful
    virtual bool goBack();
    /// go forward. returns true if navigation was successful
    virtual bool goForward();


    /// create empty document with specified message (to show errors)
    virtual void createDefaultDocument( lString16 title, lString16 message );

    /// returns default font face
    lString8 getDefaultFontFace() { return m_defaultFontFace; }
    /// set default font face
    void setDefaultFontFace( const lString8 & newFace );
    /// invalidate formatted data, request render
    void requestRender();
    /// invalidate document data, request reload
    void requestReload();
    /// invalidate image cache, request redraw
    void clearImageCache();
    /// get page image (0=current, -1=prev, 1=next)
    LVDocImageRef getPageImage( int delta );
    /// returns true if current page image is ready
    bool IsDrawed();
    /// cache page image (render in background if necessary) (0=current, -1=prev, 1=next)
    void cachePageImage( int delta );
    /// return view mutex
    LVMutex & getMutex() { return _mutex; }
    /// update selection ranges
    void updateSelections();
    /// get page document range, -1 for current page
    LVRef<ldomXRange> getPageDocumentRange( int pageIndex=-1 );
    /// get page text, -1 for current page
    lString16 getPageText( bool wrapWords, int pageIndex=-1 );
    /// calculate page header rectangle
    virtual void getPageHeaderRectangle( int pageIndex, lvRect & headerRc );
    /// calculate page header height
    virtual int getPageHeaderHeight( );
    /// set list of icons to display at left side of header
    void setHeaderIcons( LVRefVec<LVImageSource> icons );
    /// set list of battery icons to display battery state
    void setBatteryIcons( LVRefVec<LVImageSource> icons );
    /// sets page margins
    void setPageMargins( const lvRect & rc );
    /// returns page margins
    lvRect getPageMargins() const { return m_pageMargins; }
    /// sets rotate angle
    void SetRotateAngle( cr_rotate_angle_t angle );
    /// rotate rectangle by current angle, winToDoc==false for doc->window translation, true==ccw
    lvRect rotateRect( lvRect & rc, bool winToDoc );
    /// rotate point by current angle, winToDoc==false for doc->window translation, true==ccw
    lvPoint rotatePoint( lvPoint & pt, bool winToDoc );
    /// returns rotate angle
    cr_rotate_angle_t GetRotateAngle() { return m_rotateAngle; }
    /// returns true if document is opened
    bool isDocumentOpened();
    /// returns section bounds, in 1/100 of percent
    LVArray<int> & getSectionBounds( );
    /// sets battery state
    void setBatteryState( int newState ) { m_battery_state = newState; }
    /// returns battery state
    int getBatteryState( ) { return m_battery_state; }
    /// returns current time representation string
    virtual lString16 getTimeString();
    /// returns true if time changed since clock has been last drawed
    bool isTimeChanged();
    /// returns if Render has been called
    bool IsRendered() { return m_is_rendered; }
    /// returns file list with positions/bookmarks
    CRFileHist * getHistory() { return &m_hist; }
    /// returns formatted page list
    LVRendPageList * getPageList() { return &m_pages; }
    /// returns pointer to TOC root node
    LVTocItem * getToc();
    /// returns pointer to TOC root node
    bool getFlatToc( LVPtrVector<LVTocItem, false> & items );
    /// set view mode (pages/scroll)
    void setViewMode( LVDocViewMode view_mode, int visiblePageCount=-1 );
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
    /// get info line font
    font_ref_t getInfoFont() { return m_infoFont; }
    /// set info line font
    void setInfoFont( font_ref_t font ) { m_infoFont = font; }
    /// draw page header to buffer
    virtual void drawPageHeader( LVDrawBuf * drawBuf, const lvRect & headerRc, int pageIndex, int headerInfoFlags, int pageCount );
    /// draw battery state to buffer
    virtual void drawBatteryState( LVDrawBuf * drawBuf, const lvRect & rc, bool isVertical );

    /// returns background color
    lUInt32 getBackgroundColor()
    {
        return m_backgroundColor;
    }
    /// sets background color
    void setBackgroundColor( lUInt32 cl )
    {
        m_backgroundColor = cl;
        m_imageCache.clear();
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
        m_imageCache.clear();
    }

    /// returns xpointer for specified window point
    ldomXPointer getNodeByPoint( lvPoint pt );
    /// converts point from window to document coordinates, returns true if success
    bool windowToDocPoint( lvPoint & pt );
    /// converts point from documsnt to window coordinates, returns true if success
    bool docToWindowPoint( lvPoint & pt );

    /// returns document
    ldomDocument * getDocument() { return m_doc; }

    /// returns book title
    lString16 getTitle() { return m_doc->getProps()->getStringDef(DOC_PROP_TITLE); }
    /// returns book author(s)
    lString16 getAuthors() { return m_doc->getProps()->getStringDef(DOC_PROP_AUTHORS); }
    /// returns book series name and number (series name #1)
    lString16 getSeries()
    { 
        lString16 name = m_doc->getProps()->getStringDef(DOC_PROP_SERIES_NAME);
        lString16 number = m_doc->getProps()->getStringDef(DOC_PROP_SERIES_NUMBER);
        if ( !name.empty() && !number.empty() )
            name << L" #" << number;
        return name;
    }

    /// export to WOL format
    bool exportWolFile( const char * fname, bool flgGray, int levels );
    /// export to WOL format
    bool exportWolFile( const wchar_t * fname, bool flgGray, int levels );
    /// export to WOL format
    bool exportWolFile( LVStream * stream, bool flgGray, int levels );

    /// draws page to image buffer
    void drawPageTo( LVDrawBuf * drawBuf, LVRendPageInfo & page, lvRect * pageRect, int pageCount, int basePage);
    /// draws coverpage to image buffer
    void drawCoverTo( LVDrawBuf * drawBuf, lvRect & rc );
    /// returns cover page image source, if any
    LVImageSourceRef getCoverPageImage();

    /// returns bookmark
    ldomXPointer getBookmark();
    /// returns bookmark for specified page
    ldomXPointer getPageBookmark( int page );
    /// sets current bookmark
    void setBookmark( ldomXPointer bm );
    /// moves position to bookmark
    void goToBookmark( ldomXPointer bm );
    /// get page number by bookmark
    int getBookmarkPage(ldomXPointer bm);
    /// get bookmark position text
    bool getBookmarkPosText( ldomXPointer bm, lString16 & titleText, lString16 & posText );

    /// returns scrollbar control info
    const LVScrollInfo * getScrollInfo() { updateScroll(); return &m_scrollinfo; }
    /// converts scrollbar pos to doc pos
    int scrollPosToDocPos( int scrollpos );
    /// returns position in 1/100 of percents
    int getPosPercent();

    /// execute command
    void doCommand( LVDocCmd cmd, int param=0 );

    /// set document stylesheet text
    void setStyleSheet( lString8 css_text );

    /// set default interline space, percent (100..200)
    void setDefaultInterlineSpace( int percent );

    /// change font size, if rollCyclic is true, largest font is followed by smallest and vice versa
    void ZoomFont( int delta );
    /// retrieves current base font size
    int  getFontSize() { return m_font_size; }
    /// sets new base font size
    void setFontSize( int newSize );
    /// sets posible base font sizes (for ZoomFont)
    void setFontSizes( LVArray<int> & sizes, bool cyclic );

    /// get drawing buffer
    //LVDrawBuf * GetDrawBuf() { return &m_drawbuf; }
    /// draw document into buffer
    //void Draw();

    /// resize view
    void Resize( int dx, int dy );
    /// get view height
    int GetHeight();
    /// get view width
    int GetWidth();

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

    /// save last file position
    void savePosition();
    /// restore last file position
    void restorePosition();

    /// render (format) document
    void Render( int dx=0, int dy=0, LVRendPageList * pages=NULL );

    /// Constructor
    LVDocView();
    /// Destructor
    virtual ~LVDocView();
};


#endif
