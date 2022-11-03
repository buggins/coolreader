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

#ifndef FILEPROPSDLG_H
#define FILEPROPSDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

namespace Ui {
    class FilePropsDialog;
}

class CR3View;
class LVDocView;

class FilePropsDialog : public QDialog {
    Q_OBJECT
public:
    ~FilePropsDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    QString getDocText( const char * path, const char * delim );
    QString getDocAuthors( const char * path, const char * delim );
    void fillItems();
    void addPropLine( QString name, QString value );
    void addInfoSection( QString name );
    explicit FilePropsDialog(QWidget *parent, CR3View * docView );
    void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );
    QStringList prop;
    QStringList value;

private:
    Ui::FilePropsDialog *m_ui;
    CR3View * _cr3v;
    LVDocView * _docview;
};

#endif // FILEPROPSDLG_H
