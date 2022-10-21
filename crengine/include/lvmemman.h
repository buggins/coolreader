/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2011-2013 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2018 poire-z <poire-z@users.noreply.github.com>         *
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
 * \file lvmemman.h
 * \brief Fast memory manager implementation
 */

#ifndef __LV_MEM_MAN_H_INCLUDED__
#define __LV_MEM_MAN_H_INCLUDED__


#include "crsetup.h"
#include "lvtypes.h"

#define CR_FATAL_ERROR_UNKNOWN             -1
#define CR_FATAL_ERROR_INDEX_OUT_OF_BOUND   1

/// fatal error function type
typedef void (lv_FatalErrorHandler_t)(int errorCode, const char * errorText );

/// set file to remove of fatal error - for removing of book cache file which caused crash
void crSetFileToRemoveOnFatalError(const char * filename);

/// set signal handler to do some cleanup actions (e.g. delete current cache file since it might be corrupted)
void crSetSignalHandler();

/// fatal error function calls fatal error handler
void crFatalError( int code, const char * errorText );
inline void crFatalError() { crFatalError( -1, "Unknown fatal error" ); }

/// set fatal error handler
void crSetFatalErrorHandler( lv_FatalErrorHandler_t * handler );

/// typed realloc with result check (size is counted in T), fatal error if failed
template <typename T> inline T * cr_realloc( T * ptr, size_t newSize ) {
    T * newptr = reinterpret_cast<T*>(realloc(ptr, sizeof(T)*newSize));
    if ( newptr )
        return newptr;
    free(ptr); // to bypass cppcheck warning
    crFatalError(-2, "realloc failed");
    return NULL;
}


#if (LDOM_USE_OWN_MEM_MAN==1)
#include <stdlib.h>

#define THROW_MEM_MAN_EXCEPTION crFatalError(-1, "Memory manager fatal error" );

#define BLOCK_SIZE_GRANULARITY 2
#define LOCAL_STORAGE_COUNT    16
#define FIRST_SLICE_SIZE       16
#define MAX_SLICE_COUNT        24
#define MAX_LOCAL_BLOCK_SIZE   ((1<<BLOCK_SIZE_GRANULARITY)*LOCAL_STORAGE_COUNT)

/// memory block
union ldomMemBlock {
//    struct {
        char buf[4];
//    };
//    struct {
        ldomMemBlock * nextfree;
//    };
};

/// memory allocation slice
struct ldomMemSlice {
    ldomMemBlock * pBlocks; // first block
    ldomMemBlock * pEnd;    // first free byte after last block
    ldomMemBlock * pFree;   // first free block
    size_t block_size;      // size of block
    size_t block_count;     // count of blocks
    size_t blocks_used;     // number of used blocks
    //
    inline ldomMemBlock * blockByIndex( size_t index )
    {
        return (ldomMemBlock *) ( (char*)pBlocks + (block_size*index) );
    }
    inline ldomMemBlock * nextBlock( ldomMemBlock * p )
    {
        return (ldomMemBlock *) ( (char*)p + (block_size) );
    }
    inline ldomMemBlock * prevBlock( ldomMemBlock * p )
    {
        return (ldomMemBlock *) ( (char*)p - (block_size) );
    }
    //
    ldomMemSlice( size_t blockSize, size_t blockCount )
    :   block_size(blockSize),
        block_count(blockCount),
        blocks_used(0)
    {

        pBlocks = (ldomMemBlock *) malloc(block_size * block_count);
        pEnd = blockByIndex(block_count);
        pFree = pBlocks;
        for (ldomMemBlock * p = pBlocks; p<pEnd; )
        {
            p = p->nextfree = nextBlock(p);
        }
        prevBlock(pEnd)->nextfree = NULL;
    }
    ~ldomMemSlice()
    {
        free( pBlocks );
    }
    inline ldomMemBlock * alloc_block()
    {
        ldomMemBlock * res = pFree;
        pFree = res->nextfree;
        ++blocks_used;
        return res;
    }
    inline bool free_block( ldomMemBlock * pBlock )
    {
        if (pBlock < pBlocks || pBlock >= pEnd)
            return false; // chunk does not belong to this slice
        pBlock->nextfree = pFree;
        pFree = pBlock;
        --blocks_used;
        return true;
    }
};

/// storage for blocks of specified size
struct ldomMemManStorage
{
    size_t block_size;      // size of block
    size_t slice_count;     // count of existing blocks
    ldomMemSlice * slices[MAX_SLICE_COUNT];
    //======================================
    ldomMemManStorage( size_t blockSize )
        : block_size(blockSize)
    {
        slice_count = 1;
        slices[0] = new ldomMemSlice(block_size, FIRST_SLICE_SIZE);
    }
    ~ldomMemManStorage()
    {
        for (size_t i=0; i<slice_count; i++)
            delete slices[i];
    }
    void * alloc()
    {
        // search for existing slice
        for (int i=slice_count-1; i>=0; --i)
        {
            if (slices[i]->pFree != NULL)
                return slices[i]->alloc_block();
        }
        // alloc new slice
        if (slice_count >= MAX_SLICE_COUNT)
            THROW_MEM_MAN_EXCEPTION;
        slices[slice_count] = 
            new ldomMemSlice(block_size, FIRST_SLICE_SIZE << (slice_count+1));
        return slices[slice_count++]->alloc_block();
    }
    void free( ldomMemBlock * pBlock )
    {
        for (int i=slice_count-1; i>=0; --i)
        {
            if (slices[i]->free_block(pBlock))
                return;
        }
        //throw; // wrong pointer!!!
    }
};

/// allocate memory
void * ldomAlloc( size_t n );
/// free memory
void   ldomFree( void * p, size_t n );


//////////////////////////////////////////////////////////////////////

/// declare allocator for class: use in class declarations
#define DECLARE_CLASS_ALLOCATOR(classname) \
    void * operator new( size_t size ); \
    void operator delete( void * p );


/// define allocator for class: use in class definitions
#define DEFINE_CLASS_ALLOCATOR(classname) \
\
static ldomMemManStorage * pms ## classname = NULL; \
\
void * classname::operator new( size_t size ) \
{ \
    if (pms ## classname == NULL) \
    { \
        pms ## classname = new ldomMemManStorage(sizeof(classname)); \
    } \
    return pms ## classname->alloc(); \
} \
\
void classname::operator delete( void * p ) \
{ \
    pms ## classname->free((ldomMemBlock *)p); \
}

void ldomFreeStorage();

#endif

#endif //__LV_MEM_MAN_H_INCLUDED__
