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

class OptPanel : public wxPanel {
protected:
    wxStaticBoxSizer * _sizer;
public:
    OptPanel();
    void Create( wxWindow * parent, wxWindowID id, wxString title );
    virtual void CreateControls() = 0;
};

class OptWindow : public OptPanel {
private:
    wxPanel * _panel;
    wxCheckBox * _cb_menu;
    wxCheckBox * _cb_toolbar;
    wxCheckBox * _cb_statusbar;
public:
    OptWindow( wxWindow * parent )
    {
         OptPanel::Create( parent, ID_OPTIONS_WINDOW, wxT("Window options") ); 
    }
    virtual void CreateControls();
    ~OptWindow() { }
};


class CR3OptionsDialog : public wxDialog
{
private:
    wxTreebook * _notebook;
    OptWindow * _opt_window;
public:
    CR3OptionsDialog();
    virtual ~CR3OptionsDialog();
    bool Create( wxWindow* parent, wxWindowID id );
};


#endif// OPTIONS_DLG_H_INCLUDED
