/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2020 Daniel Bedrenko <d.bedrenko@gmail.com>             *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef SEARCHDLG_H
#define SEARCHDLG_H

#include <lvstring.h>
#include <QDialog>

namespace Ui {
    class SearchDialog;
}

class CR3View;

class SearchDialog : public QDialog {
    Q_OBJECT
public:
    static bool showDlg( QWidget * parent, CR3View * docView );
    bool findText( lString32 pattern, int origin, bool reverse, bool caseInsensitive );
protected:
    SearchDialog(QWidget *parent, CR3View * docView);
    ~SearchDialog();
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent* e);

private:
    Ui::SearchDialog *ui;
    CR3View * _docview;
    lString32 _lastPattern;
    static SearchDialog* _instance;
private slots:
    void on_btnFindNext_clicked();
    void on_btnClose_clicked();
};

#endif // SEARCHDLG_H
