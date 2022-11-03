/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2011 Konstantin Potapov <pkbo@users.sourceforge.net>    *
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

#ifndef RECENTDLG_H_INCLUDED
#define RECENTDLG_H_INCLUDED

#include "fsmenu.h"

class CRRecentBooksMenu : public CRFullScreenMenu
{
private:
    lString16 _helpText;
    int _helpHeight;
    LVPtrVector<CRFileHistRecord> * _files;
    bool removeItem( int index );
public:
    CRRecentBooksMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc);

    virtual ~CRRecentBooksMenu() { }

#ifdef CR_POCKETBOOK
    void showContextMenu();
    void handleContextMenu(int index);
#endif
    virtual bool onCommand( int command, int params );
};



#endif
