#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
    class AboutDialog;
}

class AboutDialog : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(AboutDialog)
public:
    explicit AboutDialog(QWidget *parent = 0);
    virtual ~AboutDialog();

    static bool showDlg();

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::AboutDialog *m_ui;
};

#endif // ABOUTDIALOG_H
