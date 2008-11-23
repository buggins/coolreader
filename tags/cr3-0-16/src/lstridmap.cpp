/*******************************************************

   CoolReader Engine DOM Tree 

   LDOMNodeIdMap.cpp:  Name to Id map

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lstridmap.h"
#include <string.h>

LDOMNameIdMap::LDOMNameIdMap(lUInt16 maxId)
{
    m_size = maxId+1;
    m_count = 0;
    m_by_id   = new LDOMNameIdMapItem * [m_size];
    memset( m_by_id, 0, sizeof(LDOMNameIdMapItem *)*m_size );  
    m_by_name = new LDOMNameIdMapItem * [m_size];
    memset( m_by_name, 0, sizeof(LDOMNameIdMapItem *)*m_size );  
    m_sorted = true;
}

/// Copy constructor
LDOMNameIdMap::LDOMNameIdMap( LDOMNameIdMap & map )
{
    m_size = map.m_size;
    m_count = map.m_count;
    m_by_id   = new LDOMNameIdMapItem * [m_size];
    int i;
    for ( i=0; i<m_size; i++ ) {
        if ( map.m_by_id[i] )
            m_by_id[i] = new LDOMNameIdMapItem( *map.m_by_id[i] );
        else
            m_by_id[i] = NULL;
    }
    m_by_name = new LDOMNameIdMapItem * [m_size];
    for ( i=0; i<m_size; i++ ) {
        if ( map.m_by_name[i] )
            m_by_name[i] = new LDOMNameIdMapItem( *map.m_by_name[i] );
        else
            m_by_name[i] = NULL;
    }
    m_sorted = map.m_sorted;
}

LDOMNameIdMap::~LDOMNameIdMap()
{
    Clear();
    delete[] m_by_name;
    delete[] m_by_id;
}

static int compare_items( const void * item1, const void * item2 )
{
    return (*((LDOMNameIdMapItem **)item1))->value.compare( (*((LDOMNameIdMapItem **)item2))->value );
}

void LDOMNameIdMap::Sort()
{
    if (m_count>1)
        qsort( m_by_name, m_count, sizeof(LDOMNameIdMapItem*), compare_items );
    m_sorted = true;
}

const LDOMNameIdMapItem * LDOMNameIdMap::findItem( const lChar16 * name )
{
    if (m_count==0 || !name || !*name)
        return NULL;
    if (!m_sorted)
        Sort();
    lUInt16 a, b, c;
    int r;
    a = 0;
    b = m_count;
    while (1)
    {
        c = (a + b)>>1;
        r = lStr_cmp( name, m_by_name[c]->value.c_str() );
        if (r == 0)
            return m_by_name[c]; // found
        if (b==a+1)
            return NULL; // not found
        if (r>0)
        {
            a = c;
        }
        else
        {
            b = c;
        }
    }
}

const LDOMNameIdMapItem * LDOMNameIdMap::findItem( const lChar8 * name )
{
    if (m_count==0 || !name || !*name)
        return NULL;
    if (!m_sorted)
        Sort();
    lUInt16 a, b, c;
    int r;
    a = 0;
    b = m_count;
    while (1)
    {
        c = (a + b)>>1;
        r = lStr_cmp( name, m_by_name[c]->value.c_str() );
        if (r == 0)
            return m_by_name[c]; // found
        if (b==a+1)
            return NULL; // not found
        if (r>0)
        {
            a = c;
        }
        else
        {
            b = c;
        }
    }
}

void LDOMNameIdMap::AddItem( lUInt16 id, const lString16 & value, const void * data )
{
    if (id==0)
        return;
    if (id>=m_size)
    {
        // reallocate storage
        lUInt16 newsize = id+1;
        m_by_id = (LDOMNameIdMapItem **)realloc( m_by_id, sizeof(LDOMNameIdMapItem *)*newsize );
        m_by_name = (LDOMNameIdMapItem **)realloc( m_by_name, sizeof(LDOMNameIdMapItem *)*newsize );
        for (lUInt16 i = m_size; i<newsize; i++)
        {
            m_by_id[i] = NULL;
            m_by_name[i] = NULL;
        }
        m_size = newsize;
    }
    if (m_by_id[id] != NULL)
        return; // elready exists
    LDOMNameIdMapItem * item = new LDOMNameIdMapItem( id, value, data );
    m_by_id[id] = item;
    m_by_name[m_count++] = item;
    m_sorted = false;
}


void LDOMNameIdMap::Clear()
{
    for (lUInt16 i = 0; i<m_count; i++)
    {
        if (m_by_name[i])
            delete m_by_name[i];
    }
    memset( m_by_id, 0, sizeof(LDOMNameIdMapItem *)*m_size);
    m_count = 0;
}

void LDOMNameIdMap::dumpUnknownItems( FILE * f, int start_id )
{
    for (int i=start_id; i<m_size; i++)
    {
        if (m_by_id[i] != NULL)
        {
            lString8 s8( m_by_id[i]->value.c_str() );
            fprintf( f, "%d %s\n", m_by_id[i]->id, s8.c_str() );
        }
    }
}

