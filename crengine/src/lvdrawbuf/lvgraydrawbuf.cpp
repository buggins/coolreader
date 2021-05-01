/*******************************************************

   CoolReader Engine

   lvgraydrawbuf.cpp:  Gray bitmap buffer class

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvgraydrawbuf.h"
#include "lvimagescaleddrawcallback.h"
#include "lvdrawbuf_utils.h"
#include "crlog.h"

#define INVERT_PRSERVE_GRAYS

#ifdef INVERT_PRSERVE_GRAYS
static const lUInt8 inverted_bytes[] = {
    0xff, 0xfd, 0xfe, 0xfc, 0xf7, 0xf5, 0xf6, 0xf4, 0xfb, 0xf9, 0xfa, 0xf8, 0xf3, 0xf1,
    0xf2, 0xf0, 0xdf, 0xdd, 0xde, 0xdc, 0xd7, 0xd5, 0xd6, 0xd4, 0xdb, 0xd9, 0xda, 0xd8,
    0xd3, 0xd1, 0xd2, 0xd0, 0xef, 0xed, 0xee, 0xec, 0xe7, 0xe5, 0xe6, 0xe4, 0xeb, 0xe9,
    0xea, 0xe8, 0xe3, 0xe1, 0xe2, 0xe0, 0xcf, 0xcd, 0xce, 0xcc, 0xc7, 0xc5, 0xc6, 0xc4,
    0xcb, 0xc9, 0xca, 0xc8, 0xc3, 0xc1, 0xc2, 0xc0, 0x7f, 0x7d, 0x7e, 0x7c, 0x77, 0x75,
    0x76, 0x74, 0x7b, 0x79, 0x7a, 0x78, 0x73, 0x71, 0x72, 0x70, 0x5f, 0x5d, 0x5e, 0x5c,
    0x57, 0x55, 0x56, 0x54, 0x5b, 0x59, 0x5a, 0x58, 0x53, 0x51, 0x52, 0x50, 0x6f, 0x6d,
    0x6e, 0x6c, 0x67, 0x65, 0x66, 0x64, 0x6b, 0x69, 0x6a, 0x68, 0x63, 0x61, 0x62, 0x60,
    0x4f, 0x4d, 0x4e, 0x4c, 0x47, 0x45, 0x46, 0x44, 0x4b, 0x49, 0x4a, 0x48, 0x43, 0x41,
    0x42, 0x40, 0xbf, 0xbd, 0xbe, 0xbc, 0xb7, 0xb5, 0xb6, 0xb4, 0xbb, 0xb9, 0xba, 0xb8,
    0xb3, 0xb1, 0xb2, 0xb0, 0x9f, 0x9d, 0x9e, 0x9c, 0x97, 0x95, 0x96, 0x94, 0x9b, 0x99,
    0x9a, 0x98, 0x93, 0x91, 0x92, 0x90, 0xaf, 0xad, 0xae, 0xac, 0xa7, 0xa5, 0xa6, 0xa4,
    0xab, 0xa9, 0xaa, 0xa8, 0xa3, 0xa1, 0xa2, 0xa0, 0x8f, 0x8d, 0x8e, 0x8c, 0x87, 0x85,
    0x86, 0x84, 0x8b, 0x89, 0x8a, 0x88, 0x83, 0x81, 0x82, 0x80, 0x3f, 0x3d, 0x3e, 0x3c,
    0x37, 0x35, 0x36, 0x34, 0x3b, 0x39, 0x3a, 0x38, 0x33, 0x31, 0x32, 0x30, 0x1f, 0x1d,
    0x1e, 0x1c, 0x17, 0x15, 0x16, 0x14, 0x1b, 0x19, 0x1a, 0x18, 0x13, 0x11, 0x12, 0x10,
    0x2f, 0x2d, 0x2e, 0x2c, 0x27, 0x25, 0x26, 0x24, 0x2b, 0x29, 0x2a, 0x28, 0x23, 0x21,
    0x22, 0x20, 0xf, 0xd, 0xe, 0xc, 0x7, 0x5, 0x6, 0x4, 0xb, 0x9, 0xa, 0x8,
    0x3, 0x1, 0x2, 0x0
};
#define GET_INVERTED_BYTE(x) inverted_bytes[x]
#else
#define GET_INVERTED_BYTE(x) ~(x)
#endif

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

static lUInt8 revByteBits2( lUInt8 b )
{
    return ( (b&0x03)<<6 )
        |  ( (b&0x0C)<<2 )
        |  ( (b&0x30)>>2 )
        |  ( (b&0xC0)>>6 );
}

static const lUInt8 fill_masks1[5] = {0x00, 0x3, 0x0f, 0x3f, 0xff};
static const lUInt8 fill_masks2[4] = {0x00, 0xc0, 0xf0, 0xfc};

static lUInt16 rgb565(int r, int g, int b) {
	// rrrr rggg gggb bbbb
    return (lUInt16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
}




int  LVGrayDrawBuf::GetBitsPerPixel()
{
    return _bpp;
}

/// rotates buffer contents by specified angle
void LVGrayDrawBuf::Rotate( cr_rotate_angle_t angle )
{
    if ( angle==CR_ROTATE_ANGLE_0 )
        return;
    int sz = (_rowsize * _dy);
    if ( angle==CR_ROTATE_ANGLE_180 ) {
        if ( _bpp==DRAW_BUF_1_BPP ) {
            for ( int i=sz/2-1; i>=0; i-- ) {
                lUInt8 tmp = revByteBits1( _data[i] );
                _data[i] = revByteBits1( _data[sz-i-1] );
                _data[sz-i-1] = tmp;
            }
        } else if ( _bpp==DRAW_BUF_2_BPP ) {
            for ( int i=sz/2-1; i>=0; i-- ) {
                lUInt8 tmp = revByteBits2( _data[i] );
                _data[i] = revByteBits2( _data[sz-i-1] );
                _data[sz-i-1] = tmp;
            }
        } else { // DRAW_BUF_3_BPP, DRAW_BUF_4_BPP, DRAW_BUF_8_BPP
            lUInt8 * buf = (lUInt8 *) _data;
            for ( int i=sz/2-1; i>=0; i-- ) {
                lUInt8 tmp = buf[i];
                buf[i] = buf[sz-i-1];
                buf[sz-i-1] = tmp;
            }
        }
        return;
    }
    int newrowsize = _bpp<=2 ? (_dy * _bpp + 7) / 8 : _dy;
    sz = (newrowsize * _dx);
    lUInt8 * dst = (lUInt8 *)calloc(sz, sizeof(*dst));
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
            if ( _bpp==DRAW_BUF_1_BPP ) {
                lUInt8 px = (src[ x >> 3 ] << (x&7)) & 0x80;
                lUInt8 * dstrow = dst + newrowsize * dsty;
                dstrow[ dstx >> 3 ] |= (px >> (dstx&7));
            } else if (_bpp==DRAW_BUF_2_BPP ) {
                lUInt8 px = (src[ x >> 2 ] << ((x&3)<<1)) & 0xC0;
                lUInt8 * dstrow = dst + newrowsize * dsty;
                dstrow[ dstx >> 2 ] |= (px >> ((dstx&3)<<1));
            } else { // DRAW_BUF_3_BPP, DRAW_BUF_4_BPP, DRAW_BUF_8_BPP
                lUInt8 * dstrow = dst + newrowsize * dsty;
                dstrow[ dstx ] = src[ x ];
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

void LVGrayDrawBuf::Draw( LVImageSourceRef img, int x, int y, int width, int height, bool dither )
{
    //fprintf( stderr, "LVGrayDrawBuf::Draw( img(%d, %d), %d, %d, %d, %d\n", img->GetWidth(), img->GetHeight(), x, y, width, height );
    if ( width<=0 || height<=0 )
        return;
    LVImageScaledDrawCallback drawcb( this, img, x, y, width, height, _ditherImages, _invertImages, _smoothImages );
    img->Decode( &drawcb );

    _drawnImagesCount++;
    _drawnImagesSurface += width*height;
}


/// get pixel value
lUInt32 LVGrayDrawBuf::GetPixel( int x, int y )
{
    if (x<0 || y<0 || x>=_dx || y>=_dy)
        return 0;
    lUInt8 * line = GetScanLine(y);
    if (_bpp==1) {
        // 1bpp
        if ( line[x>>3] & (0x80>>(x&7)) )
            return 1;
        else
            return 0;
    } else if (_bpp==2) {
        return (line[x>>2] >> (6-((x&3)<<1))) & 3;
    } else { // 3, 4, 8
        return line[x];
    }
}

void LVGrayDrawBuf::Clear( lUInt32 color )
{
    if (!_data)
        return;
    color = rgbToGrayMask( color, _bpp );
#if (GRAY_INVERSE==1)
    color ^= 0xFF;
#endif
    memset( _data, color, _rowsize * _dy );
//    for (int i = _rowsize * _dy - 1; i>0; i--)
//    {
//        _data[i] = (lUInt8)color;
//    }
    SetClipRect( NULL );
}

void LVGrayDrawBuf::FillRect( int x0, int y0, int x1, int y1, lUInt32 color32 )
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
    lUInt8 color = rgbToGrayMask( color32, _bpp );
#if (GRAY_INVERSE==1)
    color ^= 0xFF;
#endif
    lUInt8 * line = GetScanLine(y0);
    for (int y=y0; y<y1; y++)
    {
        if (_bpp==1) {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 mask = 0x80 >> (x&7);
                int index = x >> 3;
                line[index] = (lUInt8)((line[index] & ~mask) | (color & mask));
            }
        } else if (_bpp==2) {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 mask = 0xC0 >> ((x&3)<<1);
                int index = x >> 2;
                line[index] = (lUInt8)((line[index] & ~mask) | (color & mask));
            }
        } else { // 3, 4, 8
            for (int x=x0; x<x1; x++)
                line[x] = color;
        }
        line += _rowsize;
    }
}

void LVGrayDrawBuf::FillRectPattern( int x0, int y0, int x1, int y1, lUInt32 color032, lUInt32 color132, lUInt8 * pattern )
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
    lUInt8 color0 = rgbToGrayMask( color032, _bpp );
    lUInt8 color1 = rgbToGrayMask( color132, _bpp );
    lUInt8 * line = GetScanLine(y0);
    for (int y=y0; y<y1; y++)
    {
        lUInt8 patternMask = pattern[y & 3];
        if (_bpp==1) {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
                lUInt8 mask = 0x80 >> (x&7);
                int index = x >> 3;
                line[index] = (lUInt8)((line[index] & ~mask) | ((patternBit?color1:color0) & mask));
            }
        } else if (_bpp==2) {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
                lUInt8 mask = 0xC0 >> ((x&3)<<1);
                int index = x >> 2;
                line[index] = (lUInt8)((line[index] & ~mask) | ((patternBit?color1:color0) & mask));
            }
        } else {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 patternBit = (patternMask << (x&7)) & 0x80;
                line[x] = patternBit ? color1 : color0;
            }
        }
        line += _rowsize;
    }
}

void LVGrayDrawBuf::InvertRect(int x0, int y0, int x1, int y1)
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

	if (_bpp==1) {
		; //TODO: implement for 1 bit
	} else if (_bpp==2) {
                lUInt8 * line = GetScanLine(y0) + (x0 >> 2);
		lUInt16 before = 4 - (x0 & 3); // number of pixels before byte boundary
		if (before == 4)
			before = 0;
		lUInt16 w = (x1 - x0 - before);
		lUInt16 after  = (w & 3); // number of pixels after byte boundary
		w >>= 2;
		before = fill_masks1[before];
		after = fill_masks2[after];
		for (int y = y0; y < y1; y++) {
			lUInt8 *dst  = line;
			if (before) {
				lUInt8 color = GET_INVERTED_BYTE(dst[0]);
				dst[0] = ((dst[0] & ~before) | (color & before));
				dst++;
			}
			for (int x = 0; x < w; x++) {
				dst[x] = GET_INVERTED_BYTE(dst[x]);
			}
			dst += w;
			if (after) {
				lUInt8 color = GET_INVERTED_BYTE(dst[0]);
				dst[0] = ((dst[0] & ~after) | (color & after));
			}
			line += _rowsize;
		}
        }
#if 0
        else if (_bpp == 4) { // 3, 4, 8
            lUInt8 * line = GetScanLine(y0);
            for (int y=y0; y<y1; y++) {
                for (int x=x0; x<x1; x++) {
                    lUInt8 value = line[x];
                    if (value == 0 || value == 0xF0)
                        line[x] = ~value;
                }
                line += _rowsize;
            }
        }
#endif
        else { // 3, 4, 8
            lUInt8 * line = GetScanLine(y0);
            for (int y=y0; y<y1; y++) {
                for (int x=x0; x<x1; x++)
                    line[x] ^= 0xFF;
                line += _rowsize;
            }
        }
	CHECK_GUARD_BYTE;
}
void LVGrayDrawBuf::Resize( int dx, int dy )
{
    if (!_ownData) {
        _data = NULL;
        _ownData = false;
    } else if (_data) {
    	CHECK_GUARD_BYTE;
        free(_data);
        _data = NULL;
	}
    _dx = dx;
    _dy = dy;
    _rowsize = _bpp<=2 ? (_dx * _bpp + 7) / 8 : _dx;
    if (dx > 0 && dy > 0) {
        _data = (unsigned char *)calloc(_rowsize * _dy + 1, sizeof(*_data));
        _data[_rowsize * _dy] = GUARD_BYTE;
    } else {
        Clear(0);
    }
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
    _rowsize = (bpp<=2) ? (_dx * _bpp + 7) / 8 : _dx;

    _backgroundColor = GetWhiteColor(); // NOLINT: Call to virtual function during construction
    _textColor = GetBlackColor();       // NOLINT

    if ( auxdata ) {
        _data = (lUInt8 *) auxdata;
        _ownData = false;
    } else if (_dx && _dy) {
        _data = (lUInt8 *) calloc(_rowsize * _dy + 1, sizeof(*_data));
        _data[_rowsize * _dy] = GUARD_BYTE;
    }
    SetClipRect( NULL );
    CHECK_GUARD_BYTE;
}

LVGrayDrawBuf::~LVGrayDrawBuf()
{
    if (_data && _ownData ) {
    	CHECK_GUARD_BYTE;
    	free( _data );
    }
}
void LVGrayDrawBuf::DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0,int length1,int length2,int direction)
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
    lUInt8 color = rgbToGrayMask( color0, _bpp );
#if (GRAY_INVERSE==1)
    color ^= 0xFF;
#endif

    for (int y=y0; y<y1; y++)
    {
        if (_bpp==1) {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 * line = GetScanLine(y);
                if (direction==0 &&x%(length1+length2)<length1)line[x] = color;
                if (direction==1 &&y%(length1+length2)<length1)line[x] = color;
            }
        } else if (_bpp==2) {
            for (int x=x0; x<x1; x++)
            {
                lUInt8 * line = GetScanLine(y);
                if (direction==0 &&x%(length1+length2)<length1)line[x] = color;
                if (direction==1 &&y%(length1+length2)<length1)line[x] = color;
            }
        } else { // 3, 4, 8
            for (int x=x0; x<x1; x++)
            {
                lUInt8 * line = GetScanLine(y);
                if (direction==0 &&x%(length1+length2)<length1)line[x] = color;
                if (direction==1 &&y%(length1+length2)<length1)line[x] = color;
            }
        }
    }
}
void LVGrayDrawBuf::Draw( int x, int y, const lUInt8 * bitmap, int width, int height, lUInt32 * )
{
    //int buf_width = _dx; /* 2bpp */
    int initial_height = height;
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
        if (_hidePartialGlyphs && height<=initial_height/2) // HIDE PARTIAL VISIBLE GLYPHS
            return;
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
        if (_hidePartialGlyphs && height<=initial_height/2) // HIDE PARTIAL VISIBLE GLYPHS
            return;
        int clip_bottom = _clip.bottom;
        if ( _hidePartialGlyphs )
            clip_bottom = this->_dy;
        if ( y+height > clip_bottom)
            height = clip_bottom - y;
    }
    if (height<=0)
        return;

    int bytesPerRow = _rowsize;
    if ( _bpp==2 ) {
        dstline = _data + bytesPerRow*y + (x >> 2);
        shift0 = (x & 3);
    } else if (_bpp==1) {
        dstline = _data + bytesPerRow*y + (x >> 3);
        shift0 = (x & 7);
    } else {
        dstline = _data + bytesPerRow*y + x;
        shift0 = 0;// not used
    }
    dst = dstline;

    bitmap += bx + by*bmp_width;
    shift = shift0;


    lUInt8 color = rgbToGrayMask(GetTextColor(), _bpp);
//    bool white = (color & 0x80) ?
//#if (GRAY_INVERSE==1)
//            false : true;
//#else
//            true : false;
//#endif
    for (;height;height--)
    {
        src = bitmap;

        if ( _bpp==2 ) {
            // foreground color
            lUInt8 cl = (lUInt8)(rgbToGray(GetTextColor()) >> 6); // 0..3
            //cl ^= 0x03;
            for (xx = width; xx>0; --xx)
            {
                lUInt8 opaque = (*src >> 4) & 0x0F; // 0..15
                if ( opaque>0x3 ) {
                    int shift2 = shift<<1;
                    int shift2i = 6-shift2;
                    lUInt8 mask = 0xC0 >> shift2;
                    lUInt8 dstcolor;
                    if ( opaque>=0xC ) {
                        dstcolor = cl;
                    } else {
                        lUInt8 bgcolor = ((*dst)>>shift2i)&3; // 0..3
                        dstcolor = ((opaque*cl + (15-opaque)*bgcolor)>>4)&3;
                    }
                    *dst = (*dst & ~mask) | (dstcolor<<shift2i);
                }
                src++;
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
        } else { // 3,4,8
            int mask = ((1<<_bpp)-1)<<(8-_bpp);
            for (xx = width; xx>0; --xx)
            {
                lUInt8 b = (*src++);
                if ( b ) {
                    if ( b>=mask )
                        *dst = color;
                    else {
                        int alpha = b ^ 0xFF;
                        ApplyAlphaGray( *dst, color, alpha, _bpp );
                    }
                }
                dst++;
            }
        }
        /* new dest line */
        bitmap += bmp_width;
        dstline += bytesPerRow;
        dst = dstline;
        shift = shift0;
    }
	CHECK_GUARD_BYTE;
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
    // TODO: implement for byte per pixel mode
    int sz = GetRowSize();
    lUInt8 * bitmap = (lUInt8*) calloc(sz, sizeof(*bitmap));
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
	CHECK_GUARD_BYTE;
}

#if !defined(__SYMBIAN32__) && defined(_WIN32) && !defined(QT_GL)
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
	lUInt32 pal[256];
	if ( _bpp<=8 ) {
		int n = 1<<_bpp;
		for ( int i=0; i<n; i++ ) {
			int c = 255 * i / (n-1);
			pal[i] = c | (c<<8) | (c<<16);
		}
	}
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
            else // 3,4,8
            {
                int index = (src[xx] >> (8-_bpp)) & ((1<<_bpp)-1);
                dst[xx] = pal[ index ];
            }
        }
        BitBlt( dc, x, y+yy, _dx, 1, buf.GetDC(), 0, 0, SRCCOPY );
    }
}
#endif

/// draws buffer content to another buffer doing color conversion if necessary
void LVGrayDrawBuf::DrawTo( LVDrawBuf * buf, int x, int y, int options, lUInt32 * palette )
{
    CR_UNUSED2(options, palette);
    lvRect clip;
    buf->GetClipRect(&clip);

	if ( !(!clip.isEmpty() || buf->GetBitsPerPixel()!=GetBitsPerPixel() || GetWidth()!=buf->GetWidth() || GetHeight()!=buf->GetHeight()) ) {
		// simple copy
        memcpy( buf->GetScanLine(0), GetScanLine(0), GetHeight() * GetRowSize() );
		return;
	}
    int bpp = GetBitsPerPixel();
	if (buf->GetBitsPerPixel() == 32) {
		// support for 32bpp to Gray drawing
	    for (int yy=0; yy<_dy; yy++)
	    {
	        if (y+yy >= clip.top && y+yy < clip.bottom)
	        {
	            lUInt8 * src = (lUInt8 *)GetScanLine(yy);
                lUInt32 * dst = ((lUInt32 *)buf->GetScanLine(y+yy)) + x;
	            if (bpp==1)
	            {
	                int shift = x & 7;
	                for (int xx=0; xx<_dx; xx++)
	                {
	                    if ( x+xx >= clip.left && x+xx < clip.right )
	                    {
	                        lUInt8 cl = (*src << shift) & 0x80;
	                        *dst = cl ? 0xFFFFFF : 0x000000;
	                    }
	                    dst++;
	                    if (++shift >= 8) {
	                    	shift = 0;
		                    src++;
	                    }

	                }
	            }
	            else if (bpp==2)
	            {
	                int shift = x & 3;
	                for (int xx=0; xx<_dx; xx++)
	                {
	                    if ( x+xx >= clip.left && x+xx < clip.right )
	                    {
	                        lUInt32 cl = (*src << (shift<<1)) & 0xC0;
	                        cl = cl | (cl >> 2) | (cl>>4) | (cl>>6);
	                        *dst = cl | (cl << 8) | (cl << 16);
	                    }
	                    dst++;
	                    if (++shift >= 4) {
	                    	shift = 0;
		                    src++;
	                    }

	                }
	            }
	            else
	            {
	            	// byte per pixel
	                for (int xx=0; xx<_dx; xx++)
	                {
	                    if ( x+xx >= clip.left && x+xx < clip.right )
	                    {
	                        lUInt32 cl = *src;
	                        if (bpp == 3) {
	                        	cl &= 0xE0;
	                        	cl = cl | (cl>>3) | (cl>>6);
	                        } else if (bpp == 4) {
	                        	cl &= 0xF0;
	                        	cl = cl | (cl>>4);
	                        }
	                        *dst = cl | (cl << 8) | (cl << 16);
	                    }
	                    dst++;
	                    src++;
	                }
	            }
	        }
	    }
	    return;
	}
	if (buf->GetBitsPerPixel() == 16) {
		// support for 32bpp to Gray drawing
	    for (int yy=0; yy<_dy; yy++)
	    {
	        if (y+yy >= clip.top && y+yy < clip.bottom)
	        {
	            lUInt8 * src = (lUInt8 *)GetScanLine(yy);
                lUInt16 * dst = ((lUInt16 *)buf->GetScanLine(y+yy)) + x;
	            if (bpp==1)
	            {
	                int shift = x & 7;
	                for (int xx=0; xx<_dx; xx++)
	                {
	                    if ( x+xx >= clip.left && x+xx < clip.right )
	                    {
	                        lUInt8 cl = (*src << shift) & 0x80;
	                        *dst = cl ? 0xFFFF : 0x0000;
	                    }
	                    dst++;
	                    if (++shift >= 8) {
	                    	shift = 0;
		                    src++;
	                    }

	                }
	            }
	            else if (bpp==2)
	            {
	                int shift = x & 3;
	                for (int xx=0; xx<_dx; xx++)
	                {
	                    if ( x+xx >= clip.left && x+xx < clip.right )
	                    {
	                        lUInt16 cl = (*src << (shift<<1)) & 0xC0;
	                        cl = cl | (cl >> 2) | (cl>>4) | (cl>>6);
	                        *dst = rgb565(cl, cl, cl);
	                    }
	                    dst++;
	                    if (++shift >= 4) {
	                    	shift = 0;
		                    src++;
	                    }

	                }
	            }
	            else
	            {
	            	// byte per pixel
	                for (int xx=0; xx<_dx; xx++)
	                {
	                    if ( x+xx >= clip.left && x+xx < clip.right )
	                    {
	                        lUInt16 cl = *src;
	                        if (bpp == 3) {
	                        	cl &= 0xE0;
	                        	cl = cl | (cl>>3) | (cl>>6);
	                        } else if (bpp == 4) {
	                        	cl &= 0xF0;
	                        	cl = cl | (cl>>4);
	                        }
	                        *dst = rgb565(cl, cl, cl);
	                    }
	                    dst++;
	                    src++;
	                }
	            }
	        }
	    }
	    return;
	}
	if (buf->GetBitsPerPixel() != bpp)
		return; // not supported yet
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
                        if (mask & 0xFF) {
                            c = *(dst+1);
                            c &= ~(mask&0xFF);
                            c |= (cl&0xFF);
                            *(dst+1) = c;
                        }
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
                        if (mask & 0xFF) {
                            c = *(dst+1);
                            c &= ~(mask&0xFF);
                            c |= (cl&0xFF);
                            *(dst+1) = c;
                        }
                    }
                    dst++;
                    src++;
                }
            }
            else
            {
                lUInt8 * dst = buf->GetScanLine(y+yy) + x;
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
	CHECK_GUARD_BYTE;
}

/// draws buffer content to another buffer doing color conversion if necessary
void LVGrayDrawBuf::DrawOnTop( LVDrawBuf * buf, int x, int y )
{

    lvRect clip;
    buf->GetClipRect(&clip);

    if ( !(!clip.isEmpty() || buf->GetBitsPerPixel()!=GetBitsPerPixel() || GetWidth()!=buf->GetWidth() || GetHeight()!=buf->GetHeight()) ) {
        // simple copy
        memcpy( buf->GetScanLine(0), GetScanLine(0), GetHeight() * GetRowSize() );
        return;
    }
    int bpp = GetBitsPerPixel();
    if (buf->GetBitsPerPixel() == 32) {
        // support for 32bpp to Gray drawing
        for (int yy=0; yy<_dy; yy++)
        {
            if (y+yy >= clip.top && y+yy < clip.bottom)
            {
                lUInt8 * src = (lUInt8 *)GetScanLine(yy);
                lUInt32 * dst = ((lUInt32 *)buf->GetScanLine(y+yy)) + x;
                if (bpp==1)
                {
                    int shift = x & 7;
                    for (int xx=0; xx<_dx; xx++)
                    {
                        if ( x+xx >= clip.left && x+xx < clip.right )
                        {
                            lUInt8 cl = (*src << shift) & 0x80;
                            if(src!=0) *dst = cl ? 0xFFFFFF : 0x000000;
                        }
                        dst++;
                        if (++shift >= 8) {
                            shift = 0;
                            src++;
                        }

                    }
                }
                else if (bpp==2)
                {
                    int shift = x & 3;
                    for (int xx=0; xx<_dx; xx++)
                    {
                        if ( x+xx >= clip.left && x+xx < clip.right )
                        {
                            lUInt32 cl = (*src << (shift<<1)) & 0xC0;
                            cl = cl | (cl >> 2) | (cl>>4) | (cl>>6);
                            if(src!=0) *dst = cl | (cl << 8) | (cl << 16);
                        }
                        dst++;
                        if (++shift >= 4) {
                            shift = 0;
                            src++;
                        }

                    }
                }
                else
                {
                    // byte per pixel
                    for (int xx=0; xx<_dx; xx++)
                    {
                        if ( x+xx >= clip.left && x+xx < clip.right )
                        {
                            lUInt32 cl = *src;
                            if (bpp == 3) {
                                cl &= 0xE0;
                                cl = cl | (cl>>3) | (cl>>6);
                            } else if (bpp == 4) {
                                cl &= 0xF0;
                                cl = cl | (cl>>4);
                            }
                            if(src!=0) *dst = cl | (cl << 8) | (cl << 16);
                        }
                        dst++;
                        src++;
                    }
                }
            }
        }
        return;
    }
    if (buf->GetBitsPerPixel() == 16) {
        // support for 32bpp to Gray drawing
        for (int yy=0; yy<_dy; yy++)
        {
            if (y+yy >= clip.top && y+yy < clip.bottom)
            {
                lUInt8 * src = (lUInt8 *)GetScanLine(yy);
                lUInt16 * dst = ((lUInt16 *)buf->GetScanLine(y+yy)) + x;
                if (bpp==1)
                {
                    int shift = x & 7;
                    for (int xx=0; xx<_dx; xx++)
                    {
                        if ( x+xx >= clip.left && x+xx < clip.right )
                        {
                            lUInt8 cl = (*src << shift) & 0x80;
                            if(*src!=0) *dst = cl ? 0xFFFF : 0x0000;
                        }
                        dst++;
                        if (++shift >= 8) {
                            shift = 0;
                            src++;
                        }

                    }
                }
                else if (bpp==2)
                {
                    int shift = x & 3;
                    for (int xx=0; xx<_dx; xx++)
                    {
                        if ( x+xx >= clip.left && x+xx < clip.right )
                        {
                            lUInt16 cl = (*src << (shift<<1)) & 0xC0;
                            cl = cl | (cl >> 2) | (cl>>4) | (cl>>6);
                            if(*src!=0) *dst = rgb565(cl, cl, cl);
                        }
                        dst++;
                        if (++shift >= 4) {
                            shift = 0;
                            src++;
                        }

                    }
                }
                else
                {
                    // byte per pixel
                    for (int xx=0; xx<_dx; xx++)
                    {
                        if ( x+xx >= clip.left && x+xx < clip.right )
                        {
                            lUInt16 cl = *src;
                            if (bpp == 3) {
                                cl &= 0xE0;
                                cl = cl | (cl>>3) | (cl>>6);
                            } else if (bpp == 4) {
                                cl &= 0xF0;
                                cl = cl | (cl>>4);
                            }
                            if(*src!=0) *dst = rgb565(cl, cl, cl);
                        }
                        dst++;
                        src++;
                    }
                }
            }
        }
        return;
    }
    if (buf->GetBitsPerPixel() != bpp)
        return; // not supported yet
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
                        if(*src!=0) *dst = c;
                        if (mask & 0xFF) {
                            c = *(dst+1);
                            c &= ~(mask&0xFF);
                            c |= (cl&0xFF);
                            if(*src!=0) *(dst+1) = c;
                        }
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
                        if(*src!=0) *dst = c;
                        if (mask & 0xFF) {
                            c = *(dst+1);
                            c &= ~(mask&0xFF);
                            c |= (cl&0xFF);
                            if(*src!=0) *(dst+1) = c;
                        }
                    }
                    dst++;
                    src++;
                }
            }
            else
            {
                lUInt8 * dst = buf->GetScanLine(y+yy) + x;
                for (int xx=0; xx<_dx; xx++)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        if(*src!=0) *dst = *src;
                    }
                    dst++;
                    src++;
                }
            }
        }
    }
    CHECK_GUARD_BYTE;
}

/// draws rescaled buffer content to another buffer doing color conversion if necessary
void LVGrayDrawBuf::DrawRescaled(LVDrawBuf * src, int x, int y, int dx, int dy, int options)
{
    CR_UNUSED(options);
    if (dx < 1 || dy < 1)
        return;
    lvRect clip;
    GetClipRect(&clip);
    int srcdx = src->GetWidth();
    int srcdy = src->GetHeight();
    bool linearInterpolation = (srcdx <= dx || srcdy <= dy);
    //CRLog::trace("LVGrayDrawBuf::DrawRescaled bpp=%d %dx%d srcbpp=%d (%d,%d) (%d,%d)", _bpp, GetWidth(), GetHeight(), src->GetBitsPerPixel(), x, y, dx, dy);
	CHECK_GUARD_BYTE;
    for (int yy=0; yy<dy; yy++)
    {
        if (y+yy >= clip.top && y+yy < clip.bottom)
        {
            lUInt8 * dst0 = (lUInt8 *)GetScanLine(y + yy);
            if (linearInterpolation) {
                // linear interpolation
                int srcy16 = srcdy * yy * 16 / dy;
                for (int xx=0; xx<dx; xx++)	{
                    if ( x+xx >= clip.left && x+xx < clip.right ) {
                        int srcx16 = srcdx * xx * 16 / dx;
                        lUInt32 cl = src->GetInterpolatedColor(srcx16, srcy16);
                        lUInt32 alpha = (cl >> 24) & 0xFF;
                        if (_bpp==1)
                        {
                            if (alpha >= 128)
                                continue;
                            int shift = (xx + x) & 7;
                            lUInt8 * dst = dst0 + ((x + xx) >> 3);
                            lUInt32 dithered = Dither1BitColor(cl, xx, yy);
                            if (dithered)
                                *dst = (*dst) | (0x80 >> shift);
                            else
                                *dst = (*dst) & ~(0x80 >> shift);
                        }
                        else if (_bpp==2)
                        {
                            if (alpha >= 128)
                                continue;
                            lUInt8 * dst = dst0 + ((x + xx) >> 2);
                            int shift = ((x+xx) & 3) * 2;
                            lUInt32 dithered = Dither2BitColor(cl, xx, yy) << 6;
                            lUInt8 b = *dst & ~(0xC0 >> shift);
                            *dst = (lUInt8)(b | (dithered >> shift));
                        }
                        else
                        {
                            lUInt8 * dst = dst0 + x + xx;
                            lUInt32 dithered;
                            if (_bpp<8)
                                dithered = DitherNBitColor(cl, xx, yy, _bpp); // << (8 - _bpp);
                            else
                                dithered = cl;
                            if (alpha < 16)
                                *dst = (lUInt8)dithered;
                            else if (alpha < 240) {
                                lUInt32 nalpha = alpha ^ 0xFF;
                                lUInt32 pixel = *dst;
                                if (_bpp == 4)
                                    pixel = ((pixel * alpha + dithered * nalpha) >> 8) & 0xF0;
                                else
                                    pixel = ((pixel * alpha + dithered * nalpha) >> 8) & 0xFF;
                                *dst = (lUInt8)pixel;
                            }
                        }
                    }
                }
#if 1
            	{
            		if (_ownData && _data[_rowsize * _dy] != GUARD_BYTE) {
            			CRLog::error("lin interpolation, corrupted buffer, yy=%d of %d", yy, dy);
            			crFatalError(-5, "corrupted bitmap buffer");
            		}
            	}
#endif
            } else {
                // area average
                lvRect srcRect;
                srcRect.top = srcdy * yy * 16 / dy;
                srcRect.bottom = srcdy * (yy + 1) * 16 / dy;
                for (int xx=0; xx<dx; xx++)
                {
                    if ( x+xx >= clip.left && x+xx < clip.right )
                    {
                        srcRect.left = srcdx * xx * 16 / dx;
                        srcRect.right = srcdx * (xx + 1) * 16 / dx;
                        lUInt32 cl = src->GetAvgColor(srcRect);
                        if (_bpp==1)
                        {
                            int shift = (x + xx) & 7;
                            lUInt8 * dst = dst0 + ((x + xx) >> 3);
                            lUInt32 dithered = Dither1BitColor(cl, xx, yy);
                            if (dithered)
                                *dst = (*dst) | (0x80 >> shift);
                            else
                                *dst = (*dst) & ~(0x80 >> shift);
                        }
                        else if (_bpp==2)
                        {
                            lUInt8 * dst = dst0 + ((x + xx) >> 2);
                            int shift = x & 3;
                            lUInt32 dithered = Dither2BitColor(cl, xx, yy) << 6;
                            lUInt8 b = *dst & ~(0xC0 >> shift);
                            *dst = (lUInt8)(b | (dithered >> (shift * 2)));
                        }
                        else
                        {
                            lUInt8 * dst = dst0 + x + xx;
                            lUInt32 dithered;
                            if (_bpp < 8)
                                dithered = DitherNBitColor(cl, xx, yy, _bpp);// << (8 - _bpp);
                            else
                                dithered = cl;
                            *dst = (lUInt8)dithered;
                        }
                    }
                }
#if 1
                {
            		if (_ownData && _data[_rowsize * _dy] != GUARD_BYTE) {
            			CRLog::error("area avg, corrupted buffer, yy=%d of %d", yy, dy);
            			crFatalError(-5, "corrupted bitmap buffer");
            		}
            	}
#endif
            }
        }
    }
	CHECK_GUARD_BYTE;
}
