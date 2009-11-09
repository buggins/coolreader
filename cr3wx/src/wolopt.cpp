#include "wolopt.h"

#define RENDER_TIMER_ID 123

BEGIN_EVENT_TABLE( WolOptions, wxDialog )
/*    EVT_PAINT( cr3view::OnPaint )
    EVT_SIZE    ( cr3view::OnSize )
    EVT_MOUSEWHEEL( cr3view::OnMouseWheel )
    EVT_LEFT_DOWN( cr3view::OnMouseLDown )
    EVT_MENU_RANGE( 0, 0xFFFF, cr3view::OnCommand )
    EVT_SET_FOCUS( cr3view::OnSetFocus )
    EVT_TIMER(RENDER_TIMER_ID, cr3view::OnTimer)
*/
    EVT_INIT_DIALOG(WolOptions::OnInitDialog)
END_EVENT_TABLE()

void WolOptions::OnInitDialog(wxInitDialogEvent& event)
{

}

WolOptions::WolOptions( wxWindow * parent )
{
    Create( parent, 1234, wxString(L"Choose WOL export options"),
        wxDefaultPosition, wxSize( 400, 300) );

    wxSizer * btnSizer = CreateButtonSizer( wxOK | wxCANCEL );
    wxBoxSizer * sizer = new wxBoxSizer( wxVERTICAL );

    wxString modes[2];
    modes[0] = wxString(L"Graphical, Gray (2-bit)");
    modes[1] = wxString(L"Graphical, Bitmap (1-bit)");
    wxString levels[3];
    levels[0] = wxString(L"1");
    levels[1] = wxString(L"2");
    levels[2] = wxString(L"3");

    wxGridSizer * grid = new wxFlexGridSizer(2, 2, 16, 16);

    cbMode = new wxComboBox( this, 1, modes[0], wxDefaultPosition, 
        wxDefaultSize, 2, modes, wxCB_DROPDOWN | wxCB_READONLY);
    cbLevels = new wxComboBox( this, 1, levels[2], wxDefaultPosition, 
        wxDefaultSize, 3, levels, wxCB_DROPDOWN | wxCB_READONLY);

    grid->Add( new wxStaticText( this, 2, wxString(L"Output mode")), 0, wxALIGN_LEFT );
    grid->Add( cbMode, 0, wxALIGN_LEFT );
    grid->Add( new wxStaticText( this, 2, wxString(L"Catalog max levels")), 0, wxALIGN_LEFT );
    grid->Add( cbLevels, 0, wxALIGN_LEFT );

    sizer->Add(
        grid,
        1,            // make vertically stretchable
        wxEXPAND |    // make horizontally stretchable
        wxALL,        //   and make border all around
        10 );         // set border width to 10

    sizer->Add(
        btnSizer,
        0,                // make vertically unstretchable
        wxALIGN_CENTER | wxALL,
        4); // no border and centre horizontally

    sizer->SetSizeHints( this );    

    SetSizer( sizer );
    InitDialog();
}

WolOptions::~WolOptions()
{
}
