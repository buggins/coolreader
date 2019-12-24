/*******************************************************

   CoolReader Engine

   serialbuf.cpp:  serialization buffer class implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/serialbuf.h"

/// serialization/deserialization buffer

/// constructor of serialization buffer
SerialBuf::SerialBuf( int sz, bool autoresize )
	: _buf( (lUInt8*)malloc(sz) ), _ownbuf(true), _error(false), _autoresize(autoresize), _size(sz), _pos(0)
{
    memset( _buf, 0, _size );
}
/// constructor of deserialization buffer
SerialBuf::SerialBuf( const lUInt8 * p, int sz )
	: _buf( const_cast<lUInt8 *>(p) ), _ownbuf(false), _error(false), _autoresize(false), _size(sz), _pos(0)
{
}

SerialBuf::~SerialBuf()
{
    if ( _ownbuf )
        free( _buf );
}

bool SerialBuf::copyTo( lUInt8 * buf, int maxSize )
{
    if ( _pos==0 )
        return true;
    if ( _pos > maxSize )
        return false;
    memcpy( buf, _buf, _pos );
    return true;
}

/// checks whether specified number of bytes is available, returns true in case of error
bool SerialBuf::check( int reserved )
{
	if ( _error )
		return true;
	if ( space()<reserved ) {
        if ( _autoresize ) {
            _size = (_size>16384 ? _size*2 : 16384) + reserved;
            _buf = cr_realloc(_buf, _size );
            memset( _buf+_pos, 0, _size-_pos );
            return false;
        } else {
		    _error = true;
		    return true;
        }
	}
	return false;
}

// write methods
/// put magic signature
void SerialBuf::putMagic( const char * s )
{
	if ( check(1) )
		return;
	while ( *s ) {
		_buf[ _pos++ ] = *s++;
		if ( check(1) )
			return;
	}
}

#define SWAPVARS(t,a) \
{ \
  t tmp; \
  tmp = a; a = v.a; v.a = tmp; \
}
void SerialBuf::swap( SerialBuf & v )
{
    SWAPVARS(lUInt8 *, _buf)
    SWAPVARS(bool, _ownbuf)
    SWAPVARS(bool, _error)
    SWAPVARS(bool, _autoresize)
    SWAPVARS(int, _size)
    SWAPVARS(int, _pos)
}


/// add contents of another buffer
SerialBuf & SerialBuf::operator << ( const SerialBuf & v )
{
    if ( check(v.pos()) || v.pos()==0 )
		return *this;
    memcpy( _buf + _pos, v._buf, v._pos );
    _pos += v._pos;
	return *this;
}

SerialBuf & SerialBuf::operator << ( lUInt8 n )
{
	if ( check(1) )
		return *this;
	_buf[_pos++] = n;
	return *this;
}
SerialBuf & SerialBuf::operator << ( char n )
{
	if ( check(1) )
		return *this;
	_buf[_pos++] = (lUInt8)n;
	return *this;
}
SerialBuf & SerialBuf::operator << ( bool n )
{
	if ( check(1) )
		return *this;
	_buf[_pos++] = (lUInt8)(n ? 1 : 0);
	return *this;
}
SerialBuf & SerialBuf::operator << ( lUInt16 n )
{
	if ( check(2) )
		return *this;
	_buf[_pos++] = (lUInt8)(n & 255);
	_buf[_pos++] = (lUInt8)((n>>8) & 255);
	return *this;
}
SerialBuf & SerialBuf::operator << ( lInt16 n )
{
	if ( check(2) )
		return *this;
	_buf[_pos++] = (lUInt8)(n & 255);
	_buf[_pos++] = (lUInt8)((n>>8) & 255);
	return *this;
}
SerialBuf & SerialBuf::operator << ( lUInt32 n )
{
	if ( check(4) )
		return *this;
	_buf[_pos++] = (lUInt8)(n & 255);
	_buf[_pos++] = (lUInt8)((n>>8) & 255);
	_buf[_pos++] = (lUInt8)((n>>16) & 255);
	_buf[_pos++] = (lUInt8)((n>>24) & 255);
	return *this;
}
SerialBuf & SerialBuf::operator << ( lInt32 n )
{
	if ( check(4) )
		return *this;
	_buf[_pos++] = (lUInt8)(n & 255);
	_buf[_pos++] = (lUInt8)((n>>8) & 255);
	_buf[_pos++] = (lUInt8)((n>>16) & 255);
	_buf[_pos++] = (lUInt8)((n>>24) & 255);
	return *this;
}
SerialBuf & SerialBuf::operator << ( const lString16 & s )
{
	if ( check(2) )
		return *this;
	lString8 s8 = UnicodeToUtf8(s);
	lUInt16 len = (lUInt16)s8.length();
	(*this) << len;
	for ( int i=0; i<len; i++ ) {
		if ( check(1) )
			return *this;
		(*this) << (lUInt8)(s8[i]);
	}
	return *this;
}
SerialBuf & SerialBuf::operator << ( const lString8 & s8 )
{
	if ( check(2) )
		return *this;
	lUInt16 len = (lUInt16)s8.length();
	(*this) << len;
	for ( int i=0; i<len; i++ ) {
		if ( check(1) )
			return *this;
		(*this) << (lUInt8)(s8[i]);
	}
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lUInt8 & n )
{
	if ( check(1) )
		return *this;
	n = _buf[_pos++];
	return *this;
}

SerialBuf & SerialBuf::operator >> ( char & n )
{
	if ( check(1) )
		return *this;
	n = (char)_buf[_pos++];
	return *this;
}

SerialBuf & SerialBuf::operator >> ( bool & n )
{
	if ( check(1) )
		return *this;
    n = _buf[_pos++] ? true : false;
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lUInt16 & n )
{
	if ( check(2) )
		return *this;
	n = _buf[_pos++];
    n |= (((lUInt16)_buf[_pos++]) << 8);
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lInt16 & n )
{
	if ( check(2) )
		return *this;
	n = (lInt16)(_buf[_pos++]);
    n |= (lInt16)(((lUInt16)_buf[_pos++]) << 8);
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lUInt32 & n )
{
	if ( check(4) )
		return *this;
	n = _buf[_pos++];
    n |= (((lUInt32)_buf[_pos++]) << 8);
    n |= (((lUInt32)_buf[_pos++]) << 16);
    n |= (((lUInt32)_buf[_pos++]) << 24);
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lInt32 & n )
{
	if ( check(4) )
		return *this;
	n = (lInt32)(_buf[_pos++]);
    n |= (((lUInt32)_buf[_pos++]) << 8);
    n |= (((lUInt32)_buf[_pos++]) << 16);
    n |= (((lUInt32)_buf[_pos++]) << 24);
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lString8 & s8 )
{
	if ( check(2) )
		return *this;
    lUInt16 len = 0;
	(*this) >> len;
	s8.clear();
	s8.reserve(len);
	for ( int i=0; i<len; i++ ) {
		if ( check(1) )
			return *this;
        lUInt8 c = 0;
		(*this) >> c;
		s8.append(1, c);
	}
	return *this;
}

SerialBuf & SerialBuf::operator >> ( lString16 & s )
{
	lString8 s8;
	(*this) >> s8;
	s = Utf8ToUnicode(s8);
	return *this;
}

// read methods
bool SerialBuf::checkMagic( const char * s )
{
    if ( _error )
        return false;
	while ( *s ) {
		if ( check(1) )
			return false;
        if ( _buf[ _pos++ ] != *s++ ) {
            seterror();
			return false;
        }
	}
	return true;
}

/// add CRC32 for last N bytes
void SerialBuf::putCRC( int size )
{
    if ( error() )
        return;
    if ( size>_pos ) {
        *this << (lUInt32)0;
        seterror();
    }
    lUInt32 n = 0;
    n = lStr_crc32( n, _buf + _pos-size, size );
    *this << n;
}

/// get CRC32 for the whole buffer
lUInt32 SerialBuf::getCRC()
{
    if (error())
        return 0;
    lUInt32 n = 0;
    n = lStr_crc32( n, _buf, _pos );
    return n;
}

/// read crc32 code, comapare with CRC32 for last N bytes
bool SerialBuf::checkCRC( int size )
{
    if ( error() )
        return false;
    if ( size>_pos ) {
        seterror();
        return false;
    }
    lUInt32 n0 = 0;
    n0 = lStr_crc32(n0, _buf + _pos-size, size);
    lUInt32 n = 0;
    *this >> n;
    if ( error() )
        return false;
    if ( n!=n0 )
        seterror();
    return !error();
}
