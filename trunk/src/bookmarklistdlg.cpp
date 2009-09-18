#include "bookmarklistdlg.h"
#include "ui_bookmarklistdlg.h"
#include "cr3widget.h"

BookmarkListDialog::BookmarkListDialog(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::BookmarkListDialog),
    _docview(docView)
{
    m_ui->setupUi(this);
    setWindowTitle( tr("Bookmarks") );
}

BookmarkListDialog::~BookmarkListDialog()
{
    delete m_ui;
}

bool BookmarkListDialog::showDlg( QWidget * parent, CR3View * docView )
{
    BookmarkListDialog * dlg = new BookmarkListDialog( parent, docView );
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    //dlg->
    return true;
}

void BookmarkListDialog::changeEvent(QEvent *e)
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

void BookmarkListDialog::on_buttonBox_accepted()
{

}

void BookmarkListDialog::on_buttonBox_rejected()
{

}
