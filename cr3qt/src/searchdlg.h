#ifndef SEARCHDLG_H
#define SEARCHDLG_H

#include <QDialog>

namespace Ui {
    class SearchDialog;
}

class CR3View;

class SearchDialog : public QDialog {
    Q_OBJECT
public:
    SearchDialog(QWidget *parent, CR3View * docView);
    ~SearchDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    void changeEvent(QEvent *e);

private:
    Ui::SearchDialog *ui;
    CR3View * _docview;
private slots:
    void on_btnFindNext_clicked();
    void on_btnClose_clicked();
};

#endif // SEARCHDLG_H
