/** @file lvnamedstream.h
    @brief named stream class

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

#ifndef __LVNAMEDSTREAM_H_INCLUDED__
#define __LVNAMEDSTREAM_H_INCLUDED__

#include "lvstream.h"

class LVNamedStream : public LVStream
{
protected:
    lString32 m_fname;
    lString32 m_filename;
    lString32 m_path;
    lvopen_mode_t m_mode;
    lUInt32 _crc;
    bool _crcFailed;
    lvsize_t _autosyncLimit;
    lvsize_t _bytesWritten;
    virtual void handleAutoSync(lvsize_t bytesWritten) {
        _bytesWritten += bytesWritten;
        if (_autosyncLimit==0)
            return;
        if (_bytesWritten>_autosyncLimit) {
            Flush(true);
            _bytesWritten = 0;
        }
    }
public:
    LVNamedStream() : m_mode(LVOM_ERROR), _crc(0), _crcFailed(false), _autosyncLimit(0), _bytesWritten(0) { }
    /// set write bytes limit to call flush(true) automatically after writing of each sz bytes
    virtual void setAutoSyncSize(lvsize_t sz) { _autosyncLimit = sz; }
    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName();
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name);
    /// returns open mode
    virtual lvopen_mode_t GetMode()
    {
        return (lvopen_mode_t)(m_mode & LVOM_MASK);
    }
    /// calculate crc32 code for stream, if possible
    virtual lverror_t getcrc32( lUInt32 & dst );
};

#endif  // __LVNAMEDSTREAM_H_INCLUDED__
