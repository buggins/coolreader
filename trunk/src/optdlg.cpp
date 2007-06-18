// options dialog, implementation
#include <wx/wx.h>
#include <crengine.h>
#include "cr3.h"
#include "optdlg.h"


CR3OptionsDialog::CR3OptionsDialog()
: _notebook(NULL), _opt_window(NULL)
{
}

CR3OptionsDialog::~CR3OptionsDialog()
{
    if ( _notebook ) {
        delete _notebook;
        _notebook = NULL;
    }
}


bool CR3OptionsDialog::Create( wxWindow* parent, wxWindowID id )
{
    bool res = wxDialog::Create( parent, id, wxString(wxT("Options")), 
        wxDefaultPosition, wxDefaultSize, 
        wxDEFAULT_DIALOG_STYLE, wxString(wxT("dialogBox")));
    if ( res ) {

        _notebook = new wxTreebook( this, ID_OPTIONS_TREEBOOK );
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

        _opt_window = new OptWindow( this );
        _notebook->InsertPage(0, _opt_window, wxT("Window") );
        _notebook->InsertPage(1, NULL, wxT("Book") );
        _notebook->InsertPage(2, NULL, wxT("Styles") );
        _notebook->InsertSubPage(2, NULL, wxT("Body") );
        _notebook->InsertSubPage(2, NULL, wxT("Paragraph") );

        //sizer->SetSizeHints( this );
        SetSizer( sizer );
        InitDialog();
        
    }
    return res;
}
