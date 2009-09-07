#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QtGui/QDialog>
#include <QtGui/QCheckBox>

#include "crqtutil.h"

namespace Ui {
    class SettingsDlg;
}


#define PROP_WINDOW_RECT            "window.rect"
#define PROP_WINDOW_FULLSCREEN      "window.fullscreen"
#define PROP_WINDOW_MINIMIZED       "window.minimized"
#define PROP_WINDOW_MAXIMIZED       "window.maximized"
#define PROP_WINDOW_SHOW_MENU       "window.menu.show"
//#define PROP_WINDOW_ROTATE_ANGLE    "window.rotate.angle"
#define PROP_WINDOW_TOOLBAR_SIZE    "window.toolbar.size"
#define PROP_WINDOW_TOOLBAR_POSITION "window.toolbar.position"
#define PROP_WINDOW_SHOW_STATUSBAR  "window.statusbar.show"
#define PROP_WINDOW_SHOW_SCROLLBAR  "window.scrollbar.show"

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

    void setCheck( const char * optionName, int checkState );
    void optionToUi( const char * optionName, QCheckBox * cb );
private:
    Ui::SettingsDlg *m_ui;
    CR3View * m_docview;
    PropsRef m_props;

private slots:
    void on_cbWindowShowScrollbar_stateChanged(int );
    void on_cbWindowShowStatusBar_stateChanged(int );
    void on_cbWindowShowMenu_stateChanged(int );
    void on_cbWindowShowToolbar_stateChanged(int );
    void on_cbWindowFullscreen_stateChanged(int );
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
};

#endif // SETTINGSDLG_H
