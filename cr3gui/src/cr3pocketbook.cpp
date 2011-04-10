/*
 *  CR3 for PocketBook, port by pkb
 */

#include <crengine.h>
#include <crgui.h>
#include "cr3main.h"
#include "numedit.h"
#include "linksdlg.h"
#include "bmkdlg.h"
#include "mainwnd.h"
#include "selnavig.h"
#include <cr3version.h>
#include "cr3pocketbook.h"
#include <inkview.h>


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

class CRPocketBookGlobals
{
private :
	char *_fileName;
	int _keepOrientation;
	lString8  _lang;
	bool _ready_sent;
	char *_dataFile;
	char *_zetFile;
	tdocstate _docstate;
public:
    CRPocketBookGlobals(char *fileName);
	char *getFileName() { return _fileName ; }
	int getKeepOrientation() { return _keepOrientation; }
	const char *getLang() { return _lang.c_str(); }
	void saveState(int cpage, int npages);

	virtual ~CRPocketBookGlobals() { };
	void BookReady() 
	{
		if (!_ready_sent) {
			::BookReady(_fileName);
			_ready_sent = true;
		}
	}
};

CRPocketBookGlobals::CRPocketBookGlobals(char *fileName)
{
	_fileName = fileName;
	_ready_sent = false;
	iconfig *gc = OpenConfig(GLOBALCONFIGFILE, NULL);
	_lang = ReadString(gc, const_cast<char *>("language"), const_cast<char *>("en"));
	CRLog::trace("language=%s", _lang.c_str());
	if (_lang == "ua")
		_lang = "uk";
	_keepOrientation = ReadInt(gc, const_cast<char *>("keeporient"), 0);
	CloseConfig(gc);
	_dataFile = GetAssociatedFile(fileName, 0);
	_zetFile = GetAssociatedFile(fileName, 'z');
	FILE *f = iv_fopen(_dataFile, "rb");
	if (f == NULL || iv_fread(&_docstate, 1, sizeof(tdocstate), f) != sizeof(tdocstate) || _docstate.magic != 0x9751) {
		_docstate.magic = 0x9751;
		_docstate.position = 1;
		strcpy(_docstate.encoding, "auto");
		_docstate.nbmk = 0;
	} else
		_dataFile = NULL;
	if (f != NULL)
        iv_fclose(f);
}

void CRPocketBookGlobals::saveState(int cpage, int npages)
{
	CRLog::trace("CRPocketBookGlobals::saveState(%d, %d)", cpage, npages);
	if (_dataFile != NULL) {
		FILE *f = iv_fopen(_dataFile, "wb");
		if (f != NULL) {
			iv_fwrite(&_docstate, 1, sizeof(tdocstate), f);
			iv_fclose(f);
		}
	}
	if (npages - cpage < 3 && cpage >= 5) {
		FILE *f = iv_fopen(_zetFile, "w");
		fclose(f);
	}
}

class CRPocketBookGlobals * pbGlobals = NULL;

static int keyPressed = -1;
static bool exiting = false;


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
	:  CRGUIScreenBase( width, height, true )
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
	{ "@KA_nnot", -1, 0},
	{ "@KA_savp", -1, 0},
	{ "@KA_onot", -1, 0},
	{ "@KA_olnk", MCMD_GO_LINK, 0},
	{ "@KA_blnk", DCMD_LINK_BACK , 0},
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
		cr_rotate_angle_t newOrientation = pocketbook_orientations[storedOrientation];
		if (_orientation != newOrientation)
			reconfigure(ScreenWidth(), ScreenHeight(), newOrientation);
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

	bool rotateApply(int params) 
	{
		int orient = GetOrientation();
		if (orient == params)
			return true;
		if (params == -1 || pbGlobals->getKeepOrientation() == 0 || pbGlobals->getKeepOrientation() == 2) {
			SetGlobalOrientation(params);
		} else {
			SetOrientation(params);
			_props->setInt(PROP_POCKETBOOK_ORIENTATION, params);
	        saveSettings( lString16() );
		}
		cr_rotate_angle_t oldOrientation = pocketbook_orientations[orient];
		cr_rotate_angle_t newOrientation = pocketbook_orientations[params];
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
    : V3DocViewWin( wm, dataDir ), _tocLength(0), _toc(NULL), _bm3x3(NULL)
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
		case PB_CMD_CONTENTS: 
			showContents();
			return true;
        case MCMD_GO_LINK:
            showLinksDialog();
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


int InitDoc(const char *exename, char *fileName)
{
    static const lChar16 * css_file_name = L"fb2.css"; // fb2
	bool profileUsed = false;
	
    CRLog::trace("InitDoc()");

	lString16 dataDir(L""USERDATA);
	if (LVDirectoryExists(lString16(L"" USERDATA "/profiles"))) {
#ifndef __i386__
		char *currProfile = GetCurrentProfile();
		if (currProfile && currProfile[0]) {
			profileUsed = true;
			int profileType = GetProfileType(currProfile);
			if (profileType == PF_SDCARD)
				dataDir = L""USERPROFILES2 + lString16("/") + lString16(currProfile);
			else if (profileType == PF_LOCAL)
				dataDir = L""USERPROFILES + lString16("/") + lString16(currProfile);
			else 
				CRLog::error("InitDoc() : unknown profile type - %d", profileType);
		}
#endif
	}
#ifdef __i386__
    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);
#else
    InitCREngineLog(USERDATA"/share/cr3/crlog.ini");
#endif
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
    if (!InitCREngine(exename, fontDirs))
        return 0;

    {
		OpenScreen();
        CRLog::trace("creating window manager...");
        CRPocketBookWindowManager * wm = new CRPocketBookWindowManager(ScreenWidth(), ScreenHeight());

		lString8 cfgKeymapDir = UnicodeToUtf8(dataDir + lString16("/config/cr3/keymaps"));
        const char * keymap_locations [] = {
			cfgKeymapDir.c_str(),
            USERDATA"/share/cr3/keymaps",
            USERDATA2"/share/cr3/keymaps",
            NULL,
        };

        loadKeymaps(*wm, keymap_locations);
        loadPocketBookKeyMaps(*wm);
        HyphMan::initDictionaries(lString16(L""USERDATA"/share/cr3/hyph/"));
        if (!wm->loadSkin(dataDir + lString16("/config/cr3/skin")))
			if (!wm->loadSkin(lString16(L""USERDATA2"/share/cr3/skin")))
				wm->loadSkin(lString16(L""USERDATA"/share/cr3/skin"));
		if (profileUsed)
			ldomDocCache::init(dataDir + lString16("/state/cr3/.cache"), PB_CR3_CACHE_SIZE);
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
        if (!main_win->loadDefaultCover(dataDir + lString16("/config/cr3/cr3_def_cover.png")))
			if (!main_win->loadDefaultCover(lString16(L""USERDATA2"/share/cr3/cr3_def_cover.png")))
				main_win->loadDefaultCover(lString16(L""USERDATA"/share/cr3/cr3_def_cover.png"));
		if ( !main_win->loadCSS(dataDir + lString16("/config/cr3/")   + lString16(css_file_name) ) )
			if ( !main_win->loadCSS(  lString16( L""USERDATA"/share/cr3/" ) + lString16(css_file_name) ) )
                main_win->loadCSS( lString16( L""USERDATA2"/share/cr3/" ) + lString16(css_file_name) );
        main_win->setBookmarkDir(dataDir + lString16("/state/cr3/bookmarks/"));
        CRLog::trace("choosing init file...");
        lString16 iniCfgDir = dataDir + lString16("/config/cr3/");
        static const lChar16 * dirs[] = {
			iniCfgDir.c_str(),
            L""USERDATA2"/share/cr3/",
            L""USERDATA"/share/cr3/",
            iniCfgDir.c_str(),
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
			orient = main_win->getProps()->getIntDef(PROP_POCKETBOOK_ORIENTATION, GetOrientation());
			SetOrientation(orient);
		}
        wm->restoreOrientation(orient);

		if ( !main_win->loadHistory(dataDir + lString16("/state/cr3/.cr3hist")) ) 
			CRLog::error("Cannot read history file");
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

static void onAutoRotation(int par1)
{
	if (par1 < 0 || par1 > 3)
		return;
	CRPocketBookWindowManager::instance->onCommand(PB_CMD_ROTATE_ANGLE_SET, par1);
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
		// TODO: find out why don't handler receive EVT_EXIT and EVT_SNAPSHOT
		// on power off ?
		exiting = true;
		if (CRPocketBookWindowManager::instance->getWindowCount() != 0)
			CRPocketBookWindowManager::instance->closeAllWindows();
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
			ret = 1;
		} else
			keyPressed = -1;
		break;
	case EVT_KEYREPEAT:
	case EVT_KEYRELEASE:
		if (par1 == KEY_POWER) {
			if (par2 > 3 && !exiting) {
				//TODO: remove this (experiment with receiving events
				CRPocketBookScreen::instance->MakeSnapShot();
				SendEvent(main_handler, EVT_EXIT, 0, 0);
				return 1;
			}
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
    ShutdownCREngine();
    return 0;
}
