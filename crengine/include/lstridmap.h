/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
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

/**
 * \file lstridmap.h
 * \brief Name <-> Id map
 *
 * CoolReader Engine DOM Tree
 *
 * Implements mapping between Name and Id
 */

#ifndef __LSTR_ID_MAP_H__INCLUDED__
#define __LSTR_ID_MAP_H__INCLUDED__

#include "lvstring.h"
#include <stdio.h>

struct css_elem_def_props_t;
class SerialBuf;

//===========================================
class LDOMNameIdMapItem 
{
    /// custom data pointer
    css_elem_def_props_t * data;
public:
    /// id
    lUInt16    id;
    /// value
    lString32 value;
	/// constructor
    LDOMNameIdMapItem(lUInt16 _id, const lString32 & _value, const css_elem_def_props_t * _data);
    /// copy constructor
    LDOMNameIdMapItem(LDOMNameIdMapItem & item);
	/// destructor
	~LDOMNameIdMapItem();

	const css_elem_def_props_t * getData() const { return data; }

	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serialize( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	static LDOMNameIdMapItem * deserialize( SerialBuf & buf );
};
//===========================================

//===========================================
class LDOMNameIdMap
{
private:
    LDOMNameIdMapItem * * m_by_id;
    LDOMNameIdMapItem * * m_by_name;
    lUInt16 m_count; // non-empty count
    lUInt16 m_size;  // max number of ids
    bool    m_sorted;
    bool    m_changed;

    void    Sort();
public:
    /// Main constructor
    LDOMNameIdMap( lUInt16 maxId );
    /// Copy constructor
    LDOMNameIdMap( LDOMNameIdMap & map );
    ~LDOMNameIdMap();

	/// serialize to byte array (pointer will be incremented by number of bytes written)
	void serialize( SerialBuf & buf );
	/// deserialize from byte array (pointer will be incremented by number of bytes read)
	bool deserialize( SerialBuf & buf );

    void Clear();

    void AddItem( lUInt16 id, const lString32 & value, const css_elem_def_props_t * data );

    void AddItem( LDOMNameIdMapItem * item );

    const LDOMNameIdMapItem * findItem( lUInt16 id ) const
    {
       return m_by_id[id];
    }

    const LDOMNameIdMapItem * findItem( const lChar32 * name );
    const LDOMNameIdMapItem * findItem( const lChar8 * name );
    const LDOMNameIdMapItem * findItem( const lString32 & name ) { return findItem(name.c_str()); }

    inline lUInt16 idByName( const lChar32 * name )
    {
        const LDOMNameIdMapItem * item = findItem(name);
        return item?item->id:0;
    }

    inline lUInt16 idByName( const lChar8 * name )
    {
        const LDOMNameIdMapItem * item = findItem(name);
        return item?item->id:0;
    }

    inline const lString32 & nameById( lUInt16 id )
    { 
        if (id>=m_size)
            return lString32::empty_str;
        const LDOMNameIdMapItem * item = findItem(id);
        return item?item->value:lString32::empty_str;
    }

    inline const css_elem_def_props_t * dataById( lUInt16 id )
    { 
        if (id>=m_size)
            return NULL;
        const LDOMNameIdMapItem * item = findItem(id);
        return item ? item->getData() : NULL;
    }

    // debug dump of all unknown entities
    void dumpUnknownItems( FILE * f, int start_id );
    lString32 getUnknownItems( int start_id );
};

#endif
