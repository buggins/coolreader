//
// C++ Implementation: links navigation dialog
//
// Description:
//      Allows to select link from current page, and go to it.
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "linksdlg.h"
#include "mainwnd.h"

static const char * link_back_active[] = {
    "30 27 5 1",
    "0 c #000000",
    "o c #A1A1A1",
    ". c #FFFFFF",
    "x c #A1A1A1",
    "  c None",
    "..............................",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x.........00...............x.",
    ".x........0o0...............x.",
    ".x.......0oo0...............x.",
    ".x......0ooo0...............x.",
    ".x.....0oooo0...............x.",
    ".x....0ooooo00000000000000..x.",
    ".x...0ooooooooooooooooooo0..x.",
    ".x..0oooooooooooooooooooo0..x.",
    ".x.0ooooooooooooooooooooo0..x.",
    ".x..0oooooooooooooooooooo0..x.",
    ".x...0ooooooooooooooooooo0..x.",
    ".x....0ooooo00000000000000..x.",
    ".x.....0oooo0...............x.",
    ".x......0ooo0...............x.",
    ".x.......0oo0...............x.",
    ".x........0o0...............x.",
    ".x.........00...............x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "..............................",
};
static const char * link_forward_active[] = {
    "30 27 5 1",
    "0 c #000000",
    "o c #A1A1A1",
    ". c #FFFFFF",
    "x c #A1A1A1",
    "  c None",
    "..............................",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..............00..........x.",
    ".x..............0o0.........x.",
    ".x..............0oo0........x.",
    ".x..............0ooo0.......x.",
    ".x..............0oooo0......x.",
    ".x.00000000000000ooooo0.....x.",
    ".x.0ooooooooooooooooooo0....x.",
    ".x.0oooooooooooooooooooo0...x.",
    ".x.0ooooooooooooooooooooo0..x.",
    ".x.0oooooooooooooooooooo0...x.",
    ".x.0ooooooooooooooooooo0....x.",
    ".x.00000000000000ooooo0.....x.",
    ".x..............0oooo0......x.",
    ".x..............0ooo0.......x.",
    ".x..............0oo0........x.",
    ".x..............0o0.........x.",
    ".x..............00..........x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "..............................",
};
static const char * link_back_normal[] = {
    "30 27 5 1",
    "0 c #515151",
    "o c #FFFFFF",
    ". c #FFFFFF",
    "x c #A1A1A1",
    "  c None",
    "..............................",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x.........00...............x.",
    ".x........0o0...............x.",
    ".x.......0oo0...............x.",
    ".x......0ooo0...............x.",
    ".x.....0oooo0...............x.",
    ".x....0ooooo00000000000000..x.",
    ".x...0ooooooooooooooooooo0..x.",
    ".x..0oooooooooooooooooooo0..x.",
    ".x.0ooooooooooooooooooooo0..x.",
    ".x..0oooooooooooooooooooo0..x.",
    ".x...0ooooooooooooooooooo0..x.",
    ".x....0ooooo00000000000000..x.",
    ".x.....0oooo0...............x.",
    ".x......0ooo0...............x.",
    ".x.......0oo0...............x.",
    ".x........0o0...............x.",
    ".x.........00...............x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "..............................",
};
static const char * link_forward_normal[] = {
    "30 27 5 1",
    "0 c #515151",
    "o c #FFFFFF",
    ". c #FFFFFF",
    "x c #A1A1A1",
    "  c None",
    "..............................",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..............00..........x.",
    ".x..............0o0.........x.",
    ".x..............0oo0........x.",
    ".x..............0ooo0.......x.",
    ".x..............0oooo0......x.",
    ".x.00000000000000ooooo0.....x.",
    ".x.0ooooooooooooooooooo0....x.",
    ".x.0oooooooooooooooooooo0...x.",
    ".x.0ooooooooooooooooooooo0..x.",
    ".x.0oooooooooooooooooooo0...x.",
    ".x.0ooooooooooooooooooo0....x.",
    ".x.00000000000000ooooo0.....x.",
    ".x..............0oooo0......x.",
    ".x..............0ooo0.......x.",
    ".x..............0oo0........x.",
    ".x..............0o0.........x.",
    ".x..............00..........x.",
    ".x..........................x.",
    ".x..........................x.",
    ".x..........................x.",
    ".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
    "..............................",
};


void CRLinksDialog::Update()
{
    setDirty();
    _wm->update( false );
}

void CRLinksDialog::draw()
{
    if ( _invalidateRect.isEmpty() ) {
        CRLog::debug("crLinksDialog::Draw() - invalidated rect is empty!");
        return;
    }
    CRLog::debug("crLinksDialog::Draw()");
    LVDocImageRef page = _docview->getPageImage(0);
    LVDrawBuf * buf = page->getDrawBuf();
    _wm->getScreen()->draw( buf );
    CRLog::trace("draw buttons, current=%d", _currentButton);
    for ( int i=0; i<_addButtonCount; i++ ) {
        int btn = _additionalButtons[i];
        _wm->getScreen()->getCanvas()->Draw( btn==_currentButton?_activeIcons[btn]:_normalIcons[btn], _iconRects[i].left, _iconRects[i].top, _iconRects[i].width(), _iconRects[i].height() );
    }
}

CRLinksDialog * CRLinksDialog::create( CRGUIWindowManager * wm, CRViewDialog * docwin )
{
    ldomXRangeList list;
    docwin->getDocView()->getCurrentPageLinks( list );
    int backSize = docwin->getDocView()->getNavigationHistory().backCount();
    int fwdSize = docwin->getDocView()->getNavigationHistory().forwardCount();
    if ( list.length()==0 && backSize==0 && fwdSize==0)
        return NULL;
    docwin->getDocView()->clearImageCache();
    docwin->getDocView()->selectFirstPageLink();
    return new CRLinksDialog( wm, docwin );
}

CRLinksDialog::CRLinksDialog( CRGUIWindowManager * wm, CRViewDialog * docwin )
: CRGUIWindowBase( wm ), _docwin(docwin), _docview(docwin->getDocView())
{
    _invalidateRect.left = 0;
    _invalidateRect.top = 0;
    _invalidateRect.right = 600;
    _invalidateRect.bottom = 800;
    ldomXRangeList list;
    _docview->getCurrentPageLinks( list );
    _linkCount = list.length();
    _backSize = _docview->getNavigationHistory().backCount();
    _fwdSize = _docview->getNavigationHistory().forwardCount();
    CRLog::debug("LinksDialog: links=%d, back=%d, fwd=%d", _linkCount, _backSize, _fwdSize);
    _addButtonCount = 0;
    if ( _backSize )
        _additionalButtons[_addButtonCount++] = BACK;
    if ( _fwdSize )
        _additionalButtons[_addButtonCount++] = FORWARD;

    CRLog::debug("LinksDialog: creating icons");
    _activeIcons[FORWARD] = LVCreateXPMImageSource( link_forward_active );
    _activeIcons[BACK] = LVCreateXPMImageSource( link_back_active );
    _normalIcons[FORWARD] = LVCreateXPMImageSource( link_forward_normal );
    _normalIcons[BACK] = LVCreateXPMImageSource( link_back_normal );
    CRLog::debug("LinksDialog: icons created");
    int dx = _activeIcons[0]->GetWidth();
    int dy = _activeIcons[0]->GetHeight();
    int w = 3;
    lvRect rc1(w,w,w+dx,w+dy);
    lvRect rc2(w+dx+w,w,w+dx+dx+w,w+dy);
    _iconRects[0] = rc1;
    _iconRects[1] = rc2;
    // FORWARD->BACK->LINK->FORWARD->BACK
    _currentButton =    (_linkCount>0) ? LINK :  ( (_backSize>0) ? BACK :  FORWARD);
    _nextButton[BACK] = (_linkCount>0) ? LINK :  ( (_fwdSize>0)  ? FORWARD : BACK );
    _nextButton[FORWARD] = (_backSize>0) ? BACK :( (_linkCount>0)? LINK :  FORWARD);
    _nextButton[LINK] = (_fwdSize>0) ? FORWARD : ( (_backSize>0) ? BACK :  LINK);
    _prevButton[FORWARD] =(_linkCount>0) ? LINK :( (_backSize>0) ? BACK :  FORWARD );
    _prevButton[BACK] = (_fwdSize>0) ? FORWARD : ( (_linkCount>0)? LINK :  BACK);
    _prevButton[LINK] = (_backSize>0) ? BACK :   ( (_fwdSize>0)  ? FORWARD : LINK);
    CRLog::debug("dialog is created");
    _fullscreen = true;
}

/// returns true if command is processed
bool CRLinksDialog::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_CANCEL:
        _docview->clearSelection();
        _wm->closeWindow( this );
        return true;
    case MCMD_OK:
        if ( _currentButton==LINK )
            _docview->goSelectedLink();
        else if ( _currentButton==BACK )
            _docview->goBack();
        else
            _docview->goForward();
        _docview->clearSelection();
        _wm->closeWindow( this );
        return true;
    case MCMD_LONG_BACK:
        _docview->clearSelection();
        _docview->goBack();
        _wm->closeWindow( this );
        return true;
    case MCMD_LONG_FORWARD:
        _docview->clearSelection();
        _docview->goForward();
        _wm->closeWindow( this );
        return true;
    case MCMD_SCROLL_FORWARD:
    case MCMD_SELECT_0:
        invalidateCurrentSelection();
        if ( _currentButton==LINK ) {
            if ( !_docview->selectNextPageLink( _addButtonCount==0 )) {
                _currentButton = _nextButton[_currentButton];
            }
        } else {
            _currentButton = _nextButton[_currentButton];
            if ( _currentButton==LINK )
                _docview->selectNextPageLink( false );
        }
        Update();
        invalidateCurrentSelection();
        return true;
    case MCMD_SCROLL_BACK:
    case MCMD_SELECT_9:
        invalidateCurrentSelection();
        if ( _currentButton==LINK ) {
            if ( !_docview->selectPrevPageLink( _addButtonCount==0 )) {
                _currentButton = _prevButton[_currentButton];
            }
        } else {
            _currentButton = _prevButton[_currentButton];
            if ( _currentButton==LINK )
                _docview->selectPrevPageLink( false );
        }
        Update();
        invalidateCurrentSelection();
        return true;
    case MCMD_SELECT_1:
    case MCMD_SELECT_2:
    case MCMD_SELECT_3:
    case MCMD_SELECT_4:
    case MCMD_SELECT_5:
    case MCMD_SELECT_6:
    case MCMD_SELECT_7:
    case MCMD_SELECT_8:
        {
            int index = command - MCMD_SELECT_1;
            if ( index < _linkCount ) {
                ldomXRangeList list;
                _docview->getCurrentPageLinks( list );
                ldomXRange * link = list[index];
                invalidateCurrentSelection();
                _docview->selectRange( *link );
                draw();
                invalidateCurrentSelection();
                draw();
                _docview->goSelectedLink();
                    _docview->clearSelection();
                _wm->closeWindow( this );
                    return true;
            }
        }
        break;
    default:
        return true;
    }
    return true;
}

void CRLinksDialog::invalidateCurrentSelection()
{
    if ( _currentButton != LINK ) {
        _invalidateRect.left = _iconRects[0].left;
        _invalidateRect.top = _iconRects[0].top;
        _invalidateRect.right = _iconRects[1].right;
        _invalidateRect.bottom = _iconRects[1].bottom;
        CRLog::debug("invalidateCurrentSelection() : invalidating buttons rect");
        return;
    }
    ldomXRange * link = _docview->getCurrentPageSelectedLink();
    _invalidateRect.clear();
    if ( !link ) {
        CRLog::debug("invalidateCurrentSelection() : no current page selection found!");
        return;
    }
    lvRect rc;
    if ( link->getRect( rc ) ) {
        CRLog::debug("link docRect { %d, %d, %d, %d }", rc.left, rc.top, rc.right, rc.bottom);
#if 1
        _invalidateRect.left = 0;
        _invalidateRect.top = 0;
        _invalidateRect.right = 600;
        _invalidateRect.bottom = 800;
#else
        lvPoint topLeft = rc.topLeft();
        lvPoint bottomRight = rc.bottomRight();
        if ( _docview->docToWindowPoint(topLeft) && _docview->docToWindowPoint(bottomRight) ) {
            rc.left = topLeft.x;
            rc.top = topLeft.y;
            rc.right = bottomRight.x;
            rc.bottom = bottomRight.y;
            CRLog::debug("invalidating link screenRect { %d, %d, %d, %d }", rc.left, rc.top, rc.right, rc.bottom);
            _invalidateRect = rc;
        } else {
            CRLog::debug("link rect conversion error: invalidating full screen");
            _invalidateRect.left = 0;
            _invalidateRect.top = 0;
            _invalidateRect.right = 600;
            _invalidateRect.bottom = 800;
        }
#endif
    } else {
        CRLog::debug("invalidateCurrentSelection() : getRect failed for link!");
    }
}
