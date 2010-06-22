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

bool SearchDialog::findText( lString16 pattern, int origin, bool reverse, bool caseInsensitive )
{
    if ( pattern.empty() )
        return false;
    if ( pattern!=_lastPattern && origin==1 )
        origin = 0;
    _lastPattern = pattern;
    LVArray<ldomWord> words;
    lvRect rc;
    _docview->getDocView()->GetPos( rc );
    int pageHeight = rc.height();
    int start = -1;
    int end = -1;
    if ( reverse ) {
        // reverse
        if ( origin == 0 ) {
            // from end current page to first page
            end = rc.bottom;
        } else if ( origin == -1 ) {
            // from last page to end of current page
            start = rc.bottom;
        } else { // origin == 1
            // from prev page to first page
            end = rc.top;
        }
    } else {
        // forward
        if ( origin == 0 ) {
            // from current page to last page
            start = rc.top;
        } else if ( origin == -1 ) {
            // from first page to current page
            end = rc.top;
        } else { // origin == 1
            // from next page to last
            start = rc.bottom;
        }
    }
    CRLog::debug("CRViewDialog::findText: Current page: %d .. %d", rc.top, rc.bottom);
    CRLog::debug("CRViewDialog::findText: searching for text '%s' from %d to %d origin %d", LCSTR(pattern), start, end, origin );
    if ( _docview->getDocView()->getDocument()->findText( pattern, caseInsensitive, reverse, start, end, words, 200, pageHeight ) ) {
        CRLog::debug("CRViewDialog::findText: pattern found");
        _docview->getDocView()->clearSelection();
        _docview->getDocView()->selectWords( words );
        ldomMarkedRangeList * ranges = _docview->getDocView()->getMarkedRanges();
        if ( ranges ) {
            if ( ranges->length()>0 ) {
                int pos = ranges->get(0)->start.y;
                _docview->getDocView()->SetPos(pos);
            }
        }
        return true;
    }
    CRLog::debug("CRViewDialog::findText: pattern not found");
    return false;
}

void SearchDialog::on_btnFindNext_clicked()
{
    bool found = false;
    QString pattern = ui->edPattern->text();
    lString16 p16 = qt2cr(pattern);
    bool reverse = ui->rbBackward->isChecked();
    bool caseInsensitive = ui->cbCaseSensitive->checkState()!=Qt::Checked;
    found = findText(p16, 1, reverse , caseInsensitive);
    if ( !found )
        found = findText(p16, -1, reverse, caseInsensitive);
    if ( !found ) {
        QMessageBox * mb = new QMessageBox( QMessageBox::Information, tr("Not found"), tr("Search pattern is not found in document"), QMessageBox::Close, this );
        mb->exec();
    } else {
        _docview->update();
    }
}

void SearchDialog::on_btnClose_clicked()
{
    this->close();
}
