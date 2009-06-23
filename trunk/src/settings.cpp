#include "settings.h"
#include "ui_settings.h"
#include "cr3widget.h"
#include "crqtutil.h"

SettingsDlg::SettingsDlg(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::SettingsDlg),
    m_docview( docView )
{
    m_ui->setupUi(this);
}

SettingsDlg::~SettingsDlg()
{
    delete m_ui;
}

bool SettingsDlg::showDlg( CR3View * docView )
{
    SettingsDlg * dlg = new SettingsDlg( NULL, docView );
    dlg->show();
    return true;
}

void SettingsDlg::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SettingsDlg::on_buttonBox_rejected()
{

}

void SettingsDlg::on_buttonBox_accepted()
{

}
