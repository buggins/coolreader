/***************************************************************************
 *   CoolReader, wxWidgets GUI                                             *
 *   Copyright (C) 2007,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include "toc.h"
#include "cr3.h"

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

static int calcStringMatch( const lChar32 * str1, const lChar32 * str2 )
{
    int i;
    for ( i=0; str1[i] && str2[i] && str1[i]==str2[i]; i++ )
        ;
    return i;
}

void TocDialog::addTocItems( LVTocItem * tocitem, const wxTreeItemId & treeitem, ldomXPointer pos, wxTreeItemId & bestPosMatchNode )
{
    lString32 pos_str = pos.toString();
    for ( int i=0; i<tocitem->getChildCount(); i++ ) {
        LVTocItem * item = tocitem->getChild(i);
        wxTreeItemId id = _tree->AppendItem( treeitem, cr2wx(item->getName()), -1, -1, new MyItemData(item) );
        MyItemData * data = bestPosMatchNode.IsOk() ? (MyItemData*) _tree->GetItemData(bestPosMatchNode) : NULL;
        lString32 best_str;
        if ( data )
            best_str = data->item->getXPointer().toString();
        int best_match = calcStringMatch( pos_str.c_str(), best_str.c_str() );
        lString32 curr_str = item->getXPointer().toString();
        int curr_match = calcStringMatch( pos_str.c_str(), curr_str.c_str() );
        if ( best_str.empty() || best_match < curr_match )
            bestPosMatchNode = id;
        addTocItems( item, id, pos, bestPosMatchNode );
    }
}

TocDialog::TocDialog(wxWindow * parent, LVTocItem * toc, lString32 title, ldomXPointer currentPos )
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
    wxTreeItemId root = _tree->AddRoot( cr2wx(title) );
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
