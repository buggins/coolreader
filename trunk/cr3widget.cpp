#include "cr3widget.h"
#include "qpainter.h"
#include <QResizeEvent>
#include "../crengine/include/lvdocview.h"

CR3View::CR3View( QWidget *parent)
        : QWidget( parent, Qt::WindowFlags() )
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

    painter.setPen(Qt::blue);
    painter.setFont(QFont("Arial", 30));
    painter.drawText(rect(), Qt::AlignCenter, "Hello from widget");
}

