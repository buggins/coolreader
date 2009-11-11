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
#define PROP_WINDOW_ROTATE_ANGLE    "window.rotate.angle"
#define PROP_WINDOW_TOOLBAR_SIZE    "window.toolbar.size"
#define PROP_WINDOW_TOOLBAR_POSITION "window.toolbar.position"
#define PROP_WINDOW_SHOW_STATUSBAR  "window.statusbar.show"

#define PROP_PAGE_HEADER_ENABLED     "page.header.enabled"
#define PROP_PAGE_HEADER_PAGE_NUMBER "page.header.pagenumber"
#define PROP_PAGE_HEADER_PAGE_COUNT  "page.header.pagecount"
#define PROP_PAGE_HEADER_CLOCK       "page.header.clock"
#define PROP_PAGE_HEADER_BATTERY     "page.header.battery"
#define PROP_PAGE_HEADER_AUTHOR      "page.header.author"
#define PROP_PAGE_HEADER_TITLE       "page.header.title"

//#define PROP_PAGE_VIEW_MODE          "page.view.mode"
#define PROP_FONT_ANTIALIASING       "font.antialiasing.mode"
#define PROP_FONT_COLOR              "font.color.default"
#define PROP_BACKGROUND_COLOR        "background.color.default"
#define PROP_FONT_FACE               "font.face.default"

#define PROP_APP_OPEN_LAST_BOOK      "app.init.open-recent"

class PropOption {
protected:
    const char * _option;
    wxWindow * _control;
public:
    PropOption( wxWindow * control, const char * option )
    : _option(option), _control(control) { }
    virtual ~PropOption() { }
    virtual void ControlToOption( CRPropRef props ) = 0;
    virtual void OptionToControl( CRPropRef props ) = 0;
    virtual int getActionId() { return 0; }
    virtual void onAction() { }
};

class OptPanel : public wxPanel {
protected:
    wxStaticBoxSizer * _sizer;
    LVPtrVector<PropOption> _opts;
    wxWindow * AddControl(wxWindow * control);
    wxCheckBox * AddCheckbox(const char * option, wxString caption, bool defValue );
    wxComboBox * AddCombobox(const char * option, wxString caption, const wxArrayString & options, int defValue, bool storeValues = false );
    wxComboBox * AddFontFaceCombobox( const char * option, wxString caption );
    wxPanel * AddColor( const char * option, wxString caption, lvColor defValue, int buttonId );
    void OnButtonClicked( wxCommandEvent & event );
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
protected:
    DECLARE_EVENT_TABLE()
};

class CR3OptionsDialog : public wxDialog
{
private:
    wxTreebook * _notebook;
    OptPanel * _opt_window;
    OptPanel * _opt_page;
    OptPanel * _opt_view;
    OptPanel * _opt_app;
    CRPropRef _props;
    CRPropRef _oldprops;
public:
    CRPropRef getNewProps() { return _props; }
    CRPropRef getOldProps() { return _oldprops; }
    CRPropRef getChangedProps() { return _oldprops ^ _props; }
    virtual void PropsToControls();
    virtual void ControlsToProps();
    CR3OptionsDialog( CRPropRef props );
    virtual ~CR3OptionsDialog();
    bool Create( wxWindow* parent, wxWindowID id );
};

#define OPTION_DIALOG_BUTTON_START 2000
#define OPTION_DIALOG_BUTTON_END   2099


#endif// OPTIONS_DLG_H_INCLUDED
