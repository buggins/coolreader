
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
        void UpdateColumns();
    protected:
    private:
        DECLARE_EVENT_TABLE()
};

#endif // _CR3VIEW_H_
