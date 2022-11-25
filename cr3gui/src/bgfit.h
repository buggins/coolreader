/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2009 Alexander V. Nikolaev <avn@daemon.hole.ru>         *
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
// C++ Interface: selection navigation dialog
//

#ifndef BIGFIT_H_INCLUDED
#define BIGFIT_H_INCLUDED

#include "mainwnd.h"

/// window to show on top of DocView window, shifting/stretching DOCView content to fit
class BackgroundFitWindow : public CRGUIWindowBase
{
protected:
    CRDocViewWindow * _mainwin;
    virtual void draw();
public:
    BackgroundFitWindow(  CRGUIWindowManager * wm, CRDocViewWindow * mainwin )
    : CRGUIWindowBase(wm),
        _mainwin(mainwin)
    {
        _fullscreen = true;
    }
};

#endif
