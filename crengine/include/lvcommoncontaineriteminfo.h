/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __LVCOMMONCONTAINERITEMINFO_H_INCLUDED__
#define __LVCOMMONCONTAINERITEMINFO_H_INCLUDED__

#include "lvcontaineriteminfo.h"
#include "lvstring.h"

class LVCommonContainerItemInfo : public LVContainerItemInfo
{
    friend class LVDirectoryContainer;
    friend class LVArcContainer;
protected:
    lvsize_t     m_size;
    lString32    m_name;
    lUInt32      m_flags;
    bool         m_is_container;
    lvpos_t      m_srcpos;
    lvsize_t     m_srcsize;
    lUInt32      m_srcflags;
public:
    virtual lvsize_t        GetSize() const { return m_size; }
    virtual const lChar32 * GetName() const { return m_name.empty()?NULL:m_name.c_str(); }
    virtual lUInt32         GetFlags() const  { return m_flags; }
    virtual bool            IsContainer() const  { return m_is_container; }
    lvpos_t GetSrcPos() { return m_srcpos; }
    lvsize_t GetSrcSize() { return m_srcsize; }
    lUInt32 GetSrcFlags() { return m_srcflags; }
    void SetSrc( lvpos_t pos, lvsize_t size, lUInt32 flags )
    {
        m_srcpos = pos;
        m_srcsize = size;
        m_srcflags = flags;
    }
    void SetName( const lChar32 * name )
    {
        m_name = name;
    }
    void SetItemInfo( lString32 fname, lvsize_t size, lUInt32 flags, bool isContainer = false )
    {
        m_name = fname;
        m_size = size;
        m_flags = flags;
        m_is_container = isContainer;
    }
    LVCommonContainerItemInfo() : m_size(0), m_flags(0), m_is_container(false),
        m_srcpos(0), m_srcsize(0), m_srcflags(0)
    {
    }
    virtual ~LVCommonContainerItemInfo ()
    {
    }
};

#endif  // __LVCOMMONCONTAINERITEMINFO_H_INCLUDED__
