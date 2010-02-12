//
// C++ Interface: fullscreen menu dialog
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// fsmenu.h

#ifndef FSMENU_H_INCLUDED
#define FSMENU_H_INCLUDED

#include <crgui.h>


class CRFullScreenMenu : public CRMenu
{
protected:
    lString16 _helpText;
public:
    CRFullScreenMenu(CRGUIWindowManager * wm, int id, const lString16 & caption, int numItems, lvRect & rc);

    virtual const lvRect & getRect();

    virtual lvPoint getMaxItemSize();

    virtual lvPoint getSize();

    //virtual void Draw( LVDrawBuf & buf, int x, int y );

};


#endif
