//
// C++ Implementation: toc
//
// Description: 
//
//
// Author: Vadim Lopatin <Vadim.Lopatin@coolreader.org>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "toc.h"

#define TREE_ID 2345

BEGIN_EVENT_TABLE( TocDialog, wxDialog )
    EVT_INIT_DIALOG(TocDialog::OnInitDialog)
    EVT_TREE_SEL_CHANGED(TREE_ID, TocDialog::OnSelChanged)
    EVT_TREE_ITEM_ACTIVATED(TREE_ID, TocDialog::OnItemActivated)
END_EVENT_TABLE()

void TocDialog::OnInitDialog(wxInitDialogEvent& event)
{

}

class MyItemData : public wxTreeItemData
{
public:
    LVTocItem * item;
    MyItemData( LVTocItem * p ) : item(p) {}
};

void TocDialog::OnSelChanged(wxTreeEvent& event)
{
    wxTreeItemId id = _tree->GetSelection();
    MyItemData * data = (MyItemData*) _tree->GetItemData(id);
    if ( _selection )
        _selection = data->item;
}

void TocDialog::OnItemActivated(wxTreeEvent& event)
{
    OnSelChanged(event);
    EndModal(wxID_OK);
}

void TocDialog::addTocItems( LVTocItem * tocitem, wxTreeItemId treeitem )
{
    for ( int i=0; i<tocitem->getChildCount(); i++ ) {
        LVTocItem * item = tocitem->getChild(i);
        wxTreeItemId id = _tree->AppendItem( treeitem, wxString(item->getName().c_str()), -1, -1, new MyItemData(item) );
        addTocItems( item, id );
    }
}

TocDialog::TocDialog( wxWindow * parent, LVTocItem * toc )
: _selection(NULL)
{
    Create( parent, 1234, wxString(L"Table of Contents"),
        wxDefaultPosition, wxSize( 400, 400), wxRESIZE_BORDER );
    _toc = toc;
    _tree = new wxTreeCtrl( this, TREE_ID, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_SINGLE | wxTR_HIDE_ROOT );
    wxTreeItemId root = _tree->AddRoot( wxString(L"Contents") );
    addTocItems( toc, root );

    wxSizer * btnSizer = CreateButtonSizer( wxOK | wxCANCEL );
    wxBoxSizer * sizer = new wxBoxSizer( wxVERTICAL );

    sizer->Add(
        _tree,
        1,            // make vertically stretchable
        wxEXPAND |    // make horizontally stretchable
        wxALL,        //   and make border all around
        10 );         // set border width to 10

    sizer->Add(
        btnSizer,
        0,                // make vertically unstretchable
        wxALIGN_CENTER | wxALL,
        4); // no border and centre horizontally

    //sizer->SetSizeHints( this );
    SetSizer( sizer );
    InitDialog();
}

TocDialog::~TocDialog()
{
}
