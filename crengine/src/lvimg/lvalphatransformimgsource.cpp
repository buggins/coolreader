/*******************************************************

   CoolReader Engine

   lvalphatransformimgsource.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvalphatransformimgsource.h"

LVAlphaTransformImgSource::LVAlphaTransformImgSource(LVImageSourceRef src, int alpha)
    : _src( src )
    , _alpha(alpha ^ 0xFF)
{
}

LVAlphaTransformImgSource::~LVAlphaTransformImgSource() {
}

void LVAlphaTransformImgSource::OnStartDecode(LVImageSource *)
{
    _callback->OnStartDecode(this);
}

bool LVAlphaTransformImgSource::OnLineDecoded(LVImageSource *obj, int y, lUInt32 *data) {
    CR_UNUSED(obj);
    int dx = _src->GetWidth();
    
    for (int x = 0; x < dx; x++) {
        lUInt32 cl = data[x];
        int srcalpha = (cl >> 24) ^ 0xFF;
        if (srcalpha > 0) {
            srcalpha = _alpha * srcalpha;
            cl = (cl & 0xFFFFFF) | (((_alpha * srcalpha) ^ 0xFF)<<24);
        }
        data[x] = cl;
    }
    return _callback->OnLineDecoded(obj, y, data);
}

void LVAlphaTransformImgSource::OnEndDecode(LVImageSource *obj, bool res)
{
    CR_UNUSED(obj);
    _callback->OnEndDecode(this, res);
}

bool LVAlphaTransformImgSource::Decode(LVImageDecoderCallback *callback)
{
    _callback = callback;
    return _src->Decode( this );
}
