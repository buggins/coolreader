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

#ifndef __LVARCCONTAINERBASE_H_INCLUDED__
#define __LVARCCONTAINERBASE_H_INCLUDED__

#include "lvnamedcontainer.h"

class LVArcContainerBase : public LVNamedContainer
{
protected:
    LVContainer * m_parent;
    LVStreamRef m_stream;
public:
    virtual LVStreamRef OpenStream( const char32_t *, lvopen_mode_t )
    {
        return LVStreamRef();
    }
    virtual LVContainer * GetParentContainer()
    {
        return (LVContainer*)m_parent;
    }
    virtual const LVContainerItemInfo * GetObjectInfo(int index)
    {
        if (index>=0 && index<m_list.length())
            return m_list[index];
        return NULL;
    }
    virtual const LVContainerItemInfo * GetObjectInfo(lString32 name)
    {
        for ( int i=0; i<m_list.length(); i++ )
            if (m_list[i]->GetName()==name )
                return m_list[i];
        return NULL;
    }
    virtual int GetObjectCount() const
    {
        return m_list.length();
    }
    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        if (m_fname.empty())
            return LVERR_FAIL;
        *pSize = GetObjectCount();
        return LVERR_OK;
    }
    LVArcContainerBase( LVStreamRef stream ) : m_parent(NULL), m_stream(stream)
    {
    }
    virtual ~LVArcContainerBase()
    {
        SetName(NULL);
        Clear();
    }
    virtual int ReadContents() = 0;

};

#endif  // __LVARCCONTAINERBASE_H_INCLUDED__
