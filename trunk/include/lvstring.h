/** \file lvstring.h
    \brief string classes interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRING_H_INCLUDED__
#define __LV_STRING_H_INCLUDED__

#include <stdlib.h>
#include "lvtypes.h"
#include "lvmemman.h"


/// strlen for lChar16
size_t lStr_len(const lChar16 * str);
/// strlen for lChar8
size_t lStr_len(const lChar8 * str);
/// strnlen for lChar16
size_t lStr_nlen(const lChar16 * str, size_t maxcount);
/// strnlen for lChar8
size_t lStr_nlen(const lChar8 * str, size_t maxcount);
/// strcpy for lChar16
size_t lStr_cpy(lChar16 * dst, const lChar16 * src);
/// strcpy for lChar16 -> lChar8
size_t lStr_cpy(lChar16 * dst, const lChar8 * src);
/// strcpy for lChar8
size_t lStr_cpy(lChar8 * dst, const lChar8 * src);
/// strncpy for lChar16
size_t lStr_ncpy(lChar16 * dst, const lChar16 * src, size_t maxcount);
/// strncpy for lChar8
size_t lStr_ncpy(lChar8 * dst, const lChar8 * src, size_t maxcount);
/// memcpy for lChar16
void   lStr_memcpy(lChar16 * dst, const lChar16 * src, size_t count);
/// memcpy for lChar8
void   lStr_memcpy(lChar8 * dst, const lChar8 * src, size_t count);
/// memset for lChar16
void   lStr_memset(lChar16 * dst, lChar16 value, size_t count);
/// memset for lChar8
void   lStr_memset(lChar8 * dst, lChar8 value, size_t count);
/// strcmp for lChar16
int    lStr_cmp(const lChar16 * str1, const lChar16 * str2);
/// strcmp for lChar16 <> lChar8
int    lStr_cmp(const lChar16 * str1, const lChar8 * str2);
/// strcmp for lChar8 <> lChar16
int    lStr_cmp(const lChar8 * str1, const lChar16 * str2);
/// strcmp for lChar8
int    lStr_cmp(const lChar8 * str1, const lChar8 * str2);

void lStr_uppercase( lChar16 * str, int len );

void lStr_lowercase( lChar16 * str, int len );

struct lstring_chunk_t {
    friend class lString8;
    friend class lString16;
    friend struct lstring_chunk_slice_t;
public:
    lstring_chunk_t(lChar16 * _buf16) : buf16(_buf16), size(1), len(0), nref(1) {}
    lstring_chunk_t(lChar8 * _buf8) : buf8(_buf8), size(1), len(0), nref(1) {}
private:
    union {
        lstring_chunk_t * nextfree;
        lChar16 * buf16;
        lChar8  * buf8;
    };
    size_t size;   // 0 for free chunk
    size_t len;    // count of chars in string
    int nref;      // reference counter

    lstring_chunk_t() {}

    // chunk allocation functions
    static lstring_chunk_t * alloc();
    static void free( lstring_chunk_t * pChunk );

};

/**
    \brief lChar8 string

  Interface is similar to STL strings

*/
class lString8
{
    friend class lString8Collection;
public:
    // typedefs for STL compatibility
    typedef lChar8              value_type;      ///< character type
    typedef size_t              size_type;       ///< size type
    typedef int                 difference_type; ///< difference type
    typedef value_type *        pointer;         ///< pointer to char type
    typedef value_type &        reference;       ///< reference to char type
    typedef const value_type *  const_pointer;   ///< pointer to const char type
    typedef const value_type &  const_reference; ///< reference to const char type

private:
    lstring_chunk_t * pchunk;
    static lstring_chunk_t * EMPTY_STR_8;
    void alloc(size_type sz);
    void free();
    inline void addref() const { ++pchunk->nref; }
    inline void release() { if (--pchunk->nref==0) free(); }
    explicit lString8(lstring_chunk_t * chunk) : pchunk(chunk) { addref(); }
public:
    /// default constrictor
    explicit lString8() : pchunk(EMPTY_STR_8) { addref(); }
    /// copy constructor
    lString8(const lString8 & str) : pchunk(str.pchunk) { addref(); }
    /// constructor from C string
    explicit lString8(const value_type * str);
    /// constructor from 16-bit C string
    explicit lString8(const lChar16 * str);
    /// constructor from string of specified length
    explicit lString8(const value_type * str, size_type count);
    /// fragment copy constructor
    explicit lString8(const lString8 & str, size_type offset, size_type count);
    /// destructor
    ~lString8() { release(); }

    /// copy assignment
    lString8 & assign(const lString8 & str)
    { 
        if (pchunk!=str.pchunk)
        {
            release();
            pchunk = str.pchunk;
            addref();
        }
        return *this;
    }
    /// C-string assignment
    lString8 & assign(const value_type * str);
    /// C-string fragment assignment
    lString8 & assign(const value_type * str, size_type count);
    /// string fragment assignment
    lString8 & assign(const lString8 & str, size_type offset, size_type count);
    /// C-string assignment
    lString8 & operator = (const value_type * str) { return assign(str); }
    /// string copy assignment
    lString8 & operator = (const lString8 & str) { return assign(str); }
    /// erase part of string
    lString8 & erase(size_type offset, size_type count);
    /// append C-string
    lString8 & append(const value_type * str);
    /// append C-string fragment
    lString8 & append(const value_type * str, size_type count);
    /// append string
    lString8 & append(const lString8 & str);
    /// append string fragment
    lString8 & append(const lString8 & str, size_type offset, size_type count);
    /// append repeated character
    lString8 & append(size_type count, value_type ch);
    /// insert C-string
    lString8 & insert(size_type p0, const value_type * str);
    /// insert C-string fragment
    lString8 & insert(size_type p0, const value_type * str, size_type count);
    /// insert string
    lString8 & insert(size_type p0, const lString8 & str);
    /// insert string fragment
    lString8 & insert(size_type p0, const lString8 & str, size_type offset, size_type count);
    /// insert repeated character
    lString8 & insert(size_type p0, size_type count, value_type ch);
    /// replace fragment with C-string
    lString8 & replace(size_type p0, size_type n0, const value_type * str);
    /// replace fragment with C-string fragment
    lString8 & replace(size_type p0, size_type n0, const value_type * str, size_type count);
    /// replace fragment with string
    lString8 & replace(size_type p0, size_type n0, const lString8 & str);
    /// replace fragment with string fragment
    lString8 & replace(size_type p0, size_type n0, const lString8 & str, size_type offset, size_type count);
    /// replace fragment with repeated character
    lString8 & replace(size_type p0, size_type n0, size_type count, value_type ch);
    /// compare with another string
    int compare(const lString8& str) const { return lStr_cmp(pchunk->buf8, str.pchunk->buf8); }
    /// compare part of string with another string
    int compare(size_type p0, size_type n0, const lString8& str) const;
    /// compare part of string with fragment of another string
    int compare(size_type p0, size_type n0, const lString8& str, size_type pos, size_type n) const;
    /// compare with C-string
    int compare(const value_type *s) const  { return lStr_cmp(pchunk->buf8, s); }
    /// compare part of string with C-string
    int compare(size_type p0, size_type n0, const value_type *s) const;
    /// compare part of string with C-string fragment
    int compare(size_type p0, size_type n0, const value_type *s, size_type pos) const;
    /// find position of substring inside string, -1 if not found
    int pos(lString8 subStr) const;

    /// substring
    lString8 substr(size_type pos, size_type n) const;
    
    /// append single character
    lString8 & operator << (value_type ch) { return append(1, ch); }
    /// append C-string
    lString8 & operator << (const value_type * str) { return append(str); }
    /// append string
    lString8 & operator << (const lString8 & str) { return append(str); }

    /// calculate hash
    lUInt32 getHash() const;

    /// get character at specified position with range check
    value_type & at( size_type pos ) { if (pos<0||pos>pchunk->len) crFatalError(); return modify()[pos]; }
    /// get character at specified position without range check
    const value_type operator [] ( size_type pos ) const { return pchunk->buf8[pos]; }
    /// get reference to character at specified position
    value_type & operator [] ( size_type pos ) { return modify()[pos]; }

    /// ensures that reference count is 1
    void  lock( size_type newsize );
    /// returns pointer to modifable string buffer
    value_type * modify() { if (pchunk->nref>1) lock(pchunk->len); return pchunk->buf8; }
    /// clear string
    void  clear() { release(); pchunk = EMPTY_STR_8; addref(); }
    /// clear string, set buffer size
    void  reset( size_type size );
    /// returns character count
    size_type   length() const { return pchunk->len; }
    /// returns buffer size
    size_type   size() const { return pchunk->len; }
    /// changes buffer size
    void  resize(size_type count = 0, value_type e = 0);
    /// returns maximum number of chars that can fit into buffer
    size_type   capacity() const { return pchunk->size-1; }
    /// reserve space for specified amount of chars
    void  reserve(size_type count = 0);
    /// returns true if string is empty
    bool  empty() const { return pchunk->len==0; }
    /// swaps content of two strings
    void  swap( lString8 & str ) { lstring_chunk_t * tmp = pchunk; 
                pchunk=str.pchunk; str.pchunk=tmp; }
    /// pack string (free unused buffer space)
    lString8 & pack();

    /// remove spaces from begin and end of string
    lString8 & trim();
    /// convert to integer
    int atoi() const;

    /// returns C-string
    const value_type * c_str() const { return pchunk->buf8; }
    /// returns C-string
    const value_type * data() const { return pchunk->buf8; }

    /// append string
    lString8 & operator += ( lString8 s ) { return append(s); }
    /// append C-string
    lString8 & operator += ( const value_type * s ) { return append(s); }
    /// append single character
    lString8 & operator += ( value_type ch ) { return append(1, ch); }

    /// constructs string representation of integer
    static lString8 itoa( int i );
    /// constructs string representation of unsigned integer
    static lString8 itoa( unsigned int i );

    static const lString8 empty_str;

    friend class lString16Collection;
};

/**
    \brief lChar16 string

  Interface is similar to STL strings

*/
class lString16
{
public:
    // typedefs for STL compatibility
    typedef lChar16             value_type;
    typedef size_t              size_type;
    typedef int                 difference_type;
    typedef value_type *        pointer;
    typedef value_type &        reference;
    typedef const value_type *  const_pointer;
    typedef const value_type &  const_reference;

private:
    lstring_chunk_t * pchunk;
    static lstring_chunk_t * EMPTY_STR_16;
    void alloc(size_type sz);
    void free();
    inline void addref() const { ++pchunk->nref; }
    inline void release() { if (--pchunk->nref==0) free(); }
public:
    explicit lString16() : pchunk(EMPTY_STR_16) { addref(); }
    lString16(const lString16 & str) : pchunk(str.pchunk) { addref(); }
    explicit lString16(lstring_chunk_t * chunk) : pchunk(chunk) { addref(); }
    lString16(const value_type * str);
    explicit lString16(const lChar8 * str);
    explicit lString16(const value_type * str, size_type count);
    explicit lString16(const lString16 & str, size_type offset, size_type count);
    ~lString16() { release(); }

    // assignment
    lString16 & assign(const lString16 & str)
    { 
        if (pchunk!=str.pchunk)
        {
            release();
            pchunk = str.pchunk;
            addref();
        }
        return *this;
    }
    lString16 & assign(const value_type * str);
    lString16 & assign(const value_type * str, size_type count);
    lString16 & assign(const lString16 & str, size_type offset, size_type count);
    lString16 & operator = (const value_type * str) { return assign(str); }
    lString16 & operator = (const lString16 & str) { return assign(str); }
    lString16 & erase(size_type offset, size_type count);
    lString16 & append(const value_type * str);
    lString16 & append(const value_type * str, size_type count);
    lString16 & append(const lString16 & str);
    lString16 & append(const lString16 & str, size_type offset, size_type count);
    lString16 & append(size_type count, value_type ch);
    lString16 & insert(size_type p0, const value_type * str);
    lString16 & insert(size_type p0, const value_type * str, size_type count);
    lString16 & insert(size_type p0, const lString16 & str);
    lString16 & insert(size_type p0, const lString16 & str, size_type offset, size_type count);
    lString16 & insert(size_type p0, size_type count, value_type ch);
    lString16 & replace(size_type p0, size_type n0, const value_type * str);
    lString16 & replace(size_type p0, size_type n0, const value_type * str, size_type count);
    lString16 & replace(size_type p0, size_type n0, const lString16 & str);
    lString16 & replace(size_type p0, size_type n0, const lString16 & str, size_type offset, size_type count);
    lString16 & replace(size_type p0, size_type n0, size_type count, value_type ch);
    lString16 & uppercase();
    lString16 & lowercase();
    int compare(const lString16& str) const { return lStr_cmp(pchunk->buf16, str.pchunk->buf16); }
    int compare(size_type p0, size_type n0, const lString16& str) const;
    int compare(size_type p0, size_type n0, const lString16& str, size_type pos, size_type n) const;
    int compare(const value_type *s) const  { return lStr_cmp(pchunk->buf16, s); }
    int compare(const lChar8 *s) const  { return lStr_cmp(pchunk->buf16, s); }
    int compare(size_type p0, size_type n0, const value_type *s) const;
    int compare(size_type p0, size_type n0, const value_type *s, size_type pos) const;

    lString16 substr(size_type pos, size_type n) const;
    /// find position of substring inside string, -1 if not found
    int pos(lString16 subStr) const;
    
    lString16 & operator << (value_type ch) { return append(1, ch); }
    lString16 & operator << (const value_type * str) { return append(str); }
    lString16 & operator << (const lString16 & str) { return append(str); }

    lUInt32 getHash() const;

    value_type & at( size_type pos ) { if (pos<0||pos>pchunk->len) crFatalError(); return modify()[pos]; }
    const value_type operator [] ( size_type pos ) const { return pchunk->buf16[pos]; }
    value_type & operator [] ( size_type pos ) { return modify()[pos]; }

    void  lock( size_type newsize );
    value_type * modify() { if (pchunk->nref>1) lock(pchunk->len); return pchunk->buf16; }
    void  clear() { release(); pchunk = EMPTY_STR_16; addref(); }
    void  reset( size_type size );
    size_type   length() const { return pchunk->len; }
    size_type   size() const { return pchunk->len; }
    void  resize(size_type count = 0, value_type e = 0);
    size_type   capacity() const { return pchunk->size-1; }
    void  reserve(size_type count = 0);
    bool  empty() const { return pchunk->len==0; }
    void  swap( lString16 & str ) { lstring_chunk_t * tmp = pchunk; 
                pchunk=str.pchunk; str.pchunk=tmp; }
    lString16 & pack();


    lString16 & trim();
    lString16 & trimDoubleSpaces( bool allowStartSpace, bool allowEndSpace, bool removeEolHyphens=false );
    int atoi() const;
    bool atoi( int &n ) const;
    bool atoi( lInt64 &n ) const;

    const value_type * c_str() const { return pchunk->buf16; }
    const value_type * data() const { return pchunk->buf16; }

    lString16 & operator += ( lString16 s ) { return append(s); }
    lString16 & operator += ( const value_type * s ) { return append(s); }
    lString16 & operator += ( value_type ch ) { return append(1, ch); }

    /// constructs string representation of integer
    static lString16 itoa( int i );
    /// constructs string representation of unsigned integer
    static lString16 itoa( unsigned int i );
    /// constructs string representation of 64 bit integer
    static lString16 itoa( lInt64 i );
    /// constructs string representation of unsigned 64 bit integer
    static lString16 itoa( lUInt64 i );

    static const lString16 empty_str;

    friend class lString16Collection;
};

class lString16Collection
{
private:
    lstring_chunk_t * * chunks;
    size_t count;
    size_t size;
public:
    lString16Collection()
        : chunks(NULL), count(0), size(0)
    { }
    /// parse delimiter-separated string
    void parse( lString16 string, lChar16 delimiter, bool flgTrim );
    void reserve( size_t space );
    size_t add( const lString16 & str );
    void erase(int offset, int count);
    const lString16 & at( size_t index )
    {
        return ((lString16 *)chunks)[index];
    }
    const lString16 & operator [] ( size_t index ) const
    {
        return ((lString16 *)chunks)[index];
    }
    size_t length() const { return count; }
    void clear();
    ~lString16Collection()
    {
        clear();
    }
};

class lString8Collection
{
private:
    lstring_chunk_t * * chunks;
    size_t count;
    size_t size;
public:
    lString8Collection()
        : chunks(NULL), count(0), size(0)
    { }
    void reserve( size_t space );
    size_t add( const lString8 & str );
    void erase(int offset, int count);
    const lString8 & at( size_t index )
    {
        return ((lString8 *)chunks)[index];
    }
    const lString8 & operator [] ( size_t index ) const
    {
        return ((lString8 *)chunks)[index];
    }
    size_t length() const { return count; }
    void clear();
    ~lString8Collection()
    {
        clear();
    }
};


lUInt32 calcStringHash( const lChar16 * s );

class lString16HashedCollection : public lString16Collection
{
private:
    size_t hashSize;
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
    lString16HashedCollection( lUInt32 hashSize );
    ~lString16HashedCollection();
    size_t add( const lChar16 * s );
    size_t find( const lChar16 * s );
};

inline bool operator == (const lString16& s1, const lString16& s2 )
    { return s1.compare(s2)==0; }
inline bool operator == (const lString16& s1, const lChar16 * s2 )
    { return s1.compare(s2)==0; }
inline bool operator == (const lChar16 * s1, const lString16& s2 )
    { return s2.compare(s1)==0; }
inline bool operator != (const lString16& s1, const lString16& s2 )
    { return s1.compare(s2)!=0; }
inline bool operator != (const lString16& s1, const lChar16 * s2 )
    { return s1.compare(s2)!=0; }
inline bool operator != (const lChar16 * s1, const lString16& s2 )
    { return s2.compare(s1)!=0; }
inline lString16 operator + (const lString16 &s1, const lString16 &s2) { lString16 s(s1); s.append(s2); return s; }

inline bool operator == (const lString8& s1, const lString8& s2 )
    { return s1.compare(s2)==0; }
inline bool operator == (const lString8& s1, const lChar8 * s2 )
    { return s1.compare(s2)==0; }
inline bool operator == (const lChar8 * s1, const lString8& s2 )
    { return s2.compare(s1)==0; }
inline bool operator != (const lString8& s1, const lString8& s2 )
    { return s1.compare(s2)!=0; }
inline bool operator != (const lString8& s1, const lChar8 * s2 )
    { return s1.compare(s2)!=0; }
inline bool operator != (const lChar8 * s1, const lString8& s2 )
    { return s2.compare(s1)!=0; }
inline lString8 operator + (const lString8 &s1, const lString8 &s2)
    { lString8 s(s1); s.append(s2); return s; }
inline lString8 operator + (const lString8 &s1, const lChar8 * s2) 
    { lString8 s(s1); s.append(s2); return s; }


lString8  UnicodeToTranslit( const lString16 & str );
lString8  UnicodeToLocal( const lString16 & str );
lString8  UnicodeToUtf8( const lString16 & str );
lString8  UnicodeTo8Bit( const lString16 & str, const lChar8 * * table );
lString16 LocalToUnicode( const lString8 & str );
lString16 Utf8ToUnicode( const lString8 & str );


void free_ls_storage();

#ifdef _DEBUG
#include <stdio.h>
class DumpFile
{
public:
    FILE * f;
    DumpFile( const char * fname )
    : f(NULL)
    {
        if ( fname )
            f = fopen( fname, "at" );
        if ( !f )
            f = stdout;
        fprintf(f, "DumpFile log started\n");
    }
    ~DumpFile()
    { 
        if ( f!=stdout )
            fclose(f);
    }
    operator FILE * () { if (f) fflush(f); return f?f:stdout; }
};
#endif

#endif
