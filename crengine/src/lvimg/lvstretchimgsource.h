/** @file lvstretchimgsource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVSTRETCHIMGSOURCE_H_INCLUDED__
#define __LVSTRETCHIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"
#include "lvarray.h"
#include "lvimg.h"

class LVStretchImgSource : public LVImageSource, public LVImageDecoderCallback
{
protected:
    LVImageSourceRef _src;
    int _src_dx;
    int _src_dy;
    int _dst_dx;
    int _dst_dy;
    ImageTransform _hTransform;
    ImageTransform _vTransform;
    int _split_x;
    int _split_y;
    LVArray<lUInt32> _line;
    LVImageDecoderCallback * _callback;
public:
    LVStretchImgSource( LVImageSourceRef src, int newWidth, int newHeight, ImageTransform hTransform, ImageTransform vTransform, int splitX, int splitY );
    virtual ~LVStretchImgSource();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource *, bool res);
    virtual ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _dst_dx; }
    virtual int    GetHeight() const { return _dst_dy; }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVSTRETCHIMGSOURCE_H_INCLUDED__
