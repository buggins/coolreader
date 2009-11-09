#ifndef TOCDLG_H
#define TOCDLG_H

#include <QtGui/QDialog>
#include <QModelIndex>

namespace Ui {
    class TocDlg;
}

class CR3View;

class TocDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(TocDlg)
public:
    virtual ~TocDlg();

    static bool showDlg(  QWidget * parent, CR3View * docView );

protected:
    explicit TocDlg(QWidget *parent, CR3View * docView );
    virtual void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );

private:
    Ui::TocDlg *m_ui;
    CR3View * m_docview;

private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
    void on_treeWidget_doubleClicked(QModelIndex index);
};

#endif // TOCDLG_H
