/*******************************************************

   CoolReader Engine

   lvrefcache.h:  Referenced objects cache
      allows to reuse objects with the same concents

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#if !defined(__LV_REF_CACHE_H_INCLUDED__)
#define __LV_REF_CACHE_H_INCLUDED__

#include "lvref.h"

/*
    Object cache

    Requirements: 
       sz parameter of constructor should be power of 2
       bool operator == (LVRef<T> & r1, LVRef<T> & r2 ) should be defined
       lUInt32 calcHash( LVRef<T> & r1 ) should be defined
*/

template <class ref_t> 
class LVRefCache {

    class LVRefCacheRec {
        ref_t style;
        lUInt32 hash;
        LVRefCacheRec * next;
        LVRefCacheRec(ref_t & s, lUInt32 h)
            : style(s), hash(h), next(NULL) { }
        friend class LVRefCache< ref_t >;
    };

private:
    int size;
    LVRefCacheRec ** table;

public:
    // check whether equal object already exists if cache
    // if found, replace reference with cached value
    void cacheIt(ref_t & style)
    {
        lUInt32 hash = calcHash( style );
        lUInt32 index = hash & (size - 1);
        LVRefCacheRec **rr;
        rr = &table[index];
        while ( *rr != NULL )
        {
            if ( *(*rr)->style.get() == *style.get() )
            {
                style = (*rr)->style;
                return;
            }
            rr = &(*rr)->next;
        }
        *rr = new LVRefCacheRec( style, hash );
    }
    // garbage collector: remove unused entries
    void gc()
    {
        for (int index = 0; index < size; index++)
        {
            LVRefCacheRec **rr;
            rr = &table[index];
            while ( *rr != NULL )
            {
                if ( (*rr)->style.getRefCount() == 1 )
                {
                    LVRefCacheRec * r = (*rr);
                    *rr = r->next;
                    delete r;
                }
                else
                {
                    rr = &(*rr)->next;
                }
            }
        }
    }
    LVRefCache( int sz )
    {
        size = sz;
        table = new LVRefCacheRec * [ sz ];
        for( int i=0; i<sz; i++ )
            table[i] = NULL;
    }
    ~LVRefCache()
    {
        LVRefCacheRec *r, *r2;
        for ( int i=0; i < size; i++ )
        {
            for ( r = table[ i ]; r;  )
            {
                r2 = r;
                r = r->next;
                delete r2;
            }
        }
        delete[] table;
    }
};

template <typename keyT, class dataT> class LVCacheMap
{
private:
    class Pair {
    public: 
        keyT key;
        dataT data;
        int lastAccess;
    };
    Pair * buf;
    int size;
    int lastAccess;
    void checkOverflow( int oldestAccessTime )
    {
        int i;
        if ( oldestAccessTime==-1 ) {
            for ( i=0; i<size; i++ )
                if ( oldestAccessTime==-1 || buf[i].lastAccess>oldestAccessTime )
                    oldestAccessTime = buf[i].lastAccess;
        }
        if ( oldestAccessTime>1000000000 ) {
            int maxLastAccess = 0;
            for ( i=0; i<size; i++ ) {
                buf[i].lastAccess -= 1000000000;
                if ( maxLastAccess==0 || buf[i].lastAccess>maxLastAccess )
                    maxLastAccess = buf[i].lastAccess;
            }
            lastAccess = maxLastAccess+1;
        }
    }
public:
    LVCacheMap( int maxSize )
    : size(maxSize), lastAccess(1)
    {
        buf = new Pair[ size ];
        clear();
    }
    void clear()
    {
        for ( int i=0; i<size; i++ )
        {
            buf[i].key = keyT();
            buf[i].data = dataT();
            buf[i].lastAccess = 0;
        }
    }
    bool get( keyT key, dataT & data )
    {
        for ( int i=0; i<size; i++ ) {
            if ( buf[i].key == key ) {
                data = buf[i].data;
                buf[i].lastAccess = ++lastAccess;
                if ( lastAccess>1000000000 )
                    checkOverflow(-1);
                return true;
            }
        }
        return false;
    }
    bool remove( keyT key )
    {
        for ( int i=0; i<size; i++ ) {
            if ( buf[i].key == key ) {
                buf[i].key = keyT();
                buf[i].data = dataT();
                buf[i].lastAccess = 0;
                return true;
            }
        }
        return false;
    }
    void set( keyT key, dataT data )
    {
        int oldestAccessTime = -1;
        int oldestIndex = 0;
        for ( int i=0; i<size; i++ ) {
            if ( buf[i].key == key ) {
                buf[i].data = data;
                buf[i].lastAccess = ++lastAccess;
                return;
            }
            int at = buf[i].lastAccess;
            if ( at < oldestAccessTime || oldestAccessTime==-1 ) {
                oldestAccessTime = at;
                oldestIndex = i;
            }
        }
        checkOverflow(oldestAccessTime);
        buf[oldestIndex].key = key;
        buf[oldestIndex].data = data;
        buf[oldestIndex].lastAccess = ++lastAccess;
        return;
    }
    ~LVCacheMap()
    {
        delete[] buf;
    }
};

#endif // __LV_REF_CACHE_H_INCLUDED__
