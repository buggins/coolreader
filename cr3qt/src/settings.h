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

#define DECL_DEF_CR_FONT_SIZES static int cr_font_sizes[] = { 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 28, 32, 38, 42, 48, 56, 64, 72 }


struct StyleItem {
    QString paramName;
    bool updating;
    QStringList values;
    QStringList titles;
    int currentIndex;
    QComboBox * cb;
    PropsRef props;
    void init(QString param, const char * defValue, const char * styleValues[], QString styleTitles[], bool hideFirstItem, PropsRef props, QComboBox * cb) {
        updating = true;
        paramName = param;
        this->cb = cb;
        this->props = props;
        currentIndex = -1;
        values.clear();
        titles.clear();
        //CRLog::trace("StyleItem::init %s", paramName.toUtf8().constData());

        QString currentValue = props->getStringDef(paramName.toUtf8().constData(), defValue);

        for (int i=!hideFirstItem ? 0 : 1; styleValues[i]; i++) {
            QString value = styleValues[i];
            if (currentValue == value)
                currentIndex = !hideFirstItem ? i : i-1;
            values.append(value);
            titles.append(styleTitles[i]);
        }
        if (currentIndex == -1)
            currentIndex = 0;
        cb->clear();
        cb->addItems(titles);
        cb->setCurrentIndex(currentIndex);
        lString8 pname(paramName.toUtf8().constData());
        props->setString(pname.c_str(), values.at(currentIndex));
        //CRLog::trace("StyleItem::init %s done", paramName.toUtf8().constData());
        updating = false;
    }

    void init(QString param, QStringList & styleValues, QStringList & styleTitles, PropsRef props, QComboBox * cb) {
        updating = true;
        paramName = param;
        this->cb = cb;
        this->props = props;
        currentIndex = -1;
        values.clear();
        titles.clear();
        values.append(styleValues);
        titles.append(styleTitles);

        QString currentValue = props->getStringDef(paramName.toUtf8().constData(), "");

        for (int i=0; i < styleValues.length(); i++) {
            if (currentValue == styleValues.at(i))
                currentIndex = i;
        }
        if (currentIndex == -1)
            currentIndex = 0;
        cb->clear();
        cb->addItems(titles);
        cb->setCurrentIndex(currentIndex);
        lString8 pname(paramName.toUtf8().constData());
        props->setString(pname.c_str(), values.at(currentIndex));
        updating = false;
    }

    void update(int newIndex) {
        //CRLog::trace("StyleItem::update %s %s", paramName.toUtf8().constData(), updating ? "updating" : "");
        if (updating)
            return;
        currentIndex = newIndex;
        lString8 pname(paramName.toUtf8().constData());
        props->setString(pname.c_str(), values.at(currentIndex));
        //CRLog::trace("StyleItem::update '%s' = '%s'", pname, pvalue);
    }
};

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
    void optionToUiString( const char * optionName, QComboBox * cb );
    void optionToUiIndex( const char * optionName, QComboBox * cb );
    void setCheckInversed( const char * optionName, int checkState );
    void optionToUiInversed( const char * optionName, QCheckBox * cb );
    void fontToUi( const char * faceOptionName, const char * sizeOptionName, QComboBox * faceCombo, QComboBox * sizeCombo, const char * defFontFace );

    void initStyleControls(const char * styleName);

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
    QStringList m_backgroundFiles;
    QStringList m_faceList;
    QString m_styleName;
    StyleItem m_styleItemAlignment;
    StyleItem m_styleItemIndent;
    StyleItem m_styleItemMarginBefore;
    StyleItem m_styleItemMarginAfter;
    StyleItem m_styleItemMarginLeft;
    StyleItem m_styleItemMarginRight;
    StyleItem m_styleFontFace;
    StyleItem m_styleFontSize;
    StyleItem m_styleFontWeight;
    StyleItem m_styleFontStyle;
    StyleItem m_styleFontColor;
    StyleItem m_styleLineHeight;
    StyleItem m_styleTextDecoration;
    StyleItem m_verticalAlignDecoration;
    QStringList m_styleNames;

private slots:
    void on_cbPageSkin_currentIndexChanged(int index);
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
    void on_cbFloatingPunctuation_stateChanged(int );
    void on_cbFontGamma_currentIndexChanged(QString );
    void on_cbStyleName_currentIndexChanged(int index);
    void on_cbDefAlignment_currentIndexChanged(int index);
    void on_cbDefFirstLine_currentIndexChanged(int index);
    void on_cbDefMarginBefore_currentIndexChanged(int index);
    void on_cbDefMarginAfter_currentIndexChanged(int index);
    void on_cbDefMarginLeft_currentIndexChanged(int index);
    void on_cbDefMarginRight_currentIndexChanged(int index);
    void on_cbDefFontSize_currentIndexChanged(int index);
    void on_cbDefFontFace_currentIndexChanged(int index);
    void on_cbDefFontWeight_currentIndexChanged(int index);
    void on_cbDefFontStyle_currentIndexChanged(int index);
    void on_cbDefFontColor_currentIndexChanged(int index);
    void on_cbFontHinting_currentIndexChanged(int index);
    void on_cbFallbackFontFace_currentIndexChanged(const QString &arg1);
    void on_cbEnableEmbeddedFonts_toggled(bool checked);
    void on_cbEnableDocumentStyles_toggled(bool checked);
    void on_btnSelectionColor_clicked();
    void on_btnCommentColor_clicked();
    void on_btnCorrectionColor_clicked();
    void on_cbBookmarkHighlightMode_currentIndexChanged(int index);
    void on_cbImageInlineZoominMode_currentIndexChanged(int index);
    void on_cbImageInlineZoominScale_currentIndexChanged(int index);
    void on_cbImageInlineZoomoutMode_currentIndexChanged(int index);
    void on_cbImageInlineZoomoutScale_currentIndexChanged(int index);
    void on_cbImageBlockZoominMode_currentIndexChanged(int index);
    void on_cbImageBlockZoominScale_currentIndexChanged(int index);
    void on_cbImageBlockZoomoutMode_currentIndexChanged(int index);
    void on_cbImageBlockZoomoutScale_currentIndexChanged(int index);
};

#endif // SETTINGSDLG_H
