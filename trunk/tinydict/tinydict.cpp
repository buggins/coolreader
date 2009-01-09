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

int TinyDictWord::compare( const char * str )
{
    return strcmp( word, str );
}

/// factory - reading from index file
TinyDictWord * TinyDictWord::read( FILE * f, unsigned index )
{
    if ( !f || feof(f) )
        return NULL;
    char buf[1024];
    unsigned indexpos = ftell( f );
    if ( !fgets( buf, 1023, f ) )
        return NULL;
    int sz = 0;
    int tabc = 0;
    int tabs[2];
    while ( buf[sz] && buf[sz]!='\n' && buf[sz]!='\r' ) {
        if ( buf[sz] == '\t' && tabc<2 )
            tabs[tabc++] = sz;
        sz++;
    }
    buf[sz] = 0;
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

bool TinyDictWord::match( const char * str, bool exact )
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
        else if ( res > 0 ) {
            delete p;
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
    f = fopen( fname, "rt" );
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

#if 1
int main( int argc, const char * * argv )
{
    TinyDictIndexFile index;
    if ( !index.open("mueller7.index") ) {
        printf("cannot open index file");
        return -1;
    }
    TinyDictWordList words;
    const char * pattern = "full";
    index.find( pattern, false, words );
    printf( "%d words matched pattern %s\n", words.length(), pattern );
    for ( int i=0; i<words.length(); i++ ) {
        TinyDictWord * p = words.get(i);
        printf("%s %d %d\n", p->getWord(), p->getStart(), p->getSize() );
    }
    return 0;
}
#endif
