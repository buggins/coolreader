#include "addbookmarkdlg.h"
#include "ui_addbookmarkdlg.h"
#include "cr3widget.h"

static bool initialized = false;

bool AddBookmarkDialog::editBookmark( QWidget * parent, CR3View * docView, CRBookmark * bm )
{
    AddBookmarkDialog * dlg = new AddBookmarkDialog( parent, docView, bm );
    if ( !dlg->_bm ) {
        delete dlg;
        return false;
    }
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    return true;
}

AddBookmarkDialog::AddBookmarkDialog(QWidget *parent, CR3View * docView, CRBookmark * bm ) :
    QDialog(parent),
    m_ui(new Ui::AddBookmarkDialog),
    _docview( docView ),
    _bm(bm),
    _edit(bm!=NULL)
{
    initialized = false;
    m_ui->setupUi(this);
    setWindowTitle( tr("Add bookmark") );
    if ( _bm==NULL )
        _bm = docView->createBookmark();
    if ( _bm ) {
        if ( _bm->getType() == bmkt_pos ) {
            m_ui->cbType->addItem( tr("Position") );
        } else {
            m_ui->cbType->addItem( tr("Comment") );
            m_ui->cbType->addItem( tr("Correction") );
            m_ui->cbType->setCurrentIndex(0);
        }
        m_ui->edPositionText->setPlainText( cr2qt(_bm->getPosText()) );
        m_ui->edPositionText->setReadOnly( true );
        m_ui->edComment->setPlainText( cr2qt(_bm->getCommentText()) );
        m_ui->edComment->setReadOnly( false );
        m_ui->lblPosition->setText( crpercent(_bm->getPercent()) );
        m_ui->lblTitle->setText( cr2qt(_bm->getTitleText()) );
    }
    _docview->restoreWindowPos( this, "bookmarkedit." );
    initialized = true;
}

void AddBookmarkDialog::closeEvent ( QCloseEvent * event )
{
    _docview->saveWindowPos( this, "bookmarkedit." );
}


AddBookmarkDialog::~AddBookmarkDialog()
{
    delete m_ui;
}

bool AddBookmarkDialog::showDlg( QWidget * parent, CR3View * docView )
{
    AddBookmarkDialog * dlg = new AddBookmarkDialog( parent, docView, NULL );
    if ( !dlg->_bm ) {
        delete dlg;
        return false;
    }
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
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
    _bm->setCommentText( qt2cr(m_ui->edComment->toPlainText()) );
    _docview->getDocView()->clearImageCache();
    _docview->update();
    close();
}

void AddBookmarkDialog::on_buttonBox_rejected()
{
    if (!_edit && !_docview->getDocView()->removeBookmark( _bm ))
        delete _bm;
    close();
}

void AddBookmarkDialog::on_cbType_currentIndexChanged(int index)
{
    if ( !initialized )
        return;
    if ( index == 0 ) {
        _bm->setType( bmkt_comment );
        m_ui->edComment->setPlainText( QString() );
        m_ui->lblComment->setText( tr("Comment") );
    } else {
        _bm->setType( bmkt_correction );
        m_ui->edComment->setPlainText( cr2qt(_bm->getPosText()) );
        m_ui->lblComment->setText( tr("Correction") );
    }
}
