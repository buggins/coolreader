/*******************************************************

   CoolReader Engine

   lvbasedrawbuf.cpp:  Base bitmap buffer class

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvbasedrawbuf.h"

inline static lUInt32 AA(lUInt32 color) {
    return (color >> 24) & 0xFF;
}

inline static lUInt32 RR(lUInt32 color) {
	return (color >> 16) & 0xFF;
}

inline static lUInt32 GG(lUInt32 color) {
	return (color >> 8) & 0xFF;
}

inline static lUInt32 BB(lUInt32 color) {
	return color & 0xFF;
}

inline static lUInt32 RRGGBB(lUInt32 r, lUInt32 g, lUInt32 b) {
	return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

inline static lUInt32 AARRGGBB(lUInt32 a, lUInt32 r, lUInt32 g, lUInt32 b) {
    return ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
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


/// get linearly interpolated pixel value (coordinates are fixed floating points *16)
lUInt32 LVBaseDrawBuf::GetInterpolatedColor(int x16, int y16)
{
	int shx = x16 & 0x0F;
	int shy = y16 & 0x0F;
	int nshx = 16 - shx;
	int nshy = 16 - shy;
	int x = x16 >> 4;
	int y = y16 >> 4;
	int x1 = x + 1;
	int y1 = y + 1;
	if (x1 >= _dx)
		x1 = x;
	if (y1 >= _dy)
		y1 = y;
    lUInt32 cl00 = GetPixel(x, y);
    lUInt32 cl01 = GetPixel(x1, y);
    lUInt32 cl10 = GetPixel(x, y1);
    lUInt32 cl11 = GetPixel(x1, y1);
    lUInt32 a = (((AA(cl00) * nshx + AA(cl01) * shx) * nshy +
                  (AA(cl10) * nshx + AA(cl11) * shx) * shy) >> 8) & 0xFF;
    lUInt32 r = (((RR(cl00) * nshx + RR(cl01) * shx) * nshy +
                  (RR(cl10) * nshx + RR(cl11) * shx) * shy) >> 8) & 0xFF;
	lUInt32 g = (((GG(cl00) * nshx + GG(cl01) * shx) * nshy +
                  (GG(cl10) * nshx + GG(cl11) * shx) * shy) >> 8) & 0xFF;
	lUInt32 b = (((BB(cl00) * nshx + BB(cl01) * shx) * nshy +
                  (BB(cl10) * nshx + BB(cl11) * shx) * shy) >> 8) & 0xFF;
    return AARRGGBB(a, r, g, b);
}

/// get average pixel value for area (coordinates are fixed floating points *16)
lUInt32 LVBaseDrawBuf::GetAvgColor(lvRect & rc16)
{
    if (!_data)
        return 0;
    int x0 = rc16.left;
    int y0 = rc16.top;
    int x1 = rc16.right;
    int y1 = rc16.bottom;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    int maxxx = _dx << 4;
    int maxyy = _dy << 4;
    if (x1 > maxxx)
        x1 = maxxx;
    if (y1 > maxyy)
        y1 = maxyy;
    if (x0 > x1 || y0 > y1)
        return 0; // invalid rectangle
    int rs = 0;
    int gs = 0;
    int bs = 0;
    int s = 0;
    int maxy = ((y1 - 1) >> 4);
    int maxx = ((x1 - 1) >> 4);
    for (int y = (y0 >> 4); y <= maxy; y++ ) {
        int yy0 = y << 4;
        int yy1 = (y + 1) << 4;
        if (yy0 < y0)
            yy0 = y0;
        if (yy1 > y1)
            yy1 = y1;
        int ys = yy1 - yy0; // 0..16
        if (ys < 1)
            continue;
        for (int x = (x0 >> 4); x <= maxx; x++ ) {

            int xx0 = x << 4;
            int xx1 = (x + 1) << 4;
            if (xx0 < x0)
                xx0 = x0;
            if (xx1 > x1)
                xx1 = x1;
            int xs = xx1 - xx0; // 0..16
            if (xs < 1)
                continue;

            int mult = xs * ys;

            lUInt32 pixel = GetPixel(x, y);
            int r = (pixel >> 16) & 0xFF;
            int g = (pixel >> 8) & 0xFF;
            int b = pixel & 0xFF;

            rs += r * mult;
            gs += g * mult;
            bs += b * mult;
            s += mult;
        }
    }

    if (s == 0)
        return 0;
    rs = (rs / s) & 0xFF;
    gs = (gs / s) & 0xFF;
    bs = (bs / s) & 0xFF;
    return (rs << 16) | (gs << 8) | bs;
}
