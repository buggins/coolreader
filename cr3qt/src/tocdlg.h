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

#ifndef TOCDLG_H
#define TOCDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif
#include <QModelIndex>

namespace Ui {
    class TocDlg;
}

class CR3View;

class TocDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(TocDlg)
public:
    virtual ~TocDlg();

    static bool showDlg(  QWidget * parent, CR3View * docView );

protected:
    explicit TocDlg(QWidget *parent, CR3View * docView );
    virtual void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::TocDlg *m_ui;
    CR3View * m_docview;

private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
    void on_treeWidget_doubleClicked(QModelIndex index);
};

#endif // TOCDLG_H
