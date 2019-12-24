/** \file serialbuf.h
    \brief serialization buffer interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_SERIALBUF_H_INCLUDED__
#define __LV_SERIALBUF_H_INCLUDED__

#include "lvtypes.h"
#include "lvstring.h"

/// serialization/deserialization buffer
class SerialBuf
{
    lUInt8 * _buf;
    bool _ownbuf;
    bool _error;
    bool _autoresize;
    int _size;
    int _pos;
public:
    /// swap content of buffer with another buffer
    void swap( SerialBuf & v );
    /// constructor of serialization buffer
    SerialBuf( int sz, bool autoresize = true );
    SerialBuf( const lUInt8 * p, int sz );
    ~SerialBuf();
    
    void set( lUInt8 * buf, int size )
    {
        if ( _buf && _ownbuf )
            free( _buf );
        _buf = buf;
        _ownbuf = true;
        _error = false;
        _autoresize = true;
        _size = _pos = size;
    }
    bool copyTo( lUInt8 * buf, int maxSize );
    inline lUInt8 * buf() { return _buf; }
    inline void setPos( int pos ) { _pos = pos; }
    inline int space() const { return _size-_pos; }
    inline int pos() const { return _pos; }
    inline int size() const { return _size; }
    
    /// returns true if error occured during one of operations
    inline bool error() const { return _error; }
    
    inline void seterror() { _error = true; }
    /// move pointer to beginning, clear error flag
    inline void reset() { _error = false; _pos = 0; }
    
    /// checks whether specified number of bytes is available, returns true in case of error
    bool check( int reserved );
    
    // write methods
    /// put magic signature
    void putMagic( const char * s );
    
    /// add CRC32 for last N bytes
    void putCRC( int N );
    
    /// returns CRC32 for the whole buffer
    lUInt32 getCRC();
    
    /// add contents of another buffer
    SerialBuf & operator << ( const SerialBuf & v );
    
    SerialBuf & operator << ( lUInt8 n );
    
    SerialBuf & operator << ( char n );
    
    SerialBuf & operator << ( bool n );
    
    SerialBuf & operator << ( lUInt16 n );
    
    SerialBuf & operator << ( lInt16 n );
    
    SerialBuf & operator << ( lUInt32 n );
    
    SerialBuf & operator << ( lInt32 n );
    
    SerialBuf & operator << ( const lString16 & s );
    
    SerialBuf & operator << ( const lString8 & s8 );
    
    // read methods
    SerialBuf & operator >> ( lUInt8 & n );
    
    SerialBuf & operator >> ( char & n );
    
    SerialBuf & operator >> ( bool & n );
    
    SerialBuf & operator >> ( lUInt16 & n );
    
    SerialBuf & operator >> ( lInt16 & n );
    
    SerialBuf & operator >> ( lUInt32 & n );
    
    SerialBuf & operator >> ( lInt32 & n );
    
    SerialBuf & operator >> ( lString8 & s8 );
    
    SerialBuf & operator >> ( lString16 & s );
    
    bool checkMagic( const char * s );
    /// read crc32 code, comapare with CRC32 for last N bytes
    bool checkCRC( int N );
};

#endif // __LV_SERIALBUF_H_INCLUDED__
