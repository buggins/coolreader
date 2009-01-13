/** \file tinydict.h
    \brief .dict dictionary file support interface

    Lightweight implementation of .dict support, written from scratch.

    (c) Vadim Lopatin, 2009

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.



	usage: 

		init:
			// create TinyDictionaryList object
			TinyDictionaryList dicts;
			// register dictionaries using 
			dicts.add( "/dir1/dict1.index", "/dir1/dict1.dict.dz" );
			dicts.add( "/dir1/dict2.index", "/dir1/dict2.dict.dz" );

	    search:
			// container for results
			TinyDictResultList results;
		    dicts.find(results, "word", 0 ); // find exact match

		process results:
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
*/

#ifndef TINYDICT_H_INCLUDED
#define TINYDICT_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

/// dictinary data file forward declaration
class TinyDictDataFile;
/// dictionary index file forward declaration
class TinyDictIndexFile;
/// dictonary forward declaration
class TinyDictionary;

/// Word entry of index file
class TinyDictWord
{
    unsigned index;
    unsigned indexpos;
    unsigned start;
    unsigned size;
    char * word;
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
	TinyDictionary * dict;
    TinyDictWord ** list;
    int size;
    int count;
public:

	// article access functions
	/// set dictonary pointer list belongs to
	void setDict( TinyDictionary * p ) { dict = p; }
	/// returns word list's dictionary name
	const char * getDictionaryName();
	/// returns article for word by index
	const char * getArticle( int index );

	// search functions
	/// searches list position by prefix
    int find( const char * prefix );

	// word list functions
	/// returns number of words in list
    int length() { return count; }
	/// get item by index
    TinyDictWord * get( int index ) { return list[index]; }
	/// add word to list
    void add( TinyDictWord * word );
	/// clear list
    void clear();

	/// empty list constructor
    TinyDictWordList();
	/// destructor
    ~TinyDictWordList();
};

/// default mode: exact match
#define TINY_DICT_OPTION_EXACT_MATCH 0
/// search for words starting with specified pattern
#define TINY_DICT_OPTION_STARTS_WITH 1

class TinyDictionary
{
	char * name;
	TinyDictDataFile * data;
	TinyDictIndexFile * index;
public:
	/// searches dictionary for specified word, caller is responsible for deleting of returned object
    TinyDictWordList * find( const char * prefix, int options = 0 );
	/// returns short dictionary name
	const char * getDictionaryName();
	/// get dictionary data pointer
	TinyDictDataFile * getData() { return data; }
	/// get dictionary index pointer
	//TinyDictIndexFile * getIndex() { return index; }
	/// minimize memory usage
	void compact();
	/// open dictonary from files
	bool open( const char * indexfile, const char * datafile );
	/// empty dictinary constructor
	TinyDictionary();
	/// destructor
	~TinyDictionary();
};

/// dictionary search result list
class TinyDictResultList
{
    TinyDictWordList ** list;
    int size;
    int count;
public:

	// word list functions
	/// returns number of words in list
    int length() { return count; }
	/// get item by index
    TinyDictWordList * get( int index ) { return list[index]; }
	/// remove all dictionaries from list
	void clear();
	/// create empty list
	TinyDictResultList();
	/// destructor
	~TinyDictResultList();
	/// add item to list
	void add( TinyDictWordList * p );
};


/// dictionary list
class TinyDictionaryList
{
    TinyDictionary ** list;
    int size;
    int count;
public:
	/// search all dictionaries in list for specified pattern
	bool find( TinyDictResultList & result, const char * prefix, int options = 0 );

	// word list functions
	/// returns number of words in list
    int length() { return count; }
	/// get item by index
    TinyDictionary * get( int index ) { return list[index]; }
	/// remove all dictionaries from list
	void clear();
	/// create empty list
	TinyDictionaryList();
	/// destructor
	~TinyDictionaryList();
	/// try to open dictionary and add it to list
	bool add( const char * indexfile, const char * datafile );
};

#endif //TINYDICT_H_INCLUDED
