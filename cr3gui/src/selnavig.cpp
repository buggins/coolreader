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

