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

#ifndef WOLEXPORTDLG_H
#define WOLEXPORTDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

namespace Ui {
    class WolExportDlg;
}

class WolExportDlg : public QDialog {
    Q_OBJECT
public:
    WolExportDlg(QWidget *parent = 0);
    ~WolExportDlg();

    int getBitsPerPixel() { return m_bpp; }
    int getTocLevels() { return m_tocLevels; }
protected:
    void changeEvent(QEvent *e);

private:
    Ui::WolExportDlg *m_ui;
    int m_tocLevels;
    int m_bpp;

private slots:
    void on_buttonBox_accepted();
    void on_cbTocLevels_currentIndexChanged(int index);
    void on_cbBitsPerPixel_currentIndexChanged(int index);
};

#endif // WOLEXPORTDLG_H
