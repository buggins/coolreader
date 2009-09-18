#ifndef BOOKMARKLISTDLG_H
#define BOOKMARKLISTDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class BookmarkListDialog;
}

class CR3View;

class BookmarkListDialog : public QDialog {
    Q_OBJECT
public:
    ~BookmarkListDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    explicit BookmarkListDialog(QWidget *parent, CR3View * docView);
    void changeEvent(QEvent *e);

private:
    Ui::BookmarkListDialog *m_ui;
    CR3View * _docview;

private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // BOOKMARKLISTDLG_H
