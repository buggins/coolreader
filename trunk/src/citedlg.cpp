#include <lvstring.h>
#include <lvref.h>
#include <lvarray.h>
#include <lvtinydom.h>
#include <lvdocview.h>

#include <crgui.h>
#include <crtrace.h>

#include "mainwnd.h"
#include "bgfit.h"
#include "citedlg.h"
#include "citecore.h"

class CiteWindow : public BackgroundFitWindow
{
    CiteSelection selector_;
protected:
    virtual void draw()
    {
        CRLog::info("CiteWindow::draw()\n");
        BackgroundFitWindow::draw();
        CRRectSkinRef skin = _wm->getSkin()->getWindowSkin( L"#dialog" )->getClientSkin();
        LVDrawBuf * buf = _wm->getScreen()->getCanvas().get();
        skin->draw( *buf, _rect );
        lString16 prompt(L"Select text");
        buf->FillRect( _rect, 0xAAAAAA );
        lvRect keyRect = _rect;
        lvRect borders = skin->getBorderWidths();
        LVFontRef font = fontMan->GetFont( 20, 600, false, css_ff_sans_serif, lString8("Arial")); //skin->getFont();
//        int margin = 4;
        keyRect.right = _rect.right;
        if ( !keyRect.isEmpty() ) {
            skin->draw( *_wm->getScreen()->getCanvas(), keyRect );
            skin->drawText( *_wm->getScreen()->getCanvas(), keyRect, prompt );
        }
    }

public:

	CiteWindow( CRGUIWindowManager * wm, V3DocViewWin * mainwin) :
		BackgroundFitWindow(wm, mainwin),
		selector_(*mainwin->getDocView())
    {

		this->setAccelerators( mainwin->getDialogAccelerators() );
		_rect = _wm->getScreen()->getRect();
		//_rect.bottom = _rect.top;
		_rect.top = _rect.bottom - 40;
        selector_.highlight();
        setDirty();
	}

	bool onCommand( int command, int params )
	{
		switch ( command ) {
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
				break;
			case MCMD_SCROLL_FORWARD:
                selector_.stepDown();
                setDirty();
				break;
			case MCMD_SCROLL_BACK:
                selector_.stepUp();
                setDirty();
				break;
			case MCMD_OK:
				{
				};
				break;
			case MCMD_CANCEL:
                close();
				break;
		}
		return true;
	}

    void close() {
        CRLog::info("Closing cite");
        _mainwin->getDocView()->clearSelection();
        _wm->closeWindow(this);
    };

protected:
    CiteWindow(const CiteWindow&); //non-copyable
};



void activate_cite( CRGUIWindowManager *wm, V3DocViewWin * mainwin)
{
    CRLog::info("Entering cite mode\n");
    wm->activateWindow(new CiteWindow(wm, mainwin));
}
