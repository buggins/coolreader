#include "bookmarklistdlg.h"
#include "ui_bookmarklistdlg.h"
#include "cr3widget.h"

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

    m_ui->tableWidget->setColumnCount(5);
    m_ui->tableWidget->setHorizontalHeaderLabels ( QStringList() << tr("Position") << tr("Type", "bookmark type") << tr("Chapter")  << tr("Text") << tr("Comment") );
    m_ui->tableWidget->verticalHeader()->hide();
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 0, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 1, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 2, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 3, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 4, QHeaderView::ResizeToContents );
    //m_ui->tableWidget->horizontalHeader()->setResizeMode( 1, QHeaderView::Stretch ); //Stretch
    //m_ui->tableWidget->horizontalHeader()->setStretchLastSection( true );
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
    int y = 0;
    for ( int i=0; i<list.length(); i++ ) {
        CRBookmark * bm = list[i];
        if ( bm->getType() == bmkt_lastpos )
            continue;
        QString t = tr("P", "Bookmark type first letter - Position");
        if ( bm->getType()==bmkt_comment )
            t = tr("C", "Bookmark type first letter - Comment");
        else if ( bm->getType()==bmkt_correction )
            t = tr("E", "Bookmark type first letter - Correction/Edit");
        m_ui->tableWidget->setRowCount(y+1);
        m_ui->tableWidget->setItem( y, 1, new QTableWidgetItem( crpercent( bm->getPercent() )) );
        m_ui->tableWidget->setItem( y, 0, new QTableWidgetItem(t) );
        m_ui->tableWidget->setItem( y, 2, new QTableWidgetItem( limit(cr2qt(bm->getTitleText())) ) );
        m_ui->tableWidget->setItem( y, 3, new QTableWidgetItem( limit(cr2qt(bm->getPosText())) ) );
        m_ui->tableWidget->setItem( y, 4, new QTableWidgetItem( limit(cr2qt(bm->getCommentText())) ) );
        m_ui->tableWidget->verticalHeader()->setResizeMode( y, QHeaderView::ResizeToContents );
        y++;
    }
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
