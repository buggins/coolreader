// options dialog, implementation
#include <wx/wx.h>
#include <wx/colordlg.h>
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
    wxArrayString _choices;
    bool _storeStringValues;
public:
    ComboBoxOption( wxComboBox * control, const char * option, int defvalue, const wxArrayString & choices, bool storeStringValues )
    : PropOption( control, option ), _defvalue(defvalue), _choices(choices), _storeStringValues(storeStringValues)
    {
    }
    virtual void ControlToOption( CRPropRef props )
    {
        wxString v = ((wxComboBox*)_control)->GetValue();
        int tb = _defvalue;
        for ( unsigned i=0; i<_choices.GetCount(); i++ )
            if ( v==_choices[i] )
                tb = i;
        if ( _storeStringValues ) {
            props->setString( _option, lString16(_choices[tb]) );
        } else {
            props->setInt( _option, tb );
        }
    }
    virtual void OptionToControl( CRPropRef props )
    {
        unsigned tb = _defvalue;
        if ( _storeStringValues ) {
            lString8 s8 = UnicodeToUtf8( lString16(_choices[_defvalue]) );
            lString16 s16 = props->getStringDef( _option, s8.c_str() );
            wxString v = s16.c_str();
            for ( unsigned i=0; i<_choices.GetCount(); i++ )
                if ( v==_choices[i] )
                    tb = i;
        } else {
            tb = props->getIntDef( _option, _defvalue );
            if ( tb<0 )
                tb = _defvalue;
            if ( tb>=_choices.GetCount() )
                tb = _defvalue;
        }
        ((wxComboBox*)_control)->SetValue( _choices[tb] );
    }
};

class ColorOption : public PropOption {
    private:
        lvColor _value;
        wxPanel _panel;
        int _buttonId;
    public:
        ColorOption( wxPanel * control, const char * option, lUInt32 value, int buttonId )
        : PropOption( control, option ), _value(value), _buttonId(buttonId)
        {
        }
        virtual void ControlToOption( CRPropRef props )
        {
            props->setHex( _option, _value.get() );
        }
        virtual void OptionToControl( CRPropRef props )
        {
            lvColor cl = props->getIntDef( _option, _value );
            _value = cl;
            _control->SetBackgroundColour( wxColor(cl.g(), cl.g(), cl.b()) );
        }
        virtual int getActionId() { return _buttonId; }
        virtual void onAction()
        {
            wxColourData data;
            data.SetColour(wxColor(_value.g(), _value.g(), _value.b()));
            wxColourDialog dlg(_control, &data);
            if ( dlg.ShowModal()==wxID_OK ) {
                wxColourData & data = dlg.GetColourData();
                wxColor cl = data.GetColour();
                lvColor lvcl( cl.Red(), cl.Green(), cl.Blue() );
                _value = lvcl;
                _control->SetBackgroundColour( wxColor(_value.g(), _value.g(), _value.b()) );
            }
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

wxPanel * OptPanel::AddColor( const char * option, wxString caption, lvColor defValue, int buttonId )
{
    wxSizer * sizer = new wxBoxSizer( wxHORIZONTAL );
    sizer->Add( new wxStaticText( this, wxID_ANY, caption ),
                0,                // make vertically unstretchable
                wxALIGN_LEFT | wxALL,
                4); // no border and centre horizontally
    wxButton * btn = new wxButton( this, buttonId,
                                   wxString(wxT("Change")));
    sizer->Add( btn,
                0,                // make vertically unstretchable
                wxALIGN_RIGHT | wxALL,
                4); // no border and centre horizontally
    wxPanel * control = new wxPanel( this );
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
    _opts.add( new ColorOption( control, option, defValue, buttonId ) );
    return control;
}

wxComboBox * OptPanel::AddFontFaceCombobox( const char * option, wxString caption )
{
    lString16Collection list;
    fontMan->getFaceList( list );
    wxArrayString wxlist;// = new wxString[ list.length() + 1 ];
    int arialIndex = -1;
    for ( unsigned i=0; i<list.length(); i++ ) {
        if (list[i] == "Arial")
            arialIndex = i;
        wxlist.Add( wxString(list[i].c_str()) );
    }
    if ( arialIndex<0 )
        arialIndex = 0;
    wxComboBox * res = AddCombobox( option, caption, wxlist, arialIndex, true );
    //delete[] wxlist;
    return res;
}

wxComboBox * OptPanel::AddCombobox( const char * option, wxString caption, const wxArrayString & options, int defValue, bool storeValues )
{
    wxSizer * sizer = new wxBoxSizer( wxHORIZONTAL );
    sizer->Add( new wxStaticText( this, wxID_ANY, caption ),
        0,                // make vertically unstretchable
        wxALIGN_LEFT | wxALL,
        4); // no border and centre horizontally
    wxComboBox * control = new wxComboBox( this, wxID_ANY,
        options[defValue],
        wxDefaultPosition, wxDefaultSize, options,
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
    _opts.add( new ComboBoxOption( control, option, defValue, options, storeValues ) );
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
};

static wxString choices_toolbar_position[] = {
    wxT("Top"),
    wxT("Left"),
    wxT("Right"),
    wxT("Bottom"),
};

class OptPanelWindow : public OptPanel {
public:
    OptPanelWindow( wxWindow * parent )
    {
         OptPanel::Create( parent, ID_OPTIONS_WINDOW, wxT("Window options") ); 
    }
    virtual void CreateControls()
    {
        AddCombobox( PROP_WINDOW_TOOLBAR_SIZE, wxT("Toolbar size"), wxArrayString(4, choices_toolbar_size), 2 );
        AddCombobox( PROP_WINDOW_TOOLBAR_POSITION, wxT("Toolbar position"), wxArrayString(4, choices_toolbar_position), 0 );
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

static wxString choices_font_antialiasing[] = {
    wxT("No antialiasing"),
    wxT("Use for large fonts only"),
    wxT("Use always"),
    wxString()
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

enum
{
    Btn_View_Text_Color = OPTION_DIALOG_BUTTON_START,
    Btn_View_Background_Color,
};

BEGIN_EVENT_TABLE(OptPanel, wxPanel)
        EVT_COMMAND_RANGE(Btn_View_Text_Color, Btn_View_Background_Color, wxEVT_COMMAND_BUTTON_CLICKED, OptPanel::OnButtonClicked)
END_EVENT_TABLE()


void OptPanel::OnButtonClicked( wxCommandEvent & event )
{
    for ( int i=0; i<_opts.length(); i++ )
        if ( _opts[i]->getActionId()==event.GetId() )
            _opts[i]->onAction();
}

class OptPanelView : public OptPanel {
public:
    OptPanelView( wxWindow * parent ) {
         OptPanel::Create( parent, wxID_ANY, wxT("View options") );
    }
    virtual void CreateControls()
    {
        AddCombobox( PROP_PAGE_VIEW_MODE, wxT("View mode"), wxArrayString(3, choices_page), 1 );
        AddCombobox( PROP_FONT_ANTIALIASING, wxT("Font antialiasing"), wxArrayString(3, choices_font_antialiasing), 2 );
        AddColor( PROP_FONT_COLOR, wxT("Text color"), 0x000000, Btn_View_Text_Color );
        AddColor( PROP_BACKGROUND_COLOR, wxT("Background color"), 0xFFFFFF, Btn_View_Background_Color );
        AddFontFaceCombobox( PROP_FONT_FACE, wxT("Font face") );
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
