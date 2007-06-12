
#ifndef _HISTLIST_H_
#define _HISTLIST_H_

#include <wx/listctrl.h>

/**
 * @short XML Document View window
 * @author Vadim Lopatin <vadim.lopatin@coolreader.org>
 * @version 0.1
 */

class
HistList : public wxListView
{
    private:
        LVPtrVector<CRFileHistRecord> * _records;
    public:
        HistList();
        virtual ~HistList();
        virtual bool Create(wxWindow* parent, wxWindowID id );
        virtual wxString OnGetItemText(long item, long column) const;
        void SetRecords(LVPtrVector<CRFileHistRecord> & records );
/*
        void ScheduleRender() { Resize(0, 0); }
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
*/
    protected:
/*
        void Paint();
        void Resize(int dx, int dy);
*/
    private:
 /*
        LVDocView * _docview;
        wxScrollBar * _scrollbar;

        wxTimer * _renderTimer;
        wxTimer * _clockTimer;
        bool _firstRender;
*/
        DECLARE_EVENT_TABLE()
};

#endif // _CR3VIEW_H_
