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

#include "exportprogressdlg.h"
#include "ui_exportprogressdlg.h"

ExportProgressDlg::ExportProgressDlg(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::ExportProgressDlg)
{
    m_ui->setupUi(this);
    m_ui->progressBar->setRange(0, 100);
}

ExportProgressDlg::~ExportProgressDlg()
{
    delete m_ui;
}

void ExportProgressDlg::setPercent( int n )
{
    m_ui->progressBar->setValue(n);
    repaint();
    m_ui->progressBar->repaint();
}

void ExportProgressDlg::changeEvent(QEvent *e)
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
