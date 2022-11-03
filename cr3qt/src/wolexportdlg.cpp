/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#include "wolexportdlg.h"
#include "ui_wolexportdlg.h"

WolExportDlg::WolExportDlg(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::WolExportDlg)
{
    m_bpp = 2;
    m_tocLevels = 3;
    m_ui->setupUi(this);
    m_ui->cbBitsPerPixel->setCurrentIndex(1);
    m_ui->cbTocLevels->setCurrentIndex(2);
}

WolExportDlg::~WolExportDlg()
{
    delete m_ui;
}

void WolExportDlg::changeEvent(QEvent *e)
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

void WolExportDlg::on_cbBitsPerPixel_currentIndexChanged(int index)
{
    m_bpp = index+1;
}

void WolExportDlg::on_cbTocLevels_currentIndexChanged(int index)
{
    m_tocLevels = index+1;
}

void WolExportDlg::on_buttonBox_accepted()
{
    accept();
}
