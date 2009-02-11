#include "bgfit.h"


inline lUInt16 minBits( lUInt16 n1, lUInt16 n2, lUInt16 mask )
{
    if ( n1 == n2 )
        return n1 & mask;
    if ( (n1 & mask) < (n2 & mask) )
        return n1 & mask;
    else
        return n2 & mask;
}

inline lUInt16 maxBits( lUInt16 n1, lUInt16 n2, lUInt16 mask )
{
    if ( n1 == n2 )
        return n1 & mask;
    if ( (n1 & mask) > (n2 & mask) )
        return n1 & mask;
    else
        return n2 & mask;
}

void 
BackgroundFitWindow::draw()
{
    mainwin_->setDirty();
    mainwin_->flush();
    lvRect fullRect = _wm->getScreen()->getRect();
    LVDrawBuf * buf = _wm->getScreen()->getCanvas().get();
#if 0
    buf->FillRect(0, 0, 600, 60, 0xFFFFFF );
    for ( int k=0; k<60; k++ ) {
        buf->FillRect(k*10, k, k*10+10, k+1, 0 );
        buf->FillRect(k*10+100, k, k*10+10+100, k+1, 0 );
    }
#endif
    int src_y0 = fullRect.top;
    int src_y1 = fullRect.bottom;
    // TODO: support top position of window too
    int dst_y0 = fullRect.top;
    int dst_y1 = _rect.top;
    int linesz = buf->GetBitsPerPixel() * buf->GetWidth() / 8;
    int lastline = -3;
    //int delta = (src_y1 - src_y0) - (dst_y1 - dst_y0);
    for ( int y = dst_y0; y<dst_y1; y++ ) {
        int srcy = ((src_y1 - src_y0) * y) / (dst_y1 - dst_y0) + 1;
        lUInt8 * src = buf->GetScanLine( srcy );
        lUInt8 * dst = buf->GetScanLine( y );
        memcpy( dst, src, linesz );
        if ( srcy == lastline+2 ) {
            if ( buf->GetBitsPerPixel()==2 ) {
                src = buf->GetScanLine( srcy-1 );
                dst = buf->GetScanLine( y-1 );
                for ( int i=0; i<linesz; i++ ) {
                    lUInt16 n1 = src[i];
                    lUInt16 n2 = dst[i];
                    if ( n1 != n2 )
                        n1 = n1+ 0;
                    if ( false ) {
                        // min
#if GRAY_INVERSE==1
                        n1 = maxBits(n1,n2,0x03) | maxBits(n1,n2,0x0C) | maxBits(n1,n2,0x30) | maxBits(n1,n2,0xC0);
#else
                        n1 = minBits(n1,n2,0x03) | minBits(n1,n2,0x0C) | minBits(n1,n2,0x30) | minBits(n1,n2,0xC0);
#endif
                    } else {
                        // blend
                        n1 = ( (n1 & 0x33) << 8) | ((n1 & 0xCC)>>2);
                        n2 = ( (n2 & 0x33) << 8) | ((n2 & 0xCC)>>2);
                        n1 = (n1 + n2) >> 1;
                        n1 = ((n1 & 0x3300)>>8) | ((n1 & 0x33)<<2);
                    }
                    dst[i] = (lUInt8)n1;
                }
            }
            // TODO: support other color formats
        }
        lastline = srcy;
    }
    _wm->getScreen()->invalidateRect( fullRect );
}
