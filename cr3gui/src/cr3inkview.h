/***************************************************************************
 *   Copyright (C) 2011 Stephan Olbrich reader42@gmx.de                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <crengine.h>
#include <crgui.h>
#include "mainwnd.h"

#include <inkview.h>

class CRInkViewScreen : public CRGUIScreenBase
{
    public:
        CRInkViewScreen(int width, int height);
        virtual void update( const lvRect & rc2, bool full );

};

class CRInkViewWindowManager : public CRGUIWindowManager
{
    public:
        CRInkViewWindowManager( int width, int height );
        virtual void update( bool fullScreenUpdate, bool forceFlushScreen=true );
        virtual void closeAllWindows();
        virtual bool getBatteryStatus( int & percent, bool & charging );
};

class CRInkViewDocView : public V3DocViewWin
{
    private:
        tocentry *_toc;
        int _tocLength;
        imenu * _menu;
        void showInkViewMenu();
        char* strconv(const char* arg1);
    public:
        CRInkViewDocView( CRGUIWindowManager * wm, lString16 dataDir )
        : V3DocViewWin( wm, dataDir ), _tocLength(0), _toc(NULL), _menu(NULL) {};
        
        virtual bool onCommand( int command, int params );
        void showContents();
        void freeContents();
        
};
