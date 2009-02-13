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

#ifndef BMKDLG_H_INCLUDED
#define BMKDLG_H_INCLUDED

#include "fsmenu.h"


class CRBookmarkMenuItem : public CRMenuItem
{
private:
    CRBookmark * _bookmark;
    int _page;
public:
    CRBookmarkMenuItem( CRMenu * menu, int shortcut, CRBookmark * bookmark, int page );
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected );
};

class CRBookmarkMenu : public CRFullScreenMenu
{
public:
    CRBookmarkMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc);

    virtual bool onCommand( int command, int params );
};



#endif
