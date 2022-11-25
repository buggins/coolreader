/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2015 Vadim Lopatin <coolreader.org@gmail.com> *
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

#ifndef __LVNAMEDCONTAINER_H_INCLUDED__
#define __LVNAMEDCONTAINER_H_INCLUDED__

#include "lvcontainer.h"
#include "lvcommoncontaineriteminfo.h"
#include "lvptrvec.h"

class LVNamedContainer : public LVContainer
{
protected:
    lString32 m_fname;
    lString32 m_filename;
    lString32 m_path;
    lChar32 m_path_separator;
    LVPtrVector<LVCommonContainerItemInfo> m_list;
public:
    virtual bool IsContainer()
    {
        return true;
    }
    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName()
    {
        if (m_fname.empty())
            return NULL;
        return m_fname.c_str();
    }
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name)
    {
        m_fname = name;
        m_filename.clear();
        m_path.clear();
        if (m_fname.empty())
            return;
        const lChar32 * fn = m_fname.c_str();

        const lChar32 * p = fn + m_fname.length() - 1;
        for ( ;p>fn; p--) {
            if (p[-1] == '/' || p[-1]=='\\')
            {
                m_path_separator = p[-1];
                break;
            }
        }
        int pos = (int)(p - fn);
        if (p > fn)
            m_path = m_fname.substr(0, pos);
        m_filename = m_fname.substr(pos, m_fname.length() - pos);
    }
    LVNamedContainer() : m_path_separator(
#ifdef _LINUX
        '/'
#else
        '\\'
#endif
    )
    {
    }
    virtual ~LVNamedContainer()
    {
    }
    void Add( LVCommonContainerItemInfo * item )
    {
        m_list.add( item );
    }
    void Clear()
    {
        m_list.clear();
    }
};

#endif  // __LVNAMEDCONTAINER_H_INCLUDED__
