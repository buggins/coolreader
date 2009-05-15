#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);
    ui->view->setScrollBar( ui->scroll );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_view_destroyed()
{
    //
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open book file"),
         "",
         tr("All supported formats (*.fb2 *.txt *.tcr *.rtf *.epub *.html *.htm *.zip);;FB2 books (*.fb2 *.fb2.zip);;Text files (*.txt);;Rich text (*.rtf);;HTML files (*.htm *.html);;EPUB files (*.epub);;ZIP archives (*.zip)"));
    if ( !ui->view->loadDocument( fileName ) ) {
        // error
    } else {
        update();
    }
}

void MainWindow::on_actionMinimize_triggered()
{
    showMinimized();
}

void MainWindow::on_actionClose_triggered()
{
    close();
}

void MainWindow::on_actionNextPage_triggered()
{
    ui->view->nextPage();
}

void MainWindow::on_actionPrevPage_triggered()
{
    ui->view->prevPage();
}

void MainWindow::on_actionNextLine_triggered()
{
    ui->view->nextLine();
}

void MainWindow::on_actionPrevLine_triggered()
{
    ui->view->prevLine();
}

void MainWindow::on_actionFirstPage_triggered()
{
    ui->view->firstPage();
}

void MainWindow::on_actionLastPage_triggered()
{
    ui->view->lastPage();
}

void MainWindow::on_actionBack_triggered()
{
    ui->view->historyBack();
}

void MainWindow::on_actionForward_triggered()
{
    ui->view->historyForward();
}

void MainWindow::on_actionNextChapter_triggered()
{
    ui->view->nextChapter();
}

void MainWindow::on_actionPrevChapter_triggered()
{
    ui->view->prevChapter();
}

void MainWindow::on_actionToggle_Pages_Scroll_triggered()
{
    ui->view->togglePageScrollView();
}
