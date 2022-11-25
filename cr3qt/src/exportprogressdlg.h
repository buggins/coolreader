/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2010,2014 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef EXPORTPROGRESSDLG_H
#define EXPORTPROGRESSDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

namespace Ui {
    class ExportProgressDlg;
}

class ExportProgressDlg : public QDialog {
    Q_OBJECT
public:
    ExportProgressDlg(QWidget *parent = 0);
    ~ExportProgressDlg();

    void setPercent( int n );

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ExportProgressDlg *m_ui;
};

#endif // EXPORTPROGRESSDLG_H
