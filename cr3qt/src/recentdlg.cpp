#include "recentdlg.h"
#include "ui_recentdlg.h"
#include "cr3widget.h"
#include "crqtutil.h"
#include "../crengine/include/lvdocview.h"
#include <QMenu>
#include <QMessageBox>

RecentBooksDlg::RecentBooksDlg(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::RecentBooksDlg),
    m_docview(docView)
{
    m_ui->setupUi(this);
    m_ui->tableWidget->setColumnCount(4);
    m_ui->tableWidget->setHorizontalHeaderLabels ( QStringList() << tr("#") << tr("Author") << tr("Title") << tr("Filename") );
    m_ui->tableWidget->verticalHeader()->hide();
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 0, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 1, QHeaderView::ResizeToContents ); //Stretch
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 2, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 3, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setDefaultAlignment( Qt::AlignLeft );
    m_ui->tableWidget->setEditTriggers( QAbstractItemView::NoEditTriggers );
    //m_ui->tableWidget->setVerticalHeader( NULL );
    //m_ui->tableWidget->setHorizontalHeader(new QHeaderView(;
    docView->getDocView()->savePosition(); // to move current file to top
    LVPtrVector<CRFileHistRecord> & files = docView->getDocView()->getHistory()->getRecords();
    // skip Null
    m_ui->tableWidget->setRowCount(files.length()-1);
    m_ui->tableWidget->setWordWrap(false);
    m_ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->tableWidget->setSortingEnabled(true);
    int firstItem = docView->getDocView()->isDocumentOpened() ? 1 : 0;
    for ( int i=firstItem; i<files.length(); i++ ) {
        CRFileHistRecord * book = files.get( i );
        lString16 author = book->getAuthor();
        lString16 title = book->getTitle();
        lString16 series = book->getSeries();
        lString16 filename = book->getFileName();
        if ( author.empty() )
            author = "-"; //_book->getFileName();
        if ( title.empty() )
            title = "-"; //_book->getFileName();
        else if ( !series.empty() )
            title << " - " << series;
        int index = 0;
        m_ui->tableWidget->setItem( i-firstItem, index++, new QTableWidgetItem(cr2qt(lString16::itoa(i - firstItem + 1 ))));
        m_ui->tableWidget->setItem( i-firstItem, index++, new QTableWidgetItem(cr2qt(author)));
        m_ui->tableWidget->setItem( i-firstItem, index++, new QTableWidgetItem(cr2qt(title)));
        m_ui->tableWidget->setItem( i-firstItem, index++, new QTableWidgetItem(cr2qt(filename)));
        m_ui->tableWidget->verticalHeader()->setResizeMode( i-firstItem, QHeaderView::ResizeToContents );
        //CRRecentBookMenuItem * item = new CRRecentBookMenuItem( this, i, file );
        //addItem( item );
    }
    m_ui->tableWidget->resizeRowsToContents();

    addAction( m_ui->actionRemoveItem );

    m_docview->restoreWindowPos( this, "recentlist." );
}

void RecentBooksDlg::closeEvent ( QCloseEvent * event )
{
    m_docview->saveWindowPos( this, "recentlist." );
}

RecentBooksDlg::~RecentBooksDlg()
{
    delete m_ui;
}

bool RecentBooksDlg::showDlg( QWidget * parent,  CR3View * docView )
{
    RecentBooksDlg * dlg = new RecentBooksDlg( parent, docView );
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    //dlg->
    return true;
}

void RecentBooksDlg::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void RecentBooksDlg::on_buttonBox_rejected()
{
    close();
}

void RecentBooksDlg::on_buttonBox_accepted()
{
    openBook( m_ui->tableWidget->currentRow() );
}

void RecentBooksDlg::openBook( int rowIndex )
{
    if ( rowIndex < 0 || rowIndex>=m_ui->tableWidget->rowCount() )
        return;
    QString s = m_ui->tableWidget->item(rowIndex, 0)->data(Qt::DisplayRole).toString();
    bool ok;
    int n = s.toInt(&ok, 10);
    if ( !ok )
        return;
    int firstItem = m_docview->getDocView()->isDocumentOpened() ? 1 : 0;
    int index = n - 1 + firstItem;
    LVPtrVector<CRFileHistRecord> & files = m_docview->getDocView()->getHistory()->getRecords();
    if ( index < 0 || index>=files.length() )
        return;
    // go to file
    QString fn = cr2qt(files[ index ]->getFilePathName());
    m_docview->loadDocument( fn );
    close();
}

void RecentBooksDlg::on_tableWidget_doubleClicked(QModelIndex index)
{
    openBook( index.row() );
}

void RecentBooksDlg::on_tableWidget_customContextMenuRequested(QPoint pos)
{
    QMenu *menu = new QMenu;
    menu->addAction(m_ui->actionRemoveItem);
    menu->addAction(m_ui->actionClearAll);
    menu->exec(mapToGlobal(pos));
}

void RecentBooksDlg::on_actionRemoveItem_triggered()
{
    int firstItem = m_docview->getDocView()->isDocumentOpened() ? 1 : 0;
    QModelIndex index = m_ui->tableWidget->currentIndex();
    //int index = m_ui->tableWidget->
    int r = index.row();
    LVPtrVector<CRFileHistRecord> & files = m_docview->getDocView()->getHistory()->getRecords();
    if ( r>=0 && r<files.length()-firstItem ) {
        files.remove( r + firstItem );
        m_ui->tableWidget->removeRow( r );
    }
}

void RecentBooksDlg::on_actionClearAll_triggered()
{
    //
    if ( QMessageBox::question(this, tr("Remove all history items"), tr("Do you really want to remove all history records?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes ) {
        int firstItem = m_docview->getDocView()->isDocumentOpened() ? 1 : 0;
        LVPtrVector<CRFileHistRecord> & files = m_docview->getDocView()->getHistory()->getRecords();
        for ( int r=files.length(); r>=firstItem; r-- ) {
            files.remove( r);
            m_ui->tableWidget->removeRow( r - firstItem );
        }
        close();
    }
}
