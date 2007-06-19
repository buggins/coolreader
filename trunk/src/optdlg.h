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


class OptWindow : public wxPanel {
private:
    wxPanel * _panel;
    wxCheckBox * _cb_menu;
    wxCheckBox * _cb_toolbar;
    wxCheckBox * _cb_statusbar;
public:
    OptWindow( wxWindow * parent )
    {
        wxPanel::Create( parent, ID_OPTIONS_WINDOW  );
        _panel = new wxPanel(this, wxID_ANY, 
            wxDefaultPosition, 
            wxDefaultSize,
            wxTAB_TRAVERSAL | wxSUNKEN_BORDER);
        wxBoxSizer * border_sizer = new wxBoxSizer( wxVERTICAL );
        border_sizer->Add(
            _panel,
            1,
            wxALIGN_LEFT | wxALL,
            16); // no border and centre horizontally
        wxBoxSizer * sizer = new wxBoxSizer( wxVERTICAL );
        _cb_menu = new wxCheckBox( _panel, ID_OPTIONS_WINDOW_MENU, wxT("Show menu") );
        _cb_toolbar = new wxCheckBox( _panel, ID_OPTIONS_WINDOW_TOOLBAR, wxT("Show toolbar") );
        _cb_statusbar = new wxCheckBox( _panel, ID_OPTIONS_WINDOW_STATUSBAR, wxT("Show statusbar") );
        sizer->Add(
            new wxStaticText( _panel, ID_OPTIONS_PAGE_TITLE, wxT("Window options")),
            0,                // make vertically unstretchable
            wxALIGN_LEFT | wxALL,
            8); // no border and centre horizontally
        sizer->Add(
            _cb_menu,
            0,                // make vertically unstretchable
            wxALIGN_LEFT | wxALL,
            8); // no border and centre horizontally
        sizer->Add(
            _cb_toolbar,
            0,                // make vertically unstretchable
            wxALIGN_LEFT | wxALL,
            8); // no border and centre horizontally
        sizer->Add(
            _cb_statusbar,
            0,                // make vertically unstretchable
            wxALIGN_LEFT | wxALL,
            8); // no border and centre horizontally
        _panel->SetSizer( sizer );
        SetSizer( border_sizer );
        InitDialog();
    }
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
