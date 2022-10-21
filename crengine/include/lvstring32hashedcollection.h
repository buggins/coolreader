/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
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
