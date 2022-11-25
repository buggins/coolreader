/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
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
// C++ Interface: number editor dialog
//

#ifndef NUMEDIT_H_INCLUDED
#define NUMEDIT_H_INCLUDED


#include "mainwnd.h"

class CRNumberEditDialog : public CRGUIWindowBase
{
    protected:
        lString16 _title;
        lString16 _value;
        int _minvalue;
        int _maxvalue;
        int _resultCmd;
        CRMenuSkinRef _skin;
        virtual void draw();
    public:
        CRNumberEditDialog( CRGUIWindowManager * wm, lString16 title, lString16 initialValue, int resultCmd, int minvalue, int maxvalue );
        virtual ~CRNumberEditDialog()
        {
        }
        bool digitEntered( lChar16 c );
        /// returns true if command is processed
        virtual bool onCommand( int command, int params );
};


#endif
