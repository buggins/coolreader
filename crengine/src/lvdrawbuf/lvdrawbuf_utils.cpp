/*******************************************************

   CoolReader Engine

   lvdrawbuf_utils.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvdrawbuf_utils.h"
#include "lvdrawbuf.h"

//static const short dither_2bpp_4x4[] = {
//    5, 13,  8,  16,
//    9,  1,  12,  4,
//    7, 15,  6,  14,
//    11, 3,  10,  2,
//};

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

// returns byte with higher significant bits, lower bits are 0
lUInt32 DitherNBitColor( lUInt32 color, lUInt32 x, lUInt32 y, int bits )
{
    int mask = ((1<<bits)-1)<<(8-bits);
    // gray = (r + 2*g + b)>>2
    //int cl = ((((color>>16) & 255) + ((color>>(8-1)) & (255<<1)) + ((color) & 255)) >> 2) & 255;
    int cl = ((((color>>16) & 255) + ((color>>(8-1)) & (255<<1)) + ((color) & 255)) >> 2) & 255;
    int white = (1<<bits) - 1;
    int precision = white;
    if (cl<precision)
        return 0;
    else if (cl>=255-precision)
        return mask;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    // dither = 0..63
    int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;
    int shift = bits-2;
    cl = ( (cl<<shift) + d - 32 ) >> shift;
    if ( cl>255 )
        cl = 255;
    if ( cl<0 )
        cl = 0;
    return cl & mask;
}

// NOTE: For more accurate (but slightly more costly) conversions, see:
//       stb does (lUInt8) (((r*77) + (g*150) + (b*29)) >> 8) (That's roughly the Rec601Luma algo)
//       Qt5 does (lUInt8) (((r*11) + (g*16) + (b*5)) >> 5) (That's closer to Rec601Luminance or Rec709Luminance IIRC)
lUInt32 rgbToGray( lUInt32 color )
{
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return ((r + g + g + b)>>2) & 0xFF;
}

lUInt8 rgbToGray( lUInt32 color, int bpp )
{
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return (lUInt8)(((r + g + g + b)>>2) & (((1<<bpp)-1)<<(8-bpp)));
}

lUInt8 rgbToGrayMask( lUInt32 color, int bpp )
{
    switch ( bpp ) {
    case DRAW_BUF_1_BPP:
        color = rgbToGray(color) >> 7;
        color = (color&1) ? 0xFF : 0x00;
        break;
    case DRAW_BUF_2_BPP:
        color = rgbToGray(color) >> 6;
        color &= 3;
        color |= (color << 2) | (color << 4) | (color << 6);
        break;
    case DRAW_BUF_3_BPP: // 8 colors
    case DRAW_BUF_4_BPP: // 16 colors
    case DRAW_BUF_8_BPP: // 256 colors
        // return 8 bit as is
        color = rgbToGray(color);
        color &= ((1<<bpp)-1)<<(8-bpp);
        return (lUInt8)color;
    default:
        color = rgbToGray(color);
        return (lUInt8)color;
    }
    return (lUInt8)color;
}

void ApplyAlphaRGB( lUInt32 &dst, lUInt32 src, lUInt32 alpha )
{
    if ( alpha == 0 ) {
        dst = src;
    } else if ( alpha < 255 ) {
        src &= 0xFFFFFF;
        lUInt32 opaque = alpha ^ 0xFF;
        lUInt32 n1 = (((dst & 0xFF00FF) * alpha + (src & 0xFF00FF) * opaque) >> 8) & 0xFF00FF;
        lUInt32 n2 = (((dst & 0x00FF00) * alpha + (src & 0x00FF00) * opaque) >> 8) & 0x00FF00;
        dst = n1 | n2;
    }
}

void ApplyAlphaRGB565( lUInt16 &dst, lUInt16 src, lUInt32 alpha )
{
    if ( alpha==0 ) {
        dst = src;
    } else if ( alpha < 255 ) {
        lUInt32 opaque = alpha ^ 0xFF;
        lUInt32 r = (((dst & 0xF800) * alpha + (src & 0xF800) * opaque) >> 8) & 0xF800;
        lUInt32 g = (((dst & 0x07E0) * alpha + (src & 0x07E0) * opaque) >> 8) & 0x07E0;
        lUInt32 b = (((dst & 0x001F) * alpha + (src & 0x001F) * opaque) >> 8) & 0x001F;
        dst = (lUInt16)(r | g | b);
    }
}

// obsoleted, ready to remove
void ApplyAlphaGray( lUInt8 &dst, lUInt8 src, lUInt32 alpha, int bpp )
{
    if ( alpha==0 ) {
        dst = src;
    } else if ( alpha < 255 ) {
        int mask = ((1 << bpp) - 1) << (8 - bpp);
        src &= mask;
        lUInt32 opaque = alpha ^ 0xFF;
        lUInt32 n1 = ((dst * alpha + src * opaque) >> 8) & mask;
        dst = (lUInt8)n1;
    }
}

/*
void ApplyAlphaGray8( lUInt8 &dst, lUInt8 src, lUInt8 alpha )
{
    if ( alpha==0 ) {
        dst = src;
    } else if ( alpha < 255 ) {
        lUInt8 opaque = alpha ^ 0xFF;
        lUInt8 v = ((dst * alpha + src * opaque) >> 8);
        dst = (lUInt8) v;
    }
}
*/
