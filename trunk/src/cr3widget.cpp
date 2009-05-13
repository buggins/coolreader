#include "cr3widget.h"
#include "qpainter.h"
#include <QResizeEvent>
#include <QScrollBar>
#include "../crengine/include/lvdocview.h"

CR3View::CR3View( QWidget *parent)
        : QWidget( parent, Qt::WindowFlags() ), _scroll(NULL)
{
    _docview = new LVDocView();
}

CR3View::~CR3View()
{
    delete _docview;
}

bool CR3View::loadDocument( QString fileName )
{
    QByteArray utf8 = fileName.toUtf8();
    bool res = _docview->LoadDocument( utf8.constData() );
    return res;
}

void CR3View::resizeEvent ( QResizeEvent * event )
{

    QSize sz = event->size();
    _docview->Resize( sz.width(), sz.height() );
}

void CR3View::paintEvent ( QPaintEvent * event )
{
    QPainter painter(this);
    QRect rc = rect();
    LVDocImageRef ref = _docview->getPageImage(0);
    LVDrawBuf * buf = ref->getDrawBuf();
    int dx = buf->GetWidth();
    int dy = buf->GetHeight();
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
    updateScroll();
}

void CR3View::updateScroll()
{
    if ( _scroll!=NULL ) {
        // TODO: set scroll range
        const LVScrollInfo * si = _docview->getScrollInfo();
        bool changed = false;
        if ( si->maxpos-si->pagesize != _scroll->maximum() ) {
            _scroll->setMaximum( si->maxpos-si->pagesize );
            _scroll->setMinimum(0);
            changed = true;
        }
        if ( si->pagesize != _scroll->pageStep() ) {
            _scroll->setPageStep( si->pagesize );
            changed = true;
        }
        if ( si->pos != _scroll->value() ) {
            _scroll->setValue( si-> pos );
            changed = true;
        }
    }
}

void CR3View::scrollTo( int value )
{
    _docview->doCommand( DCMD_GO_POS,
        _docview->scrollPosToDocPos( value ));
    update();
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
