/** \file tinydict.cpp
    \brief .dict dictionary file support implementation

    (c) Vadim Lopatin, 2009

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include <stdlib.h>
#include "tinydict.h"

int TinyDictWord::base64table[128] = { 0 };
static const char * base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned TinyDictWord::parseBase64( const char * str )
{
    int i;
    if ( !*base64table ) {
        for ( i=0; i<128; i++ )
            base64table[i] = -1;
        for ( i=0; base64chars[i]; i++ )
            base64table[base64chars[i]] = i;
    }
    unsigned n = 0;
    for ( ; *str; str++ ) {
        int code = base64table[ *str ];
        if ( code<0 )
            return (unsigned)-1;
        n = ( n << 6 ) + code;
    }
    return n;
}

int TinyDictWord::compare( const char * str ) const
{
    return strcmp( word, str );
}

static int my_fgets( char * buf, int size, FILE * f )
{
	int i=0;
	for ( ; i<size; i++ ) {
		int ch = fgetc( f );
		if ( ch == '\n' )
			break;
		if ( ch > 0 )
			buf[i] = (char) ch;
		else 
			break;
	}
	buf[i] = 0;
	return i;
}

/// factory - reading from index file
TinyDictWord * TinyDictWord::read( FILE * f, unsigned index )
{
    if ( !f || feof(f) )
        return NULL;
    char buf[1024];
    unsigned indexpos = ftell( f );
    int sz = my_fgets( buf, 1023, f );
	if ( !sz )
        return NULL;
    int tabc = 0;
    int tabs[2];
    for ( int i=0; buf[i]; i++ ) {
        if ( buf[i] == '\t' && tabc<2 )
            tabs[tabc++] = i;
    }
    if ( tabc!=2 )
        return NULL;
    const char * word = buf;
    const char * pos_str = buf + tabs[0] + 1;
    const char * len_str = buf + tabs[1] + 1;
    buf[tabs[0]] = 0;
    buf[tabs[1]] = 0;
    unsigned start = parseBase64( pos_str );
    unsigned len = parseBase64( len_str );
    if ( start==(unsigned)-1 || len==(unsigned)-1 )
        return NULL;
    return new TinyDictWord( index, indexpos, start, len, word );
}

int TinyDictWordList::find( const char * prefix )
{
    if ( !count )
        return -1;
    int a = 0;
    int b = count;
    for ( ;a < b-1; ) {
        int c = (a + b) / 2;
        int res = list[c]->compare( prefix );
        if ( !res )
            return c;
        if ( res < 0 ) {
            a = c + 1;
        } else {
            b = c;
        }
    }
    if ( a==0 || list[a]->compare( prefix )<0 )
        return a;
    return a - 1;
}

bool TinyDictWord::match( const char * str, bool exact ) const
{
    if ( exact )
        return !strcmp( word, str );
    int i=0;
    for ( ; str[i]; i++  ) {
        if ( str[i] != word[i] )
            return false;
    }
    return str[i]==0;
}

bool TinyDictIndexFile::find( const char * prefix, bool exactMatch, TinyDictWordList & words )
{
    words.clear();
    int n = list.find( prefix );
    if ( n<0 )
        return false;
    TinyDictWord * p = list.get( n );
    if ( fseek( f, p->getIndexPos(), SEEK_SET ) )
        return false;
    int index = p->getIndex();
    for ( ;; ) {
        p = TinyDictWord::read( f, index++ );
        if ( !p )
            break;
        int res = p->compare( prefix );
        if ( p->match( prefix, exactMatch ) )
            words.add( p );
		else {
            delete p;
			if ( res > 0 )
	            break;
        }
    }
    return true;
}

bool TinyDictIndexFile::open( const char * filename )
{
    close();
    if ( filename )
        setFilename( filename );
    if ( !fname )
        return false;
    f = fopen( fname, "rb" );
    if ( !f )
        return false;
    if ( fseek( f, 0, SEEK_END ) ) {
        close();
        return false;
    }
    size = ftell( f );
    if ( fseek( f, 0, SEEK_SET ) ) {
        close();
        return false;
    }
    // test
    TinyDictWord * p;
    count = 0;
    for ( ;; count++ ) {
        p = TinyDictWord::read( f, count );
        if ( !p )
            break;
        if ( (count % factor) == 0 ) {
            list.add( p );
        } else {
            delete p;
        }
    }
    printf("%d words read from index\n", count);
    return true;
}

enum { 
    DICT_TEXT,
    DICT_GZIP,
    DICT_DZIP,
};

bool TinyDictZStream::zinit( unsigned char * next_in, unsigned avail_in, unsigned char * next_out, unsigned avail_out )
{
    zclose();
    if ( !zInitialized ) {
        zStream.zalloc    = NULL;
        zStream.zfree     = NULL;
        zStream.opaque    = NULL;
        zStream.next_in   = next_in;
        zStream.avail_in  = avail_in;
        zStream.next_out  = next_out;
        zStream.avail_out = avail_out;
        if (inflateInit2( &zStream, -15 ) != Z_OK ) {
            // zlib initialization failed
            return false;
        }
        zInitialized = true;
    }
	return true;
}

bool TinyDictZStream::zclose()
{
    if ( zInitialized ) {
        inflateEnd( &zStream );
        zInitialized = false;
    }
    return true;
}

#if 0
bool TinyDictZStream::seek( unsigned pos )
{
    if ( txtpos == pos )
        return true;

    if ( type == DICT_TEXT ) {
        if ( fseek( f, pos, SEEK_SET ) )
            return false;
        txtpos = pos;
        return true;
    }

    if ( pos >= unp_buffer_start && pos <= unp_buffer_start + unp_buffer_len )
        return skip( pos - unp_buffer_start );

    unsigned jumpPos = headerLength;
    bool jumpRequired = false;
    if ( type == DICT_DZIP ) {
        // DZIP: restart from chunk beginning
        int curr_chunk = unp_buffer_start / chunkLength;
        int dest_chunk = pos / chunkLength;
        if ( unp_buffer_start > pos || dest_chunk > curr_chunk ) {
            jumpPos = offsets[ dest_chunk ];
            unp_buffer_start = dest_chunk * chunkLength;
            unp_buffer_len = 0;
            jumpRequired = true;
        }
    } else {
        // GZIP: restart from file beginning
        if ( unp_buffer_start > pos ) {
            jumpPos = headerLength;
            unp_buffer_start = 0;
            unp_buffer_len = 0;
            jumpRequired = true;
        }
    }
    if ( jumpRequired ) {
        zclose();
        unp_buffer_start = jumpPos - headerLength;
        unp_buffer_len = 0;
        if ( fseek( f, jumpPos, SEEK_SET ) )
            return false;
    }
    // unp_buffer_len 
    if ( !zInitialized && !zinit() )
        return false; // cannot init deflate

    return skip( pos - unp_buffer_start );
}

bool TinyDictZStream::skip( unsigned sz )
{
    if ( !sz )
        return true;
    if ( sz < unp_buffer_len ) {
        int rest = unp_buffer_len - sz;
        for ( int i=0; i<rest; i++ )
            unp_buffer[ i ] = unp_buffer[ i + sz ];
        unp_buffer_len = rest;
        unp_buffer_start += sz;
        return true;
    }
    sz -= unp_buffer_len;
    unp_buffer_start += unp_buffer_len;
    unp_buffer_len = 0;

    if ( sz > 0 ) {
        if ( !zInitialized && !zinit() )
            return false; // cannot init deflate
    }
    return true;
}

#endif

bool TinyDictZStream::readChunk( unsigned n )
{
    if ( n >= chunkCount )
        return false;
    if ( !unp_buffer ) {
        unp_buffer = (unsigned char *)malloc( sizeof(unsigned char)*chunkLength );
        unp_buffer_size = chunkLength;
    }
    unp_buffer_start = n * chunkLength;

    if ( fseek( f, offsets[ n ], SEEK_SET ) ) {
        printf( "cannot seek to %d position\n", offsets[n] );
        return false;
    }
    int packsz = chunks[n];
    unsigned char * tmp = (unsigned char *)malloc( sizeof(unsigned char) * packsz );

    crc.reset();
    unsigned int bytesRead = readBytes( tmp, packsz );
    unsigned crc1 = crc.get();
    unsigned crc2 = readU32();
    if ( bytesRead != packsz || error ) {
        printf( "error reading packed data\n" );
        free( tmp );
        return false;
    }
    if ( crc1!=crc2  ) {
        printf( "CRC error: real: %08x expected: %08x\n", crc1, crc2 );
        //free( tmp );
        //return false;
    }
    zclose();
    if ( !zinit(tmp, packsz, unp_buffer, unp_buffer_size) ) {
    //if ( !zinit(unp_buffer, unp_buffer_size, tmp, packsz) ) {
        printf("cannot init deflater\n");
        return false;
    }
    printf("unpacking %d bytes\n", packsz);
/*
    zStream.next_in   = tmp;
    zStream.avail_in  = packsz;
    zStream.next_out  = unp_buffer;
    zStream.avail_out = unp_buffer_size;
    zStream.next_in   = unp_buffer;
    zStream.avail_in  = unp_buffer_size;
    zStream.next_out  = tmp;
    zStream.avail_out = packsz;
*/
    int err = inflate( &zStream,  Z_PARTIAL_FLUSH );
    printf("inflate result: %d\n", err);
    if ( err != Z_OK ) {
        printf("Inflate error %s (%d). avail_in=%d, avail_out=%d \n", zStream.msg, err, (int)zStream.avail_in, (int)zStream.avail_out);
        free( tmp );
        return false;
    }
    if ( zStream.avail_in ) {
        printf("Inflate: not all data read, still %d bytes available\n", (int)zStream.avail_in );
        free( tmp );
        return false;
    }
    unp_buffer_len = unp_buffer_size - zStream.avail_out;

    printf("freeing tmp\n");
    free( tmp );
    printf("done\n");



    if ( n < chunkCount-1 && unp_buffer_len!=chunkLength ) {
        printf("wrong chunk length\n");
        return false; // too short chunk data
    }


    zclose();
    return true;
}

bool TinyDictZStream::read( unsigned char * buf, unsigned start, unsigned len )
{
    if ( start >= unp_buffer_start && start < unp_buffer_start + unp_buffer_len ) {
        unsigned readyBytes = unp_buffer_len - (start-unp_buffer_start);
        if ( readyBytes > len )
            readyBytes = len;
        memcpy( buf, unp_buffer + (start-unp_buffer_start), readyBytes );
        buf += readyBytes;
        start += readyBytes;
        len -= readyBytes;
        if ( !len )
            return true;
    }
    unsigned n = start / chunkLength;
    if ( !readChunk( n ) )
        return false;
    unsigned readyBytes = unp_buffer_len - (start-unp_buffer_start);
    if ( readyBytes > len )
        readyBytes = len;
    memcpy( buf, unp_buffer + (start-unp_buffer_start), readyBytes );
    buf += readyBytes;
    start += readyBytes;
    len -= readyBytes;
    if ( !len )
        return true;
    return false;
}

#if 0
bool TinyDictZStream::unpack( unsigned start, unsigned len )
{
    if ( !f )
        return false;
    if ( start >= unp_buffer_start && start + len <= unp_buffer_start + unp_buffer_len )
        return true;
    if ( start + len > size )
        return false; // out of file range

    if ( !zInitialized && !zinit() )
        return false; // cannot init deflate

    unsigned end = start + len;
    unsigned firstChunk  = start / chunkLength;
    unsigned firstOffset = start - firstChunk * chunkLength;
    unsigned lastChunk   = end / chunkLength;
    unsigned lastOffset  = end - lastChunk * chunkLength;

    int idx = start / chunkLength;
    int off = start % chunkLength;
    int pos = offsets[idx];
    if ( fseek( f, pos, SEEK_SET ) )
        return false;
    unsigned sz = off + len;
    if ( !unp_buffer || unp_buffer_size < sz ) {
        unp_buffer_len = 0;
        unp_buffer_start = idx * chunkLength;
        unp_buffer = (unsigned char *)realloc( unp_buffer, unp_buffer_size );
        if ( !unp_buffer )
            return false;
        unp_buffer_size = sz;
    }
    return true;
}
#endif

TinyDictZStream::TinyDictZStream()
: f ( NULL ), size( 0 ), txtpos(0)
, headerLength(0), error( false )
, chunks(NULL), offsets(NULL), chunkLength(0), chunkCount(0)
, zInitialized(false), packed_size(0), unp_buffer(NULL), unp_buffer_start(0), unp_buffer_len(0), unp_buffer_size(0)
{
    memset( &zStream, 0, sizeof(zStream) );
}

TinyDictZStream::~TinyDictZStream()
{
    zclose();
    if ( unp_buffer )
        free( unp_buffer );
    if ( chunks )
        delete chunks;
    if ( offsets )
        delete offsets;
}

bool TinyDictZStream::open( FILE * file )
{
    f = file;
    error = false;
    if ( fseek( f, 0, SEEK_END ) ) {
        return false;
    }
    packed_size = ftell( f );
    if ( fseek( f, 0, SEEK_SET ) ) {
        return false;
    }

    crc.reset();
    unsigned char header[10];
    if ( fread( header, 1, sizeof(header), f )!=sizeof(header) ) {
        return false;
    }
    crc.update( header, sizeof(header) );
    if ( header[0]!=0x1f || header[1]!=0x8b ) { // 0x1F 0x8B -- GZIP magic
        type = DICT_TEXT;
        return true;
    }
    if ( header[2]!=8 ) {
        // unknown compression method
        return false;
    }
    unsigned char flg = header[3];
    headerLength = 10;

    const char FTEXT   = 1;    // Extra text
    const char FHCRC   = 2;    // Header CRC
    const char FEXTRA  = 4;    // Extra field
    const char FNAME   = 8;    // File name
    const char FCOMMENT = 16;   // File comment

    type = DICT_GZIP;

    packed_size = size;

    // Optional extra field
    if ( flg & FEXTRA ) {
        type = DICT_DZIP;
        extraLength = readU16();
        headerLength += extraLength + 2;
        subfieldID1 = readU8();
        subfieldID2 = readU8();
        subfieldLength = readU16(); // 2 bytes subfield length
        subfieldVersion = readU16(); // 2 bytes subfield version
        chunkLength = readU16(); // 2 bytes chunk length
        chunkCount = readU16(); // 2 bytes chunk count
        if ( error ) {
            return false;
        }
        chunks = new unsigned short[ chunkCount ];
        for (unsigned i=0; i<chunkCount; i++) {
            chunks[i] = readU16();
        }
        size = 0;
    } else {
        // GZIP is not supported, use DZIP
        return false;
    }
    // Skip optional file name
    if ( flg & FNAME ) {
        while (readU8() != 0 )
            headerLength++;
        headerLength++;
    }
    // Skip optional file comment
    if ( flg & FCOMMENT ) {
        while (readU8() != 0)
            headerLength++;
        headerLength++;
    }
    // Check optional header CRC
    if ( flg & FHCRC ) {
        int v = (int)crc.get() & 0xffff;
        if (readU16() != v) {
            // CRC failed
            error = true;
        }
        headerLength += 2;
    }

    if ( chunkCount ) {
        offsets = new unsigned int[ chunkCount ];
        offsets[0] = headerLength;
        size = chunks[0];
        for ( unsigned i=1; i<chunkCount; i++ ) {
            offsets[i] = offsets[i-1] + chunks[i-1];
            size += chunks[i];
        }
    }

    if ( fseek( f, headerLength, SEEK_SET ) ) {
        return false;
    }

    if ( !readChunk( chunkCount-1 ) ) {
        printf("Error reading chunk %d\n", chunkCount-1 );
        return false;
    }
    size = unp_buffer_start + unp_buffer_len;

    return true;
}

const char * TinyDictDataFile::read( const TinyDictWord * w )
{
    if ( !f || !w || w->getStart() + w->getSize() > size ) {
        printf("article is out of file range (%d)\n", size);
        return NULL;
    }

    reserve( w->getSize() + 1 );
    if ( !compressed ) {
        // uncompressed
        printf("reading uncompressed article\n");
        if ( fseek( f, w->getStart(), SEEK_SET ) )
            return NULL;
        if ( fread( buf, 1, w->getSize(), f ) != w->getSize() )
            return NULL;
    } else {
        // compressed
        printf("reading compressed article\n");
        if ( !zstream.read( (unsigned char*)buf, w->getStart(), w->getSize() ) )
            return NULL;
    }
    buf[ w->getSize() ] = 0;
    return buf;
}

bool TinyDictDataFile::open( const char * filename )
{
    close();
    if ( filename )
        setFilename( filename );
    if ( !fname )
        return false;
    f = fopen( fname, "rb" );
    if ( !f )
        return false;
    if ( fseek( f, 0, SEEK_END ) ) {
        close();
        return false;
    }
    size = ftell( f );
    if ( fseek( f, 0, SEEK_SET ) ) {
        close();
        return false;
    }


    unsigned char header[10];
    if ( fread( header, 1, sizeof(header), f )!=sizeof(header) ) {
        close();
        return false;
    }

    if ( header[0]!=0x1f || header[1]!=0x8b ) { // 0x1F 0x8B -- GZIP magic
        compressed = false;
        printf("data file %s is not compressed\n", filename);
        return true;
    }

    printf("data file %s is compressed\n", filename);
    compressed = true;
    if ( !zstream.open( f ) ) {
        printf("data file %s opening error\n", filename);
        close();
        return false;
    }
    size = zstream.getSize();
    return true;
}

#if 1
int main( int argc, const char * * argv )
{
    TinyDictIndexFile index;
    TinyDictDataFile data;
    TinyDictDataFile zdata;
    if ( !index.open("mueller7.index") ) {
        printf("cannot open index file mueller7.index\n");
        return -1;
    }
    if ( !data.open("mueller7.dict") ) {
        printf("cannot open data file mueller7.dict\n");
        return -1;
    }
    if ( !zdata.open("mueller7.dict.dz") ) {
        printf("cannot open data file mueller7.dict.dz\n");
        return -1;
    }
    TinyDictWordList words;
    const char * pattern = "full";
    index.find( pattern, true, words );
    printf( "%d words matched pattern %s\n", words.length(), pattern );
    for ( int i=0; i<words.length(); i++ ) {
        TinyDictWord * p = words.get(i);
        printf("%s %d %d\n", p->getWord(), p->getStart(), p->getSize() );
        const char * text = zdata.read( p );
        if ( text )
            printf( "article:\n%s\n", text );
        else
            printf( "cannot read article\n" );
    }
#ifdef _WIN32
	printf("Press any key...");
	getchar();
#endif
    return 0;
}
#endif
