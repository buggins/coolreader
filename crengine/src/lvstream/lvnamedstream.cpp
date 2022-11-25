/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009,2014,2015 Vadim Lopatin <coolreader.org@gmail.com>
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

#include "lvnamedstream.h"

const lChar32 * LVNamedStream::GetName()
{
    if (m_fname.empty())
        return NULL;
    return m_fname.c_str();
}

void LVNamedStream::SetName(const lChar32 * name)
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
            break;
    }
    int pos = (int)(p - fn);
    if (p>fn)
        m_path = m_fname.substr(0, pos);
    m_filename = m_fname.substr(pos, m_fname.length() - pos);
}

lverror_t LVNamedStream::getcrc32( lUInt32 & dst )
{
    if ( _crc!=0 ) {
        dst = _crc;
        return LVERR_OK;
    } else {
        if ( !_crcFailed ) {
            lverror_t res = LVStream::getcrc32( dst );
            if ( res==LVERR_OK ) {
                _crc = dst;
                return LVERR_OK;
            }
            _crcFailed = true;
        }
        dst = 0;
        return LVERR_FAIL;
    }
}
