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
// C++ Interface: fullscreen menu dialog
//

#ifndef FSMENU_H_INCLUDED
#define FSMENU_H_INCLUDED

#include <crgui.h>

class CRFullScreenMenu : public CRMenu
{
public:
    CRFullScreenMenu(CRGUIWindowManager * wm, int id, const lString16 & caption, int numItems, lvRect & rc);

    virtual const lvRect & getRect();

    virtual lvPoint getMaxItemSize();

    virtual lvPoint getSize();

    virtual lString16 getItemNumberKeysName();

    virtual lString16 getCommandKeyName( int cmd, int param=0 );

    //virtual void Draw( LVDrawBuf & buf, int x, int y );

};


#endif
