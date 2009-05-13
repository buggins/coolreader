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
