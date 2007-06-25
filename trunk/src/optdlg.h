// Options dialog
#ifndef OPTIONS_DLG_H_INCLUDED
#define OPTIONS_DLG_H_INCLUDED


#include <wx/treebook.h>


enum {
    ID_OPTIONS_TREEBOOK,
    ID_OPTIONS_WINDOW,
    ID_OPTIONS_WINDOW_MENU,
    ID_OPTIONS_WINDOW_STATUSBAR,
    ID_OPTIONS_WINDOW_TOOLBAR,
    ID_OPTIONS_PAGE_TITLE,
};

#define PROP_CRENGINE_FONT_SIZE     "crengine.font.size"
#define PROP_WINDOW_RECT            "window.rect"
#define PROP_WINDOW_FULLSCREEN      "window.fullscreen"
#define PROP_WINDOW_MINIMIZED       "window.minimized"
#define PROP_WINDOW_MAXIMIZED       "window.maximized"
#define PROP_WINDOW_SHOW_MENU       "window.menu.show"
#define PROP_WINDOW_TOOLBAR_SIZE    "window.toolbar.size"
#define PROP_WINDOW_SHOW_STATUSBAR  "window.statusbar.show"

class PropOption {
protected:
    const char * _option;
    wxWindow * _control;
public:
    PropOption( wxWindow * control, const char * option )
    : _option(option), _control(control) { }
    virtual ~PropOption() { }
    virtual void ControlToOption() = 0;
    virtual void OptionToControl() = 0;
};

class BoolOption : public PropOption {
private:
    bool _defvalue;
public:
    BoolOption( wxCheckBox * control, const char * option, bool defvalue )
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
    const wxChar * _choices;
    int _size;
public:
    ComboBoxOption( wxComboBox * control, const char * option, int defvalue, const wxChar * choices )
    : PropOption( control, option ), _defvalue(defvalue), _choices(choices)
    {
        for ( _size=0; _choices[_size]; _size++ )
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

class OptPanel : public wxPanel {
protected:
    wxStaticBoxSizer * _sizer;
    LVPtrVector<PropOption> _opts;
    wxWindow * AddControl(wxWindow * control);
    wxCheckBox * AddCheckbox(wxString caption);
    wxComboBox * AddCombobox(wxString caption, wxString options[], int size, int selection );
    
public:
    OptPanel();
    void Create( wxWindow * parent, wxWindowID id, wxString title );
    virtual void CreateControls() = 0;
    virtual void PropsToControls( CRPropRef props )
    {
        for ( int i=0; i<_opts.length(); i++ ) {
            _opts.get(i)->OptionToControl( props );
        }
    }
    virtual void ControlsToProps( CRPropRef props )
    {
        for ( int i=0; i<_opts.length(); i++ ) {
            _opts.get(i)->ControlToOption( props );
        }
    }
};


class CR3OptionsDialog : public wxDialog
{
private:
    wxTreebook * _notebook;
    OptPanel * _opt_window;
    OptPanel * _opt_page;
    CRPropRef _props;
public:
    virtual void PropsToControls();
    virtual void ControlsToProps();
    CR3OptionsDialog( CRPropRef props );
    virtual ~CR3OptionsDialog();
    bool Create( wxWindow* parent, wxWindowID id );
};


#endif// OPTIONS_DLG_H_INCLUDED
