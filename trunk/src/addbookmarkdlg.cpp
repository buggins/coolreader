#include "addbookmarkdlg.h"
#include "ui_addbookmarkdlg.h"
#include "cr3widget.h"

AddBookmarkDialog::AddBookmarkDialog(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::AddBookmarkDialog)
{
    m_ui->setupUi(this);
    setWindowTitle( tr("Add bookmark") );
}

AddBookmarkDialog::~AddBookmarkDialog()
{
    delete m_ui;
}

bool AddBookmarkDialog::showDlg( QWidget * parent, CR3View * docView )
{
    AddBookmarkDialog * dlg = new AddBookmarkDialog( parent, docView );
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    //dlg->
    return true;
}

void AddBookmarkDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void AddBookmarkDialog::on_buttonBox_accepted()
{

}

void AddBookmarkDialog::on_buttonBox_rejected()
{

}
