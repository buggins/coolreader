/*******************************************************

   CoolReader Engine

   lvimagescaleddrawcallback.cpp:

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvimagescaleddrawcallback.h"
#include "lvdrawbuf_utils.h"

// Quantize an 8-bit color value down to a palette of 16 evenly spaced colors, using an ordered 8x8 dithering pattern.
// With a grayscale input, this happens to match the eInk palette perfectly ;).
// If the input is not grayscale, and the output fb is not grayscale either,
// this usually still happens to match the eInk palette after the EPDC's own quantization pass.
// c.f., https://en.wikipedia.org/wiki/Ordered_dithering
// & https://github.com/ImageMagick/ImageMagick/blob/ecfeac404e75f304004f0566557848c53030bad6/MagickCore/threshold.c#L1627
// NOTE: As the references imply, this is straight from ImageMagick,
//       with only minor simplifications to enforce Q8 & avoid fp maths.
static inline lUInt8 dither_o8x8(int x, int y, lUInt8 v)
{
	// c.f., https://github.com/ImageMagick/ImageMagick/blob/ecfeac404e75f304004f0566557848c53030bad6/config/thresholds.xml#L107
	static const lUInt8 threshold_map_o8x8[] = { 1,  49, 13, 61, 4,  52, 16, 64, 33, 17, 45, 29, 36, 20, 48, 32,
						      9,  57, 5,  53, 12, 60, 8,  56, 41, 25, 37, 21, 44, 28, 40, 24,
						      3,  51, 15, 63, 2,  50, 14, 62, 35, 19, 47, 31, 34, 18, 46, 30,
						      11, 59, 7,  55, 10, 58, 6,  54, 43, 27, 39, 23, 42, 26, 38, 22 };

	// Constants:
	// Quantum = 8; Levels = 16; map Divisor = 65
	// QuantumRange = 0xFF
	// QuantumScale = 1.0 / QuantumRange
	//
	// threshold = QuantumScale * v * ((L-1) * (D-1) + 1)
	// NOTE: The initial computation of t (specifically, what we pass to DIV255) would overflow an uint8_t.
	//       With a Q8 input value, we're at no risk of ever underflowing, so, keep to unsigned maths.
	//       Technically, an uint16_t would be wide enough, but it gains us nothing,
	//       and requires a few explicit casts to make GCC happy ;).
        lUInt32 t;
        DIV255(v * ((15U << 6) + 1U), t);
	// level = t / (D-1);
	lUInt32 l = (t >> 6);
	// t -= l * (D-1);
	t = (t - (l << 6));

	// map width & height = 8
	// c = ClampToQuantum((l+(t >= map[(x % mw) + mw * (y % mh)])) * QuantumRange / (L-1));
	lUInt32 q = ((l + (t >= threshold_map_o8x8[(x & 7U) + 8U * (y & 7U)])) * 17);
	// NOTE: We're doing unsigned maths, so, clamping is basically MIN(q, UINT8_MAX) ;).
	//       The only overflow we should ever catch should be for a few white (v = 0xFF) input pixels
	//       that get shifted to the next step (i.e., q = 272 (0xFF + 17)).
	return (q > UINT8_MAX ? UINT8_MAX : static_cast<lUInt8>(q));
}

// Declare our bit of scaler ripped from Qt5...
namespace CRe {
lUInt8* qSmoothScaleImage(const lUInt8* src, int sw, int sh, bool ignore_alpha, int dw, int dh);
}



int * LVImageScaledDrawCallback::GenMap(int src_len, int dst_len)
{
    int  * map = new int[ dst_len ];
    for (int i=0; i<dst_len; i++)
    {
        map[ i ] = i * src_len / dst_len;
    }
    return map;
}

int * LVImageScaledDrawCallback::GenNinePatchMap(int src_len, int dst_len, int frame1, int frame2)
{
    int  * map = new int[ dst_len ];
    if (frame1 + frame2 > dst_len) {
        int total = frame1 + frame2;
        int extra = total - dst_len;
        int extra1 = frame1 * extra / total;
        int extra2 = frame2 * extra / total;
        frame1 -= extra1;
        frame2 -= extra2;
    }
    int srcm = src_len - frame1 - frame2 - 2;
    int dstm = dst_len - frame1 - frame2;
    if (srcm < 0)
        srcm = 0;
    for (int i=0; i<dst_len; i++)
    {
        if (i < frame1) {
            // start
            map[ i ] = i + 1;
        } else if (i >= dst_len - frame2) {
            // end
            int rx = i - (dst_len - frame2);
            map[ i ] = src_len - frame2 + rx - 1;
        } else {
            // middle
            map[ i ] = 1 + frame1 + (i - frame1) * srcm / dstm;
        }
        //            CRLog::trace("frame[%d, %d] src=%d dst=%d %d -> %d", frame1, frame2, src_len, dst_len, i, map[i]);
        //            if (map[i] >= src_len) {
        //                CRLog::error("Wrong coords");
        //            }
    }
    return map;
}

LVImageScaledDrawCallback::LVImageScaledDrawCallback(LVBaseDrawBuf *dstbuf, LVImageSourceRef img, int x, int y, int width, int height, bool dith, bool inv, bool smooth)
    : src(img), dst(dstbuf), dst_x(x), dst_y(y), dst_dx(width), dst_dy(height), xmap(0), ymap(0), dither(dith), invert(inv), smoothscale(smooth), decoded(0)
{
    src_dx = img->GetWidth();
    src_dy = img->GetHeight();
    const CR9PatchInfo * np = img->GetNinePatchInfo();
    isNinePatch = false;
    lvRect ninePatch;
    if (np) {
        isNinePatch = true;
        ninePatch = np->frame;
    }
    // If smoothscaling was requested, but no scaling was needed, disable the post-processing pass
    if (smoothscale && src_dx == dst_dx && src_dy == dst_dy) {
        smoothscale = false;
        //fprintf( stderr, "Disabling smoothscale because no scaling was needed (%dx%d -> %dx%d)\n", src_dx, src_dy, dst_dx, dst_dy );
    }
    if ( src_dx != dst_dx || isNinePatch) {
        if (isNinePatch)
            xmap = GenNinePatchMap(src_dx, dst_dx, ninePatch.left, ninePatch.right);
        else if (!smoothscale)
            xmap = GenMap( src_dx, dst_dx );
    }
    if ( src_dy != dst_dy || isNinePatch) {
        if (isNinePatch)
            ymap = GenNinePatchMap(src_dy, dst_dy, ninePatch.top, ninePatch.bottom);
        else if (!smoothscale)
            ymap = GenMap( src_dy, dst_dy );
    }
    // If we have a smoothscale post-processing pass, we'll need to build a buffer of the *full* decoded image.
    if (smoothscale) {
        // Byte-sized buffer, we're 32bpp, so, 4 bytes per pixel.
        decoded = new lUInt8[src_dy * (src_dx * 4)];
    }
}

LVImageScaledDrawCallback::~LVImageScaledDrawCallback()
{
    if (xmap)
        delete [] xmap;
    if (ymap)
        delete [] ymap;
    if (decoded)
        delete [] decoded;
}

void LVImageScaledDrawCallback::OnStartDecode(LVImageSource *)
{
}

bool LVImageScaledDrawCallback::OnLineDecoded(LVImageSource *, int y, lUInt32 *data)
{
    //fprintf( stderr, "l_%d ", y );
    if (isNinePatch) {
        if (y == 0 || y == src_dy-1) // ignore first and last lines
            return true;
    }
    // Defer everything to the post-process pass for smooth scaling, we just have to store the line in our decoded buffer
    if (smoothscale) {
        //fprintf( stderr, "Smoothscale l_%d pass\n", y );
        memcpy(decoded + (y * (src_dx * 4)), data, (src_dx * 4));
        return true;
    }
    int yy = -1;
    int yy2 = -1;
    const lUInt32 rgba_invert = invert ? 0x00FFFFFF : 0;
    const lUInt8 gray_invert = invert ? 0xFF : 0;
    if (ymap) {
        for (int i = 0; i < dst_dy; i++) {
            if (ymap[i] == y) {
                if (yy == -1)
                    yy = i;
                yy2 = i + 1;
            }
        }
        if (yy == -1)
            return true;
    } else {
        yy = y;
        yy2 = y+1;
    }
    //        if ( ymap )
    //        {
    //            int yy0 = (y - 1) * dst_dy / src_dy;
    //            yy = y * dst_dy / src_dy;
    //            yy2 = (y+1) * dst_dy / src_dy;
    //            if ( yy == yy0 )
    //            {
    //                //fprintf( stderr, "skip_dup " );
    //                //return true; // skip duplicate drawing
    //            }
    //            if ( yy2 > dst_dy )
    //                yy2 = dst_dy;
    //        }
    lvRect clip;
    dst->GetClipRect( &clip );
    for ( ;yy<yy2; yy++ )
    {
        if ( yy+dst_y<clip.top || yy+dst_y>=clip.bottom )
            continue;
        int bpp = dst->GetBitsPerPixel();
        if ( bpp >= 24 )
        {
            lUInt32 * row = (lUInt32 *)dst->GetScanLine( yy + dst_y );
            row += dst_x;
            for (int x=0; x<dst_dx; x++)
            {
                lUInt32 cl = data[xmap ? xmap[x] : x];
                int xx = x + dst_x;
                lUInt32 alpha = (cl >> 24)&0xFF;
                
                if ( xx<clip.left || xx>=clip.right ) {
                    // OOB, don't plot it!
                    continue;
                }
                
                // NOTE: Remember that for some mysterious reason, lvimg feeds us inverted alpha
                //       (i.e., 0 is opaque, 0xFF is transparent)...
                if ( alpha == 0xFF ) {
                    // Transparent, don't plot it...
                    if ( invert ) {
                        // ...unless we're doing night-mode shenanigans, in which case, we need to fake an inverted background
                        // (i.e., a *black* background, so it gets inverted back to white with NightMode, since white is our expected "standard" background color)
                        // c.f., https://github.com/koreader/koreader/issues/4986
                        row[ x ] = 0x00000000;
                    } else {
                        continue;
                    }
                } else if ( alpha == 0 ) {
                    // Fully opaque, plot it as-is
                    row[ x ] = RevRGB(cl) ^ rgba_invert;
                } else {
                    if ((row[x] & 0xFF000000) == 0xFF000000) {
                        // Plot it as-is if *buffer* pixel is transparent
                        row[ x ] = RevRGBA(cl) ^ rgba_invert;
                    } else {
                        // NOTE: This *also* has a "fully opaque" shortcut... :/
                        ApplyAlphaRGB( row[x], RevRGB(cl), alpha );
                        // Invert post-blending to avoid potential stupidity...
                        row[ x ] ^= rgba_invert;
                    }
                }
            }
        }
        else if ( bpp == 16 )
        {
            lUInt16 * row = (lUInt16 *)dst->GetScanLine( yy + dst_y );
            row += dst_x;
            for (int x=0; x<dst_dx; x++)
            {
                lUInt32 cl = data[xmap ? xmap[x] : x];
                int xx = x + dst_x;
                lUInt32 alpha = (cl >> 24)&0xFF;
                
                if ( xx<clip.left || xx>=clip.right ) {
                    // OOB, don't plot it!
                    continue;
                }
                
                // NOTE: See final branch of the ladder. Not quite sure why some alpha ranges are treated differently...
                if ( alpha >= 0xF0 ) {
                    // Transparent, don't plot it...
                    if ( invert ) {
                        // ...unless we're doing night-mode shenanigans, in which case, we need to fake an inverted background
                        // (i.e., a *black* background, so it gets inverted back to white with NightMode, since white is our expected "standard" background color)
                        // c.f., https://github.com/koreader/koreader/issues/4986
                        row[ x ] = 0x0000;
                    } else {
                        continue;
                    }
                } else if ( alpha < 16 ) {
                    row[ x ] = rgb888to565( cl ^ rgba_invert );
                } else if ( alpha < 0xF0 ) {
                    lUInt32 v = rgb565to888(row[x]);
                    ApplyAlphaRGB( v, cl, alpha );
                    row[ x ] = rgb888to565(v ^ rgba_invert);
                }
            }
        }
        else if ( bpp > 2 ) // 3,4,8 bpp
        {
            lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy + dst_y );
            row += dst_x;
            for (int x=0; x<dst_dx; x++)
            {
                int srcx = xmap ? xmap[x] : x;
                lUInt32 cl = data[srcx];
                int xx = x + dst_x;
                lUInt32 alpha = (cl >> 24)&0xFF;
                
                if ( xx<clip.left || xx>=clip.right ) {
                    // OOB, don't plot it!
                    continue;
                }
                
                if ( alpha == 0xFF ) {
                    // Transparent, don't plot it...
                    if ( invert ) {
                        // ...unless we're doing night-mode shenanigans, in which case, we need to fake a white background.
                        cl = 0x00FFFFFF;
                    } else {
                        continue;
                    }
                } else if ( alpha != 0 ) {
                    lUInt8 origLuma = row[x];
                    // Expand lower bitdepths to Y8
                    if ( bpp == 3 ) {
                        origLuma = origLuma & 0xE0;
                        origLuma = origLuma | (origLuma>>3) | (origLuma>>6);
                    } else if ( bpp == 4 ) {
                        origLuma = origLuma & 0xF0;
                        origLuma = origLuma | (origLuma>>4);
                    }
                    // Expand Y8 to RGB32 (i.e., duplicate, R = G = B = Y)
                    lUInt32 bufColor = origLuma | (origLuma<<8) | (origLuma<<16);
                    ApplyAlphaRGB( bufColor, cl, alpha );
                    cl = bufColor;
                }
                
                lUInt8 dcl;
                if ( dither && bpp < 8 ) {
#if (GRAY_INVERSE==1)
                    dcl = (lUInt8)DitherNBitColor( cl^0xFFFFFF, x, yy, bpp );
#else
                    dcl = (lUInt8)DitherNBitColor( cl, x, yy, bpp );
#endif
                } else if ( dither && bpp == 8 ) {
                    dcl = rgbToGray( cl );
                    dcl = dither_o8x8( x, yy, dcl );
                } else {
                    dcl = rgbToGray( cl, bpp );
                }
                row[ x ] = dcl ^ gray_invert;
                // ApplyAlphaGray( row[x], dcl, alpha, bpp );
            }
        }
        else if ( bpp == 2 )
        {
            //fprintf( stderr, "." );
            lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy+dst_y );
            //row += dst_x;
            for (int x=0; x<dst_dx; x++)
            {
                lUInt32 cl = data[xmap ? xmap[x] : x];
                int xx = x + dst_x;
                lUInt32 alpha = (cl >> 24)&0xFF;
                
                if ( xx<clip.left || xx>=clip.right ) {
                    // OOB, don't plot it!
                    continue;
                }
                
                int byteindex = (xx >> 2);
                int bitindex = (3-(xx & 3))<<1;
                lUInt8 mask = 0xC0 >> (6 - bitindex);
                
                if ( alpha == 0xFF ) {
                    // Transparent, don't plot it...
                    if ( invert ) {
                        // ...unless we're doing night-mode shenanigans, in which case, we need to fake a white background.
                        cl = 0x00FFFFFF;
                    } else {
                        continue;
                    }
                } else if ( alpha != 0 ) {
                    lUInt8 origLuma = (row[ byteindex ] & mask)>>bitindex;
                    origLuma = origLuma | (origLuma<<2);
                    origLuma = origLuma | (origLuma<<4);
                    lUInt32 bufColor = origLuma | (origLuma<<8) | (origLuma<<16);
                    ApplyAlphaRGB( bufColor, cl, alpha );
                    cl = bufColor;
                }
                
                lUInt32 dcl = 0;
                if ( dither ) {
#if (GRAY_INVERSE==1)
                    dcl = Dither2BitColor( cl ^ rgba_invert, x, yy ) ^ 3;
#else
                    dcl = Dither2BitColor( cl ^ rgba_invert, x, yy );
#endif
                } else {
                    dcl = rgbToGrayMask( cl ^ rgba_invert, 2 ) & 3;
                }
                dcl = dcl << bitindex;
                row[ byteindex ] = (lUInt8)((row[ byteindex ] & (~mask)) | dcl);
            }
        }
        else if ( bpp == 1 )
        {
            //fprintf( stderr, "." );
            lUInt8 * row = (lUInt8 *)dst->GetScanLine( yy+dst_y );
            //row += dst_x;
            for (int x=0; x<dst_dx; x++)
            {
                lUInt32 cl = data[xmap ? xmap[x] : x];
                int xx = x + dst_x;
                lUInt32 alpha = (cl >> 24)&0xFF;
                
                if ( xx<clip.left || xx>=clip.right ) {
                    // OOB, don't plot it!
                    continue;
                }
                
                if ( alpha & 0x80 ) {
                    // Transparent, don't plot it...
                    if ( invert ) {
                        // ...unless we're doing night-mode shenanigans, in which case, we need to fake a white background.
                        cl = 0x00FFFFFF;
                    } else {
                        continue;
                    }
                }
                
                lUInt32 dcl = 0;
                if ( dither ) {
#if (GRAY_INVERSE==1)
                    dcl = Dither1BitColor( cl ^ rgba_invert, x, yy ) ^ 1;
#else
                    dcl = Dither1BitColor( cl ^ rgba_invert, x, yy ) ^ 0;
#endif
                } else {
                    dcl = rgbToGrayMask( cl ^ rgba_invert, 1 ) & 1;
                }
                int byteindex = (xx >> 3);
                int bitindex = ((xx & 7));
                lUInt8 mask = 0x80 >> (bitindex);
                dcl = dcl << (7-bitindex);
                row[ byteindex ] = (lUInt8)((row[ byteindex ] & (~mask)) | dcl);
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

void LVImageScaledDrawCallback::OnEndDecode(LVImageSource *obj, bool)
{
    // If we're not smooth scaling, we're done!
#ifndef ANDROID
    if (!smoothscale)
    {
        return;
    }
    
    // Scale our decoded data...
    lUInt8 * sdata = nullptr;
    //fprintf( stderr, "Requesting smooth scaling (%dx%d -> %dx%d)\n", src_dx, src_dy, dst_dx, dst_dy );
    sdata = CRe::qSmoothScaleImage(decoded, src_dx, src_dy, false, dst_dx, dst_dy);
    if (sdata == nullptr) {
        // Hu oh... Scaling failed! Return *without* drawing anything!
        // We skipped map generation, so we can't easily fallback to nearest-neighbor...
        //fprintf( stderr, "Smooth scaling failed :(\n" );
        return;
    }
    
    // Process as usual, with a bit of a hack to avoid code duplication...
    smoothscale = false;
    for (int y=0; y < dst_dy; y++) {
        lUInt8 * row = sdata + (y * (dst_dx * 4));
        this->OnLineDecoded( obj, y, (lUInt32 *) row );
    }
    
    // This prints the unscaled decoded buffer, for debugging purposes ;).
    /*
        for (int y=0; y < src_dy; y++) {
            lUInt8 * row = decoded + (y * (src_dx * 4));
            this->OnLineDecoded( obj, y, (lUInt32 *) row );
        }
        */
    // And now that it's been rendered we can free the scaled buffer (it was allocated by CRe::qSmoothScaleImage).
    free(sdata);
#endif
}
