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
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected );
};

class CRBookmarkMenu : public CRFullScreenMenu
{
    bool _goToMode; // true for goTo mode, false for addMode
    LVDocView * _docview;
public:
    /// returns index of selected item, -1 if no item selected
    virtual int getSelectedItemIndex();
    void setMode( bool goToMode );
    CRBookmarkMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc, bool goToMode=false);

    virtual bool onCommand( int command, int params );
};



#endif
