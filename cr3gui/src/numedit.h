//
// C++ Interface: number editor dialog
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// numedit.h

#ifndef NUMEDIT_H_INCLUDED
#define NUMEDIT_H_INCLUDED


#include "mainwnd.h"

class CRNumberEditDialog : public CRGUIWindowBase
{
    protected:
        lString16 _title;
        lString16 _value;
        int _minvalue;
        int _maxvalue;
        int _resultCmd;
        CRWindowSkinRef _skin;
        virtual void draw();
    public:
        CRNumberEditDialog( CRGUIWindowManager * wm, lString16 title, lString16 initialValue, int resultCmd, int minvalue, int maxvalue );
        virtual ~CRNumberEditDialog()
        {
        }
        bool digitEntered( lChar16 c );
        /// returns true if command is processed
        virtual bool onCommand( int command, int params );
};


#endif
