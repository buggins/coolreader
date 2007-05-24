
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
        cr3view();
        virtual ~cr3view();
        bool LoadDocument( const wxString & fname );
        void CloseDocument();
        void SetScrollBar( wxScrollBar * sb ) { _scrollbar = sb; }
        void UpdateScrollBar();
        LVDocView * getDocView() { return _docview; }
        void doCommand( LVDocCmd cmd, int param );
        void goToBookmark(ldomXPointer bm);
        wxColour getBackgroundColour();
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
        void TogglePageHeader();
        lString16 GetHistoryFileName();
        lString16 GetLastRecentFileName();
    protected:

        void Paint();
        void Resize(int dx, int dy);
    private:
        LVDocView * _docview;
        wxScrollBar * _scrollbar;

        wxTimer * _renderTimer;
        int _newWidth;
        int _newHeight;
        bool _firstRender;
        
        DECLARE_EVENT_TABLE()
};

#endif // _CR3VIEW_H_
