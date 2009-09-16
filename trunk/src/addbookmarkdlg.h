#ifndef ADDBOOKMARKDLG_H
#define ADDBOOKMARKDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class AddBookmarkDialog;
}

class CR3View;

class AddBookmarkDialog : public QDialog {
    Q_OBJECT
public:
    ~AddBookmarkDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    explicit AddBookmarkDialog(QWidget *parent, CR3View * docView);
    void changeEvent(QEvent *e);

private:
    Ui::AddBookmarkDialog *m_ui;

private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // ADDBOOKMARKDLG_H
