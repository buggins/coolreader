/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
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

//
// C++ Interface: links navigation dialog
//

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
