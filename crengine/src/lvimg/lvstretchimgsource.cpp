/*******************************************************

   CoolReader Engine

   lvstretchimgsource.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvstretchimgsource.h"

LVStretchImgSource::LVStretchImgSource(LVImageSourceRef src, int newWidth, int newHeight, ImageTransform hTransform, ImageTransform vTransform, int splitX, int splitY)
    : _src( src )
    , _src_dx( src->GetWidth() )
    , _src_dy( src->GetHeight() )
    , _dst_dx( newWidth )
    , _dst_dy( newHeight )
    , _hTransform(hTransform)
    , _vTransform(vTransform)
    , _split_x( splitX )
    , _split_y( splitY )
    , _callback(NULL)
{
    if ( _hTransform == IMG_TRANSFORM_TILE )
        if ( _split_x>=_src_dx )
            _split_x %=_src_dx;
    if ( _vTransform == IMG_TRANSFORM_TILE )
        if ( _split_y>=_src_dy )
            _split_y %=_src_dy;
    if ( _split_x<0 || _split_x>=_src_dx )
        _split_x = _src_dx / 2;
    if ( _split_y<0 || _split_y>=_src_dy )
        _split_y = _src_dy / 2;
}

LVStretchImgSource::~LVStretchImgSource()
{
}

void LVStretchImgSource::OnStartDecode(LVImageSource *)
{
    _line.reserve( _dst_dx );
    _callback->OnStartDecode(this);
}

bool LVStretchImgSource::OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data )
{
    bool res = false;

    switch ( _hTransform ) {
    case IMG_TRANSFORM_SPLIT:
        {
            int right_pixels = (_src_dx-_split_x-1);
            int first_right_pixel = _dst_dx - right_pixels;
            int right_offset = _src_dx - _dst_dx;
            //int bottom_pixels = (_src_dy-_split_y-1);
            //int first_bottom_pixel = _dst_dy - bottom_pixels;
            for ( int x=0; x<_dst_dx; x++ ) {
                if ( x<_split_x )
                    _line[x] = data[x];
                else if ( x < first_right_pixel )
                    _line[x] = data[_split_x];
                else
                    _line[x] = data[x + right_offset];
            }
        }
        break;
    case IMG_TRANSFORM_STRETCH:
        {
            for ( int x=0; x<_dst_dx; x++ )
                _line[x] = data[x * _src_dx / _dst_dx];
        }
        break;
    case IMG_TRANSFORM_NONE:
        {
            for ( int x=0; x<_dst_dx && x<_src_dx; x++ )
                _line[x] = data[x];
        }
        break;
    case IMG_TRANSFORM_TILE:
        {
            int offset = _src_dx - _split_x;
            for ( int x=0; x<_dst_dx; x++ )
                _line[x] = data[ (x + offset) % _src_dx];
        }
        break;
    }

    switch ( _vTransform ) {
    case IMG_TRANSFORM_SPLIT:
        {
            int middle_pixels = _dst_dy - _src_dy + 1;
            if ( y < _split_y ) {
                res = _callback->OnLineDecoded( obj, y, _line.get() );
            } else if ( y==_split_y ) {
                for ( int i=0; i < middle_pixels; i++ ) {
                    res = _callback->OnLineDecoded( obj, y+i, _line.get() );
                }
            } else {
                res = _callback->OnLineDecoded( obj, y + (_dst_dy - _src_dy), _line.get() );
            }
        }
        break;
    case IMG_TRANSFORM_STRETCH:
        {
            int y0 = y * _dst_dy / _src_dy;
            int y1 = (y+1) * _dst_dy / _src_dy;
            for ( int yy=y0; yy<y1; yy++ ) {
                res = _callback->OnLineDecoded( obj, yy, _line.get() );
            }
        }
        break;
    case IMG_TRANSFORM_NONE:
        {
            if ( y<_dst_dy )
                res = _callback->OnLineDecoded( obj, y, _line.get() );
        }
        break;
    case IMG_TRANSFORM_TILE:
        {
            int offset = _src_dy - _split_y;
            int y0 = (y + offset) % _src_dy;
            for ( int yy=y0; yy<_dst_dy; yy+=_src_dy ) {
                res = _callback->OnLineDecoded( obj, yy, _line.get() );
            }
        }
        break;
    }

    return res;
}

void LVStretchImgSource::OnEndDecode(LVImageSource *, bool res)
{
    _line.clear();
    _callback->OnEndDecode(this, res);
}

bool LVStretchImgSource::Decode(LVImageDecoderCallback *callback)
{
    _callback = callback;
    return _src->Decode( this );
}
