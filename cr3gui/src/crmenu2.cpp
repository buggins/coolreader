/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Stephan Olbrich <stephan@albapasser.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "crmenu2.h"

CRMenu2::CRMenu2(CRGUIWindowManager* wm, CRMenu* parentMenu, int id, lString16 label, LVImageSourceRef image, LVFontRef defFont, LVFontRef valueFont, CRPropRef props, const char* propName, int pageItems)
: CRMenu(wm, parentMenu, id, label, image, defFont, valueFont, props, propName, pageItems)
{
    selectedItem = -1;
}

CRMenu2::CRMenu2(CRGUIWindowManager* wm, CRMenu* parentMenu, int id, const char* label, LVImageSourceRef image, LVFontRef defFont, LVFontRef valueFont, CRPropRef props, const char* propName, int pageItems)
: CRMenu(wm, parentMenu, id, label, image, defFont, valueFont, props, propName, pageItems)
{
    selectedItem = -1;
}

int CRMenu2::getSelectedItemIndex()
{
    if (selectedItem == -1)
        selectedItem = CRMenu::getSelectedItemIndex();
    return selectedItem;
}

bool CRMenu2::onCommand(int command, int params)
{
    int old = selectedItem;
    int pos;
    if ( command==MCMD_NEXT ) {
        if (selectedItem < (_items.length()-1) )
            selectedItem++;
        if (selectedItem == (_topItem + _pageItems))
            return onCommand(MCMD_SCROLL_FORWARD, 1);
        if (old!=selectedItem)
        {
            setDirty();
            _wm->updateWindow(this);
            return true;
        }
    }
    if ( command==MCMD_PREV ) {
        if (selectedItem > 0 )
            selectedItem--;
        if (selectedItem < _topItem)
            return onCommand(MCMD_SCROLL_BACK, 1);
        if (old!=selectedItem)
        {
            setDirty();
            _wm->updateWindow(this);
            return true;
        }
    }
    if ( command==MCMD_ENTER ) {
        pos = selectedItem - _topItem;
        if (pos < 0)
            return true;
        else if (pos == 9)
            return onCommand(MCMD_SELECT_0, 0);
        else
            return onCommand(MCMD_SELECT_1 + pos, 0);
    }
    return CRMenu::onCommand(command, params);
}
