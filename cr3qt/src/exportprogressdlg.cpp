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
