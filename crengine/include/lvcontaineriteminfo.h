/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __LVCONTAINERITEMINFO_H_INCLUDED__
#define __LVCONTAINERITEMINFO_H_INCLUDED__

#include "lvstream_types.h"

class LVContainerItemInfo
{
public:
    virtual lvsize_t        GetSize() const = 0;
    virtual const lChar32 * GetName() const = 0;
    virtual lUInt32         GetFlags() const = 0;
    virtual bool            IsContainer() const = 0;
    LVContainerItemInfo() {}
    virtual ~LVContainerItemInfo() {}
};

#endif  // __LVCONTAINERITEMINFO_H_INCLUDED__
