#include <wx/wx.h>
#include <wx/mstream.h>

#define USE_FREETYPE 1

#include <crengine.h>
#include "cr3.h"
#include "wolopt.h"
#include "rescont.h"
#include "cr3.xpm"


BEGIN_EVENT_TABLE( cr3Frame, wxFrame )
	EVT_MENU( Menu_File_Quit, cr3Frame::OnQuit )
	EVT_MENU( Menu_File_About, cr3Frame::OnAbout )
    EVT_MENU( wxID_OPEN, cr3Frame::OnFileOpen )
    EVT_MENU( wxID_SAVE, cr3Frame::OnFileSave )
    EVT_MENU_RANGE( 0, 0xFFFF, cr3Frame::OnCommand )
//	EVT_UPDATE_UI_RANGE( 0, 0xFFFF, cr3Frame::OnUpdateUI )
    EVT_COMMAND_SCROLL( Window_Id_Scrollbar, cr3Frame::OnScroll )
    EVT_CLOSE( cr3Frame::OnClose )
    EVT_MOUSEWHEEL( cr3Frame::OnMouseWheel )
    EVT_INIT_DIALOG( cr3Frame::OnInitDialog )
    //EVT_SIZE    ( cr3Frame::OnSize )
END_EVENT_TABLE()


BEGIN_EVENT_TABLE( cr3scroll, wxScrollBar )
    EVT_SET_FOCUS( cr3scroll::OnSetFocus )
END_EVENT_TABLE()

IMPLEMENT_APP(cr3app)
	

#define USE_XPM_BITMAPS 1


#include "resources/cr3res.h"


ResourceContainer * resources = NULL;



void cr3Frame::OnUpdateUI( wxUpdateUIEvent& event )
{

}

void cr3Frame::OnClose( wxCloseEvent& event )
{
    _view->CloseDocument();
    Destroy();
}


void cr3Frame::OnCommand( wxCommandEvent& event )
{
    _view->OnCommand( event );
	switch ( event.GetId() ) {
    case Menu_View_ToggleFullScreen:
        _isFullscreen = !_isFullscreen;
        ShowFullScreen( _isFullscreen );
        break;
	case Menu_View_ZoomIn:
	case Menu_View_ZoomOut:
	case Menu_View_NextPage:
	case Menu_View_PrevPage:
		break;
	}
}

void cr3Frame::OnMouseWheel(wxMouseEvent& event)
{
    _view->OnMouseWheel(event);
}

void cr3Frame::OnSize(wxSizeEvent& event)
{
    wxStatusBar * sb = GetStatusBar();
    if ( sb ) {
        int width, height;
        GetClientSize( &width, &height );
        int sw[3] = {width-200, 100, 100};
        sb->SetStatusWidths( 3, sw );
    }
}


void initHyph(const char * fname)
{
    //HyphMan hyphman;
    //return;

    LVStreamRef stream = LVOpenFileStream( fname, LVOM_READ);
    if (!stream)
    {
        printf("Cannot load hyphenation file %s\n", fname);
        return;
    }
    HyphMan::Open( stream.get() );
}

lString8 readFileToString( const char * fname )
{
    lString8 buf;
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return buf;
    int sz = stream->GetSize();
    if (sz>0)
    {
        buf.insert( 0, sz, ' ' );
        stream->Read( buf.modify(), sz, NULL );
    }
    return buf;
}


int cr3app::OnExit()
{
	ShutdownFontManager();
    delete resources;
    HyphMan::Close();
#if LDOM_USE_OWN_MEM_MAN == 1
	ldomFreeStorage();
#endif
	return 0;
}

wxBitmap getIcon16x16( const lChar16 * name )
{
    wxBitmap icon = resources->GetBitmap( (lString16(L"icons/22x22/") + name + L".png").c_str() );
    if ( icon.IsOk() )
        return icon;
    return wxNullBitmap;
}

bool 
cr3app::OnInit()
{
    wxImage::AddHandler(new wxPNGHandler);
    resources = new ResourceContainer();

    lString16 appname( argv[0] );
    int lastSlash=-1;
    lChar16 slashChar = '/';
    for ( int p=0; p<(int)appname.length(); p++ ) {
        if ( appname[p]=='\\' ) {
            slashChar = '\\';
            lastSlash = p;
        } else if ( appname[p]=='/' ) {
            slashChar = '/';
            lastSlash=p;
        }
    }

    lString16 appPath;
    if ( lastSlash>=0 )
        appPath = appname.substr( 0, lastSlash+1 );

    if (resources->OpenFromMemory( cr3_icons, sizeof(cr3_icons) )) {
/*
        LVStreamRef testStream = resources.GetStream(L"icons/16x16/signature.png");
        if ( !testStream.isNull() ) {
            int sz = testStream->GetSize();
            lUInt8 * buf = new lUInt8[ sz ];
            testStream->Read(buf, sz, NULL);
            printf("read: %c%c%c\n", buf[0], buf[1], buf[2]);
        }
        wxBitmap icon = resources.GetBitmap(L"icons/16x16/signature.png");
        if ( icon.IsOk() ) {
            printf( "Image opened: %dx%d:%d\n", icon.GetWidth(), icon.GetHeight(), icon.GetDepth() );
        }
*/
    }
    lString16 fontDir = appPath + L"fonts";
    fontDir << slashChar;
    InitFontManager( UnicodeToLocal(fontDir)  );

    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
	if (!fontMan->GetFontCount()) {


#if (USE_FREETYPE==1)
        LVContainerRef dir = LVOpenDirectory(fontDir.c_str());
        for ( int i=0; i<dir->GetObjectCount(); i++ ) {
            const LVContainerItemInfo * item = dir->GetObjectInfo(i);
            lString16 fileName = item->GetName();
            if ( !item->IsContainer() && fileName.length()>4 && lString16(fileName, fileName.length()-4, 4)==L".ttf" ) {
                lString8 fn = UnicodeToLocal(fileName);
                printf("loading font: %s\n", fn.c_str());
                if ( !fontMan->RegisterFont(fn) ) {
                    printf("    failed\n");
                }
            }
        }
        //fontMan->RegisterFont(lString8("arial.ttf"));
#else
        #define MAX_FONT_FILE 128
        for (int i=0; i<MAX_FONT_FILE; i++)
        {
            char fn[1024];
            sprintf( fn, "font%d.lbf", i );
            printf("try load font: %s\n", fn);
            fontMan->RegisterFont( lString8(fn) );
        }
#endif
	}

    // init hyphenation manager
    char hyphfn[1024];
    sprintf(hyphfn, "Russian_EnUS_hyphen_(Alan).pdb" );
    initHyph( (UnicodeToLocal(appPath) + hyphfn).c_str() );

    if (!fontMan->GetFontCount())
    {
        //error
#if (USE_FREETYPE==1)
        printf("Fatal Error: Cannot open font file(s) .ttf \nCannot work without font\n" );
#else
        printf("Fatal Error: Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF\n" );
#endif
        return FALSE;
    }

    printf("%d fonts loaded.\n", fontMan->GetFontCount());


	int cx = wxSystemSettings::GetMetric( wxSYS_SCREEN_X );
	int cy = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y )-40;
	int scale_x = cx * 256 / 620;
	int scale_y = cy * 256 / 830;
	int scale = scale_x < scale_y ? scale_x : scale_y;
	cx = 610 * scale / 256;
	cy = 830 * scale / 256;
	cr3Frame *frame = new cr3Frame( wxT( "CoolReader 3.0.3" ), wxPoint(20,40), wxSize(cx,cy), appPath );

	frame->Show(TRUE);
	SetTopWindow(frame);
	return TRUE;
} 

void cr3Frame::OnInitDialog(wxInitDialogEvent& event)
{
	_scrollBar = new cr3scroll(_view);

	_view->SetScrollBar( _scrollBar );
	_view->Create(this, Window_Id_View, 
		wxDefaultPosition, wxDefaultSize, 0, wxT("cr3view"));

	_scrollBar->Create(this, Window_Id_Scrollbar, 
		wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
    

	wxMenu *menuFile = new wxMenu;

    menuFile->Append( wxID_OPEN, wxT( "&Open...\tCtrl+O" ) );
    menuFile->Append( wxID_SAVE, wxT( "&Save...\tCtrl+S" ) );
    menuFile->AppendSeparator();
	menuFile->Append( Menu_File_About, wxT( "&About...\tF1" ) );
	menuFile->AppendSeparator();
	menuFile->Append( Menu_File_Quit, wxT( "E&xit\tAlt+X" ) );
	
    wxMenu *menuView = new wxMenu;

    menuView->Append( Menu_View_ZoomIn, wxT( "Zoom In" ) );
    menuView->Append( Menu_View_ZoomOut, wxT( "Zoom Out" ) );
    menuView->AppendSeparator();
    menuView->Append( Menu_View_ToggleFullScreen, wxT( "Toggle Fullscreen\tAlt+Enter" ) );
    menuView->Append( Menu_View_TogglePages, wxT( "Toggle Pages/Scroll\tCtrl+P" ) );
    menuView->Append( Menu_View_TogglePageHeader, wxT( "Toggle page heading\tCtrl+H" ) );
    
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, wxT( "&File" ) );
    menuBar->Append( menuView, wxT( "&View" ) );
	
	SetMenuBar( menuBar );
	
	CreateStatusBar();
    wxStatusBar * status = GetStatusBar();
    int sw[3] = { -1, 100, 100 };
    //int ss[3] = {wxSB_NORMAL, wxSB_FLAT, wxSB_FLAT};
    status->SetFieldsCount(2, sw);
    //status->SetStatusStyles(3, ss);


	SetStatusText( wxT( "Welcome to CoolReader 3.0!" ) );

    wxToolBar* toolBar = CreateToolBar();

    // Set up toolbar
/*
    enum
    {
        Tool_new,
        Tool_open,
        Tool_save,
        Tool_copy,
        Tool_cut,
        Tool_paste,
        Tool_print,
        Tool_help,
        Tool_zoomin,
        Tool_zoomout,
        Tool_north,
        Tool_south,
        Tool_west,
        Tool_east,
        Tool_Max
    };
*/
    //wxBitmap toolBarBitmaps[Tool_Max];
/*
#if USE_XPM_BITMAPS
    #define INIT_TOOL_BMP(bmp) \
        toolBarBitmaps[Tool_##bmp] = wxBitmap(bmp##_xpm)
#else // !USE_XPM_BITMAPS
    #define INIT_TOOL_BMP(bmp) \
        toolBarBitmaps[Tool_##bmp] = wxBITMAP(bmp)
#endif // USE_XPM_BITMAPS/!USE_XPM_BITMAPS

*/
    wxIcon icon = wxICON(cr3);
    SetIcon( icon );
/*
    INIT_TOOL_BMP(new);
    INIT_TOOL_BMP(open);
    INIT_TOOL_BMP(save);
    INIT_TOOL_BMP(copy);
    INIT_TOOL_BMP(cut);
    INIT_TOOL_BMP(paste);
    INIT_TOOL_BMP(print);
    INIT_TOOL_BMP(help);
    INIT_TOOL_BMP(zoomin);
    INIT_TOOL_BMP(zoomout);
    INIT_TOOL_BMP(north);
    INIT_TOOL_BMP(south);
    INIT_TOOL_BMP(west);
    INIT_TOOL_BMP(east);
*/
    wxBitmap fileopenBitmap = getIcon16x16(L"fileopen");

    int w = fileopenBitmap.GetWidth(),
        h = fileopenBitmap.GetHeight();

/*
    if ( !m_smallToolbar )
    {
        w *= 2;
        h *= 2;

        for ( size_t n = Tool_new; n < WXSIZEOF(toolBarBitmaps); n++ )
        {
            toolBarBitmaps[n] =
                wxBitmap(toolBarBitmaps[n].ConvertToImage().Scale(w, h));
        }
    }
*/


    toolBar->SetToolBitmapSize(wxSize(w, h));
/*
    toolBar->AddTool(wxID_NEW, _T("New"),
                     toolBarBitmaps[Tool_new], wxNullBitmap, wxITEM_NORMAL,
                     _T("New file"), _T("This is help for new file tool"));
*/
    toolBar->AddTool(wxID_OPEN, _T("Open"),
                     fileopenBitmap,//toolBarBitmaps[Tool_open], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Open file"), _T("This is help for open file tool"));

    toolBar->AddTool(wxID_SAVE, _T("Save"), 
                     getIcon16x16(L"filesaveas"),//toolBarBitmaps[Tool_save], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Save as..."), _T("Export document"));
/*
    toolBar->AddTool(wxID_COPY, _T("Copy"), toolBarBitmaps[Tool_copy], _T("Toggle button 2"), wxITEM_CHECK);
    toolBar->AddTool(wxID_CUT, _T("Cut"), toolBarBitmaps[Tool_cut], _T("Toggle/Untoggle help button"));
    toolBar->AddTool(wxID_PASTE, _T("Paste"), toolBarBitmaps[Tool_paste], _T("Paste"));
*/
    toolBar->AddSeparator();
    toolBar->AddTool(Menu_View_ZoomIn, _T("Zoom In"),
                     getIcon16x16(L"viewmag+"),//toolBarBitmaps[Tool_zoomin], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Zoom in"), _T("Increase font size"));
    toolBar->AddTool(Menu_View_ZoomOut, _T("Zoom Out"),
                     getIcon16x16(L"viewmag-"),//toolBarBitmaps[Tool_zoomout], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Zoom out"), _T("Decrease font size"));
    toolBar->AddSeparator();
    toolBar->AddTool(Menu_View_TogglePages, _T("Toggle pages (Ctrl+P)"),
                     getIcon16x16(L"view_left_right"),//toolBarBitmaps[Tool_zoomout], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Toggle pages (Ctrl+P)"), _T("Switch pages/scroll mode"));
    toolBar->AddTool(Menu_View_ToggleFullScreen, _T("Fullscreen (Alt+Enter)"),
                     getIcon16x16(L"window_fullscreen"),//toolBarBitmaps[Tool_zoomout], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Fullscreen (Alt+Enter)"), _T("Switch to fullscreen mode"));
    toolBar->AddSeparator();
//Menu_View_ToggleFullScreen
    toolBar->AddTool(Menu_View_PrevPage, _T("Previous page"),
                     getIcon16x16(L"previous"),//toolBarBitmaps[Tool_north], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Previous page"), _T("Go to previous page"));
    toolBar->AddTool(Menu_View_NextPage, _T("Next page"),
                     getIcon16x16(L"next"),//toolBarBitmaps[Tool_south], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Next page"), _T("Go to next page"));

    toolBar->AddSeparator();
    toolBar->AddTool(Menu_File_About, _T("Help"), //wxID_HELP
                     getIcon16x16(L"help"),//toolBarBitmaps[Tool_help], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Help"), _T("Help"));

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    toolBar->Realize();

    _sizer = new wxBoxSizer( wxHORIZONTAL );
    _sizer->Add( _view,
        1,
        wxALL | wxEXPAND,
        0);
    _sizer->Add( _scrollBar,
        0,
        wxALIGN_RIGHT | wxEXPAND,
        0);
    SetSizer( _sizer );

    _view->SetBackgroundColour( _view->getBackgroundColour() );
    _scrollBar->Show( true );
    SetBackgroundColour( _view->getBackgroundColour() );

    // stylesheet can be placed to file fb2.css
    // if not found, default stylesheet will be used
    char cssfn[1024];
    sprintf( cssfn, "fb2.css"); //, exedir
    lString8 css = readFileToString( (UnicodeToLocal(_appDir) + cssfn).c_str() );
    if (css.length() > 0)
    {
        printf("Style sheet file found.\n");
        _view->getDocView()->setStyleSheet( css );
    }

    _view->UpdateScrollBar();
    _view->Show( true );

    wxAcceleratorEntry entries[30];
    int a=0;
    entries[a++].Set(wxACCEL_CTRL,  (int) 'O',     wxID_OPEN);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'S',     wxID_SAVE);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'P',     Menu_View_TogglePages);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'H',     Menu_View_TogglePageHeader);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F3,      wxID_OPEN);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F2,      wxID_SAVE);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_UP,      Menu_View_PrevLine);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_DOWN,    Menu_View_NextLine);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_SPACE,    Menu_View_NextLine);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_NUMPAD_ADD,      Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_NUMPAD_SUBTRACT, Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_ADD,      Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_SUBTRACT, Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_CTRL,    WXK_NUMPAD_ADD,      Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_CTRL,    WXK_NUMPAD_SUBTRACT, Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_NORMAL,  (int) '+',     Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  (int) '-',     Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_NORMAL,  (int) '=',     Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_PAGEUP,    Menu_View_PrevPage);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_PAGEDOWN,  Menu_View_NextPage);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_HOME,      Menu_View_Begin);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_END,       Menu_View_End);
    entries[a++].Set(wxACCEL_ALT,     WXK_RETURN,     Menu_View_ToggleFullScreen);
    wxAcceleratorTable accel(a, entries);
    SetAcceleratorTable(accel);
    //_view->SetAcceleratorTable(accel);

    _view->SetFocus();

    //toolBar->SetRows(!(toolBar->IsVertical()) ? m_rows : 10 / m_rows);
    int argc = wxGetApp().argc;
    lString16 fnameToOpen;
    if ( argc>1 ) {
        if ( !_view->LoadDocument( wxString(wxGetApp().argv[1])) )
        {
            printf("cannot open document\n");
        }
    }
}

cr3Frame::cr3Frame( const wxString& title, const wxPoint& pos, const wxSize& size, lString16 appDir )
	: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{

    _appDir = appDir;

    _isFullscreen = false;

	_view = new cr3view();

    InitDialog();
}

void 
cr3Frame::OnQuit( wxCommandEvent& WXUNUSED( event ) )
{
	Close(TRUE);
}

void 
cr3Frame::OnFileOpen( wxCommandEvent& WXUNUSED( event ) )
{
    wxFileDialog dlg( this, wxT( "Choose a file to open" ), 
        wxT( "" ),
        wxT( "" ),//const wxString& defaultFile = "", 
        wxT("All supported files|*.fb2;*.zip|FictionBook files (*.fb2)|*.fb2|ZIP archieves (*.zip)|*.zip"), //const wxString& wildcard = "*.*", 
        wxFD_OPEN | wxFD_FILE_MUST_EXIST //long style = wxFD_DEFAULT_STYLE, 
        //const wxPoint& pos = wxDefaultPosition, 
        //const wxSize& sz = wxDefaultSize, 
        //const wxString& name = "filedlg"
    );
    if ( dlg.ShowModal() == wxID_OK ) {
        //
        Update();
		_view->LoadDocument( dlg.GetPath() );
    }
        
}

void 
cr3Frame::OnFileSave( wxCommandEvent& WXUNUSED( event ) )
{
    wxFileDialog dlg( this, wxT( "Choose a file to open" ), 
        wxT( "" ),
        wxT( "" ),//const wxString& defaultFile = "", 
        wxT("Wolf EBook files (*.wol)|*.wol"), //const wxString& wildcard = "*.*", 
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT //long style = wxFD_DEFAULT_STYLE, 
        //const wxPoint& pos = wxDefaultPosition, 
        //const wxSize& sz = wxDefaultSize, 
        //const wxString& name = "filedlg"
    );
    WolOptions opts( this );
    if ( dlg.ShowModal() == wxID_OK && opts.ShowModal() == wxID_OK ) {
        //
		//_view->LoadDocument( dlg.GetPath() );
        Update();
		wxCursor hg( wxCURSOR_WAIT );
		this->SetCursor( hg );
		wxSetCursor( hg );
		_view->getDocView()->exportWolFile( dlg.GetPath(), opts.getMode()==0, opts.getLevels() );
		wxSetCursor( wxNullCursor );
		this->SetCursor( wxNullCursor );
    }
}

void 
cr3Frame::OnAbout( wxCommandEvent& WXUNUSED( event ) )
{
	wxMessageBox( wxT( "Cool Reader 3.0.3\n(c) 1998-2007 Vadim Lopatin\nwxWidgets version" ),
			wxT( "About Cool Reader" ), wxOK | wxICON_INFORMATION, this );
}

void
cr3Frame::OnScroll(wxScrollEvent& event)
{
    _view->OnScroll( event );
}

void cr3scroll::OnSetFocus( wxFocusEvent& event )
{
	_view->SetFocus();
}

void cr3view::OnSetFocus( wxFocusEvent& event )
{
    GetParent()->SetFocus();
}

void
cr3Frame::OnKeyDown(wxKeyEvent& event)
{
    _view->OnKeyDown( event );
}
