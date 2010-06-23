
#ifndef _CR3_H_
#define _CR3_H_

#include "view.h"
#include "histlist.h"

/**
 * @short Application Main Window
 * @author Vadim Lopatin <vadim.lopatin@coolreader.org>
 * @version 3.0.16
 */

#include <cr3version.h>
#define CR3_VERSION CR_ENGINE_VERSION

class 
cr3app : public wxApp
{
	public:
		virtual bool OnInit();
		virtual int OnExit();
};

class
cr3scroll : public wxScrollBar
{
private:
	cr3view *  _view;
public:
	cr3scroll( cr3view * view )
		: _view( view ) { }
    void OnSetFocus( wxFocusEvent& event );
private:
	DECLARE_EVENT_TABLE()
};

enum active_mode_t {
    am_none,
    am_book,
    am_history
};

class 
cr3Frame : public wxFrame
{
    private:
        bool _isFullscreen;
        active_mode_t _activeMode;
        int  _toolbarSize;
	public:
		cr3Frame( const wxString& title, const wxPoint& p, const wxSize& sz, lString16 appDir );

        void SetActiveMode( active_mode_t mode );
        void UpdateToolbar();
        void OnOptionsChange( CRPropRef oldprops, CRPropRef newprops, CRPropRef changed );

		void OnQuit( wxCommandEvent& event );
		void OnAbout( wxCommandEvent& event );
        void OnScroll( wxScrollEvent& event );
        void OnKeyDown( wxKeyEvent& event );
        void OnSetFocus( wxFocusEvent& event );
        void OnFileOpen( wxCommandEvent& event );
        void OnFileSave( wxCommandEvent& event );
        void OnCommand( wxCommandEvent& event );
        void OnRotate( wxCommandEvent& event );
        void OnShowOptions( wxCommandEvent& event );
        void OnShowTOC( wxCommandEvent& event );
        void OnShowHistory( wxCommandEvent& event );
        void OnUpdateUI( wxUpdateUIEvent& event );
        void OnClose( wxCloseEvent& event );
        void OnMouseWheel( wxMouseEvent& event);
        void OnSize( wxSizeEvent& event);
        void OnInitDialog( wxInitDialogEvent& event);
        void OnHistItemActivated( wxListEvent& event );

        CRPropRef getProps() { return _props; }
        void SaveOptions();
        void RestoreOptions();
        void SetMenu( bool visible );
        void SetStatus( bool visible );
        void SetToolbarSize( int size );

        wxBitmap getIcon16x16( const lChar16 * name );
	protected:
    	cr3scroll * _scrollBar;
		cr3view * _view;
    	HistList * _hist;
        wxBoxSizer * _sizer;
        lString16 _appDir;
        CRPropRef _props;
	private:
		DECLARE_EVENT_TABLE()
};

enum
{
	Menu_File_Quit = 100,
	Menu_File_About,
	Menu_File_Options,
    Menu_View_ZoomIn,
    Menu_View_ZoomOut,
    Menu_View_NextPage,
    Menu_View_PrevPage,
    Menu_View_NextLine,
    Menu_View_PrevLine,
    Menu_View_Text_Format,
    Menu_Link_Back,
    Menu_Link_Forward,
    Menu_Link_Next,
    Menu_Link_Prev,
    Menu_Link_Go,
    Menu_View_Begin,
    Menu_View_End,
    Menu_View_ToggleFullScreen,
    Menu_View_TogglePages,
    Menu_View_TogglePageHeader,
    Menu_View_TOC,
    Menu_View_History,
    Menu_View_Rotate,
};

enum
{
	Window_Id_Scrollbar = 1000,
	Window_Id_View,
    Window_Id_HistList,
	Window_Id_Options,
};

#endif // _CR3_H_
