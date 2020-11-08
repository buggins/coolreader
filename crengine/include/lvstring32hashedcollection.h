/** \file lvstring16hashedcollection.h
    \brief hashed wide string collection

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRING32HASHEDCOLLECTION_H_INCLUDED__
#define __LV_STRING32HASHEDCOLLECTION_H_INCLUDED__

#include "lvstring32collection.h"

class SerialBuf;

/// hashed wide string collection
class lString32HashedCollection : public lString32Collection
{
private:
    int hashSize;
    struct HashPair {
        int index;
        HashPair * next;
        void clear() { index=-1; next=NULL; }
    };
    HashPair * hash;
    void addHashItem( int hashIndex, int storageIndex );
    void clearHash();
    void reHash( int newSize );
public:

    /// serialize to byte array (pointer will be incremented by number of bytes written)
    void serialize( SerialBuf & buf );
    /// deserialize from byte array (pointer will be incremented by number of bytes read)
    bool deserialize( SerialBuf & buf );

    lString32HashedCollection( lString32HashedCollection & v );
    lString32HashedCollection( lUInt32 hashSize );
    ~lString32HashedCollection();
    int add( const lChar32 * s );
    int find( const lChar32 * s );
};

#endif  // __LV_STRING32HASHEDCOLLECTION_H_INCLUDED__
