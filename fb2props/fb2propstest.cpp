/***************************************************************************
 *   CoolReader, FB2 file properties plugin for LBook V3                   *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#include <stdlib.h>
#include <stdio.h>
#include <crengine.h>
#include "parser-properties.h"

//prototype
int GetBookProperties(char *name,  struct BookProperties* pBookProps, int localLanguage);

#ifdef _DEBUG
extern int STARTUP_FLAG;
#endif


int main( int argc, char * argv[] )
{
#ifdef _DEBUG
    STARTUP_FLAG = 0;
#endif

    //CRLog::setStdoutLogger();
    //CRLog::setLogLevel( CRLog::LL_TRACE );
    if ( argc!=2 ) {
        printf("Usage: fb2propstest filename.fb2\n");
        //getchar();
        return 1;
    }
    CRLog::info("Checking props of file %s", argv[1]);
    BookProperties props;
    int res = 0;
    //for ( int i=0; i<1000; i++ )
        res = GetBookProperties( argv[1], &props, 0 );
    if ( res==0 ) {
        printf("Error while trying to parse file %s\n", argv[1]);
        //getchar();
        return 2;
    }
    //res = GetBookProperties( argv[1], &props, 0 );
    //res = GetBookProperties( argv[1], &props, 0 );
    //res = GetBookProperties( argv[1], &props, 0 );
    printf("Results ---------------------------------------------------- \n");
    printf("File name: %s\n", props.filename);
    printf("File time: %s\n", props.filedate);
    printf("File size: %d\n", (int)props.filesize);
    printf("Book name: %s\n", props.name);
    printf("Book author: %s\n", props.author);
    printf("Book series: %s\n", props.series);
    //getchar();
#ifdef _DEBUG
    STARTUP_FLAG = 1;
#endif
    return 0;
}
