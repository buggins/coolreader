/*******************************************************

   CoolReader Engine

   lvnamedstream.cpp:  name stream class

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*******************************************************/

#include "lvnamedstream.h"

const lChar32 * LVNamedStream::GetName()
{
    if (m_fname.empty())
        return NULL;
    return m_fname.c_str();
}

void LVNamedStream::SetName(const lChar32 * name)
{
    m_fname = name;
    m_filename.clear();
    m_path.clear();
    if (m_fname.empty())
        return;
    const lChar32 * fn = m_fname.c_str();

    const lChar32 * p = fn + m_fname.length() - 1;
    for ( ;p>fn; p--) {
        if (p[-1] == '/' || p[-1]=='\\')
            break;
    }
    int pos = (int)(p - fn);
    if (p>fn)
        m_path = m_fname.substr(0, pos);
    m_filename = m_fname.substr(pos, m_fname.length() - pos);
}

lverror_t LVNamedStream::getcrc32( lUInt32 & dst )
{
    if ( _crc!=0 ) {
        dst = _crc;
        return LVERR_OK;
    } else {
        if ( !_crcFailed ) {
            lverror_t res = LVStream::getcrc32( dst );
            if ( res==LVERR_OK ) {
                _crc = dst;
                return LVERR_OK;
            }
            _crcFailed = true;
        }
        dst = 0;
        return LVERR_FAIL;
    }
}
