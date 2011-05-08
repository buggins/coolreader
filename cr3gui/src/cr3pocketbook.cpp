/*
 *  CR3 for PocketBook, port by pkb
 */

#include <crengine.h>
#include <crgui.h>
#include "viewdlg.h"
#include "cr3main.h"
#include "numedit.h"
#include "linksdlg.h"
#include "bmkdlg.h"
#include "mainwnd.h"
#include "selnavig.h"
#include <cr3version.h>
#include "cr3pocketbook.h"
#include <inkview.h>

#define PB_CR3_TRANSLATE_DELAY 1300

#define PB_DICT_SELECT 0
#define PB_DICT_EXIT 1
#define PB_DICT_ARTICLE_LIST 2
#define PB_DICT_DEACTIVATE 3
#define PB_DICT_SEARCH 4

#define PB_LINE_HEIGHT 30

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

static int cr_oriantations[4] = {
	ROTATE0, ROTATE90, ROTATE180, ROTATE270
};

// This PB FBreader stuff just to create mark indicating that book was opened for reading

typedef struct tdocstate_s {
	int magic;
	long long position;
	char preformatted;
	char  reserved11;
	short reserved2;
	short reserved3;
	short reserved4;
	char encoding[16];
	int nbmk;
	long long bmk[30];
} tdocstate;

static void translate_timer();

class CRPocketBookGlobals
{
private :
	char *_fileName;
	int _keepOrientation;
	lString8  _lang;
	lString8  _pbDictionary;
	bool _ready_sent;
	tdocstate _docstate;
	bool createFile(char *fName);
	bool _translateTimer;
public:
    CRPocketBookGlobals(char *fileName);
	char *getFileName() { return _fileName ; }
	int getKeepOrientation() { return _keepOrientation; }
	const char *getLang() { return _lang.c_str(); }
	const char *getDictionary() { return _pbDictionary.c_str(); }
	void saveState(int cpage, int npages);

	virtual ~CRPocketBookGlobals() { };
	void BookReady() 
	{
		if (!_ready_sent) {
			::BookReady(_fileName);
			_ready_sent = true;
		}
	}
	void startTranslateTimer()
	{
		SetHardTimer(const_cast<char *>("TranslateTimer"), translate_timer, PB_CR3_TRANSLATE_DELAY);
		_translateTimer = true;
	}
	void killTranslateTimer()
	{
		if (_translateTimer) {
			ClearTimer(translate_timer);
			_translateTimer = false;
		}
	}
	void translateTimerExpired()
	{
		_translateTimer = false;
	}
	bool isTranslateTimerRunning()
	{
		return _translateTimer;
	}
};

CRPocketBookGlobals::CRPocketBookGlobals(char *fileName)
{
	CRLog::trace("CRPocketBookGlobals(%s)", fileName);
	_fileName = fileName;
	_ready_sent = false;
	_translateTimer = false;
	iconfig *gc = OpenConfig(const_cast<char *>(GLOBALCONFIGFILE), NULL);
	_lang = ReadString(gc, const_cast<char *>("language"), const_cast<char *>("en"));
	CRLog::trace("language=%s", _lang.c_str());
	if (_lang == "ua")
		_lang = "uk";
	_keepOrientation = ReadInt(gc, const_cast<char *>("keeporient"), 0);
	_pbDictionary = ReadString(gc, const_cast<char *>("dictionary"), const_cast<char *>(""));
	CloseConfig(gc);
}

bool CRPocketBookGlobals::createFile(char *fName)
{
	lString16 filename(Utf8ToUnicode(fName));
    if ( !LVFileExists(filename) ) {
        lString16 path16 = LVExtractPath( filename );
		if (LVCreateDirectory( path16 )) {
			LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
			if ( stream.isNull() ) {
				CRLog::error("Cannot create file %s", fName);
				return false;
			}
		} else {
			lString8 fn = UnicodeToUtf8(path16.c_str());
			CRLog::error("Cannot create directory %s", fn.c_str());
			return false;
		}
	}
	return true;
}

void CRPocketBookGlobals::saveState(int cpage, int npages)
{
	char *af0 = GetAssociatedFile(_fileName, 0);

	if (createFile(af0)) {
		if (npages - cpage < 3 && cpage >= 5) {
			char *afz = GetAssociatedFile(_fileName, 'z');
			createFile(afz);
		}
	}
}

class CRPocketBookGlobals * pbGlobals = NULL;

static int keyPressed = -1;
static bool exiting = false;


char key_buffer[KEY_BUFFER_LEN];

#include <cri18n.h>

class CRPocketBookScreen : public CRGUIScreenBase {
private:
	bool _forceSoft;
public:
	static CRPocketBookScreen * instance;
protected:
	virtual void update( const lvRect & rc2, bool full )
	{
		if ( rc2.isEmpty() && !full )
			return;
		lvRect rc = rc2;
		rc.left &= ~3;
		rc.right = (rc.right + 3) & ~3;

		if (!_forceSoft && rc.height() > 400
#if ENABLE_UPDATE_MODE_SETTING==1
          		&& checkFullUpdateCounter()
#endif
		)
			full = true;
		else if (!_forceSoft)
			full = false;

		lUInt8 *screenbuf =  _front->GetScanLine(0);
		int w = _front->GetWidth(); int h = _front->GetHeight();
		Stretch(screenbuf, IMAGE_GRAY2, w, h, _front->GetRowSize(), 0, 0, w, h, 0);
		if ( full )
			FullUpdate();
		else if (rc.height() < 400) {
			PartialUpdateBW(rc.left, rc.top, rc.width(), rc.height());
		} else
			SoftUpdate();
	}
public:
	virtual ~CRPocketBookScreen()
	{
		instance = NULL;
	}

	CRPocketBookScreen( int width, int height )
	:  CRGUIScreenBase( width, height, true ), _forceSoft(false)
	{
		instance = this;
	}

	void MakeSnapShot() 
	{
		ClearScreen();
		lUInt8 *screenbuf =  _front->GetScanLine(0);
		int w = _front->GetWidth(); int h = _front->GetHeight();
		Stretch(screenbuf, IMAGE_GRAY2, w, h, _front->GetRowSize(), 0, 0, w, h, 0);
		PageSnapshot();
	}
	void setForceSoftUpdate(bool force) { _forceSoft = force; }
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
	{ "@KA_prse", DCMD_MOVE_BY_CHAPTER, -1},
	{ "@KA_nxse", DCMD_MOVE_BY_CHAPTER, 1},
	{ "@KA_obmk", MCMD_BOOKMARK_LIST_GO_MODE, 0},
	{ "@KA_nbmk", MCMD_BOOKMARK_LIST, 0},
	{ "@KA_nnot", MCMD_CITE, 0},
	{ "@KA_savp", GCMD_PASS_TO_PARENT, 0},
	{ "@KA_onot", GCMD_PASS_TO_PARENT, 0},
	{ "@KA_olnk", MCMD_GO_LINK, 0},
	{ "@KA_blnk", DCMD_LINK_BACK , 0},
	{ "@KA_cnts", PB_CMD_CONTENTS, 0},
	{ "@KA_srch", MCMD_SEARCH, 0},
	{ "@KA_dict", MCMD_DICT, 0},
	{ "@KA_zmin", DCMD_ZOOM_IN, 0},
	{ "@KA_zout", DCMD_ZOOM_OUT, 0},
	{ "@KA_hidp", GCMD_PASS_TO_PARENT, 0},
	{ "@KA_rtte", PB_CMD_ROTATE, 0},
	{ "@KA_mmnu", MCMD_MAIN_MENU, 0},
	{ "@KA_exit", MCMD_QUIT, 0},
	{ "@KA_mp3o", PB_CMD_MP3, 1},
	{ "@KA_mp3p", PB_CMD_MP3, 0},
	{ "@KA_volp", PB_CMD_VOLUME, 3},
	{ "@KA_volm", PB_CMD_VOLUME, -3},
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

		if (key && key[0] == '@') {
			const char * res8 = GetLangText((char *)key);
			res = Utf8ToUnicode( lString8(res8) );
        } else {
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
        _orientation = pocketbook_orientations[GetOrientation()];
        _screen = s;
        _ownScreen = true;
		instance = this;
		initPocketBookActionsTable();
    }

	void restoreOrientation(int storedOrientation)
	{
		if (_orientation != storedOrientation)
			reconfigure(ScreenWidth(), ScreenHeight(), (cr_rotate_angle_t)storedOrientation);
		const lChar16 * imgname =
			( _orientation &1 ) ? L"cr3_logo_screen_landscape.png" : L"cr3_logo_screen.png";
		LVImageSourceRef img = getSkin()->getImage(imgname);
		if ( !img.isNull() ) {
			_screen->getCanvas()->Draw(img, 0, 0, _screen->getWidth(), _screen->getHeight(),  false );
		} else {
			_screen->getCanvas()->Clear(0xFFFFFF);
		}
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
	
	bool hasKeyMapping(int key, int flags) {
		// NOTE: orientation related key substitution is performed by inkview
		for ( int i=_windows.length()-1; i>=0; i-- ) {
			if ( _windows[i]->isVisible() ) {
				int cmd, param;
				CRGUIAcceleratorTableRef accTable = _windows[i]->getAccelerators();

				if (!accTable.isNull() && accTable->translate( key, flags, cmd, param ) ) {
					if (cmd != GCMD_PASS_TO_PARENT ) 
						return true;
				}
			}
		}
		return false;
	}
	bool onKeyPressed( int key, int flags )
	{
		CRLog::trace("CRPocketBookWindowManager::onKeyPressed(%d, %d)", key, flags);
		if (pbGlobals->isTranslateTimerRunning()) {
			CRGUIAcceleratorTableRef accTable = _accTables.get("dict");
			if (!accTable.isNull()) {
				int cmd, param;
				if (accTable->translate(key, flags, cmd, param)) {
					switch (cmd) {
					case PB_CMD_LEFT:
					case PB_CMD_RIGHT:
					case PB_CMD_UP:
					case PB_CMD_DOWN:
					case MCMD_CANCEL:
					case MCMD_OK:
						CRLog::trace("Killing translate timer, cmd = %d", cmd);
						pbGlobals->killTranslateTimer();
						if (cmd == MCMD_OK)
							onCommand(PB_CMD_TRANSLATE, 0);
						break;
					default: 
						break;
					}
				}
			}
		}
		return CRGUIWindowManager::onKeyPressed(key, flags);
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

void tocHandler(long long position) 
{
	CRPocketBookWindowManager::instance->onCommand(MCMD_GO_PAGE_APPLY, position);
}

class CRPbDictionaryDialog;
class CRPbDictionaryMenu;

class CRPbDictionaryView : public CRViewDialog
{
private:
	CRPbDictionaryDialog *_parent;
	CRPbDictionaryMenu *_dictMenu;
	LVHashTable<lString16, int> _dictsTable;
	char ** _dictNames;
	int _dictIndex;
	int _dictCount;
	lString16 _word;
	lString16 _searchPattern;
	bool _active;
	int _selectedIndex;
	int _itemsCount;
	LVImageSourceRef _toolBarImg;
	int _translateResult;
	char *_newWord;
	char *_newTranslation;
private:
	void searchDictinary();
protected:
	virtual void selectDictionary();
	virtual void onDictionarySelect();
	virtual bool onItemSelect();
	virtual lString8 createArticle(const char *word, const char *translation);
public:
	CRPbDictionaryView(CRGUIWindowManager * wm, CRPbDictionaryDialog *parent);
	virtual ~CRPbDictionaryView();
	virtual void draw();
	virtual void translate(const lString16 &w);
	virtual void showTranslation(const char *word, const char *translation);
	virtual bool onCommand( int command, int params );
	void doActivate();
	int getCurItem() { return _selectedIndex; }
	void setCurItem(int index);
	virtual void drawTitleBar();
	virtual void Update();
	bool isArticleListActive()
	{
		return (_selectedIndex == PB_DICT_ARTICLE_LIST && _translateResult != 0);
	}
	void closeDictionary();
	virtual void setRect( const lvRect & rc );
	void scheduleDictListUpdate(const char *word, const char *translation)
	{
		if (_newWord == NULL) {
			_newWord = const_cast<char *>(word);
			_newTranslation = const_cast<char *>(translation);
		}
	}
	virtual void reconfigure( int flags );
	int getDesiredHeight();
	virtual bool isDirty();
};

class CRPbDictionaryMenuItem : public CRMenuItem
{
private:
	const char *_word;
	const char *_translation;
	lString16 _word16;
	lString16 _translation16;
protected:
	lString16 createItemValue(const char *_translation);
public:
    CRPbDictionaryMenuItem(CRMenu * menu, int id, const char *word, const char *translation);
    virtual void Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected );
    const char * getWord() { return _word; }
    const char * getTranslation() { return _translation; }
};

class CRPbDictionaryMenu : public CRMenu
{
private:
	CRPbDictionaryView *_parent;
protected:
	virtual void drawTitleBar() { _parent->drawTitleBar(); }
public:
    CRPbDictionaryMenu(CRGUIWindowManager * wm, CRPbDictionaryView *parent);
    virtual const lvRect & getRect() { return _rect; }
	virtual bool onCommand( int command, int params = 0 );
	virtual void setCurItem(int nItem);
    bool newPage(const char *word, const char *translation);
    bool nextPage();
    bool prevPage();
    virtual void draw() { CRMenu::draw(); _dirty = false; }
    void invalidateMenu()
    {
	    _pageUpdate = true;
		setDirty();
	}
	virtual void reconfigure( int flags );
	virtual lvPoint getMaxItemSize()
	{
		lvPoint pt = CRMenu::getMaxItemSize();

		pt.y = pt.y * 3/4;
		return pt;
	}
};

static void translate_timer() 
{
	CRLog::trace("translate_timer()");
	pbGlobals->translateTimerExpired();
	CRPocketBookWindowManager::instance->onCommand(PB_CMD_TRANSLATE, 0);
}

class CRPbDictionaryDialog : public CRGUIWindowBase
{
protected:
	CRViewDialog * _docwin;
	LVDocView * _docview;
	LVPageWordSelector * _wordSelector;
	CRPbDictionaryView * _dictView;
	bool _dictViewActive;
	lString16 _selText;
protected:
	virtual void draw();
    void endWordSelection();
    bool isWordSelection() { return _wordSelector!=NULL; }
    void onWordSelection();
    bool _docDirty;
    static int _curPage;
    static int _lastWordX;
    static int _lastWordY;
public:
	static CRPbDictionaryDialog * create( CRGUIWindowManager * wm, CRViewDialog * docwin, lString8 css );
	CRPbDictionaryDialog( CRGUIWindowManager * wm, CRViewDialog * docwin,  lString8 css);
	virtual ~CRPbDictionaryDialog() {
		CRPocketBookScreen::instance->setForceSoftUpdate(false);
		if (_wordSelector) {
			delete _wordSelector;
			_wordSelector = NULL;
		}
		delete _dictView;
		_dictView = NULL;
	}
	/// returns true if command is processed
	virtual bool onCommand( int command, int params );
	void activateDictView(bool active) 
	{
		if (_dictViewActive != active) {
			_dictViewActive = active; 
			if (!active && isWordSelection())
				_wordSelector->moveBy(DIR_ANY, 0);
			setDocDirty();
			Update();
		}
	}
    void startWordSelection();
	virtual void Update();
	bool isDocDirty() { return _docDirty; }
	void setDocDirty() { _docDirty = true; }
	virtual void reconfigure( int flags )
	{
		CRGUIWindowBase::reconfigure(flags);
		_dictView->reconfigure(flags);
	}
};


class CRPocketBookDocView : public V3DocViewWin {
private:
	ibitmap *_bm3x3;
	char *_strings3x3[9];
	int _quick_menuactions[9];
	tocentry *_toc;
	int _tocLength;
	bool _dictCssloaded;
	lString8 _dictCss;

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
			LVImageSourceRef img = _wm->getSkin()->getImage(L"cr3_pb_quickmenu.png");
			if ( !img.isNull() ) {
				_bm3x3 = NewBitmap(img->GetWidth(), img->GetHeight());
				LVGrayDrawBuf tmpBuf( img->GetWidth(), img->GetHeight() );

				tmpBuf.Draw(img, 0, 0, img->GetWidth(), img->GetHeight(), true);
				memcpy(_bm3x3->data, tmpBuf.GetScanLine(0), _bm3x3->height * _bm3x3->scanline);
			} else
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

	bool rotateApply(int params, bool saveOrient = true) 
	{
		int orient = GetOrientation();
		if (orient == params)
			return true;
		if (params == -1 || pbGlobals->getKeepOrientation() == 0 || pbGlobals->getKeepOrientation() == 2) {
			SetGlobalOrientation(params);
			saveOrient = false;
		} else {
			SetOrientation(params);
		}
		cr_rotate_angle_t oldOrientation = pocketbook_orientations[orient];
		cr_rotate_angle_t newOrientation = pocketbook_orientations[GetOrientation()];
		if (saveOrient) {
			CRPropRef newProps = getNewProps();
			newProps->setInt(PROP_POCKETBOOK_ORIENTATION, newOrientation);
			applySettings();
			saveSettings( lString16() );
		}
		int dx = _wm->getScreen()->getWidth();
		int dy = _wm->getScreen()->getHeight();
		if ((oldOrientation & 1) == (newOrientation & 1)) {
			_wm->reconfigure(dx, dy, newOrientation);
			_wm->update(true);
		} else
			_wm->reconfigure(dy, dx, newOrientation);

		return true;
	}

	bool quickMenuApply(int params) 
	{
		if (params >= 0 && params < 9) {
			int index = _quick_menuactions[params];
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
    : V3DocViewWin( wm, dataDir ), _tocLength(0), _toc(NULL), _bm3x3(NULL), _dictCssloaded(false)
    {
        instance = this;
    }

    virtual void closing()
    {
		pbGlobals->saveState(getDocView()->getCurPage(), getDocView()->getPageCount());
		CRLog::trace("V3DocViewWin::closing();");
        V3DocViewWin::closing();
        if (!exiting)
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
		case mm_Orientation:
			{
				bool saveOrientation = (params != 1525); //magic number :)
				_newProps->getInt(PROP_POCKETBOOK_ORIENTATION, params);
				rotateApply(cr_oriantations[params], saveOrientation);
				return true;
			}
		case PB_CMD_ROTATE_ANGLE_SET: 
			return rotateApply(params);
		case MCMD_SEARCH:
			_searchPattern.clear();
			OpenKeyboard(const_cast<char *>("@Search"), key_buffer, KEY_BUFFER_LEN, 0, searchHandler);
			return true;
		case MCMD_SEARCH_FINDFIRST:
			_searchPattern += Utf8ToUnicode(key_buffer);
			if ( !_searchPattern.empty() && params ) {
                if ( findText( _searchPattern, 0, 1 ) || findText( _searchPattern, -1, 1 )) {
                    CRSelNavigationDialog * dlg = new CRSelNavigationDialog( _wm, this, _searchPattern );
                    _wm->activateWindow( dlg );
                } else
					Message(ICON_INFORMATION, const_cast<char*>("@Search"), const_cast<char*>("@No_more_matches"), 2000);
            }
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
		case MCMD_DICT:
			showDictDialog();
			return true;
		case PB_CMD_CONTENTS: 
			showContents();
			return true;
        case MCMD_GO_LINK:
            showLinksDialog();
            return true;
        case PB_CMD_MP3:
			if (params == 0)
				TogglePlaying();
			else
				OpenPlayer();
			return true;
        case PB_CMD_VOLUME:
			SetVolume(GetVolume() + params);
			return true;
		default:
			break;
		}
		return V3DocViewWin::onCommand( command, params );
	}

	bool showLinksDialog()
	{
		CRLinksDialog * dlg = CRLinksDialog::create( _wm, this );
		if ( !dlg )
			return false;
		dlg->setAccelerators( getDialogAccelerators() );
		_wm->activateWindow( dlg );
		return true;
	}

	void showDictDialog() 
	{
		if (!_dictCssloaded) {
			_dictCssloaded = true;
			lString16 filename = L"dict.css";
			if (_cssDir.length() > 0 && LVFileExists( _cssDir + filename ))
				LVLoadStylesheetFile( _cssDir + filename, _dictCss );
		}
		CRPbDictionaryDialog * dlg = new CRPbDictionaryDialog( _wm, this, _dictCss );
		_wm->activateWindow( dlg );
		dlg->startWordSelection();
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
    CRPropRef getNewProps() {
		_props = _docview->propsGetCurrent() | _props;
		_newProps = LVClonePropsContainer( _props );
		return _newProps;
	}
};

CRPocketBookDocView * CRPocketBookDocView::instance = NULL;

CRPbDictionaryView::CRPbDictionaryView(CRGUIWindowManager * wm, CRPbDictionaryDialog *parent) 
	: CRViewDialog(wm, lString16(), lString8(), lvRect(), false, true), _parent(parent), _itemsCount(5),
	_dictsTable(16), _active(false), _newWord(NULL), _newTranslation(NULL), _translateResult(0)
{
	setSkinName(lString16(L"#dict"));
	lvRect rect = _wm->getScreen()->getRect();
    if ( !_wm->getSkin().isNull() ) {
		_skin = _wm->getSkin()->getWindowSkin(getSkinName().c_str());
		_toolBarImg = _wm->getSkin()->getImage(L"cr3_dict_tools.png");
        CRRectSkinRef clientSkin = _skin->getClientSkin();
        if ( !clientSkin.isNull() ) {
            getDocView()->setBackgroundColor(clientSkin->getBackgroundColor());
            getDocView()->setTextColor(clientSkin->getTextColor());
            getDocView()->setFontSize(clientSkin->getFontSize());
            getDocView()->setDefaultFontFace(UnicodeToUtf8(clientSkin->getFontFace()));
            getDocView()->setPageMargins( clientSkin->getBorderWidths() );
        }
	}
	rect.top = rect.bottom - getDesiredHeight();
	_dictNames = EnumDictionaries();
	for (_dictCount = 0; _dictNames[_dictCount]; _dictCount++) {
		_dictsTable.set(Utf8ToUnicode( lString8(_dictNames[_dictCount]) ), _dictCount);
	}
	CRLog::trace("_dictCount = %d", _dictCount);
	_dictMenu = new CRPbDictionaryMenu(_wm, this);
	setRect(rect);
	_dictMenu->reconfigure(0);
	CRPropRef props = CRPocketBookDocView::instance->getProps();
	lString16 lastDict = props->getStringDef(PROP_POCKETBOOK_DICT, pbGlobals->getDictionary());
	_dictIndex = lastDict.empty() ? 0 : _dictsTable.get(lastDict);
	if (_dictCount > 0 && _dictIndex >= 0) {
		int rc = OpenDictionary(_dictNames[_dictIndex]);
		if (rc == 1) {
			_caption = Utf8ToUnicode( lString8(_dictNames[_dictIndex]) );
			getDocView()->createDefaultDocument(lString16(), Utf8ToUnicode(TR("@Word_not_found")));
			return;
		}
		CRLog::error("OpenDictionary(%s) returned %d", _dictNames[_dictIndex], rc);
	}
	_dictIndex = -1;
	getDocView()->createDefaultDocument(lString16(), Utf8ToUnicode(TR("@Dic_error")));	
}

CRPbDictionaryView::~CRPbDictionaryView() 
{
	if (_dictIndex >= 0)
		CloseDictionary();
	delete _dictMenu;
	_dictMenu = NULL;
}

void CRPbDictionaryView::setRect( const lvRect & rc )
{
	CRViewDialog::setRect(rc);
	_dictMenu->setRect(getRect());
}

void CRPbDictionaryView::Update()
{
	if (isArticleListActive())
		_dictMenu->setDirty();
	else
		setDirty();
	_parent->Update();
}

bool CRPbDictionaryView::isDirty()
{
	if (isArticleListActive()) {
		return _dictMenu->isDirty();
	}
	return CRViewDialog::isDirty();
}

void CRPbDictionaryView::drawTitleBar()
{
	CRLog::trace("CRPbDictionaryView::drawTitleBar()");
    LVDrawBuf & buf = *_wm->getScreen()->getCanvas();
    CRWindowSkinRef skin( _wm->getSkin()->getWindowSkin(_skinName.c_str()) );
    CRRectSkinRef titleSkin = skin->getTitleSkin();
    lvRect titleRc;
    if ( !getTitleRect( titleRc ) )
        return;
    titleSkin->draw( buf, titleRc );
	lvRect borders = titleSkin->getBorderWidths();
    buf.SetTextColor( skin->getTextColor() );
    buf.SetBackgroundColor( skin->getBackgroundColor() );
    int imgWidth = 0;
    int hh = titleRc.bottom - titleRc.top;
    if ( !_icon.isNull() ) {
        int w = _icon->GetWidth();
        int h = _icon->GetHeight();
        buf.Draw( _icon, titleRc.left + hh/2-w/2, titleRc.top + hh/2 - h/2, w, h );
        imgWidth = w + 8;
    }
    int tbWidth = 0;
    if (!_toolBarImg.isNull()) {
		tbWidth = _toolBarImg->GetWidth();
		int h = _toolBarImg->GetHeight();
		titleRc.right -= (tbWidth + titleSkin->getBorderWidths().right);
		buf.Draw(_toolBarImg, titleRc.right, titleRc.top + hh/2 - h/2, tbWidth, h );
	}
    lvRect textRect = titleRc;
    textRect.left += imgWidth;
    titleSkin->drawText( buf, textRect, _caption );	
    if (_active) {
		lvRect selRc;

		if (_selectedIndex != 0 && tbWidth > 0) {
			int itemWidth = tbWidth/(_itemsCount -1);
			selRc = titleRc;
			selRc.left = titleRc.right + itemWidth * (_selectedIndex -1);
			selRc.right = selRc.left + itemWidth;
		} else {
			selRc = textRect;
			selRc.left += borders.left;
		}
		selRc.top += borders.top;
		selRc.bottom -= borders.bottom;
		buf.InvertRect(selRc.left, selRc.top, selRc.right, selRc.bottom);
	}
}

void CRPbDictionaryView::draw()
{
	if (isArticleListActive()) {
		_dictMenu->draw();
	} else {
		CRViewDialog::draw();
		_dirty = false;
	}
}

void CRPbDictionaryView::selectDictionary()
{
	CRLog::trace("selectDictionary()");
	LVFontRef valueFont(fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Arial")));
	CRMenu * dictsMenu = new CRMenu(_wm, NULL, PB_CMD_SELECT_DICT,
			lString16(""), LVImageSourceRef(), LVFontRef(), valueFont,
			 CRPocketBookDocView::instance->getNewProps(), PROP_POCKETBOOK_DICT);
	dictsMenu->setAccelerators(_wm->getAccTables().get("menu"));
	dictsMenu->setSkinName(lString16(L"#settings"));
	for (int i = 0; i < _dictCount; i++) {
		lString16 dictName = Utf8ToUnicode(_dictNames[i]);
		dictsMenu->addItem( new CRMenuItem(dictsMenu, i,
			dictName,
			LVImageSourceRef(),
			LVFontRef(),
			dictName.c_str()));
	}
	dictsMenu->reconfigure( 0 );
	_wm->activateWindow(dictsMenu);
}

void CRPbDictionaryView::onDictionarySelect()
{
	CRPocketBookDocView::instance->applySettings();
	CRPropRef props = CRPocketBookDocView::instance->getProps();
	lString16 lastDict = props->getStringDef(PROP_POCKETBOOK_DICT);
	int index = _dictsTable.get(lastDict);
	CRLog::trace("CRPbDictionaryView::onDictionarySelect(%d)", index);
	if (index >= 0 && index <= _dictCount) {
		if (_dictIndex >= 0) {
			CloseDictionary();
		}
		int rc = OpenDictionary(_dictNames[index]);
		if (rc == 1) {
			_dictIndex = index;
			CRPocketBookDocView::instance->saveSettings(lString16());
		} else {
			_dictIndex = -1;
			CRLog::error("OpenDictionary(%s) returned %d", _dictNames[_dictIndex], rc);
		}
		lString16 word = _word;
		_word.clear();
		_caption = lastDict;
		_parent->setDocDirty();
		_selectedIndex = PB_DICT_DEACTIVATE;
		translate(word);
	}
}

void CRPbDictionaryView::setCurItem(int index)
{
	CRLog::trace("setCurItem(%d)", index);
	if (index < 0)
		index = _itemsCount -1;
	else if (index >= _itemsCount)
		index = 0;
	_selectedIndex = index;
	if (index == PB_DICT_ARTICLE_LIST) {
		if (_newWord != NULL) {
			_dictMenu->newPage(_newWord, _newTranslation);
			_newWord = _newTranslation = NULL;
		} else
			_dictMenu->invalidateMenu();
	}
	Update();
}

void CRPbDictionaryView::searchDictinary()
{
	_searchPattern.clear();
	OpenKeyboard(const_cast<char *>("@Search"), key_buffer, KEY_BUFFER_LEN, 0, searchHandler);
}

void CRPbDictionaryView::closeDictionary()
{
	_active = false;
	_parent->activateDictView(false);
	_wm->postCommand(MCMD_CANCEL, 0);
}

bool CRPbDictionaryView::onItemSelect()
{
	CRLog::trace("onItemSelect() : %d", _selectedIndex);
	switch(_selectedIndex) {
	case PB_DICT_SELECT:
		selectDictionary();
		return true;
	case PB_DICT_EXIT:
		closeDictionary();
		return true;
	case PB_DICT_ARTICLE_LIST:
		return true;
	case PB_DICT_DEACTIVATE:
		setDirty();
		_parent->activateDictView(_active = false);
		return true;
	case PB_DICT_SEARCH:
		searchDictinary();
		return true;
	}
}

bool CRPbDictionaryView::onCommand( int command, int params )
{
	CRLog::trace("CRPbDictionaryView::onCommand(%d, %d)", command, params);

	if (isArticleListActive())
		return _dictMenu->onCommand(command, params);

	switch (command) {
	case MCMD_CANCEL:
		closeDictionary();
		return true;
	case MCMD_OK:
		return onItemSelect();
	case PB_CMD_RIGHT:
		setCurItem(getCurItem() + 1);
		return true;
	case PB_CMD_LEFT:
		setCurItem(getCurItem() - 1);
		return true;
	case PB_CMD_SELECT_DICT:
		onDictionarySelect();
		return true;
	case PB_CMD_UP:
		command = DCMD_PAGEUP;
		break;
	case PB_CMD_DOWN:
		command = DCMD_PAGEDOWN;
		break;
	case MCMD_SEARCH_FINDFIRST:
		_searchPattern += Utf8ToUnicode(key_buffer);
		if ( !_searchPattern.empty() && params ) {
			translate(_searchPattern);
			setDirty();
			_parent->Update();
		}
		return true;
	default:
		break;
	}
	if (CRViewDialog::onCommand(command, params)) {
		Update();
		return true;
	}
	return false;
}

lString8 CRPbDictionaryView::createArticle(const char *word, const char *translation)
{
	lString8 article;

	article << "<title><p>" << word << "</p></title>";
	if (translation != NULL) {
		lString16 src = Utf8ToUnicode(translation), dst;
		//article << "<section style=\"text-align: left; text-indent: 0; font-size: 70%\">";
		article << "<section>";
		article << "<p>";
		int offset = 0, count = 0;
		const lChar16 *closeTag = NULL;
		for (int i = 0; i < src.length(); i++) {
			lChar16 currentChar = src[i];
			if (currentChar == 1 || currentChar == 2 || currentChar == 3 ||
				currentChar == '\n') {
				if (count > 0) {
					dst.append(src, offset, count);
					count = 0;
				}
				offset = i + 1;
				switch (currentChar) {
				case 1:
					if (closeTag != NULL) {
						dst << closeTag;
						closeTag = NULL;
					}
					break;
				case 2:
					dst << L"<emphasis>";
					closeTag = L"</emphasis>";
					break;
				case 3:
					dst << L"<strong>";
					closeTag = L"</strong>";
					break;
				case '\n':
					dst << L"</p><p>";
				default:
					break;
				}
			} else
				count++;
		}
		if (offset != 0) {
			if (count > 0)
				dst.append(src, offset, count);
			if (closeTag != NULL)
				dst.append(closeTag);
			dst << L"</p>";
			article << UnicodeToUtf8(dst);
		} else
			article << translation;
		article << "</section>";
	}
	return article;
}

void CRPbDictionaryView::translate(const lString16 &w)
{
	lString8 body;

	CRLog::trace("CRPbDictionaryView::translate() start");
	if (_dictIndex >= 0) {
		lString16 s16 = w;
		if (s16 == _word)
			return;
		_word = s16;

		s16.lowercase();
		lString8 what = UnicodeToUtf8( s16 );
		char *word = NULL, *translation = NULL;
		
		_translateResult = LookupWord((char *)what.c_str(), &word, &translation);
		CRLog::trace("LookupWord(%s) returned %d", what.c_str(), _translateResult);
		if (_translateResult != 0) {
			if (_translateResult == 2) {
				_selectedIndex = PB_DICT_ARTICLE_LIST;
				_dictMenu->newPage(word, translation);
				_newWord = _newTranslation = NULL;
				setDirty();
				return;
			}
			body = createArticle(word, translation);
			_newWord = word;
			_newTranslation = translation;
		} else {
			body << TR("@Word_not_found");
		}
	} else if (_dictCount <= 0) {
		body << TR("@Dic_not_installed");
	} else {
		body << TR("@Dic_error");
	}
	setDirty();
	_selectedIndex = PB_DICT_DEACTIVATE;
	_stream = LVCreateStringStream( CRViewDialog::makeFb2Xml( body ) );
	getDocView()->LoadDocument(_stream);
	CRLog::trace("CRPbDictionaryView::translate() end");
}

void CRPbDictionaryView::showTranslation(const char *word, const char *translation)
{
	lString8 body = createArticle(word, translation);
	_stream = LVCreateStringStream( CRViewDialog::makeFb2Xml( body ) );
	getDocView()->LoadDocument(_stream);
}

void CRPbDictionaryView::reconfigure( int flags )
{
	CRViewDialog::reconfigure(flags);
	_dictMenu->reconfigure(flags);
}

void CRPbDictionaryView::doActivate()
{
	_active = true;
	setCurItem(getCurItem());
}

int CRPbDictionaryView::getDesiredHeight()
{
	int dh = (_wm->getScreenOrientation() & 0x1) ? 200 : 300; 

	if (_skin.isNull())
		return dh;
	lvRect screenRect = _wm->getScreen()->getRect();
	lvRect skinRect;
	_skin->getRect(skinRect, screenRect);
	int sh = (screenRect.height() >> 1) - PB_LINE_HEIGHT;
	if (skinRect.height() <= 0)
		return dh;
	if (skinRect.height() > sh)
		return sh;
	return skinRect.height();
}

lString16 CRPbDictionaryMenuItem::createItemValue(const char * translation)
{
	lString16 src = Utf8ToUnicode(translation);
	lString16 dst;
	int count = src.length();
	if (count > 256)
		count = 256;
	for (int i = 0; i < count; i++) {
		lChar16 currentChar = src[i];
		if ((currentChar == 1 || currentChar == 2 || currentChar == 3))
			continue;
		if (currentChar == '\n')
			currentChar = ' ';
		dst << currentChar;
	}
	return dst;
}

CRPbDictionaryMenuItem::CRPbDictionaryMenuItem(CRMenu * menu, int id, const char *word, const char *translation)
	: CRMenuItem(menu, id, lString16(), LVImageSourceRef(), LVFontRef() ), _word( word ), _translation(translation)
{
	_word16 = Utf8ToUnicode(word);
	_translation16 = createItemValue(_translation);
}

void CRPbDictionaryMenuItem::Draw( LVDrawBuf & buf, lvRect & rc, CRRectSkinRef skin, CRRectSkinRef valueSkin, bool selected)
{
    _itemDirty = false;
    lvRect itemBorders = skin->getBorderWidths();
    skin->draw( buf, rc );
    buf.SetTextColor( 0x000000 );
    buf.SetBackgroundColor( 0xFFFFFF );
    int imgWidth = 0;
    int hh = rc.bottom - rc.top - itemBorders.top - itemBorders.bottom;
    if ( !_image.isNull() ) {
        int w = _image->GetWidth();
        int h = _image->GetHeight();
        buf.Draw( _image, rc.left + hh/2-w/2 + itemBorders.left, rc.top + hh/2 - h/2 + itemBorders.top, w, h );
        imgWidth = w + 8;
    }
    lvRect textRect = rc;
    textRect.left += imgWidth;
    lString16 word = _word16 + L" ";
    lvPoint sz = skin->measureTextItem(word);
    textRect.right = textRect.left + sz.x;
    skin->drawText( buf, textRect, word);
    textRect.left = textRect.right + 1;
    textRect.right = rc.right;
    valueSkin->drawText( buf, textRect, _translation16 );
    if (selected)
		buf.InvertRect(rc.left, rc.top, rc.right, rc.bottom);
}

    
CRPbDictionaryMenu::CRPbDictionaryMenu(CRGUIWindowManager * wm, CRPbDictionaryView *parent)
    : CRMenu(wm, NULL, 0, lString16(), LVImageSourceRef(), LVFontRef(), LVFontRef() ), _parent(parent)
{
    _fullscreen = false;
    setSkinName(lString16("#dict-list"));
}

bool CRPbDictionaryMenu::newPage(const char *firstWord, const char *firstTranslation)
{
	char *word = NULL, *translation = NULL;
	_items.clear();
	addItem(new CRPbDictionaryMenuItem(this, 0, firstWord, firstTranslation));
	for (int i = 1; i < _pageItems; i++) {
		int result = LookupNext(&word, &translation);
		if (result == 0)
			break;
		addItem(new CRPbDictionaryMenuItem(this, i, word, translation));
	}
	_topItem = _selectedItem = 0;
	invalidateMenu();
	return true;
}

bool CRPbDictionaryMenu::nextPage()
{
	char *word = NULL, *translation = NULL;
	if (_items.length() == 0)
		return false;
	CRPbDictionaryMenuItem *last = static_cast<CRPbDictionaryMenuItem *>(_items[_items.length() - 1]);
	
	int result = LookupWord(const_cast<char *>(last->getWord()), &word, &translation);
	if (result != 0) {
		result = LookupNext(&word, &translation);
		if (result != 0)
			return newPage(word, translation);
	}
	return false;
}

bool CRPbDictionaryMenu::prevPage()
{
	char *word = NULL, *translation = NULL;
	if (_items.length() == 0)
		return false;
	CRPbDictionaryMenuItem *first = static_cast<CRPbDictionaryMenuItem *>(_items[_topItem]);
	
	int result = LookupWord(const_cast<char *>(first->getWord()), &word, &translation);
	if (result != 0) {
		for (int i = 0; i < _pageItems; i++) {
			int result = LookupPrevious(&word, &translation);
			if (result == 0)
				break;
			_items.insert(0, new CRPbDictionaryMenuItem(this, i, word, translation));
		}
		int l = _items.length();
		if (l > _pageItems)
			_items.erase(_pageItems, l - _pageItems);
		_selectedItem = getLastOnPage() -1;
		invalidateMenu();
		return true;
	}
	return false;
}

void CRPbDictionaryMenu::setCurItem(int nItem)
{
	int lastOnPage = getLastOnPage();
	if (nItem < _topItem) {
		prevPage();
	} else if (nItem > lastOnPage -1) {
		nextPage();
	} else {
		_items[_selectedItem]->onLeave();
		_items[_selectedItem = nItem]->onEnter();		
	}
	_parent->Update();
}

bool CRPbDictionaryMenu::onCommand( int command, int params )
{
	int nextItem = 0;

	switch (command) {
	case MCMD_CANCEL:
		_parent->closeDictionary();
		return true;
	case MCMD_OK:
	case PB_CMD_RIGHT:
		nextItem = PB_DICT_DEACTIVATE;
		break;
	case PB_CMD_LEFT:
		nextItem = PB_DICT_EXIT;
		break;
	case PB_CMD_UP:
		setCurItem(_selectedItem - 1);
		return true;
	case PB_CMD_DOWN:
		setCurItem(_selectedItem + 1);
		return true;
	default:
		return false;
	}
	if (_selectedItem >= 0 && _selectedItem < _items.length()) {
		CRPbDictionaryMenuItem *item = static_cast<CRPbDictionaryMenuItem *>(_items[_selectedItem]);

		_parent->showTranslation(item->getWord(), item->getTranslation());
	}
	_parent->setCurItem(nextItem);
	return true;
}

void CRPbDictionaryMenu::reconfigure( int flags )
{
	int pageitems = _pageItems;
	CRMenu::reconfigure(flags);
	if (_items.length() == 0)
		return;
	if (_pageItems > pageitems) {
		CRPbDictionaryMenuItem *item = static_cast<CRPbDictionaryMenuItem *>(_items[0]);
		if (_parent->isArticleListActive()) 
			// reload menu contents
			newPage(item->getWord(), item->getTranslation());
		else 
			_parent->scheduleDictListUpdate(item->getWord(), item->getTranslation());
	}
}

int CRPbDictionaryDialog::_curPage = 0;
int CRPbDictionaryDialog::_lastWordX = 0;
int CRPbDictionaryDialog::_lastWordY = 0;

CRPbDictionaryDialog::CRPbDictionaryDialog( CRGUIWindowManager * wm, CRViewDialog * docwin, lString8 css )
: CRGUIWindowBase( wm ), _docwin(docwin), _docview(docwin->getDocView()), _docDirty(true)
{
	_wordSelector = NULL;
	_fullscreen = true;
    CRGUIAcceleratorTableRef acc = _wm->getAccTables().get("dict");
    if ( acc.isNull() )
        acc = _wm->getAccTables().get("dialog");
    _dictView = new CRPbDictionaryView(wm, this);
	if (!css.empty())
		_dictView->getDocView()->setStyleSheet(css);
	setAccelerators( acc );
	_dictViewActive = false;
	CRPocketBookScreen::instance->setForceSoftUpdate(true);
}

CRPbDictionaryDialog * CRPbDictionaryDialog::create( CRGUIWindowManager * wm, CRViewDialog * docwin, lString8 css )
{
	return new CRPbDictionaryDialog(wm, docwin, css);
}

void CRPbDictionaryDialog::startWordSelection()
{
	if (isWordSelection())
		endWordSelection();
    _wordSelector = new LVPageWordSelector(_docview);
    int curPage = _docview->getCurPage();
    CRLog::trace("CRPbDictionaryDialog::startWordSelection(), _curPage = %d, _lastWordX=%d, _lastWordY=%d",
		_curPage, _lastWordX, _lastWordY);
    if (_curPage != curPage) {
		_curPage = curPage;
		_lastWordY = _docview->GetPos();
		_lastWordX = 0;
		_selText.clear();
	}
	_wordSelector->selectWord(_lastWordX, _lastWordY);
	onWordSelection();
}

void CRPbDictionaryDialog::endWordSelection()
{
	if (isWordSelection()) {
		delete _wordSelector;
		_wordSelector = NULL;
		_docview->clearSelection();
	}
}

void CRPbDictionaryDialog::onWordSelection() 
{
	CRLog::trace("CRPbDictionaryDialog::onWordSelection()");
	ldomWordEx * word = _wordSelector->getSelectedWord();
	if ( word ) {
		lvRect dictRc = _dictView->getRect();
		lvRect rc(dictRc);
		lvRect mRc = _docview->getPageMargins();
		if (dictRc.top > 0) {
			int y = word->getMark().end.y - _docview->GetPos() + _docview->getPageHeaderHeight() + mRc.top + PB_LINE_HEIGHT;
			if (y >= dictRc.top) {
				rc.top = 0;
				rc.bottom = _dictView->getDesiredHeight();
				_dictView->setRect(rc);
			}
		} else {
			int y = word->getMark().start.y - _docview->GetPos() + _docview->getPageHeaderHeight() + mRc.top;
			if (y <= dictRc.bottom) {
				rc.bottom = _wm->getScreen()->getHeight();
				rc.top = rc.bottom - _dictView->getDesiredHeight();
				_dictView->setRect(rc);
			}
		}
		lvPoint middle = word->getMark().getMiddlePoint();
		_lastWordX = middle.x;
		_lastWordY = middle.y;
		bool firstTime = _selText.empty();
		_selText = word->getText();
		CRLog::trace("_selText = %s", UnicodeToUtf8( _selText).c_str());
		if (!firstTime) {
			pbGlobals->startTranslateTimer();
			setDocDirty();
			Update();
		} else
			_wm->onCommand(PB_CMD_TRANSLATE, 0);
	}
}

bool CRPbDictionaryDialog::onCommand( int command, int params )
{
	if (params == 0)
		params = 1;

	MoveDirection dir = DIR_ANY;
	int curPage;
	bool ret;

	CRLog::trace("CRPbDictionaryDialog::onCommand(%d,%d) _dictActive=%d", command, params, _dictViewActive);
	if (_dictViewActive)
		return _dictView->onCommand(command, params);
	
    switch ( command ) {
	case DCMD_PAGEUP:
	case DCMD_PAGEDOWN:
		curPage = _docview->getCurPage();
		ret = _docview->doCommand((LVDocCmd)command, params);
		if (curPage != _docview->getCurPage()) {
			setDocDirty();
			startWordSelection();
		}
		return ret;
    case MCMD_OK:
		_dictViewActive = true;
		_docview->clearSelection();
		setDocDirty();
		_dictView->doActivate();
		return true;
    case MCMD_CANCEL:
        _docview->clearSelection();
        _wm->closeWindow( this );
        return true;
	case PB_CMD_LEFT:
		dir = DIR_LEFT;
        break;
	case PB_CMD_RIGHT:
		dir = DIR_RIGHT;
        break;
	case PB_CMD_UP:
		dir = DIR_UP;
        break;
	case PB_CMD_DOWN:
		dir = DIR_DOWN;
        break;
    case PB_CMD_TRANSLATE:
		_dictView->translate(_selText);
		Update();
		return true;
	default:
		return false;
	}
	CRLog::trace("Before move");
	_wordSelector->moveBy(dir, params);
	onWordSelection();
	CRLog::trace("After move");
	return true;
}

void CRPbDictionaryDialog::draw()
{
	if (_docDirty) {
		LVDocImageRef page = _docview->getPageImage(0);
		LVDrawBuf & canvas = *_wm->getScreen()->getCanvas();
		lvRect saveClip;
		canvas.GetClipRect(&saveClip);
		LVDrawBuf * buf = page->getDrawBuf();
		lvRect docRc = getRect();
		lvRect dictRc = _dictView->getRect();
		if (dictRc.top > 0)
			docRc.bottom = dictRc.top - 1;
		else
			docRc.top = dictRc.bottom + 1;
		canvas.SetClipRect(&docRc);
		_wm->getScreen()->draw( buf );
		canvas.SetClipRect(&saveClip);
		_docDirty = false;
	}
	if (_dictView->isDirty())
		_dictView->draw();
}

void CRPbDictionaryDialog::Update()
{
    setDirty();
    _wm->update(false);
}

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
			pbTable.add(i, 0, commandId, commandParam);
		}
		if (keypresslong[i]) {
			CRPocketBookWindowManager::instance->getPocketBookCommand(keypresslong[i], commandId, commandParam);
			CRLog::trace("keypresslong[%d] = %s, cmd = %d, param=%d", i, keypresslong[i], commandId, commandParam);
			pbTable.add(i, 1, commandId, commandParam);
		}
	}
	CRGUIAcceleratorTableRef mainTable = winman.getAccTables().get("main");
	if (!mainTable.isNull()) {
		CRLog::trace("main accelerator table is not null");
		mainTable->addAll(pbTable);
	}
}


int InitDoc(const char *exename, char *fileName)
{
    static const lChar16 * css_file_name = L"fb2.css"; // fb2

#ifdef __i386__
    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);
#else
    InitCREngineLog(USERDATA"/share/cr3/crlog.ini");
#endif
	
    CRLog::trace("InitDoc()");

	pbGlobals = new CRPocketBookGlobals(fileName);
    char manual_file[512] = USERDATA"/share/c3/manual/cr3-manual-en.fb2";
    {
		const char *lang = pbGlobals->getLang();

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
            sprintf( manual_file, USERDATA"/share/cr3/manual/cr3-manual-%s.fb2", lang );
            if ( !LVFileExists( lString16(manual_file).c_str() ) )
                sprintf( manual_file, USERDATA2"/share/cr3/manual/cr3-manual-%s.fb2", lang );
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
    } else if ( strstr(fileName, ".doc")!=NULL ) {
        ini_fname = L"cr3-doc.ini";
        css_file_name = L"doc.css";
    } else if ( strstr(fileName, ".chm")!=NULL ) {
        ini_fname = L"cr3-chm.ini";
        css_file_name = L"chm.css";
    } else if ( strstr(fileName, ".pdb")!=NULL ) {
        ini_fname = L"cr3-txt.ini";
        css_file_name = L"txt.css";
    } else {
        ini_fname = L"cr3-fb2.ini";
        css_file_name = L"fb2.css";
    }
#endif

    lString16Collection fontDirs;
	fontDirs.add(lString16(L""USERFONTDIR));
	fontDirs.add(lString16(L""SYSTEMFONTDIR));
    CRLog::info("INIT...");
    if (!InitCREngine(exename, fontDirs))
        return 0;

    {
		OpenScreen();
        CRLog::trace("creating window manager...");
        CRPocketBookWindowManager * wm = new CRPocketBookWindowManager(ScreenWidth(), ScreenHeight());

        const char * keymap_locations [] = {
			CONFIGPATH"/cr3/keymaps",
            USERDATA"/share/cr3/keymaps",
            USERDATA2"/share/cr3/keymaps",
            NULL,
        };

        loadKeymaps(*wm, keymap_locations);
        loadPocketBookKeyMaps(*wm);
        HyphMan::initDictionaries(lString16(L""USERDATA"/share/cr3/hyph/"));
        if (!wm->loadSkin(lString16(L""CONFIGPATH"/cr3/skin")))
			if (!wm->loadSkin(lString16(L""USERDATA2"/share/cr3/skin")))
				wm->loadSkin(lString16(L""USERDATA"/share/cr3/skin"));

		ldomDocCache::init(lString16(L""STATEPATH"/cr3/.cache"), PB_CR3_CACHE_SIZE);
		if (!ldomDocCache::enabled())
			ldomDocCache::init(lString16(L""USERDATA2"/share/cr3/.cache"), PB_CR3_CACHE_SIZE);
		if (!ldomDocCache::enabled())
			ldomDocCache::init(lString16(L""USERDATA"/share/cr3/.cache"), PB_CR3_CACHE_SIZE);
        CRLog::trace("creating main window...");
        main_win = new CRPocketBookDocView(wm, lString16(L""USERDATA"/share/cr3"));
        CRLog::trace("setting colors...");
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if (manual_file[0])
            main_win->setHelpFile( lString16( manual_file ) );
        if (!main_win->loadDefaultCover(lString16(L""CONFIGPATH"/cr3/cr3_def_cover.png")))
			if (!main_win->loadDefaultCover(lString16(L""USERDATA2"/share/cr3/cr3_def_cover.png")))
				main_win->loadDefaultCover(lString16(L""USERDATA"/share/cr3/cr3_def_cover.png"));
		if ( !main_win->loadCSS(lString16(L""CONFIGPATH"/cr3/")   + lString16(css_file_name) ) )
			if ( !main_win->loadCSS(  lString16( L""USERDATA"/share/cr3/" ) + lString16(css_file_name) ) )
                main_win->loadCSS( lString16( L""USERDATA2"/share/cr3/" ) + lString16(css_file_name) );
        main_win->setBookmarkDir(lString16(L""STATEPATH"/cr3/bookmarks/"));
        CRLog::trace("choosing init file...");
        static const lChar16 * dirs[] = {
			L""CONFIGPATH"/cr3/",
            L""USERDATA2"/share/cr3/",
            L""USERDATA"/share/cr3/",
            L""CONFIGPATH"/cr3/",
            NULL
        };
        CRLog::debug("Loading settings...");
        lString16 ini;
        for (int i = 0; dirs[i]; i++ ) {
            ini = lString16(dirs[i]) + ini_fname;
            if ( main_win->loadSettings( ini ) ) {
                break;
            }
        }
        CRLog::debug("settings at %s", UnicodeToUtf8(ini).c_str() );

		int orient;

		if (GetGlobalOrientation() == -1 || pbGlobals->getKeepOrientation() == 0 || pbGlobals->getKeepOrientation() == 2) {
			orient = GetOrientation();
		} else {
			orient = main_win->getProps()->getIntDef(PROP_POCKETBOOK_ORIENTATION,
				pocketbook_orientations[GetOrientation()]);
			SetOrientation(cr_oriantations[orient]);
		}
		if ( !main_win->loadHistory(lString16(L""STATEPATH"/cr3/.cr3hist")) ) 
			CRLog::error("Cannot read history file");
        LVDocView * _docview = main_win->getDocView();
        _docview->setBatteryState(GetBatteryPower());
        wm->activateWindow( main_win );
        wm->restoreOrientation(orient);
        if ( !main_win->loadDocument( lString16(fileName) ) ) {
            printf("Cannot open book file %s\n", fileName);
            delete wm;
            return 0;
        }
    }
    return 1;
}

static void onAutoRotation(int par1)
{
	CRLog::trace("onAutoRotation(%d)", par1);
	if (par1 < 0 || par1 > 3)
		return;
	cr_rotate_angle_t oldOrientation = CRPocketBookWindowManager::instance->getScreenOrientation();
	cr_rotate_angle_t newOrientation = pocketbook_orientations[par1];
	CRLog::trace("onAutoRotation(), oldOrient = %d, newOrient = %d", oldOrientation, newOrientation);
	if (oldOrientation != newOrientation &&
		(oldOrientation & 1) == (newOrientation & 1)) {
		CRPocketBookWindowManager *_wm = CRPocketBookWindowManager::instance;
		SetOrientation(par1);
		_wm->reconfigure(_wm->getScreen()->getWidth(), _wm->getScreen()->getHeight(), newOrientation);
		_wm->update(true);
	}
}

const char * getEventName(int evt) 
{
	static char buffer[64];

	switch(evt) {
	case EVT_INIT:
		return "EVT_INIT";
	case EVT_EXIT:
		return "EVT_EXIT";
	case EVT_SHOW:
		return "EVT_SHOW";
	case EVT_HIDE:
		return "EVT_HIDE";
	case EVT_KEYPRESS:
		return "EVT_KEYPRESS";
	case EVT_KEYRELEASE:
		return "EVT_KEYRELEASE";
	case EVT_KEYREPEAT:
		return "EVT_KEYREPEAT";
	case EVT_POINTERUP:
		return "EVT_POINTERUP";
	case EVT_POINTERDOWN:
		return "EVT_POINTERDOWN";
	case EVT_POINTERMOVE:
		return "EVT_POINTERMOVE";
	case EVT_POINTERLONG:
		return "EVT_POINTERLONG";
	case EVT_POINTERHOLD:
		return "EVT_POINTERHOLD";
	case EVT_ORIENTATION:
		return "EVT_ORIENTATION";
	case EVT_SNAPSHOT:
		return "EVT_SNAPSHOT";
	case EVT_MP_STATECHANGED:
		return "EVT_MP_STATECHANGED";
	case EVT_MP_TRACKCHANGED:
		return "EVT_MP_TRACKCHANGED";
	case EVT_PREVPAGE:
		return "EVT_PREVPAGE";
	case EVT_NEXTPAGE:
		return "EVT_NEXTPAGE";
	case EVT_OPENDIC:
		return "EVT_OPENDIC";
	default:
		sprintf(buffer, "%d", evt);
		return buffer;
	}
	return "";
}

int main_handler(int type, int par1, int par2)
{
	bool process_events = false;
	bool needUpdate = false;
	int ret = 0;
	CRLog::trace("main_handler(%s, %d, %d)", getEventName(type), par1, par2);
	switch (type) {
	case EVT_SHOW:
		CRPocketBookWindowManager::instance->update(true);
		pbGlobals->BookReady();
		break;
	case EVT_EXIT:
		exiting = true;
		if (CRPocketBookWindowManager::instance->getWindowCount() != 0) 
			CRPocketBookWindowManager::instance->closeAllWindows();
		ShutdownCREngine();
		break;
	case EVT_PREVPAGE:
		CRPocketBookWindowManager::instance->onCommand(DCMD_PAGEUP, 0);
		break;
	case EVT_NEXTPAGE:
		CRPocketBookWindowManager::instance->onCommand(DCMD_PAGEDOWN, 0);
		break;
	case EVT_ORIENTATION:
		onAutoRotation(par1);
		break;
	case EVT_KEYPRESS:
		if (par1 == KEY_POWER) {
			return 0;
		}
		if (!CRPocketBookWindowManager::instance->hasKeyMapping(par1, KEY_FLAG_LONG_PRESS)) {
			needUpdate = CRPocketBookWindowManager::instance->onKeyPressed(par1, 0);
			process_events = true;
			keyPressed = par1;
		} else
			keyPressed = -1;
		ret = 1;
		break;
	case EVT_KEYREPEAT:
	case EVT_KEYRELEASE:
		if (par1 == KEY_POWER) {
			return 0;
		}
		if (keyPressed == par1) {
			keyPressed = -1;
			break;
		}
		if (type == EVT_KEYRELEASE && par2 == 0) {
			needUpdate = CRPocketBookWindowManager::instance->onKeyPressed(par1, 0);
		} else if (type == EVT_KEYREPEAT && par2 > 0)
			needUpdate = CRPocketBookWindowManager::instance->onKeyPressed(par1, KEY_FLAG_LONG_PRESS);
		process_events = true;
		keyPressed = -1;
		ret = 1;
		break;
	case EVT_SNAPSHOT:
		CRPocketBookScreen::instance->MakeSnapShot();
		break;
	default:
		break;
	}
	if (process_events)
		needUpdate = CRPocketBookWindowManager::instance->processPostedEvents() || needUpdate;
	if (needUpdate)
		CRPocketBookWindowManager::instance->update(false);
	return ret;
}

const char* TR(const char *label) 
{
	char* tr = GetLangText(const_cast<char*> (label));
	CRLog::trace("Translation for %s is %s", label, tr);
	return tr;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		Message(ICON_WARNING,  const_cast<char*>("CoolReader"), const_cast<char*>("@Cant_open_file"), 2000);
		return 1;
	}
    if (!InitDoc(argv[0], argv[1])) {
		Message(ICON_WARNING,  const_cast<char*>("CoolReader"), const_cast<char*>("@Cant_open_file"), 2000);
        return 2;
    }
    InkViewMain(main_handler);
    return 0;
}
