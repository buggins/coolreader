#include <wx/wx.h>
#include <wx/power.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <crengine.h>
#include "cr3.h"
#include "rescont.h"
#include "view.h"
#include "optdlg.h"

#define RENDER_TIMER_ID   123
#define CLOCK_TIMER_ID    124
#define CURSOR_TIMER_ID		125

BEGIN_EVENT_TABLE( cr3view, wxPanel )
    EVT_PAINT( cr3view::OnPaint )
    EVT_SIZE    ( cr3view::OnSize )
    EVT_MOUSEWHEEL( cr3view::OnMouseWheel )
    EVT_LEFT_DOWN( cr3view::OnMouseLDown )
    EVT_RIGHT_DOWN( cr3view::OnMouseRDown )
    EVT_MOTION( cr3view::OnMouseMotion )
    EVT_MENU_RANGE( 0, 0xFFFF, cr3view::OnCommand )
    EVT_SET_FOCUS( cr3view::OnSetFocus )
    EVT_TIMER(RENDER_TIMER_ID, cr3view::OnTimer)
    EVT_TIMER(CLOCK_TIMER_ID, cr3view::OnTimer)
    EVT_TIMER(CURSOR_TIMER_ID, cr3view::OnTimer)
    EVT_INIT_DIALOG(cr3view::OnInitDialog)
END_EVENT_TABLE()

wxColour cr3view::getBackgroundColour()
{
#if (COLOR_BACKBUFFER==1)
    lUInt32 cl = getDocView()->getBackgroundColor();
#else
    lUInt32 cl = 0xFFFFFF;
#endif
    wxColour wxcl( (cl>>16)&255, (cl>>8)&255, (cl>>0)&255 );
    return wxcl;
}

int propsToPageHeaderFlags( CRPropRef props )
{
    int flags = 0;
    if ( props->getBoolDef( PROP_PAGE_HEADER_ENABLED, true ) ) {
        if ( props->getBoolDef( PROP_PAGE_HEADER_PAGE_NUMBER, true ) )
            flags |= PGHDR_PAGE_NUMBER;
        if ( props->getBoolDef( PROP_PAGE_HEADER_PAGE_COUNT, true ) )
            flags |= PGHDR_PAGE_COUNT;
        if ( props->getBoolDef( PROP_PAGE_HEADER_CLOCK, true ) )
            flags |= PGHDR_CLOCK;
        if ( props->getBoolDef( PROP_PAGE_HEADER_BATTERY, true ) )
            flags |= PGHDR_BATTERY;
        if ( props->getBoolDef( PROP_PAGE_HEADER_AUTHOR, true ) )
            flags |= PGHDR_AUTHOR;
        if ( props->getBoolDef( PROP_PAGE_HEADER_TITLE, true ) )
            flags |= PGHDR_TITLE;
    }
    return flags;
}


void cr3view::OnInitDialog(wxInitDialogEvent& event)
{
    //SetBackgroundColour( getBackgroundColour() );
	_isFullscreen = _props->getBoolDef(PROP_WINDOW_FULLSCREEN);
}

lString16 cr3view::GetLastRecentFileName()
{
    if ( getDocView() && getDocView()->getHistory()->getRecords().length()>0 )
        return getDocView()->getHistory()->getRecords()[0]->getFilePathName();
    return lString16::empty_str;
}

cr3view::cr3view(CRPropRef props, lString16 exeDirPath )
: _normalCursor(wxCURSOR_ARROW)
, _linkCursor(wxCURSOR_HAND)
, _scrollbar(NULL)
, _firstRender(false)
, _allowRender(true)
, _props(props)
, _screen(300,400)
, _wm(&_screen)
{
    _wm.activateWindow( (_docwin = new CRDocViewWindow(&_wm)) );
    getDocView()->setCallback( this );
    IMAGE_SOURCE_FROM_BYTES(defCover, cr3_def_cover_gif);
    LVImageSourceRef cover = LVCreateFileCopyImageSource( exeDirPath + "cr3_def_cover.png" );
    if ( !cover.isNull() )
        getDocView()->setDefaultCover( cover );
    else
        getDocView()->setDefaultCover( defCover );
    getDocView()->setPageMargins( lvRect(14, 5, 14, 5) );

    static int fontSizes[] = {14, 16, 18, 20, 24, 28, 32, 36};
    LVArray<int> sizes( fontSizes, sizeof(fontSizes)/sizeof(int) );
    getDocView()->setFontSizes( sizes, false );
    //_docview->setBackgroundColor(0x000000);
    //_docview->setTextColor(0xFFFFFF);

    cr_rotate_angle_t angle = (cr_rotate_angle_t)(_props->getIntDef( PROP_WINDOW_ROTATE_ANGLE, 0 ) & 3);
    getDocView()->SetRotateAngle( angle );

    {
        LVStreamRef stream = LVOpenFileStream( GetHistoryFileName().c_str(), LVOM_READ );
        if ( !stream.isNull() ) {
            getDocView()->getHistory()->loadFromStream( stream );
            stream = NULL;
        }
    }



    _renderTimer = new wxTimer( this, RENDER_TIMER_ID );
    _clockTimer = new wxTimer( this, CLOCK_TIMER_ID );
    _cursorTimer = new wxTimer( this, CURSOR_TIMER_ID );

    //SetBackgroundColour( getBackgroundColour() );
    InitDialog();
    //int width, height;
    //GetClientSize( &width, &height );
	//Resize( 300, 300 );	

#if 0
    //TEST ICONS
    LVRefVec<LVImageSource> icons;
    static const char * icon1[] = {
        "8 8 3 1",
        "* c #000000",
        ". c #FFFFFF",
        "  c None",
        " ****** ",
        "*......*",
        "*.*..*.*",
        "*......*",
        "*. ** .*",
        "*......*",
        " **..** ",
        "   **   ",
    };
    static const char * icon2[] = {
        "8 8 3 1",
        "* c #00C000",
        ". c #FF80FF",
        "  c None",
        " ****** ",
        "*..**..*",
        "*.*..*.*",
        "*.*..*.*",
        "*. ** .*",
        "**.  .**",
        " **..** ",
        "   **   ",
    };
    static const char * battery4[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #AAAAAA",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0000.000.000.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery3[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #AAAAAA",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0000.ooo.000.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery2[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #AAAAAA",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0000.ooo.ooo.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery1[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #AAAAAA",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0000.ooo.ooo.ooo.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery0[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #AAAAAA",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0000.ooo.ooo.ooo.ooo.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    icons.add( LVCreateXPMImageSource( icon1 ) );
    icons.add( LVCreateXPMImageSource( icon2 ) );
    _docview->setHeaderIcons( icons );
    icons.clear();
    icons.add( LVCreateXPMImageSource( battery0 ) );
    icons.add( LVCreateXPMImageSource( battery1 ) );
    icons.add( LVCreateXPMImageSource( battery2 ) );
    icons.add( LVCreateXPMImageSource( battery3 ) );
    icons.add( LVCreateXPMImageSource( battery4 ) );
    _docview->setBatteryIcons( icons );
#endif
}

cr3view::~cr3view()
{
    delete _renderTimer;
    delete _clockTimer;
    delete _cursorTimer;
}

void cr3view::OnTimer(wxTimerEvent& event)
{
    //printf("cr3view::OnTimer() \n");
    if ( event.GetId() == RENDER_TIMER_ID ) {
        int dx;
        int dy;
        GetClientSize( &dx, &dy );
        //if ( _docview->IsRendered() && dx == _docview->GetWidth()
        //        && dy == _docview->GetHeight() )
        //    return; // no resize
        if (dx<50 || dy<50 || dx>3000 || dy>3000)
        {
            return;
        }

        if ( _firstRender ) {
            getDocView()->restorePosition();
            _firstRender = false;
            _allowRender = true;
        }

        _wm.reconfigure( dx, dy, CR_ROTATE_ANGLE_0 );

        Paint();
        UpdateScrollBar();
    } else if ( event.GetId() == CURSOR_TIMER_ID ) {
        SetCursor( wxCursor( wxCURSOR_BLANK ) );
    } else if ( event.GetId() == CLOCK_TIMER_ID ) {
        if ( IsShownOnScreen() ) {
            if ( getDocView()->IsRendered() && getDocView()->isTimeChanged() )
                Paint();
        }
    }
}

void cr3view::Paint()
{
    //printf("cr3view::Paint() \n");
    int battery_state = -1;
#ifdef _WIN32
    SYSTEM_POWER_STATUS bstatus;
    BOOL pow = GetSystemPowerStatus(&bstatus);
    if (bstatus.BatteryFlag & 128)
        pow = FALSE;
    if (bstatus.ACLineStatus!=0 || bstatus.BatteryLifePercent==255)
        pow = FALSE;
    if ( pow )
        battery_state = bstatus.BatteryLifePercent;
#else
    if ( ::wxGetPowerType() == wxPOWER_BATTERY ) {
        int n = ::wxGetBatteryState();
        if ( n == wxBATTERY_NORMAL_STATE )
            battery_state = 100;
        else if ( n == wxBATTERY_LOW_STATE )
            battery_state = 50;
        else if ( n == wxBATTERY_CRITICAL_STATE )
            battery_state = 0;
        else if ( n == wxBATTERY_SHUTDOWN_STATE )
            battery_state = 0;
    };
#endif
    getDocView()->setBatteryState( battery_state );
    //_docview->Draw();
    UpdateScrollBar();
    Refresh( FALSE );
}

static lChar16 detectSlash( lString16 path )
{
    for ( unsigned i=0; i<path.length(); i++ )
        if ( path[i]=='\\' || path[i]=='/' )
            return path[i];
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

lString16 cr3view::GetHistoryFileName()
{
    lString16 cfgdir( wxStandardPaths::Get().GetUserDataDir().c_str() );
    if ( !wxDirExists( cfgdir.c_str() ) )
        ::wxMkdir( wxString( cfgdir.c_str() ) );
    lChar16 slash = detectSlash( cfgdir );
    cfgdir << slash;
    return cfgdir + "cr3hist.bmk";
}

void cr3view::CloseDocument()
{
    //printf("cr3view::CloseDocument()  \n");
    getDocView()->savePosition();
    getDocView()->Clear();
    LVStreamRef stream = LVOpenFileStream( GetHistoryFileName().c_str(), LVOM_WRITE );
    if ( !stream.isNull() )
        getDocView()->getHistory()->saveToStream( stream.get() );
}

void cr3view::UpdateScrollBar()
{
	if ( !_scrollbar )
		return;
    if ( !getDocView()->IsRendered() )
        return;
    const LVScrollInfo * lvsi = getDocView()->getScrollInfo();
    _scrollbar->SetScrollbar(
        lvsi->pos,      //int position, 
        lvsi->pagesize, //int thumbSize, 
        lvsi->maxpos + lvsi->pagesize,   //int range, 
        lvsi->pagesize, //int pageSize, 
        true//const bool refresh = true
    );
    wxStatusBar * sb = ((wxFrame*)GetParent())->GetStatusBar();
    if ( sb )
        sb->SetStatusText( wxString( lvsi->posText.c_str() ), 1 );

}

void cr3view::OnMouseMotion(wxMouseEvent& event)
{
    int x = event.GetX();
    int y = event.GetY();
    ldomXPointer ptr = getDocView()->getNodeByPoint( lvPoint( x, y ) );
    if ( ptr.isNull() ) {
        return;
    }
    lString16 href = ptr.getHRef();
    if ( href.empty() ) {
        SetCursor(_normalCursor);
    } else {
        SetCursor(_linkCursor);
    }

    if ( _isFullscreen ) {
        _cursorTimer->Stop();
        _cursorTimer->Start( 3 * 1000, wxTIMER_ONE_SHOT );
    }

    SetCursor( wxNullCursor );
}

void cr3view::OnMouseLDown( wxMouseEvent & event )
{
    int x = event.GetX();
    int y = event.GetY();
    //lString16 txt = _docview->getPageText( true );
    //CRLog::debug( "getPageText : %s", UnicodeToUtf8(txt).c_str() );
    ldomXPointer ptr = getDocView()->getNodeByPoint( lvPoint( x, y ) );
    if ( ptr.isNull() ) {
        CRLog::debug( "cr3view::OnMouseLDown() : node not found!\n");
        return;
    }
    lString16 href = ptr.getHRef();
    if ( ptr.getNode()->isText() ) {
        lString8 s = UnicodeToUtf8( ptr.toString() );
        CRLog::debug("Text node clicked (%d, %d): %s", x, y, s.c_str() );
        ldomXRange * wordRange = new ldomXRange();
        if ( ldomXRange::getWordRange( *wordRange, ptr ) ) {
            wordRange->setFlags( 0x10000 );
            getDocView()->getDocument()->getSelections().clear();
            getDocView()->getDocument()->getSelections().add( wordRange );
            getDocView()->updateSelections();
        } else {
            delete wordRange;
        }
        if ( !href.empty() ) {
            getDocView()->goLink( href );
        }
        Paint();
        printf("text : %s     \t", s.c_str() );
    } else {
        printf("element : %s  \t", UnicodeToUtf8( ptr.toString() ).c_str() );
    }
    lvPoint pt2 = ptr.toPoint();
    printf("  (%d, %d)  ->  (%d, %d)\n", x, y+getDocView()->GetPos(), pt2.x, pt2.y);
}

void cr3view::OnMouseRDown( wxMouseEvent & event )
{
    wxMenu pm;
    pm.Append( wxID_OPEN, wxT( "&Open...\tCtrl+O" ) );
    pm.Append( Menu_View_History, wxT( "Recent books list\tF4" ) );
    pm.Append( wxID_SAVE, wxT( "&Save...\tCtrl+S" ) );
    pm.AppendSeparator();
    pm.Append( Menu_File_Options, wxT( "Options...\tF9" ) );
    pm.AppendSeparator();
    pm.Append( Menu_View_TOC, wxT( "Table of Contents\tF5" ) );
    pm.Append( Menu_File_About, wxT( "&About...\tF1" ) );
    pm.AppendSeparator();
    pm.Append( Menu_View_ZoomIn, wxT( "Zoom In" ) );
    pm.Append( Menu_View_ZoomOut, wxT( "Zoom Out" ) );
    pm.AppendSeparator();
    pm.Append( Menu_View_ToggleFullScreen, wxT( "Toggle Fullscreen\tAlt+Enter" ) );
    pm.Append( Menu_View_TogglePages, wxT( "Toggle Pages/Scroll\tCtrl+P" ) );
    pm.Append( Menu_View_TogglePageHeader, wxT( "Toggle page heading\tCtrl+H" ) );
    pm.AppendSeparator();
    pm.Append( Menu_File_Quit, wxT( "E&xit\tAlt+X" ) );

    ((wxFrame*)GetParent())->PopupMenu(&pm);
}

void cr3view::SetPageHeaderFlags()
{
    int newflags = propsToPageHeaderFlags( _props );
    int oldflags = getDocView()->getPageHeaderInfo();
    if ( oldflags==newflags )
        return;
    getDocView()->setPageHeaderInfo( newflags );
    UpdateScrollBar();
    Paint();
}

void cr3view::ToggleViewMode()
{
    int mode = _props->getIntDef( PROP_PAGE_VIEW_MODE, 2 ) ? 0 : 2;
    _props->setInt( PROP_PAGE_VIEW_MODE, mode );
    getDocView()->setViewMode( mode>0 ? DVM_PAGES : DVM_SCROLL, mode>0 ? mode : -1 );
    UpdateScrollBar();
    Paint();
}

void cr3view::OnCommand(wxCommandEvent& event)
{
	switch ( event.GetId() ) {
	case Menu_View_ZoomIn:
        {
	        wxCursor hg( wxCURSOR_WAIT );
	        this->SetCursor( hg );
	        wxSetCursor( hg );
            //===========================================
            doCommand( DCMD_ZOOM_IN, 0 );
            //===========================================
	        this->SetCursor( wxNullCursor );
	        wxSetCursor( wxNullCursor );
        }
		break;
	case Menu_View_ZoomOut:
        {
	        wxCursor hg( wxCURSOR_WAIT );
	        this->SetCursor( hg );
	        wxSetCursor( hg );
            //===========================================
    	    doCommand( DCMD_ZOOM_OUT, 0 );
            //===========================================
	        this->SetCursor( wxNullCursor );
	        wxSetCursor( wxNullCursor );
        }
		break;
	case Menu_View_NextPage:
	    doCommand( DCMD_PAGEDOWN, 1 );
        getDocView()->cachePageImage( 0 );
        getDocView()->cachePageImage( 1 );
		break;
    case Menu_Link_Forward:
		doCommand( DCMD_LINK_FORWARD, 1 );
        break;
    case Menu_Link_Back:
		doCommand( DCMD_LINK_BACK, 1 );
        break;
    case Menu_Link_Next:
		doCommand( DCMD_LINK_NEXT, 1 );
        break;
    case Menu_Link_Prev:
		doCommand( DCMD_LINK_PREV, 1 );
        break;
    case Menu_Link_Go:
		doCommand( DCMD_LINK_GO, 1 );
        break;
	case Menu_View_PrevPage:
		doCommand( DCMD_PAGEUP, 1 );
        getDocView()->cachePageImage( 0 );
        getDocView()->cachePageImage( -1 );
        break;
	case Menu_View_NextLine:
	    doCommand( DCMD_LINEDOWN, 1 );
		break;
	case Menu_View_PrevLine:
		doCommand( DCMD_LINEUP, 1 );
		break;
	case Menu_View_Begin:
	    doCommand( DCMD_BEGIN, 0 );
		break;
	case Menu_View_End:
		doCommand( DCMD_END, 0 );
		break;
    case Menu_View_TogglePages:
        ToggleViewMode();
        break;
    case Menu_View_TogglePageHeader:
        _props->setBool( PROP_PAGE_HEADER_ENABLED, !_props->getBoolDef(PROP_PAGE_HEADER_ENABLED, true) );
        SetPageHeaderFlags();
        break;
    case Menu_View_Text_Format:
		doCommand( DCMD_TOGGLE_TEXT_FORMAT, 0 );
        break;
	}
}

void cr3view::OnScroll(wxScrollEvent& event)
{
    int id = event.GetEventType();
    //printf("Scroll event: %d\n", id);
    if (id == wxEVT_SCROLL_TOP)
        doCommand( DCMD_BEGIN, 0 );
    else if (id == wxEVT_SCROLL_BOTTOM )
        doCommand( DCMD_BEGIN, 0 );
    else if (id == wxEVT_SCROLL_LINEUP )
        doCommand( DCMD_LINEUP, 0 );
    else if (id == wxEVT_SCROLL_LINEDOWN )
        doCommand( DCMD_LINEDOWN, 0 );
    else if (id == wxEVT_SCROLL_PAGEUP )
        doCommand( DCMD_PAGEUP, 0 );
    else if (id == wxEVT_SCROLL_PAGEDOWN )
        doCommand( DCMD_PAGEDOWN, 0 );
    else if (id == wxEVT_SCROLL_THUMBRELEASE || id == wxEVT_SCROLL_THUMBTRACK)
    {
        doCommand( DCMD_GO_POS,
                   getDocView()->scrollPosToDocPos( event.GetPosition() ) );
    }
}

void cr3view::OnMouseWheel(wxMouseEvent& event)
{
    int rotation = event.GetWheelRotation();
    if ( rotation>0 )
        doCommand( DCMD_LINEUP, 3 );
    else if ( rotation<0 )
        doCommand( DCMD_LINEDOWN, 3 );
}

void cr3view::OnKeyDown(wxKeyEvent& event)
{
    int code = event.GetKeyCode() ;
    
        switch( code )
        {
        case WXK_NUMPAD_ADD:
            {
        doCommand( DCMD_ZOOM_IN, 0 );
            }
            break;
        case WXK_NUMPAD_SUBTRACT:
            {
        doCommand( DCMD_ZOOM_OUT, 0 );
            }
            break;
/*        case WXK_UP:
            {
		doCommand( DCMD_LINEUP, 1 );
            }
            break;
        case WXK_DOWN:
            {
		doCommand( DCMD_LINEDOWN, 1 );
            }
            break;
        case WXK_PAGEUP:
            {
		doCommand( DCMD_PAGEUP, 1 );
            }
            break;
        case WXK_PAGEDOWN:
            {
		doCommand( DCMD_PAGEDOWN, 1 );
            }
            break;
        case WXK_HOME:
            {
		doCommand( DCMD_BEGIN, 0 );
            }
            break;
        case WXK_END:
            {
		doCommand( DCMD_END, 0 );
            }
            break;
*/

        }
}

bool cr3view::LoadDocument( const wxString & fname )
{
    //printf("cr3view::LoadDocument()\n");
    _renderTimer->Stop();
    _clockTimer->Stop();
    CloseDocument();

	wxCursor hg( wxCURSOR_WAIT );
	this->SetCursor( hg );
	wxSetCursor( hg );
    //===========================================
    GetParent()->Update();
    //printf("   loading...  ");
    bool res = getDocView()->LoadDocument( fname.c_str() );
    //printf("   done. \n");
	//DEBUG
	//_docview->exportWolFile( "test.wol", true );
	//_docview->SetPos(0);
    if ( !res )
        getDocView()->createDefaultDocument(lString16("File open error"), lString16("Cannot open file ") + fname.c_str() );
    lString16 title = getDocView()->getAuthors();
    if ( !title.empty() && !getDocView()->getTitle().empty() )
        title << L". ";
    title << getDocView()->getTitle();
    GetParent()->SetLabel( wxString( title.c_str() ) );

    //UpdateScrollBar();
    _firstRender = true;
    _allowRender = false;
    ScheduleRender();
    //_docview->restorePosition();
	//_docview->Render();
	//UpdateScrollBar();
	//Paint();
    GetParent()->SetFocus();
    //===========================================
	wxSetCursor( wxNullCursor );
	this->SetCursor( wxNullCursor );
    return res;
}

void cr3view::goToBookmark(ldomXPointer bm)
{
    getDocView()->goToBookmark(bm);
    UpdateScrollBar();
    Paint();
}

void cr3view::SetRotate( cr_rotate_angle_t angle )
{
    getDocView()->SetRotateAngle( angle );
    _props->setInt( PROP_WINDOW_ROTATE_ANGLE, angle );
    UpdateScrollBar();
    Paint();
}

void cr3view::Rotate( bool ccw )
{
    int angle = (getDocView()->GetRotateAngle() + 4 + (ccw?-1:1)) & 3;
    SetRotate( (cr_rotate_angle_t) angle );
}

void cr3view::doCommand( LVDocCmd cmd, int param )
{
    _docwin->onCommand( cmd, param );
    UpdateScrollBar();
    Paint();
}

void cr3view::Resize(int dx, int dy)
{
    //printf("   Resize(%d,%d) \n", dx, dy );
    if ( dx==0 && dy==0 ) {
        GetClientSize( &dx, &dy );
    }
    if ( getDocView()->IsRendered() && getDocView()->GetWidth() == dx && getDocView()->GetHeight() == dy )
        return; // no resize
    if (dx<5 || dy<5 || dx>3000 || dy>3000)
    {
        return;
    }

	_renderTimer->Stop();
    _renderTimer->Start( 100, wxTIMER_ONE_SHOT );
    _clockTimer->Stop();
    _clockTimer->Start( 10 * 1000, wxTIMER_CONTINUOUS );

    if ( _isFullscreen ) {
        _cursorTimer->Stop();
        _cursorTimer->Start( 3 * 1000, wxTIMER_ONE_SHOT );
    }

    SetCursor( wxNullCursor );
}

void cr3view::OnPaint(wxPaintEvent& event)
{
    //printf("   OnPaint()  \n" );
    wxPaintDC dc(this);
    if ( !_allowRender ) {
        dc.Clear();
        return;
    }

    int dx, dy;
    GetClientSize( &dx, &dy );
    if ( !getDocView()->IsRendered() && (getDocView()->GetWidth() != dx || getDocView()->GetHeight() != dy) ) {
        if ( _firstRender ) {
            getDocView()->restorePosition();
            _firstRender = false;
        }

        getDocView()->Resize( dx, dy );
        return;
    }

    // draw
    _wm.update( true );
    wxBitmap bmp = _screen.getWxBitmap();
    dc.DrawBitmap( bmp, 0, 0, false );
}

void cr3view::OnSize(wxSizeEvent& event)
{
    int width, height;
    GetClientSize( &width, &height );
    //printf("   OnSize(%d, %d)  \n", width, height );
    Resize( width, height );
}


// LVDocViewCallback override
void cr3view::OnExternalLink( lString16 url, ldomNode * node )
{
    ::wxLaunchDefaultBrowser( wxString(url.c_str()) );
}
