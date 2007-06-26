
#ifndef _CR3VIEW_H_
#define _CR3VIEW_H_

/**
 * @short XML Document View window
 * @author Vadim Lopatin <vadim.lopatin@coolreader.org>
 * @version 0.1
 */

class
cr3view : public wxPanel
{
    public:
        cr3view(CRPropRef props);
        virtual ~cr3view();
        void ScheduleRender() { Resize(0, 0); }
        bool LoadDocument( const wxString & fname );
        void CloseDocument();
        void SetScrollBar( wxScrollBar * sb ) { _scrollbar = sb; }
        void UpdateScrollBar();
        LVDocView * getDocView() { return _docview; }
        void doCommand( LVDocCmd cmd, int param );
        void goToBookmark(ldomXPointer bm);
        wxColour getBackgroundColour();
        void SetPageHeaderFlags();
        // event handlers
        void OnPaint(wxPaintEvent& event);
        void OnSize(wxSizeEvent& event);
        void OnKeyDown(wxKeyEvent& event);
        void OnMouseWheel(wxMouseEvent& event);
        void OnScroll(wxScrollEvent& event);
        void OnCommand( wxCommandEvent& event );
        void OnSetFocus( wxFocusEvent& event );
        void OnMouseLDown( wxMouseEvent & event );
        void OnMouseRDown( wxMouseEvent & event );
        void OnTimer(wxTimerEvent& event);
        void OnInitDialog(wxInitDialogEvent& event);
        void ToggleViewMode();
        lString16 GetHistoryFileName();
        lString16 GetLastRecentFileName();
    protected:

        void Paint();
        void Resize(int dx, int dy);
    private:
        LVDocView * _docview;
        wxScrollBar * _scrollbar;

        wxTimer * _renderTimer;
        wxTimer * _clockTimer;
        bool _firstRender;
        CRPropRef _props;

        DECLARE_EVENT_TABLE()
};

int propsToPageHeaderFlags( CRPropRef props );

#endif // _CR3VIEW_H_
