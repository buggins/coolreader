/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __LVJPEGIMAGESOURCE_H_INCLUDED__
#define __LVJPEGIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_LIBJPEG==1)

#include "lvnodeimagesource.h"

#include <stdio.h>
#include <setjmp.h>

#define XMD_H

extern "C" {
#include <jpeglib.h>
}
#include <jerror.h>

struct my_error_mgr {
    struct jpeg_error_mgr pub;      /* "public" fields */
    jmp_buf setjmp_buffer;          /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

class LVJpegImageSource : public LVNodeImageSource
{
    my_error_mgr jerr;
    jpeg_decompress_struct cinfo;
public:
    LVJpegImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVJpegImageSource() {}
    virtual void   Compact() { }
    virtual bool   Decode( LVImageDecoderCallback * callback );
    static bool CheckPattern( const lUInt8 * buf, int );
};

#endif  // (USE_LIBJPEG==1)

#endif  // __LVJPEGIMAGESOURCE_H_INCLUDED__
