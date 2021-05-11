/*******************************************************

   CoolReader Engine

   lvcolortransformimgsource.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvcolortransformimgsource.h"
#include "lvcolordrawbuf.h"

static inline lUInt32 limit256(int n) {
    if (n < 0)
        return 0;
    else if (n > 0xFF)
        return 0xFF;
    else
        return (lUInt32)n;
}


LVColorTransformImgSource::LVColorTransformImgSource(LVImageSourceRef src, lUInt32 addRGB, lUInt32 multiplyRGB)
    : _src( src )
    , _add(addRGB)
    , _multiply(multiplyRGB)
    , _callback(NULL)
    , _drawbuf(NULL)
    , _sumR(0)
    , _sumG(0)
    , _sumB(0)
    , _countPixels(0)
{
}

LVColorTransformImgSource::~LVColorTransformImgSource() {
    if (_drawbuf)
        delete _drawbuf;
}

void LVColorTransformImgSource::OnStartDecode(LVImageSource *)
{
    _callback->OnStartDecode(this);
    _sumR = _sumG = _sumB = _countPixels = 0;
    if (_drawbuf)
        delete _drawbuf;
    _drawbuf = new LVColorDrawBuf(_src->GetWidth(), _src->GetHeight(), 32);
}

bool LVColorTransformImgSource::OnLineDecoded(LVImageSource *obj, int y, lUInt32 *data) {
    CR_UNUSED(obj);
    int dx = _src->GetWidth();
    
    lUInt32 * row = (lUInt32*)_drawbuf->GetScanLine(y);
    for (int x = 0; x < dx; x++) {
        lUInt32 cl = data[x];
        row[x] = cl;
        if (((cl >> 24) & 0xFF) < 0xC0) { // count non-transparent pixels only
            _sumR += (cl >> 16) & 0xFF;
            _sumG += (cl >> 8) & 0xFF;
            _sumB += (cl >> 0) & 0xFF;
            _countPixels++;
        }
    }
    return true;
    
}

void LVColorTransformImgSource::OnEndDecode(LVImageSource *obj, bool res)
{
    int dx = _src->GetWidth();
    int dy = _src->GetHeight();
    // simple add
    int ar = (((_add >> 16) & 0xFF) - 0x80) * 2;
    int ag = (((_add >> 8) & 0xFF) - 0x80) * 2;
    int ab = (((_add >> 0) & 0xFF) - 0x80) * 2;
    // fixed point * 256
    int mr = ((_multiply >> 16) & 0xFF) << 3;
    int mg = ((_multiply >> 8) & 0xFF) << 3;
    int mb = ((_multiply >> 0) & 0xFF) << 3;
    
    int avgR = _countPixels > 0 ? _sumR / _countPixels : 128;
    int avgG = _countPixels > 0 ? _sumG / _countPixels : 128;
    int avgB = _countPixels > 0 ? _sumB / _countPixels : 128;
    
    for (int y = 0; y < dy; y++) {
        lUInt32 * row = (lUInt32*)_drawbuf->GetScanLine(y);
        for ( int x=0; x<dx; x++ ) {
            lUInt32 cl = row[x];
            lUInt32 a = cl & 0xFF000000;
            if (a != 0xFF000000) {
                int r = (cl >> 16) & 0xFF;
                int g = (cl >> 8) & 0xFF;
                int b = (cl >> 0) & 0xFF;
                r = (((r - avgR) * mr) >> 8) + avgR + ar;
                g = (((g - avgG) * mg) >> 8) + avgG + ag;
                b = (((b - avgB) * mb) >> 8) + avgB + ab;
                row[x] = a | (limit256(r) << 16) | (limit256(g) << 8) | (limit256(b) << 0);
            }
        }
        _callback->OnLineDecoded(obj, y, row);
    }
    if (_drawbuf)
        delete _drawbuf;
    _drawbuf = NULL;
    _callback->OnEndDecode(this, res);
}

bool LVColorTransformImgSource::Decode(LVImageDecoderCallback *callback)
{
    _callback = callback;
    return _src->Decode( this );
}
