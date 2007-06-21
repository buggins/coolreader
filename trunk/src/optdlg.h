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

class OptPanel : public wxPanel {
protected:
    wxStaticBoxSizer * _sizer;
public:
    OptPanel();
    void Create( wxWindow * parent, wxWindowID id, wxString title );
    virtual void CreateControls() = 0;
    virtual void PropsToControls( CRPropRef props ) = 0;
    virtual void ControlsToProps( CRPropRef props ) = 0;
};

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


class CR3OptionsDialog : public wxDialog
{
private:
    wxTreebook * _notebook;
    OptWindow * _opt_window;
    CRPropRef _props;
public:
    virtual void PropsToControls();
    virtual void ControlsToProps();
    CR3OptionsDialog( CRPropRef props );
    virtual ~CR3OptionsDialog();
    bool Create( wxWindow* parent, wxWindowID id );
};


#endif// OPTIONS_DLG_H_INCLUDED
