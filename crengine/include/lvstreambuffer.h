/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __LVSTREAMBUFFER_H_INCLUDED__
#define __LVSTREAMBUFFER_H_INCLUDED__

#include "lvref.h"
#include "lvstream_types.h"

/// Read or write buffer for stream region
class LVStreamBuffer : public LVRefCounter
{
public:
    /// get pointer to read-only buffer, returns NULL if unavailable
    virtual const lUInt8 * getReadOnly() = 0;
    /// get pointer to read-write buffer, returns NULL if unavailable
    virtual lUInt8 * getReadWrite() = 0;
    /// get buffer size
    virtual lvsize_t getSize() = 0;
    /// flush on destroy
    virtual ~LVStreamBuffer() {
        close(); // NOLINT: Call to virtual function during destruction
    }
    /// detach from stream, write changes if necessary
    virtual bool close() { return true; }
};

typedef LVFastRef<LVStreamBuffer> LVStreamBufferRef;

#endif  // __LVSTREAMBUFFER_H_INCLUDED__
