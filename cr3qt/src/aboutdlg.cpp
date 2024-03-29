/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2018 Sergey Torokhov <torokhov-s-a@yandex.ru>           *
 *   Copyright (C) 2018,2022 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "aboutdlg.h"
#include "ui_aboutdlg.h"
#include <cr3version.h>
#include <QDesktopServices>
#include <QTextBrowser>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::AboutDialog)
{
    m_ui->setupUi(this);
    m_ui->lblVersion->setText(QString("CoolReader %1").arg(CR_ENGINE_VERSION));
    m_ui->lblDate->setText(CR_ENGINE_BUILD_DATE);
    QString project_src_url = "https://github.com/buggins/coolreader/";
    QString aboutText
        = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
          "\"http://www.w3.org/TR/REC-html40/strict.dtd\">"
          "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
          "p, li { white-space: pre-wrap; margin-top:0px; margin-bottom:0px; margin-left:0px; "
          "margin-right:0px; -qt-block-indent:0; text-indent:0px; }"
          "</style></head><body>";
    aboutText += "<p>"
                 + tr("CoolReader is free open source e-book viewer based on CoolReader engine.")
                 + "</p>";
    aboutText += "<p>" + tr("Source code is available at")
                 + QString(" <a href=\"%1\">%1</a> ").arg(project_src_url)
                 + tr("under the terms of GNU GPL license either version 2 or (at your option) any "
                      "later version.")
                 + "</p>";
    aboutText += "<p style=\"-qt-paragraph-type:empty;\"><br/></p>";
    aboutText += "<p><span style=\"text-decoration: underline;\">"
                 + tr("Third party components used:") + "</span></p>";
    aboutText += "<p>" + QString("Qt %1 - ").arg(QT_VERSION_MAJOR) + tr("as GUI library") + "</p>";
    aboutText += "<p>" + tr("FreeType - font rendering") + "</p>";
    aboutText += "<p>" + tr("HarfBuzz - text shaping, font kerning, ligatures") + "</p>";
    aboutText += "<p>" + tr("ZLib - compressing library") + "</p>";
    aboutText += "<p>" + tr("ZSTD - compressing library") + "</p>";
    aboutText += "<p>" + tr("libpng - PNG image format support") + "</p>";
    aboutText += "<p>" + tr("libjpeg - JPEG image format support") + "</p>";
    aboutText += "<p>" + tr("FriBiDi - RTL writing support") + "</p>";
    aboutText += "<p>" + tr("libunibreak - line breaking and word breaking algorithms") + "</p>";
    aboutText += "<p>" + tr("utf8proc - for unicode string comparision") + "</p>";
    aboutText += "<p>" + tr("NanoSVG - SVG image format support") + "</p>";
    aboutText += "<p>" + tr("chmlib - chm format support") + "</p>";
    aboutText += "<p>" + tr("antiword - Microsoft Word format support") + "</p>";
    aboutText += "<p>" + tr("hyphman - AlReader hyphenation manager") + "</p>";
    aboutText += "<p>" + tr("Most hyphenation dictionaries - TEX hyphenation patterns") + "</p>";
    aboutText += "<p>" + tr("Russian hyphenation dictionary - ") + QString("https://github.com/laboratory50/russian-spellpack") + "</p>";
    aboutText += "<p>" + tr("Languages character set database by Fontconfig") + "</p>";
    aboutText += "</body></html>";
    m_ui->textBrowser->setHtml(aboutText);

    QString sourceCode;
    sourceCode += tr("Source code may be downloaded from github CoolReader project page:") + "\n";
    sourceCode += project_src_url + "\n";
    sourceCode += "\n";
    sourceCode += tr("Latest source code is available from GIT repository:") + "\n";
    sourceCode += QString("git clone %1").arg(project_src_url) + "\n";
    sourceCode += "\n";
    sourceCode += tr("See README.md at root directory of project for build instructions.") + "\n";
    m_ui->plainTextEdit_2->setPlainText(sourceCode);
}

AboutDialog::~AboutDialog()
{
    delete m_ui;
}

void AboutDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

bool AboutDialog::showDlg( QWidget * parent )
{
    AboutDialog * dlg = new AboutDialog( parent );
    //dlg->setModal( true );
    dlg->setWindowTitle(tr("About CoolReader"));
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
	return true;
}


void AboutDialog::on_buttonBox_accepted()
{
    close();
}
