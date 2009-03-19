/*
    First version of CR3 for EWL, based on etimetool example by Lunohod
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
//#include <Ewl.h>
#include <crengine.h>
#include <crgui.h>
#include <crtrace.h>

#ifdef WITH_DICT
#include "mod-dict.h"
#endif

#include "cr3main.h"
#include "mainwnd.h"
#include <cri18n.h>

#define CR_USE_XCB


#ifdef _WIN32


HWND g_hWnd = NULL;

// Global Variables:
HINSTANCE hInst;                                // current instance

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEXW);

    wcex.style            = 0; //CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = NULL; //LoadIcon(hInstance, (LPCTSTR)IDI_FONTTEST);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = NULL;
    wcex.lpszClassName    = L"CoolReader";
    wcex.hIconSm        = NULL; //LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    return RegisterClassExW(&wcex);
}

/// WXWidget support: draw to wxImage
class CRWin32Screen : public CRGUIScreenBase
{
    public:
        HWND _hWnd;
        virtual void draw( HDC hdc )
        {
            LVDrawBuf * drawBuf = getCanvas().get();
            drawBuf->DrawTo( hdc, 0, 0, 0, NULL);
        }
        virtual void paint()
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(_hWnd, &ps);
            draw( hdc );
            EndPaint(_hWnd, &ps);
        }
    protected:
        virtual void update( const lvRect & rc, bool full )
        {
            InvalidateRect(_hWnd, NULL, FALSE);
            UpdateWindow(_hWnd);
        }
    public:
        virtual ~CRWin32Screen()
        {
        }
        CRWin32Screen( HWND hwnd, int width, int height )
        :  CRGUIScreenBase( 0, 0, true )
        {
            _hWnd = hwnd;
            _width = width;
            _height = height;
            _canvas = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );
            _front = LVRef<LVDrawBuf>( new LVGrayDrawBuf( _width, _height, GRAY_BACKBUFFER_BITS ) );
        }
};

class CRWin32WindowManager : public CRGUIWindowManager
{
protected:
    HWND _hWnd;
public:
    static CRWin32WindowManager * instance;

    CRWin32WindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL)
    {
       int x=0;
       int y=0;
       lUInt32 flags = 0;
    #ifdef FIXED_JINKE_SIZE
          flags = WS_DLGFRAME | WS_MINIMIZEBOX | WS_SYSMENU | WS_VSCROLL; //WS_OVERLAPPEDWINDOW
          dx = 600 + GetSystemMetrics(SM_CXDLGFRAME)*2
               + GetSystemMetrics(SM_CXVSCROLL);
          dy = 800 + GetSystemMetrics(SM_CYDLGFRAME)*2
               + GetSystemMetrics(SM_CYCAPTION);
    #else
          flags = WS_OVERLAPPEDWINDOW;// | WS_VSCROLL; //
          dx = 600;
          dy = 750;
    #endif

       _hWnd = CreateWindowW(
           L"CoolReader",
           L"CREngine - Simple FB2 viewer",
          flags, //WS_OVERLAPPEDWINDOW
          x, y, dx, dy,
          NULL, NULL, hInst, NULL);

       if (!_hWnd)
       {
          return;
       }

       g_hWnd = _hWnd;

        CRWin32Screen * s = new CRWin32Screen( _hWnd, dx, dy );
        _screen = s;
        _ownScreen = true;
        instance = this;
    }
    // runs event loop
    virtual int runEventLoop()
    {
        ShowWindow( _hWnd, SW_SHOW );
        // Main message loop:
        MSG msg;
        bool stop = false;
        while (!stop && GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
			if ( !instance )
				break;
            processPostedEvents();
            if ( !getWindowCount() )
                stop = true;
        }
        return 0;
    }
	virtual ~CRWin32WindowManager()
	{
		instance = NULL;
	}
};

CRWin32WindowManager * CRWin32WindowManager::instance = NULL;
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	bool needUpdate = false;
    //int wmId, wmEvent;
    switch (message)
    {
        case WM_CREATE:
            {
            }
            break;
        case WM_ERASEBKGND:
            break;
        case WM_VSCROLL:
            break;
        case WM_SIZE:
			if ( CRWin32WindowManager::instance )
            {
                if (wParam!=SIZE_MINIMIZED)
                {
					RECT rect;
					::GetClientRect(hWnd, &rect );
					CRWin32WindowManager::instance->setSize( rect.right-rect.left, rect.bottom-rect.top );
                    needUpdate = true;
                }
            }
            break;
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120     /* Value for rolling one detent */
#endif
        case WM_MOUSEWHEEL:
            {
                /*
              int delta = ((lInt16)HIWORD(wParam))/WHEEL_DELTA;
              if (delta<0)
                  DoCommand( hWnd, DCMD_LINEDOWN, 3 );
              else if (delta>0)
                  DoCommand( hWnd, DCMD_LINEUP, 3 );
                */
            }
            break;
        case WM_CHAR:
			if ( CRWin32WindowManager::instance )
            {
				int shift = 0;
				if ( ::GetKeyState(VK_SHIFT) & 0x8000 )
					shift |= 1;
				if ( ::GetKeyState(VK_CONTROL) & 0x8000 )
					shift |= 1;
				char shiftChars[] = ")!@#$%^&*(";
				int ch = wParam;
				for ( int i=0; shiftChars[i]; i++ )
					if ( ch==shiftChars[i] ) {
						ch = '0' + i;
						break;
					}
                if ( ch>=' ' && ch<=127 ) {
                    needUpdate = CRWin32WindowManager::instance->onKeyPressed( ch, shift );
                }
            }
            break;
        case WM_KEYDOWN:
			if ( CRWin32WindowManager::instance )
            {
                int code = 0;
				int shift = 0;
				if ( ::GetKeyState(VK_SHIFT) & 0x8000 )
					shift |= 1;
				if ( ::GetKeyState(VK_CONTROL) & 0x8000 )
					shift |= 1;
				
                switch( wParam )
                {
                case VK_RETURN:
                    code = XK_Return;
                    break;
                case VK_ESCAPE:
                    code = XK_Escape;
                    break;
                case VK_UP:
                    code = XK_Up;
                    break;
                case VK_DOWN:
                    code = XK_Down;
                    break;
                case VK_ADD:
                    code = '+';
                    break;
                case VK_SUBTRACT:
                    code = '-';
                    break;
                }
                if ( code ) {
                    if ( CRWin32WindowManager::instance->onKeyPressed( code, shift ) )
						needUpdate = true;
                }
            }
            break;
        case WM_LBUTTONDOWN:
            {
                /*
                int xPos = lParam & 0xFFFF;
                int yPos = (lParam >> 16) & 0xFFFF;
                ldomXPointer ptr = text_view->getNodeByPoint( lvPoint( xPos, yPos ) );
                if ( !ptr.isNull() ) {
                    if ( ptr.getNode()->isText() ) {
                        ldomXRange * wordRange = new ldomXRange();
                        if ( ldomXRange::getWordRange( *wordRange, ptr ) ) {
                            wordRange->setFlags( 0x10000 );
                            text_view->getDocument()->getSelections().clear();
                            text_view->getDocument()->getSelections().add( wordRange );
                            text_view->updateSelections();
                            UpdateScrollBar( hWnd );
                        } else {
                            delete wordRange;
                        }
                    }
                }
            */
            }
            break;
        case WM_COMMAND:
            break;
        case WM_PAINT:
            {
				if ( CRWin32WindowManager::instance )
					((CRWin32Screen*)CRWin32WindowManager::instance->getScreen())->paint();
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
    if ( CRWin32WindowManager::instance && (CRWin32WindowManager::instance->processPostedEvents() || needUpdate) )
        CRWin32WindowManager::instance->update(true);
   return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

    CRLog::setFileLogger( "crengine.log" );
    CRLog::setLogLevel( CRLog::LL_TRACE );


    lString8 exe_dir;
    char exe_fn[MAX_PATH+1];
    GetModuleFileNameA( NULL, exe_fn, MAX_PATH );
    lChar16 exe_fn16[MAX_PATH+1];
    GetModuleFileNameW( NULL, exe_fn16, MAX_PATH );
	lString16 exedir = LVExtractPath(lString16(exe_fn16));	
	lString8 exedir8 = UnicodeToUtf8( exedir );

	CRMoFileTranslator * translator = new CRMoFileTranslator();
	translator->openMoFile(exedir + L"/po/ru.mo");
	CRI18NTranslator::setTranslator( translator );


	lChar16 sysdir[MAX_PATH+1];
	GetWindowsDirectoryW(sysdir, MAX_PATH);
	lString16 fontdir( sysdir );
	fontdir << L"\\Fonts\\";
	lString8 fd = UnicodeToLocal(exedir);
	lString16Collection fontDirs;
	fontDirs.add( fontdir );
	InitCREngine( exe_fn, fontDirs );
    //LVCHECKPOINT("WinMain start");

    if (!fontMan->GetFontCount())
    {
        //error
        char str[100];
#if (USE_FREETYPE==1)
        sprintf(str, "Cannot open font file(s) fonts/*.ttf \nCannot work without font\nPlace some TTF files to font\\ directory" );
#else
        sprintf(str, "Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF" );
#endif
        MessageBoxA( NULL, str, "CR Engine :: Fb2Test -- fatal error!", MB_OK);
        return 1;
    }

    lString8 cmdline(lpCmdLine);
    if ( cmdline.empty() )
        return 2; // need filename

    hInst = hInstance;
    MyRegisterClass(hInstance);

    {

		CRWin32WindowManager winman(500, 700);

		const char * keymap_locations [] = {
			exedir8.c_str(),
			NULL,
		};
		loadKeymaps( winman, keymap_locations );
		

        winman.loadSkin( LVExtractPath(LocalToUnicode(lString8(exe_fn))) + L"skin" );
        V3DocViewWin * main_win = new V3DocViewWin( &winman, LVExtractPath(LocalToUnicode(lString8(exe_fn))) );
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
		main_win->loadCSS( exedir + L"fb2.css" );
		main_win->loadSettings( exedir + L"cr3.ini" );
		main_win->loadDefaultCover( exedir + L"cr3_def_cover.png" );
		lString8 exedir8 = UnicodeToUtf8( exedir );
		const char * dirs[] = {
			exedir8.c_str(),
			NULL
		};

		loadKeymaps( winman, dirs );

        main_win->loadHistory( exedir + L"cr3hist.bmk" );

        winman.activateWindow( main_win );
        if ( !main_win->getDocView()->LoadDocument(cmdline.c_str()) ) {
            char str[100];
            sprintf(str, "Cannot open document file %s", cmdline.c_str());
            MessageBoxA( NULL, str, "CR Engine :: Fb2Test -- fatal error!", MB_OK);
            return 1;
        } else {
            winman.runEventLoop();
        }
    }
    //ShutdownFontManager();

    return 0;
}


#endif
