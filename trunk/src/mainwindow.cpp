#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include "settings.h"
#include "tocdlg.h"
#include "recentdlg.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);
    ui->view->setScrollBar( ui->scroll );
#ifdef _LINUX
    QString homeDir = QDir::toNativeSeparators(QDir::homePath() + "/.cr3/");
#else
    QString homeDir = QDir::toNativeSeparators(QDir::homePath() + "/cr3/");
#endif
    QString exeDir = qApp->applicationDirPath() + QDir::separator();
    QString histFile = homeDir + "cr3hist.bmk";
    QString iniFile = homeDir + "cr3.ini";
    QString cssFile = homeDir + "fb2.css";
    QString cssFile2 = exeDir + "fb2.css";
    ;
    ui->view->loadSettings( iniFile );
    ui->view->loadHistory( histFile );
    if ( !ui->view->loadCSS( cssFile ) )
        ui->view->loadCSS( cssFile2 );
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

void MainWindow::on_actionToggle_Full_Screen_triggered()
{
    setWindowState(windowState() ^ Qt::WindowFullScreen);
}

void MainWindow::on_actionZoom_In_triggered()
{
    ui->view->zoomIn();
}

void MainWindow::on_actionZoom_Out_triggered()
{
    ui->view->zoomOut();
}

void MainWindow::on_actionTOC_triggered()
{
    TocDlg::showDlg( ui->view );
}

void MainWindow::on_actionRecentBooks_triggered()
{
    RecentBooksDlg::showDlg( ui->view );
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDlg::showDlg( ui->view );
}
