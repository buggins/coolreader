/*******************************************************

   CoolReader Engine

   lvpngimagesource.cpp: png image decoder

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvpngimagesource.h"

#if (USE_LIBPNG==1)

#include "lvimagedecodercallback.h"
#include "crlog.h"

#include <png.h>

static void lvpng_error_func (png_structp png, png_const_charp msg)
{
    CRLog::error("libpng: %s", msg);
    longjmp(png_jmpbuf(png), 1);
}

static void lvpng_warning_func (png_structp png, png_const_charp msg)
{
    CR_UNUSED(png);
    CRLog::warn("libpng: %s", msg);
}

static void lvpng_read_func(png_structp png, png_bytep buf, png_size_t len)
{
    LVNodeImageSource * obj = (LVNodeImageSource *) png_get_io_ptr(png);
    LVStream * stream = obj->GetSourceStream();
    lvsize_t bytesRead = 0;
    if ( stream->Read( buf, (int)len, &bytesRead )!=LVERR_OK || bytesRead!=len )
        longjmp(png_jmpbuf(png), 1);
}



LVPngImageSource::LVPngImageSource( ldomNode * node, LVStreamRef stream )
        : LVNodeImageSource(node, stream)
{
}

LVPngImageSource::~LVPngImageSource()
{
}

void LVPngImageSource::Compact()
{
}

bool LVPngImageSource::Decode( LVImageDecoderCallback * callback )
{
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    _stream->SetPos( 0 );
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        (png_voidp)this, lvpng_error_func, lvpng_warning_func);
    if ( !png_ptr )
        return false;

    if (setjmp(png_jmpbuf(png_ptr))) {
        _width = 0;
        _height = 0;
        if (png_ptr)
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        }
        if (callback)
            callback->OnEndDecode(this, true); // error!
        return false;
    }

    //
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        lvpng_error_func(png_ptr, "cannot create png info struct");
    png_set_read_fn(png_ptr,
        (void*)this, lvpng_read_func);
    png_read_info( png_ptr, info_ptr );


    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
        &bit_depth, &color_type, &interlace_type,
        NULL, NULL);
    _width = width;
    _height = height;


    if ( callback )
    {
        callback->OnStartDecode(this);

        //int png_transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_INVERT_ALPHA;
            //PNG_TRANSFORM_PACKING|
            //PNG_TRANSFORM_STRIP_16|
            //PNG_TRANSFORM_INVERT_ALPHA;

        // SET TRANSFORMS
        if (color_type & PNG_COLOR_MASK_PALETTE)
            png_set_palette_to_rgb(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
//#if PNG_LIBPNG_VER_RELEASE==7
#if (PNG_LIBPNG_VER_MAJOR == 1) && (PNG_LIBPNG_VER_MINOR < 4)
            png_set_gray_1_2_4_to_8(png_ptr);
#else
            png_set_expand_gray_1_2_4_to_8(png_ptr);
#endif

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr);

        if (bit_depth == 16)
            png_set_strip_16(png_ptr);

        png_set_invert_alpha(png_ptr);

        if (bit_depth < 8)
            png_set_packing(png_ptr);

        //if (color_type == PNG_COLOR_TYPE_RGB)
            png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

        //if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        //    png_set_swap_alpha(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
                png_set_gray_to_rgb(png_ptr);

        //if (color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
        //    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)

        //if (color_type == PNG_COLOR_TYPE_RGB ||
        //    color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        png_set_bgr(png_ptr);

        png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr,info_ptr);//update after set
        png_bytep *image=NULL;
        image =  new png_bytep[height];
        for (lUInt32 i=0; i<height; i++)
            image[i] =  new png_byte[png_get_rowbytes(png_ptr,info_ptr)];
        png_read_image(png_ptr,image);
        for (lUInt32 y = 0; y < height; y++)
        {
            callback->OnLineDecoded( this, y,  (lUInt32*) image[y] );
        }
        png_read_end(png_ptr, info_ptr);

        callback->OnEndDecode(this, false);
        for (lUInt32 i=0; i<height; i++)
            delete [] image[i];
        delete [] image;
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return true;
}

bool LVPngImageSource::CheckPattern( const lUInt8 * buf, int )
{
    return( !png_sig_cmp((unsigned char *)buf, (png_size_t)0, 4) );
}

#endif  // (USE_LIBPNG==1)
