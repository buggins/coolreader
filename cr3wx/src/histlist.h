/***************************************************************************
 *   CoolReader, wxWidgets GUI                                             *
 *   Copyright (C) 2007,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef _HISTLIST_H_
#define _HISTLIST_H_

#include <wx/listctrl.h>

/**
 * @short XML Document View window
 * @author Vadim Lopatin <vadim.lopatin@coolreader.org>
 * @version 0.1
 */

class
HistList : public wxListView
{
    private:
        LVPtrVector<CRFileHistRecord> * _records;
    public:
        HistList();
        virtual ~HistList();
        virtual bool Create(wxWindow* parent, wxWindowID id );
        virtual wxString OnGetItemText(long item, long column) const;
        void SetRecords(LVPtrVector<CRFileHistRecord> & records );
        void UpdateColumns();
    protected:
    private:
        DECLARE_EVENT_TABLE()
};

#endif // _CR3VIEW_H_
