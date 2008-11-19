/***************************************************************************
 *   Copyright (C) 2007 by Vadim Lopatin   *
 *   vadim.lopatin@coolreader.org   *
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

#include "lvtypes.h"
#include "lvptrvec.h"
#include "lvdrawbuf.h"

class CRGUIWindowManager;

/// Screen object - provides canvas and interface to device screen
class CRGUIScreen
{
    public:
        /// creates compatible canvas of specified size
        virtual LVDrawBuf * createCanvas( int dx, int dy ) = 0;
        /// returns screen width
        virtual int getWidth() = 0;
        /// returns screen height
        virtual int getHeight() = 0;
        /// returns screen dimension
        virtual lvRect getRect() { return lvRect(0, 0, getWidth(), getHeight() ); }
        /// return pointer to screen canvas
        virtual LVDrawBuf * getCanvas() = 0;
        /// draw image on screen canvas
        virtual void draw( LVDrawBuf * img, int x = 0, int y = 0) = 0;
        /// transfers contents of buffer to device, if full==true, redraws whole screen, otherwise only changed area
        virtual void update( bool full ) { }
        /// invalidates rectangle: add it to bounding box of next partial update
        virtual void invalidateRect( lvRect rc ) { }
        virtual ~CRGUIScreen() { }
};

/// Window interface
class CRGUIWindow
{
    public:
        /// returns true if key is processed
        virtual bool onKeyPressed( int key ) = 0;
        /// returns true if window is visible
        virtual bool isVisible() = 0;
        /// returns true if window is changed but now drawn
        virtual bool isDirty() = 0;
        /// sets dirty flag (true means window is changed but now drawn)
        virtual void setDirty( bool dirty ) = 0;
        /// shows or hides window
        virtual void setVisible( bool visible ) = 0;
        /// returns window rectangle
        virtual const lvRect & getRect() const = 0;
        /// sets window rectangle
        virtual void setRect( lvRect rc ) = 0;
        /// draws content of window to screen
        virtual void draw() = 0;
        /// called if window gets focus
        virtual void activated() = 0;
        /// called if window loss focus
        virtual void covered() = 0;
        /// returns window manager
        virtual CRGUIWindowManager * getWindowManager() = 0;
        /// destroys window
        virtual ~CRGUIWindow() { }
};

/// Window manager
class CRGUIWindowManager
{
    protected:
        LVPtrVector<CRGUIWindow, true> _windows;
        CRGUIScreen * _screen;
    public:
        /// returns true if key is processed
        virtual bool onKeyPressed( int key )
        {
            for ( int i=_windows.length()-1; i>=0; i-- ) {
                if ( _windows[i]->onKeyPressed( key ) )
                    return true;
            }
            return false;
        }
        /// activates window, brings it on top; add to stack if not added
        void activateWindow( CRGUIWindow * window )
        {
            int index = _windows.indexOf( window );
            CRGUIWindow * lostFocus = _windows.peek();
            if ( index < 0 ) {
                _windows.push( window );
            } else if ( index < _windows.length() - 1 ) {
                _windows.push( _windows.remove( index ) );
            }
            if ( window != lostFocus )
            {
                if ( lostFocus )
                    lostFocus->covered();
                window->activated();
            }
        }
        /// closes window, removes from stack, destroys object
        void closeWindow( CRGUIWindow * window )
        {
            int index = _windows.indexOf( window );
            if ( index >= 0 ) {
                if ( window == _windows.peek() )
                    window->covered(); // send cover before close
                _windows.remove( index );
            }
            delete window;
        }
        /// redraw one window
        void updateWindow( CRGUIWindow * window )
        {
            int index = _windows.indexOf( window );
            if ( index < 0 )
                return;
            lvRect coverBox;
            if  ( _windows.empty() )
                return;
            LVPtrVector<CRGUIWindow, false> drawList;
            for ( int i=_windows.length()-1; i>=index; i-- ) {
                if ( !_windows[i]->isVisible() )
                    continue;
                lvRect rc = _windows[i]->getRect();
                if ( coverBox.isRectInside( rc ) )
                    continue; // fully covered by top window
                if ( !rc.isEmpty() )
                    drawList.add( _windows[i] );
                if ( !rc.isRectInside( coverBox ) )
                    coverBox = rc;
            }
            while ( !drawList.empty()  ) {
                CRGUIWindow * w = drawList.pop();
                if ( w->isDirty() ) {
                    w->draw();
                    _screen->invalidateRect( w->getRect() );
                    w->setDirty( false );
                }
            }
        /// invalidates rectangle: add it to bounding box of next partial update
            _screen->update( false );
        }
        /// full redraw of all windows
        void update( bool fullScreenUpdate )
        {
            lvRect coverBox;
            if  ( _windows.empty() )
                return;
            LVPtrVector<CRGUIWindow, false> drawList;
            for ( int i=_windows.length()-1; i>=0; i-- ) {
                if ( !_windows[i]->isVisible() )
                    continue;
                lvRect rc = _windows[i]->getRect();
                if ( coverBox.isRectInside( rc ) )
                    continue; // fully covered by top window
                if ( !rc.isEmpty() )
                    drawList.add( _windows[i] );
                if ( !rc.isRectInside( coverBox ) )
                    coverBox = rc;
            }
            while ( !drawList.empty()  ) {
                CRGUIWindow * w = drawList.pop();
                if ( w->isDirty() || fullScreenUpdate ) {
                    w->draw();
                    _screen->invalidateRect( w->getRect() );
                    w->setDirty( false );
                }
            }
            _screen->update( fullScreenUpdate );
        }
        /// returns screen associated with window manager
        CRGUIScreen * getScreen()
        {
            return _screen;
        }
        CRGUIWindowManager()
        {
        }
        virtual void closeAllWindows()
        {
            for ( int i=_windows.length()-1; i>=0; i-- ) {
                closeWindow(_windows[i]);
            }
        }
        /// destroy all windows on close
        virtual ~CRGUIWindowManager()
        {
            closeAllWindows();
        }
};

/// Window base implementation
class CRGUIWindowBase : public CRGUIWindow
{
    protected:
        CRGUIWindowManager * _wm;
        lvRect _rect;
        bool _visible;
        bool _dirty;
    public:
        /// returns true if window is changed but now drawn
        virtual bool isDirty()
        {
            return _dirty;
        }
        /// sets dirty flag (true means window is changed but now drawn)
        virtual void setDirty( bool dirty )
        {
            _dirty = dirty;
        }
        /// shows or hides window
        virtual void setVisible( bool visible ) = 0;
        virtual bool isVisible() const { return true; }
        virtual const lvRect & getRect() const { return _rect; }
        virtual void setRect( lvRect rc ) { _rect = rc; }
        virtual void draw() { _dirty = false; }
        virtual CRGUIWindowManager * getWindowManager() { return _wm; }
        CRGUIWindowBase( CRGUIWindowManager * wm )
        : _wm(wm), _visible(true), _dirty(true)
        {
            // fullscreen visible by default
            _rect = _wm->getScreen()->getRect();
        }
        virtual ~CRGUIWindowBase() { }
};

