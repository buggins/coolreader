#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <QtGui/QDialog>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>

#include "crqtutil.h"

namespace Ui {
    class SettingsDlg;
}


#define PROP_WINDOW_RECT            "window.rect"
#define PROP_WINDOW_FULLSCREEN      "window.fullscreen"
#define PROP_WINDOW_MINIMIZED       "window.minimized"
#define PROP_WINDOW_MAXIMIZED       "window.maximized"
#define PROP_WINDOW_SHOW_MENU       "window.menu.show"
//#define PROP_WINDOW_ROTATE_ANGLE  "window.rotate.angle"
#define PROP_WINDOW_TOOLBAR_SIZE    "window.toolbar.size"
#define PROP_WINDOW_TOOLBAR_POSITION "window.toolbar.position"
#define PROP_WINDOW_SHOW_STATUSBAR  "window.statusbar.show"
#define PROP_WINDOW_SHOW_SCROLLBAR  "window.scrollbar.show"
#define PROP_WINDOW_STYLE           "window.style"
#define PROP_APP_START_ACTION       "cr3.app.start.action"

#define DECL_DEF_CR_FONT_SIZES static int cr_font_sizes[] = { 14, 18, 20, 22, 24, 26, 28, 32, 38, 42, 48 }

class CR3View;
class SettingsDlg : public QDialog {
    Q_OBJECT
    Q_DISABLE_COPY(SettingsDlg)
public:
    virtual ~SettingsDlg();

    static bool showDlg(  QWidget * parent, CR3View * docView );
protected:
    explicit SettingsDlg(QWidget *parent, CR3View * docView );
    virtual void changeEvent(QEvent *e);

    void setCheck( const char * optionName, int checkState );
    void optionToUi( const char * optionName, QCheckBox * cb );
    void setCheckInversed( const char * optionName, int checkState );
    void optionToUiInversed( const char * optionName, QCheckBox * cb );
    void fontToUi( const char * faceOptionName, const char * sizeOptionName, QComboBox * faceCombo, QComboBox * sizeCombo, const char * defFontFace );

    QColor getColor( const char * optionName, unsigned def );
    void setColor( const char * optionName, QColor cl );
    void colorDialog( const char * optionName, QString title );

    void setBackground( QWidget * wnd, QColor cl );
    void updateStyleSample();

private:
    Ui::SettingsDlg *m_ui;
    CR3View * m_docview;
    PropsRef m_props;
    QString m_oldHyph;

private slots:
    void on_cbTxtPreFormatted_stateChanged(int );
    void on_cbTxtPreFormatted_toggled(bool checked);
    void on_cbStartupAction_currentIndexChanged(int index);
    void on_cbHyphenation_currentIndexChanged(int index);
    void on_cbInterlineSpace_currentIndexChanged(int index);
    void on_cbTextFontSize_currentIndexChanged(QString );
    void on_cbTextFontFace_currentIndexChanged(QString );
    void on_cbTitleFontSize_currentIndexChanged(QString );
    void on_cbTitleFontFace_currentIndexChanged(QString );
    void on_cbLookAndFeel_currentIndexChanged(QString );
    void on_btnHeaderTextColor_clicked();
    void on_btnBgColor_clicked();
    void on_btnTextColor_clicked();
    void on_cbMargins_currentIndexChanged(int index);
    void on_cbShowFootNotes_stateChanged(int s);
    void on_cbShowBattery_stateChanged(int s);
    void on_cbShowClock_stateChanged(int s);
    void on_cbShowBookName_stateChanged(int s);
    void on_cbShowPageHeader_stateChanged(int s);
    void on_cbViewMode_currentIndexChanged(int index);
    void on_cbWindowShowScrollbar_stateChanged(int );
    void on_cbWindowShowStatusBar_stateChanged(int );
    void on_cbWindowShowMenu_stateChanged(int );
    void on_cbWindowShowToolbar_stateChanged(int );
    void on_cbWindowFullscreen_stateChanged(int );
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
};

#endif // SETTINGSDLG_H
