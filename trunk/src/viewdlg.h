//
// C++ Interface: document view dialog
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIEWDLG_H_INCLUDED
#define VIEWDLG_H_INCLUDED

#include "mainwnd.h"


class CRViewDialog : public  CRDocViewWindow {
protected:
    lString8 _text;
    LVStreamRef _stream;
    bool _showScroll;
    bool _showFrame;
    lvRect _scrollRect;
    lvRect _clientRect;
    virtual void draw();
public:
    /// adds XML and FictionBook tags for utf8 fb2 document
    static lString8 makeFb2Xml( const lString8 & body );
    virtual void setRect( const lvRect & rc );
    CRViewDialog(CRGUIWindowManager * wm, lString16 title, lString8 text, lvRect rect, bool showScroll, bool showFrame );

    virtual bool onCommand( int command, int params = 0 );
};

#endif
