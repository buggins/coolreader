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
    lString16 sss("...");
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
    CRRectSkinRef normalItemSkin = _skin->getItemSkin();
    CRRectSkinRef valueSkin = _skin->getValueSkin();
    CRRectSkinRef selItemSkin = _skin->getSelItemSkin();
    if ( normalItemSkin.isNull() )
        normalItemSkin = clientSkin;
    if ( valueSkin.isNull() )
        valueSkin = clientSkin;
    if ( selItemSkin.isNull() )
        selItemSkin = clientSkin;
    lvRect borders = clientSkin->getBorderWidths();
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    lvRect tocRect;
    getClientRect( tocRect );
    tocRect.shrinkBy(borders);
    int curPage = _docview->getCurPage();
    // draw toc
    for (int i=0; i < _pageItems && i + _topItem < _items.length(); i++) {
        LVTocItem * item = _items[ i + _topItem];
        LVTocItem * nextitem = (i+_topItem+1) < _items.length()
                               ? _items[ i + _topItem + 1] : NULL;

        //lvRect margins( 10, 10, 10, 10 );
        lvRect itemRect = tocRect;
        bool isSelected = true;

        if ( curPage < item->getPage() )
            isSelected = false;
        else if ( nextitem!=NULL && curPage >= nextitem->getPage() &&
                  curPage > item->getPage() )
            isSelected = false;
        CRRectSkinRef itemSkin = isSelected ? selItemSkin : normalItemSkin;
        //itemRect.shrinkBy( margins );
        itemRect.top = i * _itemHeight + tocRect.top;
        itemRect.bottom = itemRect.top + _itemHeight;



        lString16 titleString = item->getName();
        lString16 pageString = lString16::itoa( item->getPage() + 1 );
        int pageNumWidth = valueSkin->getFont()->getTextWidth( pageString.c_str(), pageString.length() );
        int titleWidth = itemSkin->getFont()->getTextWidth( titleString.c_str(), titleString.length() );
        int level = item->getLevel();
        int levelMargin = 32; // TODO: get better value
        itemSkin->draw( *drawbuf, itemRect );
        itemRect.left += (level - 1) * levelMargin;
        lvRect pageNumRect = itemRect;
        pageNumRect.left = pageNumRect.right - pageNumWidth - valueSkin->getBorderWidths().left - valueSkin->getBorderWidths().right;
        itemRect.right = pageNumRect.left;
        if ( !itemRect.isEmpty() ) {
            lvRect rc = itemRect;
            //rc.extendBy( borders );
            lString16 s = limitTextWidth( titleString, rc.width()-borders.left-borders.right, itemSkin->getFont() );
            itemSkin->drawText( *drawbuf, rc, s );
        }
        if ( !pageNumRect.isEmpty() ) {
            lvRect rc = pageNumRect;
            //rc.extendBy( borders );
            valueSkin->drawText( *drawbuf, rc, pageString );
        }
        if ( itemRect.left + titleWidth < itemRect.right + 5 ) {
            itemRect.left = itemRect.left + titleWidth + itemSkin->getBorderWidths().left;
            itemRect.left = (itemRect.left + 3) & (~3);
            itemRect.right &= (~3);
            lUInt32 cl = clientSkin->getTextColor();
            if ( !itemRect.isEmpty() ) {
                // draw line of points
                for ( int i = itemRect.left; i < itemRect.right; i += 4 ) {
                    int y = itemRect.bottom - itemSkin->getFontSize()/5;
                    drawbuf->FillRect( i, y, i+1, y+1, cl );
                }
            }

        }
    }
}

CRTOCDialog::CRTOCDialog( CRGUIWindowManager * wm, lString16 title, int resultCmd, int pageCount, LVDocView * docview )
: CRNumberEditDialog( wm, title, lString16::empty_str, resultCmd, 1, pageCount )
,_docview(docview)
{
    docview->getFlatToc( _items );
    _skinName = "#toc";
    _skin = _wm->getSkin()->getMenuSkin(_skinName.c_str());
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    CRRectSkinRef itemSkin = _skin->getItemSkin();
    CRRectSkinRef valueSkin = _skin->getValueSkin();
    CRRectSkinRef selItemSkin = _skin->getSelItemSkin();
    if ( itemSkin.isNull() )
        itemSkin = clientSkin;
    if ( valueSkin.isNull() )
        valueSkin = clientSkin;
    if ( selItemSkin.isNull() )
        selItemSkin = clientSkin;

    lvRect borders = clientSkin->getBorderWidths();
    CRScrollSkinRef sskin = _skin->getScrollSkin();
    _font = itemSkin->getFont();
    _fullscreen = true;
    _rect = _wm->getScreen()->getRect();
    _caption = title;
    lvRect clientRect = _skin->getClientRect( _rect );
    lvRect tocRect;
    getClientRect( tocRect );
    tocRect.shrinkBy(borders);
    lvRect itemBorders = itemSkin->getBorderWidths();
    _itemHeight = _font->getHeight() + itemBorders.top + itemBorders.bottom;
    _pageItems = tocRect.height() / _itemHeight;
    int curItem = getCurItemIndex();
    _topItem = curItem>=0 ? curItem / _pageItems * _pageItems : 0;
    _page = _topItem / _pageItems + 1;
    _pages = (_items.length() + (_pageItems - 1)) / _pageItems;
    int curPage = _docview->getCurPage();
    int docPages = _docview->getPageCount();
    lString16 pageString(_("Current page: $1 of $2\n"));
    pageString.replaceIntParam(1, curPage+1);
    pageString.replaceIntParam(2, docPages);
    _statusText = pageString + lString16(_("Enter page number:"));
    _inputText = "_";
}

bool CRTOCDialog::digitEntered( lChar16 c )
{
    lString16 v = _value;
    v << c;
    int n = v.atoi();
    if ( n<=_maxvalue ) {
        _value = v;
        _inputText = _value + "_";
        setDirty();
        return true;
    }
    return false;
}

/// returns index of first item for current page, -1 if not found
int CRTOCDialog::getCurItemIndex()
{
    int curPage = _docview->getCurPage();
    for ( int i=0; i<_items.length(); i++ ) {
        LVTocItem * item = _items[ i ];
        LVTocItem * nextitem = (i+1) < _items.length()
                               ? _items[ i + 1] : NULL;
        bool isSelected = true;
        if ( curPage < item->getPage() )
            isSelected = false;
        else if ( nextitem!=NULL && curPage >= nextitem->getPage() &&
                  curPage > item->getPage() )
            isSelected = false;
        if ( isSelected )
            return i;
    }
    return -1;
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
            _inputText = _value + "_";
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
            int maxpos = (_items.length()-1) / _pageItems * _pageItems;
            if ( _topItem > maxpos )
                _topItem = maxpos;
            if ( _topItem < 0 )
                _topItem = 0;
            _page = _topItem / _pageItems + 1;
            _pages = (_items.length()+(_pageItems-1))/ _pageItems;
            setDirty();
        }
        break;
    case MCMD_SCROLL_BACK:
    case MCMD_SCROLL_BACK_LONG:
        {
            int step = command == MCMD_SCROLL_BACK_LONG ? 10 : 1;
            _topItem = _topItem - _pageItems * step;
            int maxpos = (_items.length()-1) / _pageItems * _pageItems;
            if ( _topItem > maxpos )
                _topItem = maxpos;
            if ( _topItem < 0 )
                _topItem = 0;
            _page = _topItem / _pageItems + 1;
            _pages = (_items.length()+(_pageItems-1))/ _pageItems;
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
