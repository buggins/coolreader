/*******************************************************

   CoolReader Engine

   lvimagesource.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvimagesource.h"
#include "crninepatchdecoder.h"

static void fixNegative(int &n) {
    if (n < 0)
        n = 0;
}


void CR9PatchInfo::applyPadding(lvRect &dstPadding) const {
    if (dstPadding.left < padding.left)
        dstPadding.left = padding.left;
    if (dstPadding.right < padding.right)
        dstPadding.right = padding.right;
    if (dstPadding.top < padding.top)
        dstPadding.top = padding.top;
    if (dstPadding.bottom < padding.bottom)
        dstPadding.bottom = padding.bottom;
}

static void fixNegative(int n[4]) {
    int d1 = n[1] - n[0];
    int d2 = n[3] - n[2];
    if (d1 + d2 > 0) {
        n[1] = n[2] = n[0] + (n[3] - n[0]) * d1 / (d1 + d2);
    } else {
        n[1] = n[2] = (n[0] + n[3]) / 2;
    }
}

/// caclulate dst and src rectangles (src does not include layout frame)
void CR9PatchInfo::calcRectangles(const lvRect &dst, const lvRect &src, lvRect dstitems[9],
                                  lvRect srcitems[9]) const {
    for (int i = 0; i < 9; i++) {
        srcitems[i].clear();
        dstitems[i].clear();
    }
    if (dst.isEmpty() || src.isEmpty())
        return;

    int sx[4], sy[4], dx[4], dy[4];
    sx[0] = src.left;
    sx[1] = src.left + frame.left;
    sx[2] = src.right - frame.right;
    sx[3] = src.right;
    sy[0] = src.top;
    sy[1] = src.top + frame.top;
    sy[2] = src.bottom - frame.bottom;
    sy[3] = src.bottom;
    dx[0] = dst.left;
    dx[1] = dst.left + frame.left;
    dx[2] = dst.right - frame.right;
    dx[3] = dst.right;
    dy[0] = dst.top;
    dy[1] = dst.top + frame.top;
    dy[2] = dst.bottom - frame.bottom;
    dy[3] = dst.bottom;
    if (dx[1] > dx[2]) {
        // shrink horizontal
        fixNegative(dx);
    }
    if (dy[1] > dy[2]) {
        // shrink vertical
        fixNegative(dy);
    }
    // fill calculated rectangles
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            int i = y * 3 + x;
            srcitems[i].left = sx[x];
            srcitems[i].right = sx[x + 1];
            srcitems[i].top = sy[y];
            srcitems[i].bottom = sy[y + 1];
            dstitems[i].left = dx[x];
            dstitems[i].right = dx[x + 1];
            dstitems[i].top = dy[y];
            dstitems[i].bottom = dy[y + 1];
        }
    }
}

CR9PatchInfo *LVImageSource::DetectNinePatch() {
    if (_ninePatch)
        return _ninePatch;
    _ninePatch = new CR9PatchInfo();
    CRNinePatchDecoder decoder(GetWidth(), GetHeight(), _ninePatch);
    Decode(&decoder);
    if (_ninePatch->frame.left > 0 && _ninePatch->frame.top > 0
        && _ninePatch->frame.left < _ninePatch->frame.right
        && _ninePatch->frame.top < _ninePatch->frame.bottom) {
        // remove 1 pixel frame
        _ninePatch->padding.left--;
        _ninePatch->padding.top--;
        _ninePatch->padding.right = GetWidth() - _ninePatch->padding.right - 1;
        _ninePatch->padding.bottom = GetHeight() - _ninePatch->padding.bottom - 1;
        fixNegative(_ninePatch->padding.left);
        fixNegative(_ninePatch->padding.top);
        fixNegative(_ninePatch->padding.right);
        fixNegative(_ninePatch->padding.bottom);
        _ninePatch->frame.left--;
        _ninePatch->frame.top--;
        _ninePatch->frame.right = GetWidth() - _ninePatch->frame.right - 1;
        _ninePatch->frame.bottom = GetHeight() - _ninePatch->frame.bottom - 1;
        fixNegative(_ninePatch->frame.left);
        fixNegative(_ninePatch->frame.top);
        fixNegative(_ninePatch->frame.right);
        fixNegative(_ninePatch->frame.bottom);
    } else {
        delete _ninePatch;
        _ninePatch = NULL;
    }
    return _ninePatch;
}

LVImageSource::~LVImageSource() {
    if (_ninePatch)
        delete _ninePatch;
}
