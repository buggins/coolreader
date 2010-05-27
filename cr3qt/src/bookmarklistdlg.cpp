#include "bookmarklistdlg.h"
#include "ui_bookmarklistdlg.h"
#include "cr3widget.h"
#include "addbookmarkdlg.h"
#include <QMenu>

#define MAX_ITEM_LEN 50
static QString limit( QString s )
{
    if ( s.length()<MAX_ITEM_LEN )
        return s;
    s = s.left( MAX_ITEM_LEN );
    return s + "...";
}

BookmarkListDialog::BookmarkListDialog(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::BookmarkListDialog),
    _docview(docView)
{
    m_ui->setupUi(this);
    setWindowTitle( tr("Bookmarks") );

    m_ui->tableWidget->setColumnCount(4);
    m_ui->tableWidget->setHorizontalHeaderLabels ( QStringList() << tr("Position") << tr("Type", "bookmark type") /*<< tr("Chapter") */ << tr("Text") << tr("Comment") );
    m_ui->tableWidget->verticalHeader()->hide();
    int i = 0;
    m_ui->tableWidget->horizontalHeader()->setResizeMode( i++, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( i++, QHeaderView::ResizeToContents );
    //m_ui->tableWidget->horizontalHeader()->setResizeMode( i++, QHeaderView::Stretch );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( i++, QHeaderView::Stretch );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( i++, QHeaderView::Stretch );
    //m_ui->tableWidget->horizontalHeader()->setResizeMode( 1, QHeaderView::Stretch ); //Stretch
    m_ui->tableWidget->horizontalHeader()->setStretchLastSection( true );
    m_ui->tableWidget->horizontalHeader()->setDefaultAlignment( Qt::AlignLeft );
    m_ui->tableWidget->setEditTriggers( QAbstractItemView::NoEditTriggers );

    m_ui->tableWidget->setWordWrap(true);
    m_ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->tableWidget->setSortingEnabled(false);


    CRFileHistRecord * rec = _docview->getDocView()->getCurrentFileHistRecord();
    if ( !rec )
        return;
    LVPtrVector<CRBookmark> & list( rec->getBookmarks() );
    int curpercent = _docview->getDocView()->getPosPercent();
    int bestdiff = -1;
    int bestindex = -1;
    int y = 0;
    for ( int i=0; i<list.length(); i++ ) {
        CRBookmark * bm = list[i];
        if ( bm->getType() == bmkt_lastpos )
            continue;
        int diff = bm->getPercent() - curpercent;
        if ( diff<0 )
            diff = -diff;
        if ( bestindex==-1 || diff < bestdiff ) {
            bestindex = i;
            bestdiff = diff;
        }
        QString t = tr("P", "Bookmark type first letter - Position");
        if ( bm->getType()==bmkt_comment )
            t = tr("C", "Bookmark type first letter - Comment");
        else if ( bm->getType()==bmkt_correction )
            t = tr("E", "Bookmark type first letter - Correction/Edit");
        m_ui->tableWidget->setRowCount(y+1);
		{
			int i=0;
			_list.append( bm );
			m_ui->tableWidget->setItem( y, i++, new QTableWidgetItem( crpercent( bm->getPercent() )) );
			m_ui->tableWidget->setItem( y, i++, new QTableWidgetItem(t) );
			//m_ui->tableWidget->setItem( y, i++, new QTableWidgetItem( limit(cr2qt(bm->getTitleText())) ) );
			m_ui->tableWidget->setItem( y, i++, new QTableWidgetItem( limit(cr2qt(bm->getPosText())) ) );
			m_ui->tableWidget->setItem( y, i++, new QTableWidgetItem( limit(cr2qt(bm->getCommentText())) ) );
			m_ui->tableWidget->verticalHeader()->setResizeMode( y, QHeaderView::ResizeToContents );
		}
        y++;
    }
    if ( bestindex>=0 ) {
        //m_ui->tableWidget->setCurrentCell( bestindex, 0, QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent );
        m_ui->tableWidget->selectRow( bestindex );
    }

    m_ui->tableWidget->resizeColumnsToContents();
    m_ui->tableWidget->resizeRowsToContents();

    _docview->restoreWindowPos( this, "bookmarklist." );
}

void BookmarkListDialog::closeEvent ( QCloseEvent * event )
{
    _docview->saveWindowPos( this, "bookmarklist." );
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

void BookmarkListDialog::on_tableWidget_customContextMenuRequested(QPoint pos)
{
    QMenu *menu = new QMenu;
    CRBookmark * bm = selectedBookmark();
    if ( bm != NULL ) {
        menu->addAction(m_ui->actionGoToBookmark);
        menu->addAction(m_ui->actionEdit_Bookmark);
        menu->addAction(m_ui->actionRemoveBookmark);
    }
    if ( _list.length()>0 )
        menu->addAction(m_ui->actionRemoveALLBookmarks);
    menu->addAction(m_ui->actionClose);
    menu->exec(mapToGlobal(pos));
}

CRBookmark * BookmarkListDialog::selectedBookmark()
{
    int index = m_ui->tableWidget->currentRow();
    if ( index<0 || index>=_list.length() || index>=m_ui->tableWidget->rowCount() )
        return NULL;
    return _list[index];
}

void BookmarkListDialog::on_actionGoToBookmark_triggered()
{
    CRBookmark * bm = selectedBookmark();
    if ( bm ) {
        //_docview->goToXPointer( cr2qt(bm->getStartPos()) );
        _docview->goToBookmark( bm );
    }
    close();
}

void BookmarkListDialog::on_actionRemoveBookmark_triggered()
{
    CRBookmark * bm = selectedBookmark();
    if ( bm ) {
        int index = m_ui->tableWidget->currentRow();
        m_ui->tableWidget->removeRow( index );
        _list.removeAt( index );
        _docview->getDocView()->removeBookmark( bm );
    }
}

void BookmarkListDialog::on_actionRemoveALLBookmarks_triggered()
{

}

void BookmarkListDialog::on_actionEdit_Bookmark_triggered()
{
    CRBookmark * bm = selectedBookmark();
    if ( bm ) {
        _docview->goToBookmark( bm );
        AddBookmarkDialog::editBookmark( parentWidget(), _docview, bm );
        close();
    }
}

void BookmarkListDialog::on_actionClose_triggered()
{
    close();
}

void BookmarkListDialog::on_tableWidget_doubleClicked(QModelIndex index)
{
    on_actionGoToBookmark_triggered();
}
