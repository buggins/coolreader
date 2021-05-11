/** @file lvalphatransformimgsource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVALPHATRANSFORMIMGSOURCE_H_INCLUDED__
#define __LVALPHATRANSFORMIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"

class LVAlphaTransformImgSource : public LVImageSource, public LVImageDecoderCallback
{
protected:
    LVImageSourceRef _src;
    LVImageDecoderCallback * _callback;
    int _alpha;
public:
    LVAlphaTransformImgSource(LVImageSourceRef src, int alpha);
    virtual ~LVAlphaTransformImgSource();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource * obj, bool res);
    virtual ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() { return _src->GetWidth(); }
    virtual int    GetHeight() { return _src->GetHeight(); }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVALPHATRANSFORMIMGSOURCE_H_INCLUDED__
