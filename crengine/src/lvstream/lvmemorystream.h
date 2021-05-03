/** @file lvmemorystream.h

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

#ifndef __LVMEMORYSTREAM_H_INCLUDED__
#define __LVMEMORYSTREAM_H_INCLUDED__

#include "lvnamedstream.h"

class LVMemoryStream : public LVNamedStream {
protected:
    lUInt8 *m_pBuffer;
    bool m_own_buffer;
    LVContainer *m_parent;
    lvsize_t m_size;
    lvsize_t m_bufsize;
    lvpos_t m_pos;
    lvopen_mode_t m_mode;
public:
    /// Check whether end of file is reached
    /**
        \return true if end of file reached
    */
    virtual bool Eof() {
        return m_pos >= m_size;
    }

    virtual lvopen_mode_t GetMode() {
        return m_mode;
    }

    /** \return LVERR_OK if change is ok */
    virtual lverror_t SetMode(lvopen_mode_t mode);

    virtual LVContainer *GetParentContainer() {
        return (LVContainer *) m_parent;
    }

    virtual lverror_t Read(void *buf, lvsize_t count, lvsize_t *nBytesRead);

    virtual lvsize_t GetSize();

    virtual lverror_t GetSize(lvsize_t *pSize);

    // ensure that buffer is at least new_size long
    lverror_t SetBufSize(lvsize_t new_size);

    virtual lverror_t SetSize(lvsize_t size);

    virtual lverror_t Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten);

    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos);

    lverror_t Close();

    lverror_t Create();

    /// Creates memory stream as copy of another stream.
    lverror_t CreateCopy(LVStreamRef srcStream, lvopen_mode_t mode);

    lverror_t CreateCopy(const lUInt8 *pBuf, lvsize_t size, lvopen_mode_t mode);

    lverror_t Open(lUInt8 *pBuf, lvsize_t size);

    LVMemoryStream();

    virtual ~LVMemoryStream();
};

#endif  // __LVMEMORYSTREAM_H_INCLUDED__
