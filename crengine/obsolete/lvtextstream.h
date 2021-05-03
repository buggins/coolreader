/** \file lvtextstream.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*/

#ifndef __LVTEXTSTREAM_H_INCLUDED__
#define __LVTEXTSTREAM_H_INCLUDED__

#include "lvstreamproxy.h"

class LVTextStream : public LVStreamProxy
{
public:
    virtual lvopen_mode_t GetMode();
    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos );
    virtual lverror_t Tell( lvpos_t * pPos );
    virtual lvpos_t   SetPos(lvpos_t p);
    virtual lvpos_t   GetPos();
    virtual lverror_t SetSize( lvsize_t size );
    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead );
    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten );
    virtual bool Eof();
    LVTextStream( LVStream * stream ) : LVStreamProxy(stream)
    { }
};

#endif  // __LVTEXTSTREAM_H_INCLUDED__
