
#ifndef _WOLOPT_H_
#define _WOLOPT_H_

#include <wx/wx.h>

/**
 * @short WOL export options window
 * @author Vadim Lopatin <vadim.lopatin@coolreader.org>
 * @version 0.1
 */

class
WolOptions : public wxDialog
{
    private:
        wxComboBox * cbMode;
        wxComboBox * cbLevels; 
    public:
        WolOptions( wxWindow * parent );
        virtual ~WolOptions();

        int getMode()
        {
            return cbMode->GetCurrentSelection();
        }
        int getLevels()
        {
            return cbLevels->GetCurrentSelection()+1;
        }
/*
        bool LoadDocument( const wxString & fname );
        void CloseDocument();
        void SetScrollBar( wxScrollBar * sb ) { _scrollbar = sb; }
        void UpdateScrollBar();
        LVDocView * getDocView() { return _docview; }
        void doCommand( LVDocCmd cmd, int param );
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
        void OnTimer(wxTimerEvent& event);
*/
        void OnInitDialog(wxInitDialogEvent& event);
    private:
        DECLARE_EVENT_TABLE()
};

#endif // _WOLOPT_H_
