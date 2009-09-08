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
    m_props = m_docview->getOptions();
    optionToUi( PROP_WINDOW_FULLSCREEN, m_ui->cbWindowFullscreen );
    optionToUi( PROP_WINDOW_SHOW_MENU, m_ui->cbWindowShowMenu );
    optionToUi( PROP_WINDOW_SHOW_SCROLLBAR, m_ui->cbWindowShowScrollbar );
    optionToUi( PROP_WINDOW_TOOLBAR_SIZE, m_ui->cbWindowShowToolbar );
    optionToUi( PROP_WINDOW_SHOW_STATUSBAR, m_ui->cbWindowShowStatusBar );
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
    close();
}

void SettingsDlg::on_buttonBox_accepted()
{
    m_docview->setOptions( m_props );
    close();
}

void SettingsDlg::optionToUi( const char * optionName, QCheckBox * cb )
{
    int state = ( m_props->getIntDef( optionName, 1 ) != 0 ) ? 1 : 0;
    CRLog::debug("optionToUI(%s,%d)", optionName, state);
    cb->setCheckState( state ? Qt::Checked : Qt::Unchecked );
}

void SettingsDlg::setCheck( const char * optionName, int checkState )
{
    int value = (checkState == Qt::Checked) ? 1 : 0;
    CRLog::debug("setCheck(%s,%d)", optionName, value);
    m_props->setInt( optionName, value );
}

void SettingsDlg::on_cbWindowFullscreen_stateChanged(int s)
{
    setCheck( PROP_WINDOW_FULLSCREEN, s );
}

void SettingsDlg::on_cbWindowShowToolbar_stateChanged(int s)
{
    setCheck( PROP_WINDOW_TOOLBAR_SIZE, s );
}

void SettingsDlg::on_cbWindowShowMenu_stateChanged(int s)
{
    setCheck( PROP_WINDOW_SHOW_MENU, s );
}

void SettingsDlg::on_cbWindowShowStatusBar_stateChanged(int s)
{
    setCheck( PROP_WINDOW_SHOW_STATUSBAR, s );
}

void SettingsDlg::on_cbWindowShowScrollbar_stateChanged(int s)
{
    setCheck( PROP_WINDOW_SHOW_SCROLLBAR, s );
}
