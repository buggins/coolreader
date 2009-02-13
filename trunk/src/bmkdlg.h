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

#include "mainwnd.h"


class CRBookmarkMenuItem : public CRMenuItem
{
private:
    CRBookmark * _bookmark;
    int _page;
public:
    CRBookmarkMenuItem( CRMenu * menu, int shortcut, CRBookmark * bookmark, int page );
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected );
};

class CRBookmarkMenu : public CRMenu
{
private:
    lString16 _helpText;
    int _helpHeight;
public:
    CRBookmarkMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc);

    virtual const lvRect & getRect();

    virtual int getItemHeight();

    virtual lvPoint getMaxItemSize();

    virtual lvPoint getSize();

    virtual void Draw( LVDrawBuf & buf, int x, int y );

    virtual bool onCommand( int command, int params );
};



#endif
