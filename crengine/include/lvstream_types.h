/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
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
 * \file lvstream_types.h
 * \brief stream types definition
 */

#ifndef LVSTREAM_TYPES_H_INCLUDED
#define LVSTREAM_TYPES_H_INCLUDED

#include "lvtypes.h"

#if LVLONG_FILE_SUPPORT == 1
typedef   lUInt64   lvsize_t;    ///< file size type
typedef   lInt64    lvoffset_t;  ///< file offset type
typedef   lUInt64   lvpos_t;     ///< file position type
#else
typedef   lUInt32   lvsize_t;    ///< file size type
typedef   lInt32    lvoffset_t;  ///< file offset type
typedef   lUInt32   lvpos_t;     ///< file position type
#endif

#define LV_INVALID_SIZE ((lvsize_t)(-1))

/// Seek origins enum
enum lvseek_origin_t {
    LVSEEK_SET = 0,     ///< seek relatively to beginning of file
    LVSEEK_CUR = 1,     ///< seek relatively to current position
    LVSEEK_END = 2      ///< seek relatively to end of file
};

/// I/O errors enum
enum lverror_t {
    LVERR_OK = 0,       ///< no error
    LVERR_FAIL,         ///< failed (unknown error)
    LVERR_EOF,          ///< end of file reached
    LVERR_NOTFOUND,     ///< file not found
    LVERR_NOTIMPL       ///< method is not implemented
};

/// File open modes enum
enum lvopen_mode_t {
    LVOM_ERROR=0,       ///< to indicate error state
    LVOM_CLOSED,        ///< to indicate closed state
    LVOM_READ,          ///< readonly mode, use for r/o mmap
    LVOM_WRITE,         ///< writeonly mode
    LVOM_APPEND,        ///< append (readwrite) mode, use for r/w mmap
    LVOM_READWRITE      ///< readwrite mode
};

#endif  // LVSTREAM_TYPES_H_INCLUDED
