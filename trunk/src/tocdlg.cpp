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

void CRTOCDialog::draw()
{
    CRRectSkinRef titleSkin = _skin->getTitleSkin();
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    lvRect borders = clientSkin->getBorderWidths();
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    _skin->draw( *drawbuf, _rect );
    lvRect titleRect = _skin->getTitleRect( _rect );
    titleSkin->draw( *drawbuf, titleRect );
    titleSkin->drawText( *drawbuf, titleRect, _title );
    // draw toc
    clientSkin->draw( *drawbuf, _tocRect );
    for ( int i=0; i<_pageItems && i+_topItem<(int)_items.length(); i++ ) {
        lvRect margins( 10, 10, 10, 10 );
        lvRect itemRect = _tocRect;
        itemRect.shrinkBy( margins );
        itemRect.top = i * _itemHeight + _tocRect.top + margins.top;
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
            clientSkin->drawText( *drawbuf, rc, titleString );
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
            lUInt32 cl = drawbuf->GetTextColor();
            if ( !itemRect.isEmpty() ) {
                // draw line of points
                for ( int i = itemRect.left; i < itemRect.right; i += 4 ) {
                    int y = itemRect.bottom - 2;
                    drawbuf->FillRect( i, y, i+1, y+1, cl );
                }
            }

        }
    }
    // draw input area
    clientSkin->draw( *drawbuf, _inputRect );
    clientSkin->drawText( *drawbuf, _inputRect, lString16("Enter page number to go: ") + _value+L"_" );
    if ( !_scrollRect.isEmpty() ) {
        // draw scrollbar
        CRScrollSkinRef sskin = _skin->getScrollSkin();
        sskin->drawScroll( *drawbuf, _scrollRect, false, _topItem, _items.length(), _pageItems );
    }
}

CRTOCDialog::CRTOCDialog( CRGUIWindowManager * wm, lString16 title, int resultCmd, int pageCount, LVDocView * docview )
: CRNumberEditDialog( wm, title, lString16(), resultCmd, 1, pageCount )
{
    docview->getFlatToc( _items );
    _skin = _wm->getSkin()->getWindowSkin(L"#toc");
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    lvRect borders = clientSkin->getBorderWidths();
    CRScrollSkinRef sskin = _skin->getScrollSkin();
    _font = clientSkin->getFont();
    _fullscreen = true;
    _rect = _wm->getScreen()->getRect();
    lvRect clientRect = _skin->getClientRect( _rect );
    _inputRect = clientRect;
    _inputRect.top = _inputRect.bottom - 40;
    _tocRect = clientRect;
    _tocRect.bottom = _inputRect.top;
    _itemHeight = _font->getHeight();
    _scrollRect = _tocRect;
    _topItem = 0;
    if ( _items.length() > _pageItems ) {
        // show scroll
        _scrollRect.top = _scrollRect.bottom - 36; //sskin->getMinSize().y
        _tocRect.bottom = _scrollRect.top;
        _pageItems = _tocRect.height() / _itemHeight;
    } else {
        // no scroll
        _scrollRect.top = _scrollRect.bottom;
    }
    _pageItems = ( _tocRect.height() - 20 ) / _itemHeight;
}

bool CRTOCDialog::digitEntered( lChar16 c )
{
    lString16 v = _value;
    v << c;
    int n = v.atoi();
    if ( n<=_maxvalue ) {
        _value = v;
        setDirty();
        return true;
    }
    return false;
}

/// returns true if command is processed
bool CRTOCDialog::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_CANCEL:
        if ( _value.length()>0 ) {
            _value.erase( _value.length()-1, 1 );
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
        {
            _topItem = _topItem + _pageItems;
            if ( _topItem > _items.length() - _pageItems )
                _topItem = _items.length() - _pageItems;
            if ( _topItem < 0 )
                _topItem = 0;
            setDirty();
        }
        break;
    case MCMD_SCROLL_BACK:
        {
            _topItem = _topItem - _pageItems;
            if ( _topItem > _items.length() - _pageItems )
                _topItem = _items.length() - _pageItems;
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
