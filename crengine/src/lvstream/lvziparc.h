/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2019,2020 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

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
