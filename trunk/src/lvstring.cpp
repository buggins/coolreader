/*******************************************************

   CoolReader Engine

   lvstring.cpp:  string classes implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstring.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>



#if !defined(__SYMBIAN32__) && defined(_WIN32)
#include <windows.h>
#endif

#define LS_DEBUG_CHECK

// memory allocation slice
struct lstring_chunk_slice_t {
    lstring_chunk_t * pChunks; // first chunk
    lstring_chunk_t * pEnd;    // first free byte after last chunk
    lstring_chunk_t * pFree;   // first free chunk
    int used;
    lstring_chunk_slice_t( int size )
    {
        pChunks = (lstring_chunk_t *) malloc(sizeof(lstring_chunk_t) * size);
        pEnd = pChunks + size;
        pFree = pChunks;
        for (lstring_chunk_t * p = pChunks; p<pEnd; ++p)
        {
            p->nextfree = p+1;
            p->size = 0;
        }
        (pEnd-1)->nextfree = NULL;
    }
    ~lstring_chunk_slice_t()
    {
        free( pChunks );
    }
    inline lstring_chunk_t * alloc_chunk()
    {
        lstring_chunk_t * res = pFree;
        pFree = res->nextfree;
        return res;
    }
    inline bool free_chunk( lstring_chunk_t * pChunk )
    {
        if (pChunk < pChunks || pChunk >= pEnd)
            return false; // chunk does not belong to this slice
/*
#ifdef LS_DEBUG_CHECK
        if (!pChunk->size)
        {
            crFatalError(); // already freed!!!
        }
        pChunk->size = 0;
#endif
*/
        pChunk->nextfree = pFree;
        pFree = pChunk;
        return true;
    }
};

//#define FIRST_SLICE_SIZE 256
//#define MAX_SLICE_COUNT  20

static lstring_chunk_slice_t * slices[MAX_SLICE_COUNT];
static int slices_count = 0;
static bool slices_initialized = false;

static lChar8 empty_str_8[] = {0};
static lstring_chunk_t empty_chunk_8(empty_str_8);
lstring_chunk_t * lString8::EMPTY_STR_8 = &empty_chunk_8;

static lChar16 empty_str_16[] = {0};
static lstring_chunk_t empty_chunk_16(empty_str_16);
lstring_chunk_t * lString16::EMPTY_STR_16 = &empty_chunk_16;

static void init_ls_storage()
{
    slices[0] = new lstring_chunk_slice_t( FIRST_SLICE_SIZE );
    slices_count = 1;
    slices_initialized = true;
}

void free_ls_storage()
{
    if (!slices_initialized)
        return;
    for (int i=0; i<slices_count; i++)
    {
        delete slices[i];
    }
    slices_count = 0;
    slices_initialized = false;
}

lstring_chunk_t * lstring_chunk_t::alloc()
{
    if (!slices_initialized)
        init_ls_storage();
    // search for existing slice
    for (int i=slices_count-1; i>=0; --i)
    {
        if (slices[i]->pFree != NULL)
            return slices[i]->alloc_chunk();
    }
    // alloc new slice
    if (slices_count >= MAX_SLICE_COUNT)
        crFatalError();
    slices[slices_count++] = new lstring_chunk_slice_t( FIRST_SLICE_SIZE << (slices_count+1) );
    return slices[slices_count-1]->alloc_chunk();
}

void lstring_chunk_t::free( lstring_chunk_t * pChunk )
{
    for (int i=slices_count-1; i>=0; --i)
    {
        if (slices[i]->free_chunk(pChunk))
            return;
    }
    crFatalError(); // wrong pointer!!!
}

////////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////////

inline size_t _lStr_len(const lChar16 * str)
{
    size_t len;
    for (len=0; *str; str++)
        len++;
    return len;
}

inline size_t _lStr_len(const lChar8 * str)
{
    size_t len;
    for (len=0; *str; str++)
        len++;
    return len;
}

inline size_t _lStr_nlen(const lChar16 * str, size_t maxcount)
{
    size_t len;
    for (len=0; *str && len<maxcount; str++)
        len++;
    return len;
}

inline size_t _lStr_nlen(const lChar8 * str, size_t maxcount)
{
    size_t len;
    for (len=0; *str && len<maxcount; str++)
        len++;
    return len;
}

inline size_t _lStr_cpy(lChar16 * dst, const lChar16 * src)
{
    size_t count;
    for ( count=0; (*dst++ = *src++); count++ )
        ;
    return count;
}

inline size_t _lStr_cpy(lChar8 * dst, const lChar8 * src)
{
    size_t count;
    for ( count=0; (*dst++ = *src++); count++ )
        ;
    return count;
}

inline size_t _lStr_cpy(lChar16 * dst, const lChar8 * src)
{
    size_t count;
    for ( count=0; (*dst++ = *src++); count++ )
        ;
    return count;
}

inline size_t _lStr_cpy(lChar8 * dst, const lChar16 * src)
{
    size_t count;
    for ( count=0; (*dst++ = (lChar8)*src++); count++ )
        ;
    return count;
}

inline size_t _lStr_ncpy(lChar16 * dst, const lChar16 * src, size_t maxcount)
{
    size_t count = 0;
    do
    {
        if (++count > maxcount)
        {
            *dst = 0;
            return count;
        }
    } while ((*dst++ = *src++));
    return count;
}

inline size_t _lStr_ncpy(lChar8 * dst, const lChar8 * src, size_t maxcount)
{
    size_t count = 0;
    do
    {
        if (++count > maxcount)
        {
            *dst = 0;
            return count;
        }
    } while ((*dst++ = *src++));
    return count;
}

inline void _lStr_memcpy(lChar16 * dst, const lChar16 * src, size_t count)
{
    while ( count-- > 0)
        (*dst++ = *src++);
}

inline void _lStr_memcpy(lChar8 * dst, const lChar8 * src, size_t count)
{
    while ( count-- > 0)
        (*dst++ = *src++);
}

inline void _lStr_memset(lChar16 * dst, lChar16 value, size_t count)
{
    while ( count-- > 0)
        *dst++ = value;
}

inline void _lStr_memset(lChar8 * dst, lChar8 value, size_t count)
{
    while ( count-- > 0)
        *dst++ = value;
}

size_t lStr_len(const lChar16 * str)
{
    return _lStr_len(str);
}

size_t lStr_len(const lChar8 * str)
{
    return _lStr_len(str);
}

size_t lStr_nlen(const lChar16 * str, size_t maxcount)
{
    return _lStr_nlen(str, maxcount);
}

size_t lStr_nlen(const lChar8 * str, size_t maxcount)
{
    return _lStr_nlen(str, maxcount);
}

size_t lStr_cpy(lChar16 * dst, const lChar16 * src)
{
    return _lStr_cpy(dst, src);
}

size_t lStr_cpy(lChar8 * dst, const lChar8 * src)
{
    return _lStr_cpy(dst, src);
}

size_t lStr_cpy(lChar16 * dst, const lChar8 * src)
{
    return _lStr_cpy(dst, src);
}

size_t lStr_ncpy(lChar16 * dst, const lChar16 * src, size_t maxcount)
{
    return _lStr_ncpy(dst, src, maxcount);
}

size_t lStr_ncpy(lChar8 * dst, const lChar8 * src, size_t maxcount)
{
    return _lStr_ncpy(dst, src, maxcount);
}

void lStr_memcpy(lChar16 * dst, const lChar16 * src, size_t count)
{
    _lStr_memcpy(dst, src, count);
}

void lStr_memcpy(lChar8 * dst, const lChar8 * src, size_t count)
{
    _lStr_memcpy(dst, src, count);
}

void lStr_memset(lChar16 * dst, lChar16 value, size_t count)
{
    _lStr_memset(dst, value, count);
}

void lStr_memset(lChar8 * dst, lChar8 value, size_t count)
{
    _lStr_memset(dst, value, count);
}

int lStr_cmp(const lChar16 * dst, const lChar16 * src)
{
    while ( *dst == *src)
    {
        if (! *dst )
            return 0;
        ++dst;
        ++src;
    }
    if ( *dst > *src )
        return 1;
    else
        return -1;
}

int lStr_cmp(const lChar8 * dst, const lChar8 * src)
{
    while ( *dst == *src)
    {
        if (! *dst )
            return 0;
        ++dst;
        ++src;
    }
    if ( *dst > *src )
        return 1;
    else
        return -1;
}

int lStr_cmp(const lChar16 * dst, const lChar8 * src)
{
    while ( *dst == (lChar16)*src)
    {
        if (! *dst )
            return 0;
        ++dst;
        ++src;
    }
    if ( *dst > (lChar16)*src )
        return 1;
    else
        return -1;
}

int lStr_cmp(const lChar8 * dst, const lChar16 * src)
{
    while ( (lChar16)*dst == *src)
    {
        if (! *dst )
            return 0;
        ++dst;
        ++src;
    }
    if ( (lChar16)*dst > *src )
        return 1;
    else
        return -1;
}

////////////////////////////////////////////////////////////////////////////
// lString16
////////////////////////////////////////////////////////////////////////////

void lString16::free()
{
    assert(pchunk->buf16[pchunk->len]==0);
    ::free(pchunk->buf16);
    for (int i=slices_count-1; i>=0; --i)
    {
        if (slices[i]->free_chunk(pchunk))
            return;
    }
    crFatalError(); // wrong pointer!!!
}

void lString16::alloc(size_t sz)
{
    pchunk = lstring_chunk_t::alloc();
    pchunk->buf16 = (lChar16*) ::malloc( sizeof(lChar16) * (sz+1) );
    assert( pchunk->buf16!=NULL );
    pchunk->size = sz;
    pchunk->nref = 1;
}

lString16::lString16(const lChar16 * str)
{
    if (!str || !(*str))
    {
        pchunk = EMPTY_STR_16; 
        addref();
        return;
    }
    size_type len = _lStr_len(str);
    alloc( len );
    pchunk->len = len;
    _lStr_cpy( pchunk->buf16, str );
}

lString16::lString16(const lChar8 * str)
{
    if (!str || !(*str))
    {
        pchunk = EMPTY_STR_16; 
        addref();
        return;
    }
    size_type len = _lStr_len(str);
    alloc( len );
    pchunk->len = len;
    _lStr_cpy( pchunk->buf16, str );
}

lString16::lString16(const value_type * str, size_type count)
{
    if ( !str || !(*str) || count<=0 )
    {
        pchunk = EMPTY_STR_16; addref();
    }
    else
    {
        size_type len = _lStr_nlen(str, count);
        alloc(len);
        _lStr_ncpy( pchunk->buf16, str, len );
        pchunk->len = len;
    }
}

lString16::lString16(const lString16 & str, size_type offset, size_type count)
{
    if ( count > str.length() - offset )
        count = str.length() - offset;
    if (count<=0 || offset<0)
    {
        pchunk = EMPTY_STR_16; addref();
    }
    else
    {
        alloc(count);
        _lStr_memcpy( pchunk->buf16, str.pchunk->buf16+offset, count );
        pchunk->buf16[count]=0;
        pchunk->len = count;
    }
}

lString16 & lString16::assign(const lChar16 * str)
{
    if (!str || !(*str))
    {
        clear();
    }
    else
    {
        size_type len = _lStr_len(str);
        if (pchunk->nref==1)
        { 
            if (pchunk->size<=len)
            {
                // resize is necessary
                pchunk->buf16 = (lChar16*) ::realloc( pchunk->buf16, sizeof(lChar16)*(len+1) );
                pchunk->size = len+1;
            }
        }
        else
        {
            release();
            alloc(len);
        }
        _lStr_cpy( pchunk->buf16, str );
        pchunk->len = len;
    }
    return *this;
}

lString16 & lString16::assign(const lChar16 * str, size_type count)
{
    if ( !str || !(*str) || count<=0 )
    {
        clear();
    }
    else
    {
        size_type len = _lStr_nlen(str, count);
        if (pchunk->nref==1)
        { 
            if (pchunk->size<=len)
            {
                // resize is necessary
                pchunk->buf16 = (lChar16*) ::realloc( pchunk->buf16, sizeof(lChar16)*(len+1) );
                pchunk->size = len+1;
            }
        }
        else
        {
            release();
            alloc(len);
        }
        _lStr_ncpy( pchunk->buf16, str, count );
        pchunk->len = len;
    }
    return *this;
}

lString16 & lString16::assign(const lString16 & str, size_type offset, size_type count)
{
    if ( count > str.length() - offset )
        count = str.length() - offset;
    if (count<=0 || offset<0)
    {
        clear();
    }
    else
    {
        if (pchunk==str.pchunk)
        {
            if (&str != this)
            {
                release();
                alloc(count);
            }
            if (offset>0)
            {
                _lStr_memcpy( pchunk->buf16, str.pchunk->buf16+offset, count );
            }
            pchunk->buf16[count]=0;
        }
        else
        {
            if (pchunk->nref==1)
            { 
                if (pchunk->size<=count)
                {
                    // resize is necessary
                    pchunk->buf16 = (lChar16*) ::realloc( pchunk->buf16, sizeof(lChar16)*(count+1) );
                    pchunk->size = count+1;
                }
            }
            else
            {
                release();
                alloc(count);
            }
            _lStr_memcpy( pchunk->buf16, str.pchunk->buf16+offset, count );
            pchunk->buf16[count]=0;
        }
        pchunk->len = count;
    }
    return *this;
}

lString16 & lString16::erase(size_type offset, size_type count)
{
    if ( count > length() - offset )
        count = length() - offset;
    if (count<=0 || offset<0)
    {
        clear();
    }
    else
    {
        size_type newlen = length()-count;
        if (pchunk->nref==1)
        {
            _lStr_memcpy( pchunk->buf16+offset, pchunk->buf16+offset+count, newlen-offset+1 );
        }
        else
        {
            lstring_chunk_t * poldchunk = pchunk;
            release();
            alloc( newlen );
            _lStr_memcpy( pchunk->buf16, poldchunk->buf16, offset );
            _lStr_memcpy( pchunk->buf16+offset, poldchunk->buf16+offset+count, newlen-offset+1 );
        }
        pchunk->len = newlen;
        pchunk->buf16[newlen]=0;
    }
    return *this;
}

void lString16::reserve(size_type n)
{
    if (pchunk->nref==1)
    {
        if (pchunk->size<=n)
        {
            pchunk->buf16 = (lChar16*) ::realloc( pchunk->buf16, sizeof(lChar16)*(n+1) );
            pchunk->size = n;
        }
    }
    else
    {
        lstring_chunk_t * poldchunk = pchunk;
        release();
        alloc( n );
        _lStr_memcpy( pchunk->buf16, poldchunk->buf16, poldchunk->len+1 );
        pchunk->len = poldchunk->len;
    }
}

void lString16::lock( size_type newsize )
{
    if (pchunk->nref>1)
    {
        lstring_chunk_t * poldchunk = pchunk;
        release();
        alloc( newsize );
        size_type len = newsize;
        if (len>poldchunk->len)
            len = poldchunk->len;
        _lStr_memcpy( pchunk->buf16, poldchunk->buf16, len );
        pchunk->buf16[len]=0;
        pchunk->len = len;
    }
}

// lock string, allocate buffer and reset length to 0
void lString16::reset( size_type size )
{
    if (pchunk->nref>1 || pchunk->size<size)
    {
        release();
        alloc( size );
    }
    pchunk->buf16[0] = 0;
    pchunk->len = 0;
}

void lString16::resize(size_type n, lChar16 e)
{
    lock( n );
    if (n>=pchunk->size)
    {
        pchunk->buf16 = (lChar16*) ::realloc( pchunk->buf16, sizeof(lChar16)*(n+1) );
        pchunk->size = n;
    }
    // fill with data if expanded
    for (size_type i=pchunk->len; i<n; i++)
        pchunk->buf16[i] = e;
    pchunk->buf16[pchunk->len] = 0;
}

lString16 & lString16::append(const lChar16 * str)
{
    size_type len = _lStr_len(str);
    reserve( pchunk->len+len );
    _lStr_memcpy(pchunk->buf16+pchunk->len, str, len+1);
    pchunk->len += len;
    return *this;
}

lString16 & lString16::append(const lChar16 * str, size_type count)
{
    size_type len = _lStr_nlen(str, count);
    reserve( pchunk->len+len );
    _lStr_ncpy(pchunk->buf16+pchunk->len, str, len);
    pchunk->len += len;
    return *this;
}

lString16 & lString16::append(const lString16 & str)
{
    size_type len2 = pchunk->len + str.pchunk->len;
    reserve( len2 );
    _lStr_memcpy( pchunk->buf16+pchunk->len, str.pchunk->buf16, str.pchunk->len+1 );
    pchunk->len = len2;
    return *this;
}

lString16 & lString16::append(const lString16 & str, size_type offset, size_type count)
{
    if ( str.pchunk->len>offset )
    {
        if ( offset + count > str.pchunk->len )
            count = str.pchunk->len - offset;
        reserve( pchunk->len+count );
        _lStr_ncpy(pchunk->buf16 + pchunk->len, str.pchunk->buf16 + offset, count);
        pchunk->len += count;
        pchunk->buf16[pchunk->len] = 0;
    }
    return *this;
}

lString16 & lString16::append(size_type count, lChar16 ch)
{
    reserve( pchunk->len+count );
    _lStr_memset(pchunk->buf16+pchunk->len, ch, count);
    pchunk->len += count;
    pchunk->buf16[pchunk->len] = 0;
    return *this;
}

lString16 & lString16::insert(size_type p0, size_type count, lChar16 ch)
{
    if (p0>pchunk->len)
        p0 = pchunk->len;
    reserve( pchunk->len+count );
    for (size_type i=pchunk->len+count; i>p0; i--)
        pchunk->buf16[i] = pchunk->buf16[i-1];
    _lStr_memset(pchunk->buf16+p0, ch, count);
    pchunk->len += count;
    pchunk->buf16[pchunk->len] = 0;
    return *this;
}

lString16 lString16::substr(size_type pos, size_type n) const
{
    if (pos>=length())
        return lString16();
    if (pos+n>length())
        n = length() - pos;
    return lString16( pchunk->buf16+pos, n );
}

lString16 & lString16::pack()
{
    if (pchunk->len + 4 < pchunk->size )
    {
        if (pchunk->nref>1)
        {
            lock(pchunk->len);
        }
        else
        {
            pchunk->buf16 = (lChar16 *) realloc( pchunk->buf16, sizeof(lChar16)*(pchunk->len+1) );
            pchunk->size = pchunk->len;
        }
    }
    return *this;
}

lString16 & lString16::trim()
{
    //
    size_t firstns;
    for (firstns = 0; firstns<pchunk->len && 
        (pchunk->buf16[firstns]==' ' || pchunk->buf16[firstns]=='\t'); ++firstns)
        ;
    if (firstns >= pchunk->len)
    {
        clear();
        return *this;
    }
    size_t lastns;
    for (lastns = pchunk->len-1; lastns>0 && 
        (pchunk->buf16[lastns]==' ' || pchunk->buf16[lastns]=='\t'); --lastns)
        ;
    size_t newlen = lastns-firstns+1;
    if (newlen == pchunk->len)
        return *this;
    if (pchunk->nref == 1)
    {
        if (firstns>0)
            lStr_memcpy( pchunk->buf16, pchunk->buf16+firstns, newlen );
        pchunk->buf16[newlen] = 0;
        pchunk->len = newlen;
    }
    else
    {
        lstring_chunk_t * poldchunk = pchunk;
        release();
        alloc( newlen );
        _lStr_memcpy( pchunk->buf16, poldchunk->buf16+firstns, newlen );
        pchunk->buf16[newlen] = 0;
        pchunk->len = newlen;
    }
    return *this;
}

int lString16::atoi() const
{
    int sgn = 1;
    int n = 0;
    const lChar16 * s = c_str();
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '-')
    {
        sgn = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }
    while (*s>='0' && *s<='9')
    {
        n = n * 10 + ( (*s++)-'0' );
    }
    return (sgn>0)?n:-n;
}

#define STRING_HASH_MULT 75317
lUInt32 lString16::getHash() const
{
    lUInt32 res = 0;
    for (lUInt32 i=0; i<pchunk->len; i++)
        res = res * STRING_HASH_MULT + pchunk->buf16[i];
    return res;
}



void lString16Collection::reserve( size_t space )
{
    if ( count + space > size )
    {
        size = count + space + 64;
        chunks = (lstring_chunk_t * *)realloc( chunks, sizeof(lstring_chunk_t *) * size );
    }
}
size_t lString16Collection::add( const lString16 & str )
{
    reserve( 1 );
    chunks[count] = str.pchunk;
    str.addref();
    return count++;
}
void lString16Collection::clear()
{
    for (size_t i=0; i<count; i++)
    {
        ((lString16 *)chunks)[i].release();
    }
    if (chunks)
        free(chunks);
    chunks = NULL;
    count = 0;
    size = 0;
}

void lString16Collection::erase(int offset, int cnt)
{
    if (count<=0)
        return;
    if (offset<0 || offset+cnt>=(int)count)
        return;
    int i;
    for (i=offset; i<offset+cnt; i++)
    {
        ((lString16 *)chunks)[i].release();
    }
    for (i=offset+cnt; i<(int)count; i++)
    {
        chunks[i-cnt] = chunks[i];
    }
    count -= cnt;
    if (!count)
        clear();    
}

void lString8Collection::erase(int offset, int cnt)
{
    if (count<=0)
        return;
    if (offset<0 || offset+cnt>(int)count)
        return;
    int i;
    for (i=offset; i<offset+cnt; i++)
    {
        ((lString8 *)chunks)[i].release();
    }
    for (i=offset+cnt; i<(int)count; i++)
    {
        chunks[i-cnt] = chunks[i];
    }
    count -= cnt;
    if (!count)
        clear();    
}

void lString8Collection::reserve( size_t space )
{
    if ( count + space > size )
    {
        size = count + space + 64;
        chunks = (lstring_chunk_t * *)realloc( chunks, sizeof(lstring_chunk_t *) * size );
    }
}
size_t lString8Collection::add( const lString8 & str )
{
    reserve( 1 );
    chunks[count] = str.pchunk;
    str.addref();
    return count++;
}
void lString8Collection::clear()
{
    for (size_t i=0; i<count; i++)
    {
        ((lString8 *)chunks)[i].release();
    }
    if (chunks)
        free(chunks);
    chunks = NULL;
    count = 0;
    size = 0;
}

lUInt32 calcStringHash( const lChar16 * s )
{
    lUInt32 a = 2166136261;
    while (*s)
    {
        a = a * 16777619 ^ (*s++);
    }
    return a;
}

void lString16HashedCollection::addHashItem( int hashIndex, int storageIndex )
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

void lString16HashedCollection::clearHash()
{
    if ( hash ) {
        for ( unsigned i=0; i<hashSize; i++) {
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

lString16HashedCollection::lString16HashedCollection( lUInt32 hash_size )
: hashSize(hash_size), hash(NULL)
{

    hash = (HashPair *)malloc( sizeof(HashPair) * hashSize );
    for ( unsigned i=0; i<hashSize; i++ )
        hash[i].clear();
}

lString16HashedCollection::~lString16HashedCollection()
{
    clearHash();
}

size_t lString16HashedCollection::find( const lChar16 * s )
{
    if ( !hash || !length() )
        return (size_t)-1;
    lUInt32 h = calcStringHash( s );
    lUInt32 n = h % hashSize;
    if ( hash[n].index!=-1 )
    {
        const lString16 & str = at( hash[n].index );
        if ( str == s )
            return hash[n].index;
        HashPair * p = hash[n].next;
        for ( ;p ;p = p->next ) {
            const lString16 & str = at( p->index );
            if ( str==s )
                return p->index;
        }
    }
    return (size_t)-1;
}

void lString16HashedCollection::reHash( int newSize )
{
    if ( hashSize == (lUInt32)newSize )
        return;
    clearHash();
    hashSize = newSize;
    if ( hashSize>0 ) {
        hash = (HashPair *)malloc( sizeof(HashPair) * hashSize );
        for ( unsigned i=0; i<hashSize; i++ )
            hash[i].clear();
    }
    for ( unsigned i=0; i<length(); i++ ) {
        lUInt32 h = calcStringHash( at(i).c_str() );
        lUInt32 n = h % hashSize;
        addHashItem( n, i );
    }
}

size_t lString16HashedCollection::add( const lChar16 * s )
{
    if ( !hash || hashSize < length()*2 ) {
        unsigned sz = 16;
        while ( sz<length() )
            sz <<= 1;
        sz <<= 1;
        reHash( sz );
    }
    lUInt32 h = calcStringHash( s );
    lUInt32 n = h % hashSize;
    if ( hash[n].index!=-1 )
    {
        const lString16 & str = at( hash[n].index );
        if ( str == s )
            return hash[n].index;
        HashPair * p = hash[n].next;
        for ( ;p ;p = p->next ) {
            const lString16 & str = at( p->index );
            if ( str==s )
                return p->index;
        }
    }
    lUInt32 i = lString16Collection::add( lString16(s) );
    addHashItem( n, i );
    return i;
}

const lString16 lString16::empty_str;


////////////////////////////////////////////////////////////////////////////
// lString8
////////////////////////////////////////////////////////////////////////////

void lString8::free()
{
    ::free(pchunk->buf8);
    for (int i=slices_count-1; i>=0; --i)
    {
        if (slices[i]->free_chunk(pchunk))
            return;
    }
    crFatalError(); // wrong pointer!!!
}

void lString8::alloc(size_t sz)
{
    pchunk = lstring_chunk_t::alloc();
    pchunk->buf8 = (lChar8*) ::malloc( sizeof(lChar8) * (sz+1) );
    assert( pchunk->buf8!=NULL );
    pchunk->size = sz;
    pchunk->nref = 1;
}

lString8::lString8(const lChar8 * str)
{
    if (!str || !(*str))
    {
        pchunk = EMPTY_STR_8; 
        addref();
        return;
    }
    size_type len = _lStr_len(str);
    alloc( len );
    pchunk->len = len;
    _lStr_cpy( pchunk->buf8, str );
}

lString8::lString8(const lChar16 * str)
{
    if (!str || !(*str))
    {
        pchunk = EMPTY_STR_8; 
        addref();
        return;
    }
    size_type len = _lStr_len(str);
    alloc( len );
    pchunk->len = len;
    _lStr_cpy( pchunk->buf8, str );
}

lString8::lString8(const value_type * str, size_type count)
{
    if ( !str || !(*str) || count<=0 )
    {
        pchunk = EMPTY_STR_8; addref();
    }
    else
    {
        size_type len = _lStr_nlen(str, count);
        alloc(len);
        _lStr_ncpy( pchunk->buf8, str, len );
        pchunk->len = len;
    }
}

lString8::lString8(const lString8 & str, size_type offset, size_type count)
{
    if ( count > str.length() - offset )
        count = str.length() - offset;
    if (count<=0 || offset<0)
    {
        pchunk = EMPTY_STR_8; addref();
    }
    else
    {
        alloc(count);
        _lStr_memcpy( pchunk->buf8, str.pchunk->buf8+offset, count );
        pchunk->buf8[count]=0;
        pchunk->len = count;
    }
}

lString8 & lString8::assign(const lChar8 * str)
{
    if (!str || !(*str))
    {
        clear();
    }
    else
    {
        size_type len = _lStr_len(str);
        if (pchunk->nref==1)
        { 
            if (pchunk->size<=len)
            {
                // resize is necessary
                pchunk->buf8 = (lChar8*) ::realloc( pchunk->buf8, sizeof(lChar8)*(len+1) );
                pchunk->size = len+1;
            }
        }
        else
        {
            release();
            alloc(len);
        }
        _lStr_cpy( pchunk->buf8, str );
        pchunk->len = len;
    }
    return *this;
}

lString8 & lString8::assign(const lChar8 * str, size_type count)
{
    if ( !str || !(*str) || count<=0 )
    {
        clear();
    }
    else
    {
        size_type len = _lStr_nlen(str, count);
        if (pchunk->nref==1)
        { 
            if (pchunk->size<=len)
            {
                // resize is necessary
                pchunk->buf8 = (lChar8*) ::realloc( pchunk->buf8, sizeof(lChar8)*(len+1) );
                pchunk->size = len+1;
            }
        }
        else
        {
            release();
            alloc(len);
        }
        _lStr_ncpy( pchunk->buf8, str, count );
        pchunk->len = len;
    }
    return *this;
}

lString8 & lString8::assign(const lString8 & str, size_type offset, size_type count)
{
    if ( count > str.length() - offset )
        count = str.length() - offset;
    if (count<=0 || offset<0)
    {
        clear();
    }
    else
    {
        if (pchunk==str.pchunk)
        {
            if (&str != this)
            {
                release();
                alloc(count);
            }
            if (offset>0)
            {
                _lStr_memcpy( pchunk->buf8, str.pchunk->buf8+offset, count );
            }
            pchunk->buf8[count]=0;
        }
        else
        {
            if (pchunk->nref==1)
            { 
                if (pchunk->size<=count)
                {
                    // resize is necessary
                    pchunk->buf8 = (lChar8*) ::realloc( pchunk->buf8, sizeof(lChar8)*(count+1) );
                    pchunk->size = count+1;
                }
            }
            else
            {
                release();
                alloc(count);
            }
            _lStr_memcpy( pchunk->buf8, str.pchunk->buf8+offset, count );
            pchunk->buf8[count]=0;
        }
        pchunk->len = count;
    }
    return *this;
}

lString8 & lString8::erase(size_type offset, size_type count)
{
    if ( count > length() - offset )
        count = length() - offset;
    if (count<=0 || offset<0)
    {
        clear();
    }
    else
    {
        size_type newlen = length()-count;
        if (pchunk->nref==1)
        {
            _lStr_memcpy( pchunk->buf8+offset, pchunk->buf8+offset+count, newlen-offset+1 );
        }
        else
        {
            lstring_chunk_t * poldchunk = pchunk;
            release();
            alloc( newlen );
            _lStr_memcpy( pchunk->buf8, poldchunk->buf8, offset );
            _lStr_memcpy( pchunk->buf8+offset, poldchunk->buf8+offset+count, newlen-offset+1 );
        }
        pchunk->len = newlen;
        pchunk->buf8[newlen]=0;
    }
    return *this;
}

void lString8::reserve(size_type n)
{
    if (pchunk->nref==1)
    {
        if (pchunk->size<=n)
        {
            pchunk->buf8 = (lChar8*) ::realloc( pchunk->buf8, sizeof(lChar8)*(n+1) );
            pchunk->size = n;
        }
    }
    else
    {
        lstring_chunk_t * poldchunk = pchunk;
        release();
        alloc( n );
        _lStr_memcpy( pchunk->buf8, poldchunk->buf8, poldchunk->len+1 );
        pchunk->len = poldchunk->len;
    }
}

void lString8::lock( size_type newsize )
{
    if (pchunk->nref>1)
    {
        lstring_chunk_t * poldchunk = pchunk;
        release();
        alloc( newsize );
        size_type len = newsize;
        if (len>poldchunk->len)
            len = poldchunk->len;
        _lStr_memcpy( pchunk->buf8, poldchunk->buf8, len );
        pchunk->buf8[len]=0;
        pchunk->len = len;
    }
}

// lock string, allocate buffer and reset length to 0
void lString8::reset( size_type size )
{
    if (pchunk->nref>1 || pchunk->size<size)
    {
        release();
        alloc( size );
    }
    pchunk->buf8[0] = 0;
    pchunk->len = 0;
}

void lString8::resize(size_type n, lChar8 e)
{
    lock( n );
    if (n>=pchunk->size)
    {
        pchunk->buf8 = (lChar8*) ::realloc( pchunk->buf8, sizeof(lChar8)*(n+1) );
        pchunk->size = n;
    }
    // fill with data if expanded
    for (size_type i=pchunk->len; i<n; i++)
        pchunk->buf8[i] = e;
    pchunk->buf8[pchunk->len] = 0;
}

lString8 & lString8::append(const lChar8 * str)
{
    size_type len = _lStr_len(str);
    reserve( pchunk->len+len );
    _lStr_memcpy(pchunk->buf8+pchunk->len, str, len+1);
    pchunk->len += len;
    return *this;
}

lString8 & lString8::append(const lChar8 * str, size_type count)
{
    size_type len = _lStr_nlen(str, count);
    reserve( pchunk->len+len );
    _lStr_ncpy(pchunk->buf8+pchunk->len, str, len);
    pchunk->len += len;
    return *this;
}

lString8 & lString8::append(const lString8 & str)
{
    size_type len2 = pchunk->len + str.pchunk->len;
    reserve( len2 );
    _lStr_memcpy( pchunk->buf8+pchunk->len, str.pchunk->buf8, str.pchunk->len+1 );
    pchunk->len = len2;
    return *this;
}

lString8 & lString8::append(const lString8 & str, size_type offset, size_type count)
{
    if ( str.pchunk->len>offset )
    {
        if ( offset + count > str.pchunk->len )
            count = str.pchunk->len - offset;
        reserve( pchunk->len+count );
        _lStr_ncpy(pchunk->buf8 + pchunk->len, str.pchunk->buf8 + offset, count);
        pchunk->len += count;
        pchunk->buf8[pchunk->len] = 0;
    }
    return *this;
}

lString8 & lString8::append(size_type count, lChar8 ch)
{
    reserve( pchunk->len+count );
    _lStr_memset(pchunk->buf8+pchunk->len, ch, count);
    pchunk->len += count;
    pchunk->buf8[pchunk->len] = 0;
    return *this;
}

lString8 & lString8::insert(size_type p0, size_type count, lChar8 ch)
{
    if (p0>pchunk->len)
        p0 = pchunk->len;
    reserve( pchunk->len+count );
    for (size_type i=pchunk->len+count; i>p0; i--)
        pchunk->buf8[i] = pchunk->buf8[i-1];
    _lStr_memset(pchunk->buf8+p0, ch, count);
    pchunk->len += count;
    pchunk->buf8[pchunk->len] = 0;
    return *this;
}

lString8 lString8::substr(size_type pos, size_type n) const
{
    if (pos>=length())
        return lString8();
    if (pos+n>length())
        n = length() - pos;
    return lString8( pchunk->buf8+pos, n );
}

int lString8::pos(lString8 subStr) const
{
    if (subStr.length()>length())
        return -1;
    int l = subStr.length();
    int dl = length() - l;
    for (int i=0; i<=dl; i++)
    {
        int flg = 1;
        for (int j=0; j<l; j++)
            if (pchunk->buf8[i+j]!=subStr.pchunk->buf8[j])
            {
                flg = 0;
                break;
            }
        if (flg)
            return i;
    }
    return -1;
}

int lString16::pos(lString16 subStr) const
{
    if (subStr.length()>length())
        return -1;
    int l = subStr.length();
    int dl = length() - l;
    for (int i=0; i<=dl; i++)
    {
        int flg = 1;
        for (int j=0; j<l; j++)
            if (pchunk->buf16[i+j]!=subStr.pchunk->buf16[j])
            {
                flg = 0;
                break;
            }
        if (flg)
            return i;
    }
    return -1;
}

lString8 & lString8::pack()
{
    if (pchunk->len + 4 < pchunk->size )
    {
        if (pchunk->nref>1)
        {
            lock(pchunk->len);
        }
        else
        {
            pchunk->buf8 = (lChar8 *) realloc( pchunk->buf8, sizeof(lChar8)*(pchunk->len+1) );
            pchunk->size = pchunk->len;
        }
    }
    return *this;
}

lString8 & lString8::trim()
{
    //
    size_t firstns;
    for (firstns = 0; 
            firstns<pchunk->len && 
            (pchunk->buf8[firstns]==' ' || 
            pchunk->buf8[firstns]=='\t'); 
            ++firstns)
        ;
    if (firstns >= pchunk->len)
    {
        clear();
        return *this;
    }
    size_t lastns;
    for (lastns = pchunk->len-1; 
            lastns>0 && 
            (pchunk->buf8[lastns]==' ' || pchunk->buf8[lastns]=='\t'); 
            --lastns)
        ;
    size_t newlen = lastns-firstns+1;
    if (newlen == pchunk->len)
        return *this;
    if (pchunk->nref == 1)
    {
        if (firstns>0)
            lStr_memcpy( pchunk->buf8, pchunk->buf8+firstns, newlen );
        pchunk->buf8[newlen] = 0;
        pchunk->len = newlen;
    }
    else
    {
        lstring_chunk_t * poldchunk = pchunk;
        release();
        alloc( newlen );
        _lStr_memcpy( pchunk->buf8, poldchunk->buf8+firstns, newlen );
        pchunk->buf8[newlen] = 0;
        pchunk->len = newlen;
    }
    return *this;
}

int lString8::atoi() const
{
    int sgn = 1;
    int n = 0;
    const lChar8 * s = c_str();
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '-')
    {
        sgn = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }
    while (*s>='0' && *s<='9')
    {
        n = n * 10 + ( (*s)-'0' );
    }
    return (sgn>0)?n:-n;
}

// constructs string representation of integer
lString8 lString8::itoa( int n )
{
    lChar8 buf[16];
    int i=0;
    int negative = 0;
    if (n==0)
        return lString8("0");
    else if (n<0)
    {
        negative = 1;
        n = -n;
    }
    for ( ; n; n/=10 )
    {
        buf[i++] = '0' + (n%10);
    }
    lString8 res;
    res.reserve(i+negative);
    if (negative)
        res.append(1, '-');
    for (int j=i-1; j>=0; j--)
        res.append(1, buf[j]);
    return res;
}

// constructs string representation of integer
lString8 lString8::itoa( unsigned int n )
{
    lChar8 buf[16];
    int i=0;
    if (n==0)
        return lString8("0");
    for ( ; n; n/=10 )
    {
        buf[i++] = '0' + (n%10);
    }
    lString8 res;
    res.reserve(i);
    for (int j=i-1; j>=0; j--)
        res.append(1, buf[j]);
    return res;
}

// constructs string representation of integer
lString16 lString16::itoa( int n )
{
    lChar16 buf[16];
    int i=0;
    int negative = 0;
    if (n==0)
        return lString16("0");
    else if (n<0)
    {
        negative = 1;
        n = -n;
    }
    for ( ; n; n/=10 )
    {
        buf[i++] = '0' + (n%10);
    }
    lString16 res;
    res.reserve(i+negative);
    if (negative)
        res.append(1, L'-');
    for (int j=i-1; j>=0; j--)
        res.append(1, buf[j]);
    return res;
}

// constructs string representation of integer
lString16 lString16::itoa( unsigned int n )
{
    lChar16 buf[16];
    int i=0;
    if (n==0)
        return lString16("0");
    for ( ; n; n/=10 )
    {
        buf[i++] = '0' + (n%10);
    }
    lString16 res;
    res.reserve(i);
    for (int j=i-1; j>=0; j--)
        res.append(1, buf[j]);
    return res;
}


lUInt32 lString8::getHash() const
{
    lUInt32 res = 0;
    for (lUInt32 i=0; i<pchunk->len; i++)
        res = res * STRING_HASH_MULT + pchunk->buf8[i];
    return res;
}

const lString8 lString8::empty_str;

int Utf8CharCount( const lChar8 * str )
{
    int count = 0;
    lUInt8 ch;
    while ( (ch=*str++) ) {
        if ( (ch & 0x80) == 0 ) {
        } else if ( (ch & 0xE0) == 0xC0 ) {
            if ( !(ch=*str++) )
                break;
        } else {
            if ( !(ch=*str++) )
                break;
            if ( !(ch=*str++) )
                break;
        }
        count++;
    }
    return count;
}

int Utf8ByteCount( const lChar16 * str )
{
    int count = 0;
    lUInt16 ch;
    while ( (ch=*str++) ) {
        if ( (ch & 0xFF80) == 0 ) {
            count++;
        } else if ( (ch & 0xF800) == 0 ) {
            count += 2;
        } else {
            count += 3;
        }
    }
    return count;
}

lString16 Utf8ToUnicode( const lString8 & str )
{
    lString16 dst;
    if (str.empty())
      return dst;
    const char * s = str.c_str();
    int len = Utf8CharCount( s );
    if (!len)
      return dst;
    dst.reserve( len );
    lUInt16 ch;
    while ( (ch=*s++) ) {
        if ( (ch & 0x80) == 0 ) {
            dst << ch;
        } else if ( (ch & 0xE0) == 0xC0 ) {
            lChar16 d = (ch & 0x1F) << 6;
            if ( !(ch=*s++) )
                break;
            d |= (ch & 0x3F);
            dst << d;
        } else {
            lChar16 d = (ch & 0x0F) << 12;
            if ( !(ch=*s++) )
                break;
            d |= (ch & 0x3F) << 6;
            if ( !(ch=*s++) )
                break;
            d |= (ch & 0x3F);
            dst << d;
        }
    }
    return dst;
}


lString8 UnicodeToUtf8( const lString16 & str )
{
    lString8 dst;
    if (str.empty())
      return dst;
    const lChar16 * s = str.c_str();
    int len = Utf8ByteCount( s );
    dst.reserve( len );
    lUInt16 ch;
    while ( (ch=*s++) ) {
        if ( (ch & 0xFF80) == 0 ) {
            dst << (lUInt8)ch;
        } else if ( (ch & 0xF800) == 0 ) {
            dst << (lUInt8) ( ((ch >> 6) & 0x1F) | 0xC0 );
            dst << (lUInt8) ( ((ch ) & 0x3F) | 0x80 );
        } else {
            dst << (lUInt8) ( ((ch >> 12) & 0x0F) | 0xE0 );
            dst << (lUInt8) ( ((ch >> 6) & 0x3F) | 0x80 );
            dst << (lUInt8) ( ((ch ) & 0x3F) | 0x80 );
        }
    }
    return dst;
}


lString8 UnicodeTo8Bit( const lString16 & str, const lChar8 * * table )
{
    lString8 buf;
    buf.reserve( str.length() );
    for ( int i=0; i<(int)str.length(); i++ ) {
        lChar16 ch = str[i];
        const lChar8 * p = table[ (ch>>8) & 255 ];
        if ( p ) {
            buf += p[ ch&255 ];
        } else {
            buf += '?';
        }
    }
    return buf;
}

#if !defined(__SYMBIAN32__) && defined(_WIN32)

lString8 UnicodeToLocal( const lString16 & str )
{
   lString8 dst;
   if (str.empty())
      return dst;
   char def_char[]="?";
   int usedDefChar = false;
   int len = WideCharToMultiByte(
      CP_ACP,
      WC_COMPOSITECHECK | WC_DISCARDNS 
       | WC_SEPCHARS | WC_DEFAULTCHAR,
      str.c_str(),
      str.length(),
      NULL,
      0,
      def_char,
      &usedDefChar
      );
   if (len)
   {
      dst.insert(0, len, ' ');
      WideCharToMultiByte(
         CP_ACP,
         WC_COMPOSITECHECK | WC_DISCARDNS 
          | WC_SEPCHARS | WC_DEFAULTCHAR,
         str.c_str(),
         str.length(),
         dst.modify(),
         len,
         def_char,
         &usedDefChar
         );
   }
   return dst;
}

lString16 LocalToUnicode( const lString8 & str )
{
   lString16 dst;
   if (str.empty())
      return dst;
   int len = MultiByteToWideChar(
      CP_ACP,
      0,
      str.c_str(),
      str.length(),
      NULL,
      0
      );
   if (len)
   {
      dst.insert(0, len, ' ');
      MultiByteToWideChar(
         CP_ACP,
         0,
         str.c_str(),
         str.length(),
         dst.modify(),
         len
         );
   }
   return dst;
}

#else

lString8 UnicodeToLocal( const lString16 & str )
{
    return UnicodeToUtf8( str );
}

lString16 LocalToUnicode( const lString8 & str )
{
    return Utf8ToUnicode( str );
}

#endif

//0x410
static const char * russian_capital[32] =
{
"A", "B", "V", "G", "D", "E", "ZH", "Z", "I", "j", "K", "L", "M", "N", "O", "P", "R", 
"S", "T", "U", "F", "H", "TS", "CH", "SH", "SH", "\'", "Y", "\'", "E", "YU", "YA"
};
static const char * russian_small[32] =
{
"a", "b", "v", "g", "d", "e", "zh", "z", "i", "j", "k", "l", "m", "n", "o", "p", "r", 
"s", "t", "u", "f", "h", "ts", "ch", "sh", "sh", "\'", "y", "\'", "e", "yu", "ya"
};
static const char * getCharTranscript( lChar16 ch )
{
    if ( ch>=0x410 && ch<0x430 )
        return russian_capital[ch-0x410];
    else if (ch>=0x430 && ch<0x450)
        return russian_small[ch-0x430];
    else if (ch==0x450)
        return "E";
    else if ( ch==0x451 )
        return "e";
    return "?";
}


lString8  UnicodeToTranslit( const lString16 & str )
{
    lString8 buf;
	if ( str.empty() )
		return buf;
    buf.reserve( str.length()*5/4 );
    for ( unsigned i=0; i<str.length(); i++ ) {
		lChar16 ch = str[i];
        if ( ch>=32 && ch<=127 ) {
            buf.append( 1, (lChar8)ch );
        } else {
            const char * trans = getCharTranscript(ch);
            buf.append( trans );
        }
	}
    buf.pack();
    return buf;
}

