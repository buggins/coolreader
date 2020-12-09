/*******************************************************

   CoolReader Engine

   lvstring16hashedcollection.cpp: hashed wide string collection

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstring32hashedcollection.h"
#include "../include/serialbuf.h"

static const char * str_hash_magic="STRS";

/// serialize to byte array (pointer will be incremented by number of bytes written)
void lString32HashedCollection::serialize( SerialBuf & buf )
{
    if ( buf.error() )
        return;
    int start = buf.pos();
    buf.putMagic( str_hash_magic );
    lUInt32 count = length();
    buf << count;
    for ( int i=0; i<length(); i++ )
    {
        buf << at(i);
    }
    buf.putCRC( buf.pos() - start );
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool lString32HashedCollection::deserialize( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    clear();
    int start = buf.pos();
    buf.putMagic( str_hash_magic );
    lInt32 count = 0;
    buf >> count;
    for ( int i=0; i<count; i++ ) {
        lString32 s;
        buf >> s;
        if ( buf.error() )
            break;
        add( s.c_str() );
    }
    buf.checkCRC( buf.pos() - start );
    return !buf.error();
}

lString32HashedCollection::lString32HashedCollection( lString32HashedCollection & v )
: lString32Collection( v )
, hashSize( v.hashSize )
, hash( NULL )
{
    hash = (HashPair *)malloc( sizeof(HashPair) * hashSize );
    for ( int i=0; i<hashSize; i++ ) {
        hash[i].clear();
        hash[i].index = v.hash[i].index;
        HashPair * next = v.hash[i].next;
        while ( next ) {
            addHashItem( i, next->index );
            next = next->next;
        }
    }
}

void lString32HashedCollection::addHashItem( int hashIndex, int storageIndex )
{
    if ( hash[ hashIndex ].index == -1 ) {
        hash[hashIndex].index = storageIndex;
    } else {
        HashPair * np = (HashPair *)malloc(sizeof(HashPair));
        np->index = storageIndex;
        np->next = hash[hashIndex].next;
        hash[hashIndex].next = np;
    }
}

void lString32HashedCollection::clearHash()
{
    if ( hash ) {
        for ( int i=0; i<hashSize; i++) {
            HashPair * p = hash[i].next;
            while ( p ) {
                HashPair * tmp = p->next;
                free( p );
                p = tmp;
            }
        }
        free( hash );
    }
    hash = NULL;
}

lString32HashedCollection::lString32HashedCollection( lUInt32 hash_size )
: hashSize(hash_size), hash(NULL)
{

    hash = (HashPair *)malloc( sizeof(HashPair) * hashSize );
    for ( int i=0; i<hashSize; i++ )
        hash[i].clear();
}

lString32HashedCollection::~lString32HashedCollection()
{
    clearHash();
}

int lString32HashedCollection::find( const lChar32 * s )
{
    if ( !hash || !length() )
        return -1;
    lUInt32 h = calcStringHash( s );
    lUInt32 n = h % hashSize;
    if ( hash[n].index!=-1 )
    {
        const lString32 & str = at( hash[n].index );
        if ( str == s )
            return hash[n].index;
        HashPair * p = hash[n].next;
        for ( ;p ;p = p->next ) {
            const lString32 & str = at( p->index );
            if ( str==s )
                return p->index;
        }
    }
    return -1;
}

void lString32HashedCollection::reHash( int newSize )
{
    if (hashSize == newSize)
        return;
    clearHash();
    hashSize = newSize;
    if (hashSize > 0) {
        hash = (HashPair *)malloc( sizeof(HashPair) * hashSize );
        for ( int i=0; i<hashSize; i++ )
            hash[i].clear();
    }
    for ( int i=0; i<length(); i++ ) {
        lUInt32 h = calcStringHash( at(i).c_str() );
        lUInt32 n = h % hashSize;
        addHashItem( n, i );
    }
}

int lString32HashedCollection::add( const lChar32 * s )
{
    if ( !hash || hashSize < length()*2 ) {
        int sz = 16;
        while ( sz<length() )
            sz <<= 1;
        sz <<= 1;
        reHash( sz );
    }
    lUInt32 h = calcStringHash( s );
    lUInt32 n = h % hashSize;
    if ( hash[n].index!=-1 )
    {
        const lString32 & str = at( hash[n].index );
        if ( str == s )
            return hash[n].index;
        HashPair * p = hash[n].next;
        for ( ;p ;p = p->next ) {
            const lString32 & str = at( p->index );
            if ( str==s )
                return p->index;
        }
    }
    lUInt32 i = lString32Collection::add( lString32(s) );
    addHashItem( n, i );
    return i;
}
