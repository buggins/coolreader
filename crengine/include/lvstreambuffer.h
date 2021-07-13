/** @file lvstreambuffer.h
    @brief Abstract read or write buffer for stream region

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

#ifndef __LVSTREAMBUFFER_H_INCLUDED__
#define __LVSTREAMBUFFER_H_INCLUDED__

#include "lvref.h"
#include "lvstream_types.h"

/// Read or write buffer for stream region
class LVStreamBuffer : public LVRefCounter
{
public:
    /// get pointer to read-only buffer, returns NULL if unavailable
    virtual const lUInt8 * getReadOnly() = 0;
    /// get pointer to read-write buffer, returns NULL if unavailable
    virtual lUInt8 * getReadWrite() = 0;
    /// get buffer size
    virtual lvsize_t getSize() = 0;
    /// flush on destroy
    virtual ~LVStreamBuffer() {
        close(); // NOLINT: Call to virtual function during destruction
    }
    /// detach from stream, write changes if necessary
    virtual bool close() { return true; }
};

typedef LVFastRef<LVStreamBuffer> LVStreamBufferRef;

#endif  // __LVSTREAMBUFFER_H_INCLUDED__
