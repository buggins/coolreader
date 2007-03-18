/** \file s32utils.cpp
    \brief misc symbian utility functions

    CoolReader Engine


    (c) torque, 2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

// lvfonttest.cpp

#include "../include/crsetup.h"
#ifdef __SYMBIAN32__

#include <e32base.h>
#include <w32std.h>

#include "../include/s32utils.h"
#include "../include/lvstream.h"


void DrawBuf2DC(CWindowGc &dc, int x, int y, LVDrawBuf * buf, unsigned long * palette, int scale )
{

    unsigned long * drawpixels;
    
    int buf_width = buf->GetWidth();
    int bytesPerRow = (buf_width * buf->GetBitsPerPixel() + 7) / 8;
    CFbsBitmap* offScreenBitmap = new (ELeave) CFbsBitmap();
		User::LeaveIfError(offScreenBitmap->Create(TSize(buf_width * scale, 1),EGray2));
		CleanupStack::PushL(offScreenBitmap);
		
		// create an off-screen device and context
		CGraphicsContext* bitmapContext=NULL;
		CFbsBitmapDevice* bitmapDevice = CFbsBitmapDevice::NewL(offScreenBitmap);
		CleanupStack::PushL(bitmapDevice);
		User::LeaveIfError(bitmapDevice->CreateContext(bitmapContext));
		CleanupStack::PushL(bitmapContext);

    drawpixels = offScreenBitmap->DataAddress();
    
    int pixelsPerByte = (8 / buf->GetBitsPerPixel());
    int mask = (1<<buf->GetBitsPerPixel()) - 1;
        
    for (int yy=0; yy<buf->GetHeight(); yy++)
    {
       unsigned char * src = buf->GetScanLine(yy);
       for (int yyi = 0; yyi<scale; yyi++)
       {
          for (int xx=0; xx<bytesPerRow; xx++)
          {
             unsigned int b = src[xx];
             int x0 = 0;
             for (int shift = 8-buf->GetBitsPerPixel(); x0<pixelsPerByte; shift -= buf->GetBitsPerPixel(), x0++ )
             {
                 int dindex = (xx*pixelsPerByte + x0)*scale;
                 if ( dindex>=buf_width )
                      break;
                 unsigned long * px = drawpixels + dindex;
                 for (int xxi=0; xxi<scale; xxi++)
                 {
                     px[xxi] = palette[(b >> shift)&mask];
                 }
             }
          }
          dc.BitBlt(TPoint(x,y+yy*scale+yyi),offScreenBitmap);
       }
    }
    // cleanup
		CleanupStack::PopAndDestroy(3);
}

#endif
