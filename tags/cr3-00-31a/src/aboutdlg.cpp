#include "aboutdlg.h"
#include "ui_aboutdlg.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AboutDialog)
{
    m_ui->setupUi(this);
}

AboutDialog::~AboutDialog()
{
    delete m_ui;
}

void AboutDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

bool AboutDialog::showDlg( QWidget * parent )
{
    AboutDialog * dlg = new AboutDialog( parent );
    //dlg->setModal( true );
    dlg->setWindowTitle(tr("About CoolReader"));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}


void AboutDialog::on_buttonBox_accepted()
{
    close();
}
