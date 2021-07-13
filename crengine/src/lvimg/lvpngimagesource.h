/** @file lvpngimagesource.h
    @brief library private stuff: png image decoder

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVPNGIMAGESOURCE_H_INCLUDED__
#define __LVPNGIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_LIBPNG==1)

#include "lvnodeimagesource.h"

class LVPngImageSource : public LVNodeImageSource
{
protected:
public:
    LVPngImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVPngImageSource();
    virtual void   Compact();
    virtual bool   Decode( LVImageDecoderCallback * callback );
    static bool CheckPattern( const lUInt8 * buf, int len );
};

#endif  // (USE_LIBPNG==1)

#endif  // __LVPNGIMAGESOURCE_H_INCLUDED__
