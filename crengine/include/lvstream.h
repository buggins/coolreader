/** @file lvstream.h
    @brief Abstract stream class

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * UNRAR library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*/

#ifndef __LVSTREAM_H_INCLUDED__
#define __LVSTREAM_H_INCLUDED__

#include "lvtypes.h"
#include "lvref.h"
#include "lvstring.h"
#include "lvarray.h"
#include "crtimerutil.h"
#include "lvstorageobject.h"

#define LVOM_MASK 7
#define LVOM_FLAG_SYNC 0x10

class LVStreamBuffer;

typedef LVFastRef<LVStreamBuffer> LVStreamBufferRef;

/// Stream base class
class LVStream : public LVStorageObject {
public:
    /// Get read buffer (optimal for mmap)
    virtual LVStreamBufferRef GetReadBuffer(lvpos_t pos, lvpos_t size);

    /// Get read/write buffer (optimal for mmap)
    virtual LVStreamBufferRef GetWriteBuffer(lvpos_t pos, lvpos_t size);

    /// Get stream open mode
    /** \return lvopen_mode_t open mode */
    virtual lvopen_mode_t GetMode() { return LVOM_READ; }

    /// Set stream mode, supported not by all streams
    /** \return LVERR_OK if change is ok */
    virtual lverror_t SetMode(lvopen_mode_t) { return LVERR_NOTIMPL; }

    /// flushes unsaved data from buffers to file, with optional flush of OS buffers
    virtual lverror_t Flush(bool /*sync*/) { return LVERR_OK; }

    virtual lverror_t Flush(bool sync, CRTimerUtil & /*timeout*/ ) { return Flush(sync); }

    /// Seek (change file pos)
    /**
        \param offset is file offset (bytes) relateve to origin
        \param origin is offset base
        \param pNewPos points to place to store new file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t *pNewPos) = 0;

    /// Tell current file position
    /**
        \param pNewPos points to place to store file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Tell(lvpos_t *pPos) { return Seek(0, LVSEEK_CUR, pPos); }

    /// Set file position
    /**
        \param p is new position
        \return lverror_t status: LVERR_OK if success
    */
    //virtual lverror_t SetPos(lvpos_t p) { return Seek(p, LVSEEK_SET, NULL); }
    virtual lvpos_t SetPos(lvpos_t p) {
        lvpos_t pos;
        return (Seek(p, LVSEEK_SET, &pos) == LVERR_OK) ? pos : (lvpos_t) (~0);
    }

    /// Get file position
    /**
        \return lvpos_t file position
    */
    virtual lvpos_t GetPos() {
        lvpos_t pos;
        if (Seek(0, LVSEEK_CUR, &pos) == LVERR_OK)
            return pos;
        else
            return (lvpos_t) (~0);
    }

    /// Get file size
    /**
        \return lvsize_t file size
    */
    virtual lvsize_t GetSize() {
        lvpos_t pos = GetPos();
        lvsize_t sz = 0;
        Seek(0, LVSEEK_END, &sz);
        SetPos(pos);
        return sz;
    }

    virtual lverror_t GetSize(lvsize_t *pSize) {
        *pSize = GetSize();
        return LVERR_OK;
    }

    /// Set file size
    /**
        \param size is new file size
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t SetSize(lvsize_t size) = 0;

    /// Read
    /**
        \param buf is buffer to place bytes read from stream
        \param count is number of bytes to read from stream
        \param nBytesRead is place to store real number of bytes read from stream
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Read(void *buf, lvsize_t count, lvsize_t *nBytesRead) = 0;

    virtual bool Read(lUInt8 *buf) {
        lvsize_t nBytesRead;
        if (Read(buf, sizeof(lUInt8), &nBytesRead) == LVERR_OK && nBytesRead == sizeof(lUInt8))
            return true;
        return false;
    }

    virtual bool Read(lUInt16 *buf) {
        lvsize_t nBytesRead;
        if (Read(buf, sizeof(lUInt16), &nBytesRead) == LVERR_OK && nBytesRead == sizeof(lUInt16))
            return true;
        return false;
    }

    virtual bool Read(lUInt32 *buf) {
        lvsize_t nBytesRead;
        if (Read(buf, sizeof(lUInt32), &nBytesRead) == LVERR_OK && nBytesRead == sizeof(lUInt32))
            return true;
        return false;
    }

    virtual int ReadByte() {
        unsigned char buf[1];
        lvsize_t sz = 0;
        if (Read(buf, 1, &sz) == LVERR_OK && sz == 1)
            return buf[0];
        return -1;
    }

    /// Write
    /**
        \param buf is data to write to stream
        \param count is number of bytes to write
        \param nBytesWritten is place to store real number of bytes written to stream
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Write(const void *buf, lvsize_t count, lvsize_t *nBytesWritten) = 0;

    /// Check whether end of file is reached
    /**
        \return true if end of file reached
    */
    virtual bool Eof() = 0;

    /// writes array
    lverror_t Write(LVArray<lUInt32> &array);

    /// calculate crc32 code for stream, if possible
    virtual lverror_t getcrc32(lUInt32 &dst);

    /// calculate crc32 code for stream, returns 0 for error or empty stream
    inline lUInt32 getcrc32() {
        lUInt32 res = 0;
        getcrc32(res);
        return res;
    }

    /// set write bytes limit to call flush(true) automatically after writing of each sz bytes
    virtual void setAutoSyncSize(lvsize_t /*sz*/) {}

    /// Constructor
    LVStream() {}

    /// Destructor
    virtual ~LVStream() {}
};

/// Stream reference
typedef LVFastRef<LVStream> LVStreamRef;

/// Writes lString32 string to stream
inline LVStream &operator<<(LVStream &stream, const lString32 &str) {
    if (!str.empty())
        stream.Write(str.c_str(), sizeof(lChar32) * str.length(), NULL);
    return stream;
}

/// Writes lString8 string to stream
inline LVStream &operator<<(LVStream &stream, const lString8 &str) {
    if (!str.empty())
        stream.Write(str.c_str(), sizeof(lChar8) * str.length(), NULL);
    return stream;
}

/// Writes lChar32 string to stream
inline LVStream &operator<<(LVStream &stream, const lChar32 *str) {
    if (str)
        stream.Write(str, sizeof(lChar32) * lStr_len(str), NULL);
    return stream;
}

/// Writes lChar8 string to stream
inline LVStream &operator<<(LVStream &stream, const lChar8 *str) {
    if (str)
        stream.Write(str, sizeof(lChar8) * lStr_len(str), NULL);
    return stream;
}

/// Writes lUInt32 to stream
inline LVStream &operator<<(LVStream &stream, lUInt32 d) {
    stream.Write(&d, sizeof(d), NULL);
    return stream;
}

/// Writes lUInt16 to stream
inline LVStream &operator<<(LVStream &stream, lUInt16 d) {
    stream.Write(&d, sizeof(d), NULL);
    return stream;
}

/// Writes lUInt8 to stream
inline LVStream &operator<<(LVStream &stream, lUInt8 d) {
    stream.Write(&d, sizeof(d), NULL);
    return stream;
}

/// Writes value array to stream
template<typename T>
inline LVStream &operator<<(LVStream &stream, LVArray<T> &array) {
    stream.Write(array.ptr(), sizeof(T) * array.length(), NULL);
    return stream;
}

#endif // __LVSTREAM_H_INCLUDED__
