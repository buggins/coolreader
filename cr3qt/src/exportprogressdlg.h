#ifndef EXPORTPROGRESSDLG_H
#define EXPORTPROGRESSDLG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

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
