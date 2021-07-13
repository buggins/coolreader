/** @file lvrararc.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVRARARC_H_INCLUDED__
#define __LVRARARC_H_INCLUDED__

#include "crsetup.h"

#if (USE_UNRAR==1)

#include "lvarccontainerbase.h"

class LVRarArc : public LVArcContainerBase
{
public:
    LVRarArc( LVStreamRef stream ) : LVArcContainerBase(stream)
    {
    }
    virtual ~LVRarArc()
    {
    }

    virtual LVStreamRef OpenStream( const lChar32 * fname, lvopen_mode_t mode );
    virtual int ReadContents();

    static LVArcContainerBase * OpenArchieve( LVStreamRef stream );
};

#endif  // (USE_UNRAR==1)

#endif  // __LVRARARC_H_INCLUDED__
