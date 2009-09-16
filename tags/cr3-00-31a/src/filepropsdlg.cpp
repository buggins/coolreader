#include "filepropsdlg.h"
#include "ui_filepropsdlg.h"
#include "cr3widget.h"

FilePropsDialog::FilePropsDialog(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::FilePropsDialog)
{
    m_ui->setupUi(this);
    setWindowTitle( tr("Document properties") );
}

FilePropsDialog::~FilePropsDialog()
{
    delete m_ui;
}

bool FilePropsDialog::showDlg( QWidget * parent, CR3View * docView )
{
    FilePropsDialog * dlg = new FilePropsDialog( parent, docView );
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    //dlg->
    return true;
}

void FilePropsDialog::changeEvent(QEvent *e)
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
