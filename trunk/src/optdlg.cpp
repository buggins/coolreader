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
        int tb = props->getIntDef( _option, _defvalue );
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

static wxString choices_toolbar_size[] = {
    wxT("Hide Toolbar"),
    wxT("Small buttons"),
    wxT("Medium buttons"),
    wxT("Large buttons"),
    wxString()
};

static wxString choices_toolbar_position[] = {
    wxT("Top"),
    wxT("Left"),
    wxT("Right"),
    wxT("Bottom"),
    wxString()
};

class OptPanelWindow : public OptPanel {
public:
    OptPanelWindow( wxWindow * parent )
    {
         OptPanel::Create( parent, ID_OPTIONS_WINDOW, wxT("Window options") ); 
    }
    virtual void CreateControls()
    {
        AddCombobox( PROP_WINDOW_TOOLBAR_SIZE, wxT("Toolbar size"), choices_toolbar_size, 2 );
        AddCombobox( PROP_WINDOW_TOOLBAR_POSITION, wxT("Toolbar position"), choices_toolbar_position, 0 );
        AddCheckbox( PROP_WINDOW_SHOW_MENU, wxT("Show menu"), true );
        AddCheckbox( PROP_WINDOW_SHOW_STATUSBAR, wxT("Show statusbar"), true );
    }
};

class OptPanelApp : public OptPanel {
public:
    OptPanelApp( wxWindow * parent )
    {
         OptPanel::Create( parent, wxID_ANY, wxT("Application options") ); 
    }
    virtual void CreateControls()
    {
        AddCheckbox( PROP_APP_OPEN_LAST_BOOK, wxT("Open last book on start"), true );
    }
};

static wxString choices_page[] = {
    wxT("Scroll view"),
    wxT("1 Book page"),
    wxT("2 Book pages"),
    wxString()
};

class OptPanelPageHeader : public OptPanel {
public:
    OptPanelPageHeader( wxWindow * parent ) {
         OptPanel::Create( parent, wxID_ANY, wxT("Page header options") );
    }
    virtual void CreateControls()
    {
        AddCheckbox( PROP_PAGE_HEADER_ENABLED, wxT("Enable page header"), true );
        AddCheckbox( PROP_PAGE_HEADER_TITLE, wxT("Show title"), true );
        AddCheckbox( PROP_PAGE_HEADER_AUTHOR, wxT("Show author"), true );
        AddCheckbox( PROP_PAGE_HEADER_PAGE_COUNT, wxT("Show page count"), true );
        AddCheckbox( PROP_PAGE_HEADER_PAGE_NUMBER, wxT("Show page number"), true );
        AddCheckbox( PROP_PAGE_HEADER_CLOCK, wxT("Show clock"), true );
        AddCheckbox( PROP_PAGE_HEADER_BATTERY, wxT("Show battery indicator"), true );
    }
};

class OptPanelView : public OptPanel {
public:
    OptPanelView( wxWindow * parent ) {
         OptPanel::Create( parent, wxID_ANY, wxT("View options") );
    }
    virtual void CreateControls()
    {
        AddCombobox( PROP_PAGE_VIEW_MODE, wxT("View mode"), choices_page, 1 );
    }
};

CR3OptionsDialog::CR3OptionsDialog(CRPropRef props)
: _notebook(NULL), _opt_window(NULL), _oldprops(props)
{
    _props = _oldprops->clone();
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

        _opt_window = new OptPanelWindow( _notebook );
        _notebook->InsertPage(0, _opt_window, wxT("Window") );

        _opt_view = new OptPanelView( _notebook );
        _notebook->InsertPage(1, _opt_view, wxT("View") );

        _opt_page = new OptPanelPageHeader( _notebook );
        _notebook->InsertPage(2, _opt_page, wxT("Page header") );

        _opt_app = new OptPanelApp( _notebook );
        _notebook->InsertPage(3, _opt_app, wxT("Application") );

        SetSizer( sizer );
        PropsToControls();
        _notebook->Show();
        InitDialog();
    }
    return res;
}
