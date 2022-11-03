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
        TocDialog( wxWindow * parent, LVTocItem * toc, lString32 title, ldomXPointer currentPos );
        virtual ~TocDialog();
        LVTocItem * getSelection() { return _selection; }
        void OnInitDialog(wxInitDialogEvent& event);
        void OnItemActivated(wxTreeEvent& event);
        void OnSelChanged(wxTreeEvent& event);
    private:
        DECLARE_EVENT_TABLE()
};

#endif // _CR3_TOC_H_
