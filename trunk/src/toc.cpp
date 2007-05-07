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
    MyItemData * data = id.IsOk() ? (MyItemData*) _tree->GetItemData(id) : NULL;
    if ( data )
        _selection = data->item;
    else
        _selection = NULL;
}

void TocDialog::OnItemActivated(wxTreeEvent& event)
{
    OnSelChanged(event);
    if ( _selection!=NULL )
        EndModal(wxID_OK);
}

static int calcStringMatch( const lChar16 * str1, const lChar16 * str2 )
{
    int i;
    for ( i=0; str1[i] && str2[i] && str1[i]==str2[i]; i++ )
        ;
    return i;
}

void TocDialog::addTocItems( LVTocItem * tocitem, const wxTreeItemId & treeitem, ldomXPointer pos, wxTreeItemId & bestPosMatchNode )
{
    lString16 pos_str = pos.toString();
    for ( int i=0; i<tocitem->getChildCount(); i++ ) {
        LVTocItem * item = tocitem->getChild(i);
        wxTreeItemId id = _tree->AppendItem( treeitem, wxString(item->getName().c_str()), -1, -1, new MyItemData(item) );
        MyItemData * data = bestPosMatchNode.IsOk() ? (MyItemData*) _tree->GetItemData(bestPosMatchNode) : NULL;
        lString16 best_str;
        if ( data )
            best_str = data->item->getXPointer().toString();
        int best_match = calcStringMatch( pos_str.c_str(), best_str.c_str() );
        lString16 curr_str = item->getXPointer().toString();
        int curr_match = calcStringMatch( pos_str.c_str(), curr_str.c_str() );
        if ( best_str.empty() || best_match < curr_match )
            bestPosMatchNode = id;
        addTocItems( item, id, pos, bestPosMatchNode );
    }
}

TocDialog::TocDialog( wxWindow * parent, LVTocItem * toc, lString16 title, ldomXPointer currentPos )
: _selection(NULL)
{
    Create( parent, 1234, wxString(L"Table of Contents"),
        wxDefaultPosition, wxSize( 400, 450), wxRESIZE_BORDER );
    _toc = toc;
    _tree = new wxTreeCtrl( this, TREE_ID, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS
        | wxTR_SINGLE
//        | wxTR_HIDE_ROOT
        );
    wxTreeItemId root = _tree->AddRoot( wxString(title.c_str()) );
    wxTreeItemId bestItem;
    addTocItems( toc, root, currentPos, bestItem );

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

    _tree->SelectItem( bestItem );
    _tree->EnsureVisible( bestItem );
}

TocDialog::~TocDialog()
{
}
