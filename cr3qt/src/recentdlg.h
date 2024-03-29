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

#ifndef RecentBooksDlg_H
#define RecentBooksDlg_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif
#include <QModelIndex>

namespace Ui {
    class RecentBooksDlg;
}

class CR3View;

class RecentBooksDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(RecentBooksDlg)
public:
    virtual ~RecentBooksDlg();

    static bool showDlg( QWidget * parent, CR3View * docView );
protected:
    explicit RecentBooksDlg(QWidget *parent, CR3View * docView );
    virtual void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::RecentBooksDlg *m_ui;
    CR3View * m_docview;
    void openBook( int index );
private slots:
    void on_actionClearAll_triggered();
    void on_actionRemoveItem_triggered();
    void on_tableWidget_customContextMenuRequested(QPoint pos);
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // RecentBooksDlg_H
