/*******************************************************

   CoolReader Engine 

   lvimg.cpp:  Image formats support

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define XMD_H

#include "../include/lvimg.h"
#include "../include/lvtinydom.h"

#if (USE_LIBPNG==1)
#include <png.h>
#endif

#if (USE_LIBJPEG==1)

//#include "../../wxWidgets/src/jpeg/jinclude.h"
extern "C" {
//#include <jpeglib.h>
#include <jpeglib.h>
}

#include <jerror.h>

#ifndef wxjpeg_boolean
#define wxjpeg_boolean boolean
#endif
#endif


LVImageSource::~LVImageSource() {}


class LVNodeImageSource : public LVImageSource
{
protected:
    ldomNode *  _node;
    LVStreamRef _stream;
    int _width;
    int _height;
public:
    LVNodeImageSource( ldomNode * node, LVStreamRef stream )
        : _node(node), _stream(stream), _width(0), _height(0)
    {
        
    }
    
    ldomNode * GetSourceNode() { return _node; }
    virtual LVStream * GetSourceStream() { return _stream.get(); }
    virtual void   Compact() { }
    virtual int    GetWidth() { return _width; }
    virtual int    GetHeight() { return _height; }
    virtual bool   Decode( LVImageDecoderCallback * callback ) = 0;
    virtual ~LVNodeImageSource() {}
};

#if (USE_LIBJPEG==1)

typedef struct {
    struct jpeg_source_mgr pub;   /* public fields */
    LVStream * stream;        /* source stream */
    JOCTET * buffer;      /* start of buffer */
    bool start_of_file;    /* have we gotten any data yet? */
} cr_jpeg_source_mgr;

#define INPUT_BUF_SIZE  4096    /* choose an efficiently fread'able size */

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

METHODDEF(void)
cr_init_source (j_decompress_ptr cinfo)
{
    cr_jpeg_source_mgr * src = (cr_jpeg_source_mgr*) cinfo->src;

    /* We reset the empty-input-file flag for each image,
     * but we don't clear the input buffer.
     * This is correct behavior for reading a series of images from one source.
     */
    src->start_of_file = true;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

METHODDEF(wxjpeg_boolean)
cr_fill_input_buffer (j_decompress_ptr cinfo)
{
    cr_jpeg_source_mgr * src = (cr_jpeg_source_mgr *) cinfo->src;
    lvsize_t bytesRead = 0;
    if ( src->stream->Read( src->buffer, INPUT_BUF_SIZE, &bytesRead ) != LVERR_OK )
        throw;

    if (bytesRead <= 0) {
        if (src->start_of_file) /* Treat empty input file as fatal error */
            ERREXIT(cinfo, JERR_INPUT_EMPTY);
        WARNMS(cinfo, JWRN_JPEG_EOF);
        /* Insert a fake EOI marker */
        src->buffer[0] = (JOCTET) 0xFF;
        src->buffer[1] = (JOCTET) JPEG_EOI;
        bytesRead = 2;
    }

    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = (lUInt32)bytesRead;
    src->start_of_file = false;

    return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

METHODDEF(void)
cr_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    cr_jpeg_source_mgr * src = (cr_jpeg_source_mgr *) cinfo->src;

    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0) {
        while (num_bytes > (long) src->pub.bytes_in_buffer) {
          num_bytes -= (long) src->pub.bytes_in_buffer;
          (void) cr_fill_input_buffer(cinfo);
          /* note we assume that fill_input_buffer will never return FALSE,
           * so suspension need not be handled.
           */
        }
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */
GLOBAL(wxjpeg_boolean)
cr_resync_to_restart (j_decompress_ptr cinfo, int desired)
{
    return FALSE;
}


/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
cr_term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

GLOBAL(void)
cr_jpeg_src (j_decompress_ptr cinfo, LVStream * stream)
{
    cr_jpeg_source_mgr * src;

    /* The source object and input buffer are made permanent so that a series
     * of JPEG images can be read from the same file by calling jpeg_stdio_src
     * only before the first one.  (If we discarded the buffer at the end of
     * one image, we'd likely lose the start of the next one.)
     * This makes it unsafe to use this manager and a different source
     * manager serially with the same JPEG object.  Caveat programmer.
     */
    if (cinfo->src == NULL) { /* first time for this JPEG object? */
        src = (cr_jpeg_source_mgr *) new cr_jpeg_source_mgr;
        cinfo->src = (struct jpeg_source_mgr *) src;
        src->buffer = new JOCTET[INPUT_BUF_SIZE];
    }

    src = (cr_jpeg_source_mgr *) cinfo->src;
    src->pub.init_source = cr_init_source;
    src->pub.fill_input_buffer = cr_fill_input_buffer;
    src->pub.skip_input_data = cr_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = cr_term_source;
    src->stream = stream;
    src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    src->pub.next_input_byte = NULL; /* until buffer loaded */
}

GLOBAL(void)
cr_jpeg_src_free (j_decompress_ptr cinfo)
{
    cr_jpeg_source_mgr * src = (cr_jpeg_source_mgr *) cinfo->src;
    if ( src && src->buffer )
    {
        delete[] src->buffer;
        src->buffer = NULL;
    }
    delete src;
}


/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
cr_error_throw (j_common_ptr cinfo)
{
    //fprintf(stderr, "cr_error_throw() : fatal error while decoding JPEG image\n");

    //char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    //(*cinfo->err->format_message) (cinfo, buffer);
  
    //fprintf( stderr, "message: %s\n", buffer );
    
    //throw;
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

#endif

#if (USE_LIBPNG==1)

static void lvpng_error_func (png_structp png, png_const_charp msg)
{
    throw;
}

static void lvpng_warning_func (png_structp png, png_const_charp msg)
{
    throw;
}

static void lvpng_read_func(png_structp png, png_bytep buf, png_size_t len)
{
    LVNodeImageSource * obj = (LVNodeImageSource *) png_get_io_ptr(png);
    LVStream * stream = obj->GetSourceStream();
    lvsize_t bytesRead = 0;
    if ( stream->Read( buf, len, &bytesRead )!=LVERR_OK || bytesRead!=(lvsize_t)len )
        throw;
}

#endif

/// dummy image source to show invalid image
class LVDummyImageSource : public LVImageSource
{
protected:
    ldomNode *  _node;
    int _width;
    int _height;
public:
    LVDummyImageSource( ldomNode * node, int width, int height )
        : _node(node), _width(width), _height(height)
    {
        
    }
    
    ldomNode * GetSourceNode() { return _node; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() { return _width; }
    virtual int    GetHeight() { return _height; }
    virtual bool   Decode( LVImageDecoderCallback * callback )
    {
        if ( callback )
        {
            callback->OnStartDecode(this);
            lUInt32 * row = new lUInt32[ _width ];
            for (int i=0; i<_height; i++)
            {
                if ( i==0 || i==_height-1 )
                {
                    for ( int x=0; x<_width; x++ )
                        row[ x ] = 0x000000;
                }
                else
                {
                    for ( int x=1; x<_width-1; x++ )
                        row[ x ] = 0xFFFFFF;
                    row[ 0 ] = 0x000000;
                    row[ _width-1 ] = 0x000000;
                }
                callback->OnLineDecoded(this, i, row);
            }
            delete row;
            callback->OnEndDecode(this, false);
        }
        return true;
    }
    virtual ~LVDummyImageSource() {}
};


#if (USE_LIBJPEG==1)

class LVJpegImageSource : public LVNodeImageSource
{
protected:
public:
    LVJpegImageSource( ldomNode * node, LVStreamRef stream )
        : LVNodeImageSource(node, stream)
    {
        
    }
    virtual ~LVJpegImageSource() {}
    virtual void   Compact() { }
    virtual bool   Decode( LVImageDecoderCallback * callback )
    {
        struct jpeg_decompress_struct cinfo;
        /* Step 1: allocate and initialize JPEG decompression object */
    
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;

        /* We set up the normal JPEG error routines, then override error_exit. */
        jpeg_error_mgr errmgr;
        cinfo.err = jpeg_std_error(&errmgr);
        errmgr.error_exit = cr_error_throw;

        lUInt8 * buffer = NULL;
        lUInt32 * row = NULL;
        
        if (setjmp(jerr.setjmp_buffer)) {
	    /* If we get here, the JPEG code has signaled an error.
	     * We need to clean up the JPEG object, close the input file, and return.
	     */
            if ( buffer )
                delete[] buffer;
            if ( row )
                delete[] row;
            cr_jpeg_src_free (&cinfo);
            jpeg_destroy_decompress(&cinfo);
            return false;
	}

        
            _stream->SetPos( 0 );
            /* Now we can initialize the JPEG decompression object. */
            jpeg_create_decompress(&cinfo);
            /* Step 2: specify data source (eg, a file) */
            cr_jpeg_src( &cinfo, _stream.get() );
            /* Step 3: read file parameters with jpeg_read_header() */
        
            //fprintf(stderr, "    try to call jpeg_read_header...\n");
            (void) jpeg_read_header(&cinfo, TRUE);
            /* We can ignore the return value from jpeg_read_header since
             *   (a) suspension is not possible with the stdio data source, and
             *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
             * See libjpeg.doc for more info.
             */
            _width = cinfo.image_width;
            _height = cinfo.image_height;
            //fprintf(stderr, "    jpeg_read_header() finished succesfully: image size = %d x %d\n", _width, _height);
            
            if ( callback )
            {
                /* Step 4: set parameters for decompression */
              
                /* In this example, we don't need to change any of the defaults set by
                 * jpeg_read_header(), so we do nothing here.
                 */
                cinfo.out_color_space = JCS_RGB;
        
                /* Step 5: Start decompressor */
            
                (void) jpeg_start_decompress(&cinfo);
                /* We can ignore the return value since suspension is not possible
                 * with the stdio data source.
                 */
                buffer = new lUInt8 [ cinfo.output_width * cinfo.output_components ];
                row = new lUInt32 [ cinfo.output_width ];
                /* Step 6: while (scan lines remain to be read) */
                /*           jpeg_read_scanlines(...); */
              
                /* Here we use the library's state variable cinfo.output_scanline as the
                 * loop counter, so that we don't have to keep track ourselves.
                 */
                while (cinfo.output_scanline < cinfo.output_height) {
                    int y = cinfo.output_scanline;
                    /* jpeg_read_scanlines expects an array of pointers to scanlines.
                     * Here the array is only one element long, but you could ask for
                     * more than one scanline at a time if that's more convenient.
                     */
                    (void) jpeg_read_scanlines(&cinfo, &buffer, 1);
                    /* Assume put_scanline_someplace wants a pointer and sample count. */
                    lUInt8 * p = buffer;
                    for (int x=0; x<(int)cinfo.output_width; x++)
                    {
                        row[x] = (((lUInt32)p[0])<<16) | (((lUInt32)p[1])<<8) | (((lUInt32)p[2])<<0);
                        p += 3;
                    }
                    callback->OnLineDecoded( this, y, row );
                }
            }        

        if ( buffer )
            delete[] buffer;
        if ( row )
            delete[] row;
        cr_jpeg_src_free (&cinfo);
        jpeg_destroy_decompress(&cinfo);
        return true;
    }
    static bool CheckPattern( const lUInt8 * buf, int len )
    {
        //check for SOI marker at beginning of file
        return (buf[0]==0xFF && buf[1]==0xD8);
    }
};

#endif


#if (USE_LIBPNG==1)

class LVPngImageSource : public LVNodeImageSource
{
protected:
public:
    LVPngImageSource( ldomNode * node, LVStreamRef stream )
        : LVNodeImageSource(node, stream)
    {
        
    }
    virtual ~LVPngImageSource() {}
    virtual void   Compact() { }
    virtual bool   Decode( LVImageDecoderCallback * callback )
    {
        png_structp png_ptr = NULL;
        png_infop info_ptr = NULL;
        lUInt32 * row = NULL;
        try {
            _stream->SetPos( 0 );
            png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
                (png_voidp)this, lvpng_error_func, lvpng_warning_func);
            if (!png_ptr)
                throw;
            //
            info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr)
                throw;
            png_set_read_fn(png_ptr,
                (voidp)this, lvpng_read_func);
            png_read_info( png_ptr, info_ptr );


            png_uint_32 width, height;
            int bit_depth, color_type, interlace_type;
            png_get_IHDR(png_ptr, info_ptr, &width, &height,
               &bit_depth, &color_type, &interlace_type,
               NULL, NULL);
            _width = width;
            _height = height;

            row = new lUInt32[ width ];

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
                    png_set_gray_1_2_4_to_8(png_ptr);

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

                int number_passes = png_set_interlace_handling(png_ptr);
                //if (color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
                //    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)

                //if (color_type == PNG_COLOR_TYPE_RGB ||
                //    color_type == PNG_COLOR_TYPE_RGB_ALPHA)
                png_set_bgr(png_ptr);

                 for (int pass = 0; pass < number_passes; pass++)
                {
                      for (lUInt32 y = 0; y < height; y++)
                    {
                        png_read_rows(png_ptr, (unsigned char **)&row, png_bytepp_NULL, 1);
                        callback->OnLineDecoded( this, y, row );
                    }
                }

                png_read_end(png_ptr, info_ptr);

                callback->OnEndDecode(this, false);
            }
            png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        } catch (...) {
            _width = 0;
            _height = 0;
            if (png_ptr)
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
            }
            if ( row )
                delete row;
            if (callback)
                callback->OnEndDecode(this, true); // error!
            return false;
        }
        return true;
    }
    static bool CheckPattern( const lUInt8 * buf, int len )
    {
        return( !png_sig_cmp((unsigned char *)buf, (png_size_t)0, 4) );
    }
};

#endif

LVImageDecoderCallback::~LVImageDecoderCallback()
{
}


/// dummy image object, to show invalid image
LVImageSourceRef LVCreateDummyImageSource( ldomNode * node, int width, int height )
{
    return LVImageSourceRef( new LVDummyImageSource( node, width, height ) );
}

/// create image from node source
LVImageSourceRef LVCreateNodeImageSource( ldomNode * node )
{
    LVImageSourceRef ref;
    if (!node->isElement())
        return ref;
    LVStreamRef stream = ((ldomElement*)node)->createBase64Stream();
    if (stream.isNull())
        return ref;
    lUInt8 hdr[256];
    lvsize_t bytesRead = 0;
    if ( stream->Read( hdr, 256, &bytesRead )!=LVERR_OK )
        return ref;
    stream->SetPos( 0 );
    

    LVImageSource * img = NULL;
#if (USE_LIBPNG==1)
    if ( LVPngImageSource::CheckPattern( hdr, (lUInt32)bytesRead ) )
        img = new LVPngImageSource( node, stream );
    else
#endif
#if (USE_LIBJPEG==1)
    if ( LVJpegImageSource::CheckPattern( hdr, (lUInt32)bytesRead ) )
        img = new LVJpegImageSource( node, stream );
    else
#endif
        img = new LVDummyImageSource( node, 50, 50 );
    if ( !img )
        return ref;
    ref = LVImageSourceRef( img );
    if ( !img->Decode( NULL ) )
    {
        return LVImageSourceRef();
    }
    return ref;
}
