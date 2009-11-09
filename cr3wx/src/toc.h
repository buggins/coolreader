//
// C++ Interface: toc
//
// Description: 
//
//
// Author: Vadim Lopatin <Vadim.Lopatin@coolreader.org>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _CR3_TOC_H_
#define _CR3_TOC_H_

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <crengine.h>

/**
 * @short WOL export options window
 */

class
TocDialog : public wxDialog
{
    private:
        wxTreeCtrl * _tree;
        LVTocItem *  _toc;
        LVTocItem *  _selection;
        void addTocItems( LVTocItem * tocitem, const wxTreeItemId & treeitem, ldomXPointer pos, wxTreeItemId & bestPosMatchNode );
    public:
        TocDialog( wxWindow * parent, LVTocItem * toc, lString16 title, ldomXPointer currentPos );
        virtual ~TocDialog();
        LVTocItem * getSelection() { return _selection; }
        void OnInitDialog(wxInitDialogEvent& event);
        void OnItemActivated(wxTreeEvent& event);
        void OnSelChanged(wxTreeEvent& event);
    private:
        DECLARE_EVENT_TABLE()
};

#endif // _CR3_TOC_H_
