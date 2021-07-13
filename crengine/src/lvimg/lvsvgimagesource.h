/** @file lvsvgimagesource.h
    @brief library private stuff: svg image decoder

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVSVGIMAGESOURCE_H_INCLUDED__
#define __LVSVGIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_NANOSVG==1)

#include "lvnodeimagesource.h"

class LVSvgImageSource : public LVNodeImageSource
{
protected:
public:
    LVSvgImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVSvgImageSource();
    virtual void   Compact();
    virtual bool   Decode( LVImageDecoderCallback * callback );
    int DecodeFromBuffer(unsigned char *buf, int buf_size, LVImageDecoderCallback * callback);
    static bool CheckPattern( const lUInt8 * buf, int len );
};

#endif  // (USE_NANOSVG==1)

#endif  // __LVSVGIMAGESOURCE_H_INCLUDED__
