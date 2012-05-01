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

#define PB_CR3_TRANSLATE_DELAY 1000

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

// The code dealing with G-sensor in order to support wrist page turn was taken from
// fbreadr180 (fbreader modification by SciFiFan, JAW, GrayNM, Antuanelli,
// Version 0.11.3
static const int angles_measured[] = { 19, 24, 29, 33, 38, 43, 47, 50, 52, 55, 57 };

static void translate_timer();
static void rotate_timer();
static void paused_rotate_timer();

class CRPocketBookGlobals
{
private :
    lString16 _fileName;
    int _keepOrientation;
    lString8  _lang;
    lString8  _pbDictionary;
    bool _ready_sent;
    bool createFile(char *fName);
    bool _translateTimer;
public:
    CRPocketBookGlobals(char *fileName);
    lString16 getFileName() { return _fileName ; }
    void setFileName( lString16 fn) { _fileName = fn; }
    int getKeepOrientation() { return _keepOrientation; }
    const char *getLang() { return _lang.c_str(); }
    const char *getDictionary() { return _pbDictionary.c_str(); }
    void saveState(int cpage, int npages);

    virtual ~CRPocketBookGlobals() { }

    void BookReady()
    {
        if (!_ready_sent) {
            ::BookReady((char *)UnicodeToLocal(_fileName).c_str());
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
    _fileName = lString16(fileName);
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
    char *af0 = GetAssociatedFile((char *)UnicodeToLocal(_fileName).c_str(), 0);

    if (createFile(af0)) {
        if (npages - cpage < 3 && cpage >= 5) {
            char *afz = GetAssociatedFile((char *)UnicodeToLocal(_fileName).c_str(), 'z');
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
    CRGUIWindowBase *m_mainWindow;
#if GRAY_BACKBUFFER_BITS == 4
    lUInt8 *_buf4bpp;
#endif
public:
    static CRPocketBookScreen * instance;
protected:
    virtual void draw(int x, int y, int w, int h);
    virtual void update( const lvRect & rc2, bool full );
public:
    virtual ~CRPocketBookScreen()
    {
        instance = NULL;
#if GRAY_BACKBUFFER_BITS == 4
        delete [] _buf4bpp;
#endif
    }

    CRPocketBookScreen( int width, int height )
        :  CRGUIScreenBase( width, height, true ), _forceSoft(false)
    {
        instance = this;
#if GRAY_BACKBUFFER_BITS == 4
        if (width > height)
            _buf4bpp = new lUInt8[ (width + 1)/2 * width ];
        else
            _buf4bpp = new lUInt8[ (height + 1)/2 * height ];
#endif
    }

    void MakeSnapShot()
    {
        ClearScreen();
        draw(0, 0, _front->GetWidth(), _front->GetHeight());
        PageSnapshot();
    }
    bool setForceSoftUpdate(bool force)
    {
        bool ret = _forceSoft;
        _forceSoft = force;
        return ret;
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
    { "@KA_pr10", PB_CMD_PAGEUP_REPEAT, 10},
    { "@KA_nx10", PB_CMD_PAGEDOWN_REPEAT, 10},
    { "@KA_goto", MCMD_GO_PAGE, 0},
    { "@KA_frst", DCMD_BEGIN, 0},
    { "@KA_last", DCMD_END, 0},
    { "@KA_prse", DCMD_MOVE_BY_CHAPTER, -1},
    { "@KA_nxse", DCMD_MOVE_BY_CHAPTER, 1},
    { "@KA_obmk", MCMD_BOOKMARK_LIST_GO_MODE, 0},
    { "@KA_nbmk", MCMD_BOOKMARK_LIST, 0},
    { "@KA_nnot", MCMD_CITE, 0},
    { "@KA_savp", GCMD_PASS_TO_PARENT, 0},
    { "@KA_onot", MCMD_CITES_LIST, 0},
    { "@KA_olnk", MCMD_GO_LINK, 0},
    { "@KA_blnk", DCMD_LINK_BACK , 0},
    { "@KA_cnts", PB_CMD_CONTENTS, 0},
    { "@KA_srch", MCMD_SEARCH, 0},
    { "@KA_dict", MCMD_DICT, 0},
    { "@KA_zmin", DCMD_ZOOM_IN, 0},
    { "@KA_zout", DCMD_ZOOM_OUT, 0},
    { "@KA_hidp", GCMD_PASS_TO_PARENT, 0},
    { "@KA_rtte", PB_CMD_ROTATE, 0},
    { "@KA_mmnu", PB_CMD_MAIN_MENU, 0},
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
        for (unsigned i = 0; i < sizeof(pbActions)/sizeof(pbActions[0]); i++) {
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
            _screen->getCanvas()->Draw(img, 0, 0, _screen->getWidth(), _screen->getHeight());
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
        return true;
    }

    int hasKeyMapping(int key, int flags) {
        // NOTE: orientation related key substitution is performed by inkview
        for ( int i=_windows.length()-1; i>=0; i-- ) {
            if ( _windows[i]->isVisible() ) {
                int cmd, param;
                CRGUIAcceleratorTableRef accTable = _windows[i]->getAccelerators();

                if (!accTable.isNull() && accTable->translate( key, flags, cmd, param ) ) {
                    if (cmd != GCMD_PASS_TO_PARENT )
                        return cmd;
                }
            }
        }
        return -1;
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

void executeCommand(int commandId, int commandParam)
{
    CRPocketBookWindowManager::instance->onCommand(commandId, commandParam);
    CRPocketBookWindowManager::instance->processPostedEvents();
}

void quickMenuHandler(int choice) 
{
    executeCommand(PB_QUICK_MENU_SELECT, choice);
}

void rotateHandler(int angle) 
{
    executeCommand(PB_CMD_ROTATE_ANGLE_SET, angle);
}

void pageSelector(int page) {
    executeCommand(MCMD_GO_PAGE_APPLY, page);
}

void searchHandler(char *s)
{
    if (s  && *s)
        executeCommand(MCMD_SEARCH_FINDFIRST, 1);
    else
        executeCommand(GCMD_PASS_TO_PARENT, 0);
} 

void tocHandler(long long position) 
{
    executeCommand(MCMD_GO_PAGE_APPLY, position);
}

int main_handler(int type, int par1, int par2);

void CRPocketBookScreen::draw(int x, int y, int w, int h)
{
#if (GRAY_BACKBUFFER_BITS == 4)
    lUInt8 * line = _front->GetScanLine(y);
    lUInt8 *dest = _buf4bpp;
    int limit = x + w -1;
    int scanline = (w + 1)/2;

    for (int yy = 0; yy < h; yy++) {
        int sx = x;
        for (int xx = 0; xx < scanline; xx++) {
            dest[xx] = line[sx++] & 0xF0;
            if (sx < limit)
                dest[xx] |= (line[sx++] >> 4);
        }
        line += _front->GetRowSize();
        dest += scanline;
    }
    Stretch(_buf4bpp, IMAGE_GRAY4, w, h, scanline, x, y, w, h, 0);
#else
    Stretch(_front->GetScanLine(y), PB_BUFFER_GRAYS, w, h, _front->GetRowSize(), x, y, w, h, 0);
#endif
}

void CRPocketBookScreen::update( const lvRect & rc2, bool full )
{
    if (rc2.isEmpty() && !full)
        return;
    bool isDocWnd = (main_win == CRPocketBookWindowManager::instance->getTopVisibleWindow());
    lvRect rc = rc2;
    rc.left &= ~3;
    rc.right = (rc.right + 3) & ~3;


    if (!_forceSoft && ( isDocWnd || rc.height() > 400)
#if ENABLE_UPDATE_MODE_SETTING==1
        && checkFullUpdateCounter()
#endif
        )
        full = true;
    else if (!_forceSoft)
        full = false;

    if ( full ) {
        draw(0, 0, _front->GetWidth(), _front->GetHeight());
        FullUpdate();
    } else {
        draw(0, rc.top, _front->GetWidth(), rc.height());
        if (!isDocWnd && rc.height() < 300)
            PartialUpdateBW(rc.left, rc.top, rc.right, rc.bottom);
        else
            SoftUpdate();
    }
}

class CRPocketBookInkViewWindow : public CRGUIWindowBase
{
protected:
    virtual void draw() 
    {
        /*
         *	iv_handler handler = GetEventHandler();
         *	if (handler != main_handler)
         *		SendEvent(handler, EVT_REPAINT, 0, 0);
         */
        CRLog::trace("CRPocketBookInkViewWindow::draw()");
    }
    virtual void showWindow() = 0;
public:
    CRPocketBookInkViewWindow( CRGUIWindowManager * wm )
        : CRGUIWindowBase( wm )	{ }
    virtual bool onCommand( int command, int params = 0 )
    {
        CRLog::trace("CRPocketBookInkViewWindow::onCommand(%d, %d)", command, params);
        switch(command) {
        case PB_QUICK_MENU_SELECT:
        case PB_CMD_ROTATE_ANGLE_SET:
        case MCMD_GO_PAGE_APPLY:
        case MCMD_SEARCH_FINDFIRST:
        case GCMD_PASS_TO_PARENT:
            _wm->postCommand(command, params);
            _wm->closeWindow( this );
            return true;
        default:
            CRLog::trace("CRPocketBookInkViewWindow::onCommand() - unhandled");
        }
        return true;
    }
    virtual bool onKeyPressed( int key, int flags )
    {
        _wm->postEvent( new CRGUIKeyDownEvent(key, flags) );
        _wm->closeWindow( this );
        return true;
    }
    virtual ~CRPocketBookInkViewWindow()
    {
        CRLog::trace("~CRPocketBookInkViewWindow()");
    }
    virtual void activated()
    {
        showWindow();
    }
};

class CRPocketBookPageSelectorWindow : public CRPocketBookInkViewWindow
{
public:
    CRPocketBookPageSelectorWindow( CRGUIWindowManager * wm )
        : CRPocketBookInkViewWindow( wm )	{}
    virtual void showWindow()
    {
        OpenPageSelector(pageSelector);
    }
};

class CRPocketBookQuickMenuWindow : public CRPocketBookInkViewWindow
{
private:
    ibitmap *_menuBitmap;
    const char **_strings;
public:
    CRPocketBookQuickMenuWindow( CRGUIWindowManager * wm, ibitmap *menu_bitmap, const char **strings)
        : CRPocketBookInkViewWindow( wm ), _menuBitmap(menu_bitmap), _strings(strings)	{}
    virtual void showWindow()
    {
        OpenMenu3x3(_menuBitmap, _strings, quickMenuHandler);
    }
};

class CRPocketBookRotateWindow : public CRPocketBookInkViewWindow
{
public:
    CRPocketBookRotateWindow( CRGUIWindowManager * wm)
        : CRPocketBookInkViewWindow( wm )	{}
    virtual void showWindow()
    {
        OpenRotateBox(rotateHandler);
    }
};

class CRPocketBookSearchWindow : public CRPocketBookInkViewWindow
{
public:
    CRPocketBookSearchWindow( CRGUIWindowManager * wm)
        : CRPocketBookInkViewWindow( wm )	{}
    virtual void showWindow()
    {
        OpenKeyboard(const_cast<char *>("@Search"), key_buffer, KEY_BUFFER_LEN, 0, searchHandler);
    }
};

class CRPocketBookContentsWindow : public CRPocketBookInkViewWindow
{
private:
    int _curPage;
    tocentry *_toc;
    int _tocLength;
public:
    CRPocketBookContentsWindow( CRGUIWindowManager * wm, tocentry *toc, int toc_length, int cur_page)
        : CRPocketBookInkViewWindow( wm ), _curPage(cur_page), _toc(toc), _tocLength(toc_length) {}
    virtual void showWindow()
    {
        OpenContents(_toc, _tocLength, _curPage, tocHandler);
    }
};

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
    bool _dictsLoaded;
    int _selectedIndex;
    int _itemsCount;
    LVImageSourceRef _toolBarImg;
    int _translateResult;
    char *_newWord;
    char *_newTranslation;
private:
    void searchDictinary();
    void loadDictionaries();
protected:
    virtual void selectDictionary();
    virtual void onDictionarySelect();
    virtual bool onItemSelect();
public:
    CRPbDictionaryView(CRGUIWindowManager * wm, CRPbDictionaryDialog *parent);
    virtual ~CRPbDictionaryView();
    virtual void draw();
    virtual void translate(const lString16 &w);
    virtual lString8 createArticle(const char *word, const char *translation);
    virtual bool onCommand( int command, int params );
    void doActivate();
    int getCurItem() { return _selectedIndex; }
    void setTranslation(lString8 translation);
    void setCurItem(int index);
    virtual void drawTitleBar();
    virtual void Update();
    bool isArticleListActive();
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
    virtual void clearSelection() { _word.clear(); }
    void noTranslation();
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

class CRPbDictionaryProxyWindow;

class CRPbDictionaryDialog : public CRGUIWindowBase
{
    friend class CRPbDictionaryProxyWindow;
protected:
    CRViewDialog * _docwin;
    LVDocView * _docview;
    LVPageWordSelector * _wordSelector;
    CRPbDictionaryView * _dictView;
    bool _dictViewActive;
    bool _autoTranslate;
    bool _wordTranslated;
    lString16 _selText;
    lString8 _prompt;
protected:
    virtual void draw();
    void endWordSelection();
    bool isWordSelection() { return _wordSelector!=NULL; }
    void onWordSelection();
    bool _docDirty;
    int _curPage;
    int _lastWordX;
    int _lastWordY;
public:
    CRPbDictionaryDialog( CRGUIWindowManager * wm, CRViewDialog * docwin,  lString8 css);
    virtual ~CRPbDictionaryDialog() {
        if (_wordSelector) {
            delete _wordSelector;
            _wordSelector = NULL;
        }
        delete _dictView;
        _dictView = NULL;
    }
    /// returns true if command is processed
    virtual bool onCommand( int command, int params );
    virtual void activateDictView(bool active)
    {
        if (_dictViewActive != active) {
            _dictViewActive = active;
            if (!active && isWordSelection())
                _wordSelector->moveBy(DIR_ANY, 0);
            setDocDirty();
            Update();
        }
    }
    virtual void startWordSelection();
    virtual void Update();
    virtual bool isDocDirty() { return _docDirty; }
    virtual void setDocDirty() { _docDirty = true; }
    virtual void reconfigure( int flags )
    {
        CRGUIWindowBase::reconfigure(flags);
        _dictView->reconfigure(flags);
    }
    virtual bool isSelectingWord()
    {
        return (!_autoTranslate && !_wordTranslated);
    }
};

class CRPbDictionaryProxyWindow : public CRPbDictionaryDialog
{
private:
    CRPbDictionaryDialog *_dictDlg;
protected:
    virtual void draw() { _dictDlg->draw(); }
public:
    CRPbDictionaryProxyWindow(CRPbDictionaryDialog * dictDialog)
        : CRPbDictionaryDialog(dictDialog->getWindowManager(), dictDialog->_docwin, lString8::empty_str), _dictDlg(dictDialog)
    {
        _dictDlg->_wordTranslated = _dictDlg->_dictViewActive = false;
        _dictDlg->_selText.clear();
        CRPocketBookScreen::instance->setForceSoftUpdate(true);
        lvRect rect = _wm->getScreen()->getRect();
        _dictDlg->setRect(rect);
        rect.top = rect.bottom - _dictDlg->_dictView->getDesiredHeight();
        _dictDlg->_dictView->setRect(rect);
        _dictDlg->_dictView->reconfigure(0);
    }
    virtual ~CRPbDictionaryProxyWindow()
    {
        CRPocketBookScreen::instance->setForceSoftUpdate(false);
    }
    virtual void activateDictView(bool active)
    {
        _dictDlg->activateDictView(active);
    }
    virtual bool onCommand( int command, int params )
    {
        if (command == MCMD_CANCEL) {
            _dictDlg->_docview->clearSelection();
            _wm->closeWindow( this );
            return true;
        }
        return _dictDlg->onCommand(command, params);
    }
    virtual void startWordSelection()
    {
        _dictDlg->setDocDirty();
        _dictDlg->startWordSelection();
    }
    virtual void Update()
    {
        _dictDlg->Update();
    }
    virtual bool isDocDirty()
    {
        return _dictDlg->isDocDirty();
    }
    virtual void setDocDirty()
    {
        _dictDlg->setDocDirty();
    }
    virtual void reconfigure( int flags )
    {
        _dictDlg->reconfigure(flags);
    }
    virtual bool isSelectingWord()
    {
        return _dictDlg->isSelectingWord();
    }
    virtual void setDirty()
    {
        _dictDlg->setDirty();
    }
    virtual bool isDirty()
    {
        return _dictDlg->isDirty();
    }
    virtual void setVisible( bool visible )
    {
        _dictDlg->setVisible(visible);
    }
    virtual bool isVisible() const
    {
        return _dictDlg->isVisible();
    }
    virtual const lvRect & getRect()
    {
        return _dictDlg->getRect();
    }
    virtual void activated()
    {
        _dictDlg->activated();
    }
};

class CRPocketBookDocView : public V3DocViewWin {
private:
    ibitmap *_bm3x3;
    char *_strings3x3[9];
    int _quick_menuactions[9];
    tocentry *_toc;
    int _tocLength;
    CRPbDictionaryDialog * _dictDlg;
    bool _rotatetimerset;
    bool _lastturn;
    int _pausedRotation;
    bool _pauseRotationTimer;
    int  m_goToPage;
    bool _restore_globOrientation;
    bool m_skipEvent;
    void freeContents()
    {
        for (int i = 0; i < _tocLength; i++) {
            if (_toc[i].text)
                free(_toc[i].text);
        }
        free(_toc);
        _toc = NULL;
    }
    void switchToRecentBook(int index)
    {
        LVPtrVector<CRFileHistRecord> & files = _docview->getHistory()->getRecords();
        if ( index >= 1 && index < files.length() ) {
            CRFileHistRecord * file = files.get( index );
            lString16 fn = file->getFilePathName();
            if ( LVFileExists(fn) ) {
                // Actually book opened in openRecentBook() we are in truble if it will fail
                pbGlobals->saveState(getDocView()->getCurPage(), getDocView()->getPageCount());
                pbGlobals->setFileName(file->getFilePathName());
            }
        }
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
            saveSettings(lString16::empty_str);
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

    void draw()
    {
        if (m_goToPage != -1) {
            CRRectSkinRef skin = _wm->getSkin()->getWindowSkin( L"#dialog" )->getClientSkin();
            LVDrawBuf * buf = _wm->getScreen()->getCanvas().get();
            lString16 text = lString16::itoa(m_goToPage + 1);
            lvPoint text_size = skin->measureText(text);
            lvRect rc;
            rc.left = _wm->getScreen()->getWidth() - 65;
            rc.top = _wm->getScreen()->getHeight() - text_size.y - 30;
            rc.right = rc.left + 60;
            rc.bottom = rc.top + text_size.y * 3/2;
            buf->FillRect(rc, _docview->getBackgroundColor());
            buf->Rect(rc, _docview->getTextColor());
            rc.shrink(1);
            buf->Rect(rc, _docview->getTextColor());
            skin->drawText(*buf, rc, text);
        } else
            V3DocViewWin::draw();
    }

    bool incrementPage(int delta)
    {
        if (m_goToPage == -1)
            m_goToPage = _docview->getCurPage();
        m_goToPage = m_goToPage + delta * _docview->getVisiblePageCount();
        bool res = true;
        int page_count = _docview->getPageCount();
        if (m_goToPage >= page_count) {
            m_goToPage = page_count - 1;
            res = false;
        }
        if (m_goToPage < 0) {
            m_goToPage = 0;
            res = false;
        }
        if (res)
            setDirty();
        return res;
    }

public:
    static CRPocketBookDocView * instance;
    CRPocketBookDocView( CRGUIWindowManager * wm, lString16 dataDir )
        : V3DocViewWin( wm, dataDir ), _bm3x3(NULL), _toc(NULL), _tocLength(0), _dictDlg(NULL), _rotatetimerset(false),
        _lastturn(true), _pauseRotationTimer(false), m_goToPage(-1), _restore_globOrientation(false), m_skipEvent(false)
    {
        instance = this;
    }

    virtual void closing()
    {
        pbGlobals->saveState(getDocView()->getCurPage(), getDocView()->getPageCount());
        CRLog::trace("V3DocViewWin::closing();");
        readingOff();
        if (_restore_globOrientation) {
            SetGlobalOrientation(-1);
            _restore_globOrientation = false;
        }
        V3DocViewWin::closing();
        if (!exiting)
            CloseApp();
    }

    bool onCommand(int command, int params)
    {
        switch(command) {
        case PB_CMD_MAIN_MENU:
            OpenMainMenu();
            return true;
        case PB_CMD_UPDATE_WINDOW:
            {
                bool save = CRPocketBookScreen::instance->setForceSoftUpdate(true);
                _wm->update(true, true);
                CRPocketBookScreen::instance->setForceSoftUpdate(save);
                return true;
            }
        case PB_QUICK_MENU:
            {
                CRPocketBookQuickMenuWindow *wnd = new CRPocketBookQuickMenuWindow(_wm,
                                                                                   getQuickMenuBitmap(), (const char **)_strings3x3);
                _wm->activateWindow(wnd);
            }
            return true;
        case PB_CMD_ROTATE:
            {
                CRPocketBookRotateWindow *wnd = new CRPocketBookRotateWindow(_wm);
                _wm->activateWindow(wnd);
            }
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
            {
                _searchPattern.clear();
                CRPocketBookSearchWindow *wnd = new CRPocketBookSearchWindow(_wm);
                _wm->activateWindow(wnd);
            }
            return true;
        case MCMD_SEARCH_FINDFIRST:
            _searchPattern += Utf8ToUnicode(key_buffer);
            if ( !_searchPattern.empty() && params ) {
                int pageIndex = findPagesText( _searchPattern, 0, 1 );
                if (pageIndex == -1)
                    pageIndex = findPagesText( _searchPattern, -1, 1 );
                if ( pageIndex != -1 ) {
                    CRSelNavigationDialog * dlg = new CRSelNavigationDialog( _wm, this, _searchPattern );
                    _wm->activateWindow( dlg );
                } else
                    Message(ICON_INFORMATION, const_cast<char*>("@Search"), const_cast<char*>("@No_more_matches"), 2000);
            }
            _wm->update(false);
            return true;
        case MCMD_GO_PAGE:
            {
                CRPocketBookPageSelectorWindow *wnd = new CRPocketBookPageSelectorWindow(_wm);
                _wm->activateWindow(wnd);
            }
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
        case MCMD_OPEN_RECENT_BOOK:
            switchToRecentBook(params);
            break;
        case PB_CMD_PAGEUP_REPEAT:
            if (m_skipEvent) {
                m_skipEvent = false;
                return false;
            }
            m_skipEvent = true;
            if (params < 1)
                params = 1;
            return incrementPage(-params);
        case PB_CMD_PAGEDOWN_REPEAT:
            if (m_skipEvent) {
                m_skipEvent = false;
                return false;
            }
            if (params < 1)
                params = 1;
            m_skipEvent = true;
            return incrementPage(params);
        case PB_CMD_REPEAT_FINISH:
            if (m_goToPage != -1) {
                int page = m_goToPage;
                m_goToPage = -1;
                m_skipEvent = false;
                _docview->goToPage(page);
                return true;
            }
            break;
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
        if (_dictDlg == NULL) {
            lString16 filename("dict.css");
            lString8 dictCss;
            if (_cssDir.length() > 0 && LVFileExists( _cssDir + filename ))
                LVLoadStylesheetFile( _cssDir + filename, dictCss );
            _dictDlg = new CRPbDictionaryDialog( _wm, this, dictCss );
        }
        CRPbDictionaryProxyWindow *dlg = new CRPbDictionaryProxyWindow(_dictDlg);
        _wm->activateWindow( dlg );
        dlg->startWordSelection();
    }

    void showContents() {
        if (_toc != NULL)
            freeContents();
        LVPtrVector<LVTocItem, false> tocItems;
        _docview->getFlatToc(tocItems);
        _tocLength = tocItems.length();

        if (_tocLength) {
            int tocSize = (_tocLength + 1) * sizeof(tocentry);
            _toc = (tocentry *) malloc(tocSize);
            int j = 0;
            for (int i = 0; i < tocItems.length(); i++) {
                LVTocItem * item = tocItems[i];
                if (item->getName().empty())
                    continue;
                _toc[j].level = item->getLevel();
                _toc[j].position = item->getPage() + 1;
                _toc[j].page = _toc[j].position;
                _toc[j].text = strdup(UnicodeToUtf8(item->getName()).c_str());
                char *p = _toc[j++].text;
                while (*p) {
                    if (*p == '\r' || *p == '\n') *p = ' ';
                    p++;
                }
            }
            _tocLength = j;
            if (j == 0) {
                free(_toc);
                _toc = NULL;
            }
        }
        if (!_tocLength) {
            Message(ICON_INFORMATION, const_cast<char*>("CoolReader"),
                    const_cast<char*>("@No_contents"), 2000);
            return;
        }
        CRPocketBookContentsWindow *wnd = new CRPocketBookContentsWindow(_wm, _toc,
                                                                         _tocLength, _docview->getCurPage() + 1);
        _wm->activateWindow( wnd );
    }

    void readingOff()
    {
        CRLog::trace("readingOff()");
        if (_rotatetimerset) {
            ClearTimer(rotate_timer);
            _rotatetimerset = false;
        }
    }

    void readingOn()
    {
        CRLog::trace("readingOn()");
        CRPropRef props = CRPocketBookDocView::instance->getProps();
        int rotate_mode = props->getIntDef(PROP_POCKETBOOK_ROTATE_MODE, PB_ROTATE_MODE_180);
        if (rotate_mode > PB_ROTATE_MODE_180_SLOW_PREV_NEXT && !_rotatetimerset) {
            SetWeakTimer("RotatePage", rotate_timer, 200);
            _rotatetimerset = true;
        }
    }

    virtual void activated()
    {
        V3DocViewWin::activated();
        readingOn();
    }

    virtual void covered()
    {
        V3DocViewWin::covered();
        readingOff();
    }

    virtual void reactivated()
    {
        V3DocViewWin::reactivated();
        readingOn();
    }

    virtual ~CRPocketBookDocView()
    {
        instance = NULL;
        if (_dictDlg != NULL)
            delete _dictDlg;
    }

    CRPropRef getNewProps() {
        _props = _docview->propsGetCurrent() | _props;
        _newProps = LVClonePropsContainer( _props );
        return _newProps;
    }

    void onRotateTimer()
    {
        CRPropRef props = CRPocketBookDocView::instance->getProps();
        int rotate_mode = props->getIntDef(PROP_POCKETBOOK_ROTATE_MODE, PB_ROTATE_MODE_180);
        int rotate_angle = props->getIntDef(PROP_POCKETBOOK_ROTATE_ANGLE, 2);
        _rotatetimerset = false;
        int cn = GetOrientation();
        int x,y,z,rotatepercent;
        bool turn = false;
        ReadGSensor(&x,&y,&z);

        if (rotate_angle < 0)
            rotate_angle = 0;
        else if (rotate_angle > 10)
            rotate_angle = 10;
        rotatepercent = angles_measured[rotate_angle];
        switch (cn) {
        case ROTATE0:
        case ROTATE180:
            turn = (abs(x) > rotatepercent);
            break;
        case ROTATE90:
        case ROTATE270:
            turn = (abs(y) > rotatepercent);
            break;

        }
        CRLog::trace("rotatepercent = %d, x = %d", rotatepercent, x);
        if (_lastturn != turn) {   // only one page turn at a time!
            _lastturn = turn;
            if (turn) {
                if (rotate_mode >= PB_ROTATE_MODE_180_FAST_PREV_NEXT) {
                    switch (cn) {
                    case ROTATE0:
                        turn = x < 0;
                        break;
                    case ROTATE90:
                        turn = y < 0;
                        break;
                    case ROTATE270:
                        turn = y > 0;
                        break;
                    case ROTATE180:
                        turn = x > 0;
                        break;
                    }
                    if (rotate_mode == PB_ROTATE_MODE_180_FAST_NEXT_PREV)
                        turn = !turn;
                }
                if (turn)
                    SendEvent(main_handler, EVT_NEXTPAGE, 0, 0);
                else
                    SendEvent(main_handler, EVT_PREVPAGE, 0, 0);
                SetAutoPowerOff(0);
                SetAutoPowerOff(1);  // reset auto-power-off timer!
            }
        }
        if (rotate_mode > PB_ROTATE_MODE_180_SLOW_PREV_NEXT) {
            SetWeakTimer("RotatePage", rotate_timer, 200);
            _rotatetimerset = true;
        }
    }

    void onPausedRotation()
    {
        _pauseRotationTimer = false;
        int orient = GetOrientation();
        if (orient == _pausedRotation)
            return;
        SetOrientation(_pausedRotation);
        int dx = _wm->getScreen()->getWidth();
        int dy = _wm->getScreen()->getHeight();
        cr_rotate_angle_t oldOrientation = pocketbook_orientations[orient];
        cr_rotate_angle_t newOrientation = pocketbook_orientations[GetOrientation()];
        if ((oldOrientation & 1) == (newOrientation & 1))
            _wm->reconfigure(dx, dy, newOrientation);
        else {
            SetGlobalOrientation(_pausedRotation);
            _restore_globOrientation = true;
            _wm->reconfigure(dy, dx, newOrientation);
        }
        _wm->update(true);
    }

    void onAutoRotation(int par1)
    {
        CRLog::trace("onAutoRotation(%d)", par1);
        if (par1 < 0 || par1 > 3)
            return;
        if (_pauseRotationTimer) {
            _pauseRotationTimer = false;
            ClearTimer(paused_rotate_timer);
        }
        cr_rotate_angle_t oldOrientation = CRPocketBookWindowManager::instance->getScreenOrientation();
        cr_rotate_angle_t newOrientation = pocketbook_orientations[par1];
        CRLog::trace("onAutoRotation(), oldOrient = %d, newOrient = %d", oldOrientation, newOrientation);
        if (oldOrientation == newOrientation)
            return;
        CRPropRef props = CRPocketBookDocView::instance->getProps();
        int rotate_mode = props->getIntDef(PROP_POCKETBOOK_ROTATE_MODE, PB_ROTATE_MODE_180);
        if (rotate_mode && (oldOrientation & 1) != (newOrientation & 1)) {
            CRLog::trace("rotate_mode && (oldOrientation & 1) != (newOrientation & 1)");
            if (rotate_mode == PB_ROTATE_MODE_180 || rotate_mode > PB_ROTATE_MODE_180_SLOW_PREV_NEXT)
                return;
            bool prev = false;
            if (rotate_mode == PB_ROTATE_MODE_180_SLOW_PREV_NEXT) {    // back+forward
                switch (oldOrientation) {
                case CR_ROTATE_ANGLE_0:
                    prev = (newOrientation == CR_ROTATE_ANGLE_90);
                    break;
                case CR_ROTATE_ANGLE_90:
                    prev = (newOrientation == CR_ROTATE_ANGLE_180);
                    break;
                case CR_ROTATE_ANGLE_270:
                    prev = (newOrientation == CR_ROTATE_ANGLE_0);
                    break;
                case CR_ROTATE_ANGLE_180:
                    prev = (newOrientation == CR_ROTATE_ANGLE_270);
                    break;
                }
            }
            if (prev)
                SendEvent(main_handler, EVT_PREVPAGE, 0, 0);
            else
                SendEvent(main_handler, EVT_NEXTPAGE, 0, 0);
            SetAutoPowerOff(0);
            SetAutoPowerOff(1);  // reset auto-power-off timer!
        } else {
            int dx = _wm->getScreen()->getWidth();
            int dy = _wm->getScreen()->getHeight();
            if ((oldOrientation & 1) == (newOrientation & 1)) {
                SetOrientation(par1);
                _wm->reconfigure(dx, dy, newOrientation);
                _wm->update(true);
            } else {
                _pausedRotation = par1;
                SetWeakTimer("RotatePage", paused_rotate_timer, 400);
                _pauseRotationTimer = true;
            }
        }
    }
    void OnFormatEnd()
    {
        V3DocViewWin::OnFormatEnd();
        if (_restore_globOrientation) {
            SetGlobalOrientation(-1);
            _restore_globOrientation = false;
        }
    }
};

CRPocketBookDocView * CRPocketBookDocView::instance = NULL;

static void rotate_timer()
{
    CRPocketBookDocView::instance->onRotateTimer();
}

static void paused_rotate_timer()
{
    CRPocketBookDocView::instance->onPausedRotation();
}

CRPbDictionaryView::CRPbDictionaryView(CRGUIWindowManager * wm, CRPbDictionaryDialog *parent) 
    : CRViewDialog(wm, lString16::empty_str, lString8::empty_str, lvRect(), false, true), _parent(parent),
    _dictsTable(16), _active(false), _dictsLoaded(false), _itemsCount(5), _translateResult(0),
    _newWord(NULL), _newTranslation(NULL)
{
    setSkinName(lString16("#dict"));
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
    _dictMenu = new CRPbDictionaryMenu(_wm, this);
    setRect(rect);
    _dictMenu->reconfigure(0);
    CRPropRef props = CRPocketBookDocView::instance->getProps();
    getDocView()->setVisiblePageCount(props->getIntDef(PROP_POCKETBOOK_DICT_PAGES, 1));
    lString16 lastDict = props->getStringDef(PROP_POCKETBOOK_DICT, pbGlobals->getDictionary());
    if (lastDict.empty()) {
        loadDictionaries();
        if (_dictCount > 0)
            lastDict = Utf8ToUnicode(lString8(_dictNames[0]));
    }
    if (!lastDict.empty()) {
        _dictIndex = 0;
        int rc = OpenDictionary((char *)UnicodeToUtf8(lastDict).c_str());
        if (rc == 1) {
            _caption = lastDict;
            getDocView()->createDefaultDocument(lString16::empty_str, Utf8ToUnicode(TR("@Word_not_found")));
            return;
        }
        lString8 dName =  UnicodeToUtf8(lastDict);
        CRLog::error("OpenDictionary(%s) returned %d", dName.c_str(), rc);
    }
    _dictIndex = -1;
    getDocView()->createDefaultDocument(lString16::empty_str, Utf8ToUnicode(TR("@Dic_error")));
}

void CRPbDictionaryView::loadDictionaries()
{
    _dictNames = EnumDictionaries();
    for (_dictCount = 0; _dictNames[_dictCount]; _dictCount++) {
        _dictsTable.set(Utf8ToUnicode( lString8(_dictNames[_dictCount]) ), _dictCount);
    }
    CRLog::trace("_dictCount = %d", _dictCount);
    _dictsLoaded = true;
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
    LVFontRef valueFont(fontMan->GetFont( VALUE_FONT_SIZE, 400, true, css_ff_sans_serif, lString8("Liberation Sans")));
    CRMenu * dictsMenu = new CRMenu(_wm, NULL, PB_CMD_SELECT_DICT,
                                    lString16(""), LVImageSourceRef(), LVFontRef(), valueFont,
                                    CRPocketBookDocView::instance->getNewProps(), PROP_POCKETBOOK_DICT);
    dictsMenu->setAccelerators(_wm->getAccTables().get("menu"));
    dictsMenu->setSkinName(lString16("#settings"));
    if (!_dictsLoaded) {
        loadDictionaries();
    }
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

bool CRPbDictionaryView::isArticleListActive()
{
    return (_selectedIndex == PB_DICT_ARTICLE_LIST && _translateResult != 0 &&
            !_parent->isSelectingWord());
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
        _caption = lastDict;
    }
    lString16 word = _word;
    _word.clear();
    _parent->setDocDirty();
    _selectedIndex = PB_DICT_DEACTIVATE;
    translate(word);
}

void CRPbDictionaryView::noTranslation()
{
    setTranslation(CRViewDialog::makeFb2Xml(lString8::empty_str));
    _newWord = _newTranslation = NULL;
    setCurItem(PB_DICT_EXIT);
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
    return false;
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
        const lChar8 *closeTag = NULL;
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
                    dst << "<emphasis>";
                    closeTag = "</emphasis>";
                    break;
                case 3:
                    dst << "<strong>";
                    closeTag = "</strong>";
                    break;
                case '\n':
                    dst << "</p><p>";
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
            dst << "</p>";
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
        //_translateResult = LookupWordExact((char *)what.c_str(), &word, &translation);
        CRLog::trace("LookupWord(%s) returned %d", what.c_str(), _translateResult);
        if (_translateResult != 0) {
            if (_translateResult == 1) {
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
            _newWord = _newTranslation = NULL;
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

void CRPbDictionaryView::setTranslation(lString8 translation)
{
    _stream = LVCreateStringStream( translation );
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
    : CRMenuItem(menu, id, lString16::empty_str, LVImageSourceRef(), LVFontRef() ), _word( word ), _translation(translation)
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
    lString16 word = _word16 + " ";
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
    : CRMenu(wm, NULL, 0, lString16::empty_str, LVImageSourceRef(), LVFontRef(), LVFontRef() ), _parent(parent)
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

    CRLog::trace("CRPbDictionaryMenu::onCommand( %d, %d )", command, params);
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
        if (params > 0)
            prevPage();
        else
            setCurItem(_selectedItem - 1);
        return true;
    case PB_CMD_DOWN:
        if (params > 0)
            nextPage();
        else
            setCurItem(_selectedItem + 1);
        return true;
    default:
        return false;
    }
    if (_selectedItem >= 0 && _selectedItem < _items.length()) {
        CRPbDictionaryMenuItem *item = static_cast<CRPbDictionaryMenuItem *>(_items[_selectedItem]);

        _parent->setTranslation(CRViewDialog::makeFb2Xml(
                _parent->createArticle(item->getWord(), item->getTranslation())));
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

CRPbDictionaryDialog::CRPbDictionaryDialog( CRGUIWindowManager * wm, CRViewDialog * docwin, lString8 css )
    : CRGUIWindowBase( wm ), _docwin(docwin), _docview(docwin->getDocView()), _docDirty(true), _curPage(0),
    _lastWordX(0), _lastWordY(0)
{
    _wordSelector = NULL;
    _fullscreen = true;
    CRGUIAcceleratorTableRef acc = _wm->getAccTables().get("dict");
    if ( acc.isNull() )
        acc = _wm->getAccTables().get("dialog");
    _dictView = new CRPbDictionaryView(wm, this);
    if (!css.empty())
        _dictView->getDocView()->setStyleSheet(css);
    int fs = _docview->getDocProps()->getIntDef( PROP_FONT_SIZE, 22 );
    _dictView->getDocView()->setFontSize(fs);
    setAccelerators( acc );
    CRPropRef props = CRPocketBookDocView::instance->getProps();
    _autoTranslate = props->getBoolDef(PROP_POCKETBOOK_DICT_AUTO_TRANSLATE, true);
    _wordTranslated = _dictViewActive = false;
    CRGUIAcceleratorTableRef mainAcc = CRPocketBookDocView::instance->getAccelerators();
    if (!mainAcc.isNull()) {
        int upKey = 0, upFlags = 0;
        int downKey = 0, downFlags = 0;
        int leftKey = 0, leftFlags = 0;
        int rightKey = 0, rightFlags = 0;
        int key = 0, keyFlags = 0;
        _acceleratorTable->findCommandKey(PB_CMD_UP, 0, upKey, upFlags);
        _acceleratorTable->findCommandKey(PB_CMD_DOWN, 0, downKey, downFlags);
        _acceleratorTable->findCommandKey(PB_CMD_LEFT, 0, leftKey, leftFlags);
        _acceleratorTable->findCommandKey(PB_CMD_RIGHT, 0, rightKey, rightFlags);

#define PB_CHECK_DICT_KEYS(key, flags)\
        (!((key == upKey && flags == upFlags) || (key = downKey && flags == downFlags) ||\
           (key == leftKey && flags == leftFlags) || (key == rightKey && flags == rightFlags)))

        if (mainAcc->findCommandKey(DCMD_PAGEUP, 0, key, keyFlags)) {
            if (PB_CHECK_DICT_KEYS(key, keyFlags))
                _acceleratorTable->add(key, keyFlags, DCMD_PAGEUP, 0);
        }
        if (mainAcc->findCommandKey(DCMD_PAGEDOWN, 0, key, keyFlags)) {
            if (PB_CHECK_DICT_KEYS(key, keyFlags))
                _acceleratorTable->add(key, keyFlags, DCMD_PAGEDOWN, 0);
        }
    }
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
        _dictView->clearSelection();
    }
}

void CRPbDictionaryDialog::onWordSelection() 
{
    CRLog::trace("CRPbDictionaryDialog::onWordSelection()");
    ldomWordEx * word = _wordSelector->getSelectedWord();
    if (!word) {
        _dictView->noTranslation();
        _wordTranslated = false;
        return;
    }
    lvRect dictRc = _dictView->getRect();
    lvRect rc(dictRc);
    lvRect wRc;
    _docview->setCursorPos(word->getWord().getStartXPointer());
    _docview->getCursorRect(wRc);
    if (dictRc.top > 0) {
        if (wRc.bottom >= dictRc.top) {
            rc.top = 0;
            rc.bottom = _dictView->getDesiredHeight();
            _dictView->setRect(rc);
        }
    } else {
        if (wRc.top <= dictRc.bottom) {
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
    if (_autoTranslate) {
        if (!firstTime) {
            setDocDirty();
            Update();
            pbGlobals->startTranslateTimer();
        } else
            _wm->onCommand(PB_CMD_TRANSLATE, 0);
    } else {
        if (_wordTranslated || firstTime) {
            if (_prompt.empty()) {
                int key = 0;
                int keyFlags = 0;
                lString16 prompt_msg = lString16(_("Press $1 to translate"));
                if (_acceleratorTable->findCommandKey(MCMD_OK, 0, key, keyFlags))
                    prompt_msg.replaceParam(1, lString16(getKeyName(key, keyFlags)));
                else
                    prompt_msg.replaceParam(1, lString16::empty_str);
                lString8 body;
                body << "<p>" << UnicodeToUtf8(prompt_msg) << "</p";
                _prompt = CRViewDialog::makeFb2Xml(body);
            }
            _dictView->setTranslation(_prompt);
            _wordTranslated = false;
        }
        setDocDirty();
        Update();
    }
}

bool CRPbDictionaryDialog::onCommand( int command, int params )
{
    MoveDirection dir = DIR_ANY;
    int curPage;
    bool ret;

    CRLog::trace("CRPbDictionaryDialog::onCommand(%d,%d) _dictActive=%d", command, params, _dictViewActive);
    if (_dictViewActive)
        return _dictView->onCommand(command, params);

    if (params == 0)
        params = 1;

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
        if (!_autoTranslate && !_wordTranslated) {
            _wm->postCommand(PB_CMD_TRANSLATE, 0);
            return true;
        }
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
        _wordTranslated = true;
        Update();
        return true;
    default:
        return false;
    }
    CRLog::trace("Before move");
    _wordSelector->moveBy(dir, 1);
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
            if (commandId != -1)
                pbTable.add(i, 0, commandId, commandParam);
        }
        if (keypresslong[i]) {
            CRPocketBookWindowManager::instance->getPocketBookCommand(keypresslong[i], commandId, commandParam);
            CRLog::trace("keypresslong[%d] = %s, cmd = %d, param=%d", i, keypresslong[i], commandId, commandParam);
            if (commandId != -1)
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
            lString16 mofilename = USERDATA"/share/cr3/i18n/" + lString16(lang) + ".mo";
            lString16 mofilename2 = USERDATA2"/share/cr3/i18n/" + lString16(lang) + ".mo";
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
    fontDirs.add(lString16(USERFONTDIR));
    fontDirs.add(lString16(SYSTEMFONTDIR));
    CRLog::info("INIT...");
    if (!InitCREngine(exename, fontDirs))
        return 0;

    {
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
        HyphMan::initDictionaries(lString16(USERDATA"/share/cr3/hyph/"));
        if (!wm->loadSkin(lString16(CONFIGPATH"/cr3/skin")))
            if (!wm->loadSkin(lString16(USERDATA2"/share/cr3/skin")))
                wm->loadSkin(lString16(USERDATA"/share/cr3/skin"));

        ldomDocCache::init(lString16(STATEPATH"/cr3/.cache"), PB_CR3_CACHE_SIZE);
        if (!ldomDocCache::enabled())
            ldomDocCache::init(lString16(USERDATA2"/share/cr3/.cache"), PB_CR3_CACHE_SIZE);
        if (!ldomDocCache::enabled())
            ldomDocCache::init(lString16(USERDATA"/share/cr3/.cache"), PB_CR3_CACHE_SIZE);
        CRLog::trace("creating main window...");
        main_win = new CRPocketBookDocView(wm, lString16(USERDATA"/share/cr3"));
        CRLog::trace("setting colors...");
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if (manual_file[0])
            main_win->setHelpFile( lString16( manual_file ) );
        if (!main_win->loadDefaultCover(lString16(CONFIGPATH"/cr3/cr3_def_cover.png")))
            if (!main_win->loadDefaultCover(lString16(USERDATA2"/share/cr3/cr3_def_cover.png")))
                main_win->loadDefaultCover(lString16(USERDATA"/share/cr3/cr3_def_cover.png"));
        if ( !main_win->loadCSS(lString16(CONFIGPATH"/cr3/")   + lString16(css_file_name) ) )
            if ( !main_win->loadCSS(  lString16(USERDATA"/share/cr3/" ) + lString16(css_file_name) ) )
                main_win->loadCSS( lString16(USERDATA2"/share/cr3/" ) + lString16(css_file_name) );
        main_win->setBookmarkDir(lString16(FLASHDIR"/cr3_notes/"));
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
        if ( !main_win->loadHistory(lString16(STATEPATH"/cr3/.cr3hist")) )
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

static bool commandCanRepeat(int command)
{
    switch (command) {
    case DCMD_LINEUP:
    case PB_CMD_PAGEUP_REPEAT:
    case PB_CMD_PAGEDOWN_REPEAT:
    case DCMD_LINEDOWN:
    case MCMD_SCROLL_FORWARD_LONG:
    case MCMD_SCROLL_BACK_LONG:
    case MCMD_NEXT_ITEM:
    case MCMD_PREV_ITEM:
    case MCMD_NEXT_PAGE:
    case MCMD_PREV_PAGE:
    case MCMD_LONG_FORWARD:
    case MCMD_LONG_BACK:
    case PB_CMD_LEFT:
    case PB_CMD_RIGHT:
    case PB_CMD_UP:
    case PB_CMD_DOWN:
        return true;
    }
    return false;
}

int main_handler(int type, int par1, int par2)
{
    bool process_events = false;
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
        CRPocketBookWindowManager::instance->postCommand(DCMD_PAGEUP, 0);
        process_events = true;
        break;
    case EVT_NEXTPAGE:
        CRPocketBookWindowManager::instance->postCommand(DCMD_PAGEDOWN, 0);
        process_events = true;
        break;
    case EVT_ORIENTATION:
        CRPocketBookDocView::instance->onAutoRotation(par1);
        break;
    case EVT_KEYPRESS:
        if (par1 == KEY_POWER) {
            return 0;
        }
        if (CRPocketBookWindowManager::instance->hasKeyMapping(par1, KEY_FLAG_LONG_PRESS) < 0) {
            CRPocketBookWindowManager::instance->onKeyPressed(par1, 0);
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
        if (type == EVT_KEYRELEASE) {
            if (par2 == 0)
                CRPocketBookWindowManager::instance->onKeyPressed(par1, 0);
            else
                CRPocketBookWindowManager::instance->postCommand(PB_CMD_REPEAT_FINISH, 0);
        } else if (type == EVT_KEYREPEAT) {
            int cmd = CRPocketBookWindowManager::instance->hasKeyMapping(par1, KEY_FLAG_LONG_PRESS);
            if (par2 == 1 || (par2 > 1 && commandCanRepeat(cmd)))
                CRPocketBookWindowManager::instance->onKeyPressed(par1, KEY_FLAG_LONG_PRESS);
        }
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
        CRPocketBookWindowManager::instance->processPostedEvents();
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
    OpenScreen();
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
