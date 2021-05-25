/** @file lvdrawbuf_utils.h

    CoolReader Engine: drawbuff private stuff

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVDRAWBUF_UTILS_H_INCLUDED__
#define __LVDRAWBUF_UTILS_H_INCLUDED__

#include "lvtypes.h"

#define GUARD_BYTE 0xa5
#define CHECK_GUARD_BYTE \
	{ \
        if (_bpp != 1 && _bpp != 2 && _bpp !=3 && _bpp != 4 && _bpp != 8 && _bpp != 16 && _bpp != 32) crFatalError(-5, "wrong bpp"); \
        if (_ownData && _data && _data[_rowsize * _dy] != GUARD_BYTE) crFatalError(-5, "corrupted bitmap buffer"); \
    }

// NOTE: By default, CRe assumes RGB (array order) actually means BGR
//       We don't, so, instead of fixing this at the root (i.e., in a *lot* of places),
//       we simply swap R<->B when rendering to 32bpp, limiting the tweaks to lvdrawbuf
//       c.f., https://github.com/koreader/koreader-base/pull/878#issuecomment-476723747
#ifdef CR_RENDER_32BPP_RGB_PXFMT
inline lUInt32 RevRGB( lUInt32 cl ) {
    return ((cl<<16)&0xFF0000) | ((cl>>16)&0x0000FF) | (cl&0x00FF00);
}

inline lUInt32 RevRGBA( lUInt32 cl ) {
    // Swap B <-> R, keep G & A
    return ((cl<<16)&0x00FF0000) | ((cl>>16)&0x000000FF) | (cl&0xFF00FF00);
}
#else
inline lUInt32 RevRGB( lUInt32 cl ) {
    return cl;
}

inline lUInt32 RevRGBA( lUInt32 cl ) {
    return cl;
}
#endif

inline lUInt32 rgb565to888( lUInt32 cl ) {
    return ((cl & 0xF800)<<8) | ((cl & 0x07E0)<<5) | ((cl & 0x001F)<<3);
}

inline lUInt16 rgb888to565( lUInt32 cl ) {
    return (lUInt16)(((cl>>8)& 0xF800) | ((cl>>5 )& 0x07E0) | ((cl>>3 )& 0x001F));
}

#define DIV255(V, t)                                                                                    \
{                                                                                                       \
        auto _v = (V) + 128;                                                                            \
        (t) = (((_v >> 8U) + _v) >> 8U);                                                                \
}

// Because of course we're not using <stdint.h> -_-".
#ifndef UINT8_MAX
	#define UINT8_MAX (255U)
#endif

lUInt32 Dither1BitColor( lUInt32 color, lUInt32 x, lUInt32 y );

lUInt32 Dither2BitColor( lUInt32 color, lUInt32 x, lUInt32 y );

// returns byte with higher significant bits, lower bits are 0
lUInt32 DitherNBitColor( lUInt32 color, lUInt32 x, lUInt32 y, int bits );

lUInt32 rgbToGray( lUInt32 color );

lUInt8 rgbToGray( lUInt32 color, int bpp );

lUInt8 rgbToGrayMask( lUInt32 color, int bpp );

void ApplyAlphaRGB( lUInt32 &dst, lUInt32 src, lUInt32 alpha );

void ApplyAlphaRGB565( lUInt16 &dst, lUInt16 src, lUInt32 alpha );

// obsoleted, ready to remove
void ApplyAlphaGray( lUInt8 &dst, lUInt8 src, lUInt32 alpha, int bpp );

//void ApplyAlphaGray8( lUInt8 &dst, lUInt8 src, lUInt8 alpha );

#endif  // __LVDRAWBUF_UTILS_H_INCLUDED__
