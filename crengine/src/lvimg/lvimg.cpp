/*******************************************************

   CoolReader Engine

   lvimg.cpp:  Image formats support

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvtinydom.h"
#include "lvxpmimagesource.h"
#if (USE_LIBJPEG==1)
#include "lvjpegimagesource.h"
#endif
#if (USE_LIBPNG==1)
#include "lvpngimagesource.h"
#endif
#include "lvdummyimagesource.h"
#if (USE_GIF==1)
#include "lvgifimagesource.h"
#endif
#if (USE_NANOSVG==1)
#include "lvsvgimagesource.h"
#endif
#include "lvstretchimgsource.h"
#include "lvcolortransformimgsource.h"
#include "lvalphatransformimgsource.h"
#include "lvunpackedimgsource.h"
#include "lvdrawbufimgsource.h"
#include "crlog.h"


LVImageSourceRef LVCreateXPMImageSource( const char * data[] )
{
    LVImageSourceRef ref( new LVXPMImageSource( data ) );
    if ( ref->GetWidth()<1 )
        return LVImageSourceRef();
    return ref;
}

/// dummy image object, to show invalid image
LVImageSourceRef LVCreateDummyImageSource( ldomNode * node, int width, int height )
{
    return LVImageSourceRef( new LVDummyImageSource( node, width, height ) );
}

LVImageSourceRef LVCreateStreamImageSource( ldomNode * node, LVStreamRef stream )
{
    LVImageSourceRef ref;
    if ( stream.isNull() )
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
#if (USE_GIF==1)
    if ( LVGifImageSource::CheckPattern( hdr, (lUInt32)bytesRead ) )
        img = new LVGifImageSource( node, stream );
    else
#endif
#if (USE_NANOSVG==1)
    if ( LVSvgImageSource::CheckPattern( hdr, (lUInt32)bytesRead ) )
        img = new LVSvgImageSource( node, stream );
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

LVImageSourceRef LVCreateStreamImageSource( LVStreamRef stream )
{
    return LVCreateStreamImageSource( NULL, stream );
}

/// create image from node source
LVImageSourceRef LVCreateNodeImageSource( ldomNode * node )
{
    LVImageSourceRef ref;
    if (!node->isElement())
        return ref;
    LVStreamRef stream = node->createBase64Stream();
    if (stream.isNull())
        return ref;
//    if ( CRLog::isDebugEnabled() ) {
//        lUInt16 attr_id = node->getDocument()->getAttrNameIndex(U"id");
//        lString32 id = node->getAttributeValue(attr_id);
//        CRLog::debug("Opening node image id=%s", LCSTR(id));
//    }
    return LVCreateStreamImageSource( stream );
}

/// creates image source as memory copy of file contents
LVImageSourceRef LVCreateFileCopyImageSource( lString32 fname )
{
    return LVCreateStreamImageSource( LVCreateMemoryStream(fname) );
}

/// creates image source as memory copy of stream contents
LVImageSourceRef LVCreateStreamCopyImageSource( LVStreamRef stream )
{
    if ( stream.isNull() )
        return LVImageSourceRef();
    return LVCreateStreamImageSource( LVCreateMemoryStream(stream) );
}

/// creates image which stretches source image by filling center with pixels at splitX, splitY
LVImageSourceRef LVCreateStretchFilledTransform( LVImageSourceRef src, int newWidth, int newHeight, ImageTransform hTransform, ImageTransform vTransform, int splitX, int splitY )
{
	if ( src.isNull() )
		return LVImageSourceRef();
    return LVImageSourceRef( new LVStretchImgSource( src, newWidth, newHeight, hTransform, vTransform, splitX, splitY ) );
}

/// creates image which fills area with tiled copy
LVImageSourceRef LVCreateTileTransform( LVImageSourceRef src, int newWidth, int newHeight, int offsetX, int offsetY )
{
    if ( src.isNull() )
        return LVImageSourceRef();
    return LVImageSourceRef( new LVStretchImgSource( src, newWidth, newHeight, IMG_TRANSFORM_TILE, IMG_TRANSFORM_TILE,
                                                     offsetX, offsetY ) );
}

/// creates image source which transforms colors of another image source (add RGB components (c - 0x80) * 2 from addedRGB first, then multiplyed by multiplyRGB fixed point components (0x20 is 1.0f)
LVImageSourceRef LVCreateColorTransformImageSource(LVImageSourceRef srcImage, lUInt32 addRGB, lUInt32 multiplyRGB) {
    return LVImageSourceRef(new LVColorTransformImgSource(srcImage, addRGB, multiplyRGB));
}

/// creates image source which applies alpha to another image source (0 is no change, 255 is totally transparent)
LVImageSourceRef LVCreateAlphaTransformImageSource(LVImageSourceRef srcImage, int alpha) {
    if (alpha <= 0)
        return srcImage;
    return LVImageSourceRef(new LVAlphaTransformImgSource(srcImage, alpha));
}

/// creates decoded memory copy of image, if it's unpacked size is less than maxSize
LVImageSourceRef LVCreateUnpackedImageSource( LVImageSourceRef srcImage, int maxSize, bool gray )
{
    if ( srcImage.isNull() )
        return srcImage;
    int dx = srcImage->GetWidth();
    int dy = srcImage->GetHeight();
    int sz = dx*dy * (gray?1:4);
    if ( sz>maxSize )
        return srcImage;
    CRLog::trace("Unpacking image %dx%d (%d)", dx, dy, sz);
    LVUnpackedImgSource * img = new LVUnpackedImgSource( srcImage, gray ? 8 : 32 );
    CRLog::trace("Unpacking done");
    return LVImageSourceRef( img );
}

/// creates decoded memory copy of image, if it's unpacked size is less than maxSize
LVImageSourceRef LVCreateUnpackedImageSource( LVImageSourceRef srcImage, int maxSize, int bpp )
{
    if ( srcImage.isNull() )
        return srcImage;
    int dx = srcImage->GetWidth();
    int dy = srcImage->GetHeight();
    int sz = dx*dy * (bpp>>3);
    if ( sz>maxSize )
        return srcImage;
    CRLog::trace("Unpacking image %dx%d (%d)", dx, dy, sz);
    LVUnpackedImgSource * img = new LVUnpackedImgSource( srcImage, bpp );
    CRLog::trace("Unpacking done");
    return LVImageSourceRef( img );
}

LVImageSourceRef LVCreateDrawBufImageSource( LVColorDrawBuf * buf, bool own )
{
    return LVImageSourceRef( new LVDrawBufImgSource( buf, own ) );
}

/// draws battery icon in specified rectangle of draw buffer; if font is specified, draws charge %
// first icon is for charging, the rest - indicate progress icon[1] is lowest level, icon[n-1] is full power
// if no icons provided, battery will be drawn
void LVDrawBatteryIcon( LVDrawBuf * drawbuf, const lvRect & batteryRc, int percent, bool charging, LVRefVec<LVImageSource> icons, LVFont * font )
{
    lvRect rc( batteryRc );
    bool drawText = (font != NULL);
    if ( icons.length()>1 ) {
        int iconIndex = 0;
        if ( !charging ) {
            if ( icons.length()>2 ) {
                int numTicks = icons.length() - 1;
                int perTick = 10000/(numTicks -1);
                //iconIndex = ((numTicks - 1) * percent + (100/numTicks/2) )/ 100 + 1;
                iconIndex = (percent * 100 + perTick/2)/perTick + 1;
                if ( iconIndex<1 )
                    iconIndex = 1;
                if ( iconIndex>icons.length()-1 )
                    iconIndex = icons.length()-1;
            } else {
                // empty battery icon, for % display
                iconIndex = 1;
            }
        }

        lvPoint sz( icons[0]->GetWidth(), icons[0]->GetHeight() );
        rc.left += (rc.width() - sz.x)/2;
        rc.top += (rc.height() - sz.y)/2;
        rc.right = rc.left + sz.x;
        rc.bottom = rc.top + sz.y;
        LVImageSourceRef icon = icons[iconIndex];
        drawbuf->Draw( icon, rc.left,
            rc.top,
            sz.x,
            sz.y, false );
        if ( charging )
            drawText = false;
        rc.left += 3;
    } else {
        // todo: draw w/o icons
    }

    if ( drawText ) {
        // rc is rectangle to draw text to
        lString32 txt;
        if ( charging )
            txt = "+++";
        else
            txt = lString32::itoa(percent); // + U"%";
        int w = font->getTextWidth(txt.c_str(), txt.length());
        int h = font->getHeight();
        int x = (rc.left + rc.right - w)/2;
        int y = (rc.top + rc.bottom - h)/2+1;
        lUInt32 bgcolor = drawbuf->GetBackgroundColor();
        lUInt32 textcolor = drawbuf->GetTextColor();

        drawbuf->SetBackgroundColor( textcolor );
        drawbuf->SetTextColor( bgcolor );
        font->DrawTextString(drawbuf, x-1, y, txt.c_str(), txt.length(), '?', NULL);
        font->DrawTextString(drawbuf, x+1, y, txt.c_str(), txt.length(), '?', NULL);
//        font->DrawTextString(drawbuf, x-1, y+1, txt.c_str(), txt.length(), '?', NULL);
//        font->DrawTextString(drawbuf, x+1, y-1, txt.c_str(), txt.length(), '?', NULL);
        font->DrawTextString(drawbuf, x, y-1, txt.c_str(), txt.length(), '?', NULL);
        font->DrawTextString(drawbuf, x, y+1, txt.c_str(), txt.length(), '?', NULL);
//        font->DrawTextString(drawbuf, x+1, y+1, txt.c_str(), txt.length(), '?', NULL);
//        font->DrawTextString(drawbuf, x-1, y+1, txt.c_str(), txt.length(), '?', NULL);
        //drawbuf->SetBackgroundColor( textcolor );
        //drawbuf->SetTextColor( bgcolor );
        drawbuf->SetBackgroundColor( bgcolor );
        drawbuf->SetTextColor( textcolor );
        font->DrawTextString(drawbuf, x, y, txt.c_str(), txt.length(), '?', NULL);
    }
}
