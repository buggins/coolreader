#include "../crengine/include/lvdocview.h"
#include "../crengine/include/crtrace.h"
#include "../crengine/include/props.h"
#include "cr3widget.h"
#include "crqtutil.h"
#include "qpainter.h"
#include "settings.h"
#include "addbookmarkdlg.h"
#include <qglobal.h>
#if QT_VERSION >= 0x050000
#include <QResizeEvent>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QStyle>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#else
#include <QtGui/QResizeEvent>
#include <QtGui/QScrollBar>
#include <QtGui/QMenu>
#include <QtGui/QStyleFactory>
#include <QtGui/QStyle>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#endif
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>

/// to hide non-qt implementation, place all crengine-related fields here
class CR3View::DocViewData
{
    friend class CR3View;
    lString16 _settingsFileName;
    lString16 _historyFileName;
    CRPropRef _props;
};

DECL_DEF_CR_FONT_SIZES;

static void replaceColor( char * str, lUInt32 color )
{
    // in line like "0 c #80000000",
    // replace value of color
    for ( int i=0; i<8; i++ ) {
        str[i+5] = toHexDigit((color>>28) & 0xF);
        color <<= 4;
    }
}

static LVRefVec<LVImageSource> cr_battery_icons;

static LVRefVec<LVImageSource>& getBatteryIcons( lUInt32 color )
{
    CRLog::debug("Making list of Battery icon bitmats");
    lUInt32 cl1 = 0x00000000|(color&0xFFFFFF);
    lUInt32 cl2 = 0x40000000|(color&0xFFFFFF);
    lUInt32 cl3 = 0x80000000|(color&0xFFFFFF);
    lUInt32 cl4 = 0xF0000000|(color&0xFFFFFF);

    static char color1[] = "0 c #80000000";
    static char color2[] = "X c #80000000";
    static char color3[] = "o c #80AAAAAA";
    static char color4[] = ". c #80FFFFFF";
    #define BATTERY_HEADER \
            "28 15 5 1", \
            color1, \
            color2, \
            color3, \
            color4, \
            "  c None",

    static const char * battery8[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0000.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0000.XXXX.XXXX.XXXX.XXXX.0.",
        "....0.XXXX.XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery7[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.oooo.XXXX.XXXX.XXXX.0.",
        ".0000.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0000.oooo.XXXX.XXXX.XXXX.0.",
        "....0.oooo.XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery6[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0......XXXX.XXXX.XXXX.0.",
        ".0000......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0000......XXXX.XXXX.XXXX.0.",
        "....0......XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery5[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0......oooo.XXXX.XXXX.0.",
        ".0000......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0000......oooo.XXXX.XXXX.0.",
        "....0......oooo.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery4[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0...........XXXX.XXXX.0.",
        ".0000...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0000...........XXXX.XXXX.0.",
        "....0...........XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery3[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0...........oooo.XXXX.0.",
        ".0000...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0000...........oooo.XXXX.0.",
        "....0...........oooo.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery2[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0................XXXX.0.",
        ".0000................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0000................XXXX.0.",
        "....0................XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery1[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "   .0................oooo.0.",
        ".0000................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0000................oooo.0.",
        "   .0................oooo.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery0[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "   .0.....................0.",
        ".0000.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0000.....................0.",
        "....0.....................0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
//#endif

    static const char * battery_charge[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.....................0.",
        ".0000............XX.......0.",
        ".0..0...........XXXX......0.",
        ".0..0..XX......XXXXXX.....0.",
        ".0..0...XXX...XXXX..XX....0.",
        ".0..0....XXX..XXXX...XX...0.",
        ".0..0.....XXXXXXX.....XX..0.",
        ".0000.......XXXX..........0.",
        "....0........XX...........0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery_frame[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.....................0.",
        ".0000.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0000.....................0.",
        "....0.....................0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };

    const char * * icon_bpm[] = {
        battery_charge,
        battery0,
        battery1,
        battery2,
        battery3,
        battery4,
        battery5,
        battery6,
        battery7,
        battery8,
        battery_frame,
        NULL
    };

    replaceColor( color1, cl1 );
    replaceColor( color2, cl2 );
    replaceColor( color3, cl3 );
    replaceColor( color4, cl4 );

    cr_battery_icons.clear();
    for ( int i=0; icon_bpm[i]; i++ )
        cr_battery_icons.add( LVCreateXPMImageSource( icon_bpm[i] ) );

    return cr_battery_icons;
}


CR3View::CR3View( QWidget *parent)
        : QWidget( parent, Qt::WindowFlags() ), _scroll(NULL), _propsCallback(NULL)
        , _normalCursor(Qt::ArrowCursor), _linkCursor(Qt::PointingHandCursor)
        , _selCursor(Qt::IBeamCursor), _waitCursor(Qt::WaitCursor)
        , _selecting(false), _selected(false), _editMode(false), _lastBatteryState(CR_BATTERY_STATE_NO_BATTERY)
{
#if WORD_SELECTOR_ENABLED==1
    _wordSelector = NULL;
#endif
    _data = new DocViewData();
    _data->_props = LVCreatePropsContainer();
    _docview = new LVDocView();
    _docview->setCallback( this );
    _selStart = ldomXPointer();
    _selEnd = ldomXPointer();
    _selText.clear();
    ldomXPointerEx p1;
    ldomXPointerEx p2;
    _selRange.setStart(p1);
    _selRange.setEnd(p2);
    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
    _docview->setFontSizes( sizes, false );

    _docview->setBatteryIcons( getBatteryIcons(0x000000) );
    _docview->setBatteryState(CR_BATTERY_STATE_NO_BATTERY); // don't show battery
    //_docview->setBatteryState( 75 ); // 75%
//    LVStreamRef stream;
//    stream = LVOpenFileStream("/home/lve/.cr3/textures/old_paper.png", LVOM_READ);
    //stream = LVOpenFileStream("/home/lve/.cr3/textures/tx_wood.jpg", LVOM_READ);
    //stream = LVOpenFileStream("/home/lve/.cr3/backgrounds/Background1.jpg", LVOM_READ);
//    if ( !stream.isNull() ) {
//        LVImageSourceRef img = LVCreateStreamCopyImageSource(stream);
//        if ( !img.isNull() ) {
//            //img = LVCreateUnpackedImageSource(img, 1256*1256*4, false);
//            _docview->setBackgroundImage(img, true);
//        }
//    }
    updateDefProps();
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void CR3View::updateDefProps()
{
    _data->_props->setStringDef( PROP_WINDOW_FULLSCREEN, "0" );
    _data->_props->setStringDef( PROP_WINDOW_SHOW_MENU, "1" );
    _data->_props->setStringDef( PROP_WINDOW_SHOW_SCROLLBAR, "1" );
    _data->_props->setStringDef( PROP_WINDOW_TOOLBAR_SIZE, "1" );
    _data->_props->setStringDef( PROP_WINDOW_SHOW_STATUSBAR, "0" );
    _data->_props->setStringDef( PROP_APP_START_ACTION, "0" );


    QStringList styles = QStyleFactory::keys();
    QStyle * s = QApplication::style();
    QString currStyle = s->objectName();
    CRLog::debug("Current system style is %s", currStyle.toUtf8().data() );
    QString style = cr2qt(_data->_props->getStringDef( PROP_WINDOW_STYLE, currStyle.toUtf8().data() ));
    if ( !styles.contains(style, Qt::CaseInsensitive) )
        _data->_props->setString( PROP_WINDOW_STYLE, qt2cr(currStyle) );
}

CR3View::~CR3View()
{
#if WORD_SELECTOR_ENABLED==1
    if ( _wordSelector )
        delete _wordSelector;
#endif
    _docview->savePosition();
    saveHistory( QString() );
    saveSettings( QString() );
    delete _docview;
    delete _data;
}

#if WORD_SELECTOR_ENABLED==1
void CR3View::startWordSelection() {
    if ( isWordSelection() )
        endWordSelection();
    _wordSelector = new LVPageWordSelector(_docview);
    update();
}

QString CR3View::endWordSelection() {
    QString text;
    if ( isWordSelection() ) {
        ldomWordEx * word = _wordSelector->getSelectedWord();
        if ( word )
            text = cr2qt(word->getText());
        delete _wordSelector;
        _wordSelector = NULL;
        _docview->clearSelection();
        update();
    }
    return text;
}
#endif

void CR3View::setHyphDir(QString dirname, bool clear)
{
    HyphMan::initDictionaries(qt2cr( dirname), clear);
    _hyphDicts.clear();
    for ( int i=0; i<HyphMan::getDictList()->length(); i++ ) {
        HyphDictionary * item = HyphMan::getDictList()->get( i );
        QString fn = cr2qt(item->getId()); //item->getFilename() );
        _hyphDicts.append( fn );
    }
    //_hyphDicts.sort();
}

const QStringList & CR3View::getHyphDicts()
{
    return _hyphDicts;
}

LVTocItem * CR3View::getToc()
{
    return _docview->getToc();
}

/// go to position specified by xPointer string
void CR3View::goToXPointer(QString xPointer)
{
    ldomXPointer p = _docview->getDocument()->createXPointer(qt2cr(xPointer));
    _docview->savePosToNavigationHistory();
    //if ( _docview->getViewMode() == DVM_SCROLL ) {
    doCommand( DCMD_GO_POS, p.toPoint().y );
    //} else {
    //    doCommand( DCMD_GO_PAGE, item->getPage() );
    //}
}

/// returns current page
int CR3View::getCurPage()
{
    return _docview->getCurPage();
}

void CR3View::setDocumentText( QString text )
{
    _docview->savePosition();
    clearSelection();
    _docview->createDefaultDocument(lString16::empty_str, qt2cr(text));
}

bool CR3View::loadLastDocument()
{
    CRFileHist * hist = _docview->getHistory();
    if ( !hist || hist->getRecords().length()<=0 )
        return false;
    return loadDocument( cr2qt(hist->getRecords()[0]->getFilePathName()) );
}

bool CR3View::loadDocument( QString fileName )
{
    _docview->savePosition();
    clearSelection();
    bool res = _docview->LoadDocument( qt2cr(fileName).c_str() );
    if ( res ) {
        //_docview->swapToCache();
        QByteArray utf8 = fileName.toUtf8();
        CRLog::debug( "Trying to restore position for %s", utf8.constData() );
        _docview->restorePosition();
        checkFontLanguageCompatibility();
    } else {
        _docview->createDefaultDocument(lString16::empty_str, qt2cr(tr("Error while opening document ") + fileName));
    }
    update();
    return res;
}

void CR3View::wheelEvent( QWheelEvent * event )
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;
    if ( numSteps==0 && numDegrees!=0 )
        numSteps = numDegrees>0 ? 1 : -1;

    if ( numSteps ) {
        if ( _docview->getViewMode() == DVM_SCROLL ) {
            if ( numSteps > 0 )
                doCommand( DCMD_LINEDOWN, -numSteps );
            else
                doCommand( DCMD_LINEUP, numSteps );
        } else {
            if ( numSteps > 0 )
                doCommand( DCMD_PAGEUP, -numSteps );
            else
                doCommand( DCMD_PAGEDOWN, numSteps );
        }
    }
    event->accept();
 }

void CR3View::resizeEvent ( QResizeEvent * event )
{

    QSize sz = event->size();
    _docview->Resize( sz.width(), sz.height() );
}

int getBatteryState()
{
#ifdef _WIN32
    // update battery state
    SYSTEM_POWER_STATUS bstatus;
    BOOL pow = GetSystemPowerStatus(&bstatus);
    if (bstatus.BatteryFlag & 128)
        return CR_BATTERY_STATE_NO_BATTERY; // no system battery
	if (bstatus.ACLineStatus==8 && bstatus.BatteryLifePercent<100 )
		return CR_BATTERY_STATE_CHARGING; // charging
    if (bstatus.BatteryLifePercent>=0 && bstatus.BatteryLifePercent<=100)
		return bstatus.BatteryLifePercent;
    return CR_BATTERY_STATE_NO_BATTERY;
#else
	return CR_BATTERY_STATE_NO_BATTERY;
#endif
}

void CR3View::paintEvent ( QPaintEvent * event )
{
    QPainter painter(this);
    QRect rc = rect();
	int newBatteryState = getBatteryState();
	if (_lastBatteryState != newBatteryState) {
		_docview->setBatteryState( newBatteryState );
		_lastBatteryState = newBatteryState;
	}
    LVDocImageRef ref = _docview->getPageImage(0);
    if ( ref.isNull() ) {
        //painter.fillRect();
        return;
    }
    LVDrawBuf * buf = ref->getDrawBuf();
    int dx = buf->GetWidth();
    int dy = buf->GetHeight();
    int bpp = buf->GetBitsPerPixel();
    if (bpp == 4 || bpp == 3) {
        QImage img(dx, dy, QImage::Format_RGB16 );
        for ( int i=0; i<dy; i++ ) {
            unsigned char * dst = img.scanLine( i );
            unsigned char * src = buf->GetScanLine(i);
            for ( int x=0; x<dx; x++ ) {
                lUInt16 cl = *src; //(*src << (8 - bpp)) & 0xF8;
                cl = (cl << 8) | (cl << 3) | (cl >> 3);
                src++;
                *dst++ = (cl & 255);
                *dst++ = ((cl >> 8) & 255);
//                *dst++ = *src++;
//                *dst++ = 0xFF;
//                src++;
            }
        }
        painter.drawImage( rc, img );
    } else if (bpp == 2) {
        QImage img(dx, dy, QImage::Format_RGB16 );
        for ( int i=0; i<dy; i++ ) {
            unsigned char * dst = img.scanLine( i );
            unsigned char * src = buf->GetScanLine(i);
            int shift = 0;
            for ( int x=0; x<dx; x++ ) {
                lUInt16 cl = *src; //(*src << (8 - bpp)) & 0xF8;
                lUInt16 cl2 = (cl << shift) & 0xC0;
                cl2 = cl2 | (cl2 >> 2);
                cl2 = (cl2 << 8) | (cl2 << 3) | (cl2 >> 3);
                if ((x & 3) == 3) {
                    src++;
                    shift = 0;
                } else {
                    shift += 2;
                }
                *dst++ = (cl2 & 255);
                *dst++ = ((cl2 >> 8) & 255);
//                *dst++ = *src++;
//                *dst++ = 0xFF;
//                src++;
            }
        }
        painter.drawImage( rc, img );

    } else if (bpp == 1) {
        QImage img(dx, dy, QImage::Format_RGB16 );
        for ( int i=0; i<dy; i++ ) {
            unsigned char * dst = img.scanLine( i );
            unsigned char * src = buf->GetScanLine(i);
            int shift = 0;
            for ( int x=0; x<dx; x++ ) {
                lUInt16 cl = *src; //(*src << (8 - bpp)) & 0xF8;
                lUInt16 cl2 = (cl << shift) & 0x80;
                cl2 = cl2 ? 0xffff : 0x0000;
                if ((x & 7) == 7) {
                    src++;
                    shift = 0;
                } else {
                    shift++;
                }
                *dst++ = (cl2 & 255);
                *dst++ = ((cl2 >> 8) & 255);
//                *dst++ = *src++;
//                *dst++ = 0xFF;
//                src++;
            }
        }
        painter.drawImage( rc, img );

    } else if (bpp == 16) {
        QImage img(dx, dy, QImage::Format_RGB16 );
        for ( int i=0; i<dy; i++ ) {
            unsigned char * dst = img.scanLine( i );
            unsigned char * src = buf->GetScanLine(i);
            for ( int x=0; x<dx; x++ ) {
                *dst++ = *src++;
                *dst++ = *src++;
//                *dst++ = *src++;
//                *dst++ = 0xFF;
//                src++;
            }
        }
        painter.drawImage( rc, img );
    } else if (bpp == 32) {
        QImage img(dx, dy, QImage::Format_RGB32 );
        for ( int i=0; i<dy; i++ ) {
            unsigned char * dst = img.scanLine( i );
            unsigned char * src = buf->GetScanLine(i);
            for ( int x=0; x<dx; x++ ) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = 0xFF;
                src++;
            }
        }
        painter.drawImage( rc, img );
    }
    if ( _editMode ) {
        // draw caret
        lvRect cursorRc;
        if ( _docview->getCursorRect( cursorRc, false ) ) {
            if ( cursorRc.left<0 )
                cursorRc.left = 0;
            if ( cursorRc.top<0 )
                cursorRc.top = 0;
            if ( cursorRc.right>dx )
                cursorRc.right = dx;
            if ( cursorRc.bottom > dy )
                cursorRc.bottom = dy;
            if ( !cursorRc.isEmpty() ) {
                painter.setPen(QColor(255,255,255));
                painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
                //QPainter::RasterOp_SourceXorDestination;
                //QPainter::CompositionMode_Xor;
                //painter.setBrush(
                painter.drawRect( cursorRc.left, cursorRc.top, cursorRc.width(), cursorRc.height() );
            }
        }
    }
    updateScroll();
}

void CR3View::updateScroll()
{
    if ( _scroll!=NULL ) {
        // TODO: set scroll range
        const LVScrollInfo * si = _docview->getScrollInfo();
        //bool changed = false;

        // change value before max scroll to avoid a ValueChange event
        if ( _scroll->value() > si->maxpos )
            _scroll->setValue( si->pos );
        if ( si->maxpos != _scroll->maximum() ) {
            _scroll->setMaximum( si->maxpos );
            _scroll->setMinimum(0);
            //changed = true;
        }
        if ( si->pagesize != _scroll->pageStep() ) {
            _scroll->setPageStep( si->pagesize );
            //changed = true;
        }
        if ( si->pos != _scroll->value() ) {
            _scroll->setValue( si-> pos );
            //changed = true;
        }
    }
}

void CR3View::scrollTo( int value )
{
    int currPos = _docview->getScrollInfo()->pos;
    if ( currPos != value && value >= 0 ) {
        doCommand( DCMD_GO_SCROLL_POS, value );
    }
}

void CR3View::nextSentence() {
    // for debugging of ReadAloud feature position movement.
    _docview->doCommand(DCMD_SELECT_NEXT_SENTENCE, 0 );
    update();
}

void CR3View::prevSentence() {
    // for debugging of ReadAloud feature position movement.
    _docview->doCommand(DCMD_SELECT_PREV_SENTENCE, 0 );
    update();
}

void CR3View::doCommand( int cmd, int param )
{
    _docview->doCommand( (LVDocCmd)cmd, param );
    update();
}

void CR3View::togglePageScrollView()
{
    if ( _editMode )
        return;
    doCommand( DCMD_TOGGLE_PAGE_SCROLL_VIEW, 1 );
    refreshPropFromView( PROP_PAGE_VIEW_MODE );
}

void CR3View::setEditMode( bool flgEdit )
{
    if ( _editMode == flgEdit )
        return;

    if ( flgEdit && _data->_props->getIntDef( PROP_PAGE_VIEW_MODE, 0 ) )
        togglePageScrollView();
    _editMode = flgEdit;
    update();
}


void CR3View::nextPage() { doCommand( DCMD_PAGEDOWN, 1 ); }
void CR3View::prevPage() { doCommand( DCMD_PAGEUP, 1 ); }
void CR3View::nextLine() { doCommand( DCMD_LINEDOWN, 1 ); }
void CR3View::prevLine() { doCommand( DCMD_LINEUP, 1 ); }
void CR3View::nextChapter() { doCommand( DCMD_MOVE_BY_CHAPTER, 1 ); }
void CR3View::prevChapter() { doCommand( DCMD_MOVE_BY_CHAPTER, -1 ); }
void CR3View::firstPage() { doCommand( DCMD_BEGIN, 1 ); }
void CR3View::lastPage() { doCommand( DCMD_END, 1 ); }
void CR3View::historyBack() { doCommand( DCMD_LINK_BACK, 1 ); }
void CR3View::historyForward() { doCommand( DCMD_LINK_FORWARD, 1 ); }

void CR3View::refreshPropFromView( const char * propName )
{
    _data->_props->setString( propName, _docview->propsGetCurrent()->getStringDef( propName, "" ) );
}

void CR3View::zoomIn()
{ 
    doCommand( DCMD_ZOOM_IN, 1 );
    refreshPropFromView( PROP_FONT_SIZE );
}

void CR3View::zoomOut()
{
    doCommand( DCMD_ZOOM_OUT, 1 );
    refreshPropFromView( PROP_FONT_SIZE );
}

QScrollBar * CR3View::scrollBar() const
{
    return _scroll;
}

void CR3View::setScrollBar( QScrollBar * scroll )
{
    _scroll = scroll;
    if ( _scroll!=NULL ) {
        QObject::connect(_scroll, SIGNAL(valueChanged(int)),
                          this,  SLOT(scrollTo(int)));
    }
}

/// load fb2.css file
bool CR3View::loadCSS( QString fn )
{
    lString16 filename( qt2cr(fn) );
    lString8 css;
    if ( LVLoadStylesheetFile( filename, css ) ) {
        if ( !css.empty() ) {
            QFileInfo f( fn );
            CRLog::info( "Using style sheet from %s", fn.toUtf8().constData() );
            _cssDir = f.absolutePath() + "/";
            _docview->setStyleSheet( css );
            return true;
        }
    }
    return false;
}

/// load settings from file
bool CR3View::loadSettings( QString fn )
{
    lString16 filename( qt2cr(fn) );
    _data->_settingsFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    bool res = false;
    if ( !stream.isNull() && _data->_props->loadFromStream( stream.get() ) ) {
        CRLog::error("Loading settings from file %s", fn.toUtf8().data() );
        res = true;
    } else {
        CRLog::error("Cannot load settings from file %s", fn.toUtf8().data() );
    }
    _docview->propsUpdateDefaults( _data->_props );
    updateDefProps();
    CRPropRef r = _docview->propsApply( _data->_props );
    PropsRef unknownOptions = cr2qt(r);
    if ( _propsCallback != NULL )
        _propsCallback->onPropsChange( unknownOptions );
    return res;
}

/// toggle boolean property
void CR3View::toggleProperty( const char * name )
{
    int state = _data->_props->getIntDef( name, 0 )!=0 ? 0 : 1;
    PropsRef props = Props::create();
    props->setString( name, state?"1":"0" );
    setOptions( props );
}

/// set new option values
PropsRef CR3View::setOptions( PropsRef props )
{
//    for ( int i=0; i<_data->_props->getCount(); i++ ) {
//        CRLog::debug("Old [%d] '%s'=%s ", i, _data->_props->getName(i), UnicodeToUtf8(_data->_props->getValue(i)).c_str() );
//    }
//    for ( int i=0; i<props->count(); i++ ) {
//        CRLog::debug("New [%d] '%s'=%s ", i, props->name(i), props->value(i).toUtf8().data() );
//    }
    CRPropRef changed = _data->_props ^ qt2cr(props);
//    for ( int i=0; i<changed->getCount(); i++ ) {
//        CRLog::debug("Changed [%d] '%s'=%s ", i, changed->getName(i), UnicodeToUtf8(changed->getValue(i)).c_str() );
//    }
    _data->_props = changed | _data->_props;
//    for ( int i=0; i<_data->_props->getCount(); i++ ) {
//        CRLog::debug("Result [%d] '%s'=%s ", i, _data->_props->getName(i), UnicodeToUtf8(_data->_props->getValue(i)).c_str() );
//    }
    CRPropRef r = _docview->propsApply( changed );
    PropsRef unknownOptions = cr2qt(r);
    if ( _propsCallback != NULL )
        _propsCallback->onPropsChange( unknownOptions );
    saveSettings( QString() );
    checkFontLanguageCompatibility();
    update();
    return unknownOptions;
}

void CR3View::saveWindowPos( QWidget * window, const char * prefix )
{
    ::saveWindowPosition( window, _data->_props, prefix );
}

void CR3View::restoreWindowPos( QWidget * window, const char * prefix, bool allowExtraStates  )
{
    ::restoreWindowPosition( window, _data->_props, prefix, allowExtraStates );
}

/// get current option values
PropsRef CR3View::getOptions()
{
    return Props::clone(cr2qt( _data->_props ));
}

/// save settings from file
bool CR3View::saveSettings( QString fn )
{
    lString16 filename( qt2cr(fn) );
    crtrace log;
    if ( filename.empty() )
        filename = _data->_settingsFileName;
    if ( filename.empty() )
        return false;
    _data->_settingsFileName = filename;
    log << "V3DocViewWin::saveSettings(" << filename << ")";
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
    if ( !stream ) {
        lString16 path16 = LVExtractPath( filename );
        lString8 path = UnicodeToUtf8(path16);
        if ( !LVCreateDirectory( path16 ) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
    }
    if ( stream.isNull() ) {
        lString8 fn = UnicodeToUtf8( filename );
        CRLog::error("Cannot save settings to file %s", fn.c_str() );
        return false;
    }
    return _data->_props->saveToStream( stream.get() );
}

/// load history from file
bool CR3View::loadHistory( QString fn )
{
    lString16 filename( qt2cr(fn) );
    CRLog::trace("V3DocViewWin::loadHistory( %s )", UnicodeToUtf8(filename).c_str());
    _data->_historyFileName = filename;
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
    if ( stream.isNull() ) {
        return false;
    }
    if ( !_docview->getHistory()->loadFromStream( stream ) )
        return false;
    return true;
}

/// save history to file
bool CR3View::saveHistory( QString fn )
{
    lString16 filename( qt2cr(fn) );
    crtrace log;
    if ( filename.empty() )
        filename = _data->_historyFileName;
    if ( filename.empty() ) {
        CRLog::info("Cannot write history file - no file name specified");
        return false;
    }
    //CRLog::debug("Exporting bookmarks to %s", UnicodeToUtf8(_bookmarkDir).c_str());
    //_docview->exportBookmarks(_bookmarkDir); //use default filename
    lString16 bmdir = qt2cr(_bookmarkDir);
    LVAppendPathDelimiter( bmdir );
    _docview->exportBookmarks( bmdir ); //use default filename
    _data->_historyFileName = filename;
    log << "V3DocViewWin::saveHistory(" << filename << ")";
    LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
    if ( !stream ) {
        lString16 path16 = LVExtractPath( filename );
        lString8 path = UnicodeToUtf8(path16);
        if ( !LVCreateDirectory( path16 ) ) {
            CRLog::error("Cannot create directory %s", path.c_str() );
        } else {
            stream = LVOpenFileStream( filename.c_str(), LVOM_WRITE );
        }
    }
    if ( stream.isNull() ) {
    	CRLog::error("Error while creating history file %s - position will be lost", UnicodeToUtf8(filename).c_str() );
    	return false;
    }
    return _docview->getHistory()->saveToStream( stream.get() );
}

void CR3View::contextMenu( QPoint pos )
{
}

/// returns true if point is inside selected text
bool CR3View::isPointInsideSelection( QPoint pos )
{
    if ( !_selected )
        return false;
    lvPoint pt( pos.x(), pos.y() );
    ldomXPointerEx p( _docview->getNodeByPoint( pt ) );
    if ( p.isNull() )
        return false;
    return _selRange.isInside( p );
}

void CR3View::mouseMoveEvent ( QMouseEvent * event )
{
    //bool left = (event->buttons() & Qt::LeftButton);
    //bool right = (event->buttons() & Qt::RightButton);
    //bool mid = (event->buttons() & Qt::MidButton);
    lvPoint pt ( event->x(), event->y() );
    ldomXPointer p = _docview->getNodeByPoint( pt );
    lString16 path;
    lString16 href;
    if ( !p.isNull() ) {
        path = p.toString();
        href = p.getHRef();
        if ( _editMode && _selecting )
            _docview->setCursorPos( p );
        updateSelection(p);
    } else {
        //CRLog::trace("Node not found for %d, %d", event->x(), event->y());
    }
    if ( _selecting )
        setCursor( _selCursor );
    else if ( href.empty() )
        setCursor( _normalCursor );
    else
        setCursor( _linkCursor );
    //CRLog::trace("mouseMoveEvent - doc pos (%d,%d), buttons: %d %d %d %s", pt.x, pt.y, (int)left, (int)right
    //             , (int)mid, href.empty()?"":UnicodeToUtf8(href).c_str()
    //             //, path.empty()?"":UnicodeToUtf8(path).c_str()
    //             );
}

void CR3View::clearSelection()
{
    if ( _selected ) {
        _docview->clearSelection();
        update();
    }
    _selecting = false;
    _selected = false;
    _selStart = ldomXPointer();
    _selEnd = ldomXPointer();
    _selText.clear();
    ldomXPointerEx p1;
    ldomXPointerEx p2;
    _selRange.setStart(p1);
    _selRange.setEnd(p2);
}

void CR3View::startSelection( ldomXPointer p )
{
    clearSelection();
    _selecting = true;
    _selStart = p;
    updateSelection( p );
}

bool CR3View::endSelection( ldomXPointer p )
{
    if ( !_selecting )
        return false;
    updateSelection( p );
    if ( _selected ) {

    }
    _selecting = false;
    return _selected;
}

bool CR3View::updateSelection( ldomXPointer p )
{
    if ( !_selecting )
        return false;
    _selEnd = p;
    ldomXRange r( _selStart, _selEnd );
    if ( r.getStart().isNull() || r.getEnd().isNull() )
        return false;
    r.sort();
    if ( !_editMode ) {
        if ( !r.getStart().isVisibleWordStart() ) {
            //CRLog::trace("calling prevVisibleWordStart : %s", LCSTR(r.getStart().toString()));
            r.getStart().prevVisibleWordStart();
            //CRLog::trace("updated : %s", LCSTR(r.getStart().toString()));
        }
        //lString16 start = r.getStart().toString();
        if ( !r.getEnd().isVisibleWordEnd() ) {
            //CRLog::trace("calling nextVisibleWordEnd : %s", LCSTR(r.getEnd().toString()));
            r.getEnd().nextVisibleWordEnd();
            //CRLog::trace("updated : %s", LCSTR(r.getEnd().toString()));
        }
    }
    if ( r.isNull() )
        return false;
    //lString16 end = r.getEnd().toString();
    //CRLog::debug("Range: %s - %s", UnicodeToUtf8(start).c_str(), UnicodeToUtf8(end).c_str());
    r.setFlags(1);
    _docview->selectRange( r );
    _selText = cr2qt( r.getRangeText( '\n', 10000 ) );
    _selected = true;
    _selRange = r;
    update();
    return true;
}

void CR3View::checkFontLanguageCompatibility()
{
    lString16 fontFace;
    _data->_props->getString(PROP_FONT_FACE, fontFace);
    lString8 fontFace_u8 = UnicodeToUtf8(fontFace);
    lString16 langCode = _docview->getLanguage();
    lString8 langCode_u8 = UnicodeToUtf8(langCode);
    if (langCode_u8.length() == 0) {
        CRLog::debug("Can't fetch book's language to check font compatibility!");
        return;
    }
    if (fontFace_u8.length() > 0) {
        bool res = fontMan->checkFontLangCompat(fontFace_u8, langCode_u8);
        CRLog::debug("Checking font \"%s\" for compatibility with language \"%s\": %d", fontFace_u8.c_str(), langCode_u8.c_str(), res);
        if (!res)
        {
            QMessageBox::warning(this, tr("Warning"), 
                                        tr("Font \"%1\" isn't compatible with language \"%2\". Instead will be used fallback font.").arg(fontFace_u8.c_str()).arg(langCode_u8.c_str()), 
                                        QMessageBox::Ok);
        }
    }
}

void CR3View::mousePressEvent ( QMouseEvent * event )
{
    bool left = event->button() == Qt::LeftButton;
    //bool right = event->button() == Qt::RightButton;
    bool mid = event->button() == Qt::MidButton;
    lvPoint pt (event->x(), event->y());
    ldomXPointer p = _docview->getNodeByPoint( pt );
    // test imageByPoint
    LVImageSourceRef img = _docview->getImageByPoint(pt);
    if (!img.isNull())
        CRLog::debug("Image %d x %d found", img->GetWidth(), img->GetHeight());
    CRBookmark * bmk = _docview->findBookmarkByPoint(pt);
    if (bmk!=NULL) {
        CRLog::trace("Found bookmark of type %d", bmk->getType());
        if (mid)
            AddBookmarkDialog::editBookmark((QWidget*)parent(), this, bmk);
    }
    lString16 path;
    lString16 href;
    if ( !p.isNull() ) {
        path = p.toString();
        CRLog::debug("mousePressEvent(%s)", LCSTR(path));
        bool ctrlPressed = (event->modifiers() & Qt::ControlModifier)!=0;
        if ( ctrlPressed || !_editMode )
            href = p.getHRef();
    }
    if ( href.empty() ) {
        //CRLog::trace("No href pressed" );
        if ( !p.isNull() && left ) {
            if ( _editMode )
                _docview->setCursorPos( p );
            startSelection(p);
        }
    } else {
        CRLog::info("Link is selected: %s", UnicodeToUtf8(href).c_str() );
        if ( left ) {
            // link is pressed
            if ( _docview->goLink( href ) )
                update();
        }
    }
    //CRLog::debug("mousePressEvent - doc pos (%d,%d), buttons: %d %d %d", pt.x, pt.y, (int)left, (int)right, (int)mid);
}

void CR3View::mouseReleaseEvent ( QMouseEvent * event )
{
    bool left = event->button() == Qt::LeftButton;
    //bool right = event->button() == Qt::RightButton;
    //bool mid = event->button() == Qt::MidButton;
    lvPoint pt (event->x(), event->y());
    ldomXPointer p = _docview->getNodeByPoint( pt );
    lString16 path;
    lString16 href;
    if ( !p.isNull() ) {
        path = p.toString();
        href = p.getHRef();
        if ( _editMode )
            _docview->setCursorPos( p );
    }
    if ( _selecting )
        endSelection(p);
    if ( href.empty() ) {
        //CRLog::trace("No href pressed" );
        if ( !p.isNull() ) {
            //startSelection(p);
        }
    } else {
        CRLog::info("Link is selected: %s", UnicodeToUtf8(href).c_str() );
        if ( left ) {
            // link is pressed
            //if ( _docview->goLink( href ) )
            //    update();
        }
    }
    //CRLog::debug("mouseReleaseEvent - doc pos (%d,%d), buttons: %d %d %d", pt.x, pt.y, (int)left, (int)right, (int)mid);
}

/// Override to handle external links
void CR3View::OnExternalLink( lString16 url, ldomNode * node )
{
    // TODO: add support of file links
    // only URL supported for now
    QUrl qturl( cr2qt(url) );
    QDesktopServices::openUrl( qturl );
}

/// create bookmark
CRBookmark * CR3View::createBookmark()
{
    CRBookmark * bm = NULL;
    if ( getSelectionText().length()>0 && !_selRange.isNull() ) {
        bm = getDocView()->saveRangeBookmark(_selRange, bmkt_comment, lString16::empty_str);
    } else {
        bm = getDocView()->saveCurrentPageBookmark(lString16::empty_str);
    }

    return bm;
}

void CR3View::goToBookmark( CRBookmark * bm )
{
    ldomXPointer start = _docview->getDocument()->createXPointer( bm->getStartPos() );
    ldomXPointer end = _docview->getDocument()->createXPointer( bm->getEndPos() );
    if ( start.isNull() )
        return;
    if ( end.isNull() )
        end = start;
    startSelection( start );
    endSelection( end );
    goToXPointer( cr2qt(bm->getStartPos()) );
    update();
}

/// rotate view, +1 = 90` clockwise, -1 = 90` counterclockwise
void CR3View::rotate( int angle )
{
    getDocView()->doCommand( DCMD_ROTATE_BY, angle );
    update();

//    int currAngle = _data->_props->getIntDef( PROP_ROTATE_ANGLE, 0 );
//    int newAngle = currAngle + angle;
//    newAngle = newAngle % 4;
//    if ( newAngle < 0 )
//        newAngle += 4;
//    if ( newAngle == currAngle )
//        return;
//    getDocView()->SetRotateAngle( (cr_rotate_angle_t) newAngle );
//    _data->_props->setInt( PROP_ROTATE_ANGLE, newAngle );
//    update();
}

/// format detection finished
void CR3View::OnLoadFileFormatDetected( doc_format_t fileFormat )
{
    QString filename = "fb2.css";
    if ( _cssDir.length() > 0 ) {
        switch ( fileFormat ) {
        case doc_format_txt:
            filename = "txt.css";
            break;
        case doc_format_rtf:
            filename = "rtf.css";
            break;
        case doc_format_epub:
            filename = "epub.css";
            break;
        case doc_format_html:
            filename = "htm.css";
            break;
        case doc_format_doc:
            filename = "doc.css";
            break;
        case doc_format_chm:
            filename = "chm.css";
            break;
        default:
            // do nothing
            ;
        }
        CRLog::debug( "CSS file to load: %s", filename.toUtf8().constData() );
        if ( QFileInfo( _cssDir + filename ).exists() ) {
            loadCSS( _cssDir + filename );
        } else if ( QFileInfo( _cssDir + "fb2.css" ).exists() ) {
            loadCSS( _cssDir + "fb2.css" );
        }
    }
}

/// on starting file loading
void CR3View::OnLoadFileStart( lString16 filename )
{
    setCursor( _waitCursor );
}

/// file load finiished with error
void CR3View::OnLoadFileError( lString16 message )
{
    setCursor( _normalCursor );
}

/// file loading is finished successfully - drawCoveTo() may be called there
void CR3View::OnLoadFileEnd()
{
    setCursor( _normalCursor );
}

/// document formatting started
void CR3View::OnFormatStart()
{
    setCursor( _waitCursor );
}

/// document formatting finished
void CR3View::OnFormatEnd()
{
    setCursor( _normalCursor );
    _docview->updateCache(); // save to cache
}

/// set bookmarks dir
void CR3View::setBookmarksDir( QString dirname )
{
    _bookmarkDir = dirname;
}

void CR3View::keyPressEvent ( QKeyEvent * event )
{
#if 0
    // testing sentence navigation/selection
    switch ( event->key() ) {
    case Qt::Key_Z:
        _docview->doCommand(DCMD_SELECT_FIRST_SENTENCE);
        update();
        return;
    case Qt::Key_X:
        _docview->doCommand(DCMD_SELECT_NEXT_SENTENCE);
        update();
        return;
    case Qt::Key_C:
        _docview->doCommand(DCMD_SELECT_PREV_SENTENCE);
        update();
        return;
    }
#endif
#if WORD_SELECTOR_ENABLED==1
    if ( isWordSelection() ) {
        MoveDirection dir = DIR_ANY;
        switch ( event->key() ) {
        case Qt::Key_Left:
        case Qt::Key_A:
            dir = DIR_LEFT;
            break;
        case Qt::Key_Right:
        case Qt::Key_D:
            dir = DIR_RIGHT;
            break;
        case Qt::Key_W:
        case Qt::Key_Up:
            dir = DIR_UP;
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            dir = DIR_DOWN;
            break;
        case Qt::Key_Q:
        case Qt::Key_Enter:
        case Qt::Key_Escape:
            {
                QString text = endWordSelection();
                event->setAccepted(true);
                CRLog::debug("Word selected: %s", LCSTR(qt2cr(text)));
            }
            return;
        case Qt::Key_Backspace:
            _wordSelector->reducePattern();
            update();
            break;
        default:
            {
                int key = event->key();
                if ( key>=Qt::Key_A && key<=Qt::Key_Z ) {
                    QString text = event->text();
                    if ( text.length()==1 ) {
                        _wordSelector->appendPattern(qt2cr(text));
                        update();
                    }
                }
            }
            event->setAccepted(true);
            return;
        }
        int dist = event->modifiers() & Qt::ShiftModifier ? 5 : 1;
        _wordSelector->moveBy(dir, dist);
        update();
        event->setAccepted(true);
    } else {
        if ( event->key()==Qt::Key_F3 && (event->modifiers() & Qt::ShiftModifier) ) {
            startWordSelection();
            event->setAccepted(true);
            return;
        }

    }
#endif
    if ( !_editMode )
        return;
    switch ( event->key() ) {
    case Qt::Key_Left:
        break;
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        break;
    }
}

/// file progress indicator, called with values 0..100
void CR3View::OnLoadFileProgress( int percent )
{
    CRLog::info( "OnLoadFileProgress(%d%%)", percent );
}

/// format progress, called with values 0..100
void CR3View::OnFormatProgress( int percent )
{
    CRLog::info( "OnFormatProgress(%d%%)", percent );
}

/// first page is loaded from file an can be formatted for preview
void CR3View::OnLoadFileFirstPagesReady()
{
#if 0 // disabled
	if ( !_data->_props->getBoolDef( PROP_PROGRESS_SHOW_FIRST_PAGE, 1 ) ) {
        CRLog::info( "OnLoadFileFirstPagesReady() - don't paint first page because " PROP_PROGRESS_SHOW_FIRST_PAGE " setting is 0" );
        return;
    }
    CRLog::info( "OnLoadFileFirstPagesReady() - painting first page" );
    _docview->setPageHeaderOverride(qt2cr(tr("Loading: please wait...")));
    //update();
    repaint();
    CRLog::info( "OnLoadFileFirstPagesReady() - painting done" );
    _docview->setPageHeaderOverride(lString16::empty_str);
    _docview->requestRender();
    // TODO: remove debug sleep
    //sleep(5);
#endif
}
