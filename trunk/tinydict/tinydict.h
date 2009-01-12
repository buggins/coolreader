/** \file tinydict.h
    \brief .dict dictionary file support interface

    Lightweight implementation of .dict support, written from scratch.

    (c) Vadim Lopatin, 2009

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef TINYDICT_H_INCLUDED
#define TINYDICT_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

/// Word entry of index file
class TinyDictWord
{
    unsigned index;
    unsigned indexpos;
    unsigned start;
    unsigned size;
    char * word;
    static int base64table[128];
    static unsigned parseBase64( const char * str );
    TinyDictWord( unsigned _index, unsigned _indexpos, unsigned _start, unsigned _size, const char * _word )
    : index(_index)
    , indexpos(_indexpos)
    , start(_start)
    , size(_size)
    , word( strdup(_word) )
    {
    }
public:
    /// factory - reading from index file
    static TinyDictWord * read( FILE * f, unsigned index );

    // getters
    unsigned getIndexPos() const { return indexpos; }
    unsigned getIndex() const { return index; }
    unsigned getStart() const { return start; }
    unsigned getSize() const { return size; }
    const char * getWord() const { return word; }

    int compare( const char * str ) const;
    bool match( const char * str, bool exact ) const;

    ~TinyDictWord() { if ( word ) free( word ); }
};

/// word entry list
class TinyDictWordList
{
    TinyDictWord ** list;
    int size;
    int count;
public:

    int length() { return count; }

    int find( const char * prefix );

    TinyDictWord * get( int index )
    {
        return list[index];
    }

    void add( TinyDictWord * word )
    {
        if ( count>=size ) {
            size = size ? size * 2 : 32;
            list = (TinyDictWord **)realloc( list, sizeof(TinyDictWord *) * size );
        }
        list[ count++ ] = word;
    }

    void clear()
    {
        if ( list ) {
            for ( int i=0; i<count; i++ )
                delete list[i];
            free( list );
            list = NULL;
            count = size = 0;
        }
    }
    TinyDictWordList() : list(NULL), size(0), count(0) { }
    ~TinyDictWordList() { clear(); }
};

///
class TinyDictFileBase
{
protected:
    char * fname;
    FILE * f;
    size_t size;
    void setFilename( const char * filename )
    {
        if ( fname )
            free( fname );
        if ( filename && *filename )
            fname = strdup( filename );
        else
            fname = NULL;
    }
public:
    TinyDictFileBase() : fname(NULL), f(NULL), size(0)
    {
    }
    virtual ~TinyDictFileBase()
    {
        close();
        setFilename( NULL );
    }
    virtual void close()
    {
        if (f)
            fclose(f);
        f = NULL;
        size = 0;
    }
};

class TinyDictIndexFile : public TinyDictFileBase
{
    int    factor;
    int    count;
    TinyDictWordList list;
public:

    bool find( const char * prefix, bool exactMatch, TinyDictWordList & words );

    TinyDictIndexFile() : factor( 16 ), count(0)
    {
    }

    virtual ~TinyDictIndexFile()
    {
    }

    bool open( const char * filename );

};

class TinyDictCRC
{
    unsigned crc;
public:
    void reset()
    {
        crc = crc32( 0L, Z_NULL, 0 );
    }
    unsigned get()
    {
        return crc;
    }
    unsigned update( const void * data, unsigned size )
    {
        crc = crc32( crc, (const unsigned char *)data, size );
        return crc;
    }
    unsigned update( unsigned char b )
    {
        return update( &b, sizeof(b) );
    }
    unsigned update( unsigned short b )
    {
        return update( &b, sizeof(b) );
    }
    unsigned update( unsigned int b )
    {
        return update( &b, sizeof(b) );
    }
    TinyDictCRC()
    {
        reset();
    }
};

class TinyDictZStream
{
    FILE * f;
    TinyDictCRC crc;

    int type;
    unsigned size;
    unsigned txtpos;

    unsigned headerLength;
    bool error;
    unsigned short * chunks;
    unsigned int * offsets;
    unsigned extraLength;
    unsigned char subfieldID1;
    unsigned char subfieldID2;
    unsigned subfieldLength;
    unsigned subfieldVersion;
    unsigned chunkLength;
    unsigned chunkCount;

    bool     zInitialized;
    z_stream zStream;
    unsigned packed_size;
    unsigned char * unp_buffer;
    unsigned unp_buffer_start;
    unsigned unp_buffer_len;
    unsigned unp_buffer_size;

    unsigned int readU32()
    {
        unsigned char buf[4];
        if ( !error && f && fread( buf, 1, 4, f )==4 ) {
            crc.update( buf, 4 );
            return (((((((unsigned int)buf[3]) << 8) + buf[2]) << 8) + buf[1]) << 8 ) + buf[0];
        }
        error = true;
        return 0;
    }

    unsigned short readU16()
    {
        unsigned char buf[2];
        if ( !error && f && fread( buf, 1, 2, f )==2 ) {
            crc.update( buf, 2 );
            return (((unsigned short)buf[1]) << 8) + buf[0];
        }
        error = true;
        return 0;
    }

    unsigned char readU8()
    {
        unsigned char buf[1];
        if ( !error && f && fread( buf, 1, 1, f )==1 ) {
            crc.update( buf, 1 );
            return buf[0];
        }
        error = true;
        return 0;
    }

    bool unpack( unsigned start, unsigned len );

    bool zinit();

    bool zclose();

    bool seek( unsigned pos );

    bool skip( unsigned sz );

public:
    unsigned getSize() { return size; }
    TinyDictZStream();
    bool open( FILE * file );
    ~TinyDictZStream();
};

class TinyDictDataFile : public TinyDictFileBase
{
    bool compressed;
    char * buf;
    int    buf_size;

    TinyDictZStream zstream;

    void reserve( int sz )
    {
        if ( buf_size < sz ) {
            buf = (char*) realloc( buf, sizeof(char) * sz );
            buf_size = sz;
        }
    }


public:

    const char * read( const TinyDictWord * w );

    bool open( const char * filename );

    TinyDictDataFile() : compressed(false), buf(0), buf_size(0)
    {
    }

    virtual ~TinyDictDataFile()
    {
        if ( buf )
            free( buf );
    }
};

#endif //TINYDICT_H_INCLUDED
