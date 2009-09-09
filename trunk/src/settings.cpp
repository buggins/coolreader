#include "settings.h"
#include "ui_settings.h"
#include "cr3widget.h"
#include "crqtutil.h"

static int def_margins[] = { 2, 6, 12, 20, 30 };

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

    optionToUi( PROP_FOOTNOTES, m_ui->cbShowFootNotes );
    optionToUi( PROP_SHOW_BATTERY, m_ui->cbShowBattery );
    optionToUi( PROP_SHOW_TIME, m_ui->cbShowClock );
    optionToUi( PROP_SHOW_TITLE, m_ui->cbShowBookName );
    m_ui->cbShowPageHeader->setCheckState( ( m_props->getIntDef( PROP_STATUS_LINE, 1 ) != 0 ) ? Qt::Unchecked : Qt::Checked );

    int lp = m_props->getIntDef( PROP_LANDSCAPE_PAGES, 2 );
    int vm = m_props->getIntDef( PROP_PAGE_VIEW_MODE, 1 );
    if ( vm==0 )
        m_ui->cbViewMode->setCurrentIndex( 2 );
    else
        m_ui->cbViewMode->setCurrentIndex( lp==1 ? 0 : 1 );

    int n = m_props->getIntDef( PROP_PAGE_MARGIN_LEFT, 8 );
    int mi = 0;
    for ( int i=0; i<5; i++ ) {
        if ( n <= def_margins[i] ) {
            mi = i;
            break;
        }
    }
    m_ui->cbMargins->setCurrentIndex( mi );
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

void SettingsDlg::on_cbViewMode_currentIndexChanged(int index)
{
    switch ( index ) {
        case 0:
            m_props->setInt( PROP_LANDSCAPE_PAGES, 1 );
            m_props->setInt( PROP_PAGE_VIEW_MODE, 1 );
            break;
        case 1:
            m_props->setInt( PROP_LANDSCAPE_PAGES, 2 );
            m_props->setInt( PROP_PAGE_VIEW_MODE, 1 );
            break;
        default:
            m_props->setInt( PROP_PAGE_VIEW_MODE, 0 );
            break;
    }
}

void SettingsDlg::on_cbShowPageHeader_stateChanged(int s)
{
    setCheck( PROP_STATUS_LINE, (s == Qt::Checked) ? Qt::Unchecked : Qt::Checked );
}

void SettingsDlg::on_cbShowBookName_stateChanged(int s)
{
    setCheck( PROP_SHOW_TITLE, s );
}

void SettingsDlg::on_cbShowClock_stateChanged(int s)
{
    setCheck( PROP_SHOW_TIME, s );
}

void SettingsDlg::on_cbShowBattery_stateChanged(int s)
{
    setCheck( PROP_SHOW_BATTERY, s );
}

void SettingsDlg::on_cbShowFootNotes_stateChanged(int s)
{
    setCheck( PROP_FOOTNOTES, s );

}

void SettingsDlg::on_cbMargins_currentIndexChanged(int index)
{
    int m = def_margins[index];
    m_props->setInt( PROP_PAGE_MARGIN_BOTTOM, m*2/3 );
    m_props->setInt( PROP_PAGE_MARGIN_TOP, m );
    m_props->setInt( PROP_PAGE_MARGIN_LEFT, m );
    m_props->setInt( PROP_PAGE_MARGIN_RIGHT, m );
}
