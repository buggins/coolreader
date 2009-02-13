//
// C++ Implementation: number editor dialog
//
// Description: 
//      Allows to select link from current page, and go to it.
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "numedit.h"

void CRNumberEditDialog::draw()
{
    CRRectSkinRef titleSkin = _skin->getTitleSkin();
    CRRectSkinRef clientSkin = _skin->getClientSkin();
    LVRef<LVDrawBuf> drawbuf = _wm->getScreen()->getCanvas();
    _skin->draw( *drawbuf, _rect );
    lvRect titleRect = _skin->getTitleRect( _rect );
    titleSkin->draw( *drawbuf, titleRect );
    titleSkin->drawText( *drawbuf, titleRect, _title );
    lvRect clientRect = _skin->getClientRect( _rect );
    clientSkin->draw( *drawbuf, clientRect );
    _skin->drawText( *drawbuf, _rect, _value+L"_" );
}

CRNumberEditDialog::CRNumberEditDialog( CRGUIWindowManager * wm, lString16 title, lString16 initialValue, int resultCmd, int minvalue, int maxvalue )
: CRGUIWindowBase( wm ), _title(title), _value(initialValue), _minvalue(minvalue), _maxvalue(maxvalue), _resultCmd(resultCmd)
{
    _skin = _wm->getSkin()->getWindowSkin(L"#toc");
    _fullscreen = false;
    lvPoint clientSize( 250, _skin->getFont()->getHeight() + 24 );
    lvPoint sz = _skin->getWindowSize( clientSize );
    lvRect rc = _wm->getScreen()->getRect();
    int x = (rc.width() - sz.x) / 2;
    int y = (rc.height() - sz.y) / 2;
    _rect.left = x;
    _rect.top = y;
    _rect.right = x + sz.x;
    _rect.bottom = y + sz.y;
}
bool CRNumberEditDialog::digitEntered( lChar16 c )
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
bool CRNumberEditDialog::onCommand( int command, int params )
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
        break;
    case MCMD_SCROLL_BACK:
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
        return false;
    }
    return true;
}
