/*******************************************************

   CoolReader Engine

   lvsvgimagesource.cpp: svg image decoder

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvsvgimagesource.h"

#if (USE_NANOSVG==1)

#include "lvimagedecodercallback.h"

// Support for SVG
#include <math.h>
#include <stdio.h>
#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <stb_image_write.h> // for svg to png conversion

LVSvgImageSource::LVSvgImageSource( ldomNode * node, LVStreamRef stream )
        : LVNodeImageSource(node, stream)
{
}
LVSvgImageSource::~LVSvgImageSource() {}

void LVSvgImageSource::Compact() { }

bool LVSvgImageSource::CheckPattern( const lUInt8 * buf, int len)
{
    // check for <?xml or <svg
    if (len > 5 && buf[0]=='<' && buf[1]=='?' &&
            (buf[2]=='x' || buf[2] == 'X') &&
            (buf[3]=='m' || buf[3] == 'M') &&
            (buf[4]=='l' || buf[4] == 'L'))
        return true;
    if (len > 4 && buf[0]=='<' &&
            (buf[1]=='s' || buf[1] == 'S') &&
            (buf[2]=='v' || buf[2] == 'V') &&
            (buf[3]=='g' || buf[3] == 'G'))
        return true;
    return false;
}

bool LVSvgImageSource::Decode( LVImageDecoderCallback * callback )
{
    if ( _stream.isNull() )
        return false;
    lvsize_t sz = _stream->GetSize();
    // if ( sz<32 || sz>0x80000 ) return false; // do not impose (yet) a max size for svg
    lUInt8 * buf = new lUInt8[ sz+1 ];
    lvsize_t bytesRead = 0;
    bool res = true;
    _stream->SetPos(0);
    if ( _stream->Read( buf, sz, &bytesRead )!=LVERR_OK || bytesRead!=sz ) {
        res = false;
    }
    else {
        buf[sz] = 0;
        res = DecodeFromBuffer( buf, sz, callback );
    }
    delete[] buf;
    return res;
}

int LVSvgImageSource::DecodeFromBuffer(unsigned char *buf, int buf_size, LVImageDecoderCallback * callback)
{
    NSVGimage *image = NULL;
    NSVGrasterizer *rast = NULL;
    unsigned char* img = NULL;
    int w, h;
    bool res = false;

    // printf("SVG: parsing...\n");
    image = nsvgParse((char*)buf, "px", 96.0f);
    if (image == NULL) {
        printf("SVG: could not parse SVG stream.\n");
        nsvgDelete(image);
        return res;
    }

    w = (int)image->width;
    h = (int)image->height;
    // The rasterizer (while antialiasing?) has a tendency to eat the last
    // right and bottom pixel. We can avoid that by adding 1 pixel around
    // each side, by increasing width and height with 2 here, and using
    // offsets of 1 in nsvgRasterize
    w += 2;
    h += 2;
    _width = w;
    _height = h;

    // int nbshapes = 0;
    // for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) nbshapes++;
    // printf("SVG: nb of shapes: %d\n", nbshapes);
    if (! image->shapes) {
        // If no supported shapes, it will be a blank empty image.
        // Better to let user know that with an unsupported image display (empty
        // square with borders).
        // But commented to not flood koreader's log for books with many such
        // svg images (crengine would log this at each page change)
        // printf("SVG: got image with zero supported shape.\n");
        nsvgDelete(image);
        return res;
    }

    if ( ! callback ) { // If no callback provided, only size is wanted.
        res = true;
    }
    else {
        rast = nsvgCreateRasterizer();
        if (rast == NULL) {
            printf("SVG: could not init rasterizer.\n");
        }
        else {
            img = (unsigned char*) malloc(w*h*4);
            if (img == NULL) {
                printf("SVG: could not alloc image buffer.\n");
            }
            else {
                // printf("SVG: rasterizing image %d x %d\n", w, h);
                nsvgRasterize(rast, image, 1, 1, 1, img, w, h, w*4); // offsets of 1 pixel, scale = 1
                // stbi_write_png("/tmp/svg.png", w, h, 4, img, w*4); // for debug
                callback->OnStartDecode(this);
                lUInt32 * row = new lUInt32 [ _width ];
                lUInt8 * p = img;
                lUInt8 r, g, b, a, ia, blend, iblend;
                lUInt32 ro, go, bo;
                for (int y=0; y<_height; y++) {
                    for (int x=0; x<_width; x++) {
                        // We mostly get full white or full black when using alpha channel like this:
                        //   row[x] = (((lUInt32)p[3])<<24) | (((lUInt32)p[0])<<16) | (((lUInt32)p[1])<<8) | (((lUInt32)p[2])<<0);
                        // We can ignore the alpha channel but we get a black background for transparent pixels with:
                        //   row[x] = (((lUInt32)p[0])<<16) | (((lUInt32)p[1])<<8) | (((lUInt32)p[2])<<0);
                        // It's better to use alpha channel here to blend pixels over a white background and set opacity to full
                        // """ To perform a source-over blend between two colors that use straight alpha format:
                        //           result = (source.RGB * source.A) + (dest.RGB * (1 - source.A))        """
                        r = (lUInt8)p[0];
                        g = (lUInt8)p[1];
                        b = (lUInt8)p[2];
                        a = (lUInt8)p[3];
                        ia = a ^ 0xFF;
                        ro = (lUInt32)( r*a + 0xff*ia );
                        go = (lUInt32)( g*a + 0xff*ia );
                        bo = (lUInt32)( b*a + 0xff*ia );
                        // More accurate divide by 256 than just >> 8 (255 becomes 254 with just >> 8)
                        ro = (ro+1 + (ro >> 8)) >> 8;
                        go = (go+1 + (go >> 8)) >> 8;
                        bo = (bo+1 + (bo >> 8)) >> 8;
                        row[x] = ro<<16|go<<8|bo;
                        // if (y == 80) // output bytes for a single row
                        // printf("SVG: byte colors %d %d %d %d > %d %d %d\n", (int)a, (int)r, (int)g, (int)b, (int)ro, (int)go, (int)bo);
                        p += 4;
                    }
                    callback->OnLineDecoded( this, y, row );
                }
                delete[] row;
                callback->OnEndDecode(this, false);
                res = true;
                free(img);
            }
        }
    }
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    return res;
}

// Convenience function to convert SVG image data to PNG
unsigned char * convertSVGtoPNG(unsigned char *svg_data, int svg_data_size, float zoom_factor, int *png_data_len)
{
    NSVGimage *image = NULL;
    NSVGrasterizer *rast = NULL;
    unsigned char* img = NULL;
    int w, h, pw, ph;
    unsigned char *png = NULL;

    // printf("SVG: converting to PNG...\n");
    image = nsvgParse((char*)svg_data, "px", 96.0f);
    if (image == NULL) {
        printf("SVG: could not parse SVG stream.\n");
        nsvgDelete(image);
        return png;
    }

    if (! image->shapes) {
        printf("SVG: got image with zero supported shape.\n");
        nsvgDelete(image);
        return png;
    }

    w = (int)image->width;
    h = (int)image->height;
    // The rasterizer (while antialiasing?) has a tendency to eat some of the
    // right and bottom pixels. We can avoid that by adding N pixels around
    // each side, by increasing width and height with 2*N here, and using
    // offsets of N in nsvgRasterize. Using zoom_factor as N gives nice results.
    int offset = zoom_factor;
    pw = w*zoom_factor + 2*offset;
    ph = h*zoom_factor + 2*offset;
    rast = nsvgCreateRasterizer();
    if (rast == NULL) {
        printf("SVG: could not init rasterizer.\n");
    }
    else {
        img = (unsigned char*) malloc(pw*ph*4);
        if (img == NULL) {
            printf("SVG: could not alloc image buffer.\n");
        }
        else {
            // printf("SVG: rasterizing to png image %d x %d\n", pw, ph);
            nsvgRasterize(rast, image, offset, offset, zoom_factor, img, pw, ph, pw*4);
            png = stbi_write_png_to_mem(img, pw*4, pw, ph, 4, png_data_len);
            free(img);
        }
    }
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    return png;
}

#endif  // (USE_NANOSVG==1)
