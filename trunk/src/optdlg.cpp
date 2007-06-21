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

static wxString tbchoices[] = {
    wxT("Hide Toolbar"),
    wxT("Show Toolbar: small buttons"),
    wxT("Show Toolbar: medium buttons"),
    wxT("Show Toolbar: large buttons"),
};

void OptWindow::PropsToControls( CRPropRef props )
{
    _cb_menu->SetValue( props->getBoolDef(PROP_WINDOW_SHOW_MENU, true) );
    int tb = props->getIntDef(PROP_WINDOW_TOOLBAR_SIZE, 2);
    if ( tb<0 )
        tb = 0;
    if ( tb>3 )
        tb = 3;
    _cb_toolbar->SetValue( tbchoices[tb] );
    _cb_statusbar->SetValue( props->getBoolDef(PROP_WINDOW_SHOW_STATUSBAR, true) );
}

void OptWindow::ControlsToProps( CRPropRef props )
{
    props->setBool( PROP_WINDOW_SHOW_MENU, _cb_menu->IsChecked() );
    wxString v = _cb_toolbar->GetValue();
    int tb = 2;
    for ( int i=0; i<4; i++ )
        if ( v==tbchoices[i] )
            tb = i;
    if ( tb<0 )
        tb = 0;
    if ( tb>3 )
        tb = 3;
    props->setInt( PROP_WINDOW_TOOLBAR_SIZE, tb );
    props->setBool( PROP_WINDOW_SHOW_STATUSBAR, _cb_statusbar->IsChecked() );
}

void OptWindow::CreateControls()
{
    _cb_menu = new wxCheckBox( this, ID_OPTIONS_WINDOW_MENU, wxT("Show menu") );
    _cb_toolbar = new wxComboBox( this, ID_OPTIONS_WINDOW_TOOLBAR, 
        wxT("Show Toolbar: medium size"), 
        wxDefaultPosition, wxDefaultSize, 4, 
        tbchoices, 
        wxCB_READONLY | wxCB_DROPDOWN );

        //wxT("Show toolbar") );
    _cb_statusbar = new wxCheckBox( this, ID_OPTIONS_WINDOW_STATUSBAR, wxT("Show statusbar") );
    _sizer->Add(
        _cb_menu,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    _sizer->Add(
        _cb_statusbar,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    _sizer->Add(
        _cb_toolbar,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
}


CR3OptionsDialog::CR3OptionsDialog(CRPropRef props)
: _notebook(NULL), _opt_window(NULL), _props(props)
{
}

CR3OptionsDialog::~CR3OptionsDialog()
{
    if ( _notebook ) {
        delete _notebook;
        _notebook = NULL;
    }
}

void CR3OptionsDialog::PropsToControls()
{
    for (unsigned i=0; i<_notebook->GetPageCount(); i++) {
        OptPanel * page = (OptPanel *)_notebook->GetPage(i);
        page->PropsToControls(_props);
    }
}

void CR3OptionsDialog::ControlsToProps()
{
    for (unsigned i=0; i<_notebook->GetPageCount(); i++) {
        OptPanel * page = (OptPanel *)_notebook->GetPage(i);
        page->ControlsToProps(_props);
    }
}

bool CR3OptionsDialog::Create( wxWindow* parent, wxWindowID id )
{
    bool res = wxDialog::Create( parent, id, wxString(wxT("CR3 Options")), 
        wxDefaultPosition, wxSize(450, 400), 
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
        PropsToControls();
        _notebook->Show();
        InitDialog();
        
    }
    return res;
}
