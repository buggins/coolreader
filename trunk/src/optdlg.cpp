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

wxWindow * OptPanel::AddControl(wxWindow * control)
{
    _sizer->Add(
        control,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    return control;
}

wxComboBox * OptPanel::AddCombobox(wxString caption, wxString options[], int size, int selection ) {
    wxSizer * sizer = new wxBoxSizer( wxHORIZONTAL );
    sizer->Add( new wxStaticText( this, wxID_ANY, caption ),
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        4); // no border and centre horizontally
    wxComboBox * control = new wxComboBox( this, wxID_ANY,
        options[selection],
        wxDefaultPosition, wxDefaultSize, size,
        options,
        wxCB_READONLY | wxCB_DROPDOWN );
    sizer->Add(
        control,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        4); // no border and centre horizontally
    _sizer->Add(
        sizer,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        4); // no border and centre horizontally
    return control;
}
wxCheckBox * OptPanel::AddCheckbox(wxString caption)
{
    wxCheckBox * control = new wxCheckBox( this, wxID_ANY, caption );
    _sizer->Add(
        control,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    return control;
}

class OptWindow : public OptPanel {
private:
    wxPanel * _panel;
    wxCheckBox * _cb_menu;
    wxComboBox * _cb_toolbar;
    wxCheckBox * _cb_statusbar;
public:
    OptWindow( wxWindow * parent )
    {
         OptPanel::Create( parent, ID_OPTIONS_WINDOW, wxT("Window options (restart to apply)") ); 
    }
    virtual void CreateControls();
    virtual void PropsToControls( CRPropRef props );
    virtual void ControlsToProps( CRPropRef props );
    ~OptWindow() { }
};


static wxString tbchoices[] = {
    wxT("Hide Toolbar"),
    wxT("Small buttons"),
    wxT("Medium buttons"),
    wxT("Large buttons"),
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
    _cb_menu = AddCheckbox( wxT("Show menu") );
    _cb_toolbar = AddCombobox( wxT("Toolbar"), tbchoices, 4, 2 );
    _cb_statusbar = AddCheckbox( wxT("Show statusbar") );
}

static wxString tbchoices_page[] = {
    wxT("1 Book page"),
    wxT("2 Book pages"),
    wxT("Scroll view"),
};


class OptPanelPage : public OptPanel {
private:
    wxPanel * _panel;
    wxComboBox * _cb_view_mode;
    wxCheckBox * _cb_title;
    wxCheckBox * _cb_author;
    wxCheckBox * _cb_page_count;
    wxCheckBox * _cb_page_number;
    wxCheckBox * _cb_clock;
    wxCheckBox * _cb_battery;
public:
    OptPanelPage( wxWindow * parent ) {
         OptPanel::Create( parent, wxID_ANY, wxT("Page options") );
    }
    virtual void CreateControls()
    {
        _cb_view_mode = AddCombobox( wxT("View mode"), tbchoices_page, 3, 1 );
        _cb_title = AddCheckbox( wxT("Show title in page header") );
        _cb_author = AddCheckbox( wxT("Show author in page header") );
        _cb_page_count = AddCheckbox( wxT("Show page count in page header") );
        _cb_page_number = AddCheckbox( wxT("Show page number in page header") );
        _cb_clock = AddCheckbox( wxT("Show clock in page header") );
        _cb_battery = AddCheckbox( wxT("Show battery indicator in page header") );
    }
    virtual void PropsToControls( CRPropRef props )
    {
    }
    virtual void ControlsToProps( CRPropRef props )
    {
    }
    ~OptPanelPage() { }
};


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
        _opt_page = new OptPanelPage( _notebook );
        _notebook->InsertPage(0, _opt_page, wxT("Page") );

        //sizer->SetSizeHints( this );
        SetSizer( sizer );
        PropsToControls();
        _notebook->Show();
        InitDialog();
        
    }
    return res;
}
