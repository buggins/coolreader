//
// C++ Interface: links navigation dialog
//
// Description:
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// linksdlg.h

#ifndef LINKSDLG_H_INCLUDED
#define LINKSDLG_H_INCLUDED

#include "viewdlg.h"


class CRLinksDialog : public CRGUIWindowBase
{
    enum {
        BACK = 0,
        FORWARD = 1,
        LINK = 2
    };
    protected:
        int _cursorPos;
        CRViewDialog * _docwin;
        LVDocView * _docview;
        lvRect _invalidateRect;
        int _additionalButtons[2];
        int _addButtonCount;
        int _linkCount;
        int _backSize;
        int _fwdSize;
        int _currentButton;
        int _nextButton[3];
        int _prevButton[3];
        LVImageSourceRef _activeIcons[2];
        LVImageSourceRef _normalIcons[2];
        lvRect _iconRects[2];
    protected:
        virtual void Update();
        virtual void draw();
    public:
        static CRLinksDialog * create( CRGUIWindowManager * wm, CRViewDialog * docwin );
        CRLinksDialog( CRGUIWindowManager * wm, CRViewDialog * docwin );
        virtual ~CRLinksDialog() { }
        /// returns true if command is processed
        virtual bool onCommand( int command, int params );
        void invalidateCurrentSelection();
};

#endif
