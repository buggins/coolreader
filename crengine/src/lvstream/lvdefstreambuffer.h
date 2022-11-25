/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
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

/**
 * \file lvdefstreambuffer.h
 * \brief Universal Read or write buffer for stream region for non-maped streams
 */

#ifndef __LVDEFSTREAMBUFFER_H_INCLUDED__
#define __LVDEFSTREAMBUFFER_H_INCLUDED__

#include "lvstreambuffer.h"
#include "lvstream.h"

// default implementation, with RAM buffer
class LVDefStreamBuffer : public LVStreamBuffer
{
protected:
    LVStreamRef m_stream;
    lUInt8 * m_buf;
    lvpos_t m_pos;
    lvsize_t m_size;
    bool m_readonly;
    bool m_writeonly;
public:
    static LVStreamBufferRef create( LVStreamRef stream, lvpos_t pos, lvsize_t size, bool readonly );

    LVDefStreamBuffer( LVStreamRef stream, lvpos_t pos, lvsize_t size, bool readonly );
    /// get pointer to read-only buffer, returns NULL if unavailable
    virtual const lUInt8 * getReadOnly()
    {
        return m_writeonly ? NULL : m_buf;
    }
    /// get pointer to read-write buffer, returns NULL if unavailable
    virtual lUInt8 * getReadWrite()
    {
        return m_readonly ? NULL : m_buf;
    }
    /// get buffer size
    virtual lvsize_t getSize()
    {
        return m_size;
    }
    /// write on close
    virtual bool close();
    /// flush on destroy
    virtual ~LVDefStreamBuffer()
    {
        close(); // NOLINT: Call to virtual function during destruction
    }
};

#endif  // __LVDEFSTREAMBUFFER_H_INCLUDED__
