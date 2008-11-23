/** \file lstridmap.h
    \brief Name <-> Id map

   CoolReader Engine DOM Tree 

   Implements mapping between Name and Id

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LSTR_ID_MAP_H__INCLUDED__
#define __LSTR_ID_MAP_H__INCLUDED__

#include "lvstring.h"
#include "stdio.h"

//===========================================
class LDOMNameIdMapItem 
{
public:
    /// id
    lUInt16    id;
    /// value
    lString16 value;
    /// custom data pointer
    const void * data;

    LDOMNameIdMapItem(lUInt16 _id, const lString16 & _value, const void * _data)
        : id(_id), value(_value), data(_data)
    {
    }
    /// copy constructor
    LDOMNameIdMapItem(LDOMNameIdMapItem & item)
        : id(item.id), value(item.value), data(item.data)
    {
    }
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

    void    Sort();
public:
    /// Main constructor
    LDOMNameIdMap( lUInt16 maxId );
    /// Copy constructor
    LDOMNameIdMap( LDOMNameIdMap & map );
    ~LDOMNameIdMap();
    
    void Clear();

    void AddItem( lUInt16 id, const lString16 & value, const void * data );

    const LDOMNameIdMapItem * findItem( lUInt16 id ) const
    {
       return m_by_id[id];
    }

    const LDOMNameIdMapItem * findItem( const lChar16 * name );
    const LDOMNameIdMapItem * findItem( const lChar8 * name );
    const LDOMNameIdMapItem * findItem( const lString16 & name ) { return findItem(name.c_str()); }

    inline lUInt16 idByName( const lChar16 * name )
    {
        const LDOMNameIdMapItem * item = findItem(name);
        return item?item->id:0;
    }

    inline lUInt16 idByName( const lChar8 * name )
    {
        const LDOMNameIdMapItem * item = findItem(name);
        return item?item->id:0;
    }

    inline const lString16 & nameById( lUInt16 id )
    { 
        if (id>=m_size)
            return lString16::empty_str;
        const LDOMNameIdMapItem * item = findItem(id);
        return item?item->value:lString16::empty_str;
    }

    inline const void * dataById( lUInt16 id )
    { 
        if (id>=m_size)
            return NULL;
        const LDOMNameIdMapItem * item = findItem(id);
        return item ? item->data : NULL;
    }

    // debug dump of all unknown entities
    void dumpUnknownItems( FILE * f, int start_id );
};

#endif
