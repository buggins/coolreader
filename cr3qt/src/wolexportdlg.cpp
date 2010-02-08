#include "wolexportdlg.h"
#include "ui_wolexportdlg.h"

WolExportDlg::WolExportDlg(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::WolExportDlg)
{
    m_bpp = 2;
    m_tocLevels = 3;
    m_ui->setupUi(this);
    m_ui->cbBitsPerPixel->setCurrentIndex(1);
    m_ui->cbTocLevels->setCurrentIndex(2);
}

WolExportDlg::~WolExportDlg()
{
    delete m_ui;
}

void WolExportDlg::changeEvent(QEvent *e)
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

void WolExportDlg::on_cbBitsPerPixel_currentIndexChanged(int index)
{
    m_bpp = index+1;
}

void WolExportDlg::on_cbTocLevels_currentIndexChanged(int index)
{
    m_tocLevels = index+1;
}

void WolExportDlg::on_buttonBox_accepted()
{
    accept();
}
