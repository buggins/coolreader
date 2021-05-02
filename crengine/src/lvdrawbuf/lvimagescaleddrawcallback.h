/** @file lvimagescaleddrawcallback.h

    CoolReader Engine: library private stuff

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVIMAGESCALEDDRAWCALLBACK_H_INCLUDED__
#define __LVIMAGESCALEDDRAWCALLBACK_H_INCLUDED__

#include "lvimagedecodercallback.h"
#include "lvbasedrawbuf.h"

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
    bool invert;
    bool smoothscale;
    lUInt8 * decoded;
    bool isNinePatch;
public:
    static int * GenMap( int src_len, int dst_len );
    static int * GenNinePatchMap( int src_len, int dst_len, int frame1, int frame2);
    LVImageScaledDrawCallback(LVBaseDrawBuf * dstbuf, LVImageSourceRef img, int x, int y, int width, int height, bool dith, bool inv, bool smooth );
    virtual ~LVImageScaledDrawCallback();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource *, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource * obj, bool );
};

#endif  // __LVIMAGESCALEDDRAWCALLBACK_H_INCLUDED__
