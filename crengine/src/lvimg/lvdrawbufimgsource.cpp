/*******************************************************

   CoolReader Engine

   lvdrawbufimgsource.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvdrawbufimgsource.h"
#include "lvimagedecodercallback.h"
#include "lvcolordrawbuf.h"

// lvdrawbuff private stuff
#include "../lvdrawbuf/lvdrawbuf_utils.h"

LVDrawBufImgSource::LVDrawBufImgSource(LVColorDrawBuf *buf, bool own)
    : _buf(buf)
    , _own(own)
    , _dx( buf->GetWidth() )
    , _dy( buf->GetHeight() )
{
}

LVDrawBufImgSource::~LVDrawBufImgSource()
{
    if ( _own )
        delete _buf;
}

bool LVDrawBufImgSource::Decode(LVImageDecoderCallback *callback)
{
    callback->OnStartDecode( this );
    //bool res = false;
    if ( _buf->GetBitsPerPixel()==32 ) {
        // 32 bpp
        for ( int y=0; y<_dy; y++ ) {
            callback->OnLineDecoded( this, y, (lUInt32 *)_buf->GetScanLine(y) );
        }
    } else {
        // 16 bpp
        lUInt32 * row = new lUInt32[_dx];
        for ( int y=0; y<_dy; y++ ) {
            lUInt16 * src = (lUInt16 *)_buf->GetScanLine(y);
            for ( int x=0; x<_dx; x++ )
                row[x] = rgb565to888(src[x]);
            callback->OnLineDecoded( this, y, row );
        }
        delete[] row;
    }
    callback->OnEndDecode( this, false );
    return true;
}
