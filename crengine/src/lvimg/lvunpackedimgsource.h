/** @file lvunpackedimgsource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVUNPACKEDIMGSOURCE_H_INCLUDED__
#define __LVUNPACKEDIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"

class LVUnpackedImgSource : public LVImageSource, public LVImageDecoderCallback
{
protected:
    bool _isGray;
    int _bpp;
    lUInt8 * _grayImage;
    lUInt32 * _colorImage;
    lUInt16 * _colorImage16;
    int _dx;
    int _dy;
public:
    LVUnpackedImgSource( LVImageSourceRef src, int bpp );
    virtual ~LVUnpackedImgSource();
    virtual void OnStartDecode( LVImageSource * );
    virtual bool OnLineDecoded( LVImageSource *, int y, lUInt32 * data );
    virtual void OnEndDecode( LVImageSource *, bool );
    virtual ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _dx; }
    virtual int    GetHeight() const { return _dy; }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVUNPACKEDIMGSOURCE_H_INCLUDED__
