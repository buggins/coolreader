/** @file lvimagedecodercallback.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVIMAGEDECODERCALLBACK_H_INCLUDED__
#define __LVIMAGEDECODERCALLBACK_H_INCLUDED__

#include "lvtypes.h"

class LVImageSource;

/// image decoding callback interface
class LVImageDecoderCallback
{
public:
    virtual ~LVImageDecoderCallback() {}
    virtual void OnStartDecode( LVImageSource * obj ) = 0;
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data ) = 0;
    virtual void OnEndDecode( LVImageSource * obj, bool errors ) = 0;
};

#endif  // __LVIMAGEDECODERCALLBACK_H_INCLUDED__
