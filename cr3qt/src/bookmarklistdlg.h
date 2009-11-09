#ifndef BOOKMARKLISTDLG_H
#define BOOKMARKLISTDLG_H

#include <QtGui/QDialog>
#include <QModelIndex>

namespace Ui {
    class BookmarkListDialog;
}

class CR3View;
class CRBookmark;

class BookmarkListDialog : public QDialog {
    Q_OBJECT
public:
    ~BookmarkListDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    explicit BookmarkListDialog(QWidget *parent, CR3View * docView);
    void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::BookmarkListDialog *m_ui;
    CR3View * _docview;
    QList<CRBookmark*> _list;
    CRBookmark * selectedBookmark();

private slots:
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_actionClose_triggered();
    void on_actionEdit_Bookmark_triggered();
    void on_actionRemoveALLBookmarks_triggered();
    void on_actionRemoveBookmark_triggered();
    void on_actionGoToBookmark_triggered();
    void on_tableWidget_customContextMenuRequested(QPoint pos);
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // BOOKMARKLISTDLG_H
