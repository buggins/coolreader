/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009,2011 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2008 Alexander V. Nikolaev <avn@daemon.hole.ru>         *
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

#ifndef __LVMEMORYSTREAM_H_INCLUDED__
#define __LVMEMORYSTREAM_H_INCLUDED__

#include "lvnamedstream.h"

class LVMemoryStream : public LVNamedStream {
protected:
    lUInt8 *m_pBuffer;
    bool m_own_buffer;
    LVContainer *m_parent;
    lvsize_t m_size;
    lvsize_t m_bufsize;
    lvpos_t m_pos;
    lvopen_mode_t m_mode;
public:
    /// Check whether end of file is reached
    /**
        \return true if end of file reached
    */
    virtual bool Eof() {
        return m_pos >= m_size;
    }

    virtual lvopen_mode_t GetMode() {
        return m_mode;
    }

    /** \return LVERR_OK if change is ok */
    virtual lverror_t SetMode(lvopen_mode_t mode);

    virtual LVContainer *GetParentContainer() {
        return (LVContainer *) m_parent;
    }

    virtual lverror_t Read(void *buf, lvsize_t count, lvsize_t *nBytesRead);

    virtual lvsize_t GetSize();

    virtual lverror_t GetSize(lvsize_t *pSize);

    // ensure that buffer is at least new_size long
    lverror_t SetBufSize(lvsize_t new_size);

    virtual lverror_t SetSize(lvsize_t size);

    virtual lverror_t Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten);

    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos);

    lverror_t Close();

    lverror_t Create();

    /// Creates memory stream as copy of another stream.
    lverror_t CreateCopy(LVStreamRef srcStream, lvopen_mode_t mode);

    lverror_t CreateCopy(const lUInt8 *pBuf, lvsize_t size, lvopen_mode_t mode);

    lverror_t Open(lUInt8 *pBuf, lvsize_t size);

    LVMemoryStream();

    virtual ~LVMemoryStream();
};

#endif  // __LVMEMORYSTREAM_H_INCLUDED__
