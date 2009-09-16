#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
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
    void toggleProperty( const char * name );

public slots:
    void contextMenu( QPoint pos );
private slots:
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
    void on_actionClose_triggered();
    void on_actionMinimize_triggered();
    void on_actionOpen_triggered();
    void on_view_destroyed();
};

#endif // MAINWINDOW_H
