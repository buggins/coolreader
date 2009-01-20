//
// C++ Implementation: settings
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "mainwnd.h"



#ifdef WITH_DICT
#include "dictdlg.h"
#endif


class CRNumberEditDialog : public CRGUIWindowBase
{
    protected:
        lString16 _title;
        lString16 _value;
        int _minvalue;
        int _maxvalue;
        int _resultCmd;
        CRWindowSkinRef _skin;
        virtual void draw()
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
    public:
        CRNumberEditDialog( CRGUIWindowManager * wm, lString16 title, lString16 initialValue, int resultCmd, int minvalue, int maxvalue )
        : CRGUIWindowBase( wm ), _title(title), _value(initialValue), _minvalue(minvalue), _maxvalue(maxvalue), _resultCmd(resultCmd)
        {
            _skin = _wm->getSkin()->getWindowSkin(L"#dialog");
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
        virtual ~CRNumberEditDialog()
        {
        }
        bool digitEntered( lChar16 c )
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
        virtual bool onCommand( int command, int params )
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
};

static const char * link_back_active[] = {
	"30 27 5 1",
	"0 c #000000",
	"o c #A1A1A1",
	". c #FFFFFF",
	"x c #A1A1A1",
	"  c None",
	"..............................",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".x.........00...............x.",
	".x........0o0...............x.",
	".x.......0oo0...............x.",
	".x......0ooo0...............x.",
	".x.....0oooo0...............x.",
	".x....0ooooo00000000000000..x.",
	".x...0ooooooooooooooooooo0..x.",
	".x..0oooooooooooooooooooo0..x.",
	".x.0ooooooooooooooooooooo0..x.",
	".x..0oooooooooooooooooooo0..x.",
	".x...0ooooooooooooooooooo0..x.",
	".x....0ooooo00000000000000..x.",
	".x.....0oooo0...............x.",
	".x......0ooo0...............x.",
	".x.......0oo0...............x.",
	".x........0o0...............x.",
	".x.........00...............x.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	"..............................",
};
static const char * link_forward_active[] = {
	"30 27 5 1",
	"0 c #000000",
	"o c #A1A1A1",
	". c #FFFFFF",
	"x c #A1A1A1",
	"  c None",
	"..............................",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".x..............00..........x.",
	".x..............0o0.........x.",
	".x..............0oo0........x.",
	".x..............0ooo0.......x.",
	".x..............0oooo0......x.",
	".x.00000000000000ooooo0.....x.",
	".x.0ooooooooooooooooooo0....x.",
	".x.0oooooooooooooooooooo0...x.",
	".x.0ooooooooooooooooooooo0..x.",
	".x.0oooooooooooooooooooo0...x.",
	".x.0ooooooooooooooooooo0....x.",
	".x.00000000000000ooooo0.....x.",
	".x..............0oooo0......x.",
	".x..............0ooo0.......x.",
	".x..............0oo0........x.",
	".x..............0o0.........x.",
	".x..............00..........x.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	"..............................",
};
static const char * link_back_normal[] = {
	"30 27 5 1",
	"0 c #515151",
	"o c #FFFFFF",
	". c #FFFFFF",
	"x c #A1A1A1",
	"  c None",
	"..............................",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".x.........00...............x.",
	".x........0o0...............x.",
	".x.......0oo0...............x.",
	".x......0ooo0...............x.",
	".x.....0oooo0...............x.",
	".x....0ooooo00000000000000..x.",
	".x...0ooooooooooooooooooo0..x.",
	".x..0oooooooooooooooooooo0..x.",
	".x.0ooooooooooooooooooooo0..x.",
	".x..0oooooooooooooooooooo0..x.",
	".x...0ooooooooooooooooooo0..x.",
	".x....0ooooo00000000000000..x.",
	".x.....0oooo0...............x.",
	".x......0ooo0...............x.",
	".x.......0oo0...............x.",
	".x........0o0...............x.",
	".x.........00...............x.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	"..............................",
};
static const char * link_forward_normal[] = {
	"30 27 5 1",
	"0 c #515151",
	"o c #FFFFFF",
	". c #FFFFFF",
	"x c #A1A1A1",
	"  c None",
	"..............................",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".x..............00..........x.",
	".x..............0o0.........x.",
	".x..............0oo0........x.",
	".x..............0ooo0.......x.",
	".x..............0oooo0......x.",
	".x.00000000000000ooooo0.....x.",
	".x.0ooooooooooooooooooo0....x.",
	".x.0oooooooooooooooooooo0...x.",
	".x.0ooooooooooooooooooooo0..x.",
	".x.0oooooooooooooooooooo0...x.",
	".x.0ooooooooooooooooooo0....x.",
	".x.00000000000000ooooo0.....x.",
	".x..............0oooo0......x.",
	".x..............0ooo0.......x.",
	".x..............0oo0........x.",
	".x..............0o0.........x.",
	".x..............00..........x.",
	".x..........................x.",
	".x..........................x.",
	".x..........................x.",
	".xxxxxxxxxxxxxxxxxxxxxxxxxxxx.",
	"..............................",
};


class CRLinksDialog : public CRGUIWindowBase
{
	enum {
		BACK = 0,
		FORWARD = 1,
		LINK = 2
	};
    protected:
        int _cursorPos;
		V3DocViewWin * _docwin;
		LVDocView * _docview;
		lvRect _invalidateRect;
		int _additionalButtons[2];
		int _addButtonCount;
		int _linkCount;
		int _backSize;
		int _fwdSize;
		int _currentButton;
		int _nextButton[3];
		int _prevButton[3];
		LVImageSourceRef _activeIcons[2];
		LVImageSourceRef _normalIcons[2];
		lvRect _iconRects[2];
	protected:
        virtual void Update()
		{
			setDirty();
			_wm->update( false );
		}
        virtual void draw()
		{
			if ( _invalidateRect.isEmpty() ) {
				CRLog::debug("crLinksDialog::Draw() - invalidated rect is empty!");
				return;
			}
			CRLog::debug("crLinksDialog::Draw()");
			LVDocImageRef page = _docview->getPageImage(0);
			LVDrawBuf * buf = page->getDrawBuf();
			_wm->getScreen()->draw( buf );
            CRLog::trace("draw buttons, current=%d", _currentButton);
			for ( int i=0; i<_addButtonCount; i++ ) {
                int btn = _additionalButtons[i];
				_wm->getScreen()->getCanvas()->Draw( btn==_currentButton?_activeIcons[btn]:_normalIcons[btn], _iconRects[i].left, _iconRects[i].top, _iconRects[i].width(), _iconRects[i].height() );
			}
		}
    public:
		static CRLinksDialog * create( CRGUIWindowManager * wm, V3DocViewWin * docwin )
		{
			ldomXRangeList list;
			docwin->getDocView()->getCurrentPageLinks( list );
			int backSize = docwin->getDocView()->getNavigationHistory().backCount();
			int fwdSize = docwin->getDocView()->getNavigationHistory().forwardCount();
			if ( list.length()==0 && backSize==0 && fwdSize==0)
				return NULL;
			docwin->getDocView()->clearImageCache();
			docwin->getDocView()->selectFirstPageLink();
			return new CRLinksDialog( wm, docwin );
		}
        CRLinksDialog( CRGUIWindowManager * wm, V3DocViewWin * docwin )
		: CRGUIWindowBase( wm ), _docwin(docwin), _docview(docwin->getDocView())
        {
			_invalidateRect.left = 0;
			_invalidateRect.top = 0;
			_invalidateRect.right = 600;
			_invalidateRect.bottom = 800;
			ldomXRangeList list;
			_docview->getCurrentPageLinks( list );
			_linkCount = list.length();
			_backSize = _docview->getNavigationHistory().backCount();
			_fwdSize = _docview->getNavigationHistory().forwardCount();
			CRLog::debug("LinksDialog: links=%d, back=%d, fwd=%d", _linkCount, _backSize, _fwdSize);
			_addButtonCount = 0;
			if ( _backSize )
				_additionalButtons[_addButtonCount++] = BACK;
			if ( _fwdSize )
				_additionalButtons[_addButtonCount++] = FORWARD;
			
			CRLog::debug("LinksDialog: creating icons");
			_activeIcons[FORWARD] = LVCreateXPMImageSource( link_forward_active );
			_activeIcons[BACK] = LVCreateXPMImageSource( link_back_active );
			_normalIcons[FORWARD] = LVCreateXPMImageSource( link_forward_normal );
			_normalIcons[BACK] = LVCreateXPMImageSource( link_back_normal );
			CRLog::debug("LinksDialog: icons created");
			int dx = _activeIcons[0]->GetWidth();
			int dy = _activeIcons[0]->GetHeight();
			int w = 3;
			lvRect rc1(w,w,w+dx,w+dy);
			lvRect rc2(w+dx+w,w,w+dx+dx+w,w+dy);
			_iconRects[0] = rc1;
			_iconRects[1] = rc2;
            // FORWARD->BACK->LINK->FORWARD->BACK
			_currentButton =    (_linkCount>0) ? LINK :  ( (_backSize>0) ? BACK :  FORWARD);
			_nextButton[BACK] = (_linkCount>0) ? LINK :  ( (_fwdSize>0)  ? FORWARD : BACK );
			_nextButton[FORWARD] = (_backSize>0) ? BACK :( (_linkCount>0)? LINK :  FORWARD);
			_nextButton[LINK] = (_fwdSize>0) ? FORWARD : ( (_backSize>0) ? BACK :  LINK);
			_prevButton[FORWARD] =(_linkCount>0) ? LINK :( (_backSize>0) ? BACK :  FORWARD );
			_prevButton[BACK] = (_fwdSize>0) ? FORWARD : ( (_linkCount>0)? LINK :  BACK);
			_prevButton[LINK] = (_backSize>0) ? BACK :   ( (_fwdSize>0)  ? FORWARD : LINK);
		    CRLog::debug("dialog is created");
            _fullscreen = true;
        }
        virtual ~CRLinksDialog() { }
        /// returns true if command is processed
        virtual bool onCommand( int command, int params )
        {
            switch ( command ) {
            case MCMD_CANCEL:
				_docview->clearSelection();
				_wm->closeWindow( this );
				return true;
            case MCMD_OK:
                if ( _currentButton==LINK )
    				_docview->goSelectedLink();
                else if ( _currentButton==BACK )
                    _docview->goBack();
                else
                    _docview->goForward();
				_docview->clearSelection();
				_wm->closeWindow( this );
				return true;
			case MCMD_LONG_BACK:
				_docview->clearSelection();
                _docview->goBack();
				_wm->closeWindow( this );
				return true;
			case MCMD_LONG_FORWARD:
				_docview->clearSelection();
                _docview->goForward();
				_wm->closeWindow( this );
				return true;
            case MCMD_SCROLL_FORWARD:
            case MCMD_SELECT_0:
				invalidateCurrentSelection();
				if ( _currentButton==LINK ) {
					if ( !_docview->selectNextPageLink( _addButtonCount==0 )) {
						_currentButton = _nextButton[_currentButton];
					}
				} else {
					_currentButton = _nextButton[_currentButton];
                    if ( _currentButton==LINK )
                        _docview->selectNextPageLink( false );
				}
				Update();
				invalidateCurrentSelection();
				return true;
            case MCMD_SCROLL_BACK:
            case MCMD_SELECT_9:
				invalidateCurrentSelection();
				if ( _currentButton==LINK ) {
					if ( !_docview->selectPrevPageLink( _addButtonCount==0 )) {
						_currentButton = _prevButton[_currentButton];
					}
				} else {
					_currentButton = _prevButton[_currentButton];
                    if ( _currentButton==LINK )
                        _docview->selectPrevPageLink( false );
				}
				Update();
				invalidateCurrentSelection();
				return true;
            case MCMD_SELECT_1:
            case MCMD_SELECT_2:
            case MCMD_SELECT_3:
            case MCMD_SELECT_4:
            case MCMD_SELECT_5:
            case MCMD_SELECT_6:
            case MCMD_SELECT_7:
            case MCMD_SELECT_8:
                {
                    int index = command - MCMD_SELECT_1;
                    if ( index < _linkCount ) {
			            ldomXRangeList list;
			            _docview->getCurrentPageLinks( list );
                        ldomXRange * link = list[index];
        				invalidateCurrentSelection();
                        _docview->selectRange( *link );
				        draw();
				        invalidateCurrentSelection();
				        draw();
        				_docview->goSelectedLink();
   				        _docview->clearSelection();
						_wm->closeWindow( this );
   				        return true;
                    }
                }
                break;
            default:
                return true;
            }
            return true;
        }
		void invalidateCurrentSelection()
		{
            if ( _currentButton != LINK ) {
			    _invalidateRect.left = _iconRects[0].left;
			    _invalidateRect.top = _iconRects[0].top;
			    _invalidateRect.right = _iconRects[1].right;
			    _invalidateRect.bottom = _iconRects[1].bottom;
				CRLog::debug("invalidateCurrentSelection() : invalidating buttons rect");
				return;
            }
			ldomXRange * link = _docview->getCurrentPageSelectedLink();
			_invalidateRect.clear();
			if ( !link ) {
				CRLog::debug("invalidateCurrentSelection() : no current page selection found!");
				return;
			}
			lvRect rc;
			if ( link->getRect( rc ) ) {
				CRLog::debug("link docRect { %d, %d, %d, %d }", rc.left, rc.top, rc.right, rc.bottom);
#if 1
				_invalidateRect.left = 0;
				_invalidateRect.top = 0;
				_invalidateRect.right = 600;
				_invalidateRect.bottom = 800;
#else
				lvPoint topLeft = rc.topLeft();
				lvPoint bottomRight = rc.bottomRight();
				if ( _docview->docToWindowPoint(topLeft) && _docview->docToWindowPoint(bottomRight) ) {
					rc.left = topLeft.x;
					rc.top = topLeft.y;
					rc.right = bottomRight.x;
					rc.bottom = bottomRight.y;
					CRLog::debug("invalidating link screenRect { %d, %d, %d, %d }", rc.left, rc.top, rc.right, rc.bottom);
					_invalidateRect = rc;
				} else {
					CRLog::debug("link rect conversion error: invalidating full screen");
					_invalidateRect.left = 0;
					_invalidateRect.top = 0;
					_invalidateRect.right = 600;
					_invalidateRect.bottom = 800;
				}
#endif
			} else {
				CRLog::debug("invalidateCurrentSelection() : getRect failed for link!");
			}
		}
};


DECL_DEF_CR_FONT_SIZES;

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


const lChar16 * defT9encoding[] = {
    L"",     // 0 STUB
    L"abc",  // 1
    L"def",  // 2
    L"ghi",  // 3
    L"jkl",  // 4
    L"mno",  // 5
    L"pqrs", // 6
    L"tuv",  // 7
    L"wxyz", // 8
    L"",      // 9 STUB
    NULL
};

bool V3DocViewWin::loadSkin( lString16 pathname )
{
    CRSkinRef skin;
    if ( !pathname.empty() )
        skin = LVOpenSkin( pathname );
    if ( skin.isNull() ) {
        skin = LVOpenSimpleSkin( lString8( cr_default_skin ) );
        _wm->setSkin( skin );
        return false;
    }
    _wm->setSkin( skin );
    return true;
}

V3DocViewWin::V3DocViewWin( CRGUIWindowManager * wm, lString16 dataDir )
: CRDocViewWindow ( wm ), _dataDir(dataDir), _t9encoding(defT9encoding)
{
    CRLog::trace("V3DocViewWin()");
    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
    _docview->setFontSizes( sizes, true );
    _props = LVCreatePropsContainer();
    _newProps = _props;
    // TODO: move skin outside
    lString16 skinfile = _dataDir;
    LVAppendPathDelimiter( skinfile );
    skinfile << L"skin";
    lString8 s8 = UnicodeToLocal( skinfile );
    CRLog::debug("Skin file is %s", s8.c_str() );
    loadSkin( skinfile );
    // TODO: move accelerator table outside
    static const int menu_acc_table[] = {
        XK_Escape, 0, MCMD_CANCEL, 0,
        XK_Return, 0, MCMD_OK, 0, 
        XK_Return, 1, MCMD_OK, 0, 
        '0', 0, MCMD_SCROLL_FORWARD, 0,
        XK_Down, 0, MCMD_SCROLL_FORWARD, 0,
        '9', 0, MCMD_SCROLL_BACK, 0,
        XK_Up, 0, MCMD_SCROLL_BACK, 0,
        '0', 1, MCMD_LONG_FORWARD, 0,
        XK_Down, 1, MCMD_LONG_FORWARD, 0,
        '9', 1, MCMD_LONG_BACK, 0,
        XK_Up, 1, MCMD_LONG_BACK, 0,
        '1', 0, MCMD_SELECT_1, 0,
        '2', 0, MCMD_SELECT_2, 0,
        '3', 0, MCMD_SELECT_3, 0,
        '4', 0, MCMD_SELECT_4, 0,
        '5', 0, MCMD_SELECT_5, 0,
        '6', 0, MCMD_SELECT_6, 0,
        '7', 0, MCMD_SELECT_7, 0,
        '8', 0, MCMD_SELECT_8, 0,
        0
    };
    static const int acc_table_dialog[] = {
        XK_Escape, 0, MCMD_CANCEL, 0,
        XK_Return, 1, MCMD_OK, 0, 
        XK_Return, 0, MCMD_OK, 0, 
        XK_Down, 0, MCMD_SCROLL_FORWARD, 0,
        XK_Up, 0, MCMD_SCROLL_BACK, 0,
        '0', 0, MCMD_SELECT_0, 0,
        '1', 0, MCMD_SELECT_1, 0,
        '2', 0, MCMD_SELECT_2, 0,
        '3', 0, MCMD_SELECT_3, 0,
        '4', 0, MCMD_SELECT_4, 0,
        '5', 0, MCMD_SELECT_5, 0,
        '6', 0, MCMD_SELECT_6, 0,
        '7', 0, MCMD_SELECT_7, 0,
        '8', 0, MCMD_SELECT_8, 0,
        '9', 0, MCMD_SELECT_9, 0,
        0
    };
    _menuAccelerators = CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable( menu_acc_table ) );
    _dialogAccelerators = CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable( acc_table_dialog ) );

    LVRefVec<LVImageSource> icons;
    static const char * battery4[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0..0.000.000.000.000.0.",
        ".0000.000.000.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery3[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0..0.ooo.000.000.000.0.",
        ".0000.ooo.000.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery2[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0..0.ooo.ooo.000.000.0.",
        ".0000.ooo.ooo.000.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery1[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0..0.ooo.ooo.ooo.000.0.",
        ".0000.ooo.ooo.ooo.000.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    static const char * battery0[] = {
        "24 13 4 1",
        "0 c #000000",
        "o c #A1A1A1",
        ". c #FFFFFF",
        "  c None",
        "   .....................",
        "   .0000000000000000000.",
        "....0.................0.",
        ".0000.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0..0.ooo.ooo.ooo.ooo.0.",
        ".0000.ooo.ooo.ooo.ooo.0.",
        "....0.................0.",
        "   .0000000000000000000.",
        "   .....................",
    };
    icons.add( LVCreateXPMImageSource( battery0 ) );
    icons.add( LVCreateXPMImageSource( battery1 ) );
    icons.add( LVCreateXPMImageSource( battery2 ) );
    icons.add( LVCreateXPMImageSource( battery3 ) );
    icons.add( LVCreateXPMImageSource( battery4 ) );
    _docview->setBatteryIcons( icons );

    static const int default_acc_table[] = {
        '6', 0, MCMD_GO_LINK, 0,
        '8', 0, MCMD_SETTINGS_FONTSIZE, 0,
        '8', 1, MCMD_SETTINGS_ORIENTATION, 0,
        XK_Escape, 0, MCMD_QUIT, 0,
        XK_Return, 0, MCMD_MAIN_MENU, 0,
        XK_Return, 1, MCMD_SETTINGS, 0,
        '0', 0, DCMD_PAGEDOWN, 0,
        XK_Up, 0, DCMD_PAGEDOWN, 0,
        '0', KEY_FLAG_LONG_PRESS, DCMD_PAGEDOWN, 10,
        XK_Up, KEY_FLAG_LONG_PRESS, DCMD_PAGEDOWN, 10,
        XK_Down, 0, DCMD_PAGEUP, 0,
        XK_Down, KEY_FLAG_LONG_PRESS, DCMD_PAGEUP, 10,
        '9', 0, DCMD_PAGEUP, 0,
        '9', KEY_FLAG_LONG_PRESS, DCMD_PAGEUP, 10,
#ifdef WITH_DICT
        '2', 0, MCMD_DICT, 0,
#endif
        '+', 0, DCMD_ZOOM_IN, 0,
        '=', 0, DCMD_ZOOM_IN, 0,
        '-', 0, DCMD_ZOOM_OUT, 0,
        '_', 0, DCMD_ZOOM_OUT, 0,
        0
    };
    setAccelerators( CRGUIAcceleratorTableRef( new CRGUIAcceleratorTable( default_acc_table ) ) );
}

bool V3DocViewWin::loadDefaultCover( lString16 filename )
{
    LVImageSourceRef cover = LVCreateFileCopyImageSource( filename.c_str() );
    if ( !cover.isNull() ) {
        _docview->setDefaultCover( cover );
        return true;
    } else {
        IMAGE_SOURCE_FROM_BYTES(defCover, cr3_def_cover_gif);
        _docview->setDefaultCover( defCover );
        return false;
    }
}

bool V3DocViewWin::loadCSS( lString16 filename )
{
    lString8 css;
    if ( LVLoadStylesheetFile( filename, css ) ) {
        if ( !css.empty() ) {
            _docview->setStyleSheet( css );
            _css = css;
            return true;
        }
    }
    return false;
}

bool V3DocViewWin::loadDictConfig( lString16 filename )
{
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( !stream.isNull() ) {
        _dictConfig = filename;
        return true;
    }
    return false;
}

bool V3DocViewWin::loadHistory( lString16 filename )
{
    _historyFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
        return false;
    }
    if ( !_docview->getHistory()->loadFromStream( stream ) )
        return false;
    return true;
}

void V3DocViewWin::closing()
{
    _docview->savePosition();
    saveHistory( lString16() );
}

bool V3DocViewWin::loadDocument( lString16 filename )
{
    if ( !_docview->LoadDocument( filename.c_str() ) )
        return false;
    _docview->restorePosition();
    return true;
}

bool V3DocViewWin::saveHistory( lString16 filename )
{
    crtrace log;
    if ( filename.empty() )
        filename = _historyFileName;
    if ( filename.empty() )
        return false;
    _historyFileName = filename;
    log << "V3DocViewWin::saveHistory(" << filename << ")";
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
    if ( !stream ) {
        lString16 path16 = LVExtractPath( filename );
        lString8 path = UnicodeToLocal( path16 );
#ifdef _WIN32
        if ( !CreateDirectoryW( path16.c_str(), NULL ) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#else
        path.erase( path.length()-1, 1 );
        CRLog::warn("Cannot create settings file, trying to create directory %s", path.c_str());
        if ( mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#endif
    }
    if ( stream.isNull() ) {
        lString8 fn = UnicodeToUtf8( filename );
        CRLog::error("Cannot open history file %s for write", fn.c_str() );
        return false;
    }
    return _docview->getHistory()->saveToStream( stream.get() );
}

void V3DocViewWin::flush()
{
    // override to update battery
    int charge = 0;
    bool charging = false;
    if ( _wm->getBatteryStatus( charge, charging ) ) {
        if ( _docview->getBatteryState() != charge )
            _docview->setBatteryState( charge );
    }
    CRDocViewWindow::flush();
}

bool V3DocViewWin::loadSettings( lString16 filename )
{
    _settingsFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
        _docview->propsUpdateDefaults( _props );
        _docview->propsApply( _props );
        return false;
    }
    if ( _props->loadFromStream( stream.get() ) ) {
        _docview->propsUpdateDefaults( _props );
        _docview->propsApply( _props );
        return true;
    }
    _docview->propsUpdateDefaults( _props );
    _docview->propsApply( _props );
    return false;
}

bool V3DocViewWin::saveSettings( lString16 filename )
{
    crtrace log;
    if ( filename.empty() )
        filename = _settingsFileName;
    if ( filename.empty() )
        return false;
    _settingsFileName = filename;
    log << "V3DocViewWin::saveSettings(" << filename << ")";
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
    if ( !stream ) {
		lString16 path16 = LVExtractPath( filename );
        lString8 path = UnicodeToLocal( path16 );
#ifdef _WIN32
		if ( !CreateDirectoryW( path16.c_str(), NULL ) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
		}
#else
        path.erase( path.length()-1, 1 );
        CRLog::warn("Cannot create settings file, trying to create directory %s", path.c_str());
        if ( mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
#endif
    }
    if ( stream.isNull() ) {
        lString8 fn = UnicodeToUtf8( filename );
        CRLog::error("Cannot open settings file %s for write", fn.c_str() );
        return false;
    }
    return _props->saveToStream( stream.get() );
}

void V3DocViewWin::applySettings()
{
    CRPropRef delta = _props ^ _newProps;
    CRLog::trace( "applySettings() - %d options changed", delta->getCount() );
    _docview->propsApply( delta );
    _props = _newProps; // | _props;
}

void V3DocViewWin::showSettingsMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    CRMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, _menuAccelerators );
    _wm->activateWindow( mainMenu );
}

void V3DocViewWin::showFontSizeMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    CRSettingsMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, _menuAccelerators );
    CRMenu * menu = mainMenu->createFontSizeMenu( NULL, _newProps );
    _wm->activateWindow( menu );
}

void V3DocViewWin::showOrientationMenu()
{
    LVFontRef menuFont( fontMan->GetFont( MENU_FONT_SIZE, 600, true, css_ff_sans_serif, lString8("Arial")) );
    //_props->set( _docview->propsGetCurrent() );
    _props = _docview->propsGetCurrent() | _props;
    _newProps = LVClonePropsContainer( _props );
    CRSettingsMenu * mainMenu = new CRSettingsMenu( _wm, _newProps, MCMD_SETTINGS_APPLY, menuFont, _menuAccelerators );
    CRMenu * menu = mainMenu->createOrientationMenu( NULL, _newProps );
    _wm->activateWindow( menu );
}

void V3DocViewWin::showMainMenu()
{
    CRMenu * menu_win = new CRMenu( _wm,
        NULL, //CRMenu * parentMenu,
        1,
        lString16(L"Main Menu"),
        LVImageSourceRef(),
        LVFontRef(),
        LVFontRef() );
/*
VIEWER_MENU_GOTOFIRSTPAGE=Go to first page
VIEWER_MENU_GOTOENDPAGE=Go to last page
VIEWER_MENU_GOTOPAGE=Go to page...
VIEWER_MENU_GOTOINDEX=Go to index
VIEWER_MENU_5ABOUT=About...
VIEWER_MENU_4ABOUT=About...
*/
    menu_win->setSkinName(lString16(L"#main"));
    menu_win->addItem( new CRMenuItem( menu_win, DCMD_BEGIN,
                _wm->translateString("VIEWER_MENU_GOTOFIRSTPAGE", "Go to first page"),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_GO_PAGE,
                _wm->translateString("VIEWER_MENU_GOTOPAGE", "Go to page ..."),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, DCMD_END,
                _wm->translateString("VIEWER_MENU_GOTOENDPAGE", "Go to last page"),
                LVImageSourceRef(),
                LVFontRef() ) );
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_SETTINGS,
                _wm->translateString("VIEWER_MENU_SETTINGS", "Settings..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#ifdef WITH_DICT
    menu_win->addItem( new CRMenuItem( menu_win, MCMD_DICT,
                _wm->translateString("VIEWER_MENU_DICTIONARY", "Dictionary..."),
                LVImageSourceRef(),
                LVFontRef() ) );
#endif
    menu_win->setAccelerators( _menuAccelerators );
    _wm->activateWindow( menu_win );
}

void V3DocViewWin::showGoToPageDialog()
{
    CRNumberEditDialog * dlg = new CRNumberEditDialog( _wm, 
        _wm->translateString("VIEWER_HINT_INPUTSKIPPAGENUM", "Enter page number"),
        lString16(), 
        MCMD_GO_PAGE_APPLY, 1, _docview->getPageCount() );
    dlg->setAccelerators( _dialogAccelerators );
    _wm->activateWindow( dlg );
}

bool V3DocViewWin::showLinksDialog()
{
	CRLinksDialog * dlg = CRLinksDialog::create( _wm, this );
	if ( !dlg )
		return false;
    dlg->setAccelerators( _menuAccelerators );
    _wm->activateWindow( dlg );
	return true;
}

/// returns true if command is processed
bool V3DocViewWin::onCommand( int command, int params )
{
    switch ( command ) {
    case MCMD_QUIT:
        getWindowManager()->closeAllWindows();
        return true;
    case MCMD_MAIN_MENU:
        showMainMenu();
        return true;
    case MCMD_SETTINGS_FONTSIZE:
        showFontSizeMenu();
        return true;
    case MCMD_SETTINGS_ORIENTATION:
        showOrientationMenu();
        return true;
    case MCMD_GO_PAGE:
        showGoToPageDialog();
        return true;
	case MCMD_GO_LINK:
		showLinksDialog();
		return true;
    case MCMD_SETTINGS:
        showSettingsMenu();
        return true;
#ifdef WITH_DICT
    case MCMD_DICT:
        CRLog::info("MCMD_DICT activated\n");
        activate_dict( _wm, this, _t9encoding );
        return true;
#endif
    case MCMD_GO_PAGE_APPLY:
        _docview->doCommand( DCMD_GO_PAGE, params-1 );
        return true;
    case MCMD_SETTINGS_APPLY:
    case mm_Orientation:
    case mm_FontSize:
        applySettings();
        saveSettings( lString16() );
        return true;
    default:
        // do nothing
        ;
    }
    return CRDocViewWindow::onCommand( command, params );
}
