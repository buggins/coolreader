/*******************************************************

   CoolReader Engine

   lvunpackedimgsource.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvunpackedimgsource.h"
#include "lvarray.h"

// lvdrawbuff private stuff
#include "../lvdrawbuf/lvdrawbuf_utils.h"

#include <string.h>

// aaaaaaaarrrrrrrrggggggggbbbbbbbb -> yyyyyyaa
inline lUInt8 grayPack(lUInt32 pixel)
{
    lUInt8 gray = (lUInt8)(( (pixel & 0xFF) + ((pixel>>16) & 0xFF) + ((pixel>>7)&510) ) >> 2);
    lUInt8 alpha = (lUInt8)((pixel>>24) & 0xFF);
    return (gray & 0xFC) | ((alpha >> 6) & 3);
}

// yyyyyyaa -> aaaaaaaarrrrrrrrggggggggbbbbbbbb
inline lUInt32 grayUnpack(lUInt8 pixel)
{
    lUInt32 gray = pixel & 0xFC;
    lUInt32 alpha = (pixel & 3) << 6;
    if ( alpha==0xC0 )
        alpha = 0xFF;
    return gray | (gray<<8) | (gray<<16) | (alpha<<24);
}


LVUnpackedImgSource::LVUnpackedImgSource(LVImageSourceRef src, int bpp)
    : _isGray(bpp<=8)
    , _bpp(bpp)
    , _grayImage(NULL)
    , _colorImage(NULL)
    , _colorImage16(NULL)
    , _dx( src->GetWidth() )
    , _dy( src->GetHeight() )
{
    if ( bpp<=8  ) {
        _grayImage = (lUInt8*)malloc( _dx * _dy * sizeof(lUInt8) );
    } else if ( bpp==16 ) {
        _colorImage16 = (lUInt16*)malloc( _dx * _dy * sizeof(lUInt16) );
    } else {
        _colorImage = (lUInt32*)malloc( _dx * _dy * sizeof(lUInt32) );
    }
    src->Decode( this );
}

LVUnpackedImgSource::~LVUnpackedImgSource()
{
    if ( _grayImage )
        free( _grayImage );
    if ( _colorImage )
        free( _colorImage );
    if ( _colorImage )
        free( _colorImage16 );
}

void LVUnpackedImgSource::OnStartDecode(LVImageSource *)
{
    //CRLog::trace( "LVUnpackedImgSource::OnStartDecode" );
}

bool LVUnpackedImgSource::OnLineDecoded(LVImageSource *, int y, lUInt32 *data)
{
    if ( y<0 || y>=_dy )
        return false;
    if ( _isGray ) {
        lUInt8 * dst = _grayImage + _dx * y;
        for ( int x=0; x<_dx; x++ ) {
            dst[x] = grayPack( data[x] );
        }
    } else if ( _bpp==16 ) {
        lUInt16 * dst = _colorImage16 + _dx * y;
        for ( int x=0; x<_dx; x++ ) {
            dst[x] = rgb888to565( data[x] );
        }
    } else {
        lUInt32 * dst = _colorImage + _dx * y;
        memcpy( dst, data, sizeof(lUInt32) * _dx );
    }
    return true;
}

void LVUnpackedImgSource::OnEndDecode(LVImageSource *, bool)
{
    //CRLog::trace( "LVUnpackedImgSource::OnEndDecode" );
}

bool LVUnpackedImgSource::Decode(LVImageDecoderCallback *callback)
{
    callback->OnStartDecode( this );
    //bool res = false;
    if ( _isGray ) {
        // gray
        LVArray<lUInt32> line;
        line.reserve( _dx );
        for ( int y=0; y<_dy; y++ ) {
            lUInt8 * src = _grayImage + _dx * y;
            lUInt32 * dst = line.ptr();
            for ( int x=0; x<_dx; x++ )
                dst[x] = grayUnpack( src[x] );
            callback->OnLineDecoded( this, y, dst );
        }
        line.clear();
    } else if ( _bpp==16 ) {
        // 16bit
        LVArray<lUInt32> line;
        line.reserve( _dx );
        for ( int y=0; y<_dy; y++ ) {
            lUInt16 * src = _colorImage16 + _dx * y;
            lUInt32 * dst = line.ptr();
            for ( int x=0; x<_dx; x++ )
                dst[x] = rgb565to888( src[x] );
            callback->OnLineDecoded( this, y, dst );
        }
        line.clear();
    } else {
        // color
        for ( int y=0; y<_dy; y++ ) {
            callback->OnLineDecoded( this, y, _colorImage + _dx * y );
        }
    }
    callback->OnEndDecode( this, false );
    return true;
}
