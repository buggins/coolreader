/** \file tinydict.cpp
    \brief .dict dictionary file support implementation

    (c) Vadim Lopatin, 2009

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include <stdlib.h>
#include "tinydict.h"


/// add word to list
void TinyDictWordList::add( TinyDictWord * word )
{
    if ( count>=size ) {
        size = size ? size * 2 : 32;
        list = (TinyDictWord **)realloc( list, sizeof(TinyDictWord *) * size );
    }
    list[ count++ ] = word;
}

/// clear list
void TinyDictWordList::clear()
{
    if ( list ) {
        for ( int i=0; i<count; i++ )
            delete list[i];
        free( list );
        list = NULL;
        count = size = 0;
    }
}

/// empty list constructor
TinyDictWordList::TinyDictWordList() : dict(NULL), list(NULL), size(0), count(0) { }

/// destructor
TinyDictWordList::~TinyDictWordList() { clear(); }


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

	void compact()
	{
		// do nothing
	}

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

    unsigned int readBytes( unsigned char * buf, unsigned size )
    {
        if ( error || !f )
            return 0;
        unsigned int bytesRead = fread( buf, 1, size, f );
        crc.update( buf, bytesRead );
        return bytesRead;
    }

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

    bool zinit(unsigned char * next_in, unsigned avail_in, unsigned char * next_out, unsigned avail_out);

    bool zclose();

    bool readChunk( unsigned n );

public:
	/// minimize memory consumption
	void compact();
	/// get unpacked data size
    unsigned getSize() { return size; }
	/// create uninitialized stream
    TinyDictZStream();
	/// open from file
    bool open( FILE * file );
	/// read block of data
    bool read( unsigned char * buf, unsigned start, unsigned len );
	/// close stream
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
            char * oldptr = buf;
            buf = (char*) realloc( buf, sizeof(char) * sz );
            if ( !buf) {
                free(oldptr);
                fprintf(stderr, "out of memory\n");
                exit(-2);
            }
            buf_size = sz;
        }
    }


public:

	void compact()
	{
		zstream.compact();
		if ( buf ) {
			free( buf );
			buf = NULL;
			buf_size = 0;
		}
	}
	
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


static int base64table[128] = { 0 };
static const char * base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned parseBase64( const char * str )
{
    int i;
    if ( !*base64table ) {
        for ( i=0; i<128; i++ )
            base64table[i] = -1;
        for ( i=0; base64chars[i]; i++ )
            base64table[(unsigned)base64chars[i]] = i;
    }
    unsigned n = 0;
    for ( ; *str; str++ ) {
        int code = base64table[ (unsigned)*str ];
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
    DICT_DZIP
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

void TinyDictZStream::compact()
{
	if ( unp_buffer ) {
		free( unp_buffer );
		unp_buffer_start = unp_buffer_len = unp_buffer_size = 0;
		unp_buffer = NULL;
	}
}

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
    unsigned packsz = chunks[n];
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
        printf("cannot init deflater\n");
        return false;
    }
    printf("unpacking %d bytes\n", packsz);
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

    //const char FTEXT   = 1;    // Extra text
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

	compact();
    return true;
}

const char * TinyDictDataFile::read( const TinyDictWord * w )
{
    if ( !f || !w || w->getStart() + w->getSize() > size ) {
        printf("article is out of file range (%d)\n", (int)size);
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

TinyDictionary::TinyDictionary()
{
	name = NULL;
	data = new TinyDictDataFile();
	index = new TinyDictIndexFile();
}

TinyDictionary::~TinyDictionary()
{
	delete data;
	delete index;
	if ( name )
		free( name );
}

void TinyDictionary::compact()
{
	index->compact();
	data->compact();
}

const char * TinyDictionary::getDictionaryName()
{
	return name;
}

bool TinyDictionary::open( const char * indexfile, const char * datafile )
{
	// use index file name w/o path and extension as dictionary name
	int lastSlash = -1;
	int lastPoint = -1;
	for ( int i=0; indexfile[i]; i++ ) {
		if ( indexfile[i]=='/' || indexfile[i]=='\\' )
			lastSlash = i;
		else if ( indexfile[i]=='.' )
			lastPoint = i;
	}
	if ( lastPoint>=0 && lastPoint>lastSlash ) {
		name = strdup( indexfile + lastSlash + 1 );
		name[ lastPoint - lastSlash - 1 ] = 0;
	}
	return index->open( indexfile ) && data->open( datafile );
}

/// returns word list's dictionary name
const char * TinyDictWordList::getDictionaryName()
{
	if ( !dict )
		return NULL;
	return dict->getDictionaryName();
}

/// returns article for word by index
const char * TinyDictWordList::getArticle( int index )
{
	if ( !dict )
		return NULL;
	if ( index<0 || index>=count )
		return NULL;
	return dict->getData()->read( list[index] );
}

/// searches dictionary for specified word, caller is responsible for deleting of returned object
TinyDictWordList * TinyDictionary::find( const char * prefix, int options )
{
	TinyDictWordList * list = new TinyDictWordList();
	list->setDict( this );
	if ( index->find( prefix, (TINY_DICT_OPTION_STARTS_WITH & options) == 0, *list ) && list->length()>0 )
		return list;
	delete list;
	return NULL;
}

bool TinyDictionaryList::add( const char * indexfile, const char * datafile )
{
	TinyDictionary * p = new TinyDictionary();
	if ( !p->open( indexfile, datafile ) ) {
		delete p;
		return false;
	}
    if ( count>=size ) {
        size = size ? size * 2 : 32;
        list = (TinyDictionary**)realloc( list, sizeof(TinyDictionary *) * size );
    }
    list[ count++ ] = p;
    return true;
}

/// create empty list
TinyDictionaryList::TinyDictionaryList() : list(NULL), size(0), count(0) { }

/// remove all dictionaries from list
void TinyDictionaryList::clear()
{
	if ( list ) {
		for ( int i=0; i<count; i++ )
			delete list[i];
		free( list );
		list = NULL;
		size = 0;
		count = 0;
	}
}

TinyDictionaryList::~TinyDictionaryList()
{
	clear();
}

/// search all dictionaries in list for specified pattern
bool TinyDictionaryList::find( TinyDictResultList & result, const char * prefix, int options )
{
	result.clear();
	for ( int i=0; i<count; i++ ) {
		TinyDictWordList * p = list[i]->find( prefix, options );
		if ( p )
			result.add( p );
	}
	return result.length() > 0;
}


/// remove all dictionaries from list
void TinyDictResultList::clear()
{
	if ( list ) {
		for ( int i=0; i<count; i++ )
			delete list[i];
		free( list );
		list = NULL;
		size = 0;
		count = 0;
	}
}

/// create empty list
TinyDictResultList::TinyDictResultList() : list(NULL), size(0), count(0) { }

/// destructor
TinyDictResultList::~TinyDictResultList()
{
	clear();
}

/// add item to list
void TinyDictResultList::add( TinyDictWordList * p )
{
    if ( count>=size ) {
        size = size ? size * 2 : 32;
        list = (TinyDictWordList**)realloc( list, sizeof(TinyDictWordList *) * size );
    }
    list[ count++ ] = p;
}

#ifdef TEST_APP
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

	{
		// create TinyDictionaryList object
		TinyDictionaryList dicts;
		// register dictionaries using 
		dicts.add( "mueller7.index", "mueller7.dict.dz" );

		// container for results
		TinyDictResultList results;
	    dicts.find(results, "empty", 0 ); // find exact match

		// for each source dictionary that matches pattern
		for ( int d = 0; d<results.length(); d++ ) {
			TinyDictWordList * words = results.get(d);
			printf("dict: %s\n", words->getDictionaryName() );
			// for each found word
			for ( int i=0; i<words->length(); i++ ) {
				TinyDictWord * word = words->get(i);
				printf("word: %s\n", word->getWord() );
				printf("article: %s\n", words->getArticle( i ) );
			}
		}
	}
#ifdef _WIN32
	printf("Press any key...");
	getchar();
#endif
    return 0;
}
#endif
