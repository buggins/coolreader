/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009-2011 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018,2020 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef __LVNAMEDSTREAM_H_INCLUDED__
#define __LVNAMEDSTREAM_H_INCLUDED__

#include "lvstream.h"

class LVNamedStream : public LVStream
{
protected:
    lString32 m_fname;
    lString32 m_filename;
    lString32 m_path;
    lvopen_mode_t m_mode;
    lUInt32 _crc;
    bool _crcFailed;
    lvsize_t _autosyncLimit;
    lvsize_t _bytesWritten;
    virtual void handleAutoSync(lvsize_t bytesWritten) {
        _bytesWritten += bytesWritten;
        if (_autosyncLimit==0)
            return;
        if (_bytesWritten>_autosyncLimit) {
            Flush(true);
            _bytesWritten = 0;
        }
    }
public:
    LVNamedStream() : m_mode(LVOM_ERROR), _crc(0), _crcFailed(false), _autosyncLimit(0), _bytesWritten(0) { }
    /// set write bytes limit to call flush(true) automatically after writing of each sz bytes
    virtual void setAutoSyncSize(lvsize_t sz) { _autosyncLimit = sz; }
    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName();
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name);
    /// returns open mode
    virtual lvopen_mode_t GetMode()
    {
        return (lvopen_mode_t)(m_mode & LVOM_MASK);
    }
    /// calculate crc32 code for stream, if possible
    virtual lverror_t getcrc32( lUInt32 & dst );
};

#endif  // __LVNAMEDSTREAM_H_INCLUDED__
