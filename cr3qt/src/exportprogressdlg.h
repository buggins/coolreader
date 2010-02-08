#ifndef EXPORTPROGRESSDLG_H
#define EXPORTPROGRESSDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class ExportProgressDlg;
}

class ExportProgressDlg : public QDialog {
    Q_OBJECT
public:
    ExportProgressDlg(QWidget *parent = 0);
    ~ExportProgressDlg();

    void setPercent( int n );

protected:
    void changeEvent(QEvent *e);

private:
    Ui::ExportProgressDlg *m_ui;
};

#endif // EXPORTPROGRESSDLG_H
