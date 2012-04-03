#include <lvstring.h>
#include <lvref.h>
#include <lvarray.h>
#include <lvtinydom.h>
#include <lvdocview.h>

#include <crgui.h>
#include <crtrace.h>
#include <cri18n.h>

#include "mainwnd.h"
#include "bgfit.h"
#include "citedlg.h"
#include "citecore.h"
#ifdef CR_POCKETBOOK
#include "cr3pocketbook.h"
#endif

class CiteWindow : public BackgroundFitWindow
{
    CiteSelection selector_;
	V3DocViewWin * mainwin_;
#ifdef CR_POCKETBOOK
	int _selectedIndex;
	int _itemsCount;

	int getCommandFromIndex(int command) 
	{
		switch (_selectedIndex) {
		case 0:
			//move
			if (command == MCMD_SCROLL_BACK)
				return MCMD_SELECT_5; //up
			else if (command == MCMD_SCROLL_FORWARD)
				return MCMD_SELECT_6; //down
			break;
		case 1:
			//grow
			if (command == MCMD_SCROLL_BACK)
				return MCMD_SELECT_1; //up
			else if (command == MCMD_SCROLL_FORWARD)
				return MCMD_SELECT_0; //down
			break;
		case 2:
			//shrink
			if (command == MCMD_SCROLL_BACK)
				return MCMD_SELECT_9; 
			else if (command == MCMD_SCROLL_FORWARD)
				return MCMD_SELECT_2;
			break;
		case 3:
			// grow phrase:
			if (command == MCMD_SCROLL_BACK)
				return MCMD_SELECT_3; // up
			else if (command == MCMD_SCROLL_FORWARD)
				return MCMD_SELECT_8; // down
		case 4:
			// Shrink phrase
			if (command == MCMD_SCROLL_BACK)
				return MCMD_SELECT_7;
			else if (command == MCMD_SCROLL_FORWARD)
				return MCMD_SELECT_4;
			break;
		}
		return -1;
	}
#endif
protected:
    virtual void draw()
    {
        BackgroundFitWindow::draw();
        CRRectSkinRef skin = _wm->getSkin()->getWindowSkin( L"#dialog" )->getClientSkin();
        LVDrawBuf * buf = _wm->getScreen()->getCanvas().get();
        skin->draw( *buf, _rect );
        lvRect borders = skin->getBorderWidths();
#ifdef CR_POCKETBOOK
        lString16 prompt;
        switch (_selectedIndex) {
        case 0:
            prompt = lString16(_("Select next/prev paragraph"));
            break;
        case 1:
            prompt = lString16(_("Select one more paragraph"));
            break;
        case 2:
            prompt = lString16(_("Deselect paragraph"));
            break;
        case 3:
            prompt = lString16(_("Select one more phrase"));
            break;
        case 4:
            prompt = lString16(_("Deselect phrase"));
            break;

        }
        lvRect keyRect = _rect;
        int promptWidth = skin->measureText(prompt).x;
        keyRect.right = keyRect.left + promptWidth + borders.left + borders.right;
        if ( !keyRect.isEmpty() ) {
            skin->draw( *_wm->getScreen()->getCanvas(), keyRect );
            skin->drawText( *_wm->getScreen()->getCanvas(), keyRect, prompt );
        }
        CRToolBarSkinRef tbSkin = _wm->getSkin()->getToolBarSkin( L"#cite-toolbar" );
        if (!tbSkin.isNull()) {
            keyRect.left += (borders.right + _wm->getScreen()->getWidth() * 2/3/*promptWidth*/);
            keyRect.right = _rect.right;
            CRButtonListRef buttons = tbSkin->getButtons();
            if (!(buttons.isNull() || _itemsCount != buttons->length()))
                tbSkin->drawToolBar(*_wm->getScreen()->getCanvas(), keyRect, true, _selectedIndex);
        }
#else
                lString16 prompt(_("Select text"));
		buf->FillRect( _rect, 0xAAAAAA );
		lvRect keyRect = _rect;
		LVFontRef font = fontMan->GetFont( 20, 600, false, css_ff_sans_serif, lString8("Arial")); //skin->getFont();
//        int margin = 4;
		keyRect.right = _rect.right;
		if ( !keyRect.isEmpty() ) {
			skin->draw( *_wm->getScreen()->getCanvas(), keyRect );
			skin->drawText( *_wm->getScreen()->getCanvas(), keyRect, prompt );
		}
#endif
    }

public:

	CiteWindow( CRGUIWindowManager * wm, V3DocViewWin * mainwin) :
		BackgroundFitWindow(wm, mainwin),
		selector_(*mainwin->getDocView()),
		mainwin_(mainwin)
    {
		CRGUIAcceleratorTableRef acc = _wm->getAccTables().get("cite");
		if ( acc.isNull() )
			this->setAccelerators( mainwin->getDialogAccelerators() );
		else
			setAccelerators( acc );
		_rect = _wm->getScreen()->getRect();
		//_rect.bottom = _rect.top;
		_rect.top = _rect.bottom - 40;
        selector_.highlight();
        setDirty();
#ifdef CR_POCKETBOOK
		_itemsCount = 5;
                _selectedIndex = 0;
#endif
	}

	bool onCommand( int command, int params )
	{
		switch ( command ) {
			case MCMD_SELECT_1:
                selector_.growUp();
                setDirty();
                break;
			case MCMD_SELECT_2:
                selector_.shrinkDown();
                setDirty();
                break;
			case MCMD_SELECT_5:
                selector_.moveUp();
                setDirty();
                break;
			case MCMD_SELECT_6:
                selector_.moveDown();
                setDirty();
                break;
			case MCMD_SELECT_3:
                selector_.growUpPhrase();
                setDirty();
                break;
			case MCMD_SELECT_4:
                selector_.shrinkDownPhrase();
                setDirty();
                break;
			case MCMD_SELECT_7:
                selector_.shrinkUpPhrase();
                setDirty();
                break;
			case MCMD_SELECT_8:
                selector_.growDownPhrase();
                setDirty();
                break;
			case MCMD_SELECT_9:
                selector_.shrinkUp();
                setDirty();
                break;
			case MCMD_SELECT_0:
                selector_.growDown();
                setDirty();
				break;
			case MCMD_SCROLL_FORWARD:
			case MCMD_SCROLL_BACK:
#ifdef CR_POCKETBOOK
				{
					int cmd = getCommandFromIndex(command);
					if (cmd != -1)
						_wm->postCommand(cmd, params);
				}
#endif			
				break;
			case MCMD_OK:
				{
					ldomXRange range;
					selector_.getRange(range);
					if ( !range.isNull() ) {
                        mainwin_->getDocView()->saveRangeBookmark( range, bmkt_comment, lString16::empty_str);
                        mainwin_->saveHistory(lString16::empty_str);
					}
					close();
				};
				break;
			case MCMD_CANCEL:
                close();
				break;
#ifdef CR_POCKETBOOK
			case PB_CMD_RIGHT:
				if (_selectedIndex == _itemsCount -1)
					_selectedIndex = 0;
				else
					_selectedIndex += 1;
				setDirty();
				break;
			case PB_CMD_LEFT:
				if (_selectedIndex == 0)
					_selectedIndex = _itemsCount -1;
				else
					_selectedIndex -= 1;
				setDirty();
				break;
#endif
		}
		return true;
	}

    void close() {
        CRLog::info("Closing cite");
        _mainwin->getDocView()->clearSelection();
        _mainwin->getDocView()->updateBookMarksRanges();
        _wm->closeWindow(this);
    }

protected:
    CiteWindow(const CiteWindow&); //non-copyable
};



void activate_cite( CRGUIWindowManager *wm, V3DocViewWin * mainwin)
{
    CRLog::info("Entering cite mode\n");
    wm->activateWindow(new CiteWindow(wm, mainwin));
}
