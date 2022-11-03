/***************************************************************************
 *   CoolReader, wxWidgets GUI                                             *
 *   Copyright (C) 2007-2010 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2008 Torque <torque@mail.ru>                            *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#ifndef _CR3VIEW_H_
#define _CR3VIEW_H_

#include <crgui.h>
/**
 * @short XML Document View window
 */

class
cr3view : public wxPanel, public LVDocViewCallback
{
    public:
        cr3view(CRPropRef props, lString32 exeDirPath );
        virtual ~cr3view();
        void ScheduleRender() { Resize(0, 0); }
        bool LoadDocument( const wxString & fname );
        void CloseDocument();
        void SetScrollBar( wxScrollBar * sb ) { _scrollbar = sb; }
        void UpdateScrollBar();
        LVDocView * getDocView() { return _docwin->getDocView(); }
        void doCommand( LVDocCmd cmd, int param );
        void goToBookmark(ldomXPointer bm);
        wxColour getBackgroundColour();
        void SetPageHeaderFlags();
        void SetRotate( cr_rotate_angle_t angle );
        void Rotate( bool ccw = false );
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
        void OnMouseMotion(wxMouseEvent& event);
        void OnTimer(wxTimerEvent& event);
        void OnInitDialog(wxInitDialogEvent& event);
        void ToggleViewMode();
		void SetFullScreenState(bool fullscreenState) { _isFullscreen = fullscreenState; }
        lString32 GetHistoryFileName();
        lString32 GetLastRecentFileName();
        // LVDocViewCallback override
        virtual void OnExternalLink( lString32 url, ldomNode * node );
    protected:

        void Paint();
        void Resize(int dx, int dy);
    private:
        wxCursor _normalCursor;
        wxCursor _linkCursor;
        //LVDocView * _docview;
        wxScrollBar * _scrollbar;

        wxTimer * _renderTimer;
        wxTimer * _clockTimer;
        wxTimer * _cursorTimer;
        bool _firstRender;
        bool _allowRender;
        CRPropRef _props;

        bool _isFullscreen;

        CRWxScreen _screen;
        CRGUIWindowManager _wm;
        CRDocViewWindow * _docwin;

        DECLARE_EVENT_TABLE()
};

int propsToPageHeaderFlags( CRPropRef props );

#endif // _CR3VIEW_H_
