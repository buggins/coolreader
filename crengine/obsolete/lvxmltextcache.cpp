/*******************************************************

   CoolReader Engine

   lvxmltextcache.cpp: document text cache

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvxmltextcache.h"
#include "lvxml.h"

LVXMLTextCache::~LVXMLTextCache()
{
    while (m_head)
    {
        cache_item * ptr = m_head;
        m_head = m_head->next;
        delete ptr;
    }
}

void LVXMLTextCache::addItem( lString32 & str )
{
    cleanOldItems( str.length() );
    cache_item * ptr = new cache_item( str );
    ptr->next = m_head;
    m_head = ptr;
}

void LVXMLTextCache::cleanOldItems( lUInt32 newItemChars )
{
    lUInt32 sum_chars = newItemChars;
    cache_item * ptr = m_head, * prevptr = NULL;
    for ( lUInt32 n = 1; ptr; ptr = ptr->next, n++ )
    {
        sum_chars += ptr->text.length();
        if (sum_chars > m_max_charcount || n>=m_max_itemcount )
        {
            // remove tail
            for (cache_item * p = ptr; p; )
            {
                cache_item * tmp = p;
                p = p->next;
                delete tmp;
            }
            if (prevptr)
                prevptr->next = NULL;
            else
                m_head = NULL;
            return;
        }
        prevptr = ptr;
    }
}

lString32 LVXMLTextCache::getText( lUInt32 pos, lUInt32 size, lUInt32 flags )
{
    // TRY TO SEARCH IN CACHE
    cache_item * ptr = m_head, * prevptr = NULL;
    for ( ;ptr ;ptr = ptr->next )
    {
        if (ptr->pos == pos)
        {
            // move to top
            if (prevptr)
            {
                prevptr->next = ptr->next;
                ptr->next = m_head;
                m_head = ptr;
            }
            return ptr->text;
        }
    }
    // NO CACHE RECORD FOUND
    // read new pme
    lString32 text;
    text.reserve(size);
    text.append(size, ' ');
    lChar32 * buf = text.modify();
    unsigned chcount = (unsigned)ReadTextBytes( pos, size, buf, size, flags );
    //CRLog::debug("ReadTextBytes(%d,%d) done - %d chars read", (int)pos, (int)size, (int)chcount);
    text.limit( chcount );
    PreProcessXmlString( text, flags );
    if ( (flags & TXTFLG_TRIM) && (!(flags & TXTFLG_PRE) || (flags & TXTFLG_PRE_PARA_SPLITTING)) ) {
        text.trimDoubleSpaces(
            (flags & TXTFLG_TRIM_ALLOW_START_SPACE)?true:false,
            (flags & TXTFLG_TRIM_ALLOW_END_SPACE)?true:false,
            (flags & TXTFLG_TRIM_REMOVE_EOL_HYPHENS)?true:false );
    }
    // ADD TEXT TO CACHE
    addItem( text );
    m_head->pos = pos;
    m_head->size = size;
    m_head->flags = flags;
    return m_head->text;
}
