/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2011 Konstantin Potapov <pkbo@users.sourceforge.net>    *
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

#ifndef SELNAVIG_H_INCLUDED
#define SELNAVIG_H_INCLUDED

#include "bgfit.h"

class CRSelNavigationDialog : public BackgroundFitWindow
{
protected:
    CRViewDialog * _mainwin;
    lString16 _pattern;
    void moveBy( int delta );
public:
    CRSelNavigationDialog(  CRGUIWindowManager * wm, CRViewDialog * mainwin, lString16 pattern );

    /// returns true if command is processed
    virtual bool onCommand( int command, int params );
};

#endif
