#include "filepropsdlg.h"
#include "ui_filepropsdlg.h"
#include "cr3widget.h"
#include "../crengine/include/lvdocview.h"
#include <cr3version.h>

FilePropsDialog::FilePropsDialog(QWidget *parent, CR3View * docView ) :
    QDialog(parent),
    m_ui(new Ui::FilePropsDialog)
    ,_cr3v(docView)
    ,_docview(docView->getDocView())
{
    m_ui->setupUi(this);
    setWindowTitle( tr("Document properties") );

    m_ui->tableWidget->setColumnCount(2);
    m_ui->tableWidget->setHorizontalHeaderLabels ( QStringList() << tr("Property") << tr("Value") );
    m_ui->tableWidget->verticalHeader()->hide();
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 0, QHeaderView::ResizeToContents );
    m_ui->tableWidget->horizontalHeader()->setResizeMode( 1, QHeaderView::Stretch ); //Stretch
    m_ui->tableWidget->horizontalHeader()->setStretchLastSection( true );
    m_ui->tableWidget->horizontalHeader()->setDefaultAlignment( Qt::AlignLeft );
    m_ui->tableWidget->setEditTriggers( QAbstractItemView::NoEditTriggers );

    m_ui->tableWidget->setWordWrap(true);
    m_ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->tableWidget->setSortingEnabled(false);

    //m_ui->tableWidget->setRowCount(1);

    fillItems();
    _cr3v->restoreWindowPos( this, "fileprops." );
}

void FilePropsDialog::closeEvent ( QCloseEvent * event )
{
    _cr3v->saveWindowPos( this, "fileprops." );
}

FilePropsDialog::~FilePropsDialog()
{
    delete m_ui;
}

bool FilePropsDialog::showDlg( QWidget * parent, CR3View * docView )
{
    FilePropsDialog * dlg = new FilePropsDialog( parent, docView );
    dlg->setModal( true );
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    //dlg->
    return true;
}

QString FilePropsDialog::getDocText( const char * path, const char * delim )
{
    ldomDocument * doc = _docview->getDocument();
    lString16 res;
    for ( int i=0; i<100; i++ ) {
        lString8 p = lString8(path) + "[" + fmt::decimal(i+1) + "]";
        //CRLog::trace("checking doc path %s", p.c_str() );
        lString16 p16 = Utf8ToUnicode(p);
        ldomXPointer ptr = doc->createXPointer( p16 );
        if ( ptr.isNull() )
            break;
        lString16 s = ptr.getText( L' ' );
        if ( s.empty() )
            continue;
        if ( !res.empty() && delim!=NULL )
            res << Utf8ToUnicode( lString8( delim ) );
        res << s;
    }
    return cr2qt(res);
}

QString FilePropsDialog::getDocAuthors( const char * path, const char * delim )
{
    lString16 res;
    for ( int i=0; i<100; i++ ) {
        lString8 p = lString8(path) + "[" + fmt::decimal(i+1) + "]";
        //CRLog::trace("checking doc path %s", p.c_str() ));
        lString16 firstName = qt2cr(getDocText( (p + "/first-name").c_str(), " " ));
        lString16 lastName = qt2cr(getDocText( (p + "/last-name").c_str(), " " ));
        lString16 middleName = qt2cr(getDocText( (p + "/middle-name").c_str(), " " ));
        lString16 nickName = qt2cr(getDocText( (p + "/nickname").c_str(), " " ));
        lString16 homePage = qt2cr(getDocText( (p + "/homepage").c_str(), " " ));
        lString16 email = qt2cr(getDocText( (p + "/email").c_str(), " " ));
        lString16 s = firstName;
        if ( !middleName.empty() )
            s << " " << middleName;
        if ( !lastName.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << lastName;
        }
        if ( !nickName.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << nickName;
        }
        if ( !homePage.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << homePage;
        }
        if ( !email.empty() ) {
            if ( !s.empty() )
                s << " ";
            s << email;
        }
        if ( s.empty() )
            continue;
        if ( !res.empty() && delim!=NULL )
            res << Utf8ToUnicode( lString8( delim ) );
        res << s;
    }
    return cr2qt(res);
}

void FilePropsDialog::addPropLine( QString name, QString v )
{
    v = v.trimmed();
    if ( v.length()==0 )
        return;
    prop.append(name);
    value.append(v);
}

void FilePropsDialog::addInfoSection( QString name )
{
    if ( prop.length()==0 )
        return;
    int y = m_ui->tableWidget->rowCount();
    m_ui->tableWidget->setRowCount(y+1);
    m_ui->tableWidget->setItem( y, 0, new QTableWidgetItem(name));
    m_ui->tableWidget->setSpan( y, 0, 1, 2 );
    //m_ui->tableWidget->setItem( y, 1, new QTableWidgetItem(value));
    for ( int i=0; i<prop.length(); i++ ) {
        int y = m_ui->tableWidget->rowCount();
        m_ui->tableWidget->setRowCount(y+1);
        m_ui->tableWidget->setItem( y, 0, new QTableWidgetItem(prop[i]));
        m_ui->tableWidget->setItem( y, 1, new QTableWidgetItem(value[i]));
        m_ui->tableWidget->verticalHeader()->setResizeMode( y, QHeaderView::ResizeToContents );
    }
    prop.clear();
    value.clear();
}

void FilePropsDialog::fillItems()
{
    _docview->savePosition();
    CRFileHistRecord * hist = _docview->getCurrentFileHistRecord();

    lString16 title("Cool Reader ");
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION CR_ENGINE_VERSION
#endif
    title << Utf8ToUnicode(lString8(PACKAGE_VERSION));

    lString8 txt;
    //=========================================================
    txt << "<table><col width=\"25%\"/><col width=\"75%\"/>\n";
    CRPropRef props = _docview->getDocProps();

    addPropLine( tr("Current page"), cr2qt(lString16::itoa(_docview->getCurPage())) );
    addPropLine( tr("Total pages"), cr2qt(lString16::itoa(_docview->getPageCount())) );
    addPropLine( tr("Battery state"), cr2qt(lString16::itoa(_docview->getBatteryState()) + "%") );
    addPropLine( tr("Current Time"), cr2qt(_docview->getTimeString()) );
    // TODO:
    if ( hist ) {
        CRBookmark * lastpos = hist->getLastPos();
        if ( lastpos ) {
            addPropLine( tr("Current chapter"), cr2qt(lastpos->getTitleText()));
        }
    }
    addInfoSection( tr("Status") );

    addPropLine( tr("Archive name"), cr2qt(props->getStringDef(DOC_PROP_ARC_NAME)) );
    addPropLine( tr("Archive path"), cr2qt(props->getStringDef(DOC_PROP_ARC_PATH)) );
    addPropLine( tr("Archive size"), cr2qt(props->getStringDef(DOC_PROP_ARC_SIZE)) );
    addPropLine( tr("File name"), cr2qt(props->getStringDef(DOC_PROP_FILE_NAME)) );
    addPropLine( tr("File path"), cr2qt(props->getStringDef(DOC_PROP_FILE_PATH)) );
    addPropLine( tr("File size"), cr2qt(props->getStringDef(DOC_PROP_FILE_SIZE)) );
    addPropLine( tr("File format"), cr2qt(props->getStringDef(DOC_PROP_FILE_FORMAT)) );
    addInfoSection( tr("File info") );

    addPropLine( tr("Title"), cr2qt(props->getStringDef(DOC_PROP_TITLE)) );
    addPropLine( tr("Author(s)"), cr2qt(props->getStringDef(DOC_PROP_AUTHORS)) );
    addPropLine( tr("Series name"), cr2qt(props->getStringDef(DOC_PROP_SERIES_NAME)) );
    addPropLine( tr("Series number"), cr2qt(props->getStringDef(DOC_PROP_SERIES_NUMBER)) );
    addPropLine( tr("Date"), getDocText( "/FictionBook/description/title-info/date", ", " ) );
    addPropLine( tr("Genres"), getDocText( "/FictionBook/description/title-info/genre", ", " ) );
    addPropLine( tr("Translator"), getDocText( "/FictionBook/description/title-info/translator", ", " ) );
    addInfoSection( tr("Book info") );

    addPropLine( tr("Document author"), getDocAuthors( "/FictionBook/description/document-info/author", " " ) );
    addPropLine( tr("Document date"), getDocText( "/FictionBook/description/document-info/date", " " ) );
    addPropLine( tr("Document source URL"), getDocText( "/FictionBook/description/document-info/src-url", " " ) );
    addPropLine( tr("OCR by"), getDocText( "/FictionBook/description/document-info/src-ocr", " " ) );
    addPropLine( tr("Document version"), getDocText( "/FictionBook/description/document-info/version", " " ) );
    addInfoSection( tr("Document info") );

    addPropLine( tr("Publication name"), getDocText( "/FictionBook/description/publish-info/book-name", " " ) );
    addPropLine( tr("Publisher"), getDocText( "/FictionBook/description/publish-info/publisher", " " ) );
    addPropLine( tr("Publisher city"), getDocText( "/FictionBook/description/publish-info/city", " " ) );
    addPropLine( tr("Publication year"), getDocText( "/FictionBook/description/publish-info/year", " " ) );
    addPropLine( tr("ISBN"), getDocText( "/FictionBook/description/publish-info/isbn", " " ) );
    addInfoSection( tr("Publication info") );

    addPropLine( tr("Custom info"), getDocText( "/FictionBook/description/custom-info", " " ) );

}


void FilePropsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
