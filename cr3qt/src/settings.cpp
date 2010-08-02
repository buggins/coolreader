#include "settings.h"
#include "ui_settings.h"
#include "cr3widget.h"
#include "crqtutil.h"
#include <QtGui/QColorDialog>
#include <QtGui/QStyleFactory>

static int def_margins[] = { 0, 5, 8, 10, 15, 20, 25, 30 };
#define MAX_MARGIN_INDEX (sizeof(def_margins)/sizeof(int))

DECL_DEF_CR_FONT_SIZES;

static bool initDone = false;

SettingsDlg::SettingsDlg(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::SettingsDlg),
    m_docview( docView )
{
    initDone = false;
    m_oldHyph = cr2qt(HyphMan::getSelectedDictionary()->getId());

    m_ui->setupUi(this);
    m_props = m_docview->getOptions();
    optionToUi( PROP_WINDOW_FULLSCREEN, m_ui->cbWindowFullscreen );
    optionToUi( PROP_WINDOW_SHOW_MENU, m_ui->cbWindowShowMenu );
    optionToUi( PROP_WINDOW_SHOW_SCROLLBAR, m_ui->cbWindowShowScrollbar );
    optionToUi( PROP_WINDOW_TOOLBAR_SIZE, m_ui->cbWindowShowToolbar );
    optionToUi( PROP_WINDOW_SHOW_STATUSBAR, m_ui->cbWindowShowStatusBar );

    optionToUi( PROP_FOOTNOTES, m_ui->cbShowFootNotes );
    optionToUi( PROP_SHOW_BATTERY, m_ui->cbShowBattery );
    optionToUi( PROP_SHOW_TIME, m_ui->cbShowClock );
    optionToUi( PROP_SHOW_TITLE, m_ui->cbShowBookName );
    optionToUi( PROP_TXT_OPTION_PREFORMATTED, m_ui->cbTxtPreFormatted );
    optionToUiInversed( PROP_STATUS_LINE, m_ui->cbShowPageHeader );
    bool b = m_props->getIntDef( PROP_STATUS_LINE, 0 )==0;
    m_ui->cbShowBattery->setEnabled( b );
    m_ui->cbShowClock->setEnabled( b );
    m_ui->cbShowBookName->setEnabled( b );

    m_ui->cbStartupAction->setCurrentIndex( m_props->getIntDef( PROP_APP_START_ACTION, 0 ) );



    int lp = m_props->getIntDef( PROP_LANDSCAPE_PAGES, 2 );
    int vm = m_props->getIntDef( PROP_PAGE_VIEW_MODE, 1 );
    if ( vm==0 )
        m_ui->cbViewMode->setCurrentIndex( 2 );
    else
        m_ui->cbViewMode->setCurrentIndex( lp==1 ? 0 : 1 );

    int n = m_props->getIntDef( PROP_PAGE_MARGIN_LEFT, 8 );
    int mi = 0;
    for ( unsigned i=0; i<MAX_MARGIN_INDEX; i++ ) {
        if ( n <= def_margins[i] ) {
            mi = i;
            break;
        }
    }
    CRLog::debug("initial margins index: %d", mi);
    m_ui->cbMargins->setCurrentIndex( mi );

    QStringList styles = QStyleFactory::keys();
    QString style = m_props->getStringDef( PROP_WINDOW_STYLE, "" );
    m_ui->cbLookAndFeel->addItems( styles );
    QStyle * s = QApplication::style();
    QString currStyle = s->objectName();
    CRLog::debug("Current system style is %s", currStyle.toUtf8().data() );
    if ( !styles.contains(style, Qt::CaseInsensitive) )
        style = currStyle;
    int index = styles.indexOf( style, Qt::CaseInsensitive );
    if ( index >=0 )
        m_ui->cbLookAndFeel->setCurrentIndex( index );

    QStringList faceList;
    crGetFontFaceList( faceList );
    m_ui->cbTextFontFace->addItems( faceList );
    m_ui->cbTitleFontFace->addItems( faceList );
    QStringList sizeList;
    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
    for ( int i=0; i<sizes.length(); i++ )
        sizeList.append( QString("%1").arg(sizes[i]) );
    m_ui->cbTextFontSize->addItems( sizeList );
    m_ui->cbTitleFontSize->addItems( sizeList );
    
    const char * defFontFace = "DejaVu Sans";
    static const char * goodFonts[] = {
        "DejaVu Sans",
        "FreeSans",
        "Liberation Sans",
        "Arial",
        NULL
    };
    for ( int i=0; goodFonts[i]; i++ ) {
	if ( faceList.indexOf(QString(goodFonts[i]))>=0 ) {
	    defFontFace = goodFonts[i];
	    break;
	}
    }

    fontToUi( PROP_FONT_FACE, PROP_FONT_SIZE, m_ui->cbTextFontFace, m_ui->cbTextFontSize, defFontFace );
    fontToUi( PROP_STATUS_FONT_FACE, PROP_STATUS_FONT_SIZE, m_ui->cbTitleFontFace, m_ui->cbTitleFontSize, defFontFace );

//		{_("90%"), "90"},
//		{_("100%"), "100"},
//		{_("110%"), "110"},
//		{_("120%"), "120"},
//		{_("140%"), "140"},
    //PROP_INTERLINE_SPACE
    //PROP_HYPHENATION_DICT
    QString v = QString("%1").arg(m_props->getIntDef(PROP_INTERLINE_SPACE, 100)) + "%";
    QStringList isitems;
    isitems.append("90%");
    isitems.append("100%");
    isitems.append("110%");
    isitems.append("120%");
    isitems.append("140%");
    m_ui->cbInterlineSpace->addItems(isitems);
    int isi = m_ui->cbInterlineSpace->findText(v);
    m_ui->cbInterlineSpace->setCurrentIndex(isi>=0 ? isi : 1);

    int hi = -1;
    v = m_props->getStringDef(PROP_HYPHENATION_DICT,"@algorithm"); //HYPH_DICT_ID_ALGORITHM;
    for ( int i=0; i<HyphMan::getDictList()->length(); i++ ) {
        HyphDictionary * item = HyphMan::getDictList()->get( i );
        if ( v == cr2qt(item->getFilename() ) )
            hi = i;
        QString s = cr2qt( item->getTitle() );
        if ( item->getType()==HDT_NONE )
            s = tr("[No hyphenation]");
        else if ( item->getType()==HDT_ALGORITHM )
            s = tr("[Algorythmic hyphenation]");
        m_ui->cbHyphenation->addItem( s );
    }
    m_ui->cbHyphenation->setCurrentIndex(hi>=0 ? hi : 1);


    m_ui->crSample->setOptions( m_props );
    m_ui->crSample->getDocView()->setShowCover( false );
    m_ui->crSample->getDocView()->setViewMode( DVM_SCROLL, 1 );
    QString testPhrase = tr("The quick brown fox jumps over the lazy dog. ");
    m_ui->crSample->getDocView()->createDefaultDocument( lString16(), qt2cr(testPhrase+testPhrase+testPhrase) );

    updateStyleSample();
    initDone = true;

    m_ui->cbPageSkin->addItem(QString("[None]"), QVariant());
}

SettingsDlg::~SettingsDlg()
{
    delete m_ui;
}

bool SettingsDlg::showDlg(  QWidget * parent, CR3View * docView )
{
    SettingsDlg * dlg = new SettingsDlg( parent, docView );
    dlg->show();
    return true;
}

void SettingsDlg::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void SettingsDlg::on_buttonBox_rejected()
{
    close();
}

void SettingsDlg::on_buttonBox_accepted()
{
    m_docview->setOptions( m_props );
    close();
}

void SettingsDlg::optionToUi( const char * optionName, QCheckBox * cb )
{
    int state = ( m_props->getIntDef( optionName, 1 ) != 0 ) ? 1 : 0;
    CRLog::debug("optionToUI(%s,%d)", optionName, state);
    cb->setCheckState( state ? Qt::Checked : Qt::Unchecked );
}

void SettingsDlg::optionToUiInversed( const char * optionName, QCheckBox * cb )
{
    int state = ( m_props->getIntDef( optionName, 1 ) != 0 ) ? 1 : 0;
    CRLog::debug("optionToUIInversed(%s,%d)", optionName, state);
    cb->setCheckState( !state ? Qt::Checked : Qt::Unchecked );
}

void SettingsDlg::setCheck( const char * optionName, int checkState )
{
    int value = (checkState == Qt::Checked) ? 1 : 0;
    CRLog::debug("setCheck(%s,%d)", optionName, value);
    m_props->setInt( optionName, value );
}

void SettingsDlg::setCheckInversed( const char * optionName, int checkState )
{
    int value = (checkState == Qt::Checked) ? 0 : 1;
    CRLog::debug("setCheckInversed(%s,%d)", optionName, value);
    m_props->setInt( optionName, value );
}

void SettingsDlg::on_cbWindowFullscreen_stateChanged(int s)
{
    setCheck( PROP_WINDOW_FULLSCREEN, s );
}

void SettingsDlg::on_cbWindowShowToolbar_stateChanged(int s)
{
    setCheck( PROP_WINDOW_TOOLBAR_SIZE, s );
}

void SettingsDlg::on_cbWindowShowMenu_stateChanged(int s)
{
    setCheck( PROP_WINDOW_SHOW_MENU, s );
}

void SettingsDlg::on_cbWindowShowStatusBar_stateChanged(int s)
{
    setCheck( PROP_WINDOW_SHOW_STATUSBAR, s );
}

void SettingsDlg::on_cbWindowShowScrollbar_stateChanged(int s)
{
    setCheck( PROP_WINDOW_SHOW_SCROLLBAR, s );
}

void SettingsDlg::on_cbViewMode_currentIndexChanged(int index)
{
    switch ( index ) {
        case 0:
            m_props->setInt( PROP_LANDSCAPE_PAGES, 1 );
            m_props->setInt( PROP_PAGE_VIEW_MODE, 1 );
            break;
        case 1:
            m_props->setInt( PROP_LANDSCAPE_PAGES, 2 );
            m_props->setInt( PROP_PAGE_VIEW_MODE, 1 );
            break;
        default:
            m_props->setInt( PROP_PAGE_VIEW_MODE, 0 );
            break;
    }
}

void SettingsDlg::on_cbShowPageHeader_stateChanged(int s)
{
    setCheckInversed( PROP_STATUS_LINE, s );
    bool b = m_props->getIntDef( PROP_STATUS_LINE, 0 )==0;
    m_ui->cbShowBattery->setEnabled( b );
    m_ui->cbShowClock->setEnabled( b );
    m_ui->cbShowBookName->setEnabled( b );
}

void SettingsDlg::on_cbShowBookName_stateChanged(int s)
{
    setCheck( PROP_SHOW_TITLE, s );
}

void SettingsDlg::on_cbShowClock_stateChanged(int s)
{
    setCheck( PROP_SHOW_TIME, s );
}

void SettingsDlg::on_cbShowBattery_stateChanged(int s)
{
    setCheck( PROP_SHOW_BATTERY, s );
}

void SettingsDlg::on_cbShowFootNotes_stateChanged(int s)
{
    setCheck( PROP_FOOTNOTES, s );

}

void SettingsDlg::on_cbMargins_currentIndexChanged(int index)
{
    int m = def_margins[index];
    CRLog::debug("marginsChanged: %d", index);
    m_props->setInt( PROP_PAGE_MARGIN_BOTTOM, m*2/3 );
    m_props->setInt( PROP_PAGE_MARGIN_TOP, m );
    m_props->setInt( PROP_PAGE_MARGIN_LEFT, m );
    m_props->setInt( PROP_PAGE_MARGIN_RIGHT, m );
}

void SettingsDlg::setBackground( QWidget * wnd, QColor cl )
{
    QPalette pal( wnd->palette() );
    pal.setColor( QPalette::Window, cl );
    wnd->setPalette( pal );
}

void SettingsDlg::updateStyleSample()
{
    QColor txtColor = getColor( PROP_FONT_COLOR, 0x000000 );
    QColor bgColor = getColor( PROP_BACKGROUND_COLOR, 0xFFFFFF );
    QColor headerColor = getColor( PROP_STATUS_FONT_COLOR, 0xFFFFFF );
    setBackground( m_ui->frmTextColor, txtColor );
    setBackground( m_ui->frmBgColor, bgColor );
    setBackground( m_ui->frmHeaderTextColor, headerColor );

    m_ui->crSample->setOptions( m_props );
    m_ui->crSample->getDocView()->setShowCover( false );
    m_ui->crSample->getDocView()->setViewMode( DVM_SCROLL, 1 );

    m_ui->crSample->getDocView()->getPageImage(0);

    HyphMan::getDictList()->activate( qt2cr(m_oldHyph) );
}

QColor SettingsDlg::getColor( const char * optionName, unsigned def )
{
    lvColor cr( m_props->getIntDef( optionName, def ) );
    return QColor( cr.r(), cr.g(), cr.b() );
}

void SettingsDlg::setColor( const char * optionName, QColor cl )
{
    m_props->setHex( optionName, lvColor( cl.red(), cl.green(), cl.blue() ).get() );
}

void SettingsDlg::colorDialog( const char * optionName, QString title )
{
    QColorDialog dlg;
    dlg.setWindowTitle(title);
    dlg.setCurrentColor( getColor( optionName, 0x000000 ) );
    if ( dlg.exec() == QDialog::Accepted ) {
        setColor( optionName, dlg.currentColor() );
        updateStyleSample();
    }
}

void SettingsDlg::on_btnTextColor_clicked()
{
    colorDialog( PROP_FONT_COLOR, tr("Text color") );
}

void SettingsDlg::on_btnBgColor_clicked()
{
    colorDialog( PROP_BACKGROUND_COLOR, tr("Background color") );
}

void SettingsDlg::on_btnHeaderTextColor_clicked()
{
    colorDialog( PROP_STATUS_FONT_COLOR, tr("Page header text color") );
}

void SettingsDlg::on_cbLookAndFeel_currentIndexChanged( QString styleName )
{
    if ( !initDone )
        return;
    CRLog::debug( "on_cbLookAndFeel_currentIndexChanged(%s)", styleName.toUtf8().data() );
    m_props->setString( PROP_WINDOW_STYLE, styleName );
}

void SettingsDlg::on_cbTitleFontFace_currentIndexChanged(QString s)
{
    if ( !initDone )
        return;
    m_props->setString( PROP_STATUS_FONT_FACE, s );
}

void SettingsDlg::on_cbTitleFontSize_currentIndexChanged(QString s)
{
    if ( !initDone )
        return;
    m_props->setString( PROP_STATUS_FONT_SIZE, s );
}

void SettingsDlg::on_cbTextFontFace_currentIndexChanged(QString s)
{
    if ( !initDone )
        return;
    m_props->setString( PROP_FONT_FACE, s );
    updateStyleSample();
}

void SettingsDlg::on_cbTextFontSize_currentIndexChanged(QString s)
{
    if ( !initDone )
        return;
    m_props->setString( PROP_FONT_SIZE, s );
    updateStyleSample();
}

void SettingsDlg::fontToUi( const char * faceOptionName, const char * sizeOptionName, QComboBox * faceCombo, QComboBox * sizeCombo, const char * defFontFace )
{
    QString faceName =  m_props->getStringDef( faceOptionName, defFontFace );
    QString sizeName =  m_props->getStringDef( sizeOptionName, sizeCombo->itemText(4).toUtf8().data() );
    int faceIndex = faceCombo->findText( faceName );
    if ( faceIndex>=0 )
        faceCombo->setCurrentIndex( faceIndex );
    int sizeIndex = sizeCombo->findText( sizeName );
    if ( sizeIndex>=0 )
        sizeCombo->setCurrentIndex( sizeIndex );
}

void SettingsDlg::on_cbInterlineSpace_currentIndexChanged(int index)
{
    if ( !initDone )
        return;
    static int n[] = {90,100,110,120,140};
    m_props->setInt( PROP_INTERLINE_SPACE, n[index] );
    updateStyleSample();
}

void SettingsDlg::on_cbHyphenation_currentIndexChanged(int index)
{
    const QStringList & dl( m_docview->getHyphDicts() );
    QString s = dl[index < dl.count() ? index : 1];
    m_props->setString( PROP_HYPHENATION_DICT, s );
    updateStyleSample();
}

void SettingsDlg::on_cbStartupAction_currentIndexChanged(int index)
{
    m_props->setInt( PROP_APP_START_ACTION, index );
}

void SettingsDlg::on_cbTxtPreFormatted_toggled(bool checked)
{

}

void SettingsDlg::on_cbTxtPreFormatted_stateChanged(int s)
{
    setCheck( PROP_TXT_OPTION_PREFORMATTED, s );
}
