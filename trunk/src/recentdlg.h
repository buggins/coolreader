//
// C++ Interface: bookmarks dialog
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// bmkdlg.h

#ifndef RECENTDLG_H_INCLUDED
#define RECENTDLG_H_INCLUDED

#include "fsmenu.h"


class CRRecentBooksMenu : public CRFullScreenMenu
{
private:
    lString16 _helpText;
    int _helpHeight;
public:
    CRRecentBooksMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc);

    virtual ~CRRecentBooksMenu() { }

    virtual bool onCommand( int command, int params );
};



#endif
