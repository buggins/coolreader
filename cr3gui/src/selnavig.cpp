//
// C++ Implementation: selection navigation dialog
//
// Description:
//      Shows keyboard, and allows to input text string
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
// selnavig.cpp

#include "selnavig.h"

void CRSelNavigationDialog::moveBy( int delta )
{
    int newIndex = _curIndex + delta;
    if ( newIndex < 0 )
        newIndex = _selPages.length() - 1;
    if ( newIndex >= _selPages.length() || newIndex < 0 )
        newIndex = 0;
    _curIndex = newIndex;
    if ( _curIndex < _selPages.length() ) {
        _mainwin->getDocView()->goToPage( _selPages[_curIndex] );
        setDirty();
        _mainwin->setDirty();
    }
}

CRSelNavigationDialog::CRSelNavigationDialog(  CRGUIWindowManager * wm, CRViewDialog * mainwin )
: BackgroundFitWindow(  wm, mainwin )
{
    _rect = _wm->getScreen()->getRect();
    _rect.top = _rect.bottom; // null height
    setAccelerators( _wm->getAccTables().get("dialog") );

    ldomMarkedRangeList * marks = _mainwin->getDocView()->getMarkedRanges();
    LVRendPageList * pages = _mainwin->getDocView()->getPageList();

    int lastPage = -1;
    _curIndex = -1;
    int cp = _mainwin->getDocView()->getCurPage();
    for ( int i=0; i<marks->length(); i++ ) {
        //
        ldomMarkedRange * mark = marks->get( i );
        int p = pages->FindNearestPage( mark->start.y, 0 );
        if ( p>=0 && lastPage!=p ) {
            _selPages.add( p );
            lastPage = p;
            if ( _curIndex==-1 && p>=cp )
                _curIndex = _selPages.length()-1;
        }
    }
    if ( _curIndex<0 )
        _curIndex = 0;
    moveBy( 0 );
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

