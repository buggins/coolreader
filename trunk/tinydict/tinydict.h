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
    unsigned getIndexPos() { return indexpos; }
    unsigned getIndex() { return index; }
    unsigned getStart() { return start; }
    unsigned getSize() { return size; }
    const char * getWord() { return word; }
    int compare( const char * str );
    bool match( const char * str, bool exact );
    ~TinyDictWord() { if ( word ) free( word ); }
};

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

class TinyDictIndexFile
{
private:
    char * fname;
    FILE * f;
    size_t size;
    int    factor;
    int    count;
    TinyDictWordList list;

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

    bool find( const char * prefix, bool exactMatch, TinyDictWordList & words );

    TinyDictIndexFile() : fname(NULL), f(NULL), size(0), factor( 16 ), count(0)
    {
    }

    ~TinyDictIndexFile()
    {
        close();
        setFilename( NULL );
    }

    bool open( const char * filename );

    void close()
    {
        if (f)
            fclose(f);
        f = NULL;
        size = 0;
    }
};

#endif //TINYDICT_H_INCLUDED
