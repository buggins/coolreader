#ifndef LVSTRING2_H
#define LVSTRING2_H

/**
 * Copy-on-write reference counting strings.
 *
 * Optimized for low footprint and high performance.
 * Thread-safe if instantiated with std::atomic_int as refcounter_type.
 *
 * Designed to be a drop-in replacement for old lString8/lString16/lString32 in coolreader core.
 *
 * Empty string is internally represented by `nullptr`.
 *
 * Length, capacity and refcount are located in signle block alongside with character data, occupying 12 bytes for 4-byte size_type and refcounter_type.
 * Space for additional zero-termination character is always allocated in addition to capacity() value.
 *
 * To minimize refcount checks, use writable reference to modify string content.
 *
 * Interface is partially compatible with std::string.
 */

#include "lvtypes.h"
#include <atomic>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>

#define DEBUG_TRACK_LSTRING2_ALLOC

void lStr_uppercase( lChar32 * str, int len );
void lStr_uppercase( lChar16 * str, int len );
void lStr_uppercase( lChar8 * str, int len );
void lStr_lowercase( lChar32 * str, int len );
void lStr_lowercase( lChar16 * str, int len );
void lStr_lowercase( lChar8 * str, int len );

namespace lv {

#ifdef DEBUG_TRACK_LSTRING2_ALLOC
struct lStringStats {
    int allocCount {0};
    int freeCount {0};
    int copyConstr {0};
    int moveConstr {0};
    int copyAssign {0};
    int moveAssign {0};
    void dump(const char * msg = "");
};
extern lStringStats ls_alloc_stats;

#define LS_COUNT_ALLOC ls_alloc_stats.allocCount++;
#define LS_COUNT_FREE ls_alloc_stats.freeCount++;
#define LS_COUNT_COPY_CONSTR ls_alloc_stats.copyConstr++;
#define LS_COUNT_MOVE_CONSTR ls_alloc_stats.moveConstr++;
#define LS_COUNT_COPY_ASSIGN ls_alloc_stats.copyAssign++;
#define LS_COUNT_MOVE_ASSIGN ls_alloc_stats.moveAssign++;

#else

#define LS_COUNT_ALLOC
#define LS_COUNT_FREE
#define LS_COUNT_COPY_CONSTR
#define LS_COUNT_MOVE_CONSTR
#define LS_COUNT_COPY_ASSIGN
#define LS_COUNT_MOVE_ASSIGN

#endif


// lv::fmt copy of fmt
namespace fmt {
    class decimal {
        lInt64 value;
      public:
        explicit decimal(lInt64 v) : value(v) { }
        lInt64 get() const { return value; }
    };

    class hex {
        lUInt64 value;
      public:
        explicit hex(lInt64 v) : value(v) { }
        lUInt64 get() const { return value; }
    };
}


// Helper functions

/// arbitrary char type strlen, supports nullptr arg
template<typename char_type, typename size_type>
inline size_type str_len(const char_type * s) {
    if (s == nullptr) {
        return 0;
    }
    size_type i = 0;
    while (s[i] != 0) {
        i++;
    }
    return i;
}

/// arbitrary char type strcmp, supports empty strings
template<typename char_type, typename size_type>
inline int str_cmp(const char_type * s1, size_type sz1, const char_type * s2, size_type sz2) {
    // support case when one of strings or both are empty
    if (sz1 == 0) {
        return sz2 > 0 ? -1 : 0;
    } else if (sz2 == 0) {
        return 1;
    }
    for (size_type i = 0; ; i++) {
        if (i >= sz1) {
            // equal or s2 is bigger
            return i < sz2 ? -1 : 0;
        } else if (i >= sz2) {
            // less
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

/// arbitrary char type strcmp, both args are non-empty
template<typename char_type, typename size_type>
inline int str_cmp_nonempty(const char_type * s1, size_type sz1, const char_type * s2, size_type sz2) {
    // we are sure that sz1>0 && sz2>0
    for (size_type i = 0; ; i++) {
        if (i >= sz1) {
            // end of s1
            // equal or s2 is bigger
            return (i < sz2) ? -1 : 0;
        } else if (i >= sz2) {
            // s2 not yet ended
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

/// arbitrary char type strcmp, both args are non-empty, s2 is zero-terminated
template<typename char_type, typename size_type>
inline int str_cmp_nonempty(const char_type * s1, size_type sz1, const char_type * s2) {
    // we are sure that sz1>0 && sz2>0
    for (size_type i = 0; ; i++) {
        if (i >= sz1) {
            // end of s1
            // equal or s2 is bigger
            return s2[i] ? -1 : 0;
        } else if (!s2[i]) {
            // end of s2
            // bigger
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

/// arbitrary char type strcmp, both args are non-empty, zero-terminated
template<typename char_type, typename size_type>
inline int str_cmp_nonempty(const char_type * s1, const char_type * s2) {
    // we are sure that sz1>0 && sz2>0
    for (size_type i = 0; ; i++) {
        if (!s1[i]) {
            // end of s1
            // equal or s2 is bigger
            return s2[i] ? -1 : 0;
        } else if (!s2[i]) {
            // end of s2
            // bigger
            return 1;
        }
        if (s1[i] < s2[i]) {
            return -1;
        }
        if (s1[i] > s2[i]) {
            return 1;
        }
        // char [i] is matching, continue
    }
}

// String data buffer with ref count

// forward declaration of string readonly class
template<typename char_type, typename refcounter_type>
class string_ro;
// forward declaration of string writable ref class
template<typename char_type, typename refcounter_type>
class string_wr;
// forward declaration of string class
template<typename char_type, typename refcounter_type>
class string;

template <typename char_type, typename size_type, typename refcounter_type = std::atomic_int>
struct lstring_chunk_t {

    friend class string<char_type, refcounter_type>;
    friend class string_wr<char_type, refcounter_type>;
    friend class string_ro<char_type, refcounter_type>;

    friend void test_lstring2_chunks();
    /// chunk allocation alignment in bytes
    static constexpr size_type alloc_align_bytes = 16;
    /// minimum reserved characters count to have allocated size (alloc_align_bytes) bytes
    static constexpr size_type min_size = static_cast<size_type>((alloc_align_bytes - sizeof(lstring_chunk_t)) / sizeof(char_type)) - 1;
    /// character count increment to keep block size aligned
    static constexpr size_type size_align = static_cast<size_type>(alloc_align_bytes / sizeof(char_type));

public:

    //const char_type * data() const { return &buf[0]; }

    /// align reserved size in characters of block to have byte alignment alloc_align_bytes, size does not include trailing 0 character
    inline static size_type alignSize(size_type sz) noexcept {
        if (sz <= min_size)
            return min_size;
        return min_size + (((sz - min_size) + size_align - 1) & (0 - size_align));
    }

    /// get reference counter value
    size_type getRefCount() const noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            return refCount;
        } else {
            // atomic
            return refCount.load(std::memory_order_relaxed);
        }
    }

    /// returns true if ref counter value is 1
    bool isOwn() const noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            return refCount == 1;
        } else {
            // atomic
            return refCount.load(std::memory_order_relaxed) == 1;
        }
    }

    /// returns true if ref counter value is 1
    bool isShared() const noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            return refCount > 1;
        } else {
            // atomic
            return refCount.load(std::memory_order_relaxed) > 1;
        }
    }

    /// 1. Hook for incrementing the counter (compatible with boost::intrusive_ptr)
    friend void intrusive_ptr_add_ref(const lstring_chunk_t* p) noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            // simple refcount
            p->refCount++;
        } else {
            p->refCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /// 1. Hook for incrementing the counter (compatible with boost::intrusive_ptr)
    friend void intrusive_ptr_add_ref_checknull(const lstring_chunk_t* p) noexcept {
        if (p) {
            if constexpr (std::is_same_v<refcounter_type, size_type>) {
                // simple refcount
                p->refCount++;
            } else {
                p->refCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    void testAddRef() {
        refCount.fetch_add(1, std::memory_order_relaxed);
    }

    void testReleaseRef() {
        refCount.fetch_sub(1, std::memory_order_acq_rel);
    }

    /// 2. Hook for decrementing and deleting the object (compatible with boost::intrusive_ptr)
    friend void intrusive_ptr_release(const lstring_chunk_t* p) noexcept {
        if constexpr (std::is_same_v<refcounter_type, size_type>) {
            if ((p->refCount--) == 1) {
                lstring_chunk_t::free(p);
            }
        } else {
            if (p->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                lstring_chunk_t::free(p);
            }
        }
    }

    /// chunk allocation function: create string buffer with reserved sz characters + zero termination char, ref counter 1
    static lstring_chunk_t * alloc(size_type sz) noexcept {
        LS_COUNT_ALLOC
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        sz = alignSize(sz);
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        res->size = sz;
        res->len = 0;
        res->buf[0] = 0;
        res->buf[sz] = 0;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create string buffer initialized with count chars from string s with reserved sz characters + zero termination char, ref counter 1
    static lstring_chunk_t * alloc(const char_type * s, size_type count, size_type sz = 0) noexcept {
        LS_COUNT_ALLOC
        if (sz < count)
            sz = count;
        sz = alignSize(sz);
        //lstring_chunk_t * res = static_cast<lstring_chunk_t *>( ::malloc(sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1)) );
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, s, count * sizeof(char_type));
        res->buf[count] = 0;
        res->buf[sz] = 0;
        res->size = sz;
        res->len = count;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create duplicate of this string buffer with ref counter 1, does not release this buffer
    lstring_chunk_t * duplicate() noexcept {
        LS_COUNT_ALLOC
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (size + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, buf, len * sizeof(char_type));
        res->buf[len] = 0;
        res->buf[size] = 0;
        res->size = size;
        res->len = len;
        res->refCount = 1;
        return res;
    }

    /// chunk allocation function: create duplicate of this string buffer with capacity newSize and ref counter 1, does not release this buffer
    lstring_chunk_t * duplicate(size_type newSize) noexcept {
        LS_COUNT_ALLOC
        size_type sz = newSize;
        size_type count = len > sz ? sz : len;
        size_t allocBytes = (sizeof(lstring_chunk_t) + sizeof(char_type) * (sz + 1));
        lstring_chunk_t * res = static_cast<lstring_chunk_t *>( std::aligned_alloc(alloc_align_bytes, allocBytes) );
        std::memcpy(res->buf, buf, count * sizeof(char_type));
        res->buf[count] = 0;
        res->buf[sz] = 0;
        res->size = sz;
        res->len = count;
        res->refCount = 1;
        return res;
    }

    /// free chunk memory
    static void free( const lstring_chunk_t * pChunk ) noexcept {
        LS_COUNT_FREE
        ::free(const_cast<lstring_chunk_t *>(pChunk));
    }

private:
    lstring_chunk_t() : size(1), len(0)
    {
        refCount = 1;
    }
    ~lstring_chunk_t() {};
    // layout for 4-byte size_type and refcounew
    /// 0: size (max capacity for chars) less 1
    size_type       size;     // 0 for free chunk
    /// 4: length (number of chars in array)
    size_type       len;      // count of chars in string
    /// 8: reference counter
    mutable refcounter_type refCount; // reference counter
    /// 12: string characters buffer of variable size
    char_type       buf[0];      // z-string
};

extern lChar32 fake_null_buffer_32;


/// writable copy of string (guaranteed to be empty or have reference counter == 1
/// can only be passed by reference created from string
/// allows modification without reference counting checks
template <typename char_type, typename refcounter_type = std::atomic_int>
class string_ro {
    friend class string_wr<char_type, refcounter_type>;
    friend class string<char_type, refcounter_type>;
  public:
    // typedefs for STL compatibility
    typedef char_type             value_type;      ///< character type
    typedef lUInt32               size_type;       ///< size type
    typedef lInt32                difference_type; ///< difference type
    typedef value_type *          pointer;         ///< pointer to char type
    typedef value_type &          reference;       ///< reference to char type
    typedef const value_type *    const_pointer;   ///< pointer to const char type
    typedef const value_type &    const_reference; ///< reference to const char type
    //typedef string<char_type, refcounter_type> string_type; ///< normal shared string reference type

    // constant for not found / unspecified position
    static const size_type npos = -1;

    typedef lstring_chunk_t<char_type, size_type, refcounter_type> chunk_t;
  protected:

    /// disabled: always created as a synonim of the string with the same template parameters
    string_ro() = default;
    /// don't free pchunk - it's owned by string
    ~string_ro() = default;
  public:

    // ============================================================================
    // size and capacity
    // ============================================================================

    /// returns true if string is empty
    bool empty() const noexcept { return pchunk == nullptr || pchunk->len==0; }
    /// returns character count
    size_type   length() const noexcept { return pchunk == nullptr ? 0 : pchunk->len; }
    /// returns character count (same as length)
    size_type   size() const noexcept { return pchunk == nullptr ? 0 : pchunk->len; }
    /// returns maximum number of chars that can fit into buffer (there is always additional one char space for trailing 0 which is not counted)
    size_type   capacity() const noexcept { return pchunk==nullptr ? 0 : pchunk->size; }

    // ============================================================================
    // item index methods
    // ============================================================================

    /// index value. No index nor empty string checks. Will crash on empty string.
    char_type operator [] (size_type index) const noexcept {
        return pchunk->buf[index];
    }

    /// index ref. No index nor refcount checks. Will crash on empty string. Only use on non-empy string which owns data (refcount==1).
    char_type& operator [] (size_type index) noexcept {
        return pchunk->buf[index];
    }

    /// index value with bounds checking. No index nor empty string checks. Will crash on empty string.
    /// Does not throw exception on index out of bounds, returns 0 instead.
    char_type at(size_type index) const noexcept {
        if (pchunk == nullptr || index >= pchunk->len) {
            return 0;
        }
        return pchunk->buf[index];
    }

    /// index ref. No index nor refcount checks. Will crash on empty string. Only use on non-empy string which owns data (refcount==1).
    /// Does not throw exception on index out of bounds, returns reference pointing to static 0 value instead.
    char_type& at(size_type index) noexcept {
        if (pchunk == nullptr || index >= pchunk->len) {
            return *(static_cast<char_type*>(&fake_null_buffer_32));
        }
        return pchunk->buf[index];
    }

    /// return C-style null-terminated string pointer
    const char_type * c_str() const noexcept {
        if (pchunk == nullptr) {
            return reinterpret_cast<const char_type *>(&fake_null_buffer_32);
        } else {
            // enforce null-termination
            const_cast<chunk_t*>(pchunk)->buf[pchunk->len] = 0;
            return pchunk->buf;
        }
    }

    /// returns last character from string, or 0 for empty string
    char_type lastChar() const noexcept {
        if (pchunk && pchunk->len) {
            return pchunk->buf[pchunk->len-1];
        } else {
            return 0;
        }
    }

    /// returns first character from string, or 0 for empty string
    char_type firstChar() const noexcept {
        if (pchunk && pchunk->len) {
            return pchunk->buf[0];
        } else {
            return 0;
        }
    }

    // ============================================================================
    // compare methods
    // ============================================================================

    /// compare this string with another string, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(const string_ro& s) const noexcept {
        if (pchunk == s.pchunk) {
            // same string
            return 0;
        }
        size_type sz1 = length();
        size_type sz2 = s.length();
        if (sz1 == 0) {
            return sz2 > 0 ? -1 : 0;
        } else if (sz2 == 0) {
            return 1;
        }
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf, sz1, s.pchunk->buf, sz2);
    }

    /// compare substring (pos..pos+n) of this string with another string, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const string_ro& s) const noexcept {
        if (!pchunk || pos >= pchunk->len) {
            // this string fragment is empty
            return s.empty() ? 0 : -1;
        }
        // clamp this fragment size
        if (pos + n > pchunk->len) {
            n = pchunk->len - pos;
        }
        // n > 0
        size_type sz2 = s.length();
        if (sz2 == 0) {
            return 1;
        }
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf + pos, n, s.pchunk->buf, sz2);
    }

    /// compare substring (pos..pos+n) of this string with substring of another string (pos2..pos2+n2), returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const string_ro& s, size_type pos2, size_type n2) const noexcept {
        // check if another string fragment is empty
        bool s_empty = (!s.pchunk || pos2 >= s.pchunk->len);
        if (!pchunk || pos >= pchunk->len) {
            // this string fragment is empty
            return s_empty ? 0 : -1;
        }
        if (s_empty) {
            // this string non-empty, another string is empty
            return 1;
        }
        // both string fragments are non-empty
        // clamp this fragment size
        if (pos + n > pchunk->len) {
            n = pchunk->len - pos;
        }
        // clamp other fragment size
        if (pos2 + n2 > s.pchunk->len) {
            n2 = s.pchunk->len - pos2;
        }
        // n > 0, n2 > 0
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf + pos, n, s.pchunk->buf + pos2, n2);
    }

    /// compare substring (pos..pos+n) of this string with substring of another string s of len n2, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const char_type * s, size_type n2) const noexcept {
        // check if another string fragment is empty
        bool s_empty = !s || !n2;
        if (!pchunk || pos >= pchunk->len) {
            // this string fragment is empty
            return s_empty ? 0 : -1;
        }
        if (s_empty) {
            // this string non-empty, another string is empty
            return 1;
        }
        // both string fragments are non-empty
        // clamp this fragment size
        if (pos + n > pchunk->len) {
            n = pchunk->len - pos;
        }
        // n > 0, n2 > 0
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf + pos, n, s, n2);
    }

    /// compare substring (pos..pos+n) of this string with another string s, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(size_type pos, size_type n, const char_type * s) const noexcept {
        size_type n2 = (!s) ? 0 : str_len<char_type, size_type>(s);
        return compare(pos, n, s, n2);
    }

    /// compare this string with string literal, returns -1 if this < s, 1 if this > s, 0 if equal
    int compare(const char_type* s) const noexcept {
        size_t sz1 = length();
        if (sz1 == 0) {
            return (s != nullptr && *s != 0) ? -1 : 0;
        } else if (s == nullptr || *s == 0) {
            return 1;
        }
        return str_cmp_nonempty<char_type, size_type>(pchunk->buf, sz1, s);
    }

    /// returns true when strings are equal
    bool operator == (const string_ro& s) const noexcept {
        return compare(s) == 0;
    }

    /// returns true when strings are not equal
    bool operator != (const string_ro& s) const noexcept {
        return compare(s) != 0;
    }

    /// returns true when this string < s
    bool operator < (const string_ro& s) const noexcept {
        return compare(s) < 0;
    }

    /// returns true when this string >= s
    bool operator <= (const string_ro& s) const noexcept {
        return compare(s) <= 0;
    }

    /// returns true when this string > s
    bool operator > (const string_ro& s) const noexcept {
        return compare(s) > 0;
    }

    /// returns true when this string <= s
    bool operator >= (const string_ro& s) const noexcept {
        return compare(s) >= 0;
    }

    /// returns true when strings are equal
    bool operator == (const char_type * s) const noexcept {
        return compare(s) == 0;
    }

    /// returns true when strings are not equal
    bool operator != (const char_type * s) const noexcept {
        return compare(s) != 0;
    }

    /// returns true when this string < s
    bool operator < (const char_type * s) const noexcept {
        return compare(s) < 0;
    }

    /// returns true when this string <= s
    bool operator <= (const char_type * s) const noexcept {
        return compare(s) <= 0;
    }

    /// returns true when this string > s
    bool operator > (const char_type * s) const noexcept {
        return compare(s) > 0;
    }

    /// returns true when this string >= s
    bool operator >= (const char_type * s) const noexcept {
        return compare(s) >= 0;
    }


    // ============================================================================
    // search methods
    // ============================================================================

    /// return first position of char c inside string starting from positon pos, or npos if no char is found.
    size_type pos(char_type c, size_type start = 0) const noexcept {
        if (pchunk) {
            const char_type * buf = pchunk->buf;
            size_type len = pchunk->len;
            for (size_type i = start; i < len; i++) {
                if (buf[i] == c) {
                    return i;
                }
            }
        }
        return npos;
    }

    /// return first position of string s starting from positon pos, or npos if no char is found.
    size_type pos(const string_ro& s, size_type start = 0) const noexcept {
        if (pchunk && s.pchunk) {
            size_type len = pchunk->len;
            if (start >= len) {
                return npos;
            }
            len -= start;
            size_type slen = s.pchunk->len;
            if (slen > len) {
                return npos;
            }
            const char_type * sbuf = s.pchunk->buf;
            const char_type * buf = pchunk->buf + start;
            size_type maxstart = len - slen;
            for (size_type i = 0; i <= maxstart; i++) {
                size_type found = i + start;
                for (size_type j = 0; j < slen; j++) {
                    if (buf[i + j] != sbuf[j]) {
                        found = npos;
                        break;
                    }
                }
                if (found != npos) {
                    return found;
                }
            }
        }
        return npos;
    }

    /// return first position of null-term string starting from positon pos, or npos if no char is found.
    size_type pos(const char_type* s, size_type start = 0) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            if (start >= len) {
                return npos;
            }
            len -= start;
            size_type slen = str_len<char_type,size_type>(s);
            if (slen > len) {
                return npos;
            }
            const char_type * buf = pchunk->buf + start;
            size_type maxstart = len - slen;
            for (size_type i = 0; i <= maxstart; i++) {
                size_type found = i + start;
                for (size_type j = 0; j < slen; j++) {
                    if (buf[i + j] != s[j]) {
                        found = npos;
                        break;
                    }
                }
                if (found != npos) {
                    return found;
                }
            }
        }
        return npos;
    }

    /// return last position of null-term string s, or npos if no char is found.
    size_type rpos(const char_type* s) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            size_type slen = str_len<char_type,size_type>(s);
            if (slen > len) {
                return npos;
            }
            const char_type * buf = pchunk->buf;
            size_type maxstart = len - slen;
            for (size_type i = maxstart; ; i--) {
                size_type found = i;
                for (size_type j = 0; j < slen; j++) {
                    if (buf[i + j] != s[j]) {
                        found = npos;
                        break;
                    }
                }
                if (found != npos) {
                    return found;
                }
                if (i == 0) {
                    break;
                }
            }
        }
        return npos;
    }

    /// returns true if this string starts with s[0..count-1], pass count=npos to calculate string size internally
    bool startsWith(const char_type* s, size_type count = npos) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            size_type slen = (count == npos) ? str_len<char_type,size_type>(s) : count;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf;
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != s[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    // ============================================================================
    // startsWith / endsWith
    // ============================================================================

    /// returns true if this string ends with s[0..count-1], pass count=npos to calculate string size internally
    bool endsWith(const char_type* s, size_type count = npos) const noexcept {
        if (pchunk && s && s[0]) {
            size_type len = pchunk->len;
            size_type slen = (count == npos) ? str_len<char_type,size_type>(s) : count;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf + (len - slen);
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != s[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /// returns true if this string starts with s
    bool startsWith(const string_ro& s) const noexcept {
        if (pchunk && s.pchunk) {
            size_type len = pchunk->len;
            size_type slen = s.pchunk->len;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf;
            const char_type * sbuf = s.pchunk->buf;
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != sbuf[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    /// returns true if this string starts with s
    bool endsWith(const string_ro& s) const noexcept {
        if (pchunk && s.pchunk) {
            size_type len = pchunk->len;
            size_type slen = s.pchunk->len;
            if (slen > len) {
                return false;
            }
            const char_type * buf = pchunk->buf + (len - slen);
            const char_type * sbuf = s.pchunk->buf;
            for (size_type j = 0; j < slen; j++) {
                if (buf[j] != sbuf[j]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }



  protected:
    /// member variables must follow in the same order as in string for reinterpret cast
    chunk_t * pchunk {nullptr};
};

/// writable copy of string (guaranteed to be empty or have reference counter == 1
/// can only be passed by reference created from string
/// allows modification without reference counting checks
template <typename char_type, typename refcounter_type = std::atomic_int>
class string_wr : public string_ro<char_type, refcounter_type> {
public:
    // typedefs for STL compatibility
    typedef char_type             value_type;      ///< character type
    typedef lUInt32               size_type;       ///< size type
    typedef lInt32                difference_type; ///< difference type
    typedef value_type *          pointer;         ///< pointer to char type
    typedef value_type &          reference;       ///< reference to char type
    typedef const value_type *    const_pointer;   ///< pointer to const char type
    typedef const value_type &    const_reference; ///< reference to const char type
    typedef string_ro<char_type, refcounter_type> ro_string_type; ///< readonly string type
    typedef string<char_type, refcounter_type> string_type; ///< normal shared string type

    // constant for not found / unspecified position
    static const size_type npos = -1;

private:
    using typename string_ro<char_type, refcounter_type>::chunk_t;
    using string_ro<char_type, refcounter_type>::pchunk;
public:
    using string_ro<char_type, refcounter_type>::empty;
    using string_ro<char_type, refcounter_type>::length;
    using string_ro<char_type, refcounter_type>::size;
    using string_ro<char_type, refcounter_type>::capacity;

    using string_ro<char_type, refcounter_type>::operator [];
    using string_ro<char_type, refcounter_type>::at;
    using string_ro<char_type, refcounter_type>::c_str;

    /// disabled: always created as a synonim of the string with the same template parameters
    string_wr() {}
    /// don't free pchunk - it's owned by string
    ~string_wr() {}
public:

    /// return C-style null-terminated string pointer
    const char_type * data() const noexcept {
        return c_str();
    }

    /// return modifable C-style null-terminated string pointer; for empty string returns pointer to fake empty z-string buffer
    char_type * data() noexcept {
        if (pchunk == nullptr) {
            return reinterpret_cast<char_type *>(&fake_null_buffer_32);
        } else {
            // enforce null-termination
            const_cast<chunk_t*>(pchunk)->buf[pchunk->len] = 0;
            return pchunk->buf;
        }
    }

    /// sets value to null string, freeing buffer
    void clear() noexcept {
        if (pchunk != nullptr) {
            // use free instead of release because we own this string and ref count should be 1 anyway
            chunk_t::free(pchunk);
            pchunk = nullptr;
        }
    }

    /// resets length to zero, preparing to modification with reserved size
    string_wr&  reset(size_type size) noexcept {
        if (pchunk == nullptr) {
            // buffer is allocated and has capacity at least size
            pchunk = chunk_t::alloc(size);
        } else {
            // this string is non-empty
            if (pchunk->size >= size) {
                // this is already our own string with enough capacity -- just reset length
                pchunk->len = 0;
            } else {
                // this string has not enough capacity for size, clear and alloc new
                chunk_t::free(pchunk);
                pchunk = chunk_t::alloc(size);
            }
        }
        return *this;
    }

    /// ensure that buffer has at least size capacity
    string_wr& reserve(size_type size) noexcept {
        if (size == 0) {
            // reserve(0) is a non-binding request, just treat this empty string as owned
            return *this;
        }
        if (pchunk != nullptr) {
            // already own buffer
            if (pchunk->size < size) {
                // only if size is not enough, create a bigger buffer copy
                chunk_t * tmp = pchunk->duplicate(size);
                chunk_t::free(pchunk);
                pchunk = tmp;
            }
        } else {
            // create new allocation of requested size
            pchunk = chunk_t::alloc(size);
            pchunk->len = 0;
            pchunk->buf[0] = 0;
        }
        return *this;
    }

    /// compact buffer if possible -- free unused buffer space; returned reference is a normal string
    string_type& pack() noexcept {
        if (pchunk) {
            size_type new_size = chunk_t::alignSize(pchunk->len);
            if (pchunk->size > new_size) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
                chunk_t::free(pchunk);
                pchunk = tmp;
            }
        }
        return *reinterpret_cast<string_type*>(this);
    }

    /// move assignment - shared string
    string_wr& assign(ro_string_type&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (this != &s) {
            if (pchunk != nullptr) {
                chunk_t::free(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
            // ensure we have owned copy of source data
            if (pchunk->isShared()) {
                chunk_t * tmp = pchunk->duplicate();
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// move assignment - shared string
    string_wr& assign(string_wr&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (this != &s) {
            if (pchunk != nullptr) {
                chunk_t::free(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
        }
        return *this;
    }

    /// copy assignment
    string_wr& assign(const ro_string_type& s) noexcept {
        LS_COUNT_COPY_ASSIGN
        if (&s == this || pchunk == s.pchunk) {
            // safe self assignment: do nothing
            return *this;
        }
        if (pchunk != nullptr) {
            // ignore self-assignment
            chunk_t::free(pchunk);
            pchunk = s.pchunk;
            if (pchunk) {
                // assigned non-empty string
                intrusive_ptr_add_ref(pchunk);
            }
        } else {
            // this string is null
            pchunk = s.pchunk;
            if (pchunk) {
                // assigned non-empty string
                intrusive_ptr_add_ref(pchunk);
            }
        }
        return *this;
    }

    /// fragment assignment; correctly covers self-assignment; doesn't check bounds
    string_wr& assign(const ro_string_type&s, size_type offset, size_type count) noexcept {
        if (s.pchunk && offset < s.pchunk->len) {
            size_type avail = s.pchunk->len - offset;
            if (count > avail) count = avail;
            chunk_t* tmp = chunk_t::alloc(s.pchunk->buf + offset, count, count);
            if (pchunk) {
                chunk_t::free(pchunk);
            }
            pchunk = tmp;
        } else {
            clear();
        }
        return *this;
    }

    /// assign from z-terminated string
    string_wr& operator = (const char_type * s) noexcept {
        return assign(s);
    }

    /// assign from z-terminated string
    string_wr& assign(const char_type * s) noexcept {
        if (s == nullptr || !*s) {
            clear();
        } else {
            size_type count = str_len<char_type, size_type>(s);
            if (pchunk != nullptr && pchunk->size >= count && pchunk->size / 2 < count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    chunk_t::free(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// assign from char pointer and char count
    string_wr& assign(const char_type * s, size_type count) noexcept {
        if (s == nullptr || !*s || count == 0) {
            clear();
        } else {
            if (pchunk != nullptr && pchunk->size >= count && pchunk->size / 2 < count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    chunk_t::free(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// convert all characters of string to uppercase
    string_wr& uppercase() noexcept {
        if (pchunk) {
            lStr_uppercase(pchunk->buf, pchunk->len);
        }
        return *this;
    }

    /// convert all characters of string to lowercase
    string_wr& lowercase() noexcept {
        if (pchunk) {
            lStr_lowercase(pchunk->buf, pchunk->len);
        }
        return *this;
    }

    /// appends decimal string representation of integer value
    string_wr& appendDecimal(lInt64 n) noexcept {
        char_type buf[24];
        int i=0;
        int negative = 0;
        if (n==0) {
            return append(1, '0');
        } else if (n<0)
        {
            negative = 1;
            n = -n;
        }
        for ( ; n; n/=10 )
        {
            buf[i++] = '0' + (n % 10);
        }
        reserve(length() + i + negative);
        if (negative)
            append(1, '-');
        for (int j=i-1; j>=0; j--)
            append(1, buf[j]);
        return *this;
    }

    /// appends hex string representation of integer value, no leading zeroes
    string_wr& appendHex(lUInt64 n) noexcept {
        if (n == 0) {
            return append(1, '0');
        }
        reserve(length() + 16);
        bool foundNz = false;
        for (int i=0; i<16; i++) {
            int digit = (n >> 60) & 0x0F;
            if (digit) {
                foundNz = true;
            }
            if (foundNz) {
                append(1, (static_cast<char_type>("0123456789abcdef"[digit])) & 0xFF);
            }
            n <<= 4;
        }
        return *this;
    }

    /// append fragment from null-terminated string; no validation of input string is performed
    string_wr& append(const char_type* s, size_type count) noexcept {
        if (count) {
            if (!pchunk) {
                pchunk = chunk_t::alloc(s, count, count);
            } else {
                size_type new_len = pchunk->len + count;
                if (pchunk->size < new_len) {
                    // need new buffer: either shared or not enough capacity
                    // if s points to our own buffer, save data before releasing
                    bool self_append = (s == pchunk->buf);
                    chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                    if (self_append) {
                        // s was our old buffer, now freed; copy from the new tmp which has the old data
                        std::memcpy(tmp->buf + tmp->len, tmp->buf, sizeof(char_type) * count);
                    } else {
                        std::memcpy(tmp->buf + tmp->len, s, sizeof(char_type) * count);
                    }
                    tmp->len = new_len;
                    tmp->buf[new_len] = 0;
                    chunk_t::free(pchunk);
                    pchunk = tmp;
                } else {
                    std::memmove(pchunk->buf + pchunk->len, s, sizeof(char_type) * count);
                    pchunk->len = new_len;
                    pchunk->buf[new_len] = 0;
                }
            }
        }
        return *this;
    }

    /// append null-terminated string
    string_wr& append(const char_type* s) noexcept {
        if (!s || !*s) {
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        if (count) {
            append(s, count);
        }
        return *this;
    }

    /// append another string
    string_wr& append(const string_wr& s) noexcept {
        if (!s.empty()) {
            return append(s.pchunk->buf, s.pchunk->len);
        }
        return *this;
    }

    /// append fragment of another string
    string_wr& append(const string_wr& s, size_type offset, size_type count) noexcept {
        if (!s.empty() && offset < s.pchunk->len) {
            if (offset + count > s.pchunk->len) {
                count = s.pchunk->len - offset;
            }
            return append(s.pchunk->buf + offset, count);
        }
        return *this;
    }

    /// append one or more characters
    string_wr& append(size_type count, char_type c) noexcept {
        if (pchunk) {
            size_type new_len = pchunk->len + count;
            if (pchunk->size < new_len) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pchunk->len + i] = c;
                }
                tmp->len = new_len;
                tmp->buf[new_len] = 0;
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // own buffer with enough capacity
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pchunk->len + i] = c;
                }
                pchunk->len = new_len;
                pchunk->buf[new_len] = 0;
            }
        } else {
            // appending to empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        }
        return *this;
    }

    /// append single character
    string_wr& operator << (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string_wr& operator << (const char_type * str) noexcept { return append(str); }
    /// append string
    string_wr& operator << (const string_wr & str) noexcept { return append(str); }
    /// append decimal number
    string_wr& operator << (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string_wr& operator << (const fmt::hex v) noexcept { return appendHex(v.get()); }

    /// append single character
    string_wr& operator += (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string_wr& operator += (const char_type * str) noexcept { return append(str); }
    /// append string
    string_wr& operator += (const string_wr & str) noexcept { return append(str); }
    /// append decimal number
    string_wr& operator += (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string_wr& operator += (const fmt::hex v) noexcept { return appendHex(v.get()); }


    /// insert from char* and size
    string_wr& insert(size_type pos, const char_type* s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(s, count, count);
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// insert from null-terminated cont char*
    string_wr& insert(size_type pos, const char_type* s) noexcept {
        if (!s || !*s) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s, str_len<char_type, size_type>(s));
    }

    /// insert from string (no self-insert protection)
    string_wr& insert(size_type pos, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s.pchunk->buf, s.pchunk->len);
    }

    /// insert one or more characters
    string_wr& insert(size_type pos, size_type count, char_type c) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const char_type * s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (empty()) {
            return assign(s, count);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, s, count);
            }
            if (pos == old_len) {
                // use append
                return append(s, count);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // replace this string with new
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, size_type count, char_type c) noexcept {
        if (!count) {
            // nothing to insert
            return *this;
        }
        if (empty()) {
            return append(count, c);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, count, c);
            }
            if (pos == old_len) {
                // use append
                return append(count, c);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // replace this string with new
                chunk_t::free(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const char_type * s) noexcept {
        if (!s || !*s) {
            // nothing to insert
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        return replace(pos, n, s, count);
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        return replace(pos, n, s.pchunk->buf, s.pchunk->len);
    }

    /// replace part of string with count chars from char*
    string_wr& replace(size_type pos, size_type n, const ro_string_type& s, size_type offset, size_type count) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        size_type s_len = s.pchunk->len;
        if (offset >= s_len) {
            // source data indexes outside source string bounds
            return *this;
        }
        // ensure source range is inside string - truncate if needed
        if (offset + count > s_len) {
            count = s_len - offset;
        }
        return replace(pos, n, s.pchunk->buf + offset, count);
    }

    /// replace all occurences of character replaceWhat with character replaceTo
    string_wr& replace(char_type replaceWhat, char_type replaceTo) noexcept {
        if (empty()) {
            // nothing to replace
            return *this;
        }
        size_type len = pchunk->len;
        char_type * buf = pchunk->buf;
        for (size_type i = 0; i < len; i++) {
            if (buf[i] == replaceWhat) {
                // first match found: replace is needed
                // own buffer : replace in-place
                buf[i++] = replaceTo;
                while (i < len) {
                    if (buf[i] == replaceWhat) {
                        buf[i] = replaceTo;
                    }
                    i++;
                }
                break;
            }
        }
        return *this;
    }

    /// erases fragment from string; if requested fragment exceeds string bounds, it's size is truncated
    string_wr& erase(size_type offset, size_type count) noexcept {
        if (pchunk && offset < pchunk->len && count) {
            if (offset + count > pchunk->len) {
                // truncate requested erased fragment size to not exceed bounds
                count = pchunk->len - offset;
            }
            size_type new_len = pchunk->len - count;
            size_type tail_start = offset + count;
            size_type tail_len = pchunk->len - tail_start;
            // erase inplace
            if (tail_len) {
                std::memmove(pchunk->buf + offset, pchunk->buf + tail_start, sizeof(char_type) * tail_len);
            }
            pchunk->len = new_len;
            pchunk->buf[pchunk->len] = 0;
        }
        return *this;
    }
};

template <typename char_type, typename refcounter_type = std::atomic_int>
class string : public string_ro<char_type, refcounter_type> {
    friend class string_wr<char_type, refcounter_type>;
public:
    // typedefs for STL compatibility
    typedef char_type             value_type;      ///< character type
    typedef lUInt32               size_type;       ///< size type
    typedef lInt32                difference_type; ///< difference type
    typedef value_type *          pointer;         ///< pointer to char type
    typedef value_type &          reference;       ///< reference to char type
    typedef const value_type *    const_pointer;   ///< pointer to const char type
    typedef const value_type &    const_reference; ///< reference to const char type

    // constant for not found / unspecified position
    static const size_type npos = -1;

    // empty string constant
    //static const string empty_str;
    //static string constexpr empty_str = nullptr;
    inline static const string empty_str{};

    // COW types
    typedef string_wr<char_type, refcounter_type> writable_string; ///< writable (owned) string
    typedef string_wr<char_type, refcounter_type>& writable_ref;   ///< writable (owned) string reference
    typedef string_ro<char_type, refcounter_type> ro_string_type; ///< readonly string type

private:

  using typename string_ro<char_type, refcounter_type>::chunk_t;
  using string_ro<char_type, refcounter_type>::pchunk;

public:
  using string_ro<char_type, refcounter_type>::empty;
  using string_ro<char_type, refcounter_type>::length;
  using string_ro<char_type, refcounter_type>::size;
  using string_ro<char_type, refcounter_type>::capacity;

  using string_ro<char_type, refcounter_type>::operator [];
  using string_ro<char_type, refcounter_type>::at;
  using string_ro<char_type, refcounter_type>::c_str;
  //using string_ro<char_type, refcounter_type>::data;


public:
    string() noexcept = default;
    string(const char_type* s, size_type count, size_type reserved) noexcept {
        if (count == 0 || s == nullptr || s[0] == 0) {
            pchunk = nullptr;
        } else {
            pchunk = chunk_t::alloc(s, count, reserved);
        }
    }
    string(const char_type* s, size_type count) noexcept {
        if (count == 0 || s == nullptr || s[0] == 0) {
            pchunk = nullptr;
        } else {
            pchunk = chunk_t::alloc(s, count, count);
        }
    }
    explicit string(const char_type* s) noexcept {
        if (s != nullptr && *s) {
            pchunk = chunk_t::alloc(s, str_len<char_type, size_type>(s), 0);
        } else {
            pchunk = nullptr;
        }
    }
    /// copy constructor
    string(const string&s) noexcept {
        LS_COUNT_COPY_CONSTR
        pchunk = s.pchunk;
        intrusive_ptr_add_ref_checknull(pchunk);
    }
    /// constructor of empty buffer with reserved size
    string(size_type size) noexcept {
        pchunk = chunk_t::alloc(size);
    }
    /// move constructor
    string(string&&s ) noexcept {
        LS_COUNT_MOVE_CONSTR
        pchunk = s.pchunk;
        s.pchunk = nullptr;
    }
    /// fragment constructor
    string(const string&s, size_type offset, size_type count) noexcept {
        if (s.pchunk && offset < s.pchunk->len) {
            size_type avail = s.pchunk->len - offset;
            if (count > avail) count = avail;
            pchunk = chunk_t::alloc(s.pchunk->buf + offset, count, count);
        } else {
            pchunk = nullptr;
        }
    }
    /// destructor - frees reference
    ~string() noexcept {
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
        }
    }

    /// move assignment
    string& operator = (string&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (this != &s) {
            if (pchunk != nullptr) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
        }
        return *this;
    }
    /// copy assignment
    string& operator = (const string& s) noexcept {
        return assign(s);
    }

    /// move assignment
    string& assign(string&& s) noexcept {
        LS_COUNT_MOVE_ASSIGN
        if (this != &s) {
            if (pchunk != nullptr) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = s.pchunk;
            s.pchunk = nullptr;
        }
        return *this;
    }

    /// copy assignment
    string& assign(const string& s) noexcept {
        LS_COUNT_COPY_ASSIGN
        if (&s == this || pchunk == s.pchunk) {
            // safe self assignment: do nothing
            return *this;
        }
        if (pchunk != nullptr) {
            // ignore self-assignment
            intrusive_ptr_release(pchunk);
            pchunk = s.pchunk;
            if (pchunk) {
                // assigned non-empty string
                intrusive_ptr_add_ref(pchunk);
            }
        } else {
            // this string is null
            pchunk = s.pchunk;
            if (pchunk) {
                // assigned non-empty string
                intrusive_ptr_add_ref(pchunk);
            }
        }
        return *this;
    }

    /// fragment assignment; correctly covers self-assignment; doesn't check bounds
    string& assign(const ro_string_type&s, size_type offset, size_type count) noexcept {
        if (s.pchunk && offset < s.pchunk->len) {
            size_type avail = s.pchunk->len - offset;
            if (count > avail) count = avail;
            chunk_t* tmp = chunk_t::alloc(s.pchunk->buf + offset, count, count);
            if (pchunk) {
                intrusive_ptr_release(pchunk);
            }
            pchunk = tmp;
        } else {
            clear();
        }
        return *this;
    }

    /// assign from z-terminated string
    string& operator = (const char_type * s) noexcept {
        return assign(s);
    }

    /// assign from z-terminated string
    string& assign(const char_type * s) noexcept {
        if (s == nullptr || !*s) {
            clear();
        } else {
            size_type count = str_len<char_type, size_type>(s);
            if (pchunk != nullptr && pchunk->isOwn() && pchunk->size >= count && pchunk->size / 2 < count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    intrusive_ptr_release(pchunk);
                }
                pchunk = tmp;
            }
        }
    }

    /// assign from char pointer and char count
    string& assign(const char_type * s, size_type count) noexcept {
        if (s == nullptr || !*s || count == 0) {
            clear();
        } else {
            if (pchunk != nullptr && pchunk->isOwn() && pchunk->size >= count && pchunk->size / 2 < count) {
                // reuse existing buffer
                std::memcpy(pchunk->buf, s, count * sizeof(char_type));
                pchunk->len = count;
            } else {
                // create new buffer
                chunk_t * tmp = chunk_t::alloc(s, count, count);
                if (pchunk) {
                    intrusive_ptr_release(pchunk);
                }
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// sets value to null string, freeing buffer
    void clear() noexcept {
        if (pchunk != nullptr) {
            intrusive_ptr_release(pchunk);
            pchunk = nullptr;
        }
    }

    /// resets length to zero, preparing to modification with reserved size
    writable_string&  reset(size_type size) noexcept {
        if (pchunk == nullptr) {
            // buffer is allocated and has capacity at least size
            pchunk = chunk_t::alloc(size);
        } else {
            // this string is non-empty
            if (pchunk->isOwn() && pchunk->size >= size) {
                // this is already our own string with enough capacity -- just reset length
                pchunk->len = 0;
            } else {
                // this string has not enough capacity for size, clear and alloc new
                intrusive_ptr_release(pchunk);
                pchunk = chunk_t::alloc(size);
            }
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// ensure that this string owns buffer and it has at least size capacity
    writable_string& reserve(size_type size) noexcept {
        if (size == 0) {
            // reserve(0) is a non-binding request, just treat this empty string as owned
            return *reinterpret_cast<writable_string*>(this);
        }
        if (pchunk != nullptr) {
            if (pchunk->isOwn()) {
                // already own buffer
                if (pchunk->size < size) {
                    // only if size is not enough, create a bigger buffer copy
                    chunk_t * tmp = chunk_t::alloc(size);
                    tmp->len = pchunk->len;
                    std::memcpy(tmp->buf, pchunk->buf, pchunk->len);
                    tmp->buf[tmp->len] = 0;
                    chunk_t::free(pchunk);
                    pchunk = tmp;
                }
            } else {
                // we don't own the string, need to create a copy
                if (size < pchunk->len) {
                    // when requested size is smaller than existing string len, ensure we will not loss chars
                    size = pchunk->len;
                }
                chunk_t * tmp = chunk_t::alloc(size);
                tmp->len = pchunk->len;
                std::memcpy(tmp->buf, pchunk->buf, pchunk->len);
                tmp->buf[tmp->len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        } else {
            // create new allocation of requested size
            pchunk = chunk_t::alloc(size);
            pchunk->len = 0;
            pchunk->buf[0] = 0;
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// get writable reference for own string - create a copy only if refcount > 1
    writable_string& writableRef() {
        if (pchunk && pchunk->isShared()) {
            // create a copy with refcount==1
            chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
            intrusive_ptr_release(pchunk);
            pchunk = tmp;
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// get writable reference for own string - create a copy if refcount > 1
    /// if own copy has to be created, reserve size
    writable_string& writableRef(size_type size) {
        if (pchunk && pchunk->isShared()) {
            // create a copy with refcount==1
            chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, size);
            intrusive_ptr_release(pchunk);
            pchunk = tmp;
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// resize string to specified length, filling new positions with e if expanded
    writable_string& resize(size_type size, char_type e = 0) noexcept {
        if (pchunk == nullptr) {
            // empty string: allocate new buffer
            pchunk = chunk_t::alloc(size);
            for (size_type i = 0; i < size; i++)
                pchunk->buf[i] = e;
            pchunk->len = size;
            pchunk->buf[size] = 0;
        } else {
            // non-empty string
            size_type oldlen = pchunk->len;
            if (pchunk->isOwn() && pchunk->size >= size) {
                // own buffer with enough capacity: just adjust length
                if (size > oldlen) {
                    for (size_type i = oldlen; i < size; i++)
                        pchunk->buf[i] = e;
                }
                pchunk->len = size;
                pchunk->buf[size] = 0;
            } else {
                // need new buffer
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, oldlen < size ? oldlen : size, size);
                if (size > oldlen) {
                    for (size_type i = oldlen; i < size; i++)
                        tmp->buf[i] = e;
                }
                tmp->len = size;
                tmp->buf[size] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return *reinterpret_cast<writable_string*>(this);
    }

    /// convert all characters of string to uppercase
    string& uppercase() noexcept {
        if (pchunk) {
            writableRef().uppercase();
        }
        return *this;
    }

    /// convert all characters of string to lowercase
    string& lowercase() noexcept {
        if (pchunk) {
            writableRef().lowercase();
        }
        return *this;
    }

    /// erases fragment from string; if requested fragment exceeds string bounds, it's size is truncated
    string& erase(size_type offset, size_type count) noexcept {
        if (pchunk && offset < pchunk->len && count) {
            if (offset + count > pchunk->len) {
                // truncate requested erased fragment size to not exceed bounds
                count = pchunk->len - offset;
            }
            size_type new_len = pchunk->len - count;
            size_type tail_start = offset + count;
            size_type tail_len = pchunk->len - tail_start;
            if (pchunk->isOwn()) {
                // erase inplace
                if (tail_len) {
                    std::memmove(pchunk->buf + offset, pchunk->buf + tail_start, sizeof(char_type) * tail_len);
                }
            } else {
                // create a copy with fragment erased
                chunk_t * tmp = chunk_t::alloc(new_len);
                if (offset) {
                    // something left in beginning of string
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * offset);
                }
                if (tail_len) {
                    std::memcpy(tmp->buf + offset, pchunk->buf + tail_start, sizeof(char_type) * tail_len);
                }
                // free old ref and use copy
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
            pchunk->len = new_len;
            pchunk->buf[pchunk->len] = 0;
        }
        return *this;
    }

    /// appends decimal string representation of integer value
    string& appendDecimal(lInt64 n) noexcept {
        char_type buf[24];
        int i=0;
        int negative = 0;
        if (n==0) {
            return append(1, '0');
        } else if (n<0)
        {
            negative = 1;
            n = -n;
        }
        for ( ; n; n/=10 )
        {
            buf[i++] = '0' + (n % 10);
        }
        reserve(length() + i + negative);
        if (negative)
            append(1, '-');
        for (int j=i-1; j>=0; j--)
            append(1, buf[j]);
        return *this;
    }

    /// appends hex string representation of integer value, no leading zeroes
    string& appendHex(lUInt64 n) noexcept {
        if (n == 0) {
            return append(1, '0');
        }
        reserve(length() + 16);
        bool foundNz = false;
        for (int i=0; i<16; i++) {
            int digit = (n >> 60) & 0x0F;
            if (digit) {
                foundNz = true;
            }
            if (foundNz) {
                append(1, (static_cast<char_type>("0123456789abcdef"[digit])) & 0xFF);
            }
            n <<= 4;
        }
        return *this;
    }

    /// append fragment from null-terminated string; no validation of input string is performed
    string& append(const char_type* s, size_type count) noexcept {
        if (count) {
            if (!pchunk) {
                pchunk = chunk_t::alloc(s, count, count);
            } else {
                size_type new_len = pchunk->len + count;
                if (pchunk->isShared() || pchunk->size < new_len) {
                    // need new buffer: either shared or not enough capacity
                    // if s points to our own buffer, save data before releasing
                    bool self_append = (s == pchunk->buf);
                    chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                    if (self_append) {
                        // s was our old buffer, now freed; copy from the new tmp which has the old data
                        std::memcpy(tmp->buf + tmp->len, tmp->buf, sizeof(char_type) * count);
                    } else {
                        std::memcpy(tmp->buf + tmp->len, s, sizeof(char_type) * count);
                    }
                    tmp->len = new_len;
                    tmp->buf[new_len] = 0;
                    intrusive_ptr_release(pchunk);
                    pchunk = tmp;
                } else {
                    std::memmove(pchunk->buf + pchunk->len, s, sizeof(char_type) * count);
                    pchunk->len = new_len;
                    pchunk->buf[new_len] = 0;
                }
            }
        }
        return *this;
    }

    /// append null-terminated string
    string& append(const char_type* s) noexcept {
        if (!s || !*s) {
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        if (count) {
            append(s, count);
        }
        return *this;
    }

    /// append another string
    string& append(const ro_string_type& s) noexcept {
        if (!s.empty()) {
            return append(s.pchunk->buf, s.pchunk->len);
        }
        return *this;
    }

    /// append fragment of another string
    string& append(const ro_string_type& s, size_type offset, size_type count) noexcept {
        if (!s.empty() && offset < s.pchunk->len) {
            if (offset + count > s.pchunk->len) {
                count = s.pchunk->len - offset;
            }
            return append(s.pchunk->buf + offset, count);
        }
        return *this;
    }

    /// append one or more characters
    string& append(size_type count, char_type c) noexcept {
        if (pchunk) {
            size_type new_len = pchunk->len + count;
            if (pchunk->isShared() || pchunk->size < new_len) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, new_len);
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pchunk->len + i] = c;
                }
                tmp->len = new_len;
                tmp->buf[new_len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // own buffer with enough capacity
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pchunk->len + i] = c;
                }
                pchunk->len = new_len;
                pchunk->buf[new_len] = 0;
            }
        } else {
            // appending to empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        }
        return *this;
    }

    /// append single character
    string& operator << (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string& operator << (const char_type * str) noexcept { return append(str); }
    /// append string
    string& operator << (const string & str) noexcept { return append(str); }
    /// append decimal number
    string& operator << (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string& operator << (const fmt::hex v) noexcept { return appendHex(v.get()); }

    /// append single character
    string& operator += (char_type ch) noexcept { return append(1, ch); }
    /// append C-string
    string& operator += (const char_type * str) noexcept { return append(str); }
    /// append string
    string& operator += (const string & str) noexcept { return append(str); }
    /// append decimal number
    string& operator += (const fmt::decimal v) noexcept { return appendDecimal(v.get()); }
    /// append hex number
    string& operator += (const fmt::hex v) noexcept { return appendHex(v.get()); }

    /// insert from char* and size
    string& insert(size_type pos, const char_type* s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(s, count, count);
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (pchunk->isShared() || new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// insert from null-terminated cont char*
    string& insert(size_type pos, const char_type* s) noexcept {
        if (!s || !*s) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s, str_len<char_type, size_type>(s));
    }

    /// insert from string (no self-insert protection)
    string& insert(size_type pos, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // attempt inserting empty string
            return *this;
        }
        return insert(pos, s.pchunk->buf, s.pchunk->len);
    }

    /// insert one or more characters
    string& insert(size_type pos, size_type count, char_type c) noexcept {
        if (!count) {
            return *this;
        }
        if (!pchunk) {
            // insert into empty string
            pchunk = chunk_t::alloc(count);
            for (size_type i = 0; i < count; i++) {
                pchunk->buf[i] = c;
            }
            pchunk->len = count;
            pchunk->buf[count] = 0;
        } else {
            if (pos > pchunk->len) {
                pos = pchunk->len;
            }
            size_type new_len = pchunk->len + count;
            size_type tail_len = pchunk->len - pos;
            if (pchunk->isShared() || new_len > pchunk->size) {
                // create copy
                chunk_t* tmp = chunk_t::alloc(new_len);
                if (pos > 0) {
                    // copy head
                    std::memcpy(tmp->buf, pchunk->buf, sizeof(char_type) * pos);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // copy tail if needed
                if (tail_len) {
                    // copy tail
                    std::memcpy(tmp->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                tmp->len = new_len;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // insert in-place
                if (tail_len) {
                    // move tail by count chars
                    std::memmove(pchunk->buf + pos + count, pchunk->buf + pos, sizeof(char_type) * tail_len);
                }
                // copy inserted content
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
        }
        return *this;
    }

    /// compact buffer if possible -- free unused buffer space
    string& pack() noexcept {
        if (pchunk) {
            size_type new_size = chunk_t::alignSize(pchunk->len);
            if (pchunk->size > new_size) {
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return *this;
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const char_type * s, size_type count) noexcept {
        if (!count) {
            return *this;
        }
        if (empty()) {
            return assign(s, count);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, s, count);
            }
            if (pos == old_len) {
                // use append
                return append(s, count);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size || pchunk->isShared()) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(tmp->buf + pos, s, sizeof(char_type) * count);
                // replace this string with new
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                std::memcpy(pchunk->buf + pos, s, sizeof(char_type) * count);
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, size_type count, char_type c) noexcept {
        if (!count) {
            // nothing to insert
            return *this;
        }
        if (empty()) {
            return append(count, c);
        } else {
            // need to modify
            size_type old_len = pchunk->len;
            if (pos > old_len) {
                pos = old_len;
            }
            if (pos + n > old_len) {
                n = old_len - pos;
            }
            if (!n) {
                // delegate to insert
                return insert(pos, count, c);
            }
            if (pos == old_len) {
                // use append
                return append(count, c);
            }
            size_type new_len = old_len + count - n;
            size_type tail_len = old_len - (pos + n);
            if (new_len > pchunk->size || pchunk->isShared()) {
                // need to create copy -- no space or has other refs
                chunk_t * tmp = chunk_t::alloc(pchunk->buf, new_len, new_len);
                // move tail to the end of buffer
                std::memmove(tmp->buf + new_len - tail_len, tmp->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    tmp->buf[pos + i] = c;
                }
                // replace this string with new
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            } else {
                // may edit in-place
                // move tail to the end of buffer
                std::memmove(pchunk->buf + new_len - tail_len, pchunk->buf + old_len - tail_len, sizeof(char_type) * tail_len);
                // fill gap with inserted string
                for (size_type i = 0; i < count; i++) {
                    pchunk->buf[pos + i] = c;
                }
            }
            // update length
            pchunk->len = new_len;
            pchunk->buf[new_len] = 0;
            return *this;
        }
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const char_type * s) noexcept {
        if (!s || !*s) {
            // nothing to insert
            return *this;
        }
        size_type count = str_len<char_type, size_type>(s);
        return replace(pos, n, s, count);
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const ro_string_type& s) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        return replace(pos, n, s.pchunk->buf, s.pchunk->len);
    }

    /// replace part of string with count chars from char*
    string& replace(size_type pos, size_type n, const ro_string_type& s, size_type offset, size_type count) noexcept {
        if (s.empty()) {
            // nothing to insert
            return *this;
        }
        size_type s_len = s.pchunk->len;
        if (offset >= s_len) {
            // source data indexes outside source string bounds
            return *this;
        }
        // ensure source range is inside string - truncate if needed
        if (offset + count > s_len) {
            count = s_len - offset;
        }
        return replace(pos, n, s.pchunk->buf + offset, count);
    }

    /// replace all occurences of character replaceWhat with character replaceTo
    string& replace(char_type replaceWhat, char_type replaceTo) noexcept {
        if (empty()) {
            // nothing to replace
            return *this;
        }
        size_type len = pchunk->len;
        char_type * buf = pchunk->buf;
        for (size_type i = 0; i < len; i++) {
            if (buf[i] == replaceWhat) {
                // first match found: replace is needed
                if (pchunk->isOwn()) {
                    // own buffer : replace in-place
                    buf[i++] = replaceTo;
                    while (i < len) {
                        if (buf[i] == replaceWhat) {
                            buf[i] = replaceTo;
                        }
                        i++;
                    }
                } else {
                    // need to create a copy
                    chunk_t * tmp = chunk_t::alloc(pchunk->buf, pchunk->len, pchunk->len);
                    // switch to new buffer
                    buf = tmp->buf;
                    // replace first char
                    buf[i++] = replaceTo;
                    while (i < len) {
                        if (buf[i] == replaceWhat) {
                            buf[i] = replaceTo;
                        }
                        i++;
                    }
                    intrusive_ptr_release(pchunk);
                    pchunk = tmp;
                }
                break;
            }
        }
        return *this;
    }

    /// return C-style null-terminated string pointer
    const char_type * data() const noexcept {
        return c_str();
    }

    /// return modifable C-style null-terminated string pointer; for empty string returns pointer to fake empty z-string buffer
    char_type * data() noexcept {
        if (pchunk == nullptr) {
            return reinterpret_cast<char_type *>(&fake_null_buffer_32);
        } else {
            lock(pchunk->len);
            // enforce null-termination
            const_cast<chunk_t*>(pchunk)->buf[pchunk->len] = 0;
            return pchunk->buf;
        }
    }

    /// ensures that reference count is 1; if string is null or we own it, do nothing
    /// only used from data() ? what is a better place for it?
    void lock( size_type newsize ) noexcept {
        if (pchunk) {
            // string is not null
            if (pchunk->isShared()) {
                // if we don't own string, make a copy
                if (newsize < pchunk->len) {
                    // ensure all chars from old string were copied
                    newsize = pchunk->len;
                }
                chunk_t * tmp = chunk_t::alloc(newsize);
                tmp->len = pchunk->len;
                std::memcpy(tmp->buf, pchunk->buf, tmp->len);
                tmp->buf[tmp->len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
    }



    /// returns pointer to modifable string buffer
    char_type * modify() noexcept {
        if (!pchunk) {
            // allocate small buffer
            pchunk = chunk_t::alloc(8);
        } else {
            // non-empty string
            // string is not null
            if (pchunk->isShared()) {
                chunk_t * tmp = chunk_t::alloc(pchunk->len);
                tmp->len = pchunk->len;
                std::memcpy(tmp->buf, pchunk->buf, tmp->len);
                tmp->buf[tmp->len] = 0;
                intrusive_ptr_release(pchunk);
                pchunk = tmp;
            }
        }
        return pchunk->buf;
    }

    /// returns substring starting with position pos, and specified count of chars (pass npos to copy till end of string)
    string substr(size_type pos, size_type count = npos) const noexcept {
        string tmp;
        if (!pchunk || pos >= pchunk->len) {
            return tmp;
        }
        size_type sz = pchunk->len - pos;
        if (count > sz) {
            count = sz;
        }
        tmp.pchunk = chunk_t::alloc(pchunk->buf + pos, count, count);
        return tmp;
    }

    /// swaps content of two strings, by swapping of pointers
    void swap(string & s) {
        chunk_t * tmp = pchunk;
        pchunk = s.pchunk;
        s.pchunk = tmp;
    }

private:
    //chunk_t * pchunk {nullptr};
};

// template<typename char_type, typename refcounter_type>
// const string<char_type, refcounter_type> string<char_type, refcounter_type>::empty_str{};

typedef lstring_chunk_t<lChar8, lUInt32, std::atomic_int> lstring8_chunk_t;
typedef lstring_chunk_t<lChar16, lUInt32, std::atomic_int> lstring16_chunk_t;
typedef lstring_chunk_t<lChar32, lUInt32, std::atomic_int> lstring32_chunk_t;

typedef string<lChar8, std::atomic_int> lString8;
typedef string<lChar16, std::atomic_int> lString16;
typedef string<lChar32, std::atomic_int> lString32;


}


#endif // LVSTRING2_H
