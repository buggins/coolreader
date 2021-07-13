#include "lvinkmeasurementdrawbuf.h"

void LVInkMeasurementDrawBuf::updateInkBounds(int x0, int y0, int x1, int y1) {
    if ( has_ink ) {
        if ( x0 < ink_left_x ) ink_left_x = x0;
        if ( x1 < ink_left_x ) ink_left_x = x1;
        if ( x1 > ink_right_x ) ink_right_x = x1;
        if ( x0 > ink_right_x ) ink_right_x = x0;
        if ( y0 < ink_top_y ) ink_top_y = y0;
        if ( y1 < ink_top_y ) ink_top_y = y1;
        if ( y1 > ink_bottom_y ) ink_bottom_y = y1;
        if ( y0 > ink_bottom_y ) ink_bottom_y = y0;
    }
    else {
        ink_left_x = x0 < x1 ? x0 : x1;
        ink_right_x = x0 > x1 ? x0 : x1;
        ink_top_y = y0 < y1 ? y0 : y1;
        ink_bottom_y = y0 > y1 ? y0 : y1;
        has_ink = true;
    }
}

bool LVInkMeasurementDrawBuf::getInkArea(lvRect &rect) {
    if ( has_ink ) {
        rect.top = ink_top_y;
        rect.bottom = ink_bottom_y;
        rect.left = ink_left_x;
        rect.right = ink_right_x;
        return true;
    }
    return false;
}

void LVInkMeasurementDrawBuf::FillRect(int x0, int y0, int x1, int y1, lUInt32 color) {
    if ( ignore_decorations )
        return;
    // printf("  ink FillRect %d %d %d %d\n", x0, y0, x1, y1);
    if ( color != GetBackgroundColor() )
        updateInkBounds(x0, y0, x1, y1);
}

void LVInkMeasurementDrawBuf::FillRectPattern(int x0, int y0, int x1, int y1, lUInt32 color0, lUInt32 color1, lUInt8 *pattern) {
    if ( ignore_decorations )
        return;
    FillRect( x0, y0, x1, y1, color0);
}

void LVInkMeasurementDrawBuf::Draw(LVImageSourceRef img, int x, int y, int width, int height, bool dither) {
    // An image (even if empty) sets the ink area
    // printf("  ink Draw image %d %d %d %d\n", x, y, width, height);
    updateInkBounds(x, y, x+width, y+height);
}

void LVInkMeasurementDrawBuf::BlendBitmap( int x, int y, const lUInt8 * bitmap, FontBmpPixelFormat bitmap_fmt, int width, int height, int bmp_pitch, lUInt32 * palette ) {
    // printf("  ink Draw %d %d %d %d\n", x, y, width, height);
    // Used to draw glyph. Trust the font that the bitmap is the glyph
    // bounding box ("blackbox" in cre), so its ink area
    updateInkBounds(x, y, x+width, y+height);
}

void LVInkMeasurementDrawBuf::DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0, int length1, int length2, int direction) {
    if ( ignore_decorations )
        return;
    // printf("  ink DrawLine %d %d %d %d\n", x0, y0, x1, y1);
    updateInkBounds(x0, y0, x1, y1);
}

void LVInkMeasurementDrawBuf::GetClipRect(lvRect *clipRect) const {
    // Drawing code might request a clip, but we don't want to impose any.
    // So, have a large dynamic one around the ink area met until then
    clipRect->top = ink_top_y - 1000;
    clipRect->bottom = ink_bottom_y + 1000;
    clipRect->left = ink_left_x - 1000;
    clipRect->right = ink_right_x + 1000;
}
