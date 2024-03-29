/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2009,2014 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef BOOKMARKLISTDLG_H
#define BOOKMARKLISTDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif
#include <QModelIndex>

namespace Ui {
    class BookmarkListDialog;
}

class CR3View;
class CRBookmark;

class BookmarkListDialog : public QDialog {
    Q_OBJECT
public:
    ~BookmarkListDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    explicit BookmarkListDialog(QWidget *parent, CR3View * docView);
    void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::BookmarkListDialog *m_ui;
    CR3View * _docview;
    QList<CRBookmark*> _list;
    CRBookmark * selectedBookmark();

private slots:
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_actionClose_triggered();
    void on_actionEdit_Bookmark_triggered();
    void on_actionRemoveALLBookmarks_triggered();
    void on_actionRemoveBookmark_triggered();
    void on_actionGoToBookmark_triggered();
    void on_tableWidget_customContextMenuRequested(QPoint pos);
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // BOOKMARKLISTDLG_H
