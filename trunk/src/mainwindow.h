#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

namespace Ui
{
    class MainWindowClass;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindowClass *ui;

private slots:
    void on_actionClose_triggered();
    void on_actionMinimize_triggered();
    void on_actionOpen_triggered();
    void on_view_destroyed();
};

#endif // MAINWINDOW_H
