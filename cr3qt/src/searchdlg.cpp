#include "cr3widget.h"
#include <QEvent>
#include <QtGui/QMessageBox>
#include "searchdlg.h"
#include "ui_searchdlg.h"

bool SearchDialog::showDlg( QWidget * parent, CR3View * docView )
{
    SearchDialog * dlg = new SearchDialog( parent, docView );
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    //dlg->
    return true;
}

SearchDialog::SearchDialog(QWidget *parent, CR3View * docView) :
    QDialog(parent),
    ui(new Ui::SearchDialog),
    _docview( docView )
{
    ui->setupUi(this);
    ui->cbCaseSensitive->setCheckState(Qt::Unchecked);
    ui->rbForward->toggle();
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SearchDialog::on_btnFindNext_clicked()
{
    QMessageBox * mb = new QMessageBox( QMessageBox::Information, tr("Not implemented"), tr("Search is not implemented yet"), QMessageBox::Close, this );
    mb->exec();
}

void SearchDialog::on_btnClose_clicked()
{
    this->close();
}
