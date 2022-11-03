/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2009-2011 Vadim Lopatin <coolreader.org@gmail.com>      *
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
// C++ Implementation: selection navigation dialog
//

#include "selnavig.h"

void CRSelNavigationDialog::moveBy( int delta )
{
#if CR_POCKETBOOK==1
    int pageIndex = -1;
    if ( delta==1 ) {
        // forward
        pageIndex = _mainwin->findPagesText(_pattern, 1, 1);
        if (pageIndex == -1)
            pageIndex = _mainwin->findPagesText(_pattern, -1, 1);
    } else if ( delta==-1 ) {
        // backward
        pageIndex = _mainwin->findPagesText(_pattern, 1, -1);
        if (pageIndex == -1)
            pageIndex = _mainwin->findPagesText(_pattern, -1, -1);
    }
#else
    if ( delta==1 ) {
        // forward
        if ( !_mainwin->findText(_pattern, 1, 1) )
            _mainwin->findText(_pattern, -1, 1);
    } else if ( delta==-1 ) {
        // backward
        if ( !_mainwin->findText(_pattern, 1, -1) )
            _mainwin->findText(_pattern, -1, -1);
    }
    ldomMarkedRangeList * ranges = _mainwin->getDocView()->getMarkedRanges();
    if ( ranges ) {
        if ( ranges->length()>0 ) {
            int pos = ranges->get(0)->start.y;
            _mainwin->getDocView()->SetPos(pos);
        }
    }
#endif
    setDirty();
    _mainwin->setDirty();
}

CRSelNavigationDialog::CRSelNavigationDialog(  CRGUIWindowManager * wm, CRViewDialog * mainwin, lString16 pattern )
: BackgroundFitWindow(  wm, mainwin ), _mainwin(mainwin), _pattern(pattern)
{
    _rect = _wm->getScreen()->getRect();
    _rect.top = _rect.bottom; // null height
    setAccelerators( _wm->getAccTables().get("dialog") );
    moveBy(0);
}


/// returns true if command is processed
bool CRSelNavigationDialog::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_OK:
    case MCMD_CANCEL:
        {
            _mainwin->getDocView()->clearSelection();
            _wm->closeWindow( this );
        }
        return true;
    case MCMD_SCROLL_FORWARD:
        {
            moveBy( 1 );
        }
        break;
    case MCMD_SCROLL_BACK:
        {
            moveBy( -1 );
        }
        break;
    }
    return true;
}

