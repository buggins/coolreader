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

#ifndef CRMENU2_H
#define CRMENU2_H

#include <crgui.h>

enum CRMenuControlCmd2 {
    MCMD_NEXT=530,
    MCMD_PREV,
    MCMD_ENTER,
};


class CRMenu2 : public CRMenu
{
protected:
    int selectedItem;

public:
    virtual int getSelectedItemIndex();
    CRMenu2(CRGUIWindowManager* wm, CRMenu* parentMenu, int id, lString16 label, LVImageSourceRef image, LVFontRef defFont, LVFontRef valueFont, CRPropRef props = CRPropRef(), const char* propName = 0, int pageItems = 8);
    CRMenu2(CRGUIWindowManager* wm, CRMenu* parentMenu, int id, const char* label, LVImageSourceRef image, LVFontRef defFont, LVFontRef valueFont, CRPropRef props = CRPropRef(), const char* propName = 0, int pageItems = 8);
    virtual bool onCommand(int command, int params = 0);
};

#endif // CRMENU2_H
