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

#ifndef ADDBOOKMARKDLG_H
#define ADDBOOKMARKDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

namespace Ui {
    class AddBookmarkDialog;
}

class CR3View;
class CRBookmark;

class AddBookmarkDialog : public QDialog {
    Q_OBJECT
public:
    ~AddBookmarkDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );
    static bool editBookmark( QWidget * parent, CR3View * docView, CRBookmark * bm );

protected:
    explicit AddBookmarkDialog(QWidget *parent, CR3View * docView, CRBookmark * bm);
    void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::AddBookmarkDialog *m_ui;
    CR3View * _docview;
    CRBookmark * _bm;
    bool _edit;

private slots:
    void on_cbType_currentIndexChanged(int index);
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // ADDBOOKMARKDLG_H
