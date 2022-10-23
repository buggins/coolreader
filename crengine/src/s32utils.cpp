/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2006 torque                                             *
 *   Copyright (C) 2007,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
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
 * \file s32utils.cpp
 * \brief misc symbian utility functions
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

    TUint16 *drawpixels;
		int dwidth = buf->GetWidth()*scale;
		int width = buf->GetWidth();
    
    int bytesPerRow = (width * buf->GetBitsPerPixel() + 7) / 8;
    CFbsBitmap* offScreenBitmap = new (ELeave) CFbsBitmap();
		User::LeaveIfError(offScreenBitmap->Create(TSize(buf->GetWidth()*scale, buf->GetHeight()*scale), EColor64K));
		
		// create an off-screen device and context
		CGraphicsContext* bitmapContext=NULL;
		CFbsBitmapDevice* bitmapDevice = CFbsBitmapDevice::NewL(offScreenBitmap);
		User::LeaveIfError(bitmapDevice->CreateContext(bitmapContext));

		offScreenBitmap->LockHeap();
		drawpixels = (TUint16 *)offScreenBitmap->DataAddress();
		offScreenBitmap->UnlockHeap();
    
    int pixelsPerByte = (8 / buf->GetBitsPerPixel());
    int mask = (1<<buf->GetBitsPerPixel()) - 1;
        
    for (int yy=0; yy<buf->GetHeight(); yy++)
    {
       unsigned char * src = buf->GetScanLine(yy);

       for (int yyi = 0; yyi<scale; yyi++)
       {
				if ( buf->GetBitsPerPixel()==32 )
          {
					for (int xx=0; xx<width; xx++)
             {
						TUint32 p32 = ((TUint32*)src)[xx];
						TUint16 p16 = ((p32&0xF80000)>>8) | ((p32&0x00FC00)>>5) | ((p32&0x0000F8)>>3);
                 for (int xxi=0; xxi<scale; xxi++)
                 {
							*drawpixels++ = p16;
                 }
             }
          }
       }
    }
		dc.BitBlt(TPoint(0, 0), offScreenBitmap);//, TRect(x, y, buf->GetWidth()*scale, buf->GetHeight()*scale));
		
		delete bitmapContext;
		delete bitmapDevice;
		delete offScreenBitmap;
}

#endif
