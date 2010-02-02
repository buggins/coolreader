/*******************************************************

   CoolReader Engine

   lvxml.cpp:  XML parser implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include "../include/crgui.h"
#include "../include/crtrace.h"

//TODO: place to skin file
#define ITEM_MARGIN 8
#define HOTKEY_SIZE 36
#define MENU_NUMBER_FONT_SIZE 24
#define SCROLL_HEIGHT 34
#define DEF_FONT_SIZE 22
#define DEF_TITLE_FONT_SIZE 28

// if 1, full page (e.g. 8 items) is scrolled even if on next page would be less items (show empty space)
#define FULL_SCROLL 1

const char * cr_default_skin =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<CR3Skin>\n"
"  <menu id=\"main\">\n"
"        <text color=\"#000000\" face=\"Arial\" size=\"25\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"        <background color=\"#AAAAAA\"/>\n"
"        <border widths=\"0,8,8,8\"/>\n"
"        <!--icon image=\"filename\" valign=\"\" halign=\"\"/-->\n"
"        <title>\n"
"            <size minvalue=\"32,0\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"25\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background color=\"#AAAAAA\"/>\n"
"            <border widths=\"4,4,4,4\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </title>\n"
"        <item>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"left\"/>\n"
"            <background image=\"std_menu_item_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </item>\n"
"        <shortcut>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background image=\"std_menu_shortcut_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </shortcut>\n"
"  </menu>\n"
"  <menu id=\"settings\">\n"
"        <text color=\"#000000\" face=\"Arial\" size=\"25\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"        <background color=\"#AAAAAA\"/>\n"
"        <border widths=\"8,8,8,8\"/>\n"
"        <!--icon image=\"filename\" valign=\"\" halign=\"\"/-->\n"
"        <title>\n"
"            <size minvalue=\"0,40\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"28\" bold=\"true\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background color=\"#AAAAAA\"/>\n"
"            <border widths=\"4,4,4,4\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </title>\n"
"        <item>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"left\"/>\n"
"            <background image=\"std_menu_item_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </item>\n"
"        <shortcut>\n"
"            <size minvalue=\"48,48\" maxvalue=\"0,0\"/>\n"
"            <text color=\"#000000\" face=\"Arial\" size=\"24\" bold=\"false\" italic=\"false\" valign=\"center\" halign=\"center\"/>\n"
"            <background image=\"std_menu_shortcut_background.xpm\" color=\"#FFFFFF\"/>\n"
"            <border widths=\"6,6,6,6\"/>\n"
"            <!--icon image=\"filename\" valign=\"\" halign=\"\"-->\n"
"        </shortcut>\n"
"  </menu>\n"
"</CR3Skin>\n";

bool CRGUIWindowManager::loadSkin( lString16 pathname )
{
    CRSkinRef skin;
    if ( !pathname.empty() )
        skin = LVOpenSkin( pathname );
    if ( skin.isNull() ) {
        skin = LVOpenSimpleSkin( lString8( cr_default_skin ) );
        setSkin( skin );
        return false;
    }
    setSkin( skin );
    return true;
}


/// add all items from another table
void CRGUIAcceleratorTable::addAll( const CRGUIAcceleratorTable & v )
{
	for ( int i=0; i<v._items.length(); i++ ) {
		CRGUIAccelerator * item = v._items.get( i );
		add( item->keyCode, item->keyFlags, item->commandId, item->commandParam );
	}
}

/// add accelerator to table or change existing
bool CRGUIAcceleratorTable::add( int keyCode, int keyFlags, int commandId, int commandParam )
{
    int index = indexOf( keyCode, keyFlags );
    if ( index >= 0 ) {
        // just update
        CRGUIAccelerator * item = _items[index];
        item->commandId = commandId;
        item->commandParam = commandParam;
        return false;
    }
    CRGUIAccelerator * item = new CRGUIAccelerator();
    item->keyCode = keyCode;
    item->keyFlags = keyFlags;
    item->commandId = commandId;
    item->commandParam = commandParam;
    _items.add(item);
    return true;
}

CRGUIAcceleratorTableRef CRGUIAcceleratorTableList::get( const lString16 & name, CRPropRef keyRemappingOptions )
{
    CRGUIAcceleratorTableRef prev = get(name);
    if ( !prev )
        return prev;
    CRPropRef keymaps = keyRemappingOptions->getSubProps(LCSTR(lString16("keymap.") + name + L"."));
    if ( keymaps.isNull() || keymaps->getCount()==0 )
        return prev;
    CRGUIAcceleratorTableRef acc( new CRGUIAcceleratorTable( *prev ));
    for ( int i=0; i<keymaps->getCount(); i++ ) {
        lString16 name( keymaps->getName(i) );
        lString16 value = keymaps->getValue(i);
//        CRLog::trace("Override key map: %s -> %s", LCSTR(name), LCSTR(value) );
        int key, flags;
        int cmd, params;
        if ( !splitIntegerList( name, lString16("."), key, flags ))
            continue;
        if ( !splitIntegerList( value, lString16(","), cmd, params ))
            continue;
        acc->add(key, flags, cmd, params);
    }
    return acc;
}

/// add all tables
void CRGUIAcceleratorTableList::addAll( const CRGUIAcceleratorTableList & v )
{
	LVHashTable<lString16, CRGUIAcceleratorTableRef>::iterator i( v._table );
	for ( ;; ) {
		LVHashTable<lString16, CRGUIAcceleratorTableRef>::pair * p = i.next();
		if ( !p )
			break;
		CRGUIAcceleratorTableRef t = _table.get( p->key );
		if ( t.isNull() ) {
			t = CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable() );
			_table.set( p->key, t );
		}
		crtrace trace;
		trace << "Merging accelerators for '"  << p->key << "'";
		t->addAll( *p->value );
	}
}

/// draws icon at center of screen
void CRGUIWindowManager::showWaitIcon( lString16 filename, int progressPercent )
{
    LVImageSourceRef img = _skin->getImage( filename );
    if ( !img.isNull() ) {
        int dx = img->GetWidth();
        int dy = img->GetHeight();
        int x = (_screen->getWidth() - dx) / 2;
        int y = (_screen->getHeight() - dy) / 2;
        CRLog::debug("Drawing wait image %s %dx%d  progress=%d%%", UnicodeToUtf8(filename).c_str(), dx, dy, progressPercent );
        _screen->getCanvas()->Draw( img, x, y, dx, dy, true );
        int gaugeH = 0;
        if ( progressPercent>=0 && progressPercent<=100 ) {
            CRScrollSkinRef skin = _skin->getScrollSkin(L"#progress");
            if ( !skin.isNull() ) {
                CRLog::trace("Drawing gauge %d%%", progressPercent);
                gaugeH = 16;
                lvRect gaugeRect( x, y+dy, x+dx, y+dy+gaugeH );
                skin->drawGauge( *(_screen->getCanvas()), gaugeRect, progressPercent );
            }
        }
        _screen->invalidateRect( lvRect(x, y, x+dx, y+dy+gaugeH) );
        _screen->flush(false);
    } else {
        CRLog::error("CRGUIWindowManager::showWaitIcon(%s): image not found in current skin", UnicodeToUtf8(filename).c_str() );
    }
}

#define PROGRESS_UPDATE_INTERVAL 2
/// draws icon at center of screen, with optional progress gauge
void CRGUIWindowManager::showProgress( lString16 filename, int progressPercent )
{
    time_t t = (time_t)time((time_t)0);
    if ( t<_lastProgressUpdate+PROGRESS_UPDATE_INTERVAL || progressPercent==_lastProgressPercent )
        return;
    showWaitIcon( filename, progressPercent );
    _lastProgressUpdate = t;
    _lastProgressPercent = progressPercent;
}

void CRGUIScreenBase::flush( bool full )
{
    if ( _updateRect.isEmpty() && !full ) {
        CRLog::trace("CRGUIScreenBase::flush() - update rectangle is empty");
        return;
    }
    if ( !_front.isNull() && !_updateRect.isEmpty() && !full ) {
        // calculate really changed area
        lvRect rc;
        lvRect lineRect(_updateRect);
        int sz = _canvas->GetRowSize();
        for ( int y = _updateRect.top; y < _updateRect.bottom; y++ ) {
            if ( y>=0 && y<_height ) {
                void * line1 = _canvas->GetScanLine( y );
                void * line2 = _front->GetScanLine( y );
                if ( memcmp( line1, line2, sz ) ) {
                    // line content is different
                    lineRect.top = y;
                    lineRect.bottom = y+1;
                    rc.extend( lineRect );
                    // copy line to front buffer
                    memcpy( line2, line1, sz );
                }
            }
        }
        if ( rc.isEmpty() ) {
            // no actual changes
            _updateRect.clear();
            return;
        }
        _updateRect.top = rc.top;
        _updateRect.bottom = rc.bottom;
    }
    if ( full && !_front.isNull() ) {
        // copy full screen to front buffer
        _canvas->DrawTo( _front.get(), 0, 0, 0, NULL );
    }
    if ( full )
        _updateRect = getRect();
    update( _updateRect, full );
    _updateRect.clear();
}

/// called on system configuration change: screen size and orientation
void CRGUIWindowBase::reconfigure( int flags )
{
    lvRect fs = _wm->getScreen()->getRect();
    if ( _fullscreen ) {
        setRect( fs );
    } else {
        lvRect rc = getRect();
        int dx = fs.width();
        int dy = fs.height();
        if ( rc.right > dx ) {
            rc.left -= rc.right - dx;
            rc.right = dx;
            if ( rc.left < 0 )
                rc.left = 0;
        }
        if ( rc.right > dx ) {
            rc.left -= rc.right - dx;
            rc.right = dx;
            if ( rc.left < 0 )
                rc.left = 0;
        }
        setRect( rc );
    }
}

/// returns true if key is processed (by default, let's translate key to command using accelerator table)
bool CRGUIWindowBase::onKeyPressed( int key, int flags )
{
    if ( _acceleratorTable.isNull() ) {
        CRLog::trace("CRGUIWindowBase::onKeyPressed( %d, %d) - no accelerator table specified!", key, flags );
        return !_passKeysToParent;
    }
    int cmd, param;
    if ( _acceleratorTable->translate( key, flags, cmd, param ) ) {
        CRLog::trace("Accelerator applied: key %d(%d) -> command(%d,%d)", key, flags, cmd, param );
		if ( cmd == GCMD_PASS_TO_PARENT ) {
			return false;
		}
        return onCommand( cmd, param );
    } else {
        CRLog::trace("Accelerator not found for key %d(%d)", key, flags );
        _acceleratorTable->dump();
    }
    return !_passKeysToParent;
}

void CRDocViewWindow::draw()
{
    lvRect clientRect = _rect;
    if ( !_skin.isNull() ) {
        clientRect = _skin->getClientRect( _rect );
        _skin->draw( *_wm->getScreen()->getCanvas(), _rect );
        if ( !_title.empty() ) {
            lvRect titleRect = _skin->getTitleRect( _rect );
            if ( !titleRect.isEmpty() ) {
                _skin->getTitleSkin()->draw( *_wm->getScreen()->getCanvas(), titleRect );
                _skin->getTitleSkin()->drawText( *_wm->getScreen()->getCanvas(), titleRect, _title );
            }
        }

    }
    LVDocImageRef pageImage = _docview->getPageImage(0);
    LVDrawBuf * drawbuf = pageImage->getDrawBuf();
    _wm->getScreen()->draw( drawbuf, clientRect.left, clientRect.top );
}

void CRDocViewWindow::setRect( const lvRect & rc )
{
    if ( rc == _rect )
        return;
    _rect = rc;
    lvRect clientRect = _rect;
    if ( !_skin.isNull() )
        clientRect = _skin->getClientRect( rc );
    _docview->Resize( clientRect.width(), clientRect.height() );
    setDirty();
}


void CRMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected )
{
    lvRect itemBorders = skin->getBorderWidths();
    skin->draw( buf, rc );
    buf.SetTextColor( skin->getTextColor() );
    buf.SetBackgroundColor( skin->getBackgroundColor() );
    int imgWidth = 0;
    int hh = rc.bottom - rc.top - itemBorders.top - itemBorders.bottom;
    if ( !_image.isNull() ) {
        int w = _image->GetWidth();
        int h = _image->GetHeight();
        buf.Draw( _image, rc.left + hh/2-w/2 + itemBorders.left, rc.top + hh/2 - h/2 + itemBorders.top, w, h );
        imgWidth = w + ITEM_MARGIN;
    }
    lvRect textRect = rc;
    textRect.left += imgWidth;
    skin->drawText( buf, textRect, _label, getFont() );
}

int CRMenu::getPageCount()
{
    return (_items.length() + _pageItems - 1) / _pageItems;
}

void CRMenu::setCurPage( int nPage )
{
    int oldTop = _topItem;
    _topItem = _pageItems * nPage;
#if FULL_SCROLL==1
    if ( _topItem >= (int)_items.length() )
        _topItem = ((int)_items.length() - 1) / _pageItems * _pageItems;
#else
    if ( _topItem + _pageItems >= (int)_items.length() )
        _topItem = (int)_items.length() - _pageItems;
#endif
    if ( _topItem < 0 )
        _topItem = 0;
    if ( _topItem != oldTop )
        setDirty();
}

int CRMenu::getCurPage( )
{
    return (_topItem + (_pageItems-1)) / _pageItems;
}

int CRMenu::getTopItem()
{
    return _topItem;
}

void CRMenu::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, bool selected )
{
    CRMenuItem::Draw( buf, rc, skin, selected );
    lString16 s = getSubmenuValue();
    if ( s.empty() )
        return;
    int w = _valueFont->getTextWidth( s.c_str(), s.length() );
    int hh = rc.bottom - rc.top;
    _valueFont->DrawTextString( &buf, rc.right - w - ITEM_MARGIN, rc.top + hh/2 - _valueFont->getHeight()/2, s.c_str(), s.length(), L'?', NULL, false, 0 );
}

lvPoint CRMenuItem::getItemSize( CRRectSkinRef skin )
{
    LVFontRef font = _defFont;
    if ( font.isNull() )
        font = skin->getFont();
    int h = font->getHeight() * 5/4;
    int w = font->getTextWidth( _label.c_str(), _label.length() );
    w += ITEM_MARGIN * 2;
    if ( !_image.isNull() ) {
        if ( _image->GetHeight()>h )
            h = _image->GetHeight() * 8 / 7;
        w += h;
    }
    lvPoint minsize = skin->getMinSize();
    if ( minsize.y>0 && h < minsize.y )
        h = minsize.y;
    if ( minsize.x>0 && w < minsize.x )
        w = minsize.x;
    return lvPoint( w, h );
}

CRMenuSkinRef CRMenu::getSkin()
{
    if ( !_skin.isNull() )
        return _skin;
    lString16 path = getSkinName();
    lString16 path2;
    if ( !path.startsWith( L"#" ) )
        path = lString16(L"/CR3Skin/") + path;
    else if ( _wm->getScreenOrientation()&1 )
        _skin = _wm->getSkin()->getMenuSkin( (path + L"-rotated").c_str() );
    if ( !_skin )
        _skin = _wm->getSkin()->getMenuSkin( path.c_str() );
    return _skin;
}

lvPoint CRMenu::getItemSize()
{
    CRMenuSkinRef skin = getSkin();
    CRRectSkinRef itemSkin = skin->getItemSkin();
    lvRect itemBorders = itemSkin->getBorderWidths();
    lvPoint sz = CRMenuItem::getItemSize( itemSkin );
    if ( !isSubmenu() || _propName.empty() || _props.isNull() )
        return sz;
    int maxw = 0;
    for ( int i=0; i<_items.length(); i++ ) {
        lString16 s = _items[i]->getLabel();
        int w = _valueFont->getTextWidth( s.c_str(), s.length() );
        if ( w > maxw )
            maxw = w;
    }
    if ( maxw>0 )
        sz.x = sz.x + itemBorders.left + itemBorders.right + maxw;
    return sz;
}

int CRMenu::getItemHeight()
{
    CRMenuSkinRef skin = getSkin();
    CRRectSkinRef itemSkin = skin->getItemSkin();
    CRRectSkinRef separatorSkin = skin->getSeparatorSkin();
    int separatorHeight = 0;
    if ( !separatorSkin.isNull() )
        separatorHeight = separatorSkin->getMinSize().y;
    int h = itemSkin->getFont()->getHeight() * 5/4;
    lvPoint minsize = skin->getMinSize();
    if ( minsize.y>0 && h < minsize.y )
        h = minsize.y;
    if ( _fullscreen ) {
        int nItems = _items.length();
        int scrollHeight = 0;
        CRScrollSkinRef sskin = skin->getScrollSkin();
        if ( nItems > _pageItems || !sskin->getAutohide() ) {
            nItems = _pageItems;
            scrollHeight = SCROLL_HEIGHT;
            if ( sskin->getMinSize().y>0 )
                scrollHeight = sskin->getMinSize().y;
        }
        lvRect rc(0,0,_wm->getScreen()->getWidth(), _wm->getScreen()->getHeight() );
        lvRect client = skin->getClientRect( rc );
        h = client.height() - scrollHeight - separatorHeight*(nItems-1) - _helpHeight;
        if ( nItems > 0 )
            h /= nItems;
    }

    return h;
}

lvPoint CRMenu::getMaxItemSize()
{
    CRMenuSkinRef skin = getSkin();
    CRRectSkinRef itemSkin = skin->getItemSkin();
    lvPoint mySize = getItemSize();
    //int itemHeight = getItemHeight();
    int maxx = 0;
    int maxy = 0;
    for ( int i=0; i<_items.length(); i++ ) {
        lvPoint sz = _items[i]->getItemSize( itemSkin );
        if ( maxx < sz.x )
            maxx = sz.x;
        if ( maxy < sz.y )
            maxy = sz.y;
    }
    if ( maxx < mySize.x )
        maxx = mySize.x;
    if ( maxy < mySize.y )
        maxy = mySize.y;
    if ( _fullscreen )
        maxy = getItemHeight();
    return lvPoint( maxx, maxy );
}

lvPoint CRMenu::getSize()
{
    if ( _fullscreen )
        return lvPoint( _wm->getScreen()->getWidth(), _wm->getScreen()->getHeight() );
    lvPoint itemSize = getMaxItemSize();
    int nItems = _items.length();
    int scrollHeight = 0;
    if ( nItems > _pageItems ) {
        nItems = _pageItems;
        scrollHeight = SCROLL_HEIGHT;
    }
    int h = nItems * (itemSize.y) + scrollHeight;
    int w = itemSize.x + 3 * ITEM_MARGIN + HOTKEY_SIZE;
    CRMenuSkinRef skin = getSkin();
    CRRectSkinRef sskin = skin->getItemShortcutSkin();
    CRRectSkinRef iskin = skin->getItemSkin();
    if ( !sskin.isNull() ) {
        lvPoint ssz = sskin->getMinSize();
        lvRect borders = sskin->getBorderWidths();
        w += ssz.x + borders.left + borders.right;
    }
    if ( !iskin.isNull() ) {
        lvRect borders = iskin->getBorderWidths();
        w += borders.left + borders.right;
    }
    if ( w>600 )
        w = 600;
    lvPoint res = skin->getWindowSize( lvPoint( w, h ) );
    if ( res.x > _wm->getScreen()->getWidth() )
        res.x = _wm->getScreen()->getWidth();
    if ( res.y > _wm->getScreen()->getHeight() )
        res.y = _wm->getScreen()->getHeight();
    return res;
}

lString16 CRMenu::getSubmenuValue()
{
    if ( !isSubmenu() || _propName.empty() || _props.isNull() )
        return lString16();
    lString16 value = getProps()->getStringDef(
                               UnicodeToUtf8(getPropName()).c_str(), "");
    for ( int i=0; i<_items.length(); i++ ) {
        if ( !_items[i]->getPropValue().empty() &&
                value==(_items[i]->getPropValue()) )
            return _items[i]->getLabel();
    }
    return lString16();
}

void CRMenu::toggleSubmenuValue()
{
    if ( !isSubmenu() || _propName.empty() || _props.isNull() )
        return;
    lString16 value = getProps()->getStringDef(
                               UnicodeToUtf8(getPropName()).c_str(), "");
    for ( int i=0; i<_items.length(); i++ ) {
        if ( !_items[i]->getPropValue().empty() &&
              value==(_items[i]->getPropValue()) ) {
            int n = (i + 1) % _items.length();
            getProps()->setString(UnicodeToUtf8(getPropName()).c_str(), _items[n]->getPropValue() );
            //return _items[i]->getLabel();
            return;
        }
    }
}

static void DrawArrow( LVDrawBuf & buf, int x, int y, int dx, int dy, lvColor cl, int direction )
{
    int x0 = x + dx/2;
    int y0 = y + dy/2;
    dx -= 4;
    dy -= 4;
    dx /= 2;
    dy /= 2;
    int deltax = direction?-1:1;
    x0 -= deltax * dx/2;
    for ( int k=0; k<dx; k++ ) {
        buf.FillRect( x0+k*deltax, y0-k, x0+k*deltax+1, y0+k+1, cl );
    }
}

int CRMenu::getScrollHeight()
{
    CRMenuSkinRef skin = getSkin();
    int nItems = _items.length();
    int scrollHeight = 0;
    CRScrollSkinRef sskin = skin->getScrollSkin();
    if ( nItems > _pageItems || !sskin->getAutohide() ) {
        nItems = _pageItems;
        scrollHeight = SCROLL_HEIGHT;
        if ( sskin->getMinSize().y>0 )
            scrollHeight = sskin->getMinSize().y;
    }
    return scrollHeight;
}

void CRMenu::Draw( LVDrawBuf & buf, int x, int y )
{
    CRMenuSkinRef skin = getSkin();
    CRRectSkinRef clientSkin = skin->getClientSkin();
    CRRectSkinRef titleSkin = skin->getTitleSkin();
    CRScrollSkinRef sskin = skin->getScrollSkin();
    CRRectSkinRef itemSkin = skin->getItemSkin();
    CRRectSkinRef itemShortcutSkin = skin->getItemShortcutSkin();
    CRRectSkinRef itemSelSkin = skin->getSelItemSkin();
    CRRectSkinRef itemSelShortcutSkin = skin->getSelItemShortcutSkin();
    CRRectSkinRef evenitemSkin = skin->getEvenItemSkin();
    CRRectSkinRef evenitemShortcutSkin = skin->getEvenItemShortcutSkin();
    CRRectSkinRef evenitemSelSkin = skin->getEvenSelItemSkin();
    CRRectSkinRef evenitemSelShortcutSkin = skin->getEvenSelItemShortcutSkin();
    if ( evenitemSkin.isNull() )
        evenitemSkin = itemSkin;
    if ( evenitemShortcutSkin.isNull() )
        evenitemShortcutSkin = itemShortcutSkin;
    if ( evenitemSelSkin.isNull() )
        evenitemSelSkin = itemSelSkin;
    if ( evenitemSelShortcutSkin.isNull() )
        evenitemSelShortcutSkin = itemSelShortcutSkin;

    CRRectSkinRef separatorSkin = skin->getSeparatorSkin();
    int separatorHeight = 0;
    if ( !separatorSkin.isNull() )
        separatorHeight = separatorSkin->getMinSize().y;

    lvRect itemBorders = itemSkin->getBorderWidths();
    lvRect headerRc = skin->getTitleRect(_rect);

    bool showShortcuts = skin->getShowShortcuts();

    buf.SetTextColor( 0x000000 );
    buf.SetBackgroundColor( 0xFFFFFF );

    skin->draw( buf, _rect );
    titleSkin->draw( buf, headerRc );
    titleSkin->drawText( buf, headerRc, _label );

    lvRect clientRect = skin->getClientRect(_rect);
    if ( !clientSkin.isNull() )
        clientSkin->draw( buf, clientRect );

    lvPoint itemSize = getMaxItemSize();
    //int hdrHeight = itemSize.y; // + ITEM_MARGIN + ITEM_MARGIN;
    lvPoint sz = getSize();

    //int nItems = _items.length();
    int scrollHeight = getScrollHeight();

    lvRect itemsRc( clientRect );
    itemsRc.bottom -= scrollHeight;
    //lvRect headerRc( x + itemBorders.left, y + itemBorders.top, x + sz.x - itemBorders.right, itemsRc.top+1 );
    lvRect scrollRc( skin->getClientRect(_rect) );
    scrollRc.top = scrollRc.bottom - scrollHeight;
    //buf.FillRect( x, y, x+sz.x, y+sz.y, buf.GetBackgroundColor() );
    //buf.Rect( headerRc, buf.GetTextColor() );
    //buf.Rect( itemsRc, buf.GetTextColor() );
    // draw scrollbar
    if ( scrollHeight ) {
        if ( !sskin.isNull() ) {
#if FULL_SCROLL==1
            int numItems = (_items.length() + _pageItems - 1) / _pageItems * _pageItems;// - 1;
#else
            int numItems = _items.length()
#endif
            sskin->drawScroll(  buf, scrollRc, false, _topItem, numItems, _pageItems );
        } else {
            int totalCount = _items.length();
            int visibleCount = _pageItems;
            buf.Rect( scrollRc, buf.GetTextColor() );
            scrollRc.shrink( 2 );
            buf.Rect( scrollRc.left, scrollRc.top, scrollRc.left+SCROLL_HEIGHT - 4, scrollRc.bottom, buf.GetTextColor() );
            buf.Rect( scrollRc.right - SCROLL_HEIGHT + 4, scrollRc.top, scrollRc.right, scrollRc.bottom, buf.GetTextColor() );
            DrawArrow( buf, scrollRc.left, scrollRc.top, SCROLL_HEIGHT-4, SCROLL_HEIGHT-4, _topItem>0?buf.GetTextColor() : 0xAAAAAA, 0 );
            DrawArrow( buf, scrollRc.right-SCROLL_HEIGHT+4, scrollRc.top, SCROLL_HEIGHT-4, SCROLL_HEIGHT-4, _topItem < totalCount - visibleCount ? buf.GetTextColor() : 0xAAAAAA, 1 );
            scrollRc.left += SCROLL_HEIGHT - 3;
            scrollRc.right -= SCROLL_HEIGHT - 3;
            int x = scrollRc.width() * _topItem / totalCount;
            int endx = scrollRc.width() * (_topItem + visibleCount) / totalCount;
            //CRLog::trace("scrollBar: x=%d, dx=%d, _topItem=%d, visibleCount=%d, totalCount=%d", x, endx, _topItem, visibleCount, totalCount );
            scrollRc.right = scrollRc.left + endx;
            scrollRc.left += x;
            buf.Rect( scrollRc, buf.GetTextColor() );
            scrollRc.shrink( 2 );
            buf.FillRect( scrollRc, 0xAAAAAA );
        }
    }
    //headerRc.shrink( 2 );
    //buf.FillRect( headerRc, 0xA0A0A0 );
    //headerRc.shrink( ITEM_MARGIN );
    //CRMenuItem::Draw( buf, headerRc, itemSkin, false );
    lvRect rc( itemsRc );
    rc.top += 0; //ITEM_MARGIN;
    //rc.left += ITEM_MARGIN;
    //rc.right -= ITEM_MARGIN;
    LVFontRef numberFont( fontMan->GetFont( MENU_NUMBER_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    for ( int index=0; index<_pageItems; index++ ) {
        int i = _topItem + index;
        if ( i >= _items.length() )
            break;
        bool selected = false; //TODO
        if ( !getProps().isNull() && !_items[i]->getPropValue().empty() &&
              getProps()->getStringDef(
                       UnicodeToUtf8(getPropName()).c_str()
                       , "")==(_items[i]->getPropValue()) )
            selected = true;

        rc.bottom = rc.top + itemSize.y;
        bool even = (i & 1);
        CRRectSkinRef is = selected
                           ? (even ? evenitemSelSkin : itemSelSkin)
                           : (even ? evenitemSkin : itemSkin);
        CRRectSkinRef ss = selected
                           ? (even ? evenitemSelShortcutSkin : itemSelShortcutSkin)
                           : (even ? evenitemShortcutSkin : itemShortcutSkin);
        if ( selected ) {
            lvRect sel = rc;
            sel.extend( 4 );
            //buf.FillRect(sel, itemSelSkin->getBackgroundColor() );
        }

        lvRect itemRc( rc );

        if ( showShortcuts ) {
            // number
            lvRect numberRc( rc );
            //numberRc.extend(ITEM_MARGIN/4); //ITEM_MARGIN/8-2);
            numberRc.right = numberRc.left + HOTKEY_SIZE;

            ss->draw( buf, numberRc );
            lString16 number = index<9 ? lString16::itoa( index+1 ) : L"0";
            buf.SetTextColor( ss->getTextColor() );
            buf.SetBackgroundColor( ss->getBackgroundColor() );
            ss->drawText( buf, numberRc, number );
            // item
            itemRc.left = numberRc.right;
        }

        is->setTextAlign( is->getTextAlign() | SKIN_EXTEND_TAB);
        CRMenuItem * item = _items[i];
        item->Draw( buf, itemRc, is, selected );

        // draw separator
        if ( separatorHeight>0 && index<_pageItems-1 ) {
            lvRect r(rc);
            r.top += itemSize.y;
            r.bottom = r.top + separatorHeight;
            separatorSkin->draw(buf, r);
        }
        rc.top += itemSize.y + separatorHeight;
    }
}

/// closes menu and its submenus
void CRMenu::destroyMenu()
{
    for ( int i=_items.length()-1; i>=0; i-- )
        if ( _items[i]->isSubmenu() ) {
            ((CRMenu*)_items[i])->destroyMenu();
            _items.remove( i );
        }
    _wm->closeWindow( this ); // close, for root menu
}

/// closes menu and its submenus
void CRMenu::closeMenu( int command, int params )
{
    for ( int i=0; i<_items.length(); i++ )
        if ( _items[i]->isSubmenu() )
            ((CRMenu*)_items[i])->closeMenu( 0, 0 );
    if ( _menu != NULL )
        _wm->showWindow( this, false ); // just hide, for submenus
    else {
        if ( command )
            _wm->postCommand( command, params );
        destroyMenu();
    }
}

/// closes top level menu and its submenus, posts command
void CRMenu::closeAllMenu( int command, int params )
{
    CRMenu* p = this;
    while ( p->_menu )
        p = p->_menu;
    if ( command )
        _wm->postCommand( command, params );
    p->destroyMenu();
}

/// called on system configuration change: screen size and orientation
void CRMenu::reconfigure( int flags )
{
    CRGUIWindowBase::reconfigure( flags );
    _skin.Clear();
    getSkin();
    _fullscreen = _fullscreen || _skin->getFullScreen();
    int pageItems = _pageItems;
    if ( _skin->getMinItemCount()>0 && pageItems<_skin->getMinItemCount() )
        pageItems = _skin->getMinItemCount();
    if ( _skin->getMaxItemCount()>0 && pageItems>_skin->getMaxItemCount() )
        pageItems = _skin->getMaxItemCount();
    if ( pageItems!=_pageItems ) {
        // change items per page
        _pageItems = pageItems;
        _topItem = _topItem / pageItems * pageItems;
    }
}

bool CRMenu::onKeyPressed( int key, int flags )
{
    CRGUIWindowBase::onKeyPressed( key, flags );
    return true; // don't allow key processing by parent window
}

/// returns true if command is processed
bool CRMenu::onCommand( int command, int params )
{
    CRLog::trace( "CRMenu::onCommand(%d, %d)", command, params );
    if ( command==MCMD_CANCEL ) {
        closeMenu( 0 );
        return true;
    }
    if ( command==MCMD_OK ) {
        int command = getId();
        if ( _menu != NULL )
            closeMenu( 0 );
        else
            closeMenu( command ); // close, for root menu
        return true;
    }
	
    if ( command==MCMD_SCROLL_FORWARD_LONG ) {
        setCurPage( getCurPage()+10 );
        return true;
    }
    if ( command==MCMD_SCROLL_BACK_LONG ) {
        setCurPage( getCurPage()-10 );
        return true;
    }
    if ( command==MCMD_SCROLL_FORWARD ) {
		if ( params==0 )
			params = 1;
        setCurPage( getCurPage()+params );
        return true;
    }
    if ( command==MCMD_SCROLL_BACK_LONG ) {
        setCurPage( getCurPage()-10 );
        return true;
    }
    if ( command==MCMD_SCROLL_BACK ) {
		if ( params==0 )
			params = 1;
        setCurPage( getCurPage()-params );
        return true;
    }
    int option = -1;
    int longPress = 0;
    if ( command>=MCMD_SELECT_0 && command<=MCMD_SELECT_9 )
        option = (command==MCMD_SELECT_0) ? 9 : command - MCMD_SELECT_1;
    if ( command>=MCMD_SELECT_0_LONG && command<=MCMD_SELECT_9_LONG ) {
        option = (command==MCMD_SELECT_0_LONG) ? 9 : command - MCMD_SELECT_1_LONG;
        longPress = 1;
    }
    if ( option < 0 ) {
        CRLog::error( "CRMenu::onCommand() - unsupported command %d, %d", command, params );
        return true;
    }
  
    option += getTopItem();
    if ( option >= getItems().length() )
        return true;
    CRLog::trace( "CRMenu::onCommand() - option %d selected", option );
    CRMenuItem * item = getItems()[option];
    if ( item->onSelect()>0 )
        return true;
    if ( item->isSubmenu() ) {
        CRMenu * menu = (CRMenu *)item;
        if ( menu->getItems().length() <= 3 ) {
            // toggle 2 and 3 choices w/o menu
            menu->toggleSubmenuValue();
            setDirty();
        } else {
            // show menu
            _wm->activateWindow( menu );
        }
        return true;
    } else {
        // command menu item
        if ( !item->getPropValue().empty() ) {
                // set property
            CRLog::trace("Setting property value");
            _props->setString( UnicodeToUtf8(getPropName()).c_str(), item->getPropValue() );
            int command = getId();
            if ( _menu != NULL )
                closeMenu( 0 );
            else
                closeMenu( command ); // close, for root menu
            return true;
        }
        int command = item->getId();
        if ( _menu != NULL )
            closeMenu( 0 );
        else
            closeMenu( command, longPress ); // close, for root menu
        return true;
    }
    return false;
}

const lvRect & CRMenu::getRect()
{
    lvPoint sz = getSize();
    lvRect rc = _wm->getScreen()->getRect();
    _rect = rc;
    _rect.top = _rect.bottom - sz.y;
    _rect.right = _rect.left + sz.x;
    return _rect;
}

void CRMenu::draw()
{
    Draw( *_wm->getScreen()->getCanvas(), _rect.left, _rect.top );
    //_wm->getScreen()->getCanvas()->Rect( _rect, 0xAAAAAA );
}

static bool readNextLine( const LVStreamRef & stream, lString16 & dst )
{
    lString8 line;
    bool flgComment = false;
    for ( ; ; ) {
        int ch = stream->ReadByte();
        if ( ch<0 )
            break;
        if ( ch=='#' && line.empty() )
            flgComment = true;
        if ( ch=='\r' || ch=='\n' ) {
            if ( flgComment ) {
                flgComment = false;
                line.clear();
            } else {
                                if ( line[0]==(lChar8)0xEf && line[1]==(lChar8)0xBB && line[2]==(lChar8)0xBF )
					line.erase( 0, 3 );
                if ( !line.empty() ) {
                    dst = Utf8ToUnicode( line );
                    return true;
                }
            }
        } else {
            line << ch;
        }
    }
    return false;
}

static bool splitLine( lString16 line, const lString16 & delimiter, lString16 & key, lString16 & value )
{
    if ( !line.empty() ) {
        unsigned n = line.pos(delimiter);
        value.clear();
        key = line;
        if ( n>0 && n <line.length()-1 ) {
            value = line.substr( n+1, line.length() - n - 1 );
            key = line.substr( 0, n );
            key.trim();
            value.trim();
            return key.length()!=0 && value.length()!=0;
        }
    }
    return false;
}

static int decodeKey( lString16 name )
{
    name.trim();
    if ( name.empty() )
        return 0;
    int key = 0;
    lChar16 ch0 = name[0];
    if ( ch0 >= '0' && ch0 <= '9' )
        return name.atoi();
    if ( ch0=='-' && name.length()>=2 && name[1] >= '0' && name[1] <= '9' )
        return name.atoi();
    if ( name.length()==3 && name[0]=='\'' && name[2]=='\'' )
        key = name[1];
    if ( name.length() == 1 )
        key = name[0];
    if ( key == 0 && name.length()>=4 && name[0]=='0' && name[1]=='x' ) {
        for ( unsigned i=2; i<name.length(); i++ ) {
            lChar16 ch = name[i];
            if ( ch>='0' && ch<='9' )
                key = key*16 + (ch-'0');
            else if ( ch>='a' && ch<='f' )
                key = key*16 + (ch-'a') + 10;
            else if ( ch>='A' && ch<='F' )
                key = key*16 + (ch-'A') + 10;
            else
                break;
        }
    }
    return key;
}

bool CRGUIAcceleratorTableList::openFromFile( const char  * defFile, const char * mapFile )
{
    _table.clear();
    LVHashTable<lString16, int> defs( 256 );
    LVStreamRef defStream = LVOpenFileStream( defFile, LVOM_READ );
    if ( defStream.isNull() ) {
        CRLog::error( "cannot open keymap def file %s", defFile );
        return false;
    }
    LVStreamRef mapStream = LVOpenFileStream( mapFile, LVOM_READ );
    if ( mapStream.isNull() ) {
        CRLog::error( "cannot open keymap file %s", defFile );
        return false;
    }
    lString16 line;
    CRPropRef props = LVCreatePropsContainer();
    while ( readNextLine(defStream, line) ) {
        lString16 name;
        lString16 value;
        if ( splitLine( line, lString16(L"="), name, value ) )  {
            int key = decodeKey( value );
            if ( key!=0 )
                defs.set( name, key );
            else
                CRLog::error("Unknown key definition in line %s", UnicodeToUtf8(line).c_str() );
        } else if ( !line.empty() )
            CRLog::error("Invalid definition in line %s", UnicodeToUtf8(line).c_str() );
    }
    if ( !defs.length() ) {
        CRLog::error("No definitions read from %s", defFile);
        return false;

    }
    lString16 section;
    CRGUIAcceleratorTableRef table( new CRGUIAcceleratorTable() );
    bool eof = false;
    do {
        eof = !readNextLine(mapStream, line);
        if ( eof || (!line.empty() && line[0]=='[') ) {
            // eof or [section] found
            // save old section
            if ( !section.empty() ) {
                if ( table->length() ) {
                    _table.set( section, table );
                }
                section.clear();
            }
            // begin new section
            if ( !eof ) {
                table = CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable() );
                int endbracket = line.pos( lString16(L"]") );
                if ( endbracket<=0 )
                    endbracket = line.length();
                if ( endbracket >= 2 )
                    section = line.substr( 1, endbracket - 1 );
                else
                    section.clear(); // wrong sectino
            }
        } else if ( !section.empty() ) {
            // read definition
            lString16 name;
            lString16 value;
            if ( splitLine( line, lString16(L"="), name, value ) ) {
                int flag = 0;
                int key = 0;
                lString16 keyName;
                lString16 flagName;
                splitLine( name, lString16(L","), keyName, flagName );
                if ( !flagName.empty() ) {
                    flag = decodeKey( flagName );
                    if ( !flag )
                        flag = defs.get( flagName );
                }
                // decoding key name
                key = decodeKey( keyName );
                if ( !key )
                    key = defs.get( keyName );
                if ( !key ) {
                    CRLog::error( "unknown key definition %s in line %s", UnicodeToUtf8(keyName).c_str(), UnicodeToUtf8(line).c_str() );
                    continue;
                }
                int cmd = 0;
                int cmdParam = 0;
                lString16 cmdName;
                lString16 paramName;
                splitLine( value, lString16(L","), cmdName, paramName );
                if ( !paramName.empty() ) {
                    cmdParam = decodeKey( paramName );
                    if ( !cmdParam )
                        cmdParam = defs.get( paramName );
                }
                cmd = decodeKey( cmdName );
                if ( !cmd )
                    cmd = defs.get( cmdName );
                if ( key != 0 && cmd != 0 ) {
                    // found valid key cmd definition
                    table->add( key, flag, cmd, cmdParam );
                    //CRLog::trace("Acc: %d, %d => %d, %d", key, flag, cmd, cmdParam);
                } else {
                    CRLog::error( "unknown command definition %s in line %s", UnicodeToUtf8(cmdName).c_str(), UnicodeToUtf8(line).c_str() );
                    continue;
                }
            }
        }
    } while ( !eof );
    return !empty();
}

// get currently set layout
CRKeyboardLayoutRef CRKeyboardLayoutList::getCurrentLayout()
{
	if ( !_current.isNull() )
		return _current;
	_current = get( lString16( L"english" ) );
	if ( !_current )
		nextLayout();
	return _current;
}

// get next layout
CRKeyboardLayoutRef CRKeyboardLayoutList::prevLayout()
{
	bool found = false;
	CRKeyboardLayoutRef prev;
	CRKeyboardLayoutRef next;
	CRKeyboardLayoutRef first;
	CRKeyboardLayoutRef last;
	LVHashTable<lString16, CRKeyboardLayoutRef>::iterator i( _table );
	for ( ;; ) {
		LVHashTable<lString16, CRKeyboardLayoutRef>::pair * item = i.next();
		if ( !item )
			break;
		if ( first.isNull() )
			first = item->value;
		last = item->value;
		if ( item->value.get() == _current.get() ) {
			found = true;
		} else {
			if ( !found )
				prev = item->value;
			if ( !found && !next.isNull() )
				next = item->value;

		}
	}
	if ( prev.isNull() )
		_current = last;
	else
		_current = prev;
	return _current;
}

// get next layout
CRKeyboardLayoutRef CRKeyboardLayoutList::nextLayout()
{
	bool found = false;
	CRKeyboardLayoutRef prev;
	CRKeyboardLayoutRef next;
	CRKeyboardLayoutRef first;
	CRKeyboardLayoutRef last;
	LVHashTable<lString16, CRKeyboardLayoutRef>::iterator i( _table );
	for ( ;; ) {
		LVHashTable<lString16, CRKeyboardLayoutRef>::pair * item = i.next();
		if ( !item )
			break;
		if ( first.isNull() )
			first = item->value;
		last = item->value;
		if ( item->value.get() == _current.get() ) {
			found = true;
		} else {
			if ( !found )
				prev = item->value;
			else if ( next.isNull() )
				next = item->value;

		}
	}
	if ( next.isNull() )
		_current = first;
	else
		_current = next;
	return _current;
}

bool CRKeyboardLayoutList::openFromFile( const char  * layoutFile )
{
    //_table.clear();
    LVStreamRef stream = LVOpenFileStream( layoutFile, LVOM_READ );
    if ( stream.isNull() ) {
        CRLog::error( "cannot open keyboard layout file %s", layoutFile );
        return false;
    }
    lString16 line;
    lString16 section;
	CRKeyboardLayoutRef table;
	LVRef<CRKeyboardLayout> layout;
    bool eof = false;
    do {
        eof = !readNextLine(stream, line);
        if ( eof || (!line.empty() && line[0]=='[') ) {
            // eof or [section] found
            // save old section
            if ( !section.empty() ) {
                if ( layout->getItems().length() ) {
                    _table.set( section, table );
                }
                section.clear();
            }
            // begin new section
            if ( !eof ) {
                int endbracket = line.pos( lString16(L"]") );
                if ( endbracket<=0 )
                    endbracket = line.length();
                if ( endbracket >= 2 )
                    section = line.substr( 1, endbracket - 1 );
                else
                    section.clear(); // wrong sectino
				lString16 langname;
				lString16 layouttype;
				if ( !section.empty() && splitLine( section, lString16(L"."), langname, layouttype ) ) {
					table = _table.get( langname );
					if ( table.isNull() ) {
						table = CRKeyboardLayoutRef( new CRKeyboardLayoutSet() );
						_table.set( langname, table );
					}
					if ( layouttype == L"tx" )
						layout = table->tXKeyboard;
					else
						layout = table->vKeyboard;
				} else
					section.clear();
            }
        } else if ( !section.empty() ) {
            // read definition
            lString16 name;
            lString16 value;
            if ( splitLine( line, lString16(L"="), name, value ) ) {
				if ( name == L"enabled" ) {
					//if ( value == L"0" )
					//	; //TODO:set disabled flag
					continue;
				}
                int item;
				if ( !name.atoi(item) )
					continue;
				layout->set( item, value );
            }
        }
    } while ( !eof );
	return _table.length()>0;
}



static int inv_control_table[] = {
    // old cmd, new cmd, param multiplier
    DCMD_LINEUP, DCMD_LINEDOWN, 1,
    DCMD_LINEDOWN, DCMD_LINEUP, 1,
    DCMD_PAGEUP, DCMD_PAGEDOWN, 1,
    DCMD_PAGEDOWN, DCMD_PAGEUP, 1,
    DCMD_MOVE_BY_CHAPTER, DCMD_MOVE_BY_CHAPTER, -1,
    0, 0, 0, 0,
};

/// returns true if command is processed
bool CRDocViewWindow::onCommand( int command, int params )
{
    if ( command >= LVDOCVIEW_COMMANDS_START && command <= LVDOCVIEW_COMMANDS_END ) {
        // TODO: rework controls inversion
#if CR_INTERNAL_PAGE_ORIENTATION==1
        cr_rotate_angle_t a = _docview->GetRotateAngle();
		if ( a==CR_ROTATE_ANGLE_90 || a==CR_ROTATE_ANGLE_180 ) {
			// inverse controls
			for ( int i=0; inv_control_table[i]; i+=3 ) {
				if ( command == inv_control_table[i] ) {
					command = inv_control_table[i+1];
					params *= inv_control_table[i+2];
					break;
				}
			}
		}
#endif
        _docview->doCommand( (LVDocCmd)command, params );
        _dirty = true;
        return true;
    }
    return !_passCommandsToParent;
}
