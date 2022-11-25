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
// C++ Interface: document view dialog
//

#ifndef VIEWDLG_H_INCLUDED
#define VIEWDLG_H_INCLUDED

#include <crgui.h>
//#ifdef WITH_DICT
#include "dictdlg.h"
//#endif



class CRViewDialog : public  CRDocViewWindow {
protected:
    lString8 _text;
    LVStreamRef _stream;
    bool _showScroll;
    bool _showFrame;
    int _lastNavigationDirection;
    lString16 _searchPattern;
	static LVRef<CRDictionary> _dict;
    virtual void draw( int pageOffset );
    virtual void draw();
public:

    int getLastNavigationDirection() { return _lastNavigationDirection; }
    void unsetLastNavigationDirection() { _lastNavigationDirection=0; }

    void prepareNextPageImage( int offset );

    void showWaitIcon() { /* _wm->showWaitIcon( lString16("cr3_wait_icon.png") );*/ }
    CRGUIAcceleratorTableRef getMenuAccelerators()
    {
        return  _wm->getAccTables().get("menu");
    }
    CRGUIAcceleratorTableRef getDialogAccelerators()
    {
        return  _wm->getAccTables().get("dialog");
    }

    void showGoToPageDialog();

    void showGoToPercentDialog();

    bool showLinksDialog();
    /// returns true if dictionaries found, shows warning window otherwise
    bool hasDictionaries();

    void showSearchDialog();

	void showDictWithVKeyboard();

    bool findText( lString16 pattern, int origin, int direction );

    int findPagesText( lString16 pattern, int origin, int direction );

	bool findInDictionary( lString16 pattern );

    void showKeymapDialog();


    /// adds XML and FictionBook tags for utf8 fb2 document
    static lString8 makeFb2Xml( const lString8 & body );
    virtual void setRect( const lvRect & rc );
    CRViewDialog(CRGUIWindowManager * wm, lString16 title, lString8 text, lvRect rect, bool showScroll, bool showFrame );

    virtual bool onCommand( int command, int params = 0 );
};

const char * getCommandName( int command, int param );
const char * getKeyName( int keyCode, int option );

#endif
