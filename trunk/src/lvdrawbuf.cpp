/*******************************************************

   CoolReader Engine 

   lvdrawbuf.cpp:  Gray bitmap buffer class

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/lvdrawbuf.h"

void LVDrawBuf::RoundRect( int x0, int y0, int x1, int y1, int borderWidth, int radius, lUInt32 color, int cornerFlags )
{
    FillRect( x0 + ((cornerFlags&1)?radius:0), y0, x1-1-((cornerFlags&2)?radius:0), y0+borderWidth, color );
    FillRect( x0, y0 + ((cornerFlags&1)?radius:0), x0+borderWidth, y1-1-((cornerFlags&4)?radius:0), color );
    FillRect( x1-borderWidth, y0 + ((cornerFlags&2)?radius:0), x1, y1-((cornerFlags&8)?radius:0), color );
    FillRect( x0 + ((cornerFlags&4)?radius:0), y1-borderWidth, x1-((cornerFlags&8)?radius:0), y1, color );
    // TODO: draw rounded corners
}

static lUInt32 rgbToGray( lUInt32 color )
{
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return ((r + g + g + b)>>2) & 0xFF;
}

static lUInt8 rgbToGrayMask( lUInt32 color, int bpp )
{
    if (bpp==1)
    {
        color = rgbToGray(color) >> 7;
        color = (color&1) ? 0xFF : 0x00;
    }
    else
    {
        color = rgbToGray(color) >> 6;
        color &= 3;
        color |= (color << 2) | (color << 4) | (color << 6);
    }
    return (lUInt8)color;
}

static void ApplyAlphaRGB( lUInt32 &dst, lUInt32 src, lUInt32 alpha )
{
    if ( alpha==0 )
        dst = src;
    else if ( alpha<255 ) {
        src &= 0xFFFFFF;
        lUInt32 opaque = 256 - alpha;
        lUInt32 n1 = (((dst & 0xFF00FF) * alpha + (src & 0xFF00FF) * opaque) >> 8) & 0xFF00FF;
        lUInt32 n2 = (((dst & 0x00FF00) * alpha + (src & 0x00FF00) * opaque) >> 8) & 0x00FF00;
        dst = n1 | n2;
    }
}

static const short dither_2bpp_4x4[] = {
    5, 13,  8,  16,
    9,  1,  12,  4,
    7, 15,  6,  14,
    11, 3,  10,  2,
};

static const short dither_2bpp_8x8[] = {
0, 32, 12, 44, 2, 34, 14, 46, 
48, 16, 60, 28, 50, 18, 62, 30, 
8, 40, 4, 36, 10, 42, 6, 38, 
56, 24, 52, 20, 58, 26, 54, 22, 
3, 35, 15, 47, 1, 33, 13, 45, 
51, 19, 63, 31, 49, 17, 61, 29, 
11, 43, 7, 39, 9, 41, 5, 37, 
59, 27, 55, 23, 57, 25, 53, 21, 
};

lUInt32 Dither2BitColor( lUInt32 color, lUInt32 x, lUInt32 y )
{
    int cl = ((((color>>16) & 255) + ((color>>8) & 255) + ((color) & 255)) * (256/3)) >> 8;
    if (cl<5)
        return 0;
    else if (cl>=250)
        return 3;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;

    cl = ( cl + d - 32 );
    if (cl<5)
        return 0;
    else if (cl>=250)
        return 3;
    return (cl >> 6) & 3;
}

lUInt32 Dither1BitColor( lUInt32 color, lUInt32 x, lUInt32 y )
{
    int cl = ((((color>>16) & 255) + ((color>>8) & 255) + ((color) & 255)) * (256/3)) >> 8;
    if (cl<16)
        return 0;
    else if (cl>=240)
        return 1;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;

    cl = ( cl + d - 32 );
    if (cl<5)
        return 0;
    else if (cl>=250)
        return 1;
    return (cl >> 7) & 1;
}

static lUInt8 revByteBits1( lUInt8 b )
{
    return ( (b&1)<<7 )
        |  ( (b&2)<<5 )
        |  ( (b&4)<<3 )
        |  ( (b&8)<<1 )
        |  ( (b&16)>>1 )
        |  ( (b&32)>>3 )
        |  ( (b&64)>>4 )
        |  ( (b&128)>>5 );
}

lUInt8 revByteBits2( lUInt8 b )
{
    return ( (b&0x03)<<6 )
        |  ( (b&0x0C)<<2 )
        |  ( (b&0x30)>>2 )
        |  ( (b&0xC0)>>6 );
}

/// rotates buffer contents by specified angle
void LVGrayDrawBuf::Rotate( cr_rotate_angle_t angle )
{
    if ( angle==CR_ROTATE_ANGLE_0 )
        return;
    int sz = (_rowsize * _dy);
    if ( angle==CR_ROTATE_ANGLE_180 ) {
        if ( _bpp==1 ) {
            for ( int i=sz/2-1; i>=0; i-- ) {
                lUInt8 tmp = revByteBits1( _data[i] );
                _data[i] = revByteBits1( _data[sz-i-1] );
                _data[sz-i-1] = tmp;
            }
        } else if ( _bpp==2 ) {
            for ( int i=sz/2-1; i>=0; i-- ) {
                lUInt8 tmp = revByteBits2( _data[i] );
                _data[i] = revByteBits2( _data[sz-i-1] );
                _data[sz-i-1] = tmp;
            }
        }
        return;
    }
    int newrowsize = _dy * _bpp / 8;
    sz = (newrowsize * _dx);
    lUInt8 * dst = (lUInt8 *)malloc(sz);
    memset( dst, 0, sz );
    for ( int y=0; y<_dy; y++ ) {
        lUInt8 * src = _data + _rowsize*y;
        int dstx, dsty;
        for ( int x=0; x<_dx; x++ ) {
            if ( angle==CR_ROTATE_ANGLE_90 ) {
                dstx = _dy-1-y;
                dsty = x;
            } else {
                dstx = y;
                dsty = _dx-1-x;
            }
            if ( _bpp==1 ) {
                lUInt8 px = (src[ x >> 3 ] << (x&7)) & 0x80;
                lUInt8 * dstrow = dst + newrowsize * dsty;
                dstrow[ dstx >> 3 ] |= (px >> (dstx&7));
            } else if (_bpp==2) {
                lUInt8 px = (src[ x >> 2 ] << ((x&3)<<1)) & 0xC0;
                lUInt8 * dstrow = dst + newrowsize * dsty;
                dstrow[ dstx >> 2 ] |= (px >> ((dstx&3)<<1));
            }
        }
    }
    free( _data );
    _data = dst;
    int tmp = _dx;
    _dx = _dy;
    _dy = tmp;
    _rowsize = newrowsize;
}

/// rotates buffer contents by specified angle
void LVColorDrawBuf::Rotate( cr_rotate_angle_t angle )
{
    if ( angle==CR_ROTATE_ANGLE_0 )
        return;
    int sz = (_dx * _dy);
    if ( angle==CR_ROTATE_ANGLE_180 ) {
        lUInt32 * buf = (lUInt32 *) _data;
        for ( int i=sz/2-1; i>=0; i-- ) {
            lUInt32 tmp = buf[i];
            buf[i] = buf[sz-i-1];
            buf[sz-i-1] = tmp;
        }
        return;
    }
    int newrowsize = _dy * 4;
    sz = (_dx * newrowsize);
    lUInt32 * dst = (lUInt32*) malloc( sz );
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    bool cw = angle!=CR_ROTATE_ANGLE_90;
#else
    bool cw = angle==CR_ROTATE_ANGLE_90;
#endif
    for ( int y=0; y<_dy; y++ ) {
        lUInt32 * src = (lUInt32*)_data + _dx*y;
        int nx, ny;
        if ( cw ) {
            nx = _dy - 1 - y;
        } else {
            nx = y;
        }
        for ( int x=0; x<_dx; x++ ) {
            if ( cw ) {
                ny = x;
            } else {
                ny = _dx - 1 - x;
            }
            dst[ _dy*ny + nx ] = src[ x ];
        }
    }
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    memcpy( _data, dst, sz );
    free( dst );
#else
    free( _data );
    _data = (lUInt8*)dst;
#endif
    int tmp = _dx;
    _dx = _dy;
    _dy = tmp;
    _rowsize = newrowsize;
}

class LVImageScaledDrawCallback : public LVImageDecoderCallback
{
private:
    LVImageSourceRef src;
    LVBaseDrawBuf * dst;
    int dst_x;
    int dst_y;
    int dst_dx;
    int dst_dy;
    int src_dx;
    int src_dy;
    int * xmap;
    int * ymap;
    bool dither;
public:
    static int * GenMap( int src_len, int dst_len )
    {
        int  * map = new int[ dst_len ];
        for (int i=0; i<dst_len; i++)
        {
            map[ i ] = i * src_len / dst_len;
        }
        return map;
    }
    LVImageScaledDrawCallback(LVBaseDrawBuf * dstbuf, LVImageSourceRef img, int x, int y, int width, int height, bool dith )
    : src(img), dst(dstbuf), dst_x(x), dst_y(y), dst_dx(width), dst_dy(height), xmap(0), ymap(0), dither(dith)
    {
        src_dx = img->GetWidth();
        src_dy = img->GetHeight();
        if ( src_dx != dst_dx )
            xmap = GenMap( src_dx, dst_dx );
        if ( src_dy != dst_dy )
            ymap = GenMap( src_dy, dst_dy );
    }
    virtual ~LVImageScaledDrawCallback()
    {
        if (xmap)
            delete[] xmap;
        if (ymap)
            delete[] ymap;
    }
    virtual void OnStartDecode( LVImageSource * obj )
    {
    }
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data )
    {
        //fprintf( stderr, "l_%d ", y );
        int yy = y;
        int yy2 = y+1;
        if ( ymap ) 
        {
            int yy0 = (y - 1) * dst_dy / src_dy;
            yy = y * dst_dy / src_dy;
            yy2 = (y+1) * dst_dy / src_dy;
            if ( yy == yy0 )
            {
                //fprintf( stderr, "skip_dup " );
                //return true; // skip duplicate drawing
            }
            if ( yy2 > dst_dy )
                yy2 = dst_dy;
        }
        lvRect clip;
        dst->GetClipRect( &clip );
        for ( ;yy<yy2; yy++ )
        {
            if ( yy+dst_y<clip.top || yy+dst_y>=clip.bottom )
                continue;
            if ( dst->GetBitsPerPixel() >= 24 )
            {
                lUInt32 * row = (lUInt32 *)dst->GetScanLine( yy + dst_y );
                row += dst_x;
                for (int x=0; x<dst_dx; x++)
                {
                    lUInt32 cl = data[xmap ? xmap[x] : x];
                    int xx = x + dst_x;
                    lUInt32 alpha = (cl >> 24)&0xFF;
                    if ( xx<clip.left || xx>=clip.right || alpha==0xFF )
                        continue;
                    if ( !alpha )
                        row[ x ] = cl;
                    else
                        ApplyAlphaRGB( row[x], cl, alpha );
                }
            }
            else if ( dst->GetBitsPerPixel() == 2 )
            {
                //fprintf( stderr, "." );
                lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy+dst_y );
                //row += dst_x;
                for (int x=0; x<dst_dx; x++)
                {
                    lUInt32 cl = data[xmap ? xmap[x] : x];
                    int xx = x + dst_x;
                    lUInt32 alpha = (cl >> 24)&0xFF;
                    if ( xx<clip.left || xx>=clip.right || alpha&0x80 )
                        continue;
                    lUInt32 dcl = 0;
                    if ( dither ) {
#if (GRAY_INVERSE==1)
                        dcl = Dither2BitColor( cl, x, yy ) ^ 3;
#else
                        dcl = Dither2BitColor( cl, x, yy );
#endif
                    } else {
                        dcl = rgbToGrayMask( cl, 2 ) & 3;
                    }
                    int byteindex = (xx >> 2);
                    int bitindex = (3-(xx & 3))<<1;
                    lUInt8 mask = 0xC0 >> (6 - bitindex);
                    dcl = dcl << bitindex;
                    row[ byteindex ] = (lUInt8)((row[ byteindex ] & (~mask)) | dcl);
                }
            }
            else if ( dst->GetBitsPerPixel() == 1 )
            {
                //fprintf( stderr, "." );
                lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy+dst_y );
                //row += dst_x;
                for (int x=0; x<dst_dx; x++)
                {
                    lUInt32 cl = data[xmap ? xmap[x] : x];
                    int xx = x + dst_x;
                    lUInt32 alpha = (cl >> 24)&0xFF;
                    if ( xx<clip.left || xx>=clip.right || alpha&0x80 )
                        continue;
                    lUInt32 dcl = 0;
                    if ( dither ) {
#if (GRAY_INVERSE==1)
                        dcl = Dither1BitColor( cl, x, yy ) ^ 1;
#else
                        dcl = Dither1BitColor( cl, x, yy ) ^ 0;
#endif
                    } else {
                        dcl = rgbToGrayMask( cl, 1 ) & 1;
                    }
                    int byteindex = (xx >> 3);
                    int bitindex = ((xx & 7));
                    lUInt8 mask = 0x80 >> (bitindex);
                    dcl = dcl << (7-bitindex);
                    row[ byteindex ] = (lUInt8)((row[ byteindex ] & (~mask)) | dcl);
                }
            }
            else
            {
                return false;
            }
        }
        return true;
    }
    virtual void OnEndDecode( LVImageSource * obj, bool errors )
    {
    }
};


int  LVBaseDrawBuf::GetWidth()
{ 
    return _dx;
}

int  LVBaseDrawBuf::GetHeight()
{ 
    return _dy; 
}

int  LVGrayDrawBuf::GetBitsPerPixel()
{ 
    return _bpp;
}

void LVGrayDrawBuf::Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither )
{
    //fprintf( stderr, "LVGrayDrawBuf::Draw( img(%d, %d), %d, %d, %d, %d\n", img->GetWidth(), img->GetHeight(), x, y, width, height );
    if ( width<=0 || height<=0 )
        return;
    LVImageScaledDrawCallback drawcb( this, img, x, y, width, height, dither );
    img->Decode( &drawcb );
}


/// get pixel value
lUInt32 LVGrayDrawBuf::GetPixel( int x, int y )
{
    if (x<0 || y<0 || x>=_dx || y>=_dy)
        return 0;
    lUInt8 * line = GetScanLine(y);
    if (_bpp==1)
    {
        // 1bpp
        if ( line[x>>3] & (0x80>>(x&7)) )
            return 1;
        else
            return 0;
    }
    else // 2bpp
    {
        if (x<8)
            return x/2;
        return (line[x>>2] >> (6-((x&3)<<1))) & 3;
    }
}

void LVGrayDrawBuf::Clear( lUInt32 color )
{
    color = rgbToGrayMask( color, _bpp );
#if (GRAY_INVERSE==1)
    color ^= 0xFF;
#endif
    for (int i = _rowsize * _dy - 1; i>0; i--)
    {
        _data[i] = (lUInt8)color;
    }
    SetClipRect( NULL );
}

void LVGrayDrawBuf::FillRect( int x0, int y0, int x1, int y1, lUInt32 color )
{
    if (x0<_clip.left)
        x0 = _clip.left;
    if (y0<_clip.top)
        y0 = _clip.top;
    if (x1>_clip.right)
        x1 = _clip.right;
    if (y1>_clip.bottom)
        y1 = _clip.bottom;
    if (x0>=x1 || y0>=y1)
        return;
    color = rgbToGrayMask( color, _bpp );
#if (GRAY_INVERSE==1)
    color ^= 0xFF;
#endif
    lUInt8 * line = GetScanLine(y0);
    for (int y=y0; y<y1; y++)
    {
        if (_bpp==1)
        {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 mask = 0x80 >> (x&7);
                int index = x >> 3;
                line[index] = (lUInt8)((line[index] & ~mask) | (color & mask));
            }
        }
        else
        {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 mask = 0xC0 >> ((x&3)<<1);
                int index = x >> 2;
                line[index] = (lUInt8)((line[index] & ~mask) | (color & mask));
            }
        }
        line += _rowsize;
    }
}

void LVGrayDrawBuf::FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern )
{
    if (x0<_clip.left)
        x0 = _clip.left;
    if (y0<_clip.top)
        y0 = _clip.top;
    if (x1>_clip.right)
        x1 = _clip.right;
    if (y1>_clip.bottom)
        y1 = _clip.bottom;
    if (x0>=x1 || y0>=y1)
        return;
    color0 = rgbToGrayMask( color0, _bpp );
    color1 = rgbToGrayMask( color1, _bpp );
    lUInt8 * line = GetScanLine(y0);
    for (int y=y0; y<y1; y++)
    {
        lUInt8 patternMask = pattern[y & 3];
        if (_bpp==1)
        {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
                lUInt8 mask = 0x80 >> (x&7);
                int index = x >> 3;
                line[index] = (lUInt8)((line[index] & ~mask) | ((patternBit?color1:color0) & mask));
            }
        }
        else
        {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
                lUInt8 mask = 0xC0 >> ((x&3)<<1);
                int index = x >> 2;
                line[index] = (lUInt8)((line[index] & ~mask) | ((patternBit?color1:color0) & mask));
            }
        }
        line += _rowsize;
    }
}

void LVGrayDrawBuf::Resize( int dx, int dy )
{
    _dx = dx;
    _dy = dy;
    _rowsize = (_dx * _bpp + 7) / 8;
    if ( !_ownData ) {
        _data = NULL;
        _ownData = false;
    }
    if ( dx && dy )
    {
        _data = (lUInt8 *) realloc(_data, _rowsize * _dy);
    }
    else if (_data)
    {
        free(_data);
        _data = NULL;
    }
    Clear(0);
    SetClipRect( NULL );
}

/// returns white pixel value
lUInt32 LVGrayDrawBuf::GetWhiteColor()
{
    return 0xFFFFFF;
    /*
#if (GRAY_INVERSE==1)
    return 0;
#else
    return (1<<_bpp) - 1;
#endif
    */
}
/// returns black pixel value
lUInt32 LVGrayDrawBuf::GetBlackColor()
{
    return 0;
    /*
#if (GRAY_INVERSE==1)
    return (1<<_bpp) - 1;
#else
    return 0;
#endif
    */
}

LVGrayDrawBuf::LVGrayDrawBuf(int dx, int dy, int bpp, void * auxdata )
    : LVBaseDrawBuf(), _bpp(bpp), _ownData(true)
{
    _dx = dx;
    _dy = dy;
    _bpp = bpp;
    _rowsize = (_dx * _bpp + 7) / 8;

    _backgroundColor = GetWhiteColor();
    _textColor = GetBlackColor();

    if ( auxdata ) {
        _data = (lUInt8 *) auxdata;
        _ownData = false;
    } else if (_dx && _dy) {
        _data = (lUInt8 *) malloc(_rowsize * _dy);
        Clear(0);
    }
    SetClipRect( NULL );
}

LVGrayDrawBuf::~LVGrayDrawBuf()
{
    if (_data && _ownData )
        free( _data );
}

void LVGrayDrawBuf::Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette )
{
    //int buf_width = _dx; /* 2bpp */
    int bx = 0;
    int by = 0;
    int xx;
    int bmp_width = width;
    lUInt8 * dst;
    lUInt8 * dstline;
    const lUInt8 * src;
    int      shift, shift0;

    if (x<_clip.left)
    {
        width += x-_clip.left;
        bx -= x-_clip.left;
        x = _clip.left;
        if (width<=0)
            return;
    }
    if (y<_clip.top)
    {
        height += y-_clip.top;
        by -= y-_clip.top;
        y = _clip.top;
        if (height<=0)
            return;
    }
    if (x + width > _clip.right)
    {
        width = _clip.right - x;
    }
    if (width<=0)
        return;
    if (y + height > _clip.bottom)
    {
        height = _clip.bottom - y;
    }
    if (height<=0)
        return;

    int bytesPerRow = _rowsize;
    if ( _bpp==2 ) {
        dstline = _data + bytesPerRow*y + (x >> 2);
        shift0 = (x & 3);
    } else {
        dstline = _data + bytesPerRow*y + (x >> 3);
        shift0 = (x & 7);
    }
    dst = dstline;
    xx = width;

    bitmap += bx + by*bmp_width;
    shift = shift0;


    bool white = (rgbToGray( _textColor ) & 0x80) ?
#if (GRAY_INVERSE==1)
            false : true;
#else
            true : false;
#endif
    for (;height;height--)
    {
        src = bitmap;

        if ( _bpp==2 ) {
            for (xx = width; xx>0; --xx)
            {
                if ( white )
                    *dst |= (( (*src++) & 0xC0 ) >> ( shift << 1 ));
                else
                    *dst &= ~(( ((*src++) & 0xC0) ) >> ( shift << 1 ));
                /* next pixel */
                if (!(++shift & 3))
                {
                    shift = 0;
                    dst++;
                }
            }
        } else if ( _bpp==1 ) {
            for (xx = width; xx>0; --xx)
            {
#if (GRAY_INVERSE==1)
                *dst |= (( (*src++) & 0x80 ) >> ( shift ));
#else
                *dst &= ~(( ((*src++) & 0x80) ) >> ( shift ));
#endif
                /* next pixel */
                if (!(++shift & 7))
                {
                    shift = 0;
                    dst++;
                }
            }
        }
        /* new dest line */
        bitmap += bmp_width;
        dstline += bytesPerRow;
        dst = dstline;
        shift = shift0;
    }
}

void LVBaseDrawBuf::SetClipRect( const lvRect * clipRect )
{
    if (clipRect)
    {
        _clip = *clipRect;
        if (_clip.left<0)
            _clip.left = 0;
        if (_clip.top<0)
            _clip.top = 0;
        if (_clip.right>_dx)
            _clip.right = _dx;
        if (_clip.bottom > _dy)
            _clip.bottom = _dy;
    }
    else
    {
        _clip.top = 0;
        _clip.left = 0;
        _clip.right = _dx;
        _clip.bottom = _dy;
    }
}

lUInt8 * LVGrayDrawBuf::GetScanLine( int y )
{
    return _data + _rowsize*y;
}

void LVGrayDrawBuf::Invert()
{
    int sz = _rowsize * _dy;
    for (int i=sz-1; i>=0; i--)
        _data[i] = ~_data[i];
}

void LVGrayDrawBuf::ConvertToBitmap(bool flgDither)
{
    if (_bpp==1)
        return;
    int sz = (_dx+7)/8*_dy;
    lUInt8 * bitmap = (lUInt8*) malloc( sizeof(lUInt8) * sz );
    memset( bitmap, 0, sz );
    if (flgDither)
    {
        static const lUInt8 cmap[4][4] = {
            { 0, 0, 0, 0},
            { 0, 0, 1, 0},
            { 0, 1, 0, 1},
            { 1, 1, 1, 1},
        };
        for (int y=0; y<_dy; y++)
        {
            lUInt8 * src = GetScanLine(y);
            lUInt8 * dst = bitmap + ((_dx+7)/8)*y;
            for (int x=0; x<_dx; x++) {
                int cl = (src[x>>2] >> (6-((x&3)*2)))&3;
                cl = cmap[cl][ (x&1) + ((y&1)<<1) ];
                if (cmap[cl][ (x&1) + ((y&1)<<1) ])
                    dst[x>>3] |= 0x80>>(x&7);
            }
        }
    }
    else
    {
        for (int y=0; y<_dy; y++)
        {
            lUInt8 * src = GetScanLine(y);
            lUInt8 * dst = bitmap + ((_dx+7)/8)*y;
            for (int x=0; x<_dx; x++) {
                int cl = (src[x>>2] >> (7-((x&3)*2)))&1;
                if (cl)
                    dst[x>>3] |= 0x80>>(x&7);
            }
        }
    }
    free( _data );
    _data = bitmap;
    _bpp = 1;
    _rowsize = (_dx+7)/8;
}

//=======================================================
// 32-bit RGB buffer
//=======================================================

/// invert image
void  LVColorDrawBuf::Invert()
{
}

/// get buffer bits per pixel
int  LVColorDrawBuf::GetBitsPerPixel()
{
    return 32;
}

void LVColorDrawBuf::Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither )
{
    //fprintf( stderr, "LVColorDrawBuf::Draw( img(%d, %d), %d, %d, %d, %d\n", img->GetWidth(), img->GetHeight(), x, y, width, height );
    LVImageScaledDrawCallback drawcb( this, img, x, y, width, height, dither );
    img->Decode( &drawcb );
}

/// fills buffer with specified color
void LVColorDrawBuf::Clear( lUInt32 color )
{
    for (int y=0; y<_dy; y++)
    {
        lUInt32 * line = (lUInt32 *)GetScanLine(y);
        for (int x=0; x<_dx; x++)
        {
            line[x] = color;
        }
    }
}

/// get pixel value
lUInt32 LVColorDrawBuf::GetPixel( int x, int y )
{
    if (!_data || y<0 || x<0 || y>=_dy || x>=_dx)
        return 0;
    return ((lUInt32*)GetScanLine(y))[x];
}

/// fills rectangle with specified color
void LVColorDrawBuf::FillRect( int x0, int y0, int x1, int y1, lUInt32 color )
{
    if (x0<_clip.left)
        x0 = _clip.left;
    if (y0<_clip.top)
        y0 = _clip.top;
    if (x1>_clip.right)
        x1 = _clip.right;
    if (y1>_clip.bottom)
        y1 = _clip.bottom;
    if (x0>=x1 || y0>=y1)
        return;
    for (int y=y0; y<y1; y++)
    {
        lUInt32 * line = (lUInt32 *)GetScanLine(y);
        for (int x=x0; x<x1; x++)
        {
            line[x] = color;
        }
    }
}

/// fills rectangle with specified color
void LVColorDrawBuf::FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 * pattern )
{
    if (x0<_clip.left)
        x0 = _clip.left;
    if (y0<_clip.top)
        y0 = _clip.top;
    if (x1>_clip.right)
        x1 = _clip.right;
    if (y1>_clip.bottom)
        y1 = _clip.bottom;
    if (x0>=x1 || y0>=y1)
        return;
    for (int y=y0; y<y1; y++)
    {
        lUInt8 patternMask = pattern[y & 3];
        lUInt32 * line = (lUInt32 *)GetScanLine(y);
        for (int x=x0; x<x1; x++)
        {
            lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
            line[x] = patternBit ? color1 : color0;
        }
    }
}

/// sets new size
void LVColorDrawBuf::Resize( int dx, int dy )
{
    // delete old bitmap
    if ( _dx>0 && _dy>0 && _data )
    {
#if !defined(__SYMBIAN32__) && defined(_WIN32)
        if (_drawbmp)
            DeleteObject( _drawbmp );
        if (_drawdc)
            DeleteObject( _drawdc );
        _drawbmp = NULL;
        _drawdc = NULL;
#else
        free(_data);
#endif
        _data = NULL;
        _rowsize = 0;
        _dx = 0;
        _dy = 0;
    }
    
    if (dx>0 && dy>0) 
    {
        // create new bitmap
        _dx = dx;
        _dy = dy;
        _rowsize = dx*4;
#if !defined(__SYMBIAN32__) && defined(_WIN32)
        BITMAPINFO bmi;
        memset( &bmi, 0, sizeof(bmi) );
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biWidth = _dx;
        bmi.bmiHeader.biHeight = _dy;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biXPelsPerMeter = 1024;
        bmi.bmiHeader.biYPelsPerMeter = 1024;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;
    
        _drawbmp = CreateDIBSection( NULL, &bmi, DIB_RGB_COLORS, (void**)(&_data), NULL, 0 );
        _drawdc = CreateCompatibleDC(NULL);
        SelectObject(_drawdc, _drawbmp);
#else
        _data = (lUInt8 *)malloc( sizeof(lUInt32) * _dx * _dy);
#endif
        memset( _data, 0, _rowsize * _dy );
    }
    SetClipRect( NULL );
}

/// draws bitmap (1 byte per pixel) using specified palette
void LVColorDrawBuf::Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * palette )
{
    //int buf_width = _dx; /* 2bpp */
    int bx = 0;
    int by = 0;
    int xx;
    int bmp_width = width;
    lUInt32 * dst;
    lUInt32 * dstline;
    const lUInt8 * src;
    lUInt32 bmpcl = palette?palette[0]:GetTextColor();

    if (x<_clip.left)
    {
        width += x-_clip.left;
        bx -= x-_clip.left;
        x = _clip.left;
        if (width<=0)
            return;
    }
    if (y<_clip.top)
    {
        height += y-_clip.top;
        by -= y-_clip.top;
        y = _clip.top;
        if (height<=0)
            return;
    }
    if (x + width > _clip.right)
    {
        width = _clip.right - x;
    }
    if (width<=0)
        return;
    if (y + height > _clip.bottom)
    {
        height = _clip.bottom - y;
    }
    if (height<=0)
        return;

    xx = width;

    bitmap += bx + by*bmp_width;

    for (;height;height--)
    {
        src = bitmap;
        dstline = ((lUInt32*)GetScanLine(y++)) + x;
        dst = dstline;

        for (xx = width; xx>0; --xx)
        {
            lUInt32 opaque = ((*(src++))>>4)&15;
            if ( opaque>=15 )
                *dst = bmpcl;
            else if ( opaque>0 ) {
                lUInt32 alpha = 15-opaque;
                lUInt32 cl1 = ((alpha*((*dst)&0xFF00FF) + opaque*(bmpcl&0xFF00FF))>>4) & 0xFF00FF;
                lUInt32 cl2 = ((alpha*((*dst)&0x00FF00) + opaque*(bmpcl&0x00FF00))>>4) & 0x00FF00;
                *dst = cl1 | cl2;
            }
            /* next pixel */
            dst++;
        }
        /* new dest line */
        bitmap += bmp_width;
    }
}

#if !defined(__SYMBIAN32__) && defined(_WIN32)
/// draws buffer content to DC doing color conversion if necessary
void LVGrayDrawBuf::DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette )
{
    if (!dc || !_data)
        return;
    LVColorDrawBuf buf( _dx, 1 );
    lUInt32 * dst = (lUInt32 *)buf.GetScanLine(0);
#if (GRAY_INVERSE==1)
    static lUInt32 def_pal_1bpp[2] = {0xFFFFFF, 0x000000};
    static lUInt32 def_pal_2bpp[4] = {0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000};
#else
    static lUInt32 def_pal_1bpp[2] = {0x000000, 0xFFFFFF};
    static lUInt32 def_pal_2bpp[4] = {0x000000, 0x555555, 0xAAAAAA, 0xFFFFFF};
#endif
    if (!palette)
        palette = (_bpp==1) ? def_pal_1bpp : def_pal_2bpp;
    for (int yy=0; yy<_dy; yy++)
    {
        lUInt8 * src = GetScanLine(yy);
        for (int xx=0; xx<_dx; xx++)
        {
            //
            if (_bpp==1)
            {
                int shift = 7-(xx&7);
                int x0 = xx >> 3;
                dst[xx] = palette[ (src[x0]>>shift) & 1];
            }
            else if (_bpp==2)
            {
                int shift = 6-((xx&3)<<1);
                int x0 = xx >> 2;
                dst[xx] = palette[ (src[x0]>>shift) & 3];
            }
        }
        BitBlt( dc, x, y+yy, _dx, 1, buf.GetDC(), 0, 0, SRCCOPY );
    }
}


/// draws buffer content to DC doing color conversion if necessary
void LVColorDrawBuf::DrawTo( HDC dc, int x, int y, int options, lUInt32 * palette )
{
    if (dc!=NULL && _drawdc!=NULL)
        BitBlt( dc, x, y, _dx, _dy, _drawdc, 0, 0, SRCCOPY );
}
#endif

/// draws buffer content to another buffer doing color conversion if necessary
void LVGrayDrawBuf::DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette )
{
	if ( !(buf->GetBitsPerPixel()!=GetBitsPerPixel() || GetWidth()!=buf->GetWidth() || GetHeight()!=buf->GetHeight()) ) {
		// simple copy
		memcpy( buf->GetScanLine(0), GetScanLine(0), GetWidth() * GetHeight() * GetBitsPerPixel() / 8);
		return;
	}
	if ( buf->GetBitsPerPixel()!=GetBitsPerPixel() )
		return; // not supported yet
    lvRect clip;
    buf->GetClipRect(&clip);
    int bpp = buf->GetBitsPerPixel();
    for (int yy=0; yy<_dy; yy++)
    {
        if (y+yy >= clip.top && y+yy < clip.bottom)
        {
            lUInt8 * src = (lUInt8 *)GetScanLine(yy);
            if (bpp==1)
            {
                int shift = x & 7;
                lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>3);
                for (int xx=0; xx<_dx; xx+=8)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        //lUInt8 mask = ~((lUInt8)0xC0>>shift);
                        lUInt16 cl = (*src << 8)>>shift;
                        lUInt16 mask = (0xFF00)>>shift;
						lUInt8 c = *dst;
						c &= ~(mask>>8);
						c |= (cl>>8);
                        *dst = c;
						c = *(dst+1);
						c &= ~(mask&0xFF);
						c |= (cl&0xFF);
                        *(dst+1) = c;
                    }    
                    dst++;
                    src++;
                }
            }
            else if (bpp==2)
            {
                int shift = (x & 3) * 2;
                lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>2);
                for (int xx=0; xx<_dx; xx+=4)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        //lUInt8 mask = ~((lUInt8)0xC0>>shift);
                        lUInt16 cl = (*src << 8)>>shift;
                        lUInt16 mask = (0xFF00)>>shift;
						lUInt8 c = *dst;
						c &= ~(mask>>8);
						c |= (cl>>8);
                        *dst = c;
						c = *(dst+1);
						c &= ~(mask&0xFF);
						c |= (cl&0xFF);
                        *(dst+1) = c;
                    }    
                    dst++;
                    src++;
                }
            }
        }
    }
}

/// draws buffer content to another buffer doing color conversion if necessary
void LVColorDrawBuf::DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette )
{
    //
    lvRect clip;
    buf->GetClipRect(&clip);
    int bpp = buf->GetBitsPerPixel();
    for (int yy=0; yy<_dy; yy++)
    {
        if (y+yy >= clip.top && y+yy < clip.bottom)
        {
            lUInt32 * src = (lUInt32 *)GetScanLine(yy);
            if (bpp==1)
            {
                int shift = x & 7;
                lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>3);
                for (int xx=0; xx<_dx; xx++)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        //lUInt8 mask = ~((lUInt8)0xC0>>shift);
#if (GRAY_INVERSE==1)
                        lUInt8 cl = (((lUInt8)(*src)&0x80)^0x80) >> (shift);
#else
                        lUInt8 cl = (((lUInt8)(*src)&0x80)) >> (shift);
#endif
                        *dst |= cl;
                    }    
                    if ( !(shift = (shift+1)&7) )
                        dst++;
                    src++;
                }
            }
            else if (bpp==2)
            {
                int shift = x & 3;
                lUInt8 * dst = buf->GetScanLine(y+yy) + (x>>2);
                for (int xx=0; xx<_dx; xx++)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        //lUInt8 mask = ~((lUInt8)0xC0>>shift);
#if (GRAY_INVERSE==1)
                        lUInt8 cl = (((lUInt8)(*src)&0xC0)^0xC0) >> (shift<<1);
#else
                        lUInt8 cl = (((lUInt8)(*src)&0xC0)) >> (shift<<1);
#endif
                        *dst |= cl;
                    }    
                    if ( !(shift = (shift+1)&3) )
                        dst++;
                    src++;
                }
            }
            else if (bpp==32)
            {
                lUInt32 * dst = ((lUInt32 *)buf->GetScanLine(y+yy)) + x;
                for (int xx=0; xx<_dx; xx++)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        *dst = *src;
                    }    
                    dst++;
                    src++;
                }
            }
        }
    }
}

/// returns scanline pointer
lUInt8 * LVColorDrawBuf::GetScanLine( int y )
{
    if (!_data || y<0 || y>=_dy)
        return NULL;
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    return _data + _rowsize * (_dy-1-y);
#else
    return _data + _rowsize * y;
#endif
}

/// returns white pixel value
lUInt32 LVColorDrawBuf::GetWhiteColor()
{
    return 0xFFFFFF;
}
/// returns black pixel value
lUInt32 LVColorDrawBuf::GetBlackColor()
{
    return 0x000000;
}


/// constructor
LVColorDrawBuf::LVColorDrawBuf(int dx, int dy)
:     LVBaseDrawBuf()
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    ,_drawdc(NULL)
    ,_drawbmp(NULL)
#endif
{
    Resize( dx, dy );
}

/// destructor
LVColorDrawBuf::~LVColorDrawBuf()
{
#if !defined(__SYMBIAN32__) && defined(_WIN32)
    if (_drawdc)
        DeleteDC(_drawdc);
    if (_drawbmp)
        DeleteObject(_drawbmp);
#else
    if (_data)
        free( _data );
#endif
}

/// convert to 1-bit bitmap
void LVColorDrawBuf::ConvertToBitmap(bool flgDither)
{
}

