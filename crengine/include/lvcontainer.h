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

#ifndef __LVCONTAINER_H_INCLUDED__
#define __LVCONTAINER_H_INCLUDED__

#include "lvstorageobject.h"

class LVStream;
class LVContainerItemInfo;

typedef LVFastRef<LVStream> LVStreamRef;

class LVContainer : public LVStorageObject
{
public:
    virtual LVContainer * GetParentContainer() = 0;
    //virtual const LVContainerItemInfo * GetObjectInfo(const char32_t * pname);
    virtual const LVContainerItemInfo * GetObjectInfo(int index) = 0;
    virtual const LVContainerItemInfo * operator [] (int index) { return GetObjectInfo(index); }
    virtual int GetObjectCount() const = 0;
    virtual LVStreamRef OpenStream( const lChar32 * fname, lvopen_mode_t mode ) = 0;
    LVContainer() {}
    virtual ~LVContainer() { }
};

/// Container reference
typedef LVFastRef<LVContainer> LVContainerRef;

#endif  // __LVCONTAINER_H_INCLUDED__
