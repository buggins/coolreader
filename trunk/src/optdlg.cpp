// options dialog, implementation
#include <wx/wx.h>
#include <crengine.h>
#include "cr3.h"
#include "optdlg.h"

OptPanel::OptPanel()
: _sizer(NULL)
{
}

void OptPanel::Create( wxWindow * parent, wxWindowID id, wxString title )
{
    wxPanel::Create( parent, id, wxDefaultPosition, wxSize( 350, 300 ),
            wxTAB_TRAVERSAL, title );
    _sizer = new wxStaticBoxSizer( wxVERTICAL, this, title );
    CreateControls();
    SetSizer( _sizer );
    //InitDialog();
}

void OptWindow::CreateControls()
{
    _cb_menu = new wxCheckBox( this, ID_OPTIONS_WINDOW_MENU, wxT("Show menu") );
    _cb_toolbar = new wxCheckBox( this, ID_OPTIONS_WINDOW_TOOLBAR, wxT("Show toolbar") );
    _cb_statusbar = new wxCheckBox( this, ID_OPTIONS_WINDOW_STATUSBAR, wxT("Show statusbar") );
    _sizer->Add(
        _cb_menu,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    _sizer->Add(
        _cb_toolbar,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    _sizer->Add(
        _cb_statusbar,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
}


CR3OptionsDialog::CR3OptionsDialog()
: _notebook(NULL), _opt_window(NULL)
{
}

CR3OptionsDialog::~CR3OptionsDialog()
{
    if ( _notebook ) {
        delete _notebook;
        _notebook = NULL;
    }
}


bool CR3OptionsDialog::Create( wxWindow* parent, wxWindowID id )
{
    bool res = wxDialog::Create( parent, id, wxString(wxT("Options")), 
        wxDefaultPosition, wxSize(400, 400), 
        wxRESIZE_BORDER|wxCLOSE_BOX|wxCAPTION, wxString(wxT("dialogBox")));
    if ( res ) {

        //_notebook = new wxNotebook( this, ID_OPTIONS_TREEBOOK );
        _notebook = new wxTreebook( this, ID_OPTIONS_TREEBOOK );
        _notebook->Hide();
        wxSizer * btnSizer = CreateButtonSizer( wxOK | wxCANCEL );
        wxBoxSizer * sizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add(
            _notebook,
            1,            // make vertically stretchable
            wxEXPAND |    // make horizontally stretchable
            wxALL,        //   and make border all around
            10 );         // set border width to 10

        sizer->Add(
            btnSizer,
            0,                // make vertically unstretchable
            wxALIGN_CENTER | wxALL,
            4); // no border and centre horizontally

        _opt_window = new OptWindow( _notebook );
        _notebook->InsertPage(0, _opt_window, wxT("Window") );
        //_notebook->InsertPage(1, NULL, wxT("Book") );
        //_notebook->InsertPage(2, NULL, wxT("Styles") );
        //_notebook->InsertSubPage(2, NULL, wxT("Body") );
        //_notebook->InsertSubPage(2, NULL, wxT("Paragraph") );

        //sizer->SetSizeHints( this );
        SetSizer( sizer );
        _notebook->Show();
        InitDialog();
        
    }
    return res;
}
