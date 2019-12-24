/** \file dumpfile.h
   \brief dump file interface

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.
*/

#ifndef __DUMPFILE_H_INCLUDED__
#define __DUMPFILE_H_INCLUDED__

#ifdef _DEBUG
#include <stdio.h>
class DumpFile
{
public:
    FILE * f;
    DumpFile( const char * fname )
     : f(NULL)
    {
        if ( fname )
            f = fopen( fname, "at" );
        if ( !f )
            f = stdout;
        fprintf(f, "DumpFile log started\n");
    }
    ~DumpFile()
    {
        if ( f!=stdout )
            fclose(f);
    }
    operator FILE * () { if (f) fflush(f); return f?f:stdout; }
};
#endif

#endif // __DUMPFILE_H_INCLUDED__
