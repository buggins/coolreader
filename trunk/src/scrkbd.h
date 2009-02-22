//
// C++ Interface: on-screen keyboard
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// scrkbd.h

#ifndef SCRKBD_H_INCLUDED
#define SCRKBD_H_INCLUDED

#include "mainwnd.h"


class CRScreenKeyboard : public CRGUIWindowBase
{
protected:
    lString16 & _buffer;
    lString16 _value;
    CRMenuSkinRef _skin;
    lString16 _title;
    int _resultCmd;
    lChar16 _lastDigit;
    int _rows;
    int _cols;
    lString16Collection _keymap;
    virtual void draw();
    virtual lChar16 digitsToChar( lChar16 digit1, lChar16 digit2 );
    bool digitEntered( lChar16 c );
public:
	void setDefaultLayout();
	void setLayout( CRKeyboardLayoutRef layout );
    CRScreenKeyboard(CRGUIWindowManager * wm, int id, const lString16 & caption, lString16 & buffer, lvRect & rc);

    virtual ~CRScreenKeyboard() { }

    //virtual const lvRect & getRect();

    //virtual lvPoint getSize();

    /// returns true if command is processed
    virtual bool onCommand( int command, int params );

};


#endif
