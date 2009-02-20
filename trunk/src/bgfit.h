//
// C++ Interface: selection navigation dialog
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
// bigfit.h

#ifndef BIGFIT_H_INCLUDED
#define BIGFIT_H_INCLUDED

#include "mainwnd.h"

/// window to show on top of DocView window, shifting/stretching DOCView content to fit
class BackgroundFitWindow : public CRGUIWindowBase
{
protected:
    CRDocViewWindow * _mainwin;
    virtual void draw();
public:
    BackgroundFitWindow(  CRGUIWindowManager * wm, CRDocViewWindow * mainwin )
    : CRGUIWindowBase(wm),
        _mainwin(mainwin)
    {
        _fullscreen = true;
    }
};

#endif
