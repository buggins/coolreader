//
// C++ Implementation: on-screen keyboard
//
// Description: 
//      Shows keyboard, and allows to input text string
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
// scrkbd.cpp

#include "scrkbd.h"

lChar16 CRScreenKeyboard::digitsToChar( lChar16 digit1, lChar16 digit2 )
{
    int row = digit1 - '1';
    int col = digit2=='0' ? 9: digit2 - '1';
    if ( row < 0 || row >= _rows )
        return 0;
    if ( col < 0 || col >= _cols )
        return 0;
    lString16 s = _keymap[ row ];
    if ( col < (int)s.length() )
        return s[col];
    return ' ';
}

bool CRScreenKeyboard::digitEntered( lChar16 c )
{
    if ( _lastDigit==0 ) {
        _lastDigit = c;
        return false;
    }
    lChar16 ch = digitsToChar( _lastDigit, c );
    _lastDigit = 0;
    if ( !ch )
        return false;
    _value << ch;
    setDirty();
    return true;
}

void CRScreenKeyboard::setLayout( CRKeyboardLayoutRef layout )
{
	_keymap.clear();
	unsigned maxcols = 0;
	if ( !layout.isNull() ) {
		for ( unsigned i=1; i<layout->vKeyboard->getItems().length(); i++ ) {
			lString16 s = layout->vKeyboard->get( i );
			if ( !s.empty() )
				_keymap.add( s );
			if ( s.length() > maxcols )
				maxcols = s.length();
		}
	}
    _rows = _keymap.length();
	_cols = maxcols<10 ? maxcols : 10;
	if ( _cols<=0 || _rows<=0 )
		setDefaultLayout();
	setDirty();
}

void CRScreenKeyboard::setDefaultLayout()
{
    _keymap.add(lString16(L"1234567890"));
    _keymap.add(lString16(L"abcdefghij"));
    _keymap.add(lString16(L"klmnopqrst"));
    _keymap.add(lString16(L"uvwxyz.,!?"));
    _keymap.add(lString16(L"+-'\":;   "));
    _rows = _keymap.length();
}

CRScreenKeyboard::CRScreenKeyboard(CRGUIWindowManager * wm, int id, const lString16 & caption, lString16 & buffer, lvRect & rc)
: CRGUIWindowBase( wm ), _buffer( buffer ), _value( buffer ), _title( caption ), _resultCmd(id), _lastDigit(0)
{
    _passKeysToParent = false;
    _passCommandsToParent = false;
    _rect = rc;
    _fullscreen = false;
    _skin = _wm->getSkin()->getMenuSkin( L"#vkeyboard" );
    //_skin = _wm->getSkin()->getWindowSkin( getSkinName().c_str() );
    setAccelerators( _wm->getAccTables().get("vkeyboard") );
    _cols = 10;
	setLayout( wm->getKeyboardLayouts().getCurrentLayout() );
}

void CRScreenKeyboard::draw()
{
    CRRectSkinRef titleSkin = _skin->getTitleSkin();
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    CRRectSkinRef itemSkin = _skin->getItemSkin();
    CRRectSkinRef shortcutSkin = _skin->getItemShortcutSkin();
    lvRect borders = clientSkin->getBorderWidths();
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    _skin->draw( *drawbuf, _rect );
    lvRect titleRect = _skin->getTitleRect( _rect );
    titleSkin->draw( *drawbuf, titleRect );
    titleSkin->drawText( *drawbuf, titleRect, _title );
    // draw toc
    lvRect inputRect = _skin->getClientRect( _rect );
    inputRect.shrinkBy( borders );
    lvRect kbdRect = inputRect;
    inputRect.bottom = inputRect.top + 40; // TODO
    kbdRect.top = inputRect.bottom;

    //clientSkin->draw( *drawbuf, kbdRect );
    int dx = kbdRect.width() / (_cols+1);
    int dy = kbdRect.height() / (_rows+1);
    for ( int y = 0; y<=_rows; y++ ) {
        for ( int x = 0; x<=_cols; x++ ) {
            lString16 txt;
            bool header = true;
            if ( y==0 && x>0 ) {
                txt = lString16::itoa( x<10 ? x : 0 );
            } else if ( x==0 && y>0 ) {
                txt = lString16::itoa( y );
            } else if ( x>0 && y>0 ) {
                header = false;
                lString16 s = _keymap[ y - 1 ];
                if ( x-1 < (int)s.length() )
                    txt = lString16(&s[ x - 1 ], 1);
                else
                    txt = L" ";
            }
            lvRect rc = kbdRect;
            rc.top += dy * y;
            rc.left += dx * x;
            rc.bottom = rc.top + dy;
            rc.right = rc.left + dx;
			if ( header )
				shortcutSkin->draw( *drawbuf, rc );
			else
				itemSkin->draw( *drawbuf, rc );
            //drawbuf->FillRect( rc, header ? 0xAAAAAA : 0xFFFFFF );
            //drawbuf->Rect( rc, header ? 0x000000 : 0x555555 );
            if ( !txt.empty() ) {
				if ( header )
					shortcutSkin->drawText( *drawbuf, rc, txt );
				else
					itemSkin->drawText( *drawbuf, rc, txt );
            }
        }
    }
    // draw input area
    clientSkin->draw( *drawbuf, inputRect );
    clientSkin->drawText( *drawbuf, inputRect, lString16(" ") + _value+L"_" );
}

/// returns true if command is processed
bool CRScreenKeyboard::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_CANCEL:
        if ( _lastDigit!=0 )
            _lastDigit = 0;
        else if ( _value.length()>0 ) {
            _value.erase( _value.length()-1, 1 );
            setDirty();
        } else {
            _wm->closeWindow( this );
        }
        return true;
    case MCMD_OK:
        {
            if ( !_value.empty() ) {
                _buffer = _value;
                _wm->postCommand( _resultCmd, 1 );
                _wm->closeWindow( this );
                return true;
            }
            _wm->closeWindow( this );
            return true;
        }
    case MCMD_KBD_NEXTLAYOUT:
    case MCMD_SCROLL_FORWARD:
        {
			setLayout( _wm->getKeyboardLayouts().nextLayout() );
        }
        break;
	case MCMD_KBD_PREVLAYOUT:
    case MCMD_SCROLL_BACK:
        {
			setLayout( _wm->getKeyboardLayouts().prevLayout() );
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
