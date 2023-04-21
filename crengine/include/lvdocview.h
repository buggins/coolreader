/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2013,2015 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2011,2014 Konstantin Potapov <pkbo@users.sourceforge.net>
 *   Copyright (C) 2012 Daniel Savard <daniels@xsoli.com>                  *
 *   Copyright (C) 2014 Qingping Hou <dave2008713@gmail.com>               *
 *   Copyright (C) 2018 EXL <exlmotodev@gmail.com>                         *
 *   Copyright (C) 2020 Jellby <jellby@yahoo.com>                          *
 *   Copyright (C) 2017-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2019-2021 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

/** \file lvdocview.h
    \brief XML/CSS document view

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2009

    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.
*/

#ifndef __LV_TEXT_VIEW_H_INCLUDED__
#define __LV_TEXT_VIEW_H_INCLUDED__

#include "crsetup.h"
#include "crskin.h"
#include "lvtinydom.h"
#include "lvpagesplitter.h"
#include "lvdrawbuf.h"
#include "lvcolordrawbuf.h"
#include "hist.h"
#include "lvthread.h"
#include "lvdocviewcmd.h"
#include "lvdocviewprops.h"


const lChar32 * getDocFormatName( doc_format_t fmt );

/// text format import options
typedef enum {
    txt_format_pre,  // no formatting, leave lines as is
    txt_format_auto  // autodetect format
} txt_format_t;

/// Battery state: no battery
#define CR_BATTERY_STATE_NO_BATTERY -2
/// Battery state: battery is charging
#define CR_BATTERY_STATE_CHARGING -1
/// Battery state: battery is discharging
#define CR_BATTERY_STATE_DISCHARGING -3

/// Battery charger connection: no connection
#define CR_BATTERY_CHARGER_NO 1
/// Battery charger connection: AC adapter
#define CR_BATTERY_CHARGER_AC 2
/// Battery charger connection: USB
#define CR_BATTERY_CHARGER_USB 3
/// Battery charger connection: Wireless
#define CR_BATTERY_CHARGER_WIRELESS 4

#ifndef CR_ENABLE_PAGE_IMAGE_CACHE
#ifdef ANDROID
#define CR_ENABLE_PAGE_IMAGE_CACHE 0
#else
#define CR_ENABLE_PAGE_IMAGE_CACHE 1
#endif
#endif//#ifndef CR_ENABLE_PAGE_IMAGE_CACHE

#if CR_ENABLE_PAGE_IMAGE_CACHE==1
/// Page imege holder which allows to unlock mutex after destruction
class LVDocImageHolder
{
private:
    LVRef<LVDrawBuf> _drawbuf;
    LVMutex & _mutex;
	LVDocImageHolder & operator = (LVDocImageHolder&) {
		// no assignment
        return *this;
    }
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

/// page image cache
class LVDocViewImageCache
{
    private:
        LVMutex _mutex;
        class Item {
            public:
                LVRef<LVDrawBuf> _drawbuf;
                LVRef<LVThread> _thread;
                int _offset;
                int _page;
                bool _ready;
                bool _valid;
        };
        Item _items[2];
        int _last;
    public:
        /// return mutex
        LVMutex & getMutex() { return _mutex; }
        /// set page to cache
        void set( int offset, int page, LVRef<LVDrawBuf> drawbuf, LVRef<LVThread> thread )
        {
            LVLock lock( _mutex );
            _last = (_last + 1) & 1;
            _items[_last]._ready = false;
            _items[_last]._thread = thread;
            _items[_last]._drawbuf = drawbuf;
            _items[_last]._offset = offset;
            _items[_last]._page = page;
            _items[_last]._valid = true;
        }
        /// return page image, wait until ready
        LVRef<LVDrawBuf> getWithoutLock( int offset, int page )
        {
            for ( int i=0; i<2; i++ ) {
                if ( _items[i]._valid &&
                     ( (_items[i]._offset == offset && offset!=-1)
                      || (_items[i]._page==page && page!=-1)) ) {
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
        LVDocImageRef get( int offset, int page )
        {
            _mutex.lock();
            LVRef<LVDrawBuf> buf = getWithoutLock( offset, page );
            if ( !buf.isNull() )
                return LVDocImageRef( new LVDocImageHolder(getWithoutLock( offset, page ), _mutex) );
            return LVDocImageRef( NULL );
        }
        bool has( int offset, int page )
        {
            _mutex.lock();
            for ( int i=0; i<2; i++ ) {
                if ( _items[i]._valid && ( (_items[i]._offset == offset && offset!=-1)
                      || (_items[i]._page==page && page!=-1)) ) {
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
                _items[i]._thread.Clear();
                _items[i]._valid = false;
                _items[i]._drawbuf.Clear();
                _items[i]._offset = -1;
                _items[i]._page = -1;
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
#endif

class LVPageWordSelector {
    LVDocView * _docview;
    ldomWordExList _words;
    void updateSelection();
public:
    // selects middle word of current page
    LVPageWordSelector( LVDocView * docview );
    // clears selection
    ~LVPageWordSelector();
    // move current word selection in specified direction, (distance) times
    void moveBy( MoveDirection dir, int distance = 1 );
    // returns currently selected word
    ldomWordEx * getSelectedWord() { return _words.getSelWord(); }
    // access to words
    ldomWordExList & getWords() { return _words; }
    // append chars to search pattern
    ldomWordEx * appendPattern( lString32 chars );
    // remove last item from pattern
    ldomWordEx * reducePattern();
    // selects word of current page with specified coords;
    void selectWord(int x, int y);
};



/// document view mode: pages/scroll
enum LVDocViewMode
{
    DVM_SCROLL,
    DVM_PAGES
};

/// document scroll position info
class LVScrollInfo
{
public:
    int pos;
    int maxpos;
    int pagesize;
    int scale;
    lString32 posText;
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
    PGHDR_CHAPTER_MARKS=64,
    PGHDR_PERCENT=128
};


//typedef lUInt64 LVPosBookmark;

typedef LVArray<int> LVBookMarkPercentInfo;

#define DEF_COLOR_BUFFER_BPP 32

/**
    \brief XML document view

    Platform independant document view implementation.

    Supports scroll view of document.
*/
class LVDocView : public CacheLoadingCallback
{
    friend class LVDrawThread;
private:
    int m_bitsPerPixel;
    int m_dx;
    int m_dy;

    // current position
    int _pos;  // >=0 is correct vertical offset inside document, <0 - get based on m_page
    int _page; // >=0 is correct page number, <0 - get based on _pos
    bool _posIsSet;
    ldomXPointer _posBookmark; // bookmark for current position

    int m_battery_state;
    int m_battery_charging_conn;
    int m_battery_charge_level;
    int m_requested_font_size;
    int m_font_size; // = m_requested_font_size, possibly scaled according to DPI
    int m_status_font_size;
    int m_def_interline_space;
#if USE_LIMITED_FONT_SIZES_SET
    LVArray<int> m_font_sizes;
    bool m_font_sizes_cyclic;
#else
    int m_min_font_size;
    int m_max_font_size;
#endif
    bool m_is_rendered;

    LVDocViewMode m_view_mode; // DVM_SCROLL, DVM_PAGES
    inline bool isPageMode() const { return m_view_mode==DVM_PAGES; }
    inline bool isScrollMode() const { return m_view_mode==DVM_SCROLL; }

    /*
#if (COLOR_BACKBUFFER==1)
    LVColorDrawBuf m_drawbuf;
#else
    LVGrayDrawBuf  m_drawbuf;
#endif
    */
    lUInt32 m_backgroundColor;
    lUInt32 m_textColor;
    lUInt32 m_statusColor;
    font_ref_t     m_font;
    font_ref_t     m_infoFont;
    LVFontRef      m_batteryFont;
    LVContainerRef m_container;
    LVStreamRef    m_stream;
    LVContainerRef m_arc;
    ldomDocument * m_doc;
    lString8 m_stylesheet;
    LVRendPageList m_pages;
    LVScrollInfo m_scrollinfo;
    LVImageSourceRef m_defaultCover;
    LVImageSourceRef m_backgroundImage;
    LVRef<LVColorDrawBuf> m_backgroundImageScaled;
    bool m_backgroundTiled;
    bool m_stylesheetNeedsUpdate;
    int m_highlightBookmarks;
    LVPtrVector<LVBookMarkPercentInfo> m_bookmarksPercents;

protected:
    lString32 m_last_clock;

    ldomMarkedRangeList m_markRanges;
    ldomMarkedRangeList m_bmkRanges;

private:
    lString32 m_filename;
#define ORIGINAL_FILENAME_PATCH
#ifdef ORIGINAL_FILENAME_PATCH
    lString32 m_originalFilename;
#endif
    lvsize_t  m_filesize;


    lvRect m_pageMargins;
    lvRect m_pageRects[2];
    int    m_pagesVisible;
    int    m_pagesVisibleOverride;
    int m_pageHeaderPos;
    int m_pageHeaderInfo;
    bool m_showCover;
    LVRefVec<LVImageSource> m_headerIcons;
    LVRefVec<LVImageSource> m_batteryIcons;

#if CR_INTERNAL_PAGE_ORIENTATION==1
    cr_rotate_angle_t m_rotateAngle;
#endif
#ifdef ANDROID
    cr_rotate_angle_t m_rotateAngleInfo;
#endif

    CRFileHist m_hist;

    LVArray<int> m_section_bounds;
    bool m_section_bounds_valid;
    bool m_section_bounds_externally_updated;

    LVMutex _mutex;
#if CR_ENABLE_PAGE_IMAGE_CACHE==1
    LVDocViewImageCache m_imageCache;
#endif


    lString8 m_defaultFontFace;
	lString8 m_statusFontFace;
    ldomNavigationHistory _navigationHistory;

    doc_format_t m_doc_format;

    LVDocViewCallback * m_callback;

    // options
    CRPropRef m_props;
    // document properties
    CRPropRef m_doc_props;

    bool m_swapDone;

    /// edit cursor position
    ldomXPointer m_cursorPos;

    lString32 m_pageHeaderOverride;

    int m_drawBufferBits;

    CRPageSkinRef _pageSkin;

    /// sets current document format
    void setDocFormat( doc_format_t fmt );


    // private functions
    void updateScroll();
    /// makes table of contents for current document
    void makeToc();
    /// updates page layout
    void updateLayout();
    /// parse document from m_stream
    bool ParseDocument( );
    /// format of document from cache is known
    virtual void OnCacheFileFormatDetected( doc_format_t fmt );
    void insertBookmarkPercentInfo(int start_page, int end_y, int percent);

    void updateDocStyleSheet();

protected:
    /// returns document offset for next page
    int getNextPageOffset();
    /// returns document offset for previous page
    int getPrevPageOffset();
    /// selects link on page, if any (delta==0 - current, 1-next, -1-previous). returns selected link range, null if no links.
    virtual ldomXRange * selectPageLink( int delta, bool wrapAround);
    /// create document and set flags
    void createEmptyDocument();
    /// get document rectangle for specified cursor position, returns false if not visible
    bool getCursorDocRect( ldomXPointer ptr, lvRect & rc );
    /// load document from stream (internal)
    bool loadDocumentInt( LVStreamRef stream, bool metadataOnly = false );
    /// get section bounds for specific root node and specific section depth level, in 1/100 of percent
    void getSectionBoundsInt( LVArray<int>& bounds, ldomNode* node , lUInt16 section_id, int target_level, int level );
public:
    /// get outer (before margins are applied) page rectangle
    virtual void getPageRectangle( int pageIndex, lvRect & pageRect ) const;
    /// get screen rectangle for specified cursor position, returns false if not visible
    bool getCursorRect( ldomXPointer ptr, lvRect & rc, bool scrollToCursor = false );
    /// set status bar and clock mode
    void setStatusMode( int pos, bool showClock, bool showTitle, bool showBattery, bool showChapterMarks, bool showPercent, bool showPageNumber, bool showPageCount );
    /// draw to specified buffer by either Y pos or page number (unused param should be -1)
    void Draw( LVDrawBuf & drawbuf, int pageTopPosition, int pageNumber, bool rotate, bool autoresize = true);
    /// ensure current position is set to current bookmark value
    void checkPos();
    LVFontRef getBatteryFont() const { return m_batteryFont; }
    void setBatteryFont( LVFontRef font ) { m_batteryFont=font; }

    /// draw current page to specified buffer
    void Draw( LVDrawBuf & drawbuf, bool autoResize = true);
    
    /// close document
    void close();
    /// set buffer format
    void setDrawBufferBits( int bits ) { m_drawBufferBits = bits; }
    /// substitute page header with custom text (e.g. to be used while loading)
    void setPageHeaderOverride( lString32 s );
    /// get screen rectangle for current cursor position, returns false if not visible
    bool getCursorRect( lvRect & rc, bool scrollToCursor = false )
    {
        return getCursorRect( m_cursorPos, rc, scrollToCursor );
    }
    /// returns cursor position
    ldomXPointer getCursorPos() const { return m_cursorPos; }
    /// set cursor position
    void setCursorPos( ldomXPointer ptr ) { m_cursorPos = ptr; }
    /// try swappping of document to cache, if size is big enough, and no swapping attempt yet done
    void swapToCache();
    /// save document to cache file, with timeout option
    ContinuousOperationResult swapToCache(CRTimerUtil & maxTime);
    /// save unsaved data to cache file (if one is created), with timeout option
    ContinuousOperationResult updateCache(CRTimerUtil & maxTime);
    /// save unsaved data to cache file (if one is created), w/o timeout
    ContinuousOperationResult updateCache();

    /// returns selected (marked) ranges
    ldomMarkedRangeList * getMarkedRanges() { return &m_markRanges; }

    /// returns XPointer to middle paragraph of current page
    ldomXPointer getCurrentPageMiddleParagraph();
    /// render document, if not rendered
    void checkRender();
    /// saves current position to navigation history, to be able return back
    bool savePosToNavigationHistory();
    /// saves position to navigation history, to be able return back
    bool savePosToNavigationHistory(lString32 path);
    /// navigate to history path URL
    bool navigateTo( lString32 historyPath );
    /// packs current file path and name
    lString32 getNavigationPath() const;
    /// returns pointer to bookmark/last position containter of currently opened file
    CRFileHistRecord * getCurrentFileHistRecord();
	/// -1 moveto previous chapter, 0 to current chaoter first pae, 1 to next chapter
	bool moveByChapter( int delta );
	/// -1 moveto previous page, 1 to next page
	bool moveByPage( int delta );
	/// saves new bookmark
    CRBookmark * saveRangeBookmark( ldomXRange & range, bmk_type type, lString32 comment );
	/// export bookmarks to text file
	bool exportBookmarks( lString32 filename );
	/// saves current page bookmark under numbered shortcut
    CRBookmark * saveCurrentPageShortcutBookmark( int number );
    /// saves current page bookmark under numbered shortcut
    CRBookmark * saveCurrentPageBookmark( lString32 comment );
    /// removes bookmark from list, and deletes it, false if not found
    bool removeBookmark( CRBookmark * bm );
    /// sets new list of bookmarks, removes old values
    void setBookmarkList(LVPtrVector<CRBookmark> & bookmarks);
    /// restores page using bookmark by numbered shortcut
	bool goToPageShortcutBookmark( int number );
    /// find bookmark by window point, return NULL if point doesn't belong to any bookmark
    CRBookmark * findBookmarkByPoint(lvPoint pt);
    /// returns true if coverpage display is on
    bool getShowCover() const { return  m_showCover; }
    /// sets coverpage display flag
    void setShowCover( bool show ) { m_showCover = show; }
    /// returns true if page image is available (0=current, -1=prev, 1=next)
    bool isPageImageReady( int delta );

    // property support methods
    /// sets default property values if properties not found, checks ranges
    void propsUpdateDefaults( CRPropRef props );

    /// applies only one property by name, return false if property is unknown
    virtual bool propApply( lString8 name, lString32 value );
    /// applies properties, returns list of not recognized properties
    virtual CRPropRef propsApply( CRPropRef props );
    /// returns current values of supported properties
    CRPropRef propsGetCurrent();

    /// get current default cover image
    LVImageSourceRef getDefaultCover() const { return m_defaultCover; }
    /// set default cover image (for books w/o cover)
    void setDefaultCover(LVImageSourceRef cover) { m_defaultCover = cover; clearImageCache(); }

    /// get background image
    LVImageSourceRef getBackgroundImage() const { return m_backgroundImage; }
    /// set background image
    void setBackgroundImage(LVImageSourceRef bgImage, bool tiled=true) { m_backgroundImage = bgImage; m_backgroundTiled=tiled; m_backgroundImageScaled.Clear(); clearImageCache(); }
    /// clears page background
    virtual void drawPageBackground( LVDrawBuf & drawbuf, int offsetX, int offsetY, int alpha = 0);

    // callback functions
    /// set callback
    LVDocViewCallback * setCallback( LVDocViewCallback * callback ) { LVDocViewCallback * old = m_callback; m_callback = callback; return old; }
    /// get callback
    LVDocViewCallback * getCallback( ) { return m_callback; }

    // doc format functions
    /// set text format options
    void setTextFormatOptions( txt_format_t fmt );
    /// get text format options
    txt_format_t getTextFormatOptions() const;
    /// get current document format
    doc_format_t getDocFormat() const { return m_doc_format; }

    // Links and selections functions
    /// sets selection for whole element, clears previous selection
    virtual void selectElement( ldomNode * elem );
    /// sets selection for range, clears previous selection
    virtual void selectRange( const ldomXRange & range );
    /// sets selection for list of words, clears previous selection
    virtual void selectWords( const LVArray<ldomWord> & words );
    /// sets selections for ranges, clears previous selections
    virtual void selectRanges(ldomXRangeList & ranges);
    /// clears selection
    virtual void clearSelection();
    /// update selection -- command handler
    int onSelectionCommand( int cmd, int param );
    /// select the next sentence, for iterating through all
    bool nextSentence();


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
    virtual bool goLink( lString32 href, bool savePos=true );
    /// follow selected link, returns true if navigation was successful
    virtual bool goSelectedLink();
    /// go back. returns true if navigation was successful
    virtual bool goBack();
    /// go forward. returns true if navigation was successful
    virtual bool goForward();
    /// check if navigation forward is possible
    virtual bool canGoBack();
    /// check if navigation back is possible
    virtual bool canGoForward();


    /// create empty document with specified message (e.g. to show errors)
    virtual void createDefaultDocument( lString32 title, lString32 message );
    /// create empty document with specified message (to show errors)
    virtual void createHtmlDocument(lString32 code);

    /// returns default font face
    lString8 getDefaultFontFace() const { return m_defaultFontFace; }
    /// set default font face
    void setDefaultFontFace( const lString8 & newFace );
    /// returns status bar font face
    lString8 getStatusFontFace() const { return m_statusFontFace; }
    /// set status bar font face
    void setStatusFontFace( const lString8 & newFace );
    /// invalidate formatted data, request render
    void requestRender();
    /// invalidate document data, request reload
    void requestReload();
    /// invalidate image cache, request redraw
    void clearImageCache();
#if CR_ENABLE_PAGE_IMAGE_CACHE==1
    /// get page image (0=current, -1=prev, 1=next)
    LVDocImageRef getPageImage( int delta );
    /// returns true if current page image is ready
    bool IsDrawed();
    /// cache page image (render in background if necessary) (0=current, -1=prev, 1=next)
    void cachePageImage( int delta );
#endif
    /// return view mutex
    LVMutex & getMutex() { return _mutex; }
    /// update selection ranges
    void updateSelections();
    void updateBookMarksRanges();
    /// get page document range, -1 for current page
    LVRef<ldomXRange> getPageDocumentRange( int pageIndex=-1 );
    /// get page text, -1 for current page
    lString32 getPageText( bool wrapWords, int pageIndex=-1 );
    /// returns number of non-space characters on current page
    int getCurrentPageCharCount();
    /// returns number of images on current page
    int getCurrentPageImageCount();
    /// returns number of images on given page
    int getPageImageCount(LVRef<ldomXRange>& range);
    /// calculate page header rectangle
    virtual void getPageHeaderRectangle( int pageIndex, lvRect & headerRc ) const;
    /// calculate page header height
    virtual int getPageHeaderHeight() const;
    /// set list of icons to display at left side of header
    void setHeaderIcons( LVRefVec<LVImageSource> icons );
    /// set list of battery icons to display battery state
    void setBatteryIcons( const LVRefVec<LVImageSource> & icons );
    /// sets page margins
    void setPageMargins(lvRect rc);
    /// update page margins based on current settings
    void updatePageMargins();
    /// returns page margins
    lvRect getPageMargins() const { return m_pageMargins; }
#if CR_INTERNAL_PAGE_ORIENTATION==1
    /// sets rotate angle
    void SetRotateAngle( cr_rotate_angle_t angle );
    /// Select appropriate AA LCD subpixel rendering mode for chosen rotate angle
    font_antialiasing_t rotateFontAntialiasMode(font_antialiasing_t aa_mode, cr_rotate_angle_t angle);
#endif
    /// rotate rectangle by current angle, winToDoc==false for doc->window translation, true==ccw
    lvRect rotateRect( lvRect & rc, bool winToDoc ) const;
    /// rotate point by current angle, winToDoc==false for doc->window translation, true==ccw
    lvPoint rotatePoint( lvPoint & pt, bool winToDoc ) const;
#if CR_INTERNAL_PAGE_ORIENTATION==1
    /// returns rotate angle
    cr_rotate_angle_t GetRotateAngle() const { return m_rotateAngle; }
#endif
    /// returns true if document is opened
    bool isDocumentOpened();
    /// returns section bounds, in 1/100 of percent
    LVArray<int> & getSectionBounds( int max_count, int depth, bool for_external_update=false );
    /// sets battery state
    virtual bool setBatteryState( int newState, int newChargingConn, int newChargeLevel );
    /// returns battery state
    int getBatteryState() const { return m_battery_state; }
    /// returns battery charging connection
    int getBatteryChargingConn() const { return m_battery_charging_conn; }
    /// returns battery charge level
    int getBatteryChargeLevel() const { return m_battery_charge_level; }
    /// returns current time representation string
    virtual lString32 getTimeString() const;
    /// returns true if time changed since clock has been last drawed
    bool isTimeChanged();
    /// returns if Render has been called
    bool IsRendered() const { return m_is_rendered; }
    /// returns file list with positions/bookmarks
    CRFileHist * getHistory() { return &m_hist; }
    /// returns formatted page list
    LVRendPageList * getPageList() { return &m_pages; }
    /// returns pointer to TOC root node
    LVTocItem * getToc();
    /// returns pointer to TOC root node
    bool getFlatToc( LVPtrVector<LVTocItem, false> & items );
    /// update page numbers for items
    void updatePageNumbers( LVTocItem * item );
    /// returns pointer to LVPageMapItems container
    LVPageMap * getPageMap();
    /// update PageMap items page infos
    void updatePageMapInfo( LVPageMap * pagemap );

    /// set view mode (pages/scroll) - DVM_SCROLL/DVM_PAGES
    void setViewMode( LVDocViewMode view_mode, int visiblePageCount=-1 );
    /// get view mode (pages/scroll)
    LVDocViewMode getViewMode() const;
    /// toggle pages/scroll view mode
    void toggleViewMode();
    /// returns current pages visible setting value (independent on window and font size)
    int getPagesVisibleSetting() const;
    /// get window visible page count (1 or 2)
    int getVisiblePageCount() const;
    /// set window visible page count (1 or 2)
    void setVisiblePageCount( int n );
    /// set window visible page count, to use exact value independent of font size and window sides
    void overrideVisiblePageCount(int n);

    /// get page header position
    int getPageheaderPosition() const { return m_pageHeaderPos; }
    /// set page header position
    void setPageHeaderPosition( int pos );
    /// get page header info mask
    int getPageHeaderInfo() const { return m_pageHeaderInfo; }
    /// set page header info mask
    void setPageHeaderInfo( int hdrFlags );
    /// get info line font
    font_ref_t getInfoFont() const { return m_infoFont; }
    /// set info line font
    void setInfoFont( font_ref_t font ) { m_infoFont = font; }
    /// draw page header to buffer
    virtual void drawPageHeader( LVDrawBuf * drawBuf, const lvRect & headerRc, int pageIndex, int headerInfoFlags, int pageCount );
    /// draw battery state to buffer
    virtual void drawBatteryState(LVDrawBuf * drawBuf, const lvRect & rc);

    /// returns background color
    lUInt32 getBackgroundColor() const { return m_backgroundColor; }
    /// sets background color
    void setBackgroundColor( lUInt32 cl )
    {
        m_backgroundColor = cl;
        clearImageCache();
    }
    /// returns text color
    lUInt32 getTextColor() const { return m_textColor; }
    /// sets text color
    void setTextColor( lUInt32 cl )
    {
        m_textColor = cl;
        m_props->setColor(PROP_FONT_COLOR, cl);
        clearImageCache();
    }

    /// returns text color
    lUInt32 getStatusColor() const { return m_statusColor; }
    /// sets text color
    void setStatusColor( lUInt32 cl )
    {
        m_statusColor = cl;
        clearImageCache();
    }

    CRPageSkinRef getPageSkin();
    void setPageSkin( CRPageSkinRef skin );

    /// returns xpointer for specified window point
    ldomXPointer getNodeByPoint( lvPoint pt, bool strictBounds=false );
    /// returns image source for specified window point, if point is inside image
    LVImageSourceRef getImageByPoint(lvPoint pt);
    /// draws scaled image into buffer, clear background according to current settings
    bool drawImage(LVDrawBuf * buf, LVImageSourceRef img, int x, int y, int dx, int dy);
    /// converts point from window to document coordinates, returns true if success
    bool windowToDocPoint( lvPoint & pt );
    /// converts point from document to window coordinates, returns true if success
    bool docToWindowPoint( lvPoint & pt, bool isRectBottom=false, bool fitToPage=false );

    /// returns document
    ldomDocument * getDocument() const {
    	if (NULL == m_doc)
    	    CRLog::error("attempt to return NULL pointer as document!");
    	return m_doc;
    }
    /// return document properties
    CRPropRef getDocProps() { return m_doc_props; }
    /// returns book title
    lString32 getTitle() const { return m_doc_props->getStringDef(DOC_PROP_TITLE); }
    /// returns book language
    lString32 getLanguage() const { return m_doc_props->getStringDef(DOC_PROP_LANGUAGE); }
    /// returns book author(s)
    lString32 getAuthors() const { return m_doc_props->getStringDef(DOC_PROP_AUTHORS); }
    /// returns book description
    lString32 getDescription() const { return m_doc_props->getStringDef(DOC_PROP_DESCRIPTION); }
    /// returns book keywords (separated by "; ")
    lString32 getKeywords() const { return m_doc_props->getStringDef(DOC_PROP_KEYWORDS); }
    /// returns book series name and number (series name #1)
    lString32 getSeries() const
    {
        lString32 name = m_doc_props->getStringDef(DOC_PROP_SERIES_NAME);
        lString32 number = m_doc_props->getStringDef(DOC_PROP_SERIES_NUMBER);
        if ( !name.empty() && !number.empty() )
            name << " #" << number;
        return name;
    }
    /// returns book series name and number (series name #1)
    lString32 getSeriesName() const
    {
        lString32 name = m_doc_props->getStringDef(DOC_PROP_SERIES_NAME);
        return name;
    }
    /// returns book series name and number (series name #1)
    int getSeriesNumber() const
    {
        lString32 name = m_doc_props->getStringDef(DOC_PROP_SERIES_NAME);
        lString32 number = m_doc_props->getStringDef(DOC_PROP_SERIES_NUMBER);
        if (!name.empty() && !number.empty())
            return number.atoi();
        return 0;
    }
    /// returns book content CRC32
    lUInt32 getFileCRC32() const {
        return (lUInt32)m_doc_props->getIntDef(DOC_PROP_FILE_CRC32, 0);
    }

    /// export to WOL format
    bool exportWolFile( const char * fname, bool flgGray, int levels );
    /// export to WOL format
    bool exportWolFile( const lChar32 * fname, bool flgGray, int levels );
    /// export to WOL format
    bool exportWolFile( LVStream * stream, bool flgGray, int levels );

    /// get a stream for reading to document internal file (path inside the ZIP for EPUBs,
    /// path relative to document directory for non-container documents like HTML)
    LVStreamRef getDocumentFileStream( lString32 filePath );

    /// draws page to image buffer
    void drawPageTo( LVDrawBuf * drawBuf, LVRendPageInfo & page, lvRect * pageRect, int pageCount, int basePage);
    /// draws coverpage to image buffer
    void drawCoverTo( LVDrawBuf * drawBuf, lvRect & rc );
    /// returns cover page image source, if any
    LVImageSourceRef getCoverPageImage();
    /// returns cover page image stream, if any
    LVStreamRef getCoverPageImageStream();

    /// returns bookmark
    ldomXPointer getBookmark( bool precise = true );
    /// returns bookmark for specified page
    ldomXPointer getPageBookmark( int page );
    /// sets current bookmark
    void setBookmark( ldomXPointer bm );
    /// moves position to bookmark
    void goToBookmark( ldomXPointer bm );
    /// get page number by bookmark
    int getBookmarkPage(ldomXPointer bm);
    /// get bookmark position text
    bool getBookmarkPosText( ldomXPointer bm, lString32 & titleText, lString32 & posText );

    /// returns scrollbar control info
    const LVScrollInfo * getScrollInfo() { updateScroll(); return &m_scrollinfo; }
    /// move to position specified by scrollbar
    bool goToScrollPos( int pos );
    /// converts scrollbar pos to doc pos
    int scrollPosToDocPos( int scrollpos );
    /// returns position in 1/100 of percents (0..10000)
    int getPosPercent();
    /// returns position in 1/100 of percents (0..10000)
    int getPosEndPagePercent();

    /// execute command
    int doCommand( LVDocCmd cmd, int param=0 );

    /// set document stylesheet text
    void setStyleSheet( lString8 css_text );

    /// set default interline space, percent (100..200)
    void setDefaultInterlineSpace( int percent );

    /// change font size, if rollCyclic is true, largest font is followed by smallest and vice versa
    void ZoomFont( int delta );
    /// retrieves current base font size
    int  getFontSize() const { return m_font_size; }
    /// retrieves requested font size (before scaling for DPI)
    int  getRequestedFontSize() const { return m_requested_font_size; }
    /// scale font size according to gRenderDPI
    int scaleFontSizeForDPI( int fontSize );
    /// sets new base font size
    void setFontSize( int newSize );
    /// retrieves current status bar font size
    int  getStatusFontSize() const { return m_status_font_size; }
    /// sets new status bar font size
    void setStatusFontSize( int newSize );

#if USE_LIMITED_FONT_SIZES_SET
    /// sets posible base font sizes (for ZoomFont)
    void setFontSizes( LVArray<int> & sizes, bool cyclic );
#else
    void setMinFontSize( int size );
    void setMaxFontSize( int size );
#endif

    /// get drawing buffer
    //LVDrawBuf * GetDrawBuf() { return &m_drawbuf; }
    /// draw document into buffer
    //void Draw();

    /// resize view
    void Resize( int dx, int dy );
    /// get view height
    int GetHeight() const;
    /// get view width
    int GetWidth() const;

    /// get full document height
    int GetFullHeight();

    /// get vertical position of view inside document
    int GetPos();
    /// get position of view inside document
    void GetPos( lvRect & rc );
    /// set vertical position of view inside document
    int SetPos( int pos, bool savePos=true, bool allowScrollAfterEnd = false);

    // get page start y (in full document height)
    int getPageStartY(int pageIndex) const;
    // get page height
    int getPageHeight(int pageIndex) const;

    /// get number of current page
    int getCurPage();
    /// move to specified page
    bool goToPage(int page, bool updatePosBookmark = true, bool regulateTwoPages = true);
    /// returns page count
    int getPageCount() const;
    /// get the flow the specified page belongs to
    int getPageFlow(int pageIndex);
    /// returns whether there are any flows besides the linear flow 0
    bool hasNonLinearFlows();

    /// clear view
    void Clear();
    /// load document from file
    bool LoadDocument( const char * fname, bool metadataOnly = false );
    /// load document from file
    bool LoadDocument( const lChar32 * fname, bool metadataOnly = false );
    /// load document from stream
    bool LoadDocument( LVStreamRef stream, const lChar32 * contentPath, bool metadataOnly = false );

    /// load document and export sentence info
    bool exportSentenceInfo(const lChar32 * inputFileName, const lChar32 * outputFileName);

    /// save last file position
    void savePosition();
    /// restore last file position
    void restorePosition();

#ifdef ORIGINAL_FILENAME_PATCH
    void setOriginalFilename( const lString32 & fn ) {
        m_originalFilename = fn;
    }
    const lString32 & getOriginalFilename() const {
        return m_originalFilename;
    }
    void setMinFileSizeToCache( int size ) {
        m_props->setInt(PROP_MIN_FILE_SIZE_TO_CACHE, size);
    }
#endif

    /// render (format) document
    void Render( int dx=0, int dy=0, LVRendPageList * pages=NULL );
    /// set properties before rendering
    void setRenderProps( int dx, int dy );
    /// Return a hash accounting for the rendering and the pages layout
    /// A changed hash let frontends know their cached values of some document
    /// properties (full height, TOC pages...) may have changed and that they
    /// need to fetch them again
    lUInt32 getDocumentRenderingHash() const;

    /// Constructor
    LVDocView( int bitsPerPixel=-1, bool noDefaultDocument=false );
    /// Destructor
    virtual ~LVDocView();
};

class SimpleTitleFormatter {
    lString32 _text;
    lString32Collection _lines;
    lString8 _fontFace;
    bool _bold;
    bool _italic;
    lUInt32 _color;
    LVFontRef _font;
    int _lineHeight;
    int _height;
    int _width;
    int _maxWidth;
    int _maxHeight;
    int _fntSize;
public:
    int getHeight() { return _height; }
    int getWidth() { return _width; }
    SimpleTitleFormatter(lString32 text, lString8 fontFace, bool bold, bool italic, lUInt32 color, int maxWidth, int maxHeight, int fntSize = 0);

    bool measure();
    bool splitLines(const char * delimiter);
    bool format(int fontSize);
    bool findBestSize();
    void draw(LVDrawBuf & buf, lString32 str, int x, int y, int align);
    void draw(LVDrawBuf & buf, lvRect rc, int halign, int valign);
};


/// draw book cover, either from image, or generated from title/authors
void LVDrawBookCover(LVDrawBuf & buf, LVImageSourceRef image, bool respectAspectRatio, lString8 fontFace, lString32 title, lString32 authors, lString32 seriesName, int seriesNumber);

#endif
