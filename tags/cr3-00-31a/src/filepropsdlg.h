#ifndef FILEPROPSDLG_H
#define FILEPROPSDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class FilePropsDialog;
}

class CR3View;

class FilePropsDialog : public QDialog {
    Q_OBJECT
public:
    ~FilePropsDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    explicit FilePropsDialog(QWidget *parent, CR3View * docView );
    void changeEvent(QEvent *e);

private:
    Ui::FilePropsDialog *m_ui;
};

#endif // FILEPROPSDLG_H
