/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef __LVSTORAGEOBJECT_H_INCLUDED__
#define __LVSTORAGEOBJECT_H_INCLUDED__

#include "lvref.h"
#include "lvstream_types.h"

class LVContainer;

class LVStorageObject : public LVRefCounter
{
public:
    // construction/destruction
    //LVStorageObject() {  }
    virtual ~LVStorageObject() { }
    // storage object methods
    /// returns true for container (directory), false for stream (file)
    virtual bool IsContainer()
    {
        return false;
    }
    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName()
    {
        return NULL;
    }
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name)
    {
    }
    /// returns parent container, if opened from container
    virtual LVContainer * GetParentContainer()
    {
        return NULL;
    }
    /// returns object size (file size or directory entry count)
    virtual lverror_t GetSize( lvsize_t * pSize ) = 0;
    /// returns object size (file size or directory entry count)
    virtual lvsize_t GetSize( )
    {
        lvsize_t sz;
        if ( GetSize( &sz )!=LVERR_OK )
            return LV_INVALID_SIZE;
        return sz;
    }
};

#endif  // __LVSTORAGEOBJECT_H_INCLUDED__
