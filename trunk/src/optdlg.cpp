// options dialog, implementation
#include <wx/wx.h>
#include <crengine.h>
#include "cr3.h"
#include "optdlg.h"

class CheckBoxOption : public PropOption {
private:
    bool _defvalue;
public:
    CheckBoxOption( wxCheckBox * control, const char * option, bool defvalue )
    : PropOption( control, option ), _defvalue(defvalue)
    {
    }
    virtual void ControlToOption( CRPropRef props )
    {
        props->setBool( _option, ((wxCheckBox*)_control)->IsChecked() );
    }
    virtual void OptionToControl( CRPropRef props )
    {
        ((wxCheckBox*)_control)->SetValue( props->getBoolDef(_option, _defvalue) );
    }
};

class ComboBoxOption : public PropOption {
private:
    int _defvalue;
    const wxString * _choices;
    int _size;
public:
    ComboBoxOption( wxComboBox * control, const char * option, int defvalue, const wxString * choices )
    : PropOption( control, option ), _defvalue(defvalue), _choices(choices)
    {
        for ( _size=0; _choices[_size].length(); _size++ )
            ;
    }
    virtual void ControlToOption( CRPropRef props )
    {
        wxString v = ((wxComboBox*)_control)->GetValue();
        int tb = _defvalue;
        for ( int i=0; i<_size; i++ )
            if ( v==_choices[i] )
                tb = i;
        props->setInt( _option, tb );
    }
    virtual void OptionToControl( CRPropRef props )
    {
        int tb = props->getIntDef( PROP_WINDOW_TOOLBAR_SIZE, 2 );
        if ( tb<0 )
            tb = _defvalue;
        if ( tb>=_size )
            tb = _defvalue;
        ((wxComboBox*)_control)->SetValue( _choices[tb] );
    }
};



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

wxComboBox * OptPanel::AddCombobox( const char * option, wxString caption, wxString options[], int defValue )
{
    int size = 0;
    for ( ; options[size].length(); size++ )
        ;
    wxSizer * sizer = new wxBoxSizer( wxHORIZONTAL );
    sizer->Add( new wxStaticText( this, wxID_ANY, caption ),
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        4); // no border and centre horizontally
    wxComboBox * control = new wxComboBox( this, wxID_ANY,
        options[defValue],
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
    _opts.add( new ComboBoxOption( control, option, defValue, options ) );
    return control;
}
wxCheckBox * OptPanel::AddCheckbox( const char * option, wxString caption, bool defValue )
{
    wxCheckBox * control = new wxCheckBox( this, wxID_ANY, caption );
    _sizer->Add(
        control,
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        8); // no border and centre horizontally
    _opts.add( new CheckBoxOption( control, option, defValue ) );
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
    ~OptWindow() { }
};

static wxString tbchoices[] = {
    wxT("Hide Toolbar"),
    wxT("Small buttons"),
    wxT("Medium buttons"),
    wxT("Large buttons"),
    wxString()
};

void OptWindow::CreateControls()
{
    _cb_menu = AddCheckbox( PROP_WINDOW_SHOW_MENU, wxT("Show menu"), true );
    _cb_toolbar = AddCombobox( PROP_WINDOW_TOOLBAR_SIZE, wxT("Toolbar"), tbchoices, 2 );
    _cb_statusbar = AddCheckbox( PROP_WINDOW_SHOW_STATUSBAR, wxT("Show statusbar"), true );
}

static wxString tbchoices_page[] = {
    wxT("1 Book page"),
    wxT("2 Book pages"),
    wxT("Scroll view"),
    wxString()
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
        _cb_view_mode = AddCombobox( PROP_PAGE_VIEW_MODE, wxT("View mode"), tbchoices_page, 1 );
        _cb_title = AddCheckbox( PROP_PAGE_HEADER_TITLE, wxT("Show title in page header"), true );
        _cb_author = AddCheckbox( PROP_PAGE_HEADER_AUTHOR, wxT("Show author in page header"), true );
        _cb_page_count = AddCheckbox( PROP_PAGE_HEADER_PAGE_COUNT, wxT("Show page count in page header"), true );
        _cb_page_number = AddCheckbox( PROP_PAGE_HEADER_PAGE_NUMBER, wxT("Show page number in page header"), true );
        _cb_clock = AddCheckbox( PROP_PAGE_HEADER_CLOCK, wxT("Show clock in page header"), true );
        _cb_battery = AddCheckbox( PROP_PAGE_HEADER_BATTERY, wxT("Show battery indicator in page header"), true );
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
