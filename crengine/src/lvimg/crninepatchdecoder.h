/** @file crninepatchdecoder.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __CRNINEPATCHDECODER_H_INCLUDED__
#define __CRNINEPATCHDECODER_H_INCLUDED__

#include "lvimagedecodercallback.h"
#include "lvimagesource.h"

class CRNinePatchDecoder : public LVImageDecoderCallback {
    int _dx;
    int _dy;
    CR9PatchInfo *_info;
public:
    CRNinePatchDecoder(int dx, int dy, CR9PatchInfo *info) : _dx(dx), _dy(dy), _info(info) {
    }
    virtual ~CRNinePatchDecoder() {}
    virtual void OnStartDecode(LVImageSource *obj) {
        CR_UNUSED(obj);
    }
    bool isUsedPixel(lUInt32 pixel);
    void decodeHLine(lUInt32 *line, int &x0, int &x1);
    void decodeVLine(lUInt32 pixel, int y, int &y0, int &y1);
    virtual bool OnLineDecoded(LVImageSource *obj, int y, lUInt32 *data);
    virtual void OnEndDecode(LVImageSource *obj, bool errors) {
        CR_UNUSED2(obj, errors);
    }
};

#endif  // __CRNINEPATCHDECODER_H_INCLUDED__
