
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
        void OnInitDialog(wxInitDialogEvent& event);
    private:
        DECLARE_EVENT_TABLE()
};

#endif // _WOLOPT_H_
