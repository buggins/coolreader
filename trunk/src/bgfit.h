#ifndef __BGFIT_H 
#define __BGFIT_H 1

#include <lvstring.h>
#include <lvref.h>
#include <lvarray.h>
#include <lvtinydom.h>
#include <lvdocview.h>

#include <crgui.h>
#include <crtrace.h>

#include "mainwnd.h"


/// window to show on top of DocView window, shifting/stretching DOCView content to fit
class BackgroundFitWindow : public CRGUIWindowBase
{
protected:
    V3DocViewWin * mainwin_;
    LVDocView& docview_;
    virtual void draw();
public:
    BackgroundFitWindow(  CRGUIWindowManager * wm, V3DocViewWin * mainwin )
    : CRGUIWindowBase(wm),
        mainwin_(mainwin),
        docview_(*mainwin->getDocView())
    {
        _fullscreen = true;
    }
};

#endif
