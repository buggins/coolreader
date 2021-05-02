/** @file lvcolortransformimgsource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVCOLORTRANSFORMIMGSOURCE_H_INCLUDED__
#define __LVCOLORTRANSFORMIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"

class LVColorDrawBuf;

class LVColorTransformImgSource : public LVImageSource, public LVImageDecoderCallback
{
protected:
    LVImageSourceRef _src;
    lUInt32 _add;
    lUInt32 _multiply;
    LVImageDecoderCallback * _callback;
    LVColorDrawBuf * _drawbuf;
    int _sumR;
    int _sumG;
    int _sumB;
    int _countPixels;
public:
    LVColorTransformImgSource(LVImageSourceRef src, lUInt32 addRGB, lUInt32 multiplyRGB);
    virtual ~LVColorTransformImgSource();
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

#endif  // __LVCOLORTRANSFORMIMGSOURCE_H_INCLUDED__
