#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class SettingsDlg;
}

class CR3View;
class SettingsDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(SettingsDlg)
public:
    virtual ~SettingsDlg();

    static bool showDlg( CR3View * docView );
protected:
    explicit SettingsDlg(QWidget *parent, CR3View * docView );
    virtual void changeEvent(QEvent *e);

private:
    Ui::SettingsDlg *m_ui;
    CR3View * m_docview;

private slots:
    void on_cbWindowFullscreen_stateChanged(int );
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
};

#endif // SETTINGSDLG_H
