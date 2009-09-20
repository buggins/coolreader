#ifndef RecentBooksDlg_H
#define RecentBooksDlg_H

#include <QtGui/QDialog>
#include <QModelIndex>

namespace Ui {
    class RecentBooksDlg;
}

class CR3View;

class RecentBooksDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(RecentBooksDlg)
public:
    virtual ~RecentBooksDlg();

    static bool showDlg( QWidget * parent, CR3View * docView );
protected:
    explicit RecentBooksDlg(QWidget *parent, CR3View * docView );
    virtual void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::RecentBooksDlg *m_ui;
    CR3View * m_docview;
    void openBook( int index );
private slots:
    void on_actionClearAll_triggered();
    void on_actionRemoveItem_triggered();
    void on_tableWidget_customContextMenuRequested(QPoint pos);
    void on_tableWidget_doubleClicked(QModelIndex index);
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // RecentBooksDlg_H
