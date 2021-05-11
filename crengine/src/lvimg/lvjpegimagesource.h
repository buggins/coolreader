/** @file lvjpegimagesource.h
    @brief library private stuff: jpeg image decoder

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

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
