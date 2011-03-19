/*
 *  CR3 for PocketBook, port by pkb
 */

#include <crengine.h>
#include <crgui.h>
#include "cr3main.h"
#include "numedit.h"
#include "tocdlg.h"
#include "mainwnd.h"
#include <cr3version.h>
#include "cr3pocketbook.h"
#include "inkview.h"

#ifdef CRMenu
#undef CRMenu
#endif

static const char *def_menutext[9] = {
	"@Goto_page", "@Exit", "@Search",
	"@Bookmarks", "@Menu", "@Rotate",
	"@Dictionary", "@Contents", "@Settings"
};

static const char *def_menuaction[9] = {
	"@KA_goto", "@KA_exit", "@KA_srch",
	"@KA_obmk", "@KA_none", "@KA_rtte",
	"@KA_dict", "@KA_cnts", "@KA_conf"
};

static cr_rotate_angle_t pocketbook_orientations[4] = {
	CR_ROTATE_ANGLE_0, CR_ROTATE_ANGLE_90, CR_ROTATE_ANGLE_270, CR_ROTATE_ANGLE_180
};

char key_buffer[KEY_BUFFER_LEN];

#include <cri18n.h>

class CRPocketBookScreen : public CRGUIScreenBase {
    public:
        static CRPocketBookScreen * instance;
    protected:
        virtual void update( const lvRect & rc2, bool full )
		{
    		CRLog::debug("CRPbScreen::update(%d, %d, %d, %d, %s)", rc2.left, rc2.top, rc2.right, rc2.bottom, full ? "full" : "partial");
        	if ( rc2.isEmpty() && !full )
        		return;
        	lvRect rc = rc2;
        	rc.left &= ~3;
        	rc.right = (rc.right + 3) & ~3;

    		if (rc.height() > 400
#if ENABLE_UPDATE_MODE_SETTING==1
          		&& checkFullUpdateCounter()
#endif
         	)
        		full = true;
    		else
        		full = false;

			lUInt8 *screenbuf =  _front->GetScanLine(0);
			int w = _front->GetWidth(); int h = _front->GetHeight();
    		Stretch(screenbuf, IMAGE_GRAY2, w, h, _front->GetRowSize(), 0, 0, w, h, 0);
        	//TODO: rework this to use FullUpdate()/SoftUpdate()
    		if ( full )
        		FullUpdate();
			else
				PartialUpdate(rc.left, rc.top, rc.width(), rc.height());
		}
    public:
        virtual ~CRPocketBookScreen()
        {
            instance = NULL;
        }
        CRPocketBookScreen( int width, int height )
        :  CRGUIScreenBase( width, height, true )
        {
            instance = this;
        }

        virtual void invalidateRect( const lvRect & rc )
        {
			CRLog::trace("CRPbScreen::invalidateRect(%d,%d,%d,%d)", rc.left, rc.top, rc.width(), rc.height());
            _updateRect.extend( rc );
        }

        virtual void draw( LVDrawBuf * img, int x = 0, int y = 0)
        {
			lUInt8 *imgbuf =  img->GetScanLine(0);
		    CRLog::trace("CRPbScreen::draw(x=%d,y=%d,w=%d,h=%d)", x,y, img->GetWidth(), img->GetHeight());
            img->DrawTo( _canvas.get(), x, y, 0, NULL );
        }
};

CRPocketBookScreen * CRPocketBookScreen::instance = NULL;

static const struct {
	char const *pbAction;
	int commandId;
	int commandParam;
} pbActions[] = {
	{ "@KA_none", -1, 0},
	{ "@KA_menu", PB_QUICK_MENU, 0},
	{ "@KA_prev", DCMD_PAGEUP, 0},
	{ "@KA_next", DCMD_PAGEDOWN, 0},
	{ "@KA_pr10", DCMD_PAGEUP, 10},
	{ "@KA_nx10", DCMD_PAGEDOWN, 10},
	{ "@KA_goto", MCMD_GO_PAGE, 0},
	{ "@KA_frst", DCMD_BEGIN, 0},
	{ "@KA_last", DCMD_END, 0},
	{ "@KA_prse", -1, 0},
	{ "@KA_nxse", -1, 0},
	{ "@KA_obmk", MCMD_BOOKMARK_LIST, 0},
	{ "@KA_nbmk", -1, 0},
	{ "@KA_nnot", -1, 0},
	{ "@KA_savp", -1, 0},
	{ "@KA_onot", -1, 0},
	{ "@KA_olnk", MCMD_GO_LINK, 0},
	{ "@KA_blnk", -1, 0},
	{ "@KA_cnts", PB_CMD_CONTENTS, 0},
	{ "@KA_srch", MCMD_SEARCH, 0},
	{ "@KA_dict", MCMD_DICT, 0},
	{ "@KA_zmin", DCMD_ZOOM_IN, 0},
	{ "@KA_zout", DCMD_ZOOM_OUT, 0},
	{ "@KA_hidp", -1, 0},
	{ "@KA_rtte", PB_CMD_ROTATE, 0},
	{ "@KA_mmnu", MCMD_MAIN_MENU, 0},
	{ "@KA_exit", MCMD_QUIT, 0},
	{ "@KA_mp3o", -1, 0},
	{ "@KA_mp3p", -1, 0},
	{ "@KA_volp", -1, 0},
	{ "@KA_volm", -1, 0},
	{ "@KA_conf", MCMD_SETTINGS, 0},
	{ "@KA_abou", MCMD_ABOUT, 0}
};

class CRPocketBookWindowManager : public CRGUIWindowManager
{
protected:
	LVHashTable<lString8, int> _pbTable;

	void initPocketBookActionsTable() {
		for (int i = 0; i < sizeof(pbActions)/sizeof(pbActions[0]); i++) {
			_pbTable.set(lString8(pbActions[i].pbAction), i);
		}
	}
public:
    /// translate string by key, return default value if not found
    virtual lString16 translateString( const char * key, const char * defValue )
    {
        CRLog::trace("Translate(%s)", key);
        lString16 res;

        const char * res8 = GetLangText((char *)key);
        if ( res8 && res8[0] ) {
            CRLog::trace("   found(%s)", res8);
            res = Utf8ToUnicode( lString8(res8) );
        } else {
            CRLog::trace("   not found");
            res = Utf8ToUnicode( lString8(defValue) );
        }
        return res;
    }

    static CRPocketBookWindowManager * instance;

    virtual ~CRPocketBookWindowManager()
    {
        instance = NULL;
    }

	void getPocketBookCommand(char *name, int &commandId, int &commandParam) {
		int index = _pbTable.get(lString8(name));
		commandId = pbActions[index].commandId;
		commandParam = pbActions[index].commandParam;
		CRLog::trace("getPocketBookCommand(%s), index = %d, commandId = %d, commandParam=%d",
			name, index, commandId, commandParam);
	}

	int getPocketBookCommandIndex(char *name) {
		return _pbTable.get(lString8(name));
	}
	
    CRPocketBookWindowManager(int dx, int dy)
    : CRGUIWindowManager(NULL), _pbTable(32)
    {
        CRPocketBookScreen * s = new CRPocketBookScreen(dx, dy);
        //FIXME: shouldn't we save and restore last screen orientation used?
        _orientation = pocketbook_orientations[GetOrientation()];
        _screen = s;
        _ownScreen = true;
		instance = this;
		initPocketBookActionsTable();
    }

    // runs event loop
    virtual int runEventLoop()
    {
        return 0; // NO EVENT LOOP AVAILABLE
    }

    bool doCommand( int cmd, int params )
    {
		CRLog::debug("doCommand(%d, %d)", cmd, params);
        if ( !onCommand( cmd, params ) )
            return false;
        update( false );
        return true;
    }

	bool getBatteryStatus(int & percent, bool & charging) {
		charging = IsCharging() > 0; // TODO: find out values returned by the IsCharging() function.
		percent = GetBatteryPower(); // It seems that the GetBatteryPower() returns what needed here
	}

};

CRPocketBookWindowManager * CRPocketBookWindowManager::instance = NULL;
V3DocViewWin * main_win = NULL;

void processPostedEvents()
{
	bool needUpdate = CRPocketBookWindowManager::instance->processPostedEvents();
	if (needUpdate)
		CRPocketBookWindowManager::instance->update(false);	
}

void quickMenuHandler(int choice) 
{
	CRPocketBookWindowManager::instance->onCommand(PB_QUICK_MENU_SELECT, choice);
	processPostedEvents();
}

void rotateHandler(int angle) {
	CRPocketBookWindowManager::instance->onCommand(PB_CMD_ROTATE_ANGLE_SET, angle);
	processPostedEvents();
}

void pageSelector(int page) {
	CRPocketBookWindowManager::instance->onCommand(MCMD_GO_PAGE_APPLY, page);
}

void searchHandler(char *s)
{
    if (s  && *s)
		CRPocketBookWindowManager::instance->onCommand(MCMD_SEARCH_FINDFIRST, 1);
	processPostedEvents();
} 

void CRPbMenu::doCloseMenu(int command, int param)
{
	if ( _menu != NULL )
		closeMenu(0);
    else
		closeMenu(command, param); // close, for root menu	
}

void CRPbMenu::activated()
{
    _selectedIndex = setInitialSelection();
    if (_selectedIndex >= 0) {
        _topItem = _selectedIndex / _pageItems * _pageItems;
    }
    setDirty();
}

int CRPbMenu::setInitialSelection() 
{
	int selected = CRMenu::getSelectedItemIndex();
	return (selected < 0 ? 0 : selected);
}

int CRPbMenu::getSelectedItemIndex() 
{ 
	return _selectedIndex; 
}

void CRPbMenu::nextPage()
{
	int nPage = getCurPage() + 1;
	
    int oldTop = _topItem;
    _topItem = _pageItems * nPage;

	if (_topItem >= (int)_items.length())
		_topItem = 0;
    if (_topItem != oldTop || oldTop == 0) {
		_selectedIndex = _topItem;
        setDirty();
	}
}

void CRPbMenu::prevPage()
{
	int nPage = getCurPage() - 1;
    int oldTop = _topItem;
    _topItem = _pageItems * nPage;

    if (_topItem < 0) {
		doCloseMenu(getId());
        return;
	}
    if ( _topItem != oldTop ) {
		_selectedIndex = _topItem + _pageItems -1;
        setDirty();
	}
}

int CRPbMenu::getLastOnPage()
{
	int lastOnPage = _topItem + _pageItems;
	if (lastOnPage >= (int)_items.length())
		lastOnPage = (int)_items.length();
	return lastOnPage;
}

void CRPbMenu::nextItem()
{
	int lastOnPage = getLastOnPage();

	if (_selectedIndex < lastOnPage -1) 
		_selectedIndex += 1;
	else
		_selectedIndex = _topItem;
	setDirty();
	_wm->updateWindow(this);		
}

void CRPbMenu::prevItem()
{
	if (_selectedIndex != _topItem)
		_selectedIndex -= 1;
	else
		_selectedIndex = getLastOnPage() -1;
	setDirty();
	_wm->updateWindow(this);
}

bool CRPbMenu::onItemSelect(int itemId, int param)
{
	if (itemId < 0 || itemId >= _items.length()) {
		CRLog::error( "CRPbMenu::onItemSelect() - invalid selection: %d", itemId);
		return true;
	}

    CRMenuItem * item = _items[itemId];

    if (item->onSelect() > 0)
        return true;

    if (item->isSubmenu()) {
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
            doCloseMenu(getId());
            return true;
        }
        doCloseMenu(item->getId(), param);
        return true;
	}
}

bool CRPbMenu::onCommand( int command, int params ) 
{
	bool ret = true;

	switch (command) {
	case MCMD_CANCEL:
		closeMenu( 0 );
		break;
	case MCMD_OK:
		ret = onItemSelect(_selectedIndex, params);
		break;
	case MCMD_SCROLL_FORWARD:
		nextItem();
		break;
	case MCMD_SCROLL_BACK:
		prevItem();
		break;
	case MCMD_SCROLL_FORWARD_LONG:
		nextPage();
		break;
	case MCMD_SCROLL_BACK_LONG:
		prevPage();
		break;
	default:
		ret = false;
		break;
	}
    return ret;
}
	

void tocHandler(long long position) 
{
	CRLog::trace("tocHandler(position=%d)", position);
	CRPocketBookWindowManager::instance->onCommand(MCMD_GO_PAGE_APPLY, position);
}



class CRPocketBookDocView : public V3DocViewWin {
private:
	ibitmap *_bm3x3;
	char *_strings3x3[9];
	int _quick_menuactions[9];
	tocentry *_toc;
	int _tocLength;
	
	void freeContents() 
	{
		for (int i = 0; i < _tocLength; i++) {
			if (_toc[i].text)
				free(_toc[i].text);
		}
		free(_toc);
		_toc = NULL;
	}

protected:
	ibitmap * getQuickMenuBitmap() {
		if (_bm3x3 == NULL) {
			_bm3x3 = GetResource(const_cast<char*>(PB_QUICK_MENU_BMP_ID), NULL);
			if (_bm3x3 == NULL) 
				_bm3x3 = NewBitmap(128, 128);
			lString8 menuTextId(PB_QUICK_MENU_TEXT_ID);
			for (int i = 0; i < 9; i++) {
				menuTextId[PB_QUICK_MENU_TEXT_ID_IDX] = '0' + i;
				_strings3x3[i] = GetThemeString((char *)menuTextId.c_str(), (char *)def_menutext[i]);
			}
			lString8 menuActionId(PB_QUICK_MENU_ACTION_ID);
			for (int i = 0; i < 9; i++) {
				menuActionId[PB_QUICK_MENU_ACTION_ID_IDX] = '0' + i;
				char *action = GetThemeString((char *)menuActionId.c_str(), (char *)def_menuaction[i]);
				_quick_menuactions[i] = CRPocketBookWindowManager::instance->getPocketBookCommandIndex(action);
			}
		}
		return _bm3x3;
	}

	bool rotateApply(int params) 
	{
		if (params >= 0 && params <= 3) {
			int orient = GetOrientation();
			if (orient != params) {
				SetOrientation(params);
				cr_rotate_angle_t oldOrientation = pocketbook_orientations[orient];
				cr_rotate_angle_t newOrientation = pocketbook_orientations[params];
				int dx = _wm->getScreen()->getWidth();
				int dy = _wm->getScreen()->getHeight();
				if ((oldOrientation & 1) == (newOrientation & 1)) {
					_wm->reconfigure(dx, dy, newOrientation);
					_wm->update(true);
				} else
					_wm->reconfigure(dy, dx, newOrientation);
			}
		} else
			// Shouldn't happen
			CRLog::error("Unexpected parameter in CRPocketBookDocView::onCommand(%d, %d)",
				PB_CMD_ROTATE_ANGLE_SET, params);
		return true;
	}

	bool quickMenuApply(int params) 
	{
		if (params >= 0 && params < 9) {
			int index = _quick_menuactions[params];
			CRLog::trace("CRPocketBookDocView::onCommand(params=%d), index=%d", params, index);
			if (pbActions[index].commandId >= 0) {
				_wm->postCommand(pbActions[index].commandId, pbActions[index].commandParam);
			}
		} else
			// Shouldn't happen
			CRLog::error("Unexpected parameter in CRPocketBookDocView::onCommand(%d, %d)",
				PB_QUICK_MENU_SELECT, params);
		return true;
	}

public:
    static CRPocketBookDocView * instance;
    CRPocketBookDocView( CRGUIWindowManager * wm, lString16 dataDir )
    : V3DocViewWin( wm, dataDir ), _tocLength(0), _toc(NULL), _bm3x3(NULL)
    {
        instance = this;
    }

    virtual void closing()
    {
        //strcpy( last_bookmark, GetCurrentPositionBookmark() );
        //last_bookmark_page = CRPocketBookDocView::instance->getDocView()->getCurPage();
        V3DocViewWin::closing();
        CloseApp();
    }

	bool onCommand(int command, int params)
	{
		switch(command) {
		case MCMD_MAIN_MENU:
			OpenMainMenu();
			return true;
		case PB_QUICK_MENU:
			OpenMenu3x3(getQuickMenuBitmap(), (const char**)_strings3x3, quickMenuHandler);
			return true;
		case PB_CMD_ROTATE:
			OpenRotateBox(rotateHandler);
			return true;
		case PB_QUICK_MENU_SELECT:
			return quickMenuApply(params);
		case PB_CMD_ROTATE_ANGLE_SET: 
			return rotateApply(params);
		case MCMD_SEARCH:
			_searchPattern.clear();
			OpenKeyboard(const_cast<char *>("@Search"), key_buffer, KEY_BUFFER_LEN, 0, searchHandler);
			return true;
		case MCMD_SEARCH_FINDFIRST:
			_searchPattern += Utf8ToUnicode(key_buffer);
			V3DocViewWin::onCommand( command, params );
			_wm->update(false);
			return true;
		case MCMD_GO_PAGE:
			OpenPageSelector(pageSelector);
			return true;
		case MCMD_GO_PAGE_APPLY:
			if (params <= 0)
				params = 1;
			if (params > _docview->getPageCount())
				params = _docview->getPageCount();
			if (V3DocViewWin::onCommand( command, params ))
				_wm->update(true);
			if (_toc)
				freeContents() ;
			return true;
		case PB_CMD_CONTENTS: 
			showContents();
			return true;
		default:
			break;
		}
		return V3DocViewWin::onCommand( command, params );
	}


	void showContents() {
		if (_toc == NULL) {
			LVPtrVector<LVTocItem, false> tocItems;
			_docview->getFlatToc(tocItems);
			_tocLength = tocItems.length();

			if (_tocLength) {
				int tocSize = (_tocLength + 1) * sizeof(tocentry);
				_toc = (tocentry *) malloc(tocSize);
				for (int i = 0; i < tocItems.length(); i++) {
					LVTocItem * item = tocItems[i];
					_toc[i].level = item->getLevel();
					_toc[i].position = item->getPage() + 1;
					_toc[i].page = _toc[i].position;
					_toc[i].text = strdup(UnicodeToUtf8(item->getName()).c_str());
					char *p = _toc[i].text;
					while (*p) {
						if (*p == '\r' || *p == '\n') *p = ' ';
							p++;
					}
				}
			} else {
				Message(ICON_INFORMATION, const_cast<char*>("CoolReader"),
						const_cast<char*>("@No_contents"), 2000);
				return;
			}
		}
		OpenContents(_toc, _tocLength, _docview->getCurPage() + 1, tocHandler);			
	}
	
    virtual ~CRPocketBookDocView()
    {
        instance = NULL;
    }
};

CRPocketBookDocView * CRPocketBookDocView::instance = NULL;


static void loadPocketBookKeyMaps(CRGUIWindowManager & winman)
{
	CRGUIAcceleratorTable pbTable;
	int commandId, commandParam;

    char *keypress[32], *keypresslong[32];
	GetKeyMapping(keypress, keypresslong);

	for (int i = 0; i < 32; i++) {
		if (keypress[i]) {
			CRPocketBookWindowManager::instance->getPocketBookCommand(keypress[i], commandId, commandParam);
			CRLog::trace("keypress[%d] = %s, cmd = %d, param=%d", i, keypress[i], commandId, commandParam);
			if (commandId >= 0)
				pbTable.add(i, 0, commandId, commandParam);
		}
		if (keypresslong[i]) {
			CRPocketBookWindowManager::instance->getPocketBookCommand(keypresslong[i], commandId, commandParam);
			CRLog::trace("keypresslong[%d] = %s, cmd = %d, param=%d", i, keypresslong[i], commandId, commandParam);
			if (commandId >= 0)
				pbTable.add(i, 1, commandId, commandParam);
		}
	}
	CRGUIAcceleratorTableRef mainTable = winman.getAccTables().get("main");
	if (!mainTable.isNull()) {
		CRLog::trace("main accelerator table is not null");
		mainTable->addAll(pbTable);
	}
}


static const char * getLang( )
{
	iconfig *gc = GetGlobalConfig();

	char *ret = ReadString(gc, const_cast<char *>("language"), const_cast<char *>("ru"));

	CloseConfig(gc);
	return ret;
}

int InitDoc(char *fileName)
{
    static const lChar16 * css_file_name = L"fb2.css"; // fb2
    static char history_file_name[1024] = USERDATA"/share/cr3/.cr3hist";

    CRLog::trace("InitDoc()");
#ifdef __i386__
    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);
#else
    InitCREngineLog(USERDATA"/share/cr3/crlog.ini");
#endif

    lString16 bookmarkDir(USERDATA"/share/cr3/bookmarks/");
    char manual_file[512] = USERDATA"/share/c3/manual/cr3-manual-en.fb2";
    {
        const char * lang = getLang();
        if ( lang && lang[0] ) {
            // set translator
            CRLog::info("Current language is %s, looking for translation file", lang);
            lString16 mofilename = L""USERDATA"/share/cr3/i18n/" + lString16(lang) + L".mo";
            lString16 mofilename2 = L""USERDATA2"/share/cr3/i18n/" + lString16(lang) + L".mo";
            CRMoFileTranslator * t = new CRMoFileTranslator();
            if ( t->openMoFile( mofilename2 ) || t->openMoFile( mofilename ) ) {
                CRLog::info("translation file %s.mo found", lang);
                CRI18NTranslator::setTranslator( t );
            } else {
                CRLog::info("translation file %s.mo not found", lang);
                delete t;
            }
            sprintf( manual_file, ""USERDATA"/share/cr3/manual/cr3-manual-%s.fb2", lang );
            if ( !LVFileExists( lString16(manual_file).c_str() ) )
                sprintf( manual_file, ""USERDATA2"/share/cr3/manual/cr3-manual-%s.fb2", lang );
        }
    }

    const lChar16 * ini_fname = L"cr3.ini";
#ifdef SEPARATE_INI_FILES
    if ( strstr(fileName, ".txt")!=NULL || strstr(fileName, ".tcr")!=NULL) {
        ini_fname = L"cr3-txt.ini";
        css_file_name = L"txt.css";
    } else if ( strstr(fileName, ".rtf")!=NULL ) {
        ini_fname = L"cr3-rtf.ini";
        css_file_name = L"rtf.css";
    } else if ( strstr(fileName, ".htm")!=NULL ) {
        ini_fname = L"cr3-htm.ini";
        css_file_name = L"htm.css";
    } else if ( strstr(fileName, ".epub")!=NULL ) {
        ini_fname = L"cr3-epub.ini";
        css_file_name = L"epub.css";
    } else {
        ini_fname = L"cr3-fb2.ini";
        css_file_name = L"fb2.css";
    }
#endif

    lString16Collection fontDirs;
	fontDirs.add(lString16(L""USERFONTDIR));
	fontDirs.add(lString16(L""SYSTEMFONTDIR));
    CRLog::info("INIT...");
    if (!InitCREngine(""USERDATA"/share/cr3", fontDirs))
        return 0;

    {
		OpenScreen();
        CRLog::trace("creating window manager...");
        CRPocketBookWindowManager * wm = new CRPocketBookWindowManager(ScreenWidth(), ScreenHeight());

        const char * keymap_locations [] = {
            USERDATA"/share/cr3/keymaps",
            USERDATA2"/share/cr3/keymaps",
            NULL,
        };
        loadKeymaps(*wm, keymap_locations);
        loadPocketBookKeyMaps(*wm);
        HyphMan::initDictionaries(lString16(L""USERDATA"/share/cr3/hyph/"));
        if (!wm->loadSkin(lString16(L""USERDATA2"/share/cr3/skin")))
            wm->loadSkin(lString16(L""USERDATA"/share/cr3/skin"));
        ldomDocCache::init(lString16(L""USERDATA2"/share/cr3/.cache"), 0x100000 * 64); /*96Mb*/
        CRLog::trace("creating main window...");
        main_win = new CRPocketBookDocView(wm, lString16(L""USERDATA"/share/cr3"));
        CRLog::trace("setting colors...");
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if (manual_file[0])
            main_win->setHelpFile( lString16( manual_file ) );
        if (!main_win->loadDefaultCover(lString16(L""USERDATA2"/share/cr3/cr3_def_cover.png")))
            main_win->loadDefaultCover(lString16(L""USERDATA"/share/cr3/cr3_def_cover.png"));
        if ( !main_win->loadCSS(  lString16( L""USERDATA"/share/cr3/" ) + lString16(css_file_name) ) )
                main_win->loadCSS( lString16( L""USERDATA2"/share/cr3/" ) + lString16(css_file_name) );
        main_win->setBookmarkDir(bookmarkDir);
        CRLog::trace("choosing init file...");
        static const lChar16 * dirs[] = {
            L""USERDATA2"/share/cr3/",
            L""USERDATA"/share/cr3/",
            NULL
        };
        int i;
        CRLog::debug("Loading settings...");
        lString16 ini;
        for ( i=0; dirs[i]; i++ ) {
            ini = lString16(dirs[i]) + ini_fname;
            if ( main_win->loadSettings( ini ) ) {
                break;
            }
        }
        CRLog::debug("settings at %s", UnicodeToUtf8(ini).c_str() );
		if ( !main_win->loadHistory( lString16(history_file_name) ) ) {
			CRLog::error("Cannot read history file %s", history_file_name);
		}
        LVDocView * _docview = main_win->getDocView();
        _docview->setBatteryState(GetBatteryPower());
        wm->activateWindow( main_win );
        if ( !main_win->loadDocument( lString16(fileName) ) ) {
            printf("Cannot open book file %s\n", fileName);
            delete wm;
            return 0;
        }
    }
    return 1;
}

int main_handler(int type, int par1, int par2)
{
	bool process_events = false;
	bool needUpdate = false;
	switch (type) {
	case EVT_SHOW:
		CRPocketBookWindowManager::instance->update(true);
		break;
	case EVT_EXIT:
		break;
	case EVT_PREVPAGE:
		CRLog::trace("EVT_PREVPAGE");
		break;
	case EVT_NEXTPAGE:
		CRLog::trace("EVT_NEXTPAGE");
		break;
	case EVT_ORIENTATION:
		CRPocketBookWindowManager::instance->onCommand(PB_CMD_ROTATE_ANGLE_SET, par1);
		break;
	case EVT_KEYREPEAT:
	case EVT_KEYRELEASE:
		if (type == EVT_KEYRELEASE && par2 == 0)
			needUpdate = CRPocketBookWindowManager::instance->onKeyPressed(par1, 0);
		else if (type == EVT_KEYREPEAT && par2 > 0)
			needUpdate = CRPocketBookWindowManager::instance->onKeyPressed(par1, KEY_FLAG_LONG_PRESS);
		process_events = true;
		break;
	default:
		break;
	}
	if (process_events)
		needUpdate = CRPocketBookWindowManager::instance->processPostedEvents() || needUpdate;
	if (needUpdate)
		CRPocketBookWindowManager::instance->update(false);
	return 0;

}

int main(int argc, char **argv)
{
	if (argc < 2) {
		Message(ICON_WARNING,  const_cast<char*>("CoolReader"), const_cast<char*>("@Cant_open_file"), 2000);
		return 1;
	}
    if (!InitDoc(argv[1])) {
		Message(ICON_WARNING,  const_cast<char*>("CoolReader"), const_cast<char*>("@Cant_open_file"), 2000);
        return 2;
    }
    InkViewMain(main_handler);
    return 0;
}
