/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2009-2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018,2020 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include "tocdlg.h"
#include "ui_tocdlg.h"
#include "cr3widget.h"
#include "../crengine/include/lvdocview.h"
#include <QStandardItemModel>
#include "crqtutil.h"

class TocItem : public QTreeWidgetItem
{
        LVTocItem * _item;
    public:
        LVTocItem * getItem() { return _item; }
        TocItem( LVTocItem * item, int currPage, int & nearestPage, TocItem * & nearestItem )
                : QTreeWidgetItem( QStringList()
                                   << (item ? cr2qt( item->getName() ) : "No TOC items" )
                                   << ( item ? cr2qt(lString32::itoa(item->getPage()+1)) : "")
                                   )
                , _item( item )
        {
            if (item) {
                int page = item->getPage();
                if ( !nearestItem || (page <= currPage && page > nearestPage) ) {
                    nearestItem = this;
                    nearestPage = page;
                }

                setData( 0, Qt::UserRole, QVariant( cr2qt(item->getPath()) ) );
                for ( int i=0; i<item->getChildCount(); i++ ) {
                    addChild( new TocItem( item->getChild(i), currPage, nearestPage, nearestItem ) );
                }
            }
        }
};

#if QT_VERSION >= 0x050000
#define setResizeModeMethod setSectionResizeMode
#else
#define setResizeModeMethod setResizeMode
#endif

bool TocDlg::showDlg(  QWidget * parent, CR3View * docView )
{
    LVTocItem * root = docView->getToc();
    if ( !root || !root->getChildCount() )
        return false;
    TocDlg * dlg = new TocDlg( parent, docView );
    dlg->show();
    return true;
}

TocDlg::TocDlg(QWidget *parent, CR3View * docView) :
    QDialog(parent),
    m_ui(new Ui::TocDlg), m_docview(docView)
{
    setAttribute( Qt::WA_DeleteOnClose, true );
    m_ui->setupUi(this);
    m_ui->treeWidget->setColumnCount(2);
    m_ui->treeWidget->setHeaderItem(new QTreeWidgetItem(QStringList() << tr("Title") << tr("Page") ));
    m_ui->treeWidget->header()->setStretchLastSection(false);
    m_ui->treeWidget->header()->setResizeModeMethod( 0, QHeaderView::Stretch );
    m_ui->treeWidget->header()->setResizeModeMethod( 1, QHeaderView::ResizeToContents );

    int nearestPage = -1;
    int currPage = docView->getCurPage();
    TocItem * nearestItem = NULL;
    LVTocItem * root = m_docview->getToc();
    for ( int i=0; i<root->getChildCount(); i++ ) {
        m_ui->treeWidget->addTopLevelItem( new TocItem( root->getChild(i), currPage, nearestPage, nearestItem ) );
    }
    m_ui->treeWidget->expandAll();
    if ( nearestItem ) {
        m_ui->treeWidget->setCurrentItem( nearestItem );
    }
    m_docview->restoreWindowPos( this, "toc." );
}

void TocDlg::closeEvent ( QCloseEvent * event )
{
    m_docview->saveWindowPos( this, "toc." );
}

TocDlg::~TocDlg()
{
    delete m_ui;
}

void TocDlg::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TocDlg::on_treeWidget_doubleClicked(QModelIndex index)
{
    QString s = index.data(Qt::UserRole).toString();
    m_docview->goToXPointer(s);
    close();
}

void TocDlg::on_buttonBox_accepted()
{
    on_treeWidget_doubleClicked(m_ui->treeWidget->currentIndex());
}

void TocDlg::on_buttonBox_rejected()
{
    close();
}
