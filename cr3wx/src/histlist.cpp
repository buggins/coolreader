/***************************************************************************
 *   CoolReader, wxWidgets GUI                                             *
 *   Copyright (C) 2007,2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
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

#include <wx/wx.h>
#include <wx/power.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/listctrl.h>
#include <crengine.h>
#include "cr3.h"
#include "rescont.h"
#include "histlist.h"

BEGIN_EVENT_TABLE( HistList, wxListView )
//    EVT_PAINT( cr3view::OnPaint )
//    EVT_SIZE    ( cr3view::OnSize )
//    EVT_MOUSEWHEEL( cr3view::OnMouseWheel )
//    EVT_LEFT_DOWN( cr3view::OnMouseLDown )
//    EVT_RIGHT_DOWN( cr3view::OnMouseRDown )
//    EVT_MENU_RANGE( 0, 0xFFFF, cr3view::OnCommand )
//    EVT_SET_FOCUS( cr3view::OnSetFocus )
//    EVT_TIMER(RENDER_TIMER_ID, cr3view::OnTimer)
//    EVT_TIMER(CLOCK_TIMER_ID, cr3view::OnTimer)
//    EVT_INIT_DIALOG(cr3view::OnInitDialog)
END_EVENT_TABLE()

HistList::HistList()
: _records(NULL)
{
}

HistList::~HistList()
{
    if ( _records )
        delete _records;
}

void HistList::SetRecords(LVPtrVector<CRFileHistRecord> & records )
{
    if ( _records )
        delete _records;
    _records = new LVPtrVector<CRFileHistRecord>( records );
    SetItemCount(_records->length());
    UpdateColumns();
    if ( GetItemCount()>0 ) {
        Select(0);
    }
}

void HistList::UpdateColumns()
{
    SetColumnWidth(0, wxLIST_AUTOSIZE);
    SetColumnWidth(1, wxLIST_AUTOSIZE);
    SetColumnWidth(2, wxLIST_AUTOSIZE);
    Layout();
}

bool HistList::Create(wxWindow* parent, wxWindowID id )
{
    bool res = wxListView::Create(parent, id, wxDefaultPosition, wxDefaultSize, 
        wxLC_REPORT | wxLC_VIRTUAL | wxLC_SINGLE_SEL );
    wxListItem col1;
    //wxListItem col2;
    //col1.SetColumn(0);
    col1.SetText(wxString(wxT("Last time")));
    //col1.SetWidth(wxLIST_AUTOSIZE);
    col1.SetAlign(wxLIST_FORMAT_CENTRE);
    InsertColumn( 0, col1 );

    col1.SetAlign(wxLIST_FORMAT_LEFT);
    col1.SetText(wxString(wxT("Book")));
    InsertColumn( 1, col1 );
    col1.SetText(wxString(wxT("Pos")));
    InsertColumn( 2, col1 );
    SetItemCount(20);

    UpdateColumns();

    Update();

    //SetColumnWidth(0, wxLIST_AUTOSIZE);
    //col1.
    //col2.SetColumn(1);
    //SetColumnWidth(1, wxLIST_AUTOSIZE);
    //SetColumn( 0, col1 );
    //SetColumn( 1, col2 );
    //SetColumn( 2, col3 );
    return res;
}

wxString HistList::OnGetItemText(long item, long column) const
{
    if ( _records && item>=0 && item<_records->length() ) {
        CRFileHistRecord * rec = (*_records)[item];
        lString32 data;
        switch ( column ) {
        case 0:
            data = rec->getLastTimeString();
            break;
        case 1:
            {
                lString32 fname = rec->getFileName();
                lString32 author = rec->getAuthor();
                lString32 title = rec->getTitle();
                lString32 series = rec->getSeries();
                if ( !series.empty() ) {
                    if ( !title.empty() )
                        title << " ";
                    title << series;
                }
                if ( !author.empty() && !title.empty() )
                    author << ". ";
                data << author << title;
                if ( data.empty() )
                    data = fname;
            }
            break;
        case 2:
            {
                data = lString32::itoa(rec->getLastPos()->getPercent()/100) + "%";
            }
            break;
        }
        return cr2wx(data);
    } else
        return wxString(wxT(""));
}

