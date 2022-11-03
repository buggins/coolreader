/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#include <crengine.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if ( argc<4 ) {
        printf("Usage: logocnv startlogo stoplogo outfile\n");
        exit(1);
    }
    LVImageSourceRef startimg = LVCreateFileCopyImageSource( lString16(argv[1]) );
    if ( startimg.isNull() ) {
        printf("Cannot open image from file %s\n", argv[1]);
        exit(1);
    }
    printf("Start image: %s %d x %d\n", argv[1], startimg->GetWidth(), startimg->GetHeight());
    LVGrayDrawBuf buf1( 600, 800, 3 );
    buf1.Draw(startimg, 0, 0, 600, 800, true);
    LVImageSourceRef stopimg = LVCreateFileCopyImageSource( lString16(argv[2]) );
    if ( stopimg.isNull() ) {
        printf("Cannot open image from file %s\n", argv[2]);
        exit(1);
    }
    printf("Stop image: %s %d x %d\n", argv[1], startimg->GetWidth(), startimg->GetHeight());
    LVGrayDrawBuf buf2( 600, 800, 3 );
    buf2.Draw(stopimg, 0, 0, 600, 800, true);

    FILE * out = fopen( argv[3], "wb" );
    if ( !out ) {
        printf("Cannot create output file %s", argv[3]);
        exit(1);
    }
    int written = 0;
    written += fwrite(buf1.GetScanLine(0), 1, buf1.GetRowSize()*buf1.GetHeight(), out );
    written += fwrite(buf2.GetScanLine(0), 1, buf2.GetRowSize()*buf2.GetHeight(), out );
    fclose(out);
    printf("%d bytes written to file %s\n", written, argv[3]);
}
