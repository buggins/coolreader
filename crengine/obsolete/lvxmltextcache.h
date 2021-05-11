/** @file lvxmltextcache.h
    @brief document text cache

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVXMLTEXTCACHE_H_INCLUDED__
#define __LVXMLTEXTCACHE_H_INCLUDED__

#include "lvtextfilebase.h"

/** @brief document text cache
    To read fragments of document text on demand.
*/
class LVXMLTextCache : public LVTextFileBase
{
private:
    struct cache_item
    {
        cache_item * next;
        lUInt32      pos;
        lUInt32      size;
        lUInt32      flags;
        lString32    text;
        cache_item( lString32 & txt )
            : next(NULL), pos(0), size(0), flags(0), text(txt)
        {
        }
    };

    cache_item * m_head;
    lUInt32    m_max_itemcount;
    lUInt32    m_max_charcount;

    void cleanOldItems( lUInt32 newItemChars );

    /// adds new item
    void addItem( lString32 & str );

public:
    /// returns true if format is recognized by parser
    virtual bool CheckFormat() {
        return true;
    }
    /// parses input stream
    virtual bool Parse() {
        return true;
    }
    /// constructor
    LVXMLTextCache( LVStreamRef stream, lUInt32 max_itemcount, lUInt32 max_charcount )
        : LVTextFileBase( stream ), m_head(NULL)
        , m_max_itemcount(max_itemcount)
        , m_max_charcount(max_charcount)
    {
    }
    /// destructor
    virtual ~LVXMLTextCache();
    /// reads text from cache or input stream
    lString32 getText( lUInt32 pos, lUInt32 size, lUInt32 flags );
};

#endif  // __LVXMLTEXTCACHE_H_INCLUDED__
