/** @file lvdrawbufimgsource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVDRAWBUFIMGSOURCE_H_INCLUDED__
#define __LVDRAWBUFIMGSOURCE_H_INCLUDED__

#include "lvimagesource.h"

class LVImageDecoderCallback;
class LVColorDrawBuf;

class LVDrawBufImgSource : public LVImageSource
{
protected:
    LVColorDrawBuf * _buf;
    bool _own;
    int _dx;
    int _dy;
public:
    LVDrawBufImgSource( LVColorDrawBuf * buf, bool own );
    virtual ~LVDrawBufImgSource();
    virtual ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() { return _dx; }
    virtual int    GetHeight() { return _dy; }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVDRAWBUFIMGSOURCE_H_INCLUDED__
