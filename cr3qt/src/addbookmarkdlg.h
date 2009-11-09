#ifndef ADDBOOKMARKDLG_H
#define ADDBOOKMARKDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class AddBookmarkDialog;
}

class CR3View;
class CRBookmark;

class AddBookmarkDialog : public QDialog {
    Q_OBJECT
public:
    ~AddBookmarkDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );
    static bool editBookmark( QWidget * parent, CR3View * docView, CRBookmark * bm );

protected:
    explicit AddBookmarkDialog(QWidget *parent, CR3View * docView, CRBookmark * bm);
    void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::AddBookmarkDialog *m_ui;
    CR3View * _docview;
    CRBookmark * _bm;
    bool _edit;

private slots:
    void on_cbType_currentIndexChanged(int index);
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // ADDBOOKMARKDLG_H
