#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QDialog>
#else
#include <QtGui/QDialog>
#endif

namespace Ui {
    class AboutDialog;
}

class AboutDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(AboutDialog)
public:
    explicit AboutDialog(QWidget *parent = 0);
    virtual ~AboutDialog();

    static bool showDlg( QWidget * parent );

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::AboutDialog *m_ui;

private slots:
//    void on_btnSite_clicked();
    void on_buttonBox_accepted();
};

#endif // ABOUTDIALOG_H
