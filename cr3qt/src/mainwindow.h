/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2009-2011,2014 Vadim Lopatin <coolreader.org@gmail.com> *
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QMainWindow>
#else
#include <QtGui/QMainWindow>
#endif
#include "settings.h"
#include "cr3widget.h"

namespace Ui
{
    class MainWindowClass;
}

class MainWindow : public QMainWindow, public PropsChangeCallback
{
    Q_OBJECT

public:
    virtual void onPropsChange( PropsRef props );
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindowClass *ui;
    QString _filenameToOpen;
    void toggleProperty( const char * name );
protected:
    virtual void showEvent ( QShowEvent * event );
    virtual void focusInEvent ( QFocusEvent * event );
    virtual void closeEvent ( QCloseEvent * event );
public slots:
    void exportSentenceInfo(QString inputFileName, QString outputFileName);
    void contextMenu( QPoint pos );
void on_actionFindText_triggered();
    private slots:
    void on_actionNextPage3_triggered();
    void on_actionToggleEditMode_triggered();
    void on_actionRotate_triggered();
    void on_actionFileProperties_triggered();
    void on_actionShowBookmarksList_triggered();
    void on_actionAddBookmark_triggered();
    void on_actionAboutCoolReader_triggered();
    void on_actionAboutQT_triggered();
    void on_actionCopy2_triggered();
    void on_actionCopy_triggered();
    void on_actionSettings_triggered();
    void on_actionRecentBooks_triggered();
    void on_actionTOC_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionZoom_In_triggered();
    void on_actionToggle_Full_Screen_triggered();
    void on_actionToggle_Pages_Scroll_triggered();
    void on_actionPrevChapter_triggered();
    void on_actionNextChapter_triggered();
    void on_actionForward_triggered();
    void on_actionBack_triggered();
    void on_actionLastPage_triggered();
    void on_actionFirstPage_triggered();
    void on_actionPrevLine_triggered();
    void on_actionNextLine_triggered();
    void on_actionPrevPage_triggered();
    void on_actionNextPage_triggered();
    void on_actionPrevPage2_triggered();
    void on_actionNextPage2_triggered();
    void on_actionClose_triggered();
    void on_actionMinimize_triggered();
    void on_actionOpen_triggered();
    void on_actionExport_triggered();
    void on_view_destroyed();
    void on_actionNextSentence_triggered();
    void on_actionPrevSentence_triggered();
};

#endif // MAINWINDOW_H
