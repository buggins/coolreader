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
    CRBookmark * getBookmark() { return _bookmark; }
    void setBookmark(CRBookmark *bookmark) { _bookmark = bookmark; _itemDirty = true; }
    int getPage() { return _page; }
};

class CRBookmarkMenu : public CRFullScreenMenu
{
protected:
    bool _goToMode; // true for goTo mode, false for addMode
    LVDocView * _docview;
public:
    /// returns index of selected item, -1 if no item selected
    virtual int getSelectedItemIndex();
    void setMode( bool goToMode );
    CRBookmarkMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc, bool goToMode=false);
#ifdef CR_POCKETBOOK
    virtual int getDefaultSelectionIndex();
    void showContextMenu();
    void handleContextMenu(int index);
#endif
    virtual bool onCommand( int command, int params );
};

class CRCitesMenu : public CRFullScreenMenu
{
protected:
    LVDocView * _docview;
    void goToCitePage(int selecteditem);
    void createDefaultItem();
public:
    /// returns index of selected item, -1 if no item selected
    virtual int getSelectedItemIndex();
    CRCitesMenu(CRGUIWindowManager * wm, LVDocView * docview, int numItems, lvRect & rc);
#ifdef CR_POCKETBOOK
    void showContextMenu();
    void handleContextMenu(int index);
#endif
    virtual bool onCommand( int command, int params );
};

#endif
