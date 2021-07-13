/** @file lvdefstreambuffer.h
    @brief Universal Read or write buffer for stream region for non-maped streams

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

#ifndef __LVDEFSTREAMBUFFER_H_INCLUDED__
#define __LVDEFSTREAMBUFFER_H_INCLUDED__

#include "lvstreambuffer.h"
#include "lvstream.h"

// default implementation, with RAM buffer
class LVDefStreamBuffer : public LVStreamBuffer
{
protected:
    LVStreamRef m_stream;
    lUInt8 * m_buf;
    lvpos_t m_pos;
    lvsize_t m_size;
    bool m_readonly;
    bool m_writeonly;
public:
    static LVStreamBufferRef create( LVStreamRef stream, lvpos_t pos, lvsize_t size, bool readonly );

    LVDefStreamBuffer( LVStreamRef stream, lvpos_t pos, lvsize_t size, bool readonly );
    /// get pointer to read-only buffer, returns NULL if unavailable
    virtual const lUInt8 * getReadOnly()
    {
        return m_writeonly ? NULL : m_buf;
    }
    /// get pointer to read-write buffer, returns NULL if unavailable
    virtual lUInt8 * getReadWrite()
    {
        return m_readonly ? NULL : m_buf;
    }
    /// get buffer size
    virtual lvsize_t getSize()
    {
        return m_size;
    }
    /// write on close
    virtual bool close();
    /// flush on destroy
    virtual ~LVDefStreamBuffer()
    {
        close(); // NOLINT: Call to virtual function during destruction
    }
};

#endif  // __LVDEFSTREAMBUFFER_H_INCLUDED__
