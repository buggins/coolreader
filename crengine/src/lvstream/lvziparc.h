/** @file lvziparc.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVZIPARC_H_INCLUDED__
#define __LVZIPARC_H_INCLUDED__

#include "crsetup.h"

#if (USE_ZLIB==1)

#include "lvarccontainerbase.h"

class LVZipArc : public LVArcContainerBase
{
protected:
    // whether the alternative "truncated" method was used, or is to be used
    bool m_alt_reading_method = false;
public:
    LVZipArc( LVStreamRef stream );
    virtual ~LVZipArc();

    bool isAltReadingMethod() { return m_alt_reading_method; }
    void setAltReadingMethod() { m_alt_reading_method = true; }

    virtual LVStreamRef OpenStream( const char32_t * fname, lvopen_mode_t /*mode*/ );
    virtual int ReadContents();

    static LVArcContainerBase * OpenArchieve( LVStreamRef stream );
};

#endif  // (USE_ZLIB==1)

#endif  // __LVZIPARC_H_INCLUDED__
