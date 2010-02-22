//
// C++ Implementation: TOC dialog
//
// Description: 
//      Shows table of contents, and allows to input page number to go
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// tocdlg.cpp

#include "tocdlg.h"
#include <cri18n.h>

lString16 limitTextWidth( lString16 s, int width, LVFontRef font )
{

    int w = font->getTextWidth(s.c_str(), s.length());
    if ( w<width )
        return s;
    lString16 sss = L"...";
    int www = font->getTextWidth(sss.c_str(), sss.length());
    while (s.length()>0) {
        s.erase(s.length()-1, 1);
        int w = font->getTextWidth(s.c_str(), s.length());
        if ( w+www<=width )
            return s+sss;
    }
    return lString16(".");
}

void CRTOCDialog::draw()
{
    CRGUIWindowBase::draw();
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    lvRect borders = clientSkin->getBorderWidths();
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    lvRect tocRect;
    getClientRect( tocRect );
    // draw toc
    for ( int i=0; i<_pageItems && i+_topItem<(int)_items.length(); i++ ) {
        lvRect margins( 10, 10, 10, 10 );
        lvRect itemRect = tocRect;
        itemRect.shrinkBy( margins );
        itemRect.top = i * _itemHeight + tocRect.top + margins.top;
        itemRect.bottom = itemRect.top + _itemHeight;
        LVTocItem * item = _items[ i + _topItem];
        lString16 titleString = item->getName();
        lString16 pageString = lString16::itoa( item->getPage() + 1 );
        int pageNumWidth = _font->getTextWidth( pageString.c_str(), pageString.length() );
        int titleWidth = _font->getTextWidth( titleString.c_str(), titleString.length() );
        int level = item->getLevel();
        int levelMargin = 32; // TODO: get better value
        itemRect.left += (level - 1) * levelMargin;
        lvRect pageNumRect = itemRect;
        pageNumRect.left = pageNumRect.right - pageNumWidth;
        itemRect.right = pageNumRect.left;
        if ( !itemRect.isEmpty() ) {
            lvRect rc = itemRect;
            rc.extendBy( borders );
            lString16 s = limitTextWidth( titleString, rc.width()-borders.left-borders.right, clientSkin->getFont() );
            clientSkin->drawText( *drawbuf, rc, s );
        }
        if ( !pageNumRect.isEmpty() ) {
            lvRect rc = pageNumRect;
            rc.extendBy( borders );
            clientSkin->drawText( *drawbuf, rc, pageString );
        }
        if ( itemRect.left + titleWidth < itemRect.right + 5 ) {
            itemRect.left = itemRect.left + titleWidth;
            itemRect.left = (itemRect.left + 3) & (~3);
            itemRect.right &= (~3);
            lUInt32 cl = clientSkin->getTextColor();
            if ( !itemRect.isEmpty() ) {
                // draw line of points
                for ( int i = itemRect.left; i < itemRect.right; i += 4 ) {
                    int y = itemRect.bottom - 2;
                    drawbuf->FillRect( i, y, i+1, y+1, cl );
                }
            }

        }
    }
}

CRTOCDialog::CRTOCDialog( CRGUIWindowManager * wm, lString16 title, int resultCmd, int pageCount, LVDocView * docview )
: CRNumberEditDialog( wm, title, lString16(), resultCmd, 1, pageCount )
{
    docview->getFlatToc( _items );
    _skinName = L"#toc";
    _skin = _wm->getSkin()->getWindowSkin(_skinName.c_str());
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    lvRect borders = clientSkin->getBorderWidths();
    CRScrollSkinRef sskin = _skin->getScrollSkin();
    _font = clientSkin->getFont();
    _fullscreen = true;
    _rect = _wm->getScreen()->getRect();
    _caption = title;
    lvRect clientRect = _skin->getClientRect( _rect );
    lvRect tocRect;
    getClientRect( tocRect );
    _itemHeight = _font->getHeight();
    _pageItems = tocRect.height() / _itemHeight;
    _topItem = 0;
    _page = _topItem / _pageItems + 1;
    _pages = (_items.length()+(_pageItems-1))/ _pageItems;
    _statusText = L"Enter page number:";
    _inputText = L"_";
}

bool CRTOCDialog::digitEntered( lChar16 c )
{
    lString16 v = _value;
    v << c;
    int n = v.atoi();
    if ( n<=_maxvalue ) {
        _value = v;
        _inputText = _value + L"_";
        setDirty();
        return true;
    }
    return false;
}

/// returns true if command is processed
bool CRTOCDialog::onCommand( int command, int params )
{
    if ( _value.empty() ) {
        if ( command == MCMD_SELECT_0 )
            command = MCMD_SCROLL_FORWARD;
        if ( command == MCMD_SELECT_0_LONG )
            command = MCMD_SCROLL_FORWARD_LONG;
    }
    switch ( command ) {
    case MCMD_CANCEL:
        if ( _value.length()>0 ) {
            _value.erase( _value.length()-1, 1 );
            _inputText = _value + L"_";
            setDirty();
        } else {
            _wm->closeWindow( this );
        }
        return true;
    case MCMD_OK:
        {
            int n = _value.atoi();
            if ( n>=_minvalue && n<=_maxvalue ) {
                _wm->postCommand( _resultCmd, n );
                _wm->closeWindow( this );
                return true;
            }
            _wm->closeWindow( this );
            return true;
        }
    case MCMD_SCROLL_FORWARD:
    case MCMD_SCROLL_FORWARD_LONG:
        {
            int step = command == MCMD_SCROLL_FORWARD_LONG ? 10 : 1;
            _topItem = _topItem + _pageItems * step;
            int maxpos = (_items.length()) / _pageItems * _pageItems;
            if ( _topItem > maxpos )
                _topItem = maxpos;
            if ( _topItem < 0 )
                _topItem = 0;
            setDirty();
        }
        break;
    case MCMD_SCROLL_BACK:
    case MCMD_SCROLL_BACK_LONG:
        {
            int step = command == MCMD_SCROLL_BACK_LONG ? 10 : 1;
            _topItem = _topItem - _pageItems * step;
            int maxpos = (_items.length()) / _pageItems * _pageItems;
            if ( _topItem > maxpos )
                _topItem = maxpos;
            if ( _topItem < 0 )
                _topItem = 0;
            setDirty();
        }
        break;
    case MCMD_SELECT_0:
    case MCMD_SELECT_1:
    case MCMD_SELECT_2:
    case MCMD_SELECT_3:
    case MCMD_SELECT_4:
    case MCMD_SELECT_5:
    case MCMD_SELECT_6:
    case MCMD_SELECT_7:
    case MCMD_SELECT_8:
    case MCMD_SELECT_9:
        digitEntered( '0' + (command - MCMD_SELECT_0) );
        break;
    default:
        return true;
    }
    return true;
}
