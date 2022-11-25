/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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
    
    SerialBuf & operator << ( const lString32 & s );
    
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
    
    SerialBuf & operator >> ( lString32 & s );
    
    bool checkMagic( const char * s );
    /// read crc32 code, comapare with CRC32 for last N bytes
    bool checkCRC( int N );
};

#endif // __LV_SERIALBUF_H_INCLUDED__
