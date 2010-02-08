#ifndef WOLEXPORTDLG_H
#define WOLEXPORTDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class WolExportDlg;
}

class WolExportDlg : public QDialog {
    Q_OBJECT
public:
    WolExportDlg(QWidget *parent = 0);
    ~WolExportDlg();

    int getBitsPerPixel() { return m_bpp; }
    int getTocLevels() { return m_tocLevels; }
protected:
    void changeEvent(QEvent *e);

private:
    Ui::WolExportDlg *m_ui;
    int m_tocLevels;
    int m_bpp;

private slots:
    void on_buttonBox_accepted();
    void on_cbTocLevels_currentIndexChanged(int index);
    void on_cbBitsPerPixel_currentIndexChanged(int index);
};

#endif // WOLEXPORTDLG_H
